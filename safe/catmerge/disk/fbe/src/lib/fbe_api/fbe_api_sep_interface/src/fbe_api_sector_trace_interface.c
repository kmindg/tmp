/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    fbe_api_sector_trace_interface.c
 *****************************************************************************
 *
 * @brief   This file contaoins the APIs for the sector trace service.  This
 *          includes APIs to set the sector trace error level (sector trace
 *          at or below this level (and function enabled) will be traced to
 *          the xor sector trace buffer.  Also contains the API to set the
 *          sector trace flags which determine which error types are traced.   
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_sector_trace_interface
 *
 *
 *****************************************************************************/

/**************************************
                Includes
**************************************/
#include "fbe_trace.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

static void fbe_api_sector_trace_default_trace_func(fbe_trace_context_t trace_context, const fbe_char_t * fmt, ...)
{
    va_list ap;
    fbe_char_t string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    va_end(ap);
    fbe_printf("%s", string_to_print);
    return;
}

/****************************************************************
 *  fbe_api_sector_trace_error_flag_to_string
 ****************************************************************
 * @brief
 *      Converts the passed error flag to a string.
 *      The second parameter is the `verbose' mode.
 *  
 * @param   trace_error_mask - Error type flag
 * @param   verbose - Display additional information about the flag
 * @param   trace_error_flag_string_p - Pointer to destination string
 * @param   string_length - Length of string passed
 *
 * @return  status
 *
 * @author
 *  02/23/2010 :Omer Miranda - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_api_sector_trace_error_flag_to_string(fbe_sector_trace_error_flags_t trace_error_flag, 
                                                             fbe_bool_t verbose,
                                                             fbe_char_t *trace_error_flag_string_p,
                                                             fbe_u32_t string_length)
{
    fbe_char_t       *flag_string;
    fbe_char_t       *descr_string;
    
    /* Validate input string is long enough
     */
    if (string_length < FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The goal of all this code is to have a single location that 
     * needs to change when a flag is added or removed!!
     */
    switch(trace_error_flag)
    {
        case FBE_SECTOR_TRACE_ERROR_FLAG_NONE:
            flag_string = "NONE ";
            descr_string = "No error reported";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_UCRC:
            flag_string = "UCRC ";                                       
            descr_string = "Unrecognizable CRC occurs";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_COH:                         
            flag_string = "COH  ";
            descr_string = "Consistency error occurs";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_ECOH:                         
            flag_string = "ECOH ";
            descr_string = "Expected coherency error";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_TS:
            flag_string = "TS   ";
            descr_string = "Error in Timestamp";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_WS:
            flag_string = "WS   ";
            descr_string = "Writestamp error";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_SS:
            flag_string = "SS   ";
            descr_string = "Shedstamp error";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_POC:
            flag_string = "POC  ";
            descr_string = "Parity of checksums error. (R6)";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_NPOC:
            flag_string = "NPOC ";
            descr_string = "Multiple Parity of checksums error. (R6)";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_UCOH:
            flag_string = "UCOH ";
            descr_string = "An unknown coherency error occurred. (R6)";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_ZERO:
            flag_string = "ZERO ";
            descr_string = "Should have been zeroed and was not. (R6)";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_LBA:
            flag_string = "LBA  ";
            descr_string = "An LBA stamp error (w/o CRC) has occurred.";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_RCRC:
            flag_string = "RCRC ";
            descr_string = "Retryable CRC error.";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_UNU1:
            flag_string = "UNU1 ";
            descr_string = "Currently unused!!";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_RINV:
            flag_string = "RINV ";
            descr_string = "RAID Invalidated this sector";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_RAID:
            flag_string = "RAID ";
            descr_string = "RAID for non-specfic error.";
            break;
        case FBE_SECTOR_TRACE_ERROR_FLAG_LOC:
            flag_string = "LOC  ";
            descr_string = "Enables saving function and line for ALL events!!";
            break;
        default:
            flag_string = "INV! ";
            descr_string = "UNKNOWN FLAG SET: FIX THIS!!";
            break;
    }

    /* If verbose display flag name and other information.
     */
    if (verbose)
    {
        _snprintf(trace_error_flag_string_p, (FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN - 1), 
                 "FBE_SECTOR_TRACE_ERROR_FLAG_%s: 0x%08X - %s",
                 flag_string, trace_error_flag, descr_string);
        trace_error_flag_string_p[(FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN - 1)] = '\0';
        return FBE_STATUS_OK;
    }

    /* Copy string and return
     */
    if (strlen(flag_string) >= string_length)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    strncpy(trace_error_flag_string_p, flag_string, strlen(flag_string) + 1); 
    return FBE_STATUS_OK;
}/* fbe_api_sector_trace_error_flag_to_string */

/*!***************************************************************************
 *          fbe_api_sector_trace_level_to_string
 *****************************************************************************
 * 
 * @brief   Returns a description for the specified sector trace level
 *  
 * @param   sector_trace_level  - The level that the description is required for.
 * @param   sector_trace_level_string_p - Pointer of sector level string to update
 * @param   string_length - Length of input string
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  02/23/2010: Omer Miranda - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_api_sector_trace_level_to_string(fbe_sector_trace_error_level_t sector_trace_level,
                                                         fbe_char_t *sector_trace_level_string_p,
                                                         fbe_u32_t string_length)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_char_t     *descr_string;
    
    /* The goal of all this code is to have a single location that 
     * needs to change when a flag is added or removed!!
     */
    switch(sector_trace_level)
    {
        case FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_CRITICAL - Unexpected error (possible DL)";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_ERROR    - Non fatal error.(DL?)";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_WARNING  - Error maybe recoverable.";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_INFO:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_INFO     - Recoverable error.";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_DATA:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_DATA     - Trace all user and meta data.";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_DEBUG:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_DEBUG    - Additional debug tracing.";
            break;
        case FBE_SECTOR_TRACE_ERROR_LEVEL_VERBOSE:
            descr_string = "FBE_SECTOR_TRACE_LEVEL_VERBOSE  - Trace every event.";
            break;
        default:
            descr_string = "Unknown trace level.  No additional tracing.";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    } 

    /* Return the description string and status
     */
    if (strlen(descr_string) >= string_length)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    strncpy(sector_trace_level_string_p, descr_string, strlen(descr_string) + 1); 
    return(status);
} /* fbe_api_sector_trace_level_to_string */

