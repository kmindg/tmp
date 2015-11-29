/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_trace.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the raid tracing functions.
 *
 * @version
 *   12/07/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_traffic_trace.h"

/* This is a simple ring mechanism that is designed to have low impact
 * on the timing of the system. 
 */

/*!*******************************************************************
 * @struct fbe_raid_log_t
 *********************************************************************
 * @brief This is a simple ring structure appropriate for logging where
 *        we do not want to affect the timing.
 *
 *********************************************************************/
typedef struct fbe_raid_log_s
{
    fbe_u32_t arg0;
    fbe_u32_t arg1;
    fbe_u32_t arg2;
    fbe_u32_t arg3;
    fbe_u32_t arg4;
}
fbe_raid_log_t;

static fbe_bool_t fbe_raid_log_is_initialized = FBE_FALSE;
#define FBE_RAID_LOG_MAX_ENTRIES 50000
static fbe_raid_log_t fbe_raid_log_array[FBE_RAID_LOG_MAX_ENTRIES];
static fbe_u32_t fbe_raid_log_index = 0;
static fbe_spinlock_t fbe_raid_log_lock;

/*!**************************************************************
 * fbe_raid_log_record()
 ****************************************************************
 * @brief
 *  This is a simple logging mechanism that is designed
 *  to have low impact on the timing of the system.
 *
 * @param arg0
 * @param arg1
 * @param arg2
 * @param arg3
 * @param arg4           
 *
 * @return fbe_status_t
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_log_record(fbe_u32_t arg0, fbe_u32_t arg1, fbe_u32_t arg2, fbe_u32_t arg3, fbe_u32_t arg4)
{
    if (fbe_raid_log_is_initialized == FBE_FALSE)
    {
        fbe_raid_log_is_initialized = FBE_TRUE;
        fbe_spinlock_init(&fbe_raid_log_lock);
    }

    fbe_spinlock_lock(&fbe_raid_log_lock);
    fbe_raid_log_array[fbe_raid_log_index].arg0 = arg0;
    fbe_raid_log_array[fbe_raid_log_index].arg1 = arg1;
    fbe_raid_log_array[fbe_raid_log_index].arg2 = arg2;
    fbe_raid_log_array[fbe_raid_log_index].arg3 = arg3;
    fbe_raid_log_array[fbe_raid_log_index].arg4 = arg4;

    fbe_raid_log_index++;
    if (fbe_raid_log_index >= FBE_RAID_LOG_MAX_ENTRIES)
    {
        fbe_raid_log_index = 0;
    }
    fbe_spinlock_unlock(&fbe_raid_log_lock);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_log_record()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_object_trace()
 ****************************************************************
 * @brief
 *  Trace out a condition.
 * 
 * @param trace_level - Level to trace at.
 * @param condition_p - Condition that failed.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 *
 * @return - None.
 * 
 * @date
 *  8/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_library_object_trace(fbe_object_id_t object_id,
                                  fbe_trace_level_t trace_level,
                                  fbe_u32_t line,
                                  const char *const function_p,
                                  fbe_char_t * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;

    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_RAID);
    if (trace_level > default_trace_level) 
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_RAID,
                     trace_level,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/**************************************
 * end fbe_raid_library_object_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_object_trace()
 ****************************************************************
 * @brief
 *  Trace out a condition.
 * 
 * @param siots_p - siots to trace for.
 * @param trace_level - Level to trace at.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 * @param fmt, ... - The string to print and its arguments.
 *
 * @return - None.
 * 
 * @date
 *  8/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_object_trace(fbe_raid_siots_t *siots_p,
                                 fbe_trace_level_t trace_level,
                                 fbe_u32_t line,
                                 const char *const function_p,
                                 fbe_char_t * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;
    fbe_raid_geometry_t *raid_geometry_p ;
    fbe_object_id_t object_id;

    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_RAID);
    if (trace_level > default_trace_level) 
    {
        return;
    }

	raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
	object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_RAID,
                     trace_level,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/**************************************
 * end fbe_raid_siots_object_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_object_trace()
 ****************************************************************
 * @brief
 *  Trace out a condition.
 * 
 * @param iots_p - iots to trace for.
 * @param trace_level - Level to trace at.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 * @param fmt, ... - The string to print and its arguments.
 *
 * @return - None.
 * 
 * @date
 *  12/13/2011 - Created. He Wei
 *
 ****************************************************************/

