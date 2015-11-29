/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_base_enclosure_data_get.c
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to retrieve data from the Base
 *      Enclosure data.
 *
 *  @ingroup fbe_edal
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      16-Sep-08   Joe Perry - Created
 *
 *
 **************************************************************************/

#include "edal_base_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"

void
edal_init_encl_component_data(fbe_enclosure_component_t *EsesEnclComponentDataPtr);


/*!*************************************************************************
 *                         @fn fbe_base_enclosure_access_getSpecificComponent
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure object's Component Type Data
 *      address.
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param componentBlockPtr - pointer to a component block.
 *      @param componentType - Component Type enumeration
 *      @param index - the component index.
 *      @param encl_component - pointer to the specified Component Data
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_base_enclosure_access_getSpecificComponent(fbe_enclosure_component_block_t *componentBlockPtr,
                                               fbe_enclosure_component_types_t componentType,
                                               fbe_u32_t index,
                                               void **encl_component)
{
    fbe_edal_status_t status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_t   *componentTypeDataPtr = NULL;

    /* we assume all the caller does the validation when calling this function
     * otherwise, we need to call 
     * fbe_base_enclosure_access_getSpecificComponent_with_sanity_check()
     */

    if (enclosure_access_getSpecifiedComponentTypeDataWithIndex(componentBlockPtr,
                                                       &componentTypeDataPtr,
                                                       componentType,
                                                       index)
        == FBE_EDAL_STATUS_OK)
    {
        if (enclosure_access_getSpecifiedComponentData(componentTypeDataPtr, 
                                                       index,
                                                       encl_component)
            == FBE_EDAL_STATUS_OK)
        {
            status = FBE_EDAL_STATUS_OK;
        }
    }
    else
    {
        status = FBE_EDAL_STATUS_COMPONENT_NOT_FOUND;
        *encl_component = NULL;
    }

    return status;

}   // end of fbe_base_enclosure_access_getSpecificComponent


