#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <math.h>

#include "types.h"
#include "led.h"
#include "buzzer.h"
#include "button.h"
#include "wifi.h"
#include "sensor.h"
#include "fall_detection.h"
#include "network.h"
#include "gps.h"
#include "atgm336h_uart.h"
#include "task_watchdog.h"
#include "config.h"
#include "config_web_server.h"

// -----NETWORK VARIABLES ------
#define TELEGRAM_BOT_TOKEN ""
#define TELEGRAM_CHAT_ID ""
#define MSG_BUFFER_SIZE 512
const char *url = "https://api.telegram.org/bot" TELEGRAM_BOT_TOKEN "/sendMessage";
char msg[MSG_BUFFER_SIZE];
char user_name[50] = "Maria Silva";
struct mg_mgr mongoose_manager;
volatile bool enabled_http_req = true;

// -----LED VARIABLES
#define BLUE_LED_PIN 12
#define RED_LED_PIN 13
#define GREEN_LED_PIN 11
QueueHandle_t led_status_queue;
volatile system_status_t current_system_status = SYSTEM_STATUS_STARTING;

// -----BUZZER--------
#define BUZZER_PIN 21
QueueHandle_t buzzer_queue;

//------BUTTON------
#define EMERGENCY_BUTTON_PIN 5
volatile uint32_t last_button_press_time = 0;
SemaphoreHandle_t emergency_button_pressed_semaphore;

//-----WIFI-----
SemaphoreHandle_t wifi_connected_semaphore;

//------SENSOR------
QueueHandle_t sensor_queue;

//------EVENT------
QueueHandle_t event_queue;

// ------GPS------
QueueHandle_t gps_queue;
QueueHandle_t gps_request_location_queue;
SemaphoreHandle_t gps_init_semaphore;

