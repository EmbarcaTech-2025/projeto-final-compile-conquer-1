#include <stdio.h>
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "atgm336h_uart.h"
#include "gps.h"
#include "types.h"
#include "hardware/watchdog.h"
#include "task_watchdog.h"

void gps_get_maps_link(char *buffer, size_t buffer_size, double lat, double lon)
{
    /*
    float fake_lat = -23.556664;
    float fake_lon = -46.653497;

    snprintf(buffer, buffer_size, "https://maps.google.com/?q=%.6f,%.6f", fake_lat, fake_lon);
    */

    if (lat == 0.0f || lon == 0.0f)
    {
        snprintf(buffer, buffer_size, "Unavailable");
        return;
    }

    snprintf(buffer, buffer_size, "https://maps.google.com/?q=%.6f,%.6f", lat, lon);
}

bool parse_nmea_sentence(const char *sentence, gps_data_t *data)
{
    switch (minmea_sentence_id(sentence, false))
    {
    case MINMEA_SENTENCE_GGA:
    {
        struct minmea_sentence_gga gga = {0};
        if (minmea_parse_gga(&gga, sentence))
        {
            if (gga.fix_quality == 0 || gga.latitude.scale == 0 || gga.longitude.scale == 0)
            {
                data->fix = false;
                return false;
            }

            float altitude = minmea_tofloat(&gga.altitude);
            float latitude = minmea_tocoord(&gga.latitude);
            float longitude = minmea_tocoord(&gga.longitude);

            if (isinf(latitude) || isnan(latitude) || isinf(longitude) || isnan(longitude) ||
                latitude < -90.0f || latitude > 90.0f || longitude < -180.0f || longitude > 180.0f)
            {
                data->fix = false;
                return false;
            }

            data->lat = latitude;
            data->lon = longitude;
            data->alt = altitude;
            data->sat = gga.satellites_tracked;
            data->fix = gga.fix_quality > 0;
            return true;
        }
        break;
    }
    default:
        return false;
    }
    return false;
}

void gps_init(gps_init_ctx_t *gps_init)
{
    system_status_t error_status = SYSTEM_STATUS_ERROR;

    if (atgm336h_uart_init() != true)
    {
        xQueueSend(gps_init->status_queue, &error_status, 0);
        printf("CRITICAL: ATGM336H UART init failed. Rebooting...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        watchdog_reboot(0, 0, 0);
    }

    xSemaphoreGive(gps_init->gps_semaphore);
}

void gps_location_task(void *pvParameters)
{
    gps_ctx_t *gps_ctx = (gps_ctx_t *)pvParameters;

    printf("GPS location task waiting for WiFi...\n");
    xSemaphoreTake(gps_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(gps_ctx->wifi_semaphore);

    watchdog_register_task("GPSLoc");

    char nmea_sentence[128];
    event_type_t request;
    gps_data_t latest_gps_data = {0};

    while (1)
    {
        watchdog_task_alive("GPSLoc");
        if (xQueueReceive(gps_ctx->gps_req, &request, pdMS_TO_TICKS(10000)) == pdTRUE)
        {
            if (request == EVENT_FALL_DETECTED || request == EVENT_EMERGENCY_BUTTON_PRESSED)
            {
                int len = atgm336h_uart_read_line(nmea_sentence, sizeof(nmea_sentence));
                if (len > 0)
                {
                    if (parse_nmea_sentence(nmea_sentence, &latest_gps_data))
                    {
                        printf("GPS get location: Ok\n");
                        xQueueSend(gps_ctx->gps_queue, &latest_gps_data, 0);
                    }
                    else
                    {
                        printf("GPS get location: Failed. Sending empty gps data.\n");
                        xQueueSend(gps_ctx->gps_queue, &latest_gps_data, 0);
                    }
                }
            }
            latest_gps_data = {0};
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
