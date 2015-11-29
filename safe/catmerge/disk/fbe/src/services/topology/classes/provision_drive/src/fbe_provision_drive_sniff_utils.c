/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_sniff_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code which provides generic function interface
 *  for the sniff functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   12/03/2010:  Created. Maya Dagon
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_provision_drive.h"
#include "base_object_private.h"
#include "fbe_provision_drive_private.h"



/*FIXME: these are bogus numbers for now*/
#define FBE_PROVISION_DRIVE_VERIFY_MONITOR_CPU_PER_SECOND 500
#define FBE_PROVISION_DRIVE_VERIFY_MONITOR_MBYTES_CONSUMPTION 1

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_status_t fbe_provision_drive_increment_sniff_count(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_inc_sniff_pass_count_persist_completion( fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/*!****************************************************************************
 *  fbe_provision_drive_get_verify_permission_by_priority
 ******************************************************************************
 *
 * @brief
 *    This function calls fbe_base_config_get_operation_permission to get permsission to issue
 *    verify/remap i/o
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   verify_permission_p   -  pointer to a boolean that will keep the permission result  
 *        
 ******************************************************************************/

fbe_status_t fbe_provision_drive_get_verify_permission_by_priority(
              fbe_provision_drive_t*      provision_drive_p,            
              fbe_bool_t*                 verify_permission_p)
{

    fbe_base_config_downstream_health_state_t  downstream_health;  // health of downstream object
    fbe_base_config_operation_permission_t     permission_params;  // verify i/o permission parameters
    fbe_status_t                               status;             // fbe status

    // get health of downstream object
    downstream_health = fbe_provision_drive_verify_downstream_health( provision_drive_p );

     // terminate current monitor cycle if unable to access logical drive object
    if ( downstream_health != FBE_DOWNSTREAM_HEALTH_OPTIMAL )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check whether current monitor cycle has remap i/o permission
    *verify_permission_p = FBE_FALSE;
    permission_params.operation_priority                        = provision_drive_p->verify_priority;
    permission_params.credit_requests.cpu_operations_per_second = fbe_provision_drive_class_get_sniff_cpu_rate();
    permission_params.credit_requests.mega_bytes_consumption    = 0;
    permission_params.io_credits                                = 1;
	
	/*we wse FBE_FALSE in the 3rd argumetn since sniff does not count as IO of power saving purposes*/
    status = fbe_base_config_get_operation_permission(
                 (fbe_base_config_t *) provision_drive_p, &permission_params, FBE_FALSE, verify_permission_p );

    return status;
} // end fbe_provision_drive_get_verify_permission_by_priority


/*!****************************************************************************
 *  fbe_provision_drive_update_verify_report_error_counts_after_full_pass
 ******************************************************************************
 *
 * @brief
 *    This function clears verify i/o error counters in the verify report
 *    structure
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  void
 *
 *
 ******************************************************************************/

void
 fbe_provision_drive_update_verify_report_error_counts_after_full_pass(
                                               fbe_provision_drive_t* in_provision_drive_p
                                             )
{
    fbe_provision_drive_verify_report_t* verify_report_p;   // pointer to sniff verify report

      // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get pointer to sniff verify report
    fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

    // copy current pass verify results to previous pass verify results
    verify_report_p->previous = verify_report_p->current;

    // clear verify results for the current pass
    fbe_zero_memory( (void *)&verify_report_p->current, sizeof(fbe_provision_drive_verify_error_counts_t) );

    return;
}   // end  fbe_provision_drive_update_verify_report_error_counts_after_full_pass()


/*!****************************************************************************
 * fbe_provision_drive_update_verify_report_after_full_pass
 ******************************************************************************
 *
 * @brief
 *    This function updates the sniff verify report whenever a verify pass
 *    completes or media errors occurred during a verify i/o.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  boolean - indicates if sniff verify pass completed
 *      
 ******************************************************************************/

fbe_bool_t
fbe_provision_drive_update_verify_report_after_full_pass(
                                          fbe_provision_drive_t* in_provision_drive_p
                                        )
{
    fbe_lba_t                                   checkpoint;       // sniff verify checkpoint
    fbe_bool_t                                  is_pass_b = FBE_FALSE;        // true if verify pass completed
    fbe_provision_drive_verify_report_t*        verify_report_p;  // sniff verify report

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get current verify checkpoint
    fbe_provision_drive_metadata_get_sniff_verify_checkpoint( in_provision_drive_p, &checkpoint );

    // determine if a verify pass just completed
    is_pass_b = ( checkpoint ? FBE_FALSE : FBE_TRUE );

    // update verify report if a verify pass completed
    if ( is_pass_b )
    {
        // get pointer to sniff verify report
        fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

        // TODO - revise locking mechanism
        // lock access to sniff verify report
        fbe_provision_drive_lock( in_provision_drive_p );
        
        fbe_provision_drive_update_verify_report_error_counts_after_full_pass( in_provision_drive_p );

        // unlock access to sniff verify report
        fbe_provision_drive_unlock( in_provision_drive_p );
    }

    return is_pass_b;
}   // end fbe_provision_drive_update_verify_report_after_full_pass()

/*!****************************************************************************
 * fbe_provision_drive_inc_sniff_pass_count_persist_completion
 ******************************************************************************
 *
 * @brief
 *   This function handles sniff verify pass count increment update completion.
 *   It completes the operation with appropriate status.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 *
 * @version
 *    06/26/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_inc_sniff_pass_count_persist_completion(
                                    fbe_packet_t * packet_p, 
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_status_t                            status;  

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* Get the packet status */
    status = fbe_transport_get_status_code(packet_p);

    /* Check if checkpoint update completed successfully */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify pass count update failed, status:%d.",
                                __FUNCTION__, status);

        return status;

    }

    fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_increment_sniff_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;;
}
/******************************************************************************
 * end fbe_provision_drive_inc_sniff_pass_count_persist_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_sniff_media_error_lba_update_completion
 ******************************************************************************
 *
 * @brief
 *   This function handles sniff verify media error lba update completion.
 *   It completes the operation with appropriate status.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 *
 * @version
 *    12/16/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_sniff_media_error_lba_update_completion(
                                    fbe_packet_t * in_packet_p, 
                                    fbe_packet_completion_context_t in_context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;  
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *)in_context;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(in_packet_p);

    // check if checkpoint update completed successfully
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify checkpoint update failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(in_packet_p, status, 0);

        return status;
        
    }

    // set metadata operation status in packet and complete operation
    fbe_transport_set_status( in_packet_p, status, 0 );

    return FBE_STATUS_OK;
}



/*!****************************************************************************
 * fbe_provision_drive_sniff_verify_checkpoint_update_completion
 ******************************************************************************
 *
 * @brief
 *   This function handles sniff verify checkpoint update completion.
 *   It completes the operation with appropriate status.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 *
 * @version
 *    02/12/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_sniff_verify_checkpoint_update_completion(
                                    fbe_packet_t * in_packet_p, 
                                    fbe_packet_completion_context_t in_context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_status_t                            status;  
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_bool_t                              is_pass_b;        // true if verify pass completed

    provision_drive_p = (fbe_provision_drive_t *)in_context;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(in_packet_p);

    /* Check if checkpoint update completed successfully */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify checkpoint update failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(in_packet_p, status, 0);

        return status;
        
    }

    /* Update verify report as needed */
    is_pass_b = fbe_provision_drive_update_verify_report_after_full_pass( provision_drive_p);

    /* If sniff verify passed, increment the pass count in NP metadata*/
    if (is_pass_b && fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p))
    {
        fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_inc_sniff_pass_count_persist_completion, 
                                                  provision_drive_p);  
    }
    
    // set completion function
    fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_sniff_media_error_lba_update_completion , provision_drive_p);    
    fbe_provision_drive_metadata_set_sniff_media_error_lba( provision_drive_p,in_packet_p, FBE_LBA_INVALID);              
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
}   // End  fbe_provision_drive_sniff_verify_checkpoint_update_completion()
   
