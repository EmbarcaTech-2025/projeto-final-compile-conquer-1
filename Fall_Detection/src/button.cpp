#include <stdio.h>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "button.h"
#include "types.h"

static emergency_button_ctx_t *global_btn_ctx = NULL;

void emergency_button_callback(uint gpio, uint32_t events)
{
    if (global_btn_ctx == NULL) return;

    if (gpio == global_btn_ctx->button_pin)
    {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        if (current_time - global_btn_ctx->last_press_time < 200)
        {
            return;
        }
        global_btn_ctx->last_press_time = current_time;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(global_btn_ctx->button_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void emergency_button_task(void *pvParameters)
{
    emergency_button_ctx_t *btn_ctx = (emergency_button_ctx_t *)pvParameters;

    printf("Emergency button task waiting for WiFi...\n");
    xSemaphoreTake(btn_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(btn_ctx->wifi_semaphore);

    gpio_init(btn_ctx->button_pin);
    gpio_set_dir(btn_ctx->button_pin, GPIO_IN);
    gpio_pull_up(btn_ctx->button_pin);

    global_btn_ctx = btn_ctx;
    gpio_set_irq_enabled_with_callback(btn_ctx->button_pin, GPIO_IRQ_EDGE_FALL, true, emergency_button_callback);

    while (1)
    {
        if (xSemaphoreTake(btn_ctx->button_semaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("Emergency button pressed!\n");
            event_type_t emergency_event = EVENT_EMERGENCY_BUTTON_PRESSED;
            xQueueSend(btn_ctx->event_queue, &emergency_event, portMAX_DELAY);
            xQueueSend(btn_ctx->buzzer_queue, &emergency_event, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}