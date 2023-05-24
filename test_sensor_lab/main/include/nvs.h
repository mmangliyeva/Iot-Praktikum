#ifndef MY_NVS_H
#define MY_NVS_H


void initMY_nvs(void);
void initNVS_json(uint8_t bool_openHandle);
void test_access(void);
void sendDataFromJSON_toDB(void *args);
void writeToNVM(const char* sensorName, uint8_t peopleCount, int8_t state, time_t time);
uint8_t restoreCount(uint8_t bool_openHandle);
void resetCount(uint8_t bool_openHandle);
#endif