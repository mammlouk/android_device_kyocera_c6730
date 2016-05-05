/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
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

#ifndef __LINUX_WCC_H_INCLUDED
#define __LINUX_WCC_H_INCLUDED

enum charge_permit
{
    CHARGE_ENABLE = 0,
    CHARGE_DISABLE = 1,
    CHARGE_LIMIT_ON = 2,
    CHARGE_LIMIT_OFF = 3,
    CHARGE_INVALID = 4,
};


struct modem_status {
    enum charge_permit  stat_permit;
};


struct nofity_data {
    uint8_t stat_permit;
};


extern void wcc_init(void);


#endif /* __LINUX_WCC_H_INCLUDED */
