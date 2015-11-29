/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_ktrace_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains user mode definitions of the fbe ktrace methods.
 *  
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define I_AM_NATIVE_CODE
#include <windows.h>
#include <time.h>
#endif
#include "fbe/fbe_time.h"
#include "fbe/fbe_winddk.h"
#include "ktrace.h"                         /*!< This module invokes ktrace.lib methods */
#include "ktrace_IOCTL.h"         
#include "fbe_trace.h"
#include "fbe_ktrace_private.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "simulation_log.h"
#ifdef SIMMODE_ENV
#include <time.h>
#endif
#include "EmcUTIL_DllLoader_Interface.h"

/*************************** 
 *  Literal Definitions    *
 ***************************/

/*****************
 *  LOCAL GLOBALS
 *****************/
static fbe_bool_t       fbe_ktrace_initialized = FBE_FALSE;
/* Create a variable for maintaining the RBA Traffic Logging state */
PVOID RBA_logging_enabled_info = NULL;
static fbe_u32_t        fbe_xor_start_entries_remaining = (2 * 1024);
#if defined(UMODE_ENV) 
static EMCUTIL_RDEVICE_REFERENCE  fbe_ktrace_handle = EMCUTIL_DEVICE_REFERENCE_INVALID;
#endif
#if defined(SIMMODE_ENV)
static EMCUTIL_DLL_HANDLE simulation_log_dll_handle;
static simulation_log_trace_func_t simulation_log_trace_func = NULL;
static simulation_log_init_func_t simulation_log_init_func = NULL;
#endif
fbe_ktrace_func_t fbe_ktrace_function = NULL;

/*!***************************************************************************
 *          fbe_ktrace_is_initialized()
 *****************************************************************************
 *
 * @brief   Simply determines if the fbe ktrace library has been initialized
 *          or not.
 *  
 * @param   None
 *
 * @return  bool - FBE_TRUE - fbe ktrace library has been initialized
 *                 FBE_FALSE - fbe ktrace library has not been initialized
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t  fbe_ktrace_is_initialized(void)
{
    return(fbe_ktrace_initialized);
}
/* end of fbe_ktrace_is_initialized() */

