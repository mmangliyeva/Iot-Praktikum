#ifndef MY_SETUP_H
#define MY_SETUP_H
// this header contains all parameter that does not change frequently

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"

// #include "main.h"

#ifdef INSTITUTE
#define WIFI_SSID "CAPS-Seminar-Room"
#define WIFI_PASS "caps-schulz-seminar-room-wifi"
#define SNTP_SERVER "ntp1.in.tum.de"
#else
#define WIFI_SSID "olchinet"
#define WIFI_PASS "hahaihropfer"
#define SNTP_SERVER "pool.ntp.org"
#endif

// IoT Platform information
#define USER_NAME "group8"
#define USERID 34   // to be found in the platform
#define DEVICEID 82 // to be found in the platform
#define SENSOR_NAME "light_barrier"
#define TOPIC "34/82/data" // userID_deviceID
#define JWT_TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODQ1MTc5MjMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQvODIifQ.Sx1yIYFaF8hRFcRXFooq4jB3VG4dePK_KcGoRm5lAtUgSEHyH8TSWRGNhzInor7qY63ZF0eum7d_eFrLbuRCftyQ8Y6OSij7C0Ck8ZfZKwR-AnxKVXtbAscDIEjucP22CxFa1UQyqpoOIh21EqfqM-K_HtMG_rZvTguZO6cG5BmuDdC1IPrV8LDFnhA_HGN98HDLZw3Ss6A4oFbSFY7zjDhr5DSUEht-izLUNv3lEEJhk9XkyJsVs4XzHyMsDovG623PZTzJZTwrnk2A0eUGAG-QcgSTkeaB8URpiCWszHaq_stixaUZwHZNSKpkNOoKEQcqVJtuiJ-q5hLdy2SwxzC69N6HK2hWnKCpB5y9STh4Qb4rQTZwgIxszyW-JJ0TNKZgbOdMSVMbAHlHhd17S8U-u05voYArYh08hv5kk1VVnn6_kmAqNRqxrkH4oEX_IUhv-YyXopovZQvN2j0OIjIGXGtTVoH2YK43R81k7otupf-F9SnQnJEni4SAA8Ewm0Q-eXNH7OwZijhfJtXveDubt-RdgSvihC7MqDnj3PO026b2_r07YsSHIC2r3Pfupjp16aGzfxIK8oCpcYkifWu7BebfrNWAYZo79b6I70r5wzYRiTMnZbYt_-_vN_rPsFH8hLrg7y0MTRQ9S1KOIfjlWQrzW-NPz_6vmdaZ8ok"
#define MQTT_SERVER "mqtt.caps-platform.live" //"138.246.236.181"
#define QOS_SAFE 1                            // sets the safty level of sending a mqtt message
#define QOS_FAST 0                            // sets the safty level of sending a mqtt message

// defines for the counting algorithm:
#define BUFF_STRING_COUNT 4 // size of the buffer to display the number
#define PIN_DETECT_1 2      // pin for outdoor barrier
#define PIN_DETECT_2 5      // pin for indoor barrier
#define PIN_TEST_MODE 19    // pin entering test mode
#define RED_INTERNAL_LED 16 // internal red led pin

#define THRESHOLD_DEBOUCE 1000             // 15000 // for isr in mu_seconds
#define THRESHOLD_DEBOUCE_TEST_MODE 200000 // for isr in mu_seconds
#define THRESHOLD_ANALIZER 4               // when process starts analizing

#define PRIO_ANALIZER 11  // process prio
#define PRIO_SHOW_COUNT 2 // process prio
#define PRIO_IN_BUFFER 10 // process prio
#define PRIO_TEST_MODE 4  // process prio
#define PRIO_SEND_NVS 3   // process prio
#define PRIO_OTA_TASK 1   // process prio

#define SIZE_QUEUE 15      // xTaskQueue size
#define SIZE_BUFFER 10     // buffer size in analizer task
#define GOING_IN_EVENT 69  // flag if detected go-in-event
#define GOING_OUT_EVENT 42 // flag if detected go-out-event
#define NO_EVENT -1        // flag if detected no event
#define Y_POS_COUNT 32     // the y position where to display, count on lcd

#define NO_OPEN_NVS -1
#define OPEN_NVS 1
// we reset between 5:00 - 5:15 and 23:00 - 23:15 the counter
#define RESET_COUNT_HOUR 5   // at what hour we reset the count
#define RESET_COUNT_HOUR2 23 // at what hour we reset the count
#define RESET_COUNT_MIN 15   // at what hour we reset the count
#define NEEDED_SPACE_NVS 400

#define TIME_TO_NEXT_EVENT 500000 // in mu_sec how close a sequence of events should be

void my_setup(void);

void displayCountPreTime(uint8_t prediction, uint8_t curCount);
void initTimer(void);
void error_message(const char *TAG, char *msg, const char *details);

extern uint8_t flag_internet_active;
extern char *tmp_message; // used for system report, imedetyl free it!
// test mode flag and task that should stop if testModeActive == 1
extern uint8_t testModeActive;
extern TaskHandle_t xProgAnalizer;
extern TaskHandle_t xProgShowCount;
extern TaskHandle_t xProgInBuffer;

extern TaskHandle_t xSendToMQTT;
extern TaskHandle_t xOTA;
#endif
