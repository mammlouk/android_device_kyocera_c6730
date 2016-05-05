/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
 */
/* drivers/input/touchscreen/atmel_mxt_kc.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#ifndef __DRIVERS_INPUT_TOUCHSCREEN_STMEL_MXT_KC_H
#define __DRIVERS_INPUT_TOUCHSCREEN_STMEL_MXT_KC_H

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_KC
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/i2c/atmel_mxt_kc.h>


#define MXT_TS_RESET_GPIO	16
#define MXT_TS_GPIO_IRQ		17

#define MXT_PANEL_HEIGHT	960
#define MXT_PANEL_WIDTH		540

#define MXT_BOARD_INFO		I2C_BOARD_INFO("mXT224S", 0x94 >> 1)

static inline int mxt_init_hw(void)
{
	int rc;

	rc = gpio_request(MXT_TS_GPIO_IRQ, "mxt_ts_irq_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_ts_irq gpio [%d]\n"
						, __func__, MXT_TS_GPIO_IRQ);
		goto err_irq_gpio_req;
	}

	rc = gpio_direction_input(MXT_TS_GPIO_IRQ);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_ts_irq gpio [%d]\n"
						, __func__, MXT_TS_GPIO_IRQ);
		goto err_irq_gpio_set;
	}

	rc = gpio_request(MXT_TS_RESET_GPIO, "mxt_reset_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_reset gpio [%d]\n"
						, __func__, MXT_TS_RESET_GPIO);
		goto err_reset_gpio_req;
	}

	rc = gpio_direction_output(MXT_TS_RESET_GPIO, 0);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_reset gpio [%d]\n"
						, __func__, MXT_TS_RESET_GPIO);
		goto err_reset_gpio_set;
	}

	return 0;

err_reset_gpio_set:
	gpio_free(MXT_TS_RESET_GPIO);
err_reset_gpio_req:
err_irq_gpio_set:
	gpio_free(MXT_TS_GPIO_IRQ);
err_irq_gpio_req:
	return rc;
}

static inline int mxt_reset_hw(void)
{
	gpio_set_value(MXT_TS_RESET_GPIO, 0);
	msleep(1);
	gpio_set_value(MXT_TS_RESET_GPIO, 1);

	return 0;
}

static inline int mxt_shutdown_hw(void)
{
	gpio_set_value(MXT_TS_RESET_GPIO, 0);
	msleep(1);

	return 0;
}

/* Register No, Data Size */
/* Data[0], Data[1], ... */
static const u8 mxt_config_data0[] = {
	/* T37 Object */
	/* T44 Object */
	/* T5 Object */
	/* T6 Object */
	/* T38 Object */
	/* T7 Object */
	   7,   4,
	 255,   8,   5,  66,
	/* T8 Object */
	   8,  10,
	  24,   0,   1,   1,   0,   0,   0,   0,  20,   0,
	/* T9 Object */
	   9,  36,
	 139,   0,   0,  20,  11,   0,  80,  31,   1,   7,
	  10,  10,   7,  64,   5,   5,  25,   0, 255,   3,
	  27,   2,  15,  10,  33,  33, 202,  50, 191,  50,
	  12,  10,   0,   0,   1,   0,
	/* T18 Object */
	/* T19 Object */
	  19,   6,
	   0,   0,   0,   0,   0,   0,
	/* T25 Object */
	  25,   7,
	   0,   0,0x48,0x71,0x08,0x52,   0,
	/* T42 Object */
	  42,  10,
	   3,  30,  38,  33,   0,   0,   0,   0,   0,   0,
	/* T46 Object */
	  46,  10,
	   0,   0,  16,  32,   0,   0,   3,   0,   0,   1,
	/* T47 Object */
	  47,  26,
	   9,   1,   1, 254, 220,  30,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T55 Object */
	  55,   6,
	   1,  10,  64,   0, 200,   0,
	/* T56 Object */
	  56,  43,
	   0,   0,   1,  41,  12,  12,  12,  12,  12,  11,
	  10,  12,  12,  11,  12,  12,  12,  12,  12,  12,
	  11,  12,   0,   0,   0,   0,   0,   0,   0,   0,
	   1,   2,  20,   4,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,
	/* T57 Object */
	  57,   3,
	   0,   0,   0,
	/* T61 Object */
	  61,  10,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T62 Object */
	  62,  54,
	   1,   3,   3,  18,   0,   0,  32,   0,   0,   0,
	   0,   0,   0,   0,   5,   0,  10,   5,   5,  96,
	  30,  17,  52,   4, 100,   6,   6,   4,  64,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,
	/* T65 Object */
	  65,  17,
	   1,   1,   1,   0,   1,   0,   1,   0,   1,   0,
	 254,   0,   0,   0, 254,   0,   0,
	/* T66 Object */
	  66,   5,
	   0,   0,   0,   0,   0,
};

