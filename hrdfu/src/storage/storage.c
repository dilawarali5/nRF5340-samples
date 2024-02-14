/**
 * @file storage.c
 * @brief Flash storage APIs. 
 * 
 * @copyright Copyright (C) A9S Inc. - All Rights Reserved.
 Unauthorized copying of this file, via any medium is strictly prohibited.
 Proprietary and confidential. * 
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>
#include <zephyr/dfu/mcuboot.h>
#include "storage.h"


#define APP_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot1_partition)
#define NET_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot3_partition)


int32_t InitFwDataWrite(struct flash_img_context *ctx, dfu_partition_t partition)
{
	int32_t ret = -1;

	if (partition == APP_CORE_DFU_PARTITION)
	{
		ret = flash_img_init_id(ctx, APP_CORE_DFU_PARTITION_ID);
	}
	else if (partition == NET_CORE_DFU_PARTITION)
	{
		ret = flash_img_init_id(ctx, NET_CORE_DFU_PARTITION_ID);
	}
	
	printk("img init: %d\r\n", ret);

	return ret;
}


int32_t WriteFwDataToFlash(struct flash_img_context *ctx, uint8_t *data, uint32_t len, bool flush)
{
	int rc = flash_img_buffered_write(ctx, data, len, flush);
	if (rc < 0)
	{
		printk("Image Write Failed: %d\n", rc);
	}

	return rc;
}

int32_t SetFwUpdatMcuBootFlag(dfu_partition_t partition)
{
	int ret = -1;
	if (partition == APP_CORE_DFU_PARTITION)
	{
		ret = boot_request_upgrade_multi(0, BOOT_UPGRADE_PERMANENT);
	}
	else if (partition == NET_CORE_DFU_PARTITION)
	{
		ret = boot_request_upgrade_multi(1, BOOT_UPGRADE_PERMANENT);
	}

	return ret;
}