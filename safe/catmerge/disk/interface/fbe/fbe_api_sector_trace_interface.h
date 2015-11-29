#ifndef FBE_API_SECTOR_TRACE_INTERFACE_H
#define FBE_API_SECTOR_TRACE_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_sector_trace_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes for the sector trace service.
 *
 * @version
 *  06/28/2010  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_trace.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe/fbe_api_common_interface.h"

/**************************
 *   STRUCTURE DEFINITIONS
 **************************/

/*!*******************************************************************
 * @struct fbe_api_sector_trace_get_info_t
 ********************************************************************* 
 * 
 * @brief   This is fbe_api struct for getting the sector trace service info.
 *
 * @ingroup fbe_api_sector_trace_interface
 * @ingroup fbe_api_sector_trace_get_info
 *********************************************************************/
typedef struct fbe_api_sector_trace_get_info_s
{
    /*! Output: bool that states whether the sector trace service has 
     *         been initialized or not.
     */
    fbe_bool_t                      b_sector_trace_initialized;

    /*! Output: the default setting of the sector trace error level 
     *          Any error at or below (i.e. higher severity) will be
     *          added to the sector trace buffer
     */
    fbe_sector_trace_error_level_t  default_sector_trace_level;

    /*! Output: the current setting of the sector trace error level 
     *          Any error at or below (i.e. higher severity) will be
     *          added to the sector trace buffer
     */
    fbe_sector_trace_error_level_t  current_sector_trace_level;

    /*! Output: the default setting of the sector trace flags.  The 
     *          flags determine which type of errors (CRC, COH, etc)
     *          result in the sector data being traced to the sector
     *          buffer.
     */
    fbe_sector_trace_error_flags_t  default_sector_trace_flags;

    /*! Output: the current setting of the sector trace flags.  The 
     *          flags determine which type of errors (CRC, COH, etc)
     *          result in the sector data being traced to the sector
     *          buffer.
     */
    fbe_sector_trace_error_flags_t  current_sector_trace_flags;

    /*! PANIC if these sector trace errors are encountered */
    fbe_sector_trace_error_flags_t  stop_on_error_flags;
            
    /*! If True PANIC if these sector traces are encountered */
    fbe_bool_t                      b_stop_on_error_flags_enabled; 
}
fbe_api_sector_trace_get_info_t;

/*!*******************************************************************
 * @struct fbe_api_sector_trace_get_counters_t
 ********************************************************************* 
 * 
 * @brief   This is fbe_api struct for getting the sector trace counters.
 *
 * @ingroup fbe_api_sector_trace_interface
 * @ingroup fbe_api_sector_trace_get_counters
 *********************************************************************/
typedef struct fbe_api_sector_trace_get_counters_s
{
    /*! Output: The number of times the sector trace method was invoked.
     *          This includes invocations that did not result in a sector
     *          trace entry being generated (due to the fact that the
     *          requested level and flags did not meet the sector trace
     *          to ring criteria).
     */
    fbe_u32_t   total_invocations;

    /*! Output: Total number of entries placed into the sector trace
     *          buffer.
     */
    fbe_u32_t   total_errors_traced;

    /*! Output: Total number of sectors that were dumped to the sector
     *          trace buffer.
     */
    fbe_u32_t   total_sectors_traced;

    /*! Output: Array of counters, one per error level
     */
    fbe_u32_t   error_level_counters[FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT];

    /*! Output: Array of counters, one per error type 
     */
    fbe_u32_t   error_type_counters[FBE_SECTOR_TRACE_ERROR_FLAG_COUNT];
}
fbe_api_sector_trace_get_counters_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*! @note Only FBE_PACKAGE_ID_SEP_0 package is supported.
 */
fbe_status_t FBE_API_CALL fbe_api_sector_trace_get_info(fbe_api_sector_trace_get_info_t *api_info_p); 

fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_trace_level(fbe_sector_trace_error_level_t new_error_level,fbe_bool_t persist); 

fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_trace_flags(fbe_sector_trace_error_flags_t new_error_flags, fbe_bool_t persist);

fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_stop_on_error_flags(fbe_sector_trace_error_flags_t new_error_flags, fbe_u32_t additional_event,fbe_bool_t persist);

fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_stop_on_error(fbe_bool_t stop_on_error);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_get_counters(fbe_api_sector_trace_get_counters_t *api_counters_p);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_reset_counters(void);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_desribe_info(fbe_api_sector_trace_get_info_t *sector_info_p,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_bool_t b_verbose);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_display_info(fbe_bool_t b_verbose);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_desribe_counters(fbe_api_sector_trace_get_counters_t *sector_counters_p,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_bool_t b_verbose);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_display_counters(fbe_bool_t b_verbose);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_restore_default_level(void);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_restore_default_flags(void);
 
fbe_status_t FBE_API_CALL fbe_api_sector_trace_flag_to_index(fbe_sector_trace_error_flags_t error_flag,
                                                             fbe_u32_t *index_p); 

fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_encryption_mode(fbe_system_encryption_mode_t new_mode,
                                                                   fbe_bool_t b_retail_build);

/**********************************************
 * end file fbe_api_sector_trace_interface.h
 **********************************************/
#endif /* FBE_API_SECTOR_TRACE_INTERFACE_H */
