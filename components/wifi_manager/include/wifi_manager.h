#include <esp_http_server.h>

esp_err_t wifi_init(void);
bool wifi_is_configured(void);
esp_err_t wifi_start_ap(void);
esp_err_t wifi_start_station(void);
static esp_err_t config_get_handler(httpd_req_t* req);
esp_err_t wifi_start_config_server(void);