#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
// #include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
// for potential meter
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "mySetup.h"
#include "nvs.h"


void app_main(void){
	initMY_nvs();
    initNVS_json();
    ESP_LOGI("APP","finished init");
    // writeToNVM("counter", 42, 1,112);
    // writeToNVM("counter", 69, -1,112);
    test_access();
}
