#ifndef WEB_F_H
#define WEB_F_H

void init_web_functions(void);
uint8_t getCount_backup(void);       // returns count from IoT-platform
void setCount_backup(uint8_t count); // backups count to IoT-platform
void systemReport(const char *msg);  // saves the msg in the IoT-platfrom error-messages-array
uint8_t getPrediction(void);         // fetches the prediction from IoT-platform
#endif
