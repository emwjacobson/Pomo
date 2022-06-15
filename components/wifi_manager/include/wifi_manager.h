#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <esp_http_server.h>

#define PATH_MAX_LENGTH ESP_VFS_PATH_MAX+128
#define SCRATCH_BUFSIZE (10240)
#define DEFAULT_SCAN_LIST_SIZE 24
#define PING_WAIT_DELAY_MS 250

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

esp_err_t wifi_init(void);
bool wifi_is_configured(void);
esp_err_t wifi_connect_to_ap(const char ssid[32], const char password[32]);
esp_err_t wifi_start_ap(void);
esp_err_t wifi_start_config_server(void);

#endif