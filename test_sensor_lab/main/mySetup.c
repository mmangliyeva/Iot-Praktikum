#include "mySetup.h"
#include "ssd1306.h"
#include "wifi.h"
#include "timeMgmt.h"
#include "mqtt.h"
#include "driver/gpio.h"
#include <driver/adc.h>


#ifdef WITH_DISPLAY
void initDisplay(void);
#endif

void my_setup(void){
    
    static const char *TAG = "SETUP";
    
    initWifi();
    initSNTP();
    initMQTT();
#ifdef WITH_DISPLAY
    initDisplay();
#endif


    // setup pins:
    //init light barrier 1
	gpio_pad_select_gpio(PIN_DETECT_1);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_1, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_1)); 
	gpio_set_intr_type(PIN_DETECT_1, GPIO_INTR_ANYEDGE);

	//init light barrier 2
	gpio_pad_select_gpio(PIN_DETECT_2);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_2, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_2));
	gpio_set_intr_type(PIN_DETECT_2, GPIO_INTR_ANYEDGE);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
}

#ifdef WITH_DISPLAY
void initDisplay(void){
    ESP_LOGI("PROGRESS", "Initializing display");

	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1306_clearScreen();
	ssd1306_printFixed(0,0,"Group 8",STYLE_NORMAL);
	ssd1306_printFixed(0,16,"People count:",STYLE_NORMAL);
	ssd1306_printFixedN(0,Y_POS_COUNT,"0",STYLE_BOLD,2);
}
#endif