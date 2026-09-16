/* Reporting only: none of these decisions controls the charger or gauge. */
#ifndef _HA100_BATTERY_REPORT_H
#define _HA100_BATTERY_REPORT_H

enum ha100_battery_status {
	HA100_BATTERY_UNKNOWN,
	HA100_BATTERY_CHARGING,
	HA100_BATTERY_DISCHARGING,
	HA100_BATTERY_NOT_CHARGING,
	HA100_BATTERY_FULL,
};

static inline enum ha100_battery_status ha100_battery_status(
	int ready, int present, int online, int inhibited, int full, int recharging)
{
	if (!ready || !present)
		return HA100_BATTERY_UNKNOWN;
	if (!online)
		return HA100_BATTERY_DISCHARGING;
	if (inhibited)
		return HA100_BATTERY_NOT_CHARGING;
	/* UI_SOC can reach 100 before termination, or stay there during recharge. */
	if (full && !recharging)
		return HA100_BATTERY_FULL;
	return HA100_BATTERY_CHARGING;
}

#endif
