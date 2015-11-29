#include "fbe_terminator.h"
#include "fbe_terminator_device_registry.h"

fbe_status_t fbe_terminator_api_remove_sas_enclosure (fbe_terminator_api_device_handle_t enclosure_handle)
{
//#ifdef NEW_INSERTIONS
//    /* search for board */
//    fbe_terminator_device_ptr_t  encl_handle;
//    fbe_terminator_device_ptr_t  board_handles[1];
//    fbe_terminator_device_ptr_t  port_handle;
//
//    tdr_status_t tdr_status = fbe_terminator_device_registry_enumerate_devices_by_type(board_handles, 1, FBE_TERMINATOR_DEVICE_TYPE_BOARD);
//    printf("Removing sas enclosure....\n");
//    if(tdr_status != TDR_STATUS_OK)
//    {
//        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Enumeration failed\n");
//    }
//    else
//    {
//        /* find port by its number -- I think that this should be replaced with Registry functions*/
//        port_handle = (fbe_terminator_device_ptr_t)board_get_port_by_number(
//                                                (terminator_board_t *)board_handles[0], port_number);
//
//        /* find enclosure by its ID -- I think that this should also be replaced with Registry functions*/
//        encl_handle = (fbe_terminator_device_ptr_t)port_get_device_by_device_id(
//                                                (terminator_port_t *)port_handle, encl_id);
//
//        /* remove enclosure from device registry */
//        tdr_status = fbe_terminator_device_registry_remove_device(encl_handle);
//        if(tdr_status != TDR_STATUS_OK)
//        {
//            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Remove enclosure from TDR failed\n");
//        }
//    }
//#endif    
    return fbe_terminator_api_remove_device(enclosure_handle);
}

fbe_status_t fbe_terminator_api_pull_sas_enclosure (fbe_terminator_api_device_handle_t enclosure_handle)
{
    return fbe_terminator_api_unmount_device(enclosure_handle);
}

fbe_status_t fbe_terminator_api_get_sas_enclosure_info(fbe_terminator_api_device_handle_t enclosure_handle, fbe_terminator_sas_encl_info_t *encl_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

//  if (port_number >= FBE_PMC_SHIM_MAX_PORTS ) {
//      return status;
//  }
//  status = fbe_terminator_get_sas_enclosure_info(port_number, encl_id, encl_info);
    
    return status;
}
fbe_status_t fbe_terminator_api_set_sas_enclosure_info(fbe_terminator_api_device_handle_t enclosure_handle, fbe_terminator_sas_encl_info_t  encl_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

//  if (port_number >= FBE_PMC_SHIM_MAX_PORTS ) {
//      return status;
//  }
//  status = fbe_terminator_set_sas_enclosure_info(port_number, encl_id, encl_info);
    return status;
}


fbe_status_t fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                         fbe_u32_t slot_number, 
                                                                         ses_stat_elem_array_dev_slot_struct drive_slot_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_set_drive_eses_status(virtual_phy_ptr, slot_number, drive_slot_stat);
    return status;
}

fbe_status_t fbe_terminator_api_get_sas_enclosure_drive_slot_eses_status(fbe_terminator_api_device_handle_t enclosure_handle, 
                                                                         fbe_u32_t slot_number, 
                                                                         ses_stat_elem_array_dev_slot_struct *drive_slot_stat)

{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_drive_eses_status(virtual_phy_ptr, slot_number, drive_slot_stat);
    return status;
}

fbe_status_t fbe_terminator_api_set_sas_enclosure_phy_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                  fbe_u32_t phy_number,
                                                                  ses_stat_elem_exp_phy_struct phy_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_set_phy_eses_status(virtual_phy_ptr, phy_number, phy_stat);
    return status;
}
fbe_status_t fbe_terminator_api_get_sas_enclosure_phy_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                  fbe_u32_t phy_number,
                                                                  ses_stat_elem_exp_phy_struct *phy_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_phy_eses_status(virtual_phy_ptr, phy_number, phy_stat);
    return status;
}

