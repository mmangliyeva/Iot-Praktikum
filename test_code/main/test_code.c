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

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("NVS", ESP_LOG_INFO);
    esp_log_level_set("PROGRESS", ESP_LOG_INFO);

    initMY_nvs();
    initNVS_json(NO_OPEN_NVS);
    ESP_LOGI("APP", "finished init");

    uint32_t size = 20;
    while (1)
    {

        for (int i = 0; i < size; i++)
        {
            writeToNVM("counter", 42, -1, 112);
        }
        test_access();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // writeToNVM("counter", 42, 1,112);
    // writeToNVM("counter", 69, -1,112);
}
