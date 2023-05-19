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


//IoT Platform information
#define USER_NAME "group8" 
#define USERID 34  //to be found in the platform 
#define DEVICEID 82  //to be found in the platform
#define SENSOR_NAME "light_barrier" 
#define TOPIC "34/82/data" //userID_deviceID
#define JWT_TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODQzMzY3MjYsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQvODIifQ.x1Vv2qN0NfV2TTx9HGjTv1Qh8yxZfIJAvDPXK1R7dcRgsKMMqOS_NL7skBFqmEyhPe30Tnu4UQeH0OdXl7lOCGIkya-SKi4K2oVZYvByGQHoHToWuOpE-e62ohW0QCXMLq2Np5qV9zkaUb5BibQOt8qE7gzZKFAbjvYRPq9xyJAFeLVCii5160CevLP5ZOCcEN_6iOaq9kDyY-JaIHKssNMWuyo-X_j9Jz53-FzgLgB-omAjd1TP5DRQQS7RdgWDmd0BlfeNvPg-1rCt7gCDNo3uA_tpoUaNVPWYOe4jF5BFph6f_ZToQwEGGk_y5OTz9Y9c9Jw823ZHjm3FJ3PtcrhvdErJJkh8d2NausVIwCZ9llWUdFCT7-buMUF2Li-TRoG0OXvb2232n5BCv6Vc26kNhrJcBvD9gmjfyNVYso1r0EjsoMLSEmcRH8RbscPUcb5-p0rCUkCZNM9xF2-fH5Okq25wZQCsZpqUMugive6lYyZKGQ7e2cZerYEyiSfE70XvVpF5vsF83AYHFlWBS82gTZfAp_Dw-9Um3PoOzdEZmkhmD5csoAiX169acQat_rD2hZh8It4f7oE2KTouBSQUFrnBhDJKrWZO0HE7Dvhu9GwYbw-CvyE8x1UBokxVpY-Id1jyAki3Vm4ClUk2rTN15sayVbA_koVTJTkVtNA"
#define MQTT_SERVER "mqtt.caps-platform.live"//"138.246.236.181"
#define QOS 1 // sets the safty level of sending a mqtt message


// defines for the counting algorithm:
#define BUFF_STRING_COUNT 4 //size of the buffer to display the number
#define PIN_DETECT_1 4      // pin for outdoor barrier
#define PIN_DETECT_2 2      // pin for indoor barrier
#define THRESHOLD_DEBOUCE 15000 // for isr in mu_seconds
#define THRESHOLD_ANALIZER 4  	// when process starts analizing
#define PRIO_ANALIZER 10		// process prio
#define PRIO_SHOW_COUNT 7		// process prio
#define PRIO_IN_BUFFER 5       // process prio
#define SIZE_QUEUE 15			// xTaskQueue size
#define SIZE_BUFFER 10			// buffer size in analizer task
#define GOING_IN_EVENT 69 		// flag if detected go-in-event 
#define GOING_OUT_EVENT 42      // flag if detected go-out-event
#define NO_EVENT -1             // flag if detected no event
#define Y_POS_COUNT 32          // the y position where to display, count on lcd

void my_setup(void);


#endif
