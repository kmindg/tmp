/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 * fbe_raid_record_errors.c
 ***************************************************************************
 *
 * @brief
 *   This file contains common functions for recording errors
 *   for the RAID Group driver.
 *
 *   In this context "record errors" is the process of keeping track
 *   of any errors encountered during a siots for the purposes of logging
 *   messages to the event log.
 *
 * @author
 *  10/6/2009 - Created. Rob Foley Created from rg_record_errors.c
 *
 *
 ***************************************************************************/

/*************************
 **   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_raid_library.h"

 
/****************************
 *  GLOBAL VARIABLES
 ****************************/
/* These two globals are allocated for use by the tracing facility.
 * We allocate these globally rather than trying to allocate
 * this large amount of space on the stack.
 */
#define ULOG_TEXT_LENGTH_MAX 100
#if 0
static char rg_trace_id_string[ULOG_TEXT_LENGTH_MAX];
static char rg_trace_ulog_msg_string[ULOG_TEXT_LENGTH_MAX];
static char rg_trace_ulog_ext_string[ULOG_TEXT_LENGTH_MAX];
#endif

extern fbe_u32_t stop_on_additional_raid_event;

/****************************
 *  static FUNCTIONS
 ****************************/

static void fbe_raid_add_vp_eboard_errors( const fbe_raid_verify_error_counts_t * const source_eboard_p, 
                                     fbe_raid_verify_error_counts_t * const total_eboard_p );
static void fbe_raid_decode_ext_status(fbe_u32_t status, char * const status_string_p);

static fbe_status_t fbe_raid_siots_display_fruts_blocks(fbe_raid_fruts_t * const input_fruts_p, 
                                                fbe_lba_t lba);
static fbe_raid_state_status_t fbe_raid_siots_retry_finished(fbe_raid_siots_t * siots_p);

static fbe_status_t fbe_raid_get_csum_error_bits_for_given_region(const fbe_xor_error_region_t * const xor_region_p,
                                                                  fbe_u16_t position_key,
                                                                  fbe_u32_t * csum_bits_p);

/*!***********************************************************
 *  fbe_raid_trace_unsol()
 ************************************************************
 *
 * @brief
 *  This routine is used to trace unsolicited info to the
 *  ktrace buffer.
 *
 * @param   siots_p - raid driver sub iots struct to log unsol for.
 * @param   sunburst - This is the unsolicited information code.
 * @param   fru_pos - The fru position to log the error for.
 * @param   lba - physical address on which error has occured 
 * @param   blocks - Blocks with error.
 * @param   error_info - error information.
 * @param   extra_info - extra information about the raid group object
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  09/01/05:  Rob Foley -- Created.
 *
 ************************************************************/

