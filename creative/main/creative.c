#include "creative.h"

static xQueueHandle gpio_evt_queue = NULL;
static int screen = 0;
static int position = 0;
static int humdt = 0;
static int screen_status = 0;
static int past_screen_status = 0;
static int tempr = 0;

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

void reverse(sh1106_t *display) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);                   // addind start 3 bits 010
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);  // Slave address adding

    if(position > 150)
        screen_status = 0;
    else if(position < -150)
        screen_status = 1;
    if(position > 150 && screen_status == past_screen_status) {
        i2c_master_write_byte(cmd, 0x00, true);
        i2c_master_write_byte(cmd, 0xAE, true);
        // i2c_master_write_byte(cmd, 0x80, true);
        i2c_master_write_byte(cmd, 0xC0, true); 
        i2c_master_write_byte(cmd, 0xA0, true);
        i2c_master_write_byte(cmd, 0xAF, true);
        past_screen_status = 1;
    }
    else if(position < -150 && screen_status == past_screen_status) {
        i2c_master_write_byte(cmd, 0x00, true);
        i2c_master_write_byte(cmd, 0xAE, true);
        // i2c_master_write_byte(cmd, 0x80, true);
        i2c_master_write_byte(cmd, 0xC8, true); 
        i2c_master_write_byte(cmd, 0xA1, true);
        i2c_master_write_byte(cmd, 0xAF, true);
        past_screen_status = 0;
    }
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
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
    reverse(display);
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


void oled_print(char *string, int column, int size, sh1106_t *display) {
    int len = strlen(string);

    for(uint8_t i = 0; i < len; i++) {
        print_char(string[i], display, i, size, column);
    }
}

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

int preparing_for_receiving_data(int gpio) {
    gpio_set_direction_wrapper(gpio,  GPIO_MODE_OUTPUT);
    gpio_set_level_wrapper(gpio,  1);
    vTaskDelay(2000u / portTICK_PERIOD_MS);
    // ets_delay_us(1500 * 1000);
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

void communicate() {
    int res = 0;
    uint8_t data[5];

    gpio_set_direction_wrapper(GPIO_POWER, GPIO_MODE_OUTPUT);
    gpio_set_level_wrapper(GPIO_POWER, 1);

    bzero(&data, sizeof(data));
    preparing_for_receiving_data(GPIO_DATA);
        
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
    humdt = data[0];
    tempr = data[2];
}

void dht11_reader(void *pvParameters) {
    while(true) {
        communicate();
        vTaskDelay(100);
    }
}

void check(esp_err_t exit_num, char *log_msg) {
    if(exit_num != ESP_OK) {
        printf("%s -- with exit code %d\n", log_msg, exit_num);
        exit(1);
    }
}
void oled_clear(sh1106_t *display) {
    for(uint8_t y = 0; y < 64; y++) {
        for(uint8_t x = 0; x < 128; x++) {
            set_pixel_sh1106(display, x, y, 0);
        }
    }
}

void windows_manager(void *pvParameters) {
    sh1106_t display;
    display.addr = SH1106_ADDR;
    display.port = SH1106_PORT;
    init_sh1106(&display);

    while(true) {
        oled_clear(&display);
        if(screen == 1) {
            char value[5];
            bzero(&value, 5);

            sprintf(&value, "%d C", tempr);
            oled_print("temerature", 1, 1, &display);
            oled_print(value, 3, 2, &display);
        }
        else if(screen == 0) {
            char value[5];
            bzero(&value, 5);

            sprintf(&value, "%d %%", humdt);
            oled_print("humidity", 1, 1, &display);
            oled_print(value, 3, 2, &display);
        }
        refresh_sh1106(&display);
        ets_delay_us(10);
    }
}

void beep_sound() {
    dac_output_enable(DAC_CHANNEL_1);
    for(int i = 0; i < 50; i++) {
        for (int j = 100; j < 200; j++) {
            dac_output_voltage(DAC_CHANNEL_1, j);
            ets_delay_us(4);
        }
    }
    vTaskDelete(NULL);
}

static void switch_button_handler(void* arg) {
    uint32_t io_num;

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            gpio_intr_disable(SW1);
            gpio_intr_disable(SW2);
            if (io_num == SW1) {
                if (screen == 0)
                    screen += 1;
                xTaskCreate(beep_sound, "beep_sound", 2048u, NULL, 5, 0);
            }
            else if (io_num == SW2) {
                if (screen == 1)
                    screen -= 1;
                xTaskCreate(beep_sound, "beep_sound", 2048u, NULL, 5, 0);
            }
            gpio_intr_enable(SW1);
            gpio_intr_enable(SW2);
        }
    }
}

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;

    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void trans_packet(spi_device_handle_t spi, uint8_t address, uint8_t data) {
    spi_transaction_t packet = {
        .flags     = SPI_TRANS_USE_RXDATA,
        .cmd       = address,
        .tx_buffer = &data,
        .length    = 8       // in bits
    };
    spi_device_polling_transmit(spi, &packet);
    // TODO : error handle function
}

void adx1345_start(spi_device_handle_t spi) {
    trans_packet(spi, BW_RATE, LOW_POWER);  // sends command and wait for delay
    vTaskDelay(UPDATE_DELAY);
}

void adx1345_read(spi_device_handle_t spi, int16_t *accs) {
    uint8_t tx_buffer[3u * sizeof(uint16_t)]; // dummy buffer
    spi_transaction_t packet = {
        .tx_buffer = tx_buffer,
        .cmd       = ADXL345_REG_READ_FLAG |
                   ADXL345_REG_MB_FLAG |
                   ADXL345_REG_DATAX0,
        .length    = sizeof(tx_buffer) * 8,   // in bits
        .rx_buffer = accs
    };
    spi_device_polling_transmit(spi, &packet);
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

    while(true) {
        adx1345_read(spi, accs);
        printf("\nx = %d\ny = %d\nz = %d\n", (int)accs[0], (int)accs[1], (int)accs[2]);
        position = (int)accs[1];
        ets_delay_us(10);
    }
}

void app_main() {
    init_i2c();
    gpio_set_direction(OLED_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_EN_ACCEL, GPIO_MODE_INPUT);
    dac_output_enable(DAC_CHANNEL_1);

    gpio_set_level(OLED_ENABLE, 1);

    spi_device_handle_t spi;

    spi_bus_config_t bus_config = {
        .miso_io_num   = PIN_MISO,
        .mosi_io_num   = PIN_MOSI,
        .sclk_io_num   = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 1000000,
        .mode           = 3,
        .spics_io_num   = PIN_CS,
        .command_bits   = 8,
        .queue_size     = 1
    };

    spi_bus_initialize(VSPI_HOST, &bus_config, 0);
    spi_bus_add_device(VSPI_HOST, &device_config, &spi);

    adx1345_start(spi);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(switch_button_handler, "switch_button_handler", 4048, NULL, 10, NULL);
    xTaskCreate(dht11_reader, "dht11_reader", 4048, NULL, 10, NULL);
    xTaskCreate(windows_manager, "windows_manager", 4048, NULL, 5, NULL);
    xTaskCreate(adx1345_read_task, "adx1345_read_task", 4048, (void*)spi, 6, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(SW1, gpio_isr_handler, (void*) SW1);
    gpio_isr_handler_add(SW2, gpio_isr_handler, (void*) SW2);
}
