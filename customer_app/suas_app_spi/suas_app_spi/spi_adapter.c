// FreeRTOS
#include <FreeRTOS.h>

// Logging
#include <blog.h>

// VFS
#include <vfs.h>
#include <device/vfs_spi.h>

// DMA memory management
#include <bl_dma.h>

// GPIO
#include <bl_gpio.h>

// Own header
#include "spi_adapter.h"

// See manual Section 8.1.2.2
// Write multiple bytes to one register
int8_t spi_write_registers(uint8_t reg, uint8_t* bytes, uint8_t len)
{
    int8_t result = 0;
    uint8_t tx_data[1];
    uint8_t rx_data[1];

    tx_data[0] = reg;
    rx_data[0] = 0x00;

    taskENTER_CRITICAL();

    // Select chip before transfering data
    bl_gpio_output_set(CS_PIN, CS_ENABLE);
    result = spi_transfer(tx_data, rx_data, 1);

    for (uint8_t index = 0; index < len; index++) {
        tx_data[0] = bytes[index];
        result = spi_transfer(tx_data, rx_data, 1);
    }

    // Deselect chip
    bl_gpio_output_set(CS_PIN, CS_DISABLE);
    taskEXIT_CRITICAL();
    return result;
}

// Write one byte to one register
int8_t spi_write_register(uint8_t reg, uint8_t byte)
{
    uint8_t tx_data[2] = {reg, byte};
    uint8_t rx_data[2] = {0x00, 0x00};

    taskENTER_CRITICAL();

    // Select chip before transfering data
    bl_gpio_output_set(CS_PIN, CS_ENABLE);
    int8_t result = spi_transfer(tx_data, rx_data, 2);
    
    // Deselect chip
    bl_gpio_output_set(CS_PIN, CS_DISABLE);
    taskEXIT_CRITICAL();

    return result;
}

// See manual Section 8.1.2.1
// Read one byte from register
uint8_t spi_read_register(uint8_t reg)
{
    // Read mode for register -> Set MSB to 1
    reg = 0x80 | reg;
    uint8_t tx_data[2] = {reg, 0x00};
    uint8_t rx_data[2] = {0x00, 0x00};
    
    taskENTER_CRITICAL();

    // Select chip before transfering data
    bl_gpio_output_set(CS_PIN, CS_ENABLE);
    spi_transfer(tx_data, rx_data, 2);

    // Deselect chip
    bl_gpio_output_set(CS_PIN, CS_DISABLE);
    taskEXIT_CRITICAL();
    
    return rx_data[1];
}

// Read register multiple times
void spi_read_registers(uint8_t reg, uint8_t len, uint8_t *bytes, uint8_t rx_align)
{
    if (len == 0) {
        return;
    }

    reg = 0x80 | reg; // read mode
    uint8_t index = 0;
    uint8_t data = 0;
    len--;

    taskENTER_CRITICAL();

    // Select chip before transfering data
    bl_gpio_output_set(CS_PIN, CS_ENABLE);
    spi_transfer(&reg, 0x00, 1);

    if (rx_align) {
        uint8_t mask = (0xFF << rx_align) & 0xFF;
        
        // read same address again
        spi_transfer(&reg, &data, 1);
        bytes[0] = (bytes[0] & ~mask) | (data & mask);
        index++;
    }

    while (index < len) {
        spi_transfer(&reg, &data, 1);
        bytes[index] = data;
        index++;
    }
    spi_transfer(0x00, &data, 1);
    bytes[index] = data;

    // Deselect chip
    bl_gpio_output_set(CS_PIN, CS_DISABLE);
    taskEXIT_CRITICAL();
}

/*
Main SPI transfer function
buffer_tx: Buffer containing data to be transmitted
buffer_rx: Buffer to read data into
len: length of buffer_tx and buffer_rx (must be equal)
*/
int8_t spi_transfer(uint8_t *buffer_tx, uint8_t *buffer_rx, uint8_t len)
{
    int spi_dev = -1;
    int result;
    struct spi_ioc_transfer *transfer = NULL;

    // Input validation
    if ((!buffer_tx) || (!buffer_rx)) {
        blog_error("No valid buffers received\r\n");
        return -1;
    }

    // Open SPI device
    spi_dev = aos_open("/dev/spi0", 0);
    if (spi_dev < 0) {
        blog_error("Can not open spi_dev: %d\r\n", spi_dev);
        return -1;
    }

    // Create transfer data structure
    // Must be created with bl_dma_mem_malloc and freed with bl_dma_mem_free
    // as this data is transferred directly with DMA
    transfer = (struct spi_ioc_transfer*)
        bl_dma_mem_malloc(2 * (sizeof(struct spi_ioc_transfer)));
    
    if (!transfer) {
        blog_error("Memory error\r\n");
        return -1;
    }

    // Fill transfer data structure
    transfer[0].tx_buf = (uint32_t) buffer_tx;
    transfer[0].rx_buf = (uint32_t) buffer_rx;
    transfer[0].len = len;

    // Request data transfer
    // Data is transferred in background with DMA
    result = aos_ioctl(spi_dev, IOCTL_SPI_IOC_MESSAGE(1),
        (unsigned long)&transfer[0]);

    // Free data structures
    bl_dma_mem_free(transfer);
    aos_close(spi_dev);
    
    // Check result of data transfer
    if (result != 0) {
        blog_error("SPI write failure");
        return -1;
    }
    return 0;
}