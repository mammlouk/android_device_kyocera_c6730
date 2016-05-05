/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
 */
/* drivers/input/touchscreen/atmel_mxt_kc.c
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
*/

#undef MXT_DUMP_OBJECT
#define FEATURE_FACTORY_TEST

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_kc.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <mach/kc_board.h>
#include <mach/board.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define MXT_SUSPEND_LEVEL 1
#endif
#include <linux/fs.h>
#include <linux/namei.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include "ts_ctrl.h"
#include "atmel_mxt_kc.h"

/* Family ID */
#define MXT224_ID		0x80
#define MXT224E_ID		0x81
#define MXT1386_ID		0xA0
#define MXT224S_FAMILY_ID	0x82

/* Variant ID */
#define MXT224S_VARIANT_ID	0x1a

/* Version */
#define MXT_FW_V1_0		0x10
#define MXT_FW_V1_9		0x19
#define MXT_FW_V3_0		0x30
#define MXT_FW_VSC_16		0x10

/* Build */
#define MXT224S_BUILD		0xAA

/* Slave addresses */
#define MXT_APP_LOW		0x4a
#define MXT_APP_HIGH		0x4b
#define MXT_BOOT_LOW		0x24
#define MXT_BOOT_HIGH		0x25

/* Firmware */
#define MXT_FW_NAME		"maxtouch.fw"

/* Registers */
#define MXT_FAMILY_ID		0x00
#define MXT_VARIANT_ID		0x01
#define MXT_VERSION		0x02
#define MXT_BUILD		0x03
#define MXT_MATRIX_X_SIZE	0x04
#define MXT_MATRIX_Y_SIZE	0x05
#define MXT_OBJECT_NUM		0x06
#define MXT_OBJECT_START	0x07

#define MXT_OBJECT_SIZE		6

/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_SPT_USERDATA_T38		38
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_TOUCH_MULTI_T9		9
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_SPT_SELFTEST_T25		25
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_TOUCH_T42		42
#define MXT_SPT_CTECONFIG_T46		46
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCI_ADAPTIVE_T55		55
#define MXT_PROCI_SHIELDLESS_T56	56
#define MXT_PROCI_EXTRA_T57		57
#define MXT_SPT_TIMER_T61		61
#define MXT_PROCG_NOISE_T62		62
#define MXT_PROCI_LENSBENDING_T65	65
#define MXT_SPT_GOLDENREFERENCES_T66	66
#define MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70	70
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71	71
#define MXT_PROCG_NOISESUPPRESSION_T72	72
#define MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80	80

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET		0
#define MXT_COMMAND_BACKUPNV		1
#define MXT_COMMAND_CALIBRATE		2
#define MXT_COMMAND_REPORTALL		3
#define MXT_COMMAND_DIAGNOSTIC		5

/* MXT_GEN_POWER_T7 field */
#define MXT_POWER_IDLEACQINT		0
#define MXT_POWER_ACTVACQINT		1
#define MXT_POWER_ACTV2IDLETO		2
#define MXT_POWER_CFG			3

/* MXT_T7 CMD field */
#define MXT_FREE_RUN_MODE		0xFF

/* MXT_GEN_ACQUIRE_T8 field */
#define MXT_ACQUIRE_CHRGTIME		0
#define MXT_ACQUIRE_TCHDRIFT		2
#define MXT_ACQUIRE_DRIFTST		3
#define MXT_ACQUIRE_TCHAUTOCAL		4
#define MXT_ACQUIRE_SYNC		5
#define MXT_ACQUIRE_ATCHCALST		6
#define MXT_ACQUIRE_ATCHCALSTHR		7
#define MXT_ACQUIRE_ATCHFRCCALTHR	8
#define MXT_ACQUIRE_ATCHFRCCALRATIO	9

/* MXT_TOUCH_MULT_T9 field */
#define MXT_TOUCH_CTRL			0
#define MXT_TOUCH_XORIGIN		1
#define MXT_TOUCH_YORIGIN		2
#define MXT_TOUCH_XSIZE			3
#define MXT_TOUCH_YSIZE			4
#define MXT_TOUCH_AKSCFG		5
#define MXT_TOUCH_BLEN			6
#define MXT_TOUCH_TCHTHR		7
#define MXT_TOUCH_TCHDI			8
#define MXT_TOUCH_ORIENT		9
#define MXT_TOUCH_MRGTIMEOUT		10
#define MXT_TOUCH_MOVHYSTI		11
#define MXT_TOUCH_MOVHYSTN		12
#define MXT_TOUCH_MOVFILTER		13
#define MXT_TOUCH_NUMTOUCH		14
#define MXT_TOUCH_MRGHYST		15
#define MXT_TOUCH_MRGTHR		16
#define MXT_TOUCH_AMPHYST		17
#define MXT_TOUCH_XRANGE_LSB		18
#define MXT_TOUCH_XRANGE_MSB		19
#define MXT_TOUCH_YRANGE_LSB		20
#define MXT_TOUCH_YRANGE_MSB		21
#define MXT_TOUCH_XLOCLIP		22
#define MXT_TOUCH_XHICLIP		23
#define MXT_TOUCH_YLOCLIP		24
#define MXT_TOUCH_YHICLIP		25
#define MXT_TOUCH_XEDGECTRL		26
#define MXT_TOUCH_XEDGEDIST		27
#define MXT_TOUCH_YEDGECTRL		28
#define MXT_TOUCH_YEDGEDIST		29
#define MXT_TOUCH_JUMPLIMIT		30
#define MXT_TOUCH_TCHHYST		31
#define MXT_TOUCH_XPITCH		32
#define MXT_TOUCH_YPITCH		33
#define MXT_TOUCH_NEXTTCHDI		34
#define MXT_TOUCH_CFG			35

/* MXT_TOUCH_PROXIMITY_T23 field */
#define MXT_TOUCH_FXDDTHR_LSB		7
#define MXT_TOUCH_FXDDTHR_MSB		8
#define MXT_TOUCH_FXDDI			9
#define MXT_TOUCH_AVERAGE		10
#define MXT_TOUCH_MVNULLRATE_LSB	11
#define MXT_TOUCH_MVNULLRATE_MSB	12
#define MXT_TOUCH_MVDTHR_LSB		13
#define MXT_TOUCH_MVDTHR_MSB		14

/* MXT_T37 field */
#define MXT_DELTA					0x10
#define MXT_CURRENT_REFERENCE		0x11

/* MXT_PROCI_TOUCH_T42 field */
#define MXT_SUPPRESSION_CTRL		0
#define MXT_SUPPRESSION_APPRTHR		1
#define MXT_SUPPRESSION_MAXAPPRAREA	2
#define MXT_SUPPRESSION_MAXTCHAREA	3
#define MXT_SUPPRESSION_SUPSTRENGTH	4
#define MXT_SUPPRESSION_SUPEXTTO	5
#define MXT_SUPPRESSION_MAXNUMTCHS	6
#define MXT_SUPPRESSION_SHAPESTRENGTH	7
#define MXT_SUPPRESSION_SUPDIST		8
#define MXT_SUPPRESSION_DISTHYST	9

/* MXT_SPT_CTECONFIG_T46 */
#define MXT_SPT_IDLESYNCSPERX		2
#define MXT_ACTVSYNCSPERX			3

/* MXT_PROCI_STYLUS_T47 field */
#define MXT_STYLUS_CTRL			0
#define MXT_STYLUS_CONTMIN		1
#define MXT_STYLUS_CONTMAX		2
#define MXT_STYLUS_STABILITY		3
#define MXT_STYLUS_MAXCHAREA		4
#define MXT_STYLUS_AMPLTHR		5
#define MXT_STYLUS_STYSHAPE		6
#define MXT_STYLUS_HOVERSUP		7
#define MXT_STYLUS_CONFTHR		8
#define MXT_STYLUS_SYNCSPERX		9
#define MXT_STYLUS_XPOSADJ		10
#define MXT_STYLUS_YPOSADJ		11
#define MXT_STYLUS_CFG			12

/* MXT_PROCI_ADAPTIVE_T55 field */
#define MXT_ADAPTIVE_CTRL		0
#define MXT_ADAPTIVE_TARGETTHR		1
#define MXT_ADAPTIVE_THRADJLIM		2
#define MXT_ADAPTIVE_RESETSTEPTIME	3
#define MXT_ADAPTIVE_FORCECHGDIST	4
#define MXT_ADAPTIVE_FORCECHGTIME	5

/* MXT_PROCI_SHIELDLESS_T56 field */
#define MXT_SHIELDLESS_CTRL		0
#define MXT_SHIELDLESS_ORIENT		2
#define MXT_SHIELDLESS_INTTIME		3
#define MXT_SHIELDLESS_INTDELAY_0	4
#define MXT_SHIELDLESS_MULTICUTGC	5
#define MXT_SHIELDLESS_GCLIMIT		6
#define MXT_SHIELDLESS_NCNCL		7
#define MXT_SHIELDLESS_TOUCHBIAS	8
#define MXT_SHIELDLESS_BASESCALE	9
#define MXT_SHIELDLESS_SHIFTLIMIT	10
#define MXT_SHIELDLESS_YLONOISEMUL_LSB	11
#define MXT_SHIELDLESS_YLONOISEMUL_MSB	12
#define MXT_SHIELDLESS_YLONOISEDIV_LSB	13
#define MXT_SHIELDLESS_YLONOISEDIV_MSB	14
#define MXT_SHIELDLESS_YHINOISEMUL_LSB	15
#define MXT_SHIELDLESS_YHINOISEMUL_MSB	16
#define MXT_SHIELDLESS_YHINOISEDIV_LSB	17
#define MXT_SHIELDLESS_YHINOISEDIV_MSB	18

/* MXT_PROCI_EXTRA_T57 field */
#define MXT_EXTRA_CTRL			0
#define MXT_EXTRA_AREATHR		1
#define MXT_EXTRA_AREAHYST		2

/* MXT_PROCG_NOISE_T62 field */
#define MXT_NOISE_CTRL			0
#define MXT_NOISE_CALCFG1		1
#define MXT_NOISE_CALCFG2		2
#define MXT_NOISE_CALCFG3		3
#define MXT_NOISE_CFG1			4
#define MXT_NOISE_BASEFREQ		7
#define MXT_NOISE_MAXSELFREQ		8
#define MXT_NOISE_FREQ_0		9
#define MXT_NOISE_FREQ_1		10
#define MXT_NOISE_FREQ_2		11
#define MXT_NOISE_FREQ_3		12
#define MXT_NOISE_FREQ_4		13
#define MXT_NOISE_HOPCNT		14
#define MXT_NOISE_HOPCNTPER		16
#define MXT_NOISE_HOPEVALTO		17
#define MXT_NOISE_HOPST			18
#define MXT_NOISE_HLGAIN		19
#define MXT_NOISE_MINNLTHR		20
#define MXT_NOISE_INCNLTHR		21
#define MXT_NOISE_ADCSPERXTHR		22
#define MXT_NOISE_NLTHRMARGIN		23
#define MXT_NOISE_MAXADCSPERX		24
#define MXT_NOISE_ACTVADCSVLDNOD	25
#define MXT_NOISE_IDLEADCSVLDNOD	26
#define MXT_NOISE_MINGCLIMIT		27
#define MXT_NOISE_MAXGCLIMIT		28
#define MXT_NOISE_T9SETTINGS		34

/* MXT_T62_MESSAGE field */
#define MXT_CALCFG1_STYAOFF		0x04
#define MXT_MINNLTHR_CMD		0xFE
#define T62_MINNLTHR			245

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1

/* MXT_SPT_GOLDENREFERENCES_T66 */
#define MXT_SPT_CTRL		0
#define MXT_SPT_FCALFAILTHR	1
#define MXT_SPT_FCALDRIFTCNT	2
#define MXT_SPT_FCALDRIFTCOEF	3
#define MXT_SPT_FCALDRIFTLIM	4
#define T66_CMD_SIZE			4

/* MXT_T66_MESSAGE field */
#define MXT_CMD_PRIME			0x04
#define MXT_MESSAGE_PRIME		0x02
#define MXT_CMD_GENERATE		0x08
#define MXT_MESSAGE_GENERATE	0x04
#define MXT_CMD_STORE			0x0c
#define MXT_MESSAGE_STORE		0x20
#define MXT_FCALCMD_STATE_MASK	0x0c
#define MXT_FCALSEQTO			0x10
#define MXT_FCALPASS			0x40
#define MXT_FCALFAIL			0x80
#define MXT_FCALSEQERR			0x08

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_RESET_ORDER		0x01
#define MXT_CALIBRATE_ORDER	0x01
#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_TIME		25	/* msec */
#define MXT224_RESET_TIME	65	/* msec */
#define MXT224E_RESET_TIME	22	/* msec */
#define MXT1386_RESET_TIME	250	/* msec */
#define MXT224S_RESET_TIME	250	/* msec */
#define MXT_RESET_TIME		250	/* msec */
#define MXT_MAX_RESET_TIME	160	/* msec */

#define MXT_FWRESET_TIME	175	/* msec */

#define MXT_WAKE_TIME		25

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA		0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK		0x02
#define MXT_FRAME_CRC_FAIL		0x03
#define MXT_FRAME_CRC_PASS		0x04
#define MXT_APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK		0x3f
#define MXT_BOOT_EXTENDED_ID	(1 << 5)
#define MXT_BOOT_ID_MASK	0x1f

#define MXT_CTRL		0
#define MXT_RPTEN		(1 << 1)

/* Current status */
#define MXT_COMSERR		(1 << 2)
#define MXT_CFGERR		(1 << 3)
#define MXT_CAL			(1 << 4)
#define MXT_SIGERR		(1 << 5)
#define MXT_OFL			(1 << 6)
#define MXT_RESET		(1 << 7)

/* Touch status */
#define MXT_SUPPRESS		(1 << 1)
#define MXT_AMP			(1 << 2)
#define MXT_VECTOR		(1 << 3)
#define MXT_MOVE		(1 << 4)
#define MXT_RELEASE		(1 << 5)
#define MXT_PRESS		(1 << 6)
#define MXT_DETECT		(1 << 7)

/* Touch orient bits */
#define MXT_XY_SWITCH		(1 << 0)
#define MXT_X_INVERT		(1 << 1)
#define MXT_Y_INVERT		(1 << 2)

/* Touch suppression */
#define MXT_TCHSUP_ACTIVE      (1 << 0)

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0x14
#define MXT_MAX_PRESSURE	0xff
#define MXT_MAX_FINGER		8
#define T7_BACKUP_SIZE		2
#define MXT_MAX_RW_TRIES	10
#define MXT_BLOCK_SIZE		256
#define MXT_MAX_RST_TRIES	2
#define ESD_POLLING_TIME	5000	/* 5 seconds */
#define MXT_MAX_ERR_CNT		255
#define MXT_MAX_CHK_TRIES	2

/* Firmware frame size including frame data and CRC */
#define MXT_SINGLE_FW_MAX_FRAME_SIZE	278


/* Define for diag */
#define MXT_DIAG_DATA_SIZE		130
#define MXT_DIAG_NUM_PAGE		6

/* sysfs ctrl command list */
#define MXT_SYSFS_LOG_FS		'l'
#define MXT_SYSFS_POLLING		'p'
#define MXT_SYSFS_INT_STATUS		'g'
#define MXT_SYSFS_WRITE			'w'
#define MXT_SYSFS_READ			'r'
#define MXT_SYSFS_T37			'd'
#define MXT_SYSFS_CONFIG		'c'
#define MXT_SYSFS_STATUS		's'
#define MXT_SYSFS_CONFIG_NV		'n'
#define MXT_SYSFS_IRQ			'i'
#define MXT_SYSFS_I2C_WRITE		'a'
#define MXT_SYSFS_I2C_READ		'b'
#define MXT_SYSFS_SENSOR_SWITCH	'o'

/* Touchpanel State Mode */
#define TOUCHPANEL_STATE_SLEEP         (0)
#define TOUCHPANEL_STATE_RESUME_BIT    (1<<0)
#define TOUCHPANEL_STATE_PATENT_BIT    (1<<1)
#define TOUCHPANEL_STATE_SELFTEST_BIT  (1<<2)

/* Log Message Macro */
#define MXT_DEV_DBG(dev, fmt, arg...)	if(ts_log_level != 0) dev_warn(dev, fmt, ## arg)

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;

	/* to map object and message */
	u8 max_reportid;
	bool report_enable;
};

struct mxt_message {
	u8 reportid;
	u8 message[7];
};

struct mxt_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
};

struct mxt_address_pair {
	int bootloader;
	int application;
};

static const struct mxt_address_pair mxt_slave_addresses[] = {
	{ 0x24, 0x4a },
	{ 0 },
};

enum mxt_device_mode { INIT, APPMODE, BOOTLOADER };

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	const struct mxt_platform_data *pdata;
	struct mxt_object *object_table;
	struct mxt_info info;
	struct mxt_finger finger[MXT_MAX_FINGER];
	unsigned int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	u8 t7_data[T7_BACKUP_SIZE];
	u16 t7_start_addr;
	u8 t9_max_reportid;
	u8 t9_min_reportid;
	u8 t15_max_reportid;
	u8 t15_min_reportid;
	u8 t25_max_reportid;
	u8 t25_min_reportid;
	u8 t42_max_reportid;
	u8 t42_min_reportid;
	unsigned int max_x;
	unsigned int max_y;
	struct mutex lock;
	int state;
	u16 max_o_size;
	struct delayed_work esdwork;
	struct delayed_work calwork;
	struct delayed_work switchwork;
	struct cdev device_cdev;
	int device_major;
	struct class *device_class;
	enum ts_config_type config_status;
	enum ts_config_type config_status_last;
	struct ts_config_nv config_nv[TS_CONFIG_MAX];
	struct completion init_done;
	bool is_enable;
	bool is_set;
	enum mxt_device_mode device_mode;
	u8 err_cnt[4];
	int selftest_status;
	const u8 *config;
	size_t config_length;
	bool stylus_flag;
	struct ts_mode_flag mode_flag;
	struct ts_mode_flag mode_flag_last;
	bool reset_flag;
};

struct sysfs_data_{
	char command;
	u16 start_addr;
	u16 size;
} sdata;

struct swichable_config{
	u8 obj_type;
	u8 offset;
	u8 value;
};

enum mxt_log_type {
	MXT_LOG_MSG,
	MXT_LOG_CONF_STAT,
	MXT_LOG_RESTART,
	MXT_LOG_RESET_STATUS,
	MXT_LOG_ESD,
	MXT_LOG_DAEMON,
	MXT_LOG_SUSPEND,
	MXT_LOG_RESUME,
	MXT_LOG_CFGERR,
	MXT_LOG_RPTEN,
	MXT_LOG_IC_RESET,
	MXT_LOG_REPORT,
	MXT_LOG_INTERRUPT_START,
	MXT_LOG_INTERRUPT_END,
	MXT_LOG_READ_MESSAGE,
	MXT_LOG_MAX,
};

enum mxt_err_type {
	MXT_ERR_CFGERR,
	MXT_ERR_RESET,
	MXT_ERR_MISBEHAVING,
	MXT_ERR_ESD,
	MXT_ERR_MAX,
};

/* workqueue for esd recovery */
static struct workqueue_struct *mxt_wq;

/* t37 data pointer for debug mode */
u8 *t37_data = NULL;

