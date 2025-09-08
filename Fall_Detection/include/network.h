#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "mongoose.h"

typedef struct{
    volatile bool *enable_req;
    char *msg;
    size_t msg_size;
    char *username;
    const char *url;
    const char *telegram_chat_id;
    struct mg_mgr *mgr;
    QueueHandle_t event_queue;
    QueueHandle_t status_queue;
    QueueHandle_t gps_queue;
    SemaphoreHandle_t wifi_semaphore;
} network_ctx_t;

void network_task(void *pvParameters);