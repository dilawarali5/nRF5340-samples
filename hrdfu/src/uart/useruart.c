
#include "useruart.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>


static struct device *uart_dev;
uint8_t serial_rx_buf[MY_RING_BUF_SIZE];
uint8_t OTABuffer[MY_RING_BUF_SIZE];
uint32_t OTADataSize = 0;
uint32_t lastInterruptTime = 0;
uint32_t FileSize = 0;
uint32_t totalBytesReceived = 0;
K_MSGQ_DEFINE(uartEventQueue, sizeof(uint32_t), 64, 1);

static void uart_irq_handler(const struct device *dev, void *context);


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

void SendUartData(uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        uart_poll_out(uart_dev, data[i]);
    }
}

int waitForUartMessageQueue(void)
{
    uint32_t msg;
    int err = k_msgq_get(&uartEventQueue, &msg, K_FOREVER);
    return err;
}

void ResetRxBuffer(void)
{
    totalBytesReceived = 0;
}