bool file_header = false;

static struct wake_lock touchpanel_wake_lock;

static int mxt_log_object(struct mxt_data *data, struct file *fp);
static int mxt_enable(struct mxt_data *data);
static void mxt_disable(struct mxt_data *data);
static int mxt_calibrate(struct mxt_data *data);
static int mxt_change_mode(struct device *dev, struct ts_mode_flag mode_flag);

static int mxt_suspend(struct device *dev);
static int mxt_resume(struct device *dev);

static void mxt_write_log(struct mxt_data *data, enum mxt_log_type type,
						void *arg)
{
	struct device *dev = &data->client->dev;
	struct file *fp;
	mm_segment_t old_fs;
	char *filename = "/data/local/tmp/ts_log";
	char buf[100];
	int len;
	int error;
	struct path path;
	struct mxt_message *message;
	struct ts_log_data *log;
	unsigned long long t;
	unsigned long nanosec_rem;
	u8 *val;
	int *id;

	switch (ts_log_file_enable) {
	case 0:
		return;
	case 1:
		switch (type) {
		case MXT_LOG_REPORT:
		case MXT_LOG_INTERRUPT_START:
		case MXT_LOG_INTERRUPT_END:
		case MXT_LOG_READ_MESSAGE:
			return;
		default:
			break;
		}
		break;
	case 2:
		switch (type) {
		case MXT_LOG_INTERRUPT_START:
		case MXT_LOG_INTERRUPT_END:
		case MXT_LOG_READ_MESSAGE:
			return;
		default:
			break;
		}
		break;
	case 3:
	case 4:
		switch (type) {
		case MXT_LOG_MSG:
		case MXT_LOG_INTERRUPT_START:
		case MXT_LOG_INTERRUPT_END:
		case MXT_LOG_READ_MESSAGE:
			return;
		default:
			break;
		}
		break;
	case 5:
		switch (type) {
		case MXT_LOG_MSG:
			return;
		default:
			break;
		}
		break;
	default:
		break;
	}

	mutex_lock(&file_lock);
	/* change to KERNEL_DS address limit */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	error = kern_path(filename, LOOKUP_FOLLOW, &path);
	if(error) {
		fp = filp_open(filename, O_CREAT, S_IRUGO|S_IWUSR);
		MXT_DEV_DBG(dev, "created /data/local/tmp/ts_log.\n");
		file_header = false;
	} else
		fp = filp_open(filename, O_WRONLY | O_APPEND, 0);

	if (!file_header) {
		len = sprintf(buf, "Time,Core,Report ID,Data01,Data02,"
					"Data03,Data04,Data05,Data06,Data07\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		memset(buf, '\0', sizeof(buf));
		file_header = true;
	}

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	len = sprintf(buf, "[%5lu.%06lu],Core[%d],", (unsigned long) t,
					nanosec_rem / 1000, smp_processor_id());
	fp->f_op->write(fp, buf, len, &(fp->f_pos));
	memset(buf, '\0', sizeof(buf));

	switch (type) {
	case MXT_LOG_MSG:
		message = arg;
		len = sprintf(buf, "%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\n",
							message->reportid,
							message->message[0],
							message->message[1],
							message->message[2],
							message->message[3],
							message->message[4],
							message->message[5],
							message->message[6]);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_CONF_STAT:
		len = sprintf(buf, "Change config_status[%d]\n", data->config_status);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		mxt_log_object(data, fp);
		break;
	case MXT_LOG_RESTART:
		len = sprintf(buf, "Controller IC Restart!\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_RESET_STATUS:
		len = sprintf(buf, "Reset status in Touch Screen Driver!\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_ESD:
		len = sprintf(buf, "Can't Read Register!\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_DAEMON:
		log = arg;
		len = sprintf(buf, "ts_daemon log[%X][%X]\n", log->flag, log->data);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_SUSPEND:
		len = sprintf(buf, "Suspend.\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_RESUME:
		len = sprintf(buf, "Resume.\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_CFGERR:
		len = sprintf(buf, "Received CFGERR!\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_RPTEN:
		val = arg;
		len = sprintf(buf, "Received T[%d] message even "
					"though RPTEN is not set!\n", *val);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_IC_RESET:
		len = sprintf(buf, "Sensor IC reset!\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_REPORT:
		id = arg;
		len = sprintf(buf, "Sync Report,slot[%d],status[%X],x[%d]"
				   ",y[%d],pressure[%d],area[%d]\n",
				   *id,
				   data->finger[*id].status,
				   data->finger[*id].x,
				   data->finger[*id].y,
				   data->finger[*id].pressure,
				   data->finger[*id].area);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_INTERRUPT_START:
		len = sprintf(buf, "mxt_interrupt() is called.\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_INTERRUPT_END:
		len = sprintf(buf, "mxt_interrupt() is completed.\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	case MXT_LOG_READ_MESSAGE:
		len = sprintf(buf, "mxt_read_message() is completed.\n");
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		break;
	default:
		break;
	}

	/* close file before return */
	if (fp)
		filp_close(fp, current->files);
	/* restore previous address limit */
	set_fs(old_fs);
	mutex_unlock(&file_lock);
}

static void mxt_dump_message(struct device *dev,
				  struct mxt_message *message)
{
	if (message->reportid != 0xFF) {
		MXT_DEV_DBG(dev, "reportid:\t0x%x\n", message->reportid);
		MXT_DEV_DBG(dev, "message1:\t0x%x\n", message->message[0]);
		MXT_DEV_DBG(dev, "message2:\t0x%x\n", message->message[1]);
		MXT_DEV_DBG(dev, "message3:\t0x%x\n", message->message[2]);
		MXT_DEV_DBG(dev, "message4:\t0x%x\n", message->message[3]);
		MXT_DEV_DBG(dev, "message5:\t0x%x\n", message->message[4]);
		MXT_DEV_DBG(dev, "message6:\t0x%x\n", message->message[5]);
		MXT_DEV_DBG(dev, "message7:\t0x%x\n", message->message[6]);
	}
}

static int mxt_get_bootloader_version(struct i2c_client *client, u8 val)
{
	u8 buf[3];

	if (val | MXT_BOOT_EXTENDED_ID)	{
		MXT_DEV_DBG(&client->dev,
				"Retrieving extended mode ID information");

		if (i2c_master_recv(client, &buf[0], 3) != 3) {
			dev_err(&client->dev, "%s: i2c recv failed\n",
								__func__);
			return -EIO;
		}

		MXT_DEV_DBG(&client->dev, "Bootloader ID:%d Version:%d",
							buf[1], buf[2]);

		return buf[0];
	} else {
		MXT_DEV_DBG(&client->dev, "Bootloader ID:%d",
					val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_switch_to_bootloader_address(struct mxt_data *data)
{
	int i;
	struct i2c_client *client = data->client;

	if (data->device_mode == BOOTLOADER) {
		MXT_DEV_DBG(&client->dev, "Already in BOOTLOADER state\n");
		return -EINVAL;
	}

	for (i = 0; mxt_slave_addresses[i].application != 0;  i++) {
		if (mxt_slave_addresses[i].application == client->addr) {
			MXT_DEV_DBG(&client->dev,
				"Changing to bootloader address: %02x -> %02x",
				client->addr,
				mxt_slave_addresses[i].bootloader);
			client->addr = mxt_slave_addresses[i].bootloader;
			data->device_mode = BOOTLOADER;
			return 0;
		}
	}

	dev_err(&client->dev, "Address 0x%02x not found in address table",
								client->addr);
	return -EINVAL;
}

static int mxt_switch_to_appmode_address(struct mxt_data *data)
{
	int i;
	struct i2c_client *client = data->client;

	if (data->device_mode == APPMODE) {
		dev_err(&client->dev, "Already in APPMODE state\n");
		return -EINVAL;
	}

	for (i = 0; mxt_slave_addresses[i].application != 0;  i++) {
		if (mxt_slave_addresses[i].bootloader == client->addr) {
			MXT_DEV_DBG(&client->dev,
				"Changing to application mode address: "
							"0x%02x -> 0x%02x",
				client->addr,
				mxt_slave_addresses[i].application);

			client->addr = mxt_slave_addresses[i].application;
			data->device_mode = APPMODE;
			return 0;
		}
	}

	dev_err(&client->dev, "Address 0x%02x not found in address table",
								client->addr);
	return -EINVAL;
}

static int mxt_check_bootloader(struct i2c_client *client,
				     unsigned int state)
{
	u8 val;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		val = mxt_get_bootloader_version(client, val);
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(&client->dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	MXT_DEV_DBG(&client->dev,
		"%s: val = %X, state = %X\n", __func__, val, state);
	if (val != state) {
		dev_err(&client->dev, "Invalid bootloader mode state %X\n",
			val);
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_fw_write(struct i2c_client *client,
			     const u8 *data, unsigned int frame_size)
{
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int mxt_bundle_fw_write(struct i2c_client *client,
			     const u8 *data, unsigned int frame_size)
{
	int ret;
	int i;
	MXT_DEV_DBG(&client->dev, "%s: mxt_bundle_fw_write(%X)\n",
						__func__, client->addr);
	while (frame_size > MXT_BLOCK_SIZE) {
		for (i = 0; i < MXT_BLOCK_SIZE; i++)
			MXT_DEV_DBG(&client->dev, " %02X", data[i]);
		ret = mxt_fw_write(client, data, MXT_BLOCK_SIZE);
		if (ret)
			return ret;
		frame_size -= MXT_BLOCK_SIZE;
		data += MXT_BLOCK_SIZE;
	}
	for (i = 0; i < frame_size; i++)
		MXT_DEV_DBG(&client->dev, " %02X", data[i]);
	ret = mxt_fw_write(client, data, frame_size);
	MXT_DEV_DBG(&client->dev, "\n");

	return ret;
}

static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];
	int i = 0;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	do {
		if (i2c_transfer(client->adapter, xfer, 2) == 2)
			return 0;
		msleep(MXT_WAKE_TIME);
	} while (++i < MXT_MAX_RW_TRIES);

	dev_err(&client->dev, "%s: i2c transfer failed\n", __func__);
	return -EIO;
}

static int mxt_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	return __mxt_read_reg(client, reg, 1, val);
}

static int __mxt_write_reg(struct i2c_client *client,
		    u16 addr, u16 length, u8 *value)
{
	u8 buf[MXT_BLOCK_SIZE + 2];
	int i, tries = 0;

	if (length > MXT_BLOCK_SIZE)
		return -EINVAL;

	buf[0] = addr & 0xff;
	buf[1] = (addr >> 8) & 0xff;
	for (i = 0; i < length; i++)
		buf[i + 2] = *value++;

	do {
		if (i2c_master_send(client, buf, length + 2) == (length + 2))
			return 0;
		msleep(MXT_WAKE_TIME);
	} while (++tries < MXT_MAX_RW_TRIES);

	dev_err(&client->dev, "%s: i2c send failed\n", __func__);
	return -EIO;
}
static int __mxt_read_fw(struct i2c_client *client,
			       u16 len, void *val)
{
	int i = 0;

	unsigned short addr;

	addr = client->addr;
	client->addr = 0x24;
	MXT_DEV_DBG(&client->dev, "%s: addr = %x, client->addr = %x\n",
				__func__, addr, client->addr);
	do {
		if (i2c_master_recv(client, val, len) == len) {
			client->addr = addr;
			return 0;
		}
		msleep(MXT_WAKE_TIME);
	} while (++i < MXT_MAX_RW_TRIES);

	client->addr = addr;

	dev_err(&client->dev, "%s: i2c transfer failed\n", __func__);
	return -EIO;
}

static int __mxt_write_fw(struct i2c_client *client,
		    u16 length, u8 *value)
{
	u8 buf[MXT_BLOCK_SIZE];
	int i, tries = 0;
	unsigned short addr;

	if (length > MXT_BLOCK_SIZE)
		return -EINVAL;

	for (i = 0; i < length; i++)
		buf[i] = *value++;

	addr = client->addr;
	client->addr = 0x24;
	MXT_DEV_DBG(&client->dev, "%s: addr = %x, client->addr = %x\n",
				__func__, addr, client->addr);
	do {
		if (i2c_master_send(client, buf, length) == length) {
			client->addr = addr;
			return 0;
		}
		msleep(MXT_WAKE_TIME);
	} while (++tries < MXT_MAX_RW_TRIES);

	client->addr = addr;

	dev_err(&client->dev, "%s: i2c send failed\n", __func__);
	return -EIO;
}

static int mxt_bundle_read_fw(struct mxt_data *data, u16 size, u8 *dp)
{
	int ret;
	while (size > MXT_BLOCK_SIZE) {
		ret = __mxt_read_fw(data->client, MXT_BLOCK_SIZE, dp);
		if (ret)
			return ret;
		size -= MXT_BLOCK_SIZE;
		dp += MXT_BLOCK_SIZE;
	}
	ret = __mxt_read_fw(data->client, size, dp);
	return ret;
}

static int mxt_bundle_write_fw(struct mxt_data *data, u16 size, u8 *dp)
{
	int ret;
	while (size > MXT_BLOCK_SIZE) {
		ret = __mxt_write_fw(data->client, MXT_BLOCK_SIZE, dp);
		if (ret)
			return ret;
		size -= MXT_BLOCK_SIZE;
		dp += MXT_BLOCK_SIZE;
	}
	ret = __mxt_write_fw(data->client, size, dp);
	return ret;
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	return __mxt_write_reg(client, reg, 1, &val);
}

static int mxt_bundle_write_reg(struct mxt_data *data, u16 size, u16 addr, u8 *dp)
{
	int ret;
	while (size > MXT_BLOCK_SIZE) {
		ret = __mxt_write_reg(data->client, addr, MXT_BLOCK_SIZE, dp);
		if (ret)
			return ret;
		size -= MXT_BLOCK_SIZE;
		addr += MXT_BLOCK_SIZE;
		dp += MXT_BLOCK_SIZE;
	}
	ret = __mxt_write_reg(data->client, addr, size, dp);
	return ret;
}

static int mxt_read_object_table(struct i2c_client *client,
				      u16 reg, u8 *object_buf)
{
	return __mxt_read_reg(client, reg, MXT_OBJECT_SIZE,
				   object_buf);
}

static struct mxt_object *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type\n");
	return NULL;
}

static int mxt_read_message(struct mxt_data *data,
				 struct mxt_message *message)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, MXT_GEN_MESSAGE_T5);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return __mxt_read_reg(data->client, reg,
			sizeof(struct mxt_message), message);
}

static int mxt_bundle_read_object(struct mxt_data *data, u8 type, u8 *val)
{
	struct mxt_object *object;
	u16 reg;
	int num;
	int ret;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	num = (object->size + 1) * (object->instances + 1);

	while (num > MXT_BLOCK_SIZE) {
		ret = __mxt_read_reg(data->client, reg, MXT_BLOCK_SIZE, val);
		if (ret)
			return ret;
		num -= MXT_BLOCK_SIZE;
		reg += MXT_BLOCK_SIZE;
		val += MXT_BLOCK_SIZE;
	}
	ret = __mxt_read_reg(data->client, reg, num, val);

	return ret;
}

static int mxt_bundle_write_object(struct mxt_data *data, u8 type, u8 *dp)
{
	struct mxt_object *object;
	u16 addr;
	int size;
	int ret;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	addr = object->start_address;
	size = (object->size + 1) * (object->instances + 1);

	ret = mxt_bundle_write_reg(data, size, addr, dp);

	return ret;
}

static int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return __mxt_read_reg(data->client, reg + offset, 1, val);
}

static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return mxt_write_reg(data->client, reg + offset, val);
}

static void mxt_check_rpten(struct mxt_data *data,
				struct mxt_object *object, u8 *config)
{

	if (object->num_report_ids>0) {
		object->report_enable = *config & MXT_RPTEN;
		MXT_DEV_DBG(&data->client->dev, "%s: t[%d] RPTEN is [%d]\n"
					, __func__
					, object->type
					, object->report_enable);
	}
}

static void mxt_diag_store(struct mxt_data *data)
{
	struct mxt_finger *finger = data->finger;
	struct device *dev = &data->client->dev;
	int i;
	int cnt = 0;

	mutex_lock(&diag_lock);
	memset(diag_data, 0, sizeof(struct ts_diag_type));
	for (i = 0; i < MXT_MAX_FINGER; i++) {
		if (!finger[i].status)
			continue;
		diag_data->ts[i].x = finger[i].x;
		diag_data->ts[i].y = finger[i].y;
		diag_data->ts[i].width = finger[i].area;
		MXT_DEV_DBG(dev, "%s: diag_data.ts[%d].x = %d\n", __func__, i,
							diag_data->ts[i].x);
		MXT_DEV_DBG(dev, "%s: diag_data.ts[%d].y = %d\n", __func__, i,
							diag_data->ts[i].y);
		MXT_DEV_DBG(dev, "%s: diag_data.ts[%d].width = %d\n", __func__, i,
							diag_data->ts[i].width);
		cnt++;
	}
	diag_data->diag_count = cnt;
	MXT_DEV_DBG(dev, "%s: diag_data.diag_count = %d\n", __func__,
							diag_data->diag_count);
	mutex_unlock(&diag_lock);
}

static void mxt_input_report(struct mxt_data *data)
{
	struct mxt_finger *finger = data->finger;
	struct input_dev *input_dev = data->input_dev;
	int finger_num = 0;
	int id;

	for (id = 0; id < MXT_MAX_FINGER; id++) {
		if (!finger[id].status)
			continue;

		if (!ts_event_control) {
			input_mt_slot(input_dev, id);
			MXT_DEV_DBG(&data->client->dev, "input event : "
							"slot[%02x]\n", id);
			input_mt_report_slot_state(input_dev, MT_TOOL_FINGER,
					finger[id].status != MXT_RELEASE);

			if (finger[id].status != MXT_RELEASE) {
				finger_num++;
				if (finger[id].area <= MXT_MAX_AREA)
					input_report_abs(input_dev,
							 ABS_MT_TOUCH_MAJOR,
							 finger[id].area);
				else
					input_report_abs(input_dev,
							 ABS_MT_TOUCH_MAJOR,
							 MXT_MAX_AREA);
				input_report_abs(input_dev, ABS_MT_POSITION_X,
						finger[id].x);
				input_report_abs(input_dev, ABS_MT_POSITION_Y,
						finger[id].y);
				input_report_abs(input_dev, ABS_MT_PRESSURE,
						finger[id].pressure);
				input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR,
							 MXT_MAX_AREA);
				MXT_DEV_DBG(&data->client->dev, "input event : "
					"touch_major[%d], pos_x[%d], pos_y[%d],"
					" pressure[%d]\n",
					finger[id].area,
					finger[id].x,
					finger[id].y,
					finger[id].pressure);
			} else {
				finger[id].status = 0;
			}
		} else {
			if (finger[id].status == MXT_RELEASE)
				finger[id].status = 0;
		}
	}

	if (!ts_event_control) {
		input_report_key(input_dev, BTN_TOUCH, finger_num > 0);
		input_sync(input_dev);
	}
	if (ts_diag_start_flag)
		mxt_diag_store(data);
}

static void mxt_input_report_clear(struct mxt_data *data)
{
	struct mxt_finger *finger = data->finger;
	int id;

	for (id = 0; id < MXT_MAX_FINGER; id++) {
		if (!finger[id].status)
			continue;
		finger[id].status = MXT_RELEASE;
		MXT_DEV_DBG(&data->client->dev, "%s:[%d] released\n", __func__, id);
		mxt_input_report(data);
	}
}

static void mxt_handle_touch_stylus(struct mxt_data *data, int area)
{
	struct device *dev = &data->client->dev;
	/* stylus is active when area is 0 */
	if (area == 0) {
		dev_warn(dev, "%s: touch stylus active\n", __func__);
		if( !data->stylus_flag 
		&&  !(data->state & TOUCHPANEL_STATE_PATENT_BIT) ){
			data->stylus_flag=true;
			queue_delayed_work(mxt_wq, &data->calwork, msecs_to_jiffies(500));
		}
	}
	else {
		if( data->stylus_flag ){
			data->stylus_flag=false;
			cancel_delayed_work(&data->calwork);
		}
	}
}

static void mxt_handle_mode_state(struct mxt_data *data, struct mxt_message *message)
{
	struct device *dev = &data->client->dev;
	int status, area;

	if ( message != NULL ){
		status = message->message[0];
		area = message->message[4];
		
		if ( status & MXT_RELEASE && area != 0 ) {
			data->mode_flag.state = TS_MODE_STATE_MOVE;
			mxt_change_mode(dev, data->mode_flag);
		}
	}
	else {
		data->mode_flag.state = TS_MODE_STATE_BOOT;
		mxt_change_mode(dev, data->mode_flag);
		if (mxt_wq) {
			ts_config_switching = 0;
			queue_delayed_work(mxt_wq, &data->switchwork, msecs_to_jiffies(500));
		}
	}
}

static void mxt_input_touchevent(struct mxt_data *data,
				      struct mxt_message *message, int id)
{
	struct mxt_finger *finger = data->finger;
	struct device *dev = &data->client->dev;
	u8 status = message->message[0];
	int x;
	int y;
	int area;
	int pressure;

	/* Check the touch status and area for touch mode_state */
	mxt_handle_mode_state(data, message);

	/* Check the touch is present on the screen */
	if (!(status & MXT_DETECT)) {
		if (status & MXT_RELEASE || status & MXT_SUPPRESS) {
			MXT_DEV_DBG(dev, "[%d] released\n", id);
			finger[id].status = MXT_RELEASE;
			mxt_input_report(data);
			mxt_write_log(data, MXT_LOG_REPORT, &id);
		}
		return;
	}

	x = (message->message[1] << 4) | ((message->message[3] >> 4) & 0xf);
	y = (message->message[2] << 4) | ((message->message[3] & 0xf));
	if (data->max_x < 1024)
		x = x >> 2;
	if (data->max_y < 1024)
		y = y >> 2;

	area = message->message[4];
	pressure = message->message[5];

	MXT_DEV_DBG(dev, "[%d] %s x: %d, y: %d, area: %d, pressure: %d\n", id,
		status & MXT_MOVE ? "moved" : "pressed",
		x, y, area, pressure);

	finger[id].status = status & MXT_MOVE ?
				MXT_MOVE : MXT_PRESS;
	finger[id].x = x;
	finger[id].y = y;
	finger[id].area = area;
	finger[id].pressure = pressure;

	/* Check the touch area for touch floating */
	mxt_handle_touch_stylus(data, area);
	
	mxt_input_report(data);
	mxt_write_log(data, MXT_LOG_REPORT, &id);
}

static int mxt_check_reset_report(struct mxt_data *data)
{
	struct mxt_message message;
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int ret = 0;
	u8 t6_reportid;

	object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!object) {
		dev_err(dev, "Failed to get T6 object!\n");
		ret = -EIO;
		goto error;
	}
	t6_reportid = object->max_reportid;

	do {
		ret = mxt_read_message(data, &message);
		if (ret) {
			dev_err(dev, "Failed to read message!\n");
			goto error;
		}
		if ((t6_reportid == message.reportid) &&
		    (message.message[0] & MXT_RESET)) {
			MXT_DEV_DBG(dev, "%s: Recieved T6 Reset Report.\n",
								__func__);
			return 0;
		} else {
			mxt_dump_message(dev, &message);
			ret = -EIO;
		}
	} while (message.reportid != 0xff);

error:
	dev_err(dev, "%s:Failed to get T6 Reset Report!\n", __func__);
	return ret;
};

static int mxt_wait_interrupt(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int i;
	int val;

	for (i = 0; i < (MXT_MAX_RESET_TIME / 10); i++) {
		val = gpio_get_value(MXT_TS_GPIO_IRQ);
		if (val == 0) {
			MXT_DEV_DBG(dev, "%s: %d0 ms wait.\n", __func__, i);
			return 0;
		}
		msleep(10);
	}

	return -ETIME;
}

static int mxt_reset_and_delay(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int i;
	int error = 0;

	for (i = 0; i < MXT_MAX_RST_TRIES; i++) {
		mxt_reset_hw();
		if (error) {
			dev_err(dev, "%s: Failed to reset hardware!\n", __func__);
			goto done;
		}
		error = mxt_wait_interrupt(data);
		if (!error) {
			MXT_DEV_DBG(dev, "%s: Received reset interrupt.\n",
								__func__);
			goto done;
		}
		dev_err(dev, "%s: Reset Retry %d times.\n", __func__, i + 1);
	}
	dev_err(dev, "%s: Interrupt wait time out!\n", __func__);
done:
	return error;
}

static int mxt_find_index_from_object(struct device *dev, 
     u8 *config, u16 size, struct mxt_object *object)
{
	int index = 0;
	int num = (object->size + 1) * (object->instances + 1);

	do {
		if (config[index] == object->type) {
			MXT_DEV_DBG(dev, "%s: match. data[%d](%d)\n", __func__, index, config[index]);
			return index;
		} else {
			index += config[index + 1] + 2;
		}
	} while ((index + num + 2) <= size);

	MXT_DEV_DBG(dev, "%s: Not found [%d] from config table\n", __func__, object->type);
	return -1;
}

static long mxt_switch_config_exec(struct mxt_data *data)
{
	struct ts_config_nv *config_nv;
	struct ts_config_nv *config_nv_last;
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	long ret = 0;
	int i, j;
	int index = 0;
	int offset;
	int num;

	if (!ts_config_switching) {
		MXT_DEV_DBG(dev, "%s: Skip. Disabled config switching.\n",
								__func__);
		data->config_status = data->config_status_last;
		return 0;
	}
	if (data->config_status_last == data->config_status) {
		MXT_DEV_DBG(dev, "%s: Skip. same status.\n", __func__);
		return 0;
	}

	config_nv = &data->config_nv[data->config_status];
	config_nv_last = &data->config_nv[data->config_status_last];

	if (!config_nv->data || !config_nv_last->data) {
		dev_info(dev, "%s: No nv data. Skip switch config.\n",
								__func__);
		data->config_status = data->config_status_last;
		return 0;
	}

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		
		index = mxt_find_index_from_object(dev, config_nv->data, config_nv->size, object);
		if (index < 0){
			MXT_DEV_DBG(dev, "%s: skip config t[%d]\n", __func__, object->type);
			continue;
		}

		num = (object->size + 1) * (object->instances + 1);
		if (((index + num + 2) > config_nv->size) ||
		    ((index + num + 2) > config_nv_last->size)) {
			dev_err(dev, "%s: Not enough config data!\n", __func__);
			dev_err(dev, "%s: Canceled after t[%d]!\n",
						__func__, object->type);
			return 0;
		}

		MXT_DEV_DBG(dev, "%s: config_nv[%d](%d), type(%d)\n", __func__,
				index, config_nv->data[index], object->type);
		index += 2;
		for (j = 0; j < num; j++) {
			offset = index + j;
			ret = memcmp(&config_nv->data[offset],
				     &config_nv_last->data[offset], 1);
			if (ret) {
				ret = mxt_write_object(data, object->type, j,
						 config_nv->data[offset]);
				if(ret){
					dev_err(dev, "%s: Error Switch Config Exec\n", __func__);
					data->config_status = data->config_status_last;
					return ret;
				}
				MXT_DEV_DBG(dev, "%s: write_object[%d]=%02X\n",
						__func__, object->type,
						config_nv->data[offset]);
			}
		}
		mxt_check_rpten(data, object, &config_nv->data[index]);
	}
	MXT_DEV_DBG(dev, "%s: Switched to config_status [%d]\n", __func__,
							data->config_status);
	mxt_write_log(data, MXT_LOG_CONF_STAT, NULL);
	return ret;
}

static long mxt_switch_config(struct device *dev, enum ts_config_type status)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	long ret = 0;

	data->config_status_last = data->config_status;
	data->config_status = status;

	ret = mxt_switch_config_exec(data);

	return ret;
}

static int mxt_check_reg_exec(struct mxt_data *data, u8 *config, size_t length)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	size_t size;
	int index = 0;
	int i;
	u8 *o_data;
	int ret = 0;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		index = mxt_find_index_from_object(dev, config, length, object);
		if (index < 0){
			MXT_DEV_DBG(dev, "%s: skip config t[%d]\n", __func__, object->type);
			continue;
		}

		size = (object->size + 1) * (object->instances + 1);
		if ((index + size) > length) {
			dev_err(dev, "Not enough config data!\n");
			dev_err(dev, "%s: Canceled after t[%d]!\n",
						__func__, object->type);
			return 0;
		}

		MXT_DEV_DBG(dev, "%s: config[%3d]=%3d, object->type=%3d\n", 
								__func__, index, config[index], object->type);
		MXT_DEV_DBG(dev, "%s: config[%3d]=%3d, object->size=%3d\n", 
								__func__, index+1, config[index+1], object->size);

		index += 2;

		o_data = kcalloc(size, sizeof(u8), GFP_KERNEL);
		if (!o_data) {
			dev_err(dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		mxt_bundle_read_object(data, object->type, o_data);
		if (memcmp(o_data, &config[index], size)) {
			MXT_DEV_DBG(dev, "%s: t[%d] is mismatched\n",
						__func__, object->type);
			ret = mxt_bundle_write_object(data,
						object->type,
						&config[index]);
			if (ret) {
				dev_err(dev, "%s: Register write error!\n",
								__func__);
				kfree(o_data);
				o_data = NULL;
				return ret;
			}
			if (object->type == MXT_GEN_POWER_T7)
				memcpy(&data->t7_data,
				       &config[index],
				       T7_BACKUP_SIZE);
			data->is_set = true;
		} else {
			MXT_DEV_DBG(dev, "%s: t[%d] is matched\n",
						__func__, object->type);
		}
		mxt_check_rpten(data, object, &config[index]);
		kfree(o_data);
		o_data = NULL;
	}
	return ret;
}

static int mxt_check_reg_nv(struct mxt_data *data)
{
	struct ts_config_nv *nv = &data->config_nv[TS_CONFIG_BOOT_NORMAL];
	struct device *dev = &data->client->dev;
	int ret;

	if (!nv->data) {
		MXT_DEV_DBG(dev, "No nv data, skipping.\n");
		return 0;
	}

	data->config_status = TS_CONFIG_BOOT_NORMAL;
	data->mode_flag_last.state = TS_MODE_STATE_BOOT;
	data->mode_flag_last.call = TS_MODE_CALL_OFF;

	MXT_DEV_DBG(dev, "%s: mxt_check_reg_exec(size = %d).\n",
						__func__, nv->size);
	ret = mxt_check_reg_exec(data, nv->data, nv->size);

	return ret;
}

static int mxt_check_reg_config(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret;

	if (!data->config) {
		MXT_DEV_DBG(dev, "No cfg data defined, skipping.\n");
		return 0;
	}

	ret = mxt_check_reg_exec(data, (u8 *)data->config,
						data->config_length);

	return ret;
}

static int mxt_backup_nv(struct mxt_data *data)
{
	int error;
	u8 command_register;
	int timeout_counter = 0;

	/* Backup to memory */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_BACKUPNV,
			MXT_BACKUP_VALUE);
	msleep(MXT_BACKUP_TIME);

	do {
		error = mxt_read_object(data, MXT_GEN_COMMAND_T6,
					MXT_COMMAND_BACKUPNV,
					&command_register);
		if (error)
			return error;

		usleep_range(1000, 2000);

	} while ((command_register != 0) && (++timeout_counter <= 100));

	if (timeout_counter > 100) {
		dev_err(&data->client->dev, "No response after backup!\n");
		return -EIO;
	}
	dev_info(&data->client->dev, "%s() is completed.\n", __func__);
	return 0;
}

static int mxt_reset_status(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret = 0;

	MXT_DEV_DBG(dev, "%s() is called.\n", __func__);

	/* Send release event as needed */
	mxt_input_report_clear(data);

	/* Check registers after reset */
	if (data->config_nv[TS_CONFIG_BOOT_NORMAL].data)
		ret = mxt_check_reg_nv(data);
	else
		ret = mxt_check_reg_config(data);
	if (!ret) {
		if (data->is_set) {
			dev_info(dev, "%s: NVM set.\n", __func__);
			/* Backup to memory */
			ret = mxt_backup_nv(data);
			if (ret)
				dev_err(dev, "%s: Fail to backup!\n", __func__);
			data->is_set = false;
		}
	} else {
		dev_err(dev, "%s: Fail to check registers after reset!\n",
								__func__);
		return ret;
	}

	/* Check and set lateset config */
	data->config_status_last = TS_CONFIG_BOOT_NORMAL;
	ret = (int)mxt_switch_config_exec(data);

	MXT_DEV_DBG(dev, "%s() is completed.\n", __func__);
	mxt_write_log(data, MXT_LOG_RESET_STATUS, NULL);
	return ret;
}

static int mxt_restart(struct mxt_data *data)
{
	int ret = 0;

	MXT_DEV_DBG(&data->client->dev, "%s() is called.\n", __func__);
	/* Reset, then wait interrupt */
	ret = mxt_reset_and_delay(data);
	if (ret)
		return ret;

	/* Check T6 reset report  */
	ret = mxt_check_reset_report(data);
	if (ret)
		return ret;

	MXT_DEV_DBG(&data->client->dev, "%s() is completed.\n", __func__);
	mxt_write_log(data, MXT_LOG_RESTART, NULL);

	return ret;
}

static int mxt_start(struct mxt_data *data)
{
	int error;

	/* restore the old power state values and reenable touch */
	error = __mxt_write_reg(data->client, data->t7_start_addr,
				T7_BACKUP_SIZE, data->t7_data);
	if (error < 0) {
		dev_err(&data->client->dev,
			"failed to restore old power state\n");
		return error;
	}

	return 0;
}

static int mxt_stop(struct mxt_data *data)
{
	int error;
	u8 t7_data[T7_BACKUP_SIZE] = {0};

	error = __mxt_write_reg(data->client, data->t7_start_addr,
				T7_BACKUP_SIZE, t7_data);
	if (error < 0) {
		dev_err(&data->client->dev,
			"failed to configure deep sleep mode\n");
		return error;
	}

	return 0;
}

static int mxt_calibrate(struct mxt_data *data)
{
	int ret = 0;

	ret = mxt_write_object(data, MXT_GEN_COMMAND_T6,
				MXT_COMMAND_CALIBRATE, MXT_CALIBRATE_ORDER);
	return ret;
}

static int mxt_change_mode(struct device *dev, struct ts_mode_flag mode_flag)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	enum ts_config_type config;
	long err = 0;
	
	MXT_DEV_DBG(dev, "%s() is called.\n", __func__);

	if (!ts_config_switching) {
		MXT_DEV_DBG(dev, "%s: Skip. Disabled config switching.\n", __func__);
		return 0;
	}
	
	if (mode_flag.state==data->mode_flag_last.state
	 && mode_flag.call==data->mode_flag_last.call ) {
		MXT_DEV_DBG(dev, "%s: Skip. same mode_flag.\n", __func__);
		return 0;
	}
	
	if(mode_flag.state==TS_MODE_STATE_MOVE){
		if(mode_flag.call==TS_MODE_CALL_STEP1){
			 config = TS_CONFIG_MOVE_CALL_STEP1;
		}
		else if(mode_flag.call==TS_MODE_CALL_STEP2){
			 config = TS_CONFIG_MOVE_CALL_STEP2;
		}
		else {
			 config = TS_CONFIG_MOVE_NORMAL;
		}
	}
	else {
		if(mode_flag.call==TS_MODE_CALL_STEP1){
			 config = TS_CONFIG_BOOT_CALL_STEP1;
		}
		else if(mode_flag.call==TS_MODE_CALL_STEP2){
			 config = TS_CONFIG_BOOT_CALL_STEP2;
		}
		else {
			 config = TS_CONFIG_BOOT_NORMAL;
		}
	}
	MXT_DEV_DBG(dev, "%s: config[%d], mode_state[%d], mode_call[%d]\n",
							__func__, config, mode_flag.state, mode_flag.call);

	err = mxt_switch_config(dev, config);
	if(!err){
		data->mode_flag_last = mode_flag;
	}
	
	return err;
}

static int mxt_change_state(struct mxt_data *data, int state)
{
	int old_status, new_status;
	int error=0;
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev = data->input_dev;
  
	MXT_DEV_DBG(dev, "%s: old_state=%x, new_state=%x\n", 
		__func__, data->state, state);
		
	if(data->state != TOUCHPANEL_STATE_SLEEP) old_status = 1;
	else old_status = 0;

	if(state != TOUCHPANEL_STATE_SLEEP) new_status = 1;
	else new_status = 0;
  
	if(old_status != new_status) {
		if (mxt_wq){
			cancel_delayed_work(&data->calwork);
		}
		
		if(new_status == 1) {
			MXT_DEV_DBG(dev, "%s: touchpanel active\n", __func__);

			if (input_dev->users) {
				mxt_input_report_clear(data);
				error = mxt_start(data);
				if (error < 0) {
					dev_err(dev, "mxt_start failed in touchpanel active\n");
					return error;
				}
				if (data->client->irq != -1) {
					enable_irq_wake(data->client->irq);
				}

				MXT_DEV_DBG(dev, "%s: T6 reset.\n", __func__);
				/* Write T6 reset */
				data->reset_flag = true;
				error = mxt_write_object(data, MXT_GEN_COMMAND_T6, MXT_COMMAND_RESET, MXT_RESET_ORDER);
				if (error) {
					dev_err(dev, "%s: Fail to software reset!\n", __func__);
				}

				MXT_DEV_DBG(dev, "%s done.\n", __func__);
			}

			if (mxt_wq) {
				queue_delayed_work(mxt_wq, &data->esdwork,
							 msecs_to_jiffies(ESD_POLLING_TIME));
			}
		}
		else {
			MXT_DEV_DBG(dev, "%s: touchpanel sleep\n", __func__);

			if (mxt_wq) {
				cancel_delayed_work_sync(&data->esdwork);
			}

			if (input_dev->users) {
				error = mxt_stop(data);
				if (error < 0) {
					dev_err(dev, "mxt_stop failed in touchpanel sleep\n");
					return error;
				}
				if (data->client->irq != -1) {
					disable_irq_wake(data->client->irq);
				}
				MXT_DEV_DBG(dev, "%s done.\n", __func__);
			}
		}
	}

	if(!(state & TOUCHPANEL_STATE_PATENT_BIT) 
	&& (data->state & TOUCHPANEL_STATE_PATENT_BIT)){
		if (mxt_wq) {
			queue_delayed_work(mxt_wq, &data->calwork, msecs_to_jiffies(500));
		}
	}

	data->state = state;

	return error;
}

static int mxt_error_check_process(struct mxt_data *data, struct mxt_message *message)
{

	struct device *dev = &data->client->dev;
	u8 reportid;
	u8 max_reportid;
	u8 min_reportid;
	struct mxt_object *object;
	int error = 0;
	u8 status = message->message[0];
	int i;

	MXT_DEV_DBG(dev, "Run %s!\n", __func__);

	reportid = message->reportid;

	if (reportid == 0xFF)
		return 0;

	/* Check if received T6 error message */
	object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!object) {
		dev_err(dev, "Failed to get T6 object!\n");
		error = -EIO;
		goto done;
	}
	max_reportid = object->max_reportid;
	min_reportid = max_reportid - object->num_report_ids + 1;
	if ((reportid >= min_reportid) && (reportid <= max_reportid)) {
		if (status & MXT_CFGERR) {
			dev_err(dev, "%s:Received GEN_COMMANDPROCESSOR_T6"
						"[%02X]!\n", __func__, status);
			mxt_write_log(data, MXT_LOG_CFGERR, NULL);
			if (data->err_cnt[MXT_ERR_CFGERR] <= MXT_MAX_ERR_CNT)
				data->err_cnt[MXT_ERR_CFGERR]++;
			goto restart;
		}
		if (status & MXT_RESET) {
			if(data->reset_flag){
				MXT_DEV_DBG(dev, "Received touchpanel resume\n");
				data->reset_flag = false;
				goto done;
			}
			else {
				dev_err(dev, "%s:Received GEN_COMMANDPROCESSOR_T6"
							"[%02X]!\n", __func__, status);
				mxt_write_log(data, MXT_LOG_IC_RESET, NULL);
				if (data->err_cnt[MXT_ERR_RESET] <= MXT_MAX_ERR_CNT)
					data->err_cnt[MXT_ERR_RESET]++;
				goto reset;
			}
		}
		if (status & MXT_CAL) {
			dev_warn(dev, "mxt Received calibration\n");
			mxt_handle_mode_state(data, NULL);
			goto done;
		}
		goto done;
	}

	/* Check if received invalid messages */
	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		if (object->num_report_ids>0)
			continue;

		max_reportid = object->max_reportid;
		min_reportid = max_reportid - object->num_report_ids + 1;
		if ((reportid >= min_reportid) &&
		    (reportid <= max_reportid) &&
		    (!object->report_enable)) {
			dev_err(dev, "%s:Received T[%d] message even though "
							"RPTEN is not set!\n"
							, __func__
							, object->type);
			mxt_write_log(data, MXT_LOG_RPTEN, &object->type);
			if (data->err_cnt[MXT_ERR_MISBEHAVING] <= MXT_MAX_ERR_CNT)
				data->err_cnt[MXT_ERR_MISBEHAVING]++;
			goto restart;
		} else if ((reportid >= min_reportid) &&
			   (reportid <= max_reportid) &&
			   (object->report_enable)) {
				MXT_DEV_DBG(dev, "%s:Received T[%d] message.\n"
							, __func__
							, object->type);
			goto done;
		}
	}
	dev_info(dev, "%s: Received unexpected report id[%02X]\n",
					__func__, message->reportid);
	goto done;
restart:
	/* Maybe abnormal state, then restart */
	error = mxt_restart(data);
	if (error)
		dev_err(dev, "Failed to restart!\n");
reset:
	/* Reset status in Touch Screen Driver */
	error = mxt_reset_status(data);
	if (error)
		dev_err(dev, "Failed to reset status!\n");
done:
	return error;
}

static void mxt_handle_touch_supression(struct mxt_data *data, u8 status)
{
	struct device *dev = &data->client->dev;
	if (status & MXT_TCHSUP_ACTIVE){
		MXT_DEV_DBG(dev, "%s: supression active\n", __func__);
		mxt_input_report_clear(data);
		if(!(data->state & TOUCHPANEL_STATE_PATENT_BIT)){
			if (mxt_wq) {
				queue_delayed_work(mxt_wq, &data->calwork, msecs_to_jiffies(500));
			}
		}
	}
}

static irqreturn_t mxt_interrupt(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;
	struct mxt_message message;
	struct device *dev = &data->client->dev;
	int id;
	u8 reportid;

	MXT_DEV_DBG(dev, "Core[%d]: %s is called.\n",
				smp_processor_id(), __func__);
	mxt_write_log(data, MXT_LOG_INTERRUPT_START, NULL);

	wake_lock_timeout(&touchpanel_wake_lock, 2 * HZ);
	mutex_lock(&data->lock);

	if (data->state!=TOUCHPANEL_STATE_SLEEP) {
		do {
			if (mxt_read_message(data, &message)) {
				dev_err(dev, "%s: Failed to read message\n",
								__func__);
				goto end;
			}
			mxt_write_log(data, MXT_LOG_READ_MESSAGE, NULL);

			reportid = message.reportid;

			MXT_DEV_DBG(dev, 
									"%s: Message=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", 
									__func__,
									message.reportid,
									message.message[0],
									message.message[1],
									message.message[2],
									message.message[3],
									message.message[4],
									message.message[5],
									message.message[6]);

			/* check whether report id is part of T9 or T25 or T42 */
			if (reportid >= data->t9_min_reportid &&
					reportid <= data->t9_max_reportid ) {
				id = reportid - data->t9_min_reportid;
				mxt_input_touchevent(data, &message, id);
			}
			else if (reportid >= data->t25_min_reportid &&
							 reportid <= data->t25_max_reportid ) {
				data->selftest_status = message.message[0];
			}
			else if (reportid >= data->t42_min_reportid &&
							 reportid <= data->t42_max_reportid ) {
				mxt_handle_touch_supression(data, message.message[0]);
			}
			else if (reportid != 0xff)
				mxt_error_check_process(data, &message);

			mxt_write_log(data, MXT_LOG_MSG, &message);

		} while (reportid != 0xff);
	} else {
		/* Maybe reset occur.
		 * Read dummy message to make high CHG pin,
		 * then configure deep sleep mode.
		 */
		dev_info(dev, "Device should be suspended. "
						"Configure deep sleep mode.\n");
		do {
			if (mxt_read_message(data, &message)) {
				dev_err(dev, "%s: Failed to read message\n",
								__func__);
				goto end;
			}
			reportid = message.reportid;
			mxt_error_check_process(data, &message);
			mxt_write_log(data, MXT_LOG_MSG, &message);
		} while (reportid != 0xff);
		if (mxt_stop(data))
			dev_err(dev, "mxt_stop failed in mxt_interrupt\n");
	}

end:
	mutex_unlock(&data->lock);
	MXT_DEV_DBG(dev, "Core[%d]: %s is completed.\n",
				smp_processor_id(), __func__);
	mxt_write_log(data, MXT_LOG_INTERRUPT_END, NULL);
	return IRQ_HANDLED;
}

static int mxt_check_reg_init(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret = 0;

	MXT_DEV_DBG(dev, "%s() is called.\n", __func__);

	data->is_set = false;
	ret = mxt_check_reg_config(data);

	return ret;
}

static int mxt_get_info(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;
	u8 val;

	error = mxt_read_reg(client, MXT_FAMILY_ID, &val);
	if (error)
		return error;
	info->family_id = val;

	error = mxt_read_reg(client, MXT_VARIANT_ID, &val);
	if (error)
		return error;
	info->variant_id = val;

	error = mxt_read_reg(client, MXT_VERSION, &val);
	if (error)
		return error;
	info->version = val;

	error = mxt_read_reg(client, MXT_BUILD, &val);
	if (error)
		return error;
	info->build = val;

	/* Update matrix size at info struct */
	error = mxt_read_reg(client, MXT_MATRIX_X_SIZE, &val);
	if (error)
		return error;
	info->matrix_xsize = val;

	error = mxt_read_reg(client, MXT_MATRIX_Y_SIZE, &val);
	if (error)
		return error;
	info->matrix_ysize = val;

	error = mxt_read_reg(client, MXT_OBJECT_NUM, &val);
	if (error)
		return error;
	info->object_num = val;

	return 0;
}

static int mxt_get_object_table(struct mxt_data *data)
{
	int error;
	int i;
	u16 reg;
	u8 reportid = 0;
	u8 buf[MXT_OBJECT_SIZE];
	u16 size;

	for (i = 0; i < data->info.object_num; i++) {
		struct mxt_object *object = data->object_table + i;

		reg = MXT_OBJECT_START + MXT_OBJECT_SIZE * i;
		error = mxt_read_object_table(data->client, reg, buf);
		if (error)
			return error;

		object->type = buf[0];
		object->start_address = (buf[2] << 8) | buf[1];
		object->size = buf[3];
		object->instances = buf[4];
		object->num_report_ids = buf[5];

		MXT_DEV_DBG(&data->client->dev,
					"type=%2d, addr=%3X, size=%3d, instances=%d, num_report=%d,\n",
					object->type, 
					object->start_address, 
					object->size, 
					object->instances, 
					object->num_report_ids);

		if (object->num_report_ids) {
			reportid += object->num_report_ids *
					(object->instances + 1);
			object->max_reportid = reportid;
		}
		size = (object->size + 1) * (object->instances + 1);
		if (data->max_o_size < size)
			data->max_o_size = size;
	}

	return 0;
}

static void mxt_reset_delay(struct mxt_data *data)
{
	struct mxt_info *info = &data->info;

	switch (info->family_id) {
	case MXT224S_FAMILY_ID:
		msleep(MXT224S_RESET_TIME);
		break;
	default:
		msleep(MXT_RESET_TIME);
	}
}

static int mxt_log_object(struct mxt_data *data, struct file *fp)
{
	int error = 0;
	struct mxt_object *object;
	int i, j;
	u8 val;
	char buf[700];
	u16 size;
	int len;

	if (ts_log_file_enable != 4)
		return 0;

	for (i = 0; i < data->info.object_num; i++) {
		memset(buf, '\0', sizeof(buf));
		object = data->object_table + i;

		len = sprintf(buf, "\n,Object table Element[%d] (Type : %d)",
						i + 1, object->type);
		fp->f_op->write(fp, buf, len, &(fp->f_pos));
		memset(buf, '\0', sizeof(buf));

		size = (object->size + 1) * (object->instances + 1);
		for (j = 0; j < size; j++) {
			if (!(j % 10)) {
				len = sprintf(buf, "\n,");
				fp->f_op->write(fp, buf, len, &(fp->f_pos));
				memset(buf, '\0', sizeof(buf));
			}
			error = mxt_read_object(data,
						object->type, j, &val);
			if (error) {
				MXT_DEV_DBG(&data->client->dev,
					"t%d[%02d] read error!\n",
					object->type, j);
				goto done;
			}
			len = sprintf(buf, "0x%02x ", val);
			fp->f_op->write(fp, buf, len, &(fp->f_pos));
			memset(buf, '\0', sizeof(buf));
		}
	}
done:
	len = sprintf(buf, "\n\n");
	fp->f_op->write(fp, buf, len, &(fp->f_pos));
	return error;
};

#ifdef MXT_DUMP_OBJECT
static int mxt_dump_object(struct mxt_data *data)
{
	struct mxt_object *object;
	int i, j;
	int error = 0;
	u8 val;
	char buf[700];
	char str[80];
	u16 size;

	for (i = 0; i < data->info.object_num; i++) {
		memset(buf, '\0', sizeof(buf));
		object = data->object_table + i;

		sprintf(str, "\nObject table Element[%d] (Type : %d)",
						i + 1, object->type);
		strcat(buf, str);

		size = (object->size + 1) * (object->instances + 1);
		for (j = 0; j < size; j++) {
			if (!(j % 10)) {
				strcat(buf, "\n");
			}
			error = mxt_read_object(data,
						object->type, j, &val);
			if (error) {
				MXT_DEV_DBG(&data->client->dev,
					"t%d[%02d] read error!\n",
					object->type, j);
				goto done;
			}
			sprintf(str, "0x%02x ", val);
			strcat(buf, str);
		}
		MXT_DEV_DBG(&data->client->dev, "buf length %d\n", strlen(buf));
		MXT_DEV_DBG(&data->client->dev, "%s\n",buf);
	}
done:
	return error;
};
#endif /* MXT_DUMP_OBJECT */

static int mxt_save_objects(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_object *t7_object;
	struct mxt_object *t9_object;
	struct mxt_object *t15_object;
	struct mxt_object *t25_object;
	struct mxt_object *t42_object;
	int error;

	/* Store T7 and T9 locally, used in suspend/resume operations */
	t7_object = mxt_get_object(data, MXT_GEN_POWER_T7);
	if (!t7_object) {
		dev_err(&client->dev, "Failed to get T7 object\n");
		return -EINVAL;
	}

	data->t7_start_addr = t7_object->start_address;
	error = __mxt_read_reg(client, data->t7_start_addr,
				T7_BACKUP_SIZE, data->t7_data);
	if (error < 0) {
		dev_err(&client->dev,
			"Failed to save current power state\n");
		return error;
	}

	/* Store T9, T15's min and max report ids */
	t9_object = mxt_get_object(data, MXT_TOUCH_MULTI_T9);
	if (!t9_object) {
		dev_err(&client->dev, "Failed to get T9 object\n");
	}
	else {
		data->t9_max_reportid = t9_object->max_reportid;
		data->t9_min_reportid = t9_object->max_reportid -
						t9_object->num_report_ids + 1;
	}
 
	t15_object = mxt_get_object(data, MXT_TOUCH_KEYARRAY_T15);
	if (!t15_object){
		dev_err(&client->dev, "T15 object is not available\n");
	}
	else {
		data->t15_max_reportid = t15_object->max_reportid;
		data->t15_min_reportid = t15_object->max_reportid -
					t15_object->num_report_ids + 1;
	}

	/* Store T25 min and max report ids */
	t25_object = mxt_get_object(data, MXT_SPT_SELFTEST_T25);
	if (!t25_object){
		dev_err(&client->dev, "T25 object is not available\n");
	}
	else {
		data->t25_max_reportid = t25_object->max_reportid;
		data->t25_min_reportid = t25_object->max_reportid -
					t25_object->num_report_ids + 1;
	}

	/* Store T42 min and max report ids */
	t42_object = mxt_get_object(data, MXT_PROCI_TOUCH_T42);
	if (!t42_object){
		dev_err(&client->dev, "T42 object is not available\n");
	}
	else {
		data->t42_max_reportid = t42_object->max_reportid;
		data->t42_min_reportid = t42_object->max_reportid -
					t42_object->num_report_ids + 1;
	}

	return 0;
}

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;

	error = mxt_get_info(data);
	if (error)
		return error;

	data->device_mode = APPMODE;

	data->object_table = kcalloc(info->object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->object_table) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Set config data */
	if(data->info.version == MXT_FW_V1_9){
		data->config = mxt_configs[0].data;
		data->config_length = mxt_configs[0].length;
	}
	else if (data->info.version == MXT_FW_VSC_16) {
		data->config = mxt_configs[1].data;
		data->config_length = mxt_configs[1].length;
	}
	else {
		if (OEM_get_board() <= OEM_BOARD_WS2_TYPE) {
			data->config = mxt_configs[2].data;
			data->config_length = mxt_configs[2].length;
		}
		else{
			data->config = mxt_configs[3].data;
			data->config_length = mxt_configs[3].length;
		}
	}

	/* Get object table information */
	error = mxt_get_object_table(data);
	if (error)
		goto free_object_table;

	/* Check T6 reset report */
	error = mxt_check_reset_report(data);
	if (error)
		goto free_object_table;

	/* Check register init values */
	error = mxt_check_reg_init(data);
	if (error)
		goto free_object_table;

	if (data->is_set) {
		dev_info(&client->dev, "%s: NVM set.\n", __func__);
		/* Backup to memory */
		error = mxt_backup_nv(data);
		if (error)
			dev_err(&client->dev, "%s: Fail to backup!\n", __func__);
		else
			data->is_set = false;
		
		dev_info(&client->dev, "%s: T6 reset.\n", __func__);
		/* Write T6 reset */
		error = mxt_write_object(data, MXT_GEN_COMMAND_T6, MXT_COMMAND_RESET, MXT_RESET_ORDER);
		if (error) {
			dev_err(&client->dev, 	"%s: Fail to software reset!\n", __func__);
		}
		error = mxt_wait_interrupt(data);
		if (error) {
			dev_err(&client->dev, "%s: Interrupt signal doesn't change\n", __func__);
		}
		/* Check T6 reset report */
		error = mxt_check_reset_report(data);
		if (error) {
			dev_err(&client->dev, "%s: don't receive reset report\n", __func__);
		}
	}

	data->config_status = TS_CONFIG_BOOT_NORMAL;
	data->config_status_last = data->config_status;

#ifdef MXT_DUMP_OBJECT
	MXT_DEV_DBG(&client->dev, "mXT224E Object dump after config\n");
	mxt_dump_object(data);
#endif /* MXT_DUMP_OBJECT */

	error = mxt_save_objects(data);
	if (error)
		goto free_object_table;

	MXT_DEV_DBG(&client->dev, "Family ID: 0x%X Variant ID: 0x%X Version: 0x%X"
			       " Build: 0x%X\n",
			       info->family_id,
			       info->variant_id,
			       info->version,
			       info->build);

	MXT_DEV_DBG(&client->dev,
			"Matrix X Size: %d Matrix Y Size: %d Object Num: %d\n",
			info->matrix_xsize, info->matrix_ysize,
			info->object_num);

	return 0;

free_object_table:
	kfree(data->object_table);
	return error;
}