/*!****************************************************************************
 * fbe_provision_drive_increment_sniff_count()
 ******************************************************************************
 * @brief
 *  This function is used to increment the sniff pass count and persist it.
 *
 * @param packet_p      - pointer to a control packet from the scheduler.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/26/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_increment_sniff_count(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    fbe_u32_t                           pass_count = 0;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_metadata_get_sniff_pass_count(provision_drive_p, &pass_count);
    pass_count++;

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_metadata_set_sniff_pass_count(provision_drive_p, packet_p, pass_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_increment_sniff_count()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_advance_verify_checkpoint
 ******************************************************************************
 *
 * @brief
 *    This function advances the verify checkpoint to point to the starting
 *    lba  of  the  next chunk to verify.  It handles "wrapping" the verify
 *    checkpoint, that is, setting  the  verify  checkpoint  to the drive's
 *    starting lba when the checkpoint is moved beyond the drive's exported
 *    capacity.
 *
 * @param   provision_drive_p - pointer to provision drive
 * @param   packet_p          - pointer to a control packet from the scheduler
 *
 * @return  fbe_status_t         - FBE_STATUS_MORE_PROCESSING_REQUIRED 
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *    02/28/2012 - Ashok Tamilarasan - Updated to include the metadata region
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_advance_verify_checkpoint(fbe_provision_drive_t* provision_drive_p,
                                              fbe_packet_t*   packet_p)
{
    fbe_lba_t                                 checkpoint;         // sniff verify checkpoint
    fbe_lba_t                                 exported_capacity;  
    fbe_status_t                              status;  
    fbe_block_count_t                         block_count;
    fbe_lba_t                                 total_capacity;
    fbe_lba_t                                 paged_metadata_capacity;
    
    
    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( provision_drive_p );

    block_count = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    // get current verify checkpoint from non-paged metadata
    fbe_provision_drive_metadata_get_sniff_verify_checkpoint( provision_drive_p, &checkpoint );
    
             
    // set completion function
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_sniff_verify_checkpoint_update_completion , provision_drive_p);    

    // get exported capacity for this provision drive
    //fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    fbe_provision_drive_get_user_capacity(provision_drive_p, &exported_capacity);
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                         &paged_metadata_capacity);

    /* Even though we are allocating space for the mirror copy this is not being used. So no need to sniff and reduce
     * capacity size for the mirror region
     */
    paged_metadata_capacity /= FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES;

    total_capacity = exported_capacity + paged_metadata_capacity;
    // if the verify checkpoint is not within the drive's consumed capacity
    // capacity then set verify checkpoint to drive's starting lba
    if ((checkpoint + block_count) >= total_capacity )
    {
        checkpoint = 0;

        // update verify checkpoint for the next verify i/o with sending request to metadata service
        status = fbe_provision_drive_metadata_set_sniff_verify_checkpoint( provision_drive_p, packet_p, checkpoint );
    }
    else
    {
        fbe_u32_t ms_since_checkpoint;
        fbe_time_t last_checkpoint_time;

        fbe_provision_drive_get_last_checkpoint_time(provision_drive_p, &last_checkpoint_time);
        ms_since_checkpoint = fbe_get_elapsed_milliseconds(last_checkpoint_time);

        if (ms_since_checkpoint > FBE_PROVISION_DRIVE_PEER_CHECKPOINT_UPDATE_MS)
        {
            // update verify checkpoint for the next verify i/o with sending request to metadata service
            status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)provision_drive_p,
                                                                            packet_p,
                                                                            (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->sniff_verify_checkpoint),
                                                                            0,
                                                                            checkpoint + block_count);
            fbe_provision_drive_update_last_checkpoint_time(provision_drive_p);
        }
        else
        {

            // update verify checkpoint for the next verify i/o with sending request to metadata service
            status = fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer((fbe_base_config_t *)provision_drive_p,
                                                                               packet_p,
                                                                               (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->sniff_verify_checkpoint),
                                                                               0,
                                                                               checkpoint,
                                                                               block_count);

        }
    }
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.  The condition will remain set so we will try again.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) provision_drive_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, "%s: error %d on nonpaged write call\n", __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}   // end fbe_provision_drive_advance_verify_checkpoint()


