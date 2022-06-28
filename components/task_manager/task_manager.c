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

static task_handle_t* tasks[MAX_TASKS];

esp_err_t task_init(void) {
    for(int i=0; i<MAX_TASKS; i++) {
        tasks[i] = NULL;
    }

    return ESP_OK;
}

esp_err_t task_add(task_type_t type) {
    int i;
    for(i=0; i<MAX_TASKS; i++) {
        if (tasks[i] == NULL || tasks[i]->is_dead) {
            // tasks[i] is a free task
        }
    }
    
    if (i == MAX_TASKS) {
        // If here, then no tasks are free!
    }
}
