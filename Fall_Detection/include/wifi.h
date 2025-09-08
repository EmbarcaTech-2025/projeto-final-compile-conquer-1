#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct {
    const char *ssid;
    const char *password;
    QueueHandle_t status_queue;
    SemaphoreHandle_t wifi_semaphore;
    SemaphoreHandle_t gps_semaphore;
} wifi_ctx_t;

int connect_to_wifi(const char *ssid, const char *password);
void wifi_init_task(void *pvParameters);
