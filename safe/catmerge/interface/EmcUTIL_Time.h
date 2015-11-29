#ifndef EMCUTIL_TIME_H_
#define EMCUTIL_TIME_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*!
 * @file EmcUTIL_Time.h
 * @addtogroup emcutil_time
 * @{
 * @brief
 *
 * EmcUTIL supports "SystemTime" and "TimeFields":
 * - EMCUTIL_SYSTEM_TIME = time in 100ns units since 1/1/1601 (on Windows), 1/1/1970 (on CSX)
 * - EMCUTIL_TIME_FIELDS = year, month, day, hour, etc. suitable for printing
 *
 * OS Cross-reference (please consult MSDN and CSX documentation for the OS-specific details)
 *
 * -# Windows 
 *  -# user-space "FILETIME" = Number of 100ns units since January 1, 1601 (UTC)
 *  -# user-space "SYSTEMTIME" = time fields (year, month, day, hour, etc.) - supports years since 1601
 *  -# kernel-space time returned by KeQuerySystemTime as a LARGE_INTEGER = Number of 100ns units since January 1, 1601 (UTC)
 *  -# kernel-space "TIME_FIELDS" = time fields (year, month, day, hour, etc.) - supports years since 1601
 *  -# supports time zones and the concept for LocalTime vs. SystemTime
 * -# CSX 
 *  -# "csx_precise_time_t" or "csx_time_t" = time in microsec/sec since 1970
 *  -# "csx_date_t" = time fields (year, month, day, hour, etc.) - supports years since 1900
 *  -# does not support time zones; LocalTime == SystemTime
 *
 */

#include "EmcUTIL.h"

CSX_CDECLS_BEGIN

////////////////////////////////////

 /*! @brief Time in units of 100s of nanoseconds */
typedef csx_s64_t EMCUTIL_SYSTEM_TIME, *PEMCUTIL_SYSTEM_TIME; /*!< Time in units of 100s of nanoseconds */

/*! @brief Represents time in Year, Month, ... Hour, Minute, Seconds... */ 
typedef struct
{
    csx_nsint_t Year;           /*!< 1900-n, CSX only supports Years >= 1900 */
    csx_nsint_t Month;          /*!< 1-12 */
    csx_nsint_t Day;            /*!< 1-31 */
    csx_nsint_t Hour;           /*!< 0-23 */
    csx_nsint_t Minute;         /*!< 0-59 */
    csx_nsint_t Second;         /*!< 0-59 */
    csx_nsint_t Milliseconds;   /*!< 0-999 */
    csx_nsint_t Weekday;        /*!< 0-6, where 0=Sunday and 6=Saturday */
}
EMCUTIL_TIME_FIELDS;

/*!
 * @brief
 *      Obtain system time in 100ns units since 1/1/1601 (on Windows), 1/1/1970 (on CSX).
 *
 * @return
 *      Nothing
 *
 */
EMCUTIL_API void EMCUTIL_CC EmcutilGetSystemTime(EMCUTIL_SYSTEM_TIME *systemTime /*!< Pointer to caller-allocated structure for time information */
                                                 );


/*!
 * @brief
 *      Obtain system time in time fields. Time is expressed in standard UTC.
 *
 * @return
 *      Nothing
 *
 */
EMCUTIL_API void EMCUTIL_CC EmcutilGetSystemTimeFields(EMCUTIL_TIME_FIELDS *pTime /*!< Pointer to caller-allocated structure for time information */
                                                       );

/*!
 * @brief
 *      Sets system time to the specified time/date as supplied by the caller.
 *
 * @return
 *      EMCUTIL_SUCCESS if successful, else an error code.
 *
 */
EMCUTIL_API EMCUTIL_STATUS EMCUTIL_CC EmcutilSetSystemTimeFields(EMCUTIL_TIME_FIELDS *pTime /*!< Filled in by caller. System time will be set to this time/date. */
                                                                 );

//**********************************************************************
/*!
 *  @brief
 *  Convert system time, expressed as time fields, to adjust for local time.
 *  Replaces Windows APIs such as GetLocalTime(), GetTimeZoneInformation(), and TzSpecificLocalTimetoSystemTime().
 *
 *  @pre
 *  timeFields points to caller-specified EMCUTIL_TIME_FIELDS structure
 *
 *  @post
 *  Structure at modified to adjust for local time
 * 
 *  @note
 *  CSX does not support time zones, so this function is a no-op on CSX
 *
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertTimeFieldsToLocalTime(EMCUTIL_TIME_FIELDS *timeFields /*!< Pointer to caller-supplied time */
                                                                );

//**********************************************************************
/*!
 *  @brief
 *  Convert system time, expressed as time fields, from local time to "system" or UTC time.
 *
 *  @pre
 *  timeFields points to caller-specified EMCUTIL_TIME_FIELDS structure
 *
 *  @post
 *  Structure at pTime modified to adjust for UTC
 * 
 *  @note
 *  CSX does not support time zones, so this function is a no-op on CSX
 *
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertTimeFieldsFromLocalTime(EMCUTIL_TIME_FIELDS *timeFields /*!< Pointer to caller-allocated structure for time information */
                                                                  );

