/*
 * fbe_envstats.h
 */
#ifndef FBE_ENVSTATS__H
#define FBE_ENVSTATS__H

#include "fbe/fbe_trace_interface.h"

#ifdef C4_INTEGRATED
typedef struct
{
  uint TemperatureMSB;
  uint TemperatureLSB;
  BOOL negativeTemp;
} Temperature;

/*
 * If an error occurs on the read of the temp, don't want to print 
 * a trace message every failure, since the temp could be polled once a 
 * second. Assume worst case of 1 second, no way to know at what rate
 * will be polled.
 */
#define FBE_RD_TEMP_TRACE_DELAY    120   

BOOL getSpTemp (Temperature * temp);
#endif
void fbe_envstats_trace(fbe_trace_level_t trace_level, const char * fmt, ...);

#endif
