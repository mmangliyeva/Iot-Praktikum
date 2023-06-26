#ifndef WEB_F_H
#define WEB_F_H

void init_web_functions(void);
uint8_t getCount_backup(void);
void setCount_backup(uint8_t count);
void systemReport(const char *msg);
uint8_t getPrediction(void);
#endif
