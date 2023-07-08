#include "mySetup.h"
#include "countingAlgo.h"
#include "mqtt.h"
#include "ssd1306.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "timeMgmt.h"
#include "webFunctions.h"

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
// poeple count, prediction variable
volatile uint8_t count = 0;
uint8_t prediction = 0;

// ---- buffer_check() code -----
uint8_t reseted_buffer = 0; // flag if buffer was reseted
int8_t state_counter = 0;	// the current state of the machine
// defines how many elements should be anlaized in a row
int16_t buffer_count_valid_elements = 0;

// ---- buffer code -----
// buffer the events form the queue
Barrier_data buffer[SIZE_BUFFER];
// points to the last element in the buffer and shows how full the buffer is
volatile uint8_t head = 0;
volatile uint8_t fillSize = 0;

// ---- isr debounce code -----
// for debounce detector:
int64_t lastTime1 = 0;
int64_t lastTime2 = 0;
uint8_t lastState1 = 0;
uint8_t lastState2 = 0;
// test-button
int64_t lastTimeISR = 0;

// all tasks:
void pushInBuffer(void *args);
void showRoomState(void *args);
void analyzer(void *args);
void testMode(void *args);
void sendFromNVS(void *args);

// all interrupt routines:
void IRAM_ATTR isr_outter_barrier(void *args);
void IRAM_ATTR isr_inner_barrier(void *args);
void IRAM_ATTR isr_test_mode(void *args);

// helper functions
#ifdef SEND_EVERY_EVENT
void sendToDatabase(uint8_t delteItemsCount);
#endif
void pauseOtherTasks(uint8_t *testModeActive, TickType_t blocktime);
// function that empties the buffer if events are too far away frome each other
int16_t checkBuffer(void);
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void start_counting_algo(void)
{
	buffer_count_valid_elements = 0;
	// restore count from IoT-plattform:
	count = getCount_backup();
	prediction = getPrediction();
	displayCountPreTime(prediction, count);
	ESP_LOGI("PROGRESS", "Restore count: %d", count);
	// set up semaphore for accessing the shared variable count...
	xAccessCount = xSemaphoreCreateBinary();
	xAccessBuffer = xSemaphoreCreateBinary();

	xSemaphoreGive(xAccessCount);
	xSemaphoreGive(xAccessBuffer);

	// queue for interprocess communication:
	queue = xQueueCreate(SIZE_QUEUE, sizeof(Barrier_data));
	if (queue == 0)
	{
		ESP_LOGI("PROGRESS", "Failed to create queue!");
	}

	// init isr
	//(void*)OUTDOOR_BARRIER
	gpio_isr_handler_add(OUTDOOR_BARRIER, isr_outter_barrier, NULL);
	gpio_isr_handler_add(INDOOR_BARRIER, isr_inner_barrier, NULL);

	// start task, for analyzing the
	xTaskCreate(analyzer, "analizer", 6000, NULL, PRIO_ANALIZER, &xProgAnalizer);
	xTaskCreate(showRoomState, "show count", 3048, NULL, PRIO_SHOW_COUNT, &xProgShowCount);
	xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, PRIO_IN_BUFFER, &xProgInBuffer);
	xTaskCreate(sendFromNVS, "send nvs to mqtt", 4000, NULL, PRIO_SEND_NVS, &xSendToMQTT);
	// test mode code:
	xPressedButton = xSemaphoreCreateBinary();
	gpio_isr_handler_add(PIN_TEST_MODE, isr_test_mode, NULL);
	xTaskCreate(testMode, "test mode", 4048, NULL, PRIO_TEST_MODE, NULL);
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
 * this task analizes the buffer and increases count or state of state_machine
 *  -> de-/increases count
 */
void analyzer(void *args)
{
	while (1)
	{
		/* Logic for analyzer:
			Use a statemachine:
			if state_counter == 4 -> count++
			if state_counter == -4 -> count--
		*/
		if (xSemaphoreTake(xAccessBuffer, portMAX_DELAY) == pdTRUE)
		{
			// uncomment if you dont want to  use the buffer
			//  if (fillSize)
			if (checkBuffer())
			{
				// bugfixing code:
				// Barrier_data firstEvent = buffer[head];
				// Barrier_data secondEvent = buffer[(head + 1) % SIZE_BUFFER];
				// Barrier_data thirdEvent = buffer[(head + 2) % SIZE_BUFFER];
				// Barrier_data fourthEvent = buffer[(head + 3) % SIZE_BUFFER];
				// ESP_LOGI("analyzer()", "in buffer:\n(%d,%d), (%d,%d), (%d,%d), (%d,%d)", firstEvent.id, firstEvent.state, secondEvent.id, secondEvent.state, thirdEvent.id, thirdEvent.state, fourthEvent.id, fourthEvent.state);

				ESP_LOGI("analyzer()", "state BEFORE: %d", state_counter);
				Barrier_data event = buffer[head];
				if (state_counter % 2 == 0)
				{
					if (event.id == OUTDOOR_BARRIER)
					{
						state_counter++;
					}
					else if (event.id == INDOOR_BARRIER)
					{
						state_counter--;
					}
					else
					{
						ESP_LOGE("analyzer()", "event ID unkown");
					}
				}
				else
				{
					if (event.id == OUTDOOR_BARRIER)
					{
						state_counter--;
					}
					else if (event.id == INDOOR_BARRIER)
					{
						state_counter++;
					}
					else
					{
						ESP_LOGE("analyzer()", "event ID unkown");
					}
				}

				ESP_LOGI("analyzer()", "state AFTER: %d", state_counter);
				// check if outgoing event:
				if (state_counter == 4)
				{ // IN-GOING event
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
							writeToNVM("counter", count, -1, get_timestamp());
							setCount_backup(count);
						}
						xSemaphoreGive(xAccessCount);
					}
					state_counter = 0;
				}
				else if (state_counter == -4)
				{ // OUT-GOING event
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
							writeToNVM("counter", count, -1, get_timestamp());
							setCount_backup(count);
						}
						xSemaphoreGive(xAccessCount);
					}
					state_counter = 0;
				}
				else if (state_counter > 4 || state_counter < -4)
				{
					error_message("analyzer()", "Invalid state not in range [-4,4] ", "");
				}

				// delte items form the buffer
				const uint8_t delteItemsCount = 1; // always delte one item
