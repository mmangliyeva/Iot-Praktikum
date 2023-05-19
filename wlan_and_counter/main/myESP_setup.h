#ifndef SETUP_HEADER
#define SETUP_HEADER

#include <stdio.h>
#include <inttypes.h>           // for using int64_t
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"      // to create tasks
#include "freertos/semphr.h"    // for using sempahores/mutex
#include "freertos/queue.h"     // for using inter-task queues
#include "freertos/event_groups.h"  // for using wi-fi
#include "esp_wifi.h"               // for using wi-fi
#include "nvs_flash.h"              // for fixing a wifi-bug
#include "mqtt_client.h"            // for using MQTT
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "ssd1306.h"                // for using the led-display
#include "wlan_and_counter.h"       // here are some functions, we use

#define BUFF_STRING_COUNT 4     //size of the buffer to display the number
#define PIN_DETECT_1 4          // pin for barrier 1
#define PIN_DETECT_2 2          // pin for barrier 2
#define THRESHOLD_DEBOUCE 50000 // for isr 
#define THRESHOLD_ANALIZER 4  	// how many events should be in the buffer until we start analizing
#define PRIO_ANALIZER 10		// process priority
#define PRIO_SHOW_COUNT 1		// process priority
#define PRIO_IN_BUFFER 10       // process priority
#define SIZE_QUEUE 20			// xTaskQueue size
#define SIZE_BUFFER 10			// buffer size in analizer task
#define GOING_IN_EVENT 69 		// Flag that is set if we found an going-in evnet
#define GOING_OUT_EVENT 42      // Flag that is set if we found an going-out evnet
#define NO_EVENT -1             // Flag that is set if we found no event
#define Q0S 1            // 0 for just sending message, 1 for sending and getting a confirmation

/* These settings are taken from the Kconfig file */
#define MY_WIFI_SSID ""
#define MY_WIFI_PASS ""
/* Bits to determine the result of the connection process*/
#define MY_WIFI_CONN_BIT BIT0
#define MY_WIFI_FAIL_BIT BIT1
/* FreeRTOS event group to signal when we are connected */


// call this function to setup up your ESP32
void setup_myESP(void);

uint64_t get_timestamp(void);




extern int wifi_conn_retry_num, wifi_conn_max_retry;
extern EventGroupHandle_t my_wifi_event_group;
extern esp_mqtt_client_handle_t my_client;
// mach nur den pointer darauf extern !!!
extern esp_mqtt_client_config_t my_mqtt_client_cfg; 
#endif
