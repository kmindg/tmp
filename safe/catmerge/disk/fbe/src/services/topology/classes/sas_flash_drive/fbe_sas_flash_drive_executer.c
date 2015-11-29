/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_flash_drive_executer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains executer functions for sas flash drive. 
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "sas_flash_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "base_physical_drive_private.h"
#include "sas_physical_drive_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t sas_flash_drive_handle_block_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_flash_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_sas_drive_status_t fbe_sas_flash_drive_process_vpd_inquiry(fbe_sas_flash_drive_t * sas_flash_drive, fbe_u8_t *vpd_inq);
static fbe_status_t sas_flash_drive_send_log_sense_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_flash_drive_send_sanitize_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_flash_drive_sanitize_tur_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_flash_drive_send_vpd_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*!**************************************************************
 * @fn fbe_sas_flash_drive_event_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_event_type_t event_type,
 *            fbe_event_context_t event_context)
 ****************************************************************
 * @brief
 *   This is the event entry function for sas flash drive.
 *  
 * @param object_handle - object handle. 
 * @param event_type - event type.
 * @param event_context - event context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_status_t status;

    sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)sas_flash_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Handle the event we have received. */
    switch (event_type)
    {
        default:
            status = fbe_sas_physical_drive_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_sas_flash_drive_io_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the event entry function for sas flash drive.
 *  
 * @param object_handle - object handle. 
 * @param packet - pointer to packet.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
     return fbe_sas_physical_drive_io_entry(object_handle, packet);
}

#if 0
fbe_status_t 
fbe_sas_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_transport_id_t transport_id;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_flash_drive_t *sas_flash_drive;
    sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    /* First we need to figure out to what transport this packet belong.
     */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch (transport_id)
    {
        case FBE_TRANSPORT_ID_DISCOVERY:
            status = fbe_sas_physical_drive_io_entry(object_handle, packet);
            break;
        case FBE_TRANSPORT_ID_BLOCK:
            fbe_transport_set_completion_function(packet, sas_flash_drive_handle_block_io_completion, sas_flash_drive);
            status = fbe_sas_physical_drive_io_entry(object_handle, packet);
            break;
        default:
            break;
    };

    return status;
}

