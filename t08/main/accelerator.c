#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "peripherals.h"
#include "register_map.h"

#define UPDATE_DELAY (200u / portTICK_PERIOD_MS)

static void gpio_set_dirrection_wrapper(int gpio, gpio_mode_t mode) {
	esp_err_t exit = gpio_set_direction(gpio, mode);

	if(exit != ESP_OK) {
    	printf("gpio_set_dirrection: error");

    }
}

void trans_packet(spi_device_handle_t spi, uint8_t address, uint8_t data) {
	esp_err_t exit;

	spi_transaction_t packet = {
		.flags     = SPI_TRANS_USE_RXDATA,
		.cmd       = address,
		.tx_buffer = &data,
		.length    = 8       // in bits
	};
	exit = spi_device_polling_transmit(spi, &packet);
	// TODO : error handle function
}

void adx1345_start(spi_device_handle_t spi) {
	trans_packet(spi, BW_RATE, LOW_POWER);  // sends command and wait for delay
	vTaskDelay(UPDATE_DELAY);
}

void adx1345_read(spi_device_handle_t spi, int16_t *accs) {
    esp_err_t exit;
    uint8_t tx_buffer[3u * sizeof(uint16_t)]; // dummy buffer
    spi_transaction_t packet = {
        .tx_buffer = tx_buffer,
    	.cmd       = ADXL345_REG_READ_FLAG |
                   ADXL345_REG_MB_FLAG |
                   ADXL345_REG_DATAX0,
        .length    = sizeof(tx_buffer) * 8,   // in bits
    	.rx_buffer = accs
    };
    exit = spi_device_polling_transmit(spi, &packet);
    // TODO : error handle function
}

int module(int num) {
    if(num < 0) {
        return -1 * num;
    }
    return num;
}

int found_max(int16_t *arr) {
    if((arr[0] > arr[1]) && (arr[0] > arr[2])) {
        return 0;
    }
    else if((arr[1] > arr[0]) && (arr[1] > arr[2])) {
        return 1;
    }
    else {
        return 2;
    }
}

void adx1345_read_task(void *pvParameters) {
    int16_t accs[3];
    spi_device_handle_t spi = (spi_device_handle_t)pvParameters;
    adx1345_read(spi, accs);

    accs[0] = module(accs[0]);
    accs[1] = module(accs[1]);
    accs[2] = module(accs[2]);

    int x_start = accs[0];
    int y_start = accs[1];
    int z_start = accs[2];
    int horizontal_status = found_max(spi);

    while(true) {
        adx1345_read(spi, accs);
        // printf("\nx = %d\ny = %d\nz = %d\n", (int)accs[0], (int)accs[1], (int)accs[2]);
        // printf("\nx start = %d\ny start = %d\nz start = %d\n", x_start, y_start, z_start);
        int x = module(accs[0]);
        int y = module(accs[1]);
        int z = module(accs[2]);

        if ((module(accs[horizontal_status]) < 100) && ((x + y + z) <= (x_start + y_start + z_start) + 100)) {
	 		gpio_set_level(GPIO_LED1, 1);
            gpio_set_level(GPIO_LED2, 1);
            gpio_set_level(GPIO_LED2, 1);
 		}
        else {
            gpio_set_level(GPIO_LED1, 0);
            gpio_set_level(GPIO_LED2, 0);
            gpio_set_level(GPIO_LED3, 0);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main() {
	esp_err_t exit;

    gpio_set_dirrection_wrapper(GPIO_LED1, GPIO_MODE_OUTPUT);
    gpio_set_dirrection_wrapper(GPIO_LED2, GPIO_MODE_OUTPUT);
    gpio_set_dirrection_wrapper(GPIO_LED3, GPIO_MODE_OUTPUT);

    gpio_set_dirrection_wrapper(EN_ACCEL, GPIO_MODE_OUTPUT); // VCC for accelerator
    gpio_set_level(EN_ACCEL, 1);

    spi_device_handle_t spi;

    spi_bus_config_t bus_config = {
    	.miso_io_num   = PIN_MISO,
    	.mosi_io_num   = PIN_MOSI,
    	.sclk_io_num   = PIN_CLK,
    	.quadwp_io_num = -1,        // don't use
    	.quadhd_io_num = -1         // don't use
    };

    spi_device_interface_config_t device_config = {
    	.clock_speed_hz = 1000000,  // Themaximum  SPI clock speed is 5 MHz
    	.mode           = 3,        // CPOL = 1, CPHA = 1
    	// Data is sampled on the falling edge of the synchronization signal
    	.spics_io_num   = PIN_CS,
    	.command_bits   = 8,        // ADXL always takes 1+7 bit command (address)
    	.queue_size     = 1
    };
    
    // set config on first spi
    exit = spi_bus_initialize(VSPI_HOST, &bus_config, 0);
    // TODO : error handle function
    // add device to bus
    exit = spi_bus_add_device(VSPI_HOST, &device_config, &spi);
    // TODO : error handle function
    // start command
    adx1345_start(spi);

    xTaskCreate(adx1345_read_task, "acceleration", 2048u, (void*)spi, 5, 0);
}
