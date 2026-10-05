#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "mpu6050.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "wifi_initialize.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define finished 1
#define started 0

void readAccel(void *);
void sendPush(void *);
void findThreshold(void *);

TaskHandle_t notis;
QueueHandle_t vibrationQueue;

bool dryer_on = false;

void app_main(void) {
  uint8_t received;

  wifi_init();

  ESP_ERROR_CHECK(mpu_6050_begin(GPIO_NUM_5, GPIO_NUM_6));
  ESP_ERROR_CHECK(get_address(&received));

  printf("Address: %X\n", received);

  xTaskCreate(sendPush, "noti", 4096, NULL, 2, &notis);
  vibrationQueue = xQueueCreate(1, sizeof(float));
  xTaskCreate(readAccel, "task", 4096, NULL, 1, NULL);
  xTaskCreate(findThreshold, "threshold", 2048, NULL, 1, NULL);
}

void readAccel(void *pvParameters) {

  data acceleration;

  while (1) {
    esp_err_t error = (read_acceleration(&acceleration));

    float magnitude = sqrtf(acceleration.x_accel * acceleration.x_accel +
                            acceleration.y_accel * acceleration.y_accel +
                            acceleration.z_accel * acceleration.z_accel);

    printf("X: %.2f g | Y: %.2f g | Z: %.2f g | Magnitude: %.2f g\n",
           acceleration.x_accel, acceleration.y_accel, acceleration.z_accel,
           magnitude);

    float vibration = fabsf(magnitude - 0.95f);

    xQueueSend(vibrationQueue, &vibration, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void findThreshold(void *pvParameters) {

  int threshold_samples = 0;
  int total_samples = 0;
  float vibration;
  int inactive_intervals = 0;

  while (1) {

    if (xQueueReceive(vibrationQueue, &vibration, portMAX_DELAY)) {

      total_samples++;

      if (vibration > 0.05f) {
        threshold_samples++;
      }

      if (total_samples == 100) {
        printf("Samples above threshold / 100: %d\n", threshold_samples);
        total_samples = 0;

        bool active = threshold_samples > 59;

        if (active) {
          inactive_intervals = 0;
          if (!dryer_on) {
            dryer_on = true;
            printf("Dryer on.\n");
            xTaskNotify(notis, started, eSetValueWithOverwrite);
          }
        } else if (dryer_on && !active) {
          inactive_intervals++;

          if (inactive_intervals > 2) {
            inactive_intervals = 0;
            dryer_on = false;
            printf("Cycle Finished!\n");
            xTaskNotify(notis, finished, eSetValueWithOverwrite);
          }
        }

        threshold_samples = 0;
      }
    }
  }
}

void sendPush(void *pvParameters) {

  uint32_t value;
  char *post_message;

  while (1) {

    xTaskNotifyWait(0, 0, &value, portMAX_DELAY);

    esp_http_client_config_t posting = {
        .url = "",
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach};

    esp_http_client_handle_t client = esp_http_client_init(&posting);

    post_message = value ? "Cycle Finished." : "Cycle Started";

    esp_http_client_set_post_field(client, post_message, strlen(post_message));

    esp_http_client_perform(client);

    esp_http_client_cleanup(client);
  }
}
