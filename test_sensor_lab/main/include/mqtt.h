#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

void initMQTT(void);
void sendToSensorCounter(uint8_t countPeople,time_t time);
#ifdef SEND_EVERY_EVENT
void sendToSensorBarrier(const char* barrierName, uint8_t countPeople, time_t time, uint8_t state);
#endif
extern esp_mqtt_client_handle_t mqttClient;
extern EventGroupHandle_t mqtt_event_group;


#endif
