#include <stdio.h>
#include "driver/gpio.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include <math.h>
#include "string.h"
#include "driver/i2c.h"
#include <stdlib.h>
#include "driver/spi_master.h"
#include "peripherals.h"
#include "register_map.h"

#define GPIO_LED1 26
#define GPIO_LED2 27
#define GPIO_LED3 33

#define EN_ACCEL 23

#define PIN_MISO 12
#define PIN_MOSI 13
#define PIN_CLK 14
#define PIN_CS 15

#define BW_RATE 0x2Du          // Data rate and power mode control
#define POWER_CTL 0x2Du        // Power-saving features control
#define LOW_POWER 0x08u        // 4-th bit gow low poerw

/** @brief ADXL345 register read flag. */
#define ADXL345_REG_READ_FLAG 0x80u
/** @brief ADXL345 register multibyte flag. */
#define ADXL345_REG_MB_FLAG 0x40u
/** @brief ADXL345 register: DATAX0. */
#define ADXL345_REG_DATAX0 0x32u

void oled(char *string, char *string2, char *string3);
