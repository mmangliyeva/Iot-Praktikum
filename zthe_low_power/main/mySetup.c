#include "mySetup.h"
#include "wifi.h"
#include "timeMgmt.h"
#include "mqtt.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "webFunctions.h"
#include "epaperInterface.h"

#ifdef WITH_DISPLAY
void initDisplay(void);
#endif
void initPins(void);

TaskHandle_t xProgAnalizer = NULL;
TaskHandle_t xProgInBuffer = NULL;

void my_setup(void)
{
	initPins();

#ifdef WITH_DISPLAY
	initDisplay(); // int external display
#endif

	initWifi();
	initSNTP(); // init correct time
	initMQTT(); // init service to send data to elastic search

#ifdef WITH_DISPLAY
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	displayCountPreTime(0, 0);
#endif
}

void initPins(void)
{
	ESP_LOGI("PROGRESS", "Initializing pins");

	// setup pins:
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

	// init light barrier 1
	gpio_pad_select_gpio(OUTDOOR_BARRIER);
	ESP_ERROR_CHECK(gpio_set_direction(OUTDOOR_BARRIER, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(OUTDOOR_BARRIER));
	gpio_set_intr_type(OUTDOOR_BARRIER, GPIO_INTR_ANYEDGE);

	// init light barrier 2
	gpio_pad_select_gpio(INDOOR_BARRIER);
	ESP_ERROR_CHECK(gpio_set_direction(INDOOR_BARRIER, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(INDOOR_BARRIER));
	gpio_set_intr_type(INDOOR_BARRIER, GPIO_INTR_ANYEDGE);

	// display power
	ESP_ERROR_CHECK(gpio_set_direction(DISPLAY_POWER, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
}

#ifdef WITH_DISPLAY
/**
 * init the external hardware display
 */
void initDisplay(void)
{
	ESP_LOGI("PROGRESS", "Initializing display");
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 1));
	epaperInit();

	epaperShow(40, 40, "BOOT", 4);

	epaperUpdate();
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
}
/**
 * updates the count or prediction on the external hardware display
 */
void displayCountPreTime(uint8_t prediction, uint8_t curCount)
{
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 1));

	epaperClear();

	char count_str[6];
	char prediction_str[6];
	char time_str[16];
	time_t now;
	struct tm *now_tm;
	sprintf(count_str, "%02d", curCount);
	sprintf(prediction_str, "%02d", prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str, "%02d o'clock", now_tm->tm_hour);

	epaperShow(10, 0, "G8", 4);
	epaperShow(80, 5, time_str, 2);

	epaperShow(10, 100, count_str, 4);
	epaperShow(80, 100, prediction_str, 3);
	epaperUpdate();

	// free(now_tm);
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
}

#endif