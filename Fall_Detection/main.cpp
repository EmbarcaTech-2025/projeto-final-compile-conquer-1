#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "mpu6050_i2c.h"
#include "hardware/watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <math.h>
#include "pico/cyw43_arch.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "mongoose.h"

// Override Edge Impulse memory allocation to use FreeRTOS heap
void *ei_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *ei_calloc(size_t nitems, size_t size)
{
    void *ptr = pvPortMalloc(nitems * size);
    if (ptr)
    {
        memset(ptr, 0, nitems * size);
    }
    return ptr;
}

void ei_free(void *ptr)
{
    if (ptr)
    {
        vPortFree(ptr);
    }
}

// Also override standard allocators for Mongoose
#ifdef __cplusplus
extern "C"
{
#endif

    void *malloc(size_t size)
    {
        return pvPortMalloc(size);
    }

    void *calloc(size_t nitems, size_t size)
    {
        void *ptr = pvPortMalloc(nitems * size);
        if (ptr)
        {
            memset(ptr, 0, nitems * size);
        }
        return ptr;
    }

    void free(void *ptr)
    {
        if (ptr)
        {
            vPortFree(ptr);
        }
    }

#ifdef __cplusplus
}
#endif

#define EMERGENCY_BUTTON_PIN 5
#define BUZZER_PIN 21
#define BLUE_LED_PIN 12
#define RED_LED_PIN 13
#define GREEN_LED_PIN 11

#define WIFI_SSID ""
#define WIFI_PASS ""
#define TELEGRAM_BOT_TOKEN ""
#define TELEGRAM_CHAT_ID ""

#define MSG_BUFFER_SIZE 512
const char *url = "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN "/sendMessage";
char msg[MSG_BUFFER_SIZE];
char user_name[50] = "Maria Silva";
struct mg_mgr mongoose_manager;

typedef enum
{
    EVENT_FALL_DETECTED,
    EVENT_DAILY_ACTIVITY,
    EVENT_EMERGENCY_BUTTON_PRESSED
} event_type_t;

typedef enum
{
    SYSTEM_STATUS_STARTING,
    SYSTEM_STATUS_WORKING,
    SYSTEM_STATUS_ERROR,
} system_status_t;

typedef struct
{
    int16_t accel[3];
    int16_t gyro[3];
} sensor_data_t;

QueueHandle_t sensor_queue;
QueueHandle_t event_queue;
QueueHandle_t buzzer_queue;
QueueHandle_t led_status_queue;
SemaphoreHandle_t wifi_connected_semaphore;
SemaphoreHandle_t emergency_button_pressed_semaphore;

volatile bool enabled_http_req = true;
volatile uint32_t last_button_press_time = 0;
volatile system_status_t current_system_status = SYSTEM_STATUS_STARTING;

static float *g_feature_ptr = NULL;

void led_init()
{

    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(BLUE_LED_PIN, 1);

    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);

    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_put(GREEN_LED_PIN, 0);
}

void update_led_status(system_status_t status)
{
    gpio_put(BLUE_LED_PIN, 0);
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);

    switch (status)
    {
    case SYSTEM_STATUS_STARTING:
        gpio_put(BLUE_LED_PIN, 1);
        break;
    case SYSTEM_STATUS_WORKING:
        gpio_put(GREEN_LED_PIN, 1);
        break;
    case SYSTEM_STATUS_ERROR:
        gpio_put(RED_LED_PIN, 1);
        break;
    }
}

