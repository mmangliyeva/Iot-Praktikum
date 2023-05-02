#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
// for potential meter
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define PIN_SWITCH_PROGS 32
#define EXTERNAL_LED 19
#define RED_INTERNAL_LED 16
#define GREEN_INTERNAL_LED 17
#define BLUE_INTERNAL_LED 18
#define BLINK_BUTTON 33

#define BIT_WIDTH_ADC 4098


// static const char *TAG = "BLINK";
// for the interrupts
SemaphoreHandle_t xSemaphore_switchProg = NULL;

// potentialmeter
static esp_adc_cal_characteristics_t adc1_chars;


void loop(void* args);
void IRAM_ATTR isr_button(void* args);

void switchProgramms(int64_t* lastTime, int64_t* threshold, const char* tag, int* programmMode, int* programmsCount, int* blickingSpeed);

void app_main(void){
	//esp_log_level_set("BLINK", ESP_LOG_ERROR);       
	esp_log_level_set("BLINK", ESP_LOG_INFO);
// ----------------------------------------------------------------
// -------------------------set up ESP32---------------------------
// ----------------------------------------------------------------
	// set masterbotton to switch between programms:
	gpio_pad_select_gpio(PIN_SWITCH_PROGS);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_SWITCH_PROGS, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_SWITCH_PROGS));
	// set up button for blinking LED
	// gpio_pad_select_gpio(BLINK_BUTTON);
	ESP_ERROR_CHECK(gpio_set_direction(BLINK_BUTTON, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(BLINK_BUTTON));

	// set up external LED
	ESP_ERROR_CHECK(gpio_set_direction(EXTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(EXTERNAL_LED, 0));
	// set up 3 internal LEDs
	ESP_ERROR_CHECK(gpio_set_direction(RED_INTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 0));
	ESP_ERROR_CHECK(gpio_set_direction(GREEN_INTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 0));
	ESP_ERROR_CHECK(gpio_set_direction(BLUE_INTERNAL_LED, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 0));
	
	
	xSemaphore_switchProg = xSemaphoreCreateBinary();
	// xSemaphore_blinkLED = xSemaphoreCreateBinary();

	gpio_set_intr_type(PIN_SWITCH_PROGS, GPIO_INTR_POSEDGE);
	// gpio_set_intr_type(BLINK_BUTTON, GPIO_INTR_ANYEDGE);

	// start task, send debounce data to 
	xTaskCreate(loop, "switch progs", 2048, NULL, 10, NULL);
	// init isr
	gpio_install_isr_service(0);
	//(void *)(&switchProgramm_IO)
    gpio_isr_handler_add(PIN_SWITCH_PROGS, isr_button, NULL);
    // gpio_isr_handler_add(BLINK_BUTTON, isr_button, (void*)BLINK_BUTTON);

	esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);

    // adc2_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_11);
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------
}


void IRAM_ATTR isr_button(void* args){
	xSemaphoreGiveFromISR(xSemaphore_switchProg, NULL);
}


// increses the program mode in order to switch programms
void loop(void *args){
		
	//debouncing code
	int64_t lastTime = 0;
	int64_t threshold = 200000;//mu seconds	

	// programm mode defines what blinking interaction the esp32 should do
	int programmMode = 0;
	int programmsCount = 3;
	int blickingSpeed = 1000; //millisec

	
	static const char *TAG_0 = "BLINK_PROG_0";
	static const char *TAG_1 = "BLINK_PROG_1";
	static const char *TAG_2 = "BLINK_PROG_2";

	while(1) {
		while(programmMode == 0){
			// ------------------- programm 0 --------------------
			// ----------------right blicks white-----------------
			ESP_LOGI(TAG_0, "flashed rgb internal LED ON");
			ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 1));
			ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 1));
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
			ESP_ERROR_CHECK(gpio_set_level(EXTERNAL_LED, 0));

			vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);

			ESP_LOGI(TAG_0, "flashed rgb internal LED OFF");
			ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 0));
			ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 0));
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 0));
			ESP_ERROR_CHECK(gpio_set_level(EXTERNAL_LED, 1));

			vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);

			switchProgramms(&lastTime, &threshold, TAG_0, &programmMode, &programmsCount, &blickingSpeed);
		}
		while(programmMode == 1){
			// ------------------- programm 1 --------------------
			// -----------blink external LED with button----------
			while(gpio_get_level(BLINK_BUTTON)){
				ESP_LOGI(TAG_1, "external OFF");
				ESP_ERROR_CHECK(gpio_set_level(EXTERNAL_LED, 0));
				vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);
				ESP_LOGI(TAG_1, "external ON");
				ESP_ERROR_CHECK(gpio_set_level(EXTERNAL_LED, 1));
				vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);
			}

			vTaskDelay(10 / portTICK_PERIOD_MS);
			switchProgramms(&lastTime, &threshold, TAG_1, &programmMode, &programmsCount, &blickingSpeed);
			
		}
		while(programmMode == 2){
			// ------------------- programm 2 --------------------
			// ----------bling red, green, blue, red,...----------
			ESP_LOGI(TAG_2, "red");
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 0));
			vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
			ESP_LOGI(TAG_2, "green");
			ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 0));
			vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);
			ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 1));
			ESP_LOGI(TAG_2, "blue");
			ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 0));
			vTaskDelay(blickingSpeed / portTICK_PERIOD_MS);
			ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 1));
			
			switchProgramms(&lastTime, &threshold, TAG_2, &programmMode, &programmsCount, &blickingSpeed);
			
		}
	}
	
}


/**
 * code that changes the programm if button was pressed
*/
void switchProgramms(int64_t* lastTime, int64_t* threshold,
					 const char* tag,
					 int* programmMode, int* programmsCount,
					 int* blickingSpeed){
	
	// check if we want to switch programms
	if(xSemaphoreTake(xSemaphore_switchProg,0) == pdTRUE) {
		int64_t curTime = esp_timer_get_time();
		// debouncing the button
		if( (curTime - *lastTime) > *threshold ){
			ESP_LOGI(tag, "switched prog!");
			*programmMode = (*programmMode + 1) % *programmsCount;
			ESP_ERROR_CHECK(gpio_set_level(GREEN_INTERNAL_LED, 1));
			ESP_ERROR_CHECK(gpio_set_level(BLUE_INTERNAL_LED, 1));
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
		}
		// to take all semaphores
		*lastTime = curTime;
	}
    // check if we want to switch blinking speed
	int adc_value;
	//read the value of the potentional meter
	adc2_get_raw(ADC2_CHANNEL_0,ADC_WIDTH_BIT_DEFAULT,&adc_value);
	// if potentialmat is fully to the right -> blink every 3 seconds
	// 4098 are 12bit width of the adc
	*blickingSpeed = ((adc_value*1000) / 4098) * 3;
	// printf("ADC Value: %d\n", *blickingSpeed);
}