#ifndef MPU6050_h
#define MPU6050_h


#include "esp_err.h"
#include "soc/gpio_num.h"
#include <stdint.h>

#define whoami 0x75
#define mpu_address 0x68
#define power_mgmt 0x6B
#define accel_start 0x3B

typedef struct{
    float x_accel;
    float y_accel;
    float z_accel;


} data;

esp_err_t mpu_6050_begin(gpio_num_t sda, gpio_num_t scl);
esp_err_t get_address(uint8_t*);
esp_err_t read_acceleration(data *acceleration);

#endif