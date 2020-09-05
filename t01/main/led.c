#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define LED1 26
#define LED2 27
#define LED3 33

#define HIGH 1
#define LOW 0

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}

void turn_led(void *pvParameters) {
    check(gpio_set_direction(LED1, GPIO_MODE_OUTPUT), "set_dirrection_led1");
    check(gpio_set_direction(LED2, GPIO_MODE_OUTPUT), "set_dirrection_led2");
    check(gpio_set_direction(LED3, GPIO_MODE_OUTPUT), "set_dirrection_led3");

    check(gpio_set_level(LED1, HIGH), "set_level_led1");
    check(gpio_set_level(LED2, HIGH), "set_level_led2");
    check(gpio_set_level(LED3, HIGH), "set_level_led3");
}

void app_main() {
    xTaskCreate(turn_led, "turn leds on", 2048u, NULL, 1, 0);
}