#include <stdint.h>

#define MY_RING_BUF_SIZE     256
#define MAX_PACKET_SIZE      128


#define START_BYTE 0x24
#define END_BYTE   0x23


extern uint32_t totalBytesReceived;
extern uint8_t serial_rx_buf[MY_RING_BUF_SIZE];
extern uint32_t lastInterruptTime;
extern uint8_t OTABuffer[MY_RING_BUF_SIZE];
extern uint32_t OTADataSize;
extern uint32_t FileSize;

typedef struct __attribute__((__packed__))
{
    uint8_t startByte;
    uint32_t packetNumber;
    uint8_t packetType;
    uint32_t packetSize;
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t endByte;
}uartPacket_t;

int init_serial_comms_handler(void);
void SendUartData(uint8_t *data, uint32_t size);
int waitForUartMessageQueue(void);
void ResetRxBuffer(void);
int verifyPacket();