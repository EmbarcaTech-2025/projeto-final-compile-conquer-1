#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct {
    QueueHandle_t sensor_queue;
    QueueHandle_t event_queue;
    QueueHandle_t buzzer_queue;
    QueueHandle_t status_queue;
    SemaphoreHandle_t wifi_semaphore;
} fall_detection_ctx_t;

int get_signal_data(size_t offset, size_t length, float *out_ptr);
void fall_detection_task(void *pvParameters);