fbe_status_t fbe_raid_trace_unsol(fbe_raid_siots_t *siots_p,
                                  fbe_u32_t error_code,
                                  fbe_u32_t fru_pos,
                                  fbe_lba_t lba,
                                  fbe_u32_t blocks,
                                  fbe_u32_t error_info,
                                  fbe_u32_t extra_info )
{
    fbe_status_t status = FBE_STATUS_OK;
#if 0
    fbe_bool_t hs_logs_as_normal_lun = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t raid_grp_obj = raid_geometry_p->object_id;


    if ( !FlareTracingIsEnabled( FLARE_RAID_HS_UNSOL_TRACING ) && 
         !FlareTracingIsEnabled( FLARE_RAID_UNSOL_TRACING ) )
    {
        /* Nothing enabled, just return. */
        return status;
    }

    if (fbe_raid_geometry_is_hot_sparing_type(raid_geometry_p) &&
         siots_p->algorithm == MIRROR_RB &&
         error_code == FBE_HOST_SECTOR_RECONSTRUCTED && (error_info & VR_CRC_RETRY))
    {
        /* For hot spares, the only errors that get logged under the 
         * normal unsol tracing level are equalizes that take crc retry errors.
         *
         * Any other error will be handled by and logged by the 
         * upper level raid group driver, so no need to log at hot spare level.
         */
        hs_logs_as_normal_lun = FBE_TRUE;
    }

    if ( fbe_raid_geometry_is_hot_sparing_type(raid_geometry_p) &&
         ( FlareTracingIsEnabled( FLARE_RAID_HS_UNSOL_TRACING ) ||
           ( FlareTracingIsEnabled( FLARE_RAID_UNSOL_TRACING ) &&
             hs_logs_as_normal_lun ) ) )
    {
        _snprintf(rg_trace_ulog_msg_string,(ULOG_TEXT_LENGTH_MAX - 1),
                  ulogEventMsg(error_code), extra_info);
        rg_trace_ulog_msg_string[ULOG_TEXT_LENGTH_MAX-1] = 0;
        if (RAID_COND( strlen(rg_trace_ulog_msg_string) >= 50 ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: rg_trace_ulog_msg_string length %d >= 50\n",
                                 strlen(rg_trace_ulog_msg_string));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_raid_decode_ext_status(error_info, rg_trace_ulog_ext_string);
        rg_trace_ulog_ext_string[ULOG_TEXT_LENGTH_MAX-1] = 0;
        if (RAID_COND( strlen(rg_trace_ulog_ext_string) >= 50 ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: rg_trace_ulog_ext_string length %d >= 50\n",
                                 strlen(rg_trace_ulog_ext_string));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        FLARE_RAID_HS_UNSOL_TRC64(( TRC_K_STD,"RAID: HS %d Unsol %s 0x%x %s\n", 
                                    raid_grp_obj, rg_trace_ulog_msg_string, error_info, 
                                    rg_trace_ulog_ext_string ));
    }
    else if ( FlareTracingIsEnabled( FLARE_RAID_UNSOL_TRACING ) )
    {
        sprintf(rg_trace_id_string, "Grp:%d Fru:%d ", 
                raid_grp_obj,  
                fru_pos);
        if (RAID_COND( strlen(rg_trace_id_string) >= 50 ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: rg_trace_id_string length %d >= 50\n",
                                 strlen(rg_trace_id_string));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        _snprintf(rg_trace_ulog_msg_string,(ULOG_TEXT_LENGTH_MAX - 1),
                  ulogEventMsg(error_code), extra_info);
        rg_trace_ulog_msg_string[ULOG_TEXT_LENGTH_MAX-1] = 0;

        if (RAID_COND( strlen(rg_trace_ulog_msg_string) >= 50 ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: rg_trace_ulog_msg_string length %d >= 50\n",
                                 strlen(rg_trace_ulog_msg_string));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_raid_decode_ext_status(error_info, rg_trace_ulog_ext_string);

        rg_trace_ulog_ext_string[ULOG_TEXT_LENGTH_MAX-1] = 0;
        if (RAID_COND( strlen(rg_trace_ulog_ext_string) >= 50 ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: rg_trace_ulog_ext_string length %d >= 50\n",
                                 strlen(rg_trace_ulog_ext_string));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        FLARE_RAID_UNSOL_TRC64(( TRC_K_STD,"RAID: %s %s 0x%x %s\n", 
                                 rg_trace_id_string, rg_trace_ulog_msg_string, error_info, 
                                 rg_trace_ulog_ext_string ));
    }
#endif
    return status;
}
/******************************
 * fbe_raid_trace_unsol()
 ******************************/
/************************************************************
 *  fbe_raid_decode_ext_status()
 ************************************************************
 *
 * @brief
 *  This routine is used to decode extended status info.
 *
 * PARAMTERS:
 * @param status - Raid Driver extended status bits 
 *                          to decode below.
 * @param status_string_p - Pointer to a buffer to add the
 *                          status information into.
 *
 * @return
 *  void
 *
 * History:
 *  08/04/07:  Rob Foley -- Created.
 *
 ************************************************************/

static void fbe_raid_decode_ext_status(fbe_u32_t status, char * const status_string_p)
{
    fbe_u32_t bit_index;
    char *current_status_p = NULL;

    /* Null terminate and initalize the string so we can do strcat 
     * to an empty string below.
     */
    status_string_p[0] = 0;

    /* Loop over all the possible status bits.  The bottom VR_BITFIELD_COUNT
     * are bit fields.
     * Decode them one at a time, doing a strcat to the input string.
     */
    for (bit_index = 0; bit_index < VR_BITFIELD_COUNT; bit_index++)
    {
        fbe_u16_t current_bit = 1 << bit_index;

        /* If the bit is set, decode it in the switch below.
         */
        if (current_bit & status)
        {
            /* Decode the current bit into text.
             */
            switch (current_bit)
            {
                case VR_UNEXPECTED_CRC:
                    current_status_p = "CRC ";
                    break;
                case VR_COH:
                    current_status_p = "COH ";
                    break;
                case VR_TS:
                    current_status_p = "TS ";
                    break;
                case VR_WS:
                    current_status_p = "WS ";
                    break;
                case VR_SS:
                    current_status_p = "SS ";
                    break;
                case VR_POC:
                    current_status_p = "POC ";
                    break;
                case VR_N_POC:
                    current_status_p = "NPOC ";
                    break;
                case VR_UNKNOWN_COH:
                    current_status_p = "UCOH ";
                    break;
                case VR_ZEROED:
                    current_status_p = "ZER ";
                    break;
                case VR_LBA:
                    current_status_p = "LBA ";
                    break;
                case VR_CRC_RETRY:
                    current_status_p = "RET ";
                    break;
                default:
                    current_status_p = "invalid ";
                    break;
            }    /* End switch for all bitwise status values. */

            if (current_status_p != NULL)
            {
                /* Concatenate the current decoded status onto the
                 * string keeping track of all status.
                 */
                strcat(status_string_p, current_status_p);
            }
        }    /* End if bit is set in status. */

    }    /* For all VR_BITFIELD_COUNT bits. */

    /* Now decode each of the reason fields if we
     * see any reasons are set.
     */
    if (status & VR_REASON_MASK)
    {
        if ((status & VR_REASON_MASK) == VR_RAID_CRC)
        {
            current_status_p = "RAID";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_KLONDIKE_CRC)
        {
            current_status_p = "KLOND";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_DH_CRC)
        {
            current_status_p = "DH";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_MEDIA_CRC)
        {
            current_status_p = "MEDIA";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_CORRUPT_CRC)
        {
            current_status_p = "CORR_CRC";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_CORRUPT_DATA)
        {
            current_status_p = "CORR_DAQ";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_SINGLE_BIT_CRC)
        {
            current_status_p = "SINGLE BIT";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_MULTI_BIT_CRC)
        {
            current_status_p = "MULTI BIT";
            strcat(status_string_p, current_status_p);
        }
	    else if ((status & VR_REASON_MASK) == VR_INVALID_CRC)
        {
            current_status_p = "INVALIDATED";
        }
	    else if ((status & VR_REASON_MASK) == VR_BAD_CRC)
        {
            current_status_p = "BAD CRC";
        }
        else if ((status & VR_REASON_MASK) == VR_COPY_CRC)
        {
            current_status_p = "COPY";
            strcat(status_string_p, current_status_p);
        }
        else if ((status & VR_REASON_MASK) == VR_PVD_METADATA_CRC)
        {
            current_status_p = "PVD METADATA";
            strcat(status_string_p, current_status_p);
        }

    }    /* End if a reason is set in the status. */
    return;
}
/* end fbe_raid_decode_ext_status() */
/************************************************************
 *  fbe_raid_siots_screen_event_code()
 ************************************************************
 *
 * @brief
 *  This routine is used to screen any interested unsol events sent.
 *If yes then panic system, esp use this for tracking SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR case
 * PARAMTERS:
 * @param   siots_p - ptr to the fbe_raid_siots_t to log unsol for.
 * @param   error_code - This is the unsolicited information code.
 *
 * @return
 *  void
 ************************************************************/

static void fbe_raid_siots_screen_event_code(fbe_raid_siots_t *siots_p,
                                 fbe_u32_t error_code)
{
    if(stop_on_additional_raid_event && error_code == stop_on_additional_raid_event)
    {
        fbe_raid_siots_object_trace(siots_p,FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,"Interesting Event(0x%x) catched:  siots_p=0x%p parity_start_pba=0x%llx ",error_code, siots_p,siots_p->parity_start);
    }
}
/*!***********************************************************
 *  fbe_raid_send_unsol()
 ************************************************************
 *
 * @brief
 *      This routine is used to send unsolicited info to the
 *      cm, which then sends it to the host.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t to log unsol for.
 * @param   error_code - This is the unsolicited information code.
 * @param   fru_pos - The fru position to log the error for.
 * @param   lba - physical address on which error has occured 
 * @param   blocks - Blocks with error.
 * @param   error_info - error information.
 * @param   extra_info - extra information about the raid group object
 *
 * @return - fbe_status_t
 *
 * @author
 *  09/20/99:  Rob Foley -- Created.
 *  09/06/00:  Rob Foley -- Hot Spare support.
 *  01/20/04:  Rob Foley -- Lba Logging (second_extra) support.
 *
 ************************************************************/
fbe_status_t fbe_raid_send_unsol(fbe_raid_siots_t *siots_p,
                                 fbe_u32_t error_code,
                                 fbe_u32_t fru_pos,
                                 fbe_lba_t lba,
                                 fbe_u32_t blocks,
                                 fbe_u32_t error_info,
                                 fbe_u32_t extra_info )
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t raid_grp_obj = raid_geometry_p->object_id;

#if defined(FBE_RAID_DEBUG) || defined(RG_ERROR_SIM_ENABLED)
    if ( raid_group_gb_ptr->options_flags.log_injected_errors_flag == 0 &&
         ( RG_IS_SIOTS_FLAG_SET(siots_p, RSUB_ERROR_INJECTED) ||
           ( RG_SIOTS_NEST(siots_p) &&
             RG_IS_SIOTS_FLAG_SET(fbe_raid_siots_get_parent(siots_p),
                                  RSUB_ERROR_INJECTED) ) ))
    {
        /* If we want to log injected errors.
         * AND
         * If this was an injected error on this SIOTS or
         * the parent siots (when this siots is nested),
         * then we shall not log any errors to the ulog.
         */
        return status;
    }
#endif

    if (RAID_COND(fru_pos >= FBE_RAID_MAX_DISK_ARRAY_WIDTH) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: fru_pos 0x%x >= FBE_RAID_MAX_DISK_ARRAY_WIDTH\n",
                             fru_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (fbe_raid_geometry_is_hot_sparing_type(raid_geometry_p))
    {
        if (siots_p->algorithm == MIRROR_RB &&
            error_code == SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED && (error_info & VR_CRC_RETRY))
        {
            /* This is a hot spare equalize.  
             * We only log these retried crc errors.
             * Any other error will be handled by and logged by the 
             * upper level raid group driver.
             * For hot spares use the group, which is the fru.
             */
            fru_pos = raid_grp_obj;
        }
        else
        {
            /* Other hs messages do not log anything, so return.
             */
            return status;
        }
    }
    
    /* Try to log this to unsol.  No effect if the runtime tracing
     * for raid unsols is not enabled.
     */
     status = fbe_event_log_write(error_code,
                                 NULL, 0, "%x %x %llx %x %x %x", 
                                 raid_grp_obj, fru_pos, lba, blocks, error_info, extra_info);
    if (fru_pos >= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH)
    {
        /* Removing this panic. Instead of this we will log the message.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "raid: fru: 0x%x >= FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH 0x%x. \n",
                             fru_pos, FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
	/*
	Try to look if we have interest in some event, no effect if no event set for interesting
	*/
	fbe_raid_siots_screen_event_code(siots_p,error_code);
    return status;
}
/*****************************************
 * fbe_raid_send_unsol()
 *****************************************/

fbe_status_t fbe_raid_siots_get_unmatched_record(fbe_raid_siots_t * const siots_p,
                                                 fbe_xor_error_region_t **error_region_pp)
{
    fbe_xor_error_regions_t* regs_xor_p = &siots_p->vcts_ptr->error_regions;
    fbe_s32_t region_index;
    
    /* Loop over each of the fbe_xor_error_region_t from the XOR driver's 
     * fbe_xor_error_regions_t. 
     * Try to find the region marked unmatched. 
     */
    for (region_index = 0; region_index < regs_xor_p->next_region_index; ++region_index)
    {
        if (regs_xor_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_UNMATCHED)
        {
            *error_region_pp = &regs_xor_p->regions_array[region_index];
            return FBE_STATUS_OK;
        }
        
    }
    /* If we are here we did not find anyting to return.
     */
    return FBE_STATUS_GENERIC_FAILURE;
}
/************************************************************
 *  fbe_raid_siots_handle_validation_failure()
 ************************************************************
 *
 * @brief
 *  There has been a validation failure.
 *  This means that the error we encountered was unexpected
 *  based on the error table currently in use.
 *  Display information about the current read fruts and
 *  display all the blocks in the read fruts.
 *  Then retry the reads so we get a fresh picture of
 *  how the data appears on disk.
 *
 * @param siots_p - Current siots.
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  04/05/07:  Rob Foley -- Created.
 *
 ************************************************************/
static fbe_status_t fbe_raid_siots_handle_validation_failure( fbe_raid_siots_t *const siots_p )
{
    fbe_u16_t active_fruts_bitmap; /* Bitmap of non-NOP opcode fruts. */
    fbe_xor_error_region_t *unmatched_rec_p = NULL;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_unmatched_record(siots_p, &unmatched_rec_p);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                "raid: Displaying sectors from pba 0x%llx\n",
                                unmatched_rec_p->lba);

    /* First display all the read blocks.
     */
    status = fbe_raid_siots_display_fruts_blocks( read_fruts_ptr, unmatched_rec_p->lba );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: %s failed to display all fruts blocks: status = 0x%x, read_fruts_ptr = 0x%p, "
                             "unmatched_rec_p->lba = 0x%llx\n",
                             __FUNCTION__, status, read_fruts_ptr, unmatched_rec_p->lba);
        return status;
    }
    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                         "%s: re-reading pba 0x%llx before panic\n",
                         __FUNCTION__,
			(unsigned long long)unmatched_rec_p->lba );

    fbe_raid_fruts_get_chain_bitmap( read_fruts_ptr, &active_fruts_bitmap );

    /* Update the wait count for all the reads we will be retrying.
     */
    siots_p->wait_count = fbe_raid_get_bitcount( active_fruts_bitmap );

    if ( siots_p->wait_count > 0 )
    {
        /* Send out retries for all reads.
         */
        status =  fbe_raid_fruts_for_each( 
            active_fruts_bitmap,  /* Retry for all non-nop fruts. */
            read_fruts_ptr,  /* Fruts chain to retry for. */
            fbe_raid_fruts_retry, /* Function to call for each fruts. */
            (fbe_u32_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* Opcode to retry as. */
            (fbe_u64_t) fbe_raid_fruts_evaluate); /* New evaluate fn. */
        if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to retry fruts chain: read_fruts_ptr 0x%p "
                                 "siots_p 0x%p. status = 0x%x\n",
                                 __FUNCTION__,
                                 read_fruts_ptr, siots_p, status);
            return FBE_FALSE;
        }
    }
    else
    {
        /* For some reason there was nothing to retry here, so
         * instead of retrying we will just fbe_panic now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL ,
                "RAID:  Critical error due to rderr validation mismatch. "
                "see above ktrace for more details\n" );
    }
    
    /* Transition to our final state where we will display the information read.
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_retry_finished);

    return status;
}
/*******************************************
 * end fbe_raid_siots_handle_validation_failure()
 *******************************************/
/************************************************************
 *  fbe_raid_siots_retry_finished()
 ************************************************************
 *
 * @brief
 *  The retry of errored blocks has finished.
 *  This allows us to display the current state of
 *  these blocks on disk.
 *
 * PARAMTERS:
 * @param siots_p - Current siots.
 *
 * @return
 *  FBE_RAID_STATE_STATUS_DONE
 *
 * History:
 *  04/04/07:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_raid_state_status_t fbe_raid_siots_retry_finished(fbe_raid_siots_t * siots_p)
{
    fbe_xor_error_region_t *unmatched_rec_p = NULL;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_unmatched_record(siots_p, &unmatched_rec_p);

    /* The blocks we display are put in the traffic buffer since it is
     * unused and so that the standard buffer is not filled up.
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                "Displaying sectors newly read from pba 0x%llx\n",
                                (unsigned long long)unmatched_rec_p->lba);

    /* Display the set of blocks that we just retried the reads for.
     */
    status = fbe_raid_siots_display_fruts_blocks( read_fruts_ptr, unmatched_rec_p->lba );
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to display fruts blocks: status = 0x%x, siots_p = 0x%p "
                             "read_fruts_ptr = 0x%p, unmatched_rec_p->lba = 0x%llx\n",
                             status, siots_p, read_fruts_ptr, unmatched_rec_p->lba);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    
    /* The validation retry is finished, we now fbe_panic and display the record that
     * was unmatched in validation.
     */
    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
            "%s:  Validation retry "
            "lba: 0x%llx blocks: 0x%x pos_bitmask: 0x%x error: 0x%x\n",
            __FUNCTION__, (unsigned long long)unmatched_rec_p->lba,
	    unmatched_rec_p->blocks, 
            unmatched_rec_p->pos_bitmask, unmatched_rec_p->error );

    fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
            "%s:  Critical error due to rderr validation mismatch. "
            "see above ktrace for more details\n", __FUNCTION__ );

    return FBE_RAID_STATE_STATUS_DONE;
}
/*************************************************
 * end of fbe_raid_siots_retry_finished()
 *************************************************/

