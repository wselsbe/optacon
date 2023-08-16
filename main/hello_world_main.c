/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include <math.h>

#define I2C_MASTER_NUM 0
#define I2C_SCL_PIN   21
#define I2C_SDA_PIN   47

#define DRV2665_ADDR    0x59
#define DRV_REGISTER_0  0x00
#define DRV_REGISTER_1  0x01
#define DRV_REGISTER_2  0x02
#define DRV_REGISTER_DATA  0x0B
#define DRV_RESET       1 << 7

#define _CHIPID_MASK 0b01111000

#define _INPUT_MASK 0b00000100
#define INPUT_DIGITAL 0 << 2
#define INPUT_ANALOG  1 << 2

#define _GAIN_MASK 0b00000011
#define GAIN_25V 0
#define GAIN_50V 1
#define GAIN_75V 2
#define GAIN_100V 3

#define _STANDBY_MASK 0b01000000
#define _STANDBY_FALSE 0 << 6
#define _STANDBY_TRUE 1 << 6

#define _TIMEOUT_MASK  0b00001100
#define TIMEOUT_5MS 0 << 2
#define TIMEOUT_10MS 1 << 2
#define TIMEOUT_15MS 2 << 2
#define TIMEOUT_20MS 3 << 2

#define _ENABLE_MASK 0b00000010
#define ENABLE_AUTO 0 << 1
#define ENABLE_OVERRIDE 1 << 1

#define FIFO_BUFFER_SIZE 100
#define SAMPLE_RATE 8000 // 8 kHz

#define SPI_BUS  SPI2_HOST
#define SPI_SCK  17
#define SPI_MISO 7
#define SPI_MOSI 6
#define SPI_CS   18


size_t sine_wave_size;
uint8_t sine_wave[8000]; // Pre-calculated sine wave for one period


void generate_sine_wave(float frequency) {
    float phase = -M_PI / 2; // Shift phase by 90 degrees to start at minimum value
    float sine_wave_step = 2 * M_PI / sine_wave_size;

    for (size_t i = 0; i < sine_wave_size; i++) {
        float value = sin(phase);
        int twos_comp_value = (int)(value * 127);
        sine_wave[i] = (uint8_t)(twos_comp_value + 0x80);

        phase += sine_wave_step;
        if (phase >= 2 * M_PI) {
            phase -= 2 * M_PI;
        }
    }
}

void drv_init() {
    i2c_config_t i2c_conf;
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = I2C_SDA_PIN;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.scl_io_num = I2C_SCL_PIN;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    i2c_conf.master.clk_speed = 100000;

    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0));

    vTaskDelay(100/portTICK_PERIOD_MS);
    
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (DRV2665_ADDR < 1 ) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, DRV_REGISTER_2, true);
    i2c_master_write_byte(i2c_cmd, DRV_RESET, true);
    i2c_master_stop(i2c_cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, i2c_cmd, pdMS_TO_TICKS(5000)));
    i2c_cmd_link_delete(i2c_cmd);
}

void drv_write_fifo(uint8_t* data, size_t data_len) {
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (DRV2665_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, DRV_REGISTER_DATA, true);
    i2c_master_write(i2c_cmd, data, data_len, true);
    i2c_master_stop(i2c_cmd);
    i2c_cmd_link_delete(i2c_cmd);
}

void drv_enable_digital() {    
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();

    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (DRV2665_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, DRV_REGISTER_1, true);    
    i2c_master_write_byte(i2c_cmd, INPUT_DIGITAL & GAIN_100V, true);
    i2c_master_stop(i2c_cmd);

    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (DRV2665_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, DRV_REGISTER_2, true);    
    i2c_master_write_byte(i2c_cmd, _STANDBY_FALSE & TIMEOUT_20MS & ENABLE_AUTO, true);
    i2c_master_stop(i2c_cmd);

    i2c_cmd_link_delete(i2c_cmd);
}

void spi_task(void *arg) {
    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .sclk_io_num = SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 500 * 1000,     // 500 Khz
        .mode = 0,                              // SPI mode 0
        .spics_io_num = SPI_CS,
        .queue_size = 1,
    };

    spi_device_handle_t spi;
    spi_bus_initialize(SPI_BUS, &bus_config, 0);
    spi_bus_add_device(SPI_BUS, &dev_config, &spi);

    uint8_t tx_data[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    while (1) {
        spi_transaction_t trans = {
            .length = 4 * 8, // Length in bits
            .tx_buffer = tx_data,
        };

        spi_device_transmit(spi, &trans);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void writer_task(void *pvParameters) {
    size_t sine_wave_index = 0;

    while (1) {
        size_t write_size = 50;
        uint8_t buffer[write_size];
        for (size_t i = 0; i < write_size; i++) {
            buffer[i] = sine_wave[sine_wave_index];
            sine_wave_index = (sine_wave_index + 1) % sine_wave_size;
        }
        drv_write_fifo(buffer, write_size);

        // Calculate write delay ticks based on write_size and sample rate
        TickType_t write_delay_ticks = (write_size * portTICK_PERIOD_MS * 1000) / SAMPLE_RATE;
        vTaskDelay(write_delay_ticks);          
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    


    float frequency = 250.0; // Desired frequency in Hz
    sine_wave_size = (size_t)((SAMPLE_RATE / frequency) + 0.5); // Rounded up

    generate_sine_wave(frequency);

    drv_init();
    drv_enable_digital();
    xTaskCreatePinnedToCore(writer_task, "writer_task", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL, 0);

    xTaskCreate(spi_task, "spi_task", 4096, NULL, 10, NULL);
    

}
