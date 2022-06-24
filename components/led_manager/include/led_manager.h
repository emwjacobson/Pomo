#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <led_strip.h>

#define LED_PIN GPIO_NUM_12
#define NUM_LEDS 16
// Ideally the default brightness should be a multiple
// of the fade steps to help create a smooth fade effect
#define LED_DEFAULT_BRIGHTNESS 32
#define LED_BRIGHTNESS_STEP 2
// #define LED_FADE_STEPS 20

static const rgb_t COLOR_ORANGE = {
    .r = 255,
    .g = 60,
    .b = 0,
};

static const rgb_t COLOR_RED = {
    .r = 255,
    .g = 0,
    .b = 0
};

static const rgb_t COLOR_GREEN = {
    .r = 0,
    .g = 255,
    .b = 0
};

static const rgb_t COLOR_OFF = {
    .r = 0,
    .g = 0,
    .b = 0
};

typedef enum {
    LED_DISPLAY_SOLID,
    LED_DISPLAY_SPIN,
    LED_DISPLAY_FADE_IN,
    LED_DISPLAY_FADE_OUT,
} led_display_type_t;

typedef struct {
    rgb_t color;
    led_display_type_t type;
    uint8_t brightness;
} led_item_t;

esp_err_t led_init(void);
esp_err_t led_set_color(rgb_t color);
esp_err_t led_set_off();
esp_err_t led_fade_in(rgb_t color);
esp_err_t led_fade_in_ISR(rgb_t color);
esp_err_t led_fade_out(void);
esp_err_t led_fade_out_ISR(void);

#endif