/*!**************************************************************
 * @fn sas_flash_drive_handle_block_io_completion(
 *            fbe_packet_t * packet,
 *            fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *   This is the completion function for the packet sent to a drive.
 *  
 * @param packet - pointer to packet. 
 * @param context - completion context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t 
sas_flash_drive_handle_block_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_status_t status = FBE_STATUS_OK;
        
    sas_flash_drive = (fbe_sas_flash_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sas_flash_drive,
                                     FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s, packet status failed:0x%X\n", __FUNCTION__, status);
                                     
       return status;       
    }

    return status;
}
#endif

/*!**************************************************************
 * @fn fbe_sas_flash_drive_send_vpd_inquiry(
 *            fbe_sas_flash_drive_t * sas_flash_drive,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sas_flash_drive - pointer to sas flash drive object.
 * @param packet - pointer to packet. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_send_vpd_inquiry(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s, entry\n",
            __FUNCTION__);    
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    if(payload_control_operation == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s,  NULL control operation, packet: %64p.\n",
                                __FUNCTION__, packet);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_INQUIRY_DATA_SIZE);

    fbe_payload_cdb_build_vpd_inquiry(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT, 
        (sas_flash_drive->vpd_pages_info.supported_vpd_pages)[sas_flash_drive->vpd_pages_info.vpd_page_counter]);

    fbe_transport_set_completion_function(packet, sas_flash_drive_send_vpd_inquiry_completion, sas_flash_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sas_flash_drive)->ssp_edge, packet);
    return status;
}

/*!**************************************************************
 * @fn sas_flash_drive_send_vpd_inquiry_completion(
 *            fbe_sas_flash_drive_t * sas_flash_drive,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the completion function for VPD inquiry packet.
 *  
 * @param packet - pointer to packet. 
 * @param context - completion context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t 
sas_flash_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_u32_t         transferred_count;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};
    fbe_base_physical_drive_t* base_physical_drive = NULL;
    
    
    sas_flash_drive = (fbe_sas_flash_drive_t *)context;
    base_physical_drive = (fbe_base_physical_drive_t*)sas_flash_drive;
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s, entry\n",
            __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_flash_drive, packet, payload_cdb_operation, &error, NULL, NULL);
        if (error == FBE_SCSI_XFERCOUNTNOTZERO)
        {
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            if (transferred_count <= FBE_SCSI_INQUIRY_DATA_SIZE)
            {
                /* Underrun is not an error */
                error = 0;
            }
        }
        if (sas_physical_drive_set_control_status(error, payload_control_operation) == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            validate_status = fbe_sas_flash_drive_process_vpd_inquiry(sas_flash_drive, (fbe_u8_t *)(sg_list[0].address));
            if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID) 
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      "%s Invalid packet status: 0x%X\n",
                                      __FUNCTION__, status);

                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_flash_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sas_flash_drive), 
                                                (fbe_base_object_t*)sas_flash_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
                
                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            
                }        
            }
            else if (validate_status == FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING) 
            {
                fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING);
            }
        }
    }

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);

    /* At this point we have read all of the VPD pages.  Since this function is called after a FW update
     * we need to notify DMO so that the paddle card FW is available in DMO and ultimately ADMIN (AR 573905)
     */
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
            FBE_TRACE_LEVEL_INFO,
            "%s: Notify DMO of FW rev %s len %d, param 0x%x\n",
            __FUNCTION__,
            base_physical_drive->drive_info.revision,
            base_physical_drive->drive_info.product_rev_len,
            base_physical_drive->drive_info.drive_parameter_flags);    
    /* Exception made for micron buckhorns */
    if (((base_physical_drive->drive_info.drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_WS_UNMAP) == 0) && 
        fbe_base_physical_drive_is_unmap_supported_override(base_physical_drive))
    {

        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                   FBE_TRACE_LEVEL_INFO,
                                                   "Unmap exception made for micron buckhorn pn:%s flags: 0x%x\n",
                                                   base_physical_drive->drive_info.part_number,
                                                   base_physical_drive->drive_info.drive_parameter_flags);  

        base_physical_drive->drive_info.drive_parameter_flags |= FBE_PDO_FLAG_SUPPORTS_WS_UNMAP;
    }
    
    if (base_physical_drive->drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                   FBE_TRACE_LEVEL_INFO,
                                                   "HE Drive disable Unmap pn:%s flags: 0x%x\n",
                                                   base_physical_drive->drive_info.part_number,
                                                   base_physical_drive->drive_info.drive_parameter_flags);  
        base_physical_drive->drive_info.drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_UNMAP);
        base_physical_drive->drive_info.drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_WS_UNMAP);
    }
    dataChangeInfo.phys_location.bus = base_physical_drive->port_number; 
    dataChangeInfo.phys_location.enclosure = base_physical_drive->enclosure_number;
    dataChangeInfo.phys_location.slot = (fbe_u8_t)base_physical_drive->slot_number;
    dataChangeInfo.data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
    dataChangeInfo.device_mask = FBE_DEVICE_TYPE_DRIVE;
    fbe_base_physical_drive_send_data_change_notification(base_physical_drive, &dataChangeInfo);    
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_flash_drive_process_vpd_inquiry(
 *            fbe_sas_flash_drive_t * sas_flash_drive,
 *            fbe_u8_t *vpd_inq)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sas_flash_drive - pointer to sas flash drive object.
 * @param vpd_inq - pointer to inquiry string. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_sas_drive_status_t 
