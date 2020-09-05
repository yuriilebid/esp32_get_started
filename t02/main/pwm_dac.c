#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/dac.h"

#define LED_ONE 26
#define LED_TWO 27
#define LED_THREE 33

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}

void dac_pulsing(void *pvParameters) {
    check(dac_output_enable(DAC_CHANNEL_2), "dac_output_enable");
    while(1) {
        for(int i = 0; i < 200; i++) {
            check(dac_output_voltage(DAC_CHANNEL_2, i), "dac_output_voltage");
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        for(int i = 200; i > 0; i--) {
            check(dac_output_voltage(DAC_CHANNEL_2, i), "dac_output_voltage");
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void pwm_pulsing(void *pvParameters) {
    ledc_timer_config_t timer_config;
    timer_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_config.freq_hz = 100;
    timer_config.duty_resolution = LEDC_TIMER_8_BIT;
    timer_config.timer_num = LEDC_TIMER_1;

    check(ledc_timer_config(&timer_config), "ledc_timer_config");

    ledc_channel_config_t channel_config;
    channel_config.gpio_num = LED_TWO;
    channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    channel_config.channel = LEDC_CHANNEL_1;
    channel_config.intr_type = LEDC_INTR_FADE_END;
    channel_config.timer_sel = LEDC_TIMER_1;
    channel_config.duty = 0;

    check(ledc_channel_config(&channel_config), "ledc_channel_config");
    ledc_fade_func_install(0);

    while(1) {
        check(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 200, 1000), "ledc_set_fade_with_time");
        check(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE), "ledc_fade_start");
        check(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0, 1000), "ledc_set_fade_with_time");
        check(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE), "ledc_fade_start");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    xTaskCreate(pwm_pulsing, "pwm_pulsing", 2048u, NULL, 1, NULL);
    xTaskCreate(dac_pulsing, "dac_pulsing", 2048u, NULL, 2, NULL);
}
