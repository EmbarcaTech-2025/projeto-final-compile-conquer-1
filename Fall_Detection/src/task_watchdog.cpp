#include <stdio.h>
#include <string.h>
#include "hardware/watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "task_watchdog.h"
#include "types.h"
#include "led.h"

#define TASK_TIMEOUT_MS 20000
#define WATCHDOG_HW_TIMEOUT_MS 8000

typedef struct {
    char name[16];
    uint32_t last_heartbeat_ms;
}task_info_t;

task_info_t tasks[10];
int task_count = 0;

void watchdog_init(){
    watchdog_enable(WATCHDOG_HW_TIMEOUT_MS, false);
    printf("Watchdog initialized: HW timeout %d ms, Task timeout %d ms\n", WATCHDOG_HW_TIMEOUT_MS, TASK_TIMEOUT_MS);
}

void watchdog_register_task(const char* task_name){
    strncpy(tasks[task_count].name, task_name,15);
    tasks[task_count].name[15] = '\0';
    tasks[task_count].last_heartbeat_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    task_count++;
    printf("Task %s registered to watchdog\n", task_name);
}

void watchdog_task_alive(const char* task_name){
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for(int i = 0; i < task_count; i++){
        if (strcmp(tasks[i].name, task_name) == 0){
            tasks[i].last_heartbeat_ms = now;
            return;
        }
    }
}

void watchdog_monitor_task(void *pvParameters){
    watchdog_register_task("MonitorTask");
    watchdog_ctx_t *watchdog_ctx = (watchdog_ctx_t *)pvParameters;
    
    while (1)
    {
        watchdog_task_alive("MonitorTask");
        watchdog_update();
        
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        for(int i = 0; i < task_count; i++){
            uint32_t silence_time = now - tasks[i].last_heartbeat_ms;

            if (now > tasks[i].last_heartbeat_ms && silence_time > TASK_TIMEOUT_MS){
                printf("CRITICAL: Task %s unresponsive for %d ms (limit %d ms) - Triggering watchdog reset.\n", tasks[i].name, silence_time, TASK_TIMEOUT_MS);
                system_status_t error = SYSTEM_STATUS_ERROR;
                xQueueSend(watchdog_ctx->status_queue, &error, 0);
                int i = 0;
                while (1)
                {
                    printf("Waiting for watchdog to reset the system...%d/%d\n", i++, WATCHDOG_HW_TIMEOUT_MS / 1000);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}