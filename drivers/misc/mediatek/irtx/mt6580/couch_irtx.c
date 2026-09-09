/* SPDX-License-Identifier: GPL-2.0 */
/* HA100 MT6580 PWM IR transmitter. ABI/register setup follows MediaTek's
 * mt_irtx_pwm.c (SoCXin/MT6737, 89eaf42); bounded synchronous transfers,
 * serialized callers and DMA quiescence replace its fixed post-send delay.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/math64.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include "couch_irtx_complete.h"
#include <linux/wakelock.h>
#include <mt-plat/mt_pwm.h>
#include <mt-plat/mt_pwm_hal_pub.h>
#include <mach/mt_pwm_hal.h>

#define IRTX_SET_CARRIER _IOW('R', 0, unsigned int)
#define IRTX_GET_SOLUTION _IOR('R', 1, unsigned int)
#define IRTX_MAX_BYTES (128 * 1024)

struct couch_irtx {
	struct miscdevice misc;
	struct device *dev;
	struct mutex lock;
	struct wake_lock awake;
	struct pwm_spec_config pwm;
	bool invert;
};

/* Couch's solution-1 ABI uses carrier-scaled samples and a final duration
 * word. The old vendor one-microsecond driver is not compatible with it. */
struct ir_file {
	struct couch_irtx *ir;
	unsigned int carrier;
};

static int ir_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	struct ir_file *ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->ir = container_of(misc, struct couch_irtx, misc);
	ctx->carrier = 38000;
	file->private_data = ctx;
	return nonseekable_open(inode, file);
}

static int ir_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static long ir_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ir_file *ctx = file->private_data;
	unsigned int carrier;
	if (cmd == IRTX_GET_SOLUTION)
		return put_user(1, (unsigned int __user *)arg);
	if (cmd != IRTX_SET_CARRIER)
		return -ENOTTY;
	if (get_user(carrier, (unsigned int __user *)arg))
		return -EFAULT;
	if (carrier < 10000 || carrier > 100000)
		return -EINVAL;
	if (mutex_lock_interruptible(&ctx->ir->lock))
		return -ERESTARTSYS;
	ctx->carrier = carrier;
	mutex_unlock(&ctx->ir->lock);
	return 0;
}

static ssize_t ir_write(struct file *file, const char __user *buf,
			size_t count, loff_t *pos)
{
	struct ir_file *ctx = file->private_data;
	struct couch_irtx *ir = ctx->ir;
	dma_addr_t physical;
	u8 *wave;
	unsigned long deadline;
	unsigned int i, finish = ir->pwm.pwm_no * 2;
	u32 duration, clocks, actual_us;
	s32 sent_before = 0, sent;
	bool saw_zero = false;
	ktime_t started, setup_started;
	s64 first_complete_us = -1, elapsed_us, setup_us;
	size_t wave_bytes = count - sizeof(u32);
	int ret;

	/* BUF0_SIZE counts whole 32-bit words minus one. Reject padding ambiguity. */
	if (count < 8 || count > IRTX_MAX_BYTES || count % 4)
		return -EINVAL;
	if (mutex_lock_interruptible(&ir->lock))
		return -ERESTARTSYS;
	/* Bound both declared and actual DMA duration; never trust the trailer
	 * as a DMA length or timeout. Partial final words add at most 32 ticks. */
	if (copy_from_user(&duration, buf + wave_bytes, sizeof(duration))) {
		ret = -EFAULT;
		goto unlock;
	}
	clocks = DIV_ROUND_CLOSEST(26000000U, ctx->carrier * 3);
	actual_us = div_u64((u64)wave_bytes * 8 * clocks + 25, 26);
	if (!duration || duration > 1000000 || actual_us > 1001100) {
		ret = -EINVAL;
		goto unlock;
	}
	ir->pwm.PWM_MODE_MEMORY_REGS.HDURATION = clocks - 1;
	ir->pwm.PWM_MODE_MEMORY_REGS.LDURATION = clocks - 1;
	wave = dma_alloc_coherent(ir->dev, wave_bytes, &physical, GFP_KERNEL);
	if (!wave) {
		ret = -ENOMEM;
		goto unlock;
	}
	if (copy_from_user(wave, buf, wave_bytes)) {
		ret = -EFAULT;
		goto free;
	}
	if (ir->invert)
		for (i = 0; i < wave_bytes; i++)
			wave[i] = ~wave[i];

	wake_lock(&ir->awake);
	/* Clock-gated PWM MMIO can wedge the bus before a timeout can run. */
	mt_pwm_power_on(ir->pwm.pwm_no, ir->pwm.pmic_pad);
	mt_pwm_26M_clk_enable_hal(1);
	mt_set_intr_ack(finish);
	mt_set_intr_ack(finish + 1);
	ir->pwm.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = physical;
	ir->pwm.PWM_MODE_MEMORY_REGS.BUF0_SIZE = wave_bytes / 4 - 1;
	sent_before = mt_get_pwm_send_wavenum_hal(ir->pwm.pwm_no);
	saw_zero = sent_before == 0;
	setup_started = ktime_get();
	ret = pwm_set_spec_config(&ir->pwm);
	if (ret) {
		/* MTK HAL errors are not all Linux errno values. */
		ret = -EIO;
		goto stop;
	}
	/* Status remains zero with IRQs masked on HA100 even after SEND_WAVENUM
	 * reaches one. Poll that counter without enabling an unhandled IRQ.
	 * Start the minimum-time guard after setup: this intentionally waits a
	 * full frame even if the hardware already began during configuration. */
	started = ktime_get();
	setup_us = ktime_us_delta(started, setup_started);
	deadline = jiffies + msecs_to_jiffies(DIV_ROUND_UP(actual_us, 1000) + 50);
	for (;;) {
		if (mt_get_intr_status(finish + 1) > 0) {
			ret = -EIO;
			break;
		}
		sent = mt_get_pwm_send_wavenum_hal(ir->pwm.pwm_no);
		if (sent < 0 || sent > 1) {
			ret = -EIO;
			break;
		}
		elapsed_us = ktime_us_delta(ktime_get(), started);
		/* Record the first valid hardware completion independently of the
		 * conservative minimum-time guard. A retained prior count is ignored. */
		first_complete_us = couch_irtx_first_complete(sent, saw_zero,
						       first_complete_us, elapsed_us);
		if (couch_irtx_complete(sent, &saw_zero, elapsed_us, actual_us)) {
			dev_info(ir->dev, "TX timing carrier=%u bytes=%zu expected_us=%u setup_us=%lld first_complete_us=%lld guard_complete_us=%lld sent_before=%d\n",
				 ctx->carrier, wave_bytes, actual_us, setup_us,
				 first_complete_us, elapsed_us, sent_before);
			ret = count;
			break;
		}
		if (time_after_eq(jiffies, deadline)) {
			dev_err_ratelimited(ir->dev, "PWM transmit timed out\n");
			ret = -ETIMEDOUT;
			break;
		}
		usleep_range(500, 1000);
	}
stop:
	if (ret < 0) {
		dev_err(ir->dev, "TX result=%d carrier=%u clocks=%u waveform_bytes=%zu expected_us=%u sent_before=%d sent_after=%d observed_zero=%u\n",
			ret, ctx->carrier, clocks, wave_bytes, actual_us, sent_before,
			mt_get_pwm_send_wavenum_hal(ir->pwm.pwm_no), saw_zero);
		/* Observe before ack/disable while MMIO remains powered. Never turn
		 * on unhandled shared interrupts merely to diagnose a timeout. */
		mt_pwm_dump_channel_hal(ir->pwm.pwm_no);
	}
	/* Disable includes MTK's drain delay. No DMA mapping is freed while live. */
	mt_set_intr_ack(finish);
	mt_set_intr_ack(finish + 1);
	mt_pwm_disable(ir->pwm.pwm_no, ir->pwm.pmic_pad);
	wake_unlock(&ir->awake);
free:
	dma_free_coherent(ir->dev, wave_bytes, wave, physical);
unlock:
	mutex_unlock(&ir->lock);
	return ret;
}

