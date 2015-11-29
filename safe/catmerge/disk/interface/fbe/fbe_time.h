#ifndef FBE_TIME_H
#define FBE_TIME_H

#include "fbe/fbe_types.h"

typedef fbe_u64_t fbe_time_t; /** 64 bit counter in miliseconds */ 

typedef struct fbe_system_time_t {
  fbe_u16_t year;
  fbe_u16_t month;
  fbe_u16_t weekday;
  fbe_u16_t day;
  fbe_u16_t hour;
  fbe_u16_t minute;
  fbe_u16_t second;
  fbe_u16_t milliseconds;
} fbe_system_time_t;

/* Useful define since the below function fbe_get_time() returns milliseconds.
 */
#define FBE_TIME_MILLISECONDS_PER_SECOND 1000
#define FBE_TIME_MICROSECONDS_PER_SECOND 1000000
#define FBE_TIME_MILLISECONDS_PER_MICROSECOND 1000

/*
 * Other common time/date defines
 */
#define FBE_TIME_MINS_PER_HOUR          60
#define FBE_TIME_HOURS_PER_DAY          24
#define FBE_TIME_DAYS_PER_WEEK          7
#define FBE_TIME_MINS_PER_DAY           (FBE_TIME_MINS_PER_DAY * FBE_TIME_HOURS_PER_DAY)
#define FBE_TIME_LAST_DAY_OF_WEEK       (FBE_TIME_DAYS_PER_WEEK - 1)
#define FBE_TIME_LAST_HOUR_OF_DAY       (FBE_TIME_HOURS_PER_DAY - 1)
#define FBE_TIME_LAST_MIN_OF_HOUR       (FBE_TIME_MINS_PER_HOUR - 1)

fbe_time_t fbe_get_time(void); /** retrieves the number of milliseconds that have elapsed since the system was started */
fbe_time_t fbe_get_time_in_us(void); /* retrives the number of microseconds from the system start */
fbe_status_t fbe_get_system_time(fbe_system_time_t * p_system_time);
fbe_u32_t fbe_get_elapsed_seconds(fbe_time_t timestamp);
fbe_u32_t fbe_get_elapsed_milliseconds(fbe_time_t timestamp);

static __forceinline fbe_u32_t fbe_get_elapsed_microseconds(fbe_time_t timestamp)
{
    fbe_time_t  delta_time = (fbe_get_time_in_us() - timestamp);

    if (delta_time > (fbe_time_t)0xffffffff)
    {
        return(0xffffffff);
    }
    else
    {
        return((fbe_u32_t)delta_time);
    }
}

void log_formatter(char *buffer, const char *content);

#endif /* FBE_TIME_H */
