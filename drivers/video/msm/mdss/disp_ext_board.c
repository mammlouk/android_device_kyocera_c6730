/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2012 KYOCERA Corporation
 * (C) 2013 KYOCERA Corporation
 * (C) 2014 KYOCERA Corporation
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
#include <linux/kernel.h>

#if 0
#include <linux/kc_led.h>
#endif

#include "mdss_dsi.h"
#include "disp_ext.h"

#define DETECT_BOARD_NUM 5

static char reg_read_cmd[1] = {0xbf};
static struct dsi_cmd_desc dsi_cmds = {
	{DTYPE_GEN_READ1, 1, 0, 1, 0, sizeof(reg_read_cmd)},
	reg_read_cmd
};
static char panel_id[4] = {0x01, 0x22, 0x33, 0x11};

static int panel_detection = 0;

int disp_ext_board_detect_board(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int i;
	int panel_found;
	struct dcs_cmd_req cmdreq;

	if( panel_detection != 0 ) {
		pr_debug("%s: panel Checked\n", __func__);
		return panel_detection;
	}

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &dsi_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = 4;
	cmdreq.cb = NULL;

	panel_found = 0;
	for (i = 0; i < DETECT_BOARD_NUM; i++) {
		mdss_dsi_cmdlist_put(ctrl, &cmdreq);
		pr_info("%s - panel ID: %02x %02x %02x %02x\n", __func__,
			ctrl->rx_buf.data[0],
			ctrl->rx_buf.data[1],
			ctrl->rx_buf.data[2],
			ctrl->rx_buf.data[3]);
		if (ctrl->rx_buf.data[0] == panel_id[0]
		&&  ctrl->rx_buf.data[1] == panel_id[1]
		&&  ctrl->rx_buf.data[2] == panel_id[2]
		&&  ctrl->rx_buf.data[3] == panel_id[3]) {
			panel_found = 1;
			break;
		}
	}
	if (panel_found != 1) {
		pr_err("%s: panel not found\n", __func__);
#if 0
		light_led_disp_set(LIGHT_MAIN_WLED_LCD_DIS);
#endif
		panel_detection = -1;
		return panel_detection;
	}
#if 0
	pr_info("%s: panel found\n", __func__);
	light_led_disp_set(LIGHT_MAIN_WLED_LCD_EN);
#endif
	panel_detection = 1;
	return panel_detection;
}

int disp_ext_board_get_panel_detect(void)
{
    return panel_detection;
}