static void mxt_calc_resolution(struct mxt_data *data)
{
	unsigned int max_x = MXT_PANEL_HEIGHT - 1;
	unsigned int max_y = MXT_PANEL_WIDTH - 1;

	if (MXT_DIAGONAL & MXT_XY_SWITCH) {
		data->max_x = max_y;
		data->max_y = max_x;
	} else {
		data->max_x = max_x;
		data->max_y = max_y;
	}
}

static ssize_t mxt_object_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int i, j;
	int error;
	u8 val;
	int num;

	mutex_lock(&data->lock);
	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				"Object table Element[%d] (Type : %d)",
				i + 1, object->type);
		if (count >= PAGE_SIZE) {
			count = PAGE_SIZE - 1;
			goto done;
		}

		num = (object->size + 1) * (object->instances + 1);
		for (j = 0; j < num; j++) {
			if (!(j % 10))
				count += scnprintf(buf + count
						, PAGE_SIZE - count
						, "\n");
			error = mxt_read_object(data,
						object->type, j, &val);
			if (error) {
				count = error;
				goto done;
			}

			count += scnprintf(buf + count, PAGE_SIZE - count,
							"0x%02x ", val);
			if (count >= PAGE_SIZE) {
				count = PAGE_SIZE - 1;
				goto done;
			}
		}

		count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
		if (count >= PAGE_SIZE) {
			count = PAGE_SIZE - 1;
			goto done;
		}
	}

