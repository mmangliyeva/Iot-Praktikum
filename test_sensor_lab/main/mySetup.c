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
}

#ifdef WITH_DISPLAY
void initDisplay(void){
    ESP_LOGI("PROGRESS", "Initializing display");

	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
    displayCountPreTime(0,0);
}

void displayCountPreTime(uint8_t prediction, uint8_t curCount){
	char count_str[BUFF_STRING_COUNT];
	char prediction_str[BUFF_STRING_COUNT];
	char time_str[17];
	time_t now;
	struct tm *now_tm;
	sprintf(count_str,"%d",curCount);
	sprintf(prediction_str,"%d",prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str,"Group 8  %0d:%0d",now_tm->tm_hour,now_tm->tm_min);

	ssd1306_clearScreen();
	ssd1306_printFixed(0,0,time_str,STYLE_NORMAL);
	ssd1306_printFixed(0,16,"cur count | predict",STYLE_NORMAL);
	ssd1306_printFixedN(0,Y_POS_COUNT,count_str,STYLE_BOLD,2);
	ssd1306_printFixedN(64,Y_POS_COUNT,prediction_str,STYLE_BOLD,1);
}
#endif