/* Interfaces for setting the resume prom buffer information */
fbe_status_t fbe_terminator_api_set_resume_prom_info(fbe_terminator_api_device_handle_t enclosure_handle, 
                                                     terminator_eses_resume_prom_id_t resume_prom_id,
                                                     fbe_u8_t *buf,
                                                     fbe_u32_t len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    ses_subencl_type_enum subencl_type;
    fbe_u8_t side;
    ses_buf_type_enum buf_type;
    fbe_u8_t buf_index;
    fbe_u8_t buf_id_read, buf_id_write;
    fbe_sas_enclosure_type_t encl_type;
    terminator_sp_id_t  spid;

    fbe_terminator_api_get_sp_id(&spid);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: terminator_virtual_phy_get_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    switch(resume_prom_id)
    {
        case LCC_A_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_LCC;
            side = SIDE_A;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = (spid == TERMINATOR_SP_A)? 2 :3;
            break;

        case LCC_B_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_LCC;
            side = SIDE_B;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = (spid == TERMINATOR_SP_A)? 3 :2;
            break;

        case CHASSIS_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_CHASSIS;
            side = SIDE_UNDEFINED;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 1;
            break;

        case PS_A_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_PS;
            side = SIDE_A;
            buf_type = SES_BUF_TYPE_EEPROM;
            if (spid == TERMINATOR_SP_A)
                buf_index = (FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM ==encl_type) ? 6: 4;
            else
                buf_index = (FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM ==encl_type) ? 7: 5;
            break;

        case PS_B_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_PS;
            side = SIDE_B;
            buf_type = SES_BUF_TYPE_EEPROM;
            if (spid == TERMINATOR_SP_A)
                buf_index = (FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM ==encl_type) ? 7: 5;
            else
                buf_index = (FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM ==encl_type) ? 6: 4;
            break;
        
        case FAN_1_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_COOLING;
            side = SIDE_A;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 8;
            break;

        case FAN_2_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_COOLING;
            side = SIDE_B;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 9;
            break;

        case FAN_3_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_COOLING;
            side = SIDE_C;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 10;
            break;
            
        case EE_LCC_A_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_LCC;
            side = SIDE_C;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 4;
            break;

        case EE_LCC_B_RESUME_PROM:
            subencl_type = SES_SUBENCL_TYPE_LCC;
            side = SIDE_D;
            buf_type = SES_BUF_TYPE_EEPROM;
            buf_index = 5;
            break;
            
        default:
           return(FBE_STATUS_GENERIC_FAILURE);
    }


    // Allocate memory for the writable buffer representing the resume prom.
    status = terminator_eses_set_buf(virtual_phy_ptr,
                                    subencl_type,
                                    side,
                                    buf_type,
                                    FBE_TRUE,
                                    1,
                                    FBE_TRUE,
                                    buf_index,
                                    FBE_FALSE,
                                    0,
                                    buf,
                                    len);
    RETURN_ON_ERROR_STATUS;

    // Check if a readable buffer exists for the same resume prom.
    status = terminator_get_buf_id_by_subencl_info(virtual_phy_ptr,
                                                   subencl_type,
                                                   side,
                                                   buf_type,
                                                   FBE_TRUE,
                                                   0,
                                                   FBE_TRUE,
                                                   buf_index,
                                                   FBE_FALSE,
                                                   0,
                                                   &buf_id_read);
    if(status == FBE_STATUS_OK)
    {
        // A readable buffer exists for the same resume prom
        // Let the buffer area of the readable buffer point 
        // to the same buffer area that was just allocated for
        // the writable buffer.

        status = terminator_get_buf_id_by_subencl_info(virtual_phy_ptr,
                                                       subencl_type,
                                                       side,
                                                       buf_type,
                                                       FBE_TRUE,
                                                       1,
                                                       FBE_TRUE,
                                                       buf_index,
                                                       FBE_FALSE,
                                                       0,
                                                       &buf_id_write);
        RETURN_ON_ERROR_STATUS;

        status = terminator_set_bufid2_buf_area_to_bufid1(virtual_phy_ptr,
                                                          buf_id_write,
                                                          buf_id_read);
        RETURN_ON_ERROR_STATUS;

    }

   return(FBE_STATUS_OK);
}

