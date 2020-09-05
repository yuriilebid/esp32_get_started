#include "creative.h"

static void print_page(sh1106_t *display, uint8_t page) {
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

void refresh_sh1106(sh1106_t *display, bool update) {
    if(update) {
        for(uint8_t y_page = 2; y_page < 10; y_page++) {
            if (display->changes & (1 << y_page)) {
                print_page(display, y_page);
            }       
        }
    }
    else {
        for(uint8_t y_page = 0; y_page < 16; y_page++) {
            if (display->changes & (1 << y_page)) {
                print_page(display, y_page);
            }       
        }
    }
    display->changes = 0x0000;
}