/*!*************************************************************************
 *                         enclosure_access_getEnclosureComponentTypeData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure object's Component Type Data
 *      address.
 *
 *  PARAMETERS:
 *      @param baseCompBlockPtr - pointer to a component block.
 *      @param enclosureComponentTypeDataPtr - pointer to the Component Type Data
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
enclosure_access_getEnclosureComponentTypeData
    (fbe_enclosure_component_block_t *baseCompBlockPtr, 
     fbe_enclosure_component_t **enclosureComponentTypeDataPtr)
{

    // validate pointer & buffer
    if (baseCompBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (baseCompBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, baseCompBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    *enclosureComponentTypeDataPtr = edal_calculateComponentTypeDataStart(baseCompBlockPtr);
    return FBE_EDAL_STATUS_OK;

}   // end of enclosure_access_getEnclosureComponentTypeData

/*!*************************************************************************
 *                         enclosure_access_getSpecifiedComponentTypeDataWithIndex
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a specific component type from the enclosure 
 *      object Data.
 *      This function goes through the chain of the blocks
 *
 *  PARAMETERS:
 *      @param baseCompBlockPtr - pointer to the base EDAL block.
 *      @param enclosureComponentTypeDataPtr - pointer to the pointer to the specified Component Type Data
 *      @param componentType - specific component to get (PS, Drive, ...)
 *      @param componentIndex - the component index among all the blocks.
 *   
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *    16-Mar-2010: PHE - Modified for the component type which is allocated in multiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
enclosure_access_getSpecifiedComponentTypeDataWithIndex(fbe_enclosure_component_block_t *baseCompBlockPtr, 
                                            fbe_enclosure_component_t       **componentTypeDataPtr,
                                            fbe_enclosure_component_types_t componentType,
                                            fbe_u32_t componentIndex)
{
    fbe_enclosure_component_block_t  *localCompBlockPtr = NULL;
    fbe_enclosure_component_t        *tempComponentTypeDataPtr;
    fbe_edal_status_t                 edal_status;

    *componentTypeDataPtr = NULL;

    // validate pointer & buffer
    if (baseCompBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }

    localCompBlockPtr = baseCompBlockPtr;
    // search all Component Data Blocks
    while (localCompBlockPtr != NULL)
    {
        edal_status = 
            enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(localCompBlockPtr,
                                                                        &tempComponentTypeDataPtr,
                                                                        componentType);

        if(edal_status == FBE_EDAL_STATUS_OK)
        {
            if(tempComponentTypeDataPtr->maxNumberOfComponents + 
                    tempComponentTypeDataPtr->firstComponentIndex > componentIndex)
            {   
                // component type found
                *componentTypeDataPtr = tempComponentTypeDataPtr;
                return FBE_EDAL_STATUS_OK;
            }
        }
        else if(edal_status != FBE_EDAL_STATUS_COMPONENT_NOT_FOUND)
        {
            // error encountered.
			//enclosure_access_log_error("EDAL:%s, Bad status %d \n", __FUNCTION__, edal_status);
			//return edal_status;
            break;
        }

        // check if there's a next block
        localCompBlockPtr = localCompBlockPtr->pNextBlock;
    }// end of while loop

    return FBE_EDAL_STATUS_COMPONENT_NOT_FOUND;
}  // end of enclosure_access_getSpecifiedComponentTypeDataWithIndex


/*!*************************************************************************
 *                         enclosure_access_getSpecifiedComponentTypeDataInLocalBlock
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a specific component type from the specified EDAL block.
 *      This function does NOT go through the chain of the blocks.
 *
 *  PARAMETERS:
 *      @param localCompBlockPtr - pointer to the local EDAL block.
 *      @param componentTypeDataPtr - pointer to the pointer to specified Component Type Data
 *      @param componentType - specific component to get (PS, Drive, ...)
 *   
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    12-Mar-2010: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t
enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(fbe_enclosure_component_block_t *localCompBlockPtr, 
                                                        fbe_enclosure_component_t       **componentTypeDataPtr,
                                                        fbe_enclosure_component_types_t componentType)
{
    fbe_enclosure_component_t   *tempComponentTypeDataPtr = NULL;
    fbe_u32_t localIndex = 0;

    *componentTypeDataPtr = NULL;

    // validate pointer & buffer
    if (localCompBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
   
    if (localCompBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, localCompBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    tempComponentTypeDataPtr = edal_calculateComponentTypeDataStart(localCompBlockPtr);

    // search through all components & find the specified one
    for (localIndex = 0; localIndex < localCompBlockPtr->numberComponentTypes; localIndex++)
    {
        if (tempComponentTypeDataPtr->componentType == componentType)
        {   
            // component type found
            *componentTypeDataPtr = tempComponentTypeDataPtr;
            return FBE_EDAL_STATUS_OK;
        }
        tempComponentTypeDataPtr++;
    }

    // component type not found, return appropriate status
    return FBE_EDAL_STATUS_COMPONENT_NOT_FOUND;

}   // end of enclosure_access_getSpecifiedComponentTypeDataInLocalBlock


/*!*************************************************************************
 *                         enclosure_access_getSpecifiedComponentData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a specific component data from the enclosure 
 *      object.
 *
 *  PARAMETERS:
 *      @param enclosureComponentTypeDataPtr - pointer to a component descriptor 
 *      @param comoponentIndex - the component index among all the EDAL blocks.
 *      @param componentDataPtr - pointer to the specified Component Data
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
enclosure_access_getSpecifiedComponentData(fbe_enclosure_component_t *enclosureComponentTypeDataPtr,
                                           fbe_u32_t comoponentIndex,
                                           void **componentDataPtr)
{
    fbe_u64_t                  address1;

    // validate pointer
    if (enclosureComponentTypeDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Type Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_TYPE_DATA_PTR;
    }

    // index out of range
    if (comoponentIndex >= enclosureComponentTypeDataPtr->firstComponentIndex + 
                           enclosureComponentTypeDataPtr->maxNumberOfComponents)
    {
        enclosure_access_log_error("EDAL:%s, Component index %d too large.\n", 
            __FUNCTION__, comoponentIndex);
        *componentDataPtr = NULL;
        return FBE_EDAL_STATUS_COMP_TYPE_INDEX_INVALID;
    }

    // calculate address of specific component
    address1 = (fbe_u64_t)(fbe_ptrhld_t)enclosureComponentTypeDataPtr;
    address1 += enclosureComponentTypeDataPtr->firstComponentStartOffset;   // go to start of component data
    address1 += (enclosureComponentTypeDataPtr->componentSize * 
        (comoponentIndex - enclosureComponentTypeDataPtr->firstComponentIndex));     // go to specified index

    *componentDataPtr = (void *)(fbe_ptrhld_t)address1;

    return FBE_EDAL_STATUS_OK;

}   // end of enclosure_access_getSpecifiedComponentData


/*!*************************************************************************
 *                         enclosure_access_getComponentStateChangeCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the StateChangeCount from a specific 
 *      component of the enclosure object.
 *
 *  PARAMETERS:
 *      @param enclosureComponentTypeDataPtr - pointer to a component type
 *      @param stateChangeCountPtr - pointer to the StateChangeCount
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
enclosure_access_getComponentStateChangeCount(fbe_enclosure_component_t   *enclosureComponentTypeDataPtr,
                                              fbe_u32_t *stateChangeCountPtr)
{

    // validate pointer
    if (enclosureComponentTypeDataPtr == NULL)
    {
        // component type not found for this enclosure, set change count to zero
        *stateChangeCountPtr = 0;
        enclosure_access_log_error("EDAL:%s, Null Component Type Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_TYPE_DATA_PTR;
    }
    else 
    {
        *stateChangeCountPtr = enclosureComponentTypeDataPtr->componentOverallStateChangeCount;
    }
    return FBE_EDAL_STATUS_OK;

}   // end of enclosure_access_getComponentStateChangeCount


/*!*************************************************************************
 *                         @fn base_enclosure_access_getBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data.
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t *returnDataPtr)
{

    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t                         status;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

     if(status != FBE_EDAL_STATUS_OK)
     {
            return(status);
     }

    // validate buffer
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    /*
     * Access the appropriate attribute fromthe General Component Data
     */
    switch (attribute)
    {
    case FBE_ENCL_COMP_INSERTED:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.componentInserted;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.componentInsertedPriorConfig;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_FAULTED:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.componentFaulted;
        status = FBE_EDAL_STATUS_OK;
        break;
        case FBE_ENCL_COMP_POWERED_OFF:
        *returnDataPtr = 
                genComponentDataPtr->generalFlags.componentPoweredOff;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_FAULT_LED_ON:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.componentFaultLedOn;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_MARKED:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.componentMarked;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_TURN_ON_FAULT_LED:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.turnComponentFaultLedOn;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_MARK_COMPONENT:
        *returnDataPtr = 
            genComponentDataPtr->generalFlags.markComponent;
        status = FBE_EDAL_STATUS_OK;
        break;       
    case FBE_ENCL_COMP_STATE_CHANGE:
        *returnDataPtr =
            genStatusDataPtr->readWriteFlags.componentStateChange;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_WRITE_DATA:
        *returnDataPtr =
            genStatusDataPtr->readWriteFlags.writeDataUpdate;
        status = FBE_EDAL_STATUS_OK;
        break;        
    case FBE_ENCL_COMP_WRITE_DATA_SENT:
        *returnDataPtr =
            genStatusDataPtr->readWriteFlags.writeDataSent;
        status = FBE_EDAL_STATUS_OK;
        break;     
    case FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA:
        *returnDataPtr =
            genStatusDataPtr->readWriteFlags.emcEnclCtrlWriteDataUpdate;
        status = FBE_EDAL_STATUS_OK;
        break;        
    case FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT:
        *returnDataPtr =
            genStatusDataPtr->readWriteFlags.emcEnclCtrlWriteDataSent;
        status = FBE_EDAL_STATUS_OK;
        break;        
    case FBE_ENCL_COMP_STATUS_VALID:
            *returnDataPtr =
                genStatusDataPtr->readWriteFlags.componentStatusValid;
        status = FBE_EDAL_STATUS_OK;
        break;       
    default:
        /* Maybe it's not common attribute, let's check on 
         * individual component handler.
         */
        switch (componentType)
        {
        case FBE_ENCL_LCC:
            status = base_enclosure_access_getLccBool(generalDataPtr,
                                                        attribute,
                                                        returnDataPtr);
            break;
        
        case FBE_ENCL_POWER_SUPPLY:
            status = base_enclosure_access_getPowerSupplyBool(generalDataPtr,
                                                              attribute,
                                                              returnDataPtr);

            break;
        case FBE_ENCL_DRIVE_SLOT:
            status = base_enclosure_access_getDriveBool(generalDataPtr,
                                                        attribute,
                                                        returnDataPtr);
            break;
        case FBE_ENCL_COOLING_COMPONENT:
            status = base_enclosure_access_getCoolingBool(generalDataPtr,
                                                          attribute,
                                                          returnDataPtr);
            break;

        case FBE_ENCL_TEMP_SENSOR:
            status = base_enclosure_access_getTempSensorBool(generalDataPtr,
                                                             attribute,
                                                             returnDataPtr);
            break;
            
        default:
            enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printComponentType(componentType),
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
        }
        break;
    }

    return status;

}   // end of base_enclosure_access_getBool

