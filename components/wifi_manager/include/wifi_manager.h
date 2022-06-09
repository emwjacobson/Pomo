#include <esp_http_server.h>

esp_err_t wifi_init(void);
bool wifi_is_configured(void);
esp_err_t wifi_connect_to_ap(const char ssid[32], const char password[32]);
esp_err_t wifi_start_ap(void);
esp_err_t wifi_start_config_server(void);