/*!****************************************************************************
 * fbe_provision_drive_increment_verify_report_unrecovered_error_count
 ******************************************************************************
 *
 * @brief
 *    This function updates verify i/o error counters in the verify report
 *    structure when a verify i/o completes with a hard media error.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_io_status          -  provision drive i/o status
 * 
 * @return  void
 *
 *
 ******************************************************************************/
void            
fbe_provision_drive_increment_verify_report_unrecovered_error_count(
                                                   fbe_provision_drive_t*  in_provision_drive_p)
{
    fbe_provision_drive_verify_error_counts_t*  verify_errors_p;        // verify error counters
    fbe_provision_drive_verify_report_t* verify_report_p;   // pointer to sniff verify report

    // get pointer to sniff verify report
    fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

    // TODO - revise locking mechanism
    // lock access to sniff verify report
    fbe_provision_drive_lock( in_provision_drive_p );

    verify_errors_p = &(verify_report_p->current);
    verify_errors_p->unrecov_read_count++;
    verify_errors_p = &(verify_report_p->totals);
    verify_errors_p->unrecov_read_count++;

    // unlock access to sniff verify report
    fbe_provision_drive_unlock( in_provision_drive_p );
}



/*!****************************************************************************
 * fbe_provision_drive_increment_verify_report_recovered_error_count
 ******************************************************************************
 *
 * @brief
 *    This function updates verify i/o error counters in the verify report
 *    structure when a verify i/o completes with a soft media error.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_io_status          -  provision drive i/o status
 * 
 * @return  void
 *
 *
 ******************************************************************************/
