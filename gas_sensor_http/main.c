// main_100%.c  — Detector de Gás Inteligente (corrigido e simplificado)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mongoose.h"
static int mg_http_match_uri(const struct mg_http_message *hm, const char *glob) {
    size_t n = hm->uri.len;
    return (n == strlen(glob) && memcmp(hm->uri.buf, glob, n) == 0);
}

//#include "dhcpserver.h"
//#include "dnsserver.h"

// --- Configurações do Modo de Setup (Access Point) ---
#define AP_SSID "PicoGasSetup"
#define AP_PASS "config1234"
#define AP_IP_ADDR "192.168.4.1"

// --- Pinos de Hardware ---
#define MQ02_PIN 17
#define RELAY_PIN 16
#define BUZZER_PIN 21
#define FACTORY_RESET_PIN 5

// --- Constantes da Memória Flash ---
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define CONFIG_MAGIC_NUMBER 0xCAFEF00D

// --- Configuração fixa do Telegram ---
#define TELEGRAM_TOKEN   ""
#define TELEGRAM_CHAT_ID ""

// --- Estrutura de Configuração ---
typedef struct {
    uint32_t magic_number;
    char wifi_ssid[64];
    char wifi_pass[64];
    char device_id[64];
} device_config_t;

// --- Variáveis Globais ---
device_config_t current_config;
char msg[200];
QueueHandle_t event_queue;
volatile bool enabled_http_req = false;

typedef enum { EVENT_GAS_DETECTED } event_type_t;

// --- Protótipos ---
bool load_config_from_flash(device_config_t *config);
void save_config_to_flash(const device_config_t *config);
void start_normal_operation_mode();
void setup_mode_task(void *pvParameters);
void gas_sensor_task(void *pvParameters);
void mongoose_task(void *pvParameters);

// =================================================================================
// FUNÇÃO PRINCIPAL
// =================================================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);

    // <<< INÍCIO DA LÓGICA DE RESET DE FÁBRICA >>>
    gpio_init(FACTORY_RESET_PIN);
    gpio_set_dir(FACTORY_RESET_PIN, GPIO_IN);
    gpio_pull_up(FACTORY_RESET_PIN);
    sleep_ms(10); // Pequeno delay para o pull-up estabilizar

    // Se o botão estiver pressionado (pino em GND), apaga a configuração
    if (!gpio_get(FACTORY_RESET_PIN)) {
        // <<< ADICIONADO: Print para confirmar que o botão foi lido >>>
        printf("CHK: Botão de reset de fábrica pressionado! Apagando configuração...\n");
        
        // Cria uma config com magic number 0 para invalidar a flash
        device_config_t empty_config = {0}; 
        save_config_to_flash(&empty_config); 
        
        // <<< ADICIONADO: Print para confirmar a ação antes de reiniciar >>>
        printf("Configuração apagada. Reiniciando...\n");
        watchdog_reboot(0, 0, 0); // Reinicia para entrar no modo de setup
    }

    memset(&current_config, 0, sizeof(current_config));

    printf("CHK: main start\n");

    if (load_config_from_flash(&current_config)) {
        printf("CHK: config loaded -> id='%s' ssid='%s'\n",
               current_config.device_id, current_config.wifi_ssid);
        start_normal_operation_mode();
    } else {
        printf("CHK: no config found -> entering setup mode\n");
        xTaskCreate(setup_mode_task, "SetupTask", 8192, NULL, 1, NULL);
        vTaskStartScheduler();
    }
    while (1) { tight_loop_contents(); }
}

// =================================================================================
// SEÇÃO 1: FUNÇÕES DE FLASH
// =================================================================================
static size_t round_up_page(size_t n) {
    const size_t P = 256;
    return (n + (P - 1)) & ~(P - 1);
}

bool load_config_from_flash(device_config_t *config) {
    memset(config, 0, sizeof(*config));
    device_config_t tmp;
    const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(&tmp, flash_target_contents, sizeof(tmp));

    if (tmp.magic_number != CONFIG_MAGIC_NUMBER) {
        printf("CHK: config not valid (magic=%08x)\n", tmp.magic_number);
        return false;
    }
    memcpy(config, &tmp, sizeof(*config));

    config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
    config->wifi_pass[sizeof(config->wifi_pass) - 1] = '\0';
    config->device_id[sizeof(config->device_id) - 1] = '\0';

    if (strlen(config->wifi_ssid) == 0 || strlen(config->wifi_pass) == 0) return false;
    return true;
}

void save_config_to_flash(const device_config_t *config) {
    device_config_t tmp;
    memcpy(&tmp, config, sizeof(tmp));
    tmp.magic_number = CONFIG_MAGIC_NUMBER;

    size_t prog_size = round_up_page(sizeof(tmp));
    uint8_t *prog_buf = malloc(prog_size);
    memset(prog_buf, 0xff, prog_size);
    memcpy(prog_buf, &tmp, sizeof(tmp));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, prog_buf, prog_size);
    restore_interrupts(ints);

    free(prog_buf);
    printf("CHK: config saved\n");
}

