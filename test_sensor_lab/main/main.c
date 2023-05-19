#include "mySetup.h"
#include "countingAlgo.h"

// static const char *TAG = "TEST_SENSOR";


void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_ERROR);
	//esp_log_level_set("*", ESP_LOG_INFO);       
    esp_log_level_set("ERROR", ESP_LOG_INFO);
    esp_log_level_set("MQTT", ESP_LOG_INFO);
    esp_log_level_set("MQTT_SEND", ESP_LOG_INFO);
    esp_log_level_set("PROGRESS",ESP_LOG_INFO);
    esp_log_level_set("BUG",ESP_LOG_INFO);
    // esp_log_level_set("analyzer()",ESP_LOG_INFO);
    // esp_log_level_set("pushInBuffer()",ESP_LOG_INFO);


    my_setup();
    start_counting_algo();

}