done:
	mutex_unlock(&data->lock);
	return count;
}

static ssize_t mxt_object_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	u8 *odata;
	unsigned int type;
	const char *p;
	int n = 0;
	int i;
	int offset = 0;
	unsigned int tmp;
	u16 size;

	odata = kcalloc(data->max_o_size, sizeof(u8), GFP_KERNEL);
	if (!odata) {
		dev_err(&data->client->dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}

	sscanf(buf,"%u ", &type);
	tmp = type;
	while (tmp > 0) {
		offset++;
		tmp /= 10;
	}

	mutex_lock(&data->lock);
	object = mxt_get_object(data, (u8)type);
	if (!object) {
		dev_err(&data->client->dev, "Invalid Object Type!\n");
		count = -EINVAL;
		goto done;
	}

	p = buf + offset;
	while (sscanf(p, "%x %n", &tmp, &offset) == 1) {
		odata[n] = (u8)tmp;
		n++;
		p += offset;
	}

	size = (object->size + 1) * (object->instances + 1);
	if (n < size) {
		dev_err(&data->client->dev, "Too short Parameters!\n");
		count = -EINVAL;
		goto done;
	} else if (n > size) {
		dev_err(&data->client->dev, "Too long Parameters!\n");
		count = -EINVAL;
		goto done;
	}

	for (i = 0; i < size; i++) {
		if (mxt_write_object(data, object->type, i, odata[i])) {
			dev_err(&data->client->dev, "failed to write object!\n");
			count = -EIO;
			goto done;
		}
	}
done:
	mutex_unlock(&data->lock);
	kfree(odata);
	return count;
}