// =================================================================================
// SEÇÃO 2: MODO DE OPERAÇÃO NORMAL
// =================================================================================
typedef struct {
    volatile bool *enabled_flag;
    char url[256];
} http_req_ctx_t;

static void event_handler_normal_mode(struct mg_connection *c, int ev, void *ev_data) {
    http_req_ctx_t *ctx = (http_req_ctx_t *) c->fn_data;
    if (ev == MG_EV_CONNECT) {
        if (ctx && ctx->url[0]) {
            struct mg_str host = mg_url_host(ctx->url);
            if (host.len > 0) mg_tls_init(c, &(struct mg_tls_opts){ .name = host });
        }
    } else if (ev == MG_EV_TLS_HS) {
        char *json_buffer = mg_mprintf("{%m:%m,%m:%m}",
                                       MG_ESC("chat_id"), MG_ESC(TELEGRAM_CHAT_ID),
                                       MG_ESC("text"), MG_ESC(msg));
        int content_length = strlen(json_buffer);
        struct mg_str host = mg_url_host(ctx->url);
        mg_printf(c, "POST %s HTTP/1.1\r\nHost: %.*s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n",
                  mg_url_uri(ctx->url), (int)host.len, host.buf, content_length);
        mg_send(c, json_buffer, content_length);
        free(json_buffer);
    } else if (ev == MG_EV_HTTP_MSG || ev == MG_EV_ERROR) {
        c->is_closing = 1;
        if (ctx && ctx->enabled_flag) *(ctx->enabled_flag) = true;
        free(ctx);
        c->fn_data = NULL;
    }
    (void) ev_data;
}

void start_normal_operation_mode() {
    event_queue = xQueueCreate(10, sizeof(event_type_t));
    if (!event_queue) watchdog_reboot(0, 0, 0);

    xTaskCreate(gas_sensor_task, "GasSensorTask", 2048, NULL, 1, NULL);
    xTaskCreate(mongoose_task, "MongooseTask", 8192, NULL, 2, NULL);
    vTaskStartScheduler();
}

