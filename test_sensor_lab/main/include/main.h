#ifndef MAIN_H
#define MAIN_H
// use this header to tune often parameters
#define INSTITUTE
#define WITH_DISPLAY
// here it is possible to activate or deactivate that 
//  the esp32 sends every event to the elastic search database
//  if one uncomment following line, esp32 sends only the count
// #define SEND_EVERY_EVENT

// for bugfixing pourpuses maybe you want to deactivate sending data
#define SEND_DATA


// delay the sending of current count of people
#define PUT_IN_NVS_DELAY 1 //Seconds, write to nvs count
#define SEND_DELAY_FROM_NVS 300
#define UPDATE_OTA 5 //seconds

#endif
