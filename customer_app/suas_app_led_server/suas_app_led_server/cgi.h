#ifndef TT_CGI_H
#define TT_CGI_H 1

/* show errors if dependencies are not included */

#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif

#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif

#if !LWIP_HTTPD_CGI
#error This needs LWIP_HTTPD_CGI
#endif

/* endpoints */
#define LED_ENDPOINT "/led.html"
#define ERROR_404_ENDPOINT "/404.html"

/* setters */
#define SET_LED_ENDPOINT "/set_led"

/* getters */
#define GET_LED_STATUS_ENDPOINT "/led_state.json"


/* initialization functions */
void custom_files_init(void);
void cgi_init(void);

#endif