fbe_sas_flash_drive_process_vpd_inquiry(fbe_sas_flash_drive_t * sas_flash_drive, fbe_u8_t *vpd_inq)
{
    fbe_u8_t vpd_page_code;
    fbe_u8_t vpd_max_supported_pages;
    fbe_u8_t page_counter;
    fbe_u8_t i;
    fbe_u8_t* vpd_inq_offset;
    fbe_u32_t drive_writes_per_day = 0;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_flash_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s, entry\n",
            __FUNCTION__);
    
    vpd_page_code = *(vpd_inq + FBE_SCSI_INQUIRY_VPD_PAGE_CODE_OFFSET);

    switch (vpd_page_code) {
        case FBE_SCSI_INQUIRY_VPD_PAGE_00:
            /* Get the number of VPD pages supported */
            vpd_max_supported_pages = *(vpd_inq + FBE_SCSI_INQUIRY_VPD_00_MAX_SUPPORTED_PAGES_OFFSET);

            /* "vpd_inq" will point to the first supported page */
            vpd_inq += FBE_SCSI_INQUIRY_VPD_00_SUPPORTED_PAGES_START_OFFSET;

            /* Store the list of supported VPD pages */
            /* Currently only C0 and D2 are supported */
            page_counter = 0;
            for (i = 0; (i < vpd_max_supported_pages) && (page_counter < FBE_SCSI_INQUIRY_VPD_MAX_PAGES); i++)
            {
                if ((*(vpd_inq+i) == FBE_SCSI_INQUIRY_VPD_PAGE_00) || 
                    (*(vpd_inq+i) == FBE_SCSI_INQUIRY_VPD_PAGE_C0) ||
                    (*(vpd_inq+i) == FBE_SCSI_INQUIRY_VPD_PAGE_D2) ||
                    (*(vpd_inq+i) == FBE_SCSI_INQUIRY_VPD_PAGE_B2))
                {
                    sas_flash_drive->vpd_pages_info.supported_vpd_pages[page_counter] = *(vpd_inq+i);
                    page_counter++;
                    sas_flash_drive->vpd_pages_info.vpd_max_supported_pages++;
                }
            }

            break;

        case FBE_SCSI_INQUIRY_VPD_PAGE_C0:
            /*Read the number of drive writes per day from a Flash Drive*/
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                    FBE_TRACE_LEVEL_DEBUG_LOW,
                    "%s: Drive is a Flash Drive\n",
                    __FUNCTION__);

            vpd_inq_offset = vpd_inq + FBE_SCSI_INQUIRY_VPD_C0_DRIVE_WRITES_PER_DAY_OFFSET;

            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                    FBE_TRACE_LEVEL_DEBUG_LOW,
                    "%s: Flash Drive W/D Byte: 0x%x\n",
                    __FUNCTION__, *vpd_inq_offset);               

            // Clear the number of writes per day in drive parameter flags first
            info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);

            if ((*vpd_inq_offset & 0x80) > 0) /*If the high order bit is set the data is valid */
            {
                //The number of drive writes / day is in the lower 7 bits
                info_ptr->drive_parameter_flags |= (*vpd_inq_offset & 0x7F);
                
                drive_writes_per_day = (fbe_u32_t)(info_ptr->drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
                
                fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        "%s: Flash Drive W/D actual: 0x%x Param Flag: 0x%x\n",
                        __FUNCTION__, drive_writes_per_day, (fbe_u32_t)(info_ptr->drive_parameter_flags));

#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
                /* Fail drives that are less than 10 WPD */
                if (drive_writes_per_day < FBE_MLC_READ_INTENSIVE_LIMIT)
                {
                    if (fbe_dcs_param_is_enabled(FBE_DCS_IGNORE_INVALID_IDENTITY))
                    {
                        fbe_pdo_maintenance_mode_set_flag(&base_physical_drive->maintenance_mode_bitmap, 
                                                          FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);        
                    }
                    else
                    {
                        /* Clear the maintenance flag if the dcs mode is not set */
                        fbe_pdo_maintenance_mode_clear_flag(&base_physical_drive->maintenance_mode_bitmap, 
                                                          FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);
                        return FBE_SAS_DRIVE_STATUS_INVALID;
                    }
                }
#endif
            }
            else
            {
                // Set writes per day in the drive pararam flags
                info_ptr->drive_parameter_flags |= (*vpd_inq_offset);
            }

            /* For drives with Paddle cards extract  the FW revision from VPD page 0xC0 */             
            if (fbe_equal_memory(info_ptr->vendor_id, "SATA", 4))
            {
                /* Extract out what we need from VPD page 0xC0 */ 
                vpd_inq_offset = vpd_inq + FBE_SCSI_INQUIRY_VPD_C0_PROD_REV_OFFSET;
                
                /* Copy the ASCII format Product Revision into the appropriate location
                 * of dl_product_rev.
                 */
                info_ptr->revision[4] = '/'; /* Coverity Fix */
                strncpy(&info_ptr->revision[5], vpd_inq_offset, 4);
                info_ptr->product_rev_len = FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE;  /* of form "DDDD/BBBB" */
            
                /* Seagate SATA drives format the revision as X.YYY.
                 * If the revision has a ".", then remove it and add the X value in it's place
                 */
                if(info_ptr->revision[0] == '.')
                {
                    info_ptr->revision[0] = vpd_inq_offset[115];
                }
            }
            
            break;

        case FBE_SCSI_INQUIRY_VPD_PAGE_D2:
            /* For drives with Paddle cards extract  the HW revision from VPD page 0xD2 */ 
            if (fbe_equal_memory(info_ptr->vendor_id, "SATA", 4))
            {
                /* Extract out what we need from VPD page 0xD2 */ 
                vpd_inq += FBE_SCSI_INQUIRY_VPD_D2_BRIDGE_HW_REV_OFFSET;
                /* Copy the Brige H/W revision into dl_bridge_hw_rev. */
                strncpy(info_ptr->bridge_hw_rev, vpd_inq, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE);
                /* This is a string so NULL terminate it */
                info_ptr->revision[info_ptr->product_rev_len] = '\0';                
            }
            break;
        case FBE_SCSI_INQUIRY_VPD_PAGE_B2:
            
            /* Extract out what we need from VPD page 0xB2 */ 
            vpd_inq += FBE_SCSI_INQUIRY_VPD_B2_SUPPORTS_UNMAP_OFFSET;

             /* Check if unmap is supported.  We only support this feature for
               medium endurance drives.  
            */
            if (fbe_base_physical_drive_get_drive_type(base_physical_drive) == FBE_DRIVE_TYPE_SAS_FLASH_ME)
            {                  
                /* 0x80 means unmap command is supported */
                if ((*vpd_inq & 0x80) != 0)
                {
                    info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_UNMAP);
                    info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_SUPPORTS_UNMAP;
                }
    
                /* 0x20 means unmap with write same is supported */
                /* Exception made for micron buckhorns */
                if ((*vpd_inq & 0x20) != 0)
                {
                    info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_WS_UNMAP);
                    info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_SUPPORTS_WS_UNMAP;
                }   
            }

            break;
        default:
            return FBE_SAS_DRIVE_STATUS_INVALID;
            break;
    }

    sas_flash_drive->vpd_pages_info.vpd_page_counter++;

    if (sas_flash_drive->vpd_pages_info.vpd_page_counter < sas_flash_drive->vpd_pages_info.vpd_max_supported_pages)
    {
        return FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_flash_drive_send_log_sense(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_log_page);

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, get_log_page->transfer_count);

    sas_physical_drive_cdb_build_log_sense(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                              get_log_page->page_code, get_log_page->transfer_count);

    fbe_transport_set_completion_function(packet, sas_flash_drive_send_log_sense_completion, sas_flash_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sas_flash_drive)->ssp_edge, packet);
    return status;
}

