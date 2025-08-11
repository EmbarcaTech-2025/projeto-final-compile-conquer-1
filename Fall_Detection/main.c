#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <math.h>
#include "pico/cyw43_arch.h"
#include "mongoose.h"

#define WIFI_SSID ""
#define WIFI_PASS ""
#define TELEGRAM_BOT_TOKEN ""
#define TELEGRAM_CHAT_ID ""

const char *url = "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN "/sendMessage";
char msg[200];

struct mg_mgr mongoose_manager;

typedef enum
{
    EVENT_FALL_DETECTED,
    EVENT_IMPACT_DETECTED
} event_type_t;

typedef struct
{
    int16_t accel[3];
    int16_t gyro[3];
} sensor_data_t;

QueueHandle_t sensor_queue;
QueueHandle_t event_queue;

volatile bool enabled_http_req = true;

void read_accel_gyro_task(void *pvParameters)
{
    mpu6050_setup_i2c();
    mpu6050_reset();

    mpu6050_set_accel_range(3);
    mpu6050_set_gyro_range(3);

    sensor_data_t sensor_data;
    int16_t temp;

    TickType_t last_wake_time = xTaskGetTickCount();

    while (true)
    {
        mpu6050_read_raw(sensor_data.accel, sensor_data.gyro, &temp);

        xQueueSend(sensor_queue, &sensor_data, portMAX_DELAY);

        /*printf("Accel: X=%d Y=%d Z=%d | Gyro: X=%d Y=%d Z=%d | Temp: %d | ",
                   sensor_data.accel[0], sensor_data.accel[1], sensor_data.accel[2],
                   sensor_data.gyro[0], sensor_data.gyro[1], sensor_data.gyro[2],
                   temp);*/

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5));
    }
}

void fall_impact_detection_task(void *pvParameters)
{
    sensor_data_t sensor_data;

    const float ACCEL_SCALE = 2048.0f;
    const float FREE_FALL_THRESHOLD = 0.3f;
    const float IMPACT_THRESHOLD = 6.0f;
    bool in_free_fall = false;

    while (true)
    {
        if (xQueueReceive(sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE)
        {
            float ax = sensor_data.accel[0] / ACCEL_SCALE;
            float ay = sensor_data.accel[1] / ACCEL_SCALE;
            float az = sensor_data.accel[2] / ACCEL_SCALE;
            float a_mag = sqrtf(ax * ax + ay * ay + az * az);

            // printf("Accel Mag: %.2f g\n", a_mag);

            if (!in_free_fall && a_mag < FREE_FALL_THRESHOLD)
            {

                in_free_fall = true;
                event_type_t event = EVENT_FALL_DETECTED;
                xQueueSend(event_queue, &event, portMAX_DELAY);
                printf("Free fall detected!\n");
            }
            else if (in_free_fall && a_mag > IMPACT_THRESHOLD)
            {
                in_free_fall = false;
                event_type_t event = EVENT_IMPACT_DETECTED;
                xQueueSend(event_queue, &event, portMAX_DELAY);
                printf("Impact detected!\n");
            }
            else if (in_free_fall && a_mag > FREE_FALL_THRESHOLD)
            {
                in_free_fall = false;
            }
        }
    }
}



int connect_to_wifi(const char *ssid, const char *password)
{
    printf("Connecting to Wifi...\n");
    if (cyw43_arch_init())
    {
        printf("Error starting Wifi: cyw43_arch_init failed\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_blocking(ssid, password, CYW43_AUTH_WPA2_AES_PSK))
    {
        printf("Error connecting to Wifi: check SSID/password\n");
        return -1;
    }
    else
    {
        printf("Connected to Wifi\n");
        return 0;
    }
}

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_OPEN:
    {
        printf("Connection opened\n");
        *(uint64_t *)c->data = mg_millis() + 10000;
        break;
    }

    case MG_EV_POLL:
    {
        if (mg_millis() > *(uint64_t *)c->data &&
            (c->is_connecting || c->is_resolving))
        {
            mg_error(c, "Connect timeout");
        }
        break;
    }

    case MG_EV_CONNECT:
    {
        printf("Connected to server, initializing TLS\n");
        struct mg_str host = mg_url_host(url);
        struct mg_tls_opts opts = {
            .name = host};

        mg_tls_init(c, &opts);
        break;
    }

    case MG_EV_TLS_HS:
    {

        char *json_buffer = mg_mprintf(
            "{%m:%m,%m:%m,%m:%s}",
            MG_ESC("chat_id"), MG_ESC(TELEGRAM_CHAT_ID),
            MG_ESC("text"), MG_ESC(msg),
            MG_ESC("disable_notification"), "false");
        if (json_buffer == NULL)
        {
            printf("Failed to allocate JSON buffer\n");
            mg_error(c, "Memory allocation failed");
            c->is_closing = 1;
            return;
        }
        int content_length = strlen(json_buffer);
        struct mg_str host = mg_url_host(url);

        // Send HTTP POST
        mg_printf(c,
                  "%s %s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %d\r\n"
                  "Connection: close\r\n"
                  "\r\n",
                  "POST", mg_url_uri(url),
                  (int)host.len, host.buf,
                  content_length);
        mg_send(c, json_buffer, content_length);
        free(json_buffer);
        printf("Request sent: %s\n", msg);
        break;
    }

    case MG_EV_HTTP_MSG:
    {
        // Response received
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        printf("Response: %.*s\n", (int)hm->message.len, hm->message.buf);

        c->is_draining = 1;
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }

    case MG_EV_ERROR:
    {
        char *err = (char *)ev_data;
        printf("Error: %s\n", err ? err : "unknown");
        c->is_closing = 1;
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }

    case MG_EV_CLOSE:
        printf("Connection closed\n");
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }
}

void mongoose_task(void *pvParameters)
{
    mg_mgr_init(&mongoose_manager);
    mg_log_set(MG_LL_DEBUG);

    if (connect_to_wifi(WIFI_SSID, WIFI_PASS) != 0) {
        printf("WiFi failed. Halting Mongoose task.\n");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    MG_INFO(("Initialising Mongoose..."));
    event_type_t event;
    while (1)
    {
        mg_mgr_poll(&mongoose_manager, 10);
        if (enabled_http_req && xQueueReceive(event_queue, &event, 0) == pdTRUE)
        {
            enabled_http_req = false;
            printf("Notification received: %lu\n", event);
            if (event == EVENT_FALL_DETECTED)
            {
                snprintf(msg, sizeof(msg), "Fall");
            }
            else if (event == EVENT_IMPACT_DETECTED)
            {
                snprintf(msg, sizeof(msg), "Impact");
            }

            mg_http_connect(&mongoose_manager, url, http_ev_handler, (void *)&enabled_http_req);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting project...\n");
    sensor_queue = xQueueCreate(32, sizeof(sensor_data_t));
    event_queue = xQueueCreate(16, sizeof(event_type_t));

    xTaskCreate(read_accel_gyro_task, "ReadAccelGyro", 512, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(fall_impact_detection_task, "FallImpactDetection", 512, NULL, configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(mongoose_task, "Mongoose", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
    while (1)
    {
        tight_loop_contents();
    }
}