/************************************************************
 *  fbe_raid_siots_display_fruts_blocks()
 ************************************************************
 *
 * @brief
 *   Display the fruts blocks for a given fruts chain
 *   and a given LBA.  All blocks in the chain for this
 *   lba will be displayed.
 *
 * PARAMTERS:
 * @param input_fruts_p - Fruts to display range for.
 * @param lba - Lba to display for.
 *
 * @return
 *  fbe_status_t.
 *
 * History:
 *  04/04/07:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_status_t fbe_raid_siots_display_fruts_blocks(fbe_raid_fruts_t * const input_fruts_p, 
                                                fbe_lba_t lba)
{
    fbe_raid_fruts_t *fruts_p;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(input_fruts_p);
    fbe_status_t status = FBE_STATUS_OK;

    /* Loop over all fruts, and display each block that matches the above LBA.
     */
    for (fruts_p = input_fruts_p;
         fruts_p != NULL;
         fruts_p = fbe_raid_fruts_get_next(fruts_p))
    {
        fbe_sg_element_t *fruts_sg_p = NULL;
        fbe_u32_t block_offset;

        fbe_raid_fruts_get_sg_ptr(fruts_p, &fruts_sg_p);
        
        /* Only display blocks that match the input lba.
         */
        if ( lba >= fruts_p->lba &&
             (lba - fruts_p->lba) <= fruts_p->blocks )
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "%s: fruts start lba: 0x%llx blocks: 0x%llx op: %d position: %d \n",
                                        __FUNCTION__, fruts_p->lba, fruts_p->blocks, fruts_p->opcode, fruts_p->position);

            /* Only display blocks that we read successfully.
             */
            if ( fruts_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS && fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID )
            {
                PVOID reference_p = NULL;
                fbe_sector_t *ref_sect_p = NULL;
                fbe_sector_t *sector_p;

                /* Adjust the sg ptr to point at the sg element with this sector.
                 */
                block_offset = fbe_raid_get_sg_ptr_offset( &fruts_sg_p,
                                                       (fbe_u32_t)(lba - fruts_p->lba) );

                /* Next adjust our sector ptr to point to the memory in this sg
                 * element with this sector.
                 */
                sector_p = (fbe_sector_t *)fruts_sg_p->address;
                if (RAID_COND(fruts_sg_p->count < (block_offset * FBE_BE_BYTES_PER_BLOCK) ))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: fruts_sg_p->count 0x%x < (block_offset 0x%x * FBE_BE_BYTES_PER_BLOCK); "
                                         "siots_p = 0x%p\n",
                                         fruts_sg_p->count, block_offset, siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                sector_p += block_offset;
            
                /* Next, map in the memory
                 */
                reference_p = (FBE_RAID_VIRTUAL_ADDRESS_AVAIL(fruts_sg_p))
                    ?(PVOID)sector_p:
                    FBE_RAID_MAP_MEMORY(((UINT_PTR)sector_p), FBE_BE_BYTES_PER_BLOCK);
                ref_sect_p = (fbe_sector_t *) reference_p;
               
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                            "%s: lba: 0x%llx mem: 0x%x mapped: 0x%x \n", 
                                            __FUNCTION__, lba, (unsigned int)sector_p, (unsigned int)ref_sect_p);
            
                /* Next display the sector.
                 */
                status = fbe_xor_lib_sector_trace_raid_sector(ref_sect_p);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }

                
                /* Finally, map out the sector.
                 */
                if( !FBE_RAID_VIRTUAL_ADDRESS_AVAIL(fruts_sg_p) )
                {
                    FBE_RAID_UNMAP_MEMORY(reference_p, FBE_BE_BYTES_PER_BLOCK);
                }
            }/* display successfully read blocks. */
        }
        else
        {
            /* We expected our error area to fall within the fruts, but it didn't
             * so we have no choice but to display this error.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                        "%s: Skip fruts 0x%p lba: 0x%llx blocks: 0x%llx unmatched lba: 0x%llx\n",
                                        __FUNCTION__, fruts_p,
				       (unsigned long long)fruts_p->lba,
				       (unsigned long long)fruts_p->blocks,
				       (unsigned long long)lba);
        }
    }
    return status;
}
/****************************************
 * end of fbe_raid_siots_display_fruts_blocks() 
 ****************************************/
/****************************************************************
 *  fbe_raid_siots_record_errors()
 ****************************************************************
 * @brief
 *  Record errors detected in the rebuilds.
 *

 * @param siots_p - ptr to VR_TS to set bits in
 * @param width - unit's array width
 * @param eboard_p - the error board to extract errors from.
 * @param record_these_bits - which bit positions to record errs for.
 * @param allow_correctable - whether or not we expect to see
 *                           any correctable errors.
 * @param validate - FBE_TRUE means Validate (debug builds only).
 *
 * @return VALUE:
 *  FBE_RAID_STATE_STATUS_DONE - If no error encountered.
 *  FBE_RAID_STATE_STATUS_WAITING - If retries performed.
 *
 *
 * @notes
 *
 *   We suppress the event log message "parity sector reconstructed"
 *   (HOST_PARITY_SECTOR_RECONSTRUCTED) when a correctable timestamp
 *   error is detected when an IORB verify operation occurs during a
 *   RAID 5 group expansion. We do this suppression here because this
 *   function appears to be the first (earliest) common point in the
 *   collection and reporting of errors.
 *
 *
 * @author
 *  03/7/00 -- created.  jj
 *  05/25/00 -- Moved from r5_rb_utils.c and renamed from r5_rb_record_errors. MPG
 *  06/28/00 -- Added header comments.
 *              Generalized and added check_these_bits argument. Rob Foley
 *  07/09/08 -- m. polia, 09-jul-2008, DIMS 198841.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_record_errors(fbe_raid_siots_t * const siots_p,
                                                     fbe_u16_t width,
                                                     fbe_xor_error_t * eboard_p,
                                                     fbe_u16_t record_these_bits,
                                                     fbe_bool_t allow_correctable,
                                                     fbe_bool_t validate)
{
    fbe_bool_t c_errors;
    fbe_bool_t u_errors;

    /* The inverse mask is everything we *don't*
     * want to record.
     */
    fbe_u16_t InverseMask = ~record_these_bits;

    fbe_raid_vrts_t * vrts_p = siots_p->vrts_ptr;

    c_errors = ((eboard_p->c_crc_bitmap |
                 eboard_p->c_coh_bitmap |
                 eboard_p->c_ts_bitmap |
                 eboard_p->c_ws_bitmap |
                 eboard_p->c_ss_bitmap |
                 eboard_p->c_n_poc_coh_bitmap |
                 eboard_p->c_poc_coh_bitmap |
                 eboard_p->c_rm_magic_bitmap |
                 eboard_p->c_rm_seq_bitmap) != 0);

    u_errors = ((eboard_p->u_crc_bitmap |
                 eboard_p->u_coh_bitmap |
                 eboard_p->u_ts_bitmap |
                 eboard_p->u_ws_bitmap |
                 eboard_p->u_ss_bitmap |
                 eboard_p->u_poc_coh_bitmap |
                 eboard_p->u_n_poc_coh_bitmap |
                 eboard_p->u_coh_unk_bitmap |
                 eboard_p->u_rm_magic_bitmap) != 0);

    if (vrts_p == NULL)
    {
        /* No verify tracking structure, cannot do anything.
         */
        return FBE_RAID_STATE_STATUS_DONE;
    }

    if ((allow_correctable == FBE_FALSE) && c_errors)
    {
        /* It is possible to get uncorrectable and correctable
         * timestamp and write stamp errors in degraded situations.
         * But shed stamp errors, and crc errors,
         * are not correctable in degraded situations.
         */
        if (eboard_p->c_ss_bitmap | eboard_p->c_crc_bitmap)
        {
            /***************************************************
             * Xor Driver has generated correctable errors
             * when we only expect uncorrectable errors.
             ***************************************************/
            //fbe_panic(RG_INVALID_XOR_DRIVER_STATUS, __LINE__);
        }
    }

    if ( validate )
    {
        fbe_raid_siots_error_validation_callback_t callback;
        fbe_raid_siots_get_error_validation_callback(siots_p, &callback);
        /* Only validate results if error(s) have been injected.
         * The logical error injection service will fill this callback 
         * in when an error is injected. 
         */
        if (callback != NULL)
        {
            fbe_status_t status;
            fbe_xor_error_region_t *unmatched_error_region_p = NULL;

            /* invoke the callback to validate this error result.
             */
            status = (callback)(siots_p, &unmatched_error_region_p);

            if (status != FBE_STATUS_OK)
            {
                /* Mark this error as unmatched so we can identify it going forward.
                 */  
                unmatched_error_region_p->error |= FBE_XOR_ERR_TYPE_UNMATCHED;
                status = fbe_raid_siots_handle_validation_failure((fbe_raid_siots_t* const)siots_p);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: failed to handle validation failure: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
                    return status;
                }
                /* We are waiting for the retries to occur so we can display status info.
                 */
                return FBE_RAID_STATE_STATUS_WAITING;
            }
        }
    }    /* end if validate */

    /* Clear out everything we *don't* want to record.
     */
    fbe_xor_eboard_clear_mask( eboard_p, InverseMask );

    /* Take the remainder and put it into our global mask.
     *
     * This saves all the local data for this portion of the
     * I/O in our global mask for the SIOTS.
     *
     * When we're done with our SIOTS and all
     * strip mining operations for this SIOTS,
     * then we will do one set of logging to the event
     * log with the data from the global eboard.
     */
    fbe_xor_update_global_eboard( &vrts_p->eboard, eboard_p );

    /* We check for vcts_ptr, since only verify uses the vcts_ptr.
     * Other algorithms also come through this code path.
     */
    if ( siots_p->vcts_ptr != NULL )
    {
        fbe_raid_position_bitmask_t soft_media_err_bitmap;
        fbe_raid_fruts_t *read_fruts_p = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

        fbe_raid_fruts_get_bitmap_for_status(read_fruts_p, &soft_media_err_bitmap, NULL, /* No counts */
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);

        /* Just log one error for the entire stripe.  Since soft errors are always
         * correctable, they do not show up in the errors that xor logs. 
         */
        if (soft_media_err_bitmap != 0)
        {
            siots_p->vcts_ptr->curr_pass_eboard.c_soft_media_count++; 
        }

        /* Take the current pass error counts and add them to the
         * overall error counts for this siots.
         */
        fbe_raid_add_vp_eboard_errors( &siots_p->vcts_ptr->curr_pass_eboard, 
                                       &siots_p->vcts_ptr->overall_eboard );
    }
    return FBE_RAID_STATE_STATUS_DONE;
}
/*******************************
 * end of fbe_raid_siots_record_errors() 
 *******************************/

