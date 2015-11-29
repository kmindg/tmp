/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Drive Management object usurper
 *  control code handling function.
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @version
 *   23-Mar-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
// local headers
#include "fbe_drive_mgmt_private.h"

static fbe_status_t fbe_drive_mgmt_get_drive_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_policy(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_change_policy(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_enable_fuel_gauge(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t *packet);
static fbe_status_t fbe_drive_mgmt_get_fuel_gauge(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t *packet);
static fbe_status_t fbe_drive_mgmt_set_fuel_gauge_poll_interval(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_fuel_gauge_poll_interval(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_drivelog(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_drive_debug_command(fbe_drive_mgmt_t *pDriveMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_usurper_dieh_load(fbe_drive_mgmt_t *pDriveMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_all_drive_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_total_drive_count(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_max_platform_drive_count(fbe_drive_mgmt_t *pDriveMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_set_crc_action(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_reduce_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_restore_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_reset_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_write_logpage_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_drive_mgmt_get_drive_configuration_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet);


/*!***************************************************************
 * fbe_drive_mgmt_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_drive_mgmt_t *drive_mgmt;
    
    drive_mgmt = (fbe_drive_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    
    control_code = fbe_transport_get_control_code(packet);

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. contol code = %d \n",
                      __FUNCTION__, control_code); 

    switch(control_code) {
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_INFO:
            status = fbe_drive_mgmt_get_drive_info(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_POLICY:
            status = fbe_drive_mgmt_get_policy(drive_mgmt, packet);
            break; 
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_CHANGE_POLICY:
            status = fbe_drive_mgmt_change_policy(drive_mgmt, packet);
            break; 
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_ENABLE_FUEL_GAUGE:
            status = fbe_drive_mgmt_enable_fuel_gauge(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE:
            status = fbe_drive_mgmt_get_fuel_gauge(drive_mgmt, packet);
            break; 
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_SET_FUEL_GAUGE_POLL_INTERVAL:
            status = fbe_drive_mgmt_set_fuel_gauge_poll_interval(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE_POLL_INTERVAL:
            status = fbe_drive_mgmt_get_fuel_gauge_poll_interval(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_LOG:
            status = fbe_drive_mgmt_get_drivelog(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DEBUG_COMMAND:
             status = fbe_drive_mgmt_drive_debug_command(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DIEH_LOAD_CONFIG:
            status = fbe_drive_mgmt_usurper_dieh_load(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_ALL_DRIVES_INFO:
            status = fbe_drive_mgmt_get_all_drive_info(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_TOTAL_DRIVES_COUNT:
            status = fbe_drive_mgmt_get_total_drive_count(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_MAX_PLATFORM_DRIVE_COUNT:
            status = fbe_drive_mgmt_get_max_platform_drive_count(drive_mgmt, packet);
            break;

        case FBE_ESP_DRIVE_MGMT_CONTROL_SET_CRC_ACTION:
            status = fbe_drive_mgmt_set_crc_action(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_REDUCE_SYSTEM_DRIVE_QUEUE_DEPTH:
            status = fbe_drive_mgmt_reduce_system_drive_queue_depth(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESTORE_SYSTEM_DRIVE_QUEUE_DEPTH:
            status = fbe_drive_mgmt_restore_system_drive_queue_depth(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESET_SYSTEM_DRIVE_QUEUE_DEPTH:
            status = fbe_drive_mgmt_reset_system_drive_queue_depth(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_WRITE_LOG_PAGE:
            status = fbe_drive_mgmt_write_logpage_info(drive_mgmt, packet);
            break;
        case FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVECONFIG_INFO:
            status = fbe_drive_mgmt_get_drive_configuration_info(drive_mgmt, packet);
            break;
        default:
            status =  fbe_base_environment_control_entry (object_handle, packet);
            break;
    }
    return status;
}
/******************************************
 * end fbe_drive_mgmt_control_entry()
 ******************************************/
/*!***************************************************************
 * fbe_drive_mgmt_get_drive_info()
 ****************************************************************
 * @brief
 *  This function processes the get driveosure info Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_get_drive_info(fbe_drive_mgmt_t *drive_mgmt, 
                                                fbe_packet_t * packet)
{
    fbe_esp_drive_mgmt_drive_info_t *drive_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t bus, drive_num, enclosure, i;
    fbe_drive_info_t *drive_mgmt_info = NULL;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 
            
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &drive_info);
    if (drive_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_drive_mgmt_drive_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    bus = drive_info->location.bus;
    enclosure = drive_info->location.enclosure;
    drive_num = drive_info->location.slot;

    /* Init the drive inserted-ness to FALSE. */
    drive_info->inserted = FALSE;
    drive_info->loop_a_valid = FALSE;
    drive_info->loop_b_valid = FALSE;
    drive_info->bypass_enabled_loop_a = FALSE;
    drive_info->bypass_enabled_loop_b = FALSE;
    drive_info->bypass_requested_loop_a = FALSE;
    drive_info->bypass_requested_loop_b = FALSE;
    drive_info->state = FBE_LIFECYCLE_STATE_INVALID;
    drive_info->death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;

    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);

    drive_mgmt_info = drive_mgmt->drive_info_array;
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {
        if(drive_mgmt_info->object_id != FBE_OBJECT_ID_INVALID) {
            if((drive_mgmt_info->location.bus == bus) &&
               (drive_mgmt_info->location.enclosure == enclosure) &&
               (drive_mgmt_info->location.slot == drive_num)){ 
                    // Copy the drive information
                    drive_info->inserted = TRUE;
                    drive_info->faulted = (drive_mgmt_info->state == FBE_LIFECYCLE_STATE_FAIL ||
                                           drive_mgmt_info->state == FBE_LIFECYCLE_STATE_PENDING_FAIL);

                    if(drive_mgmt_info->local_side_id == SP_A) 
                    {
                        // A side.
                        drive_info->loop_a_valid = TRUE;
                        drive_info->bypass_enabled_loop_a = drive_mgmt_info->bypassed;
                        drive_info->bypass_requested_loop_a = drive_mgmt_info->self_bypassed;
                    }
                    else if(drive_mgmt_info->local_side_id == SP_B)  
                    {
                        //B side.
                        drive_info->loop_b_valid = TRUE;
                        drive_info->bypass_enabled_loop_b = drive_mgmt_info->bypassed;
                        drive_info->bypass_requested_loop_b = drive_mgmt_info->self_bypassed;
                    }

                    drive_info->drive_type = drive_mgmt_info->drive_type;
                    drive_info->gross_capacity = drive_mgmt_info->gross_capacity;
                    fbe_copy_memory(&drive_info->vendor_id, &drive_mgmt_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1);
                    fbe_copy_memory(&drive_info->tla_part_number, &drive_mgmt_info->tla, FBE_SCSI_INQUIRY_TLA_SIZE+1);
                    fbe_copy_memory(&drive_info->product_rev, &drive_mgmt_info->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1);
                    fbe_copy_memory(&drive_info->product_serial_num, &drive_mgmt_info->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
                    drive_info->state = drive_mgmt_info->state;
                    drive_info->death_reason = drive_mgmt_info->death_reason;                    
                    drive_info->last_log = drive_mgmt_info->last_log;
                    drive_info->block_count = drive_mgmt_info->block_count;
                    drive_info->block_size = drive_mgmt_info->block_size;
                    drive_info->drive_qdepth = drive_mgmt_info->drive_qdepth;
                    drive_info->drive_RPM = drive_mgmt_info->drive_RPM;                    
                    drive_info->speed_capability = drive_mgmt_info->speed_capability;
                    drive_info->spin_down_qualified= drive_mgmt_info->spin_down_qualified;
                    drive_info->spin_up_count = drive_mgmt_info->spin_up_count;
                    drive_info->spin_up_time_in_minutes = drive_mgmt_info->spin_up_time_in_minutes;
                    drive_info->stand_by_time_in_minutes = drive_mgmt_info->stand_by_time_in_minutes;
                    drive_info->logical_state = drive_mgmt_info->logical_state; 

                    fbe_copy_memory (drive_info->drive_description_buff, &drive_mgmt_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE+1);
                    fbe_copy_memory (drive_info->dg_part_number_ascii, &drive_mgmt_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1);
                    fbe_copy_memory (drive_info->bridge_hw_rev, &drive_mgmt_info->bridge_hw_rev, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1);
                    fbe_copy_memory (drive_info->product_id, &drive_mgmt_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1);
                    
                    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_OK;
                }
        }   
        drive_mgmt_info++;     
    }
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

#if 0
    /* if we get here then we did not find the drive 
     * We return an error via the control status to let the caller
     * know that the drive was not found.  This is needed in the case
     * we have more drives in the array than the platform limit allows.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
#else
    // Temp work around to avoid callers that cannot handle the correct failed status
    // ARS 530402
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
#endif
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_get_drive_info()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_get_policy()
 ****************************************************************
 * @brief
 *  This function gets a given policy.  
 *  
 * @param  - None.
 *
 * @return fbe_status_t - FBE_PAYLOAD_CONTROL_STATUS_FAILURE if policy not
 *                          found in policy table.
 *
 * @author
 *  18-May-2010 - Created.  Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_get_policy(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{    
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_drive_mgmt_policy_t             *policy = NULL;
    fbe_drive_mgmt_policy_t             *policy_table_index = NULL;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &policy);
    if (policy == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* loop through table looking for correct policy and change it. If
    *  policy not found then return failure status.
    */
    fbe_spinlock_lock(&fbe_drive_mgmt_policy_table_lock);
        
    policy_table_index = &fbe_drive_mgmt_policy_table[0];
    while (policy_table_index->id != FBE_DRIVE_MGMT_POLICY_LAST)
    {
        if (policy_table_index->id == policy->id)
        {
            policy->value = policy_table_index->value;
            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
            break;
        }  
        policy_table_index++;
    }
    
    fbe_spinlock_unlock(&fbe_drive_mgmt_policy_table_lock);
    
    fbe_payload_control_set_status(control_operation, control_status);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_get_policy()
 ******************************************/


/*!***************************************************************
 * fbe_drive_mgmt_change_policy()
 ****************************************************************
 * @brief
 *  This function enables/disables a given policy.  
 *  
 * @param  - None.
 *
 * @return fbe_status_t - FBE_STATUS_OK if change was successful.
 *                      - FBE_STATUS_COMPONENT_NOT_FOUND if policy not
 *                          found in policy table.
 *
 * @author
 *  18-May-2010 - Created.  Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_change_policy(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_COMPONENT_NOT_FOUND;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_drive_mgmt_policy_t             *policy = NULL;
    fbe_drive_mgmt_policy_t             *policy_table_index = NULL;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &policy);
    if (policy == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* loop through table looking for correct policy and change it. If
    *  policy not found then FBE_STATUS_COMPONENT_NOT_FOUND is returned.
    */
    fbe_spinlock_lock(&fbe_drive_mgmt_policy_table_lock);
        
    policy_table_index = &fbe_drive_mgmt_policy_table[0];
    while (policy_table_index->id != FBE_DRIVE_MGMT_POLICY_LAST)
    {
        if (policy_table_index->id == policy->id)
        {
            policy_table_index->value = policy->value;
            status = FBE_STATUS_OK;
            break;
        }  
        policy_table_index++;
    }
    
    fbe_spinlock_unlock(&fbe_drive_mgmt_policy_table_lock);
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_drive_mgmt_change_policy()
 ******************************************/


/*!***************************************************************
 * fbe_drive_mgmt_enable_fuel_gauge()
 ****************************************************************
 * @brief
 *  This function enables/disables fuel gauge feature.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11-18-2010 - Created.  chenl6
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_enable_fuel_gauge(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_payload_ex_t                     *payload = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;
    fbe_u32_t                            *enable = 0;
    fbe_payload_control_buffer_length_t  len = 0;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &enable);
    if (enable == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_bool_t)){
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "0x%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt->fuel_gauge_info.auto_log_flag = *enable;
    
    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s Fuel gauge is enabled <1> or disabled <0>: %d.\n",
                          __FUNCTION__, *enable );
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_drive_mgmt_enable_fuel_gauge()
 ******************************************/
 
/*!***************************************************************
 * fbe_drive_mgmt_set_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function set minimum fuel gauge poll interval on memory.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11-05-2012 - Created.  chianc1
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_set_fuel_gauge_poll_interval(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           *interval = 0;
    fbe_payload_control_buffer_length_t len = 0;
 
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry. \n",
                          __FUNCTION__); 
 
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
 
    fbe_payload_control_get_buffer(control_operation, &interval);
    if (interval == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u32_t)){
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "0x%X len invalid\n", len);
 
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    drive_mgmt->fuel_gauge_info.min_poll_interval = *interval;
    
    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s the minimum gauge poll interval is set to %d.\n",
                          __FUNCTION__, drive_mgmt->fuel_gauge_info.min_poll_interval );
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/****************************************************
 * end fbe_drive_mgmt_set_fuel_gauge_poll_interval()
 ***************************************************/

 
/*!***************************************************************
 * fbe_drive_mgmt_get_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function get minimum fuel gauge poll interval from registry and set it value on memory.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11-05-2012 - Created.  chianc1
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_get_fuel_gauge_poll_interval(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           *interval = 0;
    fbe_payload_control_buffer_length_t len = 0;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    
    fbe_payload_control_get_buffer(control_operation, &interval);
    if (interval == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u32_t)){
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "0x%X len invalid\n", len);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Get the minimum poll interval from register. */ 
    fbe_drive_mgmt_get_fuel_gauge_poll_interval_from_registry(drive_mgmt, &(drive_mgmt->fuel_gauge_info.min_poll_interval));
    
    *interval = drive_mgmt->fuel_gauge_info.min_poll_interval;
    
    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s get minimum poll interval from registry %d.\n",
                          __FUNCTION__, *interval );
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/****************************************************
 * end fbe_drive_mgmt_get_fuel_gauge_poll_interval()
 ******************************************************/


/*!***************************************************************
 * fbe_drive_mgmt_get_drivelog()
 ****************************************************************
 * @brief
 *  This function processes drivegetlog command.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  03-28-2011 - Created.  chenl6
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_get_drivelog(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_device_physical_location_t      *location = NULL;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &location);
    if (location == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_device_physical_location_t)){
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "0x%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_drive_mgmt_handle_drivegetlog_request(drive_mgmt, location);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_drive_mgmt_get_drivelog()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_get_fuel_gauge()
 ****************************************************************
 * @brief
 *  This function processes get fuel gauge command.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  07-26-2012 - Created.  Christina Chiang
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_mgmt_get_fuel_gauge(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           *enable = 0;
    fbe_payload_control_buffer_length_t len = 0;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
    
    fbe_payload_control_get_buffer(control_operation, &enable);
    if (enable == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u32_t)){
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "0x%X len invalid\n", len);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* get fuel gauge is called by fbecli dmo command. */
    drive_mgmt->fuel_gauge_info.is_fbecli_cmd = FBE_TRUE;
    
    /* Check if fuel gauge is disabled/enabled, and pass it up to fbecli. */
    *enable = drive_mgmt->fuel_gauge_info.auto_log_flag;
    
    if (*enable == 1)
    {   
        /* Set fuel gauge condition */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                       (fbe_base_object_t*)drive_mgmt, 
                                       FBE_DRIVE_MGMT_FUEL_GAUGE_TIMER_COND);  
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s can't set FUEL GAUGE TIMER condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s Fuel gauge is disabled. \n",
                              __FUNCTION__);
    }
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_drive_mgmt_get_fuel_guage()
 ******************************************/


/*!***************************************************************
 * fbe_drive_mgmt_drive_debug_command()
 ****************************************************************
 * @brief
 *  Send drive debug command (cru on/off). Need to get parent obj
 *  id, send the command, and set a flag in the drive info to
 *  reflect the command option.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  08-22-11    GB created
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_drive_debug_command(fbe_drive_mgmt_t *pDriveMgmt, 
                                                fbe_packet_t * packet)
{
    fbe_esp_drive_mgmt_debug_control_t  *pCommandInfo = NULL;
    fbe_object_id_t                     parentObjId = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           bus;
    fbe_u32_t                           slot;
    fbe_u32_t                           encl;
    fbe_u32_t                           i;
    fbe_drive_info_t                    *pDriveInfo = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_base_object_mgmt_drv_dbg_action_t action = FBE_DRIVE_ACTION_NONE;
    fbe_base_object_mgmt_drv_dbg_ctrl_t debugControl;
    fbe_drive_mgmt_cmi_msg_type_t       msg_type;
    
    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pCommandInfo);
    if (pCommandInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s:fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_esp_drive_mgmt_debug_control_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s 0x%X len invalid\n",
                              __FUNCTION__,
                              length);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the parent obj id for this drive
    bus = pCommandInfo->bus;
    encl = pCommandInfo->encl;
    slot = pCommandInfo->slot;
    action = pCommandInfo->dbg_ctrl.driveDebugAction[slot];

    dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);
    pDriveInfo = pDriveMgmt->drive_info_array;
    for (i = 0; i < pDriveMgmt->max_supported_drives; i++, pDriveInfo++)
    {
        if(pDriveInfo->parent_object_id != FBE_OBJECT_ID_INVALID)
        {
            if((pDriveInfo->location.bus == bus) &&
               (pDriveInfo->location.enclosure == encl) &&
               (pDriveInfo->location.slot == slot))
            {
                pDriveInfo->driveDebugAction = action;
                break;
            }
        }
    }
    dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);
    // if no match found, return error
    if(i == pDriveMgmt->max_supported_drives)
    {
        // searched all items and didn't find location match
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Drive:%d_%d_%d: simulate physical %s, No ParentObjId.\n",
                              bus, encl, slot,
                              (action == FBE_DRIVE_ACTION_REMOVE) ? "REMOVAL":"INSERTION");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Drive:%d_%d_%d: simulate physical %s, delay opt: %s, dualsp mode: %s.\n",
                               bus, encl, slot,
                               (action == FBE_DRIVE_ACTION_REMOVE) ? "REMOVAL":"INSERTION",
                               pCommandInfo->defer? "TRUE": "FALSE",
                               pCommandInfo->dualsp? "TRUE": "FALSE");

    //If it is not a delay command, we got to perform all debug cmds.
    if(!pCommandInfo->defer)
    {
        //we got to loop all the drives to perform the real action about the drive debug control.
        dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);
        pDriveInfo = pDriveMgmt->drive_info_array;
        for (i = 0; i < pDriveMgmt->max_supported_drives; i++, pDriveInfo++)
        {
            if(pDriveInfo->parent_object_id != FBE_OBJECT_ID_INVALID && pDriveInfo->driveDebugAction != FBE_DRIVE_ACTION_NONE)
            {
                bus = pDriveInfo->location.bus;
                encl = pDriveInfo->location.enclosure;
                slot = pDriveInfo->location.slot;
                fbe_zero_memory(&debugControl, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
                debugControl.driveDebugAction[slot] = pDriveInfo->driveDebugAction;
                debugControl.driveCount = pDriveMgmt->max_supported_drives;
                parentObjId = pDriveInfo->parent_object_id;

                dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);

                fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "Drive:%d_%d_%d: simulate physical %s.\n",
                                          bus, encl, slot,
                                          (debugControl.driveDebugAction[slot] == FBE_DRIVE_ACTION_REMOVE) ? "REMOVAL":"INSERTION");

                // send the debug command
                status = fbe_api_enclosure_send_drive_debug_control(parentObjId, &debugControl);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt,
                                            FBE_TRACE_LEVEL_WARNING,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "Drive:%d_%d_%d: simulate physical %s, sending cmd to PhyPkg failed, status 0x%X.\n",
                                            bus, encl, slot,
                                            (debugControl.driveDebugAction[slot] == FBE_DRIVE_ACTION_REMOVE) ? "REMOVAL":"INSERTION",
                                            status);


                    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                if (pCommandInfo->dualsp == FBE_TRUE)
                {
                    msg_type = (debugControl.driveDebugAction[slot] == FBE_DRIVE_ACTION_REMOVE) ? 
                                    FBE_DRIVE_MGMT_CMI_MSG_DEBUG_REMOVE:FBE_DRIVE_MGMT_CMI_MSG_DEBUG_INSERT;
                    /* didn't check if peer exists or not */
                    status = fbe_drive_mgmt_send_cmi_message(pDriveMgmt, bus, encl, slot, msg_type);
                }

                dmo_drive_array_lock(pDriveMgmt,__FUNCTION__);

                // set the internal flag to indicate debug command in progress
                pDriveInfo->dbg_remove = (debugControl.driveDebugAction[slot] == FBE_DRIVE_ACTION_REMOVE)?TRUE:FALSE;
                pDriveInfo->driveDebugAction = FBE_DRIVE_ACTION_NONE;
            }
        }
        dmo_drive_array_unlock(pDriveMgmt,__FUNCTION__);
    }
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

} //fbe_drive_mgmt_set_drive_debug_command

/*!***************************************************************
 * fbe_drive_mgmt_usurper_dieh_load()
 ****************************************************************
 * @brief
 *  Handles usurper call to initialize DIEH, or reload a new
 *  DIEH configuration.
 *  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  03-23-12  Wayne Garrett -- created
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_usurper_dieh_load(fbe_drive_mgmt_t *pDriveMgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_drive_mgmt_dieh_load_config_t *pConfigInfo = NULL;
    fbe_u32_t length;
    fbe_dieh_load_status_t dieh_status;

    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",  __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pConfigInfo);
    if (pConfigInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s:fbe_payload_control_get_buffer fail\n",  __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_drive_mgmt_dieh_load_config_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s len %d invalid, expected %d \n", __FUNCTION__, length, (int)sizeof(fbe_drive_mgmt_dieh_load_config_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    dieh_status = fbe_drive_mgmt_dieh_init(pDriveMgmt, pConfigInfo->file);

    if (FBE_DMO_THRSH_NO_ERROR != dieh_status)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }


    fbe_payload_control_set_status_qualifier(control_operation, dieh_status);   /* use qualifer to pass loading status*/

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
} // fbe_drive_mgmt_usurper_dieh_load


