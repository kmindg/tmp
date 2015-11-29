/***************************************************************************
 *  terminator_encl_eses_plugin_send_diag.c
 ***************************************************************************
 *
 *  Description
 *     This contains the funtions related to processing
 *     the "send diagnostic" ESES command.
 *      
 *
 *  History:
 *      Oct-16-08   Rajesh V Created
 *    
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_terminator_common.h"

#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"

/******************************/
/*     local function         */
/*****************************/

static fbe_status_t process_encl_ctrl_diag_page(fbe_sg_element_t *sg_list,
                                         fbe_terminator_device_ptr_t virtual_phy_handle,
                                         fbe_u8_t *sense_buffer);
static fbe_status_t encl_ctrl_diag_page_process_ctrl_elems(fbe_u8_t *encl_ctrl_page_start_ptr,
                                                           fbe_terminator_device_ptr_t virtual_phy_handle,
                                                           fbe_u8_t *sense_buffer);

static fbe_status_t encl_ctrl_diag_page_process_exp_phy_ctrl_elems(fbe_u8_t *exp_phy_ctrl_elems_start_ptr,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer);
static fbe_u32_t encl_ctrl_diag_page_size(fbe_sas_enclosure_type_t encl_type);
static fbe_status_t check_ctrl_page_gen_code(fbe_sg_element_t *sg_list,
                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                            fbe_u8_t *sense_buffer);
static fbe_status_t encl_ctrl_diag_page_process_chassis_encl_ctrl_elems(fbe_u8_t *encl_ctrl_elems_start_ptr,
                                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                        fbe_u8_t *sense_buffer);
static fbe_status_t encl_ctrl_diag_page_process_local_encl_ctrl_elems(fbe_u8_t *encl_ctrl_elems_start_ptr,
                                                                      fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                      fbe_u8_t *sense_buffer);
static fbe_status_t encl_ctrl_diag_page_process_array_dev_slot_ctrl_elems(fbe_u8_t *array_dev_slot_ctrl_elems_start_ptr,
                                                                          fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                          fbe_u8_t *sense_buffer);

static fbe_status_t encl_ctrl_diag_page_process_display0_ctrl_elems(fbe_u8_t *display_ctrl_elems_start_ptr,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t *sense_buffer);
static fbe_status_t encl_ctrl_diag_page_process_display1_ctrl_elems(fbe_u8_t *display_ctrl_elems_start_ptr,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t *sense_buffer);

static fbe_status_t process_str_out_diag_page(fbe_sg_element_t *sg_list,
                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                            fbe_u8_t *sense_buffer);

static void microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info);

static fbe_status_t microcode_ctrl_page_handle_activate(
    fbe_eses_download_control_page_header_t *download_ctrl_page_hdr,
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t *sense_buffer);

static fbe_status_t process_microcode_download_ctrl_page(
    fbe_sg_element_t *sg_list,
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t *sense_buffer);

static fbe_status_t microcode_ctrl_page_handle_download(
    fbe_eses_download_control_page_header_t *download_ctrl_page_hdr,
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t *sense_buffer);

static fbe_status_t microcode_ctrl_page_image_download_complete(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u8_t *sense_buffer);


/**************************************************************************
 *  process_payload_send_diagnostic()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes the "send diagnostic" cdb command.
 *
 *  PARAMETERS:
 *      Payload pointer, virtual phy object handle 
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t process_payload_send_diagnostic(fbe_payload_ex_t * payload, 
                                             fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_eses_send_diagnostic_cdb_t *send_diag_cdb;
    fbe_u8_t *send_diag_page_code;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    status = terminator_check_sas_enclosure_type(encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_check_sas_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if(payload_cdb_operation == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get sense buffer from cdb
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    send_diag_cdb = (fbe_eses_send_diagnostic_cdb_t *)(payload_cdb_operation->cdb);

    // Check if self-test bit is set and
    // Then we just return a good status
    if(send_diag_cdb->self_test == 1)
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
        return(FBE_STATUS_OK);
    }

    // Sanity check the fields in cdb cmd block.
    if((send_diag_cdb->self_test_code != 0) ||
       (send_diag_cdb->control != 0) ||
       (send_diag_cdb->page_format != 1))
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return status;
    }

    // Get SG list from payload
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if(sg_list->count == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    send_diag_page_code = (fbe_u8_t *)(sg_list->address);

    // Check if the user marked the page as unsupported
    if(!terminator_is_eses_page_supported(FBE_SCSI_SEND_DIAGNOSTIC, *send_diag_page_code))
    {
        //set up sense data here.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    switch(*send_diag_page_code)
    {
        case SES_PG_CODE_ENCL_CTRL:
            status = process_encl_ctrl_diag_page(sg_list, virtual_phy_handle, sense_buffer);
            break;
        case SES_PG_CODE_EMC_ENCL_CTRL:
            status = process_emc_encl_ctrl_page(sg_list, virtual_phy_handle, sense_buffer);
            break;
        case SES_PG_CODE_DOWNLOAD_MICROCODE_CTRL:
            status = process_microcode_download_ctrl_page(sg_list, virtual_phy_handle, sense_buffer);
            break;
        case SES_PG_CODE_STR_OUT:
            status = process_str_out_diag_page(sg_list, virtual_phy_handle, sense_buffer);
            break;
        default:
        //set up sense data here.
            build_sense_info_buffer(sense_buffer, 
                                    FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                    FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                    FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;        
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
    }
    else
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
    }
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  process_str_out_diag_page()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes String out Diagnostic page.
 *
 *  PARAMETERS:
 *      Scatter gather list pointer containing the page, virtual phy object
 *       object handle, sense buffer to be set, if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-09-2009: Rajesh V created.
 **************************************************************************/
fbe_status_t process_str_out_diag_page(fbe_sg_element_t *sg_list,
                                    fbe_terminator_device_ptr_t virtual_phy_handle,
                                    fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_str_out_t *str_out_page = NULL;
    fbe_u16_t str_out_page_len;
    fbe_bool_t test_mode = FBE_FALSE;
    fbe_eses_pg_eenp_mode_pg_t  *eenp_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    terminator_sp_id_t spid;
    
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the current "test mode" value
    eenp_mode_pg = 
        &vp_mode_pg_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];
    test_mode = eenp_mode_pg->test_mode;

    // Get the string out diagnostic page
    str_out_page = (fbe_eses_pg_str_out_t *)fbe_sg_element_address(sg_list);
    str_out_page_len = BYTE_SWAP_16(str_out_page->page_length);
    
    if((str_out_page->echo) && (test_mode))
    {
        // Only if the echo bit is set & test mode enabled, the string is written into the
        // ACTIVE TRACE BUFFER

        fbe_u8_t *str = NULL; // string given to us in the page
        size_t str_len = 0; // length of the string.
        terminator_eses_buf_info_t *active_trace_buf_info = NULL;
        fbe_u8_t active_trace_buf_id;
        fbe_u8_t *active_trace_buf = NULL;
        fbe_u32_t active_trace_buf_len = 0;
        fbe_u8_t buf_copy_size = 0;  // This is the size of data to be written into ACTIVE TRACE
        terminator_vp_eses_page_info_t *vp_eses_page_info;

        str = &str_out_page->str_out_data[0];
        // We are currently assuming that the string data is always
        // terminated with a '\0' (NULL) character.
        str_len = strlen((char *)str);

        // Get the active "trace buffer" information
        status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
        status =  fbe_terminator_api_get_sp_id(&spid);
        
        status = eses_get_buf_id_by_subencl_info(vp_eses_page_info, 
                                                 SES_SUBENCL_TYPE_LCC, 
                                                 spid, 
                                                 SES_BUF_TYPE_ACTIVE_TRACE,
                                                 FBE_FALSE,
                                                 0,
                                                 FBE_FALSE,
                                                 0,
                                                 FBE_FALSE,
                                                 0,                                                  
                                                 &active_trace_buf_id);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        status = eses_get_buf_info_by_buf_id(virtual_phy_handle, active_trace_buf_id, &active_trace_buf_info);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        active_trace_buf = active_trace_buf_info->buf;
        active_trace_buf_len = active_trace_buf_info->buf_len;

        //Write the string into the ACTIVE TRACE BUFFER
        if(str_len <  active_trace_buf_len)
        {
            buf_copy_size = (fbe_u32_t)(str_len + 1); // we also want to copy NULL character
        }
        else
        {
            buf_copy_size = active_trace_buf_len;
        }
        
        if(buf_copy_size != 0)
        {
            memcpy(active_trace_buf, str, (buf_copy_size * sizeof(fbe_u8_t)));
        }
    }
    return(FBE_STATUS_OK);
}


