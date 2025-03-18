
 /* Includes ----------------------------------------------------------- */
#include "useruart.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>

/* Private defines ---------------------------------------------------- */
/* Private enumerate/structure ---------------------------------------- */
/* Private macros ----------------------------------------------------- */
/* Public variables --------------------------------------------------- */
/* Private variables -------------------------------------------------- */
static struct device *uart_dev;
uint8_t serial_rx_buf[MY_RING_BUF_SIZE];
uint8_t OTABuffer[MY_RING_BUF_SIZE];
uint32_t OTADataSize = 0;
uint32_t lastInterruptTime = 0;
uint32_t FileSize = 0;
uint32_t totalBytesReceived = 0;
K_MSGQ_DEFINE(uartEventQueue, sizeof(uint32_t), 64, 1);

/* Private function prototypes ---------------------------------------- */

static void uart_irq_handler(const struct device *dev, void *context);

/* Private function definitions ---------------------------------------- */

/**
 * @brief UART interrupt handler callback function
 *
 * Called when UART interrupts occur. Reads available data from the UART FIFO
 * one byte at a time and stores it in serial_rx_buf. Updates the last interrupt
 * time for timeout detection.
 *
 * @param dev Pointer to the UART device triggering the interrupt
 * @param context User-provided context data (unused)
 */
static void uart_irq_handler(const struct device *dev, void *context)
{
    ARG_UNUSED(context);

    // Bail if nothing to do.
    if (!uart_irq_update(dev))
    {
        printk("failed\r\n");
        return;
    }

    if (uart_irq_rx_ready(dev)) 
    {
        uart_fifo_read(dev, &serial_rx_buf[totalBytesReceived], 1);
        totalBytesReceived++;
    }
    lastInterruptTime = k_uptime_get_32();
}


/* Global Function definitions ----------------------------------------------- */

/**
 * @brief Validates the integrity of a received UART packet
 *
 * Examines the received data in serial_rx_buf to verify it follows the expected 
 * packet format with valid start and end bytes. If valid, copies the packet data 
 * to OTABuffer and processes any special packets (e.g., file size information).
 *
 * @return The packet type if valid, 0 if invalid packet
 */
int verifyPacket()
{

    uartPacket_t *packet = (uartPacket_t*)serial_rx_buf;

    if(packet->startByte == START_BYTE)
    {
        if(packet->packetSize < MAX_PACKET_SIZE)
        {
            printk("Packet size: %u\r\n", packet->packetSize);
            packet->endByte = serial_rx_buf[packet->packetSize + 10];
        }

        if(packet->endByte == END_BYTE)
        {
            printk("RX: %u, %u, %u\r\n", packet->packetNumber, packet->packetType, packet->packetSize);
            //copy data bytes to OTA buffer using memcpy
            memcpy(OTABuffer, packet->data, packet->packetSize);
            OTADataSize = packet->packetSize;

            if(packet->packetType == 0x02)
            {
                FileSize = OTABuffer[3] << 24 | OTABuffer[2] << 16 | OTABuffer[1] << 8 | OTABuffer[0];
                printk("File size: %u\r\n", FileSize);
            }
            return packet->packetType;
        }
        else
        {
            printk("Error: end byte not found\r\n");
            return 0;
        }
    }
    return 0;
}



/**
 * @brief Initializes the UART communication module
 *
 * Sets up the UART device, verifies it's ready, and configures the interrupt handler
 * to receive data. Enables UART receive interrupts.
 *
 * @return 0 on success, 1 if the device is not found or init fails
 */
int init_serial_comms_handler(void)
{
    int success = 0;
    uart_dev = (struct device *)device_get_binding("uart@9000");

    if (!device_is_ready(uart_dev))
    {
        printk("UART device not found!");
        success = 1;
    }
    else
    {
        /* configure interrupt and callback to receive data */
        success = uart_irq_callback_user_data_set(uart_dev, uart_irq_handler, NULL);
        uart_irq_rx_enable(uart_dev);
    }

    return success;
}

/**
 * @brief Sends data through the UART interface
 *
 * Transmits an array of bytes through the configured UART interface using polling mode.
 *
 * @param data Pointer to the array of bytes to transmit
 * @param size Number of bytes to transmit
 */
void SendUartData(uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        uart_poll_out(uart_dev, data[i]);
    }
}

/**
 * @brief Waits for a message in the UART event queue
 *
 * Blocks the calling thread until a message is available in the UART event queue.
 *
 * @return Error code from k_msgq_get operation
 */
int waitForUartMessageQueue(void)
{
    uint32_t msg;
    int err = k_msgq_get(&uartEventQueue, &msg, K_FOREVER);
    return err;
}

/**
 * @brief Resets the UART receive buffer counter
 *
 * Clears the bytes received counter to prepare for a new packet reception.
 */
void ResetRxBuffer(void)
{
    totalBytesReceived = 0;
}

/* End of file -------------------------------------------------------- */