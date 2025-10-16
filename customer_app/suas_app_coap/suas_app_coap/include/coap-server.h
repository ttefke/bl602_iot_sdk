#ifndef __COAP_SERVER_H
#define __COAP_SERVER_H

// Should already be defined by lwippools.h
// This is just to make sure the constant is defined
#ifndef MEMP_NUM_COAPSESSION
#define MEMP_NUM_COAPSESSION 2
#endif

extern "C" void task_coap_server([[gnu::unused]] void *pvParameters);
#endif