void            
fbe_provision_drive_increment_verify_report_recovered_error_count(
                                                   fbe_provision_drive_t*  in_provision_drive_p)
{
    fbe_provision_drive_verify_error_counts_t*  verify_errors_p;        // verify error counters
    fbe_provision_drive_verify_report_t* verify_report_p;   // pointer to sniff verify report

    // get pointer to sniff verify report
    fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

    // TODO - revise locking mechanism
    // lock access to sniff verify report
    fbe_provision_drive_lock( in_provision_drive_p );

    verify_errors_p = &(verify_report_p->current);
    verify_errors_p->recov_read_count++;
    verify_errors_p = &(verify_report_p->totals);
    verify_errors_p->recov_read_count++;

    // unlock access to sniff verify report
    fbe_provision_drive_unlock( in_provision_drive_p );
}



/*!****************************************************************************
 * fbe_provision_drive_update_verify_report_error_counts
 ******************************************************************************
 *
 * @brief
 *    This function updates verify i/o error counters in the verify report
 *    structure when a verify i/o completes with a hard or soft media error.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_io_status          -  provision drive i/o status
 * 
 * @return  void
 *
 *
 ******************************************************************************/

void
fbe_provision_drive_update_verify_report_error_counts(
                                                fbe_provision_drive_t*          in_provision_drive_p,
                                                fbe_provision_drive_io_status_t in_io_status
                                              )
{
    fbe_lba_t            last_media_error_lba;   // last media error lba
   

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );
    
    fbe_provision_drive_metadata_get_sniff_media_error_lba( in_provision_drive_p, &last_media_error_lba );

    // update verify i/o error counters only once per chunk
    if ( last_media_error_lba == FBE_LBA_INVALID )
    {

        // for hard media errors
        if ( in_io_status == FBE_PROVISION_DRIVE_IO_STATUS_HARD_MEDIA_ERROR )
        {
            // increment count of unrecoverable read errors
            fbe_provision_drive_increment_verify_report_unrecovered_error_count(in_provision_drive_p);
            return;
        }

        // for soft media errors
        if ( in_io_status == FBE_PROVISION_DRIVE_IO_STATUS_SOFT_MEDIA_ERROR )
        {
            // increment count of recoverable read errors
            fbe_provision_drive_increment_verify_report_recovered_error_count(in_provision_drive_p);

        }
    }

    return;
}   // end fbe_provision_drive_update_verify_report_error_counts()


