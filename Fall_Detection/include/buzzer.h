#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct  {
    uint8_t buzzer_pin;
    QueueHandle_t buzzer_queue;
    SemaphoreHandle_t wifi_semaphore;
} buzzer_ctx_t;

void buzzer_init(buzzer_ctx_t *buzzer_ctx);
void buzzer_beep(buzzer_ctx_t *buzzer_ctx, int duration_ms);
void buzzer_pattern_fall_detected(buzzer_ctx_t *buzzer_ctx);
void buzzer_pattern_emergency_button(buzzer_ctx_t *buzzer_ctx);
void buzzer_task(void *pvParameters);