/*!***************************************************************************
 *          fbe_api_sector_trace_generate_error_flags_string()
 ***************************************************************************** 
 * 
 * @brief   Generates a single string with the brief description of each sector
 *          trace error flags set in the sector trace flags passed.
 *  
 * @param   sector_trace_flags - The sector trace flags to generate string for
 * @param   sector_flags_string_p - Pointer to string to update
 * @param   string_length - Length of input string
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/303/2010 Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_api_sector_trace_generate_error_flags_string(fbe_sector_trace_error_flags_t sector_trace_flags,
                                                                     fbe_char_t *sector_flags_string_p,
                                                                     fbe_u32_t string_length)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_char_t     *mask_string = sector_flags_string_p;
    fbe_char_t     *flag_string_p = NULL;
    fbe_s32_t       error_flag_index;
    fbe_u32_t       local_flag;

    /* Validate input length
     */
    if (string_length < FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate a temporary flag string
     */
    flag_string_p = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
    if (flag_string_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Terminate string and add flags
     */
    mask_string[0] = '\0';
    for (error_flag_index = 0; error_flag_index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT; error_flag_index++)
    {
        local_flag = (1 << error_flag_index);
        if (local_flag & sector_trace_flags)
        {
            size_t mask_len = strlen(mask_string);

            if ((mask_len + FBE_SECTOR_TRACE_ERROR_FLAGS_STRING_LEN) > FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s attempt to go beyond length of mask_string %llu %llu %d\n",
                              __FUNCTION__, (unsigned long long)mask_len,
                             (unsigned long long)strlen(flag_string_p), 
                              FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN);
                fbe_api_free_memory(flag_string_p);
                return(FBE_STATUS_GENERIC_FAILURE);
            }

            status = fbe_api_sector_trace_error_flag_to_string(local_flag, FBE_FALSE, 
                                                               flag_string_p, FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s Failed to get flag string for 0x%x\n",
                              __FUNCTION__, local_flag); 
                fbe_api_free_memory(flag_string_p);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            if ((mask_len + strlen(flag_string_p)) > FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s attempt to go beyond length of mask_string %llu %llu %d\n",
                              __FUNCTION__, (unsigned long long)mask_len,
                             (unsigned long long)strlen(flag_string_p), 
                              FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN);
                fbe_api_free_memory(flag_string_p);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            strncat(mask_string, flag_string_p, strlen(flag_string_p) + 1);
        }
    }
    mask_string[(FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN - 1)] = '\0';

    /* Free out local resource and return status
     */
    fbe_api_free_memory(flag_string_p);
    return(FBE_STATUS_OK);

} 
/* end of fbe_api_sector_trace_generate_error_flags_string() */

