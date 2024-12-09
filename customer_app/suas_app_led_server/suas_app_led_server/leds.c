#include <stdio.h>
#include <bl_gpio.h>

#include "leds.h"

void apply_led_state()
{
    printf("Applying states: r: %d, g: %d, b: %d\r\n", led_state.state_led_red,
           led_state.state_led_green, led_state.state_led_blue);
    bl_gpio_output_set(LED_R_PIN, led_state.state_led_red);
    bl_gpio_output_set(LED_G_PIN, led_state.state_led_green);
    bl_gpio_output_set(LED_B_PIN, led_state.state_led_blue);
}