/*!*************************************************************************
 *                         @fn base_enclosure_access_getU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data.
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u8_t *returnDataPtr)
{
    fbe_edal_status_t                   status;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */
    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    // validate buffer
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_COMP_ADDL_STATUS:
            *returnDataPtr = genStatusDataPtr->addlComponentStatus;
            status = FBE_EDAL_STATUS_OK;
            break;

        default:
        
            switch(componentType)
            {
                case FBE_ENCL_DRIVE_SLOT:
                    status = base_enclosure_access_getDriveU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;
                    
                case FBE_ENCL_ENCLOSURE:
                    status = base_enclosure_access_getEnclosureU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

                case FBE_ENCL_COOLING_COMPONENT:
                    status = base_enclosure_access_getCoolingU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

                case FBE_ENCL_TEMP_SENSOR:
                    status = base_enclosure_access_getTempSensorU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

                case FBE_ENCL_POWER_SUPPLY:
                    status = base_enclosure_access_getPowerSupplyU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

                case FBE_ENCL_CONNECTOR:
                    status = base_enclosure_access_getConnectorU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

                case FBE_ENCL_LCC:
                    status = base_enclosure_access_getLccU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
                break;

               case FBE_ENCL_EXPANDER:
               case FBE_ENCL_EXPANDER_PHY:
               case FBE_ENCL_SSC:
               default:
                   enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
                   __FUNCTION__, 
                   enclosure_access_printComponentType(componentType),
                   enclosure_access_printAttribute(attribute));
                status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
                break;
           }
        break;
       }
    return status;

}   // end of base_enclosure_access_getU8



/*!*************************************************************************
 *                         @fn base_enclosure_access_getU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object fields
 *      that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data.
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param returnDataPtr - pointer to the U16 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    12-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t componentType,
                             fbe_u16_t *returnDataPtr)
{
    fbe_edal_status_t                   status;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data and status data based on component type
     */
    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                             componentType,
                                                             &genComponentDataPtr,
                                                             &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return(status);
    }

    // validate buffer
    if (genStatusDataPtr->componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, comp %d, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            componentType,
            EDAL_COMPONENT_CANARY_VALUE, 
            genStatusDataPtr->componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    default:
        switch(componentType)
        {
        case FBE_ENCL_POWER_SUPPLY:
            status = base_enclosure_access_getPowerSupplyU16(generalDataPtr,
                                                             attribute,
                                                             returnDataPtr);
            break;

        case FBE_ENCL_TEMP_SENSOR:
            status = base_enclosure_access_getTempSensorU16(generalDataPtr,
                                                            attribute,
                                                            returnDataPtr);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
                                       __FUNCTION__, 
                                       enclosure_access_printComponentType(componentType),
                                       enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
        }
        break;
    }

    return status;

}   // end of base_enclosure_access_getU16