#ifdef SEND_EVERY_EVENT
				sendToDatabase(delteItemsCount);
#endif
				head = (head + delteItemsCount) % SIZE_BUFFER;
				fillSize -= delteItemsCount;
			}
			xSemaphoreGive(xAccessBuffer);
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
/**
 * This funciton reset the buffer if the last event is TIME_TO_EMPTY_BUFFER sec away
 */
int16_t checkBuffer(void)
{
	// ESP_LOGI("analyzer()", "checkbuffer: fillsize: %d, buffer_cou... %d", fillSize, buffer_count_valid_elements);
	if (buffer_count_valid_elements == 0)
	{
		if (fillSize >= THRESHOLD_ANALIZER)
		{
			// analize the next 4 emelemnt
			buffer_count_valid_elements = 4;

			return buffer_count_valid_elements;
		}
		else if (fillSize > 0)
		{

			Barrier_data lastEvent = buffer[((head + fillSize - 1) % SIZE_BUFFER)];
			if (get_timestamp() - lastEvent.time > TIME_TO_EMPTY_BUFFER)
			{
				// after TIME_TO_EMPTY_BUFFER sec empty the buffer...
				ESP_LOGI("analyzer()", "fillsize %d", fillSize);

				reseted_buffer = 1;
				return fillSize;
			}
		}
		else if (reseted_buffer)
		{
			// buffer is empty and there was no new event since TIME_TO_EMPTY_BUFFER sec
			//  -> reset state_machine
			reseted_buffer = 0;
			state_counter = 0;
			ESP_LOGI("analyzer()", "reset state fillsize %d", fillSize);
		}
	}
	else
	{
		// there are still enough element to analyze
		buffer_count_valid_elements--;
		return buffer_count_valid_elements;
	}
	return 0;
}

#ifdef SEND_EVERY_EVENT
/* DO NOT USE THIS! NVS might explode -> restart of device
 * empty the buffer and sends eeeevery event the state, time, and ID
 *
 */
