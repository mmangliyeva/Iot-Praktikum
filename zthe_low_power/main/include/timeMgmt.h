#ifndef TIMEMGMT_H
#define TIMEMGMT_H

void initSNTP(void);
time_t get_timestamp(void); // returns the time in second
char *getDate(void);        // returns the current data as string
#endif
