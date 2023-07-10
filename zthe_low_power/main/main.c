#include "mySetup.h"
#include "countingAlgo.h"
#include "esp32/rom/rtc.h"
#include <time.h>

void app_main(void)
{
    //** debugging levels
    esp_log_level_set("*", ESP_LOG_ERROR);
    // esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_SEND", ESP_LOG_INFO);
    esp_log_level_set("PROGRESS", ESP_LOG_INFO);
    // esp_log_level_set("pushInBuffer()", ESP_LOG_INFO);
    // esp_log_level_set("analyzer()", ESP_LOG_INFO);
    // esp_log_level_set("platform_api", ESP_LOG_INFO);

    my_setup();
    if (rtc_get_reset_reason(0) == DEEPSLEEP_RESET)
    {
        // empty buffer if we woke up from deepsleep

        // ESP_LOGI("PROGRESS", "Woke up from deep sleep");
        // ESP_LOGI("PROGRESS", "fillsize: %d, head: %d", fillSize, head);
        // for (int i = 0; i < fillSize; i++)
        // {
        //     ESP_LOGI("PROGRESS", "first element buffer. ID: %d, time: %lld", buffer[i].id, (long long)buffer[i].time);
        // }

        // int i = 0;
        // i = testData_ingoing(i);
        // i = testData_ingoing(i);
        // i = testData_outgoing(i);
        // i = testData_outgoing(i);
        // i = testData_ingoing(i);
        start_counting_algo();
    }
    ESP_LOGI("PROGRESS", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
}
