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
#include "font6x8.h"
#include "driver/i2c.h"

#define GPIO_POWER 2
#define GPIO_DATA 4

#define WAITNULL() while(gpio_get_level(GPIO_DATA) == 1) {ets_delay_us(1);}
#define WAITONE() while(gpio_get_level(GPIO_DATA) == 0) {ets_delay_us(1);}
#define CONVERT(a, b, c) for(int i = 0; i < 8; i++) {(c) += (a[(b) * 8 + i]) * pow(2, 7 - (i));}

#define GPIO_SDA GPIO_NUM_21
#define GPIO_SCL GPIO_NUM_22
#define SH1106_ADDR 0x3C            // Deafault sh1106  address
#define SH1106_PORT I2C_NUM_0

typedef struct {
    uint8_t addr;
    i2c_port_t port;
    uint8_t grid[16][128];          // Pixesl grid (16 * byte(8 bit)) * 128
    uint16_t changes;
} sh1106_t;

void init_i2c() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_SDA,
        .scl_io_num = GPIO_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000
    };
    i2c_param_config(SH1106_PORT, &i2c_config);
    i2c_driver_install(SH1106_PORT, I2C_MODE_MASTER, 0, 0, 0);
}

void init_sh1106(sh1106_t *display) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);                   // addind start 3 bits 010
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);  // Slave address adding
    i2c_master_write_byte(cmd, 0x00, true); // command stream
    i2c_master_write_byte(cmd, 0xAE, true); // off
    i2c_master_write_byte(cmd, 0xD5, true); // clock div
    i2c_master_write_byte(cmd, 0x80, true);
    i2c_master_write_byte(cmd, 0xA8, true); // multiplex
    i2c_master_write_byte(cmd, 0xFF, true);
    i2c_master_write_byte(cmd, 0x8D, true); // charge pump
    i2c_master_write_byte(cmd, 0x14, true);
    i2c_master_write_byte(cmd, 0x10, true); // high column
    i2c_master_write_byte(cmd, 0xB0, true);
    i2c_master_write_byte(cmd, 0xC8, true);
    i2c_master_write_byte(cmd, 0x00, true); // low column
    i2c_master_write_byte(cmd, 0x10, true);
    i2c_master_write_byte(cmd, 0x40, true);
    i2c_master_write_byte(cmd, 0xA1, true); // segment remap
    i2c_master_write_byte(cmd, 0xA6, true);
    i2c_master_write_byte(cmd, 0x81, true); // contrast
    i2c_master_write_byte(cmd, 0xFF, true);
    i2c_master_write_byte(cmd, 0xAF, true); // on
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void set_pixel_sh1106(sh1106_t *display, uint8_t x, uint8_t y, bool pixel_status) {
    uint8_t page = y / 8;

    if(pixel_status == true) {
        display->grid[page][x] |= (1 << (y % 8));
    }
    else {
        display->grid[page][x] &= ~(1 << (y % 8));
    }
    display->changes |= (1 << page);
}

