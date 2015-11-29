/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_naga_iosxp_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for sas naga enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_naga_iosxp_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"


static fbe_status_t 
sas_naga_iosxp_enclosure_getEnclosurePrvtInfo(fbe_sas_naga_iosxp_enclosure_t *nagaEnclosurePtr, 
                                                fbe_packet_t *packet);

static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_fan_count(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                             fbe_packet_t *packet);

static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_fan_info(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet);

static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_ps_count(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet);

static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_ps_info(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet);

/*!**************************************************************
 * @fn fbe_sas_naga_iosxp_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the holster enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_naga_iosxp_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t                 status;
    fbe_sas_naga_iosxp_enclosure_t * pSasNagaEnclosure = NULL;
    fbe_payload_control_operation_opcode_t control_code;

    pSasNagaEnclosure = (fbe_sas_naga_iosxp_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pSasNagaEnclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaEnclosure),
                            "sas_naga_encl_ctrl_entry.\n");

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_customizable_trace((fbe_base_object_t *)pSasNagaEnclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaEnclosure),
                                       "sas_naga_encl_ctrl_entry:cntrlCode(0x%x) %s\n", 
                                       control_code,
                                       fbe_base_enclosure_getControlCodeString(control_code));
    switch(control_code) {

    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
        status = sas_naga_iosxp_enclosure_getEnclosurePrvtInfo(pSasNagaEnclosure, packet);
        break;

    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT:
        status = fbe_sas_naga_iosxp_enclosure_get_fan_count(pSasNagaEnclosure, packet);
        break;

    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO:
        status = fbe_sas_naga_iosxp_enclosure_get_fan_info(pSasNagaEnclosure, packet);
        break;
#if 0
    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_COUNT:
        status = fbe_sas_naga_iosxp_enclosure_get_ps_count(pSasNagaEnclosure, packet);
        break;

    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO:
        status = fbe_sas_naga_iosxp_enclosure_get_ps_info(pSasNagaEnclosure, packet);
        break;
#endif
    default:
        status = fbe_eses_enclosure_control_entry(object_handle, packet);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *)pSasNagaEnclosure,
                                  FBE_TRACE_LEVEL_WARNING,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaEnclosure),
                                  "sas_naga_iosxp_ctrl_entry, unsupported ctrlCode 0x%X, status 0x%X.\n", 
                                  control_code,
                                  status);
        }
        break;
    }

    return status;
}

/*!**************************************************************
 * @fn sas_naga_iosxp_enclosure_getEnclosurePrvtInfo(
 *                         fbe_sas_naga_iosxp_enclosure_t *nagaEnclosurePtr, 
 *                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function copy fbe_sas_naga_iosxp_enclosure_t structure
 *  into buffer.
 *
 * @param nagaEnclosurePtr - pointer to the fbe_sas_naga_iosxp_enclosure_t.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ****************************************************************/
static fbe_status_t 
sas_naga_iosxp_enclosure_getEnclosurePrvtInfo(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                          fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t             bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_prv_info_t   *pBuffer;
    fbe_enclosure_number_t                          enclosure_number = 0;   
    fbe_status_t                                    status;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)pNagaEncl, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t *)pNagaEncl,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                          "VoyagerICMEncl:%s entry\n", __FUNCTION__);    

    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_prv_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)pNagaEncl, TRUE, (fbe_payload_control_buffer_t *)&pBuffer, bufferLength);
    if(status == FBE_STATUS_OK)
    {
            /* Copy fbe_sas_naga_iosxp_enclosure_t structute into buffer */
                memcpy(pBuffer, pNagaEncl, sizeof(fbe_sas_naga_iosxp_enclosure_t));
        }
     
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return (status);

}   // end of sas_naga_iosxp_enclosure_getEnclosurePrvtInfo

