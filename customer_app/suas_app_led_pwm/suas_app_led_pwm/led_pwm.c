// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>

// PWM library
#include <bl_pwm.h>

// Define LED pins
#define LED_R_PIN 17
#define LED_R_PWM_CHANNEL 2

// frequency
#define PWM_FREQUENCY 2048
#define PWM_FREQUENCY_DIVIDER 4

/* LED task */
void task_led_pwm([[gnu::unused]] void *pvParameters)
{
  printf("LED PWM task started\r\n");
  
  /* Usage: bl_pwm_init(PWM channel -> see manual p. 27, pin, frequency (must be between 2,000 and 800,000)) */
  bl_pwm_init(LED_R_PWM_CHANNEL, LED_R_PIN, PWM_FREQUENCY);
  
  // Set frequency to 512Hz: set 4 as divider value -> real frequency = 2048/4 = 512Hz
  PWM_Channel_Set_Div(LED_R_PWM_CHANNEL, PWM_FREQUENCY_DIVIDER);
  
  /* Set duty cycle
     duty cycle must be between 0 and 100
  */
  bl_pwm_set_duty(LED_R_PWM_CHANNEL, 1);

  // Start PWM operations
  bl_pwm_start(LED_R_PWM_CHANNEL);
  
  while (1) {
    for (short value = 0; value < 100; value++) {          
      /* Set duty cycle
        duty cycle must be between 0 and 100
      */
      bl_pwm_set_duty(LED_R_PWM_CHANNEL, value);
         
      /* Wait for 20ms */
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
  
  
  // should never happen but would delete the task and free allocated resources
  vTaskDelete(NULL);
}