static const struct file_operations ir_fops = {
	.owner = THIS_MODULE,
	.open = ir_open,
	.release = ir_release,
	.write = ir_write,
	.unlocked_ioctl = ir_ioctl,
	.llseek = no_llseek,
};

static int ir_probe(struct platform_device *pdev)
{
	struct couch_irtx *ir;
	struct device_node *node;
	struct platform_device *provider;
	bool ready;
	u32 channel, invert = 0;
	int ret;
	/* The vendor PWM API has global MMIO state populated by its probe. */
	node = of_find_compatible_node(NULL, NULL, "mediatek,PWM");
	if (!node)
		return -ENODEV;
	provider = of_find_device_by_node(node);
	of_node_put(node);
	if (!provider)
		return -EPROBE_DEFER;
	ready = platform_get_drvdata(provider) != NULL;
	put_device(&provider->dev);
	if (!ready)
		return -EPROBE_DEFER;
	if (of_property_read_u32(pdev->dev.of_node, "pwm_ch", &channel) || channel >= 5)
		return -EINVAL;
	of_property_read_u32(pdev->dev.of_node, "pwm_data_invert", &invert);
	ir = devm_kzalloc(&pdev->dev, sizeof(*ir), GFP_KERNEL);
	if (!ir)
		return -ENOMEM;
	ir->dev = &pdev->dev;
	ir->invert = !!invert;
	mutex_init(&ir->lock);
	ret = dma_coerce_mask_and_coherent(ir->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	ir->pwm.pwm_no = channel;
	ir->pwm.mode = PWM_MODE_MEMORY;
	ir->pwm.clk_div = CLK_DIV1;
	ir->pwm.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	ir->pwm.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
	ir->pwm.PWM_MODE_MEMORY_REGS.HDURATION = 25;
	ir->pwm.PWM_MODE_MEMORY_REGS.LDURATION = 25;
	ir->pwm.PWM_MODE_MEMORY_REGS.WAVE_NUM = 1;
	ir->misc.minor = MISC_DYNAMIC_MINOR;
	ir->misc.name = "irtx";
	ir->misc.fops = &ir_fops;
	ir->misc.parent = ir->dev;
	ir->misc.mode = 0600;
	wake_lock_init(&ir->awake, WAKE_LOCK_SUSPEND, "couch-irtx");
	ret = misc_register(&ir->misc);
	if (ret) {
		wake_lock_destroy(&ir->awake);
		return ret;
	}
	platform_set_drvdata(pdev, ir);
	dev_info(ir->dev, "HA100 IR PWM%u, invert=%u, completion polling\n", channel, invert);
	return 0;
}

/* Built-in only: no removal while open descriptors refer to the hardware. */
static const struct of_device_id ir_match[] = {
	{ .compatible = "mediatek,irtx-pwm" }, { }
};
static struct platform_driver ir_driver = {
	.probe = ir_probe,
	.driver = {
		.name = "couch-irtx",
		.of_match_table = ir_match,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(ir_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HA100 MT6580 PWM infrared transmitter");
