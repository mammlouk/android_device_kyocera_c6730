/*
 * This software is contributed or developed by KYOCERA Corporation.
 * (C) 2014 KYOCERA Corporation
*/
#ifndef _CHK_SD_STATE_H_
#define _CHK_SD_STATE_H_



struct sd_status_ioctl_info {
	int            card_sts;
    int            card_total;
    int            card_space;
	unsigned int   manfid;
    int            reserve;
};

#define IOC_MAGIC 0xF8
#define IOCTL_SD_STATUS_READ_CMD          _IOR(IOC_MAGIC, 1, struct sd_status_ioctl_info)

#define SD_CARD_IN  1
#define SD_CARD_OUT 0

#endif /* _SMEM_FOTA_H_ */