static int mxt_atoi(struct device *dev, const char *src, u8 *dst)
{
	u8 val = 0;
	int cnt = 0;

	for (;; src++) {
		switch (*src) {
		case '0' ... '9':
			val = 16 * val + (*src - '0');
			break;
		case 'A' ... 'F':
			val = 16 * val + (*src - '7');
			break;
		case 'a' ... 'f':
			val = 16 * val + (*src - 'W');
			break;
		default:
			return 0;
		}
		if ((cnt % 2) == 1) {
			*dst = val;
			dst++;
			val = 0;
		}
		cnt++;
	}
	return -1;
}

static long mxt_get_debug_flag(struct device *dev, unsigned long arg)
{
	/* T.B.D. */
	return 0;
}

static int mxt_update_fw(struct device *dev, size_t size, u8 *val)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	unsigned int frame_size;
	unsigned int retry = 0;
	unsigned int pos = 0;
	int ret, i, max_frame_size;
	u8 *frame;

	if(data->info.version >= MXT_FW_V1_9) {
		dev_info(dev, "The firmware has been updated\n");
		return 0;
	}

	max_frame_size = MXT_SINGLE_FW_MAX_FRAME_SIZE;

	frame = kmalloc(max_frame_size, GFP_KERNEL);
	if (!frame) {
		dev_err(dev, "Unable to allocate memory for frame data\n");
		return -ENOMEM;
	}

	/* Change to the bootloader mode */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_RESET, MXT_BOOT_VALUE);

	ret = mxt_switch_to_bootloader_address(data);
	if (ret)
		goto free_frame;

	for (i = 0; i < 5; i++) {
		mxt_reset_delay(data);

		ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
		if (ret) {
			ret = mxt_check_bootloader(client, MXT_APP_CRC_FAIL);
			if (!ret) {
				MXT_DEV_DBG(dev,
					"%s: status is MXT_APP_CRC_FAIL\n",
					__func__);
				break;
			}
		} else {
			MXT_DEV_DBG(dev,
				"%s: status is MXT_WAITING_BOOTLOAD_CMD\n",
				__func__);
			break;
		}
	}
	if (ret) {
		dev_err(dev, "%s: status error!\n", __func__);
		goto free_frame;
	}

	MXT_DEV_DBG(dev, "Unlocking bootloader\n");
	/* Unlock bootloader */
	ret = mxt_unlock_bootloader(client);
	if (ret)
		goto return_to_app_mode;

	while (pos < size) {
		ret = mxt_wait_interrupt(data);
		if (ret) {
			dev_err(dev,
				"%s: Interrupt signal doesn't change!\n",
				__func__);
			goto return_to_app_mode;
		}
		MXT_DEV_DBG(dev, "Interruput is Lo\n");
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret) {
			dev_err(dev,
				"%s: Status error!(MXT_WAITING_FRAME_DATA)\n",
				__func__);
			goto return_to_app_mode;
		}
		MXT_DEV_DBG(dev, "status is MXT_WAITING_FRAME_DATA\n");

		frame_size = ((val[pos] << 8) | val[pos + 1]);
		MXT_DEV_DBG(dev,
			"%s: read frame_size = [%d]\n", __func__, frame_size);

		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;
		MXT_DEV_DBG(dev, "%s: frame_size = [%d]\n",
					__func__, frame_size);

		if (frame_size > max_frame_size) {
			dev_err(dev, "Invalid frame size - %d\n", frame_size);
			ret = -EINVAL;
			goto return_to_app_mode;
		}

		memcpy(frame, &val[pos], frame_size);


		/* Write one frame to device */
		ret = mxt_bundle_fw_write(client, frame, frame_size);
		if (ret) {
			dev_err(dev, "%s: fw write error!\n", __func__);
			goto return_to_app_mode;
		}

		ret = mxt_wait_interrupt(data);
		if (ret) {
			dev_err(dev,
				"%s: Interrupt signal doesn't change!\n",
				__func__);
			goto return_to_app_mode;
		}

		ret = mxt_check_bootloader(client, MXT_FRAME_CRC_PASS);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {
				dev_info(dev,
					"%s: Over retry time. crc failed! pos[%d]\n",
					__func__, pos);
				goto return_to_app_mode;
			}
		} else {
			retry = 0;
			pos += frame_size;
			MXT_DEV_DBG(dev, "status is MXT_FRAME_CRC_PASS\n");
			MXT_DEV_DBG(dev, "Updated %d/%zd bytes\n", pos, size);
		}
	}

return_to_app_mode:
	mxt_switch_to_appmode_address(data);
free_frame:
	kfree(frame);

	return ret;
}

static int mxt_clear_object(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	size_t size;
	int i;
	u8 *o_data;
	int ret = 0;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		size = (object->size + 1) * (object->instances + 1);
		o_data = kcalloc(size, sizeof(u8), GFP_KERNEL);
		if (!o_data) {
			dev_err(dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		memset(o_data, 0, size);
		ret = mxt_bundle_write_object(data, object->type, o_data);
		if (ret) {
			dev_err(dev, "%s: Register write error!\n",
							__func__);
			kfree(o_data);
			goto done;
		}
		kfree(o_data);
	}
done:
	return ret;
}

static long mxt_check_fw(struct device *dev, unsigned long arg)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct ts_nv_data *dp = (struct ts_nv_data *)arg;
	long err = 0;
	size_t size;
	char *val;


	if(data->info.variant_id != MXT224S_VARIANT_ID) {
		dev_info(dev, "Unmatch variant ID!\n");
		return -1;
	}

	err = copy_from_user(&size, (void __user *)&dp->size, sizeof(size_t));
	if (err){
		dev_err(dev, "%s: copy_from_user error\n", __func__);
		goto done;
	}

	val = kcalloc(size, sizeof(char), GFP_KERNEL);
	if (!val) {
		dev_err(dev, "%s: Failed to allocate memory\n", __func__);
		err = -ENOMEM;
		goto done;
	}
	err = copy_from_user(val, (void __user *)dp->data, size);
	if (err){
		dev_err(dev, "%s: copy_from_user error\n", __func__);
		goto free_val;
	}

	if (data->client->irq != -1) {
		mxt_disable(data);
	}
	mutex_lock(&data->lock);

	err = (long)mxt_update_fw(dev, size, val);
	if (err) {
		dev_err(dev, "The firmware update failed(%d)\n", (int)err);
	} else {
		dev_info(dev, "The firmware update succeeded\n");

		err = (long)mxt_wait_interrupt(data);
		if (err)
			dev_err(dev, "Interrupt signal doesn't change\n");

		data->device_mode = INIT;
		kfree(data->object_table);
		data->object_table = NULL;
		data->max_o_size = 0;

		err = (long)mxt_get_info(data);
		if (err) {
			dev_err(dev, "Failed to get info\n");
			goto err_unlock;
		}

		data->device_mode = APPMODE;

		data->object_table = kcalloc(data->info.object_num,
					     sizeof(struct mxt_object),
					     GFP_KERNEL);
		if (!data->object_table) {
			dev_err(dev, "Failed to allocate memory\n");
			err = -ENOMEM;
			goto err_unlock;
		}

		/* Get object table information */
		err = (long)mxt_get_object_table(data);
		if (err) {
			dev_err(dev, "Failed to get object table\n");
			goto err_unlock;
		}

		err = mxt_check_reset_report(data);
		if (err)
			dev_err(dev, "don't receive reset report\n");

		err = (long)mxt_clear_object(data);
		if (err) {
			dev_err(dev, "%s: Failed to clear object!\n", __func__);
			goto err_unlock;
		}

		/* Backup to memory */
		err = (long)mxt_backup_nv(data);
		if (err) {
			dev_err(dev, "%s: Fail to backup!\n", __func__);
			goto err_unlock;
		}

		/* Software reset */
		err = (long)mxt_write_object(data, MXT_GEN_COMMAND_T6,
				MXT_COMMAND_RESET, MXT_RESET_ORDER);
		if (err) {
			dev_err(dev,
				"%s: Fail to software reset!\n",
				__func__);
			goto err_unlock;
		}

		msleep(MXT_MAX_RESET_TIME);

		err = (long)mxt_wait_interrupt(data);
		if (err) {
			dev_err(dev,
				"%s: Interrupt signal doesn't change\n",
				__func__);
			goto err_unlock;
		}
		err = (long)mxt_check_reset_report(data);
		if (err) {
			dev_err(dev,
				"%s: don't receive reset report\n",
				__func__);
			goto err_unlock;
		}

		err = (long)mxt_reset_status(data);
		if (err) {
			dev_err(dev, "%s: Failed to reset status\n", __func__);
			goto err_unlock;
		}
	}
err_unlock:
	mutex_unlock(&data->lock);
	if ((data->client->irq != -1) && !err) {
		mxt_enable(data);
	}
free_val:
	kfree(val);
done:
	return err;
}

static long mxt_get_nv(struct device *dev, unsigned long arg)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct ts_nv_data *dp = (struct ts_nv_data *)arg;
	struct ts_config_nv *config_nv = &data->config_nv[dp->type];
	long ret = 0;
	long err;
	size_t size;
	char *str;
	char ver[5];
	char *p;

	err = copy_from_user(&size, (void __user *)&dp->size, sizeof(size_t));
	if (err){
		ret = -EFAULT;
		dev_err(dev, "%s: copy_from_user error\n", __func__);
		goto done;
	}

	str = kcalloc(size, sizeof(char), GFP_KERNEL);
	if (!str) {
		dev_err(dev, "%s: Failed to allocate memory\n", __func__);
		ret = -ENOMEM;
		goto done;
	}
	err = copy_from_user(str, (void __user *)dp->data, size);
	if (err){
		ret = -EFAULT;
		dev_err(dev, "%s: copy_from_user error\n", __func__);
		goto done;
	}

	mutex_lock(&data->lock);
	config_nv->size = size / 2 - 2;

	if (!config_nv->data) {
		config_nv->data = kcalloc(config_nv->size,
					  sizeof(char), GFP_KERNEL);
		if (!config_nv->data) {
			dev_err(dev,
				"%s: Failed to allocate memory\n", __func__);
			ret = -ENOMEM;
			goto err_free_str;
		}
	} else
		MXT_DEV_DBG(dev, "%s: config_nv->data has been allocated.\n",
								__func__);
	memset(ver, '\0', sizeof(ver));
	memcpy(ver, str, 4);
	mxt_atoi(dev, ver, (u8 *)&config_nv->ver);

	p = str + 4;
	mxt_atoi(dev, p, config_nv->data);
	MXT_DEV_DBG(dev, "%s: type = %d, size = %d\n", __func__, dp->type, size);

err_free_str:
	mutex_unlock(&data->lock);
	kfree(str);
done:
	return ret;
}

static long mxt_set_nv(struct device *dev)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	u8 *nv = data->config_nv[TS_CONFIG_BOOT_NORMAL].data;
	long ret = 0;
	long error;

	if (!nv) {
		dev_info(dev, "%s: No nv data. Skipping set nv.\n",
								__func__);
		goto done;
	}

	error = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(5 * MSEC_PER_SEC));

	if (error < 0) {
		dev_err(&client->dev,
			"Error while waiting for device init (%d)!\n", (int)error);
		return -ENXIO;
	} else if (error == 0) {
		dev_err(&client->dev,
			"Timedout while waiting for device init!\n");
		return -ENXIO;
	}

	mutex_lock(&data->lock);
	if (data->is_enable) {
		disable_irq_nosync(client->irq);
		data->is_enable = false;
	}

	ret = mxt_reset_status(data);
	if (ret){
		dev_err(dev, "Failed to reset status!\n");
 		goto error;
	}

#ifdef MXT_DUMP_OBJECT
	MXT_DEV_DBG(dev, "mXT224E Object dump after nv set\n");
	mxt_dump_object(data);
#endif /* MXT_DUMP_OBJECT */

error:
	if (!data->is_enable) {
		enable_irq(client->irq);
		data->is_enable = true;
	}
done:
	mutex_unlock(&data->lock);
	return ret;
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	}

	/* Change to the bootloader mode */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_RESET, MXT_BOOT_VALUE);

	mxt_reset_delay(data);

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	mxt_unlock_bootloader(client);

	while (pos < fw->size) {
		ret = mxt_check_bootloader(client,
						MXT_WAITING_FRAME_DATA);
		if (ret)
			goto out;

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(client,
						MXT_FRAME_CRC_PASS);
		if (ret)
			goto out;

		pos += frame_size;

		MXT_DEV_DBG(dev, "Updated %d bytes / %zd bytes\n", pos, fw->size);
	}

out:
	release_firmware(fw);

	/* Change to slave address of application */
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;

	return ret;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error;

	disable_irq(data->irq);

	error = mxt_load_fw(dev, MXT_FW_NAME);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		count = error;
	} else {
		MXT_DEV_DBG(dev, "The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_FWRESET_TIME);

		kfree(data->object_table);
		data->object_table = NULL;

		mxt_initialize(data);
	}
	enable_irq(data->irq);

	return count;
}

static int mxt_bundle_read_reg(struct mxt_data *data, u16 size, u16 addr, u8 *dp)
{
	int ret;
	while (size > MXT_BLOCK_SIZE) {
		ret = __mxt_read_reg(data->client, addr, MXT_BLOCK_SIZE, dp);
		if (ret)
			return ret;
		size -= MXT_BLOCK_SIZE;
		addr += MXT_BLOCK_SIZE;
		dp += MXT_BLOCK_SIZE;
	}
	ret = __mxt_read_reg(data->client, addr, size, dp);
	return ret;
}

static ssize_t mxt_ctrl_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	const char *p;
	unsigned int val;
	int offset = 0;
	u16 reg_addr;
	struct mxt_object *object;
	int i;
	int err = 0;
	int last_address;
	u8 w_data[MXT_BLOCK_SIZE];
	u8 *dp;
	int state = 0;

	switch (buf[0]) {
	case MXT_SYSFS_LOG_FS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_LOG_FS\n", __func__);
		sscanf(buf, "%c %x", &sdata.command,
					(unsigned int *) &ts_log_file_enable);
		MXT_DEV_DBG(dev, "ts_log_file_enable is set to %d\n",
							ts_log_file_enable);
		break;
	case MXT_SYSFS_POLLING:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_POLLING\n", __func__);
		sscanf(buf, "%c %x", &sdata.command,
					(unsigned int *) &ts_esd_recovery);
		MXT_DEV_DBG(dev, "ts_esd_recovery is set to %d\n",
							ts_esd_recovery);
		break;
	case MXT_SYSFS_INT_STATUS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_INT_STATUS\n", __func__);
		sscanf(buf, "%c", &sdata.command);
		break;
	case MXT_SYSFS_WRITE:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_WRITE\n", __func__);
		sscanf(buf, "%c", &sdata.command);
		p = buf + 1;
		if (sscanf(p, " %x%n", (unsigned int *)&sdata.start_addr,
								&offset) != 1) {
			dev_err(&data->client->dev, "Too short Parameters!\n");
			break;
		}
		p += offset;
		sdata.size = 0;
		MXT_DEV_DBG(dev, "start_address is [%04X].\n",
							sdata.start_addr);
		object = data->object_table + (data->info.object_num - 1);
		last_address = object->start_address +
				((object->size + 1) *
				 (object->instances + 1)) - 1;
		if (sdata.start_addr > last_address) {
			dev_err(dev, "%s:Invalid start address[%04X]!\n",
						__func__, sdata.start_addr);
			break;
		}
		reg_addr = sdata.start_addr;
		mutex_lock(&data->lock);
		while (sscanf(p, " %x%n", &val, &offset) == 1) {
			mxt_write_reg(data->client,
					reg_addr, (u8)val);
			p += offset;
			reg_addr++;
			if (reg_addr > last_address) {
				dev_err(dev, "%s: Invalid address!\n",
								__func__);
				break;
			}
			sdata.size++;
		}
		mutex_unlock(&data->lock);
		MXT_DEV_DBG(dev, "write size is [%d].\n", sdata.size);
		break;
	case MXT_SYSFS_READ:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_READ\n", __func__);
		sscanf(buf, "%c %x %x", &sdata.command,
			(unsigned int *)&sdata.start_addr,
			(unsigned int *)&sdata.size);
		MXT_DEV_DBG(dev, "MXT_SYSFS_READ: start_addr is [%04X]. "
				"size is [%d].\n", sdata.start_addr, sdata.size);
		break;
	case MXT_SYSFS_T37:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_T37\n", __func__);
		if (sscanf(buf, "%c %x", &sdata.command,
				(unsigned int *) &val) != 2) {
			dev_err(&data->client->dev, "Too short Parameters!\n");
			break;
		}
		if (!t37_data)
			t37_data = kcalloc(MXT_DIAG_DATA_SIZE * MXT_DIAG_NUM_PAGE,
					   sizeof(u8), GFP_KERNEL);
		else
			MXT_DEV_DBG(dev, "%s: t37 has been allocated.\n",
								__func__);

		if (!t37_data)  {
			dev_err(dev, "Failed to allocate memory!\n");
			return -ENOMEM;
		}
		mutex_lock(&data->lock);
		MXT_DEV_DBG(dev, "%s: MXT_COMMAND_DIAGNOSTIC set to [0x%02X]\n",
								__func__, val);
		mxt_write_object(data, MXT_GEN_COMMAND_T6,
				MXT_COMMAND_DIAGNOSTIC, (u8)val);
		msleep(50);

		object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
		if (!object) {
			kfree(t37_data);
			t37_data = NULL;
			mutex_unlock(&data->lock);
			return -EINVAL;
		}

		for (i = 0; i < MXT_DIAG_NUM_PAGE; i++) {
			MXT_DEV_DBG(dev, "%s: T37 page [%d] read.\n",
								__func__, i);
			mxt_bundle_read_reg(data,
					    MXT_DIAG_DATA_SIZE,
					    object->start_address,
					    &t37_data[i * MXT_DIAG_DATA_SIZE]);
			if (!(t37_data[i * MXT_DIAG_DATA_SIZE + 1] == i)) {
				dev_err(dev, "%s: T37 page [%d] read Error!\n",
								__func__, i);
				msleep(10);
				i--;
				err++;
			} else
				err = 0;

			if (err > 5) {
				dev_err(dev, "%s: Fail to read T37!\n",
								__func__);
				kfree(t37_data);
				t37_data = NULL;
				mutex_unlock(&data->lock);
				return -EIO;
			}
			if ((err == 0) && (i < 5)){
				msleep(10);
				mxt_write_object(data, MXT_GEN_COMMAND_T6,
						 MXT_COMMAND_DIAGNOSTIC, 0x01);
			}
		}
		mxt_write_object(data, MXT_GEN_COMMAND_T6,
					 MXT_COMMAND_DIAGNOSTIC, 0x00);
		mutex_unlock(&data->lock);
		break;
	case MXT_SYSFS_CONFIG:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_CONFIG\n", __func__);
		mutex_lock(&data->lock);
		sscanf(buf, "%c %x", &sdata.command,
					(unsigned int *) &ts_config_switching);
		mutex_unlock(&data->lock);
		MXT_DEV_DBG(dev, "ts_config_switching is set to %d\n",
							ts_config_switching);
		break;
	case MXT_SYSFS_STATUS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_STATUS\n", __func__);
		sscanf(buf, "%c", &sdata.command);
		break;
	case MXT_SYSFS_CONFIG_NV:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_CONFIG_NV\n", __func__);
		sscanf(buf, "%c %x", &sdata.command,
					(unsigned int *)&sdata.start_addr);
		break;
	case MXT_SYSFS_IRQ:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_IRQ\n", __func__);
		sscanf(buf, "%c %x", &sdata.command, &val);
			if (val) {
				MXT_DEV_DBG(dev, "%s: enable_irq\n", __func__);
				if (data->client->irq != -1) {
					err = mxt_enable(data);
					if (err)
						MXT_DEV_DBG(dev,
							 "%s: failed enable_irq\n",
							 __func__);
				}

			} else {
				MXT_DEV_DBG(dev, "%s: disable_irq\n", __func__);
				if (data->client->irq != -1) {
					mxt_disable(data);
				}
			}
		break;
	case MXT_SYSFS_I2C_WRITE:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_I2C_WRITE\n", __func__);
		sscanf(buf, "%c", &sdata.command);
		p = buf + 1;

		memset(w_data, 0, sizeof(w_data));
		dp = w_data;
		sdata.size = 0;
		while (sscanf(p, " %x%n", &val, &offset) == 1) {
			MXT_DEV_DBG(dev, "%s: __mxt_write_fw(%02x)\n", __func__, val);
			*dp = val;
			p += offset;
			dp++;
			sdata.size++;
			if (sdata.size >= MXT_BLOCK_SIZE)
				break;
		}
		mutex_lock(&data->lock);
		mxt_bundle_write_fw(data, sdata.size, w_data);
		mutex_unlock(&data->lock);
		break;

	case MXT_SYSFS_I2C_READ:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_I2C_READ\n", __func__);
		sscanf(buf, "%c %x", &sdata.command,
			(unsigned int *)&sdata.size);
		MXT_DEV_DBG(dev, "%s: size %d\n", __func__, sdata.size);
		break;
	case MXT_SYSFS_SENSOR_SWITCH:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_SENSOR_SWITCH\n", __func__);
		sscanf(buf, "%c %x", &sdata.command, &val);
		MXT_DEV_DBG(dev, "%s: val=%d\n", __func__, val);
		mutex_lock(&data->lock);
		state = (val==1) ? data->state | TOUCHPANEL_STATE_PATENT_BIT
										: data->state & ~TOUCHPANEL_STATE_PATENT_BIT;
		err = mxt_change_state(data, state);
		mutex_unlock(&data->lock);
		break;
	default:
		break;
	}
	return count;
}