void fbe_raid_iots_object_trace(fbe_raid_iots_t *iots_p,
                                       fbe_trace_level_t trace_level,
                                       fbe_u32_t line,
                                       const char *const function_p,
                                       fbe_char_t * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;
    fbe_raid_geometry_t *raid_geometry_p;
    fbe_object_id_t object_id;

    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_RAID);
    if (trace_level > default_trace_level) 
    {
        return;
    }

	raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
	object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_RAID,
                     trace_level,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/**************************************
 * end fbe_raid_iots_object_trace()
 **************************************/


/*!**************************************************************
 * fbe_raid_library_trace_basic()
 ****************************************************************
 * @brief
 *  Trace out a condition.
 * 
 * @param trace_level - Level to trace at.
 * @param condition_p - Condition that failed.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 *
 * @return - None.
 *
 ****************************************************************/

void fbe_raid_library_trace_basic(fbe_trace_level_t trace_level,
                                  fbe_u32_t line,
                                  const char *const function_p,
                                  fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s", buffer);
    return;
}
/**************************************
 * end fbe_raid_library_trace_basic()
 **************************************/
/*!**************************************************************
 * fbe_raid_library_trace()
 ****************************************************************
 * @brief
 *  Trace out a failure with condition, line and file info.
 * 
 * @param trace_level - Level to trace at.
 * @param condition_p - Condition that failed.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 *
 * @return - None.
 *
 ****************************************************************/

void fbe_raid_library_trace(fbe_trace_level_t trace_level,
                            fbe_u32_t line,
                            const char *const function_p,
                            fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: line: %d function: %s\n", line, function_p);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s", buffer);
    return;
}
/**************************************
 * end fbe_raid_library_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_trace_error()
 ****************************************************************
 * @brief
 *  Trace out a failure with condition, line and file info.
 * 
 * @param condition_p - Condition that failed.
 * @param line - The line number where failure occurred.
 * @param function_p - Fn of failure.
 *
 * @return - None.
 *
 ****************************************************************/

