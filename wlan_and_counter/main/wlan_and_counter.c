#include "myESP_setup.h"


typedef struct Barrier_data
{
	uint8_t id; // is the pin 
	uint8_t state; // 0 NO obsical, 1 there is an obsical
	int64_t time;
} Barrier_data;


// inter process communicaiton:
QueueHandle_t queue = NULL;
SemaphoreHandle_t xAccessCount = NULL;
SemaphoreHandle_t xAccessBuffer = NULL;

// poeple count variable
volatile uint8_t count = 0;

// buffer the events form the queue
Barrier_data buffer[SIZE_BUFFER];
// points to the last element in the buffer and shows how full the buffer is
volatile uint8_t head = 0;
volatile uint8_t fillSize = 0;

//for debounce detector:
int64_t lastTime1 = 0;
int64_t lastTime2 = 0;
uint8_t lastState1 = 0;
uint8_t lastState2 = 0;

// test function declerations
void test_mqtt(void);

void app_main(void){
	//esp_log_level_set("BLINK", ESP_LOG_ERROR);       
	// esp_log_level_set("*", ESP_LOG_INFO);
	// esp_log_level_set("*",  ESP_LOG_ERROR);
// -------------------------set up ESP32---------------------------
	// set up semaphore for accessing the shared variable count...
	xAccessCount = xSemaphoreCreateBinary();
	xAccessBuffer = xSemaphoreCreateBinary();
	
	xSemaphoreGive(xAccessCount);
	xSemaphoreGive(xAccessBuffer);

	// queue for interprocess communication:
	queue = xQueueCreate(SIZE_QUEUE,sizeof(Barrier_data));
	if (queue == 0){
    	ESP_LOGI("ERROR", "Failed to create queue!");
		// exit(9);
    }
	setup_myESP();
	// start wifi
// ----------------------------------------------------------------
}










// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// -----------------------------------------OLD CODE FROM LAST WEEK-----------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------

void pushInBuffer(void* args){
	while(1){
		if(xSemaphoreTake(xAccessBuffer,(TickType_t)0) == pdTRUE){
			if(xQueueReceive(queue, buffer+((head+fillSize)%SIZE_BUFFER),(TickType_t)5)){
				ESP_LOGI("pushInBuffer()", "id: %d state: %d time %ld", buffer[((head+fillSize)%SIZE_BUFFER)].id, buffer[((head+fillSize)%SIZE_BUFFER)].state, (long)buffer[((head+fillSize)%SIZE_BUFFER)].time);
				if(fillSize == SIZE_BUFFER-1){
					ESP_LOGI("pushInBuffer()", "ERROR: Buffer overflow, make SIZE_BUFFER larger!");
				}
				fillSize++;
			}
			xSemaphoreGive(xAccessBuffer);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/**
 * this task analyzes the queue for inconsitency and correctness
 *  -> de-/increases count
*/
void analyzer(void *args){

	// an array of funciton pointers
	while(1){
		/* Logic for analyzer:
			search for the two pattern of going in and out.
			going in: (id:1,state:1), (id:2,state:1), (id:1,state:0), (id:2,state:0)
			going out: (id:2,state:1), (id:1,state:1), (id:2,state:0), (id:1,state:0)

			if time, id or state is not correct -> delte first element in buffer 
			and start search again
		*/
		// /*
		if(xSemaphoreTake(xAccessBuffer,portMAX_DELAY) == pdTRUE){
			if(fillSize >= THRESHOLD_ANALIZER){
				uint8_t firstEvent = head;
				uint8_t secondEvent = (head+1)%SIZE_BUFFER;
				uint8_t thirdEvent = (head+2)%SIZE_BUFFER;
				uint8_t fourthEvent = (head+3)%SIZE_BUFFER;
				ESP_LOGI("analyzer()", "in buffer:\n(%d,%d), (%d,%d), (%d,%d), (%d,%d)", buffer[firstEvent].id, buffer[firstEvent].state, buffer[secondEvent].id, buffer[secondEvent].state, buffer[thirdEvent].id, buffer[thirdEvent].state, buffer[fourthEvent].id, buffer[fourthEvent].state);
				int8_t eventType = NO_EVENT;
				// check order 
				if((buffer[firstEvent].time <= buffer[secondEvent].time && buffer[secondEvent].time <= buffer[thirdEvent].time &&  buffer[thirdEvent].time <= buffer[fourthEvent].time)){
					// ESP_LOGI("analyzer()", "head: %d, size: %d",head,fillSize);
					ESP_LOGI("analyzer()", "time is correct.");
					if((buffer[firstEvent].state == 1 && buffer[secondEvent].state == 1 && buffer[thirdEvent].state == 0 && buffer[fourthEvent].state == 0)){
						ESP_LOGI("analyzer()", "states are correct.");
						// check for going-in event
						if((buffer[firstEvent].id == 1 && buffer[secondEvent].id == 2 && buffer[thirdEvent].id == 1 && buffer[fourthEvent].id == 2)){
							eventType = GOING_IN_EVENT;
							ESP_LOGI("analyzer()", "wait for semaphore");
							if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
								if(count > 250){
									ESP_LOGI("analyzer()", "Count is toooo high (>250)! Do not update count...");
								}
								else {
									ESP_LOGI("analyzer()", "detected going-in-event.");
									count++;
								}
								xSemaphoreGive(xAccessCount);
								ESP_LOGI("analyzer()", "released semaphore");
							}
						}
						// check for going-out event
						else if(buffer[firstEvent].id == 2 && buffer[secondEvent].id == 1 && buffer[thirdEvent].id == 2 && buffer[fourthEvent].id == 1){
							eventType = GOING_OUT_EVENT;
							ESP_LOGI("analyzer()", "wait for semaphore");
							if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
								if(count <= 0){
									ESP_LOGI("analyzer()", "Count is toooo low (< 0)! Do not update count...");
								}
								else {
									ESP_LOGI("analyzer()", "detected going-out-event.");
									count--;
								}
								xSemaphoreGive(xAccessCount);
								ESP_LOGI("analyzer()", "released semaphore");
							}
						}
					}
				}
				if(eventType == NO_EVENT){
					ESP_LOGI("analyzer()", "No event -> delte head");
					//no event found -> delte head
					head = (head+1)%SIZE_BUFFER;
					fillSize--;
				}
				else{
					// delte first 4 elements out of the buffer
					ESP_LOGI("analyzer()", " delete 4 elments in buffer");
					head = (head+4)%SIZE_BUFFER;
					fillSize -= 4;
				}
			}
			xSemaphoreGive(xAccessBuffer);
		}

		// */
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}							
}




/**
 * this function print the current count of people on the LCD
*/
void showRoomState(void* args){	
	char str[BUFF_STRING_COUNT];
	uint8_t oldCount = count;
	while(1){
		// ESP_LOGI("showRoomState()", "wait for semaphore");
		if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
			if(oldCount != count){
				//converts int to string
				sprintf(str,"%d",count);
				// ESP_LOGI("showRoomState()", "before access display");
				ssd1306_printFixedN(0,16,str,STYLE_BOLD,2);
				// ESP_LOGI("showRoomState()", "after access display");
				oldCount = count;
			}
			xSemaphoreGive(xAccessCount);
			// ESP_LOGI("showRoomState()", "released semaphore");
		}
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}



// for detector PIN_DETECT_1
void IRAM_ATTR isr_barrier1(void* args){
	uint8_t curState = gpio_get_level(PIN_DETECT_1);
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