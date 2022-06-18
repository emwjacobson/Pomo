#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <esp_spiffs.h>
#include <led_strip.h>

#include <led_manager.h>

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

static void IRAM_ATTR gpio_button_handler(void* arg) {
    ESP_DRAM_LOGI(TAG, "Called GPIO button handler");
    wifi_reset_config_ISR();
    return;
} 

esp_err_t init_gpio() {
    esp_err_t err;
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << 34),
        .pull_down_en = true,
        .pull_up_en = false
    };
    err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting GPIO config. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing GPIO ISR service. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = gpio_isr_handler_add(GPIO_NUM_34, gpio_button_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error adding ISR to GPIO");
        return err;
    }

    return ESP_OK;
}

void app_main(void) {
    esp_err_t err;

    err = led_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing LEDs. Error: %s", esp_err_to_name(err));
        return;
    }

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

    err = init_gpio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing GPIO. Error: %s", esp_err_to_name(err));
        return;
    }

    err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing Wi-Fi. Error: %s", esp_err_to_name(err));
        return;
    }

    err = wifi_start_http_server();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting config server. Error: %s", esp_err_to_name(err));
        return;
    }

    if (wifi_is_configured()) {
        // If wifi is configured, it should have stored info to connect to an AP

        err = wifi_connect_to_configured_ap();
        if (err != ESP_OK) {
            led_fade_in(COLOR_RED);

            err = wifi_start_ap();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
                return;
            }
        } else {
            led_fade_in(COLOR_GREEN);
            led_fade_out();
        }

        // TODO:
        // wifi_start_pomo_server()
    } else {
        // If wifi is not configured

        led_fade_in(COLOR_ORANGE);
        
        // First start the config access point
        err = wifi_start_ap();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error starting Wi-Fi access point. Error: %s", esp_err_to_name(err));
            return;
        }
    }
}
