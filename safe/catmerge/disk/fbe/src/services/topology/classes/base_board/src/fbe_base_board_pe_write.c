/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008 - 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_board_pe_write.c
 ***************************************************************************
 * @brief
 *  The routines in this file write the specl data.   
 *  
 * HISTORY:
 *  10-Mar-2010 PHE - Created.                  
 ***************************************************************************/

#include "base_board_pe_private.h"
#include "fbe_transport_memory.h"
#include "edal_base_enclosure_data.h"
#include "edal_processor_enclosure_data.h"
#include "specl_types.h"


/*!*************************************************************************
* @fn fbe_base_board_check_mgmt_vlan_mode_command            
***************************************************************************
*
* @brief
*       This function checks that the requested vlan mode setting was
*       written out successfully by comparing the requested value to the
*       value that was just read back.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_mgmt_vlan_mode_command(fbe_base_board_t *base_board, 
                                            fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_mgmt_vlan_mode_t     *mgmt_vlan_mode;
    edal_pe_mgmt_module_sub_info_t          peMgmtModuleSubInfo;
    fbe_u32_t                               component_index = 0;
    fbe_u32_t                               component_count = 0;
    fbe_edal_block_handle_t                 edalBlockHandle = NULL;
    fbe_edal_status_t                       edalStatus;

    fbe_payload_control_get_buffer(control_operation, &mgmt_vlan_mode);

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);
    if (edalBlockHandle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                    FBE_ENCL_MGMT_MODULE, 
                    &component_count);

    for(component_index = 0; component_index < component_count; component_index++)
    {
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                    FBE_ENCL_MGMT_MODULE,  // component type
                                    component_index,  // component index
                                    sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                    (fbe_u8_t *)&peMgmtModuleSubInfo);  // buffer pointer

        if(peMgmtModuleSubInfo.externalMgmtModuleInfo.associatedSp == mgmt_vlan_mode->sp_id)
        {
            // all components for this SP must match the mode issued in the write command or it is a failure.
            if(peMgmtModuleSubInfo.externalMgmtModuleInfo.vLanConfigMode == mgmt_vlan_mode->vlan_config_mode)
            {
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }
    }
    return status;
}


/*!*************************************************************************
* @fn fbe_base_board_check_mgmt_port_command            
***************************************************************************
*
* @brief
*       This function checks that the requested management port setting was
*       written out successfully by comparing the requested value to the
*       value that was just read back.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_mgmt_port_command(fbe_base_board_t *base_board, 
                                            fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_mgmt_port_t          *mgmt_port;
    edal_pe_mgmt_module_sub_info_t          peMgmtModuleSubInfo;
    fbe_u32_t                               component_index = 0;
    fbe_u32_t                               component_count = 0;
    fbe_edal_block_handle_t                 edalBlockHandle = NULL;
    fbe_edal_status_t                       edalStatus;

    fbe_payload_control_get_buffer(control_operation, &mgmt_port);

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);
    if (edalBlockHandle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                    FBE_ENCL_MGMT_MODULE, 
                    &component_count);

    for(component_index = 0; component_index < component_count; component_index++)
    {
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                    FBE_ENCL_MGMT_MODULE,  // component type
                                    component_index,  // component index
                                    sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                    (fbe_u8_t *)&peMgmtModuleSubInfo);  // buffer pointer

        if( (peMgmtModuleSubInfo.externalMgmtModuleInfo.associatedSp == mgmt_port->sp_id) &&
           (peMgmtModuleSubInfo.externalMgmtModuleInfo.mgmtID == mgmt_port->mgmtID))
        {
            if(mgmt_port->portIDType == MANAGEMENT_PORT0)
            {
                if(mgmt_port->mgmtPortConfig.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_ON)
                {
                    if(peMgmtModuleSubInfo.externalMgmtModuleInfo.managementPort.mgmtPortAutoNeg == mgmt_port->mgmtPortConfig.mgmtPortAutoNeg)
                    {
                        status = FBE_STATUS_OK;
                    }
                    else
                    {
                        status = FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    // all components for this SP must match the mode issued in the write command or it is a failure.
                    if((peMgmtModuleSubInfo.externalMgmtModuleInfo.managementPort.mgmtPortAutoNeg == mgmt_port->mgmtPortConfig.mgmtPortAutoNeg) &&
                       (peMgmtModuleSubInfo.externalMgmtModuleInfo.managementPort.mgmtPortSpeed == mgmt_port->mgmtPortConfig.mgmtPortSpeed) &&
                       (peMgmtModuleSubInfo.externalMgmtModuleInfo.managementPort.mgmtPortDuplex == mgmt_port->mgmtPortConfig.mgmtPortDuplex))
                    {
                        status = FBE_STATUS_OK;
                    }
                    else
                    {
                        status = FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                break;
            }
        }
    }
    return status;
}


/*!*************************************************************************
* @fn fbe_base_board_check_iom_fault_led_command            
***************************************************************************
*
* @brief
*       This function checks that the requested io module fault led setting was
*       written out successfully by comparing the requested value to the
*       value that was just read back.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_iom_fault_led_command(fbe_base_board_t *base_board, 
                                               fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_iom_fault_LED_t      *iomFaultLEDPtr = NULL;
    edal_pe_io_comp_sub_info_t              PeIoModuleSubInfo;
    fbe_u32_t                               component_index = 0;
    fbe_u32_t                               component_count = 0;
    fbe_edal_block_handle_t                 edalBlockHandle = NULL;
    fbe_edal_status_t                       edalStatus;

    fbe_payload_control_get_buffer(control_operation, &iomFaultLEDPtr);
    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    if (edalBlockHandle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                    FBE_ENCL_MGMT_MODULE, 
                    &component_count);

    for(component_index = 0; component_index < component_count; component_index++)
    {
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                        FBE_ENCL_IO_MODULE,  // component type
                                        component_index,  // component index
                                        sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&PeIoModuleSubInfo);  // buffer pointer


        if( (PeIoModuleSubInfo.externalIoCompInfo.associatedSp == iomFaultLEDPtr->sp_id) &&
            (PeIoModuleSubInfo.externalIoCompInfo.slotNumOnBlade == iomFaultLEDPtr->slot) )
        {
            // all components for this SP must match the mode issued in the write command or it is a failure.
            if(PeIoModuleSubInfo.externalIoCompInfo.faultLEDStatus == iomFaultLEDPtr->blink_rate)
            {
                status = FBE_STATUS_OK;
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }
    }
    return status;
}

/*!*************************************************************************
* @fn fbe_base_board_check_hold_in_post_and_or_reset            
***************************************************************************
*
* @brief
*       This function checks that the reboot request completed successfully.
*       For the local sp, if we are here, then we didn't reboot.  For the
*       peer SP we check that the boot status has changed from when we
*       requested the reboot.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    11-Feb-2011: bphilbin - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_hold_in_post_and_or_reset(fbe_base_board_t *base_board, 
                                               fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_PostAndOrReset_t     *post_reset = NULL;
    fbe_board_mgmt_misc_info_t              miscInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status;
    SPECL_MISC_SUMMARY                      SpeclMiscSum;
    fbe_payload_control_get_buffer(control_operation, &post_reset);

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_MISC_INFO, //Attribute
                                     FBE_ENCL_MISC, // component type
                                     0, // comonent index
                                     sizeof(fbe_board_mgmt_misc_info_t), // buffer length
                                     (fbe_u8_t *)&miscInfo); // buffer pointer 

    /*
     * For the peer SP we will check the peer status in the fbe_pe_flt_exp_info_t
     * structure and verify that it has changed after issuing the reboot request. 
     * For the local SP if we can check, we didn't reboot. 
     */

    // local sp
    if(post_reset->sp_id == miscInfo.spid)
    {
        status = fbe_base_board_check_local_sp_reboot(base_board);
    }
    else
    {
        if(post_reset->holdInReset)
        {
            status = fbe_base_board_pe_get_misc_status(&SpeclMiscSum);
            if(status == FBE_STATUS_OK)
            {
                if(SpeclMiscSum.peerHoldInReset)
                {
                    return FBE_STATUS_OK;
                }
                else
                {
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
            }        
        }
        else
        {
            status = fbe_base_board_check_peer_sp_reboot(base_board);
        }
        if (status != FBE_STATUS_OK && post_reset->retryStatusCount) 
        {
            post_reset->retryStatusCount--;
            return FBE_STATUS_PENDING;
        }
    }
    return status;

}