void fbe_raid_trace_error(fbe_u32_t line,
                          const char *const function_p,
                          fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: line: %d function: %s\n", line, function_p);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s", buffer);
    return;
}
/**************************************
 * end fbe_raid_trace_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_trace()
 ****************************************************************
 * @brief
 *  For a given iots trace some information.
 * 
 * @param iots_p - siots to transition to failed.
 * @param line - The line number where failure occurred.
 * @param function_p - File of failure.
 * @param fmt - This is the string describing the failure.
 *
 * @return - None.
 *
 * @author
 *  11/13/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_iots_trace(fbe_raid_iots_t *const iots_p,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t line,
                          const char *function_p,
                          fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p lba/bl: 0x%llx/0x%llx state: 0x%p status: 0x%x \n", 
                           iots_p, (unsigned long long)iots_p->lba, (unsigned long long)iots_p->blocks, fbe_raid_iots_get_state(iots_p), iots_p->status);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p op: 0x%x b_rem: 0x%llx b_xf: 0x%llx\n", 
                           iots_p, iots_p->current_opcode, 
                           (unsigned long long)iots_p->blocks_remaining, (unsigned long long)iots_p->blocks_transferred);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p c_lba: 0x%llx o_l/b: 0x%llx/0x%llx fl: 0x%x\n", 
                           iots_p, (unsigned long long)iots_p->current_lba, (unsigned long long)iots_p->current_operation_lba, (unsigned long long)iots_p->current_operation_blocks,
                           iots_p->flags);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p host_off: 0x%llx reqs: 0x%x plba/bl: 0x%llx/0x%llx\n", 
                           iots_p, (unsigned long long)iots_p->host_start_offset, iots_p->outstanding_requests, (unsigned long long)iots_p->packet_lba, (unsigned long long)iots_p->packet_blocks);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p chunk_sz: 0x%x chunk_info[0]: v: %x nr: %x vr: %x r: %x\n", 
                           iots_p, iots_p->chunk_size, iots_p->chunk_info[0].valid_bit, iots_p->chunk_info[0].needs_rebuild_bits,
                           iots_p->chunk_info[0].verify_bits, iots_p->chunk_info[0].reserved_bits);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p chunk_info[1]: v: %x nr: %x vr: %x r: %x\n", 
                           iots_p, iots_p->chunk_info[1].valid_bit, iots_p->chunk_info[1].needs_rebuild_bits,
                           iots_p->chunk_info[1].verify_bits, iots_p->chunk_info[1].reserved_bits);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: iots_p: 0x%p error line: %d function: %s\n", 
                           iots_p, line, function_p);
    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               buffer);
    }
    return;
}
/**************************************
 * end fbe_raid_iots_trace()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_iots_trace_rebuild_chunk_info()
 *****************************************************************************
 *
 * @brief   For a given iots trace the rebuild chunk information for the number
 *          of chunks specified.
 * 
 * @param iots_p - iots with chunk infor to trace
 * @param chunk_count - Number of chunks to trace
 *
 * @return - None.
 *
 * @author
 *  01/31/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
void fbe_raid_iots_trace_rebuild_chunk_info(fbe_raid_iots_t *const iots_p,
                                            fbe_chunk_count_t chunk_count)
{
    fbe_raid_group_paged_metadata_t *chunk_info_p = NULL;

    /* If more than supported chunk is requested, somethign is wrong.
     */
    if (chunk_count > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        /* Trace an error.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: trace rebuild chunk info: chunk_count: %d greater than max: %d \n",
                            chunk_count, FBE_RAID_IOTS_MAX_CHUNKS);
        return;
    }

    /* Display up to (10) entries at (5) per line.
     */
    chunk_info_p = &iots_p->chunk_info[0];
    if ((FBE_RAID_IOTS_MAX_CHUNKS >= 5) &&
        (chunk_count > 0)                  )
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                              "raid: rebuild %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                              (chunk_count > 0) ? "NR[0]" : " n/a ",
                              (chunk_count > 0) ? chunk_info_p[0].needs_rebuild_bits : 0,
                              (chunk_count > 1) ? "NR[1]" : " n/a ",
                              (chunk_count > 1) ? chunk_info_p[1].needs_rebuild_bits : 0,
                              (chunk_count > 2) ? "NR[2]" : " n/a ",
                              (chunk_count > 2) ? chunk_info_p[2].needs_rebuild_bits : 0,
                              (chunk_count > 3) ? "NR[3]" : " n/a ",
                              (chunk_count > 3) ? chunk_info_p[3].needs_rebuild_bits : 0,
                              (chunk_count > 4) ? "NR[4]" : " n/a ",
                              (chunk_count > 4) ? chunk_info_p[4].needs_rebuild_bits : 0);
        if ((FBE_RAID_IOTS_MAX_CHUNKS >= 10) &&
            (chunk_count > 5)                   )
        {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                  "raid: rebuild %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                                  (chunk_count > 5) ? "NR[5]" : " n/a ",
                                  (chunk_count > 5) ? chunk_info_p[5].needs_rebuild_bits : 0,
                                  (chunk_count > 6) ? "NR[6]" : " n/a ",
                                  (chunk_count > 6) ? chunk_info_p[6].needs_rebuild_bits : 0,
                                  (chunk_count > 7) ? "NR[7]" : " n/a ",
                                  (chunk_count > 7) ? chunk_info_p[7].needs_rebuild_bits : 0,
                                  (chunk_count > 8) ? "NR[8]" : " n/a ",
                                  (chunk_count > 8) ? chunk_info_p[8].needs_rebuild_bits : 0,
                                  (chunk_count > 9) ? "NR[9]" : " n/a ",
                                  (chunk_count > 9) ? chunk_info_p[9].needs_rebuild_bits : 0);
        }
    }

    return;
}
/*************************************************
 * end of fbe_raid_iots_trace_rebuild_chunk_info()
 *************************************************/