static ssize_t mxt_ctrl_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int val;
	int last_address;
	int i;
	u16 offset;
	u8 *r_data;
	u8 *dp;
	int j;
	struct ts_config_nv *config_nv;

	switch (sdata.command) {
	case MXT_SYSFS_LOG_FS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_LOG_FS\n", __func__);
		count += scnprintf(buf, PAGE_SIZE - count,
				   "ts_log_file_enable is [%d]\n",
				   ts_log_file_enable);
		break;
	case MXT_SYSFS_POLLING:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_POLLING\n", __func__);
		count += scnprintf(buf, PAGE_SIZE - count, "ts_esd_recovery is "
							"[%d]\n", ts_esd_recovery);
		break;
	case MXT_SYSFS_INT_STATUS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_INT_STATUS\n", __func__);
		val = gpio_get_value(MXT_TS_GPIO_IRQ);
		count += scnprintf(buf, PAGE_SIZE - count, "CHG signal is "
							"[%d]\n", val);
		break;

	case MXT_SYSFS_WRITE:
	case MXT_SYSFS_READ:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_READ/WRITE\n", __func__);
		object = data->object_table + (data->info.object_num - 1);
		last_address = object->start_address +
				((object->size + 1) *
				 (object->instances + 1)) - 1;
		if (sdata.start_addr > last_address) {
			dev_err(dev, "%s:Invalid start address[%04X]!\n",
						__func__, sdata.start_addr);
			return PAGE_SIZE - 1;
		}
		if ((!sdata.size) ||
		    (sdata.size > (last_address - sdata.start_addr + 1))) {
			sdata.size = last_address - sdata.start_addr + 1;
			MXT_DEV_DBG(dev, "%s:size is set to[%d].\n",
							__func__, sdata.size);
		}
		r_data = kcalloc(sdata.size, sizeof(u8), GFP_KERNEL);

		mutex_lock(&data->lock);
		mxt_bundle_read_reg(data, sdata.size, sdata.start_addr, r_data);
		mutex_unlock(&data->lock);

		count += scnprintf(buf + count, PAGE_SIZE - count, "        ");
		if (count >= PAGE_SIZE) {
			kfree(r_data);
			return PAGE_SIZE - 1;
		}
		for (i = 0; i < 16; i++) {
			count += scnprintf(buf + count, PAGE_SIZE - count,
								"%2x ", i);
			if (count >= PAGE_SIZE) {
				kfree(r_data);
				return PAGE_SIZE - 1;
			}
		}
		offset = sdata.start_addr & 0xfff0;
		dp = r_data;
		for (i = 0; i < (sdata.size + (sdata.start_addr & 0xf)) ; i++) {
			if (!(i  & 0xf)) {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "\n%04x :\t",
						   i + offset);
				if (count >= PAGE_SIZE) {
					kfree(r_data);
					return PAGE_SIZE - 1;
				}
			}
			if (i < (sdata.start_addr & 0xf)) {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "   ");
				if (count >= PAGE_SIZE) {
					kfree(r_data);
					return PAGE_SIZE - 1;
				}
			} else {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "%02x ", *dp++);
				if (count >= PAGE_SIZE) {
					kfree(r_data);
					return PAGE_SIZE - 1;
				}
			}
		}
		count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

		if (count >= PAGE_SIZE) {
			kfree(r_data);
			return PAGE_SIZE - 1;
		}
		kfree(r_data);
		break;
	case MXT_SYSFS_T37:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_T37[%s]\n",
					__func__, &sdata.command);
		if (!t37_data)  {
			dev_err(dev, "Failed to allocate memory!\n");
			return -ENOMEM;
		}
		for (i = 0; i < MXT_DIAG_NUM_PAGE; i++) {

			count += scnprintf(buf + count,
					   PAGE_SIZE - count,
					   "T37 page[%d]\n", i);
			count += scnprintf(buf + count,
					   PAGE_SIZE - count, "        ");
			if (count >= PAGE_SIZE) {
				kfree(t37_data);
				t37_data = NULL;
				return PAGE_SIZE - 1;
			}
			for (j = 0; j < 16; j++) {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "%2x ", j);
				if (count >= PAGE_SIZE) {
					kfree(t37_data);
					t37_data = NULL;
					return PAGE_SIZE - 1;
				}
			}
			dp = &t37_data[i * MXT_DIAG_DATA_SIZE];
			for (j = 0; j < MXT_DIAG_DATA_SIZE ; j++) {
				if (!(j & 0xf)) {
					count += scnprintf(buf + count,
							   PAGE_SIZE - count,
							   "\n%04x :\t",
							   j);
					if (count >= PAGE_SIZE) {
						kfree(t37_data);
						t37_data = NULL;
						return PAGE_SIZE - 1;
					}
				}
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "%02x ", *dp++);
				if (count >= PAGE_SIZE) {
					kfree(t37_data);
					t37_data = NULL;
					return PAGE_SIZE - 1;
				}
			}
			count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
		}
		kfree(t37_data);
		t37_data = NULL;
		break;
	case MXT_SYSFS_CONFIG:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_CONFIG\n", __func__);
		count += scnprintf(buf, PAGE_SIZE - count,
				   "config switching is [%d]\n",
				   ts_config_switching);
		break;
	case MXT_SYSFS_STATUS:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_STATUS\n", __func__);
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->info.family_id is [%X]\n",
				   data->info.family_id);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->info.variant_id is [%X]\n",
				   data->info.variant_id);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->info.version is [%X]\n",
				   data->info.version);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->info.build is [%X]\n",
				   data->info.build);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->info.object_num is [%X]\n",
				   data->info.object_num);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->init_done.done is [%X]\n",
				   data->init_done.done);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->client->irq is [%d]\n",
				   data->client->irq);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->is_enable is [%d]\n",
				   data->is_enable);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->state is [%X]\n",
				   data->state);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->config_status is [%d]\n",
				   data->config_status);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->config_status_last is [%d]\n",
				   data->config_status_last);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "data->lock.count.counter is [%d]\n",
				   data->lock.count.counter);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		val = gpio_get_value(MXT_TS_GPIO_IRQ);
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "CHG signal is [%d]\n", val);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		for (i = 0; i < MXT_ERR_MAX; i++) {
			count += scnprintf(buf + count, PAGE_SIZE - count,
					   "data->err_cnt[%d] is [%d]\n",
					   i,
					   data->err_cnt[i]);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}
		for (i = 0; i < data->info.object_num; i++) {
			object = data->object_table + i;
			count += scnprintf(buf + count, PAGE_SIZE - count, "object->type [%d]\n", object->type);
			count += scnprintf(buf + count, PAGE_SIZE - count, "object->start_address [%x]\n", object->start_address);
			count += scnprintf(buf + count, PAGE_SIZE - count, "object->size [%d]\n", object->size);
			count += scnprintf(buf + count, PAGE_SIZE - count, "object->instances [%d]\n", object->instances);
			count += scnprintf(buf + count, PAGE_SIZE - count, "object->num_report_ids [%d]\n", object->num_report_ids);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}

		break;
	case MXT_SYSFS_CONFIG_NV:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_CONFIG_NV\n", __func__);
		if (sdata.start_addr >= TS_CONFIG_MAX) {
			count += scnprintf(buf + count,
					   PAGE_SIZE - count,
					   "parameter error!\n");
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
			break;
		}
		config_nv = &data->config_nv[sdata.start_addr];
		count += scnprintf(buf + count,
				   PAGE_SIZE - count,
				   "config_nv[%d]\n", sdata.start_addr);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count,
				   PAGE_SIZE - count,
				   "ver:%04X\n", config_nv->ver);
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		count += scnprintf(buf + count,
				   PAGE_SIZE - count, "        ");
		if (count >= PAGE_SIZE)
			return PAGE_SIZE - 1;
		for (j = 0; j < 16; j++) {
			count += scnprintf(buf + count,
					   PAGE_SIZE - count,
					   "%2x ", j);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}
		dp = config_nv->data;
		for (j = 0; j < config_nv->size ; j++) {
			if (!(j & 0xf)) {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "\n%04x :\t",
						   j);
				if (count >= PAGE_SIZE)
					return PAGE_SIZE - 1;
			}
			count += scnprintf(buf + count,
					   PAGE_SIZE - count,
					   "%02x ", *dp++);
			if (count >= PAGE_SIZE)
				return PAGE_SIZE - 1;
		}
		count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
		break;
	case MXT_SYSFS_I2C_READ:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_I2C_READ\n", __func__);
		if (sdata.size == 0)
			return -EIO;

		r_data = kcalloc(sdata.size, sizeof(u8) , GFP_KERNEL);
		if (!r_data)  {
			dev_err(dev, "Failed to allocate memory!\n");
			return -ENOMEM;
		}

		mutex_lock(&data->lock);
		mxt_bundle_read_fw(data, sdata.size, r_data);
		mutex_unlock(&data->lock);
		for (i = 0; i < sdata.size; i++)
			MXT_DEV_DBG(dev, "%s : r_data[%d] = %x\n",
						__func__, i, r_data[i]);


		count += scnprintf(buf + count, PAGE_SIZE - count, "        ");
		if (count >= PAGE_SIZE) {
			kfree(r_data);
			return PAGE_SIZE - 1;
		}
		for (i = 0; i < 16; i++) {
			count += scnprintf(buf + count, PAGE_SIZE - count,
								"%2x ", i);
			if (count >= PAGE_SIZE) {
				kfree(r_data);
				return PAGE_SIZE - 1;
			}
		}
		dp = r_data;
		for (i = 0; i < sdata.size ; i++) {
			if (!(i  & 0xf)) {
				count += scnprintf(buf + count,
						   PAGE_SIZE - count,
						   "\n%04x :\t",
						   i);
				if (count >= PAGE_SIZE) {
					kfree(r_data);
					return PAGE_SIZE - 1;
				}
			}
			count += scnprintf(buf + count,
					   PAGE_SIZE - count,
					   "%02x ", *dp++);
			if (count >= PAGE_SIZE) {
				kfree(r_data);
				return PAGE_SIZE - 1;
			}
		}
		count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

		if (count >= PAGE_SIZE) {
			kfree(r_data);
			return PAGE_SIZE - 1;
		}
		kfree(r_data);
		break;
	case MXT_SYSFS_SENSOR_SWITCH:
		MXT_DEV_DBG(dev, "%s: MXT_SYSFS_SENSOR_SWITCH\n", __func__);
		mutex_lock(&data->lock);
		count += scnprintf(buf, PAGE_SIZE - count,
				   "state_sensor_switch is [%d]\n",
				   !!(data->state & TOUCHPANEL_STATE_PATENT_BIT));
		mutex_unlock(&data->lock);
		break;
	default:
		break;
	}
	return count;
}

static DEVICE_ATTR(object, S_IRUGO|S_IWUSR, mxt_object_show, mxt_object_store);
static DEVICE_ATTR(update_fw, S_IRUGO|S_IWUSR, NULL, mxt_update_fw_store);
static DEVICE_ATTR(ctrl, S_IRUGO|S_IWUSR, mxt_ctrl_show, mxt_ctrl_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_object.attr,
	&dev_attr_update_fw.attr,
	&dev_attr_ctrl.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static int mxt_enable(struct mxt_data *data)
{
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	mutex_lock(&data->lock);
	if (data->is_enable)
		goto done;

	data->is_enable = true;
	enable_irq(data->client->irq);
done:
	mutex_unlock(&data->lock);
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
	return 0;
}

static void mxt_disable(struct mxt_data *data)
{
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	mutex_lock(&data->lock);
	if (!data->is_enable)
		goto done;

	disable_irq_nosync(data->client->irq);
	data->is_enable = false;
done:
	mutex_unlock(&data->lock);
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
}

static long mxt_diag_data_start(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret = 0;

	MXT_DEV_DBG(dev, "%s is called.\n", __func__);
	if (diag_data == NULL) {
		diag_data = kzalloc(sizeof(struct ts_diag_type), GFP_KERNEL);
		if (!diag_data) {
			dev_err(dev, "Failed to allocate memory!\n");
			return -ENOMEM;
		}

		mutex_init(&diag_lock);
		memset(diag_data, 0, sizeof(struct ts_diag_type));
		ts_diag_start_flag = 1;
	}

	if (data->client->irq != -1)
		ret = mxt_enable(data);
	return ret;
}

static long mxt_diag_data_end(struct device *dev)
{
	ts_diag_start_flag = 0;

	MXT_DEV_DBG(dev, "%s is called.\n", __func__);
	if (diag_data != NULL) {
		mutex_lock(&diag_lock);
		kfree(diag_data);
		diag_data = NULL;
		mutex_unlock(&diag_lock);

		mutex_destroy(&diag_lock);
	}
	return 0;
}

static int mxt_ts_open(struct inode *inode, struct file *file)
{
	struct mxt_data *data =
		container_of(inode->i_cdev, struct mxt_data, device_cdev);
	MXT_DEV_DBG(&data->client->dev, "%s() is called.\n", __func__);

	file->private_data = data;
	return 0;
};

static int mxt_ts_release(struct inode *inode, struct file *file)
{
	return 0;
};

static int mxt_get_t37_data(struct mxt_data *data, char cmd)
{
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int i;
	int err = 0;

	MXT_DEV_DBG(dev, "%s: start\n", __func__);
	if (!t37_data)
		t37_data = kcalloc(MXT_DIAG_DATA_SIZE * MXT_DIAG_NUM_PAGE,
							sizeof(u8), GFP_KERNEL);
	else
		MXT_DEV_DBG(dev, "%s: t37 has been allocated.\n",__func__);

	if (!t37_data)  {
		dev_err(dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}
	MXT_DEV_DBG(dev, "%s: MXT_COMMAND_DIAGNOSTIC set to [0x%02X]\n"
					,__func__, cmd);
	mxt_write_object(data, MXT_GEN_COMMAND_T6, MXT_COMMAND_DIAGNOSTIC,
						(u8)cmd);
	msleep(50);

	object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!object) {
		dev_err(dev, "Failed object error!\n");
		kfree(t37_data);
		t37_data = NULL;
		return -EINVAL;
	}

	for (i = 0; i < MXT_DIAG_NUM_PAGE; i++) {
		MXT_DEV_DBG(dev, "%s: T37 page [%d] read.\n", __func__, i);
		mxt_bundle_read_reg(data,
				    MXT_DIAG_DATA_SIZE,
				    object->start_address,
				    &t37_data[i * MXT_DIAG_DATA_SIZE]);
		if (!(t37_data[i * MXT_DIAG_DATA_SIZE + 1] == i)) {
			dev_err(dev, "%s: T37 page [%d] read Error! error=%d\n",
						__func__, i, err);
			msleep(10);
			i--;
			err++;
		} else
			err = 0;

		if (err > 5) {
			dev_err(dev, "%s: Fail to read T37!\n", __func__);
			kfree(t37_data);
			t37_data = NULL;
			mxt_write_object(data, MXT_GEN_COMMAND_T6,
							 MXT_COMMAND_DIAGNOSTIC, 0x00);
			return -EIO;
		}
		if ((err == 0) && (i < 5)){
			msleep(10);
			mxt_write_object(data, MXT_GEN_COMMAND_T6,
					 MXT_COMMAND_DIAGNOSTIC, 0x01);
		}
	}
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
				 MXT_COMMAND_DIAGNOSTIC, 0x00);

	return err;
}

static int mxt_wait_message_of_golden(struct mxt_data *data, char *t66_fcal,
									struct mxt_object *object, u8 t66_message)
{
	struct mxt_message message;
	struct device *dev = &data->client->dev;
	u16 i;

