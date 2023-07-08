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

#define WAKEUP_AFTER 10 // sec

#endif
