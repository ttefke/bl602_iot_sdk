#include <stdio.h>
#include <string.h>
#include <lwip/pbuf.h>

#if WITH_SNIFFER
void sniffer_inspect(struct pbuf *p) {
  // 1. Create pointer we work with pointing to the packet
  struct pbuf *t = p;

  // 2. Print packet (which is then collected by the monitor)
  // 2.1 Ensure we start with a new line on the serial line
  printf("\r\n\r\n");

  // 2.2 Our custom header to differentiate packets from normal printf statements
  printf("#pkt#");

  // 2.3 Iterate over all packet structures
  do {
    // 2.3.1 Copy packet to own data structure
    uint8_t *pkt = (uint8_t *) malloc(sizeof(uint8_t) * t->len);
    memcpy(pkt, t->payload, t->len);

    // 2.3.2 And print it
    for (uint16_t i = 0; i < t->len; i++) {
      printf("%02x", pkt[i]);
    }

    // 2.3.3 Afterwards, free it
    free(pkt);
    pkt = NULL;

    // 2.3.4 Now evaluate the next buffer
    t = t->next;
  } while(t != NULL); // 2.3.5 Stop if there is no next buffer

  // 2.4 Ensure the line ends here
  printf("\r\n");
}
#endif