/*!***********************************************************
 *  fbe_raid_get_checksum_error_bits()
 ************************************************************
 *
 * @brief
 *  Determine the types of checksum error bits we
 *  received for this particular position..
 *
 * @param   eboard_p - eboard to inspect
 * @param   position_key - current position key to inspect.
 *
 * @return
 *  fbe_u32_t - The bits to log to the event log.
 *
 * @author
 *  01/31/05:  Rob Foley -- Created.
 *
 ************************************************************/
fbe_u32_t fbe_raid_get_checksum_error_bits( const fbe_xor_error_t * const eboard_p, 
                                      fbe_u16_t const position_key )
{
    /* Set to zero, since initially we have not detected any
     * errors set in a bitmask.
     */
    fbe_u32_t return_errors = 0;

    /* If we have a CRC error, then we may have one of these
     * errors as well.
     * Note that these status values are exclusive since they
     * use the same bits.  It is pretty unlikely that we'll see
     * more than one of these in the same stripe,
     * so we'll only log the first one we see.
     *
     * We've put a priority order here in cases where there is more
     * than one of these.  Reasoning is some errors are more 
     * important than others.
     * We'll put the crc errors that are
     * most likely to be caused by media errors first
     * (Klondike and media errors) because we want to know
     * about hardware type errors first.
     */
    if ( (eboard_p->crc_lba_stamp_bitmap & position_key ) != 0 )
    {
        /* Lba Stamp error.
         */
        return_errors |= VR_LBA_STAMP;
    }
    else if ( ( eboard_p->media_err_bitmap & position_key ) != 0 )
    {
        /* Media error detected, add it to our mask.
         */
        return_errors |= VR_MEDIA_CRC;
    }
    else if ( ( eboard_p->crc_klondike_bitmap & position_key ) != 0 )
    {
        /* Klondike invalid crc detected, add it to our mask.
         */
        return_errors |= VR_KLONDIKE_CRC;
    }
    else if ( ( eboard_p->corrupt_crc_bitmap & position_key ) != 0 )
    {
        /* Corrupt CRC invalid format detected.
         * This means that RAID intentionally invalidated this
         * block because of a corrupt crc command.
         *
         * Add it to our mask.
         */
        return_errors |= VR_CORRUPT_CRC;
    }
    else if ( ( eboard_p->crc_dh_bitmap & position_key ) != 0 )
    {
        /* DH invalid crc detected, add it to our mask.
         */
        return_errors |= VR_DH_CRC;
    }
    else if ( ( eboard_p->crc_raid_bitmap & position_key ) != 0 )
    {
        /* Raid invalid crc detected, add it to our mask.
         */
        return_errors |= VR_RAID_CRC;
    }
    else if ( ( eboard_p->crc_invalid_bitmap & position_key ) != 0 )
    {
        /* `Data Lost' - Invalidated crc detected, add it to our mask.
         */
        return_errors |= VR_INVALID_CRC;
    }
    else if ( ( eboard_p->crc_bad_bitmap & position_key ) != 0 )
    {
        /* Invalid crc on write detected, add it to our mask.
         */
        return_errors |= VR_BAD_CRC;
    }
    else if ( ( eboard_p->corrupt_data_bitmap & position_key ) != 0 )
    {
        /* Raid invalid crc detected, add it to our mask.
         */
        return_errors |= VR_CORRUPT_DATA;
    }
    else if ( ( eboard_p->crc_copy_bitmap & position_key ) != 0 )
    {
        /* COPY invalid crc detected, add it to our mask.
         */
        return_errors |= VR_COPY_CRC;
    }
    else if ( ( eboard_p->crc_pvd_metadata_bitmap & position_key ) != 0 )
    {
        /* PVD Metadata invalidated CRC, add it to our mask.
         */
        return_errors |= VR_PVD_METADATA_CRC;
    }
    

    if ( ((( eboard_p->crc_single_bitmap & position_key ) != 0) ||
          (( eboard_p->crc_multi_bitmap & position_key ) != 0)) ||
         return_errors == 0 )
    {
        /* We either did not find any checksum errors that
         * were recognized as having a certain pattern.
         * In this case we default to logging unknown, and
         * that will be the only bit logged.
         *
         * OR
         *
         * We found the bit set for the unexpected checksum. 
         * We add in the unexpected checksum here and
         * it is possible that other bits were logged above
         * If other blocks on this position were detected to 
         * have checksum errors of other types.
         *
         * We will log this as an "Unexpected checkum error".
         * 
         * If the multi bitmap is set, then set the reason to 
         * mutli bit CRC error.  Otherwise, assume a single
         * bit error.
         */
        return_errors |= VR_UNEXPECTED_CRC;

        if( (eboard_p->crc_multi_bitmap & position_key) != 0 )
        {
            return_errors |= VR_MULTI_BIT_CRC;
        }
        else if((eboard_p->crc_single_bitmap & position_key )!= 0 )
        {
            return_errors |= VR_SINGLE_BIT_CRC;
        }
    }
    return return_errors;
}
/****************************************
 * fbe_raid_get_checksum_error_bits()
 ****************************************/

/*!***********************************************************
 *  fbe_raid_get_correctable_status_bits()
 ************************************************************
 *
 * @brief
 *  Determine the correctable extended status bits to log
 *  for a given set of vrts errors.
 *
 * @param   vrts_p - vrts to inspect
 * @param   position_key - current position key to inspect.
 *
 * @return
 *  fbe_u32_t - The bits to log to the event log.
 *
 * @author
 *  01/28/05:  Rob Foley -- Created.
 *
 ************************************************************/
