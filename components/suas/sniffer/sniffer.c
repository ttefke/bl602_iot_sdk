#include <FreeRTOS.h>
#include <task.h>

#include <stdint.h>
#include <stdio.h>

// Wifi sniffer
void sniffer_suas_cb(void *env, uint8_t *pkt, int len)
{
  static unsigned int sniffer_counter;
  sniffer_counter++;

  taskENTER_CRITICAL();
  printf("#pkt#");

  for (int i = 0; i < len; i++) {
    printf("%02x", pkt[i]);
  }

  taskEXIT_CRITICAL();

  printf("\r\n");
}