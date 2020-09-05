#include "creative.h"

#define SW1     39
#define SW2     18

void first_screen() {
        int temperature = dht11_get_temp();
        printf("Yura temp = %d\n\n", temperature);
        char temp_value[6];
        
        bzero(&temp_value, 4);
        sprintf(temp_value, "%d C", temperature);
        oled_print(temp_value, 3, 2, 1);
        // ets_delay_us(100);
}

void second_screen() {
        int humidity = dht11_get_humidity();
        char temp_value[6];
        
        bzero(&temp_value, 6);
        sprintf(temp_value, "%d PER", humidity);
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

void app_main() {
    gpio_set_direction(SW1, GPIO_MODE_INPUT);
    gpio_set_direction(SW2, GPIO_MODE_INPUT);
    dac_output_enable(DAC_CHANNEL_1);

    dac_output_voltage(DAC_CHANNEL_1, 200);

    int screen = 1;
    oled_init();

    while(1) {
        // if(gpio_get_level(SW1) == 0) {
        //     play(5, 255, 0.1, 0.5);
        //     screen = 1;
        // }
        // if(gpio_get_level(SW2) == 0) {
        //     screen = 2;
        //     play(5, 254, 0.1, 0.5);
        // }
        // if(screen == 1) {
        //     first_screen();
        // }
        // if(screen == 2) {
        //     second_screen();
        // }
        first_screen();
    }
}
