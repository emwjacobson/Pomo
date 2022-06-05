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

    err = wifi_start_ap();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
        exit(1);
    }
}
