#include "creative.h"

#define SW1     39
#define SW2     18

#define DHT11_POWER 2
#define DHT11_DATA  4

static int screen = 1;
static int last_screen = 2;
static int humdt = 0;
static int tempr = 0;

void gpio_set_direction_wrapper(int gpio, int mode) {
    if (gpio_set_direction(gpio, mode) != ESP_OK) {
        ESP_LOGI("gpio_set_direction: ", "%s", "some error occured.");
        exit(1);
    }
}

void gpio_set_level_wrapper(int gpio, int level) {
    if (gpio_set_level(gpio, level) != ESP_OK) {
        ESP_LOGI("gpio_set_level ", "%s", "some error occured.");
        exit(1);
    }
}

int wait_status(int time, _Bool status) {
    int count = 0;
    while (gpio_get_level(GPIO_DATA) == status){
        if (count > time)
            return -1;
        ets_delay_us(1);
        count++;
    }
    if (count == 0)
        return -1;
    return count;
}

static int preparing_for_receiving_data(int gpio) {
    gpio_set_direction_wrapper(gpio,  GPIO_MODE_OUTPUT);
    gpio_set_level_wrapper(gpio,  1);
    ets_delay_us(1500 * 1000);
    gpio_set_level_wrapper(gpio, 0);
    ets_delay_us(18000);
    gpio_set_level_wrapper(gpio, 1);
    ets_delay_us(30);
    gpio_set_direction_wrapper(gpio, GPIO_MODE_INPUT);
    if (wait_status(80, 0) == -1) {
        printf("%s\n", "Wrong response from server: must have being returning 0 during 80 microseconds");
        return -1;
    }
    if (wait_status(80, 1) == -1) {
        printf("%s\n", "Wrong response from server: must have being returning 1 during 80 microseconds");
        return -1;
    }
    return 1;
}

int communicate(int rec) {
    int res = 0;
    uint8_t data[5];

        bzero(&data, sizeof(data));
        preparing_for_receiving_data(DHT11_DATA);
        
        // Getting data.
        for (int i = 1, j = 0; i < 41; i++) {
            if ((res = wait_status(50, 0)) == -1) {
                printf("%s\n", "Error during sending data.");
            }
            if ((res = wait_status(70, 1)) == -1) {
                printf("%s\n", "Error during sending data.");
            }
            if (res > 28) {
                data[j] <<= 1;
                data[j] += 1;
            }
            else
                data[j] <<= 1;

            if (i % 8 == 0)
                j++;
        }

        if (data[0] + data[1] + data[2] + data[3] != data[4]) {
            printf("Invalid");
        }
    return data[rec];
}

void dht11_get_humidity() {
    while(true) {
        gpio_set_direction_wrapper(DHT11_POWER, GPIO_MODE_OUTPUT);
        gpio_set_level_wrapper(DHT11_POWER, 1);

        tempr = communicate(2);
        ets_delay_us(500);
    }
}

void dht11_get_temp() {
    while(true) {
        gpio_set_direction_wrapper(DHT11_POWER, GPIO_MODE_OUTPUT); // VCC power.
        gpio_set_level_wrapper(DHT11_POWER, 1);                    // switch on power.

        tempr = communicate(0);
        ets_delay_us(500);
    }
}

void first_screen() {
        printf("Yura temp = %d\n\n", tempr);
        char temp_value[6];
        
        bzero(&temp_value, 4);
        sprintf(temp_value, "%d C", tempr);
        oled_print(temp_value, 3, 2, 1);
        // ets_delay_us(100);
}

void second_screen() {
        char temp_value[6];
        
        bzero(&temp_value, 6);
        sprintf(temp_value, "%d PER", humdt);
        oled_print_hum(temp_value, 3, 2);
        // ets_delay_us(100);
}

static void play(int duration, int voltage, float frequency, int delay) {
    while (duration > 0) {
        for (float i = 0; i < voltage; i+= frequency) {
            dac_output_voltage(DAC_CHANNEL_1, i);
            ets_delay_us(delay);
        }
        duration--;
        ets_delay_us(10);
    }
    vTaskDelay(10);
}

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}

void button_one(void *pvParameters) {
    int press_count = 0;
    int led_status = 0;

    while(true) {
        if(gpio_get_level(SW1) == 1) {
            if(press_count < 40 && led_status == 0) {
                press_count++;
            }
            else if(press_count > 0 && led_status == 1) {
                press_count--;
            }
        }
        else {
            if(press_count >= 40 && led_status == 0) {
                screen = 2;
            }
            // else if(press_count <= 0 && led_status == 1) {
            //     screen = 0;
            // }
        }
    }
}

void button_two(void *pvParameters) {
    int press_count = 0;
    int led_status = 0;

    while(true) {
        if(gpio_get_level(SW2) == 1) {
            if(press_count < 40 && led_status == 0) {
                press_count++;
            }
            else if(press_count > 0 && led_status == 1) {
                press_count--;
            }
        }
        else {
            if(press_count >= 40 && led_status == 0) {
                screen = 1;
            }
            // else if(press_count <= 0 && led_status == 1) {
            //     screen = 0;
            // }
        }
    }
}

void app_main() {
    gpio_set_direction(SW1, GPIO_MODE_INPUT);
    gpio_set_direction(SW2, GPIO_MODE_INPUT);
    dac_output_enable(DAC_CHANNEL_1);

    dac_output_voltage(DAC_CHANNEL_1, 200);

    oled_init();

    xTaskCreate(button_one, "button_one", 2048u, NULL, 1, 0);
    xTaskCreate(button_two, "button_two", 2048u, NULL, 2, 0);
    xTaskCreate(dht11_get_temp, "dht11_get_temp", 2048u, NULL, 5, 0);

    while(true) {
        if(screen == last_screen) {
            if(screen == 1) {
                first_screen();
                printf("first\n");
                ets_delay_us(100);
            }
            else {
                second_screen();
                printf("second\n");
                ets_delay_us(100);
            }
            if(screen == 1) {
                last_screen = 2;
            }
            else {
                last_screen = 1;
            }
        }
        ets_delay_us(10);
    }
}
