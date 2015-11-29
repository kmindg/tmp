/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_eses_enclosure_data_set.h
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to update data from the ESES
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
 *                         @fn eses_enclosure_access_setEnclosureBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new fbe_bool_t value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Nov-2008: PHE - Created
 *    24-Nov-2008: Arun S - Added cases for Status pages
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setEnclosureBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t newValue)

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
    case FBE_ENCL_TRACE_PRESENT:    
        if (enclDataPtr->esesStatusFlags.tracePresent != newValue)
        {
            enclDataPtr->esesStatusFlags.tracePresent = newValue;
            
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    case FBE_ENCL_PARTIAL_SHUTDOWN:    
        if (enclDataPtr->esesStatusFlags.partialShutDown != newValue)
        {
            enclDataPtr->esesStatusFlags.partialShutDown = newValue;
            
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    case FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED:
        enclDataPtr->esesStatusFlags.addl_status_unsupported = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED:
        enclDataPtr->esesStatusFlags.emc_specific_unsupported = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;  

    case FBE_ENCL_MODE_SENSE_UNSUPPORTED:
        enclDataPtr->esesStatusFlags.mode_sense_unsupported = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 

    case FBE_ENCL_MODE_SELECT_UNSUPPORTED:
        enclDataPtr->esesStatusFlags.mode_select_unsupported = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 

    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_setBool(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_ENCLOSURE,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setEnclosureBool

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setLccBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an LCC component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new fbe_bool_t value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2013: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t newValue)

{
    fbe_eses_lcc_info_t        *lccDataPtr;
    fbe_edal_status_t           status = FBE_EDAL_STATUS_ERROR;

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
        if (lccDataPtr->ecbFault != newValue)
        {
            lccDataPtr->ecbFault = newValue;
            
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_LCC,
                                                   TRUE);
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_setBool(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_LCC,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setLccBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setExpPhyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object Expander
 *      PHY fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Expander PHY component
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
eses_enclosure_access_setExpPhyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t newValue)

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
        if (expPhyDataPtr->phyFlags.phyDisabled != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);

            expPhyDataPtr->phyFlags.phyDisabled = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_READY:
        if (expPhyDataPtr->phyFlags.phyReady != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phyReady = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_LINK_READY:
        if (expPhyDataPtr->phyFlags.phyLinkReady != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phyLinkReady = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_FORCE_DISABLED:
        if (expPhyDataPtr->phyFlags.phyForceDisabled != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phyForceDisabled = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_CARRIER_DETECTED:
        if (expPhyDataPtr->phyFlags.phyCarrierDetected != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phyCarrierDetected = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_SPINUP_ENABLED:
        if (expPhyDataPtr->phyFlags.phySpinupEnabled != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phySpinupEnabled = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD:
        if (expPhyDataPtr->phyFlags.phySataSpinupHold != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);
            expPhyDataPtr->phyFlags.phySataSpinupHold = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_EXP_PHY_DISABLE:
        /* we only need to issue disable if the phy is not disabled */
        if ((newValue != expPhyDataPtr->phyFlags.disable) || 
            (newValue != expPhyDataPtr->phyFlags.phyDisabled))
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_EXPANDER_PHY,
                                                   TRUE);

            expPhyDataPtr->phyFlags.disable = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_EXPANDER_PHY,
                                              newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setExpPhyBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setConnectorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object Connector
 *      fields that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Connector component
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
eses_enclosure_access_setConnectorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_bool_t newValue)

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
        connectorDataPtr->connectorFlags.connectorInsertMasked = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_CONNECTOR_DISABLED:
        if (connectorDataPtr->connectorFlags.connectorDisabled != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_CONNECTOR,
                                                   TRUE);
            
            connectorDataPtr->connectorFlags.connectorDisabled = newValue;
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_CONNECTOR_DEGRADED:
        
        if (connectorDataPtr->connectorFlags.connectorDegraded != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_CONNECTOR,
                                                   TRUE);
            
            connectorDataPtr->connectorFlags.connectorDegraded = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_CONNECTOR_PRIMARY_PORT:
        connectorDataPtr->connectorFlags.primaryPort = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR:
        connectorDataPtr->connectorFlags.isEntireConnector = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_CONNECTOR_ILLEGAL_CABLE:
        if (connectorDataPtr->connectorFlags.illegalCable  != newValue)
        {
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        connectorDataPtr->connectorFlags.illegalCable  = newValue;
        break; 
    case FBE_ENCL_COMP_IS_LOCAL:
        connectorDataPtr->connectorFlags.isLocalConnector = newValue;
        status = FBE_EDAL_STATUS_OK;
        break ;
   case FBE_ENCL_COMP_CLEAR_SWAP:
        if ((connectorDataPtr->connectorFlags.clearSwap != newValue) && (newValue == TRUE))
        {
            status = base_enclosure_Increment_ComponentSwapCount(generalDataPtr);
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        connectorDataPtr->connectorFlags.clearSwap = newValue;
        break; 
    case FBE_ENCL_CONNECTOR_DEV_LOGGED_IN:
        connectorDataPtr->connectorFlags.deviceLoggedIn = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in super enclosure
        status = sas_enclosure_access_setBool(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_CONNECTOR,
                                              newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setConnectorBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - enum for the component type
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
eses_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t newValue)
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
        // check Expander PHY specific fields
        status = eses_enclosure_access_setExpPhyBool(generalDataPtr,
                                                    attribute,
                                                    newValue);
        break;
    case FBE_ENCL_CONNECTOR:
        // check Connector specific fields
        status = eses_enclosure_access_setConnectorBool(generalDataPtr,
                                                       attribute,
                                                       newValue);
        break;
    case FBE_ENCL_ENCLOSURE:
        // check Enclosure specific fields
        status = eses_enclosure_access_setEnclosureBool(generalDataPtr,
                                                       attribute,
                                                       newValue);
        break;
    case FBE_ENCL_LCC:
        // check Enclosure specific fields
        status = eses_enclosure_access_setLccBool(generalDataPtr,
                                                  attribute,
                                                  newValue);
        break;
    default:
        // use super class handler
        status = sas_enclosure_access_setBool(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_setBool


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setDriveU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will 
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Drive component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new boolean value
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
eses_enclosure_access_setDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue)

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
        if (driveDataPtr->drivePhyIndex != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_DRIVE_SLOT,
                                                   TRUE);
            driveDataPtr->drivePhyIndex = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        driveDataPtr->driveElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_POWER_DOWN_COUNT:
        driveDataPtr->drivePowerDownCount = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_DRIVE_SLOT,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setDriveU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setPowerSupplyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Power Supply component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Power Supply component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
eses_enclosure_access_setPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t newValue)

{
    fbe_eses_power_supply_info_t     *psDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psDataPtr = (fbe_eses_power_supply_info_t *)generalDataPtr;
    if (psDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }


    switch (attribute)
    {
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        psDataPtr->psSubEnclId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_COMP_ELEM_INDEX:
        psDataPtr->psElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        psDataPtr->bufferDescriptorInfo.bufferDescBufferId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        psDataPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        psDataPtr->bufferDescriptorInfo.bufferDescBufferType = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        psDataPtr->bufferDescriptorInfo.bufferDescBufferIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        psDataPtr->bufferDescriptorInfo.bufferDescElogBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        psDataPtr->bufferDescriptorInfo.bufferDescActBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        psDataPtr->psContainerIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_POWER_SUPPLY,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setPowerSupplyU8


/*!******************************************************************************
 *                         @fn eses_enclosure_access_setCoolingU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will attributes at the ESES level, for cooling components.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Cooling component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    17-Sep-2008: hgd - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)

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
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        coolingInfoPtr->coolingSubEnclId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_BD_BUFFER_ID:
        coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferType = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        coolingInfoPtr->bufferDescriptorInfo.bufferDescBufferIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        coolingInfoPtr->coolingElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        coolingInfoPtr->coolingContainerIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_COOLING_COMPONENT,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setCoolingU8


/*!******************************************************************************
 *                         @fn eses_enclosure_access_setTempSensorU8
 *******************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will attributes at the ESES level, for Temp Sensor components.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Temperature Sensor component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    26-Sep-2008: hgd - Created
 *
 ******************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)

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
        tempSensorInfoPtr ->tempSensorElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        tempSensorInfoPtr ->tempSensorContainerIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE:
        tempSensorInfoPtr ->tempSensorMaxTemperature = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setTempSensorU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setExpPhyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Expander PHY component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Expander PHY component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
eses_enclosure_access_setExpPhyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t newValue)

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
        expPhyDataPtr->phyElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX:
        expPhyDataPtr->expanderElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_ID:
        expPhyDataPtr->phyIdentifier = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_EXP_PHY_DISABLE_REASON:
        if(expPhyDataPtr->phyDisableReason != newValue)
        {
            expPhyDataPtr->phyDisableReason = newValue;
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_EXPANDER_PHY,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setExpPhyU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setConnectorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Connector component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Connector component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
eses_enclosure_access_setConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue)

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
        connectorDataPtr->connectorElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_TYPE:
        if (connectorDataPtr->connectedComponentType != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_CONNECTOR,
                                                   TRUE);
            connectorDataPtr->connectedComponentType = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_CONNECTOR_PHY_INDEX:
        connectorDataPtr->connectorPhyIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_COMP_CONTAINER_INDEX:
        connectorDataPtr->connectorContainerIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break; 
    case FBE_ENCL_CONNECTOR_ID:
        connectorDataPtr->ConnectorId= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;   
    case FBE_ENCL_CONNECTOR_TYPE:
        connectorDataPtr->connectorType= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID:
        connectorDataPtr->attachedSubEncId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;  
    case FBE_ENCL_COMP_LOCATION:
        connectorDataPtr->connectorLocation = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;   
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_CONNECTOR,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setConnectorU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setExpanderU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Expander component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Expander component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08-Sep-2008: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setExpanderU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t newValue)

{
    fbe_eses_encl_expander_info_t   *expanderDataPtr;
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
    case FBE_ENCL_COMP_ELEM_INDEX:
        expanderDataPtr->expanderElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        expanderDataPtr->expanderSideId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
 
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_EXPANDER,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setExpanderU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setEnclosureU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
eses_enclosure_access_setEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue)

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
        enclDataPtr->enclSubEnclId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_ELEM_INDEX:
        enclDataPtr->enclElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_ID:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIdWriteable= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferType = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_INDEX:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescBufferIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescElogBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        enclDataPtr->esesEnclosureData.bufferDescriptorInfo.bufferDescActBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_SHUTDOWN_REASON:
        if (enclDataPtr->enclShutdownReason != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->enclShutdownReason = newValue;
            status = FBE_EDAL_STATUS_OK_VAL_CHANGED;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_REQUEST:
        if (enclDataPtr->esesEnclosureData.enclPowerCycleRequest != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->esesEnclosureData.enclPowerCycleRequest = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DURATION:
        if (enclDataPtr->esesEnclosureData.enclPowerOffDuration != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->esesEnclosureData.enclPowerOffDuration = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DELAY:
        if (enclDataPtr->esesEnclosureData.enclPowerCycleDelay != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->esesEnclosureData.enclPowerCycleDelay = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_ENCLOSURE,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setEnclosureU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setLccU8
 **************************************************************************
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an LCC component.
 *
 *  @param   generalDataPtr - pointer to an Enclosure component
 *  @param   attribute - enum for attribute to get
 *  @param   newValue - new U8 value
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
eses_enclosure_access_setLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue)

{
    fbe_eses_lcc_info_t   *lccDataPtr = NULL;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        lccDataPtr->lccSubEnclId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_COMP_ELEM_INDEX:
        lccDataPtr->lccElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_ID:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIdWriteable= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;          
    case FBE_ENCL_BD_BUFFER_TYPE:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferType = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_BUFFER_INDEX:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescBufferIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ELOG_BUFFER_ID:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescElogBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_BD_ACT_TRC_BUFFER_ID:
        lccDataPtr->esesLccData.bufferDescriptorInfo.bufferDescActBufferID= newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_REQUEST:
        if (lccDataPtr->esesLccData.enclPowerCycleRequest != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_LCC,
                                                   TRUE);
            lccDataPtr->esesLccData.enclPowerCycleRequest = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DURATION:
        if (lccDataPtr->esesLccData.enclPowerOffDuration != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_LCC,
                                                   TRUE);
            lccDataPtr->esesLccData.enclPowerOffDuration = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_LCC_POWER_CYCLE_DELAY:
        if (lccDataPtr->esesLccData.enclPowerCycleDelay != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_LCC,
                                                   TRUE);
            lccDataPtr->esesLccData.enclPowerCycleDelay = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_LCC,
                                            newValue);
        break;
    }

    return status;

} // End of function eses_enclosure_access_setLccU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setDisplayU8
 **************************************************************************
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Display component.
 *
 *  @param   generalDataPtr - pointer to an Enclosure component
 *  @param   attribute - enum for attribute to get
 *  @param   newValue - new U8 value
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
eses_enclosure_access_setDisplayU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue)

{
    fbe_eses_display_info_t *displayDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        if (displayDataPtr->displayModeStatus != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_DISPLAY,
                                                   TRUE);
            displayDataPtr->displayModeStatus = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_MODE:
        /* This is checking against the read value */
        if (displayDataPtr->displayModeStatus != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_DISPLAY,
                                                   TRUE);
        }
        displayDataPtr->displayMode = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_CHARACTER_STATUS:
        if (displayDataPtr->displayCharacterStatus != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_DISPLAY,
                                                   TRUE);
            displayDataPtr->displayCharacterStatus = newValue;
        }
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_DISPLAY_CHARACTER:
        /* This is checking against the read value */
        if (displayDataPtr->displayCharacterStatus != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_WRITE_DATA,
                                                   FBE_ENCL_DISPLAY,
                                                   TRUE);
        }
        displayDataPtr->displayCharacter = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_ELEM_INDEX:
        displayDataPtr->displayElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SUBTYPE:
        displayDataPtr->displaySubtype = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_DISPLAY,
                                            newValue);
        break;
    }

    return status;

} // End of function eses_enclosure_access_setDisplayU8

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setSpsU8
 **************************************************************************
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param   generalDataPtr - pointer to an Enclosure component
 *  @param   attribute - enum for attribute to get
 *  @param   newValue - new U8 value
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
eses_enclosure_access_setSpsU8(fbe_edal_general_comp_handle_t generalDataPtr,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_u8_t newValue)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        spsDataPtr->spsElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SIDE_ID:
        spsDataPtr->spsSideId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_COMP_SUB_ENCL_ID:
        spsDataPtr->spsSubEnclId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID:
        spsDataPtr->bufferDescriptorInfo.bufferDescBufferId = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_ID_WRITEABLE:
        spsDataPtr->bufferDescriptorInfo.bufferDescBufferIdWriteable = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ACT_RAM_BUFFER_ID:
        spsDataPtr->bufferDescriptorInfo.bufferDescActRamID = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_ACT_RAM_BUFFER_ID_WRITEABLE:
        spsDataPtr->bufferDescriptorInfo.bufferDescActRamIdWriteable = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_TYPE:
        spsDataPtr->bufferDescriptorInfo.bufferDescBufferType = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_BD_BUFFER_INDEX:
        spsDataPtr->bufferDescriptorInfo.bufferDescBufferIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_SPS,
                                            newValue);
        break;
    }

    return status;

} // End of function eses_enclosure_access_setSpsU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setSpsU16
 **************************************************************************
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param   generalDataPtr - pointer to an Enclosure component
 *  @param   attribute - enum for attribute to get
 *  @param   newValue - new U16 value
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
eses_enclosure_access_setSpsU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_u16_t newValue)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        if (spsDataPtr->spsStatus != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_SPS,
                                                   TRUE);
            spsDataPtr->spsStatus = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_SPS_BATTIME:
        if (spsDataPtr->spsBatTime != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_SPS,
                                                   TRUE);
            spsDataPtr->spsBatTime = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU16(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_SPS,
                                             newValue);
        break;
    }

    return status;

} // End of function eses_enclosure_access_setSpsU16

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setSpsU32
 **************************************************************************
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an SPS component.
 *
 *  @param   generalDataPtr - pointer to an Enclosure component
 *  @param   attribute - enum for attribute to get
 *  @param   newValue - new U16 value
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
eses_enclosure_access_setSpsU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_u32_t newValue)

