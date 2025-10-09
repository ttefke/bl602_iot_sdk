// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>

// Servo library
#include <suas_servo.h>

#define WAIT vTaskDelay(2000)

/* Connection setup
    Servo   PineCone
    GND     GND
    VCC     3V3
    IN      IO0
*/

/* Servo task */
void task_servo_pwm([[gnu::unused]] void *pvParameters)
{
  printf("Servo task started\r\n");

  /* 1. Create configuration for the servo driver */
  suas_servo_conf_t conf = {
    .pin = 0, // Pin of the GPIO pin to use
    .degrees = 0 // Initial degrees
  };

  /* 2. Initializie servo */
  suas_servo_init(&conf);

  /* 3. Set servo to 45 degrees after waiting for two seconds */
  WAIT;
  conf.degrees = 45;
  suas_servo_set_degrees(&conf);

  /* 4. Set servo to 90 degrees after waiting for two seconds */
  WAIT;
  conf.degrees = 90;
  suas_servo_set_degrees(&conf);

  /* 5. Set servo to 180 degrees after waiting for two seconds */
  WAIT;
  conf.degrees = 180;
  suas_servo_set_degrees(&conf);

  printf("Done\r\n");

  /* 6. End task */
  vTaskDelete(NULL);
}
