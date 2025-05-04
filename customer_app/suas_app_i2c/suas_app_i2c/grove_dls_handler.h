// Header guards
#ifndef __GROVE_DLS_HANDLER_H
#define __GROVE_DLS_HANDLER_H

// Include for input_event_t data type
#include <aos/yloop.h>

// Function prototypes
void grove_handler(void *pvParameters);
void event_cb_i2c_interrupt(input_event_t *event, void *private_data);

#endif