{
    fbe_eses_sps_info_t     *spsDataPtr = NULL;
    fbe_edal_status_t       status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        if (spsDataPtr->spsFfid != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_SPS,
                                                   TRUE);
            spsDataPtr->spsFfid = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_SPS_BATTERY_FFID:
        if (spsDataPtr->spsBatteryFfid != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_SPS,
                                                   TRUE);
            spsDataPtr->spsBatteryFfid = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;

    default:
        // check if attribute defined in base enclosure
        status = sas_enclosure_access_setU32(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_SPS,
                                             newValue);
        break;
    }

    return status;

} // End of function eses_enclosure_access_setSpsU32


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setDriveU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Drive component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Drive component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
eses_enclosure_access_setDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t newValue)

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
//        driveDataPtr->eses_sas_address = newValue;
//        status = FBE_EDAL_STATUS_OK;
//        break;
        /*  TO DO: Move to discovery edge
    case FBE_ENCL_DRIVE_SAS_ADDRESS:
        driveDataPtr->driveSasAddress = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
        */
    default:
        // check if attribute defined in super clase
        status = sas_enclosure_access_setU64(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_DRIVE_SLOT,
                                             newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setDriveU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
eses_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue)

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
        enclDataPtr->enclTimeOfLastGoodStatusPage = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU64(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_ENCLOSURE,
                                             newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setEnclosureU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Enclosure component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U32 value
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
eses_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t newValue)

