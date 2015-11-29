#ifndef BGSL_TIME_H
#define BGSL_TIME_H

#include "fbe/bgsl_types.h"

typedef bgsl_u64_t bgsl_time_t; /** 64 bit counter in miliseconds */ 

typedef struct bgsl_system_time_t {
  bgsl_u16_t year;
  bgsl_u16_t month;
  bgsl_u16_t weekday;
  bgsl_u16_t day;
  bgsl_u16_t hour;
  bgsl_u16_t minute;
  bgsl_u16_t second;
  bgsl_u16_t milliseconds;
} bgsl_system_time_t;

bgsl_time_t bgsl_get_time(void); /** retrieves the number of milliseconds that have elapsed since the system was started */
bgsl_status_t bgsl_get_system_time(bgsl_system_time_t * p_system_time);

#endif /* BGSL_TIME_H */
