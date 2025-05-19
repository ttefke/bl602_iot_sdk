// FreeRTOS headers
#include <FreeRTOS.h>
#include <task.h>

// Boolean data type
#include <stdbool.h>

// Standard input/output
#include <stdio.h>

// Own headers
#include "spi.h"
#include "spi_adapter.h"

// Driver is inspired from here:
// https://github.com/miguelbalboa/rfid/
// Selected parts were adapted for the PineCode

// Resets the card reader
void rc522_reset() {
    uint8_t count = 0;
    spi_write_register(RF522_COMMAND_REG, RF522_SOFT_RESET);
    do {
        // delay 75 ms
        vTaskDelay(75 / portTICK_PERIOD_MS);
    } while ((spi_read_register(RF522_COMMAND_REG) & (1 << 4)) && (++count) < 3);
}

// Turns on antenna of card reader
void rc522_antenna_on() {
    uint8_t value = spi_read_register(RF522_TX_CONTROL_REG);

    if ((value & 0x03) != 0x03) {
        spi_write_register(RF522_TX_CONTROL_REG, value | 0x03);
    }
}

// Initializes the card reader
void rc522_init() {
    // Perform soft reset
   rc522_reset();

   // Reset baud rates
   spi_write_register(RF522_TX_MODE_REG, 0x00);
   spi_write_register(RF522_RX_MODE_REG, 0x00);

   // Reset mod width reg
   spi_write_register(RF522_MOD_WIDTH_REG, 0x26);

   // Set timeouts
   spi_write_register(RF522_T_MODE_REG, 0x80);
   spi_write_register(RF522_T_PRESCALER_REG, 0xA9);
   spi_write_register(RF522_T_RELOAD_REG_H, 0x03);
   spi_write_register(RF522_T_RELOAD_REG_L, 0xE8);
   spi_write_register(RF522_TX_ASK_REG, 0x40);
   spi_write_register(RF522_T_MODE_REG, 0x3D);

    // Turn on antenna
    rc522_antenna_on();
}

// Clear register bit mask
void rf522_clear_register_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = spi_read_register(reg);
    spi_write_register(reg, tmp & (~mask)); // clear bit mask
}

// Set register bit mask
void rf522_set_register_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = spi_read_register(reg);
    spi_write_register(reg, tmp | mask);
}

// Cyclic redundancy check: Detect possible errors during data transmission
uint rf522_calculate_crc(uint8_t *data, uint8_t len, uint8_t *result)
{
    spi_write_register(RF522_COMMAND_REG, RF522_IDLE);
    spi_write_register(RF522_DIV_IRQ_REG, 0x04);
    spi_write_register(RF522_FIFO_LEVEL_REG, 0x80);
    spi_write_registers(RF522_FIFO_DATA_REG, data, len);
    spi_write_register(RF522_COMMAND_REG, RF522_CALC_CRC);

    for (uint8_t attempts = 10; attempts > 0; attempts--) {
        uint8_t n = spi_read_register(RF522_DIV_IRQ_REG);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (n & 0x04) {
            spi_write_register(RF522_COMMAND_REG, RF522_IDLE);
            result[0] = spi_read_register(RF522_CRC_RESULT_REG_L);
            result[1] = spi_read_register(RF522_CRC_RESULT_REG_H);
            return RF522_STATUS_OK;
        }
    };

    return RF522_STATUS_TIMEOUT;
}

