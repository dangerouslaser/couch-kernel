/* Host regression: cc -Wall -Wextra -Werror tools/testing/ha100-led-board-test.c -o /tmp/ha100-led-board-test && /tmp/ha100-led-board-test */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../drivers/misc/mediatek/leds/mt6580/ha100-led-board.h"

int main(void)
{
	unsigned int i;
	const int normal[] = {HA100_SUPPLY_HIGH, HA100_BUTTON_HIGH,
		HA100_CHARGE_HIGH, HA100_STANDBY_HIGH};
	const int recovery[] = {HA100_SUPPLY_HIGH, HA100_BUTTON_LOW,
		HA100_CHARGE_HIGH, HA100_STANDBY_HIGH};

	for (i = 0; i < 4; i++) {
		assert(ha100_led_boot_state(i, 1) == normal[i]);
		assert(ha100_led_boot_state(i, 0) == recovery[i]);
	}
	/* A missing empty default is acceptable; no functional state is optional. */
	assert(!ha100_led_state_required(HA100_DEFAULT));
	for (i = HA100_SUPPLY_LOW; i < HA100_STATE_COUNT; i++)
		assert(ha100_led_state_required(i));
	assert(!ha100_led_state_required(HA100_STATE_COUNT));
	assert(ha100_led_boot_state(4, 1) == -1);
	assert(ha100_led_runtime_state("red", 2, 1) == HA100_CHARGE_HIGH);
	assert(ha100_led_runtime_state("red", 2, 255) == HA100_CHARGE_HIGH);
	assert(ha100_led_runtime_state("red", 2, 0) == HA100_CHARGE_LOW);
	assert(ha100_led_runtime_state("button-backlight", 4, 1) == HA100_BUTTON_HIGH);
	assert(ha100_led_runtime_state("button-backlight", 4, 0) == HA100_BUTTON_LOW);
	assert(ha100_led_runtime_state("button-backlight", 4, -1) == HA100_BUTTON_LOW);
	assert(ha100_led_runtime_state("red", 4, 1) == -1);
	assert(ha100_led_runtime_state("button-backlight", 2, 1) == -1);
	assert(ha100_led_runtime_state("lcd-backlight", 4, 1) == -1);
	assert(ha100_led_runtime_state(NULL, 2, 1) == -1);
	assert(ha100_led_runtime_state("red", 17, 1) == -1);
	for (i = 0; i < HA100_STATE_COUNT; i++)
		assert(ha100_led_state_names[i] && ha100_led_state_names[i][0]);
	assert(!strcmp(ha100_led_state_names[HA100_SUPPLY_HIGH], "btn_light_ldo_3v3_high"));
	assert(!strcmp(ha100_led_state_names[HA100_STANDBY_HIGH], "stdby_gpio_high"));
	puts("HA100 LED sequence/selector tests passed");
	return 0;
}
