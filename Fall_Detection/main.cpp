#include <stdio.h>
#include "pico/stdlib.h"

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

// -----NETWORK VARIABLES ------
#define WIFI_SSID ""
#define WIFI_PASS ""
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
SemaphoreHandle_t gps_fixed_semaphore;

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("\nStarting project...\n");

    gps_fixed_semaphore = xSemaphoreCreateBinary();
    wifi_connected_semaphore = xSemaphoreCreateBinary();
    emergency_button_pressed_semaphore = xSemaphoreCreateBinary();

    sensor_queue = xQueueCreate(500, sizeof(sensor_data_t));
    event_queue = xQueueCreate(16, sizeof(event_type_t));
    buzzer_queue = xQueueCreate(16, sizeof(event_type_t));
    led_status_queue = xQueueCreate(8, sizeof(system_status_t));
    gps_queue = xQueueCreate(16, sizeof(gps_data_t));
    gps_request_location_queue = xQueueCreate(16, sizeof(event_type_t));

    led_ctx_t led_ctx = {
        .red_pin = RED_LED_PIN,
        .green_pin = GREEN_LED_PIN,
        .blue_pin = BLUE_LED_PIN,
        .status_queue = led_status_queue,
        .current_status = SYSTEM_STATUS_STARTING};
    xTaskCreate(led_rgb_status_task, "LEDStatus", 512, &led_ctx, configMAX_PRIORITIES - 1, NULL);
    sleep_ms(100);

    gps_init_ctx_t gps_init_ctx = {
        .gps_queue = gps_queue,
        .gps_req = gps_request_location_queue,
        .gps_semaphore = gps_fixed_semaphore,
        .status_queue = led_status_queue};
    xTaskCreate(gps_init_task, "GPSInit", 1024, &gps_init_ctx, configMAX_PRIORITIES - 1, NULL);

    system_status_t init_status = SYSTEM_STATUS_STARTING;
    xQueueSend(led_status_queue, &init_status, 0);

    wifi_ctx_t wifi_ctx = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .status_queue = led_status_queue,
        .wifi_semaphore = wifi_connected_semaphore,
        .gps_semaphore = gps_fixed_semaphore
    };
    xTaskCreate(wifi_init_task, "WiFiInit", 2048, &wifi_ctx, configMAX_PRIORITIES - 2, NULL);

    sensor_ctx_t sensor_ctx = {
        .sensor_queue = sensor_queue,
        .wifi_semaphore = wifi_connected_semaphore,
        .status_queue = led_status_queue};
    xTaskCreate(read_accel_gyro_task, "ReadAccelGyro", 2048, &sensor_ctx, configMAX_PRIORITIES - 3, NULL);

    emergency_button_ctx_t button_ctx = {
        .button_pin = EMERGENCY_BUTTON_PIN,
        .button_semaphore = emergency_button_pressed_semaphore,
        .event_queue = event_queue,
        .wifi_semaphore = wifi_connected_semaphore,
        .buzzer_queue = buzzer_queue,
        .gps_req = gps_request_location_queue,
        .last_press_time = last_button_press_time};
    xTaskCreate(emergency_button_task, "EmergencyButton", 512, &button_ctx, configMAX_PRIORITIES - 3, NULL);

    fall_detection_ctx_t fall_ctx = {
        .sensor_queue = sensor_queue,
        .event_queue = event_queue,
        .buzzer_queue = buzzer_queue,
        .gps_req = gps_request_location_queue,
        .wifi_semaphore = wifi_connected_semaphore};
    xTaskCreate(fall_detection_task, "FallDetection", 8192, &fall_ctx, configMAX_PRIORITIES - 4, NULL);

    buzzer_ctx_t buzzer_ctx = {
        .buzzer_pin = BUZZER_PIN,
        .buzzer_queue = buzzer_queue,
        .wifi_semaphore = wifi_connected_semaphore};
    xTaskCreate(buzzer_task, "Buzzer", 512, &buzzer_ctx, configMAX_PRIORITIES - 5, NULL);

    gps_ctx_t gps_ctx = {
        .gps_queue = gps_queue,
        .gps_req = gps_request_location_queue,
        .wifi_semaphore = wifi_connected_semaphore,
    };
    xTaskCreate(gps_location_task, "GPSLocation", 1024, &gps_ctx, configMAX_PRIORITIES - 6, NULL);

    network_ctx_t network_ctx = {
        .enable_req = &enabled_http_req,
        .msg = msg,
        .msg_size = MSG_BUFFER_SIZE,
        .username = user_name,
        .url = url,
        .telegram_chat_id = TELEGRAM_CHAT_ID,
        .mgr = &mongoose_manager,
        .event_queue = event_queue,
        .status_queue = led_status_queue,
        .gps_queue = gps_queue,
        .wifi_semaphore = wifi_connected_semaphore};
    xTaskCreate(network_task, "Network", 2048, &network_ctx, configMAX_PRIORITIES - 7, NULL);

    vTaskStartScheduler();
    while (1)
    {
        tight_loop_contents();
    }
}