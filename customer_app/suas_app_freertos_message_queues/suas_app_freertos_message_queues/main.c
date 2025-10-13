// FreeRTOS includes
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

// Standard library
#include <inttypes.h>
#include <stdio.h>

// Stack size defintion
#define STACK_SIZE 128

/* Declaration of the task sending a number */
void task_sender(void *pvParameters) {
  /* Get queue from parameters */
  QueueHandle_t queue = (QueueHandle_t)pvParameters;

  /* Define value to send*/
  uint32_t value = 1;

  /* Define result we get */
  BaseType_t result;

  while (1) {
    /* Send data to tail of the queue*/
    result = xQueueSendToBack(
        /* Queue handle */ queue,
        /* Pointer to the value */ &value,
        /* Timeout in ms to wait until space is available in the queue */ 10);

    /* Get result*/
    if (result != pdPASS) {
      /* Sending data failed */
      printf("Could not send items to the queue\r\n");
    }

    /* Increase number */
    value++;

    /* Wait for 2.5 s */
    vTaskDelay(pdMS_TO_TICKS(2500));
  }

  vTaskDelete(NULL);
}

/* Declaration of the task receiving the number */
void task_receiver(void *pvParameters) {
  /* Get queue from parameters */
  QueueHandle_t queue = (QueueHandle_t)pvParameters;

  /* Define receiver value*/
  uint32_t value = 0;

  /* Define result we get*/
  BaseType_t result;

  while (1) {
    /* Check if there is data in the queue */
    if (uxQueueMessagesWaiting(queue) != 0) {
      /* Receive data from the queue */
      result = xQueueReceive(
          /* Queue to query */ queue,
          /* Pointer to value storing the retrieved data */ &value,
          /* Timeout in ms to wait until we got a value */ 100);

      /* Query our result whether we got information */
      if (result == pdPASS) {
        printf("Received number %" PRIu32 " from the queue\r\n", value);
      } else {
        printf("Could not receive data from queue/no data present\r\n");
      }
    }
    /* Wait for 2.5 s */
    vTaskDelay(pdMS_TO_TICKS(2500));
  }

  vTaskDelete(NULL);
}

/* Main function - entry point */
void bfl_main(void) {
  // Initialize system
  vInitializeBL602();

  /* Create a queue that can hold at max two uint32_t data types */
  QueueHandle_t numberQueue = xQueueCreate(
      /* Number of objects (queue length) */ 2,
      /* Size of each object in bytes */ sizeof(uint32_t));

  /* Check if queue was created and start tasks */

  if (numberQueue != NULL) {
    printf("Created queue, starting tasks\r\n");

    /* Set up a sender and a receiver task */
    xTaskCreate(
        task_sender,           /* function implementing the task */
        (char *)"sender task", /* human readable task name */
        STACK_SIZE,            /* maximum stack depth */
        (void *)numberQueue,   /* parameters handed over */
        10,                    /* task priority */
        NULL /* task handle (can be used to access the task later or null) */
    );

    xTaskCreate(task_receiver, (char *)"receiver task", STACK_SIZE,
                (void *)numberQueue, 10, NULL);

    // Start task scheduler -> starts tasks
    vTaskStartScheduler();
  } else {
    printf("Queue could not be created, terminating\r\n");
  }
}