/*!*************************************************************************
* @fn fbe_base_board_check_last_resume_prom_write_state
***************************************************************************
*
* @brief
*       This function checks if the last resume prom write successed.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    28-Aug-2012: dongz - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_resume_prom_last_write_status(fbe_base_board_t *base_board,
                                                               fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_resume_prom_cmd_t                   *pWriteResumeCmd = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    SPECL_RESUME_DATA                       *pSpeclResumeData = NULL;
    SMB_DEVICE                              smb_device;

    fbe_base_object_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n",
                            __FUNCTION__);

     fbe_payload_control_get_buffer(control_operation, &pWriteResumeCmd);
     if (pWriteResumeCmd == NULL)
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, fbe_payload_control_get_buffer fail\n",
                               __FUNCTION__);
         return status;
     }

     fbe_payload_control_get_buffer_length(control_operation, &len);
     if (len != sizeof(fbe_resume_prom_cmd_t))
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, %d len invalid\n",
                               __FUNCTION__,len);
         return status;
     }

     /* Get the SMB device by passing in the sp, slot and device_type */
     status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                        pWriteResumeCmd->deviceType,
                                                        &pWriteResumeCmd->deviceLocation,
                                                        &smb_device);

     pSpeclResumeData = fbe_memory_ex_allocate(sizeof(SPECL_RESUME_DATA));
     if(pSpeclResumeData == NULL)
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can not allocate Resume Data.\n",
                               __FUNCTION__);
         return status;
     }

     fbe_zero_memory(pSpeclResumeData, sizeof(SPECL_RESUME_DATA));

     fbe_base_board_get_resume(smb_device, pSpeclResumeData);

     if(pSpeclResumeData->lastWriteStatus == STATUS_SPECL_TRANSACTION_IN_PROGRESS
             || pSpeclResumeData->lastWriteStatus == STATUS_SPECL_TRANSACTION_PENDING)
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, resume prom write in progess.\n",
                               __FUNCTION__);
         status = FBE_STATUS_PENDING;
     }
     else if(pSpeclResumeData->lastWriteStatus == EMCPAL_STATUS_SUCCESS)
     {
         status = FBE_STATUS_OK;
     }
     else
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Failed to get resume prom, device type: %lld, bus: %d, encl: %d, status %d.\n",
                               pWriteResumeCmd->deviceType,
                               pWriteResumeCmd->deviceLocation.bus,
                               pWriteResumeCmd->deviceLocation.enclosure,
                               pSpeclResumeData->lastWriteStatus);
         status = FBE_STATUS_GENERIC_FAILURE;
     }

     fbe_memory_ex_release(pSpeclResumeData);

     return status;
}

