#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct{
    uint32_t magic;
    char wifi_ssid[64];
    char wifi_password[64];
    char user_name[64];
    bool valid;
} config_t;

void config_set_defaults(config_t *config);
bool config_load(config_t *config);
bool config_save(config_t *config);