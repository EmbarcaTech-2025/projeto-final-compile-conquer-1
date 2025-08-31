#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct{
    uint8_t button_pin;
    SemaphoreHandle_t button_semaphore;
    QueueHandle_t event_queue;
    SemaphoreHandle_t wifi_semaphore;
    QueueHandle_t buzzer_queue;
    uint32_t last_press_time;

} emergency_button_ctx_t;

void emergency_button_callback(uint gpio, uint32_t events);
void emergency_button_task(void *pvParameters);