/*!***************************************************************************
 *          fbe_ktrace_initialize()
 *****************************************************************************
 *
 * @brief   Initialize any structures required for the fbe ktrace library
 *  
 * @param   None
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t __cdecl fbe_ktrace_initialize(void)
{
    /* Only initialize if not already done
     */
    if (fbe_ktrace_initialized == FBE_FALSE)
    {
        #if defined(SIMMODE_ENV)
        {
            simulation_log_dll_handle = emcutil_simple_dll_load("simulation_log");
            if(simulation_log_dll_handle == NULL)
            {
                printf("Cannot load simulation_log.dll!\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
            simulation_log_trace_func = (simulation_log_trace_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_trace");
            simulation_log_init_func = (simulation_log_init_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_init");
            simulation_log_init_func();
        }
		#else
			#if defined(UMODE_ENV)
            if (fbe_ktrace_handle == EMCUTIL_DEVICE_REFERENCE_INVALID)
            {
                EMCUTIL_STATUS             status;
                status = EmcutilRemoteDeviceOpen(KTRACE_UMODE_NT_DEVICE_NAME,
                                                NULL,
                                                & fbe_ktrace_handle);
                if(!EMCUTIL_SUCCESS(status))
                {
                    fbe_ktrace_handle = EMCUTIL_DEVICE_REFERENCE_INVALID;
                }
            }
			#else
			RBA_logging_enabled_info = KtraceInitN(TRC_K_TRAFFIC);
			#endif
		 #endif
        fbe_ktrace_initialized = FBE_TRUE;
    }

    /* Always return status.
     */
    return(FBE_STATUS_OK);
}
/* end of fbe_ktrace_initialize() */

/*!***************************************************************************
 *          fbe_ktrace_destroy()
 *****************************************************************************
 *
 * @brief   Destroy any structures required for the fbe ktrace library
 *  
 * @param   None
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t __cdecl fbe_ktrace_destroy(void)
{
    /* Only destroy if initialized
     */
    if (fbe_ktrace_initialized == FBE_TRUE)
    {
    #if defined(SIMMODE_ENV)
    {
        simulation_log_destroy_func_t simulation_log_destroy_func = NULL;
        simulation_log_destroy_func = (simulation_log_destroy_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_destroy");
        if(simulation_log_destroy_func != NULL)
        {
            simulation_log_destroy_func();
        }
        emcutil_simple_dll_unload(simulation_log_dll_handle);
        simulation_log_dll_handle = NULL;
    }
	#else
		#if defined(UMODE_ENV)
        if (fbe_ktrace_handle != EMCUTIL_DEVICE_REFERENCE_INVALID)
        {
            EmcutilRemoteDeviceClose(fbe_ktrace_handle);
        }
		#endif
    #endif

        fbe_ktrace_initialized = FBE_FALSE;
    }

    //
    //  In Umode or SIMMODE we need to unload the simulation.dll that we loaded during initialize.
    //  Otherwise, the dll unload of the application cli.exe will get a BUSY from the 
    //  simulation.dll.
    //
#if defined(SIMMODE_ENV)

    if ( simulation_log_dll_handle != NULL )
    {
        emcutil_simple_dll_unload(simulation_log_dll_handle);
        simulation_log_dll_handle = NULL;
    }

#endif // #if defined(SIMMODE_ENV)

    /* Always return status.
     */
    return(FBE_STATUS_OK);
}
/* end of fbe_ktrace_destroy() */

/*!**************************************************************
 * fbe_ktrace_set_trace_function()
 ****************************************************************
 * @brief
 *  Set the alternate trace function.
 *
 * @param void               
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/12/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_ktrace_set_trace_function(fbe_ktrace_func_t func)
{
    fbe_ktrace_function = func;

    /* Always return status.
     */
    return(FBE_STATUS_OK);
}
/******************************************
 * end fbe_ktrace_set_trace_function()
 ******************************************/

/*!***************************************************************************
 *          fbe_ktrace_trace_ring_to_ktrace_ring()
 *****************************************************************************
 *
 * @brief   Simply converts from a fbe trace ring identifier to a ktrace ring
 *          identifier.
 *  
 * @param   fbe_buf - The fbe trace ring identifier 
 *
 * @return  ktrace_ring - The corresponding ktrace ring identifier
 *
 * @author
 *  06/25/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static __forceinline KTRACE_ring_id_T fbe_ktrace_trace_ring_to_ktrace_ring(fbe_trace_ring_t fbe_buf)
{
    KTRACE_ring_id_T ktrace_ring = TRC_K_NONE;

    /* Simply convert from fbe buffer to ktrace ring */
    switch(fbe_buf)
    {
        case FBE_TRACE_RING_DEFAULT:
            ktrace_ring = TRC_K_STD;
            break;

        case FBE_TRACE_RING_STARTUP:
            ktrace_ring = TRC_K_START;
            break;

        case FBE_TRACE_RING_XOR_START:
            ktrace_ring = TRC_K_XOR_START;
            break;

        case FBE_TRACE_RING_XOR:
            ktrace_ring = TRC_K_XOR;
            break;

        case FBE_TRACE_RING_TRAFFIC:
            ktrace_ring = TRC_K_TRAFFIC;
            break;
        case FBE_TRACE_RING_INVALID:
        default:
            /* Simply print an error message */
            fbe_printf("ktrace: %s - Unsupported fbe buffer: %d \n",
                   __FUNCTION__, fbe_buf);
            break;
    }

    /* Always return the ktrace ring */
    return(ktrace_ring);
}
/* end of fbe_ktrace_trace_ring_to_ktrace_ring() */ 

/*!***************************************************************************
 *          fbe_KtCprint()
 *****************************************************************************
 *
 * @brief   Common ktrace print method
 *  
 * @param   fbe_buf - The fbe trace ring identifier
 * @param   fmt - Format string
 * @param   argptr - va argument list
 *
 * @return  fbe_char_t * - Currently always NULL
 *
 * @author
 *  06/25/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_char_t *fbe_KtCprint(fbe_trace_ring_t fbe_buf, const fbe_char_t *fmt, va_list argptr)
{
    KTRACE_ring_id_T ktrace_ring;

    /* Convert from fbe buffer to ktrace ring 
     */
    ktrace_ring = fbe_ktrace_trace_ring_to_ktrace_ring(fbe_buf);

    /* Increment xor start entries used 
     */
    if (fbe_buf == FBE_TRACE_RING_XOR_START)
    {
        if ((fbe_s32_t)fbe_xor_start_entries_remaining > 0)
        {
            fbe_xor_start_entries_remaining--;
        }
    }

    /*! @todo This is temporary.  Although ktrace supports routing the output to 
     *        the console OR a file it currently doesn't do both.  Once it does
     *        the user and kernel versions will behav the same way.
     */
#if defined(SIMMODE_ENV)
    {
        fbe_char_t date[32];
        fbe_char_t buffer[512];
        EMCUTIL_TIME_FIELDS st;

        EmcutilGetSystemTimeFields(&st);
        EmcutilConvertTimeFieldsToLocalTime(&st);
        _strdate(date);
        vsprintf(buffer, fmt, argptr);

        if (fbe_ktrace_function != NULL)
        {
            if (strlen(buffer) >= KT_STR_LEN - 1)
            {
                (fbe_ktrace_function)("%s %02u:%02u:%02u.%03u [KTRACE STRING TOO LONG ERROR] %s", 
                                      date, st.Hour, st.Minute, st.Second,
                                      st.Milliseconds, buffer);
            }
            else
            {
                (fbe_ktrace_function)("%s %02u:%02u:%02u.%03u %s", date,
                                      st.Hour, st.Minute, st.Second,
                                      st.Milliseconds, buffer);
            }
            return NULL;
        }
        if(simulation_log_dll_handle == NULL)
        {
            simulation_log_dll_handle = emcutil_simple_dll_load("simulation_log");
        }
        if(simulation_log_dll_handle != NULL)
        {
            if(simulation_log_trace_func == NULL)
            {
                simulation_log_trace_func = (simulation_log_trace_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_trace");
            }
            if(simulation_log_trace_func != NULL)
            {
                /* if the buffer length exceeds ktrace string length, add error message */
                if(strlen(buffer) >= KT_STR_LEN - 1)
                {
                    simulation_log_trace_func("%s %02u:%02u:%02u.%03u [KTRACE STRING TOO LONG ERROR] %s",
                                              date, st.Hour, st.Minute,
                                              st.Second, st.Milliseconds,
                                              buffer);
                }
                else
                {
                    simulation_log_trace_func("%s %02u:%02u:%02u.%03u %s", date,
                                              st.Hour, st.Minute, st.Second,
                                              st.Milliseconds, buffer);
                }
            }
            else
            {
                /* if the buffer length exceeds ktrace string length, add error message */
                if(strlen(buffer) >= KT_STR_LEN - 1)
                {
                    printf("%s %02u:%02u:%02u.%03u [KTRACE STRING TOO LONG ERROR] %s",
                           date, st.Hour, st.Minute, st.Second, st.Milliseconds,
                           buffer);
                }
                else
                {
                    printf("%s %02u:%02u:%02u.%03u %s", date, st.Hour,
                           st.Minute, st.Second, st.Milliseconds, buffer);
                }
            }
        }
        else
        {
            /* if the buffer length exceeds ktrace string length, add error message */
            if(strlen(buffer) >= KT_STR_LEN - 1)
            {
                printf("%s %02u:%02u:%02u.%03u [KTRACE STRING TOO LONG ERROR] %s",
                       date, st.Hour, st.Minute, st.Second, st.Milliseconds,
                       buffer);
            }
            else
            {
                printf("%s %02u:%02u:%02u.%03u %s", date, st.Hour, st.Minute,
                       st.Second, st.Milliseconds, buffer);
            }
        }
    }
#else
	#if defined(UMODE_ENV)
    {
        EMCUTIL_STATUS             status;
        KUTRACE_INFO_NO_ARG kutrace_info;
        ULONG dummy;
        csx_size_t bytesReturned;

        if (fbe_ktrace_handle != EMCUTIL_DEVICE_REFERENCE_INVALID)
        {
            vsnprintf((char *)kutrace_info.message, KT_STR_LEN-1, fmt, argptr);
            // Write into KTrace buffer
            kutrace_info.ring_id = TRC_K_USER;
            kutrace_info.length = strnlen((char *) kutrace_info.message, KT_STR_LEN) + 1;

            status = EmcutilRemoteDeviceIoctl(fbe_ktrace_handle,   // handle to device of interest
                IOCTL_KTRACE_KUTRACEX,      // control code of operation to perform
                (LPVOID)&kutrace_info,      // pointer to buffer to supply input data
                sizeof(kutrace_info),       // size of input buffer
                &dummy,                     // pointer to buffer to receive output data
                sizeof(dummy),              // size of output buffer
                &bytesReturned             // pointer to variable to receive output byte count
                );

        }
    }
	#else

    /* Else if kernel mode simply invoke the ktrace method.
     */
    _KtCprint(ktrace_ring, fmt, argptr);
	#endif
#endif /* end else kernel mode*/

    return NULL;
}
/* end of fbe_KtCprint() */

void fbe_KvTraceStart(const fbe_char_t *fmt, ...)
{
    va_list argptr;
	va_start(argptr, fmt);
	(void)fbe_KtCprint(FBE_TRACE_RING_STARTUP, fmt, argptr);
	va_end(argptr);
}

void fbe_KvTrace(const fbe_char_t *fmt, ...)
{
    va_list argptr;
	va_start(argptr, fmt);
	(void)fbe_KtCprint(FBE_TRACE_RING_DEFAULT, fmt, argptr);
	va_end(argptr);
}

void fbe_KvTraceRing(fbe_trace_ring_t fbe_buf, const fbe_char_t *fmt, ...)
{
    va_list argptr;
	va_start(argptr, fmt);
	(void)fbe_KtCprint(fbe_buf, fmt, argptr);
	va_end(argptr);
}

void fbe_KvTraceRingArgList(fbe_trace_ring_t fbe_buf, const fbe_char_t *fmt, va_list argptr)
{
	(void)fbe_KtCprint(fbe_buf, fmt, argptr);
}

long fbe_KtraceEntriesRemaining(const fbe_trace_ring_t buf)
{
    if (buf == FBE_TRACE_RING_XOR_START)
    {
        return(fbe_xor_start_entries_remaining);
    }
    else
    {
        return(0);
    }
}

/*!***************************************************************************
 *          fbe_KvTraceLocation()
 ***************************************************************************** 
 * 
 * @brief   Generate a fbe ktrace entry and dynamically add the `location'
 *          information.
 *  
 * @param   buf - The fbe ktrace ring buffer to trace to     
 * @param   function - Function name that invoked the ktrace
 * @param   line - The line number of functions that invoked us
 * @param   function_string_size - The size of the function string buffer
 * @param   line_string_size - The size of the line string buffer
 * @param   fmt - Pointer to format string for this request.
 * @param   argptr - Variable argument list pointer                                       
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *****************************************************************************/
fbe_status_t fbe_KvTraceLocation(const fbe_trace_ring_t buf,
                         const fbe_char_t *function, unsigned long line,
                         fbe_u32_t function_string_size, fbe_u32_t line_string_size, 
                         const fbe_char_t *fmt, va_list argptr)
{
    fbe_status_t    status = FBE_STATUS_OK;
    va_list         temp_arg_list;
    fbe_char_t           *prefix_function_string;
    fbe_char_t           *prefix_line_string;  

    /* Assumes extra format parameters have been added!!
     */
    if ((fmt[0] != '%') ||
        (fmt[1] != 's') ||
        (fmt[2] != '%') ||
        (fmt[3] != 's')    )
    {
        fbe_KvTrace("%s - First (4) characters of fmt not expected: %s \n",
                    __FUNCTION__, fmt);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* To prevent extra character without line tracing
     * get the argument list now. va_copy isn't defined
     * in Windows but it is defined in other environments.
     */
#ifndef va_copy
#define va_copy(dst, src) ((void)((dst) = (src)))
#endif
    va_copy(temp_arg_list, argptr);

    /* Get the addresses of the function and line
     * strings. 
     */
    prefix_function_string = va_arg(temp_arg_list, fbe_char_t *);
    prefix_line_string = va_arg(temp_arg_list, fbe_char_t *);

    /* Create valid function and line information.
     */
    strncpy(prefix_function_string, function, (function_string_size - 1));
    prefix_function_string[(function_string_size - 1)] = '\0';
    _snprintf(prefix_line_string, (function_string_size - 1), " line %lu->",
	      line);
    prefix_line_string[(line_string_size - 1)] = '\0';

    /* Now generate the ktrace entry with the original argument list
     */
    (void)fbe_KtCprint(buf, fmt, argptr);

    /* Now close our local argument list 
     */
    va_end(temp_arg_list);

    /* Return status
     */
    return(status);
}
/* end of fbe_KvTraceLocation() */


/*!***************************************************************************
 *          fbe_ktrace_flush()
 *****************************************************************************
 * @brief   Flush our log file out to disk.
 *  
 * @param   None
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/8/2014 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_ktrace_flush(void)
{

#if defined(SIMMODE_ENV)
    if (fbe_ktrace_initialized == FBE_TRUE) {
        simulation_log_destroy_func_t simulation_log_destroy_func = NULL;
        simulation_log_destroy_func = (simulation_log_destroy_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_flush");
        if (simulation_log_destroy_func != NULL) {
            simulation_log_destroy_func();
        }
    }
#endif

    /* Always return status.
     */
    return(FBE_STATUS_OK);
}
/* end of fbe_ktrace_flush() */

