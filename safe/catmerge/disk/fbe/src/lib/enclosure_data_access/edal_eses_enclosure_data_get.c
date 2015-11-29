/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_eses_enclosure_data_get.h
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to retrieve data from the ESES
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

#include "edal_eses_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getEnclosureBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
*       This function will set data in an ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data
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
 *    17-Nov-2008: PHE - Created
 *    24-Nov-2008: Arun S - Added cases for Status pages
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getEnclosureBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr)

{
    fbe_eses_enclosure_info_t    *enclDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_eses_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_TRACE_PRESENT:
        *returnDataPtr = enclDataPtr->esesStatusFlags.tracePresent;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_PARTIAL_SHUTDOWN:
        *returnDataPtr = enclDataPtr->esesStatusFlags.partialShutDown;
        status = FBE_EDAL_STATUS_OK;
        break;
    
    case FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED:
        *returnDataPtr = enclDataPtr->esesStatusFlags.addl_status_unsupported;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED:
        *returnDataPtr = enclDataPtr->esesStatusFlags.emc_specific_unsupported;
        status = FBE_EDAL_STATUS_OK;
        break; 

    case FBE_ENCL_MODE_SENSE_UNSUPPORTED:
        *returnDataPtr = enclDataPtr->esesStatusFlags.mode_sense_unsupported;
        status = FBE_EDAL_STATUS_OK;
        break; 

    case FBE_ENCL_MODE_SELECT_UNSUPPORTED:
        *returnDataPtr = enclDataPtr->esesStatusFlags.mode_select_unsupported;
        status = FBE_EDAL_STATUS_OK;
        break; 
   
    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_getBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_ENCLOSURE,
                                              returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getEnclosureBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getLccBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
*       This function will set data in an ESES enclosure object fields
 *      of an LCC component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data
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
 *    09-Jan-2013: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr)

