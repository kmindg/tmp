/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_paddlecard_drive_executer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains executer functions for sata paddlecard drive. 
 * 
 * HISTORY
 *   1/05/2011:  Created. Wayne Garrett
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_transport_memory.h"
#include "sas_physical_drive_private.h"
#include "sata_paddlecard_drive_private.h"


/*************************
 *   PROTOTYPES
 *************************/
static fbe_status_t sata_paddlecard_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_sas_drive_status_t fbe_sata_paddlecard_drive_process_vpd_inquiry(fbe_sata_paddlecard_drive_t * sata_paddlecard_drive, fbe_u8_t *vpd_inq);


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_sata_paddlecard_drive_event_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_event_type_t event_type,
 *            fbe_event_context_t event_context)
 ****************************************************************
 * @brief
 *   This is the event entry function for sata paddlecard drive.
 *  
 * @param object_handle - object handle. 
 * @param event_type - event type.
 * @param event_context - event context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/05/2011 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_paddlecard_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sata_paddlecard_drive_t * sata_paddlecard_drive = NULL;
    fbe_status_t status;

    sata_paddlecard_drive = (fbe_sata_paddlecard_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)sata_paddlecard_drive,
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
 * @fn fbe_sata_paddlecard_drive_io_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the event entry function for sata paddlecard drive.
 *  
 * @param object_handle - object handle. 
 * @param packet - pointer to packet.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/05/2011 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_paddlecard_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
     return fbe_sas_physical_drive_io_entry(object_handle, packet);
}

/*!**************************************************************
 * @fn fbe_sata_paddlecard_drive_send_vpd_inquiry(
 *            fbe_sata_paddlecard_drive_t * sata_paddlecard_drive,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sata_paddlecard_drive - pointer to sata paddlecard drive object.
 * @param packet - pointer to packet. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/10/2011 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_paddlecard_drive_send_vpd_inquiry(fbe_sata_paddlecard_drive_t * sata_paddlecard_drive, fbe_packet_t * packet)
{
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_control_operation = fbe_payload_ex_get_control_operation(payload);
    if(payload_control_operation == NULL)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_paddlecard_drive,
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
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_paddlecard_drive,
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
        (sata_paddlecard_drive->vpd_pages_info.supported_vpd_pages)[sata_paddlecard_drive->vpd_pages_info.vpd_page_counter]);

    fbe_transport_set_completion_function(packet, sata_paddlecard_drive_send_vpd_inquiry_completion, sata_paddlecard_drive);

    status = fbe_ssp_transport_send_functional_packet(&((fbe_sas_physical_drive_t *)sata_paddlecard_drive)->ssp_edge, packet);
    return status;
}

/*!**************************************************************
 * @fn sata_paddlecard_drive_send_vpd_inquiry_completion(
 *            fbe_sata_paddlecard_drive_t * sata_paddlecard_drive,
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
 *  1/10/2011 - Created. Wayne Garrett
 *
 ****************************************************************/
static fbe_status_t 
sata_paddlecard_drive_send_vpd_inquiry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_paddlecard_drive_t * sata_paddlecard_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_scsi_error_code_t error = 0;
    fbe_sas_drive_status_t validate_status = FBE_SAS_DRIVE_STATUS_OK;
    fbe_u32_t transferred_count;     

    sata_paddlecard_drive = (fbe_sata_paddlecard_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_paddlecard_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        sas_physical_drive_get_scsi_error_code((fbe_sas_physical_drive_t *)sata_paddlecard_drive, packet, payload_cdb_operation, &error, NULL, NULL);
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
            validate_status = fbe_sata_paddlecard_drive_process_vpd_inquiry(sata_paddlecard_drive, (fbe_u8_t *)(sg_list[0].address));
            if (validate_status == FBE_SAS_DRIVE_STATUS_INVALID) 
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_paddlecard_drive, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      "%s Invalid packet status: 0x%X\n",
                                      __FUNCTION__, status);

                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sata_paddlecard_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sata_paddlecard_drive,   
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_paddlecard_drive, 
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

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sata_paddlecard_drive_process_vpd_inquiry(
 *            fbe_sata_paddlecard_drive_t * sata_paddlecard_drive,
 *            fbe_u8_t *vpd_inq)
 ****************************************************************
 * @brief
 *   This is the function to send a VPD inquiry packet.
 *  
 * @param sata_paddlecard_drive - pointer to sata paddlecard drive object.
 * @param vpd_inq - pointer to inquiry string. 
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/10/2011 - Created. Wayne Garrett
 *
 ****************************************************************/
static fbe_sas_drive_status_t 
fbe_sata_paddlecard_drive_process_vpd_inquiry(fbe_sata_paddlecard_drive_t * sata_paddlecard_drive, fbe_u8_t *vpd_inq)
{
    fbe_u8_t vpd_page_code;
    fbe_u8_t vpd_max_supported_pages;
    fbe_u8_t page_counter;
    fbe_u8_t i;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sata_paddlecard_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;

    vpd_page_code = *(vpd_inq + FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_CODE_OFFSET);
    switch (vpd_page_code) {
        case FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_00:
            /* Get the number of VPD pages supported */
            vpd_max_supported_pages = *(vpd_inq + FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_00_MAX_SUPPORTED_PAGES_OFFSET);

            /* "vpd_inq" will point to the first supported page */
            vpd_inq += FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_00_SUPPORTED_PAGES_START_OFFSET;

            /* Store the list of supported VPD pages */
            /* Currently only C0 and D2 are supported */
            page_counter = 0;
            for (i = 0; (i < vpd_max_supported_pages) && (page_counter < FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_MAX_PAGES); i++)
            {
                if ((*(vpd_inq+i) == FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_00) || 
                    (*(vpd_inq+i) == FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_C0) ||
                    (*(vpd_inq+i) == FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_D2))
                {
                    sata_paddlecard_drive->vpd_pages_info.supported_vpd_pages[page_counter] = *(vpd_inq+i);
                    page_counter++;
                    sata_paddlecard_drive->vpd_pages_info.vpd_max_supported_pages++;
                }
            }

            break;

        case FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_C0:
                        
            /* Extract out what we need from VPD page 0xC0 */ 
            vpd_inq += FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_C0_PROD_REV_OFFSET;
                
            /* Copy the ASCII format Product Revision into the appropriate location
             * of dl_product_rev.
             */
            info_ptr->revision[4] = '/';
            strncpy(&info_ptr->revision[5], vpd_inq, 4);
            info_ptr->product_rev_len = FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE;  /* of form "DDDD/BBBB" */
            
            /* Seagate SATA drives format the revision as X.YYY.
             * If the revision has a ".", then remove it and add the X value in it's place
             */
            if(info_ptr->revision[0] == '.')
            {
                info_ptr->revision[0] = vpd_inq[115];
            }
 
            break;

        case FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_PAGE_D2:
           
            /* Extract out what we need from VPD page 0xD2 */ 
            vpd_inq += FBE_SATA_PADDLECARD_SCSI_INQUIRY_VPD_D2_BRIDGE_HW_REV_OFFSET;
            /* Copy the Brige H/W revision into dl_bridge_hw_rev. */
            strncpy(info_ptr->bridge_hw_rev, vpd_inq, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE);

            break;

        default:
            return FBE_SAS_DRIVE_STATUS_INVALID;
            break;
    }

    sata_paddlecard_drive->vpd_pages_info.vpd_page_counter++;

    if (sata_paddlecard_drive->vpd_pages_info.vpd_page_counter < sata_paddlecard_drive->vpd_pages_info.vpd_max_supported_pages)
    {
        return FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING;
    }

    return FBE_STATUS_OK;
}

