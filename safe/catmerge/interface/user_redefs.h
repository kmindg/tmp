#ifndef USER_REDEFS_H
#define USER_REDEFS_H
/***********************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***********************************************************************/
/***************************************************************************
 * user_redefs.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file defines the standard types.
 *
 * NOTES:
 *
 * HISTORY:
 *      12-July-2011 - Vaibhav Gaonkar  - Created
 ***************************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV

#ifdef __cplusplus
extern "C" {
#endif

#define WINBASEAPI
#define WINAPI __stdcall
    //
//  File System time stamps are represented with the following structure:
//
#ifndef _FILETIME_
typedef struct _FILETIME
{
    DWORD           dwLowDateTime;
    DWORD           dwHighDateTime;
}
FILETIME       , *PFILETIME, *LPFILETIME;
#endif
//
// System time is represented with the following structure:
//
typedef struct _SYSTEMTIME
{
    WORD            wYear;
    WORD            wMonth;
    WORD            wDayOfWeek;
    WORD            wDay;
    WORD            wHour;
    WORD            wMinute;
    WORD            wSecond;
    WORD            wMilliseconds;
}
SYSTEMTIME     , *PSYSTEMTIME, *LPSYSTEMTIME;


WINBASEAPI BOOL WINAPI FileTimeToSystemTime (CONST FILETIME * lpFileTime,
                                             LPSYSTEMTIME lpSystemTime);
WINBASEAPI VOID WINAPI GetSystemTimeAsFileTime(OUT LPFILETIME lpSystemTimeAsFileTime);
WINBASEAPI BOOL WINAPI SystemTimeToFileTime(__in   const SYSTEMTIME *lpSystemTime,
                                            __out  LPFILETIME lpFileTime);
WINBASEAPI BOOL WINAPI FileTimeToLocalFileTime (CONST FILETIME * lpFileTime,
                                                      LPFILETIME lpLocalFileTime);
#ifdef __cplusplus
}
#endif

#endif /* ALAMOSA_WINDOWS_ENV - NTHACK */

#endif /* USER_REDEF_H */
