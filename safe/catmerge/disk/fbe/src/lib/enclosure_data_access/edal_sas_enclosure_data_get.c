/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file edal_sas_enclosure_data_get.h
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module contains functions to retrieve data from the SAS
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
 *                         @fn sas_enclosure_access_getBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
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
sas_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
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
    // nothing SAS specific - go to super class
    default:
        status = base_enclosure_access_getBool(generalDataPtr,
                                               attribute,
                                               component,
                                               returnDataPtr);
        break;
    }

    return status;

}   // end of sas_enclosure_access_getBool


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getEnclosureU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
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
sas_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t *returnDataPtr)

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
        *returnDataPtr = enclDataPtr->sesSasAddress;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_SMP_SAS_ADDRESS:
        *returnDataPtr = enclDataPtr->smpSasAddress;
        status = FBE_EDAL_STATUS_OK;
        break;
    case FBE_ENCL_SERVER_SAS_ADDRESS:
        *returnDataPtr = enclDataPtr->serverSasAddress;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
         // check if attribute defined in base enclosure
        status = base_enclosure_access_getU64(generalDataPtr,
                                               attribute,
                                               FBE_ENCL_ENCLOSURE,
                                               returnDataPtr);
        break;
    }

    return status;

}   // end of sas_enclosure_access_getEnclosureU64


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getEnclosureU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
 *      of an Enclosure component.
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
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
sas_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u32_t *returnDataPtr)

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
        *returnDataPtr = enclDataPtr->uniqueId;
        status = FBE_EDAL_STATUS_OK;
        break;
    default:
         // check if attribute defined in base enclosure
        status = base_enclosure_access_getU32(generalDataPtr,
                                               attribute,
                                               FBE_ENCL_ENCLOSURE,
                                               returnDataPtr);
        break;
    }

    return status;

}   // end of sas_enclosure_access_getEnclosureU32


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
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
sas_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
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
    default:
        // check if component defined in base enclosure
        status = base_enclosure_access_getU8(generalDataPtr,
                                              attribute,
                                              component,
                                              returnDataPtr);

        break;             
    }

    return status;

}   // end of sas_enclosure_access_getU8


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
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
 *    12-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t 
sas_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
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
    default:
        // check if component defined in base enclosure
        status = base_enclosure_access_getU16(generalDataPtr,
                                              attribute,
                                              component,
                                              returnDataPtr);

        break;             
    }

    return status;

}   // end of sas_enclosure_access_getU16


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
 *      that are U64 type.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to a component data
 *      @param attribute - enum for attribute to get
 *      @param component - type of Component (PS, Drive, ...)
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
sas_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
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
    case FBE_ENCL_ENCLOSURE:
        status = sas_enclosure_access_getEnclosureU64(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
        break;
    default:
        // check super class handler
        status = base_enclosure_access_getU64(generalDataPtr,
                                               attribute,
                                               component,
                                               returnDataPtr);
        break;              
    }

    return status;

}   // end of sas_enclosure_access_getU64


/*!*************************************************************************
 *                         @fn sas_enclosure_access_getU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get data from SAS enclosure object fields
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
sas_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
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
        status = sas_enclosure_access_getEnclosureU32(generalDataPtr,
                                                      attribute,
                                                      returnDataPtr);
        break;
    default:
        // check super class handler
        status = base_enclosure_access_getU32(generalDataPtr,
                                              attribute,
                                              component,
                                              returnDataPtr);
        break;              
    }

    return status;

}   // end of sas_enclosure_access_getU32