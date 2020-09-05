#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <strings.h>

#define DHT11_POWER 2
#define DHT11_DATA  4

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}

int wait_status(_Bool status) {
    int count = 0;

    while (gpio_get_level(DHT11_DATA) == status) {
        count++;
        ets_delay_us(1);
    }
    return count;
}

void preparing_for_receiving_data() {
    check(gpio_set_direction(DHT11_DATA,  GPIO_MODE_OUTPUT), "gpio_set_direction_gpio_1");
    check(gpio_set_level(DHT11_DATA, 1), "gpio_set_level_gpio_1");
    ets_delay_us(1500 * 1000);
    check(gpio_set_level(DHT11_DATA, 0), "gpio_set_level_gpio_2");
    ets_delay_us(18000);
    check(gpio_set_level(DHT11_DATA, 1), "gpio_set_level_gpio_3");
    ets_delay_us(30);
    check(gpio_set_direction(DHT11_DATA, GPIO_MODE_INPUT), "gpio_set_direction_gpio_2");

    wait_status(0);
    wait_status(1);
}

void weather(void *pvParameters) {
    int res = 0;
    uint8_t data[5];

    while(true) {
        bzero(&data, sizeof(data));
        preparing_for_receiving_data();
        
        for (int i = 1, j = 0; i < 41; i++) {
            wait_status(0);
            res = wait_status(1);
            if (res > 28) {
                data[j] <<= 1;
                data[j] += 1;
            }
            else {
                data[j] <<= 1;
            }
            if (i % 8 == 0) {
                j++;
            }
        }
        if (data[0] + data[1] + data[2] + data[3] != data[4]) {
            printf("Invalid sum\n");
        }
        printf("Temperature: %d\n", data[2]);
        printf("Humidity: %d\n", data[0]);
        vTaskDelay(5);
    }
    vTaskDelete(NULL);
}

void app_main() {
    check(gpio_set_direction(DHT11_POWER, GPIO_MODE_OUTPUT), "gpio_set_direction_power");
    check(gpio_set_level(DHT11_POWER, 1), "gpio_set_level_power");

    xTaskCreate(weather, "weather", 2048u, NULL, 1, 0);
}