#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

void initMQTT(void);
void sendToMQTT(const char *msg, uint8_t qos); // sends msg to elastic search

extern esp_mqtt_client_handle_t mqttClient;
extern EventGroupHandle_t mqtt_event_group;

#endif
