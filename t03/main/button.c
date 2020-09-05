#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define LED1 26
#define LED2 27

#define BUTTON1 39
#define BUTTON2 18

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}

void button_one(void *pvParameters) {
    int press_count = 0;
    int led_status = 0;

    check(gpio_set_direction(BUTTON1, GPIO_MODE_INPUT), "gpio_set_dirrection_in_1");
    check(gpio_set_direction(LED1, GPIO_MODE_OUTPUT), "gpio_set_dirrection_out_1");
    while(true) {
        if(gpio_get_level(BUTTON1) == 1) {
            if(press_count < 40 && led_status == 0) {
                press_count++;
            }
            else if(press_count > 0 && led_status == 1) {
                press_count--;
            }
        }
        else {
            if(press_count >= 40 && led_status == 0) {
                led_status = 1;
            }
            else if(press_count <= 0 && led_status == 1) {
                led_status = 0;
            }
            check(gpio_set_level(LED1, led_status), "gpio_set_level_2");
        }
    }
    vTaskDelete(NULL);
}

void button_two(void *pvParameters) {
    int press_count = 0;
    int led_status = 0;

    check(gpio_set_direction(BUTTON2, GPIO_MODE_INPUT), "gpio_set_dirrection_in_2");
    check(gpio_set_direction(LED2, GPIO_MODE_OUTPUT), "gpio_set_dirrection_out_2");
    while(true) {
        if(gpio_get_level(BUTTON2) == 1) {
            if(press_count < 40 && led_status == 0) {
                press_count++;
            }
            else if(press_count > 0 && led_status == 1) {
                press_count--;
            }
        }
        else {
            if(press_count >= 40 && led_status == 0) {
                led_status = 1;
            }
            else if(press_count <= 0 && led_status == 1) {
                led_status = 0;
            }
            check(gpio_set_level(LED2, led_status), "gpio_set_level_2");
        }
    }
    vTaskDelete(NULL);
}

void app_main() {
    xTaskCreate(button_one, "button_one", 2048u, NULL, 5, 0);
    xTaskCreate(button_two, "button_two", 2048u, NULL, 5, 0);
}
