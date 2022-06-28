#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <FreeRTOSConfig.h>
#include <freertos/task.h>

#include "task_manager.h"

static const char* TAG = "Task Manager";

static task_handle_t tasks[MAX_TASKS];

esp_err_t task_init(void) {
    for(int i=0; i<MAX_TASKS; i++) {
        tasks[i].task_handle = NULL;
    }

    return ESP_OK;
}

void sample_task(void* arg) {
    int task_pos = (int)arg;

    int i = 0;
    while(1) {
        ESP_LOGI(TAG, "Running ID #%i", task_pos);
        vTaskDelay(pdMS_TO_TICKS(1000));
        i++;
        if (i > 10) {
            tasks[task_pos].task_handle = NULL;
            vTaskDelete(NULL);
        }
    }
}

esp_err_t task_add(task_type_t type) {
    int i;
    for(i=0; i<MAX_TASKS; i++) {
        if (tasks[i].task_handle == NULL) {
            // tasks[i] is a free task
            xTaskCreate(sample_task, NULL, configMINIMAL_STACK_SIZE + 1024, (void*)i, tskIDLE_PRIORITY + 5, &(tasks[i].task_handle));
            break;
        }
    }
    
    if (i == MAX_TASKS) {
        // No task positions are open
        return ESP_FAIL;
    }

    return ESP_OK;
}
