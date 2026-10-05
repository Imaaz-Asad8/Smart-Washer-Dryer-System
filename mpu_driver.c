#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "hal/i2c_types.h"
#include "mpu6050.h"
#include "soc/clk_tree_defs.h"
#include "soc/gpio_num.h"
#include <stdint.h>

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t mpu;



i2c_device_config_t device = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = mpu_address,
    .scl_speed_hz = 100000,
};


esp_err_t mpu_6050_begin(gpio_num_t sda, gpio_num_t scl){

    i2c_master_bus_config_t master = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = scl,
    .sda_io_num = sda,
    
};

    esp_err_t error;

    error = i2c_new_master_bus(&master, &bus);

    if (error != ESP_OK){
        return error;
    }

    error = i2c_master_bus_add_device(bus, &device, &mpu);

    if (error != ESP_OK){
        return error;
    }

    uint8_t data[2] = {power_mgmt, 0x0};
    error = i2c_master_transmit(mpu, data, 2, 5000);

    if (error != ESP_OK){
        return error;
    }

    return ESP_OK;
}

esp_err_t get_address(uint8_t *received){
    uint8_t sent[1] = {whoami};
    esp_err_t error;
    

    error = i2c_master_transmit_receive(mpu, sent, 1, received, 1, 5000);

    return error;
}

esp_err_t read_acceleration(data *acceleration){
    uint8_t sent = accel_start;
    uint8_t received[6];
    esp_err_t error;

    error = i2c_master_transmit_receive(mpu, &sent, 1, received, 6, 5000);

    if (error != ESP_OK){
        return error;
    }

    int16_t raw_x = (int16_t) ((received[0] << 8) | received[1]);
    int16_t raw_y = (int16_t) ((received[2] << 8) | received[3]);
    int16_t raw_z = (int16_t) ((received[4] << 8) | received[5]);

    acceleration->x_accel = raw_x / 16384.0f;
    acceleration->y_accel = raw_y / 16384.0f;
    acceleration->z_accel = raw_z / 16384.0f;

    return ESP_OK;

}

