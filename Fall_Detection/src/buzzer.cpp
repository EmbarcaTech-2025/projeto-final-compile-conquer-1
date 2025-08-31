
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buzzer.h"
#include "types.h"

void buzzer_init(buzzer_ctx_t *buzzer_ctx)
{
    gpio_set_function(buzzer_ctx->buzzer_pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(buzzer_ctx->buzzer_pin);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 500);
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(buzzer_ctx->buzzer_pin, 0);
}

void buzzer_beep(buzzer_ctx_t *buzzer_ctx, int duration_ms)
{
    pwm_set_gpio_level(buzzer_ctx->buzzer_pin, 250);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    pwm_set_gpio_level(buzzer_ctx->buzzer_pin, 0);
}

void buzzer_pattern_fall_detected(buzzer_ctx_t *buzzer_ctx)
{
    for (int i = 0; i < 3; i++)
    {
        buzzer_beep(buzzer_ctx, 200);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void buzzer_pattern_emergency_button(buzzer_ctx_t *buzzer_ctx)
{
    buzzer_beep(buzzer_ctx, 150);
}

void buzzer_task(void *pvParameters)
{
    buzzer_ctx_t *buzzer_ctx = (buzzer_ctx_t *)pvParameters;

    printf("Buzzer task waiting for WiFi...\n");
    xSemaphoreTake(buzzer_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(buzzer_ctx->wifi_semaphore);

    buzzer_init(buzzer_ctx);

    event_type_t event;
    while (1)
    {
        if (xQueueReceive(buzzer_ctx->buzzer_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event == EVENT_FALL_DETECTED)
            {
                buzzer_pattern_fall_detected(buzzer_ctx);
            }
            else if (event == EVENT_EMERGENCY_BUTTON_PRESSED)
            {
                buzzer_pattern_emergency_button(buzzer_ctx);
            }
        }
    }
}