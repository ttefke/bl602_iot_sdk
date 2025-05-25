#ifndef __BOARD_H
#define __BOARD_H

// Include main.h file from blemesh to define bt_mesh_output_action_t
#include <main.h>

// LED GPIO pins
#define LED_RED_PIN     17
#define LED_GREEN_PIN   14
#define LED_BLUE_PIN    11

// Function prototypes
void board_init();
void board_output_number(bt_mesh_output_action_t action, uint32_t number);
void board_prov_complete();
void board_prov_reset();
void board_change_state(uint8_t new_state);
#endif