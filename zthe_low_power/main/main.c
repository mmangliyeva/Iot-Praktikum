#include "mySetup.h"
#include "countingAlgo.h"
#include "esp32/rom/rtc.h"
#include <time.h>
#include "powerMesurment.h"
#include "esp_sleep.h"

void app_main(void)
{
    //** debugging levels
    esp_log_level_set("*", ESP_LOG_ERROR);
    // esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_SEND", ESP_LOG_INFO);
    esp_log_level_set("PROGRESS", ESP_LOG_INFO);
    esp_log_level_set("pm", ESP_LOG_INFO);
    // esp_log_level_set("pushInBuffer()", ESP_LOG_INFO);
    // esp_log_level_set("analyzer()", ESP_LOG_INFO);
    // esp_log_level_set("platform_api", ESP_LOG_INFO);
    ESP_LOGI("PROGRESS", "reason 0: %d and reason 1: %d", rtc_get_reset_reason(0), rtc_get_reset_reason(1));

    my_setup();
    // wait until wifi is ready56
    // if (xSemaphoreTake(xInternetActive, portMAX_DELAY) == pdTRUE)
    // {
    //     xSemaphoreGive(xInternetActive);
    // }
    // Measurements 2.
    // startMeasure();
    // int64_t startTime = esp_timer_get_time();
    // vTaskDelay(10000 / portTICK_PERIOD_MS);

    // Measurments 3. & 4.
    // calculation();

    // Measurment 5.
    // int sleep_sec = 10;
    // ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(1000000LL * sleep_sec));
    // esp_light_sleep_start();

    // stopMeasure();

    // printf("time: %lld\n", (esp_timer_get_time() - startTime) / 1000000);
    // vTaskDelay(8000 / portTICK_PERIOD_MS);

    if (rtc_get_reset_reason(0) == DEEPSLEEP_RESET)
    {

        // empty buffer if we woke up from deepsleep

        ESP_LOGI("PROGRESS", "Woke up from deep sleep");
        // for (int i = 0; i < fillSize; i++)
        // {
        //     ESP_LOGI("PROGRESS", "first element buffer. ID: %d, time: %lld", buffer[i].id, (long long)buffer[i].time);
        // }

        // increase the count plus 1
        int i = 0;
        i = testData_ingoing(i);
        i = testData_ingoing(i);
        i = testData_outgoing(i);
        i = testData_outgoing(i);
        i = testData_ingoing(i);
        ESP_LOGI("PROGRESS", "fillsize: %d, head: %d", fillSize, head);

        start_counting_algo();
    }
    ESP_LOGI("PROGRESS", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
}
