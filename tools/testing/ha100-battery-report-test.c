/* cc -Wall -Wextra -Werror tools/testing/ha100-battery-report-test.c -o /tmp/ha100-battery-report-test */
#include <assert.h>
#include <stdio.h>
#include "../../drivers/power/mediatek/ha100-battery-report.h"

int main(void)
{
	assert(ha100_battery_status(0, 1, 1, 0, 1, 0) == HA100_BATTERY_UNKNOWN);
	assert(ha100_battery_status(1, 0, 1, 0, 1, 0) == HA100_BATTERY_UNKNOWN);
	/* Unplugging wins even if the last full flag has not yet cleared. */
	assert(ha100_battery_status(1, 1, 0, 0, 1, 0) == HA100_BATTERY_DISCHARGING);
	assert(ha100_battery_status(1, 1, 1, 1, 1, 0) == HA100_BATTERY_NOT_CHARGING);
	assert(ha100_battery_status(1, 1, 1, 0, 1, 0) == HA100_BATTERY_FULL);
	assert(ha100_battery_status(1, 1, 1, 0, 1, 1) == HA100_BATTERY_CHARGING);
	assert(ha100_battery_status(1, 1, 1, 0, 0, 0) == HA100_BATTERY_CHARGING);
	puts("HA100 battery reporting tests passed");
	return 0;
}
