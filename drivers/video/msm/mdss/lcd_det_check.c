/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

#include "lcd_det_check.h"

int lcd_det_check(void)
{
	int disp_det_gpio = GPIO_LCD_DET_NUM;
	unsigned int det_check = 0;
	int rc;

	rc = gpio_tlmm_config(GPIO_CFG(
			disp_det_gpio, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
			
	if (rc) {
		pr_err("%s: unable to config tlmm = %d\n",
			__func__, disp_det_gpio);
		return -ENODEV;
	}

	rc = gpio_direction_input(disp_det_gpio);
	if (rc) {
		pr_err("set_direction for disp_en gpio failed, rc=%d\n",
		       rc);
		return -ENODEV;
	}
	pr_debug("%s: disp_det_gpio=%d\n", __func__,
				disp_det_gpio);

 	det_check = gpio_get_value(disp_det_gpio);

	if(det_check==0){
		det_check=1;
	}else{
		det_check=0;
	}
	rc = gpio_tlmm_config(GPIO_CFG(
			disp_det_gpio, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	if (rc) {
			pr_err("%s: unable to config tlmm = %d\n",
				__func__, disp_det_gpio);
			return -ENODEV;
	}
	rc = gpio_direction_input(disp_det_gpio);

	if (rc) {
		pr_err("set_direction for disp_en gpio failed, rc=%d\n",
		       rc);
		return -ENODEV;
	}
	pr_debug("%s: disp_det_gpio=%d\n", __func__,
				disp_det_gpio);

	printk("[LCD] det_check=%x\n",(int)det_check);
	return det_check;
}