/*!*************************************************************************
* @fn fbe_sas_naga_iosxp_enclosure_get_fan_count()                  
***************************************************************************
* @brief
*       This function gets the total fan count in the enclosure.
*
* @param     pNagaEnclosure - The pointer to the fbe_sas_naga_iosxp_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   06-Dec-2013     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_fan_count(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                             fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t    coolingComponentCount;
    fbe_u32_t   coolingIndex = 0;
    fbe_u8_t    containerIndex = 0;
    fbe_u8_t    subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u32_t * pFanCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    // Get the pointer to the fan count, that should be filled in.
    pFanCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    
    /* We only return the standalone fan count. 
     */ 
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_CHASSIS;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)pNagaEncl,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    /* Caluclate the Fan count by the number of the containing subenclosures.
     * (Do not include the chassis)
     */
    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // Get the attribute which indicates if it is individual or overall element.
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if((subtype == targetSubtype) &&
           (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            (*pFanCount)++;
        }
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_sas_naga_iosxp_enclosure_get_fan_count

/*!*************************************************************************
* @fn fbe_sas_naga_iosxp_enclosure_get_fan_info()                  
***************************************************************************
* @brief
*       This function gets the fan info.
*
* @param     pNagaEncl - The pointer to the fbe_sas_naga_iosxp_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   05-Dec-2013     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_fan_info(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_cooling_info_t * pGetFanInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t coolingComponentCount = 0;
    fbe_u32_t coolingIndex = 0;
    fbe_bool_t inserted = FALSE; 
    fbe_bool_t faulted = FALSE; 
    fbe_edal_fw_info_t edalFwInfo = {0};

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetFanInfo);
    if( (pGetFanInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                               "%s, get_buffer failed, status 0x%x, pGetFanInfo %p.\n",
                               __FUNCTION__, status, pGetFanInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_cooling_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                               "%s, get_buffer_length, bufferLen %d, expectedLen %llu, status 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_cooling_info_t),
                               status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)pNagaEncl,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pGetFanInfo->slotNumOnEncl >= coolingComponentCount)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                               "%s, incorrect slot num %d, fanCount %d.\n", 
                               __FUNCTION__, 
                               pGetFanInfo->slotNumOnEncl, 
                               coolingComponentCount);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // The first one in the edal is the overall element for all 10 fans.
    coolingIndex = pGetFanInfo->slotNumOnEncl + 1;
    // Inserted.
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                    FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                    FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                    coolingIndex, // Component index.
                                                    &inserted);  // The value to be returned.

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    pGetFanInfo->inserted = inserted ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;

    // Faulted.
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                    FBE_ENCL_COMP_FAULTED, // Attribute.
                                                    FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                                    coolingIndex, // Component index.
                                                    &faulted); // The value to be returned.

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    pGetFanInfo->fanFaulted = faulted ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;

    // Firmware Rev.
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                        FBE_ENCL_COMP_FW_INFO,// Component Attribute
                        FBE_ENCL_COOLING_COMPONENT,  // Component type.
                        coolingIndex, // Component index.
                        sizeof(fbe_edal_fw_info_t), 
                        (char *)&edalFwInfo);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetFanInfo->firmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetFanInfo->firmwareRev[0], 
                        &edalFwInfo.fwRevision[0], 
                        FBE_ESES_FW_REVISION_SIZE);
    }

    // Naga fans are not subEnclosures, so zero out the Subencl product id field.
    fbe_zero_memory(&pGetFanInfo->subenclProductId[0],
                    FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);
    
    /* The enclosure object does not report the processor enclosure FANs.
     So set inProcessEncl to FALSE */
    pGetFanInfo->inProcessorEncl = FALSE;
    pGetFanInfo->bDownloadable = FALSE;
    pGetFanInfo->fanDegraded = FBE_MGMT_STATUS_FALSE;
    pGetFanInfo->multiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->persistentMultiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;
    /* Currently only voyager has standalone fans, and they have no associatedSp. */
    pGetFanInfo->associatedSp= SP_INVALID;
    pGetFanInfo->slotNumOnSpBlade = FBE_INVALID_SLOT_NUM;
    pGetFanInfo->hasResumeProm = FALSE;

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_sas_naga_iosxp_enclosure_get_fan_info


/*!*************************************************************************
* @fn base_enclosure_get_ps_count()                  
***************************************************************************
* @brief
*   This functions gets the total ps count.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   30-Dec-2013     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_ps_count(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;   
    fbe_payload_control_buffer_t        control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t                           *pPsCount = NULL;
    fbe_u32_t                           psIndex;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            containerIndex = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    fbe_u8_t                            number_of_components = 0;


    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pPsCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                                "%s, fbe_payload_control_get_buffer_length failed.\n", 
                                __FUNCTION__);

        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)pNagaEncl),
                                "get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)pNagaEncl,
                                                          FBE_ENCL_POWER_SUPPLY,
                                                          &number_of_components); 
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pPsCount = 0;

    // loop to count power supplies, only count the containers.
    for (psIndex = 0; psIndex < number_of_components; psIndex++)
    {
        // get the container index
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &containerIndex);               // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }


        if(containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED)
        {
            (*pPsCount)++;
        }
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!*************************************************************************
* @fn fbe_sas_naga_iosxp_enclosure_get_ps_info()                  
***************************************************************************
* @brief
*       This function gets the PS info.
*
* @param     pNagaEncl - The pointer to the fbe_sas_naga_iosxp_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   30-Dec-2013     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_sas_naga_iosxp_enclosure_get_ps_info(fbe_sas_naga_iosxp_enclosure_t *pNagaEncl, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_power_supply_info_t *getPsInfo = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              inserted, faulted, acFail;
    fbe_u8_t                subenclProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE] = {0};
    fbe_u8_t                number_of_components = 0;
    fbe_u32_t               psIndex;
    fbe_u32_t               psAcFailIndex = 0;
    fbe_edal_fw_info_t      edal_fw_info;
    fbe_u8_t                sideId = 0;
    fbe_u8_t                containerIndex = 0;
    fbe_u32_t               slotNum = 0;
    fbe_u8_t                psMarginTestMode, psMarginTestResults;
    fbe_u8_t                sp_id = 0;
    fbe_u8_t                psSide = 0;
    fbe_u8_t                psCountPerSide = 0;
    fbe_u8_t                revIndex = 0;
    fbe_u8_t                psSlotNumBasedOnSide = 0;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                            "%s, get_payload failed.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                               "%s, status: 0x%x, control_buffer: %p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    getPsInfo = (fbe_power_supply_info_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_power_supply_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                                "%s, fbe_payload_control_get_buffer_length failed.\n", 
                                __FUNCTION__);

        fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                                "Control_buffer_length %d, expected_length: %llu, status: 0x%x.\n", 
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_power_supply_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    slotNum = getPsInfo->slotNumOnEncl;

    /*
     * Get enclosure Power Supply info
     */
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)pNagaEncl,
                                                          FBE_ENCL_POWER_SUPPLY,
                                                          &number_of_components); 
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    // copy the version desc rev (cdes1 or cdes2)
    getPsInfo->esesVersionDesc = pNagaEncl->eses_enclosure.eses_version_desc;

    /* Expander would not return the process enclosure power supply status.
     * So we should return FALSE here always.
     */
    getPsInfo->inProcessorEncl = FALSE;

    // Loop through the number of ps components and set the data.
    // Use the slot from the location (input) to determine if the data
    // belongs to this ps. The ps items with side 0 will go with slot 0.
    // The ps items with side 1 will go with slot 1.
    for(psIndex=0; psIndex<number_of_components; psIndex++)
    {
        // get the side id
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &sideId);               // The value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // get the container index
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &containerIndex);               // The value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // if this is the correct sub element
        if((sideId == slotNum) && (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            // get insert status
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                                      FBE_ENCL_COMP_INSERTED,   // Attribute.
                                                                      FBE_ENCL_POWER_SUPPLY,    // Component type.
                                                                      psIndex,                  // Component index.
                                                                      &inserted);               // The value to be returned.

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->bInserted |= inserted;
            }
            
            // get fault bit
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                            FBE_ENCL_COMP_FAULTED,  // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &faulted);              // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->generalFault |= faulted;
            }

            // get ac fail bit
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                            FBE_ENCL_PS_AC_FAIL,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &acFail);               // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->ACFail |= acFail;
                getPsInfo->ACFailDetails[psAcFailIndex] = acFail;
                psAcFailIndex++;

            }

            // get PS Margining info
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                            FBE_ENCL_PS_MARGIN_TEST_MODE,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &psMarginTestMode);     // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->psMarginTestMode = psMarginTestMode;
            }
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                                            FBE_ENCL_PS_MARGIN_TEST_RESULTS,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &psMarginTestResults);  // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->psMarginTestResults = psMarginTestResults;
            }

            // Get detailed FW Rev
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                            FBE_ENCL_COMP_FW_INFO,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            sizeof(fbe_edal_fw_info_t), 
                                            (char *)&edal_fw_info);     // copy the string here

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->psRevInfo[revIndex].firmwareRevValid = edal_fw_info.valid;
                getPsInfo->psRevInfo[revIndex].bDownloadable = edal_fw_info.downloadable;
                fbe_zero_memory(&getPsInfo->psRevInfo[revIndex].firmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

                fbe_copy_memory(&getPsInfo->psRevInfo[revIndex].firmwareRev[0],
                                &edal_fw_info.fwRevision,
                                FBE_ESES_FW_REVISION_SIZE);

                revIndex++;
            }
        }

        if((sideId == slotNum) && (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            // Get the PS firmware rev (the lowest rev of the MCUs)
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                            FBE_ENCL_COMP_FW_INFO,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            sizeof(fbe_edal_fw_info_t), 
                                            (char *)&edal_fw_info);     // copy the string here

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->firmwareRevValid = edal_fw_info.valid;

                fbe_zero_memory(&getPsInfo->firmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

                fbe_copy_memory(&getPsInfo->firmwareRev[0],
                                &edal_fw_info.fwRevision,
                                FBE_ESES_FW_REVISION_SIZE);
            }

            // Get Subencl product id.
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)pNagaEncl,
                                            FBE_ENCL_SUBENCL_PRODUCT_ID,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                            &subenclProductId[0]);
        
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_zero_memory(&getPsInfo->subenclProductId[0],
                                FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);
        
                fbe_copy_memory(&getPsInfo->subenclProductId[0],
                                 &subenclProductId[0],
                                 FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
            }
        
            fbe_eses_enclosure_convert_subencl_product_id_to_unique_id((fbe_eses_enclosure_t *)pNagaEncl,
                                                                       &getPsInfo->subenclProductId[0], 
                                                                       &getPsInfo->uniqueId);
        }
    }

    // Check whether it is downloadable.
    if(fbe_eses_enclosure_is_ps_downloadable((fbe_eses_enclosure_t *)pNagaEncl, 
                                        slotNum,
                                        &getPsInfo->firmwareRev[0],
                                        getPsInfo->uniqueId))
    {
        getPsInfo->bDownloadable = TRUE;
    }
    else
    {
        getPsInfo->bDownloadable = FALSE;
    }

    /* Get PS AC/DC type */
    getPsInfo->ACDCInput = fbe_eses_enclosure_get_ps_ac_dc_type((fbe_eses_enclosure_t *)pNagaEncl, getPsInfo->uniqueId);

    // determine which SP & SPS the PS is associated with
    encl_status = fbe_eses_enclosure_get_local_lcc_side_id((fbe_eses_enclosure_t *)pNagaEncl, &sp_id);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    /* We could get the psSide(not psSlot) from ESES configuration page.
     * However we already use FBE_ENCL_COMP_SIDE_ID for psSlot.
     * My another concern is that CDES also uses side id 0, 1, 2, 3 for Voyager LCC 0, 1, 2 ,3.
     * So I am not sure whether one day CDES would use side id 0, 1, 2, 3 for power supplies as well. 
     * So for simplicity, I just use this approach to get the psSide (It is A side or B side, not slot).
     */
    if ((psCountPerSide = fbe_eses_enclosure_get_number_of_power_supplies_per_side((fbe_eses_enclosure_t *)pNagaEncl)) == 0)
    {
        psCountPerSide = 1;
    }

    /* When the Naga enclosure is connected as a boot enclosure,
     * the power supplies in slot 0(PSA1) and slot 1(PSA0) are connected to SPS A.
     * and the power supplies in slot 2(PSB1) and slot 3(PSB0) are connected to SPS B.
     */
    psSide = slotNum/psCountPerSide;

    if(psSide == SP_A)
    {
        getPsInfo->associatedSps = SP_A;
        getPsInfo->associatedSp = SP_A;
    }
    else
    {
        getPsInfo->associatedSps = SP_B;
        getPsInfo->associatedSp = SP_B;
    }

    /* The power supplies in slot 0(PSA1) and slot 2(PSB1) supplies the power to LCC A
     * The power supplies in slot 1(PSA0) and slot 3(PSB0) supplies the power to LCC B
     */
    psSlotNumBasedOnSide = slotNum%psCountPerSide;
    if(psSlotNumBasedOnSide == 0)
    {  
        getPsInfo->isLocalFru = (sp_id == SP_A) ? FBE_TRUE : FBE_FALSE;
        
    }
    else
    {
        getPsInfo->isLocalFru = (sp_id == SP_A) ? FBE_FALSE : FBE_TRUE;
    }
    
    getPsInfo->ambientOvertempFault = FBE_MGMT_STATUS_NA;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pNagaEncl,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaEncl),
                                        "%s, ps %d, insert %d, fault %d, acFail %d\n", 
                                        __FUNCTION__,
                                        getPsInfo->slotNumOnEncl,
                                        getPsInfo->bInserted,
                                        getPsInfo->generalFault,
                                        getPsInfo->ACFail);

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, encl_status);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_sas_naga_iosxp_enclosure_get_ps_info


//End of file fbe_sas_naga_iosxp_enclosure_usurper.c