//------CONFIG WEB SERVER-----
#define CONFIG_BUTTON 6
config_t current_config = {0};
bool is_config_button_held()
{
    gpio_init(CONFIG_BUTTON);
    gpio_set_dir(CONFIG_BUTTON, GPIO_IN);
    gpio_pull_up(CONFIG_BUTTON);
    sleep_ms(100);
    return !gpio_get(CONFIG_BUTTON);
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("\nStarting project...\n");

    bool config_button = is_config_button_held();
    bool config_valid = config_load(&current_config);

    if (config_button || !config_valid)
    {
        printf("Entering configuration mode: ");
        if (config_button)
            printf("Config button held.\n");
        if (!config_valid)
            printf("No valid config found.\n");

        config_set_defaults(&current_config);

        led_status_queue = xQueueCreate(8, sizeof(system_status_t));

        led_ctx_t led_ctx_config {
            .red_pin = RED_LED_PIN,
            .green_pin = GREEN_LED_PIN,
            .blue_pin = BLUE_LED_PIN,
            .status_queue = led_status_queue,
            .current_status = SYSTEM_STATUS_CONFIG};
        xTaskCreate(led_rgb_status_task, "LEDStatus", 512, &led_ctx_config, configMAX_PRIORITIES - 1, NULL);

        xTaskCreate(config_web_server_task, "ConfigWeb", 4096, &current_config, configMAX_PRIORITIES - 2, NULL);

    vTaskStartScheduler();

        while (1)
        {
            tight_loop_contents();
        }
    }
    else
    {
        watchdog_init();

        printf("Starting normal operation...\n");
        printf("Username: %s \n", current_config.user_name);
        printf("Wifi SSID: %s \n", current_config.wifi_ssid);

        gps_init_semaphore = xSemaphoreCreateBinary();
        wifi_connected_semaphore = xSemaphoreCreateBinary();
        emergency_button_pressed_semaphore = xSemaphoreCreateBinary();

        sensor_queue = xQueueCreate(500, sizeof(sensor_data_t));
        event_queue = xQueueCreate(16, sizeof(event_type_t));
        buzzer_queue = xQueueCreate(16, sizeof(event_type_t));
        led_status_queue = xQueueCreate(8, sizeof(system_status_t));
        gps_queue = xQueueCreate(16, sizeof(gps_data_t));
        gps_request_location_queue = xQueueCreate(16, sizeof(event_type_t));

        system_status_t init_status = SYSTEM_STATUS_STARTING;
        xQueueSend(led_status_queue, &init_status, 0);

        watchdog_ctx_t watchdog_ctx = {
            .status_queue = led_status_queue};
        xTaskCreate(watchdog_monitor_task, "WatchdogMonitor", 1024, &watchdog_ctx, configMAX_PRIORITIES - 1, NULL);

        led_ctx_t led_ctx = {
            .red_pin = RED_LED_PIN,
            .green_pin = GREEN_LED_PIN,
            .blue_pin = BLUE_LED_PIN,
            .status_queue = led_status_queue,
            .current_status = SYSTEM_STATUS_STARTING};
        xTaskCreate(led_rgb_status_task, "LEDStatus", 512, &led_ctx, configMAX_PRIORITIES - 1, NULL);
        // sleep_ms(100);

        gps_init_ctx_t gps_init_ctx = {
            .gps_queue = gps_queue,
            .gps_semaphore = gps_init_semaphore,
            .status_queue = led_status_queue};
        gps_init(&gps_init_ctx);

        wifi_ctx_t wifi_ctx = {
            .ssid = current_config.wifi_ssid,
            .password = current_config.wifi_password,
            .status_queue = led_status_queue,
            .wifi_semaphore = wifi_connected_semaphore,
            .gps_semaphore = gps_init_semaphore};
        xTaskCreate(wifi_init_task, "WiFiInit", 2048, &wifi_ctx, configMAX_PRIORITIES - 1, NULL);

        sensor_ctx_t sensor_ctx = {
            .sensor_queue = sensor_queue,
            .wifi_semaphore = wifi_connected_semaphore,
            .status_queue = led_status_queue};
        xTaskCreate(read_accel_gyro_task, "ReadAccelGyro", 2048, &sensor_ctx, configMAX_PRIORITIES - 2, NULL);

        emergency_button_ctx_t button_ctx = {
            .button_pin = EMERGENCY_BUTTON_PIN,
            .button_semaphore = emergency_button_pressed_semaphore,
            .event_queue = event_queue,
            .wifi_semaphore = wifi_connected_semaphore,
            .buzzer_queue = buzzer_queue,
            .gps_req = gps_request_location_queue,
            .last_press_time = last_button_press_time};
        xTaskCreate(emergency_button_task, "EmergencyButton", 512, &button_ctx, configMAX_PRIORITIES - 2, NULL);

        fall_detection_ctx_t fall_ctx = {
            .sensor_queue = sensor_queue,
            .event_queue = event_queue,
            .buzzer_queue = buzzer_queue,
            .gps_req = gps_request_location_queue,
            .wifi_semaphore = wifi_connected_semaphore};
        xTaskCreate(fall_detection_task, "FallDetection", 8192, &fall_ctx, configMAX_PRIORITIES - 3, NULL);

        buzzer_ctx_t buzzer_ctx = {
            .buzzer_pin = BUZZER_PIN,
            .buzzer_queue = buzzer_queue,
            .wifi_semaphore = wifi_connected_semaphore};
        xTaskCreate(buzzer_task, "Buzzer", 512, &buzzer_ctx, configMAX_PRIORITIES - 4, NULL);

        gps_ctx_t gps_ctx = {
            .gps_queue = gps_queue,
            .gps_req = gps_request_location_queue,
            .wifi_semaphore = wifi_connected_semaphore,
        };
        xTaskCreate(gps_location_task, "GPSLocation", 1024, &gps_ctx, configMAX_PRIORITIES - 5, NULL);

        network_ctx_t network_ctx = {
            .enable_req = &enabled_http_req,
            .msg = msg,
            .msg_size = MSG_BUFFER_SIZE,
            .username = current_config.user_name,
            .url = url,
            .telegram_chat_id = TELEGRAM_CHAT_ID,
            .mgr = &mongoose_manager,
            .event_queue = event_queue,
            .status_queue = led_status_queue,
            .gps_queue = gps_queue,
            .wifi_semaphore = wifi_connected_semaphore};
        xTaskCreate(network_task, "Network", 2048, &network_ctx, configMAX_PRIORITIES - 6, NULL);

        vTaskStartScheduler();

        while (1)
        {
            tight_loop_contents();
        }
    }
}