/* Interfaces for setting(writing) the ESES buffer */
fbe_status_t fbe_terminator_api_set_buf(fbe_terminator_api_device_handle_t enclosure_handle, 
                                        ses_subencl_type_enum subencl_type,
                                        fbe_u8_t side,
                                        ses_buf_type_enum buf_type,
                                        fbe_u8_t *buf,
                                        fbe_u32_t len)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_set_buf(virtual_phy_ptr, 
                                     subencl_type,
                                     side,
                                     buf_type,
                                     FBE_FALSE,
                                     0,
                                     FBE_FALSE,
                                     0,
                                     FBE_FALSE,
                                     0,
                                     buf,
                                     len);

    return(status);
} 

fbe_status_t fbe_terminator_api_set_buf_by_buf_id(fbe_terminator_api_device_handle_t enclosure_handle, 
                                                fbe_u8_t buf_id,
                                                fbe_u8_t *buf,
                                                fbe_u32_t len)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_set_buf_by_buf_id(virtual_phy_ptr, buf_id, buf, len);
    return(status);
} 

fbe_status_t fbe_terminator_api_set_enclosure_firmware_activate_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t time_interval)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_set_enclosure_firmware_activate_time_interval(virtual_phy_ptr, time_interval);
    return status;
}

fbe_status_t fbe_terminator_api_get_enclosure_firmware_activate_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t *time_interval)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_enclosure_firmware_activate_time_interval(virtual_phy_ptr, time_interval);
    return status;
}

fbe_status_t fbe_terminator_api_set_enclosure_firmware_reset_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t time_interval)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_set_enclosure_firmware_reset_time_interval(virtual_phy_ptr, time_interval);
    return status;
}

fbe_status_t fbe_terminator_api_get_enclosure_firmware_reset_time_interval(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t *time_interval)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_enclosure_firmware_reset_time_interval(virtual_phy_ptr, time_interval);
    return status;
}

/*********************************************************************
*        fbe_terminator_api_mark_page_unsupported()
*********************************************************************
*
*  This functions marks the given page as unsupported for the ESES
*   code. 
*
*  Inputs:
*   cdb_opcode: "receive diagnostic" or "send diagnostic" opcode 
*       indicating either "status" page or "control" page.
*   diag_page_code: Corresponding  diagnostic page code to be 
*       to be marked as unsupported.
*
*  Return Value: success or failure
*
*  History:
*    Oct08 created
*    
*********************************************************************/
fbe_status_t fbe_terminator_api_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    return(terminator_mark_eses_page_unsupported(cdb_opcode, diag_page_code));
}