/*!*************************************************************************
 *                         @fn base_enclosure_access_getU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data.
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    31-Aug-2008: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u64_t *returnDataPtr)
{
    fbe_edal_status_t                        status;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */
    switch (componentType)
    {
    case FBE_ENCL_DRIVE_SLOT:
        status = base_enclosure_access_getDriveU64(generalDataPtr,
                                                attribute,
                                                returnDataPtr);
        break;
    case FBE_ENCL_ENCLOSURE:
        status = base_enclosure_access_getEnclosureU64(generalDataPtr,
                                                attribute,
                                                returnDataPtr);
        break;


    case FBE_ENCL_LCC:
        status = base_enclosure_access_getLccU64(generalDataPtr,
                                                attribute,
                                                returnDataPtr);
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(componentType),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_getU64



/*!*************************************************************************
 *                         @fn base_enclosure_access_getU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data.
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param returnDataPtr - pointer to the U32 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    31-Aug-2008: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t *returnDataPtr)
{
    fbe_edal_status_t                        status;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */
    switch (componentType)
    {
    case FBE_ENCL_ENCLOSURE:
        status = base_enclosure_access_getEnclosureU32(generalDataPtr,
                                                       attribute,
                                                       returnDataPtr);
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(componentType),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_getU32

/*!*************************************************************************
 *  @fn base_enclosure_access_getStr
 **************************************************************************
 *  @brief
 *      This function gets string data from base enclosure object.
 *
 *  @param generalDataPtr - pointer to component data.
 *  @param attribute - enum for attribute to get
 *  @param componentType - component type
 *  @param length - the length of the stringe
 *  @param returnDataPtr - pointer to the U64 return value
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t length,
                              char *returnDataPtr)
{
    fbe_edal_status_t                        status;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */
    switch (componentType)
    {
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(componentType),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_getStr


/*!*************************************************************************
 *                         @fn base_enclosure_access_getLccBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object LCC fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a power supply component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    25-Feb-2009: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t *returnDataPtr)
{
    fbe_base_lcc_info_t              *lccInfoPtr;
    fbe_bool_t                        returnValue = FALSE;
    fbe_edal_status_t                 status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    lccInfoPtr = (fbe_base_lcc_info_t *)generalDataPtr;
    if (lccInfoPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccInfoPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_IS_LOCAL:
        returnValue = lccInfoPtr->lccFlags.isLocalLcc;
        status = FBE_EDAL_STATUS_OK;
        break;  

    case FBE_LCC_FAULT_MASKED:
        returnValue = lccInfoPtr->lccFlags.lccFaultMasked;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_LCC),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    *returnDataPtr = returnValue;
    return status;

}   // end of base_enclosure_access_getLccBool

/*!*************************************************************************
 *                         @fn base_enclosure_access_getLccU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object LCC fields
 *      that are fbe_u64_t type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a power supply component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    15-Jul-2014: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getLccU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u64_t *returnDataPtr)
{
    fbe_base_lcc_info_t              *lccInfoPtr;
    fbe_u64_t                         returnValue = FALSE;
    fbe_edal_status_t                 status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    lccInfoPtr = (fbe_base_lcc_info_t *)generalDataPtr;
    if (lccInfoPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccInfoPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_LCC_FAULT_START_TIMESTAMP:
        returnValue = lccInfoPtr->lccData.lccFaultStartTimestamp;
        status = FBE_EDAL_STATUS_OK;
        break;  

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_LCC),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    *returnDataPtr = returnValue;
    return status;

}   // end of base_enclosure_access_getLccU64


/*!*************************************************************************
 *                         @fn base_enclosure_access_getPowerSupplyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a power supply component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getPowerSupplyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t *returnDataPtr)
{
    fbe_encl_power_supply_info_t     *psInfoPtr;
    fbe_bool_t                        returnValue = FALSE;
    fbe_edal_status_t                 status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psInfoPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
    if (psInfoPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psInfoPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_PS_AC_FAIL:
        returnValue = psInfoPtr->psFlags.acFail;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_PS_DC_DETECTED:
        returnValue = psInfoPtr->psFlags.dcDetected;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_PS_SUPPORTED:
        returnValue = psInfoPtr->psFlags.psSupported;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_POWER_SUPPLY),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    *returnDataPtr = returnValue;
    return status;

}   // end of base_enclosure_access_getPowerSupplyBool


/*!*************************************************************************
 *                         @fn base_enclosure_access_getCoolingBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Cooling Component
 *      fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a power supply component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    15-Sep-2008: hgd - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getCoolingBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t *returnDataPtr)
{
    fbe_encl_cooling_info_t        *coolingInfoPtr;
    fbe_bool_t                      returnValue = FALSE;
    fbe_edal_status_t               status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    coolingInfoPtr = (fbe_encl_cooling_info_t *)generalDataPtr;
    if (coolingInfoPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            coolingInfoPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COOLING_MULTI_FAN_FAULT:
        returnValue = coolingInfoPtr->coolingFlags.multiFanFault;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_COOLING_COMPONENT),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    *returnDataPtr = returnValue;
    return status;

}   // end of enclosure_access_getCoolingBool


/*!*************************************************************************
 *                         @fn base_enclosure_access_getTempSensorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Cooling Component
 *      fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a power supply component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    25-Sep-2008: hgd - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getTempSensorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t *returnDataPtr)
{
    fbe_encl_temp_sensor_info_t    *tempSensorInfoPtr;
    fbe_bool_t                      returnValue = FALSE;
    fbe_edal_status_t               status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    tempSensorInfoPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;
    if (tempSensorInfoPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            tempSensorInfoPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_TEMP_SENSOR_OT_WARNING:
        returnValue = tempSensorInfoPtr->tempSensorFlags.overTempWarning;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_TEMP_SENSOR_OT_FAILURE:
        returnValue = tempSensorInfoPtr->tempSensorFlags.overTempFailure;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_TEMP_SENSOR_TEMPERATURE_VALID:
        returnValue = tempSensorInfoPtr->tempSensorFlags.tempValid;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_TEMP_SENSOR),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    *returnDataPtr = returnValue;
    return status;

}   // end of  -  base_enclosure_access_getTempSensorBool()


/*!*************************************************************************
 *                         @fn base_enclosure_access_getDriveU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Drive fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{
    fbe_encl_drive_slot_info_t      *driveDataPtr;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    driveDataPtr = (fbe_encl_drive_slot_info_t *)generalDataPtr;
    if (driveDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            driveDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_TYPE:
        *returnDataPtr = driveDataPtr->enclDriveType;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_SLOT_NUMBER:
        *returnDataPtr = driveDataPtr->driveSlotNumber;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_DEVICE_OFF_REASON:
        *returnDataPtr = driveDataPtr->driveDeviceOffReason;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of enclosure_access_getDriveU8

/*!*************************************************************************
 *                         @fn base_enclosure_access_getEnclosureU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Enclosure fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{
    fbe_base_enclosure_info_t           *enclDataPtr;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_base_enclosure_info_t *)generalDataPtr;
    if(enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary !=
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {   
        case FBE_ENCL_POSITION:
            *returnDataPtr = enclDataPtr->enclPosition;
            status = FBE_EDAL_STATUS_OK;
            break;
        case FBE_ENCL_ADDRESS:
            *returnDataPtr = enclDataPtr->enclAddress;
            status = FBE_EDAL_STATUS_OK;
            break;
        case FBE_ENCL_PORT_NUMBER:
            *returnDataPtr = enclDataPtr->enclPortNumber;
            status = FBE_EDAL_STATUS_OK;
            break;
        case FBE_ENCL_COMP_SIDE_ID:
            *returnDataPtr = enclDataPtr->enclSideId;
            status = FBE_EDAL_STATUS_OK;
            break;
        case FBE_ENCL_MAX_SLOTS:
            *returnDataPtr = enclDataPtr->enclMaxSlot;
            status = FBE_EDAL_STATUS_OK;
            break;
        default:
            enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_ENCLOSURE),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
    return status;
}   // end of enclosure_access_getEnclosureU8
/*!*************************************************************************
 *                         @fn base_enclosure_access_getPowerSupplyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Power supply fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{

    fbe_encl_power_supply_info_t *psDataPtr = NULL;
    fbe_edal_status_t                   status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
    if (psDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = psDataPtr->psData.psSideId;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_PS_INPUT_POWER_STATUS:
        *returnDataPtr = psDataPtr->psData.inputPowerStatus;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_PS_MARGIN_TEST_MODE:
        *returnDataPtr = psDataPtr->psData.psMarginTestMode;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL:
        *returnDataPtr = psDataPtr->psData.psMarginTestModeControl;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_PS_MARGIN_TEST_RESULTS:
        *returnDataPtr = psDataPtr->psData.psMarginTestResults;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_POWER_SUPPLY),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
    return status;

}   // end of enclosure_access_getPowerSupplyU8


/*!*************************************************************************
 *                         @fn base_enclosure_access_getPowerSupplyU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Power supply fields
 *      that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U16 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Nov-2009: Rajesh V. - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getPowerSupplyU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u16_t *returnDataPtr)
{

    fbe_encl_power_supply_info_t *psDataPtr = NULL;
    fbe_edal_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
    if (psDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_PS_INPUT_POWER:
        *returnDataPtr = psDataPtr->psData.inputPower;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
                                   __FUNCTION__, 
                                   enclosure_access_printComponentType(FBE_ENCL_POWER_SUPPLY),
                                   enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
    return status;

}   // end of enclosure_access_getPowerSupplyU16


/*!*************************************************************************
 *                         @fn base_enclosure_access_getConnectorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Connector fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{

    fbe_base_connector_info_t           *connectorDataPtr;  
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    connectorDataPtr = (fbe_base_connector_info_t *)generalDataPtr;
    if (connectorDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            connectorDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {   

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_CONNECTOR),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
    return status;
}   // end of enclosure_access_getConnectorU8
/*!*************************************************************************
 *                         @fn base_enclosure_access_getLccU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object LCC fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{

    fbe_base_lcc_info_t                 *lccDataPtr = NULL;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    lccDataPtr = (fbe_base_lcc_info_t *)generalDataPtr;
    if (lccDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = lccDataPtr->lccData.lccSideId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_LOCATION:
        *returnDataPtr = lccDataPtr->lccData.lccLocation;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_LCC),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of enclosure_access_getLccU8
/*!*************************************************************************
 *                         @fn base_enclosure_access_getTempSensorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Temp Sensor fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{

    fbe_encl_temp_sensor_info_t         *tempSensorDataPtr = NULL;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    tempSensorDataPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;
    if (tempSensorDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            tempSensorDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {   
    case FBE_ENCL_COMP_SUBTYPE:
        *returnDataPtr = tempSensorDataPtr->tempSensorData.tempSensorSubtype;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = tempSensorDataPtr->tempSensorData.tempSensorSideId;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_TEMP_SENSOR),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of enclosure_access_getTempSensorU8


/*!*************************************************************************
 *       @fn base_enclosure_access_getTempSensorU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object 
 *      Temp Sensor fields that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U16 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jan-2010: Rajesh V. - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getTempSensorU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u16_t *returnDataPtr)
{

    fbe_encl_temp_sensor_info_t         *tempSensorDataPtr = NULL;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    tempSensorDataPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;
    if (tempSensorDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            tempSensorDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {   
    case FBE_ENCL_TEMP_SENSOR_TEMPERATURE:
        *returnDataPtr = tempSensorDataPtr->tempSensorData.temp;
        status = FBE_EDAL_STATUS_OK;
        break;
    
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_TEMP_SENSOR),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of enclosure_access_getTempSensorU16


/*!*************************************************************************
 *                         @fn base_enclosure_access_getCoolingU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Colling fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Feb-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_u8_t *returnDataPtr)
{

    fbe_encl_cooling_info_t             *coolingDataPtr = NULL;
    fbe_edal_status_t                   status; 

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    coolingDataPtr = (fbe_encl_cooling_info_t *)generalDataPtr;
    if (coolingDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            coolingDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    
    case FBE_ENCL_COMP_SUBTYPE:
        *returnDataPtr = coolingDataPtr->coolingData.coolingSubtype;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = coolingDataPtr->coolingData.coolingSideId;
        status = FBE_EDAL_STATUS_OK;
        break;
            
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_COOLING_COMPONENT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
    
    return status;

}   // end of enclosure_access_getCoolingU8

/*!*************************************************************************
 *                         @fn base_enclosure_access_getEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Encl fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U32 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    09-Mar-2000: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t *returnDataPtr)
{
    fbe_base_enclosure_info_t       *enclDataPtr;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_base_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_RESET_RIDE_THRU_COUNT:
        *returnDataPtr = enclDataPtr->enclResetRideThruCount;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_ENCLOSURE),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_getEnclosureU32

/*!*************************************************************************
 *                         @fn base_enclosure_access_getDriveU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u64_t *returnDataPtr)
{
    fbe_encl_drive_slot_info_t      *driveDataPtr;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    driveDataPtr = (fbe_encl_drive_slot_info_t *)generalDataPtr;
    if (driveDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            driveDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;
}   // end of enclosure_access_getDriveU64


/*!*************************************************************************
 *                         @fn base_enclosure_access_getEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Encl fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    09-Mar-2000: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr)
{
    fbe_base_enclosure_info_t       *enclDataPtr;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_base_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_FAULT_LED_REASON:
        *returnDataPtr = enclDataPtr->enclFaultLedReason;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_ENCLOSURE),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_getEnclosureU32

/*!*************************************************************************
 *                         @fn base_enclosure_access_component_status_data_ptr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get and return component and status data pointers
 *       for all components.
 *      
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param componentType - enum for components
 *      @param genComponentDataPtr - pointer to the general component data
 *      @param genStatusDataPtr - pointer to the general status data
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    04-Jan-2009: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_component_status_data_ptr(fbe_edal_general_comp_handle_t generalDataPtr,
                                                fbe_enclosure_component_types_t componentType,
                                                fbe_encl_component_general_info_t   **genComponentDataPtr,
                                                fbe_encl_component_general_status_info_t  **genStatusDataPtr)
{

    fbe_encl_drive_slot_info_t          *driveDataPtr;
    fbe_encl_power_supply_info_t        *psDataPtr;
    fbe_encl_cooling_info_t             *coolingDataPtr;
    fbe_encl_temp_sensor_info_t         *tempSensorDataPtr;
    fbe_base_lcc_info_t                 *lccDataPtr = NULL;
    fbe_base_enclosure_info_t           *enclDataPtr = NULL;
    fbe_base_connector_info_t           *connectorDataPtr;

    switch (componentType)
    {
    case FBE_ENCL_POWER_SUPPLY:
        psDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
        *genComponentDataPtr = &(psDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_DRIVE_SLOT:
        driveDataPtr = (fbe_encl_drive_slot_info_t *)generalDataPtr;
        *genComponentDataPtr = &(driveDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_COOLING_COMPONENT:
        coolingDataPtr = (fbe_encl_cooling_info_t *)generalDataPtr;
        *genComponentDataPtr = &(coolingDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_TEMP_SENSOR:
        tempSensorDataPtr = (fbe_encl_temp_sensor_info_t *)generalDataPtr;
        *genComponentDataPtr = &(tempSensorDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_LCC:
        lccDataPtr = (fbe_base_lcc_info_t *)generalDataPtr;
        *genComponentDataPtr = &(lccDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_ENCLOSURE:  
        enclDataPtr = (fbe_base_enclosure_info_t *)generalDataPtr;
        *genComponentDataPtr = &(enclDataPtr->generalEnclosureInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_CONNECTOR:
        connectorDataPtr = (fbe_base_connector_info_t *)generalDataPtr;
        *genComponentDataPtr = &(connectorDataPtr->generalComponentInfo);
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_EXPANDER:
        *genComponentDataPtr = (fbe_encl_component_general_info_t *)generalDataPtr;
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;;
        break;
    case FBE_ENCL_EXPANDER_PHY:
        *genComponentDataPtr = (fbe_encl_component_general_info_t *)generalDataPtr;
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;;
    break;
    case FBE_ENCL_DISPLAY:
        *genComponentDataPtr = (fbe_encl_component_general_info_t *)generalDataPtr;
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    case FBE_ENCL_IO_MODULE:
    case FBE_ENCL_BEM:
    case FBE_ENCL_IO_PORT:
    case FBE_ENCL_MGMT_MODULE:
    case FBE_ENCL_MEZZANINE:
    case FBE_ENCL_PLATFORM:
    case FBE_ENCL_SUITCASE:
    case FBE_ENCL_BMC:
    case FBE_ENCL_MISC:
    case FBE_ENCL_FLT_REG:
    case FBE_ENCL_SLAVE_PORT:
    case FBE_ENCL_RESUME_PROM:
    case FBE_ENCL_TEMPERATURE:
    case FBE_ENCL_SPS:
    case FBE_ENCL_BATTERY:
    case FBE_ENCL_CACHE_CARD:
    case FBE_ENCL_DIMM:
    case FBE_ENCL_SSD:
    case FBE_ENCL_SSC:
        *genComponentDataPtr = (fbe_encl_component_general_info_t *)generalDataPtr;
        *genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, no support for %s\n", 
            __FUNCTION__, enclosure_access_printComponentType(componentType));
        return FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED;
        break;
    }
 
    return FBE_EDAL_STATUS_OK;

}

/*!*************************************************************************
 *                         @fn base_enclosure_access_getDriveBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from base enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_getDriveBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_bool_t *returnDataPtr)
{
    fbe_encl_drive_slot_info_t      *driveDataPtr;
    fbe_edal_status_t                    status;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    driveDataPtr = (fbe_encl_drive_slot_info_t *)generalDataPtr;
    if (driveDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            driveDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_DRIVE_BYPASSED:
        *returnDataPtr = driveDataPtr->driveFlags.enclDriveBypassed;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_LOGGED_IN:
        *returnDataPtr = driveDataPtr->driveFlags.enclDriveLoggedIn;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_INSERT_MASKED:
        *returnDataPtr = driveDataPtr->driveFlags.enclDriveInsertMasked;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BYPASS_DRIVE:
        *returnDataPtr = driveDataPtr->driveFlags.bypassEnclDrive;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_DEVICE_OFF:
        *returnDataPtr = driveDataPtr->driveFlags.driveDeviceOff;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL:
        *returnDataPtr = driveDataPtr->driveFlags.userReqPowerCntl;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_POWER_CYCLE_PENDING:
        *returnDataPtr = driveDataPtr->driveFlags.drivePowerCyclePending;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_POWER_CYCLE_COMPLETED:
        *returnDataPtr = driveDataPtr->driveFlags.drivePowerCycleCompleted;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_BATTERY_BACKED:
        *returnDataPtr = driveDataPtr->driveFlags.driveBatteryBacked;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_getDriveBool

/*!*************************************************************************
 * @fn fbe_edal_init_block
 **************************************************************************
 *
 *  @brief
 *      This function allocates and initializes the EDAL block.
 *
 *  @param edal_block_handle - Pointer to the edal block.
 *  @param block_size - The size of the edal block.
 *  @param enclosure_type - The enclosure type.
 *  @param number_of_component_types - 
 *
 *  @return   fbe_edal_status_t - 
 *
 *  NOTES:
 *
 *  HISTORY:
 *    22-Feb-2010: PHE - Copied from fbe_sas_viper_allocate_encl_data_block() and 
 *                       make it more generic.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_init_block(fbe_edal_block_handle_t edal_block_handle, 
                         fbe_u32_t block_size, 
                         fbe_u8_t enclosure_type,
                         fbe_u8_t number_of_component_types)
{
    fbe_enclosure_component_block_t   * component_block = NULL;
    fbe_edal_status_t                   edal_status = FBE_EDAL_STATUS_OK;

    if(edal_block_handle == NULL)
    {
        edal_status = FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    else
    {
        component_block = (fbe_enclosure_component_block_t *)edal_block_handle;

        component_block->enclosureType = enclosure_type;
        component_block->enclosureBlockCanary = EDAL_BLOCK_CANARY_VALUE;
        component_block->numberComponentTypes = 0;
        component_block->maxNumberComponentTypes = number_of_component_types;
        component_block->overallStateChangeCount = 0;
        component_block->pNextBlock = NULL;
        component_block->blockSize = block_size;
        component_block->availableDataSpace  = component_block->blockSize - 
                        (sizeof(fbe_enclosure_component_t) * component_block->maxNumberComponentTypes) -
                        (sizeof(fbe_enclosure_component_block_t));
    }

    return (edal_status);
}// End of fbe_edal_init_block


/*!*************************************************************************
 * @fn fbe_edal_can_individual_component_fit_in_block
 **************************************************************************
 *
 *  @brief
 *      This function checks whether the component with the specified count 
 *      can fit into one block.
 *
 *  @param    edal_block_handle - The pointer to the edal block.
 *  @param    component_type - the component type. 
 *
 *  @return   fbe_edal_status_t - 
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_can_individual_component_fit_in_block(fbe_edal_block_handle_t edal_block_handle, 
                                         fbe_enclosure_component_types_t component_type)
{
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK;
    fbe_enclosure_component_block_t * component_block = NULL;
    fbe_u32_t component_size = 0;  // individual size.

    component_block = (fbe_enclosure_component_block_t *)edal_block_handle;

    edal_status = fbe_edal_get_component_size(component_block->enclosureType, 
                                              component_type, 
                                              &component_size);
    if(edal_status == FBE_EDAL_STATUS_OK)
    {
        if ((component_size +            
                (sizeof(fbe_enclosure_component_t) * component_block->maxNumberComponentTypes) +
                (sizeof(fbe_enclosure_component_block_t))) > component_block->blockSize)
        {
            edal_status = FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE;
        }   
    }   

    return edal_status;
}

/*!*************************************************************************
 *                         @fn fbe_edal_fit_component
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function tries to fit a component into its block chain.
 *
 *  PARAMETERS:
 *      @param component_block - pointer to first EDAL block in the chain.
 *      @param componentType - component type
 *      @param number_elements - the total number of element with this component type
 *      @param componentTypeSize - component size
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the fit operation
 *         FBE_EDAL_STATUS_OK - the operation is successful.
 *         FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE - not enough space, need to allocate new block.
 *         Otherwise - error countered.        
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Dec-2008: NC - Created
 *    16-Mar-2010: PHE - Modified to handle the case that the component type could be 
 *                       located in mulitiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_fit_component(fbe_enclosure_component_block_t *component_block,
                       fbe_enclosure_component_types_t componentType,
                       fbe_u32_t componentCount,
                       fbe_u32_t componentSize)
{
    fbe_enclosure_component_block_t *local_block;
    fbe_u32_t componentCountAllowed = 0;
    fbe_u32_t componentCountInLocalBlock = 0;
    fbe_u32_t firstComponentIndex = 0;
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK; 
    
    if((componentCount == 0) || (componentSize == 0))
    {
        /* We only need the space for the component type descriptor. 
         * This space was reserved while we calculate the availableDataSpace during the block init.
         * It should never fail.
         */
        edal_status = fbe_edal_fit_component_in_local_block(component_block,
                                componentType,
                                0,
                                componentCount,
                                componentSize);

        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            enclosure_access_log_info("EDAL: %s (total size 0) fit_component_in_local_block failed, "
                    "it should never happen.\n",
                    enclosure_access_printComponentType(componentType));
        }
            
        // No need to continue.
        return edal_status;
    }

    local_block = component_block;
    /* Check whether the existing blocks have enough space to fit all the components. */
    while (local_block != NULL)
    {
        if(local_block->numberComponentTypes >= local_block->maxNumberComponentTypes)
        {
            enclosure_access_log_error("EDAL: %s exceeding maxNumberComponentTypes %d.\n",
                                    enclosure_access_printComponentType(componentType),
                                    local_block->maxNumberComponentTypes);

            return FBE_EDAL_STATUS_ERROR;
        }
            
        componentCountAllowed += local_block->availableDataSpace/componentSize;

        if(componentCountAllowed < componentCount) 
        {
            /* Continue to look for more space in the next block. */
            local_block = local_block->pNextBlock;
        }
        else
        {
            /* The enough space is found. let's break out */
            break;
        }
    }

    /* if it fits */
    if (local_block!=NULL)
    {
        /* Start from the first block in the chain. */
        local_block = component_block;
        
        while (local_block != NULL)
        {
            componentCountAllowed = local_block->availableDataSpace/componentSize;
        
            if(componentCountAllowed > 0)
            {
                componentCountInLocalBlock  = 
                    (componentCountAllowed < (componentCount - firstComponentIndex)) ? 
                          componentCountAllowed : (componentCount - firstComponentIndex);
                
                edal_status = fbe_edal_fit_component_in_local_block(local_block,
                                componentType,
                                firstComponentIndex,
                                componentCountInLocalBlock,
                                componentSize);

                if(edal_status != FBE_EDAL_STATUS_OK)
                {
                    enclosure_access_log_info("EDAL: %s fit_component_in_local_block failed, "
                         "it should never happen.\n",
                         enclosure_access_printComponentType(componentType));

                    return edal_status; 
                }
            
                firstComponentIndex += componentCountInLocalBlock;

                if(firstComponentIndex < componentCount)
                {
                    local_block = local_block->pNextBlock;
                }
                else
                {
                    /* all done. */
                    break;
                }
            }
            else
            {
                local_block = local_block->pNextBlock;
            }
        }// End of - while (local_block != NULL)
    }// End of - if (local_block!=NULL)         
    else
    {
        /* No enough space availabe to fit all the components. */
        edal_status = FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE;
    }

    return edal_status;
}

