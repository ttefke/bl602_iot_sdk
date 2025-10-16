extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>
}

#include <etl/string.h>

extern "C" void bfl_main(void) {
  /* Set up servo task */
  constexpr uint16_t SERVO_STACK_SIZE = 512;
  constinit static StackType_t servo_stack_pwm[SERVO_STACK_SIZE]{};
  constinit static StaticTask_t servo_task_pwm{};

  // Initialize system
  vInitializeBL602();

  /* Start up servo task */
  extern void task_servo_pwm(void *pvParameters);
  /* This function is defined in servo_pwm.c but add prototype here so that the
  compiler knows we define this in another file. You could also use a header
  file for this */

  /* Create the task */
  xTaskCreateStatic(
      task_servo_pwm, /* name of the function implementing the task -> defined
                         in servo_pwm.c */
      etl::string_view("servo pwm")
          .data(),      /* human readable name of the task */
      SERVO_STACK_SIZE, /* Stack size */
      nullptr,          /* parameters for the function -> not required here */
      15,               /* Task priority */
      servo_stack_pwm,  /* Stack to use for the task */
      &servo_task_pwm   /* Task handle (could be referenced in API calls later,
                           e.g. for changing its priority )*/
  );

  /* Also start the task (task creation only creates the task control block) */
  vTaskStartScheduler();
}
