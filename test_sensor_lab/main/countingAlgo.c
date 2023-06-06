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
	uint8_t id;	   // is the pin
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

// for debounce detector:
int64_t lastTime1 = 0;
int64_t lastTime2 = 0;
uint8_t lastState1 = 0;
uint8_t lastState2 = 0;
// button
int64_t lastTimeISR = 0;

// all tasks:
void pushInBuffer(void *args);
void showRoomState(void *args);
void analyzer(void *args);
void testMode(void *args);
void sendFromNVS(void *args);

// all interrupt routines:
void IRAM_ATTR isr_barrier1(void *args);
void IRAM_ATTR isr_barrier2(void *args);
void IRAM_ATTR isr_test_mode(void *args);

// helper functions
#ifdef SEND_EVERY_EVENT
void sendToDatabase(uint8_t delteItemsCount);
#endif
void pauseOtherTasks(uint8_t *testModeActive, TickType_t blocktime);
void checkTimes(Barrier_data firstEvent,
				Barrier_data secondEvent,
				Barrier_data thirdEvent,
				Barrier_data fourthEvent,
				uint8_t *delteItemsCount);
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void start_counting_algo(void)
{
	// restore count from nvs:
	count = restoreCount(NO_OPEN_NVS);
	displayCountPreTime(prediction, count);
	ESP_LOGI("PROGRESS", "Restore count: %d", count);
	// set up semaphore for accessing the shared variable count...
	xAccessCount = xSemaphoreCreateBinary();
	xAccessBuffer = xSemaphoreCreateBinary();

	xSemaphoreGive(xAccessCount);
	xSemaphoreGive(xAccessBuffer);
	// xAccessHead = xSemaphoreCreateBinary();

	// queue for interprocess communication:
	queue = xQueueCreate(SIZE_QUEUE, sizeof(Barrier_data));
	if (queue == 0)
	{
		ESP_LOGI("PROGRESS", "Failed to create queue!");
	}

	// init isr
	//(void*)PIN_DETECT_1
	gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
	gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);

	// start task, for analyzing the
	xTaskCreate(analyzer, "analizer", 4000, NULL, PRIO_ANALIZER, &xProgAnalizer);
	xTaskCreate(showRoomState, "show count", 2048, NULL, PRIO_SHOW_COUNT, &xProgShowCount);
	xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, PRIO_IN_BUFFER, &xProgInBuffer);
	xTaskCreate(sendFromNVS, "send nvs to mqtt", 4000, NULL, PRIO_SEND_NVS, &xSendToMQTT);

	// test mode code:
	xPressedButton = xSemaphoreCreateBinary();
	gpio_isr_handler_add(PIN_TEST_MODE, isr_test_mode, NULL);
	xTaskCreate(testMode, "test mode", 2048, NULL, PRIO_TEST_MODE, NULL);
}
/**
 * TASK
 * puts form the xQueue the elements in the buffer
 */
