#define I_AM_NATIVE_CODE
#include <windows.h>
#include <time.h>
#include "fbe/fbe_time.h"
#include "fbe/fbe_winddk.h"
#include "simulation_log.h"
#include "EmcPAL_Memory.h"
static csx_timestamp_frequency_t performance_frequency = 0;

fbe_time_t 
fbe_get_time()
{
    fbe_time_t            t = 0;
    csx_timestamp_value_t performance_count = 0;
    
    /* The first time we need to retreive the performance_frequency */
    if (performance_frequency == 0){
        csx_p_get_timestamp_frequency(&performance_frequency);
    }
    
    csx_p_get_timestamp_value(&performance_count);
    if (performance_frequency > 1000L) {
        t = performance_count / (performance_frequency / 1000L);
    } else {
        t = (performance_count * 1000L) / performance_frequency;
    }

    return t;
}
fbe_time_t
fbe_get_time_in_us()
{
    fbe_time_t t = 0;
    t = fbe_get_time();
    return (t * 1000L); /* Convert Milisecond into Microseconds */
}

fbe_status_t 
fbe_get_system_time(fbe_system_time_t * p_system_time)
{
    EMCUTIL_TIME_FIELDS system_time;

    EmcutilGetSystemTimeFields(&system_time);

    p_system_time->year = system_time.Year;
    p_system_time->month = system_time.Month;
    p_system_time->weekday = system_time.Weekday;
    p_system_time->day = system_time.Day;
    p_system_time->hour = system_time.Hour;
    p_system_time->minute = system_time.Minute;
    p_system_time->second = system_time.Second;
    p_system_time->milliseconds = system_time.Milliseconds;

    return FBE_STATUS_OK;
}

fbe_u32_t fbe_get_elapsed_seconds(fbe_time_t timestamp)
{
    fbe_time_t  delta_time = fbe_get_time();
    
    if (delta_time <= timestamp)
    {
        return 0;  // Don't allow a negative elapsed time.
    }
    else
    {
        delta_time = (delta_time - timestamp) / ((fbe_time_t)1000);
    }

    if (delta_time > (fbe_time_t)0xffffffff)
    {
        return(0xffffffff);
    }
    else
    {
        return((fbe_u32_t)delta_time);
    }
}
fbe_u32_t fbe_get_elapsed_milliseconds  (fbe_time_t timestamp)
{
    fbe_time_t  delta_time = fbe_get_time();
    
    if (delta_time <= timestamp)
    {
        return 0;  // Don't allow a negative elapsed time.
    }
    else
    {
        delta_time -= timestamp;
    }
    
    if (delta_time > (fbe_time_t)0xffffffff)
    {
        return(0xffffffff);
    }
    else
    {
        return((fbe_u32_t)delta_time);
    }
}

void log_formatter(char *buffer, const char *content)
{
    char date[32];
    EMCUTIL_TIME_FIELDS st;

    EmcutilGetSystemTimeFields(&st);
    EmcutilConvertTimeFieldsToLocalTime(&st);
    _strdate(date);

    sprintf(buffer, "%s %02u:%02u:%02u.%03u %s", date, st.Hour, st.Minute,
            st.Second, st.Milliseconds, content);
}
