#ifndef __COAP__CLIENT_H
#define __COAP__CLIENT_H

// Define URI to request data from
#ifdef COAP_URI
#undef COAP_URI // undefine standard URI
#endif
#define COAP_URI "coap://192.168.169.1/random"

void task_coap_client(void *pvParameters);
#endif