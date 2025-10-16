extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>
}

#include <etl/string.h>

// Our ADC header
#include "adc.h"

/* Size of the stack for the task */

extern "C" void bfl_main(void) {
  /* Set up ADC reading task */
  constexpr uint16_t ADC_STACK_SIZE = 512;
  constinit static StackType_t adc_stack[ADC_STACK_SIZE]{};
  constinit static StaticTask_t adc_task{};

  /* Initialize system */
  vInitializeBL602();

  /* Start up ADC task */

  /* Create the task */
  xTaskCreateStatic(
      task_adc, /* name of the function implementing the task ->
                   defined in adc.c */
      etl::string_view("adc").data(), /* human readable name of the task */
      ADC_STACK_SIZE,                 /* Stack size */
      nullptr,   /* parameters for the function -> not required here */
      15,        /* Task priority */
      adc_stack, /* Stack to use for the task */
      &adc_task  /* Task handle (could be referenced in API calls
                    later, e.g. for changing its priority )*/
  );

  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