/**************************************************************************
 *  process_encl_ctrl_diag_page()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes Enclosure Control Diagnostic Page.
 *
 *  PARAMETERS:
 *      Scatter gather list pointer containing the page, virtual phy object
 *       object handle, sense buffer to be set if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t process_encl_ctrl_diag_page(fbe_sg_element_t *sg_list,
                                         fbe_terminator_device_ptr_t virtual_phy_handle,
                                         fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_pg_encl_ctrl_struct *encl_ctrl_page_hdr;
    fbe_u16_t page_length, page_size;
    fbe_u32_t gen_code;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    encl_ctrl_page_hdr = (ses_pg_encl_ctrl_struct *)fbe_sg_element_address(sg_list);
    status = config_page_get_encl_stat_diag_page_size(virtual_phy_handle, &page_size);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }
    
    page_length = page_size - 4;
    if((BYTE_SWAP_16(encl_ctrl_page_hdr->pg_len)) != page_length)
    {   // Pagelength given by config page does not match the
        // page length in the status page.

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    status = config_page_get_gen_code(virtual_phy_handle, &gen_code);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    if((BYTE_SWAP_32(encl_ctrl_page_hdr->gen_code)) != gen_code)
    {
        // Generation code mismatch.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
   

    status = encl_ctrl_diag_page_process_ctrl_elems((fbe_u8_t *)encl_ctrl_page_hdr,
                                                     virtual_phy_handle,
                                                     sense_buffer);
    return(status);
}

/**************************************************************************
 *  encl_ctrl_diag_page_process_ctrl_elems()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes Control elments in Enclosure Control
 *      diagnostic page.
 *
 *  PARAMETERS:
 *      Start address of the Encl ctrl diagostic page, virtual phy object 
 *          handle, sense buffer to be filled in if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t encl_ctrl_diag_page_process_ctrl_elems(fbe_u8_t *encl_ctrl_page_start_ptr,
                                                    fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t *stat_elem_start_ptr = encl_ctrl_page_start_ptr;
    //This contains the start offset of the particular element set as 
    //dictated by configuration page.
    fbe_u16_t stat_elem_byte_offset = 0;
    terminator_sp_id_t spid;

    status =  fbe_terminator_api_get_sp_id(&spid);

    // Process phy control elements.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_EXP_PHY,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_exp_phy_ctrl_elems(stat_elem_start_ptr, 
                                                            virtual_phy_handle, 
                                                            sense_buffer);
        RETURN_ON_ERROR_STATUS;  
    }


    // Process array device slot control elements.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_ARRAY_DEV_SLOT, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {
        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_array_dev_slot_ctrl_elems(stat_elem_start_ptr, 
                                                                    virtual_phy_handle, 
                                                                    sense_buffer);
        RETURN_ON_ERROR_STATUS; 
    }



    // Process Enclosure control elements in the Chassis Subenclosure.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_CHASSIS,
                                                            SIDE_UNDEFINED, 
                                                            SES_ELEM_TYPE_ENCL, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_chassis_encl_ctrl_elems(stat_elem_start_ptr, 
                                                                    virtual_phy_handle, 
                                                                    sense_buffer);
        RETURN_ON_ERROR_STATUS; 
    }

    // Process Enclosure control elements in the LOCAL LCC SUBEnclosure.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_ENCL, 
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);


    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_local_encl_ctrl_elems(stat_elem_start_ptr, 
                                                                virtual_phy_handle, 
                                                                sense_buffer);
        RETURN_ON_ERROR_STATUS;
    }


    // NOTE: In future, all the below display functions should be implemented
    // in a single function. This may be useful as we get enclosures with
    // more than 2 displays and different characters in each display. The single
    // function should determine these display parameters and process display elements
    // based on the enclosures display properties/Configuration page.

    // Process display elements for the first physical display that has 2 characters.

    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_DISPLAY, 
                                                            TRUE,
                                                            2,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);

   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_display0_ctrl_elems(stat_elem_start_ptr, 
                                                                virtual_phy_handle, 
                                                                sense_buffer);
        RETURN_ON_ERROR_STATUS;
    }


    // Process display elements for the second physical display that has 1 character.
    status = config_page_get_start_elem_offset_in_stat_page(virtual_phy_handle,
                                                            SES_SUBENCL_TYPE_LCC,
                                                            spid, 
                                                            SES_ELEM_TYPE_DISPLAY, 
                                                            TRUE,
                                                            1,
                                                            FALSE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            &stat_elem_byte_offset);
        
   
    if((status != FBE_STATUS_COMPONENT_NOT_FOUND) &&
       (status != FBE_STATUS_OK))
    {
        // FBE_STATUS_COMPONENT_NOT_FOUND is allowed as it means the configuration page
        // does not have the particular element.
        ema_not_ready(sense_buffer);
        return(status);
    }
    else if(status == FBE_STATUS_OK)
    {

        stat_elem_start_ptr = encl_ctrl_page_start_ptr + stat_elem_byte_offset;
        status = encl_ctrl_diag_page_process_display1_ctrl_elems(stat_elem_start_ptr, 
                                                                virtual_phy_handle, 
                                                                sense_buffer);
        RETURN_ON_ERROR_STATUS;
    }


    return(FBE_STATUS_OK);
}

/**************************************************************************
 *  encl_ctrl_diag_page_process_exp_phy_ctrl_elems()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes PHY Control elmeents in Enclosure Control
 *      diagnostic page.
 *
 *  PARAMETERS:
 *      Start address of the PHY Control elements, virtual phy object 
 *          handle, sense buffer to be filled in if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *   ALGORITHM:
 *    For each of the PHYS
 *      if force disabled bit is NOT set // force disabled bit can only be set/cleared using API indicating EMA bypass
 *          if ctrl page asks to disable the phy
 *              if PHY status is NOT (SES_STAT_CODE_UNAVAILABLE)
 *                  set the phy status to SES_STAT_CODE_UNAVAILABLE
 *                  set the phy ready to FALSE
 *                  if the (PHY corresponds to drive slot) &&  (drive is still logged in)
 *                      Generate a logout for the drive.
 *          else  //ctrl page asks to enable phy
 *               if PHY status is SES_STAT_CODE_UNAVAILABLE
 *                  set the phy status to OK
 *                  if the (PHY corresponds to drive slot)&&(drive is inserted in the slot)&& 
 *                         (DEVICE OFF" BIT is NOT SET in the Existing Array Device Slot Status element) 
 *                       set the phy ready to TRUE
 *                       Generate a login for the drive.
 *          endif //ctrl page asks to disable phy
 *    
 *      endif // force disabled bit is NOT set 
 *    //For loop end     
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t encl_ctrl_diag_page_process_exp_phy_ctrl_elems(fbe_u8_t *exp_phy_ctrl_elems_start_ptr,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_exp_phy_struct *exp_phy_ctrl_elem;
    ses_stat_elem_exp_phy_struct exp_phy_stat_elem;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_elem;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t max_phys, drive_slot, i;
    fbe_bool_t drive_slot_available = FBE_FALSE, drive_logged_in = FBE_FALSE;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    status = terminator_max_phys(encl_type, &max_phys);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    // Get pointer to the ovarall phy ctrl element
    exp_phy_ctrl_elem = (ses_ctrl_elem_exp_phy_struct *)(exp_phy_ctrl_elems_start_ptr);

    // Phys are assumed to be in the consecutive order of 
    // their PHY IDS
    for(i=0;i<max_phys; i++)
    {
        //Ignore overall control element
        exp_phy_ctrl_elem++;


        // If the select bit is NOT set, then EMA will not change the
        // element status based on control page.
        if(exp_phy_ctrl_elem->cmn_stat.select != 0x1)
        {
            continue;
        }
        
        // Get the current status of the phy from the status element
        status = terminator_get_phy_eses_status(virtual_phy_handle,
                                                i, 
                                                &exp_phy_stat_elem);
        if(status != FBE_STATUS_OK)
        {
            ema_not_ready(sense_buffer);
            return(status);
        }

        // We need not do anything if the "force disabled" bit is set
        // This indicates user disabled the phy using the TAPI.
        // So the user should once again enable the PHY explicitly
        // using the TAPI. The API corresponds to EMA disabling
        // the PHY due to some reason( see Spec)

        if(!exp_phy_stat_elem.force_disabled)
        {   //Force disabled bit not set for the PHY

            if(exp_phy_ctrl_elem->cmn_stat.disable == 1)
            {   //request to Disable the particular phy
                        
                //Check the current state of the PHY
                if(exp_phy_stat_elem.cmn_stat.elem_stat_code != 
                   SES_STAT_CODE_UNAVAILABLE)
                {
                    //Phy is not disabled, so disable it.
                    exp_phy_stat_elem.cmn_stat.elem_stat_code = SES_STAT_CODE_UNAVAILABLE;
                    exp_phy_stat_elem.phy_rdy = FBE_FALSE;

                    //Generate a logout if the phy corresponds to drive slot
                    //and a drive is inserted in the slot.
                    if(terminator_phy_corresponds_to_drive_slot(i, &drive_slot, encl_type))
                    { // Phy corresponds to drive slot

                        //Check if drive is inserted
                        status = terminator_virtual_phy_is_drive_slot_available(virtual_phy_handle, 
                                                                                drive_slot, 
                                                                                &drive_slot_available);
                        if(status != FBE_STATUS_OK)
                        {
                            ema_not_ready(sense_buffer);
                            return(status);
                        }
                        if(!drive_slot_available)
                        {   //drive inserted

                            status = terminator_virtual_phy_is_drive_in_slot_logged_in(virtual_phy_handle,
                                                                                        drive_slot,
                                                                                        &drive_logged_in);
                            if(status != FBE_STATUS_OK)
                            {
                                ema_not_ready(sense_buffer);
                                return(status);
                            }

                        
                            //Generate a logout for the drive present
                            if(drive_logged_in)
                            {
                                status = terminator_virtual_phy_logout_drive_in_slot(virtual_phy_handle, 
                                                                                    drive_slot);
                                if(status != FBE_STATUS_OK)
                                {
                                    ema_not_ready(sense_buffer);
                                    return(status);
                                }
                            }
                         }
                    } // terminator_phy_corresponds_to_drive_slot. 

                } // exp_phy_stat_elem.cmn_stat.elem_stat_code !=  SES_STAT_CODE_UNAVAILABLE

            }// exp_phy_ctrl_elem.cmn_stat.disable == 0 , request to disable the PHY
            else
            { // Ctrl Page Request to enable the phy
              
                if(exp_phy_stat_elem.cmn_stat.elem_stat_code == 
                   SES_STAT_CODE_UNAVAILABLE)
                {
                    exp_phy_stat_elem.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

                    // Generate a Login if the phy corresponds to drive slot
                    // and drive is inserted in the slot and the drive slot is
                    // powered ON
                    if(terminator_phy_corresponds_to_drive_slot(i, &drive_slot, encl_type))
                    { // Phy corresponds to drive slot

                        //Check if drive is inserted
                        status = terminator_virtual_phy_is_drive_slot_available(virtual_phy_handle, 
                                                                                drive_slot, 
                                                                                &drive_slot_available);
                        if(status != FBE_STATUS_OK)
                        {
                            ema_not_ready(sense_buffer);
                            return(status);
                        }

                        if(!drive_slot_available)
                        {   //drive inserted

                            status = terminator_get_drive_eses_status(virtual_phy_handle, drive_slot, &array_dev_slot_stat_elem);
                            if(status != FBE_STATUS_OK)
                            {
                                ema_not_ready(sense_buffer);
                                return(status);
                            }

                            if(!array_dev_slot_stat_elem.dev_off)
                            {// drive slot is powered ON

                                // Set the phy ready bit.
                                exp_phy_stat_elem.phy_rdy = FBE_TRUE;

                                //Generate a login for the drive present
                                status = terminator_virtual_phy_login_drive_in_slot(virtual_phy_handle, 
                                                                                    drive_slot);
                                if(status != FBE_STATUS_OK)
                                {
                                    ema_not_ready(sense_buffer);
                                    return(status);
                                }
                            } // drive slot is powered ON
                         } // Drive inserted
                    } // terminator_phy_corresponds_to_drive_slot. 

                } //exp_phy_stat_elem.cmn_stat.elem_stat_code != SES_STAT_CODE_OK

            } // ctrl page request to enable the PHY

        } //if force disabled bit not set.

        // Set the phy status to the changed values
        status = terminator_set_phy_eses_status(virtual_phy_handle, i, exp_phy_stat_elem);
        if(status != FBE_STATUS_OK)
        {
            ema_not_ready(sense_buffer);
            return(status);
        }                                          
    } // for(i=0;i<max_phys, i++)

    return(FBE_STATUS_OK);
    
}


// Ignore page size for now as we are using ctrl page offsets.
fbe_u32_t encl_ctrl_diag_page_size(fbe_sas_enclosure_type_t encl_type)
{
    return(0);
}

/**************************************************************************
 *  check_ctrl_page_gen_code()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function checks the generation code in the control page given.
 *
 *  PARAMETERS:
 *      Scatter gather list pointer containing the page, virtual phy object
 *      handle, sense buffer to be set if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      There are some control pages (ex: Microcode download control page)
 *      for which we do not do any processing (for now..) and just check 
 *      that the generation code is as expected.
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t check_ctrl_page_gen_code(fbe_sg_element_t *sg_list,
                                      fbe_terminator_device_ptr_t virtual_phy_handle,
                                      fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_common_pg_hdr_struct *page_hdr;
    fbe_u32_t gen_code;

    page_hdr = (ses_common_pg_hdr_struct *)fbe_sg_element_address(sg_list);

    status = config_page_get_gen_code(virtual_phy_handle, &gen_code);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    if((BYTE_SWAP_32(page_hdr->gen_code)) != gen_code)
    {
        // Generation code mismatch.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    return(FBE_STATUS_OK);
}

/**************************************************************************
 *  encl_ctrl_diag_page_process_array_dev_slot_ctrl_elems()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes Array Device slot Control elements in 
 *      Enclosure Control diagnostic page.
 *
 *  PARAMETERS:
 *      Start address of the Device Control elements, virtual phy object 
 *          handle, sense buffer to be filled in if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      DRIVE POWER OFF ALG:
 *
 *          For each of the DRIVE SLOTS
 *              if Drive is INSERTED in the drive slot
 *                  if (("DEVICE OFF" BIT in Array device slot control element is SET) AND 
 *                      ("DEVICE OFF" BIT IN existing Array Device slot status element is NOT SET)AND 
 *                      (DRIVE STILL LOGGED IN))  // Could be logged out because the phy could have been disabled using phy ctrl elems 
 *                           Set Phy ready bit to false              
 *                           Generate a LOGOUT for the drive  
 *              else if (("DEVICE OFF" BIT in Array device slot control element is NOT SET)AND 
 *                       ("DEVICE OFF" BIT IN existing Array Device slot status element is SET)AND 
 *                       (PHY ELEMENT STATUS CODE is OK)) 
 *                            Set Phy ready bit to true
 *                            Generate a LOGIN for the Drive
 *              Copy the "DEVICE OFF" BIT in Array device slot control element TO
 *                  "DEVICE OFF" BIT in Array device slot status element.
 *
 *  HISTORY:
 *      Dec-02-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t encl_ctrl_diag_page_process_array_dev_slot_ctrl_elems(fbe_u8_t *array_dev_slot_ctrl_elems_start_ptr,
                                                                   fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                   fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_array_dev_slot_struct *array_dev_slot_ctrl_elem;
    ses_stat_elem_array_dev_slot_struct array_dev_slot_stat_elem;
    ses_stat_elem_exp_phy_struct exp_phy_stat_elem;
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u8_t max_drive_slots, drive_slot, phy_id;
    fbe_bool_t drive_slot_available = FBE_FALSE, drive_logged_in = FBE_FALSE;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get pointer to the ovarall device slot ctrl element
    array_dev_slot_ctrl_elem = (ses_ctrl_elem_array_dev_slot_struct *)(array_dev_slot_ctrl_elems_start_ptr);

    // Device slot( Drive slot) elements are assumed to be
    // ordered consecutively as per their physical slot number.
    for(drive_slot = 0;drive_slot < max_drive_slots; drive_slot++)
    {
        //Ignore overall control element
        array_dev_slot_ctrl_elem++;

        // If the select bit is NOT set, then EMA will not change the
        // element status based on control page.
        if( array_dev_slot_ctrl_elem->cmn_ctrl.select != 0x1)
        {
            continue;
        }
        
        // Get the current status of the drive slot.
        status = terminator_get_drive_eses_status(virtual_phy_handle,
                                                  drive_slot, 
                                                  &array_dev_slot_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        // Get the  ID & status of the phy corresponding to the drive slot.
        status = terminator_get_drive_slot_to_phy_mapping(drive_slot, &phy_id, encl_type);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        status = terminator_get_phy_eses_status(virtual_phy_handle, phy_id, &exp_phy_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        //Power off the drive if requested.
        //(DRIVE POWER OFF ALGORITHM as per ESES 0.16)
        //Check if drive is inserted
        status = terminator_virtual_phy_is_drive_slot_available(virtual_phy_handle, 
                                                                drive_slot, 
                                                                &drive_slot_available);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        if(!drive_slot_available)
        {   // drive inserted

            if((array_dev_slot_ctrl_elem->dev_off) &&
               (!array_dev_slot_stat_elem.dev_off))
            {// Control element(client) wants EMA to power OFF the drive and the drive 
             // is currently powered ON. 

                fbe_u8_t drive_slot_power_down_count;

                if(exp_phy_stat_elem.cmn_stat.elem_stat_code != SES_STAT_CODE_UNAVAILABLE)
                {
                    exp_phy_stat_elem.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
                }

                // Clear the PHY READY bit.
                exp_phy_stat_elem.phy_rdy = FBE_FALSE;

                // Call terminator to know if the drive is currently logged in or logged out.
                status = terminator_virtual_phy_is_drive_in_slot_logged_in(virtual_phy_handle,
                                                                            drive_slot,
                                                                            &drive_logged_in);
                RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

                if(drive_logged_in)
                {// We  need to check that the drive is CURRENTLY LOGGED
                 // IN because drive could have been logged out when client
                 // disabled the corresponding PHY using "phy ctrl element"

                    // Generate a logout.
                    status = terminator_virtual_phy_logout_drive_in_slot(virtual_phy_handle, 
                                                                        drive_slot);
                    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
                }

                // Increment the drive slot power down count
                status = terminator_get_drive_power_down_count(virtual_phy_handle,
                                                               drive_slot,
                                                               &drive_slot_power_down_count);
                RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

                drive_slot_power_down_count ++;

                status = terminator_set_drive_power_down_count(virtual_phy_handle,
                                                               drive_slot,
                                                               drive_slot_power_down_count);
                RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
            }
            else if((!array_dev_slot_ctrl_elem->dev_off) &&
                    (array_dev_slot_stat_elem.dev_off) &&
                    (exp_phy_stat_elem.cmn_stat.elem_stat_code == SES_STAT_CODE_OK ))
            {// Control element(client) wants EMA to power ON the drive and the drive 
             // is currently powered OFF. We also need to check that the corresponding
             // PHY status code is OK.
                //Set the PHY READY bit and generate a login
                exp_phy_stat_elem.phy_rdy = FBE_TRUE;
                status = terminator_virtual_phy_login_drive_in_slot(virtual_phy_handle, 
                                                                    drive_slot);
                RETURN_EMA_NOT_READY_ON_ERROR_STATUS;                                 
            }
        }

        // Irrespective of the drive being inserted or not in the slot, EMA will still
        // power off the drive slot, if requested by client. A drive inserted in a
        // powered off drive slot will NOT be automatically powered on, unless the 
        // EMA client requests the drive slot to be powered on OR a reboot occurs.
        array_dev_slot_stat_elem.dev_off = array_dev_slot_ctrl_elem->dev_off;
        //(END OF DRIVE POWER OFF ALG)


        // Set the patterns as requested by the control element.
        array_dev_slot_stat_elem.ok = array_dev_slot_ctrl_elem->rqst_ok;
        array_dev_slot_stat_elem.ident = array_dev_slot_ctrl_elem->rqst_ident;
        array_dev_slot_stat_elem.fault_requested = array_dev_slot_ctrl_elem->rqst_fault;

        // Set the status of the drive slot.
        status = terminator_set_drive_eses_status(virtual_phy_handle,
                                                  drive_slot, 
                                                  array_dev_slot_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        // Set the corresponding phy status
        status = terminator_set_phy_eses_status(virtual_phy_handle, 
                                                phy_id, 
                                                exp_phy_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
    }

    return(FBE_STATUS_OK);
}


/**************************************************************************
 *  encl_ctrl_diag_page_process_chassis_encl_ctrl_elems()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes Chassis enclosure Control elements in 
 *    Enclosure Control diagnostic page. (Enclosure element in
 *    CHASSIS subenclosure)
 *
 *  PARAMETERS:
 *      Start address of the chassis encl Control elements, virtual phy object 
 *          handle, sense buffer to be filled in if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Dec-02-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t encl_ctrl_diag_page_process_chassis_encl_ctrl_elems(fbe_u8_t *encl_ctrl_elems_start_ptr,
                                                                 fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                 fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_encl_struct *encl_ctrl_elem;
    ses_stat_elem_encl_struct encl_stat_elem;

    // Get pointer to the ovarall encl ctrl element
    encl_ctrl_elem = (ses_ctrl_elem_encl_struct *)(encl_ctrl_elems_start_ptr);

    //Ignore overall control element
    encl_ctrl_elem++;

    // If the select bit is NOT set, then EMA will not change the
    // element status based on control page.
    if(encl_ctrl_elem->cmn_ctrl.select != 0x1)
    {
        return(FBE_STATUS_OK);
    }
    
    // Get the current status of chassis encl control element
    status = terminator_get_chassis_encl_eses_status(virtual_phy_handle,
                                                     &encl_stat_elem);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS

    // set the patterns as requested by the client.
    encl_stat_elem.ident = encl_ctrl_elem->rqst_ident;
    encl_stat_elem.failure_indicaton = encl_ctrl_elem->rqst_failure;
    encl_stat_elem.failure_rqsted = encl_ctrl_elem->rqst_failure;
    encl_stat_elem.warning_indicaton = encl_ctrl_elem->rqst_warning;
    encl_stat_elem.warning_rqsted = encl_ctrl_elem->rqst_warning;


    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s:Raj:Ctrl page Chassis CTRL elem, rqst_failure set to %d \n",
                        __FUNCTION__, encl_ctrl_elem->rqst_failure);

    status = terminator_set_chassis_encl_eses_status(virtual_phy_handle,
                                                     encl_stat_elem);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/**************************************************************************
 *  encl_ctrl_diag_page_process_local_encl_ctrl_elems()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes Local enclosure Control elements in 
 *    Enclosure Control diagnostic page. (Enclosure element in
 *    LOCAL LCC subenclosure)
 *
 *  PARAMETERS:
 *      Start address of the chassis encl Control elements, virtual phy object 
 *          handle, sense buffer to be filled in if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Dec-02-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t encl_ctrl_diag_page_process_local_encl_ctrl_elems(fbe_u8_t *encl_ctrl_elems_start_ptr,
                                                                 fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                 fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_encl_struct *encl_ctrl_elem;
    ses_stat_elem_encl_struct encl_stat_elem;

    // Get pointer to the ovarall encl ctrl element
    encl_ctrl_elem = (ses_ctrl_elem_encl_struct *)(encl_ctrl_elems_start_ptr);

    //Ignore overall control element
    encl_ctrl_elem++;

    // If the select bit is NOT set, then EMA will not change the
    // element status based on control page.
    if(encl_ctrl_elem->cmn_ctrl.select != 0x1)
    {
        return(FBE_STATUS_OK);
    }
    
    // Get the current status of chassis encl control element
    status = terminator_get_encl_eses_status(virtual_phy_handle,
                                             &encl_stat_elem);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }

    // set the patterns as requested by the client.
    encl_stat_elem.ident = encl_ctrl_elem->rqst_ident;
    encl_stat_elem.failure_indicaton = encl_ctrl_elem->rqst_failure;
    encl_stat_elem.failure_rqsted = encl_ctrl_elem->rqst_failure;
    encl_stat_elem.warning_indicaton = encl_ctrl_elem->rqst_warning;
    encl_stat_elem.warning_rqsted = encl_ctrl_elem->rqst_warning;

    status = terminator_set_encl_eses_status(virtual_phy_handle,
                                             encl_stat_elem);

    // Check for power cycle request bit being set.
    if(encl_ctrl_elem->power_cycle_rqst == 0x03)
    {
        // power cylce request bit is not allowed this value.
        build_sense_info_buffer(sense_buffer,
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST,
                                FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    // Check if the power cycle request is set.
    else if(encl_ctrl_elem->power_cycle_rqst == 0x01)
    {
        // Do some sanity checking on power cycle related parameters 
        if((encl_ctrl_elem->power_off_duration != 0) ||
            (encl_ctrl_elem->power_cycle_delay > 60))
        {
            build_sense_info_buffer(sense_buffer,
                                    FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST,
                                    FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION,
                                    FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        // NOTE: Viper(derringer too?) does not support power cycling the entire
        // enclosure but only resetting the LCC. This reset is still done through
        // enclosure element in chassis subenclosure. 
        status = terminator_virtual_phy_power_cycle_lcc(virtual_phy_handle, TERMINATOR_ESES_LCC_POWER_CYCLE_DELAY);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        // Clear the drive power down count and drive slot insert count
        status = terminator_clear_drive_power_down_counts(virtual_phy_handle);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
        status = terminator_clear_drive_slot_insert_counts(virtual_phy_handle);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    }

    else if(encl_ctrl_elem->power_cycle_rqst == 0x02)
    {
        // Cancel a sheduled power cycle request. Not supported for now.
    }


    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

fbe_status_t encl_ctrl_diag_page_process_display0_ctrl_elems(fbe_u8_t *display_ctrl_elems_start_ptr,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_display_struct *display_ctrl_elem;
    ses_stat_elem_display_struct display_stat_elem;
    fbe_u8_t display_char_index;

    // Get pointer to the ovarall display ctrl element
    display_ctrl_elem = (ses_ctrl_elem_display_struct *)(display_ctrl_elems_start_ptr);

    display_ctrl_elem++;

    //DISPLAY0 will have first two display characters
    for(display_char_index = DISPLAY_CHARACTER_0; display_char_index <= DISPLAY_CHARACTER_1; display_char_index++)
    {
        // If the select bit is NOT set, then EMA will not change the
        // element status based on control page.
        if(display_ctrl_elem->cmn_ctrl.select != 0x1)
        {
            continue;
        }

        status = terminator_get_display_eses_status(virtual_phy_handle,
                                                    display_char_index,
                                                    &display_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        display_stat_elem.fail = display_ctrl_elem->rqst_fail;
        display_stat_elem.ident = display_ctrl_elem->rqst_ident;
        
        if(display_ctrl_elem->display_mode == 0x02)
        {
            display_stat_elem.display_char_stat = display_ctrl_elem->display_char;
            display_stat_elem.display_mode_status = display_ctrl_elem->display_mode;
        }

        status = terminator_set_display_eses_status(virtual_phy_handle,
                                                    display_char_index,
                                                    display_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        display_ctrl_elem++;
    }

    return(FBE_STATUS_OK);
}


fbe_status_t encl_ctrl_diag_page_process_display1_ctrl_elems(fbe_u8_t *display_ctrl_elems_start_ptr,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_ctrl_elem_display_struct *display_ctrl_elem;
    ses_stat_elem_display_struct display_stat_elem;
    fbe_u8_t display_char_index;

    // Get pointer to the ovarall display ctrl element
    display_ctrl_elem = (ses_ctrl_elem_display_struct *)(display_ctrl_elems_start_ptr);

    display_ctrl_elem++;

    // DISPLAY1 will have the third display character
    for(display_char_index = DISPLAY_CHARACTER_2; display_char_index <= DISPLAY_CHARACTER_2; display_char_index++)
    {
        // If the select bit is NOT set, then EMA will not change the
        // element status based on control page.
        if(display_ctrl_elem->cmn_ctrl.select != 0x1)
        {
            continue;
        }

        status = terminator_get_display_eses_status(virtual_phy_handle,
                                                    display_char_index,
                                                    &display_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        display_stat_elem.fail = display_ctrl_elem->rqst_fail;
        display_stat_elem.ident = display_ctrl_elem->rqst_ident;
        
        if(display_ctrl_elem->display_mode == 0x02)
        {
            display_stat_elem.display_char_stat = display_ctrl_elem->display_char;
            display_stat_elem.display_mode_status = display_ctrl_elem->display_mode;
        }

        status = terminator_set_display_eses_status(virtual_phy_handle,
                                                    display_char_index,
                                                    display_stat_elem);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        display_ctrl_elem++;
    }

    return(FBE_STATUS_OK);
}

/**************************************************************************
 *  process_microcode_download_ctrl_page()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes microcode_download_ctrl_page.
 *
 *  PARAMETERS:
 *     Scatter gather list pointer containing the page, virtual phy object
 *     object handle, sense buffer to be set if necessary.
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *     Oct-05-2010: Rajesh V created.
 **************************************************************************/
