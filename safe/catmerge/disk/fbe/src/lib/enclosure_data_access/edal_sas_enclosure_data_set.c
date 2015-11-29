/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_sas_enclosure_data_set.h
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to update data from the SAS
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

#include "edal_sas_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      that are Boolean type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
sas_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
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
        // nothing special, use super class handler
    default:
        // use super class handler
        status = base_enclosure_access_setBool(generalDataPtr,
                                               attribute,
                                               component,
                                               newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setBool


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      of an Enclosure component.
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
sas_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t newValue)

{
    fbe_sas_enclosure_info_t        *enclDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_sas_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_SES_SAS_ADDRESS:
        if (enclDataPtr->sesSasAddress != newValue)
        {
            status = sas_enclosure_access_setBool(generalDataPtr,
                                                  FBE_ENCL_COMP_STATE_CHANGE,
                                                  FBE_ENCL_ENCLOSURE,
                                                  TRUE);
            enclDataPtr->sesSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_SMP_SAS_ADDRESS:
        if (enclDataPtr->smpSasAddress != newValue)
        {
            status = sas_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->smpSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    case FBE_ENCL_SERVER_SAS_ADDRESS:
        if (enclDataPtr->serverSasAddress != newValue)
        {
            status = sas_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->serverSasAddress = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_setU64(generalDataPtr,
                                               attribute,
                                               FBE_ENCL_ENCLOSURE,
                                               newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setEnclosureU64


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      of an Enclosure component.
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
sas_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u32_t newValue)

{
    fbe_sas_enclosure_info_t        *enclDataPtr;
    fbe_edal_status_t    status = FBE_EDAL_STATUS_ERROR;

    // validate pointer & buffer
    if (generalDataPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Component Data Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_COMP_DATA_PTR;
    }
    enclDataPtr = (fbe_sas_enclosure_info_t *)generalDataPtr;
    if (enclDataPtr->baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary != EDAL_COMPONENT_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Component Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, 
            EDAL_COMPONENT_CANARY_VALUE, 
            enclDataPtr->baseEnclosureInfo.generalEnclosureInfo.generalStatus.componentCanary);
        return FBE_EDAL_STATUS_INVALID_COMP_CANARY;
    }

    switch (attribute)
    {
    case FBE_ENCL_UNIQUE_ID:
        if (enclDataPtr->uniqueId != newValue)
        {
            status = sas_enclosure_access_setBool(generalDataPtr,
                                                   FBE_ENCL_COMP_STATE_CHANGE,
                                                   FBE_ENCL_ENCLOSURE,
                                                   TRUE);
            enclDataPtr->uniqueId = newValue;
        }
        else
        {
            status = FBE_EDAL_STATUS_OK;
        }
        break;
    default:
        // check if attribute defined in base enclosure
        status = base_enclosure_access_setU32(generalDataPtr,
                                              attribute,
                                              FBE_ENCL_ENCLOSURE,
                                              newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setEnclosureU32


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      that are U8 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
sas_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
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
    default:
        // check if component defined in super class
        status = base_enclosure_access_setU8(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setU8

/*!*************************************************************************
 *                         @fn sas_enclosure_access_setU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      that are U16 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
 *      @param newValue - new U16 value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    12-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
sas_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
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
    default:
        // check if component defined in super class
        status = base_enclosure_access_setU16(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setU16


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
sas_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
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
    case FBE_ENCL_ENCLOSURE:
        status = sas_enclosure_access_setEnclosureU64(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    default:
        // check super class handler
        status = base_enclosure_access_setU64(generalDataPtr,
                                               attribute,
                                               component,
                                               newValue);
        break;
    }

    return status;

}   // end of sas_enclosure_access_setU64


/*!*************************************************************************
 *                         @fn sas_enclosure_access_setU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set data in an SAS enclosure object fields
 *      that are U32 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
sas_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
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
        status = sas_enclosure_access_setEnclosureU32(generalDataPtr,
                                                      attribute,
                                                      newValue);
        break;
    default:
        // check super class handler
        status = base_enclosure_access_setU32(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
        break;              // status defaulted to failure
    }

    return status;

}   // end of sas_enclosure_access_setU32