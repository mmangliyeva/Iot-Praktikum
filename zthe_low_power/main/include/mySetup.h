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

#include "main.h"

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

#define OUTDOOR_BARRIER 25    // pin for outdoor barrier
#define INDOOR_BARRIER 26     // pin for indoor barrier
#define OUTDOOR_BARRIER_RTC 6 // pin for outdoor barrier
#define INDOOR_BARRIER_RTC 7  // pin for indoor barrier

#define DISPLAY_POWER 18

#define PIN_TEST_MODE 19 // pin entering test mode

#define PRIO_ANALIZER 11  // process prio
#define PRIO_IN_BUFFER 10 // process prio

#define SIZE_QUEUE 5   // xTaskQueue size
#define SIZE_BUFFER 80 // buffer size in analizer task

// we reset between 5:00 - 5:15 and 23:00 - 23:15 the counter
#define RESET_COUNT_HOUR2 23 // at what hour we reset the count
#define RESET_COUNT_MIN 15   // at what hour we reset the count

#define SIZE_OF_MESSAGE 100

void my_setup(void);
// display on the hardware display count and prediction
void displayCountPreTime(uint8_t prediction, uint8_t curCount);

// all processes globally because for test-mode
extern TaskHandle_t xProgAnalizer;
extern TaskHandle_t xProgInBuffer;

#endif
