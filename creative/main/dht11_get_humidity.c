#include <stdio.h>
#include "driver/gpio.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/ledc.h"
#include "driver/timer.h"
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "esp_log.h"
#include "creative.h" 
#include "esp_log.h"

#define DHT11_POWER 2
#define DHT11_DATA  4