	for(i=0; i<510; i++){
		msleep(100);
		do {
			mxt_read_message(data, &message);
			if( object->max_reportid == message.reportid){
				if(t66_message & MXT_MESSAGE_GENERATE){
					t66_fcal[0] = message.message[0];
					t66_fcal[1] = message.message[1];
					t66_fcal[2] = message.message[2];
					t66_fcal[3] = message.message[3];
				}
				MXT_DEV_DBG(dev, "%s: T66 message is %02x %02x %02x %02x\n",
							__func__,message.message[0],message.message[1],
									 message.message[2],message.message[3]);
				if( t66_message == (message.message[0] & t66_message)){
					MXT_DEV_DBG(dev, "%s: T66 message OK\n",__func__);
					return 0;
				} else if( (MXT_FCALSEQTO | MXT_FCALSEQERR | MXT_FCALFAIL)
							& message.message[0] ){
					dev_err(dev, "%s: T66 message Error! Status=%02x\n"
							,__func__, message.message[0]);
					return -1;
				} else {
					dev_err(dev, "%s: T66 unexpected message! Status=%02x\n"
							,__func__, message.message[0]);
					return -1;
				}
			}
		} while (message.reportid != 0xff);
		MXT_DEV_DBG(dev, "%s: T66 Wait Message %d\n", __func__, i);
	}
	return -1;
}

static struct mxt_object *mxt_data_write_to_object(struct mxt_data *data,
									u8 obj_type, u8 obj_field, u8 w_status, u8 cmd_mask)
{
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int err = 0;
	u8 *read_obj = NULL;
	u8 w_data;

	object = mxt_get_object(data, obj_type);
	if (!object){
		dev_err(dev, "%s: T%d_Get_Object_ERROR\n", __func__, obj_type);
		return NULL;
	}

	read_obj = kcalloc(object->size, sizeof(u8), GFP_KERNEL);
	if (!read_obj) {
		dev_err(dev, "%s: Failed to allocate memory\n", __func__);
		return NULL;
	}

	err = mxt_bundle_read_object(data, object->type, read_obj);
	if(err){
		dev_err(dev, "%s: T%d_Read_Object_ERROR\n", __func__, obj_type);
		kfree(read_obj);
		return NULL;
	}

	read_obj[obj_field] &= ~cmd_mask;
	w_data = w_status | read_obj[obj_field];
	err = mxt_write_object(data, obj_type, obj_field , w_data );

	if(err){
		dev_err(dev, "%s: Write Object ERROR!\n", __func__);
		object = NULL;
	}

	MXT_DEV_DBG(dev, "%s: T%d w_data=%02x, read_obj=%02x\n", __func__,
				 obj_type, w_data, read_obj[obj_field]);
	kfree(read_obj);

	return object;
}

static int mxt_get_golden_reference(struct mxt_data *data, char *t66_fcal)
{
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int err = 0;
	struct ts_config_nv *config_nv;

	MXT_DEV_DBG(dev, "%s: Start\n", __func__);

	err |= mxt_write_object(data, MXT_SPT_GOLDENREFERENCES_T66,
							 MXT_SPT_FCALFAILTHR, t66_fcal[0] );
	err |= mxt_write_object(data, MXT_SPT_GOLDENREFERENCES_T66,
							 MXT_SPT_FCALDRIFTCNT, t66_fcal[1] );
	err |= mxt_write_object(data, MXT_SPT_GOLDENREFERENCES_T66,
							 MXT_SPT_FCALDRIFTCOEF, t66_fcal[2] );
	err |= mxt_write_object(data, MXT_SPT_GOLDENREFERENCES_T66,
							 MXT_SPT_FCALDRIFTLIM, t66_fcal[3] );

	if(err){
		dev_err(dev, "%s: T66_write_ERROR\n", __func__);
		return err;
	}

	err |= mxt_write_object(data, MXT_SPT_CTECONFIG_T46,
							 MXT_SPT_IDLESYNCSPERX , 0xFF );
	err |= mxt_write_object(data, MXT_SPT_CTECONFIG_T46,
							 MXT_ACTVSYNCSPERX , 0xFF );

	if(err){
		dev_err(dev, "%s: T46_write_ERROR\n", __func__);
		return err;
	}

	err = mxt_write_object( data, MXT_PROCG_NOISE_T62,
							MXT_NOISE_MINNLTHR, MXT_MINNLTHR_CMD );
	if(err){
		dev_err(dev, "%s: T62_write_ERROR\n", __func__);
		return err;
	}

	object = mxt_data_write_to_object(data, MXT_SPT_GOLDENREFERENCES_T66,
									MXT_SPT_CTRL, MXT_CMD_PRIME, MXT_FCALCMD_STATE_MASK);
	if(!object){
		dev_err(dev, "%s: T66_write_ERROR PRIME\n", __func__);
		return -ENOMEM;
	}
	err = mxt_wait_message_of_golden(data, t66_fcal, object, MXT_MESSAGE_PRIME);
	if(err){
		dev_err(dev, "%s: T66_message_TimeOut PRIME\n", __func__);
		return err;
	}

	object = mxt_data_write_to_object(data, MXT_SPT_GOLDENREFERENCES_T66,
									MXT_SPT_CTRL, MXT_CMD_GENERATE, MXT_FCALCMD_STATE_MASK);
	if(!object){
		dev_err(dev, "%s: T66_write_ERROR GENERATE\n", __func__);
		return -ENOMEM;
	}
	err = mxt_wait_message_of_golden(data, t66_fcal, object, (MXT_MESSAGE_GENERATE | MXT_FCALPASS));
	if(err){
		dev_err(dev, "%s: T66_message_TimeOut GENERATE\n", __func__);
		return err;
	}

	object = mxt_data_write_to_object(data, MXT_SPT_GOLDENREFERENCES_T66,
									MXT_SPT_CTRL, MXT_CMD_STORE, MXT_FCALCMD_STATE_MASK);
	if(!object){
		dev_err(dev, "%s: T66_write_ERROR STORE\n", __func__);
		return -ENOMEM;
	}
	err = mxt_wait_message_of_golden(data, t66_fcal, object, MXT_MESSAGE_STORE);
	if(err){
		dev_err(dev, "%s: T66_message_TimeOut STORE\n", __func__);
		return err;
	}

	config_nv = &data->config_nv[TS_CONFIG_MOVE_NORMAL];
	MXT_DEV_DBG(dev, "%s: mxt_T62->config = %02x\n",__func__, config_nv->data[T62_MINNLTHR]);

	err = mxt_write_object( data, MXT_PROCG_NOISE_T62,
							MXT_NOISE_MINNLTHR, config_nv->data[T62_MINNLTHR] );
	if(err){
		dev_err(dev, "%s: T62_write_ERROR\n", __func__);
		return err;
	}

	err = mxt_backup_nv(data);
	if(err){
		dev_err(dev, "%s: Failed mxt_backup_nv\n", __func__);
		return err;
	}

	return err;
}

static int mxt_diag_selftest(struct mxt_data *data)
{
	char buf[2] = {0x03, 0xfe};
	struct mxt_object *object;
	u8 ret=0;
	bool wait_cal = false;

	printk("%s: start\n", __func__);

	mutex_lock(&data->lock);
	if(data->state == TOUCHPANEL_STATE_SLEEP){
		wait_cal = true;
	}
	mxt_change_state(data, data->state | TOUCHPANEL_STATE_SELFTEST_BIT);

	if(wait_cal){
		mutex_unlock(&data->lock);
		msleep(100);
		mutex_lock(&data->lock);
	}
	
	data->selftest_status = -1;

	object = mxt_get_object(data, MXT_SPT_SELFTEST_T25);
	if (!object){
		dev_err(&data->client->dev, "Failed to get T25 object\n");
		mutex_unlock(&data->lock);
		return -1;
	}
	
	__mxt_write_reg(data->client, object->start_address, 2, buf);
	mutex_unlock(&data->lock);
		
	printk("%s: 100ms waiting start.\n", __func__);
	msleep(100);
	printk("%s: 100ms waiting stop.\n", __func__);

	mutex_lock(&data->lock);
	if(data->selftest_status == 0xfe){
		ret = 0;
	} 
	else {
		ret = 1;
	}
	printk("%s: result=0x%X\n", __func__, data->selftest_status);
	
	data->selftest_status = -1;

	mxt_change_state(data, data->state & ~TOUCHPANEL_STATE_SELFTEST_BIT);
	mutex_unlock(&data->lock);
	
	return ret;
}

static long mxt_copy_reg_to_config(struct device *dev, enum ts_config_type type)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct ts_config_nv *config_nv = &data->config_nv[type];
	long ret = 0;

	mutex_lock(&data->lock);
	if (!config_nv->data) {
		config_nv->data = kcalloc(data->config_length, sizeof(char), GFP_KERNEL);
		if (!config_nv->data) {
			dev_err(dev,
				"%s: Failed to allocate memory\n", __func__);
			ret = -ENOMEM;
			goto err_allocate;
		}
	} else
		MXT_DEV_DBG(dev, "%s: config_nv->data has been allocated.\n",
								__func__);

	config_nv->ver = 0;
	config_nv->size = data->config_length;
	memcpy(config_nv->data,  data->config, data->config_length);

err_allocate:
	mutex_unlock(&data->lock);
	return ret;
}

static void mxt_set_config(struct device *dev, enum ts_config_type type, 
              struct swichable_config change_cfg[], int change_num){
	struct mxt_data *data = dev_get_drvdata(dev);
	struct ts_config_nv *config_nv = &data->config_nv[type];
	struct mxt_object *object;
	int i, j, num, index;
	
	mutex_lock(&data->lock);
	if (!config_nv->data) {
		goto err;
	} 

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		index = mxt_find_index_from_object(dev, config_nv->data, config_nv->size, object);
		if (index < 0){
			MXT_DEV_DBG(dev, "%s: skip config t[%d]\n", __func__, object->type);
			continue;
		}

		num = (object->size + 1) * (object->instances + 1);
		if ( (index + num + 2) > config_nv->size ) {
			dev_err(dev, "%s: Not enough config data!\n", __func__);
			dev_err(dev, "%s: Canceled after t[%d]!\n", __func__, object->type);
			goto err;
		}		
		index += 2;
		
		for (j = 0; j < change_num; j++) {
			if(change_cfg[j].obj_type == config_nv->data[index - 2]){
				int offset = index + change_cfg[j].offset;
				config_nv->data[offset] = change_cfg[j].value;
				
  			MXT_DEV_DBG(dev, "%s: object[%d],offset[%d]:%02X\n",
  					__func__, object->type, change_cfg[j].offset, config_nv->data[offset]);
			}
		}
		mxt_check_rpten(data, object, &config_nv->data[index]);
	}

err:
	mutex_unlock(&data->lock);
}
  
