#include "mySetup.h"
#include "ssd1306.h"
#include "wifi.h"
#include "timeMgmt.h"
#include "mqtt.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "nvs.h"
#include "esp_timer.h"
#include "caps_ota.h"


#ifdef WITH_DISPLAY
void initDisplay(void);
#endif
void initPins(void);
void sendFromNVS(void* args);
void updateOTA(void* args);

uint8_t testModeActive = 0;
TaskHandle_t xProgAnalizer = NULL;
TaskHandle_t xProgShowCount = NULL;
TaskHandle_t xProgSendToDB = NULL;
TaskHandle_t xProgInBuffer = NULL;
TaskHandle_t xSendToMQTT = NULL;
TaskHandle_t xOTA = NULL;
void my_setup(void){

#ifdef WITH_DISPLAY
    initDisplay();
#endif
    initMY_nvs(); //must before wifi
    initWifi();
    initSNTP();
    initMQTT();

	initPins();
    ssd1306_clearScreen();

	displayCountPreTime(0,0);

	//start sending from nvs task
	xTaskCreate(sendFromNVS, "send nvs to mqtt", 2000, NULL, PRIO_SEND_NVS, &xSendToMQTT);
	// xTaskCreate(updateOTA, "update OTA", 4000, NULL, PRIO_OTA_TASK, &xOTA);

}


void updateOTA(void* args){
	while(1){
		vTaskDelay((UPDATE_OTA*1000) / portTICK_PERIOD_MS);
		if(ota_update() == ESP_OK){
			ESP_LOGI("PROGRESS","UPDATES ESP");
			
			esp_restart();
		}
		else{
			ESP_LOGI("BUG","NO ESP UPDATE");
		}

	}
}

void sendFromNVS(void* args){
	while(1){
		vTaskDelay((1000*SEND_DELAY_FROM_NVS) / portTICK_PERIOD_MS);
		sendDataFromJSON_toDB(NULL);
	}
}

void initTimer(void){
	ESP_LOGI("PROGRESS", "Initializing timer");

	// init timer for sending data to 
	const esp_timer_create_args_t my_timer_args = {
      .callback = sendDataFromJSON_toDB,
      .name = "send to mqtt timer"};
	esp_timer_handle_t timer_handler;
	ESP_ERROR_CHECK(esp_timer_create(&my_timer_args, &timer_handler));
	uint64_t microSec = 1000*1000*SEND_DELAY_FROM_NVS;// every 10 sec
	ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, microSec));	
}

void initPins(void){
	ESP_LOGI("PROGRESS", "Initializing pins");

	// setup pins:
	gpio_install_isr_service(0);

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

	// int pin for entering test mode
	gpio_pad_select_gpio(PIN_TEST_MODE);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_TEST_MODE, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_TEST_MODE));
	gpio_set_intr_type(PIN_TEST_MODE, GPIO_INTR_POSEDGE);
	// internal red LED
	ESP_ERROR_CHECK(gpio_set_direction(RED_INTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
}


#ifdef WITH_DISPLAY
void initDisplay(void){
    ESP_LOGI("PROGRESS", "Initializing display");

	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1306_clearScreen();
	ssd1306_printFixedN(0,0,"BOOT",STYLE_BOLD,2);
}


void displayCountPreTime(uint8_t prediction, uint8_t curCount){
	char count_str[BUFF_STRING_COUNT];
	char prediction_str[BUFF_STRING_COUNT];
	char time_str[10];
	time_t now;
	struct tm *now_tm;
	sprintf(count_str,"%02d",curCount);
	sprintf(prediction_str,"%02d",prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str,"%02d:%02d",now_tm->tm_hour,now_tm->tm_min);

	ssd1306_printFixedN(0,0,"G8",STYLE_NORMAL,1);
	ssd1306_printFixedN(64,0,time_str,STYLE_NORMAL,1);
	ssd1306_printFixedN(0,Y_POS_COUNT,count_str,STYLE_BOLD,2);
	ssd1306_printFixedN(96,Y_POS_COUNT,prediction_str,STYLE_BOLD,1);
}


#endif