/*!*************************************************************************
 *                         @fn fbe_edal_fit_component_in_local_block
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function tries to fit the components into the specified local block only.
 *
 *  PARAMETERS:
 *      @param local_block - pointer to the local edal block
 *      @param componentType - component type
 *      @param firstComponentIndex - the first component index for the component type in this block.
 *      @param numberOfElements - number of elements to be added
 *      @param componentSize - component size
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the fit operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Mar-2010: PHE - Moved over from fbe_edal_fit_component
 *
 **************************************************************************/
fbe_edal_status_t 
fbe_edal_fit_component_in_local_block(fbe_enclosure_component_block_t *local_block,
                       fbe_enclosure_component_types_t componentType,
                       fbe_u32_t firstComponentIndex,
                       fbe_u32_t numberOfElements,
                       fbe_u32_t componentSize)
{
    fbe_enclosure_component_t   *component_data;
    fbe_u32_t totalSize = 0;
    
    totalSize = numberOfElements * componentSize;
     
    // Sanity check. 
    if(totalSize > local_block->availableDataSpace)
    {
        enclosure_access_log_info("EDAL: failed to fit the components into local block, comp type %d, numberOfElements %d.\n",
                    componentType, numberOfElements);
        
        return FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE;
    }
    
    component_data = edal_calculateComponentTypeDataStart(local_block);
            
    /* move down its own slot */
    component_data += local_block->numberComponentTypes;

    component_data->componentType = componentType;
    component_data->maxNumberOfComponents = numberOfElements;
    component_data->firstComponentIndex = firstComponentIndex;

    /* offset between the begining of the component descriptor and begining of the data 
        * begining of the data: 
        *      total size - available size
        * begining of the component descriptor:
        *      block header + number of component * component descriptor
        */
    component_data->firstComponentStartOffset = 
        local_block->blockSize - local_block->availableDataSpace - 
        (sizeof(fbe_enclosure_component_t) * local_block->numberComponentTypes) -
        (sizeof(fbe_enclosure_component_block_t));

    component_data->componentSize = componentSize;
    component_data->componentOverallStateChangeCount = 0;
    component_data->componentOverallStatus = FBE_ENCL_COMPONENT_TYPE_STATUS_OK;

    edal_init_encl_component_data(component_data);

    // reduce the spare
    local_block->availableDataSpace -= totalSize;
    // add one entry
    local_block->numberComponentTypes ++;
#if 0  // debug only
    enclosure_access_log_info("EDAL: Component%s (size %d) has %d in local block, offset 0x%x.\n",
                        enclosure_access_printComponentType(componentType), 
                        component_data->componentSize,
                        component_data->maxNumberOfComponents,
                        component_data->firstComponentStartOffset);
#endif
    return FBE_EDAL_STATUS_OK;
}

