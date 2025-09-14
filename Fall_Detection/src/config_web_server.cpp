#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "mongoose.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "task_watchdog.h"
#include "config_web_server.h"

static struct mg_mgr mgr;
static bool config_mode = false;
static volatile TickType_t reboot_tick = 0;

const char *config_html =
    "<!DOCTYPE html>\n"
    "<html><head><title>Fall Detection Config</title>\n"
    "<style>\n"
    "body{font-family:Arial;margin:40px;background:#f0f0f0}\n"
    ".container{max-width:500px;margin:0 auto;background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}\n"
    "h1{color:#333;text-align:center}\n"
    ".form-group{margin-bottom:20px}\n"
    "label{display:block;margin-bottom:5px;font-weight:bold;color:#555}\n"
    "input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}\n"
    "button{background:#007cba;color:white;padding:12px 30px;border:none;border-radius:5px;cursor:pointer;font-size:16px;width:100%}\n"
    "button:hover{background:#005a8b}\n"
    ".status{padding:10px;margin:10px 0;border-radius:5px}\n"
    ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}\n"
    ".error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}\n"
    "</style></head><body>\n"
    "<div class='container'>\n"
    "<h1>Fall Detection Device Configuration</h1>\n"
    "<form method='POST' action='/save'>\n"
    "<div class='form-group'>\n"
    "<label>WiFi SSID:</label>\n"
    "<input type='text' name='wifi_ssid' value='%s' required>\n"
    "</div>\n"
    "<div class='form-group'>\n"
    "<label>WiFi Password:</label>\n"
    "<input type='password' name='wifi_password' value='%s'>\n"
    "</div>\n"
    "<div class='form-group'>\n"
    "<label>Username:</label>\n"
    "<input type='text' name='user_name' value='%s' required>\n"
    "</div>\n"
    "<button type='submit'>Save Configuration</button>\n"
    "</form>\n"
    "</div></body></html>";

const char *success_html =
    "<!DOCTYPE html>\n"
    "<html><head><title>Configuration Saved</title>\n"
    "<style>body{font-family:Arial;text-align:center;margin-top:100px;background:#f0f0f0}\n"
    ".message{background:white;display:inline-block;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}\n"
    "</style></head><body>\n"
    "<div class='message'>\n"
    "<h1 style='color:#28a745'>Configuration Saved!</h1>\n"
    "<p>Device will restart and connect to the configured WiFi network.</p>\n"
    "</div></body></html>";

static void http_web_server_handler(struct mg_connection *c, int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        config_t *curr_config = (config_t *)c->fn_data;
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            char *response = mg_mprintf(config_html,
                                        curr_config->wifi_ssid,
                                        curr_config->wifi_password,
                                        curr_config->user_name);
            if (!response)
            {
                mg_http_reply(c, 500, "Content-Type: text/plain\r\n", "Allocation error");
                return;
            }
            mg_http_reply(c, 200, "Content-Type: text/html; charset=utf-8\r\n", "%s", response);
            free(response);
        }
        else if (mg_match(hm->uri, mg_str("/save"), NULL) && mg_strcmp(hm->method, mg_str("POST")) == 0)
        {
            char ssid[64] = {0};
            char password[64] = {0};
            char username[64] = {0};

            mg_http_get_var(&hm->body, "wifi_ssid", ssid, sizeof(ssid));
            mg_http_get_var(&hm->body, "wifi_password", password, sizeof(password));
            mg_http_get_var(&hm->body, "user_name", username, sizeof(username));

            if (strlen(ssid) == 0 || strlen(password) == 0 || strlen(username) == 0)
            {
                mg_http_reply(c, 400, "Content-Type: text/plain\r\n", "All fields are required.");
                return;
            }

            strncpy(curr_config->wifi_ssid, ssid, sizeof(curr_config->wifi_ssid) - 1);
            strncpy(curr_config->wifi_password, password, sizeof(curr_config->wifi_password) - 1);
            strncpy(curr_config->user_name, username, sizeof(curr_config->user_name) - 1);
            curr_config->valid = true;

            if (config_save(curr_config))
            {
                mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", success_html);
                printf("Configuration saved. Reboot in 10s...\n");
                reboot_tick = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
            }
            else
            {
                mg_http_reply(c, 500, "Content-Type: text/plain\r\n", "Failed to save configuration.");
            }
        }
        else
        {
            mg_http_reply(c, 404, "", "Not Found");
        }
    }
}

void config_web_server_task(void *pvParameters)
{
    printf("Web server task starting...\n");
    config_t *curr_config = (config_t *)pvParameters;

    const char *ap_ssid = "FallDetector_Config";
    const char *ap_password = "config123";

    if (!cyw43_is_initialized(&cyw43_state))
    {
        printf("Initializing CYW43 for AP mode...\n");
        int r = cyw43_arch_init();
        if (r != 0)
        {
            printf("CRITICAL: cyw43_arch_init failed (%d). AP unavailable.\n", r);
            vTaskDelay(pdMS_TO_TICKS(1000));
            watchdog_reboot(0, 0, 0);
        }
    }
    else
    {
        printf("CYW43 already initialized.\n");
    }

    cyw43_arch_enable_ap_mode(ap_ssid, ap_password, CYW43_AUTH_WPA2_AES_PSK);
    printf("AP mode enabled. SSID=%s PASS=%s Connect and browse http://192.168.4.1\n", ap_ssid, ap_password);

    mg_mgr_init(&mgr);
    mg_log_set(MG_LL_INFO);

    struct mg_connection *c = mg_http_listen(&mgr, "http://0.0.0.0:80", http_web_server_handler, curr_config);
    if (c == NULL)
    {
        printf("CRITICAL: Failed to start HTTP listener on port 80!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        watchdog_reboot(0, 0, 0);
    }
    printf("HTTP listener started successfully\n");
    while (1)
    {
        mg_mgr_poll(&mgr, 10);
        if (reboot_tick && xTaskGetTickCount() >= reboot_tick)
        {   
            printf("Rebooting now...\n");
            watchdog_reboot(0, 0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}