static
fbe_status_t process_microcode_download_ctrl_page(fbe_sg_element_t *sg_list,
                                        fbe_terminator_device_ptr_t virtual_phy_handle,
                                        fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_download_control_page_header_t *download_ctrl_page_hdr;
    fbe_u16_t download_ctrl_page_length;
    fbe_u32_t gen_code;
    fbe_bool_t download = FALSE;
    fbe_bool_t activate = FALSE;
    fbe_download_status_desc_t download_stat_desc;
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info;
    fbe_u32_t microcode_image_length, microcode_data_length;
    // Pointer to contiguous virtual memory buffer to copy sg_list memory.
    fbe_u8_t *contiguous_page_p = NULL;
        
#if 0
    fbe_sas_enclosure_type_t encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        ema_not_ready(sense_buffer);
        return(status);
    }
#endif

    // Get the download microcode status which may need to be updated later.
    status = terminator_eses_get_download_microcode_stat_page_stat_desc(virtual_phy_handle, 
                                                                        &download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the download microcode control page information from Virtual Phy
    status = terminator_eses_get_vp_download_microcode_ctrl_page_info(virtual_phy_handle,
                                                                      &vp_download_microcode_ctrl_page_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;    
    
    // Pointer to the contiguous virtual memory buffer where we temporarily copy the
    // information from the sg_list. This should always be freed on encountering some
    // error or when we are done processing the current page information.
    contiguous_page_p = fbe_eses_copy_sg_list_into_contiguous_buffer(sg_list);
    if(contiguous_page_p == NULL)
    {
        //Something went wrong while processing sg_list
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Unable to copy sglist into contiguos buffer\n", __FUNCTION__);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
                
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    download_ctrl_page_hdr = 
        (fbe_eses_download_control_page_header_t *)contiguous_page_p;

    // Set the subenclosure identifier & status in the microcode status
    download_stat_desc.subencl_id = download_ctrl_page_hdr->subenclosure_id;
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_IN_PROGRESS; 
    status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                        download_stat_desc);

    download_ctrl_page_length = (((fbe_u16_t)(download_ctrl_page_hdr->page_length_msb)) << 8) +
                                ((fbe_u16_t)(download_ctrl_page_hdr->page_length_lsb));

    // Validate the control page length
    if(download_ctrl_page_length > 4096)
    { 
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: control page length:%d not allowed\n", __FUNCTION__, download_ctrl_page_length);

        terminator_free_mem_on_not_null(contiguous_page_p);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
                
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    status = config_page_get_gen_code(virtual_phy_handle, &gen_code);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Validate the control page generation code.
    if((BYTE_SWAP_32(download_ctrl_page_hdr->config_gen_code)) != gen_code)
    {
        // Generation code mismatch.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Generation code mismatch, config page:%d,download page:%d\n", 
                         __FUNCTION__, 
                         gen_code,
                         BYTE_SWAP_32(download_ctrl_page_hdr->config_gen_code));

        terminator_free_mem_on_not_null(contiguous_page_p);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // This is the total length of the image.
    microcode_image_length = BYTE_SWAP_32(download_ctrl_page_hdr->image_length);

    // This is the length of the image chunk transfered in this particular page.
    microcode_data_length = BYTE_SWAP_32(download_ctrl_page_hdr->transfer_length);

    // See if it is a download or an activate (or both).
    if(download_ctrl_page_hdr->mode == FBE_ESES_DL_CONTROL_MODE_DOWNLOAD)
    {
        download = TRUE;
    }
    else if(download_ctrl_page_hdr->mode == FBE_ESES_DL_CONTROL_MODE_ACTIVATE)
    {
        activate = TRUE;
        if((microcode_data_length != 0) && 
           (microcode_data_length == microcode_image_length))
        {
            // The entire image is sent in one send diagostic page
            // which requires download along with activate

            download = TRUE;
        }
        else if(microcode_data_length != microcode_image_length)
        {
            // Not allowed to do activate in one of the send diagnostic pages
            // that contains a part of the image. 

            terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Activate not allowed in send diag page containing partial image\n", __FUNCTION__);

            terminator_free_mem_on_not_null(contiguous_page_p);

            microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
                vp_download_microcode_ctrl_page_info);

            download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
            status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                                download_stat_desc);
            RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

            build_sense_info_buffer(sense_buffer, 
                                    FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                    FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                    FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
            return(FBE_STATUS_GENERIC_FAILURE);            
        }
    }
    else
    {
        // Any thing other than activate or download is not allowed. Return error.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Unknown mode:%d in ctrl page\n",__FUNCTION__, download_ctrl_page_hdr->mode);

        terminator_free_mem_on_not_null(contiguous_page_p);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);        

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);  
    }

    // Do download if required
    if(download)
    {
        status = microcode_ctrl_page_handle_download(download_ctrl_page_hdr,
                                                     virtual_phy_handle,
                                                     sense_buffer);
        if(status != FBE_STATUS_OK) 
        { 
            terminator_free_mem_on_not_null(contiguous_page_p);                          
            return(status);         
        }  
    }

    // Do activate if required
    if(activate)
    {
        status = microcode_ctrl_page_handle_activate(download_ctrl_page_hdr,
                                                     virtual_phy_handle,
                                                     sense_buffer);
        if(status != FBE_STATUS_OK) 
        { 
            terminator_free_mem_on_not_null(contiguous_page_p);                          
            return(status);         
        }  
    }

    terminator_free_mem_on_not_null(contiguous_page_p);

    return FBE_STATUS_OK;
                    
}

