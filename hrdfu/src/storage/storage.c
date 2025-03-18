/**
 * @file storage.c
 * @brief Flash storage APIs. 
 * 
 * @copyright Copyright (C) A9S Inc. - All Rights Reserved.
 Unauthorized copying of this file, via any medium is strictly prohibited.
 Proprietary and confidential. * 
 */

/* Includes ----------------------------------------------------------- */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>
#include <zephyr/dfu/mcuboot.h>
#include "storage.h"

/* Private defines ---------------------------------------------------- */
#define APP_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot1_partition)
#define NET_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot3_partition)

/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
/* Private function prototypes ---------------------------------------- */
/* Private function definitions ---------------------------------------- */

/* Global Function definitions ----------------------------------------------- */

/**
 * @brief Initialize firmware data write operation for a specific partition
 *
 * Sets up the flash image context for writing firmware data to either the
 * application core or network core partition.
 *
 * @param ctx Pointer to flash image context structure to initialize
 * @param partition Target partition for firmware update (APP_CORE_DFU_PARTITION or NET_CORE_DFU_PARTITION)
 *
 * @return 0 on success, negative error code on failure
 */
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

/**
 * @brief Write firmware data to flash
 *
 * Writes a chunk of firmware data to the flash memory using the provided
 * flash image context. The function uses buffered write to optimize flash
 * operations.
 *
 * @param ctx Pointer to initialized flash image context
 * @param data Pointer to firmware data to write
 * @param len Length of data in bytes
 * @param flush Boolean indicating whether to flush the write buffer
 *
 * @return 0 on success, negative error code on failure
 */
int32_t WriteFwDataToFlash(struct flash_img_context *ctx, uint8_t *data, uint32_t len, bool flush)
{
    int rc = flash_img_buffered_write(ctx, data, len, flush);
    if (rc < 0)
    {
        printk("Image Write Failed: %d\n", rc);
    }

    return rc;
}

/**
 * @brief Set MCUboot flags to perform firmware update on next boot
 *
 * Sets the appropriate MCUboot flags to trigger a firmware update for
 * either the application core or network core on the next system reboot.
 *
 * @param partition Target partition for firmware update (APP_CORE_DFU_PARTITION or NET_CORE_DFU_PARTITION)
 *
 * @return 0 on success, negative error code on failure
 */
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