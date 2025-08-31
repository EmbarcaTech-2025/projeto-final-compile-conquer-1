#include <stdio.h>
#include "network.h"
#include "types.h"
#include "memory.h"

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    network_ctx_t *net_ctx = (network_ctx_t *)c->fn_data;
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
        struct mg_str host = mg_url_host(net_ctx->url);
        struct mg_tls_opts opts = {
            .name = host};

        mg_tls_init(c, &opts);
        break;
    }

    case MG_EV_TLS_HS:
    {

        char *json_buffer = mg_mprintf(
            "{%m:%m,%m:%m,%m:%s}",
            MG_ESC("chat_id"), MG_ESC(net_ctx->telegram_chat_id),
            MG_ESC("text"), MG_ESC(net_ctx->msg),
            MG_ESC("disable_notification"), "false");
        if (json_buffer == NULL)
        {
            printf("Failed to allocate JSON buffer\n");
            mg_error(c, "Memory allocation failed");
            c->is_closing = 1;
            return;
        }
        int content_length = strlen(json_buffer);
        struct mg_str host = mg_url_host(net_ctx->url);

        // Send HTTP POST
        mg_printf(c,
                  "%s %s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %d\r\n"
                  "Connection: close\r\n"
                  "\r\n",
                  "POST", mg_url_uri(net_ctx->url),
                  (int)host.len, host.buf,
                  content_length);
        mg_send(c, json_buffer, content_length);
        free(json_buffer);
        printf("Request sent: %s\n", net_ctx->msg);
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
            xQueueSend(net_ctx->status_queue, &working_status, 0);
        }
        else
        {
            system_status_t error_status = SYSTEM_STATUS_ERROR;
            xQueueSend(net_ctx->status_queue, &error_status, 0);
        }

        c->is_draining = 1;
        if (net_ctx->enable_req)
            *((volatile bool *)net_ctx->enable_req) = true;
        break;
    }

    case MG_EV_ERROR:
    {
        char *err = (char *)ev_data;
        printf("Error: %s\n", err ? err : "unknown");
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(net_ctx->status_queue, &error_status, 0);
        c->is_closing = 1;
        if (net_ctx->enable_req)
            *((volatile bool *)net_ctx->enable_req) = true;
        break;
    }

    case MG_EV_CLOSE:
        printf("HTTP Connection closed\n");
        if (net_ctx->enable_req)
            *((volatile bool *)net_ctx->enable_req) = true;
        break;
    }
}

void network_task(void *pvParameters)
{
    network_ctx_t *net_ctx = (network_ctx_t *)pvParameters;

    printf("Mongoose task waiting for WiFi...\n");
    xSemaphoreTake(net_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(net_ctx->wifi_semaphore);

    printf("Starting Mongoose HTTP client...\n");
    mg_mgr_init(net_ctx->mgr);
    mg_log_set(MG_LL_INFO);

    MG_INFO(("Initialising Mongoose..."));
    event_type_t event;
    while (1)
    {
        mg_mgr_poll(net_ctx->mgr, 10);
        if (*net_ctx->enable_req && xQueueReceive(net_ctx->event_queue, &event, 0) == pdTRUE)
        {
            *net_ctx->enable_req = false;
            // printf("Notification received: %lu\n", event);
            if (event == EVENT_FALL_DETECTED)
            {
                snprintf(net_ctx->msg, net_ctx->msg_size, "🚨 Fall Alert: %s has fallen and might need assistance.", net_ctx->username);
            }
            else if (event == EVENT_EMERGENCY_BUTTON_PRESSED)
            {
                snprintf(net_ctx->msg, net_ctx->msg_size, "🖲️ Emergency Button: %s has pressed the emergency button and might need assistance.", net_ctx->username);
            }

            mg_http_connect(net_ctx->mgr, net_ctx->url, http_ev_handler, (void *)net_ctx);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}