#include "led.h"
#include "types.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "task_watchdog.h"

void led_rgb_init(led_ctx_t *led_ctx)
{

    gpio_init(led_ctx->blue_pin);
    gpio_set_dir(led_ctx->blue_pin, GPIO_OUT);
    gpio_put(led_ctx->blue_pin, 1);

    gpio_init(led_ctx->red_pin);
    gpio_set_dir(led_ctx->red_pin, GPIO_OUT);
    gpio_put(led_ctx->red_pin, 0);

    gpio_init(led_ctx->green_pin);
    gpio_set_dir(led_ctx->green_pin, GPIO_OUT);
    gpio_put(led_ctx->green_pin, 0);
}

void led_rgb_update_status(led_ctx_t *led_ctx, system_status_t status)
{
    gpio_put(led_ctx->blue_pin, 0);
    gpio_put(led_ctx->red_pin, 0);
    gpio_put(led_ctx->green_pin, 0);

    switch (status)
    {
    case SYSTEM_STATUS_STARTING:
        gpio_put(led_ctx->blue_pin, 1);
        break;
    case SYSTEM_STATUS_WORKING:
        gpio_put(led_ctx->green_pin, 1);
        break;
    case SYSTEM_STATUS_ERROR:
        gpio_put(led_ctx->red_pin, 1);
        break;
    }
}

void led_rgb_status_task(void *pvParameters)
{   
    watchdog_register_task("LED");
    led_ctx_t *led_ctx = (led_ctx_t *)pvParameters;
    led_rgb_init(led_ctx);
    system_status_t status;
    while (1)
    {
        watchdog_task_alive("LED");
        if (xQueueReceive(led_ctx->status_queue, &status, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (status != led_ctx->current_status)
            {
                led_ctx->current_status = status;
                led_rgb_update_status(led_ctx, status);
                printf("LED status updated: %d\n", status);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}