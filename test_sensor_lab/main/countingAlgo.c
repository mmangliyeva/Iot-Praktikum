#include "mySetup.h"
#include "countingAlgo.h"
#include "mqtt.h"
#include "ssd1306.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "timeMgmt.h"
 
// -------------------------------------------------------------------------
// --------------------capsel this data-------------------------------------
// ----------------that it is only accsable from countingAlgo.c-------------
// inter process communicaiton:
QueueHandle_t queue = NULL;
TaskHandle_t xAnalyzeProc = NULL;
TaskHandle_t xShowStateProc = NULL;
SemaphoreHandle_t xAccessCount = NULL;
SemaphoreHandle_t xAccessBuffer = NULL;
SemaphoreHandle_t xPressedButton = NULL;

// struct with data that is inside the buffer
typedef struct Barrier_data
{
	uint8_t id; // is the pin 
	uint8_t state; // 0 NO obsical, 1 there is an obsical
	time_t time;
} Barrier_data;
// poeple count variable
volatile uint8_t count = 0;
uint8_t prediction = 0;

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
//button
int64_t lastTimeISR = 0;

// all tasks:
void pushInBuffer(void* args);
void showRoomState(void* args);
void sendCountToDatabase(void* args);
void analyzer(void *args);
void testMode(void* args);

// all interrupt routines:
void IRAM_ATTR isr_barrier1(void* args);
void IRAM_ATTR isr_barrier2(void* args);
void IRAM_ATTR isr_test_mode(void* args);
//helper functions
#ifdef SEND_EVERY_EVENT
void sendToDatabase(uint8_t delteItemsCount);
#endif
void pauseOtherTasks(uint8_t* testModeActive,TickType_t blocktime);
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------



void start_counting_algo(void){
	// restore count from nvs:
	count = restoreCount(NO_OPEN_NVS);
	displayCountPreTime(prediction,count);
	ESP_LOGI("PROGRESS","Restore count: %d",count);
    // set up semaphore for accessing the shared variable count...
	xAccessCount = xSemaphoreCreateBinary();
	xAccessBuffer = xSemaphoreCreateBinary();
	
	xSemaphoreGive(xAccessCount);
	xSemaphoreGive(xAccessBuffer);
	// xAccessHead = xSemaphoreCreateBinary();

	// queue for interprocess communication:
	queue = xQueueCreate(SIZE_QUEUE,sizeof(Barrier_data));
	if (queue == 0){
    	ESP_LOGI("PROGRESS", "Failed to create queue!");
    }
	

	// init isr
	//(void*)PIN_DETECT_1
    gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
    gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);

    // start task, for analyzing the 
	xTaskCreate(analyzer, "analizer", 8000, NULL, PRIO_ANALIZER, &xProgAnalizer);
	xTaskCreate(showRoomState, "show count", 2048, NULL, PRIO_SHOW_COUNT, &xProgShowCount);
	xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, PRIO_IN_BUFFER, &xProgInBuffer);
	xTaskCreate(sendCountToDatabase, "send count to database", 8000, NULL, PRIO_SEND_TO_DB, &xProgSendToDB);

    // test mode code:
    xPressedButton = xSemaphoreCreateBinary();
    gpio_isr_handler_add(PIN_TEST_MODE, isr_test_mode, NULL);
    xTaskCreate(testMode, "test mode", 2048, NULL, PRIO_TEST_MODE, NULL);

}




