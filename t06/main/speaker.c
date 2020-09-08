#include <stdio.h>
#include "driver/gpio.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/dac.h"
#include "esp_spi_flash.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include <math.h>
#include "string.h"
#include "driver/i2c.h"

void beep_sound(void *pvParameters) {
    for(int i = 0; true; i-=-1) {
        for (int j = 0; j < 256; j++) {
            dac_output_voltage(DAC_CHANNEL_1, j);
            ets_delay_us(1000);
        }
        ets_delay_us(1000 * 1000);
    }
    vTaskDelete(NULL);
}

void app_main() {
	dac_output_enable(DAC_CHANNEL_1);
    xTaskCreate(beep_sound, "beep_sound", 2048u, NULL, 5, 0);
}
