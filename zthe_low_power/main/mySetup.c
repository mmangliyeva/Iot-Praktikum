#include "mySetup.h"
#include "wifi.h"
#include "timeMgmt.h"
#include "mqtt.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "webFunctions.h"
#include "epaperInterface.h"
#include "wakeup.h"

// deep sleep stuff
#include "esp32/rom/rtc.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
// power managment
#include "esp_pm.h"
#include "powerMesurment.h"
#include "esp32/clk.h"
// #include "esp_private/esp_clk.h"
#include "esp_timer.h"

#ifdef WITH_DISPLAY
void initDisplay(void);
#endif
void initPins(void);

QueueHandle_t queue = NULL;
SemaphoreHandle_t xInternetActive = NULL;
// ----- init buffer -----
RTC_NOINIT_ATTR uint8_t head;
RTC_NOINIT_ATTR uint8_t fillSize;
RTC_NOINIT_ATTR Barrier_data buffer[SIZE_BUFFER];
// for time in deepsleep
RTC_NOINIT_ATTR time_t timeOffset = 0;

// TASKs
void initInternet(void *args);
void IRAM_ATTR isr_barrier(void *args);
void pushInBuffer(void *args);
// helpers
void inintDFS_or_ALS(void);

void my_setup(void)
{
	inintDFS_or_ALS();
	esp_set_deep_sleep_wake_stub(&wakeup_routine);
	initPins();
	// if (rtc_get_reset_reason(0) == DEEPSLEEP_RESET)
	// {
	// 	// Measurment 6.
	// 	stopMeasure();
	// }
	// Measurments 1.
	// startMeasure();

#ifdef WITH_DISPLAY
	initDisplay(); // int external display

#endif

	if (rtc_get_reset_reason(0) != DEEPSLEEP_RESET)
	{
		displayCountPreTime(0, 0);
		// go to deepsleep IF we did not woke up from deepsleep
		// startMeasure();
		deep_sleep_routine(WAKEUP_AFTER);
	}
	// continue here if ESP started from deepsleep
	//  quite complex code, that init internet in a task AND
	//  pushes simoultanoiusly events in the buffer
	xInternetActive = xSemaphoreCreateBinary();
	queue = xQueueCreate(SIZE_QUEUE, sizeof(Barrier_data));
#ifdef USE_WIFI
	xTaskCreate(initInternet, "initInternet", 6000, NULL, 15, NULL);
#else
	xSemaphoreGive(xInternetActive);
#endif

	xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, 3, NULL);
}

/**
 * TASK
 * that inits the internet
 */
void initInternet(void *args)
{
	initWifi();
	initSNTP();
#ifdef SEND_DATA
	initMQTT();
#endif
	xSemaphoreGive(xInternetActive);
	vTaskSuspend(NULL); // suspend own task
}

/**
 * inits automatic light sleep ALS or
 * dynamic frequency scaling DFS
 */
void inintDFS_or_ALS(void)
{
#ifdef ALS_DFS

	esp_pm_config_esp32_t pm_config = {
		.max_freq_mhz = 240,
		.min_freq_mhz = 80,
		.light_sleep_enable = true};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif

#ifdef DFS
	esp_pm_config_esp32_t pm_config = {
		.max_freq_mhz = 240,
		.min_freq_mhz = 80,
		.light_sleep_enable = false};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif
}

/**
 * this function goes to deepsleep for WAKEUP_AFTER seconds
 */
void deep_sleep_routine(uint32_t seconds)
{

	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
	head = 0;
	fillSize = 0;

	ESP_LOGI("PROGRESS", "Enter deep sleep");
	timeOffset = get_timestamp();
	esp_deep_sleep(1000000 * seconds);
}

