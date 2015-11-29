/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_drive_configuration_service_download.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the drive firmware download service.
 *
 * @version
 *   11/16/2011:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe/fbe_drive_configuration_download.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_drive_configuration_service_private.h"
#include "fbe_drive_configuration_service_download_private.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe_notification.h"


/* Currently we allow one download process at a time. 
 * It should be easy to expand to multiple processes. 
 */
static fbe_drive_configuration_download_process_t drive_download_info;



/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 
static fbe_status_t fbe_drive_configuration_init_download(fbe_drive_configuration_download_process_t * fwdl_process);
static fbe_status_t fbe_drive_configuration_init_download_process(fbe_drive_configuration_download_process_t *fwdl_process,
                                                                  fbe_drive_configuration_control_fw_info_t * fw_info);
static fbe_status_t fbe_drive_configuration_get_drive_download_info(fbe_physical_drive_get_download_info_t * get_download_p, fbe_object_id_t object_id);
static fbe_bool_t fbe_drive_configuration_download_check_tla(fbe_u8_t *fw_tla, fbe_u8_t *drive_tla);
static fbe_bool_t fbe_drive_configuration_download_check_vendor(fbe_u8_t *fw_vendor, fbe_u8_t *drive_vendor);
static fbe_bool_t fbe_drive_configuration_download_check_product_id(fbe_u8_t *fw_product, fbe_u8_t *drive_product);
static fbe_bool_t fbe_drive_configuration_download_check_revision(fbe_u8_t *fw_rev, fbe_u8_t *drive_rev);
static fbe_bool_t fbe_drive_configuration_download_is_drive_selected(fbe_drive_selected_element_t *element, fbe_u8_t bus,
                                      fbe_u8_t encl, fbe_u8_t slot, fbe_u16_t num_drives);
static void         dcs_download_notify_drive_state_changed(const fbe_drive_configuration_download_drive_t *drive_p);
static fbe_status_t fbe_drive_configuration_start_download(fbe_drive_configuration_download_process_t * fwdl_process);
static fbe_status_t fbe_drive_configuration_send_download_to_pdo(fbe_drive_configuration_download_process_t * fwdl_process,
                                             fbe_object_id_t object_id);
static fbe_status_t fbe_drive_configuration_send_download_to_pdo_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_drive_configuration_finish_download(fbe_drive_configuration_download_process_t * fwdl_process);
static fbe_status_t fbe_drive_configuration_abort_download(fbe_drive_configuration_download_process_t * fwdl_process);
static fbe_status_t fbe_drive_configuration_send_abort_to_pdo(fbe_object_id_t object_id);
static fbe_status_t fbe_drive_configuration_send_abort_to_pdo_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t dcs_download_allocate_image_and_copy(fbe_u8_t * image_ptr, fbe_u32_t allocation_size_in_bytes, fbe_sg_element_t* sg_list, fbe_u32_t *num_elements);
static void         dcs_download_release_image(fbe_sg_element_t* sg_list);