{
    fbe_eses_enclosure_info_t   *enclDataPtr;
    fbe_edal_status_t           status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
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
        enclDataPtr->gen_code = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;  

    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU32(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_ENCLOSURE,
                                             newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setEnclosureU32


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setConnectorU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Connector component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a Connector component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
eses_enclosure_access_setConnectorU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue)

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
        if (connectorDataPtr->expanderSasAddress != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_CONNECTOR,
                                                   TRUE);
            connectorDataPtr->expanderSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS:
        if (connectorDataPtr->attachedSasAddress != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_CONNECTOR,
                                                   TRUE);
            connectorDataPtr->attachedSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU64(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_CONNECTOR,
                                             newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setConnectorU64

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setExpanderU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an Expander component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Expander component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
eses_enclosure_access_setExpanderU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t newValue)

{
    fbe_eses_encl_expander_info_t   *expanderDataPtr;
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
    case FBE_ENCL_EXP_SAS_ADDRESS:
        if (expanderDataPtr->expanderSasAddress != newValue)
        {
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_EXPANDER,
                                                   TRUE);
            expanderDataPtr->expanderSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
   
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU64(generalDataPtr,
                                             attribute,
                                             FBE_ENCL_EXPANDER,
                                             newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setExpanderU64

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
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
eses_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u8_t newValue)
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
        status = eses_enclosure_access_setPowerSupplyU8(generalDataPtr,
                                                        attribute,
                                                        newValue);
        break;
    case FBE_ENCL_COOLING_COMPONENT:
        status = eses_enclosure_access_setCoolingU8(generalDataPtr,
                                                    attribute,
                                                    newValue);
        break;
    case FBE_ENCL_TEMP_SENSOR:
        status = eses_enclosure_access_setTempSensorU8(generalDataPtr,
                                                       attribute,
                                                       newValue);
        break;
    case FBE_ENCL_DRIVE_SLOT:
        status = eses_enclosure_access_setDriveU8(generalDataPtr,
                                                  attribute,
                                                  newValue);
        break;
    case FBE_ENCL_EXPANDER_PHY:
        status = eses_enclosure_access_setExpPhyU8(generalDataPtr,
                                                   attribute,
                                                   newValue);
        break;
    case FBE_ENCL_CONNECTOR:
        status = eses_enclosure_access_setConnectorU8(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    case FBE_ENCL_EXPANDER:
        status = eses_enclosure_access_setExpanderU8(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    case FBE_ENCL_ENCLOSURE:
        status = eses_enclosure_access_setEnclosureU8(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    case FBE_ENCL_LCC:
        status = eses_enclosure_access_setLccU8(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    case FBE_ENCL_DISPLAY:
        status = eses_enclosure_access_setDisplayU8(generalDataPtr,
                                                    attribute,
                                                    newValue);
        break;
    case FBE_ENCL_SPS:
        status = eses_enclosure_access_setSpsU8(generalDataPtr,
                                                attribute,
                                                newValue);
        break;
    case FBE_ENCL_SSC:
        status = eses_enclosure_access_setSscU8(generalDataPtr,
                                                attribute,
                                                newValue);
        break;
    default:
        // check if component defined in base enclosure
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            component,
                                            newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_setU8


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U64 value
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
eses_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u64_t newValue)
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
        status = eses_enclosure_access_setDriveU64(generalDataPtr,
                                                   attribute,
                                                   newValue);
        break;
    case FBE_ENCL_ENCLOSURE:
        status = eses_enclosure_access_setEnclosureU64(generalDataPtr,
                                                       attribute,
                                                       newValue);
        break;
    case FBE_ENCL_CONNECTOR:
        status = eses_enclosure_access_setConnectorU64(generalDataPtr,
                                                       attribute,
                                                       newValue);
        break;
    case FBE_ENCL_EXPANDER:
        status = eses_enclosure_access_setExpanderU64(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    default:
        // check super class handler
        status = sas_enclosure_access_setU64(generalDataPtr,
                                             attribute,
                                             component,
                                             newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_setU64


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U32 value
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
eses_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u32_t newValue)
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
        status = eses_enclosure_access_setEnclosureU32(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    case FBE_ENCL_SPS:
        status = eses_enclosure_access_setSpsU32(generalDataPtr,
                                                 attribute,
                                                 newValue);
        break;
    case FBE_ENCL_POWER_SUPPLY:
        status = eses_enclosure_access_setPowerSupplyU32(generalDataPtr,
                                                 attribute,
                                                 newValue);
        break;
    default:
        // check super class handler
        status = sas_enclosure_access_setU32(generalDataPtr,
                                             attribute,
                                             component,
                                             newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_setU32


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U16 value
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
eses_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u16_t newValue)
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
        status = eses_enclosure_access_setSpsU16(generalDataPtr,
                                                 attribute,
                                                 newValue);
        break;
    default:
        // check super class handler
        status = sas_enclosure_access_setU16(generalDataPtr,
                                             attribute,
                                             component,
                                             newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of eses_enclosure_access_setU16


/*!*************************************************************************
 *                         @fn eses_enclosure_access_setEnclStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will copy an ascii string into the eses enclosure component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware attribute to reference
 *  @param newString - pointer to the source string to copy
 *  length - string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    09-Dec-2008: NC - Created
 *
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_setEnclStr(fbe_edal_general_comp_handle_t generalDataPtr,
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
        fbe_copy_memory(enclDataPtr->enclSerialNumber, 
                  newString, 
                  length);
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
} // end eses_enclosure_access_setEnclStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setLCCStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will copy an ascii string into the eses enclosure component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware attribute to reference
 *  @param newString - pointer to the source string to copy
 *  length - string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    28-July-2009: Prasenjeet - Created
 *
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_setLCCStr(fbe_edal_general_comp_handle_t generalDataPtr,
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
   // if (enclDataPtr->sasEnclosureInfo.baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != 
   if(lccDataPtr->baseLccInfo.generalComponentInfo.generalStatus.componentCanary !=
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

       if(memcmp(lccDataPtr->lccSerialNumber,newString,length))
       {
          fbe_copy_memory(lccDataPtr->lccSerialNumber, 
                                         newString, 
                                         length);
         base_enclosure_Increment_ComponentSwapCount(generalDataPtr);         
      }
      else
      {
        enclosure_access_log_debug("EDAL:%s, There is no change for Component %s for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_LCC),
            enclosure_access_printAttribute(attribute));
      }
      status = FBE_EDAL_STATUS_OK;
        break;

     case FBE_ENCL_SUBENCL_PRODUCT_ID:
        if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
        {
            length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
        }
        
        if(memcmp(lccDataPtr->lccProductId,newString,length))
        {
            fbe_copy_memory(lccDataPtr->lccProductId, 
                                         newString, 
                                         length);
            base_enclosure_Increment_ComponentSwapCount(generalDataPtr);         
        }
        else
        {
            enclosure_access_log_debug("EDAL:%s, There is no change for Component %s for %s\n", 
                __FUNCTION__, 
                enclosure_access_printComponentType(FBE_ENCL_LCC),
                enclosure_access_printAttribute(attribute));
        }
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
          fbe_copy_memory(&lccDataPtr->fwInfo, 
              newString,
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
          fbe_copy_memory(&lccDataPtr->fwInfoExp, 
              newString,
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
          fbe_copy_memory(&lccDataPtr->fwInfoBoot, 
              newString,
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
          fbe_copy_memory(&lccDataPtr->fwInfoInit, 
              newString,
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
          fbe_copy_memory(&lccDataPtr->fwInfoFPGA, 
              newString,
              length);
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
} // end eses_enclosure_access_setLCCStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setPSStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will copy an ascii string into the eses Power Supply component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware attribute to reference
 *  @param newString - pointer to the source string to copy
 *  length - string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    20-July-2009: Prasenjeet - Created
 *
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_setPSStr(fbe_edal_general_comp_handle_t generalDataPtr,
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
    if (PSDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
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
        
       if(memcmp(PSDataPtr->PSSerialNumber,newString,length))
       {
          fbe_copy_memory(PSDataPtr->PSSerialNumber, 
                                         newString, 
                                         length);
         base_enclosure_Increment_ComponentSwapCount(generalDataPtr);         
         
       }
       else
       {
          enclosure_access_log_debug("EDAL:%s, There is no change for Component %s for %s\n", 
             __FUNCTION__, 
             enclosure_access_printComponentType(FBE_ENCL_POWER_SUPPLY),
             enclosure_access_printAttribute(attribute));
       }
       status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_SUBENCL_PRODUCT_ID:
        if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
        {
            length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
        }
      
        fbe_copy_memory(PSDataPtr->psProductId, 
                                        newString, 
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
        fbe_copy_memory(&PSDataPtr->fwInfo, 
            newString,
            length);
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
} // end eses_enclosure_access_setEnclStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setCoolingStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function will copy an ascii string into the eses Cooling component.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware attribute to reference
 *  @param newString - pointer to the source string to copy
 *  length - string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    30-Apr-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_setCoolingStr(fbe_edal_general_comp_handle_t generalDataPtr,
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
    if (coolingDataPtr->generalCoolingInfo.generalComponentInfo.generalStatus.componentCanary != 
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
        
       if(memcmp(coolingDataPtr->coolingSerialNumber,newString,length))
       {
          fbe_copy_memory(coolingDataPtr->coolingSerialNumber, 
                                         newString, 
                                         length);
         base_enclosure_Increment_ComponentSwapCount(generalDataPtr);         
         
       }
       else
       {
          enclosure_access_log_debug("EDAL:%s, There is no change for Component %s for %s\n", 
             __FUNCTION__, 
             enclosure_access_printComponentType(FBE_ENCL_COOLING_COMPONENT),
             enclosure_access_printAttribute(attribute));
       }
       status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_SUBENCL_PRODUCT_ID:
        if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
        {
            length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
        }

        fbe_copy_memory(coolingDataPtr->coolingProductId, 
                                        newString, 
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
        fbe_copy_memory(&coolingDataPtr->fwInfo, 
            newString,
            length);
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
} // end eses_enclosure_access_setCoolingStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setSpsStr
 **************************************************************************
 *  @brief
 *  This function will copy an ascii string into the eses SPS component.
 *
 *  @param generalDataPtr - pointer to the component specific information
 *  @param attribute - specifies the firmware attribute to reference
 *  @param length - string length to copy
 *  @param newString - pointer to the source string to copy
 *
 *  @return fbe_edal_status_t - status of the operation
 *
 *  @version:
 *    21-Sep-2012: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t eses_enclosure_access_setSpsStr(fbe_edal_general_comp_handle_t generalDataPtr,
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
    if (spsDataPtr->baseSpsInfo.generalComponentInfo.generalStatus.componentCanary != 
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
        
       if(memcmp(spsDataPtr->spsSerialNumber,newString,length))
       {
          fbe_copy_memory(spsDataPtr->spsSerialNumber, 
                                         newString, 
                                         length);
         base_enclosure_Increment_ComponentSwapCount(generalDataPtr);         
         
       }
       else
       {
          enclosure_access_log_debug("EDAL:%s, There is no change for Component %s for %s\n", 
             __FUNCTION__, 
             enclosure_access_printComponentType(FBE_ENCL_SPS),
             enclosure_access_printAttribute(attribute));
       }
       status = FBE_EDAL_STATUS_OK;
        break;

    case FBE_ENCL_SUBENCL_PRODUCT_ID:
        if (length > FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE) 
        {
            length = FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE;
        }

        fbe_copy_memory(spsDataPtr->spsProductId, 
                                        newString, 
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
        fbe_copy_memory(&spsDataPtr->primaryFwInfo, 
            newString,
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
        fbe_copy_memory(&spsDataPtr->secondaryFwInfo, 
            newString,
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
        fbe_copy_memory(&spsDataPtr->batteryFwInfo, 
            newString,
            length);
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Component %s has no support for %s\n", 
            __FUNCTION__, 
            enclosure_access_printComponentType(FBE_ENCL_SPS),
            enclosure_access_printAttribute(attribute));
        status = FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;
} // end eses_enclosure_access_setSpsStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *  This function calls a function to set a new string value. The input
 *  element_component  determines which function to call.
 *
 *  PARAMETERS:
 *  @param generalDataPtr - pointer to eses object data
 *  @param attribute - specifies the target atttribute to reference
 *  @param enclosure_component - specifies the target enclosure component
 *  @param newString - pointer to the string to write
 *  length - string length to copy
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  HISTORY:
 *    13-Oct-2008: GB - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t enclosure_component,
                             fbe_u32_t length,
                             char *newString)
{
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }

    switch (enclosure_component)
    {
    case FBE_ENCL_ENCLOSURE:
        status = eses_enclosure_access_setEnclStr(generalDataPtr,
                                                attribute,
                                                length,
                                                newString);
        break;
    case FBE_ENCL_LCC:
        status = eses_enclosure_access_setLCCStr(generalDataPtr,
                                                attribute,
                                                length,
                                                newString);
        break;
    case FBE_ENCL_POWER_SUPPLY:
        status = eses_enclosure_access_setPSStr(generalDataPtr,
                                                attribute,
                                                length,
                                                newString);
        break;
    case FBE_ENCL_COOLING_COMPONENT:
        status = eses_enclosure_access_setCoolingStr(generalDataPtr,
                                                attribute,
                                                length,
                                                newString);
        break;
    case FBE_ENCL_SPS:
        status = eses_enclosure_access_setSpsStr(generalDataPtr,
                                                attribute,
                                                length,
                                                newString);
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

}   // end eses_enclosure_access_setStr

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setPowerSupplyU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of a Power Supply component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Enclosure component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U32 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Dec-2013: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setPowerSupplyU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u32_t newValue)

{
    fbe_eses_power_supply_info_t     *psDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    psDataPtr = (fbe_eses_power_supply_info_t *)generalDataPtr;
    if (psDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary != 
        EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            psDataPtr->generalPowerSupplyInfo.generalComponentInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }


    switch (attribute)
    {
    case FBE_ENCL_BD_BUFFER_SIZE:
        psDataPtr->bufferDescriptorInfo.bufferSize = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;
    
    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU32(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_POWER_SUPPLY,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setPowerSupplyU32

/*!*************************************************************************
 *                         @fn eses_enclosure_access_setSscU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an ESES enclosure object fields
 *      of an SSC(System Status Card) component.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an Expander PHY component
 *      @param attribute - enum for attribute to get
 *      @param newValue - new U8 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    10-Jan-2014: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t 
eses_enclosure_access_setSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t newValue)

{
    fbe_eses_encl_ssc_info_t    *sscDataPtr;
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
        sscDataPtr->sscElementIndex = newValue;
        status = FBE_EDAL_STATUS_OK;
        break;

    default:
        // check if attribute defined in super class
        status = sas_enclosure_access_setU8(generalDataPtr,
                                            attribute,
                                            FBE_ENCL_SSC,
                                            newValue);
        break;
    }

    return status;

}   // end of eses_enclosure_access_setSscU8
