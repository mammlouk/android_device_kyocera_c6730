/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2012 KYOCERA Corporation
 * (C) 2013 KYOCERA Corporation
 * (C) 2014 KYOCERA Corporation
 *
 * drivers/video/msm/disp_ext_diag.c
 *
 * Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/hrtimer.h>
#include <mach/board.h>
#include "mdss_fb.h"
#include "mdss_dsi.h"
#include "mdss_mdp.h"
#include "disp_ext.h"
#include "lcd_det_check.h"

extern int disp_ext_first_flg;
extern int disp_ext_mipi_dsi_err_cnt;

#define DISP_EXT_TIMER_CYCLE 5000
#define REFRASH_CHECK_CYCLE  4000
unsigned int refresh_check_time = 0; 

static int kcfactory_check_flg = -1;
int disp_ext_kcfactory_check( void )
{
	struct device_node *chosen_node;
	char *disp_idx;
	static const char *cmd_line;
	int cmd_len, len = 0;

	if(kcfactory_check_flg != -1){
		return kcfactory_check_flg;
	}

	chosen_node = of_find_node_by_name(NULL, "chosen");
	if (!chosen_node) {
		kcfactory_check_flg = 3;
		return kcfactory_check_flg;
	}

	cmd_line = of_get_property(chosen_node, "bootargs", &len);
	if (!cmd_line || len <= 0) {
		kcfactory_check_flg = 2;
		return kcfactory_check_flg;
	}
	cmd_len = strlen(cmd_line);
	disp_idx = strnstr(cmd_line, "kcfactory", cmd_len);
	if (!disp_idx) {
		kcfactory_check_flg = 0;
	}else{
		kcfactory_check_flg = 1;
	}
	return kcfactory_check_flg;
}

void disp_ext_force_mipi_clk_hs( int onoff, unsigned char *ctrl_base )
{
  u32 tmp;
  if(ctrl_base == NULL){
    printk("%s() PTR NULL!!! \n", __func__);
    return;
  }

  tmp = MIPI_INP(ctrl_base + 0xac);
  if(onoff){
    tmp |= (1<<28);
  }else{
    tmp &= ~(1<<28);
  }
  MIPI_OUTP(ctrl_base + 0xac, tmp);
  wmb();
  printk("FORCE HS [%x]\n", tmp);
}

extern u32 mdss_dsi_panel_cmd_read(struct mdss_dsi_ctrl_pdata *ctrl, char cmd0,
		char cmd1, void (*fxn)(int), char *rbuf, int len);

char disp_ext_kernel_reg_read(struct msm_fb_data_type *mfd, char cmd)
{
  char   rbuf[4] = {0};
  struct mdss_panel_data *pdata;
  struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
  pdata = dev_get_platdata(&mfd->pdev->dev);
  ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
  mdss_dsi_panel_cmd_read(ctrl_pdata, cmd, 0x00, NULL, (char *)rbuf, 1);
  return(rbuf[0]);
}

int disp_ext_refresh_time_check_func( void )
{
  unsigned int now_time = ktime_to_ms(ktime_get());
  if((now_time - refresh_check_time) > REFRASH_CHECK_CYCLE){
    refresh_check_time = now_time;
    return 1;
  }
  return 0;
}

void disp_ext_panel_refresh_check( struct fb_info *info )
{
  int refresh_flg = 0;
  struct mdss_panel_data *pdata;
  struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)info->par;
  if(!mfd->panel_power_on){
    return;
  }
  if(!disp_ext_refresh_time_check_func()){
    return;
  }
  if(disp_ext_mipi_dsi_err_cnt){
    return;
  }
  if((disp_ext_kernel_reg_read(mfd, 0x0A) != 0x1C) ||
     (disp_ext_kernel_reg_read(mfd, 0x0C) != 0x70)){
    refresh_flg = 1;
  }
  if(refresh_flg){
    pdata = dev_get_platdata(&mfd->pdev->dev);
    disp_ext_panel_refresh(pdata);
  }
}

static struct fb_info *disp_ext_file_fb_info(struct file *file)
{
  struct inode *inode = file->f_path.dentry->d_inode;
  int fbidx = iminor(inode);
  struct fb_info *info = registered_fb[fbidx];

  if (info != file->private_data){
    info = NULL;
  }
  return info;
}

static struct fb_info *disp_exit_fb0_info_get( void )
{
  char *filename = "/dev/graphics/fb0";
  struct fb_info *info;
  struct file *file;
  file = filp_open(filename, O_RDWR | O_APPEND, 0);
  info = disp_ext_file_fb_info(file);
  return info;
}

static struct work_struct disp_ext_timer_work;
static struct timer_list  disp_ext_timer;
DEFINE_MUTEX(disp_ext_timer_lock);
static void disp_ext_timer_f(unsigned long data)
{
  schedule_work(&disp_ext_timer_work);
}
static void disp_ext_timer_test( void )
{
  mutex_lock(&disp_ext_timer_lock);
  del_timer(&disp_ext_timer);
  disp_ext_timer.function = disp_ext_timer_f;
  disp_ext_timer.data = (unsigned long)NULL;
  disp_ext_timer.expires = jiffies + ((DISP_EXT_TIMER_CYCLE * HZ) / 1000);
  add_timer(&disp_ext_timer);
  mutex_unlock(&disp_ext_timer_lock);
}

static void disp_ext_timer_w(struct work_struct *work)
{
  struct fb_info *info;
  info = disp_exit_fb0_info_get();
  if(info != NULL){
    disp_ext_panel_refresh_check(info);
  }
  disp_ext_timer_test();
}

int disp_ext_refresh_start_check( void )
{
  if(1){return 1;}

  if(disp_ext_kcfactory_check()){
    return 1;
  }

  if(disp_ext_first_flg){
    return 1;
  }

  if(lcd_det_check() == 0){
    printk("%s() LCD not Connect !!! \n ", __func__);
    return 1;
  }

  if(oem_is_off_charge()){
    return 1;
  }

  return 0;
}

void disp_ext_timer_cycle_start( void )
{
  static int first_flg = 1;

  if(disp_ext_refresh_start_check()){
    return;
  }

  if(first_flg){
    INIT_WORK(&disp_ext_timer_work, disp_ext_timer_w);
    init_timer(&disp_ext_timer);
    first_flg = 0;
  }
  disp_ext_timer_test();
}

void disp_ext_timer_cycle_stop( void )
{
  if(disp_ext_refresh_start_check()){
    return;
  }

  del_timer_sync(&disp_ext_timer);
}
