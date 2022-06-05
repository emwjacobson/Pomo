#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>
#include <sdkconfig.h>

void app_main(void) {
    int i = 0;
    while (1) {
        printf("Hey there! i = %i\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        i++;
    }
}
