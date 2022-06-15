#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <led_strip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "led_manager.h"

static const char *TAG = "LED Manager";
static QueueHandle_t led_queue;

static led_strip_t strip = {
    .type = LED_STRIP_WS2812,
    .length = NUM_LEDS,
    .gpio = LED_PIN,
    .buf = NULL,
    .brightness = 20,
};

static void led_task(void* args) {
    while(1) {
        led_item_t led_item;
        xQueueReceive(led_queue, &led_item, portMAX_DELAY);

        switch(led_item.type) {
            case LED_DISPLAY_SOLID:
                strip.brightness = led_item.brightness;
                led_strip_fill(&strip, 0, strip.length, led_item.color);
                led_strip_flush(&strip);
                break;
            case LED_DISPLAY_SPIN:
                break;
            case LED_DISPLAY_FADE_IN:
                for(int i = 0; i < LED_FADE_STEPS; i++) { // Fade in in 10 increments from 0 to set brightness
                    strip.brightness = i * (led_item.brightness / LED_FADE_STEPS);
                    ESP_LOGI(TAG, "Setting brightness to %i", strip.brightness);
                    led_strip_fill(&strip, 0, strip.length, led_item.color);
                    // led_strip_wait(&strip, portMAX_DELAY);
                    led_strip_flush(&strip);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "Final setting value to %i", led_item.brightness);
                strip.brightness = led_item.brightness;
                led_strip_fill(&strip, 0, strip.length, led_item.color);
                // led_strip_wait(&strip, portMAX_DELAY);
                led_strip_flush(&strip);
                break;
        }
    }
}

esp_err_t led_init() {
    ESP_LOGI(TAG, "Initializing LED Manager");
    led_strip_install();

    led_strip_init(&strip);

    led_queue = xQueueCreate(5, sizeof(led_item_t));

    xTaskCreate(led_task, "LED Manager", configMINIMAL_STACK_SIZE + 1024, NULL, tskIDLE_PRIORITY + 5, NULL);

    ESP_LOGI(TAG, "Finished initializing LED Manager");

    return ESP_OK;
}

esp_err_t led_set_color(rgb_t color) {
    led_item_t led_item = {
        .type = LED_DISPLAY_SOLID,
        .color = color,
        .brightness = LED_DEFAULT_BRIGHTNESS
    };
    
    xQueueSend(led_queue, &led_item, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t led_fade_in(rgb_t color) {
    led_item_t led_item = {
        .type = LED_DISPLAY_FADE_IN,
        .color = color,
        .brightness = LED_DEFAULT_BRIGHTNESS
    };

    xQueueSend(led_queue, &led_item, portMAX_DELAY);

    return ESP_OK;
}