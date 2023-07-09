#ifndef WEB_F_H
#define WEB_F_H

uint8_t getCount_backup(void);       // returns count from IoT-platform
void setCount_backup(uint8_t count); // backups count to IoT-platform
uint8_t getPrediction(void);         // fetches the prediction from IoT-platform
#endif
