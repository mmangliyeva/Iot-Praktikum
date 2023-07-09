#include "mySetup.h"
#include "ssd1306.h"
#include "wifi.h"
#include "timeMgmt.h"
#include "mqtt.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "nvs.h"
#include "esp_timer.h"
#include "webFunctions.h"

#ifdef WITH_DISPLAY
void initDisplay(void);
#endif
void initPins(void);
void replacedSpaces(char *str); // replaces space with '_'

uint8_t testModeActive = 0;
TaskHandle_t xProgAnalizer = NULL;
TaskHandle_t xProgShowCount = NULL;
TaskHandle_t xProgSendToDB = NULL;
TaskHandle_t xProgInBuffer = NULL;
TaskHandle_t xSendToMQTT = NULL;
TaskHandle_t xOTA = NULL;
uint8_t flag_internet_active = 0;
char *tmp_message = NULL; // meassage for function: error_meassage()

void my_setup(void)
{

#ifdef WITH_DISPLAY
	initDisplay(); // int external display
#endif
	initMY_nvs();

	initWifi();
	initSNTP(); // init correct time
	initMQTT(); // init service to send data to elastic search

	initPins();

#ifdef WITH_DISPLAY
	ssd1306_clearScreen();

	displayCountPreTime(0, 0);
#endif
	// init service to send/fetch data to the IoT platform (HTTP requests)
	init_web_functions();
}

void initPins(void)
{
	ESP_LOGI("PROGRESS", "Initializing pins");

	// setup pins:
	gpio_install_isr_service(0);

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

	// int pin for entering test mode
	gpio_pad_select_gpio(PIN_TEST_MODE);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_TEST_MODE, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_TEST_MODE));
	gpio_set_intr_type(PIN_TEST_MODE, GPIO_INTR_POSEDGE);
	// internal red LED
	ESP_ERROR_CHECK(gpio_set_direction(RED_INTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
}

/**
 * Sends an error message to the IoT-platfrom with HTTP
 */
void error_message(const char *TAG, char *msg, const char *details)
{
	char *date = getDate();
	asprintf(&tmp_message, "%s_%s_%s", date, msg, details);
	ESP_LOGE(TAG, "%s", tmp_message);
	replacedSpaces(tmp_message);
	// printf("send message: %s", tmp_message);
	if (flag_internet_active)
		systemReport(tmp_message);
	free(tmp_message);
	// free(date);
}
/**
 * Replaces whitespaces with '_'
 * Needed because there should be spaces in http-request
 */
void replacedSpaces(char *str)
{
	int i = 0;
	while (str[i])
	{
		if (isspace(str[i]))
			str[i] = '_';
		i++;
	}
}

#ifdef WITH_DISPLAY
/**
 * init the external hardware display
 */
void initDisplay(void)
{
	ESP_LOGI("PROGRESS", "Initializing display");

	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();
	ssd1306_printFixedN(0, 0, "BOOT", STYLE_BOLD, 2);
}

#endif
/**
 * updates the count or prediction on the external hardware display
 */
void displayCountPreTime(uint8_t prediction, uint8_t curCount)
{
#ifdef WITH_DISPLAY
	char count_str[BUFF_STRING_COUNT];
	char prediction_str[BUFF_STRING_COUNT];
	char time_str[10];
	time_t now;
	struct tm *now_tm;
	sprintf(count_str, "%02d", curCount);
	sprintf(prediction_str, "%02d", prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str, "%02d:%02d", now_tm->tm_hour, now_tm->tm_min);

	ssd1306_printFixedN(0, 0, "G8", STYLE_NORMAL, 1);
	ssd1306_printFixedN(64, 0, time_str, STYLE_NORMAL, 1);
	ssd1306_printFixedN(0, Y_POS_COUNT, count_str, STYLE_BOLD, 2);
	ssd1306_printFixedN(96, Y_POS_COUNT, prediction_str, STYLE_BOLD, 1);
#endif
	// free(now_tm);
}