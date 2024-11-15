#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <sht3x.h>
#include <string.h>
#include <esp_err.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <ds3231.h>
#include <errno.h>

#define EXAMPLE_MAX_CHAR_SIZE    256
#define FILE_NAME "data.txt"
#define MOUNT_POINT "/sdcard"

// Pin definitions (can be changed with menuconfig)
#define PIN_NUM_MISO  0
#define PIN_NUM_MOSI  5
#define PIN_NUM_CLK   4
#define PIN_NUM_CS    1

// I2C pins for SHT3x - Note! They share pins with the DS3231!
#define SDA 14
#define SCL 13

// Logging tag (for debug messages)
static const char *TAG = "example";

// Global time variable
struct tm current_time;

// File writing function
static esp_err_t s_example_write_file(const char *path, const char *data) {
    ESP_LOGI(TAG, "Opening file: %s", path);
    FILE *f = fopen(path, "a");  // Open file in append mode
    if (f == NULL) {
        ESP_LOGE(TAG, "Unable to open file: %s", strerror(errno));
        return ESP_FAIL;
    }
    int written = fprintf(f, "%s", data);
    if (written < 0) {
        ESP_LOGE(TAG, "Error writing to file: %s", strerror(errno));
        fclose(f);
        return ESP_FAIL;
    }
    fclose(f);
    ESP_LOGI(TAG, "File written successfully.");
    return ESP_OK;
}

// DS3231 RTC function
void ds3231_test(void *pvParameters) {
    i2c_dev_t dev;
    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, SDA, SCL)); // Initialize DS3231

    struct tm time = {
        .tm_year = 124, // Year since 1900
        .tm_mon  = 11,  // Month is 0-based
        .tm_mday = 13,
        .tm_hour = 12,
        .tm_min  = 50,
        .tm_sec  = 10
    };
    ESP_ERROR_CHECK(ds3231_set_time(&dev, &time)); // Set initial time to DS3231

    // Transfer time to global variable
    memcpy(&current_time, &time, sizeof(struct tm));

    // Start infinite loop
    while (1) {
        // Continuously retrieve time from DS3231
        ESP_ERROR_CHECK(ds3231_get_time(&dev, &current_time));  // Update time each loop
        // Pause briefly after retrieving time
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay
    }
}

void app_main() {
    esp_err_t ret;
    static sht3x_t dev_sht;
    
    // Initialize I2C for SHT3x
    while (i2cdev_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C could not be initialized, retrying.");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    memset(&dev_sht, 0, sizeof(sht3x_t)); // Reset SHT3x sensor configuration data
    ESP_ERROR_CHECK(sht3x_init_desc(&dev_sht, 0x44, 0, SDA, SCL)); // Initialize SHT3x sensor
    ESP_ERROR_CHECK(sht3x_init(&dev_sht)); // Start SHT3x sensor

    i2c_dev_t dev_rtc;
    ESP_ERROR_CHECK(ds3231_init_desc(&dev_rtc, 0, SCL, SDA)); // Initialize DS3231 descriptor

    // Start DS3231 task
    xTaskCreate(ds3231_test, "ds3231_test", configMINIMAL_STACK_SIZE * 3, &dev_rtc, 5, NULL);

    // SD Card Mount settings
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI bus
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    // Mount SD card file system
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "File system could not be mounted: %s", esp_err_to_name(ret));
        spi_bus_free(host.slot);
        return;
    }

    ESP_LOGI(TAG, "File system mounted successfully.");
    sdmmc_card_print_info(stdout, card);

    char full_file_path[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(full_file_path, sizeof(full_file_path), "%s/%s", MOUNT_POINT, FILE_NAME); // Create file path

    // Write header (written for the first time)
    char header[256];
    snprintf(header, sizeof(header), "TIME,TEMPERATURE,HUMIDITY,DATE\n");
    s_example_write_file(full_file_path, header);

    // Data collection loop
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay

        // Get temperature and humidity data from SHT3x
        float temperature, humidity;
        esp_err_t err_sht = sht3x_measure(&dev_sht, &temperature, &humidity);
        if (err_sht != ESP_OK) {
            ESP_LOGE(TAG, "SHT3x measurement error: %s", esp_err_to_name(err_sht));
        }

        // Write data to file
        char data[256];
        snprintf(data, sizeof(data), "%02d:%02d:%02d,%.2f,%.2f,%04d-%02d-%02d\n",
                current_time.tm_hour, current_time.tm_min, current_time.tm_sec,
                temperature, humidity, current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday);
        
        s_example_write_file(full_file_path, data);  // Write data to file

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay
    }

    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card); // Unmount SD card
    spi_bus_free(host.slot); // Free SPI bus
}