/*!***************************************************************************
 * fbe_raid_iots_trace_verify_chunk_info()
 *****************************************************************************
 *
 * @brief   For a given iots trace the verify chunk information for the number
 *          of chunks specified.
 * 
 * @param iots_p - iots with chunk infor to trace
 * @param chunk_count - Number of chunks to trace
 *
 * @return - None.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 *****************************************************************************/
void fbe_raid_iots_trace_verify_chunk_info(fbe_raid_iots_t *const iots_p,
                                           fbe_chunk_count_t chunk_count)
{
    fbe_raid_group_paged_metadata_t *chunk_info_p = NULL;

    /* If more than supported chunk is requested, somethign is wrong.
     */
    if (chunk_count > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        /* Trace an error.
         */
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                            "raid: trace verify chunk info: chunk_count: %d greater than max: %d \n",
                            chunk_count, FBE_RAID_IOTS_MAX_CHUNKS);
        return;
    }

    /* Display up to (10) entries at (5) per line.
     */
    chunk_info_p = &iots_p->chunk_info[0];
    if ((FBE_RAID_IOTS_MAX_CHUNKS >= 5) &&
        (chunk_count > 0)                  )
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                              "raid: verify %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                              (chunk_count > 0) ? "VR[0]" : " n/a ",
                              (chunk_count > 0) ? chunk_info_p[0].verify_bits : 0,
                              (chunk_count > 1) ? "VR[1]" : " n/a ",
                              (chunk_count > 1) ? chunk_info_p[1].verify_bits : 0,
                              (chunk_count > 2) ? "VR[2]" : " n/a ",
                              (chunk_count > 2) ? chunk_info_p[2].verify_bits : 0,
                              (chunk_count > 3) ? "VR[3]" : " n/a ",
                              (chunk_count > 3) ? chunk_info_p[3].verify_bits : 0,
                              (chunk_count > 4) ? "VR[4]" : " n/a ",
                              (chunk_count > 4) ? chunk_info_p[4].verify_bits : 0);
        if ((FBE_RAID_IOTS_MAX_CHUNKS >= 10) &&
            (chunk_count > 5)                   )
        {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO,
                                  "raid: verify %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                                  (chunk_count > 5) ? "VR[5]" : " n/a ",
                                  (chunk_count > 5) ? chunk_info_p[5].verify_bits : 0,
                                  (chunk_count > 6) ? "VR[6]" : " n/a ",
                                  (chunk_count > 6) ? chunk_info_p[6].verify_bits : 0,
                                  (chunk_count > 7) ? "VR[7]" : " n/a ",
                                  (chunk_count > 7) ? chunk_info_p[7].verify_bits : 0,
                                  (chunk_count > 8) ? "VR[8]" : " n/a ",
                                  (chunk_count > 8) ? chunk_info_p[8].verify_bits : 0,
                                  (chunk_count > 9) ? "VR[9]" : " n/a ",
                                  (chunk_count > 9) ? chunk_info_p[9].verify_bits : 0);
        }
    }

    return;
}
/*************************************************
 * end of fbe_raid_iots_trace_verify_chunk_info()
 *************************************************/

/*!**************************************************************
 * fbe_raid_siots_trace()
 ****************************************************************
 * @brief
 *  For a given siots transition it to an unexpected error state.
 * 
 * @param siots_p - siots to transition to failed.
 * @param line - The line number where failure occurred.
 * @param function_p - File of failure.
 * @param fmt - This is the string describing the failure.
 *
 * @return - None.
 *
 * @author
 *  11/13/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_trace(fbe_raid_siots_t *const siots_p,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t line,
                          const char *function_p,
                          fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: siots_p: %p lba: 0x%llx blks: 0x%llx alg: 0x%x \n", 
                           siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, siots_p->algorithm);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: siots_p: %p pst/pcnt: 0x%llx/0x%llx flags: 0x%x\n", 
                           siots_p, (unsigned long long)siots_p->parity_start, (unsigned long long)siots_p->parity_count, siots_p->flags);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: siots_p: %p wait: %d s_pos: %d d_disks: %d p_pos: %d\n", 
                           siots_p, (int)siots_p->wait_count, siots_p->start_pos, siots_p->data_disks, siots_p->parity_pos);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: siots_p: %p dead: %d dead_2: %d error: 0x%x qual: 0x%x\n", 
                           siots_p, siots_p->dead_pos, siots_p->dead_pos_2, siots_p->error, siots_p->qualifier);
    if (siots_p->common.parent_p != NULL) 
    {
        iots_p = fbe_raid_siots_get_iots(siots_p);
        packet_p = fbe_raid_iots_get_packet(iots_p);
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               "raid: siots_p: %p iots_p: %p iots lba: 0x%llx bl: 0x%llx op: 0x%x \n", 
                               siots_p, iots_p, (unsigned long long)iots_p->current_lba, (unsigned long long)iots_p->blocks, opcode);
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               "raid: siots_p: %p pkt: %p\n", 
                               siots_p, packet_p);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: siots_p: %p error line: %d function: %s\n", 
                           siots_p, line, function_p);
    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               buffer);
    }
    return;
}
/**************************************
 * end fbe_raid_siots_trace()
 **************************************/