void pushInBuffer(void* args){
	while(1){
		if(xSemaphoreTake(xAccessBuffer,(TickType_t)0) == pdTRUE){
			if(xQueueReceive(queue, buffer+((head+fillSize)%SIZE_BUFFER),(TickType_t)5)){
				ESP_LOGI("pushInBuffer()", "id: %d state: %d time %ld", buffer[((head+fillSize)%SIZE_BUFFER)].id, buffer[((head+fillSize)%SIZE_BUFFER)].state, (long)buffer[((head+fillSize)%SIZE_BUFFER)].time);
				if(fillSize == SIZE_BUFFER-1){
					ESP_LOGE("ERROR", "ERROR: Buffer overflow, make SIZE_BUFFER larger!");
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
                uint8_t delteItemsCount = 0;
                // send here stuff to mqtt with for-loop
				if(eventType == NO_EVENT){
                    delteItemsCount = 1;
					// ESP_LOGI("analyzer()", " delete all elments in buffer");
					// head = (head+fillSize)%SIZE_BUFFER;
					// fillSize = 0;
				}
				else{
                    delteItemsCount = 4;
					// delte first 4 elements out of the buffer
				}
#ifdef SEND_EVERY_EVENT
                sendToDatabase(delteItemsCount);
#endif                
                // delte events form buffer:
                head = (head+delteItemsCount)%SIZE_BUFFER;
                fillSize -= delteItemsCount;
			}
			xSemaphoreGive(xAccessBuffer);
		}

		// */
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}							
}

#ifdef SEND_EVERY_EVENT
void sendToDatabase(uint8_t delteItemsCount){
    for(uint8_t i = head; i < (head+delteItemsCount)%SIZE_BUFFER; i = ((i+1)%SIZE_BUFFER)){
        if(buffer[i].id == 1){
            const char* barrierName = "outdoor_barrier";
			writeToNVM(barrierName,count, buffer[i].state, buffer[i].time);
        }
        else{
            const char* barrierName = "indoor_barrier";
			writeToNVM(barrierName,count, buffer[i].state, buffer[i].time);
        }

    }
}
#endif                

/**
 * this function print the current count of people on the LCD
*/
void showRoomState(void* args){	
	uint8_t oldCount = 0;
    if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
	    oldCount = count;
        xSemaphoreGive(xAccessCount);
    }
	while(1){
		// ESP_LOGI("showRoomState()", "wait for semaphore");
		if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
			if(oldCount != count){
				//converts int to string
                displayCountPreTime(prediction,count);
				oldCount = count;

			}
			xSemaphoreGive(xAccessCount);
			// ESP_LOGI("showRoomState()", "released semaphore");
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
/**
 * this task sends the current count of people
 * to our database
*/
void sendCountToDatabase(void* args){
    // time_t now = 0;
    uint8_t oldCount = 0;
    if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
	    oldCount = count;
        // time(&now);
		writeToNVM("counter",count, -1, get_timestamp());
        xSemaphoreGive(xAccessCount);
    }
	while(1){
		// ESP_LOGI("showRoomState()", "wait for semaphore");
		if(xSemaphoreTake(xAccessCount,portMAX_DELAY) == pdTRUE){
			if(oldCount != count){	
				oldCount = count;
                // time(&now);
                writeToNVM("counter",count, -1, get_timestamp());
			}
			xSemaphoreGive(xAccessCount);
			// ESP_LOGI("showRoomState()", "released semaphore");
		}
		// check if we are having the reset hours -> reset counter 
		resetCount(NO_OPEN_NVS);
        // dont send soo often 
		// esp_timer_dump(stdout); // for timer interrupts
		vTaskDelay((1000*PUT_IN_NVS_DELAY) / portTICK_PERIOD_MS);
	}
}	
/**
 * this tasks simply waits until button is pressed and then
 * pause all other tasks
*/
void testMode(void* args){
        // time_t now = 0;
        // time(&now);
        // todo maybe implement that automatically switches back to normal-mode
        while(1){
            pauseOtherTasks(&testModeActive,portMAX_DELAY);
            while(testModeActive){
                ssd1306_clearScreen();
                ssd1306_printFixedN(8,16,"TEST",STYLE_BOLD,2);
                ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 0));
                ssd1306_clearScreen();
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                pauseOtherTasks(&testModeActive,0);

            }
			vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

void pauseOtherTasks(uint8_t* testModeActive,TickType_t blocktime){
    if(xSemaphoreTake(xPressedButton,blocktime) == pdTRUE) {
        *testModeActive = !(*testModeActive);
        // ets_printf("test mode %d", *testModeActive);
        if(*testModeActive == 1){
            vTaskSuspend(xProgAnalizer);
            vTaskSuspend(xProgShowCount);
            vTaskSuspend(xProgSendToDB);
            vTaskSuspend(xProgInBuffer);
            vTaskSuspend(xSendToMQTT);
            // vTaskSuspend(xOTA);
            gpio_isr_handler_remove(PIN_DETECT_1);
            gpio_isr_handler_remove(PIN_DETECT_2);
            // taskDISABLE_INTERRUPTS();
        }
        else{
            vTaskResume(xProgAnalizer);
            vTaskResume(xProgShowCount);
            vTaskResume(xProgSendToDB);
            vTaskResume(xProgInBuffer);
			vTaskResume(xSendToMQTT);
            // vTaskResume(xOTA);
            gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
            gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);
            ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
            ssd1306_clearScreen();
            displayCountPreTime(prediction,count);
            // taskENABLE_INTERRUPTS();
        }
    }
}



//------------------------------------------------------------------------------
//-----------------------------ISR routines-------------------------------------
//------------------------------------------------------------------------------



// for detector PIN_DETECT_1
void IRAM_ATTR isr_barrier1(void* args){
	// ets_printf("isr_barrier1\n ");	
	
	uint8_t curState = gpio_get_level(PIN_DETECT_1);
	// //debounce code
	if(lastState1 != curState){
        int64_t curTime = esp_timer_get_time();

        // ets_printf("curTime1: %ld, lastTime1 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime1, (long)(curTime - lastTime1), curState);
		if (curTime - lastTime1 > THRESHOLD_DEBOUCE){
			//  ets_printf("registered 1\n");
			// AND last statechange long ago
			Barrier_data data = {1,curState,get_timestamp()};
			xQueueSendFromISR(queue,&data,(TickType_t)0);
			lastState1 = curState;
			lastTime1 = curTime;
		}
	}	
}
// just toggle state...
void IRAM_ATTR isr_barrier2(void* args){
	// ets_printf("isr_barrier2\n ");	

	uint8_t curState = gpio_get_level(PIN_DETECT_2);
	//debounce code
	if(lastState2 != curState){
		
		int64_t curTime = esp_timer_get_time();
		// ets_printf("curTime2: %ld, lastTime2 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime2, (long)(curTime - lastTime2), curState);
		if (curTime - lastTime2 > THRESHOLD_DEBOUCE){
			//  ets_printf("registered 2\n");
			// AND last statechange long ago
			Barrier_data data = {2,curState,get_timestamp()};
			xQueueSendFromISR(queue,&data,(TickType_t)0);
			lastState2 = curState;
			lastTime2 = curTime;
		}
	}	
}
// interrupt for test mode button
void IRAM_ATTR isr_test_mode(void* args){
	// //debounce code	
	// ets_printf("isr_test_mode\n ");	
    int64_t curTime = esp_timer_get_time();
    if (curTime - lastTimeISR > THRESHOLD_DEBOUCE_TEST_MODE){
        // ets_printf("curTime: %ld, difference: %ld\n",(long)curTime, (long)(curTime - lastTimeISR));
        xSemaphoreGiveFromISR(xPressedButton,NULL);
        lastTimeISR = curTime;
    }
}