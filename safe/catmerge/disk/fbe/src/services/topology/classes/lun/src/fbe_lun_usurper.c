/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the lun object code for the usurper.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_lun_private.h"
#include "fbe_transport_memory.h"
#include "fbe_database.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe/fbe_types.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t
fbe_lun_set_configuration(fbe_lun_t * lun_p, fbe_packet_t * packet_p);

static fbe_status_t 
fbe_lun_usurper_initiate_verify(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t 
fbe_lun_usurper_initiate_zero(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t 
fbe_lun_usurper_initiate_hard_zero(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);


static fbe_status_t 
fbe_lun_usurper_initiate_zero_completion(fbe_packet_t * packet_p,
                                                       fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_lun_usurper_initiate_hard_zero_completion(fbe_packet_t * packet_p,
                                              fbe_packet_completion_context_t context);

static fbe_status_t
fbe_lun_usurper_get_verify_report(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);  
static fbe_status_t
fbe_lun_usurper_get_verify_status(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t
fbe_lun_usurper_get_verify_status_completion(fbe_packet_t* in_packet_p, fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_lun_usurper_get_info(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_get_raid_info_completion(fbe_packet_t* in_packet_p, 
                                                             fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_lun_usurper_set_power_save_info(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_trespass_op(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_zero_fill(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t fbe_lun_usurper_clear_verify_report(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static void fbe_lun_usurper_clear_verify_result_counts(fbe_lun_verify_counts_t* in_results_p);

static void fbe_lun_usurper_clear_mirror_objects_count(fbe_u32_t* mirror_objects_count_p);

static fbe_status_t fbe_lun_usurper_get_control_buffer(
                                fbe_lun_t*                          in_lun_p,
                                fbe_packet_t*                       in_packet_p,
                                fbe_payload_control_buffer_length_t in_buffer_length,
                                fbe_payload_control_buffer_t*       out_buffer_pp);

static fbe_status_t fbe_lun_usurper_prepare_to_destroy_lun( fbe_lun_t* in_lun_p,
                                                            fbe_packet_t* in_packet_p);

static fbe_status_t fbe_lun_usurper_set_power_saving_policy( fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_get_power_saving_policy( fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t fbe_lun_initialize_power_saving(fbe_lun_t *lun_p, fbe_database_lun_configuration_t * set_config_p);

static fbe_status_t fbe_lun_usurper_get_zero_status(fbe_lun_t * in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_get_lun_info(fbe_lun_t * in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t fbe_lun_usurper_enable_performace_statistics(fbe_lun_t* in_lun_p,
                                                        fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_disable_performace_statistics(fbe_lun_t* in_lun_p,
                                                        fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_get_performace_statistics(fbe_lun_t *in_lun_p,
                                                 fbe_packet_t *in_packet_p);

static fbe_status_t fbe_lun_usurper_get_attributes(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_export_device(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t lun_connect_to_bvd(fbe_lun_t *lun_p);
static fbe_status_t fbe_lun_connect_bvd_interface(fbe_lun_number_t lun_number, fbe_lun_t * const lun_p);
static fbe_status_t fbe_lun_usurper_set_gen_num(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p);

static fbe_status_t fbe_lun_usurper_set_write_bypass(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p, fbe_bool_t write_bypass_mode);
static fbe_status_t fbe_lun_usurper_get_rebuild_status(fbe_lun_t * in_lun_p, fbe_packet_t* in_packet_p);
static fbe_status_t fbe_lun_usurper_get_rebuild_status_completion(fbe_packet_t * packet_p,
                                                                  fbe_packet_completion_context_t context);
static fbe_status_t fbe_lun_reset_unexpected_error_info(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_disable_fail_on_unexpected_errors(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_enable_fail_on_unexpected_errors(fbe_lun_t* lun_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_lun_prepare_for_power_shutdown(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_set_read_and_pin_index(fbe_lun_t* lun_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_lun_enable_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_disable_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_get_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_lun_usurper_start_clear_page(fbe_packet_t * packet_p,
                                                     fbe_packet_completion_context_t context);

/*!***************************************************************
 * fbe_lun_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the lun object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_lun_control_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_t * lun_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    if(object_handle == NULL){
        return  fbe_lun_class_control_entry(packet_p);
    }

    lun_p = (fbe_lun_t *)fbe_base_handle_to_pointer(object_handle);

    payload_p = fbe_transport_get_payload_ex(packet_p);

    control_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode)
    {
        // lun specific handling here.
        case FBE_LUN_CONTROL_CODE_SET_CONFIGURATION:
            status = fbe_lun_set_configuration(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_INITIATE_VERIFY:
            status = fbe_lun_usurper_initiate_verify(lun_p, packet_p);
            break;
        
        case FBE_LUN_CONTROL_CODE_INITIATE_ZERO:
            status = fbe_lun_usurper_initiate_zero(lun_p, packet_p);
            break;
            
        case FBE_LUN_CONTROL_CODE_INITIATE_HARD_ZERO:
            status = fbe_lun_usurper_initiate_hard_zero(lun_p, packet_p);
            break;
            
        case FBE_LUN_CONTROL_CODE_SET_NONPAGED_GENERATION_NUM:
            status = fbe_lun_usurper_set_gen_num(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_VERIFY_STATUS:
            status = fbe_lun_usurper_get_verify_status(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_VERIFY_REPORT:
            status = fbe_lun_usurper_get_verify_report(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_CLEAR_VERIFY_REPORT:
            status = fbe_lun_usurper_clear_verify_report(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_SET_POWER_SAVE_INFO:
            status = fbe_lun_usurper_set_power_save_info(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_TRESPASS_OP:
            status = fbe_lun_usurper_trespass_op(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_PREPARE_TO_DESTROY_LUN:
            status = fbe_lun_usurper_prepare_to_destroy_lun(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY:
            status = fbe_lun_usurper_set_power_saving_policy(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_POWER_SAVING_POLICY:
            status = fbe_lun_usurper_get_power_saving_policy(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_ZERO_STATUS:
            status = fbe_lun_usurper_get_zero_status(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_ENABLE_PERFORMANCE_STATS:
            status = fbe_lun_usurper_enable_performace_statistics(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_DISABLE_PERFORMANCE_STATS:
            status = fbe_lun_usurper_disable_performace_statistics(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_PERFORMANCE_STATS:
            status = fbe_lun_usurper_get_performace_statistics(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_ATTRIBUTES:
            status = fbe_lun_usurper_get_attributes(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_LUN_INFO:
            status = fbe_lun_usurper_get_lun_info(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_EXPORT_DEVICE:
            status = fbe_lun_usurper_export_device(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_ENABLE_WRITE_BYPASS:
            status = fbe_lun_usurper_set_write_bypass(lun_p, packet_p, FBE_TRUE);
            break;

        case FBE_LUN_CONTROL_CODE_DISABLE_WRITE_BYPASS:
            status = fbe_lun_usurper_set_write_bypass(lun_p, packet_p, FBE_FALSE);
            break;
            
        /* This is for fixing object which is stuck in SPECIALIZED when NP load failed */
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED:
            status = fbe_lun_metadata_set_nonpaged_metadata(lun_p, packet_p);
            break;
        /* This is for LUN object perform NP write/verify when do metadata raw mirror rebuild
             system Lun object may not have NP lock, so it need to write/verify its NP without NP lock
             Other system objects can call the common util in base config control entry to 
             perform NP write/verify with NP lock. So only Lun object has its own  
             usurper control entry*/
        case FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_WRITE_VERIFY:
            status = fbe_base_config_write_verify_non_paged_metadata_without_NP_lock((fbe_base_config_t*)lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_GET_REBUILD_STATUS:
            status = fbe_lun_usurper_get_rebuild_status(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_CLEAR_UNEXPECTED_ERROR_INFO:
            status = fbe_lun_reset_unexpected_error_info(lun_p, packet_p);
            break;

        case FBE_LUN_CONTROL_CODE_ENABLE_FAIL_ON_UNEXPECTED_ERROR:
            status = fbe_lun_enable_fail_on_unexpected_errors(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_DISABLE_FAIL_ON_UNEXPECTED_ERROR:
            status = fbe_lun_disable_fail_on_unexpected_errors(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_PREPARE_FOR_POWER_SHUTDOWN:
            status = fbe_lun_prepare_for_power_shutdown(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_SET_READ_AND_PIN_INDEX:
            status = fbe_lun_set_read_and_pin_index(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_ENABLE_IO_LOOPBACK:
            status = fbe_lun_enable_io_loopback(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_DISABLE_IO_LOOPBACK:
            status = fbe_lun_disable_io_loopback(lun_p, packet_p);
            break;
        case FBE_LUN_CONTROL_CODE_GET_IO_LOOPBACK:
            status = fbe_lun_get_io_loopback(lun_p, packet_p);
            break;
        default:
            // Allow the object to handle all other ioctls.
            status = fbe_base_config_control_entry(object_handle, packet_p);

            /* If Traverse status is returned and Lun is the most derived
             * class then we have no derived class to handle Traverse status.
             * Complete the packet with Error.
             */
            if((FBE_STATUS_TRAVERSE == status) &&
               (FBE_CLASS_ID_LUN == fbe_base_object_get_class_id(object_handle)))
            {
                fbe_base_object_trace((fbe_base_object_t*) lun_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived LUN class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

            return status;
    }
    return status;
}
/* end fbe_lun_control_entry() */

/*!**************************************************************
 * fbe_lun_set_configuration()
 ****************************************************************
 * @brief
 *  This function sets up the basic configuration info
 *  for this lun object.
 *
 * @param lun_p - The lun.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/26/2009 - Created. rfoley
 *
 ****************************************************************/
static fbe_status_t
fbe_lun_set_configuration(fbe_lun_t * lun_p, fbe_packet_t * in_packet_p)
{
    fbe_database_lun_configuration_t * set_config_p = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;  

    
    FBE_LUN_TRACE_FUNC_ENTRY(lun_p);

    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_lun_usurper_get_control_buffer(lun_p, in_packet_p,
                                       sizeof(fbe_database_lun_configuration_t),
                                       (fbe_payload_control_buffer_t *)&set_config_p);
    if (set_config_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)lun_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_lun_lock(lun_p);        
        fbe_lun_set_capacity(lun_p, set_config_p->capacity);
        fbe_lun_set_ndb_b(lun_p, ((set_config_p->config_flags & FBE_LUN_CONFIG_NO_USER_ZERO)==FBE_LUN_CONFIG_NO_USER_ZERO));

        /* set initial verify status */
        fbe_lun_set_noinitialverify_b(lun_p, ((set_config_p->config_flags & FBE_LUN_CONFIG_NO_INITIAL_VERIFY)==FBE_LUN_CONFIG_NO_INITIAL_VERIFY));
        
        /* set clear_need_zero*/
        fbe_lun_set_clear_need_zero_b(lun_p, ((set_config_p->config_flags & FBE_LUN_CONFIG_CLEAR_NEED_ZERO)==FBE_LUN_CONFIG_CLEAR_NEED_ZERO));
        
        /* set export_lun*/
        fbe_lun_set_export_lun_b(lun_p, ((set_config_p->config_flags & FBE_LUN_CONFIG_EXPROT_LUN)==FBE_LUN_CONFIG_EXPROT_LUN));
        
        /* Set the generation number. */
        fbe_base_config_set_generation_num((fbe_base_config_t*)lun_p, set_config_p->generation_number);
        
        fbe_lun_unlock(lun_p);

        fbe_lun_initialize_power_saving(lun_p, set_config_p);/*does not have to be under lock*/

    }

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}
/**************************************
 * end fbe_lun_set_configuration()
 **************************************/

/*!**************************************************************
 * fbe_lun_usurper_initiate_verify()
 ****************************************************************
 * @brief
 *  This function sets up a verify operation for this lun object.
 *
 * @param in_lun_p      - The lun.
 * @param in_packet_p   - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/19/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_initiate_verify(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_lun_initiate_verify_t*          initiate_verify_p = NULL;
    fbe_lun_verify_report_t*            verify_report_p = NULL;
    fbe_status_t                        status;
    fbe_lba_t                           lun_block_count;
    fbe_lba_t                           lun_start_lba;
    fbe_object_id_t                     object_id;


    FBE_LUN_TRACE_FUNC_ENTRY(in_lun_p);

    // Get a pointer to the verify report
    verify_report_p = fbe_lun_get_verify_report_ptr(in_lun_p);

    // Copy the current pass data to the previous pass data
    verify_report_p->previous = verify_report_p->current;

    // Clear the current pass data
    fbe_lun_usurper_clear_verify_result_counts(&verify_report_p->current);

    // Clear the current mirror object counts
    fbe_lun_usurper_clear_mirror_objects_count(&verify_report_p->mirror_objects_count);

    // Get the control buffer pointer from the packet payload.
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                            sizeof(fbe_lun_initiate_verify_t),
                                            (fbe_payload_control_buffer_t*)&initiate_verify_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }
    
    // Get the start lba of the lun in the rg
    fbe_lun_get_offset(in_lun_p, &lun_start_lba);

    // This is an imported capacity of the lun
    fbe_lun_get_imported_capacity(in_lun_p, &lun_block_count);

    /* Validate the request.
     */
    if ((initiate_verify_p->verify_type < FBE_LUN_VERIFY_TYPE_USER_RW) ||
        (initiate_verify_p->verify_type > FBE_LUN_VERIFY_TYPE_SYSTEM) )
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Invalid verify_type: %d\n",
                              __FUNCTION__, initiate_verify_p->verify_type);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* If we were request to verify the entire range of the lun, do so.
     */
    if (initiate_verify_p->b_verify_entire_lun == FBE_TRUE)
    {
        /* Ignore the start_lba and blocks from the request.
         */
        initiate_verify_p->start_lba    = 0;
        initiate_verify_p->block_count  = lun_block_count;
    }
    else
    {
        /* Else validate the request
         */
        if ((initiate_verify_p->start_lba + initiate_verify_p->block_count) > lun_block_count)
        {
            fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Start lba: 0x%llx and/or block_count: 0x%llx is greater than capacity: 0x%llx\n",
                                  __FUNCTION__,
                  (unsigned long long)initiate_verify_p->start_lba,
                  (unsigned long long)initiate_verify_p->block_count,
                  (unsigned long long)lun_block_count);
            fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(in_packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Don't modify start_lba or block_count
         */
    }
    /* If there is nothing in progress already, then simply queue it to the monitor. 
     * This speeds up the verify completion for large configs. 
     */
    if (in_lun_p->lun_verify.verify_type == FBE_LUN_VERIFY_TYPE_NONE){
        fbe_copy_memory(&in_lun_p->lun_verify, initiate_verify_p, sizeof(fbe_lun_initiate_verify_t));
    
        fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const,(fbe_base_object_t*)in_lun_p, FBE_LUN_LIFECYCLE_COND_VERIFY_LUN);

        fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(in_packet_p);

        status = FBE_STATUS_OK;
    }
    else {
        /* Otherwise do this operation inline so it does not get lost, we can only queue one 
         * verify to the monitor at a time. If we get more than one we'll perform it inline.
         */
        /* Treat this like a monitor op so it doesn't get queued if the lun has failed.
         */
        fbe_base_object_get_object_id((fbe_base_object_t*)in_lun_p, &object_id);
        fbe_transport_set_packet_attr(in_packet_p, FBE_PACKET_FLAG_MONITOR_OP);        
        fbe_transport_set_monitor_object_id(in_packet_p, object_id);

        /* Tell the RAID object to initialize its verify data for this lun.
         */
        status = fbe_lun_usurper_send_initiate_verify_to_raid(in_lun_p, in_packet_p, initiate_verify_p);
    }

    return status;
}
/* end of fbe_lun_usurper_initiate_verify() */

/*!**************************************************************
 * fbe_lun_usurper_set_gen_num()
 ****************************************************************
 * @brief
 *  
 *
 * @param in_lun_p        - The lun.
 * @param in_packet_p   - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/27/2012 - Created. He Wei
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_set_gen_num(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t                status;
    fbe_u64_t                   gen_num_offset;

    fbe_config_generation_t*    p_gen_num;
    fbe_config_generation_t     generation_number;
    /*
     * Get the control buffer pointer from the packet payload.
     */
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                                sizeof(fbe_config_generation_t),
                                                (fbe_payload_control_buffer_t*)&p_gen_num);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    generation_number = *p_gen_num;
    fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s change generation number to %llu.", __FUNCTION__, 
                          (unsigned long long)generation_number);

    gen_num_offset = (fbe_u64_t)(&((fbe_lun_nonpaged_metadata_t*)0)->base_config_nonpaged_metadata.generation_number);
    status = fbe_lun_metadata_nonpaged_set_generation_num((fbe_base_config_t *) in_lun_p,
                                                          in_packet_p,
                                                          gen_num_offset,
                                                          (fbe_u8_t *)&generation_number,
                                                          sizeof(generation_number));
    return status;
}
/* end of fbe_lun_usurper_set_gen_num() */


/*!**************************************************************
 * fbe_lun_usurper_initiate_zero()
 ****************************************************************
 * @brief
 *  This function sets up a zero operation for this lun object.
 *
 * @param in_lun_p          - The lun.
 * @param in_packet_p     - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/05/2012 - Created. He Wei
 *  16/07/2012 - Modified. Jingcheng Zhang for force zero LUN and ignore the ndb_b flag, 
 *                         it is to clear database LUN and restore configuration
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_initiate_zero(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t    status;
    fbe_lun_init_zero_t *init_lun_zero_p = NULL;


    /* get init zero request parameter from control buffer. */
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                                sizeof(fbe_lun_init_zero_t),
                                                (fbe_payload_control_buffer_t *)&init_lun_zero_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s : fail get control buffer \n", __FUNCTION__);
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    /* If ndb_b flag is TRUE then bypass lun zeroing operation;
       but if user specify "force_zero" in fbe_lun_init_zero_t, ignore
       the ndb_b flag to send zero request
     */
    if(in_lun_p->ndb_b == FBE_TRUE && !init_lun_zero_p->force_init_zero) 
    {
        fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s : NDB flag status: %u \n", __FUNCTION__, in_lun_p->ndb_b);
        
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the completion function before we initialize the lun zero. */
    fbe_transport_set_completion_function(in_packet_p, fbe_lun_usurper_initiate_zero_completion, in_lun_p);

    /* initialize lun zero. */ 
    status = fbe_lun_zero_start_lun_zeroing_packet_allocation(in_lun_p,in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }
    
    return status;
}
/* end of fbe_lun_usurper_initiate_zero() */

/*!**************************************************************
 * fbe_lun_usurper_initiate_zero_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the LUN zero
 *  condition.
 *
 * @param packet_p      - pointer to a control packet.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  1/20/2012 - Created. He, Wei
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_initiate_zero_completion(fbe_packet_t * packet_p,
                                                                           fbe_packet_completion_context_t context)
{
    fbe_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s lun zero failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
        return status;
    }
    
    return FBE_STATUS_OK;
}   
/* End fbe_lun_usurper_initiate_zero_completion()*/

fbe_status_t 
fbe_lun_usurper_send_initiate_verify_to_raid_completion(fbe_packet_t * packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t pkt_status;
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qual;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p); 
     
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);
    pkt_status = fbe_transport_get_status_code(packet_p);

    fbe_base_object_trace((fbe_base_object_t*) context, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lun initiate vr compl lba:0x%llx bl:0x%llx pk_st: 0x%x blk_st:0x%x bl_q:0x%x\n",
                          (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count, pkt_status, block_status, block_qual);    

    if(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS == block_status)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    }
    else
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    return pkt_status;
}

/*!**************************************************************
 * fbe_lun_usurper_send_initiate_verify_to_raid()
 ****************************************************************
 * @brief
 *  This function allocates memory to send initiate verify to raid
 *  It allocates 2 chunk one for the raid initiate verify buffer
 *  and another chunk for the packet. Whenever a memory request
 *  is being made with the packet, same packet should not be 
 *  sent down since somether service or object might try
 *  to allocate memory and the memory allocation will fail
 *  since the memory is in use by the one that allocated it 
 *
 * @param in_lun_p                  - The lun object pointer.
 * @param in_packet_p               - The packet requesting this operation.
 * @param in_initiate_lun_verify_p  - Pointer to the LUN verify command data.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/20/2009 - Created. mvalois
 *
 ****************************************************************/
fbe_status_t fbe_lun_usurper_send_initiate_verify_to_raid(fbe_lun_t * lun_p, 
                                                          fbe_packet_t * packet_p, 
                                                          fbe_lun_initiate_verify_t * initiate_lun_verify_p)
{
    fbe_payload_ex_t*                       payload_p = NULL;    
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_block_edge_t *                      block_edge_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_status_t                            status;

    FBE_LUN_TRACE_FUNC_ENTRY(lun_p);

    switch(initiate_lun_verify_p->verify_type)
    {
        case FBE_LUN_VERIFY_TYPE_USER_RO:
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY;
            break;
        case FBE_LUN_VERIFY_TYPE_USER_RW:
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY;
            break;
        case FBE_LUN_VERIFY_TYPE_SYSTEM:
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY;
            break;
        case FBE_LUN_VERIFY_TYPE_ERROR:
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY;
            break;
        case FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE:
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY;
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)lun_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Invalid verify type %d", __FUNCTION__, initiate_lun_verify_p->verify_type);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_operation_p   = fbe_payload_ex_allocate_block_operation(payload_p);
    
    fbe_lun_get_block_edge(lun_p, &block_edge_p);

    fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: area: lba:0x%llx, blk_count:0x%llx, pkt:0x%p\n",
                          __FUNCTION__,
                          (unsigned long long)initiate_lun_verify_p->start_lba, (unsigned long long)initiate_lun_verify_p->block_count, packet_p);

    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(&lun_p->block_edge, &optimum_block_size);

    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(block_operation_p,
                                      block_opcode,
                                      initiate_lun_verify_p->start_lba,
                                      initiate_lun_verify_p->block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    fbe_transport_set_completion_function(packet_p, fbe_lun_usurper_send_initiate_verify_to_raid_completion, lun_p);

    fbe_payload_ex_increment_block_operation_level(payload_p);
        
    /* invoke bouncer to forward i/o request to downstream object */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)lun_p, packet_p);
    if(status == FBE_STATUS_OK){ /* Small stack */
        status = fbe_lun_send_io_packet(lun_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;

}   // End fbe_lun_usurper_send_initiate_verify_to_raid()

/*!**************************************************************
 * fbe_lun_usurper_get_verify_status()
 ****************************************************************
 * @brief
 *  This function gets the verify status information from the
 *  raid object for this lun object's extent.  It copies the
 *  status from RAID to the LUN.
 *
 *  The verify status provides chunk count information that may
 *  be used to determine the progress of a verify operation.
 *
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/19/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_verify_status(fbe_lun_t* in_lun_p,fbe_packet_t* in_packet_p)
{
    fbe_payload_ex_t*                      payload_p;
    fbe_payload_control_operation_t*        control_operation_p;
    fbe_block_edge_t*                       edge_p;
    fbe_semaphore_t                         semaphore;
    fbe_lun_get_verify_status_t*            lun_verify_status_p;
    fbe_raid_group_get_lun_verify_status_t  raid_verify_status = {0};
    fbe_status_t                            status;


    FBE_LUN_TRACE_FUNC_ENTRY(in_lun_p);

    // Allocate and build a control operation to get the status of the raid verify operation.
    status = fbe_lun_usurper_allocate_and_build_control_operation(in_lun_p, in_packet_p, 
                                                FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_VERIFY_STATUS,
                                                &raid_verify_status,
                                                sizeof(fbe_raid_group_get_lun_verify_status_t));

    // Initialize a semaphore to trigger on the count of 1
    //! @todo: consider an alternative to using a semaphore here.
    fbe_semaphore_init(&semaphore, 0, 1);

    // Set the completion function on the stack.
    status = fbe_transport_set_completion_function(in_packet_p, fbe_lun_usurper_get_verify_status_completion, &semaphore);

    // Send the packet to the RAID object on this LUN's lower edge and wait for it.
    fbe_base_config_get_block_edge((fbe_base_config_t*)in_lun_p, &edge_p, 0);
    fbe_block_transport_send_control_packet(edge_p, in_packet_p);
    fbe_semaphore_wait(&semaphore, NULL);

    // The RAID object has returned the verify status information.
    // Release the RAID payload and the semaphore.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL control op pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_semaphore_destroy(&semaphore);

    // The current payload is now the LUN's payload.
    // Get the pointer to the LUN verify status data.
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL control op pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    // Get the control buffer pointer from the packet payload.
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p,
                                                sizeof(fbe_lun_get_verify_status_t),
                                                (fbe_payload_control_buffer_t)&lun_verify_status_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }


    // Copy raid verify status info into the LUN verify status.
    lun_verify_status_p->total_chunks = raid_verify_status.total_chunks;
    lun_verify_status_p->marked_chunks = raid_verify_status.marked_chunks;

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}   // End fbe_lun_usurper_get_verify_status()


/*!**************************************************************
 * fbe_lun_usurper_get_verify_status_completion()
 ****************************************************************
 * @brief
 *  This function processes the packet completion for a get
 *  verify status operation.  It releases the semaphore for the
 *  "get vefify status" control op that was sent to the raid 
 *  object.
 *
 * @param in_packet_p  - The packet requesting this operation.
 * @param in_context   - The address of the semaphore.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/20/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_verify_status_completion(fbe_packet_t* in_packet_p, 
                                                                 fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t*    semaphore_p;

    semaphore_p = (fbe_semaphore_t*)in_context;
    fbe_semaphore_release(semaphore_p, 0, 1, FALSE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // End fbe_lun_usurper_get_verify_status_completion()

/*!**************************************************************
 * fbe_lun_usurper_get_verify_report()
 ****************************************************************
 * @brief
 *  This function gets the verify report. The verify report is a
 *  collection of RAID stripe error count information along with
 *  other pertinent information.
 *
 * @param in_lun_p      - The lun.
 * @param in_packet_p   - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/19/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_verify_report(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_lun_verify_report_t*            req_buffer_p;
    fbe_status_t                        status;
    fbe_lun_verify_report_t *			lu_report;

    FBE_LUN_TRACE_FUNC_ENTRY(in_lun_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                            sizeof(fbe_lun_verify_report_t),
                                            (fbe_payload_control_buffer_t)&req_buffer_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // Copy the the LUN verify report data into the requester's buffer.
    lu_report = fbe_lun_get_verify_report_ptr(in_lun_p);
    fbe_copy_memory(req_buffer_p, lu_report, sizeof(fbe_lun_verify_report_t));
    
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
} // End fbe_lun_usurper_get_verify_report()



/*!**************************************************************
 * fbe_lun_usurper_clear_verify_report()
 ****************************************************************
 * @brief
 *  This function clears the LUN verify report data.
 *
 * @param in_lun_p      - The lun.
 * @param in_packet_p   - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/06/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_clear_verify_report(fbe_lun_t*      in_lun_p, 
                                                        fbe_packet_t*   in_packet_p)
{
    fbe_lun_verify_report_t*    verify_report_p;
    fbe_status_t                        status = FBE_STATUS_OK;

    FBE_LUN_TRACE_FUNC_ENTRY(in_lun_p);

    // Get a pointer to the verify report
    verify_report_p = fbe_lun_get_verify_report_ptr(in_lun_p);

    // Clear the pass count
    verify_report_p->pass_count = 0;

    verify_report_p->mirror_objects_count = 0;

    // Initialize the current result counters
    fbe_lun_usurper_clear_verify_result_counts(&verify_report_p->current);
    fbe_lun_usurper_clear_verify_result_counts(&verify_report_p->previous);
    fbe_lun_usurper_clear_verify_result_counts(&verify_report_p->totals);

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
} // End fbe_lun_usurper_clear_verify_report()


/*!**************************************************************
 * fbe_lun_usurper_clear_verify_result_counts()
 ****************************************************************
 * @brief
 *  This function clears the raid error counters in the specified
 *  verify results.
 *
 * @param in_results_p  - Pointer to the verify results
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/20/2009 - Created. mvalois
 *
 ****************************************************************/
static void fbe_lun_usurper_clear_verify_result_counts(fbe_lun_verify_counts_t* in_results_p)
{
    // Clear the error counts
    fbe_zero_memory(in_results_p, sizeof(fbe_lun_verify_counts_t));

    return;

}   // End fbe_lun_usurper_clear_verify_result_counts()



/*!**************************************************************
 * fbe_lun_usurper_clear_mirror_objects_count()
 ****************************************************************
 * @brief
 *  This function clears the mirror objects counts
 *
 * @param mirror_objects_count_p  
 *
 
 *
 * @author
 *  1/29/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static void fbe_lun_usurper_clear_mirror_objects_count(fbe_u32_t* mirror_objects_count_p)
{
    // Clear the mirror objects count
    fbe_zero_memory(mirror_objects_count_p, sizeof(fbe_u32_t));
    return;

}   // End fbe_lun_usurper_clear_mirror_objects_count()

/*!**************************************************************
 * fbe_lun_usurper_add_verify_result_counts()
 ****************************************************************
 * @brief
 *  This function adds the raid error counts to the lun verify 
 *  results counts.
 *
 * @param in_raid_verify_error_counts_p - Pointer to the raid verify counters
 * @param in_lun_verify_counts_p        - Pointer to the lun verify results
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/20/2009 - Created. mvalois
 *
 ****************************************************************/
void fbe_lun_usurper_add_verify_result_counts(
                                    fbe_raid_verify_error_counts_t* in_raid_verify_error_counts_p,
                                    fbe_lun_verify_counts_t*        in_lun_verify_counts_p)
{
    // Add the error counts from one set to another set
    in_lun_verify_counts_p->uncorrectable_multi_bit_crc += in_raid_verify_error_counts_p->u_crc_count;
    in_lun_verify_counts_p->correctable_multi_bit_crc += in_raid_verify_error_counts_p->c_crc_count;
    in_lun_verify_counts_p->uncorrectable_coherency += in_raid_verify_error_counts_p->u_coh_count;
    in_lun_verify_counts_p->correctable_coherency += in_raid_verify_error_counts_p->c_coh_count;
    in_lun_verify_counts_p->uncorrectable_time_stamp += in_raid_verify_error_counts_p->u_ts_count;
    in_lun_verify_counts_p->correctable_time_stamp += in_raid_verify_error_counts_p->c_ts_count;
    in_lun_verify_counts_p->uncorrectable_write_stamp += in_raid_verify_error_counts_p->u_ws_count;
    in_lun_verify_counts_p->correctable_write_stamp += in_raid_verify_error_counts_p->c_ws_count;
    in_lun_verify_counts_p->uncorrectable_shed_stamp += in_raid_verify_error_counts_p->u_ss_count;
    in_lun_verify_counts_p->correctable_shed_stamp += in_raid_verify_error_counts_p->c_ss_count;
    in_lun_verify_counts_p->uncorrectable_lba_stamp += in_raid_verify_error_counts_p->u_ls_count;
    in_lun_verify_counts_p->correctable_lba_stamp += in_raid_verify_error_counts_p->c_ls_count;
    in_lun_verify_counts_p->uncorrectable_media += in_raid_verify_error_counts_p->u_media_count;
    in_lun_verify_counts_p->correctable_media += in_raid_verify_error_counts_p->c_media_count;
    in_lun_verify_counts_p->correctable_soft_media += in_raid_verify_error_counts_p->c_soft_media_count;

    return;

}   // End fbe_lun_usurper_add_verify_result_counts()



/*!**************************************************************
 * fbe_lun_usurper_set_power_save_info()
 ****************************************************************
 * @brief
 *  This function gets the LUN information. It also sends
 *  usurper command to RAID to get RAID specific information for
 *  this LUN.
 *
 *  
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/17/2009 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_set_power_save_info(fbe_lun_t* in_lun_p,
                                                        fbe_packet_t* in_packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    /*! @todo: No support needed for now because we dont tie this as
     *  usurper command currently
     */
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}   // End fbe_lun_usurper_set_power_save_info()

/*!**************************************************************
 * fbe_lun_usurper_trespass_op()
 ****************************************************************
 * @brief
 *  This function handle the trespass operation received
 *
 *  
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/17/2009 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_trespass_op(fbe_lun_t* in_lun_p,
                                                fbe_packet_t* in_packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 

    /*! This is a no-op from SEP perspective so just return success
     */
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}   // End fbe_lun_usurper_trespass_op()


/*!**************************************************************
 * fbe_lun_usurper_prepare_to_destroy_lun()
 ****************************************************************
 * @brief
 *  This function handles the control code to prepare a LUN
 *  to be destroyed. This includes adding the packet to the
 *  usurper queue and draining any I/O in progress; new I/O
 *  requests sent to the LUN are blocked.  A condition is set 
 *  to check the flush status.
 * 
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/2010 - Created. rundbs
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_prepare_to_destroy_lun( fbe_lun_t*      in_lun_p, fbe_packet_t*   in_packet_p)
{
    fbe_bool_t is_empty;
    /* Flush I/O in progress on LUN and ensure new I/O is blocked */
    fbe_block_transport_server_flush_and_block_io_for_destroy(&in_lun_p->base_config.block_transport_server);

    fbe_block_transport_is_empty(&in_lun_p->base_config.block_transport_server, &is_empty);

    if(is_empty == FBE_TRUE){
        fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    } else {
        /* Enqueue usurper packet */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)in_lun_p, in_packet_p);

        /* Set condition to check flush status; when flushing complete, monitor will dequeue 
         * usurper packet
         */
        fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const, (fbe_base_object_t*) in_lun_p,
                               FBE_LUN_LIFECYCLE_COND_LUN_CHECK_FLUSH_FOR_DESTROY);
    }

    return FBE_STATUS_PENDING;

}   // End fbe_lun_usurper_prepare_to_destroy_lun()

/*!**************************************************************
 * fbe_lun_usurper_get_control_buffer()
 ****************************************************************
 * @brief
 *  This function gets the buffer pointer out of the packet
 *  and validates it. It returns error status if the buffer 
 *  pointer is not NULL or the buffer size doesn't match the
 *  specifed buffer length.
 *
 * @param in_lun_p          - Pointer to the lun.
 * @param in_packet_p       - Pointer to the packet.
 * @param in_buffer_length  - Expected length of the buffer.
 * @param out_buffer_pp     - Pointer to the buffer pointer. 
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  10/30/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_control_buffer(
                                    fbe_lun_t*                          in_lun_p,
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_payload_control_buffer_length_t in_buffer_length,
                                    fbe_payload_control_buffer_t*       out_buffer_pp)
{
    fbe_payload_ex_t*                  payload_p;
    fbe_payload_control_operation_t*    control_operation_p;  
    fbe_payload_control_buffer_length_t buffer_length;


    *out_buffer_pp = NULL;
    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if(control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL control op pointer line %d", __FUNCTION__, __LINE__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        *out_buffer_pp = NULL;
        // The buffer length doesn't match!
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length mismatch\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}   // End fbe_lun_usurper_get_control_buffer()


/*!**************************************************************
 * fbe_lun_usurper_build_control_packet()
 ****************************************************************
 * @brief
 *  This function builds a control operation in the packet.
 *  It returns error status if the payload pointer is invalid or 
 *  a control operation cannot be allocated.
 *
 * @param in_lun_p          - Pointer to the lun.
 * @param in_packet_p       - Pointer to the packet.
 * @param in_op_code        - Operation code
 * @param in_buffer_p       - Pointer to the buffer. 
 * @param in_buffer_length  - Expected length of the buffer.
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  10/30/2009 - Created. mvalois
 *
 ****************************************************************/
fbe_status_t fbe_lun_usurper_allocate_and_build_control_operation(
                                fbe_lun_t*                              in_lun_p,
                                fbe_packet_t*                           in_packet_p,
                                fbe_payload_control_operation_opcode_t  in_op_code,
                                fbe_payload_control_buffer_t            in_buffer_p,
                                fbe_payload_control_buffer_length_t     in_buffer_length)
{
    fbe_payload_ex_t*                  payload_p;
    fbe_payload_control_operation_t*    control_operation_p;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL control op pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    // Build the control operation.
    fbe_payload_control_build_operation(control_operation_p, in_op_code, in_buffer_p, in_buffer_length);

    return FBE_STATUS_OK;

}   // End fbe_lun_usurper_allocate_and_build_control_operation()


static fbe_status_t fbe_lun_usurper_set_power_saving_policy( fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_set_power_saving_policy_t * policy = NULL;
    
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                            sizeof(fbe_lun_set_power_saving_policy_t),
                                            (fbe_payload_control_buffer_t*)&policy);
    if (status != FBE_STATUS_OK){
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    /*some sanity check to make sure the user is not placing a value which is too big for us to process*/
    if ((policy->lun_delay_before_hibernate_in_sec) > 0xFFFFFFFF ||
        (policy->max_latency_allowed_in_100ns / 10000000) > 0xFFFFFFFF) {

        fbe_base_object_trace((fbe_base_object_t *)in_lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Invaid values passed in\n", __FUNCTION__);

        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;

    }

    /*upper layers set values in 100ns so we need to convert it to seconds*/
    in_lun_p->power_save_io_drain_delay_in_sec = policy->lun_delay_before_hibernate_in_sec;
    in_lun_p->max_lun_latency_time_is_sec = (fbe_u64_t)(policy->max_latency_allowed_in_100ns / 10000000);

    
    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);
    return FBE_STATUS_OK;

}

static fbe_status_t
fbe_lun_initialize_power_saving(fbe_lun_t *lun_p,
                                fbe_database_lun_configuration_t * set_config_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    fbe_lun_set_power_save_info(lun_p, set_config_p->power_save_io_drain_delay_in_sec, set_config_p->max_lun_latency_time_is_sec);
    fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)lun_p, set_config_p->power_saving_idle_time_in_seconds);
    if (!set_config_p->power_saving_enabled) {
        status = base_config_disable_object_power_save((fbe_base_config_t *)lun_p);
    }else{
        status = fbe_base_config_enable_object_power_save((fbe_base_config_t *)lun_p);
    }

    return status;
}

static fbe_status_t fbe_lun_usurper_get_power_saving_policy( fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_set_power_saving_policy_t * policy = NULL;
    fbe_u64_t                           idle_time = 0;

    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                                sizeof(fbe_lun_set_power_saving_policy_t),
                                                (fbe_payload_control_buffer_t*)&policy);
    if (status != FBE_STATUS_OK){
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }
    
    /*upper layers set values in 100ns so we need to convert it to 100ns*/
    policy->lun_delay_before_hibernate_in_sec = in_lun_p->power_save_io_drain_delay_in_sec;
    policy->max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)in_lun_p->max_lun_latency_time_is_sec * (fbe_time_t)10000000);
    fbe_base_config_get_power_saving_idle_time(&in_lun_p->base_config, &idle_time);
    
    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_lun_usurper_get_zero_status()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero percentage for the lun extent.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  10/13/2010      - Created. Dhaval Patel
 *  07/02/2012      - Modified Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_usurper_get_zero_status(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                        status;
    fbe_lun_get_zero_status_t *                         zero_status_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_lba_t                                           current_zero_checkpoint;

    /* get the zero percentage buffer from the packet. */
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_lun_get_zero_status_t),
                                                (fbe_payload_control_buffer_t *)&zero_status_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* first determine lun zero checkpoint stored in nonpaged metadata. 
     */
    status = fbe_lun_metadata_get_zero_checkpoint(lun_p, &current_zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to get checkpoint. Status: 0x%X\n", __FUNCTION__, status);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }

    /* compute zeroing percentage using checkpoint. */
    fbe_lun_zero_calculate_zero_percent_for_lu_extent(lun_p,
                                                      current_zero_checkpoint,
                                                      &zero_status_p->zero_percentage);

    zero_status_p->zero_checkpoint = current_zero_checkpoint;
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_usurper_get_zero_status()
 ******************************************************************************/

/*!**************************************************************
 * fbe_lun_usurper_enable_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to enable performance statistics to 
 *  modify LUN specific counters incremented by LUN object or
 *  raid group object
 *
 *  
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *   4/24/2012 - Updated for compatibility with perfstats service. Ryan Gadsby
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_enable_performace_statistics(fbe_lun_t* in_lun_p,
                                                        fbe_packet_t* in_packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 

    if (in_lun_p->performance_stats.counter_ptr.lun_counters) //don't enable if we don't have valid  stat memory
    {
        in_lun_p->b_perf_stats_enabled = FBE_TRUE;
    }

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}
/******************************
 * end fbe_lun_usurper_enable_performace_statistics()
 ******************************/

/*!**************************************************************
 * fbe_lun_usurper_disable_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to disable performance statistics to not
 *  modify LUN specific counters incremented by LUN object or
 *  raid group object
 *
 *  
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2010 - Created. Swati Fursule
 *   4/24/2012 - Updated for compatibility with perfstats service. Ryan Gadsby
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_disable_performace_statistics(fbe_lun_t* in_lun_p,
                                                        fbe_packet_t* in_packet_p)
{
    fbe_status_t status = FBE_STATUS_OK; 
    in_lun_p->b_perf_stats_enabled = FBE_FALSE;
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}


/*!**************************************************************
 * fbe_lun_usurper_get_performace_statistics()
 ****************************************************************
 * @brief
 *  This function is used to directly copy the performance statistics of 
 * the LUN to a different area of memory. This function should only be used 
 * for test purposes due to the performance overhead of copying all this memory! 
 *
 *  If you need to reference counter memory in a non-test environment, use the perfstats service!
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/11/2012 - Created. Ryan Gadsby
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_performace_statistics(fbe_lun_t *in_lun_p,
                                                 fbe_packet_t *in_packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_lun_performance_counters_t              *perf_counters = NULL;

    if (in_lun_p->b_perf_stats_enabled == FBE_FALSE)
    {
        status =  FBE_STATUS_GENERIC_FAILURE;
    }
    else
    { //copy the stat memory
        fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                           sizeof(fbe_lun_performance_counters_t),
                                           (fbe_payload_control_buffer_t *)&perf_counters);
        if (perf_counters == NULL || in_lun_p->performance_stats.counter_ptr.lun_counters == NULL) 
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            memcpy(perf_counters, in_lun_p->performance_stats.counter_ptr.lun_counters, sizeof(fbe_lun_performance_counters_t));
        }
    }
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;

}
/******************************
 * end fbe_lun_usurper_get_performace_statistics()
 ******************************/

/*!**************************************************************
 * fbe_lun_usurper_get_attributes()
 ****************************************************************
 * @brief
 *  get attributes bitmap associated with the LUN
 *
 *  
 * @param in_lun_p    - The lun.
 * @param in_packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_get_attributes(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_lun_get_attributes_t*   req_buffer_p;
    fbe_status_t                status;
    /*
     * Get the control buffer pointer from the packet payload.
     */
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                                sizeof(fbe_lun_get_attributes_t),
                                                (fbe_payload_control_buffer_t*)&req_buffer_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    req_buffer_p->lun_attr = in_lun_p->lun_attributes;

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;

}
/******************************
 * end fbe_lun_usurper_get_attributes()
 ******************************/
/*!****************************************************************************
 * fbe_lun_usurper_get_lun_info()
 ******************************************************************************
 * @brief
 *  This function is used to get the information for the lun extent.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  04/25/2011      - Created. NCHIU
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_usurper_get_lun_info(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                        status;
    fbe_lun_get_lun_info_t                              *get_lun_info = NULL;
    fbe_lba_t               capacity;          /*!< LU Capacity */    
    fbe_lba_t               offset;

    /* get the zero percentage buffer from the packet. */
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_lun_get_lun_info_t),
                                                (fbe_payload_control_buffer_t *)&get_lun_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* we should consider a getter function */
    get_lun_info->peer_lifecycle_state = lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state;
    
   // Get the lun offset
    fbe_lun_get_offset(lun_p, &offset);
    get_lun_info->offset = offset;


    // This is an imported capacity of the lun
    fbe_lun_get_imported_capacity(lun_p, &capacity);
    get_lun_info->capacity = capacity;
    get_lun_info->b_initial_verify_already_run = fbe_lun_metadata_was_initial_verify_run(lun_p);
    get_lun_info->b_initial_verify_requested = !fbe_lun_get_noinitialverify_b(lun_p);
    fbe_transport_set_status(packet_p, status, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_usurper_get_lun_info()
 ******************************/

/*!****************************************************************************
 * fbe_lun_usurper_export_device(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
 ******************************************************************************
 * @brief
 *  Export the lun as a device to the OS
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 ******************************************************************************/
static fbe_status_t fbe_lun_usurper_export_device(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_block_edge_t *     block_edge_p = NULL;
    fbe_path_attr_t        attr = 0;

    /* Connect to the bvd */
    status = lun_connect_to_bvd(in_lun_p);

    /* Get the path attr from the edge */
    fbe_lun_get_block_edge(in_lun_p, &block_edge_p);
    fbe_block_transport_get_path_attributes(block_edge_p, &attr);

    /* Assign the attr to the LUN's prev path attr */
    in_lun_p->prev_path_attr = attr;

    /*After connect, propagate the path attribute to BVD*/
    fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&((fbe_base_config_t *)in_lun_p)->block_transport_server,
                                        attr,
                                        ~FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN);
    
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;

}

/****************************************************************
 *  lun_connect_to_bvd(fbe_lun_t *lun_p)
 ****************************************************************
 * @brief
 *  This function connects the LUN to the BVD object.
 *
 * @param lun_p - a pointer to lun
 *
 * @return
 *  fbe_status_t
 ****************************************************************/
static fbe_status_t lun_connect_to_bvd(fbe_lun_t *lun_p)
{
    fbe_status_t        status;
    fbe_base_config_t * base_config_p = (fbe_base_config_t *)lun_p;
    fbe_u32_t           clients = 0;
    fbe_bool_t          export_type = FBE_FALSE;
    fbe_object_id_t     object_id;
    fbe_lun_number_t    lun_number;
    fbe_base_object_t * object_p = (fbe_base_object_t *)lun_p;

    fbe_base_object_trace(object_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Entry\n",
                          __FUNCTION__);
    /* get number of clients */
    fbe_block_transport_server_get_number_of_clients(&base_config_p->block_transport_server, &clients);
    if (clients != 0) {
        /* if not 0, then lun and bvd already connected, clear the condition and we are done here */
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Lun is already connected to BVD interface. Clear the condition.\n",
                              __FUNCTION__);
        
        return FBE_STATUS_OK;
    }

    /* Get the the export type of this lun base on it's object id */
    fbe_base_object_get_object_id(object_p, &object_id);
    status = fbe_database_get_lun_export_type(object_id, &export_type);
    if (export_type == FBE_TRUE){
        status = fbe_database_get_lun_number(object_id, &lun_number);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: obj0x%x can't get lun number, status: 0x%X\n",
                                  __FUNCTION__,object_id,status);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* if type belongs to export, then connect the lun with bvd interface */
        status = fbe_lun_connect_bvd_interface(lun_number, lun_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't connect to BVD, status: 0x%X\n",
                                  __FUNCTION__,
                                  status);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    

    return FBE_STATUS_OK;
}

/****************************************************************
 *  fbe_lun_connect_bvd_interface
 ****************************************************************
 * @brief
 *  This function connects an edge between the lun and bvd interface,
 *  and register the lun with the bvd interface, so a windows device
 *  is created and available to upper drivers.
 *
 * @param lun_p - a pointer to lun
 * @param packet_p - 
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  05/03/2011 - Created. 
 ****************************************************************/
static fbe_status_t fbe_lun_connect_bvd_interface(fbe_lun_number_t lun_number, fbe_lun_t * const lun_p)
{
    fbe_packet_t *                         new_packet_p = NULL;    
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_operation_t *      control_operation = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_status_t		   control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_bvd_interface_register_lun_t       register_lun;

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: entry.\n", 
                          __FUNCTION__);

    /*set the usurper correctly*/
    /* register the lun */
    fbe_lun_get_capacity(lun_p, &register_lun.capacity);
    fbe_base_object_get_object_id((fbe_base_object_t *)lun_p, &register_lun.server_id);
    register_lun.lun_number = lun_number;
    register_lun.export_lun_b = fbe_lun_get_export_lun_b(lun_p);
    
    /*sets up the packet */
    new_packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet_p);
    payload = fbe_transport_get_payload_ex(new_packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if(control_operation == NULL) {    
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to allocate control operation\n",__FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
     
    fbe_payload_control_build_operation(control_operation,  
                                        FBE_BVD_INTERFACE_CONTROL_CODE_REGISTER_LUN,  
                                        &register_lun, 
                                        sizeof(fbe_bvd_interface_register_lun_t)); 
    fbe_transport_set_address(new_packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_BVD_INTERFACE,
                              FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE);
    
    fbe_transport_set_sync_completion_type(new_packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_transport_set_packet_attr(new_packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    
    status  = fbe_service_manager_send_control_packet(new_packet_p);  
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {    
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to send packet\n",__FUNCTION__);
    
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_transport_release_packet(new_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    
    fbe_transport_wait_completion(new_packet_p);
    
    status = fbe_transport_get_status_code(new_packet_p);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_transport_release_packet(new_packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_lun_usurper_set_write_bypass(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p, fbe_bool_t write_bypass_mode)
{
    fbe_status_t status = FBE_STATUS_OK; 

    in_lun_p->write_bypass_mode = write_bypass_mode;
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}

/*****************************************
 * end of fbe_lun_connect_bvd_interface()
 *****************************************/

/*!****************************************************************************
* fbe_lun_metadata_set_default_nonpaged_metadata()
******************************************************************************
* @brief
*   This function is used to set and persist the metadata with default values for 
*   the non paged metadata.
*
* @param lun_p		 - pointer to the provision drive
* @param packet_p 			- pointer to a monitor packet.
*
* @return  fbe_status_t  
*
* @author
*   05/15/2012 - Created. zhangy26
*
******************************************************************************/
fbe_status_t 
fbe_lun_metadata_set_nonpaged_metadata(fbe_lun_t * lun_p,
                                                      fbe_packet_t *packet_p)
{
    fbe_status_t				 status = FBE_STATUS_OK;
    fbe_lun_nonpaged_metadata_t lun_nonpaged_metadata;
    fbe_base_config_control_set_nonpaged_metadata_t * control_set_np_p = NULL;	
    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_control_operation_t*    control_operation_p;  
    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                                (fbe_payload_control_buffer_t *)&control_set_np_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }
    
      
    if(control_set_np_p->set_default_nonpaged_b)
    {
        
        /* We must zero the local structure since it will be used to set all the
        * default metadata values.
        */
        fbe_zero_memory(&lun_nonpaged_metadata, sizeof(fbe_lun_nonpaged_metadata_t));
         
        /* Now set the default non-paged metadata.
        */
        status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)lun_p,
                                                                    (fbe_base_config_nonpaged_metadata_t *)&lun_nonpaged_metadata);

        /* Force set it to initialized */
        fbe_base_config_nonpaged_metadata_set_np_state((fbe_base_config_nonpaged_metadata_t *)&lun_nonpaged_metadata, 
                                                   FBE_NONPAGED_METADATA_INITIALIZED);
        
        if (status != FBE_STATUS_OK)
        {
            /* Trace an error and return.
            */
            fbe_base_object_trace((fbe_base_object_t *)lun_p,
                                    FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Set default base config nonpaged failed status: 0x%x\n",
                                    __FUNCTION__, status);

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);

			return FBE_STATUS_GENERIC_FAILURE;

        }
         
        /* Now set the lun specific fields.
        */
        lun_nonpaged_metadata.zero_checkpoint = 0;
        lun_nonpaged_metadata.has_been_written = FBE_FALSE;
        lun_nonpaged_metadata.flags = FBE_LUN_NONPAGED_FLAGS_NONE;
            
         /* Send the nonpaged metadata write request to metadata service. 
        */
        fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) lun_p,
                                                         packet_p,
                                                         0,
                                                         (fbe_u8_t *) &lun_nonpaged_metadata, /* non paged metadata memory. */
                                                         sizeof(fbe_lun_nonpaged_metadata_t)); /* size of the non paged metadata. */
         
    }else{
         /* set the object NP according to caller input. 
        */
        fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) lun_p,
                                                          packet_p,
                                                          0,
                                                          control_set_np_p->np_record_data, /* non paged metadata memory. */
                                                          sizeof(fbe_lun_nonpaged_metadata_t)); /* size of the non paged metadata. */
         
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
* end fbe_lun_metadata_write_default_nonpaged_metadata()
******************************************************************************/

/*!****************************************************************************
 * fbe_lun_usurper_get_rebuild_status()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for the lun extent.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  06/11/2012      - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_usurper_get_rebuild_status(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_lun_rebuild_status_t                   *lun_rebuild_status_p = NULL;
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_block_edge_t                           *block_edge_p = NULL;
    fbe_lba_t                                   offset;
    fbe_lba_t                                   capacity;

    /* get the rebuild percentage buffer from the packet. */
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_lun_rebuild_status_t),
                                                (fbe_payload_control_buffer_t *)&lun_rebuild_status_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate the buffer to send the raid group get rebuild status usurper. */
    lun_percent_rebuilt_p = (fbe_raid_group_get_lun_percent_rebuilt_t *)fbe_transport_allocate_buffer();
    if(lun_percent_rebuilt_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the raid extent rebuild checkpoint as zero. */
    /* Populate the offset and capacity
     */
    fbe_lun_get_offset(lun_p, &offset);
    fbe_lun_get_imported_capacity(lun_p, &capacity);
    lun_percent_rebuilt_p->lun_capacity = capacity;
    lun_percent_rebuilt_p->lun_offset = offset;
    lun_percent_rebuilt_p->lun_percent_rebuilt = 0;
    lun_percent_rebuilt_p->lun_rebuild_checkpoint = 0;
    lun_percent_rebuilt_p->debug_flags = 0;
    lun_percent_rebuilt_p->is_rebuild_in_progress = FBE_FALSE;
    
    /* allocate the new control operation. */
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT,
                                        lun_percent_rebuilt_p,
                                        sizeof(*lun_percent_rebuilt_p));
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    /* set the completion function. */
    fbe_transport_set_completion_function(packet_p, fbe_lun_usurper_get_rebuild_status_completion, lun_p);

    /* send the packet to raid object to get the rebuild status for raid extent. */
    fbe_lun_get_block_edge(lun_p, &block_edge_p);
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    return FBE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_lun_usurper_get_rebuild_status()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_usurper_get_rebuild_status_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion function for the raid rebuild
 *  extent and it also calculate the rebuild percentage for the lun extent from
 *  raid extent.
 *
 * @param packet_p      - packet requesting this operation.
 * @param lun_p         - pointer to lun object.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/11/2012          - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_usurper_get_rebuild_status_completion(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_status_t                                status;
    fbe_lun_t                                  *lun_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_control_operation_t            *prev_control_operation_p = NULL;
    fbe_payload_control_status_t                control_status;
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = NULL;
    fbe_lun_rebuild_status_t                   *lu_rebuild_status_p = NULL;

    /* Cast the context to base config object pointer. */
    lun_p = (fbe_lun_t *) context;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &lun_percent_rebuilt_p);

    /* get the previous control operation. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &lu_rebuild_status_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* if any of the subpacket failed with bad status then update the master status. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* release the allocated buffer and current control operation. */
        fbe_transport_release_buffer(lun_percent_rebuilt_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* Copy the data from the result to the request.
     */
    lu_rebuild_status_p->rebuild_checkpoint = lun_percent_rebuilt_p->lun_rebuild_checkpoint;
    lu_rebuild_status_p->rebuild_percentage = lun_percent_rebuilt_p->lun_percent_rebuilt;

    /* release the allocated buffer and control operation. */
    fbe_transport_release_buffer(lun_percent_rebuilt_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    /* complete the packet with good status. */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_usurper_get_rebuild_status_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_reset_unexpected_error_info()
 ******************************************************************************
 * @brief
 *  Reset the unexpected information so that we can get the LUN out of fail.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_reset_unexpected_error_info(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_lun_reset_unexpected_errors(lun_p);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************
 * end fbe_lun_reset_unexpected_error_info()
 ******************************/

/*!****************************************************************************
 * fbe_lun_enable_fail_on_unexpected_errors()
 ******************************************************************************
 * @brief
 *  Enable unexpected error handling.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_enable_fail_on_unexpected_errors(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    if (fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS))
    {
        fbe_lun_clear_flag(lun_p, FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun enable unexpected error limits\n");
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_enable_fail_on_unexpected_errors()
 ******************************/
/*!****************************************************************************
 * fbe_lun_disable_fail_on_unexpected_errors()
 ******************************************************************************
 * @brief
 *  disable unexpected error handling.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_disable_fail_on_unexpected_errors(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    if (!fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS))
    {
        fbe_lun_set_flag(lun_p, FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun disable unexpected error limits\n");
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_disable_fail_on_unexpected_errors()
 ******************************/

/*!****************************************************************************
 * fbe_lun_mark_lun_clean_completion()
 ******************************************************************************
 * @brief
 *
 * @param packet_p                  - pointer to a packet.
 * @param context                   - pointer to lun object.
 * 
 * @return fbe_status_t             - status of the operation.
 *
 * @author
 *  12/19/2012 - Created. MFerson
 ******************************************************************************/
static fbe_status_t
fbe_lun_mark_lun_clean_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context)
{
    fbe_lun_t*                              lun_p = NULL;
    fbe_status_t                            status;
    fbe_lun_clean_dirty_context_t *         clean_dirty_context = NULL;


    lun_p = (fbe_lun_t*)context;
    clean_dirty_context = &lun_p->clean_dirty_usurper_context;

    if(clean_dirty_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: write failed\n", __FUNCTION__);  
    }
    else {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: LUN dirty flag has been cleared\n", __FUNCTION__);
    }

    fbe_spinlock_lock(&lun_p->io_counter_lock);
    lun_p->clean_pending = FBE_FALSE;
    fbe_spinlock_unlock(&lun_p->io_counter_lock);

    fbe_lun_handle_queued_packets_waiting_for_clear_dirty_flag(lun_p);

    fbe_transport_set_status(packet_p, clean_dirty_context->status, 0);
    status = clean_dirty_context->status;

    return status;
}
/******************************************************************************
 * end fbe_lun_mark_lun_clean_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_mark_lun_clean()
 ******************************************************************************
 * @brief
 *  Mark a LUN as clean
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  12/19/2012 - Created. Matthew Ferson
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_prepare_for_power_shutdown(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lun_clean_dirty_context_t *         context = NULL;

    fbe_spinlock_lock(&lun_p->io_counter_lock);

    if(lun_p->io_counter > 0) {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: LUN has writes in progress\n", __FUNCTION__);

        fbe_spinlock_unlock(&lun_p->io_counter_lock);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }
    else if(! lun_p->marked_dirty) {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: LUN is not dirty\n", __FUNCTION__);

        fbe_spinlock_unlock(&lun_p->io_counter_lock);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: LUN will be marked clean\n", __FUNCTION__);

    lun_p->clean_pending    = TRUE;
    lun_p->clean_time       = 0;
    lun_p->marked_dirty     = FALSE;

    fbe_spinlock_unlock(&lun_p->io_counter_lock);

    context = &(lun_p->clean_dirty_usurper_context);
    context->dirty      = FBE_FALSE;
    context->is_read    = FBE_FALSE;
    context->lun_p      = lun_p;
    context->sp_id      = FBE_CMI_SP_ID_INVALID;

    status = fbe_lun_update_dirty_flag(context, packet_p,
                                       fbe_lun_mark_lun_clean_completion);   

    return FBE_STATUS_PENDING;
}
/******************************
 * end fbe_lun_mark_lun_clean()
 ******************************/

/*!****************************************************************************
 * fbe_lun_set_read_and_pin_index()
 ******************************************************************************
 * @brief
 *  Save the Read and Pin index.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_set_read_and_pin_index(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_lun_set_read_and_pin_index_t *set_read_pin_index_p = NULL;
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the zero percentage buffer from the packet. */
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_lun_set_read_and_pin_index_t),
                                                (fbe_payload_control_buffer_t *)&set_read_pin_index_p);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }
    /* Save this in the object for future use.
     */
    lun_p->object_index = set_read_pin_index_p->index;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_PENDING;
}
/******************************
 * end fbe_lun_set_read_and_pin_index()
 ******************************/

/*!****************************************************************************
 * fbe_lun_enable_io_loopback()
 ******************************************************************************
 * @brief
 *  Enable IO loopback.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  10/28/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_enable_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    if (!fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_ENABLE_IO_LOOPBACK))
    {
        fbe_lun_set_flag(lun_p, FBE_LUN_FLAGS_ENABLE_IO_LOOPBACK);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun enable IO loopback\n");
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_enable_io_loopback()
 ******************************/

/*!****************************************************************************
 * fbe_lun_disable_io_loopback()
 ******************************************************************************
 * @brief
 *  Disable IO loopback.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  10/28/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_disable_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    if (fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_ENABLE_IO_LOOPBACK))
    {
        fbe_lun_clear_flag(lun_p, FBE_LUN_FLAGS_ENABLE_IO_LOOPBACK);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun disable IO loopback\n");
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_disable_io_loopback()
 ******************************/

/*!****************************************************************************
 * fbe_lun_get_io_loopback()
 ******************************************************************************
 * @brief
 *  Get IO loopback information.
 *
 * @param lun_p                         - pointer to lun object.
 * @param packet_p                      - packet requesting this operation.
 *
 * @return status                       - status of the operation.
 *
 * @author
 *  10/28/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_get_io_loopback(fbe_lun_t* lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_bool_t                                 *enabled = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the buffer from the packet. */
    status = fbe_lun_usurper_get_control_buffer(lun_p, packet_p, 
                                                sizeof(fbe_bool_t),
                                                (fbe_payload_control_buffer_t *)&enabled);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get IO loopback information.
     */
    *enabled = fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_ENABLE_IO_LOOPBACK);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************
 * end fbe_lun_get_io_loopback()
 ******************************/

/*!**************************************************************
 * fbe_lun_usurper_initiate_hard_zero()
 ****************************************************************
 * @brief
 *  This function write zeros to a specific lba range to a LUN 
 *
 * @param in_lun_p          - The lun.
 * @param in_packet_p     - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  27/12/2014 - Created. Geng Han
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_initiate_hard_zero(fbe_lun_t* in_lun_p, fbe_packet_t* in_packet_p)
{
    fbe_status_t    status;
    fbe_lun_hard_zero_t *init_lun_hard_zero_p = NULL;


    /* get init zero request parameter from control buffer. */
    status = fbe_lun_usurper_get_control_buffer(in_lun_p, in_packet_p, 
                                                sizeof(fbe_lun_hard_zero_t),
                                                (fbe_payload_control_buffer_t *)&init_lun_hard_zero_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s : fail get control buffer \n", __FUNCTION__);
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }


    fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_object_trace((fbe_base_object_t*)in_lun_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n", __FUNCTION__);
    
    /* Set the completion function before we initialize the lun zero. */
    fbe_transport_set_completion_function(in_packet_p, fbe_lun_usurper_initiate_hard_zero_completion, in_lun_p);

    if (init_lun_hard_zero_p->clear_paged_metadata) {
        fbe_transport_set_completion_function(in_packet_p, fbe_lun_usurper_start_clear_page, in_lun_p);
    }

    /* initialize lun hard zero. */ 

    status = fbe_lun_zero_start_lun_hard_zeroing_packet_allocation(in_lun_p, 
                                                                   in_packet_p, 
                                                                   init_lun_hard_zero_p->lba, 
                                                                   init_lun_hard_zero_p->blocks, 
                                                                   init_lun_hard_zero_p->threads); 
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }
    
    return status;
}
/* end of fbe_lun_usurper_initiate_hard_zero() */


/*!**************************************************************
 * fbe_lun_usurper_start_clear_page()
 ******************************************************************************
 * @brief
 *  This function is used to clear paged area of lun
 *  condition.
 *
 * @param packet_p      - pointer to a control packet.
 * @param context       - context, which is a pointer to the base object.
 *
 * @return fbe_status_t    - Always FBE_STATUS_MORE_PROCESSING_REQUIRED
 *
 * @author
 *  03/24/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_start_clear_page(fbe_packet_t * packet_p,
                                                     fbe_packet_completion_context_t context)
{
    fbe_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    lun_p = (fbe_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK)) {
        return status;
    }
    fbe_lun_metadata_write_default_paged_metadata(lun_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* End fbe_lun_usurper_initiate_hard_zero_completion()*/


/*!**************************************************************
 * fbe_lun_usurper_initiate_hard_zero_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the LUN zero
 *  condition.
 *
 * @param packet_p      - pointer to a control packet.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  12/27/2014 - Created. Geng Han
 *
 ****************************************************************/
static fbe_status_t fbe_lun_usurper_initiate_hard_zero_completion(fbe_packet_t * packet_p,
                                                                  fbe_packet_completion_context_t context)
{
    fbe_lun_t *                         lun_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    lun_p = (fbe_lun_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    if((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s lun hard zero failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
        return status;
    }
    
    return FBE_STATUS_OK;
}   
/* End fbe_lun_usurper_initiate_hard_zero_completion()*/


/******************************
 * end fbe_lun_usurper.c
 ******************************/
