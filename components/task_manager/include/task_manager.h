#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <freertos/task.h>

#define MAX_TASKS 32

typedef enum {
    TASK_TYPE_REPEATING,
    TASK_TYPE_ONE_TIME,
    TASK_TYPE_POMODORO
} task_type_t;

typedef struct {
    task_type_t type;
    TaskHandle_t task_handle;
    bool is_enabled;
    bool is_dead;
} task_handle_t;

esp_err_t task_init(void);

#endif