void led_status_task(void *pvParameters)
{
    led_init();
    system_status_t status;
    while (1)
    {
        if (xQueueReceive(led_status_queue, &status, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (status != current_system_status)
            {
                current_system_status = status;
                update_led_status(status);
                printf("LED status updated: %d\n", status);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int get_signal_data(size_t offset, size_t length, float *out_ptr)
{
    // This function is called by Edge Impulse to get chunks of your sensor data
    // Parameters:
    //   offset: starting position in the features array
    //   length: how many values to copy
    //   out_ptr: where to write the data (Edge Impulse expects float*)

    for (size_t i = 0; i < length; i++)
    {
        out_ptr[i] = g_feature_ptr[offset + i];
    }
    return 0;
}

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
    printf("WiFi initialization task started\n");
    printf("WiFi SSID: %s\n", WIFI_SSID);

    printf("Attempting WiFi connection...\n");
    if (connect_to_wifi(WIFI_SSID, WIFI_PASS) != 0)
    {
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(led_status_queue, &error_status, 0);
        printf("CRITICAL: WiFi failed. System will restart.\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        watchdog_reboot(0, 0, 0);
    }

    xSemaphoreGive(wifi_connected_semaphore);

    system_status_t working_status = SYSTEM_STATUS_WORKING;
    xQueueSend(led_status_queue, &working_status, 0);

    printf("WiFi initialization complete. Deleting WiFi init task.\n");
    vTaskDelete(NULL);
}

void read_accel_gyro_task(void *pvParameters)
{
    printf("Accelerometer task waiting for WiFi...\n");
    xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_connected_semaphore);

    printf("Starting accelerometer readings...\n");

    int mpu_result = mpu6050_setup_i2c();

    if (mpu_result != 0)
    {
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(led_status_queue, &error_status, 0);
        printf("CRITICAL: MPU6050 setup failed.\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        watchdog_reboot(0, 0, 0);
    }

    mpu6050_reset();
    printf("MPU6050 initialized successfully.\n");

    mpu6050_set_accel_range(3);
    mpu6050_set_gyro_range(3);
    printf("Accelerometer and Gyroscope ranges set.\n");

    sensor_data_t sensor_data;
    int16_t temp;

    printf("Starting sensor reading task...\n");
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true)
    {
        mpu6050_read_raw(sensor_data.accel, sensor_data.gyro, &temp);

        xQueueSend(sensor_queue, &sensor_data, portMAX_DELAY);

        /*printf("Accel: X=%d Y=%d Z=%d | Gyro: X=%d Y=%d Z=%d | Temp: %d\n",
                   sensor_data.accel[0], sensor_data.accel[1], sensor_data.accel[2],
                   sensor_data.gyro[0], sensor_data.gyro[1], sensor_data.gyro[2],
                   temp);*/

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(20));
    }
}

void fall_detection_task(void *pvParameters)
{
    printf("Fall detection task waiting for WiFi...\n");
    xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_connected_semaphore);

    printf("Starting fall detection...\n");
    sensor_data_t sensor_data;

    static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
    static size_t feature_index = 0;

    g_feature_ptr = features;

    while (true)
    {
        if (xQueueReceive(sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE)
        {
            int16_t ax = sensor_data.accel[0];
            int16_t ay = sensor_data.accel[1];
            int16_t az = sensor_data.accel[2];
            int16_t gx = sensor_data.gyro[0];
            int16_t gy = sensor_data.gyro[1];
            int16_t gz = sensor_data.gyro[2];

            // Acumulate data for machine learning inference
            if (feature_index < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
            {
                features[feature_index++] = (float)(ax);
                features[feature_index++] = (float)(ay);
                features[feature_index++] = (float)(az);
                features[feature_index++] = (float)(gx);
                features[feature_index++] = (float)(gy);
                features[feature_index++] = (float)(gz);
            }

            // Just run inference when we have enough data
            if (feature_index >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
            {

                signal_t signal;
                signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
                signal.get_data = &get_signal_data;

                // Inferencia
                ei_impulse_result_t result = {0};
                EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

                if (res == EI_IMPULSE_OK)
                {
                    printf("---------------\n");
                    event_type_t event = EVENT_DAILY_ACTIVITY;
                    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
                    {
                        printf("%s: %.5f\n",
                               result.classification[i].label,
                               result.classification[i].value);

                        if (result.classification[i].value > 0.9f &&
                            strcmp(result.classification[i].label, "Fall") == 0)
                        {
                            event = EVENT_FALL_DETECTED;
                            xQueueSend(event_queue, &event, portMAX_DELAY);
                            xQueueSend(buzzer_queue, &event, portMAX_DELAY);
                            printf("⚠️ ML DETECTED FALL! ⚠️\n");
                        }
                    }
                    if (event != EVENT_FALL_DETECTED)
                    {
                        printf("No fall detected\n");
                    }
                }
                else
                {
                    system_status_t error_status = SYSTEM_STATUS_ERROR;
                    xQueueSend(led_status_queue, &error_status, 0);
                    printf("Error running classifier: %d\n", res);
                }
                printf("Inference done\n");
                printf("---------------\n");
                feature_index = 0;
                memset(features, 0, sizeof(features));
            }
        }
    }
}

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_OPEN:
    {
        printf("HTTP Connection opened\n");
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
        // printf("Connected to server, initializing TLS\n");
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
        // printf("Response: %.*s\n", (int)hm->message.len, hm->message.buf);

        bool success = (strstr(hm->body.buf, "\"ok\":true") != NULL);
        printf("Telegram response: %s\n", success ? "OK" : "ERROR");

        if (success)
        {
            system_status_t working_status = SYSTEM_STATUS_WORKING;
            xQueueSend(led_status_queue, &working_status, 0);
        }
        else
        {
            system_status_t error_status = SYSTEM_STATUS_ERROR;
            xQueueSend(led_status_queue, &error_status, 0);
        }

        c->is_draining = 1;
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }

    case MG_EV_ERROR:
    {
        char *err = (char *)ev_data;
        printf("Error: %s\n", err ? err : "unknown");
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(led_status_queue, &error_status, 0);
        c->is_closing = 1;
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }

    case MG_EV_CLOSE:
        printf("HTTP Connection closed\n");
        if (c->fn_data)
            *((volatile bool *)c->fn_data) = true;
        break;
    }
}

void network_task(void *pvParameters)
{
    printf("Mongoose task waiting for WiFi...\n");
    xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_connected_semaphore);

    printf("Starting Mongoose HTTP client...\n");
    mg_mgr_init(&mongoose_manager);
    mg_log_set(MG_LL_INFO);

    MG_INFO(("Initialising Mongoose..."));
    event_type_t event;
    while (1)
    {
        mg_mgr_poll(&mongoose_manager, 10);
        if (enabled_http_req && xQueueReceive(event_queue, &event, 0) == pdTRUE)
        {
            enabled_http_req = false;
            // printf("Notification received: %lu\n", event);
            if (event == EVENT_FALL_DETECTED)
            {
                snprintf(msg, sizeof(msg), "🚨 Fall Alert: %s has fallen and might need assistance.", user_name);
            }
            else if (event == EVENT_EMERGENCY_BUTTON_PRESSED)
            {
                snprintf(msg, sizeof(msg), "🖲️ Emergency Button: %s has pressed the emergency button and might need assistance.", user_name);
            }

            mg_http_connect(&mongoose_manager, url, http_ev_handler, (void *)&enabled_http_req);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void emergency_button_callback(uint gpio, uint32_t events)
{
    if (gpio == EMERGENCY_BUTTON_PIN)
    {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        if (current_time - last_button_press_time < 200)
        {
            return;
        }
        last_button_press_time = current_time;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(emergency_button_pressed_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void emergency_button_task(void *pvParameters)
{

    printf("Emergency button task waiting for WiFi...\n");
    xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_connected_semaphore);

    gpio_init(EMERGENCY_BUTTON_PIN);
    gpio_set_dir(EMERGENCY_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(EMERGENCY_BUTTON_PIN);

    gpio_set_irq_enabled_with_callback(EMERGENCY_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, emergency_button_callback);

    while (1)
    {
        if (xSemaphoreTake(emergency_button_pressed_semaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("Emergency button pressed!\n");
            event_type_t emergency_event = EVENT_EMERGENCY_BUTTON_PRESSED;
            xQueueSend(event_queue, &emergency_event, portMAX_DELAY);
            xQueueSend(buzzer_queue, &emergency_event, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void buzzer_init()
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 500);
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void buzzer_beep(int duration_ms)
{
    pwm_set_gpio_level(BUZZER_PIN, 250);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void buzzer_pattern_fall_detected()
{
    for (int i = 0; i < 3; i++)
    {
        buzzer_beep(200);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void buzzer_pattern_emergency_button()
{
    buzzer_beep(150);
}

void buzzer_task(void *pvParameters)
{
    printf("Buzzer task waiting for WiFi...\n");
    xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY);
    xSemaphoreGive(wifi_connected_semaphore);

    buzzer_init();

    event_type_t event;
    while (1)
    {
        if (xQueueReceive(buzzer_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event == EVENT_FALL_DETECTED)
            {
                buzzer_pattern_fall_detected();
            }
            else if (event == EVENT_EMERGENCY_BUTTON_PRESSED)
            {
                buzzer_pattern_emergency_button();
            }
        }
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting project...\n");

    wifi_connected_semaphore = xSemaphoreCreateBinary();
    emergency_button_pressed_semaphore = xSemaphoreCreateBinary();

    sensor_queue = xQueueCreate(500, sizeof(sensor_data_t));
    event_queue = xQueueCreate(16, sizeof(event_type_t));
    buzzer_queue = xQueueCreate(16, sizeof(event_type_t));
    led_status_queue = xQueueCreate(8, sizeof(system_status_t));

    xTaskCreate(led_status_task, "LEDStatus", 512, NULL, configMAX_PRIORITIES - 1, NULL);
    sleep_ms(100);

    system_status_t init_status = SYSTEM_STATUS_STARTING;
    xQueueSend(led_status_queue, &init_status, 0);

    xTaskCreate(wifi_init_task, "WiFiInit", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(read_accel_gyro_task, "ReadAccelGyro", 2048, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(emergency_button_task, "EmergencyButton", 512, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(fall_detection_task, "FallDetection", 8192, NULL, configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(buzzer_task, "Buzzer", 512, NULL, configMAX_PRIORITIES - 4, NULL);
    xTaskCreate(network_task, "Network", 2048, NULL, configMAX_PRIORITIES - 5, NULL);

    vTaskStartScheduler();
    while (1)
    {
        tight_loop_contents();
    }
}