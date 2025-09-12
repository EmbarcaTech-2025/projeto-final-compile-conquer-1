#pragma once
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    QueueHandle_t status_queue;
} watchdog_ctx_t;

void watchdog_init();
void watchdog_register_task(const char* task_name);
void watchdog_task_alive(const char* task_name);
void watchdog_monitor_task(void *pvParameters);