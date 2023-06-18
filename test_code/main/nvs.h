#ifndef MY_NVS_H
#define MY_NVS_H

void initMY_nvs(void);
void initNVS_json(uint8_t bool_openHandle);
void test_access(void);
void sendDataFromJSON_toDB(uint8_t bool_openHandle);
void writeToNVM(const char *sensorName, uint8_t peopleCount, int8_t state, time_t time);

#endif