/*!*************************************************************************
 * @fn fbe_edal_drop_the_tail_block(
 *                  fbe_enclosure_component_block_t *component_block)
 **************************************************************************
 *
 *  @brief:
 *      This function will drop the last block from the chain
 *
 *
 *  @param    component_block - pointer to component_block
 *
 *  @return   pointer the block it has dropped, NULL if nothing
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Dec-2008: Naizhong Chiu - created
 *
 **************************************************************************/
fbe_enclosure_component_block_t *
fbe_edal_drop_the_tail_block(fbe_enclosure_component_block_t *component_block)
{
    fbe_enclosure_component_block_t *prev_block, *dropped_block = NULL;

    prev_block = component_block;
    if (prev_block!=NULL)
    {
        dropped_block = prev_block->pNextBlock;
        if (dropped_block != NULL)
        {
            while (dropped_block->pNextBlock != NULL)
            {
                prev_block = dropped_block;
                dropped_block = dropped_block->pNextBlock;
            }
        }
        /* remove it from the chain */
        prev_block->pNextBlock = NULL;
    }

    return (dropped_block);
}

/*!*************************************************************************
 * @fn fbe_edal_append_block(
 *                  fbe_enclosure_component_block_t *component_block)
 *                  fbe_enclosure_component_block_t *new_block)
 **************************************************************************
 *
 *  @brief:
 *      This function will append a new block to the end of the chain.
 *      If the head of the chain is NULL, the chain will point to the
 *  new block.
 *
 *
 *  @param    component_block - pointer to component_block
 *  @param    new_block - pointer to the new block to be appended
 *
 *  @return   fbe_edal_status_t
 *
 *  NOTES:
 *
 *  HISTORY:
 *    23-Dec-2008: Naizhong Chiu - created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_append_block(fbe_enclosure_component_block_t *component_block,
                      fbe_enclosure_component_block_t *new_block)
{
    fbe_enclosure_component_block_t *local_block, *prev_block;

    if (component_block == NULL)
    {
        component_block = new_block;
    }
    else
    {
        prev_block = component_block;
        local_block = prev_block->pNextBlock;
        while (local_block != NULL)
        {
            /* Move down the chain */
            prev_block = local_block;
            local_block = local_block->pNextBlock;
        }
        prev_block->pNextBlock = new_block;
        new_block->pNextBlock = NULL;
    }

    return (FBE_EDAL_STATUS_OK);
}

