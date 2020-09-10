#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "font6x8.h"
#include <math.h>
#include "esp_log.h"
#include "driver/dac.h"
#include <pthread.h>

typedef struct {
    uint8_t addr;
    i2c_port_t port;
    uint8_t grid[8][128];          // Pixesl grid (16 * byte(8 bit)) * 128
    uint16_t changes;
} sh1106_t;

#define ESP_INTR_FLAG_DEFAULT 0

#define UPDATE_DELAY (200u / portTICK_PERIOD_MS)

#define GPIO_POWER 2
#define GPIO_DATA 4

#define SW1 39
#define SW2 18

#define GPIO_SDA GPIO_NUM_21
#define GPIO_SCL GPIO_NUM_22
#define SH1106_ADDR 0x3C            // Deafault sh1106  address
#define SH1106_PORT I2C_NUM_0

#define BW_RATE 0x2Du          // Data rate and power mode control
#define POWER_CTL 0x2Du        // Power-saving features control
#define LOW_POWER 0x08u        // 4-th bit gow low power

/** @brief ADXL345 register read flag. */
#define ADXL345_REG_READ_FLAG 0x80u
/** @brief ADXL345 register multibyte flag. */
#define ADXL345_REG_MB_FLAG 0x40u
/** @brief ADXL345 register: DATAX0. */
#define ADXL345_REG_DATAX0 0x32u

#define GPIO_LED1 26
#define GPIO_LED2 27
#define GPIO_LED3 33

#define GPIO_EN_ACCEL 23

#define PIN_MISO 12
#define PIN_MOSI 13
#define PIN_CLK 14
#define PIN_CS 15

#define OLED_ENABLE 32
#define GPIO_NUM_5 5

#define GPIO_INPUT_PIN_SEL ((1ULL<<SW1) | (1ULL<<SW2))


void refresh_sh1106(sh1106_t *display);
