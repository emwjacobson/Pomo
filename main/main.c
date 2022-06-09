#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <esp_spiffs.h>

#include "wifi_manager.h"

static const char *TAG = "Main";

esp_err_t init_fs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_SETUP_FS_BASE,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error registering SPIFFS. Error: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void app_main(void) {
    esp_err_t err;

    err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing NVS flash. Error: %s", esp_err_to_name(err));
        return;
    }

    err = init_fs();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing filesystem. Error: %s", esp_err_to_name(err));
        return;
    }

    err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Wi-Fi. Error: %s", esp_err_to_name(err));
        return;
    }

    if (wifi_is_configured()) {
        // If wifi is configured, it should have stored info to connect to an AP

        // TODO: Implement this
    } else {
        // If wifi is not configured
        
        // First start the config access point
        err = wifi_start_ap();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
            return;
        }

        // Start the webserver for the config server
        err = wifi_start_config_server();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error starting config server. Error: %s", esp_err_to_name(err));
            return;
        }
    }

}
