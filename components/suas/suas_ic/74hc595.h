#if WITH_SUAS_74HC595

#ifndef SUAS_74HC595_H
#define SUAS_74HC595_H

#include <stdint.h>

struct config_74hc595 {
    /* Obligatory pins */
    uint8_t pin_data_signal;
    uint8_t pin_shift_clock;
    uint8_t pin_store_clock;

    /* Optional pins*/
    uint8_t with_output_enable;
    uint8_t pin_output_enable;

    uint8_t with_master_reset;
    uint8_t pin_master_reset;

    /* Number of registers for cascade */
    uint8_t number_of_registers;
};

void configure_74hc595(struct config_74hc595 *config);

void store_data_74hc595(struct config_74hc595 *config, uint8_t data);

void clear_74hc595(struct config_74hc595 *config);

#endif
#endif
