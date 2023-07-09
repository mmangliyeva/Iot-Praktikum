#include "mySetup.h"
#include "countingAlgo.h"
#include "mqtt.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "timeMgmt.h"
#include "webFunctions.h"
#include "cJSON.h"
#include "esp_sleep.h"

// -------------------------------------------------------------------------
// --------------------capsel this data-------------------------------------
// ----------------that it is only accsable from countingAlgo.c-------------

// poeple count, prediction variable
volatile uint8_t count = 0;
volatile uint8_t prediction = 0;

int8_t state_counter = 0; // the current state of the machine

// all function calls:
void analyzer(void);

void after_analizer(cJSON *root);

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
	ESP_LOGI("PROGRESS", "Restore count: %d, prediction: %d", count, prediction);

	analyzer();
}

/**
 * TASK
 * this task analizes the buffer and increases count or state of state_machine
 *  -> de-/increases count
 */
void analyzer(void)
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

		// uncomment if you dont want to  use the buffer
		if (fillSize)
		{
			if (root == NULL)
			{
				root = createJSON();
			}
			// bugfixing code:
			// Barrier_data firstEvent = buffer[head];
			// Barrier_data secondEvent = buffer[(head + 1) % SIZE_BUFFER];
			// Barrier_data thirdEvent = buffer[(head + 2) % SIZE_BUFFER];
			// Barrier_data fourthEvent = buffer[(head + 3) % SIZE_BUFFER];
			// ESP_LOGI("analyzer()", "in buffer:\n(%d,%d), (%d,%d), (%d,%d), (%d,%d)", firstEvent.id, firstEvent.state, secondEvent.id, secondEvent.state, thirdEvent.id, thirdEvent.state, fourthEvent.id, fourthEvent.state);
			// ESP_LOGI("analyzer()", "in buffer:\n(%d), (%d), (%d), (%d)", firstEvent.id, secondEvent.id, thirdEvent.id, fourthEvent.id);

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

				state_counter = 0;
			}
			else if (state_counter == -4)
			{ // OUT-GOING event

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
			// and go to deepsleep
			after_analizer(root);
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
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

	setCount_backup(count);

	if (root != NULL)
	{
		// send the stuff to mqtt
		char *msg = cJSON_PrintUnformatted(root);

		// ets_printf("\n sended message:\n%s\n", msg);
		sendToMQTT(msg, QOS_SAFE);
		free(msg);
		cJSON_Delete(root);
		root = NULL;
	}

	// check if we are having the reset hours -> reset counter
	now = time(NULL);
	now_tm = localtime(&now);

	// showRoomState
	prediction = getPrediction();
	displayCountPreTime(prediction, count);

	if ((now_tm->tm_hour == RESET_COUNT_HOUR2) && now_tm->tm_min < RESET_COUNT_MIN)
	{
		// go into deep sleep until 7 o clock, so for 8 hours
		const uint64_t sevenHours = 25200000000;
		ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));

		esp_deep_sleep(sevenHours);
	}

	deep_sleep_routine();
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
