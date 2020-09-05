#include "creative.h"

static void set_pixel_sh1106(sh1106_t *display, uint8_t x, uint8_t y, bool pixel_status) {
    uint8_t page = y / 8;

    if(pixel_status == true) {
        display->grid[page][x] |= (1 << (y % 8));
    }
    else {
        display->grid[page][x] &= ~(1 << (y % 8));
    }
    display->changes |= (1 << page);
}

static uint8_t dewrite_byte(uint8_t num, bool up) {
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

static void print_char(char ch, sh1106_t *display, uint8_t x_str, int size, int page) {
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

void oled_print_hum(char *string, int column, int size) {
	int len = strlen(string);
    char *temp = "humidity";
    int len1 = strlen(temp);
	sh1106_t display;
    display.addr = SH1106_ADDR;
    display.port = SH1106_PORT;
    init_sh1106(&display);


    for(uint8_t y = 0; y < 128; y++) {
        for(uint8_t x = 0; x < 128; x++) {
            set_pixel_sh1106(&display, x, y, 0);
        }
    }
    for(uint8_t i = 0; i < len1; i++) {
        print_char(temp[i], &display, i, 1, 1);
    }
    for(uint8_t i = 0; i < len; i++) {
        print_char(string[i], &display, i, size, column);
    }
    refresh_sh1106(&display, false);
}