fbe_u32_t fbe_raid_get_correctable_status_bits( const fbe_raid_vrts_t * const vrts_p, 
                                          fbe_u16_t position_key )
{
    /* Set to zero, since initially we have not detected any
     * errors set in a bitmask.
     */
    fbe_u32_t return_errors = 0;

    /* For each of the possible correctable errors, determine 
     * if this position has taken an correctable error or not.
     *
     * If this position does get detected with an correctable
     * error, then we will add this type of error to the 
     * return_errors.
     *
     * The bits we add to return_errors are defined in rg_ts.h,
     * and all begin with VR_
     *
     */
    if ( ( vrts_p->eboard.c_crc_bitmap & position_key ) != 0 )
    {
        /* Extract the types of recognized checksum errors.
         */
        return_errors |= fbe_raid_get_checksum_error_bits( &vrts_p->eboard,
                                                     position_key );
    }

    if ( ( vrts_p->eboard.c_coh_bitmap & position_key ) != 0 )
    {
        /* Coherency error detected, add it to bitmask.
         */
        return_errors |= VR_COH;
    }

    if ( ( vrts_p->eboard.c_ts_bitmap & position_key ) != 0 )
    {
        /* Time stamp error detected, add it to bitmask.
         */
        return_errors |= VR_TS;
    }

    if ( ( vrts_p->eboard.c_ws_bitmap & position_key ) != 0 )
    {
        /* Write stamp error detected, add it to bitmask.
         */
        return_errors |= VR_WS;
    }

    if ( ( vrts_p->eboard.c_ss_bitmap & position_key ) != 0 )
    {
        /* Shed stamp error detected, add it to bitmask.
         */
        return_errors |= VR_SS;
    }

    if ( ( vrts_p->eboard.c_n_poc_coh_bitmap & position_key ) != 0 )
    {
        /* Multiple POC errors, but we don't know which
         * position is in error, add it to bitmask.
         */
        return_errors |= VR_N_POC;
    }

    if ( ( vrts_p->eboard.c_poc_coh_bitmap & position_key ) != 0 )
    {
        /* Parity of checksums error, add it to the bitmask.
         */
        return_errors |= VR_POC;
    }

    if ( ( vrts_p->eboard.zeroed_bitmap & position_key ) != 0 )
    {
        /* If we have a zeroed error on this position, then
         * go ahead and log it in the extended status.
         * Zeroed errors are only correctable.
         */
        return_errors |= VR_ZEROED;
    }

    if ( ( vrts_p->retried_crc_bitmask & position_key ) != 0 )
    {
        /* Retried crc errors are in the vrts structure,
         * add it to the bitmask.
         */
        return_errors |= VR_CRC_RETRY;
    }

    if ( ( vrts_p->eboard.c_rm_magic_bitmap & position_key ) != 0 )
    {
        /* Raw mirror magic num error, add it to the bitmask.
         */
        return_errors |= VR_RAW_MIRROR_MAGIC_NUM;
    }

    if ( ( vrts_p->eboard.c_rm_seq_bitmap & position_key ) != 0 )
    {
        /* Raw mirror sequence num error, add it to the bitmask.
         */
        return_errors |= VR_RAW_MIRROR_SEQ_NUM;
    }

    return return_errors;
}
/****************************************
 * fbe_raid_get_correctable_status_bits()
 ****************************************/

/*!***********************************************************
 *  fbe_raid_get_uncorrectable_status_bits()
 ************************************************************
 *
 * @brief
 *  Determine the uncorrectable extended status bits to log
 *  for a given set of eboard errors.
 *
 * @param   eboard_p - eboard to inspect
 * @param   position_key - current position key to inspect.
 *
 * @return
 *  fbe_u32_t - The bits to log to the event log.
 *
 * @author
 *  01/28/05:  Rob Foley -- Created.
 *
 ************************************************************/
fbe_u32_t fbe_raid_get_uncorrectable_status_bits(const fbe_xor_error_t * const eboard_p, 
                                                 fbe_u16_t position_key )
{
    /* Set to zero, since initially we have not detected any
     * errors set in a bitmask.
     */
    fbe_u32_t return_errors = 0;

    /* For each of the possible uncorrectable errors,
     * determine if this position has taken an uncorrectable
     * error or not.
     *
     * If this position does get detected with an uncorrectable
     * error, then we will add this type of error to the return_errors.
     *
     * The bits we add to return_errors are defined in rg_ts.h,
     * and all begin with VR_
     *
     */
    if ( ( eboard_p->u_crc_bitmap & position_key ) != 0 )
    {
        /* Extract the types of recognized checksum errors.
         */
        return_errors |= fbe_raid_get_checksum_error_bits( eboard_p, position_key );
    }

    if ( ( eboard_p->u_coh_bitmap & position_key ) != 0 )
    {
        /* Coherency error detected, add it to bitmask.
         */
        return_errors |= VR_COH;
    }

    if ( ( eboard_p->u_ts_bitmap & position_key ) != 0 )
    {
        /* Time stamp error detected, add it to bitmask.
         */
        return_errors |= VR_TS;
    }

    if ( ( eboard_p->u_ws_bitmap & position_key ) != 0 )
    {
        /* Write stamp error detected, add it to bitmask.
         */
        return_errors |= VR_WS;
    }

    if ( ( eboard_p->u_ss_bitmap & position_key ) != 0 )
    {
        /* Shed stamp error detected, add it to bitmask.
         */
        return_errors |= VR_SS;
    }

    if ( ( eboard_p->u_coh_unk_bitmap & position_key ) != 0 )
    {
        /* An unknown, unhandled error, add it to bitmask.
         */
        return_errors |= VR_UNKNOWN_COH;
    }

    if ( ( eboard_p->u_n_poc_coh_bitmap & position_key ) != 0 )
    {
        /* Multiple POC errors, but we don't know which
         * position is in error, add it to bitmask.
         */
        return_errors |= VR_POC;
    }

    if ( ( eboard_p->u_poc_coh_bitmap & position_key ) != 0 )
    {
        /* Parity of checksums error, add it to the bitmask.
         */
        return_errors |= VR_N_POC;
    }

    if ( ( eboard_p->u_rm_magic_bitmap & position_key ) != 0 )
    {
        /* Magic numbers wrong on raw mirror, add it to the bitmask.
         */
        return_errors |= VR_RAW_MIRROR_MAGIC_NUM;
    }

    return return_errors;
}
/****************************************
 * rg_get_uncorrectable_status_bits()
 ****************************************/

/****************************************************************
 *  fbe_raid_report_errors()
 ****************************************************************
 * @brief
 *  Reports errors to the event log.  This function just
 *  routes a general call to a specific unit type call.
 *
 * @param siots_p - ptr to siots to use for logging errors.
 *
 * @return  fbe_status_t
 *
 * @author
 *  08/16/05 -- Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_report_errors( fbe_raid_siots_t * const siots_p )
{
#if 0
    fbe_raid_geometry_t *rgdb_p = fbe_raid_siots_get_raid_geometry( siots_p );

    /* Just detect the unit type and route it to the
     * appropriate function for reporting errors.
     */
    if (fbe_raid_group_is_r0(rgdb_p))
    {
        r0_vr_report_errors(siots_p);
    }
    else if (fbe_raid_group_is_parity(rgdb_p))
    {
        r5_vr_report_errors(siots_p);
    }
    else if (RG_IS_MIRROR_GROUP(rgdb_p))
    {
        mirror_vr_report_errors(siots_p);
    }
    else if (RG_IS_HOT_SPARE(rgdb_p))
    {
        mirror_vr_report_errors(siots_p);
    }
    else
    {
        KTRACE("RAID: fbe_raid_report_errors, unexpected rgdb type %x %x\n",
               rgdb_p, siots_p, 0, 0);
        fbe_panic(RG_INVALID_STATE, (fbe_u32_t *) siots_p);
    }
    /* Update the IOTS' error counts with the totals from this siots.
     */
    rg_update_iots_vp_eboard( siots_p );
#endif
    return FBE_STATUS_OK;
}
/****************************************
 * fbe_raid_report_errors()
 ****************************************/

/****************************************************************
 *  rg_report_media_errors()
 ****************************************************************
 * @brief
 *  Reports errors to the event log in the case where a media
 *  error is going to be reported on this I/O.
 *  Remember that media error captures the cases where we
 *  take a checksum error and cannot fix the error,
 *  or the case where we genuinely took a media error from the drive.
 *
 * @param siots_p - ptr to siots to use for logging errors.
 * @param fruts_p - ptr to fru ts chain.
 * @param correctable - FBE_TRUE means these should be reported as
 *                     correctable errors, FBE_FALSE to report
 *                     as uncorrectable errors.
 *
 * ASSUMPTIONS:
 *  We assume that either media errors or checksum errors were
 *  prior to this point.
 *
 * @return VALUE:
 *  none.
 *
 * @author
 *  08/24/05 -- Created. Rob Foley
 *
 ****************************************************************/