/*!***************************************************************************
 *          fbe_raid_write_journal_trace()
 *****************************************************************************
 *
 * @brief   If write journal tracing is enabled trace the requested
 *          message to the standard trace log with a level of info.
 * 
 * @param   siots_p - siots pointer used to get the trace enabled flag
 * @param   fmt - format string
 * @param   ... - Additional parameters
 *
 * @return - None.
 *
 * @author
 *  03/08/2011 Created. Daniel Cummins
 *
 ****************************************************************/
#define FBE_RAID_MAX_TRACE_PREAMBLE 32 
void fbe_raid_write_journal_trace(fbe_raid_siots_t *const siots_p,                          
				  fbe_trace_level_t trace_level,
				  fbe_u32_t line,
				  const char *function_p,
				  const fbe_char_t *fmt, ...)
{
	fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
	char                    buffer[FBE_RAID_MAX_TRACE_CHARS];
	char					preamble[FBE_RAID_MAX_TRACE_PREAMBLE];
	fbe_u32_t               buffer_len = 0;
	fbe_u32_t               preamble_len = 0;
	va_list                 argList;

	/* Only trace if the rebuild logging flag is set.
	 */
	if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, 
											FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING))
	{
		/* Only invoke trace if caller is sending a message.
		 */
		if (fmt != NULL)
		{
			fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
			fbe_object_id_t object_id;

			fbe_transport_get_object_id(fbe_raid_iots_get_packet(iots_p), &object_id);

			preamble_len = _snprintf(preamble, FBE_RAID_MAX_TRACE_PREAMBLE, "write_journal 0x%X ",object_id);

			va_start(argList, fmt);
			buffer_len = _vsnprintf(buffer, (FBE_RAID_MAX_TRACE_CHARS-preamble_len), fmt, argList);
			buffer[(FBE_RAID_MAX_TRACE_CHARS-preamble_len) - 1] = '\0';
			va_end(argList);

			/* Display the string, if it has some characters.
			 * Currently the trace level is `informational'
			 */
			fbe_base_library_trace(FBE_LIBRARY_ID_RAID, 
								   trace_level, 
								   FBE_TRACE_MESSAGE_ID_INFO,
								   "%s%s",
								   preamble,
								   buffer);
		}
	}
	return;
}
/* end of fbe_raid_write_journal_trace() */