void gas_sensor_task(void *pvParameters) {
    // --- Configuração dos pinos ---
    gpio_init(MQ02_PIN); gpio_set_dir(MQ02_PIN, GPIO_IN);
    gpio_init(RELAY_PIN); gpio_set_dir(RELAY_PIN, GPIO_OUT);
    gpio_put(RELAY_PIN, 1); // Relé começa ligado

    // --- Configuração do Buzzer ---
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 500);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // buzzer desligado

    bool gas_was_detected = false;
    
    // --- Variáveis para o long press (CORRIGIDO) ---
    uint64_t reset_button_press_start_time = 0; // <<< CORRIGIDO para uint64_t
    const uint32_t reset_press_duration_ms = 5000; // 5 segundos

    while (1) {
        // --- Lógica do Sensor de Gás (sem alterações) ---
        bool gas_is_detected = !gpio_get(MQ02_PIN);

        if (gas_is_detected) {
            gpio_put(RELAY_PIN, 0);
            pwm_set_gpio_level(BUZZER_PIN, 250);

            if (!gas_was_detected) {
                printf("Gás detectado! Cortando fluxo e enviando notificação...\n");
                event_type_t ev = EVENT_GAS_DETECTED;
                xQueueSend(event_queue, &ev, 0);
                gas_was_detected = true;
            }
        } else {
            gpio_put(RELAY_PIN, 1);
            pwm_set_gpio_level(BUZZER_PIN, 0);

            if (gas_was_detected) {
                printf("Gás dissipado. Restaurando fluxo.\n");
                gas_was_detected = false;
            }
        }

        // --- Lógica do Botão de Reset de Fábrica (long press) (CORRIGIDO) ---
        if (!gpio_get(FACTORY_RESET_PIN)) { // Se o botão está pressionado
            if (reset_button_press_start_time == 0) {
                // Marca o tempo de início do pressionamento
                reset_button_press_start_time = to_ms_since_boot(get_absolute_time());
                printf("Botão de reset pressionado. Segure por 5 segundos para apagar a configuração...\n");
            } else {
                // Se o botão continua pressionado, verifica se já se passaram 5 segundos
                uint64_t current_time = to_ms_since_boot(get_absolute_time()); // <<< CORRIGIDO para uint64_t
                if (current_time - reset_button_press_start_time >= reset_press_duration_ms) {
                    printf("Reset de fábrica ativado! Apagando configuração e reiniciando...\n");
                    device_config_t empty_config = {0};
                    save_config_to_flash(&empty_config);
                    watchdog_reboot(0, 0, 0);
                }
            }
        } else { // Se o botão está solto
            if (reset_button_press_start_time != 0) {
                // Se o botão foi solto antes dos 5 segundos, cancela o reset
                printf("Reset cancelado. Botão solto antes do tempo.\n");
                reset_button_press_start_time = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Verifica o estado a cada 100ms
    }
}

void mongoose_task(void *pvParameters) {
    if (!current_config.wifi_ssid[0] || !current_config.wifi_pass[0]) vTaskDelete(NULL);

    if (cyw43_arch_init()) vTaskDelete(NULL);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(current_config.wifi_ssid,
                                           current_config.wifi_pass,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        watchdog_reboot(0, 0, 0);
    }

    struct mg_mgr mgr; mg_mgr_init(&mgr);
    char base_url[256];
    snprintf(base_url, sizeof(base_url), "https://api.telegram.org/bot%s/sendMessage",
             TELEGRAM_TOKEN);

    enabled_http_req = false;
    snprintf(msg, sizeof(msg), "Detector Online: %s", current_config.device_id);

    http_req_ctx_t *ctx = malloc(sizeof(http_req_ctx_t));
    ctx->enabled_flag = &enabled_http_req;
    strncpy(ctx->url, base_url, sizeof(ctx->url) - 1);
    mg_http_connect(&mgr, ctx->url, event_handler_normal_mode, (void *) ctx);

    event_type_t event;
    while (1) {
        mg_mgr_poll(&mgr, 100);
        if (enabled_http_req && xQueueReceive(event_queue, &event, 0) == pdTRUE) {
            enabled_http_req = false;
            snprintf(msg, sizeof(msg), "ALERTA: Gás detectado em '%s'!", current_config.device_id);
            http_req_ctx_t *ctx2 = malloc(sizeof(http_req_ctx_t));
            ctx2->enabled_flag = &enabled_http_req;
            strncpy(ctx2->url, base_url, sizeof(ctx2->url) - 1);
            mg_http_connect(&mgr, ctx2->url, event_handler_normal_mode, (void *) ctx2);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =================================================================================
// SEÇÃO 3: MODO DE CONFIGURAÇÃO
// =================================================================================
static void event_handler_setup_mode(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_http_match_uri(hm, "/save")) {
            device_config_t new_cfg; memset(&new_cfg, 0, sizeof(new_cfg));
            mg_http_get_var(&hm->body, "ssid", new_cfg.wifi_ssid, sizeof(new_cfg.wifi_ssid));
            mg_http_get_var(&hm->body, "pass", new_cfg.wifi_pass, sizeof(new_cfg.wifi_pass));
            mg_http_get_var(&hm->body, "device_id", new_cfg.device_id, sizeof(new_cfg.device_id));

            save_config_to_flash(&new_cfg);

            mg_http_reply(c, 200, "Content-Type: text/html; charset=utf-8\r\n",
                          "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                          "<style>body{font-family:sans-serif;background:#f5f5f5;text-align:center;}"
                          "form{margin:20px auto;padding:20px;background:white;max-width:300px;border-radius:10px;}"
                          "input,button{margin:5px;width:90%;padding:8px;border-radius:5px;border:1px solid #ccc;}"
                          "button{background:#0078d7;color:white;border:none;cursor:pointer;}"
                          "button:hover{background:#005a9e;}</style></head><body>"
                          "<h2>✅ Configuração salva!</h2>"
                          "<p>O dispositivo será reiniciado...</p>"
                          "</body></html>");
            sleep_ms(2000);
            watchdog_reboot(0, 0, 0);
        } else {
            mg_http_reply(c, 200, "Content-Type: text/html; charset=utf-8\r\n",
                          "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                          "<style>body{font-family:sans-serif;background:#f5f5f5;text-align:center;}"
                          "form{margin:20px auto;padding:20px;background:white;max-width:300px;border-radius:10px;}"
                          "input,button{margin:5px;width:90%;padding:8px;border-radius:5px;border:1px solid #ccc;}"
                          "button{background:#0078d7;color:white;border:none;cursor:pointer;}"
                          "button:hover{background:#005a9e;}</style></head><body>"
                          "<h2>Configuração Pico Gas</h2>"
                          "<form method='POST' action='/save'>"
                          "<input name='ssid' placeholder='Nome do WiFi (SSID)'><br>"
                          "<input name='pass' placeholder='Senha do WiFi' type='password'><br>"
                          "<input name='device_id' placeholder='ID do Dispositivo'><br>"
                          "<button type='submit'>Salvar e Reiniciar</button>"
                          "</form></body></html>");
        }
    }
}

void setup_mode_task(void *pvParameters) {
    if (cyw43_arch_init()) vTaskDelete(NULL);
    cyw43_arch_enable_ap_mode(AP_SSID, AP_PASS, CYW43_AUTH_WPA2_AES_PSK);

    struct mg_mgr mgr; mg_mgr_init(&mgr);
    struct mg_connection *c = mg_http_listen(&mgr, "http://0.0.0.0:80",
                                             event_handler_setup_mode, NULL);
    if (!c) vTaskDelete(NULL);

    printf("CHK: AP '%s' criado. Acesse http://%s\n", AP_SSID, AP_IP_ADDR);

    while (1) {
        mg_mgr_poll(&mgr, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
