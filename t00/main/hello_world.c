#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void print_hello(void *pvParameters) {
    while(true) {
        printf("Hello world\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    xTaskCreate(print_hello, "hello_world", 2048u, NULL, 5, 0);
}
