/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_calypso_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for sas calypso enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *  07-Apr-2014     Joe Perry - Created. 
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_calypso_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"


static fbe_status_t 
sas_calypso_enclosure_getEnclosurePrvtInfo(fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
                                     fbe_packet_t *packetPtr);
static fbe_status_t 
sas_calypso_enclosure_getFanCount(fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
                                  fbe_packet_t *packet);

static fbe_status_t 
sas_calypso_enclosure_getFanInfo(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet);

/*!**************************************************************
 * @fn fbe_sas_calypso_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the calypso enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *  07-Apr-2014     Joe Perry - Created. 
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_calypso_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_calypso_enclosure_t * sas_calypso_enclosure = NULL;
     fbe_payload_control_operation_opcode_t control_code;

    sas_calypso_enclosure = (fbe_sas_calypso_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_calypso_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_calypso_enclosure),
                            "%s entry .\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_customizable_trace((fbe_base_object_t *)sas_calypso_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_calypso_enclosure),
                                       "%s: cntrlCode %s\n", 
                                       __FUNCTION__, 
                                       fbe_base_enclosure_getControlCodeString(control_code));
    switch(control_code) {

		case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
            status = sas_calypso_enclosure_getEnclosurePrvtInfo(sas_calypso_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO:
            status = sas_calypso_enclosure_getFanInfo((fbe_eses_enclosure_t *)sas_calypso_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT:
            status = sas_calypso_enclosure_getFanCount(sas_calypso_enclosure, packet);
            break;

        default:
            status = fbe_eses_enclosure_control_entry(object_handle, packet);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *)sas_calypso_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_calypso_enclosure),
                                      "%s: failed to process cntrlCode 0x%x, status %d\n",
                                        __FUNCTION__, 
                                        control_code,
                                        status);
            }
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn sas_calypso_enclosure_getEnclosurePrvtInfo(
 *                         fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function copy fbe_sas_calypso_enclosure_t structure
 *  into buffer.
 *
 * @param calypsoEnclosurePtr - pointer to the fbe_sas_calypso_enclosure_t.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *  07-Apr-2014     Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_status_t 
sas_calypso_enclosure_getEnclosurePrvtInfo(fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t        bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_prv_info_t   *bufferPtr;
    fbe_enclosure_number_t      enclosure_number = 0;   
    fbe_status_t status;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)calypsoEnclosurePtr, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t *)calypsoEnclosurePtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)calypsoEnclosurePtr),
                          "calypsoEncl:%s entry\n", __FUNCTION__);    

    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_prv_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)calypsoEnclosurePtr, TRUE, (fbe_payload_control_buffer_t *)&bufferPtr, bufferLength);
    if(status == FBE_STATUS_OK)
    {
	    /* Copy fbe_sas_calypso_enclosure_t structute into buffer */
		memcpy(bufferPtr, calypsoEnclosurePtr, sizeof(fbe_sas_calypso_enclosure_t));
	}
     
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}   // end of sas_calypso_enclosure_getEnclosurePrvtInfo


/*!*************************************************************************
* @fn sas_calypso_enclosure_getFanInfo()                  
***************************************************************************
* @brief
*       This function gets the fan info for Calypso enclosures.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   03-Nov-2014     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
sas_calypso_enclosure_getFanInfo(fbe_eses_enclosure_t *eses_enclosure, 
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
    fbe_u8_t containerIndex = 0; 
    fbe_u8_t sideId = 0;
    fbe_bool_t inserted = FALSE; 
    fbe_bool_t faulted = FALSE; 
    fbe_u8_t subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_edal_fw_info_t edalFwInfo = {0};
    fbe_u8_t subenclProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE] = {0};
    fbe_u32_t   realFanSlotIndex = 0;

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
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetFanInfo);
    if( (pGetFanInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
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
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen %llu, status 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_cooling_info_t),
			       status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* We only return the standalone fan.
     */ 
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get containerIndex */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get sideId. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SIDE_ID, // Attribute.
                                                               FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                               coolingIndex, // Component index.
                                                               &sideId); // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if((subtype == targetSubtype) &&
           (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
           (sideId != FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
        {
            if (realFanSlotIndex == pGetFanInfo->slotNumOnEncl)
            {
                // Inserted.
                enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
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
                enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
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

                // Side/AssociateSp & Slot
                pGetFanInfo->associatedSp= sideId;
                pGetFanInfo->slotNumOnSpBlade = realFanSlotIndex;

                // Firmware Rev.
                enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
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

                // Subencl product id.
                enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_SUBENCL_PRODUCT_ID,
                                                FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                                coolingIndex, // Component index.
                                                FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                                &subenclProductId[0]);

                if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                else
                {
                    fbe_zero_memory(&pGetFanInfo->subenclProductId[0],
                                    FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);

                    fbe_copy_memory(&pGetFanInfo->subenclProductId[0],
                                     &subenclProductId[0],
                                     FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
                }

                break;
            }

            realFanSlotIndex++;
        } // End of subtype match
    }// End of - for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)

    fbe_eses_enclosure_convert_subencl_product_id_to_unique_id(eses_enclosure,
                                                               &pGetFanInfo->subenclProductId[0], 
                                                               &pGetFanInfo->uniqueId);

    pGetFanInfo->inProcessorEncl = TRUE;
    pGetFanInfo->bDownloadable = FALSE;
    pGetFanInfo->fanDegraded = FBE_MGMT_STATUS_FALSE;
    pGetFanInfo->multiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->persistentMultiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;
    pGetFanInfo->slotNumOnEncl = sideId;
    pGetFanInfo->hasResumeProm = FALSE;
    pGetFanInfo->fanLocation = FBE_FAN_LOCATION_BACK;

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - sas_calypso_enclosure_getFanInfo

/*!**************************************************************
 * @fn sas_calypso_enclosure_getFanCount(
 *                         fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function returns the Fan count for Calypso enclosure.
 *
 * @param calypsoEnclosurePtr - pointer to the fbe_sas_calypso_enclosure_t.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *  30-Sep-2014     Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_status_t 
sas_calypso_enclosure_getFanCount(fbe_sas_calypso_enclosure_t *calypsoEnclosurePtr, 
                                  fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t    coolingComponentCount;
    fbe_u32_t   coolingIndex = 0;
    fbe_u8_t    containerIndex = 0;
    fbe_u8_t    sideId = 0;
    fbe_u8_t    subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u32_t * pFanCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)calypsoEnclosurePtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)calypsoEnclosurePtr),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)calypsoEnclosurePtr,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)calypsoEnclosurePtr),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)calypsoEnclosurePtr,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)calypsoEnclosurePtr),
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
        fbe_base_object_customizable_trace((fbe_base_object_t*)calypsoEnclosurePtr,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)calypsoEnclosurePtr),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // Calypso enclosure has LCC Internal fans
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)calypsoEnclosurePtr,
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
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t*)calypsoEnclosurePtr,
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
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t*)calypsoEnclosurePtr,
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

        /* Get sideId */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t*)calypsoEnclosurePtr,
                                                               FBE_ENCL_COMP_SIDE_ID,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &sideId);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if((subtype == targetSubtype) &&
           (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
           (sideId != FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
        {
            (*pFanCount)++;
        }
    }

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}   // end of sas_calypso_enclosure_getFanCount

//End of file fbe_sas_calypso_enclosure_usurper.c
