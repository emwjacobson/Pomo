#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <mdns.h>

#include "wifi_manager.h"

static const char *TAG = "Wifi Manager";

esp_err_t wifi_init(void) {
    esp_err_t err;
    
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_netif_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_event_loop_create_default`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `mdns_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = mdns_hostname_set("pomo");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting mdns hostname. Error %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

bool wifi_is_configured(void) {
    return false;
}

esp_err_t wifi_start_ap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err;
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_wifi_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_WIFI_CONFIG_SSID,
            .ssid_len = strlen(CONFIG_WIFI_CONFIG_SSID),
            .channel = 6,
            .password = CONFIG_WIFI_CONFIG_PASSWORD,
            .max_connection = ESP_WIFI_MAX_CONN_NUM,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false
            }
        }
    };

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_mode`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_config`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_start`. Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Finished setting up wifi.");
    return ESP_OK;
}

esp_err_t wifi_start_station(void) {
    return ESP_ERR_NOT_FINISHED;
}

static esp_err_t config_get_handler(httpd_req_t* req) {
    httpd_resp_send(req, "Hello World!", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t config_index = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = config_get_handler,
    .user_ctx = NULL
};

esp_err_t wifi_start_config_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // config.lru_purge_enable = true; TODO: Decide if this stays

    esp_err_t err;
    err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting httpd server. Error: %s", esp_err_to_name(err));
        return err;
    }

    httpd_register_uri_handler(server, &config_index);

    return ESP_OK;
}
