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

uint8_t testModeActive = 0;
TaskHandle_t xProgAnalizer = NULL;
TaskHandle_t xProgShowCount = NULL;
TaskHandle_t xProgSendToDB = NULL;
TaskHandle_t xProgInBuffer = NULL;
TaskHandle_t xSendToMQTT = NULL;
TaskHandle_t xOTA = NULL;
void my_setup(void)
{

#ifdef WITH_DISPLAY
	initDisplay();
#endif
	initMY_nvs(); // must before wifi

	initWifi();
	initSNTP();
	initMQTT();

	initPins();

#ifdef WITH_DISPLAY
	ssd1306_clearScreen();

	displayCountPreTime(0, 0);
#endif
	init_web_functions();
}

void initPins(void)
{
	ESP_LOGI("PROGRESS", "Initializing pins");

	// setup pins:
	gpio_install_isr_service(0);

	// init light barrier 1
	gpio_pad_select_gpio(PIN_DETECT_1);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_1, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_1));
	gpio_set_intr_type(PIN_DETECT_1, GPIO_INTR_ANYEDGE);

	// init light barrier 2
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
void initDisplay(void)
{
	ESP_LOGI("PROGRESS", "Initializing display");

	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();
	ssd1306_printFixedN(0, 0, "BOOT", STYLE_BOLD, 2);
}

void displayCountPreTime(uint8_t prediction, uint8_t curCount)
{
	char count_str[BUFF_STRING_COUNT];
	char prediction_str[BUFF_STRING_COUNT];
	char time_str[10];
	char *tmp_g8 = "G8";
	time_t now;
	struct tm *now_tm;
	sprintf(count_str, "%02d", curCount);
	sprintf(prediction_str, "%02d", prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str, "%02d:%02d", now_tm->tm_hour, now_tm->tm_min);

	ssd1306_printFixedN(0, 0, tmp_g8, STYLE_NORMAL, 1);
	ssd1306_printFixedN(64, 0, time_str, STYLE_NORMAL, 1);
	ssd1306_printFixedN(0, Y_POS_COUNT, count_str, STYLE_BOLD, 2);
	ssd1306_printFixedN(96, Y_POS_COUNT, prediction_str, STYLE_BOLD, 1);
	// free(count_str);
	// free(prediction_str);
	// free(prediction_str);
}

#endif