/*!*************************************************************************
 * @fn edal_init_encl_component_data(
 *                  fbe_enclosure_component_block_t *enclComponentBlockPtr)
 **************************************************************************
 *
 *  @brief:
 *      This function will initialize enclosure object component data.
 *      So far it only fills in component canaries.
 *
 *
 *  @param    enclComponentTypeDataPtr - pointer to component type descriptor
 *
 *  @return   void
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Jul-2008: Joe Perry - Created
 *    23-Dec-2008: Naizhong Chiu - Moved from eses_enclosure_main
 *
 **************************************************************************/
void
edal_init_encl_component_data(fbe_enclosure_component_t *enclComponentTypeDataPtr)
{
    fbe_u32_t                           componentIndex;
    fbe_encl_component_general_info_t   *generalComponentDataPtr;

    switch (enclComponentTypeDataPtr->componentType)
    {
    case FBE_ENCL_ENCLOSURE:
    case FBE_ENCL_POWER_SUPPLY:
    case FBE_ENCL_COOLING_COMPONENT:
    case FBE_ENCL_DRIVE_SLOT:
    case FBE_ENCL_TEMP_SENSOR:
    case FBE_ENCL_CONNECTOR:
    case FBE_ENCL_EXPANDER:
    case FBE_ENCL_EXPANDER_PHY:
    case FBE_ENCL_LCC:
    case FBE_ENCL_DISPLAY:
    case FBE_ENCL_IO_MODULE:
    case FBE_ENCL_IO_PORT:
    case FBE_ENCL_MGMT_MODULE:
    case FBE_ENCL_BEM:
    case FBE_ENCL_MEZZANINE:
    case FBE_ENCL_PLATFORM:
    case FBE_ENCL_SUITCASE:
    case FBE_ENCL_BMC:
    case FBE_ENCL_MISC:
    case FBE_ENCL_FLT_REG:
    case FBE_ENCL_SLAVE_PORT:
    case FBE_ENCL_RESUME_PROM:
    case FBE_ENCL_TEMPERATURE:
    case FBE_ENCL_SPS:
    case FBE_ENCL_BATTERY:
    case FBE_ENCL_CACHE_CARD:
    case FBE_ENCL_DIMM:
    case FBE_ENCL_SSD:
    case FBE_ENCL_SSC:
        generalComponentDataPtr = (fbe_encl_component_general_info_t *)
            ((fbe_u8_t *)enclComponentTypeDataPtr + enclComponentTypeDataPtr->firstComponentStartOffset);
        for (componentIndex = 0; componentIndex < enclComponentTypeDataPtr->maxNumberOfComponents; componentIndex++)
        {
            generalComponentDataPtr->generalStatus.componentCanary = EDAL_COMPONENT_CANARY_VALUE;
            // set pointer to next component
            generalComponentDataPtr = (fbe_encl_component_general_info_t *)
                ((fbe_u8_t *)generalComponentDataPtr + enclComponentTypeDataPtr->componentSize);
        }
        break;

    default:
        enclosure_access_log_error("CompType %d Not Supported\n", enclComponentTypeDataPtr->componentType);
        break;

    }   // end of switch

}   // end of edal_init_encl_component_data

/*!*************************************************************************
 *    @fn fbe_base_enclosure_access_getSpecificComponent_with_sanity_check
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure object's Component Type Data
 *      address.
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param componentBlockPtr - pointer to a component block.
 *      @param componentType - Component Type enumeration
 *      @param index - number of the component to access
 *      @param encl_component - pointer to the specified Component Data
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    8-Jun-2009: Sachin D - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(fbe_enclosure_component_block_t *enclosureComponentBlockPtr,
                                               fbe_enclosure_component_types_t component,
                                               fbe_u32_t index,
                                               void **encl_component)
{
    fbe_edal_status_t status = FBE_EDAL_STATUS_ERROR;
    
    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }

    if (enclosureComponentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, enclosureComponentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    status  = 
    fbe_base_enclosure_access_getSpecificComponent(enclosureComponentBlockPtr,
                                                    component,
                                                    index,
                                                    encl_component);
    return status;
}   // end of fbe_base_enclosure_access_getSpecificComponent_with_sanity_check
