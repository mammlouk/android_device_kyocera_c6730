/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2011 KYOCERA Corporation
 * (C) 2012 KYOCERA Corporation
 * (C) 2013 KYOCERA Corporation
 * (C) 2014 KYOCERA Corporation
 *
 * drivers/input/touchscreen/ts_ctrl.h
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

#ifndef __LINUX_TS_CTRL_H
#define __LINUX_TS_CTRL_H

#include <linux/types.h>

enum ts_config_type {
	TS_CONFIG_BOOT_NORMAL = 0,
	TS_CONFIG_BOOT_CALL_STEP1,
	TS_CONFIG_BOOT_CALL_STEP2,
	TS_CONFIG_MOVE_NORMAL,
	TS_CONFIG_MOVE_CALL_STEP1,
	TS_CONFIG_MOVE_CALL_STEP2,
	TS_CONFIG_DEBUG_FLAG,
	TS_CONFIG_MAX,
};

enum ts_mode_call { 
  TS_MODE_CALL_OFF = 0,
  TS_MODE_CALL_STEP1,
  TS_MODE_CALL_STEP2,
};

enum ts_mode_state { 
  TS_MODE_STATE_BOOT = 0,
  TS_MODE_STATE_MOVE,
};

struct ts_diag_event {
	int x;
	int y;
	int width;
};

struct ts_diag_type {
	struct ts_diag_event ts[10];
	int diag_count;
};

struct ts_nv_data {
	size_t size;
	char *data;
	int type;
};

struct ts_config_nv {
	size_t size;
	u16 ver;
	u8 *data;
};
struct ts_log_data {
	int flag;
	int data;
};

struct ts_mode_flag {
	int state;
	int call;
};

extern int ts_diag_start_flag;
extern struct ts_diag_type *diag_data;
extern struct mutex diag_lock;
extern struct mutex file_lock;
extern char ts_event_control;
extern char ts_log_level;
extern char ts_log_file_enable;
extern char ts_esd_recovery;
extern char ts_config_switching;

int ts_ctrl_init(struct cdev *device_cdev, const struct file_operations *fops);
int ts_ctrl_exit(struct cdev *device_cdev);

#define IOC_MAGIC 't'
#define IOCTL_SET_CONF_STAT _IOW(IOC_MAGIC, 1, enum ts_config_type)
#define IOCTL_GET_CONF_STAT _IOR(IOC_MAGIC, 2, enum ts_config_type)
#define IOCTL_SET_LOG _IOW(IOC_MAGIC, 3, struct ts_log_data)
#define IOCTL_SET_NV _IO(IOC_MAGIC, 8)
#define IOCTL_LOAD_DEBUG_FLAG_NV _IOW(IOC_MAGIC, 9, struct ts_nv_data)
#define IOCTL_LOAD_WS_NV _IOW(IOC_MAGIC, 10, struct ts_nv_data)
#define IOCTL_CHECK_FW _IOW(IOC_MAGIC, 11, struct ts_nv_data)
#define IOCTL_GET_INFO _IOW(IOC_MAGIC, 12, unsigned char)

#define IOCTL_DIAG_START _IO(IOC_MAGIC, 0xA1)
#define IOCTL_MULTI_GET _IOR(IOC_MAGIC, 0xA2, struct ts_diag_type)
#define IOCTL_COODINATE_GET _IOR(IOC_MAGIC, 0xA3, struct ts_diag_type)
#define IOCTL_DIAG_END _IO(IOC_MAGIC, 0xA4)
#define IOCTL_DIAG_LOG_LEVEL _IOW(IOC_MAGIC, 0xA5, unsigned char)
#define IOCTL_DIAG_EVENT_CTRL _IOW(IOC_MAGIC, 0xA6, unsigned char)
#define IOCTL_DIAG_RESET_HW _IO(IOC_MAGIC, 0xA7)
#define IOCTL_GET_GOLDEN_REFERENCE _IOWR(IOC_MAGIC, 0xA8, unsigned char)
#define IOCTL_DIAG_GET_C_REFERENCE _IOR(IOC_MAGIC, 0xA9, unsigned char)
#define IOCTL_DIAG_GET_DELTA _IOR(IOC_MAGIC, 0xA0, unsigned char)

#define IOCTL_DIAG_SELFTEST _IOR(IOC_MAGIC, 0xC0, int)
#define IOCTL_COPY_REG_TO_CONFIG _IOW(IOC_MAGIC, 0xC1, enum ts_config_type)
#define IOCTL_SET_SWITCHABLE_CONFIG _IOW(IOC_MAGIC, 0xC2, enum ts_config_type)
#define IOCTL_LOAD_CONFIG_NV _IOW(IOC_MAGIC, 0xC3, struct ts_nv_data)
#define IOCTL_CHANGE_MODE _IOW(IOC_MAGIC, 0xC4, struct ts_mode_flag)
#define IOCTL_GET_MODE_FLAG _IOR(IOC_MAGIC, 0xC5, struct ts_mode_flag)
#define IOCTL_CHANGE_LCD_STATUS _IOW(IOC_MAGIC, 0xC6, bool)

#endif /* __LINUX_TS_CTRL_H */
