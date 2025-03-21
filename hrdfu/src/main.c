/**
 * @file main.c
 * @brief Application main entry point for Host Receive Device Firmware Update (HRDFU)
 *
 * This application demonstrates a Bluetooth Low Energy OTA firmware update mechanism for
 * the nRF5340 dual-core SoC. It supports updates for both the application and network cores.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>

#include "useruart.h"
#include "storage.h"

/**
 * @brief Bluetooth advertising data - defines services advertised by the device
 */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
              BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
              BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
              BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

/**
 * @brief Bluetooth scan response data - contains custom UUID for DFU service
 */
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
              0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
              0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

/** @brief Select target core for firmware update: 1 for network core, 0 for app core */
#define TESTING_NET_CORE_DFU 1

/** @brief Flash image context for firmware update operations */
struct flash_img_context flash_ctx;

/**
 * @brief Callback for Bluetooth connection events
 *
 * Called when a device connects to this device over Bluetooth
 *
 * @param conn Pointer to the connection that was established
 * @param err Error code (0 for successful connection)
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    } else {
        printk("Connected\n");
    }
}

/**
 * @brief Callback for Bluetooth disconnection events
 *
 * Called when a device disconnects from this device
 *
 * @param conn Pointer to the connection that was terminated
 * @param reason Reason code for the disconnection
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);
}

/** @brief Define Bluetooth connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * @brief Initialize and start Bluetooth advertising
 *
 * Called once Bluetooth stack is fully initialized
 */
static void bt_ready(void)
{
    int err;

    printk("Bluetooth initialized\n");

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

/**
 * @brief Callback for authentication cancellation events
 *
 * Called when a pairing/authentication process is cancelled
 *
 * @param conn Pointer to the connection for which authentication was cancelled
 */
static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

/** @brief Authentication callbacks structure */
static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
};

/**
 * @brief Update and notify battery service level
 *
 * Simulates decreasing battery level and notifies subscribers
 */
static void bas_notify(void)
{
    uint8_t battery_level = bt_bas_get_battery_level();

    battery_level--;

    if (!battery_level) {
        battery_level = 100U;
    }

    bt_bas_set_battery_level(battery_level);
}

/**
 * @brief Update and notify heart rate service
 *
 * Simulates heart rate measurements and notifies subscribers
 */
static void hrs_notify(void)
{
    static uint8_t heartrate = 90U;

    /* Heartrate measurements simulation */
    heartrate++;
    if (heartrate == 160U) {
        heartrate = 90U;
    }

    bt_hrs_notify(heartrate);
}

/**
 * @brief Structure to hold network core firmware version information
 */
typedef struct
{
    uint8_t Major;
    uint8_t Minor;
    uint8_t Build;
} netCoreFwVersion_Struct;

/** @brief ACK response byte to indicate successful operation */
uint8_t ACK_BYTE = 0x06;

/** @brief NACK response byte to indicate failed operation */
uint8_t NACK_BYTE = 0x15;

/**
 * @brief Main application entry point
 *
 * Initializes Bluetooth, UART communication and handles the firmware update process
 *
 * @return 0 on successful completion (never reached in normal operation)
 */
int main(void)
{
    int err;
    printk("Starting DFU Test ...\n");

    // Initialize Bluetooth stack
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    bt_ready();

    bt_conn_auth_cb_register(&auth_cb_display);

    /* Implement notification. At the moment there is no suitable way
     * of starting delayed work so we do it here
     */

    // Initialize UART for receiving firmware updates
    printk("serial init: %d\r\n", init_serial_comms_handler());

    // Read network core firmware version from shared memory
    netCoreFwVersion_Struct *netCoreFwVersion= NULL;
    netCoreFwVersion = (netCoreFwVersion_Struct*)DT_REG_ADDR(DT_ALIAS(sramnetcorefwdata));
    k_sleep(K_SECONDS(5));
    printk("netcore: %d.%d.%d\r\n", netCoreFwVersion->Major, netCoreFwVersion->Minor, netCoreFwVersion->Build);

    // Track total bytes written during firmware update
    uint32_t TotalBytesWritten = 0;
    
    // Main application loop
    while (1) {
        k_sleep(K_MSEC(5));

        // Check for new UART data by detecting idle time after receiving data
        bool flush = false;
        if ((k_uptime_get_32() - lastInterruptTime) > 100 && totalBytesReceived > 0)
        {
            // Verify if received data forms a valid packet
            err = verifyPacket();
            
            // Handle firmware data packet (type 0x03)
            if(err == 0x03)
            {
                // Set flush flag for last data packet in a sequence
                if (OTADataSize != MAX_PACKET_SIZE)
                {
                    printk("flushing\r\n");
                    flush = true;
                }
                
                // Write firmware data to flash memory
                if(WriteFwDataToFlash(&flash_ctx, OTABuffer, OTADataSize, flush) == 0)
                {
                    TotalBytesWritten += OTADataSize;
                    printk("TotalBytesWritten: %d\r\n", TotalBytesWritten);
                    ResetRxBuffer();
                    SendUartData(&ACK_BYTE, 1);  // Acknowledge successful write
                }
                else
                {
                    // Handle flash write failure
                    ResetRxBuffer();
                    SendUartData(&NACK_BYTE, 1);  // Send negative acknowledgement
                }
            }
            // Handle file size information packet (type 0x02)
            else if (err == 0x02)
            {
                ResetRxBuffer();

                // Initialize flash context for appropriate core
                #if TESTING_NET_CORE_DFU == 1
                InitFwDataWrite(&flash_ctx, NET_CORE_DFU_PARTITION);
                #else
                InitFwDataWrite(&flash_ctx, APP_CORE_DFU_PARTITION);
                #endif
                TotalBytesWritten = 0;

                SendUartData(&ACK_BYTE, 1);  // Acknowledge initialization
            }
            else
            {
                // Invalid packet received
                SendUartData(&NACK_BYTE, 1);
            }
        }

        // Check if firmware transfer is complete
        if ((TotalBytesWritten == FileSize) && (FileSize != 0) && (TotalBytesWritten != 0))
        {
            printk("File transfer complete ... %d/%d\r\n", TotalBytesWritten, FileSize);
            
            // Set MCUboot flag to perform update on next boot
            #if TESTING_NET_CORE_DFU == 1
            SetFwUpdatMcuBootFlag(NET_CORE_DFU_PARTITION);
            #else
            SetFwUpdatMcuBootFlag(APP_CORE_DFU_PARTITION);
            #endif

            // Wait briefly then reset the system to apply the update
            k_sleep(K_SECONDS(5));
            NVIC_SystemReset();
        }
    }
    return 0;  // Never reached
}
