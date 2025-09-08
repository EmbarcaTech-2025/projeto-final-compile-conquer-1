#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "wifi.h"
#include "types.h"


int connect_to_wifi(const char *ssid, const char *password)
{
    printf("Connecting to Wifi...\n");

    int init_result = cyw43_arch_init();
    if (init_result != 0)
    {
        printf("Error starting Wifi: cyw43_arch_init failed with code %d\n", init_result);
        return -1;
    }
    printf("CYW43 architecture initialized successfully.\n");

    cyw43_arch_enable_sta_mode();
    printf("STA mode enabled.\n");

    printf("Attempting to connect to SSID: %s\n", ssid);

    int attempt = 0;
    while (attempt <= 10)
    {
        printf("WiFi connection attempt %d...\n", attempt + 1);

        int connect_result_code = cyw43_arch_wifi_connect_timeout_ms(
            ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000);

        if (connect_result_code != 0)
        {
            printf("Error connecting to Wifi\n");
            printf("Attempt %d: code %d\n", attempt, connect_result_code);
        }
        else
        {
            printf("Connected to Wifi successfully!\n");
            return 0;
        }

        if (attempt < 10)
        {
            printf("Waiting 15 seconds before next attempt...\n");
            sleep_ms(15000);
        }
        attempt++;
    }

    printf("Max wifi connect attempts reached\n");
    return -1;
}

void wifi_init_task(void *pvParameters)
{   
    wifi_ctx_t *wifi_ctx = (wifi_ctx_t *)pvParameters;

    printf("WiFi init task waiting for GPS fix...\n");
    xSemaphoreTake(wifi_ctx->gps_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_ctx->gps_semaphore);

    printf("WiFi initialization task started\n");
    printf("WiFi SSID: %s\n", wifi_ctx->ssid);

    if (connect_to_wifi(wifi_ctx->ssid, wifi_ctx->password) != 0)
    {
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(wifi_ctx->status_queue, &error_status, 0);
        printf("CRITICAL: WiFi failed. System will restart.\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        watchdog_reboot(0, 0, 0);
    }

    printf("WiFi initialization complete. Deleting WiFi init task.\n");
    xSemaphoreGive(wifi_ctx->wifi_semaphore);
    system_status_t working_status = SYSTEM_STATUS_WORKING;
    xQueueSend(wifi_ctx->status_queue, &working_status, 0);
    vTaskDelete(NULL);
}  