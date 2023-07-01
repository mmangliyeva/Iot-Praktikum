#ifndef MAIN_H
#define MAIN_H
// use this header to tune often parameters
// active whether the ESP32 is at my home or in the institue
#define INSTITUTE
// initilize the display
#define WITH_DISPLAY

// DO NOT UNCOMMENT following define
// here it is possible to activate or deactivate that
//  the esp32 sends every event to the elastic search database
//  if one uncomment following line, esp32 sends only the count
// #define SEND_EVERY_EVENT

// for bugfixing pourpuses maybe you want to deactivate sending data to elastic
#define SEND_DATA

// delay the sending of current count of people
#define SEND_COUNT_DELAY 300 // seconds 300
// checks every UPDATE_OTA seconds to update OTA
#define UPDATE_OTA 150 // seconds 150
// after  TIME_TO_EMPTY_BUFFER sec empty the buffer...
#define TIME_TO_EMPTY_BUFFER 300

#endif
