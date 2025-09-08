#pragma once
#include "minmea.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct {
    float lat;
    float lon;
    float alt;
    int sat;
    bool fix;
} gps_data_t;

typedef struct {
    QueueHandle_t gps_queue;
    QueueHandle_t gps_req;
    SemaphoreHandle_t wifi_semaphore;
    SemaphoreHandle_t gps_semaphore;
} gps_ctx_t;

typedef struct {
    QueueHandle_t gps_queue;
    QueueHandle_t gps_req;
    SemaphoreHandle_t wifi_semaphore;
    SemaphoreHandle_t gps_semaphore;
    QueueHandle_t status_queue;
} gps_init_ctx_t;

void gps_get_maps_link(char *buffer, size_t buffer_size ,double lat, double lon);
bool parse_nmea_sentence(const char* sentence, gps_data_t* data);
void gps_init_task(void *pvParameters);
void gps_location_task(void *pvParameters);