#ifndef __WIFI_H
#define __WIFI_H

// Warning: do not use hardcoded credentials in production code!
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PW "YOUR_PASSWORD"

// Wifi modes
enum app_wifi_role { UNINITIALIZED = 0, AP, STA };

// Function prototype
void task_wifi([[gnu::unused]] void *pvParameters);
#endif /* __WIFI_H */