/*!**************************************************************
 * fbe_raid_memory_info_trace()
 ****************************************************************
 * @brief
 *  Trace memory request information
 * 
 * @param memory_info_p - Pointer to memory request information
 * @param line - The line number where failure occurred.
 * @param function_p - File of failure.
 * @param fail_string_p - This is the string describing the failure.
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_info_trace(fbe_raid_memory_info_t *const memory_info_p,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t line,
                          const char *function_p,
                          fbe_char_t * fail_string_p, ...)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_memory_info_get_siots(memory_info_p);
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RAID_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                           "raid: memory info for siots lba: 0x%llx blks: 0x%llx blks to allocate: 0x%llx num_pages: %d/%d\n",
                           (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, 
                           (unsigned long long)siots_p->total_blocks_to_allocate, siots_p->num_ctrl_pages, siots_p->num_data_pages);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: memory_info_p: 0x%p siots_p: 0x%p page_size_in_bytes: 0x%x\n", 
                           memory_info_p, siots_p, fbe_raid_memory_info_get_page_size_in_bytes(memory_info_p));
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: line: %d function: %s\n", line, function_p);
    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid: master_p: 0x%p current_p: 0x%p bytes remaining in page: 0x%x \n",
                           fbe_raid_memory_info_get_master_header(memory_info_p),
                           fbe_raid_memory_info_get_current_element(memory_info_p),
                           fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p));

    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               buffer);
    }
    return;
}
/**************************************
 * end fbe_raid_memory_info_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_traffic_trace()
 ****************************************************************
 * @brief
 *  Print out the RG trace for this siots.
 *
 * @param siots_p - Ptr to this I/O.
 * @param b_start - TRUE if this is the start trace, FALSE otherwise.
 *
 * @return None.
 *
 * @author
 *  5/24/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_traffic_trace(fbe_raid_siots_t *siots_p,
                                  fbe_bool_t b_start)
{
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_u16_t read_mask = 0;
    fbe_u16_t read2_mask = 0;
    fbe_u16_t write_mask = 0;
    fbe_u64_t crc_info = 0;
    fbe_u64_t rg_info;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_group_number_t rg_number = FBE_RAID_GROUP_INVALID;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_block_count_t packet_blocks;
    fbe_lba_t iots_lba;
    fbe_block_count_t host_start_offset;
    fbe_block_count_t block_offset;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t rg_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    if ((rg_object_id != FBE_OBJECT_ID_INVALID) && (rg_object_id != FBE_U32_MAX))
    {
        fbe_database_get_rg_number(rg_object_id, &rg_number);

        if (rg_number == FBE_RAID_GROUP_INVALID)
        {
            rg_number = rg_object_id;
        }
    }

    /* Fetch the crcs for a portion of the blocks so we can trace them.
     */
    fbe_raid_siots_get_sg_ptr(siots_p, &sg_p);

    /* Our offset is the offset from the start of the overall transfer 
     * (host_start_offset) plus our offset from the start of the IOTS). 
     */
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);
    fbe_raid_iots_get_current_op_lba(iots_p, &iots_lba);
    fbe_raid_iots_get_host_start_offset(iots_p, &host_start_offset);
    block_offset = siots_p->start_lba - iots_lba + host_start_offset;

    /* In these cases do not display the data since the parent already did.
     */
    if (!fbe_raid_siots_is_nested(siots_p)      &&
         (siots_p->xfer_count <= packet_blocks) &&
         (siots_p->start_lba >= iots_lba)          ) {
        fbe_traffic_trace_get_data_crc(sg_p, siots_p->xfer_count, ((fbe_u32_t)block_offset), &crc_info);
    }

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_fruts_get_success_chain_bitmap(read_fruts_ptr,  &read_mask, &write_mask, b_start);

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    fbe_raid_fruts_get_success_chain_bitmap(read2_fruts_ptr, &read_mask, &write_mask, b_start);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fruts_get_success_chain_bitmap(write_fruts_ptr, &read_mask, &write_mask, b_start);
    /* Arg2 has: 
     *   (state << 48) | (algorithm << 40) | (read_mask << 24) | (write_mask << 8) | opcode 
     */
    rg_info = ((fbe_u64_t)write_mask) << 8;
    rg_info |= (((fbe_u64_t)read_mask | read2_mask) << 24);
    rg_info |= ( ((fbe_u64_t)siots_p->algorithm) << 40);
    rg_info |= ( ((fbe_u64_t)degraded_bitmask) << 48);

    fbe_traffic_trace_block_opcode(FBE_CLASS_ID_RAID_GROUP,
                                   siots_p->start_lba,
                                   siots_p->xfer_count,
                                   opcode,
                                   rg_number,
                                   crc_info, /*extra information*/
                                   rg_info,
                                   b_start, /*RBA trace start*/
                                   fbe_traffic_trace_get_priority(packet_p));
}
/******************************************
 * end fbe_raid_siots_traffic_trace()
 ******************************************/