void rg_report_media_errors( fbe_raid_siots_t * const siots_p,
                             fbe_raid_fruts_t * const fruts_p,
                             const fbe_bool_t correctable )
{
    /* Determine if hard errors were taken.
     */
    (void) fbe_raid_fruts_get_media_err_bitmap(
                                              fruts_p,
                                              &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

    if ( siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap != 0 )
    {
        /* Put the media errors into the VRTS since this is what report
         * errors uses.  Use the appropriate bitmap depending on whether or
         * not we were asked to log as correctable or not.
         */
        if ( correctable )
        {
            siots_p->vrts_ptr->eboard.c_crc_bitmap |=
            siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
        }
        else
        {
            siots_p->vrts_ptr->eboard.u_crc_bitmap |=
            siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
        }

        /* Also setup the media err bitmap.
         */
        siots_p->vrts_ptr->eboard.media_err_bitmap |=
        siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
    }

    /* Report these errors to the event log.
     */
    fbe_raid_report_errors( siots_p );

    return;
}
/****************************************
 * rg_report_media_errors()
 ****************************************/

/****************************************************************
 * fbe_raid_add_vp_eboard_errors()
 ****************************************************************
 * @brief
 *  This function will add the source eboard counts to the
 *  total eboard counts.
 *
 * @param source_eboard_p - ptr to source eboard counts to be 
 *                         added to the total eboard counts.
 *
 *  total_eboard_p,  [IO]- ptr to the total eboard counts.
 *                       
 *
 * @return VALUE:
 *  None.
 *
 * @author
 *  02/08/06 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_raid_add_vp_eboard_errors( const fbe_raid_verify_error_counts_t * const source_eboard_p, 
                                           fbe_raid_verify_error_counts_t * const total_eboard_p )
{
    /* CRC errors.
     */
    total_eboard_p->c_crc_count += source_eboard_p->c_crc_count;
    total_eboard_p->u_crc_count += source_eboard_p->u_crc_count;
    total_eboard_p->c_crc_multi_count += source_eboard_p->c_crc_multi_count;
    total_eboard_p->u_crc_multi_count += source_eboard_p->u_crc_multi_count;

    /* Sectors which have been invalidated
     */
    total_eboard_p->invalidate_count += source_eboard_p->invalidate_count;

    /* Coherency errors.
     */
    total_eboard_p->c_coh_count += source_eboard_p->c_coh_count;
    total_eboard_p->u_coh_count += source_eboard_p->u_coh_count;

    /* Write stamp errors.
     */
    total_eboard_p->c_ws_count += source_eboard_p->c_ws_count;
    total_eboard_p->u_ws_count += source_eboard_p->u_ws_count;

    /* Time stamp errors.
     */
    total_eboard_p->c_ts_count += source_eboard_p->c_ts_count;
    total_eboard_p->u_ts_count += source_eboard_p->u_ts_count;

    /* Shed stamp errors.
     */
    total_eboard_p->c_ss_count += source_eboard_p->c_ss_count;
    total_eboard_p->u_ss_count += source_eboard_p->u_ss_count;

    /* Lba stamp errors.
     */
    total_eboard_p->c_ls_count += source_eboard_p->c_ls_count;
    total_eboard_p->u_ls_count += source_eboard_p->u_ls_count;

    /* Media errors.
     */
    total_eboard_p->c_media_count += source_eboard_p->c_media_count;
    total_eboard_p->u_media_count += source_eboard_p->u_media_count;
    total_eboard_p->c_soft_media_count += source_eboard_p->c_soft_media_count;
    total_eboard_p->retryable_errors += source_eboard_p->retryable_errors;
    total_eboard_p->non_retryable_errors += source_eboard_p->non_retryable_errors;
    total_eboard_p->shutdown_errors += source_eboard_p->shutdown_errors;

    /* Raw mirror errors.
     */
    total_eboard_p->u_rm_magic_count += source_eboard_p->u_rm_magic_count;
    total_eboard_p->c_rm_magic_count += source_eboard_p->c_rm_magic_count;
    total_eboard_p->c_rm_seq_count += source_eboard_p->c_rm_seq_count;

    return;
}
/**************************************
 * end fbe_raid_add_vp_eboard_errors()
 **************************************/

/****************************************************************
 * fbe_raid_siots_update_iots_vp_eboard()
 ****************************************************************
 * @brief
 *  This function will update the iots vp eboard with the
 *  totals from this siots' vcts eboard.
 *
 * @param siots_p  - pointer to siots_p
 *                       
 * ASSUMPTIONS:
 *  We do not assume anything about the context we are called in
 *  for this reason we check for existance of the fbe_raid_verify_error_counts_t
 *  and the vcts.
 *
 * @return VALUE:
 *  None.
 *
 * @author
 *  02/08/06 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_update_iots_vp_eboard( fbe_raid_siots_t *siots_p)
{
    fbe_raid_verify_error_counts_t *vp_eboard_local_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        
    /* If the client object supplied a vp eboard, add the local
     * counts.
     */
    vp_eboard_local_p = fbe_raid_siots_get_verify_eboard(siots_p);

    if ((vp_eboard_local_p != NULL) && (siots_p->vcts_ptr != NULL ))
    {
        /* We have an errorboard, and a siots_p->vcts_ptr.
         * Put it into the errorboard that was passed in to us,
         * so that the caller will get back the totals for this Verify.
         */
        fbe_raid_add_vp_eboard_errors(&siots_p->vcts_ptr->overall_eboard, 
                                      vp_eboard_local_p );

        /* For raw mirror reads, the client uses a different data structure. For correctable errors, a bitmask of 
         * failed mirrors is returned along with the error counts.
         */
        if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
        {
            fbe_raid_verify_raw_mirror_errors_t  *reference_p;

            reference_p = (fbe_raid_verify_raw_mirror_errors_t*)vp_eboard_local_p;

            reference_p->raw_mirror_magic_bitmap |= (siots_p->misc_ts.cxts.eboard.c_rm_magic_bitmap |
                                                     siots_p->misc_ts.cxts.eboard.u_rm_magic_bitmap);
            reference_p->raw_mirror_seq_bitmap |= siots_p->misc_ts.cxts.eboard.c_rm_seq_bitmap;
        }
    }

    return;
}
/****************************************
 * fbe_raid_siots_update_iots_vp_eboard
 ****************************************/

/*!***********************************************************
 *  fbe_raid_get_region_lba_for_position()
 ************************************************************
 *
 * @brief
 *  Get the lba for a given position within the error region
 *  structure.  We return the lba for the first error
 *  region that includes this position.
 *
 * PARAMTERS:
 * @param siots_p - Siots to inspect.
 * @param pos - Position to look for.
 * @param default_lba - Default LBA to return if error
 *                      region is empty (or does not exist).
 *
 * @return
 *  fbe_lba_t - lba of first error with this position.
 *
 * History:
 *  11/02/06:  Rob Foley -- Created.
 *
 ************************************************************/
fbe_lba_t fbe_raid_get_region_lba_for_position(fbe_raid_siots_t * const siots_p,
                                         fbe_u16_t pos,
                                         fbe_lba_t default_lba)
{
    /* Default return_lba to an unsupported lba.
     */
    fbe_lba_t return_lba = FBE_LBA_INVALID;
    fbe_s32_t idx;
    fbe_xor_error_regions_t * regions_p = &siots_p->vcts_ptr->error_regions;

    /* Only check the error region if it was allocated and is not empty.
     */
    if ( siots_p->vcts_ptr != NULL && !FBE_XOR_ERROR_REGION_IS_EMPTY(regions_p) )
    {
        fbe_u16_t pos_bitmask = (1 << pos);

        /* Not empty.
         * Check each of the error regions for this position.
         * If this position is found in one of the error regions,
         * then set the return lba to this region.
         */
        for (idx = 0; idx < regions_p->next_region_index; ++idx)
        {
            const fbe_xor_error_region_t *eregion_p = &regions_p->regions_array[idx];

            /* Check current region for a match
             * that is less than the current return value.
             * This allows us to find the first lba on this
             * drive that has an error.
             */
            if ( eregion_p->pos_bitmask & pos_bitmask &&
                 eregion_p->lba < return_lba )
            {
                /* Found a match, return this LBA.
                 */
                return_lba = eregion_p->lba;
            }
        }    /* end of for (idx...) */

    }    /* end if error regions not empty. */

    /* If return_lba was not changed, just return the default lba.
     */
    if ( return_lba == FBE_LBA_INVALID )
    {
        return_lba = default_lba;
    }
    return return_lba;
}
/***********************************************
 * end rg_get_region_lba_for_position()
 ***********************************************/

/*!**************************************************************
 *          rg_group_log_traffic()
 ****************************************************************
 * 
 *  @brief  Similar function as fbe_raid_siots_log_traffic except this
 *          log information when an RG (`group') isn't in the expected
 *          state which may or may not result in a IOTS being deferred
 *          or removed from the waiting queue.
 *
 * @param rgdb_p - RAID Group pointer that wasn't in the expected state  
 * @param iots_p - Optional IOTS to log
 * @param string_p - String with info on this request.
 *
 * @return - None.   
 *
 * @author
 *  03/23/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
#if 0 //def RG_DISPLAY_TRAFFIC_ENABLE
void rg_group_log_traffic(fbe_raid_geometry_t *const rgdb_p,
                          fbe_raid_iots_t *const iots_p, char *const string_p) 
{
    /* We always trace the RG information.
     */
    FLARE_RAID_GROUP_TRC64((
                           TRC_K_STD,
                           "RAID: %s rgstate: 0x%x rgevent: 0x%x rgactive: %d rgwait: %d schwait: %d schgen: %d\n",
                           string_p, rgdb_p->rg_state, rgdb_p->state_info.event,
                           rgdb_p->active_q.count, rgdb_p->wait_q.count,
                           rgdb_p->schedule_db.rg_waiting_count, rgdb_p->schedule_db.rg_generating_count));

    /* There maybe cases where there isn't a relavant IOTS assocaited with the
     * event.  If there is a valid IOTS add the information to the RG log.
     */
    if (iots_p != NULL)
    {
        FLARE_RAID_GROUP_TRC64((
                               TRC_K_STD,
                               "RAID: \t lun: 0x%x op:0x%x lba:0x%llX bl:0x%x \n",
                               iots_p->unit, iots_p->iorb.opcode, iots_p->iorb.lba, iots_p->iorb.blocks));
    }

    return;
}
#endif
/**************************************
 * end rg_group_log_traffic()
 **************************************/

