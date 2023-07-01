#include "mySetup.h"
#include "countingAlgo.h"
#include "ssd1306.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

void app_main(void)
{
    //** debugging levels
    esp_log_level_set("*", ESP_LOG_ERROR);
    // esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ERROR", ESP_LOG_INFO);
    esp_log_level_set("BUG", ESP_LOG_INFO);
    esp_log_level_set("MQTT", ESP_LOG_INFO);
    esp_log_level_set("MQTT_SEND", ESP_LOG_INFO);
    esp_log_level_set("PROGRESS", ESP_LOG_INFO);
    esp_log_level_set("NVS", ESP_LOG_INFO);
    // esp_log_level_set("pushInBuffer()", ESP_LOG_INFO);
    // esp_log_level_set("analyzer()", ESP_LOG_INFO);
    // esp_log_level_set("platform_api", ESP_LOG_INFO);

    my_setup();

    start_counting_algo();
    ESP_LOGI("PROGRESS", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
}