void initPins(void)
{
	ESP_LOGI("PROGRESS", "Initializing pins");

	// setup pins:
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

	// init light barrier 1
	// gpio_pad_select_gpio(OUTDOOR_BARRIER);
	ESP_ERROR_CHECK(gpio_set_direction(OUTDOOR_BARRIER, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(OUTDOOR_BARRIER));
	ESP_ERROR_CHECK(gpio_pullup_dis(OUTDOOR_BARRIER));

	// gpio_set_intr_type(OUTDOOR_BARRIER, GPIO_INTR_ANYEDGE);

	// init light barrier 2
	// gpio_pad_select_gpio(INDOOR_BARRIER);
	ESP_ERROR_CHECK(gpio_set_direction(INDOOR_BARRIER, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(INDOOR_BARRIER));
	ESP_ERROR_CHECK(gpio_pullup_dis(INDOOR_BARRIER));
	// gpio_set_intr_type(INDOOR_BARRIER, GPIO_INTR_ANYEDGE);

	// wake up button
	ESP_ERROR_CHECK(gpio_set_direction(WAKE_UP_BUTTON, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(WAKE_UP_BUTTON));
	ESP_ERROR_CHECK(gpio_pullup_dis(WAKE_UP_BUTTON));

	const uint64_t ext_wakeup_pin_1_mask = 1ULL << OUTDOOR_BARRIER;
	const uint64_t ext_wakeup_pin_2_mask = 1ULL << INDOOR_BARRIER;
	const uint64_t ext_wakeup_pin_3_mask = 1ULL << WAKE_UP_BUTTON;
	esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask | ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

	// display power
	ESP_ERROR_CHECK(gpio_set_direction(DISPLAY_POWER, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));

	// LED measurment
	// ESP_ERROR_CHECK(gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT));
	// ESP_ERROR_CHECK(gpio_set_level(LED_PIN, 1));

	// // pins for measurment
	// ESP_ERROR_CHECK(gpio_set_direction(TOGGL_ENERGY_PIN, GPIO_MODE_OUTPUT));
	// ESP_ERROR_CHECK(gpio_set_level(TOGGL_ENERGY_PIN, 0));
	// ESP_ERROR_CHECK(gpio_set_direction(REST_ENERGY_PIN, GPIO_MODE_OUTPUT));
	// ESP_ERROR_CHECK(gpio_set_level(REST_ENERGY_PIN, 0));

	// set isr to pin:
	//(void*)OUTDOOR_BARRIER
	gpio_isr_handler_add(OUTDOOR_BARRIER, isr_barrier, (void *)OUTDOOR_BARRIER);
	gpio_set_intr_type(OUTDOOR_BARRIER, GPIO_INTR_ANYEDGE);
	gpio_isr_handler_add(INDOOR_BARRIER, isr_barrier, (void *)INDOOR_BARRIER);
	gpio_set_intr_type(INDOOR_BARRIER, GPIO_INTR_ANYEDGE);
}

// ------------------------------------------------------------
// ----------------------Display operations--------------------
// ------------------------------------------------------------

#ifdef WITH_DISPLAY
/**
 * init the external hardware display
 */
void initDisplay(void)
{
	ESP_LOGI("PROGRESS", "Initializing display");
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 1));
	epaperInit();

	// epaperUpdate();
	// ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
}
/**
 * updates the count or prediction on the external hardware display
 */
#endif
void displayCountPreTime(uint8_t prediction, uint8_t curCount)
{
#ifdef WITH_DISPLAY
	ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 1));

	epaperClear();

	char count_str[6];
	char prediction_str[6];
	char time_str[16];
	time_t now;
	struct tm *now_tm;
	sprintf(count_str, "%02d", curCount);
	sprintf(prediction_str, "%02d", prediction);

	time(&now);
	now_tm = localtime(&now);
	sprintf(time_str, "%02d o'clock", now_tm->tm_hour);

	epaperShow(10, 0, "G8", 4);
	epaperShow(80, 5, time_str, 2);

	epaperShow(10, 100, count_str, 4);
	epaperShow(80, 100, prediction_str, 3);

	epaperShow(117, 145, "z", 4);
	epaperShow(107, 155, "z", 2);
	epaperShow(10, 160, "I'm sleeping z", 1);
	epaperUpdate();
	// vTaskDelay(1000 / portTICK_PERIOD_MS);
	// free(now_tm);
	// ESP_ERROR_CHECK(gpio_set_level(DISPLAY_POWER, 0));
#endif
}

// ------------------------------------------------------------
// ----------------------Buffer operations---------------------
// ------------------------------------------------------------

/**
 * TASK
 * puts form the xQueue the elements in the buffer
 */
void pushInBuffer(void *args)
{
	// runs this tasks as long as we wait for connecting to wifi
	while (xSemaphoreTake(xInternetActive, 0) != pdTRUE)
	{
		if (fillSize < SIZE_BUFFER)
		{
			if (xQueueReceive(queue, buffer + ((head + fillSize) % SIZE_BUFFER), (TickType_t)5))
			{
				ESP_LOGI("pushInBuffer()", "id: %d time %ld", buffer[((head + fillSize) % SIZE_BUFFER)].id,
						 (long)buffer[((head + fillSize) % SIZE_BUFFER)].time);
				fillSize++;
			}
		}
		else
		{
			ESP_LOGE("PROGRESS", "Buffer is full increase SIZE_BUFFER!");
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	xSemaphoreGive(xInternetActive);

	vTaskSuspend(NULL); // suspend own task
}

// ISR routine douring wakup time, while wifi inits...
void IRAM_ATTR isr_barrier(void *args)
{
	Barrier_data data = {(int)args, get_timestamp()};
	xQueueSendFromISR(queue, &data, (TickType_t)0);
}

// test code --------------------------------------------------------------
// ------------------------------------------------------------------------
int testData_ingoing(int i)
{
	buffer[i] = (Barrier_data){OUTDOOR_BARRIER, get_timestamp()};
	buffer[i + 1] = (Barrier_data){INDOOR_BARRIER, get_timestamp()};
	buffer[i + 2] = (Barrier_data){OUTDOOR_BARRIER, get_timestamp()};
	buffer[i + 3] = (Barrier_data){INDOOR_BARRIER, get_timestamp()};
	fillSize += 4;
	return i + 4;
}
int testData_outgoing(int i)
{

	buffer[i] = (Barrier_data){INDOOR_BARRIER, get_timestamp()};
	buffer[i + 1] = (Barrier_data){OUTDOOR_BARRIER, get_timestamp()};
	buffer[i + 2] = (Barrier_data){INDOOR_BARRIER, get_timestamp()};
	buffer[i + 3] = (Barrier_data){OUTDOOR_BARRIER, get_timestamp()};
	fillSize += 4;
	return i + 4;
}