/* Default Register is [BOOT STATE:CALL OFF] */
/* Register No, Data Size */
/* Data[0], Data[1], ... */
static const u8 mxt_config_data1[] = {
	/* T37 Object */
	/* T44 Object */
	/* T5 Object */
	/* T6 Object */
	/* T38 Object */
	/* T71 Object */
	  71, 112,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,
	/* T7 Object */
	   7,   4,
	 255,   8,   5,  66,
	/* T8 Object */
	   8,  14,
	  20,   0,   1,   1,   0,   0,   0,   0,  20,   0,
	   0,   0,   0,   0,
	/* T9 Object */
	   9,  46,
	 139,   0,   0,  20,  11,   0,  96,  35,   1,   7,
	  10,  26,   7,   0,   5,   5,  75,   2, 255,   3,
	  27,   2,  15,   2,  42,  42, 202,  50, 191,   5,
	  14,   5,   0,   0,   1,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T15 Object */
	  15,  11,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,
	/* T18 Object */
	/* T19 Object */
	/* T25 Object */
	  25,  15,
	   0,   0,0x30,0x75,0x2c,0x4c,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,
	/* T42 Object */
	/* T46 Object */
	  46,  10,
	   0,   0,  16,  32,   0,   0,   3,   0,   0,   1,
	/* T47 Object */
	  47,  26,
	   9,   1,   1, 254, 216,  20,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T55 Object */
	/* T56 Object */
	  56,  30,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T57 Object */
	  57,   3,
	   0,   0,   0,
	/* T61 Object */
	  61,  10,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T62 Object */
	/* T65 Object */
	  65,  11,
	   1,   5,   1,   0,   1,   0,   1,   0,   1,   0,
	 254,
	/* T66 Object */
	  66,   3,
	   0,   0,   0,
	/* T70 Object */
	  70,  80,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T72 Object */
	  72,  80,
	   3,   0,   0,   0,   0,   5,  10,   5,   5,  48,
	  31,  25,  15,   2,  32,   0,   0,  80,   0,   0,
	   2,   0,   5,  10,  15,  20,  16,  16,  16,  16,
	  16,   8,   8,   8,   8,   8,   0,   0,   0,   0,
	   3,   0,   5,  10,  15,  20,  24,  24,  24,  24,
	  24,  16,  16,  16,  16,  16,   0,   0,   0,   0,
	   3,   0,   5,  10,  15,  20,  32,  32,  32,  32,
	  32,  24,  24,  24,  24,  24,   0,   0,   0,   0,
	/* T80 Object */
	  80,   4,
	   0,   0,   0,   0,
};

