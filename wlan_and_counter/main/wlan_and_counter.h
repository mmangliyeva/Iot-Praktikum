#ifndef MAIN_H
#define MAIN_H

// function declartion
void analyzer(void* args);
void pushInBuffer(void* args);
void IRAM_ATTR isr_barrier1(void* args);
void IRAM_ATTR isr_barrier2(void* args);
void showRoomState(void* args);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif