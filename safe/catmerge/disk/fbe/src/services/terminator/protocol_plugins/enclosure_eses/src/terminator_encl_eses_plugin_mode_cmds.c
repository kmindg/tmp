#include "fbe/fbe_types.h"

#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"

/******************************/
/*     local functions        */
/*****************************/
static fbe_status_t process_emc_eses_persistent_mode_page(fbe_u8_t *start_ptr,
                                                        fbe_u8_t **end_ptr,
                                                        fbe_u8_t *sense_buffer,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t process_emc_eses_non_persistent_mode_page(fbe_u8_t *start_ptr,
                                                            fbe_u8_t **end_ptr,
                                                            fbe_u8_t *sense_buffer,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t build_emc_eses_persistent_mode_page(fbe_u8_t *start_ptr,
                                                        fbe_u8_t **end_ptr,
                                                        terminator_eses_mode_param_value_type mode_param_value_type,
                                                        fbe_u8_t *sense_buffer,
                                                        fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t build_emc_eses_non_persistent_mode_page(fbe_u8_t *start_ptr,
                                                            fbe_u8_t **end_ptr,
                                                            terminator_eses_mode_param_value_type mode_param_type,
                                                            fbe_u8_t *sense_buffer,
                                                            fbe_terminator_device_ptr_t virtual_phy_handle);
static fbe_status_t persist_all_current_savable_mode_pages(fbe_u8_t *sense_buffer,
                                                    fbe_terminator_device_ptr_t virtual_phy_handle);

fbe_status_t mode_page_initialize_mode_page_info(terminator_eses_vp_mode_page_info_t *vp_mode_page_info)
{
    fbe_eses_pg_eep_mode_pg_t *default_eep_mode_pg, *current_eep_mode_pg, *saved_eep_mode_pg, *changeable_eep_mode_pg;
    fbe_eses_pg_eenp_mode_pg_t *default_eenp_mode_pg, *current_eenp_mode_pg, *saved_eenp_mode_pg, *changeable_eenp_mode_pg;
    
    memset(vp_mode_page_info, 0, sizeof(terminator_eses_vp_mode_page_info_t));

    default_eep_mode_pg = 
        &vp_mode_page_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_DEFAULT];
    current_eep_mode_pg = 
        &vp_mode_page_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];
    saved_eep_mode_pg = 
        &vp_mode_page_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_SAVED];
    changeable_eep_mode_pg = 
        &vp_mode_page_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CHANGEABLE];

    default_eenp_mode_pg = 
        &vp_mode_page_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_DEFAULT];
    current_eenp_mode_pg = 
        &vp_mode_page_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];
    saved_eenp_mode_pg = 
        &vp_mode_page_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_SAVED];
    changeable_eenp_mode_pg = 
        &vp_mode_page_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CHANGEABLE];

    // Set the saved values for EMC ESES Persistent mode page.
    saved_eep_mode_pg->mode_pg_common_hdr.ps = 1;
    saved_eep_mode_pg->mode_pg_common_hdr.spf = 0;
    saved_eep_mode_pg->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG;
    saved_eep_mode_pg->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN;
    // All other values in EMC ESES persistent mode are zero during initialization (like HA MODE-etc).
    // Copy the saved parameter values of EMC ESES persistent page to default & current parameter values.
    memcpy(default_eep_mode_pg,
           saved_eep_mode_pg,
           sizeof(fbe_eses_pg_eep_mode_pg_t));
    memcpy(current_eep_mode_pg,
           saved_eep_mode_pg,
           sizeof(fbe_eses_pg_eep_mode_pg_t));
    memcpy(changeable_eep_mode_pg,
           saved_eep_mode_pg,
           sizeof(fbe_eses_pg_eep_mode_pg_t));
    // Set the changeable parameters appropriately
    changeable_eep_mode_pg->disable_indicator_ctrl = 1;
    changeable_eep_mode_pg->ssu_disable = 1;
    changeable_eep_mode_pg->ha_mode = 1;
    changeable_eep_mode_pg->bad_exp_recovery_enabled = 1;


    // Set the saved values for EMC ESES NON Persistent mode page.
    saved_eenp_mode_pg->mode_pg_common_hdr.ps = 0;
    saved_eenp_mode_pg->mode_pg_common_hdr.spf = 0;
    saved_eenp_mode_pg->mode_pg_common_hdr.pg_code = SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG;
    saved_eenp_mode_pg->mode_pg_common_hdr.pg_len = FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN;
    // All other values in EMC ESES NON persistent mode are zero during initialization (like TEST MODE-etc).
    // Copy the saved values of EMC ESES persistent page to default & current pages.
    memcpy(default_eenp_mode_pg,
           saved_eenp_mode_pg,
           sizeof(fbe_eses_pg_eenp_mode_pg_t));
    memcpy(current_eenp_mode_pg,
           saved_eenp_mode_pg,
           sizeof(fbe_eses_pg_eenp_mode_pg_t));
    memcpy(changeable_eenp_mode_pg,
           saved_eenp_mode_pg,
           sizeof(fbe_eses_pg_eenp_mode_pg_t));
    // Set the changeable parameters appropriately.
    changeable_eenp_mode_pg->test_mode = 1;

    return(FBE_STATUS_OK);                   
}

fbe_status_t process_payload_mode_sense_10(fbe_payload_ex_t * payload, 
                                           fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_eses_mode_sense_10_cdb_t *mode_sense_cdb;
	fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer;
    fbe_u8_t pg_code, subpg_code;
    fbe_eses_mode_param_list_t *mode_param_list;
    fbe_u8_t *pg_start_ptr = NULL, *pg_end_ptr = NULL;

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

    mode_sense_cdb = (fbe_eses_mode_sense_10_cdb_t *)(payload_cdb_operation->cdb);
    pg_code = mode_sense_cdb->pg_code;
    subpg_code = mode_sense_cdb->subpg_code;

    // Get the sg list
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if(sg_list->count == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    mode_param_list = (fbe_eses_mode_param_list_t *)fbe_sg_element_address(sg_list);
    memset(mode_param_list, 0, sizeof(fbe_eses_mode_param_list_t));
    pg_start_ptr = (fbe_u8_t *)(mode_param_list + 1);

    if((pg_code == 0x20) && (subpg_code == 0x00))
    {
        // Asking for EMC persistent mode page.
        status = build_emc_eses_persistent_mode_page(pg_start_ptr,
                                                     &pg_end_ptr,
                                                     mode_sense_cdb->pc,
                                                     sense_buffer,
                                                     virtual_phy_handle);
    }
    else if((pg_code == 0x21) && (subpg_code == 0x00))
    {
        // Asking for EMC Non-Persistent mode page.
        status = build_emc_eses_non_persistent_mode_page(pg_start_ptr,
                                                        &pg_end_ptr,
                                                        mode_sense_cdb->pc,
                                                        sense_buffer,
                                                        virtual_phy_handle);
    }
    else if((pg_code == 0x3F) && ((subpg_code == 0x00) || (subpg_code == 0xFF)))
    {
        // Asking for All the supported pages.

        status = build_emc_eses_persistent_mode_page(pg_start_ptr,
                                                     &pg_end_ptr,
                                                     mode_sense_cdb->pc,
                                                     sense_buffer,
                                                     virtual_phy_handle);
        if(status == FBE_STATUS_OK)
        {
            pg_start_ptr = pg_end_ptr;
            status = build_emc_eses_non_persistent_mode_page(pg_start_ptr,
                                                            &pg_end_ptr,
                                                            mode_sense_cdb->pc,
                                                            sense_buffer,
                                                            virtual_phy_handle);
        }

        // As more mode pages are supported, they are added here.
    }
    else
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        status = FBE_STATUS_GENERIC_FAILURE;

    }

    if(status != FBE_STATUS_OK)
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);        
    }

    mode_param_list->mode_data_len = 
        (fbe_u16_t)(((fbe_u8_t *)(pg_end_ptr)) - ((fbe_u8_t *)(mode_param_list)) - 2);
    mode_param_list->mode_data_len = 
        BYTE_SWAP_16(mode_param_list->mode_data_len);

    fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
    fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS); 

    return(FBE_STATUS_OK);

} // End of Mode Sense