// Communicate with the card
uint8_t rf522_communicate_with_picc(uint8_t command, uint8_t waitIRq, uint8_t *send_data,
    uint8_t send_len, uint8_t *back_data, uint8_t *back_len, uint8_t *valid_bits,
    uint8_t rx_align, bool checkCRC)
{
    uint8_t tx_last_bits = *valid_bits ? *valid_bits : 0;
    uint8_t bit_framing = (rx_align << 4) + tx_last_bits;

    spi_write_register(RF522_COMMAND_REG, RF522_IDLE); // stop any active command
    spi_write_register(RF522_COM_IRQ_REG, 0x7F); // clear all interrupt request bits
    spi_write_register(RF522_FIFO_LEVEL_REG, 0x80); // flush buffer, fifo initialization
    spi_write_registers(RF522_FIFO_DATA_REG, send_data, send_len); // write send_data to fifo
    spi_write_register(RF522_BIT_FRAMING_REG, bit_framing);
    spi_write_register(RF522_COMMAND_REG, command);

    if (command == RF522_TRANSCEIVE) {
        rf522_set_register_bit_mask(RF522_BIT_FRAMING_REG, 0x80);
    }

    bool completed = false;

    for (uint8_t attempts = 5; attempts > 0; attempts--) {
        uint8_t n = spi_read_register(RF522_COM_IRQ_REG);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (n & waitIRq) {
            completed = true;
            break;
        }
        if (n & 0x01) {
            printf("Error. Timeout while reading card\r\n");
            return RF522_STATUS_TIMEOUT;
        }
    }
    
    // Timeout
    if (!completed) {
        return RF522_STATUS_TIMEOUT;
    }

    // Detect errors except collisions
    uint8_t error_reg_value = spi_read_register(RF522_ERROR_REG);
    if (error_reg_value & 0x13) {
        printf("Error happened while reading card.\r\n");
        return RF522_STATUS_ERROR;
    }

    uint8_t _valid_bits = 0;

    if (back_data && back_len) {
        uint8_t n = spi_read_register(RF522_FIFO_LEVEL_REG);

        if (n > *back_len) {
            printf("Error. Read too much data.\r\n");
            return RF522_STATUS_NO_ROOM;
        }

        *back_len = n;
        spi_read_registers(RF522_FIFO_DATA_REG, n, back_data, rx_align);
        _valid_bits = spi_read_register(RF522_CONTROL_REG) & 0x07;
        if (valid_bits) {
            *valid_bits = _valid_bits;
        }
    }

    // Collisions
    if (error_reg_value & 0x08) {
        printf("Error. Collision.\r\n");
        return RF522_STATUS_COLLISION;
    }

    // Perform crc_a validation if required

    if (back_data && checkCRC) {
        if (*back_len == 1 && _valid_bits == 4) {
            printf("Error. Not acknowledged.\r\n");
            return RF522_STATUS_MIFARE_NACK;
        }
        if (*back_len < 2 || _valid_bits != 0) {
            printf("Error. CRC wrong.\r\n");
            return RF522_STATUS_CRC_WRONG;
        }
        uint8_t control_buffer[2];
        uint8_t status = rf522_calculate_crc(&back_data[0], *back_len -2, &control_buffer[0]);
        if (status != RF522_STATUS_OK) {
            printf("Error. CRC validation not successful.\r\n");
            return status;
        }
        if ((back_data[*back_len -2] != control_buffer[0]) || (back_data[*back_len -1] != control_buffer[1])) {

            printf("Error. CRC wrong.\r\n");
            return RF522_STATUS_CRC_WRONG;
        }
    }

    return RF522_STATUS_OK;
}

// Transceive data with card
uint8_t rf522_transceive_data(uint8_t *send_data, uint8_t send_len, uint8_t* back_data,
    uint8_t *back_len, uint8_t *valid_bits, uint8_t rx_align, bool checkCRC)
{
    uint8_t waitIRq = 0x30;
    return rf522_communicate_with_picc(RF522_TRANSCEIVE, waitIRq, send_data, send_len, back_data,
        back_len, valid_bits, rx_align, checkCRC);
}

// Wrapper / Adapter for transceive card function
uint8_t rf522_transceive_data_(uint8_t *send_data, uint8_t send_len, uint8_t* back_data,
    uint8_t *back_len, uint8_t *valid_bits)
{
    return rf522_transceive_data(send_data, send_len, back_data, back_len, valid_bits, 0, false);
}

// Send command to card
uint8_t rf522_reqa_or_wupa(uint8_t command, uint8_t *buffer_ATQA, uint8_t *buffer_size)
{
    uint8_t valid_bits = 7;
    uint8_t status_code;

    if ((buffer_ATQA == NULL) || (*buffer_size < 2)) {
        // buffer too small
        return RF522_STATUS_NO_ROOM;
    }

    rf522_clear_register_bit_mask(RF522_COLL_REG, 0x80);
    status_code = rf522_transceive_data_(&command, 1, buffer_ATQA, buffer_size, &valid_bits);
    if (status_code != RF522_STATUS_OK) {
        return status_code;
    }
    if ((*buffer_size != 2) || (valid_bits != 0)) {
        return RF522_STATUS_ERROR;
    }
    return RF522_STATUS_OK;
}

// Wrapper function for requesting information from card
uint8_t rf522_request_a(uint8_t *buffer_ATQA, uint8_t *buffer_size)
{
    return rf522_reqa_or_wupa(RF522_PICC_CMD_REQA, buffer_ATQA, buffer_size);
}

// Check if new card is present
uint8_t rc522_is_new_card_present()
{
    // Create buffer
    uint8_t buffer_ATQA[2];
    uint8_t buffer_size = sizeof(buffer_ATQA);

    // Reset baud rates
    spi_write_register(RF522_TX_MODE_REG, 0x00);
    spi_write_register(RF522_RX_MODE_REG, 0x00);

    // Reset mod width reg
    spi_write_register(RF522_MOD_WIDTH_REG, 0x26);

    uint8_t result = rf522_request_a(buffer_ATQA, &buffer_size);
    return (result == RF522_STATUS_OK || result == RF522_STATUS_COLLISION);
}

// Our task handle
void spi_proc(void *pvParameters) {
    // Wait until event loop initialized SPI handle
    vTaskDelay(2500);
    printf("Hello from spi proc\r\n");

    // Initialize card reader
    rc522_init();

    while (1) {
        // Frequently check for new cards
        if (rc522_is_new_card_present()) {
            printf("New card is present\r\n");
        } else {
            printf("No new card detect\r\n");
        }

        vTaskDelay(2000);
    }
    vTaskDelete(NULL);
}