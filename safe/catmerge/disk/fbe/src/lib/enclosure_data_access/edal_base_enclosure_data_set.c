/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_base_enclosure_data_set.c
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to update data from the Base
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


/*!*************************************************************************
 *                         @fn base_enclosure_access_setBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param newValue - new boolean value
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
base_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t newValue)
{
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    fbe_edal_status_t                        status;


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
        if (genComponentDataPtr->generalFlags.componentInserted != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genComponentDataPtr->generalFlags.componentInserted = newValue;
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        break;

    case FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG:
        genComponentDataPtr->generalFlags.componentInsertedPriorConfig = newValue;
        break;

    case FBE_ENCL_COMP_FAULTED:
        if (genComponentDataPtr->generalFlags.componentFaulted != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genComponentDataPtr->generalFlags.componentFaulted = newValue;
        }
        break;

    case FBE_ENCL_COMP_POWERED_OFF:
        if (genComponentDataPtr->generalFlags.componentPoweredOff != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genComponentDataPtr->generalFlags.componentPoweredOff = newValue;
        }
        break;

    case FBE_ENCL_COMP_FAULT_LED_ON:
        if (genComponentDataPtr->generalFlags.componentFaultLedOn != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genComponentDataPtr->generalFlags.componentFaultLedOn = newValue;
        }
        break;

    case FBE_ENCL_COMP_MARKED:  // This status gets set by polling every 3 seconds.
        if (genComponentDataPtr->generalFlags.componentMarked != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genComponentDataPtr->generalFlags.componentMarked = newValue;
        }
        break;

    case FBE_ENCL_COMP_TURN_ON_FAULT_LED:
        /* check against the read value or the write value.
         * The reason to check with the write value:
         * We don't want to send out the write command again 
         * if the status is already what we want.
         * The reason to check with the write value:
         * If the turning off command comes down after the turning on command,
         * and the componentFaultLedOn status has not reflected the ON status yet, 
         * the turning off command would be ignored which is not what we want.
         */
        if((newValue != genComponentDataPtr->generalFlags.componentFaultLedOn) ||
         (newValue != genComponentDataPtr->generalFlags.turnComponentFaultLedOn))
        {
            genStatusDataPtr->readWriteFlags.writeDataUpdate = TRUE;
            genStatusDataPtr->readWriteFlags.writeDataSent = FALSE;   
            genComponentDataPtr->generalFlags.turnComponentFaultLedOn = newValue;
        }
        break;

    case FBE_ENCL_COMP_MARK_COMPONENT:
        /* check against the read value */
        if (newValue != genComponentDataPtr->generalFlags.componentMarked)
        {
            genStatusDataPtr->readWriteFlags.writeDataUpdate = TRUE;
            genStatusDataPtr->readWriteFlags.writeDataSent = FALSE;   
            genComponentDataPtr->generalFlags.markComponent = newValue;
        }
        break;

    case FBE_ENCL_COMP_STATE_CHANGE:
        // Do Not set state change for the state change flag itself
        genStatusDataPtr->readWriteFlags.componentStateChange = newValue;
        break;

    case FBE_ENCL_COMP_WRITE_DATA:
        // Do Not set state change for this field
        genStatusDataPtr->readWriteFlags.writeDataUpdate = newValue;
        
        // Update the value of that attribute FBE_ENCL_COMP_WRITE_DATA_SENT at the same time.
        genStatusDataPtr->readWriteFlags.writeDataSent = FALSE;   

        break;

    case FBE_ENCL_COMP_WRITE_DATA_SENT:
        // Do Not set state change for this field
        genStatusDataPtr->readWriteFlags.writeDataSent = newValue;
        break;

    case FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA:
        // Do Not set state change for this field
        genStatusDataPtr->readWriteFlags.emcEnclCtrlWriteDataUpdate = newValue;

        // Update the value of that attribute FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT at the same time.
        genStatusDataPtr->readWriteFlags.emcEnclCtrlWriteDataSent = FALSE;   
       
        break;

    case FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT:
        // Do Not set state change for this field
        genStatusDataPtr->readWriteFlags.emcEnclCtrlWriteDataSent = newValue;
        break;

    case FBE_ENCL_COMP_STATUS_VALID:
        if (genStatusDataPtr->readWriteFlags.componentStatusValid != newValue)
        {
            genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
            genStatusDataPtr->readWriteFlags.componentStatusValid = newValue;
        }
         break;

    default:
        /* Maybe it's not common attribute, let's check on 
         * individual component handler.
         */
        switch (componentType)
        {
        case FBE_ENCL_POWER_SUPPLY:
            status = base_enclosure_access_setPowerSupplyBool(generalDataPtr,
                                                              attribute,
                                                              newValue);

            break;
        case FBE_ENCL_DRIVE_SLOT:
            status = base_enclosure_access_setDriveBool(generalDataPtr,
                                                        attribute,
                                                        newValue);
            break;
        case FBE_ENCL_COOLING_COMPONENT:
            status = base_enclosure_access_setCoolingBool(generalDataPtr,
                                                          attribute,
                                                          newValue);
            break;
        case FBE_ENCL_TEMP_SENSOR:
            status = base_enclosure_access_setTempSensorBool(generalDataPtr,
                                                             attribute,
                                                             newValue);
            break;
        case FBE_ENCL_LCC:
            status = base_enclosure_access_setLccBool(generalDataPtr,
                                                             attribute,
                                                             newValue);
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

}   // end of base_enclosure_access_setBool

