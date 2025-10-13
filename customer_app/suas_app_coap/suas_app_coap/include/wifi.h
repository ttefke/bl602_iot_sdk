#ifndef __WIFI_H
#define __WIFI_H

// Credentials
// Do not hardcode credentials in production code
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PW "YOUR_PASSWORD"

// Wifi modes
enum app_wifi_role { UNINITIALIZED = 0, AP, STA };

// Function prototypes
void task_wifi(void *pvParameters);
#endif
