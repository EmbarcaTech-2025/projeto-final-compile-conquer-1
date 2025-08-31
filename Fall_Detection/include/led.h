#pragma _once

#include "types.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    uint8_t red_pin;
    uint8_t green_pin;
    uint8_t blue_pin;
    QueueHandle_t status_queue;
    system_status_t current_status;
} led_ctx_t;

void led_rgb_init(led_ctx_t *led_ctx);
void led_rgb_update_status(led_ctx_t *led_ctx, system_status_t status);
void led_rgb_status_task(void *pvParameters);