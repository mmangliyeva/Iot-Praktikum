#ifndef MAIN_H
#define MAIN_H
// use this header to tune often parameters
// active whether the ESP32 is at my home or in the institue
// #define INSTITUTE
// initilize the display
#define WITH_DISPLAY

// for bugfixing pourpuses maybe you want to deactivate sending data to elastic
#define SEND_DATA

// after empting the whole buffer stay STAY_AWAKE seconds awake
// 1200 sec == 20 min
#define WAKEUP_AFTER 10 // sec

#define USE_WIFI
#define WIFI_SAVINGS // power savings for wifi
// #define WITH_CALCULATION // little bit of parrallel calcuation code
// #define DFS // dynamic frequency scaling
// #define ALS_DFS // automatic sleep and dynamic frquency scalling

#endif