/*!**************************************************************
 * @fn fbe_drive_configuration_service_initialize_download
 ****************************************************************
 * @brief
 * ' This function initializes download during service initialization. 
 * 
 * @param none.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_service_initialize_download(void)
{
    /* Initialize download lock. */
    fbe_spinlock_init(&drive_download_info.download_lock);
    drive_download_info.max_dl_count =  FBE_DCS_MAX_CONFIG_DL_DRIVES;

    drive_download_info.num_image_sg_elements = 0;
    fbe_sg_element_terminate(&drive_download_info.image_sgl[0]);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_service_initialize_download() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_service_destroy_download
 ****************************************************************
 * @brief
 *  This function destroys download during service. 
 * 
 * @param none.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   02/06/2012:  nqiu - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_service_destroy_download(void)
{
    /* destroy download lock. */
    fbe_spinlock_destroy(&drive_download_info.download_lock);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_service_destroy_download() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_download_firmware
 ****************************************************************
 * @brief
 *  This function handles download request from user. 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_download_firmware(fbe_packet_t *packet)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_payload_control_status_t                            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_sg_element_t *                                      sg_list;
    fbe_u32_t                                               sg_list_count;
    fbe_drive_configuration_download_process_t *            fwdl_process = NULL;    
    fbe_drive_configuration_control_fw_info_t               fw_info;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   
    status = fbe_payload_ex_get_sg_list(payload, &sg_list, &sg_list_count);
    if (sg_list == NULL)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: sg list NULL\n");
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the right download process. For now we support one process only. */
    fwdl_process = &drive_download_info;
    fbe_spinlock_lock (&fwdl_process->download_lock);
    if (fwdl_process->in_progress)
    {
        fbe_spinlock_unlock (&fwdl_process->download_lock);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: Another download is in process.\n");

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fwdl_process->in_progress = FBE_TRUE;
    /*Initializing to a sane value when the fw download is initiated.*/
    fwdl_process->cleanup_in_progress = FBE_FALSE;
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Handle download request */
    fw_info.header_image_p = (fbe_fdf_header_block_t *)sg_list->address;
    fw_info.header_size = sg_list->count;
    fbe_sg_element_increment(&sg_list);
    fw_info.data_image_p = sg_list->address;
    fw_info.data_size = sg_list->count;


    /* First, initialize download process structure */
    status = fbe_drive_configuration_init_download_process(fwdl_process, &fw_info);
    if (status != FBE_STATUS_OK)
    {
        /* fail the whole process */
        fwdl_process->fail_abort = FBE_TRUE;
        fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
        fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_MISSING_TLA;

        /* TLA and fw_rev will not be updated in fwdl_process yet.
         * We do not want to log wrong information. So leave these fields off the trace.
         */
        fbe_event_log_write(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED, NULL, 0, "%s %s", 
                                        ".", ".");

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS download: Inability to initialize fwdl_process structure due to lack of memory, status=%d\n", status);
            control_status = FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS download: Inability to initialize fwdl_process structure due to corruption of firmware file, status=%d\n", status);
            /* We only want to complete the packet with a bad status, when it's a serious internal error */
            status = FBE_STATUS_OK;
        }

        fbe_drive_configuration_finish_download(fwdl_process);

        /* Complete the control packet before we exit this function */
        fbe_payload_control_set_status (control_operation, control_status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
    }
    else
    {
        fbe_event_log_write(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED, NULL, 0, "%s %s", 
                                        fwdl_process->tla_number, fwdl_process->fw_rev);

        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: file=%s\n", fwdl_process->fdf_filename);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: tla=%s, rev=%s\n",
                                  fwdl_process->tla_number, fwdl_process->fw_rev);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: cflags=0x%x, cflags2=0x%x\n", 
                                  fwdl_process->header_ptr->cflags, fwdl_process->header_ptr->cflags2);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: trial_run=%s, fast_download=%s fail_nonqual=%s force_download=%s\n",
                                  (fwdl_process->trial_run ? "TRUE" : "FALSE"), 
                                  (fwdl_process->fast_download ? "TRUE" : "FALSE"), 
                                  (fwdl_process->fail_nonqualified ? "TRUE" : "FALSE"), 
                                  (fwdl_process->force_download ? "TRUE" : "FALSE"));

        status = fbe_drive_configuration_init_download(fwdl_process);
        if (status != FBE_STATUS_OK)
        {
            if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
            {
                drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS download: Inability to obtain list of target PDOs due to lack of memory, status=%d\n", status);
                control_status = FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES;
            }
            else if ((status != FBE_STATUS_NO_OBJECT) &&
                     (status != FBE_STATUS_NO_DEVICE))
            {
                drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS download: Inability to obtain list of target PDOs due to serious issue other than lack of memory, status=%d\n", status);
                control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            }
            else
            {
                drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS download: Either zero PDOs discovered or none found to match criteria of firmware file header flags, status=%d\n", status);
                /* We only want to complete the packet with a bad status, when it's a serious internal error */
                status = FBE_STATUS_OK;
            }

            fbe_drive_configuration_finish_download(fwdl_process);

            /* Complete the control packet before we exit this function */
            fbe_payload_control_set_status (control_operation, control_status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
        }
        else
        {
            /* Complete the control packet before we start the download */
            fbe_payload_control_set_status (control_operation, control_status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);

            /* Start download only if we didn't encounter an error during the initialization process */
            fbe_drive_configuration_start_download(fwdl_process);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_control_download_firmware() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_abort_download
 ****************************************************************
 * @brief
 *  This function handles abort_download request from user. 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_abort_download(fbe_packet_t *packet)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                                         payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_download_process_t *            fwdl_process;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    /* Find the process we are looking for. */
    fwdl_process = &drive_download_info;

    /* Handle abort request. */
    status = fbe_drive_configuration_abort_download(fwdl_process);

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_control_abort_download() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_get_download_process_info
 ****************************************************************
 * @brief
 *  This function handles request to get download process information. 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/22/2011:  chenl6 - Created.
 *   07/17/2012:  Added support to return operation type as part
 *                of the download process information. Darren Insko
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_download_process_info(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                         payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_download_get_process_info_t  *  get_process_info = NULL;
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_drive_configuration_download_process_t *            fwdl_process;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer(control_operation, &get_process_info); 
    if(get_process_info == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_download_get_process_info_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(get_process_info, sizeof(fbe_drive_configuration_download_get_process_info_t));

    /* Find the right download process. For now we support one process only. */
    fwdl_process = &drive_download_info;
    fbe_spinlock_lock (&fwdl_process->download_lock);
    get_process_info->in_progress = fwdl_process->in_progress;
    get_process_info->state = fwdl_process->process_state;
    get_process_info->fail_code = fwdl_process->process_fail_reason;
    get_process_info->operation_type = fwdl_process->operation_type;
    get_process_info->start_time = fwdl_process->start_time;
    get_process_info->stop_time = fwdl_process->stop_time;
    get_process_info->num_qualified = fwdl_process->num_qualified_drives;
    get_process_info->num_complete = (fbe_u32_t)fwdl_process->num_done_drives;
    fbe_copy_memory(get_process_info->fdf_filename, fwdl_process->fdf_filename, FBE_FDF_FILENAME_LEN);
    get_process_info->fdf_filename[FBE_FDF_FILENAME_LEN - 1] = '\0';
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Complete control packet now */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_control_get_download_process_info() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_get_download_drive_info
 ****************************************************************
 * @brief
 *  This function handles request to get download drive information. 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/22/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_download_drive_info(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                         payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_download_get_drive_info_t  *    get_drive_info = NULL;
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_drive_configuration_download_process_t *            fwdl_process;
    fbe_u32_t                                               i;
    fbe_drive_configuration_download_drive_t *              drive;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer(control_operation, &get_drive_info); 
    if(get_drive_info == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_download_get_drive_info_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the right download process. For now we support one process only. */
    fwdl_process = &drive_download_info;
    fbe_spinlock_lock (&fwdl_process->download_lock);
    get_drive_info->state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_IDLE;
    get_drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE;
    for (i = 0; i < fwdl_process->num_qualified_drives; i++)
    {
        drive = &fwdl_process->drives[i];
        if (drive->location.bus == get_drive_info->bus &&
            drive->location.enclosure == get_drive_info->enclosure &&
            drive->location.slot == get_drive_info->slot)
        {
            get_drive_info->state = drive->dl_state;
            get_drive_info->fail_code = drive->fail_code;
            fbe_copy_memory(&get_drive_info->prior_product_rev, &drive->prior_product_rev, 
                            FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
            (get_drive_info->prior_product_rev)[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';

            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS get_download_drive_info: Drive %d_%d_%d matches drive #%d state: %x fail_code: %x\n",
                                      get_drive_info->bus, get_drive_info->enclosure, get_drive_info->slot, i,
                                      get_drive_info->state, get_drive_info->fail_code);
            break;
        }
    }
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Complete control packet now */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_control_get_download_drive_info() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_init_download
 ****************************************************************
 * @brief
 *  This function initializes download process. 
 * 
 * @param fwdl_process - pointer to process structure.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *   05/20/2013:  Moved call to init_download_process to
 *                control_download_firmware to improve error handling
 *                when contents of firmware file does not match the
 *                upgrade behavior that's desired. Darren Insko
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_init_download(fbe_drive_configuration_download_process_t * fwdl_process)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_fdf_header_block_t *                                header_ptr = NULL;
    fbe_fdf_serviceability_block_t *                        trailer_ptr = NULL;
    fbe_u32_t                                               offset;
    fbe_bool_t                                              check_tla = FBE_FALSE;
    fbe_bool_t                                              check_rev = FBE_FALSE;
    fbe_bool_t                                              select_drives = FBE_FALSE;
    fbe_drive_selected_element_t *                          first_element = NULL;
    fbe_u32_t                                               object_idx = 0;
    fbe_topology_control_get_physical_drive_objects_t *     get_drive_objects = NULL;
    fbe_physical_drive_get_download_info_t                  get_download_info;
    fbe_drive_configuration_download_drive_t *              current_drive = &fwdl_process->drives[0];


    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    header_ptr = (fbe_fdf_header_block_t *)fwdl_process->header_ptr;

    /* Second, select eligible drives */
    offset = header_ptr->trailer_offset;
    trailer_ptr = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)header_ptr + offset);
    /* For each drive:
     * 1. selected or not;
     * 2. check TLA;
     * 3. check fw rev
     * 4. check vendor ID;
     * 5. check product ID;
     */
    if (header_ptr->cflags2 & FBE_FDF_CFLAGS2_CHECK_TLA)
    {
        check_tla = FBE_TRUE;
    }
    if (header_ptr->cflags & FBE_FDF_CFLAGS_CHECK_REVISION)
    {
        check_rev = FBE_TRUE;
    }
    if (header_ptr->cflags & FBE_FDF_CFLAGS_SELECT_DRIVE)
    {
        select_drives = FBE_TRUE;
        first_element = &header_ptr->first_drive; 
    }

    get_drive_objects = (fbe_topology_control_get_physical_drive_objects_t *)
        fbe_memory_native_allocate(sizeof(fbe_topology_control_get_physical_drive_objects_t));
    if (get_drive_objects == NULL) {
        fwdl_process->fail_abort = FBE_TRUE;
        fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
        fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_GENERIC_FAILURE;
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Get the total number of objects */
    get_drive_objects->number_of_objects = 0;
    status = fbe_drive_configuration_get_physical_drive_objects(get_drive_objects);

    if (status != FBE_STATUS_OK)
    {
        fwdl_process->fail_abort = FBE_TRUE;
        fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
        fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_GENERIC_FAILURE;
        fbe_memory_native_release(get_drive_objects);
        return status;
    }

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS init_download: number_of_objects=%d to check if qualified\n",
                              get_drive_objects->number_of_objects);

    if (get_drive_objects->number_of_objects == 0)
    {
        if (fwdl_process->trial_run)
        {
            fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL;
            fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
        }
        else
        {
            fwdl_process->fail_abort = FBE_TRUE;
            fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
            fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE;
        }
        fbe_memory_native_release(get_drive_objects);
        return FBE_STATUS_NO_OBJECT;
    }

    /* For each object, get the drive information */
    for (object_idx = 0; object_idx < get_drive_objects->number_of_objects; object_idx++)
    {
        status = fbe_drive_configuration_get_drive_download_info(&get_download_info, get_drive_objects->pdo_list[object_idx]);
        if (status != FBE_STATUS_OK)
        {
            continue;
        }
        /* Selected? */
        if (select_drives)
        {
            if (fwdl_process->num_qualified_drives >= header_ptr->num_drives_selected)
            {
                drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS init_download: Stopping process init. num_qualified_drives(%d) >= num_drives_selected(%d)\n",
                                          fwdl_process->num_qualified_drives, header_ptr->num_drives_selected);
                break;
            }
            if (fbe_drive_configuration_download_is_drive_selected(first_element, 
                                                      get_download_info.port, 
                                                      get_download_info.enclosure,
                                                      (fbe_u8_t)get_download_info.slot, 
                                                      header_ptr->num_drives_selected) == FBE_FALSE)
            {
                drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS init_download: skipping %d_%d_%d not selected\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot);
                continue;
            }
        }
        /* Check TLA */
        if (check_tla && 
            (fbe_drive_configuration_download_check_tla(trailer_ptr->tla_number, get_download_info.tla_part_number) == FBE_FALSE))
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS init_download: skipping %d_%d_%d check_tla:%s\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot,
                                      check_tla?"true":"false");
            continue;
        }
        /* Check fw rev */
        if (check_rev && 
            (fbe_drive_configuration_download_check_revision(fwdl_process->fw_rev, get_download_info.product_rev) == FBE_TRUE))
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS init_download: skipping %d_%d_%d check_rev:%s\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot,
                                      check_rev?"true":"false");
            continue;
        }
        /* Check vendor */
        if (fbe_drive_configuration_download_check_vendor(header_ptr->vendor_id, get_download_info.vendor_id) == FBE_FALSE)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS init_download: skipping %d_%d_%d vendor ID mismatch\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot);
            continue;
        }
        /* Check product id */
        if (fbe_drive_configuration_download_check_product_id(header_ptr->product_id, get_download_info.product_id) == FBE_FALSE)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS init_download: skipping %d_%d_%d product ID mismatch\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot);
            continue;
        }

        /* This is a qualified drive. */
        drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS init_download: %d_%d_%d qualifies\n", get_download_info.port, get_download_info.enclosure, get_download_info.slot);

        current_drive->object_id = get_drive_objects->pdo_list[object_idx];
        current_drive->location.bus = get_download_info.port;
        current_drive->location.enclosure = get_download_info.enclosure;
        current_drive->location.slot = (fbe_u8_t)get_download_info.slot;
        current_drive->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT;        
        fbe_copy_memory(&(current_drive->prior_product_rev), &(get_download_info.product_rev), FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
        (current_drive->prior_product_rev)[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';
   
        dcs_download_notify_drive_state_changed(current_drive);        

        fwdl_process->num_qualified_drives++;
        current_drive++;
    }

    fbe_memory_native_release(get_drive_objects);

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS init_download: num_qualified_drives=%d\n",
                              fwdl_process->num_qualified_drives);

    /* Only in the case where we're not in a trial run and no drives were qualified to receive the
     * firmware upgrade should we fail the overall download process.
     */
    if (fwdl_process->num_qualified_drives == 0)
    {
        if (fwdl_process->trial_run)
        {
            fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL;
            fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
        }
        else
        {
            fwdl_process->fail_abort = FBE_TRUE;
            fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
            fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE;
        }
        return FBE_STATUS_NO_DEVICE;
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_init_download() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_init_download_process
 ****************************************************************
 * @brief
 *  This function initializes download process structure. 
 * 
 * @param fwdl_process - pointer to process structure.
 * @param fw_info - pointers to download information.  
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *   07/17/2012:  Added support to set the operation type in the
 *                download process structure. Darren Insko
 *   05/20/2013:  Moved initialization of the fwdl_process to the
 *                beginning so we don't have any stale data left
 *                over from the previous execution. Darren Insko
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_init_download_process(fbe_drive_configuration_download_process_t *fwdl_process,
                                                           fbe_drive_configuration_control_fw_info_t * fw_info)
{
    fbe_fdf_serviceability_block_t *   trailer_ptr = NULL;
    fbe_fdf_header_block_t *           header_ptr = NULL;
    fbe_u32_t                          offset;
    fbe_status_t                       status;
    fbe_u32_t                          i;

    /* Initialize download process structure. */
    fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_RUNNING;
    fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
    fwdl_process->operation_type = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_NOP;
    fwdl_process->fail_abort = FBE_FALSE;
    fwdl_process->trial_run = FBE_FALSE;
    fwdl_process->fast_download = FBE_FALSE;
    fwdl_process->fail_nonqualified = FBE_FALSE;
    fwdl_process->force_download = FBE_FALSE;
    fwdl_process->num_qualified_drives = 0;
    fwdl_process->num_done_drives = 0;
    fbe_zero_memory(fwdl_process->fw_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
    fbe_zero_memory(fwdl_process->fdf_filename, FBE_FDF_FILENAME_LEN);

    for (i=0; i<FBE_MAX_DOWNLOAD_DRIVES; i++)
    {
        fwdl_process->drives[i].object_id = FBE_OBJECT_ID_INVALID;
        fwdl_process->drives[i].location.bus = FBE_INVALID_PORT_NUM;
        fwdl_process->drives[i].location.enclosure = FBE_INVALID_ENCL_NUM;
        fwdl_process->drives[i].location.slot = FBE_INVALID_SLOT_NUM;
        fwdl_process->drives[i].dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_IDLE;
        fwdl_process->drives[i].fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE;
        fwdl_process->drives[i].prior_product_rev[0] = '\0';
    }     
    fbe_get_system_time(&fwdl_process->start_time);

    /* Allocate header */
    fwdl_process->header_ptr = fbe_memory_native_allocate(fw_info->header_size);
    if (fwdl_process->header_ptr == NULL)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Could not allocate header.\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_copy_memory(fwdl_process->header_ptr, fw_info->header_image_p, fw_info->header_size);

    header_ptr = (fbe_fdf_header_block_t *)fwdl_process->header_ptr;

    /* Verify header */
    if (header_ptr->header_marker != FBE_FDF_HEADER_MARKER)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Header corrupted.\n", __FUNCTION__);
        fbe_memory_native_release(fwdl_process->header_ptr);
        fwdl_process->header_ptr = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (header_ptr->image_size != fw_info->data_size)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Image corrupted size. expected:%d actual:%d.\n", __FUNCTION__, header_ptr->image_size, fw_info->data_size);
        fbe_memory_native_release(fwdl_process->header_ptr);
        fwdl_process->header_ptr = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy image */
    status = dcs_download_allocate_image_and_copy(fw_info->data_image_p, fw_info->data_size, &fwdl_process->image_sgl[0], &fwdl_process->num_image_sg_elements);

    if (status != FBE_STATUS_OK)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Could not allocate image.\n", __FUNCTION__);
        fbe_memory_native_release(fwdl_process->header_ptr);
        fwdl_process->header_ptr = NULL;
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    offset = header_ptr->trailer_offset;
    trailer_ptr = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)header_ptr + offset);

    /* Now that we know we have enough memory to store the header image and we know it's not corrupt,
       extract the filename and flags from the header and bring them into the download process structure. */
    fbe_copy_memory(fwdl_process->fdf_filename, header_ptr->ofd_filename, FBE_FDF_FILENAME_LEN);
    fwdl_process->fdf_filename[FBE_FDF_FILENAME_LEN] = '\0';
    fwdl_process->fail_nonqualified = (header_ptr->cflags & FBE_FDF_CFLAGS_FAIL_NONQUALIFIED)? FBE_TRUE : FBE_FALSE;
    fwdl_process->force_download = (header_ptr->cflags2 & FBE_FDF_CFLAGS2_FORCE)? FBE_TRUE : FBE_FALSE;

    if ((header_ptr->cflags2 & FBE_FDF_CFLAGS2_TRIAL_RUN) ||
        ((header_ptr->cflags & FBE_FDF_CFLAGS_SELECT_DRIVE) &&
        (header_ptr->num_drives_selected == 0)))
    {
        /* Clearing the SELECT_DRIVE bits here will cause all drives to be searched when
         * when determining what drives are qualified to receive a firmware upgrade.  This
         * behavior occurs by preventing a search for a specific list of selected drive
         * locations that exists within the header of the firmware download process.
        */
        header_ptr->cflags &= ~FBE_FDF_CFLAGS_SELECT_DRIVE;
        fwdl_process->trial_run = FBE_TRUE;
        fwdl_process->operation_type = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_TRIAL_RUN;
    }
    else if (header_ptr->cflags2 & FBE_FDF_CFLAGS2_FAST_DOWNLOAD)
    {
        fwdl_process->fast_download = FBE_TRUE;
        fwdl_process->operation_type = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_FAST_UPGRADE;
    }
    else
    {
        fwdl_process->operation_type = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_OP_UPGRADE;
    }

    /* Extract the firmware revision from the image */
    if (!(header_ptr->cflags & FBE_FDF_CFLAGS_TRAILER_PRESENT))
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_FDF_CFLAGS_TRAILER_PRESENT not set.\n", __FUNCTION__);
        fbe_memory_native_release(fwdl_process->header_ptr);
        fwdl_process->header_ptr = NULL;
        dcs_download_release_image(&fwdl_process->image_sgl[0]);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    /* Verify trailer */
    if ( trailer_ptr->trailer_marker != FBE_FDF_TRAILER_MARKER )
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_FDF_TRAILER_MARKER not found.\n", __FUNCTION__);
        fbe_memory_native_release(fwdl_process->header_ptr);
        fwdl_process->header_ptr = NULL;
        dcs_download_release_image(&fwdl_process->image_sgl[0]);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Extract the firmware rev and TLA number from the trailer and bring them into the download process structure */
    fbe_copy_memory(fwdl_process->fw_rev, trailer_ptr->fw_revision, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);     
    fwdl_process->fw_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';
    if (fwdl_process->fw_rev[4] != '/')
    {
        fbe_zero_memory(&fwdl_process->fw_rev[4],(FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE - 4));
    }

    fbe_copy_memory(fwdl_process->tla_number, trailer_ptr->tla_number, FBE_FDF_TLA_SIZE);
    fwdl_process->tla_number[FBE_FDF_TLA_SIZE] = '\0';

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_init_download_process() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_configuration_get_drive_download_info()
 ****************************************************************
 * @brief
 *  Get the total number of objects of a given class.
 *
 * @param get_download_p - Ptr to get structure.               
 * @param object_id - PDO id.               
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_drive_configuration_get_drive_download_info(fbe_physical_drive_get_download_info_t * get_download_p, 
                                                fbe_object_id_t object_id)
{
    fbe_status_t                                            status;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                       control_p = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO,
                                        get_download_p,
                                        sizeof(fbe_physical_drive_get_download_info_t));
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_release_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_drive_configuration_get_drive_download_info()
 ******************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_download_check_tla
 ****************************************************************
 * @brief
 *  This function checks whether TLA on a drive and fw is the same. 
 ****************************************************************/
static fbe_bool_t 
fbe_drive_configuration_download_check_tla(fbe_u8_t *fw_tla, fbe_u8_t *drive_tla)
{
    fbe_u32_t                                               index;

    for (index = 0; index < (FBE_FDF_TLA_SIZE-3); index++)
    {
        /* If the header block TLA_number character == 0xff this means
         * this is a wild card character and should not be compared.
         */
        if ((fw_tla[index] != 0xff) && 
            (fw_tla[index] != drive_tla[index]))
        {
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_download_check_vendor
 ****************************************************************
 * @brief
 *  This function checks whether vendor on a drive and fw is the same. 
 ****************************************************************/
static fbe_bool_t 
fbe_drive_configuration_download_check_vendor(fbe_u8_t *fw_vendor, fbe_u8_t *drive_vendor)
{
    fbe_u32_t                                               index;

    for (index = 0; index < FBE_SCSI_INQUIRY_VENDOR_ID_SIZE; index++)
    {
        if ( (fw_vendor[index] != 0xff) &&
             (fw_vendor[index] != drive_vendor[index]) )
        {
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_download_check_product_id
 ****************************************************************
 * @brief
 *  This function checks whether pid on a drive and fw is the same. 
 ****************************************************************/
static fbe_bool_t 
fbe_drive_configuration_download_check_product_id(fbe_u8_t *fw_product, fbe_u8_t *drive_product)
{
    fbe_u32_t                                               index;

    for (index = 0; index < 8; index++)
    {
        if ( (fw_product[index] != 0xff) &&
             (fw_product[index] != drive_product[index]) )
        {
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_download_check_revision
 ****************************************************************
 * @brief
 *  This function checks whether rev on a drive and fw is the same. 
 ****************************************************************/
static fbe_bool_t 
fbe_drive_configuration_download_check_revision(fbe_u8_t *fw_rev, fbe_u8_t *drive_rev)
{
    return (fbe_equal_memory(fw_rev, drive_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE));
}

/*!**************************************************************
 * @fn fbe_drive_configuration_download_is_drive_selected
 ****************************************************************
 * @brief
 *  This function checks whether a drive is selected by fw. 
 ****************************************************************/
static fbe_bool_t 
fbe_drive_configuration_download_is_drive_selected(fbe_drive_selected_element_t *element, fbe_u8_t bus,
                                      fbe_u8_t encl, fbe_u8_t slot, fbe_u16_t num_drives)
{
    fbe_u16_t                                               index;

    for (index = 0; index < num_drives; index++)
    {
        if ((element->bus == bus) && (element->enclosure == encl) && (element->slot == slot))
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS download: Drive %d_%d_%d matches drive #%d from selected drive #s 0-%d\n",
                                      bus, encl, slot, index, num_drives-1);
            return FBE_TRUE;
        }
        element++;
    }
    return FBE_FALSE;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_is_system_drive
 ****************************************************************
 * @brief
 *   This function check whether a drive is system drive
 *
 * @param
 *   drive_info_p - pointer to a download drive
 ****************************************************************/
static fbe_bool_t
fbe_drive_configuration_is_system_drive(fbe_drive_configuration_download_drive_t *drive)
{
    fbe_drive_selected_element_t *element = &drive->location;

    if ((element->bus == 0) && (element->enclosure == 0) && (element->slot < 4)) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_send_download
 ****************************************************************
 * @brief
 *  This function send download to a drive
 *
 * @param
 *   fwdl_process - pointer to process structure.
 *   drive_info   - to drive to download
 * @return status - The status of the operation.
 *
 * @version
 *   08/25/2014:  Created. Kang Jamin
 *
 ****************************************************************/
static fbe_status_t
fbe_drive_configuration_send_download(fbe_drive_configuration_download_process_t * fwdl_process,
                                      fbe_drive_configuration_download_drive_t *drive_info)
{
    fbe_status_t status;
    fbe_object_id_t object_id;

    object_id = drive_info->object_id;
    dcs_download_notify_drive_state_changed(drive_info);
    status = fbe_drive_configuration_send_download_to_pdo(fwdl_process, object_id);
    fbe_spinlock_lock (&fwdl_process->download_lock);
    if (status != FBE_STATUS_OK) {
        /* The packet was not successfully sent.  This code is not expected to handle
           a control operation failure, since the send_download_to_pdo_completion will
           handle it. */

        drive_configuration_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail, drive: %u_%u_%u, status: 0x%x\n",
            __FUNCTION__, drive_info->location.bus, drive_info->location.enclosure,
            drive_info->location.slot, status);

        /* verify request hasn't already been completed. Only valid states are ACTIVE */
        if (FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE != drive_info->dl_state) {
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS start_download: invalid state=%d pdo=0x%x\n",
                                      drive_info->dl_state, object_id);
        } else {
            drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
            drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_GENERIC_FAILURE;
            fbe_spinlock_unlock (&fwdl_process->download_lock);

            dcs_download_notify_drive_state_changed(drive_info);

            fbe_spinlock_lock (&fwdl_process->download_lock);
            /* for trial_run we continue with requests, else abort it. */
            if (!fwdl_process->trial_run) {
                fwdl_process->fail_abort = FBE_TRUE;
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_GENERIC_FAILURE;
            }
            fbe_atomic_increment(&fwdl_process->num_done_drives);
        }
    }
    fbe_spinlock_unlock(&fwdl_process->download_lock);

    return status;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_start_download
 ****************************************************************
 * @brief
 *  This function starts download process. 
 * 
 * @param fwdl_process - pointer to process structure.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_start_download(fbe_drive_configuration_download_process_t * fwdl_process)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_u32_t                                               index;
    fbe_drive_configuration_download_drive_t *              drive_info;
    fbe_bool_t                                              need_finish = FBE_FALSE;
    fbe_bool_t                                              is_system_drive_sent = FBE_FALSE;

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry, num_qualified_drives=%d  num_done_drives=%d\n",
                              __FUNCTION__, fwdl_process->num_qualified_drives, fwdl_process->num_done_drives);

    fbe_spinlock_lock (&fwdl_process->download_lock);
   /* For each object, sends download request to PDO */
    for (index = 0; index < fwdl_process->num_qualified_drives; index++)
    {
        drive_info = &fwdl_process->drives[index];

        /* It's possible the process was aborted either by user or PDO completion function while
           we are in the middle of sending the DL requests. Mark each drive as aborted and
           skip sending the request. 
         */
        if (fwdl_process->fail_abort)
        {
            /* For system drive, the dl_state may be set by PDO completion function.
             * So if dl_state isn't INIT, that means this drive has been processed and
             * we can't increase the count here.
             */
            if (fbe_drive_configuration_is_system_drive(drive_info) &&
                drive_info->dl_state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT) {
                drive_configuration_trace(
                    FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: skip sending notification for system drive %u_%u_%u\n",
                    __FUNCTION__, drive_info->location.bus, drive_info->location.enclosure,
                    drive_info->location.slot);
                continue;
            }

            drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
            drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED;
            fbe_atomic_increment(&fwdl_process->num_done_drives);

            fbe_spinlock_unlock (&fwdl_process->download_lock);
            dcs_download_notify_drive_state_changed(drive_info);
            fbe_spinlock_lock (&fwdl_process->download_lock);
            continue;
        }
        if (fbe_drive_configuration_is_system_drive(drive_info) && is_system_drive_sent)
        {
            drive_configuration_trace(
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: defer download for system drive %u_%u_%u\n",
                __FUNCTION__, drive_info->location.bus, drive_info->location.enclosure,
                drive_info->location.slot);
            continue;
        }

        drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE;        
        fbe_spinlock_unlock (&fwdl_process->download_lock);

        status = fbe_drive_configuration_send_download(fwdl_process, drive_info);

        fbe_spinlock_lock(&fwdl_process->download_lock);
        if (status == FBE_STATUS_OK && fbe_drive_configuration_is_system_drive(drive_info))
        {
            is_system_drive_sent = FBE_TRUE;
            drive_configuration_trace(
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: start download for system drive %u_%u_%u\n",
                __FUNCTION__, drive_info->location.bus, drive_info->location.enclosure,
                drive_info->location.slot);
        }
    }

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "DCS start_download: num_qualified_drives=%d  num_done_drives=%d\n",
                              fwdl_process->num_qualified_drives, fwdl_process->num_done_drives);

    /* We may end up none of the requests is sent to PDO */
    if (fwdl_process->num_done_drives == fwdl_process->num_qualified_drives)
    {
        need_finish = FBE_TRUE;
    }
    fbe_spinlock_unlock (&fwdl_process->download_lock);


    if (need_finish)
    {
        fbe_drive_configuration_finish_download(fwdl_process);
    }
    else if (fwdl_process->fail_abort)
    {
        fbe_drive_configuration_abort_download(fwdl_process);
    }


    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_start_download() 
 ******************************************************/

/*!****************************************************************************
 * fbe_drive_configuration_send_download_to_pdo()
 ******************************************************************************
 * @brief
 *  This function sends download request to PDO.
 *
 * @param fwdl_process - Ptr to download process.
 * @param object_id - PDO id.
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ******************************************************************************/
fbe_status_t
fbe_drive_configuration_send_download_to_pdo(fbe_drive_configuration_download_process_t * fwdl_process,
                                             fbe_object_id_t object_id)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_physical_drive_mgmt_fw_download_t *                 send_download_p;
    fbe_physical_drive_control_code_t                       command;
    fbe_cpu_id_t                                            cpu_id;
    fbe_fdf_header_block_t *                                header_ptr = (fbe_fdf_header_block_t *)fwdl_process->header_ptr;

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry, object_id: 0x%x\n", __FUNCTION__, object_id);

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: object_id 0x%x Error allocating packet\n", 
                                  __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    send_download_p = (fbe_physical_drive_mgmt_fw_download_t *)fbe_memory_native_allocate(sizeof (fbe_physical_drive_mgmt_fw_download_t));
    if (send_download_p == NULL) 
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: object_id 0x%x Error allocating buffer\n", 
                                  __FUNCTION__, object_id);
        fbe_transport_release_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fwdl_process->trial_run)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN\n");
        command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN;
    }
    else
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download: command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD\n");
        command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD;
    }
    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    /* Get usurper packet info */
    send_download_p->object_id = object_id;
    send_download_p->trial_run = fwdl_process->trial_run;
    send_download_p->fast_download = fwdl_process->fast_download;
    send_download_p->force_download = fwdl_process->force_download;
    send_download_p->retry_count = 0;
    send_download_p->download_error_code = 0;
    send_download_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
    send_download_p->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
    send_download_p->transfer_count = header_ptr->image_size;

    fbe_payload_control_build_operation (control_operation_p,
                                         command,
                                         send_download_p,
                                         sizeof(fbe_physical_drive_mgmt_fw_download_t));

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_payload_ex_set_sg_list(payload_p, fwdl_process->image_sgl, fwdl_process->num_image_sg_elements);
    fbe_transport_set_completion_function(packet_p, fbe_drive_configuration_send_download_to_pdo_completion, fwdl_process);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    /* Even if there was a problem in the topology sending the packet and we received a control operation failure
     * status, the completion function will always be called where the packet is completed and the error handling
     * is done.  Therefore, all we need to do here is return FBE_STATUS_OK.
     */
        
    return FBE_STATUS_OK;    
}
/******************************************************************************
 * end fbe_drive_configuration_send_download_to_pdo()
 ******************************************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_download_next_system_drive
 ****************************************************************
 * @brief
 *  This function starts download for next system drive
 *
 * @param fwdl_process - pointer to process structure.
 *
 * @return status - The status of the operation.
 *
 * @version
 *   08/25/2014:  Kang Jamin
 *
 ****************************************************************/
static fbe_status_t
fbe_drive_configuration_download_next_system_drive(fbe_drive_configuration_download_process_t * fwdl_process)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_u32_t                                               index;
    fbe_drive_configuration_download_drive_t *              drive_info;

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry, num_qualified_drives=%d  num_done_drives=%d\n",
                              __FUNCTION__, fwdl_process->num_qualified_drives, fwdl_process->num_done_drives);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    /* For each object, sends download request to PDO */
    for (index = 0; index < fwdl_process->num_qualified_drives; index++) {
        drive_info = &fwdl_process->drives[index];

        /* Only download for system drive with state == INIT */
        if (! fbe_drive_configuration_is_system_drive(drive_info)) {
            continue;
        }
        if (drive_info->dl_state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT) {
            continue;
        }
        /* It's possible the process was aborted either by user or PDO completion function while
           we are in the middle of sending the DL requests. Mark each drive as aborted and
           skip sending the request.
         */
        if (fwdl_process->fail_abort) {
            drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
            drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED;
            fbe_atomic_increment(&fwdl_process->num_done_drives);

            fbe_spinlock_unlock (&fwdl_process->download_lock);
            dcs_download_notify_drive_state_changed(drive_info);
            fbe_spinlock_lock (&fwdl_process->download_lock);
            continue;
        }

        drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE;
        fbe_spinlock_unlock (&fwdl_process->download_lock);

        status = fbe_drive_configuration_send_download(fwdl_process, drive_info);

        fbe_spinlock_lock(&fwdl_process->download_lock);
        if (status == FBE_STATUS_OK) {
            drive_configuration_trace(
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: start download for system drive %u_%u_%u\n",
                __FUNCTION__, drive_info->location.bus, drive_info->location.enclosure,
                drive_info->location.slot);
            break;
        }
        /**
         * If send fail, the PDO completion function will not be called, so we need to download next
         * system drive
         */
    }
    fbe_spinlock_unlock(&fwdl_process->download_lock);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_download_next_system_drive() 
 ******************************************************/

/*!****************************************************************************
 * fbe_provision_drive_send_download_to_pdo_completion()
 ******************************************************************************
 * @brief
 *  This function completes download request from PDO.
 *
 * @param packet_p - The packet requesting this operation.
 * @param context - Ptr to context.
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *   04/15/2013:  Modified how a firmware upgrade request is failed when it is
 *                rejected by an upstream object so that oustanding firmware
 *                upgrades are no longer aborted. Darren Insko
 *
 ******************************************************************************/
fbe_status_t
fbe_drive_configuration_send_download_to_pdo_completion(fbe_packet_t * packet_p, 
                                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_drive_configuration_download_process_t *            fwdl_process = (fbe_drive_configuration_download_process_t *)context;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_status_t                            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_physical_drive_mgmt_fw_download_t *                 send_download_p;
    fbe_drive_configuration_download_drive_t *              drive_info;
    fbe_drive_configuration_download_drive_t                drive_info_copy;
    fbe_u32_t                                               index;
    fbe_bool_t                                              need_finish = FBE_FALSE;
    fbe_bool_t                                              need_abort = FBE_FALSE;
 
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    /* Check packet status and release memory */
    status = fbe_transport_get_status_code(packet_p);

    /* release the block operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &send_download_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS download_to_pdo_completion: object_id: 0x%x status: 0x%x error: 0x%x\n", 
                              send_download_p->object_id, send_download_p->status, send_download_p->download_error_code);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    for (index = 0; index < fwdl_process->num_qualified_drives; index++)
    {
        drive_info = &fwdl_process->drives[index];
        if (send_download_p->object_id == drive_info->object_id)
        {
            /* verify request hasn't already been completed. Only valid states are INIT and ACTIVE */
            if (FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT   != drive_info->dl_state &&
                FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE != drive_info->dl_state)
            {
                /* NOTE, this can happen if an abort finishes first. */
                drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                          "DCS download_to_pdo_completion: object_id=0x%x invalid dl_state=%d\n",
                                          drive_info->object_id, drive_info->dl_state);

                fbe_atomic_increment(&fwdl_process->num_done_drives); 

                drive_info_copy = *drive_info;
                fbe_spinlock_unlock (&fwdl_process->download_lock);

                dcs_download_notify_drive_state_changed(&drive_info_copy);                
                
                fbe_memory_native_release(send_download_p);
                fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
                fbe_transport_release_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            if (status != FBE_STATUS_OK)
            {
                if (status == FBE_STATUS_CANCELED)
                {
                    /* Download was aborted. */
                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED;
                }
                else
                {
                    /* Download failed due to a control operation failure, which we treat as a non qualified drive.
                       An Example of this failure would be PDO or upstream objects not in a supported lifecycle state. */
                    drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                              "DCS download_to_pdo_completion: Non Qual failure. object_id:0x%x pkt status:%d \n", 
                                              send_download_p->object_id, status);

                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED;

                    /* Just because the PDO or upstream object is not in a supported lifecycle state,
                     * we shouldn't abort and prevent the outstanding firmware downloads from completing.
                     */
                }
            }
            else if (send_download_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
            {
                if (fwdl_process->trial_run)
                {
                    /* Trial Run succeeded. */
                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE_TRIAL_RUN;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE;                    
                }
                else
                {
                    /* Download succeeded. */
                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_COMPLETE;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE;
                }
            }
            else
            {
                /* Download failed with FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED. */
                /*TODO:  we need a better way to pass back failure reason.  Consider adding a logical reason, 
                         and scsi_error (for WB failure case)
                */
                if (send_download_p->download_error_code == 0)
                {
                    /* If an upstream object rejected the firmware download request, mark the firmware download
                     * to this drive as failed due to it not being qualified.  However, don't abort and prevent the
                     * outstanding firmware downloads from completing.
                     */
                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED;
                }
                else
                {
                    drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
                    drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_WRITE_BUFFER;
                    need_abort = FBE_TRUE;
                    fwdl_process->fail_abort = FBE_TRUE;
                    fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                    fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_WRITE_BUFFER;
                }
            }

            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS download_to_pdo_completion: Matched drive %d  object_id=0x%x  need_abort=%s\n",
                                      index, drive_info->object_id, (need_abort ? "TRUE" : "FALSE"));
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS download_to_pdo_completion: dl_state=%d  fail_code=%d  process_state=%d process_fail_reason=%d\n", 
                                      drive_info->dl_state, drive_info->fail_code, fwdl_process->process_state, fwdl_process->process_fail_reason);

            drive_info_copy = *drive_info;
            fbe_spinlock_unlock (&fwdl_process->download_lock);

            dcs_download_notify_drive_state_changed(&drive_info_copy);
            if (fbe_drive_configuration_is_system_drive(drive_info)) {
                drive_configuration_trace(
                    FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: system drive %u_%u_%u finished\n",
                    __FUNCTION__, drive_info->location.bus,
                    drive_info->location.enclosure, drive_info->location.slot);
                fbe_drive_configuration_download_next_system_drive(fwdl_process);
            }
            fbe_spinlock_lock (&fwdl_process->download_lock);
            break;
        }
    }
    fbe_atomic_increment(&fwdl_process->num_done_drives);    

    if (fwdl_process->num_done_drives == fwdl_process->num_qualified_drives)
    {
        need_finish = FBE_TRUE;
    }
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "DCS download_to_pdo_completion: num_qualified_drives=%d  num_done_drives=%d  need_finish=%s\n",
                              fwdl_process->num_qualified_drives, fwdl_process->num_done_drives,
                              (need_finish ? "TRUE" : "FALSE"));

    fbe_memory_native_release(send_download_p);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_release_packet(packet_p);

    if (need_finish)
    {
        fbe_drive_configuration_finish_download(fwdl_process);
    }
    else if (need_abort)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS download_to_pdo_completion: Starting abort of outstanding downloads...\n");

        fbe_drive_configuration_abort_download(fwdl_process);
    }

    return FBE_STATUS_OK;    
}
/******************************************************************************
 * end fbe_drive_configuration_send_download_to_pdo_completion()
 ******************************************************************************/


