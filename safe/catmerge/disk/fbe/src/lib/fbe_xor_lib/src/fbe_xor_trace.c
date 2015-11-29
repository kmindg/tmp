/*******************************************************************
 * Copyright (C) EMC Corporation 2000 - 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *
 *  @brief
 *    This file contains methods for Xor library debug tracing.
 *      
 *  @author
 *
 *  04-16-2008  RDP - Original version.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_library_interface.h"
#include "fbe_xor_private.h"
#include "fbe_trace.h"
/***********************
 * IMPORTS
 ***********************/
 
/***********************
 * GLOBAL VARIABLES
 ***********************/


/****************************************
 * MACROS
 ****************************************/


/****************************************
 * FUNCTIONS
 ****************************************/

/*!***************************************************************************
 *          fbe_xor_trace_pointer_to_u32()                              
 *****************************************************************************
 *
 * @brief
 *    This function converts (and thus may truncate) a pointer to a 32-bit
 *    quantity.
 *
 * @param
 *     void * - Pointer 
 *
 * @return
 *     UINT_32  - 32-bit (truncated if neccessary) pointer value
 *
 * @note
 *     This function will truncate 64-bit pointer to the lower 32-bits
 *
 * @author
 *     06/04/2009   Ron Proulx - Created
 *
 *************************************************************************/
static fbe_u32_t fbe_xor_trace_pointer_to_u32(void *ptr)
{
    fbe_u64_t temp;
    
#if !defined(_WIN64)
    temp = (fbe_u32_t)ptr;
#else
    temp = (fbe_u64_t)ptr;
#endif
    return((fbe_u32_t)temp);
}
/* end of fbe_xor_trace_pointer_to_u32() */

/*!**************************************************************
 * fbe_xor_library_trace()
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

void fbe_xor_library_trace(fbe_trace_level_t trace_level,
                           fbe_u32_t line,
                           const char *const function_p,
                           fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_XOR_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (trace_level > fbe_trace_get_default_trace_level())
    {
        return;
    }

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_XOR_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_XOR_MAX_TRACE_CHARS-1] = '\0';
        va_end(argList);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "xor: line: %d function: %s\n", line, function_p);
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s\n", buffer);
    return;
}
/**************************************
 * end fbe_xor_library_trace()
 **************************************/

/*!***************************************************************************
 *          fbe_xor_trace_sector_basic()
 *****************************************************************************
 *
 * @brief   Traces a portion of a sector (instead of teh entire sector contents).
 *
 * @param   sector_p - Pointer to fbe_sector_t in error
 * @param   seed - LBA in error
 *
 * @return  fbe_status_t
 *
 * @author  
 *  08/11/2011  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_xor_trace_sector_basic(fbe_sector_t *const sector_p,
                                        const fbe_lba_t seed)
{
    fbe_u64_t          *data_ptr = NULL;

    /* XOR should only issue requests with a valid sector pointer
     */
    data_ptr = (fbe_u64_t *)sector_p;
    if (data_ptr == NULL)
    {
        /* Some request may not have data associated with them.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "secp: %08x l:%016llx no data to dump",
                              fbe_xor_trace_pointer_to_u32(sector_p),
			      (unsigned long long)seed);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else dump the first and last 16 bytes of the first and last blocks
     * of the request.  Since stripped mirrors use a different block
     * size, assert if that is the case.
     * Note!! The `:' matters so that we can screen for sector_p and lba etc!!
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "secp: %08x l:%016llx %016llx %016llx %016llx %016llx \n",
                           fbe_xor_trace_pointer_to_u32(sector_p),
			   (unsigned long long)seed, 
                           (unsigned long long)data_ptr[0],
			   (unsigned long long)data_ptr[1], 
                           (unsigned long long)data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                           (unsigned long long)data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_xor_trace_sector_basic()
 **************************************/

/*!***************************************************************************
 *          fbe_xor_trace_checksum_errored_sector()
 *****************************************************************************
 *
 * @brief   Simply traces information about a sector that detected a checksum
 *          error.
 *
 * @param   sector_p - Pointer to fbe_sector_t in error
 * @param   position_mask - Position bitmask of error
 * @param   seed - LBA in error
 * @param   option - XOR options selected
 * @param   xor_status - XOR status for this block
 *
 * @return  fbe_status_t
 *
 * @author  
 *  05/04/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_xor_trace_checksum_errored_sector(fbe_sector_t *const sector_p,
                                                   const fbe_u16_t position_mask, 
                                                   const fbe_lba_t seed,
                                                   const fbe_xor_option_t option,
                                                   const fbe_xor_status_t xor_status)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Trace information about the CRC error.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "xor: checksum errored sector - pos mask: 0x%x lba: 0x%llx option: 0x%x status: 0x%x \n",
                           position_mask, (unsigned long long)seed, option,
			   xor_status);

    /* Use `basic' sector dump to display part of the sector data and metadata.
     */
    status = fbe_xor_trace_sector_basic(sector_p, seed);
    if (XOR_COND(status != FBE_STATUS_OK))
    {
        /* All request should have data.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "secp: %08x l:%016llx posmask: 0x%x status: 0x%x no data to dump",
                              fbe_xor_trace_pointer_to_u32(sector_p),
			      (unsigned long long)seed, position_mask,
			      xor_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*  The trace facility will panic if enabled
     */
    if (XOR_COND(((xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR) != 0) &&
                 (option & FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)         ))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "xor: checksum errored sector - lba: 0x%llx pos mask: 0x%x \n",
                              (unsigned long long)seed, position_mask);
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/* end of fbe_xor_trace_checksum_errored_sector() */

/***************************************************************************
 * END fbe_xor_trace.c
 ***************************************************************************/

