/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/power_supply.h>
#include <oem-bms.h>

#undef BMS_DEBUG
#ifdef BMS_DEBUG
	#define BMS_LOG pr_err
#else
	#define BMS_LOG pr_debug
#endif

#define LOWER_LIMIT_SOC_CHARGING 0
#define UPPER_LIMIT_SOC_CHARGING 99
#define OEM_SOC_FULL 100

#define OEM_SOC_SHUT_DOWN	0
#define OEM_SHUT_DOWN_UV	3200000
#define SHUT_DOWN_MONITOR_TIME_SEC	30




static int oem_bms_debug_enable = 0;
module_param_named(debug_enable, oem_bms_debug_enable, int, S_IRUGO | S_IWUSR);


static int oem_bms_get_battery_prop(enum power_supply_property psp)
{
	union power_supply_propval ret = {0,};
	static struct power_supply *batatery_psy;

	if (batatery_psy == NULL)
		batatery_psy = power_supply_get_by_name("battery");
	if (batatery_psy) {
		/* if battery has been registered, use the status property */
		batatery_psy->get_property(batatery_psy, psp, &ret);
		return ret.intval;
	}

	/* Default to false if the battery power supply is not registered. */
	BMS_LOG("battery power supply is not registered\n");
	return POWER_SUPPLY_STATUS_UNKNOWN;
}

static bool oem_bms_is_battery_charging(void)
{
	return oem_bms_get_battery_prop(POWER_SUPPLY_PROP_STATUS) == POWER_SUPPLY_STATUS_CHARGING;
}

static bool oem_bms_is_battery_full(void)
{
	return oem_bms_get_battery_prop(POWER_SUPPLY_PROP_STATUS) == POWER_SUPPLY_STATUS_FULL;
}

static int oem_bms_bound_soc_in_charging(int soc)
{
	soc = max(LOWER_LIMIT_SOC_CHARGING, soc);
	soc = min(UPPER_LIMIT_SOC_CHARGING, soc);
	return soc;
}

static int shutdown_base_time = 0;
static bool shutdown_alert = false;
int oem_bms_correct_soc(int in_soc)
{
	struct timespec ts;
	int elapsed_time;
	int result_soc = in_soc;
	int batt_uv;

	if(oem_bms_is_battery_charging()) {
		result_soc = oem_bms_bound_soc_in_charging(in_soc);

	} else {
		batt_uv = oem_bms_get_battery_prop(POWER_SUPPLY_PROP_VOLTAGE_NOW);

		if (batt_uv <= OEM_SHUT_DOWN_UV) {
			getnstimeofday(&ts);

			if (!shutdown_alert) {
				shutdown_base_time = ts.tv_sec;
				shutdown_alert = true;
			}
			elapsed_time = ts.tv_sec - shutdown_base_time;
			pr_info("low battery result_soc:%d in_soc:%d batt_mv:%d elapsed_time:%d\n",
				result_soc, in_soc, batt_uv/1000, elapsed_time);

			if (elapsed_time >= SHUT_DOWN_MONITOR_TIME_SEC) {
				result_soc = OEM_SOC_SHUT_DOWN;
				pr_err("to shut down, because battery is low voltage\n");
			}
		} else {
			shutdown_alert = false;
		}

	}

	if(oem_bms_is_battery_full()) {
		result_soc = OEM_SOC_FULL;
	}

	BMS_LOG("result soc = %d, in_soc = %d \n", result_soc, in_soc);

	return result_soc;
}
EXPORT_SYMBOL(oem_bms_correct_soc);