/*!***************************************************************************
 *          fbe_api_sector_trace_get_info()
 ***************************************************************************** 
 * 
 * @brief   Populate the sector trace service information which includes is
 *          the service initialized, the current sector trace level and the
 *          current sector trace flags.
 *
 * @param   api_info_p - Pointer to sector api information block
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_get_info(fbe_api_sector_trace_get_info_t *api_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_get_info_t get_info;

    /* Validate the request
     */
    if (api_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: null api_info_p pointer \n", 
                      __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&get_info, sizeof(fbe_sector_trace_get_info_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO,
                                                           &get_info,
                                                           sizeof(fbe_sector_trace_get_info_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else populate the requested buffer
     */
    api_info_p->b_sector_trace_initialized = get_info.b_sector_trace_initialized;
    api_info_p->default_sector_trace_level = get_info.default_sector_trace_level;
    api_info_p->current_sector_trace_level = get_info.current_sector_trace_level;
    api_info_p->default_sector_trace_flags = get_info.default_sector_trace_flags;
    api_info_p->current_sector_trace_flags = get_info.current_sector_trace_flags;
    api_info_p->stop_on_error_flags        = get_info.stop_on_error_flags;
    api_info_p->b_stop_on_error_flags_enabled = get_info.b_stop_on_error_flags_enabled;

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_get_info() */

/*!***************************************************************************
 *          fbe_api_sector_trace_set_trace_level()
 ***************************************************************************** 
 * 
 * @brief   Change the sector trace `tracing level' which determines if the
 *          error should result in the sector being saved in the sector trace
 *          buffer or not. All errors equal to or less than the error level
 *          passed will result in the sector being traced.
 *
 * @param   new_error_level - The new error level to set for the sector trace
 *          service
 *             persist - persist the setting if true
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 * 
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_trace_level(fbe_sector_trace_error_level_t new_error_level,fbe_bool_t persist)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_set_level_t    set_level;

    /* Validate the request
     */
    if ((new_error_level <  FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL) ||
        (new_error_level >  FBE_SECTOR_TRACE_ERROR_LEVEL_MAX)         )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: Error level must be between %d and %d inclusive\n", 
                      __FUNCTION__, FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL, FBE_SECTOR_TRACE_ERROR_LEVEL_MAX); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to set the sector trace error level.
     * Any error (that is enabled) of this level or lower will get recorded 
     * in the sector trace buffer. 
     */
    set_level.new_sector_trace_level = new_error_level;
    set_level.persist = persist;
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_SET_LEVEL,
                                                           &set_level,
                                                           sizeof(fbe_sector_trace_set_level_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_set_trace_level() */

/*!***************************************************************************
 *          fbe_api_sector_trace_set_trace_flags()
 ***************************************************************************** 
 * 
 * @brief   Change the sector trace `trace flags' which determines if the
 *          error should result in the sector being saved in the sector trace
 *          buffer or not.  The error flags determine which type (i.e. CRC,
 *          COH, RAID, etc) of errors should result in the sector being saved
 *          to the sector trace buffer or not.
 *
 * @param   new_error_flags - The new error flags to set for the sector trace
 *          service
*             persist - persist the setting if true
* 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_trace_flags(fbe_sector_trace_error_flags_t new_error_flags,fbe_bool_t persist)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_set_flags_t    set_flags;
    fbe_sector_trace_error_flags_t  invalid_flags;

    /* Validate the request
     */
    invalid_flags = ~(FBE_SECTOR_TRACE_ERROR_FLAG_MASK);
    if ((new_error_flags & invalid_flags) != 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: Error flags must be between 0x%x and 0x%x inclusive\n", 
                      __FUNCTION__, FBE_SECTOR_TRACE_ERROR_FLAG_NONE, FBE_SECTOR_TRACE_ERROR_FLAG_MAX); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    set_flags.new_sector_trace_flags = new_error_flags;
    set_flags.persist = persist;
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_SET_FLAGS,
                                                           &set_flags,
                                                           sizeof(fbe_sector_trace_set_flags_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_set_trace_flags() */

/*!***************************************************************************
 *          fbe_api_sector_trace_set_stop_on_error()
 ***************************************************************************** 
 * 
 * @brief   Change the sector trace to stop_on_error.
 *
 * @param   stop_on_err - FBE_TRUE to stop
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 * 
 * @author
 *  11/11/2011  nchiu  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_stop_on_error(fbe_bool_t stop_on_err)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_stop_system_on_error_t    set_stop;

    /* Send out the request to set the sector trace error level.
     * Any error (that is enabled) of this level or lower will get recorded 
     * in the sector trace buffer. 
     */
    set_stop.b_stop_system = stop_on_err;
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_STOP_SYSTEM_ON_ERROR,
                                                           &set_stop,
                                                           sizeof(fbe_sector_trace_stop_system_on_error_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_set_stop_on_error() */

/*!***************************************************************************
 *          fbe_api_sector_trace_set_stop_on_error_flags()
 ***************************************************************************** 
 * 
 * @brief   Change the sector trace `debug flags' which determines if the
 *          error should be triggered error count, and possible stop the system.
 *
 * @param   new_error_flags - The new error flags to set for the sector trace
 *          service
 *               addtional_event  To stop on addtional events specified, 0 for none
 *               persist -persist to registry
 *           
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  11/11/2011  nchiu  - copied from fbe_api_sector_trace_set_trace_flags()
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_stop_on_error_flags(fbe_sector_trace_error_flags_t new_error_flags,fbe_u32_t additional_event,fbe_bool_t persist)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_set_stop_on_error_flags_t    set_flags;
    fbe_sector_trace_error_flags_t  invalid_flags;

    /* Validate the request
     */
    invalid_flags = ~(FBE_SECTOR_TRACE_ERROR_FLAG_MASK);
    if ((new_error_flags & invalid_flags) != 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: Error flags must be between 0x%x and 0x%x inclusive\n", 
                      __FUNCTION__, FBE_SECTOR_TRACE_ERROR_FLAG_NONE, FBE_SECTOR_TRACE_ERROR_FLAG_MAX); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    set_flags.new_sector_trace_stop_on_error_flags = new_error_flags;
    set_flags.persist = persist;
	set_flags.additional_event = additional_event;
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_SET_STOP_ON_ERROR_FLAGS,
                                                           &set_flags,
                                                           sizeof(fbe_sector_trace_set_stop_on_error_flags_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_set_stop_on_error_flags() */

/*!***************************************************************************
 *          fbe_api_sector_trace_get_counters()
 ***************************************************************************** 
 * 
 * @brief   Retrieve the counters for the sector trace service.  This includes
 *          the number of times the sector trace service was invoked and
 *          the total number of sectors that were saved to the sector trace
 *          buffer.
 *
 * @param   api_counters_p - Pointer to sector api counter block
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  06/30/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_get_counters(fbe_api_sector_trace_get_counters_t *api_counters_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_get_counters_t get_counters;
    fbe_u32_t                   index;

    /* Validate the request
     */
    if (api_counters_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: null api_counters_p pointer \n", 
                      __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&get_counters, sizeof(fbe_sector_trace_get_counters_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_GET_COUNTERS,
                                                           &get_counters,
                                                           sizeof(fbe_sector_trace_get_counters_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else populate the requested buffer
     */
    api_counters_p->total_invocations = get_counters.total_invocations;
    api_counters_p->total_errors_traced = get_counters.total_errors_traced;
    api_counters_p->total_sectors_traced = get_counters.total_sectors_traced;

    /* Populate the error level counters
     */
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT; index++)
    {
        api_counters_p->error_level_counters[index] = get_counters.error_level_counters[index];
    }

    /* Populate the error type counters
     */
    for (index = 0; index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT; index++)
    {
        api_counters_p->error_type_counters[index] = get_counters.error_type_counters[index];
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_get_counters() */

/*!***************************************************************************
 *          fbe_api_sector_trace_set_trace_flags()
 ***************************************************************************** 
 * 
 * @brief   Change the sector trace `trace flags' which determines if the
 *          error should result in the sector being saved in the sector trace
 *          buffer or not.  The error flags determine which type (i.e. CRC,
 *          COH, RAID, etc) of errors should result in the sector being saved
 *          to the sector trace buffer or not.
 *
 * @param   None
 * 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_reset_counters(void)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_reset_counters_t       reset_counters;

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&reset_counters, sizeof(fbe_sector_trace_reset_counters_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_RESET_COUNTERS,
                                                           &reset_counters,
                                                           sizeof(fbe_sector_trace_reset_counters_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_reset_counters() */

/*!***************************************************************************
 *          fbe_api_sector_trace_desribe_cleanup()
 ***************************************************************************** 
 * 
 * @brief   Describe info cleanup.  Perform any cleanup
 *
 * @param   default_trace_level_string
 * @param   current_trace_level_string
 * @param   default_trace_flags_string
 * @param   current_trace_flags_string
 * 
 * @return  None
 *
 *****************************************************************************/
static void FBE_API_CALL fbe_api_sector_trace_desribe_cleanup(fbe_char_t *default_trace_level_string,
                                                              fbe_char_t *current_trace_level_string,
                                                              fbe_char_t *default_trace_flags_string,
                                                              fbe_char_t *current_trace_flags_string)
{
    if (default_trace_level_string != NULL) { fbe_api_free_memory(default_trace_level_string); }
    if (current_trace_level_string != NULL) { fbe_api_free_memory(current_trace_level_string); }
    if (default_trace_flags_string != NULL) { fbe_api_free_memory(default_trace_flags_string); }
    if (current_trace_flags_string != NULL) { fbe_api_free_memory(current_trace_flags_string); }
    return;

}
/* end of fbe_api_sector_trace_desribe_cleanup()*/

/*!***************************************************************************
 *          fbe_api_sector_trace_desribe_info()
 ***************************************************************************** 
 * 
 * @brief   Describes (i.e. dumps to trace function) the sector trace
 *          information passed.  This includes the sector trace error level and
 *          sector trace flags.
 *
 * @param   sector_info_p - Pointer to sector trace information to describe
 * @param   trace_func - The trace function to describe into
 * @param   trace_context - Context to use with trace function
 * @param   b_verbose - Determinse if verbose information is dumped or not
 * 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  06/28/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_desribe_info(fbe_api_sector_trace_get_info_t *sector_info_p,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_bool_t b_verbose)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_char_t     *default_trace_level_string = NULL;
    fbe_char_t     *current_trace_level_string = NULL;
    fbe_char_t     *default_trace_flags_string = NULL;
    fbe_char_t     *current_trace_flags_string = NULL;
    fbe_u32_t       index;

    /* Validate arguments
     */
    if ((sector_info_p == NULL) ||
        (trace_func == NULL)       )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - sector_info_p: 0x%p and trace_func: 0x%p must be valid \n",
                      __FUNCTION__, sector_info_p, trace_func);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*  Allocate memory for each of our strings
     */
    default_trace_level_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
    current_trace_level_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
    default_trace_flags_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN);
    current_trace_flags_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
    if ((default_trace_level_string == NULL) || 
        (current_trace_level_string == NULL) ||
        (default_trace_flags_string == NULL) ||
        (current_trace_flags_string == NULL)    )
    {
        /* Report the error and free the allocated strings
         */
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to allocate %d bytes for sector trace strings \n",
                      __FUNCTION__, ((FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN * 2) + (FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN * 2)));
        fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                             default_trace_flags_string, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the string value for the default and current sector trace level.
     */
    status = fbe_api_sector_trace_level_to_string(sector_info_p->default_sector_trace_level,
                                                  default_trace_level_string,
                                                  FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to get default trace level. status: %d \n",
                      __FUNCTION__, status);
        fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                             default_trace_flags_string, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    status = fbe_api_sector_trace_level_to_string(sector_info_p->current_sector_trace_level,
                                                  current_trace_level_string,
                                                  FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to get current trace level. status: %d \n",
                      __FUNCTION__, status);
        fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                             default_trace_flags_string, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the string value for the default and current sector trace flags.
     */
    status = fbe_api_sector_trace_generate_error_flags_string(sector_info_p->default_sector_trace_flags,
                                                              default_trace_flags_string,
                                                              FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to get default trace flags. status: %d \n",
                      __FUNCTION__, status);
        fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                             default_trace_flags_string, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    status = fbe_api_sector_trace_generate_error_flags_string(sector_info_p->current_sector_trace_flags,
                                                              current_trace_flags_string,
                                                              FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to get current trace flags. status: %d \n",
                      __FUNCTION__, status);
        fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                             default_trace_flags_string, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Now `trace' the sector trace level to the output
     */
    trace_func(trace_context, " Sector Trace Level: Determines Which Level of Errors will be Logged.\n");
    trace_func(trace_context, "                     Use -set_trace_level (-stl) to change.\n");
    trace_func(trace_context, "   Current Level: %d - %s \n",
               sector_info_p->current_sector_trace_level, current_trace_level_string);
    trace_func(trace_context, "   Default Level: %d - %s \n",
               sector_info_p->default_sector_trace_level, default_trace_level_string);
    trace_func(trace_context, "------------------------------------------------------------------------------------------\n");

    /* If verbose was selected, display the meaning of each level
     */
    if (b_verbose == FBE_TRUE)
    {
        for (index = 0; index <= FBE_SECTOR_TRACE_ERROR_LEVEL_MAX; index++)
        {
            status = fbe_api_sector_trace_level_to_string(index,
                                                          current_trace_level_string,
                                                          FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s - Failed to get trace level. status: %d \n",
                              __FUNCTION__, status);
                fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                                     default_trace_flags_string, current_trace_flags_string);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            trace_func(trace_context, "\t %d - %s \n",
                       index, current_trace_level_string);
        }
    }

    /* Now trace the sector trace flags to the output
     */
    trace_func(trace_context, " Sector Trace Mask: Determines Which Type of Errors will be Logged.\n");
    trace_func(trace_context, "                    Use -set_trace_mask (-stm) to change.\n");
    trace_func(trace_context, "   Current Mask: 0x%08X - %s \n",
               sector_info_p->current_sector_trace_flags, current_trace_flags_string);
    trace_func(trace_context, "   Default Mask: 0x%08X - %s \n",
               sector_info_p->default_sector_trace_flags, default_trace_flags_string);
    trace_func(trace_context, "---------------------------------------------------------------------------------------\n");

    /* If verbose was selected, display the meaning of each flag
     */
    if (b_verbose == FBE_TRUE)
    {
        for (index = 0; 
             ((index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT) && (sector_info_p->current_sector_trace_flags != 0)); 
             index++)
        {
            status = fbe_api_sector_trace_error_flag_to_string((1 << index), 
                                                               FBE_TRUE,
                                                               current_trace_flags_string, 
                                                               FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s - Failed to get error flag. status: %d \n",
                              __FUNCTION__, status);
                fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                                     default_trace_flags_string, current_trace_flags_string);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            trace_func(trace_context, "\t 0x%08x - %s \n",
                       (1 << index), current_trace_flags_string);
        }
    }

    /* Display the stop on error information.
     */
    trace_func(trace_context, "   Stop On Error Mask: 0x%08X  Stop on Error Enabled: %d\n",
               sector_info_p->stop_on_error_flags, sector_info_p->b_stop_on_error_flags_enabled);

    /* Perform any neccessary cleanup
     */
    fbe_api_sector_trace_desribe_cleanup(default_trace_level_string, current_trace_level_string,
                                         default_trace_flags_string, current_trace_flags_string);    

    /* Always return the executiuon status
     */
    return(status);
}
/* end of fbe_api_sector_trace_desribe_info() */

/*!***************************************************************************
 *          fbe_api_sector_trace_display_info()
 ***************************************************************************** 
 * 
 * @brief   Dump the sector trace information (sector trace level, sector trace
 *          flags, etc) to the standard output.
 *  
 * @param   b_verbose - Display detailed information or not
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/303/2010 Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_display_info(fbe_bool_t b_verbose)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_sector_trace_get_info_t sector_trace_info;

    /* First get the sector trace information
     */
    status = fbe_api_sector_trace_get_info(&sector_trace_info);
    if (status != FBE_STATUS_OK)
    {
        return(status);
    }

    /* Now `describe' (i.e. print) the sector trace information using our
     * print function.
     */
    status = fbe_api_sector_trace_desribe_info(&sector_trace_info,
                                               fbe_api_sector_trace_default_trace_func,
                                               (fbe_trace_context_t)NULL,
                                               b_verbose);

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_display_info()*/

/*!***************************************************************************
 *          fbe_api_sector_trace_desribe_counters()
 ***************************************************************************** 
 * 
 * @brief   Describes (i.e. dumps to trace function) the sector trace
 *          counters passed.
 *
 * @param   sector_counter_p - Pointer to sector trace counters to describe
 * @param   trace_func - The trace function to describe into
 * @param   trace_context - Context to use with trace function
 * @param   b_verbose - Determinse if verbose information is dumped or not
 * 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  06/30/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_desribe_counters(fbe_api_sector_trace_get_counters_t *sector_counters_p,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_bool_t b_verbose)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t       *current_trace_level_string = NULL;
    fbe_u8_t       *current_trace_flags_string = NULL;
    fbe_u32_t       index;

    /* Validate arguments
     */
    if ((sector_counters_p == NULL) ||
        (trace_func == NULL)       )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - sector_counters_p: 0x%p and trace_func: 0x%p must be valid \n",
                      __FUNCTION__, sector_counters_p, trace_func);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Allocate memory for level and flags strings
     */
    current_trace_level_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
    current_trace_flags_string = fbe_api_allocate_memory(FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
    if ((current_trace_level_string == NULL) ||
        (current_trace_flags_string == NULL)    )
    {
        /* Report the error and free the allocated strings
         */
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - Failed to allocate %d bytes for sector trace strings \n",
                      __FUNCTION__, (FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN + FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN));
        fbe_api_sector_trace_desribe_cleanup(NULL, current_trace_level_string,
                                             NULL, current_trace_flags_string);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Display the total invocations, errors and sectors dumped to sector
     * trace buffer
     */
    trace_func(trace_context, " Sector Trace Counters: Times the sector trace sevices was called\n");
    trace_func(trace_context, "   Total Invocations:     %010d\n",
               sector_counters_p->total_invocations);
    trace_func(trace_context, "   Total Trace Entries:   %010d\n",
               sector_counters_p->total_errors_traced);
    trace_func(trace_context, "   Total Sectors Traced:  %010d\n",
               sector_counters_p->total_sectors_traced);
    trace_func(trace_context, "------------------------------------------------------------------------------------------\n");

    /* If verbose was selected, display the meaning of each level
     */
    if (b_verbose == FBE_TRUE)
    {
        for (index = 0; index <= FBE_SECTOR_TRACE_ERROR_LEVEL_MAX; index++)
        {
            status = fbe_api_sector_trace_level_to_string(index,
                                                          current_trace_level_string,
                                                          FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s - Failed to get trace level. status: %d \n",
                              __FUNCTION__, status);
                fbe_api_sector_trace_desribe_cleanup(NULL, current_trace_level_string,
                                                     NULL, current_trace_flags_string);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            trace_func(trace_context, "\t level[%2d] count: %010d (%s) \n",
                       index, sector_counters_p->error_level_counters[index], current_trace_level_string);
        }
        trace_func(trace_context, "------------------------------------------------------------------------------------------\n");
    }

    /* If verbose was selected, display the meaning of each flag
     */
    if (b_verbose == FBE_TRUE)
    {
        for (index = 0; index < FBE_SECTOR_TRACE_ERROR_FLAG_COUNT; index++)
        {
             status = fbe_api_sector_trace_error_flag_to_string((1 << index),
                                                                FBE_FALSE,
                                                                current_trace_flags_string,
                                                                FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s - Failed to get trace flags. status: %d \n",
                              __FUNCTION__, status);
                fbe_api_sector_trace_desribe_cleanup(NULL, current_trace_level_string,
                                                     NULL, current_trace_flags_string);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            trace_func(trace_context, "\t flags[%2d] count: %010d (%s) \n",
                       index, sector_counters_p->error_type_counters[index], current_trace_flags_string);
        }
        trace_func(trace_context, "------------------------------------------------------------------------------------------\n");
    }
    
    /* Free any allocated resources
     */
    fbe_api_sector_trace_desribe_cleanup(NULL, current_trace_level_string,
                                         NULL, current_trace_flags_string);

    /* Always return the executiuon status
     */
    return(status);
}
/* end of fbe_api_sector_trace_desribe_counters() */

/*!***************************************************************************
 *          fbe_api_sector_trace_display_counters()
 ***************************************************************************** 
 * 
 * @brief   Dump the sector counters information to the standard output.
 *  
 * @param   b_verbose - Display detailed information or not
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/303/2010 Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_display_counters(fbe_bool_t b_verbose)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;

    /* First get the sector trace counters
     */
    status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
    if (status != FBE_STATUS_OK)
    {
        return(status);
    }

    /* Now `describe' (i.e. print) the sector trace counters using our
     * print function.
     */
    status = fbe_api_sector_trace_desribe_counters(&sector_trace_counters,
                                               fbe_api_sector_trace_default_trace_func,
                                               (fbe_trace_context_t)NULL,
                                               b_verbose);

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_display_counters()*/

/*!***************************************************************************
 *          fbe_api_sector_trace_restore_default_level()
 ***************************************************************************** 
 * 
 * @brief   Set the sector trace level back to it's default
 *
 * @param   None
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  07/23/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_restore_default_level(void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_get_info_t get_info;

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&get_info, sizeof(fbe_sector_trace_get_info_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO,
                                                           &get_info,
                                                           sizeof(fbe_sector_trace_get_info_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else if the current doesn't equal the default set the current back
     * to the default.
     */
    if (get_info.current_sector_trace_level != get_info.default_sector_trace_level)
    {
        status = fbe_api_sector_trace_set_trace_level(get_info.default_sector_trace_level,FBE_FALSE);
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_restore_default_level() */

/*!***************************************************************************
 *          fbe_api_sector_trace_restore_default_level()
 ***************************************************************************** 
 * 
 * @brief   Set the sector trace flags back to it's default
 *
 * @param   None
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  07/23/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_restore_default_flags(void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_get_info_t get_info;

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&get_info, sizeof(fbe_sector_trace_get_info_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO,
                                                           &get_info,
                                                           sizeof(fbe_sector_trace_get_info_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else if the current doesn't equal the default set the current back
     * to the default.
     */
    if (get_info.current_sector_trace_flags != get_info.default_sector_trace_flags)
    {
        status = fbe_api_sector_trace_set_trace_flags(get_info.default_sector_trace_flags,FBE_FALSE);
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_sector_trace_restore_default_flags() */

/*!***************************************************************************
 *          fbe_api_sector_trace_flag_to_index()
 *****************************************************************************
 *
 * @brief   Convert the sector trace error flag to an index into the error
 *          counters array index.
 *  
 * @param   error_flag - The error flag for the error reported
 * @param   index_p - Index of error counter
 * 
 * @note    If more than (1) bit is set it is an error.
 * 
 * @return  status
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_sector_trace_flag_to_index(fbe_sector_trace_error_flags_t error_flag,
                                                              fbe_u32_t *index_p)
{
    fbe_u32_t   bits_set = 0;
    fbe_u32_t   bit_position;

    /* Initialize the output
     */
    *index_p = (fbe_u32_t)-1;

    /* Validate error flag.
     */
    if (error_flag > FBE_SECTOR_TRACE_ERROR_FLAG_MAX)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: error_flag: 0x%x greater than max: 0x%x\n", 
                      __FUNCTION__, error_flag, FBE_SECTOR_TRACE_ERROR_FLAG_MAX);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Count total bits set in a bitmask.
     */
    for (bit_position = 0; bit_position < (sizeof(error_flag) * FBE_BYTE_NUM_BITS); bit_position++)
    {
        if ((error_flag & (1 << bit_position)) != 0)
        {
            bits_set++;
            if (bits_set > 1)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                              "%s: More than 1 bit set in error_flag: 0x%x \n", 
                              __FUNCTION__, error_flag);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            *index_p = bit_position;
        }
    }

    /* If no bits are set it is an error.
     */
    if (bits_set != 1)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: 1 bit must be set in error_flag: 0x%x \n", 
                      __FUNCTION__, error_flag);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success
     */
    return FBE_STATUS_OK;
}
/* end of fbe_api_sector_trace_flag_to_index()*/


fbe_status_t FBE_API_CALL fbe_api_sector_trace_set_encryption_mode(fbe_system_encryption_mode_t new_mode,
                                                                   fbe_bool_t b_external_build)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sector_trace_encryption_mode_t      set_enc_mode;

    /* Send out the request to populate the sector trace information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&set_enc_mode, sizeof(fbe_sector_trace_encryption_mode_t));
    set_enc_mode.encryption_mode = new_mode;
    set_enc_mode.b_is_external_build = b_external_build;

    status = fbe_api_common_send_control_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_SET_ENCRYPTION_MODE,
                                                           &set_enc_mode,
                                                           sizeof(fbe_sector_trace_encryption_mode_t),
                                                           FBE_SERVICE_ID_SECTOR_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status
     */
    return(status);
}


/********************************************
 * end of fbe_api_sector_trace_interface.c
 *******************************************/