static const u8 mxt_config_data2[] = {
	/* T37 Object */
	/* T44 Object */
	/* T5 Object */
	/* T6 Object */
	/* T38 Object */
	/* T71 Object */
	  71, 112,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,
	/* T7 Object */
	   7,   4,
	 255,   8,   5,  66,
	/* T8 Object */
	   8,  14,
	  20,   0,   1,   1,   0,   0,   0,   0,  20,   0,
	   0,   0,   0,   0,
	/* T9 Object */
	   9,  46,
	 139,   0,   0,  20,  11,   0,  96,  40,   1,   7,
	  10,  25,   7,   0,   5,   5,  75,   2, 255,   3,
	  27,   2,  15,  10,  33,  33, 202,  50, 191,  50,
	  14,   5,   0,   0,   1,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T18 Object */
	/* T19 Object */
	/* T25 Object */
	  25,   9,
	   0,   0,0x30,0x75,0x2c,0x4c,   0,0x04,0x29,
	/* T42 Object */
	  42,  10,
	   3,  20,  20,  12,   0,   0,   0,   0,   0,   0,
	/* T46 Object */
	  46,  10,
	   0,   0,  16,  32,   0,   0,   3,   0,   0,   1,
	/* T47 Object */
	  47,  26,
	   9,   1,   1, 254, 216,  20,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T55 Object */
	/* T56 Object */
	  56,  30,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T57 Object */
	  57,   3,
	   0,   0,   0,
	/* T61 Object */
	  61,  10,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T62 Object */
	/* T65 Object */
	  65,  11,
	   1,   5,   1,   0,   1,   0,   1,   0,   1,   0,
	 254,
	/* T66 Object */
	  66,   3,
	   0,   0,   0,
	/* T70 Object */
	  70,  80,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T72 Object */
	  72,  80,
	   3,   0,   0,   0,   0,   5,  10,   5,   5,   0,
	   0,   0,  15,   2,  32,  30,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,
	   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   8,   0,   3,
	/* T80 Object */
	  80,   4,
	   0,   0,   0,   0,
};

static const u8 mxt_config_data3[] = {
	/* T37 Object */
	/* T44 Object */
	/* T5 Object */
	/* T6 Object */
	/* T38 Object */
	/* T71 Object */
	  71, 112,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,
	/* T7 Object */
	   7,   4,
	 255,   8,   5,  66,
	/* T8 Object */
	   8,  14,
	  24,   0,   1,   1,   0,   0,   0,   0,  15,   0,
	   0,   0,   0,   0,
	/* T9 Object */
	   9,  46,
	 139,   0,   0,  20,  11,   0,  80,  29,   1,   7,
	  10,  10,   7,   0,   5,   5,  20,   0, 255,   3,
	  27,   2,  15,  10,  33,  33, 202,  50, 191,  50,
	  12,   5,   0,   0,   1,   0,   3, 225,   2,   1,
	 254,   0,   0,   0,   0,  30,
	/* T18 Object */
	/* T19 Object */
	/* T25 Object */
	  25,   9,
	   0,   0,0x30,0x75,0x2c,0x4c,   0,0x04,0x29,
	/* T42 Object */
	  42,  10,
	   3,  20,  20,  12,   0,   0,   0,   0,   0,   0,
	/* T46 Object */
	  46,  10,
	   0,   0,  16,  32,   0,   0,   3,   0,   0,   1,
	/* T47 Object */
	  47,  26,
	   9,   1,   1, 254, 219,   5,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,
	/* T55 Object */
	/* T56 Object */
	  56,  30,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T57 Object */
	  57,   3,
	   0,   0,   0,
	/* T61 Object */
	  61,  10,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T62 Object */
	/* T65 Object */
	  65,  11,
	   1,   1,   1,   0,   1,   0,   1,   0,   1,   0,
	  15,
	/* T66 Object */
	  66,   3,
	   0,   0,   0,
	/* T70 Object */
	  70,  80,	
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	/* T72 Object */
	  72,  80,
	   3,   1,   0,   0,   0, 128, 128, 128, 128,   0,
	  29,  20,  20,   8,  32,   0,   0,  80,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,
	   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   8,   0,   3,
	/* T80 Object */
	  80,   4,
	   1,  10, 128,  25,
};

const struct mxt_config mxt_configs[] = {
	{
		mxt_config_data0,
		ARRAY_SIZE(mxt_config_data0),
	},
	{
		mxt_config_data1,
		ARRAY_SIZE(mxt_config_data1),
	},
	{
		mxt_config_data2,
		ARRAY_SIZE(mxt_config_data2),
	},
	{
		mxt_config_data3,
		ARRAY_SIZE(mxt_config_data3),
	},
};

#endif /* CONFIG_TOUCHSCREEN_ATMEL_MXT_KC */
#endif /* __DRIVERS_INPUT_TOUCHSCREEN_STMEL_MXT_KC_H */
