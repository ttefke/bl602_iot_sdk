#ifndef __SPI_ADAPTER_H
#define __SPI_ADAPTER_H

// CS pin definitions
#define CS_PIN      4
#define CS_ENABLE   0
#define CS_DISABLE  1

// Function prototypes
int8_t spi_write_registers(uint8_t reg, uint8_t* bytes, uint8_t len);
int8_t spi_write_register(uint8_t reg, uint8_t byte);
uint8_t spi_read_register(uint8_t reg);
void spi_read_registers(uint8_t reg, uint8_t len, uint8_t *bytes, uint8_t rx_align);
int8_t spi_transfer(uint8_t *buffer_tx, uint8_t *buffer_rx, uint8_t len); 
#endif