void print_page(sh1106_t *display, uint8_t page) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x80, true);                // single command
    i2c_master_write_byte(cmd, 0xB0 + page, true);         // Goes to our page
    i2c_master_write_byte(cmd, 0x40, true);                // set display start line
    i2c_master_write(cmd, display->grid[page], 128, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void refresh_sh1106(sh1106_t *display) {
    for(uint8_t y_page = 0; y_page < 16; y_page++) {
        if (display->changes & (1 << y_page)) {
            print_page(display, y_page);
        }       
    }
    display->changes = 0x0000;
}

uint8_t dewrite_byte(uint8_t num, bool up) {
    uint8_t result = 0x00;
    int byte_change = 0;
    uint8_t test;

    if(up == false) {
        byte_change = 3;
    }
    for(int i = 0; i < 8; i++) {
        test = 0x00;
        if(1 & (num >> byte_change)) {
            test += pow(2, i);
        }
        result |= test;
        if(i % 2) {
            byte_change++;
        }
    }
    return result;
}

void print_char(char ch, sh1106_t *display, uint8_t x_str, int size, int page) {
    int index = (ch - 32) * 6;
    uint8_t y_str = 0;

    while(x_str > 128) {
        x_str -= 128;
        y_str++;
    }
    if(size == 1) {
        for(uint8_t x = 0; x < 6; x++) {
            display->grid[page + y_str][x + x_str * 6] |=  (font6x8[index + x]);
        }
    }
    if(size == 2) {
        for(uint8_t x = 0; x < 12; x++) {
            display->grid[page + y_str][x + x_str * 12] |=  dewrite_byte(font6x8[index + x / 2], true);
            display->grid[page + y_str + 1][x + x_str * 12] |=  dewrite_byte(font6x8[index + x / 2], false);
        }
    }
}

void print_string(char *str, sh1106_t *display, int size, int page) {
    int len = strlen(str);

    for(uint8_t i = 0; i < len; i++) {
        print_char(str[i], display, i, size, page);
    }
}
void app_main() {
    gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32, 1);
    init_i2c();
    sh1106_t display;
    display.addr = SH1106_ADDR;
    display.port = SH1106_PORT;
    init_sh1106(&display);

    // gpio_set_direction();  // NC, not connected
    // gpio_set_direction();  // GND, ground arduino

    /* DATA is used for communication between the microprocessor 
       and DHT11 and synchronization, single-bus data format, a 
       40-bit data transfer, high first-out. */

    /* 0011 0101       0000 0000        0001 1000          0000 0000        0100 1101 
       ---------   |   ---------   |    ---------     |    ---------    |   ---------
     High humidity |  Low humidity | High temperature | Low temperature |  Parity bit */


    int data[40];
    int num1 = 0;
    int num2 = 0;
    int num3 = 0;
    int num4 = 0;
    int num5 = 0;

    // do {
    while(1) {
        num1 = 0;
        num2 = 0;
        num3 = 0;
        num4 = 0;
        num5 = 0;
        gpio_set_direction(GPIO_POWER, GPIO_MODE_OUTPUT);       // VCC power
        gpio_set_direction(GPIO_DATA, GPIO_MODE_OUTPUT);        // Data

        gpio_set_level(GPIO_POWER, 1);
        gpio_set_level(GPIO_DATA, 1);

        ets_delay_us(2500 * 1000);                         // wait 2.5 seconds to cross the unstable state

        gpio_set_level(GPIO_DATA, 0);
        ets_delay_us(20 * 1000);                           // should be >= 18 ms
        gpio_set_level(GPIO_DATA, 1);
        ets_delay_us(40);
        gpio_set_direction(GPIO_DATA, GPIO_MODE_INPUT);

        int x = gpio_get_level(GPIO_DATA);
        printf("Answear, about getting info - %d\n", x);
        WAITONE();
        x = gpio_get_level(GPIO_DATA);
        printf("Answear, about getting info - %d\n", x);
        WAITNULL();
        
        for(int i = 0; i < 35; i++) {
            WAITONE();
            ets_delay_us(30);
            if(gpio_get_level(GPIO_DATA)) {
                data[i] = 1;
                WAITNULL();
            }
            else if(!gpio_get_level(GPIO_DATA)) {
                data[i] = 0;
            }
            // printf("%d\n", data[i]);
        }

        CONVERT(data, 0, num1);
        CONVERT(data, 1, num2);
        CONVERT(data, 2, num3);
        CONVERT(data, 3, num4);
        CONVERT(data, 4, num5);

        printf("First number - %d\n", num1);
        printf("Second number - %d\n", num2);
        printf("Third number - %d\n", num3);
        printf("Fourth number - %d\n", num4);

        for(uint8_t y = 0; y < 128; y++) {
            for(uint8_t x = 0; x < 128; x++) {
                set_pixel_sh1106(&display, x, y, 0);
            }
        }

        print_string("temperature", &display, 1, 0);
        char temperature_num[30];
        bzero(&temperature_num, '\0');
        sprintf(temperature_num, "+%dC", num3);
        print_string(temperature_num, &display, 2, 2);
        print_string("humidity", &display, 1, 4);
        char humidity_num[30];
        bzero(&humidity_num, '\0');
        sprintf(humidity_num, "%d per", num1);
        print_string(humidity_num, &display, 2, 6);
        refresh_sh1106(&display);
    }
}
