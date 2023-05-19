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


    my_setup();
    start_counting_algo();
/*
    int value=0;
    while (true){
        if (value>=10) {
            value=0;
        } else {
            value++;
        }
        char msg[256];
        int qos_test = 1;
        time_t now = 0;
	    time(&now);
        sprintf(msg, "{\"sensors\":[{\"name\": \"%s\", \"values\":[{\"timestamp\":%lld000, \"countPeople\": %d}]}]}",SENSOR_NAME,(long long)now,value);
	    ESP_LOGI("MQTT_SEND", "Topic %s: %s\n", TOPIC, msg);
	    int msg_id = esp_mqtt_client_publish(mqttClient, TOPIC, msg, strlen(msg), qos_test, 0);
    	if (msg_id==-1){
            ESP_LOGE(TAG, "msg_id returned by publish is -1!\n");
        } 
#ifdef WITH_DISPLAY
        showNumber(value);
#endif
        vTaskDelay(SEND_DELAY*1000 / portTICK_RATE_MS);
    }
    */
}
