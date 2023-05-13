#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "ssd1306.h"

#define BUFF_STRING_COUNT 4 //size of the buffer to display the number
#define PIN_DETECT_1 4
#define PIN_DETECT_2 2
#define THRESHOLD_DEBOUCE 50000
#define PRIO_ANALIZER 5
#define PRIO_SHOW_COUNT 1

typedef struct Barrier_data
{
	uint8_t id; // is the pin 
	uint8_t state; // 0 NO obsical, 1 there is an obsical
	int64_t time;
} Barrier_data;

// for the interrupts
SemaphoreHandle_t xSemaphore_switchProg = NULL;

// inter process communicaiton:
QueueHandle_t queue = NULL;
TaskHandle_t xAnalyzeProc = NULL;
TaskHandle_t xShowStateProc = NULL;

// poeple count variable
volatile uint8_t count = 0;

//for debounce detector:
int64_t lastTime1 = 0;
int64_t lastTime2 = 0;
uint8_t lastState1 = 0;
uint8_t lastState2 = 0;


// function declartion
void analyzer(void* args);
void IRAM_ATTR isr_barrier1(void* args);
void IRAM_ATTR isr_barrier2(void* args);
void showRoomState(void);
 

void app_main(void){
	//esp_log_level_set("BLINK", ESP_LOG_ERROR);       
	esp_log_level_set("APP", ESP_LOG_INFO);
// ----------------------------------------------------------------
// -------------------------set up ESP32---------------------------
// ----------------------------------------------------------------
	// init display:
	ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();
	ssd1306_printFixed(0,0,"People count:",STYLE_NORMAL);

	//init light barrier 1
	gpio_pad_select_gpio(PIN_DETECT_1);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_1, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_1)); 
	
	//init light barrier 2
	gpio_pad_select_gpio(PIN_DETECT_2);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_2, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_2));


	// xSemaphore_switchProg = xSemaphoreCreateBinary();
	// queue for interprocess communication:
	queue = xQueueCreate(20,sizeof(Barrier_data));
	if (queue == 0){
    	ESP_LOGI("ERROR", "Failed to create queue!");
		// exit(9);
    }
	
	gpio_set_intr_type(PIN_DETECT_1, GPIO_INTR_POSEDGE);
	gpio_set_intr_type(PIN_DETECT_2, GPIO_INTR_POSEDGE);
	// init isr
	gpio_install_isr_service(0);
	//(void*)PIN_DETECT_1
    gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
    gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);

	// start task, for analyzing the 
	xTaskCreate(analyzer, "analizer", 4096, NULL, PRIO_ANALIZER, NULL);
}


/**
 * this task analyzes the queue for inconsitency and correctness
 *  -> de-/increases count
*/
void analyzer(void *args){
	Barrier_data data;
	// an array of funciton pointers
	while(1){
		if( xQueueReceive(queue, &data, (TickType_t)5) ){
			// sprintf(outputMessage,"id: %d state: %d time %ld", data.id, data.state, (long)data.time);
			ESP_LOGI("MY_ANALIZER", "id: %d state: %d time %ld", data.id, data.state, (long)data.time);
			count++;
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	
							
}
/**
 * this function print the current count of people on the LCD
*/
void showRoomState(void){	
	char str[BUFF_STRING_COUNT];
	//converts int to string
	sprintf(str,"%d",count);
	ssd1306_printFixedN(0,16,str,STYLE_BOLD,2);
}



// for detector PIN_DETECT_1
void IRAM_ATTR isr_barrier1(void* args){
	uint8_t curState = gpio_get_level(PIN_DETECT_2);
	// //debounce code
	if(lastState1 != curState){
		int64_t curTime = esp_timer_get_time();
		if (curTime - lastTime1 > THRESHOLD_DEBOUCE){
			// AND last statechange long ago
			Barrier_data data = {1,curState,curTime};
			xQueueSendFromISR(queue,&data,(TickType_t)0);
			lastState1 = curState;
		}
		lastTime1 = curTime;
	}	
}
void IRAM_ATTR isr_barrier2(void* args){
	uint8_t curState = gpio_get_level(PIN_DETECT_2);
	// //debounce code
	if(lastState2 != curState){
		int64_t curTime = esp_timer_get_time();
		if (curTime - lastTime2 > THRESHOLD_DEBOUCE){
			// AND last statechange long ago
			Barrier_data data = {2,curState,curTime};
			xQueueSendFromISR(queue,&data,(TickType_t)0);
			lastState2 = curState;
		}
		lastTime2 = curTime;
	}	
}