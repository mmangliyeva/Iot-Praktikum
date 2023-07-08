#ifndef CAPS_OTA_H
#define CAPS_OTA_H

void init(void);

int ota_update(void);

void set_bearer_token(const char* token, size_t size);

#endif