/*!***************************************************************
 * fbe_drive_mgmt_get_all_drive_info()
 ****************************************************************
 * @brief
 *  similar to get_drive_info but for all drives
 *  
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_get_all_drive_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_esp_drive_mgmt_drive_info_t *       drive_info = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t     *   control_operation = NULL;
    fbe_u32_t                               i;
    fbe_drive_info_t *                      drive_mgmt_info = NULL;
    dmo_get_all_drives_info_t *             get_all_drives_info_p = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 
            
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &get_all_drives_info_p);
    if (get_all_drives_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(dmo_get_all_drives_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need the sg list in order to fill this memory since we can't pass pointers in the data structure itself*/
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_esp_drive_mgmt_drive_info_t)) != get_all_drives_info_p->expected_count)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                              __FUNCTION__, sg_element->count, get_all_drives_info_p->expected_count);
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /*start of buffer containing get_all_drives_info_p->expected_count * sizeof(dmo_get_all_drives_info_t)*/
    drive_info = (fbe_esp_drive_mgmt_drive_info_t *)sg_element->address;

    fbe_zero_memory(drive_info, get_all_drives_info_p->expected_count * sizeof(fbe_esp_drive_mgmt_drive_info_t));

    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);

    drive_mgmt_info = drive_mgmt->drive_info_array;
    get_all_drives_info_p->actual_count = 0;

    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {                
        if(drive_mgmt_info->object_id != FBE_OBJECT_ID_INVALID && 
           get_all_drives_info_p->actual_count < get_all_drives_info_p->expected_count) {

            drive_info->location.bus = drive_mgmt_info->location.bus;
            drive_info->location.enclosure = drive_mgmt_info->location.enclosure;
            drive_info->location.slot = drive_mgmt_info->location.slot;

             // Copy the drive information
            drive_info->inserted = TRUE;
            drive_info->faulted = (drive_mgmt_info->state == FBE_LIFECYCLE_STATE_FAIL ||
                                   drive_mgmt_info->state == FBE_LIFECYCLE_STATE_PENDING_FAIL);

            if(drive_mgmt_info->local_side_id == SP_A) 
            {
                // A side.
                drive_info->loop_a_valid = TRUE;
                drive_info->bypass_enabled_loop_a = drive_mgmt_info->bypassed;
                drive_info->bypass_requested_loop_a = drive_mgmt_info->self_bypassed;
            }
            else if(drive_mgmt_info->local_side_id == SP_B)  
            {
                //B side.
                drive_info->loop_b_valid = TRUE;
                drive_info->bypass_enabled_loop_b = drive_mgmt_info->bypassed;
                drive_info->bypass_requested_loop_b = drive_mgmt_info->self_bypassed;
            }

            drive_info->drive_type = drive_mgmt_info->drive_type;
            drive_info->gross_capacity = drive_mgmt_info->gross_capacity;
            fbe_copy_memory(&drive_info->vendor_id, &drive_mgmt_info->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1);
            fbe_copy_memory(&drive_info->tla_part_number, &drive_mgmt_info->tla, FBE_SCSI_INQUIRY_TLA_SIZE+1);
            fbe_copy_memory(&drive_info->product_rev, &drive_mgmt_info->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1);
            fbe_copy_memory(&drive_info->product_serial_num, &drive_mgmt_info->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
            drive_info->state = drive_mgmt_info->state;
            drive_info->death_reason = drive_mgmt_info->death_reason;            
            drive_info->last_log = drive_mgmt_info->last_log;
            drive_info->block_count = drive_mgmt_info->block_count;
            drive_info->block_size = drive_mgmt_info->block_size;
            drive_info->drive_qdepth = drive_mgmt_info->drive_qdepth;
            drive_info->drive_RPM = drive_mgmt_info->drive_RPM;
            drive_info->speed_capability = drive_mgmt_info->speed_capability;
            drive_info->spin_down_qualified= drive_mgmt_info->spin_down_qualified;
            drive_info->spin_up_count = drive_mgmt_info->spin_up_count;
            drive_info->spin_up_time_in_minutes = drive_mgmt_info->spin_up_time_in_minutes;
            drive_info->stand_by_time_in_minutes = drive_mgmt_info->stand_by_time_in_minutes;
            drive_info->logical_state = drive_mgmt_info->logical_state; 

            fbe_copy_memory (drive_info->drive_description_buff, &drive_mgmt_info->drive_description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE+1);
            fbe_copy_memory (drive_info->dg_part_number_ascii, &drive_mgmt_info->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1);
            fbe_copy_memory (drive_info->bridge_hw_rev, &drive_mgmt_info->bridge_hw_rev, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1);
            fbe_copy_memory (drive_info->product_id, &drive_mgmt_info->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1);
            
            drive_info++;/*go to the next memory location*/
            get_all_drives_info_p->actual_count++;
            
        }  

        drive_mgmt_info++;/*go to next one*/
    }
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_mgmt_get_total_drive_count(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    dmo_get_total_drives_count_t *          drive_count = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t     *   control_operation = NULL;
    fbe_u32_t                               i;
    fbe_drive_info_t *                      drive_mgmt_info = NULL;
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 
            
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &drive_count);
    if (drive_count == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(dmo_get_total_drives_count_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_mgmt_info = drive_mgmt->drive_info_array;
    drive_count->total_count = 0;
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {                
        if(drive_mgmt_info->object_id != FBE_OBJECT_ID_INVALID){
            drive_count->total_count++;
        }

        drive_mgmt_info++;/*go to next one*/
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_get_max_platform_drive_count()
 ****************************************************************
 * @brief
 *  Get maximum number of drive count that the platform allows.
 * 
 *  @param drive_mgmt - The pointer to the drive_mgmt.
 *  @param packet - the pointer to the packet.
 *
 * @return fbe_status_t
 * @author 
 *    14-May-2013  PHE - Created.
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_get_max_platform_drive_count(fbe_drive_mgmt_t * pDriveMgmt, 
                                                                fbe_packet_t * packet)
{
    fbe_u32_t                           *   pMaxPlatformDriveCount = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t     *   control_operation = NULL;

    
    fbe_base_object_trace((fbe_base_object_t *)pDriveMgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "DMO: %s entry. \n",
                      __FUNCTION__); 
            
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pMaxPlatformDriveCount);
    if (pMaxPlatformDriveCount == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pDriveMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len 0x%X\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pMaxPlatformDriveCount = pDriveMgmt->max_supported_drives;
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_set_crc_action()
 ****************************************************************
 * @brief
 *  Handles usurper call to set the PDO CRC actions.  
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05-11-12  Wayne Garrett -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_set_crc_action(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_pdo_logical_error_action_t *pAction = NULL;
    fbe_u32_t length = 0;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n",  __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pAction);
    if (pAction == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s:fbe_payload_control_get_buffer fail\n",  __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_pdo_logical_error_action_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s len %d invalid, expected %d \n", __FUNCTION__, length, (int)sizeof(fbe_pdo_logical_error_action_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_PDO_ACTION_USE_REGISTRY == *pAction)   
    {
        fbe_drive_mgmt_get_logical_error_action(drive_mgmt, pAction);
    }

    drive_mgmt->logical_error_actions = *pAction;
    fbe_drive_mgmt_set_all_pdo_crc_actions(drive_mgmt, *pAction);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_set_system_drive_queue_depth()
 ****************************************************************
 * @brief
 *  Handles usurper call to reduce the system drive queue depth
 *  to a specifed value.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 * @parem depth - Queue depth to set.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_set_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet, fbe_u32_t depth)
{
    fbe_drive_info_t *  pDriveInfo      = NULL;
    fbe_status_t        status          = FBE_STATUS_INVALID;
    fbe_bool_t          is_system_drive = FBE_FALSE;
    fbe_u32_t           i;
    fbe_u32_t           system_drive_max;
    fbe_u32_t           system_drive_count = 0;
    fbe_u32_t *         system_drive_array_p = NULL;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n", __FUNCTION__);

    system_drive_max        = 4; //fbe_private_space_layout_get_number_of_system_drives();

    system_drive_array_p    = fbe_base_env_memory_ex_allocate(
        (fbe_base_environment_t *)drive_mgmt, 
        sizeof(fbe_u32_t) * system_drive_max);

    if(system_drive_array_p == NULL) {
       fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
       fbe_transport_complete_packet(packet);

       return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
    pDriveInfo = drive_mgmt->drive_info_array;
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {                
        if(pDriveInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            status = fbe_drive_mgmt_is_system_drive(drive_mgmt, &pDriveInfo->location, &is_system_drive);
            if((status == FBE_STATUS_OK) && is_system_drive)
            {
                system_drive_array_p[system_drive_count] = pDriveInfo->object_id;
                system_drive_count++;
            }
        }
        pDriveInfo++;
    }
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

    for (i = 0; i < system_drive_count; i++) {
        status = fbe_api_physical_drive_set_object_queue_depth(
            system_drive_array_p[i],
            depth,
            FBE_PACKET_FLAG_NO_ATTRIB);

        if (FBE_STATUS_OK != status)
        {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s: failed to change q depth, object_id 0x%x",
                                  __FUNCTION__, pDriveInfo->object_id);                
        }
    }
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, system_drive_array_p);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_reduce_system_drive_queue_depth()
 ****************************************************************
 * @brief
 *  Handles usurper call to reduce the system drive queue depth
 *  to a value defined in the DIEH XML file.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_reduce_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_u32_t reduce_count;
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n", __FUNCTION__);

    dmo_drive_array_lock(drive_mgmt, __FUNCTION__);
    reduce_count = drive_mgmt->system_drive_queue_depth_reduce_count;
    drive_mgmt->system_drive_queue_depth_reduce_count++;
    dmo_drive_array_unlock(drive_mgmt, __FUNCTION__);

    fbe_scheduler_start_to_monitor_system_drive_queue_depth();

    if(reduce_count == 0) {
        fbe_drive_mgmt_set_system_drive_queue_depth(
            drive_mgmt,
            packet,
            drive_mgmt->system_drive_reduced_queue_depth
        );
    }
    else {
       fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
       fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_restore_system_drive_queue_depth()
 ****************************************************************
 * @brief
 *  Handles usurper call to restore the system drive queue depth
 *  to the normal value defined in the DIEH XML file.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_restore_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_u32_t reduce_count = 0;
    fbe_bool_t bailout = FBE_FALSE;
    fbe_status_t status = FBE_STATUS_INVALID;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n", __FUNCTION__);

    dmo_drive_array_lock(drive_mgmt, __FUNCTION__);
    if(drive_mgmt->system_drive_queue_depth_reduce_count == 0)
    {
        bailout = FBE_TRUE;
    }
    else
    {
        drive_mgmt->system_drive_queue_depth_reduce_count--;
        reduce_count = drive_mgmt->system_drive_queue_depth_reduce_count;
    }
    dmo_drive_array_unlock(drive_mgmt, __FUNCTION__);

    if(FBE_IS_TRUE(bailout)) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: Restore Q Depth would have caused count to go negative\n");

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_OK;
    }

    if(reduce_count == 0) {
        status = fbe_drive_mgmt_set_system_drive_queue_depth(
            drive_mgmt,
            packet,
            drive_mgmt->system_drive_normal_queue_depth
        );
        if(status == FBE_STATUS_OK)
        {
            fbe_scheduler_stop_monitoring_system_drive_queue_depth();
        }
    }
    else {
       fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
       fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_reset_system_drive_queue_depth()
 ****************************************************************
 * @brief
 *  Handles usurper call to reset the system drive queue depth
 *  to the normal value defined in the DIEH XML file.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_reset_system_drive_queue_depth(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{

    fbe_status_t status = FBE_STATUS_INVALID;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n", __FUNCTION__);

    dmo_drive_array_lock(drive_mgmt, __FUNCTION__);
    drive_mgmt->system_drive_queue_depth_reduce_count = 0;
    dmo_drive_array_unlock(drive_mgmt, __FUNCTION__);

    status = fbe_drive_mgmt_set_system_drive_queue_depth(
        drive_mgmt,
        packet,
        drive_mgmt->system_drive_normal_queue_depth
    );

    if(status == FBE_STATUS_OK)
    {
        fbe_scheduler_stop_monitoring_system_drive_queue_depth();
    }


    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_drive_mgmt_write_logpage_info()
 ****************************************************************
 * @brief
 *   This function will set up the fuel gauge logpage info so that
 *   the fuel gauge can generate the nice format type.
 *  
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_write_logpage_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{
    fbe_physical_drive_bms_op_asynch_t      *bms_write_p = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               bytes_to_write = 0;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;


    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry. \n",
                      __FUNCTION__); 
            
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &bms_write_p);
    if (bms_write_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_physical_drive_bms_op_asynch_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Set-up fuel gauge data
    drive_mgmt->fuel_gauge_info.is_initialized = FBE_FALSE;
    drive_mgmt->fuel_gauge_info.auto_log_flag = FBE_TRUE;

    fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_in_progrss, FBE_FALSE);
    fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_1st_time, FBE_TRUE);

    drive_mgmt->fuel_gauge_info.current_object =  bms_write_p->object_id;
    drive_mgmt->fuel_gauge_info.current_location.bus = bms_write_p->get_drive_info.port_number;
    drive_mgmt->fuel_gauge_info.current_location.enclosure= bms_write_p->get_drive_info.enclosure_number;
    drive_mgmt->fuel_gauge_info.current_location.slot = bms_write_p->get_drive_info.slot_number;

    fbe_copy_memory(drive_mgmt->fuel_gauge_info.current_vendor, bms_write_p->get_drive_info.vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
    fbe_copy_memory(drive_mgmt->fuel_gauge_info.current_sn, bms_write_p->get_drive_info.product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    fbe_copy_memory(drive_mgmt->fuel_gauge_info.current_tla, bms_write_p->get_drive_info.tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE);
    fbe_copy_memory(drive_mgmt->fuel_gauge_info.current_fw_rev, bms_write_p->get_drive_info.product_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);

    drive_mgmt->fuel_gauge_info.currrent_drive_type = bms_write_p->get_drive_info.drive_type;
    drive_mgmt->fuel_gauge_info.is_fbecli_cmd = FBE_TRUE;
    drive_mgmt->fuel_gauge_info.current_supported_page_index = bms_write_p->get_log_page.page_code;

    drive_mgmt->fuel_gauge_info.power_on_hours = 0;
    drive_mgmt->fuel_gauge_info.current_PE_cycles = 0;
    drive_mgmt->fuel_gauge_info.EOL_PE_cycles = 0;

    fbe_drive_mgmt_fuel_gauge_init(drive_mgmt);

    drive_mgmt->fuel_gauge_info.fuel_gauge_op->get_log_page.page_code = bms_write_p->get_log_page.page_code;
    drive_mgmt->fuel_gauge_info.fuel_gauge_op->get_log_page.transfer_count = bms_write_p->get_log_page.transfer_count;

    fbe_copy_memory(drive_mgmt->fuel_gauge_info.read_buffer, bms_write_p->response_buf, DMO_FUEL_GAUGE_READ_BUFFER_LENGTH);
    fbe_copy_memory(drive_mgmt->fuel_gauge_info.write_buffer, bms_write_p->response_buf, DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH);

    drive_mgmt->fuel_gauge_info.fuel_gauge_op->drive_mgmt = drive_mgmt;

    drive_mgmt->fuel_gauge_info.need_drive_header = FBE_TRUE;

    switch (bms_write_p->get_log_page.page_code)
    {
        case LOG_PAGE_15:
        case LOG_PAGE_30:
        case LOG_PAGE_31:
            // Fill in the write buffer 
            fbe_drive_mgmt_fuel_gauge_fill_write_buffer(drive_mgmt, &bytes_to_write);

            bms_write_p->rw_buffer_length = bytes_to_write;

            // get the sg element 
            fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

            if (sg_element != NULL)
            {
                // Copy the write buffer back to SG list
                fbe_copy_memory(sg_element[0].address, drive_mgmt->fuel_gauge_info.write_buffer, bytes_to_write);
                fbe_copy_memory(bms_write_p->response_buf, drive_mgmt->fuel_gauge_info.write_buffer, bytes_to_write);
            }

            fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s: Completed. \n", __FUNCTION__);

            break;
            
        default:
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Log page %x is not supported.\n",
                                  __FUNCTION__, bms_write_p->get_log_page.page_code);
            break;
        
    }

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    fbe_drive_mgmt_fuel_gauge_cleanup(drive_mgmt);

    return FBE_STATUS_OK;
} // end of fbe_drive_mgmt_write_logpage_info



/*!***************************************************************
 * fbe_drive_mgmt_get_drive_configuration_info()
 ****************************************************************
 * @brief
 *  Returns the DriveConfiguration XML info, such as a version
 *  and description of what was last loaded.
 *  
 * @param drive_mgmt - Pointer to drive mgmt object.
 * @param packet - Pointer to usurper packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  07-27-15  Wayne Garrett -- created
 *
 ****************************************************************/
static fbe_status_t fbe_drive_mgmt_get_drive_configuration_info(fbe_drive_mgmt_t *drive_mgmt, fbe_packet_t * packet)
{   
    fbe_esp_drive_mgmt_driveconfig_info_t   *driveconfig_info_p;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;    

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s entry. \n",
                      __FUNCTION__); 

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &driveconfig_info_p);
    if (driveconfig_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_drive_mgmt_driveconfig_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s len %d invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    driveconfig_info_p->xml_info = drive_mgmt->drive_config_xml_info;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
