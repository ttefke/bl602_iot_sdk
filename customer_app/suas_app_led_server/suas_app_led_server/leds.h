#ifndef LED_DEFS_H
#define LED_DEFS_H

/* LED pins*/
#define LED_R_PIN 17
#define LED_G_PIN 14
#define LED_B_PIN 11

#define LED_ON 0
#define LED_OFF 1

/* data structures */
enum selected_led {
   LED_RED,
   LED_GREEN,
   LED_BLUE
};

struct led_state {
   uint8_t state_led_red;
   uint8_t state_led_green;
   uint8_t state_led_blue;
} led_state;

/* function to apply current led_state*/
void apply_led_state();
#endif