/*!**************************************************************
 * @fn fbe_raid_siots_log_traffic(fbe_raid_siots_t *const siots_p, char *const string_p)
 ****************************************************************
 * @brief
 *  Log information about a siots to the ktrace.
 *  
 * @param siots_p - Siots to log.
 * @param string_p - String with info on this request.
 *
 * @return - None.   
 *
 * @author
 *  12/2/2008 - Created. Rob Foley
 *
 ****************************************************************/

#if 0 // def RG_DISPLAY_TRAFFIC_ENABLE
void fbe_raid_siots_log_traffic(fbe_raid_siots_t *const siots_p, char *const string_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    FLARE_RAID_TRAFFIC_TRC64((
                             TRC_K_STD,
                             "RAID: %s lun: 0x%x op:0x%x lba:0x%llX bl:0x%x slba:0x%llX sbl:0x%x ps:0x%llx pc:0x%x alg:0x%x\n",
                             string_p, iots_p->unit, iots_p->iorb.opcode, iots_p->iorb.lba, iots_p->iorb.blocks,
                             siots_p->start_lba, siots_p->xfer_count, 
                             siots_p->parity_start, siots_p->parity_count, siots_p->algorithm));
    return;
}
#endif
/**************************************
 * end fbe_raid_siots_log_traffic()
 **************************************/


/*!**********************************************
 * @brief Adjust the reason code in the error bits based on error info from the error region.
 *
 * @details
 * If there are multiple error regions containing CRC errors on the same drive,
 * we can't determine whether an individual error is single or multi bit just
 * by using the eboard, since it is shared by all regions. We must also consider 
 * the region specific error information (contained in error_type). 
 * 
 * @param xor_error_type - Error information from the error region
 * @param err_bits - Pointer to the error bits derived from the eboard.
 *
 * @return void - None. * * @version:
 *  06/29/09:  HEW -- Created.
 ***********************************************/
void fbe_raid_region_fix_crc_reason( fbe_u16_t xor_error_type, fbe_u32_t *err_bits )
{
    fbe_u32_t reason_code = (*err_bits) & VR_REASON_MASK;

    if ( (xor_error_type == FBE_XOR_ERR_TYPE_MULTI_BIT_CRC) && (reason_code == VR_SINGLE_BIT_CRC))
    {
        *err_bits  &= ~VR_REASON_MASK;
        *err_bits |= VR_MULTI_BIT_CRC; 
    }
    else if ( (xor_error_type == FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC) && (reason_code == VR_MULTI_BIT_CRC))
    {
        *err_bits  &= ~VR_REASON_MASK;
        *err_bits |= VR_SINGLE_BIT_CRC; 
    }
    return;
}

/***********************************************
 * end rg_region_fix_crc_reason()
 ***********************************************/




/*!***********************************************************
 *  fbe_raid_get_csum_error_bits_for_given_region()
 ************************************************************
 *
 * @brief
 *  Determine the correctable extended checksum bits as per
 *  available information in XOR error region and eboard.
 *
 * @param   xor_region_p   - pointer to the xor region to inspect
 * @param   position_key   - current position key to inspect. 
 * @param   csum_bits_p    - pointer to return error status bits
 *
 * @return fbe_status_t
 *
 * @author
 *  11/28/2010:  Jyoti Ranjan -- Created.
 *
 ************************************************************/
 fbe_status_t fbe_raid_get_csum_error_bits_for_given_region(const fbe_xor_error_region_t * const xor_region_p,
                                                            fbe_u16_t position_key,
                                                            fbe_u32_t * csum_bits_p)
 {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t xor_error_code = 0;
    fbe_u32_t vr_error_bits = 0;
    
    if ((xor_region_p->pos_bitmask & position_key ) == 0 )
    {
        /* XOR regions entry is not meant for the given position
         */
        *csum_bits_p = 0;
        return FBE_STATUS_OK;
    }

    /* Get the xor error code (from xor error region) by chopping 
     * reason-code.
     */
    xor_error_code = xor_region_p->error & FBE_XOR_ERR_TYPE_MASK;

    /* Determine appropriate CRC error bits as per the xor region 
     * error code.
     */    
    switch(xor_error_code)
    {
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
            /* Media error detected, add it to our mask.
             */
            vr_error_bits |= VR_MEDIA_CRC;
            break;

        case FBE_XOR_ERR_TYPE_KLOND_CRC:
            /* Klondike invalid crc detected, add it to our mask.
             */
            vr_error_bits |= VR_KLONDIKE_CRC;
            break;

        case FBE_XOR_ERR_TYPE_DH_CRC:
            /* DH invalid crc detected, add it to our mask.
             */
            vr_error_bits |= VR_DH_CRC;
            break;

        case FBE_XOR_ERR_TYPE_RAID_CRC:
            /* Raid invalid crc detected, add it to our mask.
             */
            vr_error_bits |= VR_RAID_CRC;
            break;

        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED:
            /* Corrupt CRC invalid format detected. This means 
             * that RAID intentionally invalidated this block 
             * because of a corrupt crc command.
             *
             * Add it to our mask.
             */
            vr_error_bits |= VR_CORRUPT_CRC;
            break;
        
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED:
            /* Raid invalid crc detected, add it to our mask.
             */
            vr_error_bits |= VR_CORRUPT_DATA;
            break;

        case FBE_XOR_ERR_TYPE_INVALIDATED:
            /* Flag the `data lost - invalidated' sector.
             */
            vr_error_bits |= VR_INVALID_CRC;
            break;
                  
        case FBE_XOR_ERR_TYPE_BAD_CRC:
            /* Attempt write data with bad crc
             */
            vr_error_bits |= VR_BAD_CRC;
            break;
                        
        case FBE_XOR_ERR_TYPE_LBA_STAMP:
            /* Lba-stamp error detected, add it to our mask
             */
            vr_error_bits |= VR_LBA_STAMP;
            break;

        case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
            /* Unexpected single-bit CRC detected, add it to our mask.
             */
            vr_error_bits |= (VR_UNEXPECTED_CRC | VR_SINGLE_BIT_CRC);
            break;
        
        case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
            /* Unexpected multi-bit CRC detected, add it to our mask.
             */
            vr_error_bits |= (VR_UNEXPECTED_CRC | VR_MULTI_BIT_CRC);
            break;

        case FBE_XOR_ERR_TYPE_CRC:
            /* Each unknown CRC should have either been classified as 
             * single-bit CRC or multi-bit CRC. So, we do not expect
             * any XOR region with above error code.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
            /* We do expect it to translate to either 
             * FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR or FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR.
             * And, we do not expect error region to be crated with
             * this code.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_XOR_ERR_TYPE_COPY_CRC:
            /* COPY invalid crc detected, add it to our mask.
             */
            vr_error_bits |= VR_COPY_CRC;
            break;

        case FBE_XOR_ERR_TYPE_PVD_METADATA:
            /* PVD Metadata crc detected, add it to our mask.
             */
            vr_error_bits |= VR_PVD_METADATA_CRC;
            break;     
    }

    *csum_bits_p = vr_error_bits;
    return status;
 }
/**********************************************************
 * fbe_raid_get_csum_error_bits_for_given_region()
 *********************************************************/



