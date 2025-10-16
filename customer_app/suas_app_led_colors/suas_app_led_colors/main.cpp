extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>
}

#include <etl/string.h>

extern "C" void bfl_main(void) {
  /* Set up LED task */
  constexpr uint16_t LED_STACK_SIZE = 512;
  constinit static StackType_t led_stack[LED_STACK_SIZE]{};
  constinit static StaticTask_t led_task{};

  /* Initialize system */
  vInitializeBL602();

  /* Start up LED task */
  extern void task_led(void *pvParameters);
  /* This function is defined in led.c but add prototype here so that the
  compiler knows we define this in another file. You could also use a header
  file for this */

  /* Create the task */
  xTaskCreateStatic(
      task_led, /* name of the function implementing the task ->
                   defined in led.c */
      etl::string_view("led").data(), /* human readable name of the task */
      LED_STACK_SIZE,                 /* Stack size */
      nullptr,   /* parameters for the function -> not required here */
      15,        /* Task priority */
      led_stack, /* Stack to use for the task */
      &led_task  /* Task handle (could be referenced in API calls
                    later, e.g. for changing its priority )*/
  );

  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