static long mxt_set_switchable_config(struct device *dev, enum ts_config_type type)
{
	struct swichable_config config_call_step1[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   5 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_call_step2[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   2 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_move_satate[] = {
		{ MXT_PROCI_TOUCH_T42,  1 ,  30 },
		{ MXT_PROCI_TOUCH_T42,  2 ,  38 },
		{ MXT_PROCI_TOUCH_T42,  3 ,  33 },
		{ MXT_PROCI_TOUCH_T42,  4 , 254 },
	};

	if(type == TS_CONFIG_BOOT_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
	}
	else if(type == TS_CONFIG_BOOT_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
	}
	else if(type == TS_CONFIG_MOVE_NORMAL){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else {
		MXT_DEV_DBG(dev, "%s: not exist config type=%d.\n", __func__, type);
		return -1;
	}

	return 0;
}

static long mxt_set_switchable_config_SC16(struct device *dev, enum ts_config_type type)
{
	struct swichable_config config_move_satate[] = {
		{ MXT_GEN_ACQUIRE_T8,   8 , 219 },
		{ MXT_TOUCH_MULTI_T9,  31 ,  20 },
		{ MXT_PROCI_STYLUS_T47, 0 ,   9 },
	};

	if(type == TS_CONFIG_MOVE_NORMAL){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP1){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP2){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else {
		MXT_DEV_DBG(dev, "%s: not exist config type=%d.\n", __func__, type);
		return -1;
	}

	return 0;
}

static long mxt_set_switchable_config_V3_0(struct device *dev, enum ts_config_type type)
{
	struct swichable_config config_call_step1[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   5 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_call_step2[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   2 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_move_satate[] = {
		{ MXT_GEN_ACQUIRE_T8,   8 , 217 },
		{ MXT_TOUCH_MULTI_T9,  31 ,  20 },
		{ MXT_PROCI_TOUCH_T42,  1 ,  33 },
		{ MXT_PROCI_TOUCH_T42,  2 ,  55 },
		{ MXT_PROCI_TOUCH_T42,  3 ,  52 },
		{ MXT_PROCI_TOUCH_T42,  4 , 254 },
	};

	if(type == TS_CONFIG_BOOT_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
	}
	else if(type == TS_CONFIG_BOOT_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
	}
	else if(type == TS_CONFIG_MOVE_NORMAL){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else {
		MXT_DEV_DBG(dev, "%s: not exist config type=%d.\n", __func__, type);
		return -1;
	}

	return 0;
}

static long mxt_set_switchable_config_V3_0_lab(struct device *dev, enum ts_config_type type)
{
	struct swichable_config config_call_step1[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   5 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_call_step2[] = {
		{ MXT_PROCI_TOUCH_T42,  0 ,   7 },
		{ MXT_PROCI_TOUCH_T42,  6 ,   2 },
		{ MXT_PROCI_TOUCH_T42,  7 ,  20 },
	};
	struct swichable_config config_move_satate[] = {
		{ MXT_TOUCH_MULTI_T9,  31 ,   8 },
		{ MXT_PROCI_TOUCH_T42,  1 ,  28 },
		{ MXT_PROCI_TOUCH_T42,  2 ,  38 },
		{ MXT_PROCI_TOUCH_T42,  3 ,  33 },
		{ MXT_PROCI_TOUCH_T42,  4 , 254 },
	};

	if(type == TS_CONFIG_BOOT_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
	}
	else if(type == TS_CONFIG_BOOT_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
	}
	else if(type == TS_CONFIG_MOVE_NORMAL){
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP1){
		mxt_set_config(dev, type, config_call_step1,
		               ARRAY_SIZE(config_call_step1) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else if(type == TS_CONFIG_MOVE_CALL_STEP2){
		mxt_set_config(dev, type, config_call_step2,
		               ARRAY_SIZE(config_call_step2) );
		mxt_set_config(dev, type, config_move_satate,
		               ARRAY_SIZE(config_move_satate) );
	}
	else {
		MXT_DEV_DBG(dev, "%s: not exist config type=%d.\n", __func__, type);
		return -1;
	}

	return 0;
}

static long mxt_ts_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
{
	struct mxt_data *data = (struct mxt_data *)file->private_data;
	struct device *dev = &data->client->dev;
	char *t66_fcal;
	long err = 0;
	struct ts_log_data log;
	int state;
	struct ts_mode_flag mode_flag;
	enum ts_config_type type;
	bool lcd_stat;

	MXT_DEV_DBG(dev, "%s() is called.\n", __func__);
	switch (cmd) {
	case IOCTL_SET_CONF_STAT:
		MXT_DEV_DBG(dev, "%s: IOCTL_SET_CONF_STAT\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		if (copy_from_user(&type, (void __user *)arg, sizeof(type))) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			goto done;
		}
		mutex_lock(&data->lock);
		err = mxt_switch_config(dev, type);
		mutex_unlock(&data->lock);
		break;
	case IOCTL_GET_CONF_STAT:
		MXT_DEV_DBG(dev, "%s: IOCTL_GET_CONF_STAT\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}

		mutex_lock(&data->lock);
		if (copy_to_user((void __user *)arg,
				 &data->config_status,
				 sizeof(data->config_status))) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_to_user error\n", __func__);
		}
		mutex_unlock(&data->lock);
		MXT_DEV_DBG(dev, "%s: config_status is [%d]\n", __func__,
							data->config_status);
		break;
	case IOCTL_SET_LOG:
		MXT_DEV_DBG(dev, "%s: IOCTL_SET_LOG\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}

		if (copy_from_user(&log, (void __user *)arg, sizeof(log))) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			goto done;
		}

		MXT_DEV_DBG(dev, "mxt_write_log(data, MXT_LOG_DAEMON, &flag)");
		mxt_write_log(data, MXT_LOG_DAEMON, &log);
		break;
	case IOCTL_DIAG_START:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_START\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		err = mxt_diag_data_start(data);
		break;

	case IOCTL_MULTI_GET:
	case IOCTL_COODINATE_GET:
		MXT_DEV_DBG(dev, "%s: IOCTL_MULTI_GET\n", __func__);
		MXT_DEV_DBG(dev, "%s: IOCTL_COODINATE_GET\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		if (!access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
 		if (diag_data != NULL) {
			mutex_lock(&diag_lock);
			err = copy_to_user((void __user *)arg, diag_data,
						sizeof(struct ts_diag_type));
			mutex_unlock(&diag_lock);
		} else
			dev_info(dev, "Touchscreen Diag not active!\n");

		if (err) {
			dev_err(dev, "%s: copy_to_user error\n", __func__);
			return -EFAULT;
		}
		break;

	case IOCTL_DIAG_END:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_END\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		err = mxt_diag_data_end(dev);
		break;

	case IOCTL_DIAG_EVENT_CTRL:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_EVENT_CTRL\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = copy_from_user(&ts_event_control, (void __user *)arg,
						sizeof(unsigned char));
		if (err){
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			return -EFAULT;
		}
		MXT_DEV_DBG(dev, "%s: ts_event_control is [%d]\n", __func__
							, ts_event_control);
		break;

	case IOCTL_DIAG_SELFTEST:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_SELFTEST\n", __func__);
		if (!access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		
		state = mxt_diag_selftest(data);
		MXT_DEV_DBG(dev, "%s: mxt_diag_selftest is [%d]\n", __func__, state);
		
		err = copy_to_user((void __user *)arg, &state,
					sizeof(state));
		if (err) {
			dev_err(dev, "%s: copy_to_user error\n", __func__);
			return -EFAULT;
		}
		break;
	case IOCTL_LOAD_CONFIG_NV:
		MXT_DEV_DBG(dev, "%s: IOCTL_LOAD_CONFIG_NV\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = mxt_get_nv(dev, arg);
		break;
	case IOCTL_SET_NV:
		MXT_DEV_DBG(dev, "%s: IOCTL_SET_NV\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		err = mxt_set_nv(dev);
		break;
	case IOCTL_DIAG_LOG_LEVEL:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_LOG_LEVEL\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = copy_from_user(&ts_log_level, (void __user *)arg,
						sizeof(unsigned char));
		if (err){
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			return -EFAULT;
		}
		MXT_DEV_DBG(dev, "%s: ts_log_level is [%d]\n",
						__func__ , ts_log_level);
		break;
	case IOCTL_LOAD_DEBUG_FLAG_NV:
		MXT_DEV_DBG(dev, "%s: IOCTL_LOAD_DEBUG_FLAG_NV\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = mxt_get_debug_flag(dev, arg);
		break;
	case IOCTL_DIAG_RESET_HW:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_RESET_HW\n", __func__);
		if (data->client->irq == -1) {
			dev_err(dev, "driver is abnormal status.\n");
			return -1;
		}
		mutex_lock(&data->lock);
		mxt_reset_hw();
		mutex_unlock(&data->lock);
		break;
	case IOCTL_GET_GOLDEN_REFERENCE:
		MXT_DEV_DBG(dev, "%s: IOCTL_GET_GOLDEN_REFERENCE\n", __func__);

		mutex_lock(&data->lock);
		if (data->info.version != MXT_FW_V1_9) {
			err = mxt_get_info(data);
			if (err) {
				dev_err(dev, "Failed mxt_get_info\n");
				mutex_unlock(&data->lock);
				goto done;
			}
			kfree(data->object_table);
			data->object_table = kcalloc(data->info.object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
			if (!data->object_table) {
				err = -ENOMEM;
				dev_err(dev, "Failed to allocate memory\n");
				mutex_unlock(&data->lock);
				goto done;
			}

			err = mxt_get_object_table(data);
			if (err) {
				dev_err(dev, "Failed mxt_get_object_table\n");
				mutex_unlock(&data->lock);
				goto done;
			}
		}
		mutex_unlock(&data->lock);

		if (data->info.version != MXT_FW_V1_9) {
			dev_err(dev, "%s: This FW is not Ver.1.9\n", __func__);
			err = -1;
			goto done;
		}

		if (!access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		t66_fcal = kcalloc(T66_CMD_SIZE, sizeof(char), GFP_KERNEL);
		if (!t66_fcal) {
			dev_err(dev, "%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}
		err = copy_from_user(t66_fcal, (void __user *)arg, T66_CMD_SIZE);
		if (err){
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			kfree(t66_fcal);
			return -EFAULT;
		}

		mutex_lock(&data->lock);

		err |= mxt_write_object( data, MXT_GEN_POWER_T7,
								 MXT_POWER_IDLEACQINT, MXT_FREE_RUN_MODE);
		err |= mxt_write_object( data, MXT_GEN_POWER_T7,
								 MXT_POWER_ACTVACQINT, MXT_FREE_RUN_MODE);
		if(err){
			dev_err(dev, "%s: Failed Write T7 Object\n", __func__);
			mutex_unlock(&data->lock);
			goto done;
		}

		err = mxt_get_golden_reference(data, t66_fcal);
		mutex_unlock(&data->lock);
		if(err)
			dev_err(dev, "%s: mxt_get_golden_reference ERROR!\n", __func__);

		if (copy_to_user((void __user *)arg, t66_fcal, T66_CMD_SIZE)) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_to_user error\n", __func__);
		}

		kfree(t66_fcal);
		break;
	case IOCTL_DIAG_GET_C_REFERENCE:
	case IOCTL_DIAG_GET_DELTA:
		MXT_DEV_DBG(dev, "%s: IOCTL_DIAG_GET_C_REFERENCE\n", __func__);
		if (!access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		mutex_lock(&data->lock);
		err = mxt_start(data);
		if(err){
			dev_err(dev, "%s: Failed mxt_start\n", __func__);
			mutex_unlock(&data->lock);
			goto done;
		}

		if(cmd == IOCTL_DIAG_GET_C_REFERENCE)
			err = mxt_get_t37_data(data, MXT_CURRENT_REFERENCE);
		else if(cmd == IOCTL_DIAG_GET_DELTA)
			err = mxt_get_t37_data(data, MXT_DELTA);
		if(!data->state){
			if(mxt_stop(data))
				dev_err(dev, "%s: Failed mxt_stop", __func__);
		}
		if(err){
			dev_err(dev, "%s: Failed get t37_data\n", __func__);
			mutex_unlock(&data->lock);
			goto done;
		}
		mutex_unlock(&data->lock);
		err = copy_to_user((void __user *)arg, t37_data,
							 MXT_DIAG_DATA_SIZE*MXT_DIAG_NUM_PAGE);

		kfree(t37_data);
		t37_data = NULL;

		if (err) {
			dev_err(dev, "%s: copy_to_user error\n", __func__);
			goto done;
		}
		break;
	case IOCTL_CHECK_FW:
		MXT_DEV_DBG(dev, "%s: IOCTL_CHECK_FW\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		MXT_DEV_DBG(dev, "%s: mxt_check_fw\n", __func__);
		err = mxt_check_fw(dev, arg);
		break;
	case IOCTL_GET_INFO:
		MXT_DEV_DBG(dev, "%s: IOCTL_GET_INFO\n", __func__);
		if (!access_ok(VERIFY_WRITE, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		MXT_DEV_DBG(dev, "%s: mxt_check_fw\n", __func__);
		mutex_lock(&data->lock);
		err = copy_to_user((void __user *)arg, &data->info,
							sizeof(data->info));
		mutex_unlock(&data->lock);
		break;
	case IOCTL_COPY_REG_TO_CONFIG:
		MXT_DEV_DBG(dev, "%s: IOCTL_COPY_REG_TO_CONFIG\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = mxt_copy_reg_to_config(dev, arg);
		break;
	case IOCTL_SET_SWITCHABLE_CONFIG:
		MXT_DEV_DBG(dev, "%s: IOCTL_SET_SWITCHABLE_CONFIG\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}

		if (data->info.version == MXT_FW_V1_9) {
			err = mxt_set_switchable_config(dev, arg);
		}
		else if (data->info.version == MXT_FW_VSC_16) {
			err = mxt_set_switchable_config_SC16(dev, arg);
		}
		else {
			if (OEM_get_board() <= OEM_BOARD_WS2_TYPE) {
				err = mxt_set_switchable_config_V3_0(dev, arg);
			}
			else {
				err = mxt_set_switchable_config_V3_0_lab(dev, arg);
			}
		}
		break;
	case IOCTL_CHANGE_MODE:
		MXT_DEV_DBG(dev, "%s: IOCTL_CHANGE_MODE\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}
		err = copy_from_user(&mode_flag, (void __user *)arg,
				sizeof(struct ts_mode_flag));
		if (err){
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			err = -EFAULT;
		}
		else {
			mutex_lock(&data->lock);
			data->mode_flag = mode_flag;
			err = mxt_change_mode(dev, data->mode_flag);
			mutex_unlock(&data->lock);
		}		
		break;
	case IOCTL_GET_MODE_FLAG:
		MXT_DEV_DBG(dev, "%s: IOCTL_GET_MODE_FLAG\n", __func__);
		if (!access_ok(VERIFY_READ, (void __user *)arg,
						_IOC_SIZE(cmd))) {
			err = -EFAULT;
			dev_err(dev, "%s: invalid access\n", __func__);
			goto done;
		}

		mutex_lock(&data->lock);
		if (copy_to_user((void __user *)arg,
				 &data->mode_flag,
				 sizeof(data->mode_flag))) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_to_user error\n", __func__);
		}
		mutex_unlock(&data->lock);
		MXT_DEV_DBG(dev, "%s: mode_flag state[%d], call[%d]\n", __func__,
							data->mode_flag.state,
							data->mode_flag.call);
		break;
	case IOCTL_CHANGE_LCD_STATUS:
		MXT_DEV_DBG(dev, "%s: IOCTL_CHANGE_LCD_STATUS\n", __func__);
		if (copy_from_user(&lcd_stat, (void __user *)arg, sizeof(bool))) {
			err = -EFAULT;
			dev_err(dev, "%s: copy_from_user error\n", __func__);
			goto done;
		}

		if (lcd_stat) {
			mxt_resume(dev);
		}
		else {
			mxt_suspend(dev);
		}
		break;
	default:
		dev_err(dev, "%s: cmd error[%X]\n", __func__, cmd);
		return -EINVAL;
		break;
	}
done:
	return err;
}

const struct file_operations mxt_ts_fops = {
	.owner = THIS_MODULE,
	.open = mxt_ts_open,
	.unlocked_ioctl = mxt_ts_ioctl,
	.release = mxt_ts_release,
};

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);
	int error;

	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	error = wait_for_completion_interruptible_timeout(&data->init_done,
			msecs_to_jiffies(5 * MSEC_PER_SEC));

	if (error > 0) {
		if (data->client->irq != -1)
			error = mxt_enable(data);
		else {
			dev_info(&data->client->dev,
				"Can't enable irq!\n");
#ifdef FEATURE_FACTORY_TEST
			error = 0;
#else /* FEATURE_FACTORY_TEST */
			error = -ENXIO;
#endif /* FEATURE_FACTORY_TEST */
		}
	} else if (error < 0) {
		dev_err(&data->client->dev,
			"Error while waiting for device init (%d)!\n", error);
		error = -ENXIO;
	} else if (error == 0) {
		dev_err(&data->client->dev,
			"Timedout while waiting for device init!\n");
		error = -ENXIO;
	}

	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
						smp_processor_id(), __func__);
	return error;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	if (data->client->irq != -1)
		mxt_disable(data);
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
						smp_processor_id(), __func__);
}


#ifdef CONFIG_PM
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	int error = 0;

	mutex_lock(&data->lock);
	/* unset state_normal bit => usually is state_sleep */
	error = mxt_change_state(data, data->state & ~TOUCHPANEL_STATE_RESUME_BIT);
	if (error < 0) {
		dev_err(dev, "mxt_change_state failed in suspend\n");
	}
	else 
		MXT_DEV_DBG(dev, "%s done.\n", __func__);
	mutex_unlock(&data->lock);
	return error;
}

static int mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	int error = 0;

	mutex_lock(&data->lock);
	/* set state_normal bit */
	error = mxt_change_state(data, data->state | TOUCHPANEL_STATE_RESUME_BIT);
	if (error < 0) {
		dev_err(dev, "mxt_change_state failed in resume\n");
	}
	else 
		MXT_DEV_DBG(dev, "%s done.\n", __func__);
	mutex_unlock(&data->lock);
	return error;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);

	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	mxt_suspend(&data->client->dev);
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
	mxt_write_log(data, MXT_LOG_SUSPEND, NULL);
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);

	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	mxt_resume(&data->client->dev);
	MXT_DEV_DBG(&data->client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
	mxt_write_log(data, MXT_LOG_RESUME, NULL);
}
#endif

static const struct dev_pm_ops mxt_pm_ops = {
#if 0
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,
#endif
};
#endif

static void mxt_esd_recovery_work(struct work_struct *work)
{
	struct mxt_data *data = container_of(work, struct mxt_data, esdwork.work);
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	u8 val;

	if ((ts_esd_recovery != 0) && mutex_trylock(&data->lock)) {
		/* Check Family ID */
		mxt_read_reg(client, MXT_FAMILY_ID, &val);
		if (val != info->family_id) {
			/*
			 * Can't read family id correctly.
			 * Recovery process start.
			 */
			mxt_write_log(data, MXT_LOG_ESD, NULL);
			dev_err(&client->dev, "%s: Recovery start! "
				"Uncorrect family id [%02X]\n", __func__, val);
			if (data->is_enable) {
				disable_irq_nosync(data->irq);
				data->is_enable = false;
			}
			if (mxt_restart(data))
				dev_err(&client->dev,
					"Failed to restart!\n");
			if (mxt_reset_status(data))
				dev_err(&client->dev,
					"Failed to reset status!\n");
			if (!data->is_enable) {
				enable_irq(data->irq);
				data->is_enable = true;
			}
			/* Read dummy to avoid inconsistent */
		} else
			MXT_DEV_DBG(&client->dev, "%s: Correct family id [%02X]\n"
							, __func__, val);
		mutex_unlock(&data->lock);
	}
	if (mxt_wq) {
		queue_delayed_work(mxt_wq, &data->esdwork,
					msecs_to_jiffies(ESD_POLLING_TIME));
	}
}

static void mxt_calibration_work(struct work_struct *work)
{
	struct mxt_data *data = container_of(work, struct mxt_data, calwork.work);
	struct i2c_client *client = data->client;
	int error;

	mutex_lock(&data->lock);
	error = mxt_calibrate(data);
	if (error < 0) {
		dev_err(&client->dev, "mxt_calibrate failed in %s\n", __func__);
	} 
	else {
		MXT_DEV_DBG(&client->dev, "%s done.\n", __func__);
	}
	mutex_unlock(&data->lock);
}

static void mxt_restart_switch_work(struct work_struct *work)
{
	struct mxt_data *data = container_of(work, struct mxt_data, switchwork.work);
	struct i2c_client *client = data->client;

	mutex_lock(&data->lock);
	MXT_DEV_DBG(&client->dev, "%s is called.\n", __func__);
	ts_config_switching = 1;
	mxt_change_mode(&client->dev, data->mode_flag);
	mutex_unlock(&data->lock);
}

static ssize_t mxt_vkeys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":115:997:120:54"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":270:997:120:54"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":425:997:120:54"
	"\n");
}

static struct kobj_attribute mxt_vkeys_attr = {
	.attr = {
		.mode = S_IRUGO,
		.name = "virtualkeys.Atmel_maXTouch_Touchscreen",
	},
	.show = &mxt_vkeys_show,
};

static struct attribute *mxt_properties_attrs[] = {
	&mxt_vkeys_attr.attr,
	NULL
};

static struct attribute_group mxt_properties_attr_group = {
	.attrs = mxt_properties_attrs,
};

static void mxt_init_vkeys(void)
{
	int rc = 0;
	static struct kobject *mxt_properties_kobj;

	mxt_properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (mxt_properties_kobj)
		rc = sysfs_create_group(mxt_properties_kobj, &mxt_properties_attr_group);
	if (!mxt_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	return;
}

static int __devinit mxt_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct mxt_data *data;
	struct input_dev *input_dev;
	int error;

	data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!data || !input_dev) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	data->reset_flag = false;

	data->device_mode = INIT;
	input_dev->name = "Atmel_maXTouch_Touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	data->client = client;
	data->input_dev = input_dev;
	data->irq = client->irq;
	init_completion(&data->init_done);
	mutex_init(&data->lock);
	mutex_init(&file_lock);

	mxt_calc_resolution(data);
	memset(data->err_cnt, 0, sizeof(data->err_cnt));

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(KEY_HOME, input_dev->keybit);
	__set_bit(KEY_MENU, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);

	mxt_init_vkeys();

	/* For multi touch */
	input_mt_init_slots(input_dev, MXT_MAX_FINGER);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, MXT_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,
			     0, MXT_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			     0, MXT_MAX_PRESSURE, 0, 0);

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	error = mxt_init_hw();

	if (error) {
		dev_err(&client->dev, "Failed to intialize hardware\n");
#ifdef FEATURE_FACTORY_TEST
		goto err_dev_access;
#else /* FEATURE_FACTORY_TEST */
		goto err_free_mem;
#endif /* FEATURE_FACTORY_TEST */
	}

	error = mxt_reset_and_delay(data);
	if (error) {
		dev_err(&client->dev, "Failed mxt_reset_and_delay\n");
#ifdef FEATURE_FACTORY_TEST
		goto err_dev_access;
#else /* FEATURE_FACTORY_TEST */
		goto err_free_mem;
#endif /* FEATURE_FACTORY_TEST */
	}

	if (mxt_wq) {
		INIT_DELAYED_WORK(&data->esdwork, mxt_esd_recovery_work);
		/* Check start after minimum 5 secs */
		queue_delayed_work(mxt_wq, &data->esdwork,
				   msecs_to_jiffies(ESD_POLLING_TIME + 5000));

		INIT_DELAYED_WORK(&data->calwork, mxt_calibration_work);
		INIT_DELAYED_WORK(&data->switchwork, mxt_restart_switch_work);
	}

	error = mxt_initialize(data);
	if (error) {
		dev_err(&client->dev, "Failed mxt_initialize\n");
#ifdef FEATURE_FACTORY_TEST
		goto err_dev_access;
#else /* FEATURE_FACTORY_TEST */
		goto err_free_mem;
#endif /* FEATURE_FACTORY_TEST */
	}

	data->state = TOUCHPANEL_STATE_SLEEP;

	wake_lock_init(&touchpanel_wake_lock, WAKE_LOCK_SUSPEND, client->dev.driver->name);
	
	error = request_threaded_irq(client->irq, NULL, mxt_interrupt,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->dev.driver->name, data);
	if (error) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_free_object;
	}

	disable_irq(client->irq);
	complete_all(&data->init_done);

	error = input_register_device(input_dev);
	if (error)
		goto err_free_irq;

	/* Create sysfs */
	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error)
		goto err_unregister_device;

	/* Create cdev file ts_ctrl */
	ts_ctrl_init(&(data->device_cdev), &mxt_ts_fops);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						MXT_SUSPEND_LEVEL;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	dev_info(&data->client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
	return 0;

err_unregister_device:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_free_irq:
	free_irq(client->irq, data);
err_free_object:
	kfree(data->object_table);
err_free_mem:
	input_free_device(input_dev);
	mutex_destroy(&data->lock);
	mutex_destroy(&file_lock);
	kfree(data);
	return error;

#ifdef FEATURE_FACTORY_TEST
err_dev_access:
	client->irq = -1;
	complete_all(&data->init_done);
	error = input_register_device(input_dev);
	if (error)
		goto err_free_mem;
	/* Create sysfs */
	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error)
		goto err_free_mem;
	/* Create cdev file ts_ctrl */
	ts_ctrl_init(&(data->device_cdev), &mxt_ts_fops);
	return error;
#endif /* FEATURE_FACTORY_TEST */
}

static void mxt_shutdown(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_DEV_DBG(&client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	if (mxt_wq) {
		cancel_delayed_work_sync(&data->esdwork);
		cancel_delayed_work_sync(&data->calwork);
		cancel_delayed_work_sync(&data->switchwork);
	}
	mutex_lock(&data->lock);
	mxt_shutdown_hw();
	mutex_unlock(&data->lock);
	MXT_DEV_DBG(&client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);
	int i;

	MXT_DEV_DBG(&client->dev, "Core[%d]: %s is called.\n",
					smp_processor_id(), __func__);
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
	free_irq(data->irq, data);
	input_unregister_device(data->input_dev);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&data->early_suspend);
#endif

	ts_ctrl_exit(&(data->device_cdev));

	for (i = 0; i < TS_CONFIG_MAX; i++) {
		if (data->config_nv[i].data)
			kfree(data->config_nv[i].data);
	}
	kfree(data->object_table);
	kfree(data);

	MXT_DEV_DBG(&client->dev, "Core[%d]: %s is completed.\n",
					smp_processor_id(), __func__);
	return 0;
}

static const struct i2c_device_id mxt_id[] = {
	{ "mXT224S", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct of_device_id atmel_match_table[] = {
	{ .compatible = "atmel_mxt_kc",},
	{ },
};

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "atmel_mxt_kc",
		.owner	= THIS_MODULE,
		.of_match_table = atmel_match_table,
#ifdef CONFIG_PM
		.pm	= &mxt_pm_ops,
#endif
	},
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.shutdown	= mxt_shutdown,
	.id_table	= mxt_id,
};

static int __init mxt_init(void)
{
	
	if (oem_is_off_charge()) {
		return 0;
	}
	

	mxt_wq = alloc_workqueue("mxt_wq", WQ_MEM_RECLAIM, 1);
	if (!mxt_wq)
		printk(KERN_ERR "%s: Fail to allocate workqueue!\n", __func__);

	return i2c_add_driver(&mxt_driver);
}

static void __exit mxt_exit(void)
{
	
	if (oem_is_off_charge()) {
		return;
	}
	

	if (mxt_wq)
		destroy_workqueue(mxt_wq);

	i2c_del_driver(&mxt_driver);
}

module_init(mxt_init);
module_exit(mxt_exit);

/* Module information */
MODULE_AUTHOR("KYOCERA Corporation");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