//**********************************************************************
/*!
 *  @brief
 *  Convert time, expressed as 100s of nanoseconds, into date/time fields.
 *  Note that to obtain the current system time directly in time fields, use the EmcutilGetSystemTimeFields() API.
 *
 *  @pre
 *  time points to time as previously obtained by caller
 *
 *  @post
 *  timeFields is filled in with the corresponding values in date/time format.
 * 
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertToTimeFields(EMCUTIL_SYSTEM_TIME *time,
                                            EMCUTIL_TIME_FIELDS *timeFields
                                            );

//**********************************************************************
/*!
 *  @brief
 *  Convert time, expressed as date/time fields, into 100s of nanoseconds.
 *  Note that to obtain the current system time directly 100s of nanoseconds, use the EmcutilGetSystemTime() API.
 *
 *  @pre
 *  timeFields points to time as previously obtained by caller
 *
 *  @post
 *  time is filled in with the corresponding time value in 100s of nanoseconds.
 * 
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertTimeFieldsToTime(EMCUTIL_TIME_FIELDS *timeFields,
                                                EMCUTIL_SYSTEM_TIME *time
                                                );

//**********************************************************************
/*!
 *  @brief
 *  Convert time to adjust for local time zone.
 *
 *  @pre
 *  SystemTime points to a timestamp previously obtained by caller, using an API such as EmcutilGetSystemTime().
 *
 *  @post
 *  LocalTime is filled in with the corresponding time adjusted for local time.
 * 
 *  @note
 *  CSX does not support time zones, so this function is a no-op on CSX
 *
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertToLocalTime(EMCUTIL_SYSTEM_TIME *SystemTime,
                                           EMCUTIL_SYSTEM_TIME *LocalTime
                                           );

//**********************************************************************
/*!
 *  @brief
 *  Convert time to adjust for local time zone.
 *
 *  @pre
 *  SystemTime points to a timestamp previously obtained by caller, which was adjusted for local time.
 *
 *  @post
 *  LocalTime is filled in with the corresponding time adjusted for local time.
 * 
 *  @note
 *  CSX does not support time zones, so this function is a no-op on CSX
 *
 *  @return
 *  void
 */
EMCUTIL_API void EMCUTIL_CC EmcutilConvertFromLocalTime(EMCUTIL_SYSTEM_TIME *LocalTime,
                                             EMCUTIL_SYSTEM_TIME *SystemTime
                                             );

//**********************************************************************
/*!
 *  @brief
 *  Compare two timestamps.
 *
 *  @return
 *  - -1 if timeFields1 < timeFields2
 *  -  0 if timeFields1 == timeFields2
 *  -  1 if timeFields1 > timeFields2
 */
EMCUTIL_API  csx_nsint_t EMCUTIL_CC EmcutilCompareSystemTime(EMCUTIL_SYSTEM_TIME *systemTime1, 
                                                             EMCUTIL_SYSTEM_TIME *systemTime2
                                                             );

//**********************************************************************
/*!
 *  @brief
 *  Compare two timestamps expressed in "time fields" format.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *  - -1 if timeFields1 < timeFields2
 *  -  0 if timeFields1 == timeFields2
 *  -  1 if timeFields1 > timeFields2
 */
EMCUTIL_API  csx_nsint_t EMCUTIL_CC EmcutilCompareTimeFields(EMCUTIL_TIME_FIELDS *timeFields1, 
                                                             EMCUTIL_TIME_FIELDS *timeFields2
                                                             );

//**********************************************************************
/*!
 *  @brief
 *  Sleep for N milliseconds.
 *
 *  @return
 *  - Nothing
 */
EMCUTIL_API void EMCUTIL_CC EmcutilSleep(int msecs);


////////////////////////////////////

CSX_CDECLS_END


/*!
 * @brief 
 * Convert from milliseconds to NT (kernel) time units which are expressed in hundreds of nanoseconds
 */
#define EMCUTIL_CONVERT_MSECS_TO_100NSECS(_ms) (_ms * CSX_XSEC_PER_MSEC)

/*!
 * @brief 
 * This macro converts the time values in units of 100nSec that are  returned by Windows to the standard Emcpal units of milliseconds.
 */
#define EMCUTIL_CONVERT_100NSECS_TO_MSECS(_ns) (_ns / CSX_XSEC_PER_MSEC)

/*!
 * @brief 
 * This macro gives a continous counter of msecs elapsed since start time
 */
CSX_STATIC_INLINE
ULONG
EmcutilGetTickCount(VOID)
{
    csx_u64_t clock_time_msecs;
    csx_p_get_clock_time_msecs(&clock_time_msecs);
    return (ULONG) clock_time_msecs;
}

/*!
 * @brief 
 * This macro gives access to the processor timestamp counter
 */
CSX_STATIC_INLINE
ULONGLONG
EmcutilGetTimestampCounter(ULONGLONG *TimestampFrequency)
{       
    csx_timestamp_value_t timestamp;
    csx_p_get_timestamp_value(&timestamp);
    if (TimestampFrequency != CSX_NULL)
    {
        csx_p_get_timestamp_frequency(TimestampFrequency);
    }
    return(timestamp);
}

//++
//.End file EmcUTIL_Time.h
//--

/*!
 * @} end group emcutil_time
 * @} end file EmcUTIL_Time.h
 */

#endif                                     /* EMCUTIL_TIME_H_ */
