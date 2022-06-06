#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#include "wifi_manager.h"

static const char *TAG = "Main";

void app_main(void) {
    esp_err_t err;

    err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing NVS flash. Error: %s", esp_err_to_name(err));
        exit(1);
    }

    err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Wi-Fi. Error: %s", esp_err_to_name(err));
        exit(1);
    }

    if (wifi_is_configured()) {
        // If wifi is configured, it should have stored info to connect to an AP
        err = wifi_start_station();
        if (err != ESP_OK) {
            // If there was an error, then we could not connect to the configured AP, so start the config AP
            ESP_LOGW(TAG, "Error connecting to configured AP, falling back to config mode. Error: %s", esp_err_to_name(err));
            err = wifi_start_ap();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
                exit(1);
            }
        } else {
            // If station connected, then continue with setup
        }
    } else {
        // If wifi is not configured, then start configuration AP
        err = wifi_start_ap();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
            exit(1);
        }

        wifi_start_config_server();
    }

}