{
    fbe_eses_lcc_info_t    *lccDataPtr;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    lccDataPtr = (fbe_eses_lcc_info_t *)generalDataPtr;
    if (lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_LCC_ECB_FAULT:
        *returnDataPtr = lccDataPtr->ecbFault;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_getBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_LCC,
                                              returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getLccBool

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getExpPhyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object Expander
 *      PHY fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to component data
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
eses_enclosure_access_getExpPhyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr)

{
    fbe_eses_expander_phy_info_t    *expPhyDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    expPhyDataPtr = (fbe_eses_expander_phy_info_t *)generalDataPtr;
    if (expPhyDataPtr->sasExpPhyInfo.generalExpanderPhyInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            expPhyDataPtr->sasExpPhyInfo.generalExpanderPhyInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
   case FBE_ENCL_EXP_PHY_DISABLED:
        *returnDataPtr = expPhyDataPtr->phyFlags.phyDisabled;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_READY:
        *returnDataPtr = expPhyDataPtr->phyFlags.phyReady;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_LINK_READY:
        *returnDataPtr = expPhyDataPtr->phyFlags.phyLinkReady;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_FORCE_DISABLED:
        *returnDataPtr = expPhyDataPtr->phyFlags.phyForceDisabled;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_CARRIER_DETECTED:
        *returnDataPtr = expPhyDataPtr->phyFlags.phyCarrierDetected;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_SPINUP_ENABLED:
        *returnDataPtr = expPhyDataPtr->phyFlags.phySpinupEnabled;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD:
        *returnDataPtr = expPhyDataPtr->phyFlags.phySataSpinupHold;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_DISABLE:
        *returnDataPtr = expPhyDataPtr->phyFlags.disable;
        status = FBE_EDAL_STATUS_OK;
        break;
    
    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_getBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_EXPANDER_PHY,
                                              returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getExpPhyBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getConnectorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object Connector
 *      fields that are Boolean type.
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
eses_enclosure_access_getConnectorBool(void   *generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_bool_t *returnDataPtr)

{
    fbe_eses_encl_connector_info_t   *connectorDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    connectorDataPtr = (fbe_eses_encl_connector_info_t *)generalDataPtr;
    if (connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_CONNECTOR_INSERT_MASKED:
        *returnDataPtr = connectorDataPtr->connectorFlags.connectorInsertMasked;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_CONNECTOR_DISABLED:
        *returnDataPtr = connectorDataPtr->connectorFlags.connectorDisabled;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_CONNECTOR_PRIMARY_PORT:
        *returnDataPtr = connectorDataPtr->connectorFlags.primaryPort;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR:
        *returnDataPtr = connectorDataPtr->connectorFlags.isEntireConnector;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_ILLEGAL_CABLE:
        *returnDataPtr = connectorDataPtr->connectorFlags.illegalCable;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_DEGRADED:
        *returnDataPtr = connectorDataPtr->connectorFlags.connectorDegraded;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case  FBE_ENCL_COMP_CLEAR_SWAP:
        *returnDataPtr = connectorDataPtr->connectorFlags.clearSwap;
        status = FBE_EDAL_STATUS_OK;
        break    ;
    case  FBE_ENCL_CONNECTOR_DEV_LOGGED_IN:
        *returnDataPtr = connectorDataPtr->connectorFlags.deviceLoggedIn;
        status = FBE_EDAL_STATUS_OK;
        break    ;
    case FBE_ENCL_COMP_IS_LOCAL:
        *returnDataPtr = connectorDataPtr->connectorFlags.isLocalConnector;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_getBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_CONNECTOR,
                                              returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getConnectorBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
eses_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t *returnDataPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (component)
    {
    case FBE_ENCL_EXPANDER_PHY:
        status = eses_enclosure_access_getExpPhyBool(generalDataPtr,
                                                     attribute,
                                                     returnDataPtr);
        break;
    case FBE_ENCL_CONNECTOR:
        status = eses_enclosure_access_getConnectorBool(generalDataPtr,
                                                        attribute,
                                                        returnDataPtr);
        break;
    case FBE_ENCL_ENCLOSURE:
        status = eses_enclosure_access_getEnclosureBool(generalDataPtr,
                                                        attribute,
                                                        returnDataPtr);
        break;

    case FBE_ENCL_LCC:
        status = eses_enclosure_access_getLccBool(generalDataPtr,
                                                  attribute,
                                                  returnDataPtr);
        break;

    // nothing ESES specific go to super class
    default:
        status = sas_enclosure_access_getBool(generalDataPtr,
                                              attribute,
                                              component,
                                              returnDataPtr);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_getBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getDriveU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Drive component.
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
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr)

{
    fbe_eses_encl_drive_info_t       *driveDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    driveDataPtr = (fbe_eses_encl_drive_info_t *)generalDataPtr;
    if (driveDataPtr->enclDriveSlotInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            driveDataPtr->enclDriveSlotInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_DRIVE_PHY_INDEX:
        *returnDataPtr = driveDataPtr->drivePhyIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = driveDataPtr->driveElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_POWER_DOWN_COUNT:
        *returnDataPtr = driveDataPtr->drivePowerDownCount;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_DRIVE_SLOT,
                                              returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getDriveU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getPowerSupplyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Power Supply component.
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
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t *returnDataPtr)

{
    fbe_eses_power_supply_info_t   *psInfoPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psInfoPtr = (fbe_eses_power_supply_info_t *)generalDataPtr;
    if (psInfoPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psInfoPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        *returnDataPtr = psInfoPtr->psSubEnclId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = psInfoPtr->psElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescBufferId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescBufferType;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescBufferIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescElogBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;      
    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferDescActBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        *returnDataPtr = psInfoPtr->psContainerIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_POWER_SUPPLY,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getPowerSupplyU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getCoolingU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Cooling component.
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
 *
 *  HISTORY:
 *    26-Sep-2008: hgd - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t *returnDataPtr)

{
    fbe_eses_cooling_info_t     *coolingInfoPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    coolingInfoPtr = (fbe_eses_cooling_info_t *)generalDataPtr;
    if (coolingInfoPtr->generalCoolingInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            coolingInfoPtr->generalCoolingInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {    
    case FBE_ENCL_BD_BUFFER_ID:
        *returnDataPtr = coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        *returnDataPtr = coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        *returnDataPtr = coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferType;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        *returnDataPtr = coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        *returnDataPtr = coolingInfoPtr->coolingSubEnclId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = coolingInfoPtr->coolingElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;    
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        *returnDataPtr = coolingInfoPtr->coolingContainerIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_COOLING_COMPONENT,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getCoolingU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getTempSensorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Temp Sensor component.
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
 *
 *  HISTORY:
 *    26-Sep-2008: hgd - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t *returnDataPtr)

{
    fbe_eses_temp_sensor_info_t    *tempSensorInfoPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    tempSensorInfoPtr = (fbe_eses_temp_sensor_info_t *)generalDataPtr;
    if (tempSensorInfoPtr->generalTempSensorInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            tempSensorInfoPtr->generalTempSensorInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {   
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = tempSensorInfoPtr->tempSensorElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        *returnDataPtr = tempSensorInfoPtr->tempSensorContainerIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE:
        *returnDataPtr = tempSensorInfoPtr->tempSensorMaxTemperature;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getTempSensorU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getExpPhyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Expander PHY component.
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
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getExpPhyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t *returnDataPtr)

{
    fbe_eses_expander_phy_info_t    *expPhyDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    expPhyDataPtr = (fbe_eses_expander_phy_info_t *)generalDataPtr;
    if (expPhyDataPtr->sasExpPhyInfo.generalExpanderPhyInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            expPhyDataPtr->sasExpPhyInfo.generalExpanderPhyInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = expPhyDataPtr->phyElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;        
    case FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX:
        *returnDataPtr = expPhyDataPtr->expanderElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_ID:
        *returnDataPtr = expPhyDataPtr->phyIdentifier;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_DISABLE_REASON:
        *returnDataPtr = expPhyDataPtr->phyDisableReason;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_EXPANDER_PHY,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getExpPhyU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getConnectorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Connector component.
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
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t *returnDataPtr)

{
    fbe_eses_encl_connector_info_t   *connectorDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    connectorDataPtr = (fbe_eses_encl_connector_info_t *)generalDataPtr;
    if (connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = connectorDataPtr->connectorElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_TYPE:
        *returnDataPtr = connectorDataPtr->connectedComponentType;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_CONNECTOR_PHY_INDEX:
        *returnDataPtr = connectorDataPtr->connectorPhyIndex;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        *returnDataPtr = connectorDataPtr->connectorContainerIndex;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_ID:
        *returnDataPtr = connectorDataPtr->ConnectorId;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_TYPE:
        *returnDataPtr = connectorDataPtr->connectorType;
        status = FBE_EDAL_STATUS_OK;
        break;     
    case FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID:
        *returnDataPtr = connectorDataPtr->attachedSubEncId;
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_COMP_LOCATION:
        *returnDataPtr = connectorDataPtr->connectorLocation;
        status = FBE_EDAL_STATUS_OK;
        break;   
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_CONNECTOR,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getConnectorU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getExpanderU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Expander component.
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
 *
 *  HISTORY:
 *    07-Set-2008: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getExpanderU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr)

{
    fbe_eses_encl_expander_info_t   *expanderDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    expanderDataPtr = (fbe_eses_encl_expander_info_t *)generalDataPtr;
    if (expanderDataPtr->sasExpanderInfo.generalExpanderInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            expanderDataPtr->sasExpanderInfo.generalExpanderInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = expanderDataPtr->expanderElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = expanderDataPtr->expanderSideId;
        status = FBE_EDAL_STATUS_OK;
        break;
 
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_EXPANDER,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getExpanderU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getEnclosureU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Enclosure component.
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
 *
 *  HISTORY:
 *    15-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr)

{
    fbe_eses_enclosure_info_t   *enclDataPtr;
    fbe_edal_status_t           status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_eses_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        *returnDataPtr = enclDataPtr->enclSubEnclId;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = enclDataPtr->enclElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferType;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescElogBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;      
    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        *returnDataPtr = enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescActBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_REQUEST:
        *returnDataPtr = enclDataPtr->esesEnclosureData.enclPowerCycleRequest;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_DURATION:
        *returnDataPtr = enclDataPtr->esesEnclosureData.enclPowerOffDuration;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_LCC_POWER_CYCLE_DELAY:
        *returnDataPtr = enclDataPtr->esesEnclosureData.enclPowerCycleDelay;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_SHUTDOWN_REASON:
        *returnDataPtr = enclDataPtr->enclShutdownReason;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_ENCLOSURE,
                                             returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getEnclosureU8


/*!*************************************************************************
 *  @fn eses_enclosure_access_getLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                   fbe_base_enclosure_attributes_t attribute,
 *                                   fbe_u8_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a LCC component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      28-Oct-2008: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr)

{
    fbe_eses_lcc_info_t   *lccDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    lccDataPtr = (fbe_eses_lcc_info_t *)generalDataPtr;
    if (lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        *returnDataPtr = lccDataPtr->lccSubEnclId;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = lccDataPtr->lccElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_ID:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferType;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_INDEX:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIndex;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescElogBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        *returnDataPtr = lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescActBufferID;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_REQUEST:
        *returnDataPtr = lccDataPtr->esesLccData.enclPowerCycleRequest;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DURATION:
        *returnDataPtr = lccDataPtr->esesLccData.enclPowerOffDuration;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DELAY:
        *returnDataPtr = lccDataPtr->esesLccData.enclPowerCycleDelay;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_LCC,
                                             returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getLccU8


/*!*************************************************************************
 *  @fn eses_enclosure_access_getDisplayU8(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                         fbe_base_enclosure_attributes_t attribute,
 *                                         fbe_u8_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Display component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      03-Nov-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getDisplayU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t *returnDataPtr)

{
    fbe_eses_display_info_t *displayDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    displayDataPtr = (fbe_eses_display_info_t *)generalDataPtr;
    if (displayDataPtr->generalDisplayInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            displayDataPtr->generalDisplayInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_DISPLAY_MODE_STATUS:
        *returnDataPtr = displayDataPtr->displayModeStatus;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_MODE:
        *returnDataPtr = displayDataPtr->displayMode;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_CHARACTER_STATUS:
        *returnDataPtr = displayDataPtr->displayCharacterStatus;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_CHARACTER:
        *returnDataPtr = displayDataPtr->displayCharacter;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = displayDataPtr->displayElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SUBTYPE:
        *returnDataPtr = displayDataPtr->displaySubtype;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_DISPLAY,
                                             returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getDisplayU8


/*!*************************************************************************
 *  @fn eses_enclosure_access_getSpsU8(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                         fbe_base_enclosure_attributes_t attribute,
 *                                         fbe_u8_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      01-Aug-2012: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getSpsU8(fbe_edal_general_comp_handle_t generalDataPtr,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_u8_t *returnDataPtr)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    spsDataPtr = (fbe_eses_sps_info_t *)generalDataPtr;
    if (spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = spsDataPtr->spsElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        *returnDataPtr = spsDataPtr->spsSideId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        *returnDataPtr = spsDataPtr->spsSubEnclId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescBufferId;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ACT_RAM_BUFFER_ID:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescActRamID;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ACT_RAM_BUFFER_ID_WRITEABLE:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescActRamIdWriteable;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescBufferType;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        *returnDataPtr = spsDataPtr->bufferDescriptorInfo.bufferDescBufferIndex;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_SPS,
                                             returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getSpsU8

/*!*************************************************************************
 *  @fn eses_enclosure_access_getSpsU16(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                         fbe_base_enclosure_attributes_t attribute,
 *                                         fbe_u16_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      01-Aug-2012: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getSpsU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_u16_t *returnDataPtr)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    spsDataPtr = (fbe_eses_sps_info_t *)generalDataPtr;
    if (spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_SPS_STATUS:
        *returnDataPtr = spsDataPtr->spsStatus;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_SPS_BATTIME:
        *returnDataPtr = spsDataPtr->spsBatTime;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU16(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_SPS,
                                              returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getSpsU16

/*!*************************************************************************
 *  @fn eses_enclosure_access_getSpsU32(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                         fbe_base_enclosure_attributes_t attribute,
 *                                         fbe_u32_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      01-Aug-2012: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getSpsU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_u32_t *returnDataPtr)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    spsDataPtr = (fbe_eses_sps_info_t *)generalDataPtr;
    if (spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_SPS_FFID:
        *returnDataPtr = spsDataPtr->spsFfid;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_SPS_BATTERY_FFID:
        *returnDataPtr = spsDataPtr->spsBatteryFfid;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU32(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_SPS,
                                              returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getSpsU32


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of the Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    16-Dec-2008: AS - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u32_t *returnDataPtr)

{
    fbe_eses_enclosure_info_t   *enclDataPtr;
    fbe_edal_status_t           status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    enclDataPtr = (fbe_eses_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_GENERATION_CODE:
            *returnDataPtr = enclDataPtr->gen_code;
            status = FBE_EDAL_STATUS_OK;
            break;
        
        default:
            // check if attribute defined in base enclosure
            status = sas_enclosure_access_getU32(generalDataPtr,
                                                 attribute,
                                                 FBE_ENCL_ENCLOSURE,
                                                 returnDataPtr);
            break;
    }

    return status;

}   // end of eses_enclosure_access_getEnclosureU32

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getDriveU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Drive component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t *returnDataPtr)

{
    fbe_eses_encl_drive_info_t       *driveDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    driveDataPtr = (fbe_eses_encl_drive_info_t *)generalDataPtr;
    if (driveDataPtr->enclDriveSlotInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            driveDataPtr->enclDriveSlotInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    //    case FBE_ENCL_DRIVE_ESES_SAS_ADDRESS:
    //        *returnDataPtr = driveDataPtr->eses_sas_address;
    //        status = FBE_EDAL_STATUS_OK;
    //        break;
            /* TO DO: move to discovery edge
        case FBE_ENCL_DRIVE_SAS_ADDRESS:
            *returnDataPtr = driveDataPtr->driveSasAddress;
            status = FBE_EDAL_STATUS_OK;
            break;
            */
        default:
            // check if attribute defined in base enclosure
            status = sas_enclosure_access_getU64(generalDataPtr,
                                                 attribute,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 returnDataPtr);
            break;
    }

    return status;

}   // end of eses_enclosure_access_getDriveU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr)

{
    fbe_eses_enclosure_info_t   *enclDataPtr;
    fbe_edal_status_t           status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_eses_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE:
            *returnDataPtr = enclDataPtr->enclTimeOfLastGoodStatusPage;
            status = FBE_EDAL_STATUS_OK;
            break;

        default:
             // check if attribute defined in super enclosure
            status = sas_enclosure_access_getU64(generalDataPtr,
                                                 attribute,
                                                 FBE_ENCL_ENCLOSURE,
                                                 returnDataPtr);
            break;
    }

    return status;

}   // end of eses_enclosure_access_getEnclosureU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getConnectorU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Connector component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getConnectorU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr)

{
    fbe_eses_encl_connector_info_t   *connectorDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    connectorDataPtr = (fbe_eses_encl_connector_info_t *)generalDataPtr;
    if (connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            connectorDataPtr->baseConnectorInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS:
            *returnDataPtr = connectorDataPtr->expanderSasAddress;
            status = FBE_EDAL_STATUS_OK;
            break;
        case FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS:
            *returnDataPtr = connectorDataPtr->attachedSasAddress;
            status = FBE_EDAL_STATUS_OK;
            break;
        default:
            // check if attribute defined in super enclosure
            status = sas_enclosure_access_getU64(generalDataPtr,
                                                 attribute,
                                                 FBE_ENCL_CONNECTOR,
                                                 returnDataPtr);
            break;
    }

    return status;

}   // end of eses_enclosure_access_getConnectorU64

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getExpanderU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Expander component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U64 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    07-Sep-2008: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getExpanderU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t *returnDataPtr)

{
    fbe_eses_encl_expander_info_t   *expanderDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    expanderDataPtr = (fbe_eses_encl_expander_info_t *)generalDataPtr;
    if (expanderDataPtr->sasExpanderInfo.generalExpanderInfo.generalStatus.componentCanary 
        != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            expanderDataPtr->sasExpanderInfo.generalExpanderInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_EXP_SAS_ADDRESS:
            *returnDataPtr = expanderDataPtr->expanderSasAddress;
            status = FBE_EDAL_STATUS_OK;
            break;
      
        default:
            // check if attribute defined in super enclosure
            status = sas_enclosure_access_getU64(generalDataPtr,
                                                 attribute,
                                                 FBE_ENCL_CONNECTOR,
                                                 returnDataPtr);
            break;
    }

    return status;

}   // end of eses_enclosure_access_getExpanderU64

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
 *      @param returnDataPtr - pointer to the U8 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u8_t *returnDataPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (component)
    {
        case FBE_ENCL_POWER_SUPPLY:
            status = eses_enclosure_access_getPowerSupplyU8(generalDataPtr,
                                                            attribute,
                                                            returnDataPtr);
            break;
        case FBE_ENCL_COOLING_COMPONENT:
            status = eses_enclosure_access_getCoolingU8(generalDataPtr,
                                                            attribute,
                                                            returnDataPtr);
            break;
        case FBE_ENCL_TEMP_SENSOR:
            status = eses_enclosure_access_getTempSensorU8(generalDataPtr,
                                                            attribute,
                                                            returnDataPtr);
            break;
        case FBE_ENCL_DRIVE_SLOT:
            status = eses_enclosure_access_getDriveU8(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
            break;
        case FBE_ENCL_EXPANDER_PHY:
            status = eses_enclosure_access_getExpPhyU8(generalDataPtr,
                                                       attribute,
                                                       returnDataPtr);
            break;
        case FBE_ENCL_CONNECTOR:
            status = eses_enclosure_access_getConnectorU8(generalDataPtr,
                                                          attribute,
                                                          returnDataPtr);
            break;
        case FBE_ENCL_EXPANDER:
            status = eses_enclosure_access_getExpanderU8(generalDataPtr,
                                                          attribute,
                                                          returnDataPtr);
            break;
        case FBE_ENCL_ENCLOSURE:
            status = eses_enclosure_access_getEnclosureU8(generalDataPtr,
                                                          attribute,
                                                          returnDataPtr);
            break;
        case FBE_ENCL_LCC:
            status = eses_enclosure_access_getLccU8(generalDataPtr,
                                                    attribute,
                                                    returnDataPtr);
            break;
        case FBE_ENCL_DISPLAY:
            status = eses_enclosure_access_getDisplayU8(generalDataPtr,
                                                        attribute,
                                                        returnDataPtr);
            break;
        case FBE_ENCL_SPS:
            status = eses_enclosure_access_getSpsU8(generalDataPtr,
                                                    attribute,
                                                    returnDataPtr);
        break;
        case FBE_ENCL_SSC:
            status = eses_enclosure_access_getSscU8(generalDataPtr,
                                                    attribute,
                                                    returnDataPtr);
        break;
        default:
            // check if component defined in base enclosure
            status = sas_enclosure_access_getU8(generalDataPtr,
                                                attribute,
                                                component,
                                                returnDataPtr);

            break;             
    }

    return status;

}   // end of eses_enclosure_access_getU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
 *      @param returnDataPtr - pointer to the boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u64_t *returnDataPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (component)
    {
        case FBE_ENCL_DRIVE_SLOT:
            status = eses_enclosure_access_getDriveU64(generalDataPtr,
                                                       attribute,
                                                       returnDataPtr);
            break;
        case FBE_ENCL_ENCLOSURE:
            status = eses_enclosure_access_getEnclosureU64(generalDataPtr,
                                                           attribute,
                                                           returnDataPtr);
            break;
        case FBE_ENCL_CONNECTOR:
            status = eses_enclosure_access_getConnectorU64(generalDataPtr,
                                                           attribute,
                                                           returnDataPtr);
            break;
        case FBE_ENCL_EXPANDER:
            status = eses_enclosure_access_getExpanderU64(generalDataPtr,
                                                           attribute,
                                                           returnDataPtr);
            break;
        case FBE_ENCL_POWER_SUPPLY:
        default:
            // check super class handler
            status = sas_enclosure_access_getU64(generalDataPtr,
                                                 attribute,
                                                 component,
                                                 returnDataPtr);
            break;              
    }

    return status;

}   // end of eses_enclosure_access_getU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
 *      @param returnDataPtr - pointer to the U32 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u32_t *returnDataPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (component)
    {
        case FBE_ENCL_ENCLOSURE:
            status = eses_enclosure_access_getEnclosureU32(generalDataPtr,
                                                          attribute,
                                                          returnDataPtr);
            break;
        case FBE_ENCL_SPS:
            status = eses_enclosure_access_getSpsU32(generalDataPtr,
                                                     attribute,
                                                     returnDataPtr);
            break;
        case FBE_ENCL_POWER_SUPPLY:
            status = eses_enclosure_access_getPowerSupplyU32(generalDataPtr,
                                                     attribute,
                                                     returnDataPtr);
            break;
        default:
            // check super class handler
            status = sas_enclosure_access_getU32(generalDataPtr,
                                                 attribute,
                                                 component,
                                                 returnDataPtr);
            break;              
    }

    return status;

}   // end of eses_enclosure_access_getU32


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
 *      @param returnDataPtr - pointer to the U16 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u16_t *returnDataPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (component)
    {
        case FBE_ENCL_SPS:
            status = eses_enclosure_access_getSpsU16(generalDataPtr,
                                                     attribute,
                                                     returnDataPtr);
            break;
        default:
            // check super class handler
            status = sas_enclosure_access_getU16(generalDataPtr,
                                                 attribute,
                                                 component,
                                                 returnDataPtr);
            break;              
    }

    return status;

}   // end of eses_enclosure_access_getU16


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getEnclStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will get copy of string from enclosure component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware information to reference
 *  @param newString - takes the address of the string that is returned
 *  @param length - the string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    09-Dec-2008: NC - Created
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_getEnclStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString)
{
    fbe_eses_enclosure_info_t   *enclDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    if (newString == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null newString\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    enclDataPtr = (fbe_eses_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_SERIAL_NUMBER:    
            if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
            {
                length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
            }
            fbe_copy_memory(newString, 
                      enclDataPtr->enclSerialNumber, 
                      length);
            status = FBE_EDAL_STATUS_OK;
            break;
            
        default:
            /* We may need to call into sas_enclosure_access_getStr() if it's supported */
            enclosure_access_log_error("EDAL:%s, Component Encl has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;
} // eses_enclosure_access_getEnclStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getLccStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will get copy of string from enclosure component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware information to reference
 *  @param newString - takes the address of the string that is returned
 *  @param length - the string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    28-July-2009: Prasenjeet - Created
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_getLccStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString)

{
    fbe_eses_lcc_info_t   *lccDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    if (newString == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null newString\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    lccDataPtr = (fbe_eses_lcc_info_t *)generalDataPtr;
    if (lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_SUBENCL_SERIAL_NUMBER:
            if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
            {
                length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
            }
            fbe_copy_memory(newString, 
                      lccDataPtr->lccSerialNumber, 
                      length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_SUBENCL_PRODUCT_ID:
            if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
            {
                length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
            }
            fbe_copy_memory(newString, 
                      lccDataPtr->lccProductId, 
                      length);
            status = FBE_EDAL_STATUS_OK;
            break;

        // This really belongs to base_enclosure level.
        case FBE_ENCL_COMP_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                         &lccDataPtr->fwInfo, 
                         length);
            break;
  
        case FBE_LCC_EXP_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                         &lccDataPtr->fwInfoExp, 
                         length);
            break;
  
        case FBE_LCC_BOOT_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                         &lccDataPtr->fwInfoBoot, 
                         length);
            break;
        case FBE_LCC_INIT_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                         &lccDataPtr->fwInfoInit, 
                         length);
            break;
        case FBE_LCC_FPGA_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                         &lccDataPtr->fwInfoFPGA, 
                         length);
            break;

        default:
            /* We may need to call into sas_enclosure_access_getStr() if it's supported */
            enclosure_access_log_error("EDAL:%s, Component LCC has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;
}// eses_enclosure_access_getLccStr


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getPSStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will get copy of string from enclosure component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware information to reference
 *  @param newString - takes the address of the string that is returned
 *  @param length - the string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    28-July-2009: Prasenjeet - Created
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_getPSStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString)

{
    fbe_eses_power_supply_info_t *PSDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    if (newString == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null newString\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    PSDataPtr = (fbe_eses_power_supply_info_t *)generalDataPtr;
    if ( PSDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
          EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            PSDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_SUBENCL_SERIAL_NUMBER:
            if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
            {
                length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
            }
            fbe_copy_memory(newString, 
                    PSDataPtr->PSSerialNumber, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_SUBENCL_PRODUCT_ID:
            if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
            {
                length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
            }
            fbe_copy_memory(newString, 
                    PSDataPtr->psProductId, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_COMP_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                    &PSDataPtr->fwInfo, 
                    length);
            break;

        default:
            /* We may need to call into sas_enclosure_access_getStr() if it's supported */
            enclosure_access_log_error("EDAL:%s, Component PS has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;
}// eses_enclosure_access_getPSStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getCoolingStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will get copy of string from cooling component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware information to reference
 *  @param newString - takes the address of the string that is returned
 *  @param length - the string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    30-Apr-2010: NCHIU - Created
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_getCoolingStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString)

{
    fbe_eses_cooling_info_t *coolingDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    if (newString == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null newString\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    coolingDataPtr = (fbe_eses_cooling_info_t *)generalDataPtr;
    if ( coolingDataPtr->generalCoolingInfo.generalComponentInfo.generalStatus.componentCanary != 
          EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
             coolingDataPtr->generalCoolingInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_SUBENCL_SERIAL_NUMBER:
            if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
            {
                length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
            }
            fbe_copy_memory(newString, 
                    coolingDataPtr->coolingSerialNumber, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_SUBENCL_PRODUCT_ID:
            if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
            {
                length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
            }
            fbe_copy_memory(newString, 
                    coolingDataPtr->coolingProductId, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_COMP_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                    &coolingDataPtr->fwInfo, 
                    length);
            break;

        default:
            /* We may need to call into sas_enclosure_access_getStr() if it's supported */
            enclosure_access_log_error("EDAL:%s, Component cooling has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;
}// eses_enclosure_access_getCoolingStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getSpsStr
 **************************************************************************
 *  @brief
 *  This function will get copy of string from SPS component.
 *
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware information to reference
 *  @param newString - takes the address of the string that is returned
 *  @param length - the string length to copy
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  @version
 *    21-Sep-2012: PHE - Created
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_getSpsStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString)

{
    fbe_eses_sps_info_t *spsDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    if (newString == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null newString\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    spsDataPtr = (fbe_eses_sps_info_t *)generalDataPtr;
    if ( spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary != 
          EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
             spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
        case FBE_ENCL_SUBENCL_SERIAL_NUMBER:
            if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
            {
                length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
            }
            fbe_copy_memory(newString, 
                    spsDataPtr->spsSerialNumber, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_SUBENCL_PRODUCT_ID:
            if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
            {
                length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
            }
            fbe_copy_memory(newString, 
                    spsDataPtr->spsProductId, 
                    length);
            status = FBE_EDAL_STATUS_OK;
            break;

        case FBE_ENCL_COMP_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                    &spsDataPtr->primaryFwInfo, 
                    length);
            break;

        case FBE_SPS_SECONDARY_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                    &spsDataPtr->secondaryFwInfo, 
                    length);
            break;

        case FBE_SPS_BATTERY_FW_INFO:
            if (length > sizeof(fbe_edal_fw_info_t)) 
            {
                length = sizeof(fbe_edal_fw_info_t);
                status = FBE_EDAL_STATUS_SIZE_MISMATCH;
            }
            else
            {
                status = FBE_EDAL_STATUS_OK;
            }
            fbe_copy_memory(newString, 
                    &spsDataPtr->batteryFwInfo, 
                    length);
            break;

        default:
            /* We may need to call into sas_enclosure_access_getStr() if it's supported */
            enclosure_access_log_error("EDAL:%s, Component SPS has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;
}// eses_enclosure_access_getSpsStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_getStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will get data from the eses enclosure object.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to eses object data
 *  @param attribute - specifies the target atttribute to reference
 *  @param enclosure_component - specifies the target enclosure component
 *  @param length - string length (bytes) to copy
 *  @param returnStringPtr - where the return string is placed
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    13-Oct-2008: GB - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t enclosure_component,
                             fbe_u32_t length,
                             char *returnStringPtr)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if ((generalDataPtr == NULL) || (returnStringPtr == NULL))
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (enclosure_component)
    {
        case FBE_ENCL_ENCLOSURE:
            status = eses_enclosure_access_getEnclStr(generalDataPtr,
                                                    attribute,
                                                    length,
                                                    returnStringPtr);
            break;
        case FBE_ENCL_POWER_SUPPLY:
            status = eses_enclosure_access_getPSStr(generalDataPtr,
                                                    attribute,
                                                    length,
                                                    returnStringPtr);

            break;
        case FBE_ENCL_COOLING_COMPONENT:
            status = eses_enclosure_access_getCoolingStr(generalDataPtr,
                                                    attribute,
                                                    length,
                                                    returnStringPtr);
            break;
        case FBE_ENCL_LCC:
            status = eses_enclosure_access_getLccStr(generalDataPtr,
                                                    attribute,
                                                    length,
                                                    returnStringPtr);
            break;
        case FBE_ENCL_SPS:
            status = eses_enclosure_access_getSpsStr(generalDataPtr,
                                                    attribute,
                                                    length,
                                                    returnStringPtr);
            break;
        default:
            // there are no other strings
            enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
                __FUNCTION__, 
                enclosure_access_printComponentType(enclosure_component),
                enclosure_access_printAttribute(attribute));
            status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
            break;
    }

    return status;

}   // end of eses_enclosure_access_getStr


/*!*************************************************************************
 *                         @fn eses_enclosure_access_getPowerSupplyU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a Power Supply component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param returnDataPtr - pointer to the U32 return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Dec-2013: PHE  - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getPowerSupplyU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u32_t *returnDataPtr)

{
    fbe_eses_power_supply_info_t   *psInfoPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psInfoPtr = (fbe_eses_power_supply_info_t *)generalDataPtr;
    if (psInfoPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psInfoPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_BD_BUFFER_SIZE:
        *returnDataPtr = psInfoPtr->bufferDescriptorInfo.bufferSize;
        status = FBE_EDAL_STATUS_OK;
        break;
    
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_getU32(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_POWER_SUPPLY,
                                            returnDataPtr);
        break;
    }

    return status;

}   // end of eses_enclosure_access_getPowerSupplyU32


/*!*************************************************************************
 *  @fn eses_enclosure_access_getSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
 *                                   fbe_base_enclosure_attributes_t attribute,
 *                                   fbe_u8_t *returnDataPtr)
 **************************************************************************
 *  @brief
 *      This function will get data from ESES enclosure object fields
 *      of a ESC Electronics component.
 *
 *  @param generalDataPtr - pointer to a component data
 *  @param attribute - enum for attribute to get
 *  @param returnDataPtr - pointer to the U8 return value
 *
 *  @return 
 *      fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *      14-Jan-2014: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_getSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr)

{
    fbe_eses_encl_ssc_info_t   *sscDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    sscDataPtr = (fbe_eses_encl_ssc_info_t *)generalDataPtr;
    if (sscDataPtr->baseSscInfo.generalComponentInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            sscDataPtr->baseSscInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_COMP_ELEM_INDEX:
        *returnDataPtr = sscDataPtr->sscElementIndex;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_SSC,
                                             returnDataPtr);
        break;
    }

    return status;

}// End of function eses_enclosure_access_getSscU8
