/*******************************************************************
 * Copyright (C) EMC Corporation 2000 - 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *
 *  @brief
 *    This file contains functions for the Sector tracing facility.
 *    
 *      
 *  @author
 *
 *  01-14-2010  Omer Miranda - Original version.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_sector_trace_interface.h"


/***********************
 * GLOBAL VARIABLES
 ***********************/

/****************************************
 * MACROS
 ****************************************/

/*!***********************************************************
 *          fbe_xor_sector_trace_sector()
 ************************************************************
 *
 * @brief
 *  Trace a copy of this sector to the RAID error trace buffer.
 *
 *
 * @param 
 *  error_level - Trace severity level
 *  error_flag - Trace error type
 *  sector_p    - The sector to display.
 *
 * @return
 *  none
 *
 * History:
 *  03/22/2010:  Omer Miranda - Created from xor_sh_trace_sector written
 *                     by Rob Foley.
 *
 ************************************************************/
void fbe_xor_sector_trace_sector(fbe_sector_trace_error_level_t error_level,
                                 fbe_sector_trace_error_flags_t error_flag,
                                 const fbe_sector_t *const sector_p)
{
    fbe_u32_t word_index;
    fbe_u32_t calc_csum;
    char   *prefix;

    /* Determine and set prefix.  Currently only two are supported:
     *      XOR 
     *      RAID
     */
    if (error_flag & FBE_SECTOR_TRACE_ERROR_FLAG_RAID)
    {
        prefix = "raid";
    }
    else
    {
        prefix = "xor";
    }

    /* Calculate the crc so we can display the difference between
     * the expected and actual crc.
     */
    calc_csum = xorlib_cook_csum(xorlib_calc_csum(sector_p->data_word), (fbe_u32_t) - 1);
    
    /* Display the checksum and calculated checksum.
     */
    if ( calc_csum == sector_p->crc)
    {
        fbe_sector_trace_entry(error_level,error_flag,
                               FBE_SECTOR_TRACE_DATA_START,
                               "%s: Checksum OK    Actual csum: 0x%04x Calculated csum: 0x%04x\n",
                               FBE_SECTOR_TRACE_LOCATION_BUFFERS,
                               prefix, sector_p->crc, calc_csum);
    }
    else
    {
        fbe_sector_trace_entry(error_level,error_flag,
                               FBE_SECTOR_TRACE_DATA_START,
                               "%s: CHECKSUM ERROR Actual csum: 0x%04x Calculated csum: 0x%04x\n",
                               FBE_SECTOR_TRACE_LOCATION_BUFFERS, 
                               prefix, sector_p->crc, calc_csum);
    }
    
    /* We never trace user data on an encrypted system.
     */
    if (fbe_sector_trace_can_trace_data())
    {
        /* Trace out all the data words in the sector.
         */
        for ( word_index = 0; 
            word_index < (FBE_BYTES_PER_BLOCK / sizeof(fbe_u32_t)); 
            word_index += 4 ) {
            fbe_sector_trace_entry(error_level,error_flag,
                                   FBE_SECTOR_TRACE_DATA_PARAMS,
                                   "DATA:\t %08X %08X %08X %08X \n",
                                   FBE_SECTOR_TRACE_LOCATION_BUFFERS,
                                   sector_p->data_word[word_index + 0],
                                   sector_p->data_word[word_index + 1],
                                   sector_p->data_word[word_index + 2],
                                   sector_p->data_word[word_index + 3] );
        }
    }
    /* Trace out the metadata.
     */
    fbe_sector_trace_entry(error_level,error_flag,
                           FBE_SECTOR_TRACE_DATA_PARAMS,
                           "META:\t ts:%04X crc:%04X lba/ss:%04X ws:%04X \n",
                           FBE_SECTOR_TRACE_LOCATION_BUFFERS, 
                           sector_p->time_stamp, sector_p->crc, 
                           sector_p->lba_stamp, sector_p->write_stamp);
    return;
} 
/* end of fbe_xor_sector_trace_sector() */

/*!***********************************************************
 *          fbe_xor_sector_trace_generate_trace_flag()
 ************************************************************
 *
 * @brief
 *  Determines (based on priority) the sector trace flag to assigned
 *  using the eboard passed.
 *
 * @param
 *      err_type        - The unique XOR error identifier
 *      error_string_p  - Address to set return string value to.
 *
 * @notes
 *  Only those eboard errors that are valid are currently handled.
 *  If different error types will be assumed add them at the correct
 *  priority below.
 *
 * @return
 *  trace_flag        - Result trace flag to use.
 *
 * History:
 *  03/24/2010:  Omer Miranda -- Created.
 *  
 ************************************************************/