fbe_status_t fbe_terminator_api_enclosure_bypass_drive_slot (fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t slot_number)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    ses_stat_elem_exp_phy_struct phy_stat;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    // update if needed
    status = terminator_update_drive_parent_device(&virtual_phy_ptr, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        KvTrace ("%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    status = terminator_get_drive_eses_status(virtual_phy_ptr, slot_number, &drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    status = terminator_set_drive_eses_status(virtual_phy_ptr, slot_number, drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    // User correct drive to slot mapping.
    status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, &phy_id, encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_get_phy_eses_status(virtual_phy_ptr, phy_id, &phy_stat); 
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    phy_stat.phy_rdy = FBE_FALSE;
    // force_disabled indicates EMA is bypassing the drive.
    // client cannot bypass when this bit is set. It
    // is upto the EMA to unbypass again.
    phy_stat.force_disabled = FBE_TRUE; 
    status = terminator_set_phy_eses_status(virtual_phy_ptr, phy_id, phy_stat);

    return(status);
}
fbe_status_t fbe_terminator_api_enclosure_unbypass_drive_slot (fbe_terminator_api_device_handle_t enclosure_handle, fbe_u32_t slot_number)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    ses_stat_elem_exp_phy_struct phy_stat;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    // update if needed
    status = terminator_update_drive_parent_device(&virtual_phy_ptr, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        KvTrace ("%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    status = terminator_get_drive_eses_status(virtual_phy_ptr, 
        slot_number, &drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = terminator_set_drive_eses_status(virtual_phy_ptr, 
        slot_number, drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    // User  correct drive to slot mapping.
    status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, &phy_id, encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_get_phy_eses_status(virtual_phy_ptr, 
       phy_id, &phy_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    if(!drive_slot_stat.dev_off)
    {
        phy_stat.phy_rdy = FBE_TRUE;
    }
    phy_stat.force_disabled = FBE_FALSE;
    status = terminator_set_phy_eses_status(virtual_phy_ptr, 
       phy_id, phy_stat);

    return(status);
}

fbe_status_t fbe_terminator_api_enclosure_getEmcEnclStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                            ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    fbe_sas_enclosure_type_t encl_type;
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_get_emcEnclStatus(virtual_phy_ptr, emcEnclStatusPtr);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

fbe_status_t fbe_terminator_api_enclosure_setEmcEnclStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                            ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    fbe_sas_enclosure_type_t encl_type;
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_set_emcEnclStatus(virtual_phy_ptr, emcEnclStatusPtr);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

fbe_status_t fbe_terminator_api_enclosure_getEmcPsInfoStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                              ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    fbe_sas_enclosure_type_t encl_type;
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_get_emcPsInfoStatus(virtual_phy_ptr, emcPsInfoStatusPtr);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

fbe_status_t fbe_terminator_api_enclosure_setEmcPsInfoStatus (fbe_terminator_api_device_handle_t enclosure_handle, 
                                                              ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;
    fbe_sas_enclosure_type_t encl_type;
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_ptr, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_set_emcPsInfoStatus(virtual_phy_ptr, emcPsInfoStatusPtr);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

fbe_status_t fbe_terminator_api_eses_increment_config_page_gen_code(fbe_terminator_api_device_handle_t enclosure_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_increment_config_page_gen_code(virtual_phy_ptr);

    return(status);
}


fbe_status_t fbe_terminator_api_set_unit_attention(fbe_terminator_api_device_handle_t enclosure_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_virtual_phy_set_unit_attention(virtual_phy_ptr, 1);
    return(status);
}


fbe_status_t fbe_terminator_api_eses_set_ver_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t  side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct ver_desc)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_set_ver_desc(virtual_phy_ptr,
                                        subencl_type,
                                        side,
                                        comp_type,
                                        ver_desc);
    return(status);
}

fbe_status_t fbe_terminator_api_eses_get_ver_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct *ver_desc)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_get_ver_desc(virtual_phy_ptr,
                                        subencl_type,
                                        side,
                                        comp_type,
                                        ver_desc);

    return(status);
}


fbe_status_t fbe_terminator_api_eses_get_subencl_id(fbe_terminator_api_device_handle_t enclosure_handle,
                                                    ses_subencl_type_enum subencl_type,
                                                    fbe_u8_t side,
                                                    fbe_u8_t *subencl_id)
{
fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_get_subencl_id(virtual_phy_ptr, subencl_type, side, subencl_id);
    return(status);
}


fbe_status_t fbe_terminator_api_eses_set_download_microcode_stat_page_stat_desc(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                                fbe_download_status_desc_t download_stat_desc)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_eses_set_download_microcode_stat_page_stat_desc(virtual_phy_ptr, download_stat_desc);
    return(status);
}
fbe_status_t fbe_terminator_api_get_ps_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct *ps_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_ps_eses_status(virtual_phy_ptr, ps_id, ps_stat);
    return status;
}

fbe_status_t fbe_terminator_api_set_ps_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct ps_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_set_ps_eses_status(virtual_phy_ptr, ps_id, ps_stat);
    return status;
}

fbe_status_t fbe_terminator_api_get_sas_conn_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct *sas_conn_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_sas_conn_eses_status(virtual_phy_ptr, sas_conn_id, sas_conn_stat);
    return status;
}

fbe_status_t fbe_terminator_api_set_sas_conn_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct sas_conn_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_set_sas_conn_eses_status(virtual_phy_ptr, sas_conn_id, sas_conn_stat);
    return status;
}

fbe_status_t fbe_terminator_api_get_lcc_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        ses_stat_elem_encl_struct *lcc_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_encl_eses_status(virtual_phy_ptr, lcc_stat);
    return status;
}


fbe_status_t fbe_terminator_api_set_lcc_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        ses_stat_elem_encl_struct lcc_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_set_encl_eses_status(virtual_phy_ptr, lcc_stat);
    return status;
}

