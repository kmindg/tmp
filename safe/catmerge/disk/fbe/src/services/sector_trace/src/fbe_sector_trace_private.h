#ifndef FBE_SECTOR_TRACE_PRIVATE_H
#define FBE_SECTOR_TRACE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 *
 * @brief
 *
 *  This file contains structures and definitions that are private to the 
 *  sector tracing facility.
 *
 * @author
 *
 *  01/13/2010  - Omer Miranda 
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_sector_trace_interface.h"
 
/*************************
 * LITERAL DEFINITIONS
 *************************/
#define FBE_SECTOR_TRACE_MAX_FMT_STRING_WITH_PREFIX   (4  + 80 + 1)
#define FBE_SECTOR_TRACE_FUNCTION_STR_LEN             (32 + 1)
#define FBE_SECTOR_TRACE_LINE_STR_LEN                 (16 + 1)

/*************************
 * STRUCTURE DEFINITIONS
 *************************/

/*
 *  FBE_SECTOR_TRACE_INFO - Global structure that contains all the Sector
 *                 global structures.  Structure is not allocated
 *                 instead it is a static global.
 */
typedef struct fbe_sector_trace_info_s
{
    /* This is the sector error type.  
     */
    fbe_sector_trace_error_flags_t  sector_trace_err_flag;
    
    /* This is the fbe sector error level.  For ALL types of errors
     * this determines which level of errors get traced.  Thus
     * if the registry trace level is INFO even if COH error
     * tracing is enabled, we will NOT log DATA level errors.
     */
    fbe_sector_trace_error_level_t  sector_trace_err_level;
    
    /* When this flag is set,
     * all traceable error of level INFO or lower will
     * include the function and line number.
     */
    fbe_bool_t                      sector_trace_location;

    /* If any of these type of errors are traced and `stop on error' is set
     * we will PANIC the SP.
     */
    fbe_sector_trace_error_flags_t  sector_trace_stop_on_error_flag;

    /* When this flag is set,
     * all traceable error will causes system to stop.
     */
    fbe_bool_t                      sector_trace_stop_on_error;

    /* This spin lock is used to make the sector tracing atomic.
     * Since different threads could be access the Xor tracing facility
     * when multi-line traces are done we lock out other requestors
     * so that the sector data does not appear interspersed.
     */
    fbe_spinlock_t                  sector_trace_lock;

    /*! Last time we traced some sectors. 
     *  We use this to limit the number of sectors we trace to a reasonable number
     *  every second.
     */
    fbe_time_t                      last_trace_time;
    /*! Total entries traced in the current quanta (since last_trace_time).  
     * We use this to limit how many sectors we trace. 
     */
    fbe_u32_t                       num_entries_traced;
    /*! Total number of entries not logged because we throttled when we 
     *  went past the limit of entries to log during a quanta. 
     */
    fbe_u32_t                       total_entries_dropped;
    /* This spinlock protect the function and line buffers in a SMP
     * environment.
     */
    fbe_spinlock_t                  location_strings_lock;
    char                            function_string[FBE_SECTOR_TRACE_FUNCTION_STR_LEN];
    char                            line_string[FBE_SECTOR_TRACE_FUNCTION_STR_LEN];

    /* Array of error counters.  One counter for each level and one
     * for each error type.
     */
    fbe_u32_t                       total_invocations;
    fbe_u32_t                       total_errors_traced;
    fbe_u32_t                       total_sectors_traced;
    fbe_u32_t                       error_level_counters[FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT];
    fbe_u32_t                       error_type_counters[FBE_SECTOR_TRACE_ERROR_FLAG_COUNT];

    fbe_system_encryption_mode_t    encryption_mode;
    fbe_bool_t                      b_is_external_build;

} fbe_sector_trace_info_t;

/*
 *  Prototypes from fbe_sector_trace.c
 */

/****************************************
 * END fbe_sector_trace_private.h
 ****************************************/
#endif /*  FBE_SECTOR_TRACE_PRIVATE_H */