fbe_sector_trace_error_flags_t fbe_xor_sector_trace_generate_trace_flag(const fbe_xor_error_type_t err_type,
                                                                        const char **error_string_p)
{
    fbe_sector_trace_error_flags_t trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;

    /* Simply use the error type to determine the 
     * trace type.
     */
    switch(err_type)
    {
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "Media Error";
            break;

        case FBE_XOR_ERR_TYPE_CRC:
        case FBE_XOR_ERR_TYPE_KLOND_CRC:
        case FBE_XOR_ERR_TYPE_DH_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
        case FBE_XOR_ERR_TYPE_COPY_CRC:
        case FBE_XOR_ERR_TYPE_PVD_METADATA:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *error_string_p = "CRC Error";
            break;
        
        case FBE_XOR_ERR_TYPE_RAID_CRC:
        case FBE_XOR_ERR_TYPE_INVALIDATED:
            /* RAID has corrupted the CRC due to a previous error!
             * Therefore DO NOT report this error!
             */
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RINV;
            *error_string_p = "Invalidated CRC";
            break;

        case FBE_XOR_ERR_TYPE_WS:
        case FBE_XOR_ERR_TYPE_BOGUS_WS:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_WS;
            *error_string_p = "Write Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_TS:
        case FBE_XOR_ERR_TYPE_BOGUS_TS:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_TS;
            *error_string_p = "Time Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_SS:
        case FBE_XOR_ERR_TYPE_BOGUS_SS:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_SS;
            *error_string_p = "Shed Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_1NS:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1NS Error";
            break;

        case FBE_XOR_ERR_TYPE_1S:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1S Error";
            break;

        case FBE_XOR_ERR_TYPE_1R:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1R Error";
            break;

        case FBE_XOR_ERR_TYPE_1D:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1D Error";
            break;

        case FBE_XOR_ERR_TYPE_1COD:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1COD Error";
            break;

        case FBE_XOR_ERR_TYPE_1COP:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1COP Error";
            break;

        case FBE_XOR_ERR_TYPE_1POC:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1POC Error";
            break;

        case FBE_XOR_ERR_TYPE_COH:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "Coherency";
            break;

        case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *error_string_p = "Corrupt Data";
            break;

        case FBE_XOR_ERR_TYPE_N_POC_COH:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_NPOC;
            *error_string_p = "Multiple POC";
            break;

        case FBE_XOR_ERR_TYPE_POC_COH:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_POC;
            *error_string_p = "POC COH Error";
            break;

        case FBE_XOR_ERR_TYPE_COH_UNKNOWN:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_UCOH;
            *error_string_p = "Unknown coherency";
            break;

        case FBE_XOR_ERR_TYPE_RB_FAILED:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_RAID;
            *error_string_p = "Rebuild Failed";
            break;

        case FBE_XOR_ERR_TYPE_LBA_STAMP:
            trace_flag = FBE_SECTOR_TRACE_ERROR_FLAG_LBA;
            *error_string_p = "LBA Stamp Error";
            break;

        default:
            /* For all other value use the default trace flag.
             */
            *error_string_p = "Generic Error";
            break;
     
    } /* end switch on err_type */

    /* Now return the result of building the trace id.
     */
    return trace_flag;
} /* fbe_xor_sector_trace_generate_trace_flag */

/*!***************************************************************************
 *          fbe_xor_sector_trace_raid_sector()
 *****************************************************************************
 *
 * @brief   Wrapper function for tracing either mismatched (i.e. either error 
 *          was detected that wasn't injected or visa versa) sector or a dump
 *          of an errored siots.
 *
 * @param   sector_p    - The sector to display.
 *
 * @return  status
 *
 * History:
 *  06/21/2010  - Ron Proulx    - Created
 *
 *****************************************************************************/
fbe_status_t fbe_xor_lib_sector_trace_raid_sector(const fbe_sector_t *const sector_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Simply dump the sector with the proper error level and flags.
     */
    fbe_xor_sector_trace_sector(FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL,
                                FBE_SECTOR_TRACE_ERROR_FLAG_RAID,
                                sector_p);

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_xor_sector_trace_raid_sector() */

/***************************************************************************
 * END fbe_sector_trace.c
 ***************************************************************************/