fbe_status_t fbe_terminator_api_set_cooling_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct cooling_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    status = terminator_set_cooling_eses_status(virtual_phy_ptr, cooling_id, cooling_stat);
    return status;
}

fbe_status_t fbe_terminator_api_get_cooling_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct *cooling_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_cooling_eses_status(virtual_phy_ptr, cooling_id, cooling_stat);
    return status;
}

fbe_status_t fbe_terminator_api_set_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_set_temp_sensor_eses_status(virtual_phy_ptr, temp_sensor_id, temp_sensor_stat);
    return status;
}

fbe_status_t fbe_terminator_api_get_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_get_temp_sensor_eses_status(virtual_phy_ptr, temp_sensor_id, temp_sensor_stat);
    return status;
}

/**************************************************************************
 *  fbe_terminator_api_set_overall_temp_sensor_eses_status()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the ovarall temperature sensor element of the
 *      given temperature sensor for the given device.
 *
 *  PARAMETERS:
 *      port number, Id of the enclosure containing the temperature sensor,
 *          Id of the temperature sensor, temperature sensor blob to be
 *          written.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t fbe_terminator_api_set_overall_temp_sensor_eses_status(fbe_terminator_api_device_handle_t enclosure_handle,
                                                                    terminator_eses_temp_sensor_id temp_sensor_id,
                                                                    ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    //Get the virtual phy ptr.
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    status = terminator_set_overall_temp_sensor_eses_status(virtual_phy_ptr, temp_sensor_id, temp_sensor_stat);
    return status;
}






/*!****************************************************************************
 * @fn      fbe_terminator_api_enclosure_find_slot_parent ()
 *
 * @brief
 * This function finds the parent enclosure (edge expander in the
 * Voyager) associated with the slot number and overwrites the
 * enclosure_handle with the corresponding handle.
 * This is called with the enclosure handle of the ICM.
 *
 * @param enclosure_handle - pointer to ICM terminator handle.
 * @param slot_number - pointer to the slot number we wish to locate.
 *
 * @return
 * FBE_STATUS_OK if all goes well.  
 * The enclosure_handle is also updated to refer to the particular EE
 * that contains the slot number.  
 * The slot_number is update with the value of slot number local to
 * the EE. That is relative to the EE.
 *
 * HISTORY
 *  06/02/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t fbe_terminator_api_enclosure_find_slot_parent (fbe_terminator_api_device_handle_t *enclosure_handle, fbe_u32_t *slot_number)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    tdr_device_handle_t device_handle;
    tdr_status_t tdr_status;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(*enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }

    status = terminator_update_drive_parent_device(&enclosure_ptr, slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: terminator_update_drive_parent_device failed\n", __FUNCTION__);
        return(status);
    }

    tdr_status = fbe_terminator_device_registry_get_device_handle(enclosure_ptr, &device_handle);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }

    *enclosure_handle = device_handle;

    return FBE_STATUS_OK;
}




/*!****************************************************************************
 * @fn      fbe_terminator_api_get_connector_id_list_for_enclosure ()
 *
 * @brief
 * This function retrives the list of connector ids associated with an
 * enclosure.  Specifically this is implemented for the Voyager which
 * has an enclosure (i.e.  the ICM) with associated edge expanders
 * (i.e.  EEs).  These edge expanders are identified by way of a
 * connector id.  This function returns the list of those ids.  This
 * function should be called with the enclosure handle of the ICM.  It
 * can also be called with the enclosure handle for the the other
 * enclosures (i.e.  Viper, Derringer, etc), but these have no EEs and
 * therefore no connector_ids.  In this case the function will return
 * an empty list of connector_ids.
 *
 * @param enclosure_handle - terminator handle of the enclosure we want to get from.
 * @param connector_ids - fixed sized list of connectors with the count of valid ones.
 *
 * @return
 *  FBE_STATUS_OK if all goes well else an error status. 
 *
 * HISTORY
 *  06/02/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t fbe_terminator_api_get_connector_id_list_for_enclosure (fbe_terminator_api_device_handle_t enclosure_handle,
                                                        fbe_term_encl_connector_list_t *connector_ids)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t enclosure_ptr;
    tdr_status_t tdr_status;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }

    return terminator_get_connector_id_list_for_parent_device(enclosure_ptr, connector_ids);
}
    
