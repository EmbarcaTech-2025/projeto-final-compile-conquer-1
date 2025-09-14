#pragma once
#include <stdint.h>

typedef enum
{
    EVENT_FALL_DETECTED,
    EVENT_DAILY_ACTIVITY,
    EVENT_EMERGENCY_BUTTON_PRESSED
} event_type_t;

typedef enum
{
    SYSTEM_STATUS_CONFIG,
    SYSTEM_STATUS_STARTING,
    SYSTEM_STATUS_WORKING,
    SYSTEM_STATUS_ERROR,
} system_status_t;

typedef struct
{
    int16_t accel[3];
    int16_t gyro[3];
} sensor_data_t;