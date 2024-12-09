#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>

#include <lwip/apps/httpd.h>


/* hello task */
void task_httpd(void *pvParameters)
{
  /* wait for half a second */
  printf("Starting up httpd\r\n");
  
  httpd_init();
  
  /* initialize CGI */
  while(1) {
    printf("httpd keepalive\r\n");
    vTaskDelay(5*60*1000);
  }
  
  /* will never happen */
  vTaskDelete(NULL);
}
