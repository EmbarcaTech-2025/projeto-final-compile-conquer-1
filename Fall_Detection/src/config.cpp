#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>
#include "config.h"

#define CONFIG_MAGIC 0xDEADBEEF
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#define CONFIG_OFFSET ( PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE )

void config_set_defaults(config_t *config){
    memset(config, 0, sizeof(config_t));
    config->magic = CONFIG_MAGIC;
    strcpy(config->wifi_ssid, "");
    strcpy(config->wifi_password, "");
    strcpy(config->user_name, "");
    config->valid = false;
}

bool config_load(config_t *config){
    const config_t *stored_config = (const config_t *)(XIP_BASE + CONFIG_OFFSET);
    memcpy(config, stored_config, sizeof(config_t));

    return (config->magic == CONFIG_MAGIC && config->valid);
}

bool config_save(config_t *config ){
    config_t config_copy = *config;
    config_copy.magic = CONFIG_MAGIC;
    config_copy.valid = true;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(CONFIG_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(CONFIG_OFFSET, (const uint8_t *)&config_copy, sizeof(config_t));
    restore_interrupts(ints);       

    return true;
}