/* SPDX-License-Identifier: GPL-2.0 */
#ifndef HA100_LED_BOARD_H
#define HA100_LED_BOARD_H

/* Stock HA100 LED data values select board functions, never GPIO numbers. */
enum ha100_led_state {
	HA100_DEFAULT, HA100_SUPPLY_LOW, HA100_SUPPLY_HIGH,
	HA100_BUTTON_LOW, HA100_BUTTON_HIGH, HA100_CHARGE_LOW,
	HA100_CHARGE_HIGH, HA100_STANDBY_LOW, HA100_STANDBY_HIGH,
	HA100_STATE_COUNT
};

static const char * const ha100_led_state_names[HA100_STATE_COUNT] = {
	"default", "btn_light_ldo_3v3_low", "btn_light_ldo_3v3_high",
	"btn_light_gpio_low", "btn_light_gpio_high", "chrg_gpio_low",
	"chrg_gpio_high", "stdby_gpio_low", "stdby_gpio_high"
};

static int ha100_led_runtime_state(const char *name, long selector, int level)
{
	if (!name)
		return -1;
	if (!strcmp(name, "button-backlight") && selector == 4)
		return level > 0 ? HA100_BUTTON_HIGH : HA100_BUTTON_LOW;
	if (!strcmp(name, "red") && selector == 2)
		return level > 0 ? HA100_CHARGE_HIGH : HA100_CHARGE_LOW;
	return -1;
}

static int ha100_led_boot_state(unsigned int step, int normal_boot)
{
	switch (step) {
	case 0: return HA100_SUPPLY_HIGH;
	case 1: return normal_boot ? HA100_BUTTON_HIGH : HA100_BUTTON_LOW;
	case 2: return HA100_CHARGE_HIGH;
	case 3: return HA100_STANDBY_HIGH;
	default: return -1;
	}
}
#endif
