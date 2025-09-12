#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct
{
    QueueHandle_t sensor_queue;
    SemaphoreHandle_t wifi_semaphore;
    QueueHandle_t status_queue;
} sensor_ctx_t;

void read_accel_gyro_task(void *pvParameters);