/*
 * This is the completion function for receive_diag packet sent to sas_physical_drive.
 */
static fbe_status_t 
sas_flash_drive_send_log_sense_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_u32_t  transferred_count;

    sas_flash_drive = (fbe_sas_flash_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &get_log_page);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    } else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_flash_drive, packet,
                                               payload_cdb_operation, &error, NULL, NULL);
        if (!error || (error == FBE_SCSI_XFERCOUNTNOTZERO)) 
        {
            status = FBE_STATUS_OK;
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            get_log_page->transfer_count = transferred_count;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return status;
}

/*
 * Send a vpd inquiry to the sas_flash_physical_drive.
 */
fbe_status_t
fbe_sas_flash_drive_send_vpd(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_log_page);

    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, get_log_page->transfer_count);

    fbe_payload_cdb_build_vpd_inquiry(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT, 
                                      get_log_page->page_code);

    fbe_transport_set_completion_function(packet, sas_flash_drive_send_vpd_completion, sas_flash_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sas_flash_drive)->ssp_edge, packet);
    return status;
}

/*
 * This is the completion function for sending a vpd inquiry to the sas_flash_physical_drive.
 */
static fbe_status_t 
sas_flash_drive_send_vpd_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_u32_t  transferred_count;

    sas_flash_drive = (fbe_sas_flash_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &get_log_page);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    } else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sas_flash_drive, packet,
                                               payload_cdb_operation, &error, NULL, NULL);
        if (!error || (error == FBE_SCSI_XFERCOUNTNOTZERO)) 
        {
            status = FBE_STATUS_OK;
            fbe_payload_cdb_get_transferred_count(payload_cdb_operation, &transferred_count);
            get_log_page->transfer_count = transferred_count;
        }
        else
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return status;
}

fbe_status_t
fbe_sas_flash_drive_send_sanitize(fbe_sas_flash_drive_t* sas_flash_drive, fbe_packet_t* packet, fbe_scsi_sanitize_pattern_t pattern)
{
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_cdb_operation_t *           payload_cdb_operation = NULL;
    fbe_sg_element_t  *                     sg_list = NULL;     
    fbe_u8_t *                              buffer = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    
    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, fbe_transport_allocate_buffer failed\n", __FUNCTION__);
        fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = FBE_SCSI_FORMAT_UNIT_SANITIZE_HEADER_LENGTH;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);  /* data starts after sg_list terminator */
    sg_list[1].count = 0;
    sg_list[1].address = NULL;
    
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);    
    fbe_payload_cdb_set_transfer_count(payload_cdb_operation, FBE_SCSI_FORMAT_UNIT_SANITIZE_HEADER_LENGTH);

    sas_physical_drive_cdb_build_sanitize(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_FORMAT_TIMEOUT, sg_list[0].address, pattern);
    
    fbe_transport_set_completion_function(packet, sas_flash_drive_send_sanitize_completion, sas_flash_drive);
                                              
    status = fbe_ssp_transport_send_functional_packet(&sas_flash_drive->sas_physical_drive.ssp_edge, packet);
    
    return status;          
}


