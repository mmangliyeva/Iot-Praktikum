#ifndef MY_NVS_H
#define MY_NVS_H

void initMY_nvs(void);                               // inits the nvs
void initNVS_json(uint8_t bool_openHandle);          // inits a json-file in the nvs
void test_access(void);                              // bugfixing function
void sendDataFromJSON_toDB(uint8_t bool_openHandle); // send all the data in the nvs to elastic search DB
// writes following parameter to the nvs. set state -1 to send only count
void writeToNVM(const char *sensorName, uint8_t peopleCount, int8_t state, time_t time);

#endif