/*!*************************************************************************
 *                         @fn base_enclosure_access_setU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are uint8.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Aug-2008: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u8_t newValue)
{
    fbe_edal_status_t                        status;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return (status);
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
     * Access the appropriate attribute from the General Component Data
     */
    switch (attribute)
    {
        case FBE_ENCL_COMP_ADDL_STATUS:
            if(genStatusDataPtr->addlComponentStatus != newValue)
            {
                genStatusDataPtr->readWriteFlags.componentStateChange = TRUE;
                genStatusDataPtr->addlComponentStatus = newValue;
                status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
        break;

        default:
        /* Maybe it's not common attribute, let's check on 
         * individual component handler.
         */
            switch (componentType)
            {
                case FBE_ENCL_DRIVE_SLOT:
                    status = base_enclosure_access_setDriveU8(generalDataPtr,
                                                             attribute,
                                                             newValue);
                break;

                case FBE_ENCL_ENCLOSURE:
                    status = base_enclosure_access_setEnclosureU8(generalDataPtr,
                                                                 attribute,
                                                                 newValue);
                break;

                case FBE_ENCL_POWER_SUPPLY:
                    status = base_enclosure_access_setPowerSupplyU8(generalDataPtr,
                                                               attribute,
                                                               newValue);
                break;
                
                case FBE_ENCL_COOLING_COMPONENT:
                    status = base_enclosure_access_setCoolingU8(generalDataPtr,
                                                               attribute,
                                                               newValue);
                break;

                case FBE_ENCL_TEMP_SENSOR:
                    status = base_enclosure_access_setTempSensorU8(generalDataPtr,
                                                               attribute,
                                                               newValue);
                break;

                case FBE_ENCL_LCC:
                    status = base_enclosure_access_setLccU8(generalDataPtr,
                                                               attribute,
                                                               newValue);
                break;

                case FBE_ENCL_CONNECTOR:
                case FBE_ENCL_EXPANDER:
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
}   // end of base_enclosure_access_setU8


/*!*************************************************************************
 *                         @fn base_enclosure_access_setU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are uint16.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param newValue - new U16 value
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
base_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t componentType,
                             fbe_u16_t newValue)
{
    fbe_edal_status_t                        status;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                             componentType,
                                                             &genComponentDataPtr,
                                                             &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return (status);
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
     * Access the appropriate attribute from the General Component Data
     */
    switch (attribute)
    {
    default:

        // Check the individual component handler for the attribute
        switch(componentType)
        {
        case FBE_ENCL_POWER_SUPPLY:
            status = base_enclosure_access_setPowerSupplyU16(generalDataPtr,
                                                             attribute,
                                                             newValue);
            break;

        case FBE_ENCL_TEMP_SENSOR:
            status = base_enclosure_access_setTempSensorU16(generalDataPtr,
                                                            attribute,
                                                            newValue);
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

}   // end of base_enclosure_access_setU16


/*!*************************************************************************
 *                         @fn base_enclosure_access_setU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are uint64.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param newValue - new U64 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Aug-2008: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u64_t newValue)
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
        status = base_enclosure_access_setDriveU64(generalDataPtr,
                                                attribute,
                                                newValue);
        break;
    case FBE_ENCL_ENCLOSURE:
        status = base_enclosure_access_setEnclosureU64(generalDataPtr,
                                                attribute,
                                                newValue);
        break;


    case FBE_ENCL_LCC:
        status = base_enclosure_access_setLccU64(generalDataPtr,
                                                attribute,
                                                newValue);
        break;

    case FBE_ENCL_POWER_SUPPLY:
    case FBE_ENCL_CONNECTOR:
    case FBE_ENCL_EXPANDER:
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(componentType),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_setU64


/*!*************************************************************************
 *                         @fn base_enclosure_access_setU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from enclosure object Drive fields
 *      that are uint32.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param componentType - component type
 *      @param newValue - new U32 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t newValue)
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
        status = base_enclosure_access_setEnclosureU32(generalDataPtr,
                                                       attribute,
                                                       newValue);
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

}   // end of base_enclosure_access_setU32

/*!*************************************************************************
 *                         @fn base_enclosure_access_setU8
 **************************************************************************
 *  @brief
 *      This function updates string data for base enclosure components.
 *
 *  @param generalDataPtr - pointer to a component object
 *  @param attribute - enum for attribute to get
 *  @param componentType - component type
 *  @param length - the length of the string
 *  @param newValue - the pointer to the string
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  @version
 *    23-Feb-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t length,
                            char * newValue)
{
    fbe_edal_status_t                        status;
    fbe_encl_component_general_info_t   *genComponentDataPtr = NULL;
    fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    /*
     * Get the General Component Data based on component type
     */

    status = base_enclosure_access_component_status_data_ptr(generalDataPtr,
                                                componentType,
                                                &genComponentDataPtr,
                                                &genStatusDataPtr);

    if(status != FBE_EDAL_STATUS_OK)
    {
        return (status);
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
     * Access the appropriate attribute from the General Component Data
     */
    switch (attribute)
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
}   // end of base_enclosure_access_setStr

/*!*************************************************************************
 *                         @fn base_enclosure_access_setPowerSupplyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Power Supply 
 *      fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
base_enclosure_access_setPowerSupplyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                         fbe_base_enclosure_attributes_t attribute,
                                         fbe_bool_t newValue)
{
    fbe_encl_power_supply_info_t    *psInfoPtr;
    fbe_edal_status_t                     status;

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
        if (psInfoPtr->psFlags.acFail != newValue)
        {
            psInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            psInfoPtr->psFlags.acFail = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_PS_DC_DETECTED:
        if (psInfoPtr->psFlags.dcDetected != newValue)
        {
            psInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            psInfoPtr->psFlags.dcDetected = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
  case FBE_ENCL_PS_SUPPORTED:
        if (psInfoPtr->psFlags.psSupported != newValue)
        {
            psInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;           
            psInfoPtr->psFlags.psSupported = newValue;
        }
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

    return status;

}   // end of base_enclosure_access_setPowerSupplyBool()


/*!*************************************************************************
 *                         @fn base_enclosure_access_setCoolingBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object cooling 
 *      component fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
base_enclosure_access_setCoolingBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t newValue)
{
    fbe_encl_cooling_info_t    *coolingDataPtr;
    fbe_edal_status_t                status;

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
    case FBE_ENCL_COOLING_MULTI_FAN_FAULT:
        if (coolingDataPtr->coolingFlags.multiFanFault != newValue)
        {
            coolingDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            coolingDataPtr->coolingFlags.multiFanFault = newValue;
        }
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

    return status;

}   // end of base_enclosure_access_setCoolingBool()


/*!*************************************************************************
 *                         @fn base_enclosure_access_setTempSensorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object temperature 
 *      sensor fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
base_enclosure_access_setTempSensorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t newValue)
{
    fbe_encl_temp_sensor_info_t    *tempSensorInfoPtr;
    fbe_edal_status_t                    status;

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
        if (tempSensorInfoPtr->tempSensorFlags.overTempWarning != newValue)
        {
            tempSensorInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            tempSensorInfoPtr->tempSensorFlags.overTempWarning = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_TEMP_SENSOR_OT_FAILURE:
        if (tempSensorInfoPtr->tempSensorFlags.overTempFailure != newValue)
        {
            tempSensorInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            tempSensorInfoPtr->tempSensorFlags.overTempFailure = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_TEMP_SENSOR_TEMPERATURE_VALID:
        if (tempSensorInfoPtr->tempSensorFlags.tempValid != newValue)
        {
            tempSensorInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            tempSensorInfoPtr->tempSensorFlags.tempValid = newValue;
        }
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

    return status;

}   // end of base_enclosure_access_setTempSensorBool()

/*!*************************************************************************
 *                         @fn base_enclosure_access_setLccBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the data in an enclosure object LCC fields 
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
base_enclosure_access_setLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t newValue)
{
    fbe_base_lcc_info_t    *lccInfoPtr;
    fbe_edal_status_t                    status;

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
        if (lccInfoPtr->lccFlags.isLocalLcc != newValue)
        {
            lccInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            lccInfoPtr->lccFlags.isLocalLcc = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_LCC_FAULT_MASKED:
        if (lccInfoPtr->lccFlags.lccFaultMasked != newValue)
        {
            lccInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            lccInfoPtr->lccFlags.lccFaultMasked = newValue;
        }
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

    return status;

}   // end of base_enclosure_access_setLccBool()

/*!*************************************************************************
 *                         @fn base_enclosure_access_setLccU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the data in an enclosure object LCC fields 
 *      that are fbe_u64_t type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
base_enclosure_access_setLccU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u64_t newValue)
{
    fbe_base_lcc_info_t    *lccInfoPtr;
    fbe_edal_status_t       status;

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
        if (lccInfoPtr->lccData.lccFaultStartTimestamp != newValue)
        {
            lccInfoPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            lccInfoPtr->lccData.lccFaultStartTimestamp = newValue;
        }
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

    return status;

}   // end of base_enclosure_access_setLccU64()

/*!*************************************************************************
 *                         @fn base_enclosure_access_setDriveU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Drive fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
base_enclosure_access_setDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue)
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
    case FBE_ENCL_COMP_TYPE:
        if (driveDataPtr->enclDriveType != newValue)
        {
            driveDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            driveDataPtr->enclDriveType = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_DEVICE_OFF_REASON:
        driveDataPtr->driveDeviceOffReason = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_SLOT_NUMBER:
        driveDataPtr->driveSlotNumber = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        status =  FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

}   // end of base_enclosure_access_setDriveU8

/*!******************************************************************************
 *                         @fn base_enclosure_access_setPowerSupplyU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Power Supply
 *      fields that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    25-Feb-2009: PHE - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)
{
    fbe_encl_power_supply_info_t      *powerSupplyDataPtr;
        fbe_edal_status_t             status = FBE_STATUS_GENERIC_FAILURE;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    powerSupplyDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;
    if (powerSupplyDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            powerSupplyDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SIDE_ID:
        powerSupplyDataPtr->psData.psSideId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_PS_INPUT_POWER_STATUS:
        if (powerSupplyDataPtr->psData.inputPowerStatus != newValue)
        {
            /* Remove the setting of the powerSupplyDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange 
             * to TRUE to prevent many notification being sent. 
             */
            powerSupplyDataPtr->psData.inputPowerStatus = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_PS_MARGIN_TEST_MODE:
        if (powerSupplyDataPtr->psData.psMarginTestMode != newValue)
        {
            powerSupplyDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            powerSupplyDataPtr->psData.psMarginTestMode = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL:
        if (powerSupplyDataPtr->psData.psMarginTestModeControl != newValue)
        {
            status = base_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_POWER_SUPPLY,
                                                   TRUE);
            powerSupplyDataPtr->psData.psMarginTestModeControl = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_PS_MARGIN_TEST_RESULTS:
        if (powerSupplyDataPtr->psData.psMarginTestResults != newValue)
        {
            powerSupplyDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            powerSupplyDataPtr->psData.psMarginTestResults = newValue;
        }
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

}   // end of base_enclosure_access_setPowerSupplyU8


/*!******************************************************************************
 *                         @fn base_enclosure_access_setPowerSupplyU16
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Power Supply
 *      fields that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U16 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    29-Oct-2009: Rajesh V. - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setPowerSupplyU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u16_t newValue)
{
    fbe_encl_power_supply_info_t  *powerSupplyDataPtr;
    fbe_edal_status_t             status = FBE_STATUS_GENERIC_FAILURE;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    powerSupplyDataPtr = (fbe_encl_power_supply_info_t *)generalDataPtr;

    if (powerSupplyDataPtr->generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            powerSupplyDataPtr->generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_PS_INPUT_POWER:
        // Component State change needs to be set if this value changes
        // as EMC status page is now requested every 30 seconds.
        if(powerSupplyDataPtr->psData.inputPower != newValue)
        {
            /* Remove the setting of the powerSupplyDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange 
             * to TRUE to prevent many notification being sent. 
             */
            powerSupplyDataPtr->psData.inputPower = newValue;
        }
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

}   // end of base_enclosure_access_setPowerSupplyU8

/*!******************************************************************************
 *                         @fn base_enclosure_access_setCoolingU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Cooling component 
 *      fields that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    17-Sep-2008: hgd - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)
{
    fbe_encl_cooling_info_t      *coolingDataPtr;

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
        coolingDataPtr->coolingData.coolingSubtype = newValue;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        coolingDataPtr->coolingData.coolingSideId = newValue;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_COOLING_COMPONENT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_setCoolingU8

/*!******************************************************************************
 *                         @fn base_enclosure_access_setTempSensorU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object temp sensor 
 *      fields that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    11-Nov-2008: PHE - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)
{
    fbe_encl_temp_sensor_info_t      *tempSensorDataPtr;

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
        tempSensorDataPtr->tempSensorData.tempSensorSubtype = newValue;
        break;

    case FBE_ENCL_COMP_SIDE_ID:
        tempSensorDataPtr->tempSensorData.tempSensorSideId = newValue;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_TEMP_SENSOR),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_setTempSensorU8


/*!******************************************************************************
 *             @fn base_enclosure_access_setTempSensorU16
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object temp sensor 
 *      fields that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U16 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jan-2010: Rajesh V. - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setTempSensorU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u16_t newValue)
{
    fbe_encl_temp_sensor_info_t      *tempSensorDataPtr;

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
        tempSensorDataPtr->tempSensorData.temp = newValue;
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_TEMP_SENSOR),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_setTempSensorU16

/*!******************************************************************************
 *                         @fn base_enclosure_access_setEnclosureU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Enclosure component 
 *      fields that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    29-Sep-2008: Joe Perry - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue)
{
    fbe_base_enclosure_info_t       *enclDataPtr;
    fbe_edal_status_t                    status;

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
    case FBE_ENCL_POSITION:
        if (enclDataPtr->enclPosition != newValue)
        {
            enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            enclDataPtr->enclPosition = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_ADDRESS:
        if (enclDataPtr->enclAddress != newValue)
        {
            enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            enclDataPtr->enclAddress = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_PORT_NUMBER:
        if (enclDataPtr->enclPortNumber != newValue)
        {
            enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            enclDataPtr->enclPortNumber = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_MAX_SLOTS:
        if (enclDataPtr->enclMaxSlot != newValue)
        {
            enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            enclDataPtr->enclMaxSlot = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        if (enclDataPtr->enclSideId != newValue)
        {
            // value change to and from 0xFF should not be considered as side change.            
            if((enclDataPtr->enclSideId == FBE_DIPLEX_ADDRESS_INVALID) || 
                (newValue == FBE_DIPLEX_ADDRESS_INVALID))
            {
                status = FBE_EDAL_STATUS_OK;   
            }
            else
            {
                enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
                status = FBE_EDAL_STATUS_OK_VAL_CHANGED;   
            }
            enclDataPtr->enclSideId = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_ENCLOSURE),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }
 
    return status;

}   // end of base_enclosure_access_setEnclosureU8

/*!******************************************************************************
 *  @fn base_enclosure_access_setLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                    fbe_base_enclosure_attributes_t attribute,
 *                                    fbe_u8_t newValue)
 *******************************************************************************
 *  @brief
 *      This function will set data in an enclosure object Enclosure component 
 *      fields that are U8 type.
 *
 *  @param generalDataPtr - pointer to a component object
 *  @param attribute - enum for attribute to get
 *  @param newValue - new U8 value
 *
 *  @return
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before using the
 *      returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Oct-2008: PHE - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue)
{
    fbe_base_lcc_info_t       *lccDataPtr = NULL;
    fbe_edal_status_t         status = FBE_EDAL_STATUS_ERROR;

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
        lccDataPtr->lccData.lccSideId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_LOCATION:
        lccDataPtr->lccData.lccLocation = newValue;
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
 
    return status;

} // End of function base_enclosure_access_setLccU8

/*!*************************************************************************
 *                         @fn base_enclosure_access_setEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Encl fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U32 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    09-Mar-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t newValue)
{
    fbe_base_enclosure_info_t            *enclDataPtr;

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
        enclDataPtr->enclResetRideThruCount = newValue;
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_setEnclosureU32

/*!*************************************************************************
 *                         @fn base_enclosure_access_setDriveU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Drive fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
base_enclosure_access_setDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t newValue)
{
    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
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

}   // end of base_enclosure_access_setDriveU64


/*!*************************************************************************
 *                         @fn base_enclosure_access_setEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an enclosure object Encl fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    14-June-2013: Rui Chang - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue)
{
    fbe_base_enclosure_info_t            *enclDataPtr;

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
        if (enclDataPtr->enclFaultLedReason != newValue)
        {
            enclDataPtr->generalEnclosureInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            enclDataPtr->enclFaultLedReason = newValue;
        }
        break;
    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_DRIVE_SLOT),
            enclosure_access_printAttribute(attribute));
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of base_enclosure_access_setEnclosureU32

/*!*************************************************************************
 *                         @fn base_enclosure_access_setDriveBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object Drive fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status 
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
base_enclosure_access_setDriveBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_bool_t newValue)

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
        if (driveDataPtr->driveFlags.enclDriveBypassed != newValue)
        {
            driveDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            driveDataPtr->driveFlags.enclDriveBypassed = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_LOGGED_IN:
        driveDataPtr->driveFlags.enclDriveLoggedIn = newValue;;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_INSERT_MASKED:
        if (driveDataPtr->driveFlags.enclDriveInsertMasked != newValue)
        {
            driveDataPtr->generalComponentInfo.generalStatus.readWriteFlags.componentStateChange = TRUE;
            driveDataPtr->driveFlags.enclDriveInsertMasked = newValue;;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BYPASS_DRIVE:
        /* check against read value */
        if (newValue != driveDataPtr->driveFlags.enclDriveBypassed)
        {
            base_enclosure_access_setBool(generalDataPtr,
                                        FBE_ENCL_COMP_WRITE_DATA,
                                        FBE_ENCL_DRIVE_SLOT,
                                        TRUE);

        }
        driveDataPtr->driveFlags.bypassEnclDrive = newValue;
        
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_DRIVE_DEVICE_OFF:
        /* If user requested a new action */
         if ((newValue != driveDataPtr->driveFlags.driveDeviceOff)||
            (newValue != driveDataPtr->generalComponentInfo.generalFlags.componentPoweredOff))
        {
            /* setting this flag will force driveDeviceOff value down to the expander,
             * otherwise, the read status will be used to build the control bytes.
             */
            driveDataPtr->driveFlags.userReqPowerCntl = TRUE;           
            status = base_enclosure_access_setBool(generalDataPtr,
                                        FBE_ENCL_COMP_WRITE_DATA,
                                        FBE_ENCL_DRIVE_SLOT,
                                        TRUE);
            driveDataPtr->driveFlags.driveDeviceOff = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    case FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL:
        driveDataPtr->driveFlags.userReqPowerCntl = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_POWER_CYCLE_PENDING:
        driveDataPtr->driveFlags.drivePowerCyclePending = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

     case FBE_ENCL_COMP_POWER_CYCLE_COMPLETED:
        driveDataPtr->driveFlags.drivePowerCycleCompleted = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

      case FBE_ENCL_DRIVE_BATTERY_BACKED:
         driveDataPtr->driveFlags.driveBatteryBacked = newValue;
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

}   // end of base_enclosure_access_setDriveBool
/*!*************************************************************************
 *                         @fn base_enclosure_Increment_ComponentSwapCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set component swap count
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component object
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2009: Prasenjeet Ghaywat- Created
       31-Aug-2009 : UDY - Modified
 *
 **************************************************************************/
fbe_edal_status_t base_enclosure_Increment_ComponentSwapCount(fbe_edal_component_data_handle_t generalDataPtr)
{

    fbe_edal_status_t   status = FBE_EDAL_STATUS_OK;
    
     fbe_encl_component_general_status_info_t    *genStatusDataPtr = NULL;
    
     genStatusDataPtr = (fbe_encl_component_general_status_info_t *)generalDataPtr;
     genStatusDataPtr->componentSwapCount++;
    //Set the  state change bit 
    genStatusDataPtr->readWriteFlags.componentStateChange = 1;
    
    return status;                       
}
