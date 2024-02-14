/**
 * @file storage.h
 * @brief Flash storage APIs. 
 * 
 * @copyright Copyright (C) A9S Inc. - All Rights Reserved.
 Unauthorized copying of this file, via any medium is strictly prohibited.
 Proprietary and confidential. * 
 */


#ifndef Storage_h
#define Storage_h

#ifdef __cplusplus
extern "C" {
#endif
#include <zephyr/dfu/flash_img.h>

/*Tags to use for return data of files list*/
#define START_BYTE          0x0E
#define END_BYTE            0x0

#define NRF5340_APP         0
#define NRF5340_NET         1

// Enum for app core and net cor DFU partition
typedef enum
{
    APP_CORE_DFU_PARTITION = NRF5340_APP,
    NET_CORE_DFU_PARTITION = NRF5340_NET,
    MAX_DFU_PARTITION
} dfu_partition_t;

int32_t InitFwDataWrite(struct flash_img_context *ctx, dfu_partition_t partition);
int32_t WriteFwDataToFlash(struct flash_img_context *ctx, uint8_t *data, uint32_t len, bool flush);
int32_t SetFwUpdatMcuBootFlag(dfu_partition_t partition);

#ifdef __cplusplus
}
#endif

#endif