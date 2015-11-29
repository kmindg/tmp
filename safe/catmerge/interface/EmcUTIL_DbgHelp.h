#ifndef __EMCUTIL_DBGHELP_H_
#define __EMCUTIL_DBGHELP_H_

/************/

#include "csx_ext.h"
#include <stddef.h>


/************/   

#define EMCUTIL_DBGHELP_API
#define EMCUTIL_DBGHELP_CC __cdecl

#define EMCUTIL_DBGHELP_XSEC_AT_EPOCH        0x19db1ded53e8000ULL
#define EMCUTIL_DBGHELP_XSEC_PER_SEC         10000000
#define EMCUTIL_DBGHELP_XSEC_PER_MSEC        10000
#define EMCUTIL_DBGHELP_XSEC_PER_USEC        10
#define EMCUTIL_DBGHELP_USEC_PER_SEC         1000000
#define EMCUTIL_DBGHELP_MSEC_PER_SEC         1000
#define EMCUTIL_DBGHELP_USEC_PER_MSEC        1000

typedef unsigned long long emcutil_dbghelp_nt_time_t;

typedef struct {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} emcutil_dbghelp_nt_time_fields_t;

#define EMCUTIL_DBGHELP_NT_TIME_FIELD_VARS(_nt_time_fields) \
    _nt_time_fields.wYear, _nt_time_fields.wMonth, _nt_time_fields.wDayOfWeek, _nt_time_fields.wDay, \
    _nt_time_fields.wHour, _nt_time_fields.wMinute, _nt_time_fields.wSecond, _nt_time_fields.wMilliseconds
#define EMCUTIL_DBGHELP_NT_TIME_FIELD_SPECS "%d %d %d %d %d %d %d %d"

typedef struct {
    long long tv_sec;                      /*!< secs part of time since 1970 */
    int tv_usec;                           /*!< usecs part of time since 1970 */
    int tv_pad;                            /*!< pad */
} emcutil_dbghelp_unix_time_t;

/************/

CSX_CDECLS_BEGIN

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_get_current_nt_time_utc(
    emcutil_dbghelp_nt_time_t * nt_time_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_get_current_unix_time_utc(
    emcutil_dbghelp_unix_time_t * unix_time_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_unix_time_to_nt_time(
    emcutil_dbghelp_unix_time_t * unix_time,
    emcutil_dbghelp_nt_time_t * nt_time_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_nt_time_to_unix_time(
    emcutil_dbghelp_nt_time_t * nt_time,
    emcutil_dbghelp_unix_time_t * unix_time_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_nt_time_utc_to_nt_time_local(
    emcutil_dbghelp_nt_time_t * nt_time_ptr,
    emcutil_dbghelp_nt_time_t * nt_time_local_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_nt_time_to_nt_time_fields(
    emcutil_dbghelp_nt_time_t * nt_time_ptr,
    emcutil_dbghelp_nt_time_fields_t * nt_time_fields_rv);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_nt_time_fields_to_nt_time(
    emcutil_dbghelp_nt_time_fields_t * nt_time_fields,
    emcutil_dbghelp_nt_time_t * nt_time_ptr_rv);

#define EMCUTIL_DBGHELP_NT_TIME_TO_STRING_TYPE_STD          1
#define EMCUTIL_DBGHELP_NT_TIME_TO_STRING_TYPE_STD_NO_MSEC  2

EMCUTIL_DBGHELP_API size_t EMCUTIL_DBGHELP_CC
emcutil_dbghelp_nt_time_to_string(
    emcutil_dbghelp_nt_time_t * nt_time_ptr,
    char *buffer_base,
    size_t buffer_size,
    int type);

EMCUTIL_DBGHELP_API void *EMCUTIL_DBGHELP_CC
emcutil_dbghelp_load_library(
        const char *dll_path);

EMCUTIL_DBGHELP_API void EMCUTIL_DBGHELP_CC
emcutil_dbghelp_free_library(
    void * dll_handle);

EMCUTIL_DBGHELP_API void * EMCUTIL_DBGHELP_CC
emcutil_dbghelp_lookup(
    void *dll_handle, const char *symbol_name);

CSX_CDECLS_END

/************/

#endif /* __EMCUTIL_DBGHELP_H_ */
