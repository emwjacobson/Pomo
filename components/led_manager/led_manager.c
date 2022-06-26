#include <stdio.h>
#include <string.h>
#include <math.h>
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

static rgb_t last_color;

static led_strip_t strip = {
    .type = LED_STRIP_WS2812,
    .length = NUM_LEDS,
    .gpio = LED_PIN,
    .buf = NULL,
    .brightness = 20,
};

union display_data_t {
    int brightness;
};

static void led_task(void* args) {
    led_item_t led_item;
    int t = 0;
    bool display_done = false;

    // Wait forever for first time to make sure we have something to display
    xQueueReceive(led_queue, &led_item, portMAX_DELAY);

    while (1) {
        // Transitions
        // Only do transitions if a new item has been received
        if (display_done && xQueueReceive(led_queue, &led_item, 0) == pdPASS) {
            // If here, then there is a new lighting effect that should take place
            ESP_LOGI(TAG, "Got new item from queue.");

            switch (led_item.type) {
                case LED_DISPLAY_FADE_IN:
                    strip.brightness = 0;
                    led_strip_fill(&strip, 0, strip.length, led_item.color);
                    led_strip_flush(&strip);
                    break;
                default:
                    break;
            }

            t = 0;
            // if (led_item.type != LED_DISPLAY_FADE_OUT)
            //     last_color = led_item.color;

            if (led_item.type == LED_DISPLAY_FADE_OUT) {
                led_item.color = last_color;
            } else {
                last_color = led_item.color;
            }
            display_done = false;
        }

        // Actions
        switch (led_item.type) {
            case LED_DISPLAY_SOLID:
                strip.brightness = led_item.brightness;
                led_strip_fill(&strip, 0, strip.length, led_item.color);
                led_strip_flush(&strip);
                display_done = true;
                break;
            case LED_DISPLAY_SPIN:
                display_done = true;
                break;
            case LED_DISPLAY_PULSE:
                // brightness = (led_item.brightness / 4) * sin(t / 2pi) + led_item.brightness
                strip.brightness = (int)((led_item.brightness / 2) * sinf(((float)t) / (2 * 3.14159)) + led_item.brightness);
                ESP_LOGI(TAG, "Pulsing LED brightness to %i", strip.brightness);
                led_strip_fill(&strip, 0, strip.length, led_item.color);
                led_strip_flush(&strip);

                // Set it as done after X * LED_TICKS_PER_SECOND seconds
                display_done = (t >= 2 * LED_TICKS_PER_SECOND);
                break;
            case LED_DISPLAY_FADE_IN:
                if (strip.brightness != led_item.brightness) {
                    if (strip.brightness > led_item.brightness) {
                        ESP_LOGI(TAG, "Fade in complete");
                        strip.brightness = led_item.brightness;
                    } else if (strip.brightness < led_item.brightness) {
                        strip.brightness += LED_BRIGHTNESS_STEP;
                    }
                    ESP_LOGI(TAG, "Fading in brightness to %i", strip.brightness);
                    led_strip_fill(&strip, 0, strip.length, led_item.color);
                    led_strip_flush(&strip);
                } else {
                    display_done = true;
                }
                break;
            case LED_DISPLAY_FADE_OUT:
                if (strip.brightness != led_item.brightness) {
                    if (strip.brightness < led_item.brightness) {
                        ESP_LOGI(TAG, "Fade out complete");
                        strip.brightness = led_item.brightness;
                    } else if (strip.brightness > led_item.brightness) {
                        strip.brightness -= LED_BRIGHTNESS_STEP;
                    }
                    ESP_LOGI(TAG, "Fading out brightness to %i", strip.brightness);
                    led_strip_fill(&strip, 0, strip.length, last_color);
                    led_strip_flush(&strip);
                } else {
                    display_done = true;
                }
                break;
        }

        t++;
        // Run at roughly 10Hz
        vTaskDelay(pdMS_TO_TICKS(LED_DELAY));
    }
}

esp_err_t led_init() {
    ESP_LOGI(TAG, "Initializing LED Manager");
    led_strip_install();

    led_strip_init(&strip);

    led_queue = xQueueCreate(5, sizeof(led_item_t));

    xTaskCreate(led_task, "LED Manager", configMINIMAL_STACK_SIZE + 2048, NULL, tskIDLE_PRIORITY + 5, NULL);

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

esp_err_t led_set_pulse(rgb_t color) {
    led_item_t led_item = {
        .type = LED_DISPLAY_PULSE,
        .color = color,
        .brightness = LED_DEFAULT_BRIGHTNESS
    };

    xQueueSend(led_queue, &led_item, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t led_set_off() {
    led_item_t led_item = {
        .type = LED_DISPLAY_SOLID,
        .color = COLOR_OFF,
        .brightness = 0
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

esp_err_t led_fade_in_ISR(rgb_t color) {
    led_item_t led_item = {
        .type = LED_DISPLAY_FADE_IN,
        .color = color,
        .brightness = LED_DEFAULT_BRIGHTNESS
    };

    xQueueSendFromISR(led_queue, &led_item, NULL);

    return ESP_OK;
}

esp_err_t led_fade_out(void) {
    led_item_t led_item = {
        .type = LED_DISPLAY_FADE_OUT,
        .color = { { 0 } },
        .brightness = 0
    };

    xQueueSend(led_queue, &led_item, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t led_fade_out_ISR(void) {
    led_item_t led_item = {
        .type = LED_DISPLAY_FADE_OUT,
        .color = { { 0 } },
        .brightness = 0
    };

    xQueueSendFromISR(led_queue, &led_item, NULL);

    return ESP_OK;
}
