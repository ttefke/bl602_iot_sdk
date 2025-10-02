// FreeRTOS headers
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// GPIO header
#include <bl_gpio.h>

// Driver header
#include <suas_rfid.h>

// SPI header
#include "spi.h"

/* 
    Connection between BL602 and RFID-RCC522:
    BL602 pin   RFID-RCC522     Functionality
    3V3         3.3V            Power supply
    GND         GND             Power supply
    2           SDA             Chip select
    3           SCK             Serial clock line
    1           MOSI            Communication PineCone -> RFID sensor
    4           MISO            Communication RFID sensor -> PineCone
    5           RST             Used to reset the RFID sensor
*/

#define MAX_CARD_LEN 10 // RFID UIDs have a length of at most 10

// Allow all blue keys
static uint8_t allowed_cards[5][MAX_CARD_LEN] = {
    {0x32, 0x0D, 0x0E, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x32, 0xA9, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x94, 0x20, 0xB2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xD1, 0x8B, 0xB2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xE2, 0xCD, 0xB5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

// Check if the card is in the list of allowed cards
static bool is_card_allowed(uid_t card) {
    // Iterates over all cards
    for (uint8_t i = 0; i < sizeof(allowed_cards) / MAX_CARD_LEN; i++) {
        // Iterate over the individual bytes of each card
        for (uint8_t j = 0; j < card.size; j++) {
            if (allowed_cards[i][j] != card.uid_byte[j]) {
                break; // go to outer loop to try for next card
            } else if (j == (card.size - 1)) {
                // We are at the last byte and this as well as all previous bytes matched
                return true;
            }
        }
    }
    return false;
}

// Our task handle
void spi_proc([[gnu::unused]] void *pvParameters) {
    // Turn LEDs off
    bl_gpio_output_set(LED_GREEN, LED_OFF);
    bl_gpio_output_set(LED_RED, LED_OFF);

    // Wait until event loop initialized SPI handle
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize card reader
    suas_rfid_init();
    printf("Scanning for cards...\r\n");

    while (1) {
        // Frequently check for new cards
        if (suas_rfid_is_new_card_present() && suas_rfid_read_card()) {
            printf("Card is present: ");

            // Get id stored on the card and show it
            uid_t card_data = suas_get_uid();
            for (uint8_t i = 0; i < card_data.size; i++) {
                printf("%02x", card_data.uid_byte[i]);
            }

            printf("\r\n");

            // Check if LED is in allowed_cards array and set LEDs accordingly
            if (is_card_allowed(card_data)) {
                bl_gpio_output_set(LED_GREEN, LED_ON);
                bl_gpio_output_set(LED_RED, LED_OFF);
            } else {
                bl_gpio_output_set(LED_GREEN, LED_OFF);
                bl_gpio_output_set(LED_RED, LED_ON);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    vTaskDelete(NULL);
}