/*!***********************************************************
 *  fbe_raid_get_error_bits_for_given_region()
 ************************************************************
 *
 * @brief
 *  Determine the correctable extended status bits as per 
 *  available information in eboard and xor error region.
 *
 * @param  xor_region_p  - pointer to the xor region to inspect
 * @param  b_errors_retried - a flag to indicate whether CRC was retried or not. 
 *                            Effect of this parameter is only on CRC related 
 *                            error. For all other types of errors, it is ignored.
 * @param  position_key  - current position key to inspect.
 * @param  b_correctable - If FBE_TRUE, get correctable error bits
 *                         else get uncorrectable error bits for
 *                         the given XOR error region
 * @param  err_bits_p    - pointer to return correctable status bits

 *
 * @return fbe_status_t
 *
 * @author
 *  11/28/2010:  Jyoti Ranjan -- Created.
 *
 * @Note:
 *
 * Please note that if value of b_errors_retried is FBE_TRUE 
 * then error status bits returned by caller is ORed with 
 * VR_CRC_RETRY. It is to indicate that checksum/stamp error(s) 
 * was/were retired. Caller is not expected to send FBE_TRUE
 * for b_errors_retried if error region is meant for 
 * uncorrectable error. It is so because, we do report 
 * VR_CRC_RETRY in error reason field only for those errors 
 * which did not occur -AFTER- retry.
 *
 ************************************************************/
 fbe_status_t fbe_raid_get_error_bits_for_given_region(const fbe_xor_error_region_t * const xor_region_p,
                                                       fbe_bool_t b_errors_retried,
                                                       fbe_u16_t position_key,
                                                       fbe_bool_t b_correctable,
                                                       fbe_u32_t * err_bits_p)
{
    fbe_u32_t vr_error_bits;
    fbe_u32_t xor_error_code;
    fbe_status_t status = FBE_STATUS_OK;


    /* No further processing is required if following conditions are meet:
     *     1. xor error region is of -uncorrectable- error and caller
     *        has asked for -correctable- error bits.
     *     2. xor error region is of -correctable- error and caller 
     *        has asked for -uncorrectable- error bits.
     */
    if (((xor_region_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) && (b_correctable == FBE_TRUE)) ||
        (((xor_region_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) == 0) && (b_correctable == FBE_FALSE)) ||
        ((xor_region_p->pos_bitmask & position_key ) == 0 ))
    {
        *err_bits_p = 0;
        return FBE_STATUS_OK;
    }


     /* Set to zero, since initially we have not detected any
      * errors set in a bitmask.
      */
    vr_error_bits = 0;

    /* Get the XOR error code by masking reason code
     */
    xor_error_code = xor_region_p->error & FBE_XOR_ERR_TYPE_MASK;

    /* Determins error bits as per XOR error code
     */
    switch(xor_error_code)
    {
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_CRC:
        case FBE_XOR_ERR_TYPE_KLOND_CRC:
        case FBE_XOR_ERR_TYPE_DH_CRC:
        case FBE_XOR_ERR_TYPE_INVALIDATED:
        case FBE_XOR_ERR_TYPE_RAID_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
        case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
        case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED:
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED:
        case FBE_XOR_ERR_TYPE_LBA_STAMP:
        case FBE_XOR_ERR_TYPE_COPY_CRC:
        case FBE_XOR_ERR_TYPE_PVD_METADATA:
            /* CRC error detected, add it to bitmask.
             */
            status = fbe_raid_get_csum_error_bits_for_given_region(xor_region_p,
                                                                   position_key,
                                                                   &vr_error_bits);
            break;

        case FBE_XOR_ERR_TYPE_WS:
            /* Write stamp error detected, add it to bitmask.
             */
            vr_error_bits |= VR_WS;
            break;

        case FBE_XOR_ERR_TYPE_TS:
            /* Time stamp error detected, add it to bitmask.
             */
            vr_error_bits |= VR_TS;
            break;

        case FBE_XOR_ERR_TYPE_SS:
            /* Shed stamp error detected, add it to bitmask.
             */
            vr_error_bits |= VR_SS;
            break;
        
        case FBE_XOR_ERR_TYPE_COH:
            /* Coherency error detected, add it to bitmask.
             */
            vr_error_bits |= VR_COH;
            break;

        case FBE_XOR_ERR_TYPE_COH_UNKNOWN:
             /* An unknown, unhandled coherency error, add it to bitmask.
              */
             vr_error_bits |= VR_UNKNOWN_COH;
            break;

        case FBE_XOR_ERR_TYPE_N_POC_COH:
             /* Multiple POC errors, but we don't know which
              * position is in error, add it to bitmask.
              */
             vr_error_bits |= VR_N_POC;
            break;

        case FBE_XOR_ERR_TYPE_POC_COH:
            /* Parity of checksums error, add it to the bitmask.
             */
            vr_error_bits |= VR_POC;
            break;


        case FBE_XOR_ERR_TYPE_RB_FAILED:
            /* Its different from all in the sense that we do 
             * expect a XOR region to be created with this code. 
             * But we will not report it. So we will not 
             * modify error bits.
             */
            break;

            
        case FBE_XOR_ERR_TYPE_BOGUS_TS:
        case FBE_XOR_ERR_TYPE_BOGUS_WS:
        case FBE_XOR_ERR_TYPE_BOGUS_SS:
            /* Above error type should get translated to general TS, 
             * WS and SS errors. So, we do not expect it an xor-region
             * to be created with above code.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_XOR_ERR_TYPE_1POC:
        case FBE_XOR_ERR_TYPE_1NS:
        case FBE_XOR_ERR_TYPE_1S:
        case FBE_XOR_ERR_TYPE_1R:
        case FBE_XOR_ERR_TYPE_1D:
        case FBE_XOR_ERR_TYPE_1COD:
        case FBE_XOR_ERR_TYPE_1COP:
        case FBE_XOR_ERR_TYPE_TIMEOUT_ERR:
            /* We do not expect an XOR error regions to be created
             * with this error code
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    if (status != FBE_STATUS_OK) { return status; }

    /* Let us add bits indicating reason related information, if
     * possible. Also, please note that we do not have corresponding
     * VR_* bits for all XOR reason code.
     */
    if (xor_region_p->error & FBE_XOR_ERR_TYPE_ZEROED)
    {   
        vr_error_bits |= VR_ZEROED;
    }
    
    if (b_errors_retried != FBE_FALSE)
    {
        vr_error_bits |= VR_CRC_RETRY;
    }

    *err_bits_p = vr_error_bits;
    return FBE_STATUS_OK;
}
/**********************************************************
 * fbe_raid_get_error_bits_for_given_region()
 *********************************************************/

/*!***************************************************************************
 *          fbe_raid_record_bad_crc_on_write()
 *****************************************************************************
 * 
 * @brief   This method uses the error regions to report `Bad CRC' (i.e.
 *          checksum on write data received was bad and the sector was not
 *          invalidated due to data lost).  In the past we would panic the SP
 *          when this error occurred. Now report the errors and it is up to the
 *          client to determine if the SP should be faulted or not.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return  fbe_status_t
 *
 * @author
 *  07/12/2011  Ron Proulx  - Created
 *****************************************************************************/
fbe_status_t fbe_raid_record_bad_crc_on_write(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index;
    fbe_u32_t error_code;
    fbe_u32_t error_bits;
    fbe_u32_t array_pos;
    fbe_u16_t array_pos_mask;
    const fbe_xor_error_regions_t * regions_p;

    regions_p = &siots_p->vcts_ptr->error_regions;

    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* array_pos_mask has the current position in the error bitmask 
         * that we need to check below.
         */
        array_pos_mask = 1 << array_pos;

        /* traverse the xor error region for current disk position
         */
        for (region_index = 0; region_index < regions_p->next_region_index; ++region_index)
        {
            if ((regions_p->regions_array[region_index].error != 0) &&
                (regions_p->regions_array[region_index].pos_bitmask & array_pos_mask))
            {
                /* We only report `Bad CRC' errors
                 */
                error_bits = 0;
                error_code = regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
                switch(error_code)
                {
                    case FBE_XOR_ERR_TYPE_BAD_CRC:
                        error_bits = (VR_BAD_CRC | VR_UNEXPECTED_CRC);
                        break;
                    default:
                        break;
                }

                /* We will log message even if drive is dead as we want to
                 * notify that correspondign sector of drive is lost.
                 */
                if (error_bits != 0)
                {
                    fbe_raid_send_unsol(siots_p,
                                        SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR,
                                        array_pos,
                                        regions_p->regions_array[region_index].lba,
                                        regions_p->regions_array[region_index].blocks,
                                        error_bits,
                                        fbe_raid_extra_info(siots_p));
                }

            } /* end if ((regions_p->regions_array[region_idx].error != 0) ... */
        } /* end for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx) */
    } /* end for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++) */ 

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_raid_record_bad_crc_on_write()
 **********************************************/

/*!*******************************************************************
 * fbe_raid_extra_info()
 *********************************************************************
 * @brief This information is logged in the event log.
 *        The below formats the extra info as follows.  See below.
 * 
 * @param siots_p - Current I/O
 * 
 * @return fbe_u32_t information to log in event log.
 * 
 * @param
 * byte   |   3    |    2    |   1   |   0   |
 *        | unused |  flags  |   Algorithm   |
 *
 *********************************************************************/
fbe_u32_t fbe_raid_extra_info(fbe_raid_siots_t *siots_p)
{
    return fbe_raid_extra_info_alg(siots_p, siots_p->algorithm);
}
/**************************************
 * end fbe_raid_extra_info()
 **************************************/

/*!*******************************************************************
 * fbe_raid_extra_info_alg()
 *********************************************************************
 * @brief This information is logged in the event log.
 *        The below formats the extra info as follows.  See below.
 * 
 * @param siots_p - Current I/O
 * 
 * @return fbe_u32_t information to log in event log.
 * 
 * Extra info format:
 * byte   |   3    |    2    |   1   |   0   |
 *        | unused |  flags  |   Algorithm   |
 *
 *********************************************************************/
fbe_u32_t fbe_raid_extra_info_alg(fbe_raid_siots_t *siots_p, fbe_u32_t algorithm)
{
    fbe_u32_t extra_info;
    fbe_raid_group_paged_metadata_t * chunk_info_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    extra_info = algorithm;

    fbe_raid_iots_get_chunk_info(fbe_raid_siots_get_iots(siots_p), 0, &chunk_info_p);
    if (chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_ERROR)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_ERROR_VR << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }
    if (chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_INCOMPLETE_WR_VR << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }
    if (chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_SYSTEM)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_SYSTEM_VR << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }
    if (chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_USER_RD_VR << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }
    if (chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_USER_VR << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
    {
        extra_info |= FBE_RAID_EXTRA_INFO_FLAGS_VERIFY_WRITE << FBE_RAID_EXTRA_INFO_FLAGS_OFFSET;
    }
    return extra_info;
}
/**************************************
 * end fbe_raid_extra_info_alg()
 **************************************/
/******************************
 * end fbe_raid_record_errors.c
 ******************************/