void pushInBuffer(void *args)
{
	while (1)
	{
		if (xSemaphoreTake(xAccessBuffer, (TickType_t)0) == pdTRUE)
		{
			if (xQueueReceive(queue, buffer + ((head + fillSize) % SIZE_BUFFER), (TickType_t)5))
			{
				ESP_LOGI("pushInBuffer()", "id: %d state: %d time %ld", buffer[((head + fillSize) % SIZE_BUFFER)].id, buffer[((head + fillSize) % SIZE_BUFFER)].state, (long)buffer[((head + fillSize) % SIZE_BUFFER)].time);
				if (fillSize == SIZE_BUFFER - 1)
				{
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
 * TASK
 * this task analyzes the queue for inconsitency and correctness
 *  -> de-/increases count
 */
void analyzer(void *args)
{
	// an array of funciton pointers
	while (1)
	{
		/* Logic for analyzer:
			search for the two pattern of going in and out.
			going in: (id:1,state:1), (id:2,state:1), (id:1,state:0), (id:2,state:0)
			going out: (id:2,state:1), (id:1,state:1), (id:2,state:0), (id:1,state:0)

			if time, id or state is not correct -> delte first element in buffer
			and start search again
		*/
		// /*
		if (xSemaphoreTake(xAccessBuffer, portMAX_DELAY) == pdTRUE)
		{

			if (fillSize >= THRESHOLD_ANALIZER)
			{
				Barrier_data firstEvent = buffer[head];
				Barrier_data secondEvent = buffer[(head + 1) % SIZE_BUFFER];
				Barrier_data thirdEvent = buffer[(head + 2) % SIZE_BUFFER];
				Barrier_data fourthEvent = buffer[(head + 3) % SIZE_BUFFER];
				ESP_LOGI("analyzer()", "in buffer:\n(%d,%d), (%d,%d), (%d,%d), (%d,%d)", firstEvent.id, firstEvent.state, secondEvent.id, secondEvent.state, thirdEvent.id, thirdEvent.state, fourthEvent.id, fourthEvent.state);
				int8_t eventType = NO_EVENT;
				// check order and if events lay near together...
				// incorrecness can be to events are really far away than the rest
				uint8_t delteItemsCount = 0;

				// check now wheter there the time stamps
				// changes maybe delteItemsCount...
				ets_printf("start check times");
				ESP_LOGI("PROGRESS", "before check time");
				checkTimes(firstEvent, secondEvent, thirdEvent, fourthEvent, &delteItemsCount);
				ets_printf("delteItemsCount: %d", delteItemsCount);
				if (delteItemsCount == 0)
				{
					// ESP_LOGI("analyzer()", "head: %d, size: %d",head,fillSize);
					ESP_LOGI("analyzer()", "time is correct.");
					if ((firstEvent.state == 1 && secondEvent.state == 1 && thirdEvent.state == 0 && fourthEvent.state == 0))
					{
						ESP_LOGI("analyzer()", "states are correct.");
						// check for going-in event
						if ((firstEvent.id == 1 && secondEvent.id == 2 && thirdEvent.id == 1 && fourthEvent.id == 2))
						{
							eventType = GOING_IN_EVENT;
							ESP_LOGI("analyzer()", "wait for semaphore");
							if (xSemaphoreTake(xAccessCount, portMAX_DELAY) == pdTRUE)
							{
								if (count > 250)
								{
									ESP_LOGI("analyzer()", "Count is toooo high (>250)! Do not update count...");
								}
								else
								{
									ESP_LOGI("analyzer()", "detected going-in-event.");
									count++;
									writeToNVM("counter", "", count, -1, get_timestamp());
								}
								xSemaphoreGive(xAccessCount);
								ESP_LOGI("analyzer()", "released semaphore");
							}
						}
						// check for going-out event
						else if (firstEvent.id == 2 && secondEvent.id == 1 && thirdEvent.id == 2 && fourthEvent.id == 1)
						{
							eventType = GOING_OUT_EVENT;
							ESP_LOGI("analyzer()", "wait for semaphore");
							if (xSemaphoreTake(xAccessCount, portMAX_DELAY) == pdTRUE)
							{
								if (count <= 0)
								{
									ESP_LOGI("analyzer()", "Count is toooo low (< 0)! Do not update count...");
								}
								else
								{
									ESP_LOGI("analyzer()", "detected going-out-event.");
									count--;
									writeToNVM("counter", "", count, -1, get_timestamp());
								}
								xSemaphoreGive(xAccessCount);
								ESP_LOGI("analyzer()", "released semaphore");
							}
						}
					}
				}

				// send here stuff to mqtt with for-loop
				if (eventType == GOING_IN_EVENT || eventType == GOING_OUT_EVENT)
				{
					delteItemsCount = 4;
					// ESP_LOGI("analyzer()", " delete all elments in buffer");
					// head = (head+fillSize)%SIZE_BUFFER;
					// fillSize = 0;
				}
				else if (delteItemsCount == 0)
				{
					delteItemsCount = 1;
				}
#ifdef SEND_EVERY_EVENT
				sendToDatabase(delteItemsCount);
#endif
				// delte events form buffer:
				head = (head + delteItemsCount) % SIZE_BUFFER;
				fillSize -= delteItemsCount;
			}
			xSemaphoreGive(xAccessBuffer);
		}

		// */
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

/**
 * helper function
 * of course the events should be all in one line after each other
 * but also not too far away from each other
 *
 * returns which index of the four elements in the window should be deleted
 */
void checkTimes(Barrier_data firstEvent,
				Barrier_data secondEvent,
				Barrier_data thirdEvent,
				Barrier_data fourthEvent,
				uint8_t *delteItemsCount)
{
	// detect  with for loop when we should delte

	Barrier_data array_data[4] = {firstEvent, secondEvent, thirdEvent, fourthEvent};
	for (uint8_t i = 1; i < 4; i++)
	{
		// sanity check
		if (array_data[i - 1].time <= array_data[i].time)
		{
			// time is in mu-sec
			if ((array_data[i].time - array_data[i - 1].time) > TIME_TO_NEXT_EVENT)
			{
				ESP_LOGI("analyzer()", "event too far away: %ld and i: %d", (long)(array_data[i].time - array_data[i - 1].time), i);

				// if the time between two events is too big delte until this event the events
				*delteItemsCount = i;
			}
		}
		else
		{
			ESP_LOGI("analyzer()", "Sanity check faild i: %d", i);
			*delteItemsCount = i;
		}
	}
	// the max we delte are 3 events, which seems ok...
}

#ifdef SEND_EVERY_EVENT
void sendToDatabase(uint8_t delteItemsCount)
{
	for (uint8_t i = head; i < (head + delteItemsCount) % SIZE_BUFFER; i = ((i + 1) % SIZE_BUFFER))
	{
		if (buffer[i].id == 1)
		{
			const char *barrierName = "outdoor_barrier";
			writeToNVM(barrierName, "", count, buffer[i].state, buffer[i].time);
		}
		else
		{
			const char *barrierName = "indoor_barrier";
			writeToNVM(barrierName, "", count, buffer[i].state, buffer[i].time);
		}
	}
}
#endif

/**
 * TASK
 * this function print the current count of people on the LCD
 */
void showRoomState(void *args)
{

	while (1)
	{
		// update every second the display
		displayCountPreTime(prediction, count);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
/**
 * TASK
 * this task sends the current count of people
 * to our NVS and then to elastic
 */
void sendFromNVS(void *args)
{
	while (1)
	{
		vTaskDelay((1000 * SEND_DELAY_FROM_NVS) / portTICK_PERIOD_MS);
		if (xSemaphoreTake(xAccessCount, portMAX_DELAY) == pdTRUE)
		{
			writeToNVM("counter", "", count, -1, get_timestamp());
			xSemaphoreGive(xAccessCount);
		}
		// check if we are having the reset hours -> reset counter
		resetCount(NO_OPEN_NVS);

		sendDataFromJSON_toDB(NULL);
	}
}

/**
 * TASK
 * this tasks simply waits until button is pressed and then
 * pause all other tasks
 */
void testMode(void *args)
{
	// time_t now = 0;
	// time(&now);
	// todo maybe implement that automatically switches back to normal-mode
	while (1)
	{
		pauseOtherTasks(&testModeActive, portMAX_DELAY);
		while (testModeActive)
		{
			ssd1306_clearScreen();
			ssd1306_printFixedN(8, 16, "TEST", STYLE_BOLD, 2);
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 0));
			ssd1306_clearScreen();
			vTaskDelay(1000 / portTICK_PERIOD_MS);

			pauseOtherTasks(&testModeActive, 0);
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
// helper function
void pauseOtherTasks(uint8_t *testModeActive, TickType_t blocktime)
{
	if (xSemaphoreTake(xPressedButton, blocktime) == pdTRUE)
	{
		*testModeActive = !(*testModeActive);
		// ets_printf("test mode %d", *testModeActive);
		if (*testModeActive == 1)
		{
			vTaskSuspend(xProgAnalizer);
			vTaskSuspend(xProgShowCount);
			vTaskSuspend(xProgInBuffer);
			vTaskSuspend(xSendToMQTT);
			// vTaskSuspend(xOTA);
			gpio_isr_handler_remove(PIN_DETECT_1);
			gpio_isr_handler_remove(PIN_DETECT_2);
			// taskDISABLE_INTERRUPTS();
		}
		else
		{
			vTaskResume(xProgAnalizer);
			vTaskResume(xProgShowCount);
			vTaskResume(xProgInBuffer);
			vTaskResume(xSendToMQTT);
			// vTaskResume(xOTA);
			gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
			gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
			ssd1306_clearScreen();
			displayCountPreTime(prediction, count);
			// taskENABLE_INTERRUPTS();
		}
	}
}

//------------------------------------------------------------------------------
//-----------------------------ISR routines-------------------------------------
//------------------------------------------------------------------------------

// for detector PIN_DETECT_1
void IRAM_ATTR isr_barrier1(void *args)
{
	// ets_printf("isr_barrier1\n ");

	uint8_t curState = gpio_get_level(PIN_DETECT_1);
	// //debounce code
	if (lastState1 != curState)
	{
		int64_t curTime = esp_timer_get_time();

		// ets_printf("curTime1: %ld, lastTime1 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime1, (long)(curTime - lastTime1), curState);
		if (curTime - lastTime1 > THRESHOLD_DEBOUCE)
		{
			//  ets_printf("registered 1\n");
			// AND last statechange long ago
			Barrier_data data = {1, curState, get_timestamp()};
			// ets_printf("ISR1 pushed\n");
			xQueueSendFromISR(queue, &data, (TickType_t)0);
			lastState1 = curState;
			lastTime1 = curTime;
		}
	}
}
// just toggle state...
void IRAM_ATTR isr_barrier2(void *args)
{
	// ets_printf("isr_barrier2\n ");

	uint8_t curState = gpio_get_level(PIN_DETECT_2);
	// debounce code
	if (lastState2 != curState)
	{

		int64_t curTime = esp_timer_get_time();
		// ets_printf("curTime2: %ld, lastTime2 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime2, (long)(curTime - lastTime2), curState);
		if (curTime - lastTime2 > THRESHOLD_DEBOUCE)
		{
			//  ets_printf("registered 2\n");
			// AND last statechange long ago
			Barrier_data data = {2, curState, get_timestamp()};
			// ets_printf("ISR2 pushed\n");
			xQueueSendFromISR(queue, &data, (TickType_t)0);
			lastState2 = curState;
			lastTime2 = curTime;
		}
	}
}
// interrupt for test mode button
void IRAM_ATTR isr_test_mode(void *args)
{
	// //debounce code
	// ets_printf("isr_test_mode\n ");
	int64_t curTime = esp_timer_get_time();
	if (curTime - lastTimeISR > THRESHOLD_DEBOUCE_TEST_MODE)
	{
		// ets_printf("curTime: %ld, difference: %ld\n",(long)curTime, (long)(curTime - lastTimeISR));
		xSemaphoreGiveFromISR(xPressedButton, NULL);
		lastTimeISR = curTime;
	}
}