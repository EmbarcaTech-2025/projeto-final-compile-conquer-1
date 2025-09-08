#include <stdio.h>
#include "fall_detection.h"
#include "types.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "memory.h"

static float *g_feature_ptr = NULL;

int get_signal_data(size_t offset, size_t length, float *out_ptr)
{
    // This function is called by Edge Impulse to get chunks of your sensor data
    // Parameters:
    //   offset: starting position in the features array
    //   length: how many values to copy
    //   out_ptr: where to write the data (Edge Impulse expects float*)

    for (size_t i = 0; i < length; i++)
    {
        out_ptr[i] = g_feature_ptr[offset + i];
    }
    return 0;
}


void fall_detection_task(void *pvParameters)
{
    fall_detection_ctx_t *fall_ctx = (fall_detection_ctx_t *)pvParameters;

    printf("Fall detection task waiting for WiFi...\n");
    xSemaphoreTake(fall_ctx->wifi_semaphore, portMAX_DELAY);
    xSemaphoreGive(fall_ctx->wifi_semaphore);

    printf("Starting fall detection...\n");
    sensor_data_t sensor_data;

    static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
    static size_t feature_index = 0;

    g_feature_ptr = features;

    while (true)
    {
        if (xQueueReceive(fall_ctx->sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE)
        {
            int16_t ax = sensor_data.accel[0];
            int16_t ay = sensor_data.accel[1];
            int16_t az = sensor_data.accel[2];
            int16_t gx = sensor_data.gyro[0];
            int16_t gy = sensor_data.gyro[1];
            int16_t gz = sensor_data.gyro[2];

            // Acumulate data for machine learning inference
            if (feature_index < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
            {
                features[feature_index++] = (float)(ax);
                features[feature_index++] = (float)(ay);
                features[feature_index++] = (float)(az);
                features[feature_index++] = (float)(gx);
                features[feature_index++] = (float)(gy);
                features[feature_index++] = (float)(gz);
            }

            // Just run inference when we have enough data
            if (feature_index >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
            {

                signal_t signal;
                signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
                signal.get_data = &get_signal_data;

                // Inferencia
                ei_impulse_result_t result = {0};
                EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

                if (res == EI_IMPULSE_OK)
                {
                    printf("---------------\n");
                    event_type_t event = EVENT_DAILY_ACTIVITY;
                    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
                    {
                        printf("%s: %.5f\n",
                               result.classification[i].label,
                               result.classification[i].value);

                        if (result.classification[i].value > 0.9f &&
                            strcmp(result.classification[i].label, "Fall") == 0)
                        {
                            event = EVENT_FALL_DETECTED;
                            xQueueSend(fall_ctx->event_queue, &event, portMAX_DELAY);
                            xQueueSend(fall_ctx->buzzer_queue, &event, portMAX_DELAY);
                            xQueueSend(fall_ctx->gps_req, &event, portMAX_DELAY);
                            printf("⚠️ MACHINE LEARNING DETECTED FALL! ⚠️\n");
                        }
                    }
                    if (event != EVENT_FALL_DETECTED)
                    {
                        printf("No fall detected\n");
                    }
                }
                else
                {
                    system_status_t error_status = SYSTEM_STATUS_ERROR;
                    xQueueSend(fall_ctx->status_queue, &error_status, 0);
                    printf("Error running classifier: %d\n", res);
                }
                printf("Inference done\n");
                printf("---------------\n");
                feature_index = 0;
                memset(features, 0, sizeof(features));
            }
        }
    }
}