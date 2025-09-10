#include <coap3/coap.h>

const char *const coap_log_str[] = {"Emerg", "Alert", "Crit", "Err", "Warn", "Notice", "Info", "Dbg", "Oscore", "DtlsBase", "Unknown"};
void coap_log_handler(coap_log_t level, const char *message) {
  printf("[%s] %s: %s", __func__, coap_log_str[level], message);
  printf("\r\n");
}