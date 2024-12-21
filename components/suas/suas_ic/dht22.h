#if WITH_SUAS_DHT22

#define DHT22_VOLTAGE_HIGH 1
#define DHT22_VOLTAGE_LOW 0

#define DHT22_ENABLE_PULLUP 1
#define DHT22_DISABLE_PULLUP 0

#define DHT22_ENABLE_PULLDOWN 1
#define DHT22_DISABLE_PULLDOWN 0

double getTemperature(uint8_t data_pin);
double getHumidity(uint8_t data_pin);
#endif