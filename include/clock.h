#ifndef __DEVICE_CLOCK_H
#define __DEVICE_CLOCK_H

#include "stdint.h"

struct time{
	uint32_t sec;
	uint32_t min;
	uint32_t hour;
	uint32_t day;
	uint32_t month;
	uint32_t year;
};

int fmt_time(struct time *tm, char *buf, unsigned int size);
void get_clock(struct time *tm);
unsigned long mktime(const unsigned int year0, const unsigned int month0,
					const unsigned int day, const unsigned int hour,
					const unsigned int min, const unsigned int sec);
#endif
