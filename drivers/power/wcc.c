/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
 */
/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define DEBUG_MODE          0

#if DEBUG_MODE
#define DEBUG_LOG( arg... )   printk(KERN_DEBUG arg)
#else
#define DEBUG_LOG( arg... )
#endif

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <mach/board.h>
#include <mach/msm_smd.h>
#include <linux/wcc.h>
#include <oem-chg_control.h>


static struct smd_channel *smd_wcc_ch = NULL;
static bool   smd_wcc_opend = false;
#define	WCC_SMD_RETRY_COUNT		20					/* times */
#define	WCC_SMD_RETRY_DELAY		10					/* ms */
#define WCC_SMD_PORT_NAME		"apr_wcc_svc"
#define	WCC_OPEN_RETRY_DELAY	2000				/* ms */

static void wcc_queue_timer_cb(struct work_struct *);
static DECLARE_DELAYED_WORK(wcc_timer, wcc_queue_timer_cb);

static struct modem_status	modem_stat;



/*
 * SMD Event Handler
 */
static void wcc_modem_notify(void *priv, unsigned event)
{
	int len;
	struct nofity_data	data;

	switch (event)
	{
		case SMD_EVENT_DATA:
			DEBUG_LOG("WCC: SMD_EVENT_DATA\n");

			len = smd_read_avail(smd_wcc_ch);
			if (len == (sizeof(struct nofity_data)))
			{
				smd_read_from_cb(smd_wcc_ch, &data, sizeof(struct nofity_data));

				modem_stat.stat_permit = (enum charge_permit)data.stat_permit;
				DEBUG_LOG("WCC: stat=%u\n", (unsigned)modem_stat.stat_permit);

				oem_chg_set_rf_request((int)modem_stat.stat_permit);
			}
			else if (len > 0)		/* if receive size is invalid, then read it all */
			{
				char buf[10];
				while (smd_read_avail(smd_wcc_ch) > 0)
				{
					smd_read_from_cb(smd_wcc_ch, &buf, 10);
				}
			}
			break;
		case SMD_EVENT_OPEN:
			DEBUG_LOG("WCC: smd opened\n");
			smd_wcc_opend = true;
			break;
		case SMD_EVENT_CLOSE:
			DEBUG_LOG("WCC: smd closed\n");
			break;
		case SMD_EVENT_STATUS:
			DEBUG_LOG("WCC: smd status\n");
			break;
		case SMD_EVENT_REOPEN_READY:
			DEBUG_LOG("WCC: smd reopen ready\n");
			break;
	}
}

/*
 * Start SMD
 */
static void wcc_start(void)
{
	int rc;
	int count;

	/* open SMD port */
	rc = smd_open(WCC_SMD_PORT_NAME, &smd_wcc_ch, NULL, wcc_modem_notify);
	if (rc < 0)
	{
		/* open error */
		smd_wcc_ch = NULL;
		pr_err("Now waiting until Wireless Charging Control process on modem is to up. (retry after %d ms)\n", WCC_OPEN_RETRY_DELAY);
	}
	else
	{
		/* open success */

		/* wait until receive SMD_EVENT_OPEN */
		for (count = 0; count < WCC_SMD_RETRY_COUNT; ++count)
		{
			if (smd_wcc_opend)
			{
				/* SMD_EVENT_OPEN reception success */
				printk(KERN_DEBUG "WCC: Wireless Charging Control is Up.\n");
				break;
			}
			mdelay(WCC_SMD_RETRY_DELAY);
		}

		/* check SMD_EVENT_OPEN reception */
		if (count >= WCC_SMD_RETRY_COUNT)
		{
			/* CRITICAL ERROR!  SMD_EVENT_OPEN was not received */
			/* Once, start over this operation */
			smd_close(smd_wcc_ch);
			smd_wcc_ch = NULL;
			pr_err("smd_open() internal error (retry after %d ms)\n", WCC_OPEN_RETRY_DELAY);
		}
	}

	/* retry open process if SMD is not opened */
	if (!smd_wcc_ch)
	{
		DEBUG_LOG("WCC: set retry after %d ms\n", WCC_OPEN_RETRY_DELAY);

		schedule_delayed_work(&wcc_timer, msecs_to_jiffies(WCC_OPEN_RETRY_DELAY));
		return;
	}
}

static void wcc_queue_timer_cb(struct work_struct *work)
{
	DEBUG_LOG("%s: WCC init retry process\n", __func__);

	wcc_start();
}

/*
 * Initialize routine
 */
void wcc_init(void)
{
	modem_stat.stat_permit = CHARGE_INVALID;

	if (oem_is_off_charge())
	{
		/* we have no modem in off charge */
		return;
	}

	/* call initialize main */
	wcc_start();
}