/**************************************************************************
 *  microcode_ctrl_page_handle_activate()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Handle the activate request in a microcode control diagnostic page.
 *
 *  PARAMETERS:
 *     microcode control diagnostic page header ptr, virtual phy object
 *     object handle, sense buffer to be set on some error.
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-15-2010: Rajesh V created.
 **************************************************************************/
fbe_status_t 
microcode_ctrl_page_handle_activate(
    fbe_eses_download_control_page_header_t *download_ctrl_page_hdr,
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t *sense_buffer)
{
    //vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info;
    vp_config_diag_page_info_t *vp_config_diag_page_info;
    ses_subencl_desc_struct *subencl_desc;
    fbe_status_t status;
    fbe_download_status_desc_t download_stat_desc;
    fbe_u8_t subencl_id = 0;
    fbe_u8_t subencl_slot = 0;

    // Get the subenclosure download microcode status that refects the status of
    // of the microcode download/activate.
    status = terminator_eses_get_download_microcode_stat_page_stat_desc(virtual_phy_handle, 
                                                                        &download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;     

    // Get the configuration page information from the Virtual Phy
    status = eses_get_config_diag_page_info(virtual_phy_handle, 
                                            &vp_config_diag_page_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get subencl descriptor info.
    status = config_page_get_subencl_desc_by_subencl_id(
        vp_config_diag_page_info,
        download_ctrl_page_hdr->subenclosure_id,
        &subencl_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Call the terminator interface to do the activate, which will be done in a 
    // seperate thread.

        // **NOTE: The existing terminator interface for activate takes "component
        // type" and only does the activate of that particular component type in the
        // subenclosure, in a different thread. However as per ESES, activate should 
        // be done for all downloaded component types at a time and download/activate status
        // changed as a whole.As the current terminator activate interface does NOT 
        // support that, for now we call the interface with all the possible component
        // types. If a particular component type does NOT have fw downloaded, this interface
        // returns an error before invoking the activate thread and so should be OK. 
        // The below interface should be changed in the future but for now this should be
        // OK as we only do one download and activate at a time.

    subencl_id = download_ctrl_page_hdr->subenclosure_id;
    subencl_slot = vp_config_diag_page_info->config_page_info->subencl_slot[subencl_id];
    status = terminator_enclosure_firmware_activate(virtual_phy_handle,
                                                    subencl_desc->subencl_type,
                                                    subencl_desc->side,
                                                    subencl_slot);
  
    if(status != FBE_STATUS_OK)
    {
        // We did NOT download any microcode earlier to do the activate.
        // return error.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: No microcode downloaded earlier to proceed with activate\n",__FUNCTION__);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_NO_IMAGE; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
                
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    download_stat_desc.status = ESES_DOWNLOAD_STATUS_UPDATING_NONVOL; 
    status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                        download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    return FBE_STATUS_OK;
}


/**************************************************************************
 *  microcode_ctrl_page_handle_download()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Handle the download request in a microcode control diagnostic page.
 *
 *  PARAMETERS:
 *     microcode control diagnostic page header ptr, virtual phy object
 *     object handle, sense buffer to be set on some error.
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-8-2010: Rajesh V created.
 **************************************************************************/
fbe_status_t 
microcode_ctrl_page_handle_download(
    fbe_eses_download_control_page_header_t *download_ctrl_page_hdr,
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t *sense_buffer)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_download_status_desc_t download_stat_desc;
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info;
    fbe_u32_t buffer_offset, microcode_image_length, microcode_data_length;

    buffer_offset = BYTE_SWAP_32(download_ctrl_page_hdr->buffer_offset);
    microcode_image_length = BYTE_SWAP_32(download_ctrl_page_hdr->image_length);
    microcode_data_length = BYTE_SWAP_32(download_ctrl_page_hdr->transfer_length);

    // Get the subenclosure download microcode status that refects the status of
    // of the microcode download/activate.
    status = terminator_eses_get_download_microcode_stat_page_stat_desc(virtual_phy_handle, 
                                                                        &download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;  


    // Get the pointer to the Virtual Phy download ctrl page information 
    // Which will have the buffer for downloading the microcode image and
    // also have other structures that will be updated according to the 
    // download ctrl page information that we are currently receiving from
    // the client.
    status = terminator_eses_get_vp_download_microcode_ctrl_page_info(virtual_phy_handle,
                                                                      &vp_download_microcode_ctrl_page_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Do some validations specific to download.
    // Removed the check for (microcode_data_length == 0)
    // The reason is that when the upgrade is aborted, the physical package could send
    // the download ctrl page with 0 data size.
    if(microcode_image_length == 0) 
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "microcode_ctrl_page_handle_download:Invalid values in ctrl page,image_length:%d,data_length:%d\n", 
                         microcode_image_length, 
                         microcode_data_length);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    if((vp_download_microcode_ctrl_page_info->image_download_in_progress == TRUE) &&
       (vp_download_microcode_ctrl_page_info->subenclosure_id != 
        download_ctrl_page_hdr->subenclosure_id))
    {
        /* A download was already going on and we got download for an other subenclosure
         * It is possible that the previous one already was aborted in the physical package.
         */
        terminator_trace(FBE_TRACE_LEVEL_INFO, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "microcode_ctrl_page_handle_download:Microcode download req for subencl ID:%d,prev subencl ID: %d\n", 
                         download_ctrl_page_hdr->subenclosure_id,
                         vp_download_microcode_ctrl_page_info->subenclosure_id);

        vp_download_microcode_ctrl_page_info->subenclosure_id = 
            download_ctrl_page_hdr->subenclosure_id;
        vp_download_microcode_ctrl_page_info->buffer_id = 
            download_ctrl_page_hdr->buffer_id;
        vp_download_microcode_ctrl_page_info->image_length =
            microcode_image_length;

        if(vp_download_microcode_ctrl_page_info->image_buffer != NULL) 
        {
            fbe_terminator_free_memory(vp_download_microcode_ctrl_page_info->image_buffer);
        }

        vp_download_microcode_ctrl_page_info->image_buffer =
            (fbe_u8_t *)fbe_terminator_allocate_memory(microcode_image_length * sizeof(fbe_u8_t));
        if (vp_download_microcode_ctrl_page_info->image_buffer == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for image buffer at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_zero_memory(vp_download_microcode_ctrl_page_info->image_buffer, microcode_image_length * sizeof(fbe_u8_t));

        // Set the download status to indicate the download has started.
        download_stat_desc.status = ESES_DOWNLOAD_STATUS_IN_PROGRESS; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
    }
    else if((vp_download_microcode_ctrl_page_info->image_download_in_progress == FALSE) ||
            (vp_download_microcode_ctrl_page_info->subenclosure_id != 
             download_ctrl_page_hdr->subenclosure_id))
    {
        // First send diagnostic page received for the microcode download

        vp_download_microcode_ctrl_page_info->image_download_in_progress = TRUE;
        vp_download_microcode_ctrl_page_info->subenclosure_id = 
            download_ctrl_page_hdr->subenclosure_id;
        vp_download_microcode_ctrl_page_info->buffer_id = 
            download_ctrl_page_hdr->buffer_id;
        vp_download_microcode_ctrl_page_info->image_length =
            microcode_image_length;
        vp_download_microcode_ctrl_page_info->image_buffer = 
            (fbe_u8_t *)fbe_terminator_allocate_memory(microcode_image_length * sizeof(fbe_u8_t));
        if (vp_download_microcode_ctrl_page_info->image_buffer == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for image buffer at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_zero_memory(vp_download_microcode_ctrl_page_info->image_buffer, microcode_image_length * sizeof(fbe_u8_t));

        // Set the download status to indicate the download has started.
        download_stat_desc.status = ESES_DOWNLOAD_STATUS_IN_PROGRESS; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
    }

    if(vp_download_microcode_ctrl_page_info->image_length != microcode_image_length)
    {
        // This ctrl page has a different image length than the one reported by 
        // previous ctrl pages.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "microcode_ctrl_page_handle_download:Image length expected:%d, received:%d\n", 
                         vp_download_microcode_ctrl_page_info->image_length,
                         microcode_image_length);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE); 

    }

    if((buffer_offset + microcode_data_length) > 
       (vp_download_microcode_ctrl_page_info->image_length))
    {
        // The size of the microcode image we got is greater than the expected value.
        // This "expected value" is the one we assumed from the earlier send diagnostic
        // pages for this download. This is not allowed.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "microcode_ctrl_page_handle_download:Img size exceeds expected len:%d,offset:%d,data_len:%d\n", 
                         vp_download_microcode_ctrl_page_info->image_length,
                         buffer_offset,
                         microcode_data_length);

        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);        
    }

    // Download the partial image to the buffer in the Virtual Phy, starting at the 
    // given buffer offset.
    fbe_copy_memory( ( vp_download_microcode_ctrl_page_info->image_buffer + buffer_offset),
                     ( ((fbe_u8_t *)download_ctrl_page_hdr) + FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET),
                     microcode_data_length);

    // Check if we downloaded the complete image at this point
    if((buffer_offset + microcode_data_length) == 
       (vp_download_microcode_ctrl_page_info->image_length))
    {
        // Call the interface to handle the downloaded/saved image.
        status = microcode_ctrl_page_image_download_complete(virtual_phy_handle, sense_buffer); 
        RETURN_ON_ERROR_STATUS;         
        
        // Call the interface to reset information structures used during the microcode
        // image download , so that they can be used by the next download operation.
        // NOTE: We only support one download at a time.
        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(vp_download_microcode_ctrl_page_info);
        
    }

    return FBE_STATUS_OK;
                     
}

/**************************************************************************
 *  microcode_ctrl_page_image_download_complete()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This interface is called when the the image in the microcode control 
 *    diagnostic page has been completely downloaded to our internal buffer,
 *    so that it can parse the downloaded image/header and update any internal
 *    structures as necessary.
 *    
 *
 *  PARAMETERS:
 *     virtual phy object object handle
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-14-2010: Rajesh V created.
 **************************************************************************/
fbe_status_t 
microcode_ctrl_page_image_download_complete(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u8_t *sense_buffer)
{
    fbe_status_t status;
    fbe_u32_t component_type;
    fbe_u32_t component_type_big_endian;
    fbe_u8_t *image;
    fbe_u8_t *image_rev;
    fbe_download_status_desc_t download_stat_desc;
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info;
    vp_config_diag_page_info_t *vp_config_diag_page_info;
    ses_subencl_desc_struct *subencl_desc;
    fbe_u8_t subencl_id = 0;
    fbe_u8_t subencl_slot = 0;
    fbe_u16_t eses_version_desc = 0;

    // Get the download microcode status
    status = terminator_eses_get_download_microcode_stat_page_stat_desc(virtual_phy_handle, 
                                                                        &download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;  

    // Set the download status to indicate that the image download was complete 
    // and we are updating non-volataile storage.
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_UPDATING_FLASH; 
    status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                        download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the download microcode control page information from Virtual Phy
    status = terminator_eses_get_vp_download_microcode_ctrl_page_info(virtual_phy_handle,
                                                                      &vp_download_microcode_ctrl_page_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;  
    
    // Do some validation on the existing microcode control page info.
    if((vp_download_microcode_ctrl_page_info->image_download_in_progress == FALSE) ||
       (vp_download_microcode_ctrl_page_info->image_length == 0) || 
       (vp_download_microcode_ctrl_page_info->image_buffer == NULL))
    {
        // There is no image in the image buffer. Some thing went wrong.

        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s:No image downloaded, download_in_progr:%d,image len:%d,image buf:%p\n", 
                         __FUNCTION__,
                         vp_download_microcode_ctrl_page_info->image_download_in_progress,
                         vp_download_microcode_ctrl_page_info->image_length,
                         vp_download_microcode_ctrl_page_info->image_buffer);
        
        microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
            vp_download_microcode_ctrl_page_info);

        download_stat_desc.status = ESES_DOWNLOAD_STATUS_ERROR_BACKUP; 
        status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                            download_stat_desc);
        RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED,
                                FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED);
        return(FBE_STATUS_GENERIC_FAILURE);        
    }

    vp_download_microcode_ctrl_page_info->image_download_in_progress = FALSE;

    image = (fbe_u8_t *)(vp_download_microcode_ctrl_page_info->image_buffer);

    /* Get the eses_version_desc ESES_REVISION_1_0 or ESES_REVISION_2_0 */
    fbe_terminator_sas_enclosure_get_eses_version_desc(virtual_phy_handle, &eses_version_desc);

    // Get version descriptor component type and image revision from the downloaded
    // image
    if(eses_version_desc == ESES_REVISION_1_0) 
    {
        
        component_type = image[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET]; 
        image_rev = &image[FBE_ESES_MCODE_IMAGE_REV_OFFSET];
    }
    else
    {
        fbe_copy_memory(&component_type_big_endian, 
                        &image[FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_OFFSET], 
                        FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_SIZE_BYTES); 

        component_type = swap32(component_type_big_endian); 
        image_rev = &image[FBE_ESES_CDES2_MCODE_IMAGE_REV_OFFSET];
    }

    // Get the configuration page information from the Virtual Phy
    status = eses_get_config_diag_page_info(virtual_phy_handle, 
                                            &vp_config_diag_page_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    status = config_page_get_subencl_desc_by_subencl_id(
        vp_config_diag_page_info,
        vp_download_microcode_ctrl_page_info->subenclosure_id,
        &subencl_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;
        
    subencl_id = vp_download_microcode_ctrl_page_info->subenclosure_id;
    subencl_slot = vp_config_diag_page_info->config_page_info->subencl_slot[subencl_id];
    // Update the image revision in the intermediate structures which will be used
    // during image activation. 
    status = terminator_enclosure_firmware_download(
        virtual_phy_handle,
        subencl_desc->subencl_type,
        subencl_desc->side,
        component_type,
        subencl_slot,
        (CHAR *)image_rev);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Set the download status to indicate that the image download was complete 
    // and that we updated NON VOLATAILE MEM and are awaiting activate.
    download_stat_desc.status = ESES_DOWNLOAD_STATUS_NEEDS_ACTIVATE; 
    status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_handle,
                                                                        download_stat_desc);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);                                                 
}