/*!**************************************************************
 * @fn dcs_download_notify_drive_state_changed
 ****************************************************************
 * @brief
 *  Sends notification to clients to inform them that the following
 *  drive has a download status that changed.  
 * 
 * @param pdo_oid - Object ID for PDO
 * @param bus, encl, slot - drive location
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   02/10/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void dcs_download_notify_drive_state_changed(const fbe_drive_configuration_download_drive_t *drive_p)
{
    fbe_notification_info_t  notification;

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    notification.class_id = FBE_CLASS_ID_DRIVE_MGMT;            /* this used to come directly from DMO. For legacy reasons keeping it the same */
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
    notification.notification_data.data_change_info.device_mask = FBE_DEVICE_TYPE_DRIVE;
    notification.notification_data.data_change_info.data_type = FBE_DEVICE_DATA_TYPE_FUP_INFO;
    notification.notification_data.data_change_info.phys_location.bus = drive_p->location.bus;
    notification.notification_data.data_change_info.phys_location.enclosure = drive_p->location.enclosure;
    notification.notification_data.data_change_info.phys_location.slot = drive_p->location.slot;
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS notify_drive_state_changed,: %d_%d_%d pdo:0x%x state:%d fail_code:%d prior_rev:%s\n",  
                              drive_p->location.bus, drive_p->location.enclosure, drive_p->location.slot,
                              drive_p->object_id, drive_p->dl_state, drive_p->fail_code, drive_p->prior_product_rev);
    fbe_notification_send(drive_p->object_id, notification);
}