void sendToDatabase(uint8_t delteItemsCount)
{
	for (uint8_t i = head; i < (head + delteItemsCount) % SIZE_BUFFER; i = ((i + 1) % SIZE_BUFFER))
	{
		if (buffer[i].id == 1)
		{
			const char *barrierName = "outdoor_barrier";
			writeToNVM(barrierName, count, buffer[i].state, buffer[i].time);
		}
		else
		{
			const char *barrierName = "indoor_barrier";
			writeToNVM(barrierName, count, buffer[i].state, buffer[i].time);
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
		vTaskDelay((1000 * REFRESH_RATE_DISPLAY) / portTICK_PERIOD_MS);
	}
}
/**
 * TASK
 * this task backup the current count of people
 * to the IoT-platform
 */
void sendFromNVS(void *args)
{
	time_t now = get_timestamp();
	struct tm *now_tm = localtime(&now);
	time_t previousTime_min = now_tm->tm_min; // used for fetching the predition

	while (1)
	{
		vTaskDelay((1000 * SEND_COUNT_DELAY) / portTICK_PERIOD_MS);

		if (xSemaphoreTake(xAccessCount, portMAX_DELAY) == pdTRUE)
		{
			// joa sadly memory leaks somewhere here :'(

			// uint32_t heapSize_begin = esp_get_free_heap_size();

			setCount_backup(count);
			// uint32_t heapSize_end = esp_get_free_heap_size();
			// if (heapSize_begin - heapSize_end != 0)
			// {
			// 	ESP_LOGE("PROGRESS", "Memory leaks  %lu  in backup count.", (long)(heapSize_begin - heapSize_end));
			// }
			// heapSize_begin = esp_get_free_heap_size();
			writeToNVM("counter", count, -1, get_timestamp());
			// heapSize_end = esp_get_free_heap_size();
			// if (heapSize_begin - heapSize_end != 0)
			// {
			// 	ESP_LOGE("PROGRESS", "Memory leaks  %lu  in writeToNVM.", (long)(heapSize_begin - heapSize_end));
			// }

			// heapSize_begin = esp_get_free_heap_size();
			sendDataFromJSON_toDB(NO_OPEN_NVS);
			// heapSize_end = esp_get_free_heap_size();
			// if (heapSize_begin - heapSize_end != 0)
			// {
			// 	ESP_LOGE("PROGRESS", "Memory leaks  %lu  in sendDataFromJSON_toDB.", (long)(heapSize_begin - heapSize_end));
			// }

			xSemaphoreGive(xAccessCount);
		}
		// check if we are having the reset hours -> reset counter
		now = time(NULL);
		now_tm = localtime(&now);
		if ((now_tm->tm_hour == RESET_COUNT_HOUR || now_tm->tm_hour == RESET_COUNT_HOUR2) && now_tm->tm_min < RESET_COUNT_MIN)
		{
			setCount_backup(0);
			esp_restart();
		}
		// update every 15 min the prediction
		if (now_tm->tm_min - previousTime_min >= 15)
		{
			prediction = getPrediction();
			previousTime_min = now_tm->tm_min;
		}
	}
}

/**
 * TASK
 * this tasks simply waits until button is pressed and then
 * pause all other tasks
 */
void testMode(void *args)
{
	while (1)
	{
		pauseOtherTasks(&testModeActive, portMAX_DELAY);
		while (testModeActive)
		{
			// make display blink with "TEST"
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
/**
 * helper function for testMode()
 */
void pauseOtherTasks(uint8_t *testModeActive, TickType_t blocktime)
{
	if (xSemaphoreTake(xPressedButton, blocktime) == pdTRUE)
	{
		*testModeActive = !(*testModeActive);
		if (*testModeActive == 1)
		{
			vTaskSuspend(xProgAnalizer);
			vTaskSuspend(xProgShowCount);
			vTaskSuspend(xProgInBuffer);
			vTaskSuspend(xSendToMQTT);
			vTaskSuspend(xOTA);
			gpio_isr_handler_remove(OUTDOOR_BARRIER);
			gpio_isr_handler_remove(INDOOR_BARRIER);
		}
		else
		{
			vTaskResume(xProgAnalizer);
			vTaskResume(xProgShowCount);
			vTaskResume(xProgInBuffer);
			vTaskResume(xSendToMQTT);
			vTaskResume(xOTA);
			gpio_isr_handler_add(OUTDOOR_BARRIER, isr_outter_barrier, NULL);
			gpio_isr_handler_add(INDOOR_BARRIER, isr_inner_barrier, NULL);
			ESP_ERROR_CHECK(gpio_set_level(RED_INTERNAL_LED, 1));
			ssd1306_clearScreen();
			displayCountPreTime(prediction, count);
		}
	}
}

//------------------------------------------------------------------------------
//-----------------------------ISR routines-------------------------------------
//------------------------------------------------------------------------------

void IRAM_ATTR isr_inner_barrier(void *args)
{
	// ets_printf("from isr  pin %d\n", pin);
	uint8_t curState = gpio_get_level(INDOOR_BARRIER);
	// debounce code
	if (lastState1 != curState)
	{
		int64_t curTime = esp_timer_get_time();
		// ets_printf("curTime1: %ld, lastTime1 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime1, (long)(curTime - lastTime1), curState);
		if (curTime - lastTime1 > THRESHOLD_DEBOUCE)
		{
			//  ets_printf("registered 1\n");
			Barrier_data data = {INDOOR_BARRIER, curState, get_timestamp()};
			xQueueSendFromISR(queue, &data, (TickType_t)0);
			lastState1 = curState;
			lastTime1 = curTime;
		}
	}
}
void IRAM_ATTR isr_outter_barrier(void *args)
{
	// ets_printf("isr_barrier2\n ");
	uint8_t curState = gpio_get_level(OUTDOOR_BARRIER);
	// debounce code
	if (lastState2 != curState)
	{
		int64_t curTime = esp_timer_get_time();
		// ets_printf("curTime2: %ld, lastTime2 %ld, difference: %ld state %d\n",(long)curTime, (long)lastTime2, (long)(curTime - lastTime2), curState);
		if (curTime - lastTime2 > THRESHOLD_DEBOUCE)
		{
			//  ets_printf("registered 2\n");
			Barrier_data data = {OUTDOOR_BARRIER, curState, get_timestamp()};
			// ets_printf("ISR2 pushed\n");
			xQueueSendFromISR(queue, &data, (TickType_t)0);
			lastState2 = curState;
			lastTime2 = curTime;
		}
	}
}
/**
 *  Interrupt for test mode button
 */
void IRAM_ATTR isr_test_mode(void *args)
{
	// //debounce code
	int64_t curTime = esp_timer_get_time();
	if (curTime - lastTimeISR > THRESHOLD_DEBOUCE_TEST_MODE)
	{
		// ets_printf("curTime: %ld, difference: %ld\n",(long)curTime, (long)(curTime - lastTimeISR));
		xSemaphoreGiveFromISR(xPressedButton, NULL);
		lastTimeISR = curTime;
	}
}