fbe_status_t process_payload_mode_select_10(fbe_payload_ex_t * payload, 
                                            fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_eses_mode_select_10_cdb_t *mode_select_cdb;
	fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer;
    fbe_eses_mode_param_list_t *mode_param_list;
    fbe_u8_t *pg_start_ptr = NULL, *pg_end_ptr = NULL;
    fbe_u16_t bytes_read = 0, bytes_to_read = 0;
    fbe_bool_t save_pages = FBE_FALSE;
    fbe_u16_t param_list_len;

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

    mode_select_cdb = (fbe_eses_mode_select_10_cdb_t *)(payload_cdb_operation->cdb);
    save_pages = mode_select_cdb->sp;
    param_list_len = 0x100 * (mode_select_cdb->param_list_length_msbyte) +
                     0x1 * (mode_select_cdb->param_list_length_lsbyte);

    // Get the sg list
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if(sg_list->count == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    mode_param_list = (fbe_eses_mode_param_list_t *)fbe_sg_element_address(sg_list);
    pg_start_ptr = (fbe_u8_t *)(mode_param_list + 1);
    bytes_to_read = (fbe_u16_t)
        (param_list_len - sizeof(fbe_eses_mode_param_list_t));

    bytes_read = 0;
    while(bytes_read < bytes_to_read)
    {
        switch(*pg_start_ptr)
        {
            case SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG:
                status = process_emc_eses_persistent_mode_page(pg_start_ptr,
                                                               &pg_end_ptr,
                                                               sense_buffer,
                                                               virtual_phy_handle);


                break;
            case SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG:
                status = process_emc_eses_non_persistent_mode_page(pg_start_ptr,
                                                                &pg_end_ptr,
                                                                sense_buffer,
                                                                virtual_phy_handle);
                break;
            default:
                build_sense_info_buffer(sense_buffer, 
                                        FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                        FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
                                        FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;

        }
        if(status != FBE_STATUS_OK)
        {
            break;
        }

        bytes_read += ((fbe_u16_t)(pg_end_ptr - pg_start_ptr));
        pg_start_ptr = pg_end_ptr;
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);       
    }
    else
    {

        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);

        // Check if the client asked us to save all the current savable mode pages
        // in the Persisent memory.
        if(save_pages)
        {
            status = persist_all_current_savable_mode_pages(sense_buffer,
                                                            virtual_phy_handle);
            if(status != FBE_STATUS_OK)
            {
                fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
                fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);  
            }
        }
    }

    return(FBE_STATUS_OK);

} // End of Mode Select


fbe_status_t process_emc_eses_persistent_mode_page(fbe_u8_t *start_ptr,
                                                   fbe_u8_t **end_ptr,
                                                   fbe_u8_t *sense_buffer,
                                                   fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_eep_mode_pg_t   *given_eep_mode_pg = NULL, *current_eep_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the current values of the EMC ESES Persistent mode page
    current_eep_mode_pg = 
        &vp_mode_pg_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];

    // Get the EMC ESES persistent mode page provided by the client.
    given_eep_mode_pg = (fbe_eses_pg_eep_mode_pg_t *)start_ptr;

    // Perform some checks on the EEP page provided by the client
    // In future also add checks for reserved bits
    // being zero, if required.
    if((given_eep_mode_pg->mode_pg_common_hdr.ps != 0) ||
       (given_eep_mode_pg->mode_pg_common_hdr.spf != 0) ||
       (given_eep_mode_pg->mode_pg_common_hdr.pg_len != 
            FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN))
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        return(FBE_STATUS_GENERIC_FAILURE);
    }


    // Copy the client provided EMC ESES Persistent mode page data
    // to the current values of EEP mode page in the virtual phy
    current_eep_mode_pg->disable_indicator_ctrl = given_eep_mode_pg->disable_indicator_ctrl;
    current_eep_mode_pg->ssu_disable = given_eep_mode_pg->ssu_disable;
    current_eep_mode_pg->ha_mode = given_eep_mode_pg->ha_mode;
    current_eep_mode_pg->bad_exp_recovery_enabled = given_eep_mode_pg->bad_exp_recovery_enabled;

    *end_ptr = (fbe_u8_t *)(given_eep_mode_pg + 1);

    return(FBE_STATUS_OK);
}


fbe_status_t process_emc_eses_non_persistent_mode_page(fbe_u8_t *start_ptr,
                                                       fbe_u8_t **end_ptr,
                                                       fbe_u8_t *sense_buffer,
                                                       fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_eenp_mode_pg_t  *given_eenp_mode_pg = NULL, *current_eenp_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the current values of the EMC ESES non Persistent mode page
    current_eenp_mode_pg = 
        &vp_mode_pg_info->eenp_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];

    // Get the EMC ESES non persistent mode page provided by the client.
    given_eenp_mode_pg = (fbe_eses_pg_eenp_mode_pg_t *)start_ptr;

    // Perform some checks on the EENP page provided by the client
    // In future also add checks for reserved bits
    // being zero, if required.
    if((given_eenp_mode_pg->mode_pg_common_hdr.ps != 0) ||
       (given_eenp_mode_pg->mode_pg_common_hdr.spf != 0) ||
       (given_eenp_mode_pg->mode_pg_common_hdr.pg_len != 
        FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN))
