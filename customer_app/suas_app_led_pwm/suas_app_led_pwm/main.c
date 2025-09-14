// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

/* Size of the stack for the task */
#define LED_STACK_SIZE 512

void bfl_main(void)
{  
  /* Set up LED task */
  static StackType_t led_stack_pwm[LED_STACK_SIZE];
  static StaticTask_t led_task_pwm;

  // Initialize system
  vInitializeBL602();
  
  /* Start up LED task */
  extern void task_led_pwm(void *pvParameters); 
  /* This function is defined in led.c but add prototype here so that the compiler knows we define this in another file.
  You could also use a header file for this */

  /* Create the task */
  xTaskCreateStatic(
    task_led_pwm, /* name of the function implementing the task -> defined in led_pwm.c */
    (char*)"led pwm", /* human readable name of the task */
    LED_STACK_SIZE, /* Stack size */
    NULL, /* parameters for the function -> not required here */
    15, /* Task priority */
    led_stack_pwm, /* Stack to use for the task */
    &led_task_pwm /* Task handle (could be referenced in API calls later, e.g. for changing its priority )*/    
  );
  
  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
