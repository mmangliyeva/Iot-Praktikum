#ifndef MY_NVS_H
#define MY_NVS_H


void initMY_nvs(void);
void initNVS_json(void);
void test_access(void);
void writeToNVM(const char* sensorName, uint8_t peopleCount, int8_t state, time_t time);
#endif