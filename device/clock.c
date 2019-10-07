#include "clock.h"
#include "io.h"
#include "stdio.h"
#include "kernelio.h"

#define TM_SIZE 20  ///< 默认格式串长度
static const char *tm_fmt_str = "%d/%02d/%02d %02d:%02d:%02d"; ///< 默认格式字符串

/** @brief The base I/O port of the CMOS. */
#define RTC_PORT(x) (0x70 + (x))

/** @brief Offset in the CMOS for the current number of seconds. */
#define RTC_SECONDS			 0

/** @brief Offset in the CMOS for the current number of minutes. */
#define RTC_MINUTES			 2

/** @brief Offset in the CMOS for the current number of hours. */
#define RTC_HOURS			   4

/** @brief Offset in the CMOS for the current day of the week. */
#define RTC_DAY_OF_WEEK		 6

/** @brief Offset in the CMOS for the current day of the month. */
#define RTC_DAY_OF_MONTH		7

/** @brief Offset in the CMOS for the current month. */
#define RTC_MONTH			   8

/**
 * @brief Offset in the CMOS for the current year.
 * 
 * A one digit value means before 2000, and a two-digit value,
 * i.e. >= 100, is after the year 2000.
 */
#define RTC_YEAR				9

/** @brief Assume that a two-digit year is after 2000 */
#define CMOS_YEARS_OFFS		 2000

/** @brief Offset in CMOS for the status A register. */
#define RTC_STATUS_A		10

/** @brief Offset in CMOS for the status B register. */
#define RTC_STATUS_B			11

/** @brief Update in progress flag. */
#define RTC_UIP				 0x80

/** @brief Daylight savings flag.  */
#define RTC_DLS				 0x01

/** @brief 24 hour mode flag. */
#define RTC_24H				 0x02

/** @brief Time/date in binary/BCD flag. */
#define RTC_BCD				 0x04

/**
 * bcd码转二进制码
 */
static inline unsigned char bcd2bin(unsigned char val){

	return (val & 0xf) + (val >> 4) * 10;
}

/**
 * 读取cmos端口值
 */
static inline unsigned char read_cmos(unsigned char addr){
	
	outb(RTC_PORT(0), addr);
	return inb(RTC_PORT(1));
}

/**
 * 合成时间戳，来自linux kernel.
 */
unsigned long mktime(const unsigned int year0, const unsigned int month0,
					const unsigned int day, const unsigned int hour,
					const unsigned int min, const unsigned int sec){

	unsigned int month = month0, year = year0;
	
	/* 1..12 -> 11,12,1..10 */
	if( 0 >= (int) (month -= 2))
	{
		month += 12; /* Puts Feb last since it has leap day */
		year -= 1;
	}
	
	return ((((unsigned long)
				(year/4 - year/100 + year/400 + 367*month/12 + day) +
				year*365 - 719499
			)*24 + hour /* now have hours */
		)*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
}

/**
 * 获取当前时间，结果存放在tm中
 */
void get_clock(struct time *tm){

	/* 确保此时所有时间点都已经更新完毕 */
	while((read_cmos(RTC_STATUS_A) & RTC_UIP));
	
	/* Read the date/time values from the CMOS. */
	tm->sec   = read_cmos(RTC_SECONDS);
	tm->min   = read_cmos(RTC_MINUTES);
	tm->hour  = read_cmos(RTC_HOURS);
	tm->day   = read_cmos(RTC_DAY_OF_MONTH);
	tm->month = read_cmos(RTC_MONTH);
	tm->year  = read_cmos(RTC_YEAR);

	/** 确认是否是bcd码形式 */

//	if( (read_cmos(RTC_STATUS_B) & RTC_BCD) )
//	bochs好像不支持这个
	{
		tm->sec =  bcd2bin(tm->sec);
		tm->min =  bcd2bin(tm->min);
		tm->hour = bcd2bin(tm->hour);
		tm->day  = bcd2bin(tm->day);
		tm->month= bcd2bin(tm->month);
		tm->year = bcd2bin(tm->year);
	}
	
	if(tm->year < 100)
	{
		tm->year += CMOS_YEARS_OFFS;
	}
}

/**
 * 生成时间字符串
 */
int fmt_time(struct time *tm, char *buf, unsigned int size){

	if(size < TM_SIZE){
		return -1;
	}
	sprintf(buf, tm_fmt_str, tm->year,
				tm->month,
				tm->day,
				tm->hour,
				tm->min,
				tm->sec
		   );
	return 0;
}
