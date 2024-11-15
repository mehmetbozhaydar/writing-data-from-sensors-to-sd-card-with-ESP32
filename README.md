
# Example of writing data from sensors to sd card with ESP32

**Components used;**

ds3231, sht3x and sd card adapter.

**The code is designed to perform the following tasks:**

*  Initialize I2C Communication: The program initializes I2C communication for two sensors: the SHT3x temperature and humidity sensor and the DS3231 Real-Time Clock (RTC).

*   Read Time and Sensor Data: The DS3231 RTC is configured with an initial time and is read continuously to obtain the current time. The SHT3x sensor measures the temperature and humidity at regular intervals.

*   Write Data to SD Card: The program mounts an SD card and opens a file (data.txt). Every second, the current time from the DS3231 and the temperature and humidity readings from the SHT3x are written to the file in CSV format.

*   SPI and SD Card Handling: The code configures the SPI bus to communicate with the SD card, ensuring data is saved properly to the file. The SD card is mounted at the start and unmounted at the end.

*   Continuous Loop: The program runs in a continuous loop where it updates the file every second with the current time and sensor data. This loop also ensures that any errors in reading the sensors or writing to the file do not crash the system and are logged.

*  Freeing Resources: The program ensures that the SD card is unmounted and the SPI bus is freed when the application ends or after operations are completed.


### Pin assignments

The GPIO pin numbers used to connect an SD card can be customized. This can be done in two ways:

1. Using menuconfig: Run `idf.py menuconfig` in the project directory and open "SD SPI Example Configuration" menu.
2. In the source code: See the initialization of ``spi_bus_config_t`` and ``sdspi_device_config_t`` structures in the example code.

This example doesn't utilize card detect (CD) and write protect (WP) signals from SD card slot.

The table below shows the default pin assignments.

SD card pin | SPI pin | ESP32 pin     | ESP32-S2, ESP32-S3 | ESP32-P4 | ESP32-H2 | ESP32-C3 and other chips |  Notes
------------|---------|---------------|--------------------|----------|----------|--------------------------|------------
 D0         | MISO    | GPIO2         | GPIO37             | GPIO13   | GPIO0    | GPIO6                    |
 D3         | CS      | GPIO13 (MTCK) | GPIO34             | GPIO10   | GPIO1    | GPIO1                    |
 CLK        | SCK     | GPIO14 (MTMS) | GPIO36             | GPIO12   | GPIO4    | GPIO5                    |
 CMD        | MOSI    | GPIO15 (MTDO) | GPIO35             | GPIO11   | GPIO5    | GPIO4                    | 10k pullup



### Build and flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with serial port name.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


## Example output

Here is an example console output. 

```
I (288) app_start: Starting scheduler on CPU0
I (293) main_task: Started on CPU0
I (293) main_task: Calling app_main()
I (303) gpio: GPIO[1]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (423) sdspi_transaction: cmd=5, R1 response: command not supported
I (573) example: File system mounted successfully.
Name: ED2S5
Type: SDHC/SDXC
Speed: 16.00 MHz (limit: 20.00 MHz)
Size: 122240MB
CSD: ver=2, sector_size=512, capacity=250347520 read_bl_len=9
SSR: bus_width=1
I (583) example: Opening file: /sdcard/data.txt
I (623) example: File written successfully.
I (1653) example: Opening file: /sdcard/data.txt
I (1663) example: File written successfully.
I (3693) example: Opening file: /sdcard/data.txt
I (3703) example: File written successfully.
I (5733) example: Opening file: /sdcard/data.txt
I (5743) example: File written successfully.
I (7773) example: Opening file: /sdcard/data.txt
I (7783) example: File written successfully.
I (9813) example: Opening file: /sdcard/data.txt
```

## Example output

Here is an example txt file output. 

```
TIME, TEMPERATURE, HUMIDITY, DATE
12:50:11,23.04,41.81,2024-12-13
12:50:13,23.00,42.38,2024-12-13
12:50:15,23.01,42.42,2024-12-13
12:50:17,23.01,42.21,2024-12-13
12:50:19,23.01,41.97,2024-12-13
12:50:21,23.00,41.70,2024-12-13
12:50:23,23.00,41.44,2024-12-13
12:50:25,23.03,41.29,2024-12-13
12:50:27,23.01,41.19,2024-12-13
12:50:29,23.00,41.09,2024-12-13
12:50:31,23.00,41.05,2024-12-13
12:50:33,23.01,41.02,2024-12-13
12:50:35,23.01,41.00,2024-12-13
12:50:37,23.01,40.95,2024-12-13
12:50:39,23.01,40.96,2024-12-13

```