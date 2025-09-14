#include <stdio.h>
#include "sensor.h"
#include "mpu6050_i2c.h"
#include "hardware/watchdog.h"
#include "types.h"
#include "task_watchdog.h"

void read_accel_gyro_task(void *pvParameters)
{
    sensor_ctx_t *sensor_ctx = (sensor_ctx_t *)pvParameters;

    printf("Accelerometer task waiting for WiFi...\n");
    xSemaphoreTake(sensor_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(sensor_ctx->wifi_semaphore);

    watchdog_register_task("AccelGyro");

    int mpu_result = mpu6050_setup_i2c();
    if (mpu_result != 0)
    {
        system_status_t error_status = SYSTEM_STATUS_ERROR;
        xQueueSend(sensor_ctx->status_queue, &error_status, 0);
        printf("CRITICAL: MPU6050 setup failed. Rebooting...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        watchdog_reboot(0, 0, 0);
    }

    mpu6050_reset();
    mpu6050_set_accel_range(3);
    mpu6050_set_gyro_range(3);

    sensor_data_t sensor_data;
    int16_t temp;

    TickType_t last_wake_time = xTaskGetTickCount();
    printf("Starting AccelGyro reading task...\n");

    while (true)
    {
        watchdog_task_alive("AccelGyro");
        mpu6050_read_raw(sensor_data.accel, sensor_data.gyro, &temp);
        xQueueSend(sensor_ctx->sensor_queue, &sensor_data, 0);

        /*printf("Accel: X=%d Y=%d Z=%d | Gyro: X=%d Y=%d Z=%d | Temp: %d\n",
                   sensor_data.accel[0], sensor_data.accel[1], sensor_data.accel[2],
                   sensor_data.gyro[0], sensor_data.gyro[1], sensor_data.gyro[2],
                   temp);*/

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(20));
    }
}