/*!*************************************************************************
* @fn fbe_base_board_check_resume_prom_last_write_status_async
***************************************************************************
*
* @brief
*       This function checks if the last resume prom write successed.
*
* @param      base_board - Object context.
* @param      control_operation - control op requested
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*    28-Aug-2012: Dipak Patel - Created
*
***************************************************************************/
fbe_status_t fbe_base_board_check_resume_prom_last_write_status_async(fbe_base_board_t *base_board,
                                                                      fbe_payload_control_operation_t *control_operation)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_resume_prom_write_asynch_cmd_t      *rpWriteAsynchCmd = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    SPECL_RESUME_DATA                       *pSpeclResumeData = NULL;
    SMB_DEVICE                              smb_device;

        fbe_base_object_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n",
                            __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation, &rpWriteAsynchCmd);
    if (rpWriteAsynchCmd == NULL)
    {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, fbe_payload_control_get_buffer fail\n",
                               __FUNCTION__);
         return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_write_asynch_cmd_t))
    {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, %d len invalid\n",
                               __FUNCTION__,len);
         return status;
    }

    /* Get the SMB device by passing in the sp, slot and device_type */
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       rpWriteAsynchCmd->deviceType,
                                                       &rpWriteAsynchCmd->deviceLocation,  
                                                       &smb_device);

     pSpeclResumeData = fbe_memory_ex_allocate(sizeof(SPECL_RESUME_DATA));
     if(pSpeclResumeData == NULL)
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can not allocate Resume Data.\n",
                               __FUNCTION__);
         return status;
     }

     fbe_zero_memory(pSpeclResumeData, sizeof(SPECL_RESUME_DATA));

     fbe_base_board_get_resume(smb_device, pSpeclResumeData);

     if(pSpeclResumeData->lastWriteStatus == STATUS_SPECL_TRANSACTION_IN_PROGRESS
             || pSpeclResumeData->lastWriteStatus == STATUS_SPECL_TRANSACTION_PENDING)
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, resume prom write in progess.\n",
                               __FUNCTION__);
         status = FBE_STATUS_PENDING;
     }
     else if(pSpeclResumeData->lastWriteStatus == EMCPAL_STATUS_SUCCESS)
     {
         status = FBE_STATUS_OK;
     }
     else
     {
         fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Failed to get resume prom, device type: %lld, bus: %d, encl: %d, status %d.\n",
                               rpWriteAsynchCmd->deviceType,
                               rpWriteAsynchCmd->deviceLocation.bus,
                               rpWriteAsynchCmd->deviceLocation.enclosure,
                               pSpeclResumeData->lastWriteStatus);
         status = FBE_STATUS_GENERIC_FAILURE;
     }

     fbe_memory_ex_release(pSpeclResumeData);

     return status;
}