/**************************************************************************
 *  microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This interface is called when we want to reset the Virtual Phy internal
 *    structures(download microcode ctrl page information) that have the state & 
 *    other information about the current microcode download that is going on. It
 *    will also free any buffers from previous download
 *    
 *
 *  PARAMETERS:
 *     pointer to virtual phy microcode ctrl information to be reset.
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *   This is called either when some unexpected error occurs during current download or
 *   when the download of the image and parsing of the image are complete. Only after
 *   this interface is called can EMA start accepting a new image download in the microcode
 *   control diagnostic page.
 *
 *  HISTORY:
 *      Oct-14-2010: Rajesh V created.
 **************************************************************************/
void 
microcode_ctrl_page_reset_vp_download_microcode_ctrl_page_info(
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_page_info)
{
    terminator_free_mem_on_not_null(vp_download_microcode_ctrl_page_info->image_buffer);

    fbe_zero_memory(vp_download_microcode_ctrl_page_info,
                    sizeof(vp_download_microcode_ctrl_diag_page_info_t));
}

/**************************************************************************
 *  download_ctrl_page_initialize_page_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function initializes the download ctrl page structures in Virtual
 *    Phy 
 *
 *  PARAMETERS:
 *     pointer to Virtual Phy download ctrl page information
 *
 *  RETURN VALUES/ERRORS:
 *     success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *     Oct-5-2010: Rajesh V created.
 **************************************************************************/
fbe_status_t 
download_ctrl_page_initialize_page_info(
    vp_download_microcode_ctrl_diag_page_info_t *vp_download_microcode_ctrl_diag_page_info)
{
    fbe_zero_memory(vp_download_microcode_ctrl_diag_page_info, 
                    sizeof(vp_download_microcode_ctrl_diag_page_info_t));
    return FBE_STATUS_OK;
}

