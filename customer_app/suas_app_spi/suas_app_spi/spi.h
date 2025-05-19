#ifndef __SPI_H
#define __SPI_H

// Registers: See Section 9.2 of manual

// Page 0: Command and statuses
#define RF522_COMMAND_REG               (0x01 << 1)
#define RF522_COM_IRQ_REG               (0x04 << 1)
#define RF522_DIV_IRQ_REG               (0x05 << 1)
#define RF522_ERROR_REG                 (0x06 << 1)
#define RF522_FIFO_DATA_REG             (0x09 << 1)
#define RF522_FIFO_LEVEL_REG            (0x0A << 1)
#define RF522_CONTROL_REG               (0x0C << 1)
#define RF522_BIT_FRAMING_REG           (0x0D << 1)
#define RF522_COLL_REG                  (0x0E << 1)

// Page 1: Command
#define RF522_TX_MODE_REG               (0x12 << 1)
#define RF522_RX_MODE_REG               (0x13 << 1)
#define RF522_TX_CONTROL_REG            (0x14 << 1)
#define RF522_TX_ASK_REG                (0x15 << 1)

// Page 2: configuration
#define RF522_CRC_RESULT_REG_H          (0x21 << 1)
#define RF522_CRC_RESULT_REG_L          (0x22 << 1)
#define RF522_MOD_WIDTH_REG             (0x24 << 1)
#define RF522_T_MODE_REG                (0x2A << 1)
#define RF522_T_PRESCALER_REG           (0x2B << 1)
#define RF522_T_RELOAD_REG_H            (0x2C << 1)
#define RF522_T_RELOAD_REG_L            (0x2D << 1)

// Command set: See Section 10
#define RF522_IDLE                      (0x00)
#define RF522_CALC_CRC                  (0x03)
#define RF522_TRANSCEIVE                (0x0C)
#define RF522_SOFT_RESET                (0x0F)

// PICC commands
#define RF522_PICC_CMD_REQA             (0x26)
#define RF522_PICC_CMD_CT               (0x88)
#define RF522_PICC_CMD_SEL_CL1          (0x93)
#define RF522_PICC_CMD_SEL_CL2          (0x95)
#define RF522_PICC_CMD_SEL_CL3          (0x97)
#define RF522_PICC_CMD_HLTA             (0x50)

// Status: return codes
#define RF522_STATUS_OK                 1
#define RF522_STATUS_ERROR              2
#define RF522_STATUS_COLLISION          3
#define RF522_STATUS_TIMEOUT            4
#define RF522_STATUS_NO_ROOM            5
#define RF522_STATUS_INTERNAL_ERROR     6
#define RF522_STATUS_CRC_WRONG          7
#define RF522_STATUS_MIFARE_NACK        8

// Function prototype for task handle
void spi_proc(void *pvParameters);

#endif