static fbe_status_t
sas_flash_drive_send_sanitize_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = (fbe_sas_flash_drive_t *)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sg_element_t * sg_list = NULL;
        
    payload = fbe_transport_get_payload_ex(packet);    
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid packet status: 0x%X\n",
                                                   __FUNCTION__, status);
    }
    else
    {
        /* caution... this call can initiate DiskCollect if last two args are not NULL.  This means if we have any issues with this cmd
           that DC will attempt to write to the drive, which could also get expected errors if format was initiated.  Drive could then
           be shot.  Review code path before changing the following function. */
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t*)sas_flash_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        if (error != 0)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s cmd failed. error:0x%x\n", __FUNCTION__, error);

            fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }
        else // successfully sent
        {            
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_INFO,
                                                       "%s cmd successful\n", __FUNCTION__);
        }
    }
    
    /* Release payload and buffer. */    
    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);
    fbe_transport_release_buffer(sg_list);
        
    return status;
}   

/*!**************************************************************
 * @fn fbe_sas_flash_drive_send_sanitize_tur
 ****************************************************************
 * @brief
 *   This is the function sends a TUR to get the santization status.
 *   The caller must provide a GET_SANITIZE_STATUS control operation
 *   in the packet.   The completion function will populate this
 *   control operation with the results.
 *  
 * @param sas_flash_drive - pointer to sas flash drive object.
 * @param vpd_inq - pointer to inquiry string. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  07/25/2014 - Created.  Wayne Garrett
 *
 ****************************************************************/
fbe_status_t fbe_sas_flash_drive_send_sanitize_tur(fbe_sas_flash_drive_t* sas_flash_drive, fbe_packet_t* packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(payload);

    fbe_payload_cdb_build_tur(payload_cdb_operation, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT);

    fbe_transport_set_completion_function(packet, sas_flash_drive_sanitize_tur_completion, sas_flash_drive);

    status = fbe_ssp_transport_send_functional_packet(&sas_flash_drive->sas_physical_drive.ssp_edge, packet);

    return status;
}

static fbe_status_t 
sas_flash_drive_sanitize_tur_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_physical_drive_sanitize_info_t * get_sanitize_status = NULL;
    fbe_status_t status;
    fbe_scsi_error_code_t error = 0;

    sas_flash_drive = (fbe_sas_flash_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
    fbe_payload_control_get_buffer(payload_control_operation, &get_sanitize_status);


    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t*)sas_flash_drive, packet, payload_cdb_operation, &error, NULL, NULL);

        if (error == 0)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_INFO,
                                                        "%s sanitize not in progress\n", __FUNCTION__);
            sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_OK;
            sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_percent = 0xffff;
        }
        else if (error == FBE_SCSI_CC_FORMAT_IN_PROGRESS)  
        {
            fbe_u8_t * sense_data = NULL;
            fbe_bool_t is_sksv_set = FBE_FALSE;

            sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS;

            /* get percent complete. */
            fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
            fbe_payload_cdb_parse_sense_data_for_sanitization_percent(sense_data, &is_sksv_set, &sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_percent);
            if(!is_sksv_set) /* percent not valid */
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_WARNING,
                                                           "%s SKSV bit not set.  Must be bug in drive fw. set prcnt=0\n", __FUNCTION__);
                sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_percent = 0;  /* indicates inprogress, but SKSV bit not set.  No way to now how far along we are. */
            }
            
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_INFO,
                                                       "%s sanitize in progress. percent:0x%04x/0x%04x \n", 
                                                       __FUNCTION__, sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_percent, FBE_DRIVE_MAX_SANITIZATION_PERCENT);            
        }
        else if (error == FBE_SCSI_CC_SANITIZE_INTERRUPTED) 
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_INFO,
                                                                   "%s sanitize needs restart \n", __FUNCTION__);
            sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_RESTART;
        }
        else  /* An unexpected error */
        {
            /* For any unexpected error (e.g. port error) we will report IN_PROGRESS.  This will cause PVD to retry.
             */
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s cmd failed. error:0x%x\n", __FUNCTION__, error);

            sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS;
            /* sanitize percentage will be whatever was last reported.*/
        }
    }

    get_sanitize_status->status = sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_status;
    get_sanitize_status->percent = sas_flash_drive->sas_physical_drive.sas_drive_info.sanitize_percent;

    fbe_payload_control_set_status (payload_control_operation, 
        (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_payload_ex_release_cdb_operation(payload, payload_cdb_operation);

    return FBE_STATUS_OK;
}

