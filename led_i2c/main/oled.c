#include "driver/gpio.h"
#include "driver/i2c.h"
#include <stdbool.h>
#include "font6x8.h"
#include "string.h"

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
    // непонятная хуйня тут должна быть
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);  // Slave address adding
    // i2c_master_write_byte(cmd, 0xAE, true);  // off screen  // ACK bit (last argument) mean checker
    // i2c_master_write_byte(cmd, 0x10, true);  // set 4 higher bits of column address of displat RAM in register
    // i2c_master_write_byte(cmd, 0x00, true);  // set 4 lower bits of column address of displat RAM in register
    // i2c_master_write_byte(cmd, 0x81, true);  // // byte to set contrast
    // i2c_master_write_byte(cmd, 0xFF, true);  // // contrast level
    // i2c_master_write_byte(cmd, 0xA1, true);  // // segment remap
    // i2c_master_write_byte(cmd, 0xA6, true);  // // segment level
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

void print_char(char ch, sh1106_t *display, uint8_t x_str, int size) {
    int index = (ch - 32) * 6;
    uint8_t y_str = 0;

    while(x_str > 128) {
        x_str -= 128;
        y_str++;
    }
    for(uint8_t x = 0; x < 6; x++) {
	    for(uint8_t y = 0; y < 8; y++) {
            display->grid[y / 8 + y_str][x + x_str] |=  (font6x8[index + x]);
        }
    }
}

void print_string(char *str, sh1106_t *display, int size) {
	int len = strlen(str);

	for(uint8_t i = 0; i < len; i++) {
		print_char(str[i], display, i * 6, size);
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

    for(uint8_t y = 0; y < 128; y++) {
    	for(uint8_t x = 0; x < 128; x++) {
    		set_pixel_sh1106(&display, x, y, 0);
    	}
    }
    print_string("Hello world!", &display, 2);
    refresh_sh1106(&display);
}

