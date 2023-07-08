#include "mySetup.h"
#include "countingAlgo.h"
#include "mqtt.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "timeMgmt.h"
#include "webFunctions.h"
#include "cJSON.h"

// -------------------------------------------------------------------------
// --------------------capsel this data-------------------------------------
// ----------------that it is only accsable from countingAlgo.c-------------
// inter process communicaiton:
QueueHandle_t queue = NULL;
SemaphoreHandle_t xAccessCount = NULL;
SemaphoreHandle_t xAccessBuffer = NULL;
SemaphoreHandle_t xPressedButton = NULL;

// struct with data that is inside the buffer
typedef struct Barrier_data
{
	uint8_t id; // is the pin
	// uint8_t state; // 0 NO obsical, 1 there is an obsical
	time_t time;
} Barrier_data;
// poeple count, prediction variable
volatile uint8_t count = 0;
volatile uint8_t prediction = 0;

int8_t state_counter = 0; // the current state of the machine

// ---- buffer code -----
// buffer the events form the queue
Barrier_data buffer[SIZE_BUFFER];
// points to the last element in the buffer and shows how full the buffer is
volatile uint8_t head = 0;
volatile uint8_t fillSize = 0;

// all tasks:
void pushInBuffer(void *args);
void analyzer(void *args);

void after_analizer(cJSON *root);
// all interrupt routines:
void IRAM_ATTR isr_barrier(void *args);

// helper function
void addEventToJSON(cJSON *root, uint8_t peopleCount, time_t time);
cJSON *createJSON(void);
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void start_counting_algo(void)
{
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
		ESP_LOGE("PROGRESS", "Failed to create queue!");
	}

	// init isr
	//(void*)OUTDOOR_BARRIER
	gpio_isr_handler_add(OUTDOOR_BARRIER, isr_barrier, (void *)OUTDOOR_BARRIER);
	gpio_isr_handler_add(INDOOR_BARRIER, isr_barrier, (void *)INDOOR_BARRIER);

	// start task, for analyzing the
	xTaskCreate(analyzer, "analizer", 6000, NULL, PRIO_ANALIZER, &xProgAnalizer);
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
				// ESP_LOGI("pushInBuffer()", "id: %d state: %d time %ld", buffer[((head + fillSize) % SIZE_BUFFER)].id, buffer[((head + fillSize) % SIZE_BUFFER)].state, (long)buffer[((head + fillSize) % SIZE_BUFFER)].time);
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
	cJSON *root = NULL;

	// time_t now = get_timestamp();
	// // struct tm *now_tm = localtime(&now);
	// time_t started_analizer = now;
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
			if (fillSize)
			{
				if (root == NULL)
				{
					root = createJSON();
					started_analizer = get_timestamp();
				}
				// bugfixing code:
				// Barrier_data firstEvent = buffer[head];
				// Barrier_data secondEvent = buffer[(head + 1) % SIZE_BUFFER];
				// Barrier_data thirdEvent = buffer[(head + 2) % SIZE_BUFFER];
				// Barrier_data fourthEvent = buffer[(head + 3) % SIZE_BUFFER];
				// ESP_LOGI("analyzer()", "in buffer:\n(%d,%d), (%d,%d), (%d,%d), (%d,%d)", firstEvent.id, firstEvent.state, secondEvent.id, secondEvent.state, thirdEvent.id, thirdEvent.state, fourthEvent.id, fourthEvent.state);

				// ESP_LOGI("analyzer()", "state BEFORE: %d", state_counter);
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

				// ESP_LOGI("analyzer()", "state AFTER: %d", state_counter);
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
							addEventToJSON(root, count, event.time);
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
							addEventToJSON(root, count, event.time);
						}
						xSemaphoreGive(xAccessCount);
					}
					state_counter = 0;
				}
				else if (state_counter > 4 || state_counter < -4)
				{
					ESP_LOGI("analyzer()", "Invalid state not in range [-4,4] ");
				}

				// delte items form the buffer
				head = (head + 1) % SIZE_BUFFER;
				fillSize -= 1;
			}
			else
			{
				// buffer is now empty -> send json and free json
				after_analizer(root);
				// go back to deepsleep because there was no new event

				// if (get_timestamp() - started_analizer > STAY_AWAKE)
				// {
				// 	// go back to deepsleep because there was no new event
				// }
			}
			xSemaphoreGive(xAccessBuffer);
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/**
 * This funciton is called after the anlizer finished analizeing the buffer
 * because we call every '' minutes the analizer -> this function call
 * gets executed every '' minutes
 */
void after_analizer(cJSON *root)
{
	// sendFromNVS
	time_t now = get_timestamp();
	struct tm *now_tm = localtime(&now);

	if (xSemaphoreTake(xAccessCount, portMAX_DELAY) == pdTRUE)
	{
		setCount_backup(count);

		if (root != NULL)
		{
			// send the stuff to mqtt
			char *msg = cJSON_Print(root);
			sendToMQTT(msg, QOS_SAFE);
			free(msg);
			cJSON_Delete(root);
			root = NULL;
		}

		xSemaphoreGive(xAccessCount);
	}

	// check if we are having the reset hours -> reset counter
	now = time(NULL);
	now_tm = localtime(&now);
	if ((now_tm->tm_hour == RESET_COUNT_HOUR2) && now_tm->tm_min < RESET_COUNT_MIN)
	{
		setCount_backup(0);
		// showRoomState
		prediction = getPrediction();
		displayCountPreTime(prediction, count);

		// go into deep sleep until 7 o clock, so for 8 hours
	}

	// showRoomState
	prediction = getPrediction();
	displayCountPreTime(prediction, count);
}

/**
 * create the json object we want to add our events
 */
cJSON *createJSON(void)
{
	return cJSON_Parse("{\"sensors\":[{\"name\":\"counter\",\"values\":[]}]}");
}

/**
 * Adds the event to the json object
 */
void addEventToJSON(cJSON *root, uint8_t peopleCount, time_t time)
{

	cJSON *sensor_array = cJSON_GetObjectItem(root, "sensors");
	// get correct sensor:
	cJSON *sensor = NULL;
	cJSON *current_element = NULL;
	cJSON_ArrayForEach(current_element, sensor_array)
	{
		char *curName = cJSON_GetObjectItem(current_element, "name")->valuestring;
		if (strcmp("counter", curName) == 0)
		{
			// strings are equal
			sensor = current_element;
			break;
		}
		cJSON_Delete(current_element);
		free(curName);
	}
	cJSON *values_array = cJSON_GetObjectItem(sensor, "values");

	// we dont have 64 bit so do some shitty trick...
	char *newEvent_str = malloc(SIZE_OF_MESSAGE);
	sprintf(newEvent_str, "{\"timestamp\":%lld000,\"countPeople\":%d}", (long long)time, peopleCount);

	cJSON *newEvent = cJSON_Parse(newEvent_str);

	cJSON_AddItemToArray(values_array, newEvent);

	free(newEvent_str);
}

//------------------------------------------------------------------------------
//-----------------------------ISR routine-------------------------------------
//------------------------------------------------------------------------------

void IRAM_ATTR isr_barrier(void *args)
{
	Barrier_data data = {(int)args, get_timestamp()};
	xQueueSendFromISR(queue, &data, (TickType_t)0);
}