/*!**************************************************************
 * @fn fbe_drive_configuration_finish_download
 ****************************************************************
 * @brief
 *  This function finishes download process. 
 * 
 * @param fwdl_process - Ptr to download process.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *   04/15/2013:  Modified how overall download process status is
 *                determined when it finishes instead of when one
 *                of the drives encounters a failure. Darren Insko
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_finish_download(fbe_drive_configuration_download_process_t * fwdl_process)
{
    fbe_u32_t                                               index;
    fbe_drive_configuration_download_drive_t *              drive_info;
    fbe_bool_t                                              is_no_failure = FBE_FALSE;
    fbe_bool_t                                              is_fail_nonqualified = FBE_FALSE;
    fbe_bool_t                                              is_fail_generic_failure = FBE_FALSE;
    fbe_fdf_header_block_t *                                tmp_header = NULL;    

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    if (fwdl_process->in_progress == FBE_TRUE){
        if (fwdl_process->cleanup_in_progress == FBE_FALSE){
            fwdl_process->cleanup_in_progress = FBE_TRUE;
        }else{
            fbe_spinlock_unlock (&fwdl_process->download_lock);
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DCS finish_download: Skipping cleanup since cleanup is already in progress\n");
            return FBE_STATUS_OK;
        }
    }else{
        fbe_spinlock_unlock (&fwdl_process->download_lock);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DCS finish_download: Skipping cleanup since in_progress is FALSE.\n");
        return FBE_STATUS_OK;
    }

    /* Stop time */
    fbe_get_system_time(&fwdl_process->stop_time);
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    if (fwdl_process->fail_abort == FBE_FALSE)
    {
        /* If we haven't aborted the firmware trial run or upgrade, we need to determine the status of the
         * overall download process by checking each of the drives' failure reasons in the context of whether
         * a firmware trial run or upgrade was being performed.
         */
        for (index = 0; index < fwdl_process->num_qualified_drives; index++)
        {
            drive_info = &fwdl_process->drives[index];
            if (fwdl_process->trial_run)
            {
                /* If we're in a trial run... */
                if (drive_info->fail_code != FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED)
                {
                    /* ...and we've proven that at least one drive is not FAIL_NONQUALIFIED, then we can't fail. */
                    fbe_spinlock_lock (&fwdl_process->download_lock);
                    fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL;
                    fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
                    fbe_spinlock_unlock (&fwdl_process->download_lock);
                    break;
                }
            }
            else
            {
                /* If we're not in a trial run... */
                if (drive_info->fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_NONQUALIFIED)
                {
                    /* ...keep track of when a drive failed to upgrade because it was not qualified */
                    is_fail_nonqualified = FBE_TRUE;

                    if (fwdl_process->fail_nonqualified)
                    {
                        /* ...and if a flag was set indicating we should fail when one or more drives are
                         * nonqualified, then we should stop looking at drives' status and fail below.
                         */
                        break;
                    }
                }
                else if (drive_info->fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_NO_FAILURE)
                {
                    /* ...keep track of when a drive was upgraded without a failure. */
                    is_no_failure = FBE_TRUE;
                }
                else if (drive_info->fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_GENERIC_FAILURE)
                {
                    is_fail_generic_failure = FBE_TRUE;
                }
            }
        }

        /* Now that we're finished checking the individual drives' failure reasons, determine the status of the
         * overall download process.
         */
        if (fwdl_process->trial_run)
        {
            /* If we're in a trial run... */
            if (fwdl_process->process_state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL)
            {
                /* ... and we've haven't proven that there is at least one drive that is qualified to be upgraded,
                 * then none of the drives are qualified to be upgraded.
                 */
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NONQUALIFIED;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
        }
        else
        {
            /* If we're not in a trial run... */
            if (is_fail_nonqualified &&
                fwdl_process->fail_nonqualified)
            {
                /* ...and if a flag was set indicating we should fail when one or more drives are
                 * nonqualified, then we should fail if we detected a nonqualified drive.
                 */
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NONQUALIFIED;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
            else if (is_fail_generic_failure)
            {
                /* ...we should fail when at least one drive encountered a GENERIC failure. */
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_GENERIC_FAILURE;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
            else if (!is_no_failure)
            {
                /* ... we should fail when no drives were successfully upgraded. */
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
            else
            {
                /* ...otherwise, if none of the drives' firmare download states have caused the overall download
                 * process to fail, then we must be successful.
                 */
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
        }
    }
    else
    {
        /* Sanity check, we should never be here ... */
        fbe_spinlock_lock(&fwdl_process->download_lock);
        for (index = 0; index < fwdl_process->num_qualified_drives; index += 1)
        {
            drive_info = &fwdl_process->drives[index];
            if (!fbe_drive_configuration_is_system_drive(drive_info))
            {
                continue;
            }
            if (drive_info->dl_state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_INIT)
            {
                drive_configuration_trace(
                    FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: system drive %u_%u_%u is in INIT state\n",
                    __FUNCTION__, drive_info->location.bus,
                    drive_info->location.enclosure, drive_info->location.slot);
                drive_info->dl_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL;
                drive_info->fail_code = FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED;
            }
        }
        fbe_spinlock_unlock(&fwdl_process->download_lock);

        if (fwdl_process->process_state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED)
        {
            /* If we didn't fail immediately, we need to check each of the drives' failure reasons
             * to determine if any of them were aborted during an upgrade or a trial run.
             */
            for (index = 0; index < fwdl_process->num_qualified_drives; index++)
            {
                drive_info = &fwdl_process->drives[index];
                if (drive_info->fail_code == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_FAIL_ABORTED)
                {
                    /* ...we've proven that at least one drive is ABORTED. */
                    fbe_spinlock_lock (&fwdl_process->download_lock);
                    fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED;
                    fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED;
                    fbe_spinlock_unlock (&fwdl_process->download_lock);
                    break;
                }
            }

            if (fwdl_process->process_state != FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED)
            {
                fbe_spinlock_lock (&fwdl_process->download_lock);
                fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_SUCCESSFUL;
                fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_NO_FAILURE;
                fbe_spinlock_unlock (&fwdl_process->download_lock);
            }
        }
    }

    if (fwdl_process->process_state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAILED)
    {
        fbe_event_log_write(PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED, NULL, 0, "%s %s %d", 
                            fwdl_process->tla_number, fwdl_process->fw_rev, fwdl_process->process_fail_reason);
    }
    else if (fwdl_process->process_state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED)
    {
        fbe_event_log_write(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED, NULL, 0, NULL);
    }
    else
    {
        fbe_event_log_write(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED, NULL, 0, "%s %s", 
                            fwdl_process->tla_number, fwdl_process->fw_rev);
    }

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS finish_download: process_state=%d  process_fail_reason=%d\n", 
                              fwdl_process->process_state, fwdl_process->process_fail_reason);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    fbe_zero_memory(fwdl_process->fw_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
    fbe_zero_memory(fwdl_process->tla_number, FBE_FDF_TLA_SIZE);

    tmp_header = fwdl_process->header_ptr;
    fwdl_process->header_ptr = NULL;
    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Deallocate header and image. okay to do this outside lock since we are now
       finished so no other thread will attempt to access header or trailer info 
    */
    if (tmp_header != NULL) /* may be null if failure during init. so we check for it.*/
    {
        fbe_memory_native_release(tmp_header);
    }

    dcs_download_release_image(&fwdl_process->image_sgl[0]);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    /* Last thing. */
    /* Please keep the setting of the values of in_progress and cleanup_in_progress
     * together here and inside the lock.
     */
    fwdl_process->in_progress = FBE_FALSE;
    fwdl_process->cleanup_in_progress = FBE_FALSE;
    fbe_spinlock_unlock (&fwdl_process->download_lock);
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DCS finish_download: Completed cleanup\n");

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_finish_download() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_abort_download
 ****************************************************************
 * @brief
 *  This function aborts download process. 
 * 
 * @param fwdl_process - Ptr to download process.
 * 
 * @return fbe_status_t 
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_abort_download(fbe_drive_configuration_download_process_t * fwdl_process)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_u32_t                                               index;
    fbe_drive_configuration_download_drive_t *              drive_info;
    fbe_object_id_t                                         object;
    

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry, num_qualified_drives=%d  num_done_drives=%d\n",
                              __FUNCTION__, fwdl_process->num_qualified_drives, fwdl_process->num_done_drives);

    fbe_spinlock_lock (&fwdl_process->download_lock);
    if (!fwdl_process->in_progress)
    {
        fbe_spinlock_unlock (&fwdl_process->download_lock);
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s No download process to ABORT.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fwdl_process->fail_abort == FBE_FALSE)
    {
        fwdl_process->fail_abort = FBE_TRUE;
        fwdl_process->process_state = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_ABORTED;
        fwdl_process->process_fail_reason = FBE_DRIVE_CONFIGURATION_DOWNLOAD_PROCESS_FAIL_ABORTED;
    }

    for (index = 0; index < fwdl_process->num_qualified_drives; index++)
    {
        drive_info = &fwdl_process->drives[index];
        if (drive_info->dl_state == FBE_DRIVE_CONFIGURATION_DOWNLOAD_DRIVE_ACTIVE)
        {
            /* Send abort to PDO */
            object = drive_info->object_id;
            fbe_spinlock_unlock (&fwdl_process->download_lock);
            status = fbe_drive_configuration_send_abort_to_pdo(object);
            
            fbe_spinlock_lock (&fwdl_process->download_lock);
        }
    }
    
    fbe_spinlock_unlock (&fwdl_process->download_lock);
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_abort_download() 
 ******************************************************/

/*!****************************************************************************
 * fbe_drive_configuration_send_abort_to_pdo()
 ******************************************************************************
 * @brief
 *  This function sends download request to PDO.
 *
 * @param object_id - PDO id.
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ******************************************************************************/
fbe_status_t
fbe_drive_configuration_send_abort_to_pdo(fbe_object_id_t object_id)
{
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                                          packet_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry, object_id: 0x%x\n", __FUNCTION__, object_id);

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: object_id 0x%x Error allocating packet\n", 
                                  __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation (control_operation_p,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_ABORT,
                                         NULL,
                                         0);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_completion_function(packet_p, fbe_drive_configuration_send_abort_to_pdo_completion, &object_id);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
        
    return FBE_STATUS_OK;    
}
/******************************************************************************
 * end fbe_drive_configuration_send_abort_to_pdo()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_drive_configuration_send_abort_to_pdo_completion()
 ******************************************************************************
 * @brief
 *  This function completes download request from PDO.
 *
 * @param packet_p - The packet requesting this operation.
 * @param context - Ptr to context.
 *
 * @return status - The status of the operation.
 *
 * @version
 *   11/16/2011:  chenl6 - Created.
 *
 ******************************************************************************/
fbe_status_t
fbe_drive_configuration_send_abort_to_pdo_completion(fbe_packet_t * packet_p, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t                                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t *                                object_id = (fbe_object_id_t *)context;
    fbe_payload_ex_t *                               payload_p = NULL;
    fbe_payload_control_operation_t *                control_operation_p = NULL;
 
    /* Check packet status and release memory */
    status = fbe_transport_get_status_code(packet_p);

    /* release the block operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: object 0x%x aborted status 0x%x\n", 
                              __FUNCTION__,
                              *object_id, status);
    
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_release_packet(packet_p);
    
    return FBE_STATUS_OK;    
}
/******************************************************************************
 * end fbe_drive_configuration_send_abort_to_pdo_completion()
 ******************************************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_get_max_dl_count
 ****************************************************************
 * @brief
 *  This function handles request to get max download drive information. 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @note This has been deprecated, but still here since Admin still makes
 *       calls to get it.  
 * @version
 *   05/02/2012:      - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_max_dl_count(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_get_download_max_drive_count_t *get_max_dl_info = NULL;
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_drive_configuration_download_process_t *            fwdl_process;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer(control_operation, &get_max_dl_info); 
    if(get_max_dl_info == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_get_download_max_drive_count_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the right download process. For now we support one process only. */
    fwdl_process = &drive_download_info;
    fbe_spinlock_lock (&fwdl_process->download_lock);

    get_max_dl_info->max_dl_count  =  fwdl_process->max_dl_count;

    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Complete control packet now */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_drive_configuration_control_get_max_dl_count() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_drive_configuration_control_get_all_download_drives_info
 ****************************************************************
 * @brief
 *  This function is similar to fbe_drive_configuration_control_get_download_drive_info
 *  but returns the information for all drives 
 * 
 * @param packet - pointer to packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   05/09/2012:  Shay Harel - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_control_get_all_download_drives_info(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                          payload = NULL;
    fbe_payload_control_operation_t *                           control_operation = NULL;
    fbe_drive_configuration_download_get_drive_info_t  *        get_drive_info = NULL;
    fbe_payload_control_buffer_length_t                         len = 0;
    fbe_drive_configuration_download_process_t *                fwdl_process;
    fbe_u32_t                                                   i;
    fbe_drive_configuration_download_drive_t *                  drive;
    fbe_sg_element_t *                                          sg_element = NULL;
    fbe_u32_t                                                   sg_elements = 0;
    fbe_drive_configuration_control_get_all_download_drive_t *  get_drives;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_drives); 
    if(get_drives == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_get_all_download_drive_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need the sg list in order to fill this memory with the object IDs*/
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_drive_configuration_download_get_drive_info_t)) != FBE_MAX_DOWNLOAD_DRIVES)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                                  __FUNCTION__, sg_element->count, FBE_MAX_DOWNLOAD_DRIVES);
        
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*point to start of output buffer*/
    get_drive_info = (fbe_drive_configuration_download_get_drive_info_t*)sg_element->address;

    /* Find the right download process. For now we support one process only. */
    fwdl_process = &drive_download_info;

    fbe_spinlock_lock (&fwdl_process->download_lock);

    get_drives->actual_returned = fwdl_process->num_qualified_drives;

    for (i = 0; i < fwdl_process->num_qualified_drives; i++)
    {
        drive = &fwdl_process->drives[i];
        get_drive_info->bus = drive->location.bus;
        get_drive_info->enclosure = drive->location.enclosure;
        get_drive_info->slot = drive->location.slot;
        get_drive_info->state = drive->dl_state;
        get_drive_info->fail_code = drive->fail_code;
        fbe_copy_memory(&get_drive_info->prior_product_rev, &drive->prior_product_rev, 
                        FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
        (get_drive_info->prior_product_rev)[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';
        get_drive_info++; 
    }

    fbe_spinlock_unlock (&fwdl_process->download_lock);

    /* Complete control packet now */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn dcs_download_allocate_image_and_copy
 ****************************************************************
 * @brief
 *  DCS utility function for allocating memory for image.   It
 *  attempts to allcate entire chuck from from native physically
 *  contiguous pool first.   If it fails it will attempt
 *  allocating smaller chunks and rely on sgl for sending image.
 * 
 * @param image_ptr - raw ptr to fw image which was passed to us.
 * @param allocation_size_in_bytes - size of image.
 * @param sg_lsit - scatter gather list to hold fw image.
 * @param num_elements - num elements created to hold image.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   10/23/2012:  Wayne Garrett- Created.
 *
 ****************************************************************/
static fbe_status_t dcs_download_allocate_image_and_copy(fbe_u8_t * image_ptr, fbe_u32_t allocation_size_in_bytes, fbe_sg_element_t* sg_list, fbe_u32_t *num_elements)
{
    void * ptr = NULL;
    fbe_u32_t num_copied = 0;
    fbe_u32_t chunk_size =  fbe_dcs_get_fw_image_chunk_size();     
    fbe_u32_t bytes_remaining;

    /* init sg list before allocating*/
    *num_elements = 0;
    fbe_sg_element_terminate(&sg_list[0]);

 
    if (!fbe_dcs_param_is_enabled(FBE_DCS_BREAK_UP_FW_IMAGE))
    {
        /* Attempt to allocate the entire image in one physically contiguous chunk.
        */
        ptr = fbe_memory_native_allocate(allocation_size_in_bytes);

        if (NULL != ptr)
        {
            fbe_copy_memory(ptr, image_ptr, allocation_size_in_bytes); 
    
            /* sg_list */
            fbe_sg_element_init(&sg_list[*num_elements], 
                                allocation_size_in_bytes, 
                                ptr);  
            (*num_elements)++;
            fbe_sg_element_terminate(&sg_list[*num_elements]);

            return FBE_STATUS_OK;
        }
        /* else there was a failure so fall through to code that breaks up fw image into smaller chunks.*/

        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DCS Download: Failed allocation full size:%dK\n", allocation_size_in_bytes/1024);
    }
    else
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DCS Download: FBE_DCS_BREAK_UP_FW_IMAGE is enabled. \n");
    }



    /* Break up fw image by allocating smaller chunks of physically contiguous memory
    */

    for (num_copied = 0;  num_copied < allocation_size_in_bytes; num_copied += chunk_size)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DCS Download: total:%d num_copied:%d chunk_size:%d\n", allocation_size_in_bytes, num_copied, chunk_size);
    
        if ( (*num_elements) >= DCS_MAX_DOWNLOAD_SG_ELEMENTS-1)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DCS Download: No more elements for allocation. max=%d. chunk_size %d too small\n", DCS_MAX_DOWNLOAD_SG_ELEMENTS, chunk_size);
            dcs_download_release_image(&sg_list[0]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        bytes_remaining = allocation_size_in_bytes-num_copied;
    
        if (bytes_remaining < chunk_size)  /* handle last copy*/
        {              
            chunk_size = bytes_remaining;
    
            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DCS Download: last copy.  num_copied:%d chunk_size:%d\n", num_copied, chunk_size);
        }
    
        ptr = fbe_memory_native_allocate(chunk_size);
        
        if (NULL == ptr)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DCS Download: Failed allocation chunk size:%d, num_copied:%d, total:%d\n", chunk_size, num_copied, allocation_size_in_bytes);
            dcs_download_release_image(&sg_list[0]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_copy_memory(ptr, image_ptr + num_copied, chunk_size);
    
        /* sg_list */
        fbe_sg_element_init(&sg_list[*num_elements], 
                            chunk_size, 
                            ptr);  
        (*num_elements)++;
        fbe_sg_element_terminate(&sg_list[*num_elements]);
    }

    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn dcs_download_release_image
 ****************************************************************
 * @brief
 *  DCS utility function to release memory for image. It iterate
 *  over each image sg element releasing each one.
 * 
 * @param sg_list - scatter gather list containing fw image.
 * 
 * @return void
 *
 * @version
 *   10/23/2012:  Wayne Garrett- Created.
 *
 ****************************************************************/
static void dcs_download_release_image(fbe_sg_element_t* sg_list)
{
    fbe_u32_t i = 0;

    while (NULL != fbe_sg_element_address(&sg_list[i]))
    {
        if (i >= DCS_MAX_DOWNLOAD_SG_ELEMENTS)
        {
            drive_configuration_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s: attempted to overrun array\n", __FUNCTION__);
            break;
        }

        fbe_memory_native_release(fbe_sg_element_address(&sg_list[i]));
        fbe_sg_element_terminate(&sg_list[i]);
        i++;

    }
}

