// Include bluetooth definition for bt_mesh_output_action_t
#include <main.h>

// GPIO HAL
#include <bl_gpio.h>

// Standard input/output
#include <stdio.h>

// Include our own header for PIN definitions
#include "include/board.h"

// GPIO light configuration
// Reset GPIO pins: turn off all lights
void board_reset_leds() {
    bl_gpio_output_set(LED_RED_PIN, 1);
    bl_gpio_output_set(LED_GREEN_PIN, 1);
    bl_gpio_output_set(LED_BLUE_PIN, 1);
}

// Initialize GPIO pins to be able to output information
void board_init() {
    bl_gpio_enable_output(LED_RED_PIN, 0, 0);
    bl_gpio_enable_output(LED_GREEN_PIN, 0, 0);
    bl_gpio_enable_output(LED_BLUE_PIN, 0, 0);
    board_reset_leds();
}

// Output number callback: turn off all LEDs
void board_output_number(bt_mesh_output_action_t action, uint32_t number) {
    printf("[BOARD] Output number callback function called\r\n");
    board_reset_leds();
}

// Board is provisioned: turn on blue LED
void board_prov_complete(void) {
    printf("[BOARD] Provisioned complete callback function called\r\n");
    board_reset_leds();
    bl_gpio_output_set(LED_BLUE_PIN, 0);
}

// Provisioning reset: turn on red LED
void board_prov_reset() {
    printf("[BOARD] Provision reset callback function called\r\n");
    board_reset_leds();
    bl_gpio_output_set(LED_RED_PIN, 0);
}

// Change state: turn green LED on or off
void board_change_state(uint8_t new_state) {
    // Turn off all LEDs
    board_reset_leds();
    
    // Turn green LED on or off
    // Note: LED turns on with 0, off with 1
    // So we have to reverse the state here
    uint8_t led_state = new_state == 0 ? 1 : 0;
    bl_gpio_output_set(LED_GREEN_PIN, led_state);
}