//       FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN) ||
//   given_eenp_mode_pg->sps_dev_supported)
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST,
//                            FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO);
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Copy the client provided EMC ESES non persistent mode page data
    // to the current values of EENP mode page in the virtual phy
    current_eenp_mode_pg->test_mode = given_eenp_mode_pg->test_mode;
    current_eenp_mode_pg->sps_dev_supported = given_eenp_mode_pg->sps_dev_supported;

    *end_ptr = (fbe_u8_t *)(given_eenp_mode_pg + 1);

    return(FBE_STATUS_OK);
}

fbe_status_t build_emc_eses_persistent_mode_page(fbe_u8_t *start_ptr,
                                                fbe_u8_t **end_ptr,
                                                terminator_eses_mode_param_value_type mode_param_value_type,
                                                fbe_u8_t *sense_buffer,
                                                fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_eep_mode_pg_t   *eep_mode_pg = NULL, *requested_eep_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the existing EMC ESES persistent mode page data of the 
    // given mode parameter value(saved, current, changeable, default)
    requested_eep_mode_pg = 
        &vp_mode_pg_info->eep_mode_pg[mode_param_value_type];

    eep_mode_pg = (fbe_eses_pg_eep_mode_pg_t *)start_ptr;
    // Copy the requeted EMC ESES Persistent mode page data
    // in the virtual phy TO the passed buffer.
    memcpy(eep_mode_pg, requested_eep_mode_pg, sizeof(fbe_eses_pg_eep_mode_pg_t));

    *end_ptr = (fbe_u8_t *)(eep_mode_pg + 1);

    return(FBE_STATUS_OK);
}


fbe_status_t build_emc_eses_non_persistent_mode_page(fbe_u8_t *start_ptr,
                                                    fbe_u8_t **end_ptr,
                                                    terminator_eses_mode_param_value_type mode_param_value_type,
                                                    fbe_u8_t *sense_buffer,
                                                    fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_eenp_mode_pg_t  *eenp_mode_pg = NULL, *requested_eenp_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the existing EMC ESES non-persistent mode page data of the 
    // given mode parameter value(saved, current, changeable, default)
    requested_eenp_mode_pg = 
        &vp_mode_pg_info->eenp_mode_pg[mode_param_value_type];

    eenp_mode_pg = (fbe_eses_pg_eenp_mode_pg_t *)start_ptr;
    // Copy the requeted EMC ESES Persistent mode page data
    // in the virtual phy TO the passed buffer.
    memcpy(eenp_mode_pg, requested_eenp_mode_pg, sizeof(fbe_eses_pg_eenp_mode_pg_t));

    *end_ptr = (fbe_u8_t *)(eenp_mode_pg + 1);

    return(FBE_STATUS_OK);
} 


fbe_status_t persist_all_current_savable_mode_pages(fbe_u8_t *sense_buffer,
                                                    fbe_terminator_device_ptr_t virtual_phy_handle)
{
     fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_eses_pg_eep_mode_pg_t   *current_eep_mode_pg = NULL, *saved_eep_mode_pg = NULL;
    terminator_eses_vp_mode_page_info_t *vp_mode_pg_info = NULL;
    
    // Get the pointer to the mode page information.
    status = eses_get_mode_page_info(virtual_phy_handle, &vp_mode_pg_info);
    RETURN_EMA_NOT_READY_ON_ERROR_STATUS;

    // Get the saved & current EMC ESES Persistent mode page values.
    current_eep_mode_pg = 
        &vp_mode_pg_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_CURRENT];
    saved_eep_mode_pg = 
        &vp_mode_pg_info->eep_mode_pg[TERMINATOR_ESES_MODE_PARAMETER_VALUE_TYPE_SAVED];

    // Copy the current values of EEP into the saved values(indicating
    // we are persisting them in NON-Volataile memory).
     memcpy(saved_eep_mode_pg, current_eep_mode_pg, sizeof(fbe_eses_pg_eep_mode_pg_t));

    // EMC Enclosure NON-Persistent values Cannot be saved (persisted)

    // As we add support for more "savable" mode pages, we will have code here to
    // save(persist) their current values as well.

    return(FBE_STATUS_OK);
} 

















    
