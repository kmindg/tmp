/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file fbe_enclosure_data_access.c
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module is a library that provides access functions (get)
 *      for Enclosure Object data.  The enclosure object has multiple
 *      classes where the data may reside and the caller does not require
 *      any knowledge about what class it is referencing.
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
 *      06-Aug-08   Joe Perry - Created
 *
 *
 **************************************************************************/

// Includes
#include "fbe/fbe_winddk.h"
#include "fbe_enclosure_data_access_public.h"
#include "fbe_enclosure_data_access_private.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_eses.h"
#include "fbe_enclosure.h"
#include "edal_processor_enclosure_data.h"



//***********************  Local Prototypes ***********************************



/*!*************************************************************************
 *                        @fn fbe_edal_getNumberOfComponents
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure block's number of components.
 *  It will not look into the chain of the blocks.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to data store
 *      @param returnValuePtr - pointer used to return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    16-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getNumberOfComponents(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                               fbe_u32_t *returnValuePtr)
{
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    *returnValuePtr = componentBlockPtr->numberComponentTypes;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_getNumberOfComponents

/*!*************************************************************************
 *                        @fn fbe_edal_getComponentType
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the type of component by index
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to data store
 *      @param index - index into component desriptor.
 *      @param returnValuePtr - pointer used to return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    22-Dec-2008: NC - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getComponentType(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                          fbe_u32_t index,
                          fbe_enclosure_component_types_t *returnValuePtr)
{
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_enclosure_component_t       *componentTypeDescriptors;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    if (index >= componentBlockPtr->numberComponentTypes)
    {
        enclosure_access_log_error("EDAL:%s, index %d over max valid number of type %d\n", 
            __FUNCTION__, index, componentBlockPtr->numberComponentTypes);
        return FBE_EDAL_STATUS_COMP_TYPE_INDEX_INVALID;
    }
    /* find the start of component type descriptor */
    componentTypeDescriptors = edal_calculateComponentTypeDataStart(componentBlockPtr);
    /* locate the requested entry */
    componentTypeDescriptors += index;

    *returnValuePtr = componentTypeDescriptors->componentType;

    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_getComponentType


/*!*************************************************************************
 *                        @fn fbe_edal_getOverallStateChangeCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure block's overall State Change count
 *  This function will traverse through the chain of the blocks.
 *
 *  PARAMETERS:
 *      @param enclosurePtr - pointer to some type of enclosure object
 *      @param returnValuePtr - pointer used to return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    16-Oct-2008: Joe Perry - Created
 *    17-Mar-2010: PHE - Modified for the component type in multiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getOverallStateChangeCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                    fbe_u32_t *returnValuePtr)
{
    fbe_enclosure_component_block_t *componentBlockPtr;

    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }
	
    *returnValuePtr = 0;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    *returnValuePtr = componentBlockPtr->overallStateChangeCount;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_getOverallStateChangeCount


/*!*************************************************************************
 *                        @fn fbe_edal_getComponentOverallStatus
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the overall Status for a specific component
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosurePtr - pointer to some type of enclosure object
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer used to return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    25-Nov-2008: Joe Perry - Created
 *    17-Mar-2010: PHE - Modified for the component type in multiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getComponentOverallStatus(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t component,
                                   fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t               status;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /* The same component type has the same componentOverallStatus
     * Even though it could be in multiple EDAL blocks.
     * So we just need to get the first specified component type.
     */
    status = enclosure_access_getSpecifiedComponentTypeDataWithIndex(componentBlockPtr,
                                                            &componentTypeDataPtr,
                                                            component,
                                                            0);   // The first EDAL block which has the component type.
                                                                  
    if (status == FBE_EDAL_STATUS_OK)
    {
        *returnValuePtr = componentTypeDataPtr->componentOverallStatus;
    }
    return status;
} // end of fbe_edal_getComponentOverallStatus

/*!*************************************************************************
 *                        @fn fbe_edal_getComponentOverallStateChangeCount
 **************************************************************************
 *  @brief
 *      This function gets componentOverallStateChangeCount for the specified component.
 *
 *  @param enclosurePtr - pointer to some type of enclosure object
 *  @param component - specific component to get (PS, Drive, ...)
 *  @param returnValuePtr - pointer used to return value
 *
 *  @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    17-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getComponentOverallStateChangeCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t componentType,
                                   fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t               edal_status;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /* The same component type has the same componentOverallStatus
     * Even though it could be in multiple EDAL blocks.
     * So we just need to get the first specified component type.
     */
    edal_status = enclosure_access_getSpecifiedComponentTypeDataWithIndex(componentBlockPtr,
                                                            &componentTypeDataPtr,
                                                            componentType,
                                                            0);   // The first EDAL block which has the component type.
                                                                  
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        *returnValuePtr = componentTypeDataPtr->componentOverallStateChangeCount;
    }

    return edal_status;

}   // end of fbe_edal_getComponentOverallStateChangeCount


/*!*************************************************************************
 *                        @fn fbe_edal_getSpecificComponentCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the number of components for a specific component
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to some type of enclosure object
 *      @param componentType - the component type(PS, Drive, ...)
 *      @param componentCountPtr - pointer to the component count
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *        If the component type does not exist in the EDAL block chains, 
 *        return FBE_EDAL_STATUS_OK with component count 0.
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    16-Oct-2008: Joe Perry - Created
 *    16-Mar-2010: PHE - Modified for the component type which is allocated in multiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getSpecificComponentCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t componentType,
                                   fbe_u32_t *componentCountPtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_OK;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_enclosure_component_block_t *localCompBlockPtr = NULL;


    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer.\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (componentCountPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null componentCountPtr.\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    *componentCountPtr = 0;

    localCompBlockPtr = componentBlockPtr;

    // search all Component Data Blocks
    while (localCompBlockPtr != NULL)
    {
        status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(localCompBlockPtr,
                                                                &componentTypeDataPtr,
                                                                componentType);
        if (status == FBE_EDAL_STATUS_OK)
        {
            *componentCountPtr += componentTypeDataPtr->maxNumberOfComponents;
        }

        // check if there's a next block
        localCompBlockPtr = localCompBlockPtr->pNextBlock;
    }// end of while loop

    return FBE_EDAL_STATUS_OK;

} // end of fbe_edal_getSpecificComponentCount

/*!*************************************************************************
 *                        @fn fbe_edal_getEnclosureType
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure object's Type
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param enclosureTypePtr - pointer used to return enclosure Type
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the enclosureTypePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getEnclosureType(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_enclosure_types_t *enclosureTypePtr)
{
    fbe_enclosure_component_block_t *dataPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to Base enclosure to get Base enclosure type
    dataPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (dataPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, dataPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    *enclosureTypePtr = dataPtr->enclosureType;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_getEnclosureType




/*!*************************************************************************
 *                        @fn fbe_edal_getEnclosureSide
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the enclosure object's side (A or B).
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param returnValuePtr - pointer to returned Side info
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    01-Dec-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getEnclosureSide(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t *returnValuePtr)
{
    fbe_enclosure_component_block_t *dataPtr;
    //fbe_u8_t                        lccIndex;
    //fbe_u32_t                       lccCount;
    //fbe_bool_t                      isLocalLcc;
    fbe_edal_status_t               status;
    fbe_u8_t                        enclSide;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to Base enclosure to get Base enclosure type
    dataPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (dataPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, dataPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    // component_index is taken as 0.
    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                            FBE_ENCL_COMP_SIDE_ID,
                            FBE_ENCL_ENCLOSURE,
                            0,
                            &enclSide);

    if(status == FBE_EDAL_STATUS_OK)
    {
        *returnValuePtr = enclSide;
        return FBE_EDAL_STATUS_OK;
    }
    enclosure_access_log_error("EDAL:%s, Get FBE_ENCL_COMP_SIDE_ID failed, status %d\n", 
        __FUNCTION__, status);

    return FBE_EDAL_STATUS_ERROR;
} // end of fbe_edal_getEnclosureSide


/*!*************************************************************************
 *                         @fn fbe_edal_getBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Boolean attribute value from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t index,
                 fbe_bool_t *returnValuePtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t                status;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &generalDataPtr);
    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getBool_direct(generalDataPtr,
                                         componentBlockPtr->enclosureType,
                                         attribute,
                                         component,
                                         returnValuePtr);
    }
    return status;
} // end of fbe_edal_getBool

/*!*************************************************************************
 *                         @fn fbe_edal_getBool_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Boolean attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getBool_direct(fbe_edal_component_data_handle_t generalDataPtr,
                        fbe_enclosure_types_t  enclosureType,  
                        fbe_base_enclosure_attributes_t attribute,
                        fbe_enclosure_component_types_t component,
                        fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;

    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:        
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getBool(generalDataPtr,
                                               attribute,
                                               component,
                                               returnValuePtr);
            break;

        case FBE_ENCL_PE:
            status = processor_enclosure_access_getBool(generalDataPtr,
                                                        attribute,
                                                        component,
                                                        returnValuePtr);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }

    return status;
} // end of fbe_edal_getBool_direct

/**************************************************************************
 *                         @fn fbe_edal_getU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a U8 attribute value from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param returnValuePtr - pointer to a U8 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
               fbe_base_enclosure_attributes_t attribute,
               fbe_enclosure_component_types_t component,
               fbe_u32_t index,
               fbe_u8_t *returnValuePtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getU8_direct(generalDataPtr,
                                       componentBlockPtr->enclosureType,
                                       attribute,
                                       component,
                                       returnValuePtr);
    }
    return status;
} // end of fbe_edal_getU8

/*!*************************************************************************
 *                         @fn fbe_edal_getU8_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a u8 attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU8_direct(fbe_edal_component_data_handle_t generalDataPtr,
                      fbe_enclosure_types_t  enclosureType,  
                      fbe_base_enclosure_attributes_t attribute,
                      fbe_enclosure_component_types_t component,
                      fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;

    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getU8(generalDataPtr,
                                                 attribute,
                                                 component,
                                                 returnValuePtr);
            break;

        case FBE_ENCL_PE:
           status = processor_enclosure_access_getU8(generalDataPtr, 
                                                     attribute, 
                                                     component, 
                                                     returnValuePtr);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_getU8_direct

/**************************************************************************
 *                         @fn fbe_edal_getU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a U16 attribute value from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param returnValuePtr - pointer to a U16 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u16_t *returnValuePtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getU16_direct(generalDataPtr,
                                        componentBlockPtr->enclosureType,  
                                        attribute,
                                        component,
                                        returnValuePtr);
    }
    return status;
} // end of fbe_edal_getU16


/*!*************************************************************************
 *                         @fn fbe_edal_getU16_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a u16 attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU16_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u16_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
   
    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getU16(generalDataPtr,
                                              attribute,
                                              component,
                                              returnValuePtr);
            break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_getU16_direct

/**************************************************************************
 *                         @fn fbe_edal_getU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a U32 attribute value from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param returnValuePtr - pointer to a U32 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU32(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t *returnValuePtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getU32_direct(generalDataPtr,
                                        componentBlockPtr->enclosureType,  
                                        attribute,
                                        component,
                                        returnValuePtr);
    }
    return status;
} // end of fbe_edal_getU32


/*!*************************************************************************
 *                         @fn fbe_edal_getU32_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a u32 attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU32_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;

    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getU32(generalDataPtr,
                                              attribute,
                                              component,
                                              returnValuePtr);
            break;

        case FBE_ENCL_PE:
           status = processor_enclosure_access_getU32(generalDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      returnValuePtr);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;          
            break;
    }
    return status;
} // end of fbe_edal_getU32_direct

/**************************************************************************
 *                         @fn fbe_edal_getU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a U64 attribute value from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param returnValuePtr - pointer to a U64 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u64_t *returnValuePtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getU64_direct(generalDataPtr,
                                        componentBlockPtr->enclosureType,
                                        attribute,
                                        component,
                                        returnValuePtr);
    }
    return status;
} // end of fbe_edal_getU64


/*!*************************************************************************
 *                         @fn fbe_edal_getU64_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a u64 attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t 
fbe_edal_getU64_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u64_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
   
    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getU64(generalDataPtr,
                                                  attribute,
                                                  component,
                                                  returnValuePtr);
            break;
    
        case FBE_ENCL_PE:
           status = processor_enclosure_access_getU64(generalDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      returnValuePtr);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;              
            break;
    }
    return status;
} // end of fbe_edal_getU64_direct


/*!*************************************************************************
 *                         @fn fbe_edal_getBuffer
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a block of data in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param componentType - specific component to get (PS, Drive, ...)
 *      @param componentIndex - number of the Drive component to access (drive in slot 5)
 *      @param bufferLength - string length to get/copy
 *      @param bufferPtr - output buffer pointer 
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    17-Apr-2010: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getBuffer(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t componentType,
                fbe_u32_t componentIndex,
                fbe_u32_t bufferLength,
                fbe_u8_t *bufferPtr)
{
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            componentType,
															componentIndex,
                                                            &generalDataPtr);
    if (status == FBE_EDAL_STATUS_OK)
    {
        switch (componentBlockPtr->enclosureType)
        {
        case FBE_ENCL_PE:
            status =
                processor_enclosure_access_getBuffer(generalDataPtr,
                                                attribute,
                                                componentType,
                                                bufferLength,
                                                bufferPtr);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                componentBlockPtr->enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
        }
    }
    return status;

}   // end of fbe_edal_getBuffer

/*!*************************************************************************
 *                         @fn fbe_edal_getStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a string pointer from an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr (in) - pointer to the component block
 *      @param attribute (in) - enum for attribute to get
 *      @param component (in) - specific element to get (PS, Drive, ...)
 *      @param index (in) - specific firmware component device (main, expander, etc)
 *      @param length (in) - string length to get/copy
 *      @param returnStringPtr (in/out) - pointer where the target gets copied
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnStringPtr contents.
 *
 *  HISTORY:
 *    15-Oct-2008: GB - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getStr(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t length,
                char *returnStringPtr)
{
    void                            *componentTypeDataPtr;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (returnStringPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnStringPtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    // get a pointer to the component type data
    status = fbe_base_enclosure_access_getSpecificComponent(componentBlockPtr,
                                                            component,
                                                            index,
                                                            &componentTypeDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_getStr_direct(componentTypeDataPtr,
                                        componentBlockPtr->enclosureType,
                                        attribute,
                                        component,
                                        length,
                                        returnStringPtr);
    }
    return status;

} // end of fbe_edal_getStr

/*!*************************************************************************
 *                         @fn fbe_edal_getStr_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a string attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param componentTypeDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnStringPtr contents.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getStr_direct(fbe_edal_component_data_handle_t componentTypeDataPtr,
                       fbe_enclosure_types_t  enclosureType,
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u32_t length,
                       char *returnStringPtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;

    switch (enclosureType)
    {
        case FBE_ENCL_SAS_CITADEL:
        case FBE_ENCL_SAS_BUNKER:
        case FBE_ENCL_SAS_DERRINGER:
        case FBE_ENCL_SAS_VIPER:
        case FBE_ENCL_SAS_MAGNUM:
        case FBE_ENCL_SAS_FALLBACK:
        case FBE_ENCL_SAS_BOXWOOD:
        case FBE_ENCL_SAS_KNOT:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_getStr((fbe_edal_general_comp_handle_t) componentTypeDataPtr,
                                              attribute,
                                              component,
                                              length,
                                              returnStringPtr);
            break;
    
        case FBE_ENCL_PE:
           status = processor_enclosure_access_getStr((fbe_edal_general_comp_handle_t) componentTypeDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      length,
                                                      returnStringPtr);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_getStr_direct

/*!*************************************************************************
 *                         @fn fbe_edal_getDriveBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get an Drive Boolean attribute value from 
 *      a chained EDAL blocks based on Drive slot number.
 *      This function will try to match the drive slot from all the chained 
 *      EDAL blocks.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive_slot - slot number of the Drive 
 *      @param returnValuePtr - pointer to Boolean return value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    19-May-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getDriveBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                      fbe_base_enclosure_attributes_t attribute,
                      fbe_u32_t drive_slot,
                      fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr = NULL;
    fbe_edal_component_data_handle_t generalDataPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    status = fbe_edal_getU8MatchingComponent(enclosureComponentBlockPtr,
                                             FBE_ENCL_DRIVE_SLOT_NUMBER,
                                             FBE_ENCL_DRIVE_SLOT,
                                             drive_slot,
                                             &generalDataPtr);

    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, Drive not found, %d\n", 
            __FUNCTION__, drive_slot);
        return status;
    }
    status = fbe_edal_getBool_direct(generalDataPtr,
                                    componentBlockPtr->enclosureType,  
                                    attribute,
                                    FBE_ENCL_DRIVE_SLOT,
                                    returnValuePtr);

    return status;
} // end of fbe_edal_getDriveBool

/*!*************************************************************************
 *                         @fn fbe_edal_getDrivePhyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get an Expander PHY Boolean attribute value from an
 *      Enclosure Object based on Drive number.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive - number of the Drive whose PHY attribute is wanted
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getDrivePhyBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_u32_t drive,
                         fbe_bool_t *returnValuePtr)
{
    fbe_u8_t                        phyIndex = 0;
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr = NULL;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                            FBE_ENCL_DRIVE_PHY_INDEX,
                            FBE_ENCL_DRIVE_SLOT,
                            drive,
                            &phyIndex);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, Drive not found, %d\n", 
            __FUNCTION__, drive);
        return status;
    }

    status = fbe_edal_getBool(enclosureComponentBlockPtr,
                              attribute,
                              FBE_ENCL_EXPANDER_PHY,
                              phyIndex,
                              returnValuePtr);

    return status;

} // end of fbe_edal_getDrivePhyBool


/*!*************************************************************************
 *                         @fn fbe_edal_getDrivePhyU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get an Expander PHY Boolean attribute value from an
 *      Enclosure Object based on Drive number.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive - number of the Drive whose PHY attribute is wanted
 *      @param returnValuePtr - pointer to a U8 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getDrivePhyU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_u32_t drive,
                       fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t                status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr = NULL;
    fbe_u8_t                        phyIndex = 0;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                            FBE_ENCL_DRIVE_PHY_INDEX,
                            FBE_ENCL_DRIVE_SLOT,
                            drive,
                            &phyIndex);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, Drive not found, %d\n", 
            __FUNCTION__, drive);
        return status;
    }

    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                            attribute,
                            FBE_ENCL_EXPANDER_PHY,
                            phyIndex,
                            returnValuePtr);

    return status;

} // end of fbe_edal_getDrivePhyU8


/*!*************************************************************************
 *                         @fn fbe_edal_getConnectorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Connector attribute value from an
 *      Enclosure Object based on connector type (primary, expansion).
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param connectorType - type of connector that is wanted
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getConnectorBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                          fbe_base_enclosure_attributes_t attribute,
                          fbe_edal_connector_type_t connectorType,
                          fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       connectorIndex;
    fbe_u32_t                       connectorCount;
    fbe_bool_t                      primaryPort;
    fbe_bool_t                      isLocalConnector;
    fbe_bool_t                      isEntireConnector;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    // find the correct Connector specified by the caller
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_CONNECTOR,
                                                &connectorCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, Get Connector Count failed, status %d\n", 
            __FUNCTION__, status);
        return status;
    }

    // search through Connectors (connectors on side A are stored first, followed by connectors on side B)
    for (connectorIndex = 0; connectorIndex < connectorCount; connectorIndex++)
    {
        status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_IS_LOCAL,
                                  FBE_ENCL_CONNECTOR,
                                  connectorIndex,
                                  &isLocalConnector);

        if((status == FBE_EDAL_STATUS_OK) &&isLocalConnector)
        {
            status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,
                                    FBE_ENCL_CONNECTOR,
                                    connectorIndex,
                                    &isEntireConnector);

            if((status == FBE_EDAL_STATUS_OK) && isEntireConnector)
            {
                // get Primary Port flag from Connector & compare with parameter
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                    FBE_ENCL_CONNECTOR,
                                    connectorIndex,
                                    &primaryPort);            

                if ((status == FBE_EDAL_STATUS_OK) && 
                    (((connectorType == FBE_EDAL_PRIMARY_CONNECTOR) && primaryPort) ||
                    ((connectorType ==  FBE_EDAL_EXPANSION_CONNECTOR) && !primaryPort)))
                {
                    status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_CONNECTOR,
                                            connectorIndex,
                                            returnValuePtr);
                    return status;
                }
            }
        }
    }

    return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;

} // end of fbe_edal_getConnectorBool



/*!*************************************************************************
* @fn fbe_edal_getConnectorIndex ( fbe_edal_block_handle_t enclosureComponentBlockPtr,
*                             fbe_base_enclosure_attributes_t entireConnector,
*                             fbe_base_enclosure_attributes_t localConnector,
*                             fbe_base_enclosure_attributes_t primaryPort,
*                             fbe_u8_t *connectorIndex)
************************************************************************
*
* @brief
*       This function will return the index of the mezanine
*        connector
*
* @param      fbe_edal_block_handle_t - pointer to the connector block

*             Connector attributes to specify which connector we want the index for:
*             fbe_base_enclosure_attributes_t entireConnector
*             fbe_base_enclosure_attributes_t localConnector 
*             fbe_base_enclosure_attributes_t primaryPort
**
* @return     fbe_u8_t - index of the mezanine
*                         primary connector.
*
* NOTES
*
* HISTORY
*   9-June-2010     D. McFarland - Created initially to get the index for the mezanine connector.
***************************************************************************/
fbe_edal_status_t 
fbe_edal_getConnectorIndex ( fbe_edal_block_handle_t enclosureComponentBlockPtr,
                             fbe_base_enclosure_attributes_t entireConnector,
                             fbe_base_enclosure_attributes_t localConnector,
                             fbe_base_enclosure_attributes_t primaryPort,
                             fbe_u8_t *connectorIndex)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t           connectorCount;
    fbe_bool_t          isPrimaryPort;
    fbe_bool_t          isLocalConnector;
    fbe_bool_t          isEntireConnector;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (connectorIndex == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    // Get the number of connectors:
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_CONNECTOR,
                                                &connectorCount);

    // Parse through connectors for the proper index
    // Connectors are stored in this order: SPA Primary, SPA Expansion, SPB Primary, SPB Expansion.
    // Each connector block has 5 elements: Entire, Phy 0, Phy 1, Phy 2, Phy 3
    //
    for (*connectorIndex = 0; *connectorIndex < connectorCount; (*connectorIndex)++)
    {
        status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                  localConnector,           // <<<< Local meaning whether it's on this SP, vs the peer.
                                  FBE_ENCL_CONNECTOR,
                                  *connectorIndex,
                                  &isLocalConnector);

        if((status == FBE_EDAL_STATUS_OK) &&isLocalConnector)
        {
            status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    entireConnector,        // Whether this is the attribute representing the entire connector.
                                    FBE_ENCL_CONNECTOR,
                                    *connectorIndex,
                                    &isEntireConnector);

            if((status == FBE_EDAL_STATUS_OK) && isEntireConnector)
            {
                // get Primary Port flag from Connector & compare with parameter
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    primaryPort,            // Primary port vs the Expansion port.
                                    FBE_ENCL_CONNECTOR,
                                    *connectorIndex,
                                    &isPrimaryPort);            

                if ( (status == FBE_EDAL_STATUS_OK) && !isPrimaryPort )  // !primaryPort means the expansion port.
                {
                    return status; // Done!  The only thing we want is the index for the mezanine expansion connector.
                }
            }
        }
    }

    // Should never get here.  There should always be a local, entire, expansion connector. 
    return FBE_EDAL_STATUS_ERROR;   
}


/*!*************************************************************************
 *                         @fn fbe_edal_getConnectorU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Connector attribute value from an
 *      Enclosure Object based on connector type (primary, expansion).
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param connectorType - type of connector that is wanted
 *      @param returnValuePtr - pointer to a U64 to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getConnectorU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_connector_type_t connectorType,
                         fbe_u64_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u8_t                        connectorIndex;
    fbe_u32_t                       connectorCount;
    fbe_bool_t                      primaryPort;
    fbe_bool_t                      isLocalConnector;
    fbe_bool_t                      isEntireConnector;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    // find the correct Connector specified by the caller
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_CONNECTOR,
                                                &connectorCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, Get Connector Count failed, status %d\n", 
            __FUNCTION__, status);
        return status;
    }

    // search through Connectors (local connectors are stored first, followed by peer connectors)
    for (connectorIndex = 0; connectorIndex < connectorCount; connectorIndex++)
    {
        status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_IS_LOCAL,
                                  FBE_ENCL_CONNECTOR,
                                  connectorIndex,
                                  &isLocalConnector);

        if((status == FBE_EDAL_STATUS_OK) && isLocalConnector)
        {
            status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,
                                    FBE_ENCL_CONNECTOR,
                                    connectorIndex,
                                    &isEntireConnector);

            if((status == FBE_EDAL_STATUS_OK) && isEntireConnector)
            {
                // get Primary Port flag from Connector & compare with parameter        
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                        FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                        FBE_ENCL_CONNECTOR,
                                        connectorIndex,
                                        &primaryPort);

                if ((status == FBE_EDAL_STATUS_OK) && 
                    (((connectorType == FBE_EDAL_PRIMARY_CONNECTOR) && primaryPort) ||
                    ((connectorType ==  FBE_EDAL_EXPANSION_CONNECTOR) && !primaryPort)))
                {
                    status = fbe_edal_getU64(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_CONNECTOR,
                                            connectorIndex,
                                            returnValuePtr);
                    return status;
                }
            }
        }
    }

    return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;

} // end of fbe_edal_getConnectorU64


/*!*************************************************************************
 *                         @fn fbe_edal_getTempSensorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Temperature Sensor Boolean attribute value 
 *      from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive - location/type of attribute is wanted
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getTempSensorBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                           fbe_base_enclosure_attributes_t attribute,
                           fbe_edal_status_type_flag_t statusType,
                           fbe_edal_side_indicator_t sideIndicator,
                           fbe_u32_t index,
                           fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_edal_status_t               status1, status2;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       tempSensorIndex;
    fbe_u32_t                       maxTempSensorCount;
    fbe_u32_t                       individualTempSensorCount = 0;
    fbe_u8_t                        containerIndex, compSideId;
    fbe_bool_t                      tempSensorFound = FALSE;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /*
     * Determine which Temp Sensor is desired
     */
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_TEMP_SENSOR,
                                                &maxTempSensorCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, fbe_edal_getSpecificComponentCount returns 0x%x\n", 
            __FUNCTION__, status);
        return status;
    }

    // For 12G DAE's, just use specific index
    if (statusType == FBE_EDAL_STATUS_12G_CHASSIS_OVERALL)
    {
        status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                  attribute,
                                  FBE_ENCL_TEMP_SENSOR,
                                  FBE_EDAL_STATUS_12G_CHASSIS_OVERALL,
                                  returnValuePtr);
        if (status != FBE_EDAL_STATUS_OK)
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve attribute %d info: 0x%x\n", 
                __FUNCTION__, attribute, status);
        }
        return status;
    }

    for (tempSensorIndex = 0; tempSensorIndex < maxTempSensorCount; tempSensorIndex++) 
    {
        // retrieve necessary Temp Sensor info
        status1 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_ENCL_COMP_CONTAINER_INDEX,
                                FBE_ENCL_TEMP_SENSOR,
                                tempSensorIndex,
                                &containerIndex);
        status2 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                 FBE_ENCL_COMP_SIDE_ID,
                                 FBE_ENCL_TEMP_SENSOR,
                                 tempSensorIndex,
                                 &compSideId);
        if ((status1 != FBE_EDAL_STATUS_OK) ||
            (status2 != FBE_EDAL_STATUS_OK))
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve TempSensor info: 0x%x, 0x%x\n", 
                __FUNCTION__, status1, status2);
            return FBE_EDAL_STATUS_ERROR;
        }

        switch (statusType)
        {
        case FBE_EDAL_STATUS_CHASSIS_OVERALL:
            if ((compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE) &&
                (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
            {
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                          attribute,
                                          FBE_ENCL_TEMP_SENSOR,
                                          tempSensorIndex,
                                          returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_SIDE_OVERALL:
            if ((containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                          attribute,
                                          FBE_ENCL_TEMP_SENSOR,
                                          tempSensorIndex,
                                          returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                if (index == individualTempSensorCount)
                {
                    status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                              attribute,
                                              FBE_ENCL_TEMP_SENSOR,
                                              tempSensorIndex,
                                              returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        case FBE_EDAL_STATUS_CHASSIS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
            {
                if (index == individualTempSensorCount) // Note: chassis components present after both LCC's
                {
                    status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                              attribute,
                                              FBE_ENCL_TEMP_SENSOR,
                                              tempSensorIndex,
                                              returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Unsupported statusType %d\n", 
                __FUNCTION__, statusType);
            return FBE_EDAL_STATUS_ERROR;
            break;
        }

        // exit loop once desired Temp Sensor is found
        if (tempSensorFound)
        {
            enclosure_access_log_debug("EDAL:%s, tempSensorIndex %d, (%d, %d, %d)\n", 
                __FUNCTION__, tempSensorIndex, statusType, sideIndicator, index);
            if (status != FBE_EDAL_STATUS_OK)
            {
                enclosure_access_log_error("EDAL:%s, found, status bad 0x%x (%d, %d, %d)\n", 
                    __FUNCTION__, status, statusType, sideIndicator, index);
            }
            break;
        }

    }   // end of for loop

    // It is not an error if there are no temp sensors in this enclosure
    if (maxTempSensorCount != 0 && !tempSensorFound)
    {
        enclosure_access_log_error("EDAL:%s, not found (%d, %d, %d)\n", 
            __FUNCTION__, statusType, sideIndicator, index);
        status = FBE_EDAL_STATUS_ERROR;
    }

    return status;

} // end of fbe_edal_getTempSensorBool


/*!*************************************************************************
 *                         @fn fbe_edal_getTempSensorU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Temperature Sensor 8 bit unsigned attribute 
 *      value from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param statusType - status type of the containing ESES element(individual or
 *          overall)
 *      @sideIndicator - Side of the containing subenclosure
 *      @index - index of the Temperature Sensor
 *      @param returnValuePtr - pointer to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    04-Nov-2009: Rajesh V. - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getTempSensorU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_status_type_flag_t statusType,
                         fbe_edal_side_indicator_t sideIndicator,
                         fbe_u32_t index,
                         fbe_u8_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_edal_status_t               status1, status2;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       tempSensorIndex;
    fbe_u32_t                       maxTempSensorCount;
    fbe_u32_t                       individualTempSensorCount = 0;
    fbe_u8_t                        containerIndex, compSideId;
    fbe_bool_t                      tempSensorFound = FALSE;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /*
     * Determine which Temp Sensor is desired
     */
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_TEMP_SENSOR,
                                                &maxTempSensorCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, fbe_edal_getSpecificComponentCount returns 0x%x\n", 
            __FUNCTION__, status);
        return status;
    }

    // For 12G DAE's, just use specific index
    if (statusType == FBE_EDAL_STATUS_12G_CHASSIS_OVERALL)
    {
        status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                attribute,
                                FBE_ENCL_TEMP_SENSOR,
                                FBE_EDAL_STATUS_12G_CHASSIS_OVERALL,
                                returnValuePtr);
        if (status != FBE_EDAL_STATUS_OK)
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve attribute %d info: 0x%x\n", 
                __FUNCTION__, attribute, status);
        }
        return status;
    }

    for (tempSensorIndex = 0; tempSensorIndex < maxTempSensorCount; tempSensorIndex++)
    {
        // retrieve necessary Temp Sensor info
        status1 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_ENCL_COMP_CONTAINER_INDEX,
                                FBE_ENCL_TEMP_SENSOR,
                                tempSensorIndex,
                                &containerIndex);
        status2 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                 FBE_ENCL_COMP_SIDE_ID,
                                 FBE_ENCL_TEMP_SENSOR,
                                 tempSensorIndex,
                                 &compSideId);
        if ((status1 != FBE_EDAL_STATUS_OK) ||
            (status2 != FBE_EDAL_STATUS_OK))
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve TempSensor info: 0x%x, 0x%x\n", 
                __FUNCTION__, status1, status2);
            return FBE_EDAL_STATUS_ERROR;
        }

        switch (statusType)
        {
        case FBE_EDAL_STATUS_CHASSIS_OVERALL:
            if ((compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE) &&
                (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
            {
                status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                        attribute,
                                        FBE_ENCL_TEMP_SENSOR,
                                        tempSensorIndex,
                                        returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_SIDE_OVERALL:
            if ((containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                        attribute,
                                        FBE_ENCL_TEMP_SENSOR,
                                        tempSensorIndex,
                                        returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                if (index == individualTempSensorCount)
                {
                    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            tempSensorIndex,
                                            returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        case FBE_EDAL_STATUS_CHASSIS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
            {
                if (index == individualTempSensorCount) // Note: chassis components present after both LCC's
                {
                    status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            tempSensorIndex,
                                            returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Unsupported statusType %d\n", 
                __FUNCTION__, statusType);
            return FBE_EDAL_STATUS_ERROR;
            break;
        }

        // exit loop once desired Temp Sensor is found
        if (tempSensorFound)
        {
            enclosure_access_log_debug("EDAL:%s, tempSensorIndex %d, (%d, %d, %d)\n", 
                __FUNCTION__, tempSensorIndex, statusType, sideIndicator, index);
            if (status != FBE_EDAL_STATUS_OK)
            {
                enclosure_access_log_error("EDAL:%s, found, status bad 0x%x (%d, %d, %d)\n", 
                    __FUNCTION__, status, statusType, sideIndicator, index);
            }
            break;
        }

    }   // end of for loop

    // It is not an error if there are no temp sensors in this enclosure
    if (maxTempSensorCount != 0 && !tempSensorFound)
    {
        enclosure_access_log_error("EDAL:%s, not found (%d, %d, %d)\n", 
            __FUNCTION__, statusType, sideIndicator, index);
        status = FBE_EDAL_STATUS_ERROR;
    }

    return status;

} // end of fbe_edal_getTempSensorU8



/*!*************************************************************************
 *        @fn fbe_edal_getTempSensorU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Temperature Sensor 16 bit unsigned attribute 
 *      value from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param statusType - status type of the containing ESES element(individual or
 *          overall)
 *      @sideIndicator - Side of the containing subenclosure
 *      @index - index of the Temperature Sensor
 *      @param returnValuePtr - pointer to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnDataPtr contents.
 *
 *  HISTORY:
 *    28-Jan-2009: Rajesh V. - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getTempSensorU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_status_type_flag_t statusType,
                         fbe_edal_side_indicator_t sideIndicator,
                         fbe_u32_t index,
                         fbe_u16_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_edal_status_t               status1, status2;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       tempSensorIndex;
    fbe_u32_t                       maxTempSensorCount;
    fbe_u32_t                       individualTempSensorCount = 0;
    fbe_u8_t                        containerIndex, compSideId;
    fbe_bool_t                      tempSensorFound = FALSE;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /*
     * Determine which Temp Sensor is desired
     */
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_TEMP_SENSOR,
                                                &maxTempSensorCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, fbe_edal_getSpecificComponentCount returns 0x%x\n", 
            __FUNCTION__, status);
        return status;
    }

    // For 12G DAE's, just use specific index
    if (statusType == FBE_EDAL_STATUS_12G_CHASSIS_OVERALL)
    {
        status = fbe_edal_getU16(enclosureComponentBlockPtr,
                                attribute,
                                FBE_ENCL_TEMP_SENSOR,
                                FBE_EDAL_STATUS_12G_CHASSIS_OVERALL,
                                returnValuePtr);
        if (status != FBE_EDAL_STATUS_OK)
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve attribute %d info: 0x%x\n", 
                __FUNCTION__, attribute, status);
        }
        return status;
    }

    for (tempSensorIndex = 0; tempSensorIndex < maxTempSensorCount; tempSensorIndex++)
    {
        // retrieve necessary Temp Sensor info
        status1 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_ENCL_COMP_CONTAINER_INDEX,
                                FBE_ENCL_TEMP_SENSOR,
                                tempSensorIndex,
                                &containerIndex);
        status2 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                 FBE_ENCL_COMP_SIDE_ID,
                                 FBE_ENCL_TEMP_SENSOR,
                                 tempSensorIndex,
                                 &compSideId);
        if ((status1 != FBE_EDAL_STATUS_OK) ||
            (status2 != FBE_EDAL_STATUS_OK))
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve TempSensor info: 0x%x, 0x%x\n", 
                __FUNCTION__, status1, status2);
            return FBE_EDAL_STATUS_ERROR;
        }

        switch (statusType)
        {
        case FBE_EDAL_STATUS_CHASSIS_OVERALL:
            if ((compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE) &&
                (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
            {
                status = fbe_edal_getU16(enclosureComponentBlockPtr,
                                        attribute,
                                        FBE_ENCL_TEMP_SENSOR,
                                        tempSensorIndex,
                                        returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_SIDE_OVERALL:
            if ((containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                status = fbe_edal_getU16(enclosureComponentBlockPtr,
                                        attribute,
                                        FBE_ENCL_TEMP_SENSOR,
                                        tempSensorIndex,
                                        returnValuePtr);
                tempSensorFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                if (index == individualTempSensorCount)
                {
                    status = fbe_edal_getU16(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            tempSensorIndex,
                                            returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        case FBE_EDAL_STATUS_CHASSIS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
            {
                if (index == individualTempSensorCount) // Note: chassis components present after both LCC's
                {
                    status = fbe_edal_getU16(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_TEMP_SENSOR,
                                            tempSensorIndex,
                                            returnValuePtr);
                    tempSensorFound = TRUE;
                }
                else
                {
                    individualTempSensorCount++;
                }
            }
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Unsupported statusType %d\n", 
                __FUNCTION__, statusType);
            return FBE_EDAL_STATUS_ERROR;
            break;
        }

        // exit loop once desired Temp Sensor is found
        if (tempSensorFound)
        {
            enclosure_access_log_debug("EDAL:%s, tempSensorIndex %d, (%d, %d, %d)\n", 
                __FUNCTION__, tempSensorIndex, statusType, sideIndicator, index);
            if (status != FBE_EDAL_STATUS_OK)
            {
                enclosure_access_log_error("EDAL:%s, found, status bad 0x%x (%d, %d, %d)\n", 
                    __FUNCTION__, status, statusType, sideIndicator, index);
            }
            break;
        }

    }   // end of for loop

    // It is not an error if there are no temp sensors in this enclosure
    if (maxTempSensorCount != 0 && !tempSensorFound)
    {
        enclosure_access_log_error("EDAL:%s, not found (%d, %d, %d)\n", 
            __FUNCTION__, statusType, sideIndicator, index);
        status = FBE_EDAL_STATUS_ERROR;
    }

    return status;

}   // end of fbe_edal_getTempSensorU16


/*!*************************************************************************
 *                         @fn fbe_edal_getCoolingBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get a Cooling Boolean attribute value 
 *      from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive - location/type of attribute is wanted
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    27-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getCoolingBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                        fbe_base_enclosure_attributes_t attribute,
                        fbe_edal_status_type_flag_t statusType,
                        fbe_edal_side_indicator_t sideIndicator,
                        fbe_u32_t index,
                        fbe_bool_t *returnValuePtr)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_edal_status_t               status1, status2;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       coolingIndex;
    fbe_u32_t                       maxCoolingCount;
    fbe_u32_t                       individualCoolingCount = 0;
    fbe_u8_t                        containerIndex, compSideId;
    fbe_bool_t                      coolingFound = FALSE;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /*
     * Determine which Cooling is desired
     */
    status = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                                FBE_ENCL_COOLING_COMPONENT,
                                                &maxCoolingCount);
    if (status != FBE_EDAL_STATUS_OK)
    {
        enclosure_access_log_error("EDAL:%s, fbe_edal_getSpecificComponentCount returns 0x%x\n", 
            __FUNCTION__, status);
        return status;
    }
    for (coolingIndex = 0; coolingIndex < maxCoolingCount; coolingIndex++)
    {
        // retrieve necessary Cooling info
        status1 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_ENCL_COMP_CONTAINER_INDEX,
                                FBE_ENCL_COOLING_COMPONENT,
                                coolingIndex,
                                &containerIndex);
        status2 = fbe_edal_getU8(enclosureComponentBlockPtr,
                                 FBE_ENCL_COMP_SIDE_ID,
                                 FBE_ENCL_COOLING_COMPONENT,
                                 coolingIndex,
                                 &compSideId);
        if ((status1!= FBE_EDAL_STATUS_OK) ||
            (status2 != FBE_EDAL_STATUS_OK))
        {
            enclosure_access_log_error("EDAL:%s, could not retrieve Cooling info: 0x%x, 0x%x\n", 
                __FUNCTION__, status1, status2);
            return FBE_EDAL_STATUS_ERROR;
        }


        switch (statusType)
        {
        case FBE_EDAL_STATUS_CHASSIS_OVERALL:
            if ((compSideId == FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE) &&
                (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
            {
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                          attribute,
                                          FBE_ENCL_COOLING_COMPONENT,
                                          coolingIndex,
                                          returnValuePtr);
                coolingFound = TRUE;
            }
            break;  

        case FBE_EDAL_STATUS_SIDE_OVERALL:
            if ((containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                          attribute,
                                          FBE_ENCL_COOLING_COMPONENT,
                                          coolingIndex,
                                          returnValuePtr);
                coolingFound = TRUE;
            }
            break;

        case FBE_EDAL_STATUS_INDIVIDUAL:
            if ((containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
                (compSideId == sideIndicator))
            {
                if (index == individualCoolingCount)
                {
                    status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                              attribute,
                                              FBE_ENCL_COOLING_COMPONENT,
                                              coolingIndex,
                                              returnValuePtr);
                    coolingFound = TRUE;
                }
                else
                {
                    individualCoolingCount++;
                }
            }
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Unsupported statusType %d\n", 
                __FUNCTION__, statusType);
            return FBE_EDAL_STATUS_ERROR;
            break;
        }

        // exit loop once desired Cooling is found
        if (coolingFound)
        {
            enclosure_access_log_debug("EDAL:%s, coolingIndex %d, (%d, %d, %d)\n", 
                __FUNCTION__, coolingIndex, statusType, sideIndicator, index);
            if (status != FBE_EDAL_STATUS_OK)
            {
                enclosure_access_log_error("EDAL:%s, found, status bad 0x%x (%d, %d, %d)\n", 
                    __FUNCTION__, status, statusType, sideIndicator, index);
            }
            break;
        }

    }   // end of for loop

    // It is not an error if there are no cooling components in this enclosure
    if (maxCoolingCount != 0 && !coolingFound)
    {
        enclosure_access_log_error("EDAL:%s, not found (%d, %d, %d)\n", 
            __FUNCTION__, statusType, sideIndicator, index);
        status = FBE_EDAL_STATUS_ERROR;
    }

    return status;

} // end of fbe_edal_getCoolingBool



/*!****************************************************************************
 * @fn      fbe_edal_convertDisplayStringToIntegers
 *
 * @brief
 * This function converts two display characters (ASCII) into a
 * display value (integer).
 *
 * @param displayChar0 - upper digit to display (tens place)
 * @param displayChar1 - lower digit to display (ones place)
 * @param displayValue - pointer to a u32 which will hold the result.
 *
 * @return
 *  FBE_EDAL_STATUS_OK if the digits are both in range (i.e. ASCII 0-9 inclusive).
 *
 * HISTORY
 *
 *****************************************************************************/
fbe_edal_status_t
fbe_edal_convertDisplayStringToIntegers(fbe_u8_t displayChar0,
                                        fbe_u8_t displayChar1,
                                        fbe_u32_t *displayValue)
{

    if ((displayChar0 < FBE_EDAL_CHAR_0) || (displayChar0 > FBE_EDAL_CHAR_9) ||
        (displayChar1 < FBE_EDAL_CHAR_0) || (displayChar1 > FBE_EDAL_CHAR_9))
    {
        return FBE_EDAL_STATUS_ERROR;
    }

    *displayValue = (displayChar0 - FBE_EDAL_CHAR_0) * 10;
    *displayValue += (displayChar1 - FBE_EDAL_CHAR_0);

    return FBE_EDAL_STATUS_OK;

} // end of fbe_edal_convertDisplayStringToIntegers   

/*!*************************************************************************
 *                         @fn fbe_edal_getDisplayValue
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the Display Character value 
 *      from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param displayType - enum for type of Display (Bus or Enclosure)
 *      @param returnValuePtr - pointer to a U32 to pass value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the returnValuePtr contents.
 *
 *  HISTORY:
 *    18-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getDisplayValue(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_edal_display_type_t displayType,
                         fbe_u32_t *returnValuePtr)
{
    fbe_edal_status_t                status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr = NULL;
    fbe_u8_t                        displayValue0, displayValue1;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }
    if (returnValuePtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null returnValuePtr\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_RETURN_PTR;
    }

    /*
     * Determine which Display is desired
     */
    switch (displayType)
    {
    case FBE_EDAL_DISPLAY_BUS:
        status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER_STATUS,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_0,
                                &displayValue0);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER_STATUS,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_1,
                                &displayValue1);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_convertDisplayStringToIntegers(displayValue0,
                                                         displayValue1,
                                                         returnValuePtr);
        break;

    case FBE_EDAL_DISPLAY_ENCL:
        status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER_STATUS,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_ENCL_0,
                                &displayValue0);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_convertDisplayStringToIntegers(FBE_EDAL_CHAR_0,
                                                         displayValue0,
                                                         returnValuePtr);
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Unsupported Display type %d\n", 
            __FUNCTION__, displayType);
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

} // end of fbe_edal_getDisplayValue



/*!*************************************************************************
 *                         @fn fbe_edal_getSpecificComponent
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function will retrieve pointer to a specific Component.
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *      @param componentType - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param encl_component - pointer to the component that was specified
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    14-Nov-2008: Joe Perry - Created
 *    17-Mar-2010: PHE - Modified.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getSpecificComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t index,
                              void **encl_component)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;

    status = fbe_base_enclosure_access_getSpecificComponent(enclosureComponentBlockPtr,
                                                            componentType,
                                                            index,
                                                            encl_component);

    return status;

}   // end of fbe_edal_getSpecificComponent

/*!*************************************************************************
 *                         @fn fbe_edal_getBoolMatchingComponent
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function will retrieve pointer to a specific Component with matching.
 *  Boolean attribute.
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *      @param attribute - attribute to check
 *      @param componentType - specific component to get (PS, Drive, ...)
 *      @param check_value - expected value
 *      @param encl_component - pointer to the component that was specified
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    22-Mar-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getBoolMatchingComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_enclosure_component_types_t componentType,
                                  fbe_bool_t check_value,
                                  void **encl_component)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       index;
    fbe_bool_t                      edal_value;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    // traverse through all blocks
    do 
    {
        // check local block for the specified type
        status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(componentBlockPtr,
                                                       &componentTypeDataPtr,
                                                       componentType);

        // found the type, now search for matching attribute
        if (status == FBE_EDAL_STATUS_OK)
        {
            // going through all components
            for(index = 0; index < componentTypeDataPtr->maxNumberOfComponents; index ++)
            {
                status =  fbe_edal_getBool(componentBlockPtr,
                                        attribute,
                                        componentType,
                                        index,
                                        &edal_value);
                if ((status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
                {
                    // get the component pointer
                    status = enclosure_access_getSpecifiedComponentData(componentTypeDataPtr, 
                                                            index,
                                                            encl_component);
                    break; /* found match*/
                }
            }
            if ((index < componentTypeDataPtr->maxNumberOfComponents)&&
                (status == FBE_EDAL_STATUS_OK))
            {
                // found our match, let's break out of the while loop
                break;
            }
        }
        // check if there's a next block
        componentBlockPtr = componentBlockPtr->pNextBlock;
    }
    while (componentBlockPtr!=NULL);

    // if we didn't find what we are looking for, make sure return the correct status
    if (componentBlockPtr==NULL)
    {
        *encl_component = NULL;
        status = FBE_EDAL_STATUS_COMPONENT_NOT_FOUND;
    }

    return status;

} // end of fbe_edal_getBoolMatchingComponent

/*!*************************************************************************
 *                         @fn fbe_edal_getU8MatchingComponent
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function will retrieve pointer to a specific Component with matching.
 *  U8 attribute.
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *      @param attribute - attribute to check
 *      @param componentType - specific component to get (PS, Drive, ...)
 *      @param check_value - expected value
 *      @param encl_component - pointer to the component that was specified
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    22-Mar-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getU8MatchingComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u8_t check_value,
                                void **encl_component)
{
    fbe_edal_status_t               status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_u32_t                       index;
    fbe_u8_t                        edal_value;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }


    // traverse through all blocks
    do 
    {
        // check local block for the specified type
        status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(componentBlockPtr,
                                                       &componentTypeDataPtr,
                                                       componentType);

        // found the type, now search for matching attribute
        if (status == FBE_EDAL_STATUS_OK)
        {
            // going through all components
            for(index = 0; index < componentTypeDataPtr->maxNumberOfComponents; index ++)
            {
                status =  fbe_edal_getU8(componentBlockPtr,
                                        attribute,
                                        componentType,
                                        index,
                                        &edal_value);
                if ((status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
                {
                    // get the component pointer
                    status = enclosure_access_getSpecifiedComponentData(componentTypeDataPtr, 
                                                            index,
                                                            encl_component);
                    break; /* found match*/
                }
            }
            if ((index < componentTypeDataPtr->maxNumberOfComponents)&&
                (status == FBE_EDAL_STATUS_OK))
            {
                // found our match, let's break out of the while loop
                break;
            }
        }
        // check if there's a next block
        componentBlockPtr = componentBlockPtr->pNextBlock;
    }
    while (componentBlockPtr!=NULL);


    // if we didn't find what we are looking for, make sure return the correct status
    if (componentBlockPtr==NULL)
    {
        *encl_component = NULL;
        status = FBE_EDAL_STATUS_COMPONENT_NOT_FOUND;
    }

    return status;

} // end of fbe_edal_getU8MatchingComponent

/*!*************************************************************************
 *                         @fn fbe_edal_copyBlockData
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function copies all EDAL blocks to the new buffer.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *      @param targetBuffer - pointer to the target buffer
 *      @param bufferSize - target buffer size
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    18-Dec-2008: NC - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_copyBlockData(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                       fbe_u8_t *targetBuffer,
                       fbe_u32_t bufferSize)
{
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_enclosure_component_block_t *targetComponentBlockPtr;
    fbe_u32_t bufferUsed;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    /* assume there's at least one block used */
    bufferUsed = componentBlockPtr->blockSize;
    targetComponentBlockPtr = (fbe_enclosure_component_block_t *)targetBuffer;
    while (componentBlockPtr!=NULL)
    {
        /* copy the current block */
        memcpy(targetComponentBlockPtr, componentBlockPtr, componentBlockPtr->blockSize);

        /* size includes the next block */
        bufferUsed += componentBlockPtr->blockSize;

        // check if there's a next block
        componentBlockPtr = componentBlockPtr->pNextBlock;
        // manipulate the pNextBlock in the target block if there's more on the chain
        if (componentBlockPtr!=NULL)
        {
            /* If we don't have enough space */
            if (bufferUsed>bufferSize)
            {
                /* terminate the chain, so partial data still usable */
                targetComponentBlockPtr->pNextBlock = NULL;
                enclosure_access_log_error("EDAL:%s, insufficient memory, expected 0x%x, actual 0x%x\n", 
                    __FUNCTION__, bufferUsed, bufferSize);
                return FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE;
            }
            else
            {
                targetComponentBlockPtr->pNextBlock = 
                    (fbe_enclosure_component_block_t *)((fbe_u8_t *)targetComponentBlockPtr + 
                                                        componentBlockPtr->blockSize);
                // move the target chain along
                targetComponentBlockPtr = targetComponentBlockPtr->pNextBlock;
            }
        }
    }

    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_copyBlockData


/*!*************************************************************************
 *                         @fn fbe_edal_fixNextBlockPtr
 **************************************************************************
 *  @brief
 *      This function fixes the pointer to the next data block 
 *      when all the EDAL blocks are contiguous.
 *
 *  @param baseCompBlockPtr - pointer to the first EDAL block in the chain. 
 *
 *  @return fbe_edal_status_t - status of the Set operation
 *
 *  @note
 *
 *  @version
 *    21-Apr-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_fixNextBlockPtr(fbe_edal_block_handle_t baseCompBlockPtr)
{
    fbe_enclosure_component_block_t   * localCompBlockPtr = NULL;
    
    localCompBlockPtr = (fbe_enclosure_component_block_t *)baseCompBlockPtr;

    while (localCompBlockPtr->pNextBlock!=NULL)
    {
        localCompBlockPtr->pNextBlock = (fbe_enclosure_component_block_t *)((fbe_u8_t *)localCompBlockPtr + 
                                                        localCompBlockPtr->blockSize);
        localCompBlockPtr = localCompBlockPtr->pNextBlock;
    }

    return FBE_EDAL_STATUS_OK;
}


/*!*************************************************************************
 *                         @fn fbe_edal_copy_backup_data
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function copies all EDAL blocks to the BackUp EDAL.
 *
 *  PARAMETERS:
 *      @param compBlockPtr - pointer to component block(active EDAL)
 *      @param backupCompBlockPtr - pointer to component block(backup EDAL)
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    06-Jan-2010: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_copy_backup_data(fbe_edal_block_handle_t compBlockPtr,
                       fbe_edal_block_handle_t backupCompBlockPtr)                       
{
    fbe_enclosure_component_block_t *componentBlockPtr = NULL;
    fbe_enclosure_component_block_t *backupComponentBlockPtr = NULL;
    fbe_enclosure_component_t * componentDataPtr = NULL;
    fbe_enclosure_component_t * backUpcomponentDataPtr = NULL;

    componentBlockPtr = (fbe_enclosure_component_block_t *) compBlockPtr;
    backupComponentBlockPtr = (fbe_enclosure_component_block_t *) backupCompBlockPtr;

    // validate pointer & buffer
    if (componentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    if (backupComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Backup Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    
    if (componentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, componentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    if (backupComponentBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid BackUp Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, backupComponentBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    /* First Copy generation Count of first block of EDAL */
    backupComponentBlockPtr->generation_count = componentBlockPtr->generation_count;

    while (componentBlockPtr!=NULL && backupComponentBlockPtr!= NULL)
    {    
    
       componentDataPtr = edal_calculateComponentTypeDataStart(componentBlockPtr);
       backUpcomponentDataPtr = edal_calculateComponentTypeDataStart(backupComponentBlockPtr);
     
        /* copy the current block. We should exclude size of header of EDAL and available
            space in EDAL */
        memcpy(backUpcomponentDataPtr, componentDataPtr, 
                              (componentBlockPtr->blockSize -(sizeof(fbe_enclosure_component_block_t) + componentBlockPtr->availableDataSpace)));

        // check if there's a next block
        componentBlockPtr = componentBlockPtr->pNextBlock;
        backupComponentBlockPtr = backupComponentBlockPtr->pNextBlock;        
    }

    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_copy_backup_data

/**************************************************************************
*          fbe_edal_get_fw_target_component_type_attr()                  
***************************************************************************
* DESCRIPTION
*   This function maps fw_target to appropriate 
* component type and attribute
*
* PARAMETERS
*   fw_target (in) - fbe eses target type (used by shim)
*   comp_type (out) - mapped edal component type
*   comp_attr (out) - mapped edal attribute
* 
*RETURN VALUES
*   fbe_edal_status_t - failure when target not found
*
* HISTORY
*   28-Apr-2009  -NCHIU Created
***************************************************************************/
fbe_edal_status_t
fbe_edal_get_fw_target_component_type_attr(fbe_enclosure_fw_target_t target,
                                           fbe_enclosure_component_types_t *comp_type, 
                                           fbe_base_enclosure_attributes_t *comp_attr)
{
    fbe_edal_status_t ret_status = FBE_EDAL_STATUS_OK;

    switch (target)
    {
    case FBE_FW_TARGET_LCC_EXPANDER:
        *comp_type = FBE_ENCL_LCC;
        *comp_attr = FBE_LCC_EXP_FW_INFO;
        break;
    case FBE_FW_TARGET_LCC_BOOT_LOADER:
        *comp_type = FBE_ENCL_LCC;
        *comp_attr = FBE_LCC_BOOT_FW_INFO;
        break;
    case FBE_FW_TARGET_LCC_INIT_STRING:
        *comp_type = FBE_ENCL_LCC;
        *comp_attr = FBE_LCC_INIT_FW_INFO;
        break;
    case FBE_FW_TARGET_LCC_FPGA:
        *comp_type = FBE_ENCL_LCC;
        *comp_attr = FBE_LCC_FPGA_FW_INFO;
        break;
    case FBE_FW_TARGET_LCC_MAIN:
        *comp_type = FBE_ENCL_LCC;
        *comp_attr = FBE_ENCL_COMP_FW_INFO;
        break;
    case FBE_FW_TARGET_PS:
        *comp_type = FBE_ENCL_POWER_SUPPLY;
        *comp_attr = FBE_ENCL_COMP_FW_INFO;
        break;
    case FBE_FW_TARGET_COOLING:
        *comp_type = FBE_ENCL_COOLING_COMPONENT;
        *comp_attr = FBE_ENCL_COMP_FW_INFO;
        break;
    case FBE_FW_TARGET_SPS_PRIMARY:
        *comp_type = FBE_ENCL_SPS;
        *comp_attr = FBE_ENCL_COMP_FW_INFO;
        break;
    case FBE_FW_TARGET_SPS_SECONDARY:
        *comp_type = FBE_ENCL_SPS;
        *comp_attr = FBE_SPS_SECONDARY_FW_INFO;
        break;
    case FBE_FW_TARGET_SPS_BATTERY:
        *comp_type = FBE_ENCL_SPS;
        *comp_attr = FBE_SPS_BATTERY_FW_INFO;
        break;   
    default:
        *comp_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
        *comp_attr = FBE_ENCL_COMP_FW_INFO;
        ret_status = FBE_EDAL_STATUS_ERROR;
        break;
    }

    return (ret_status);
}

/*!*************************************************************************
 *                        @fn fbe_edal_getGenerationCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the Generation Count
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param generation_count - pointer used to return Generation Count
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the generation_count contents.
 *
 *  HISTORY:
 *    06-Jan-2010: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getGenerationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u32_t  *generation_count)
{
    fbe_enclosure_component_block_t *dataPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to enclosure components to get generation count
    dataPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (dataPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, dataPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    *generation_count = dataPtr->generation_count;
    return FBE_EDAL_STATUS_OK;

} // end of fbe_edal_getGenerationCount


/*!*************************************************************************
 *                        @fn fbe_edal_getEdalLocale
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the locale of the edal block
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param locale - pointer used to return edal block locale
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the generation_count contents.
 *
 *  HISTORY:
 *    05-Mar-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_getEdalLocale(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t  *edalLocale)
{
    fbe_enclosure_component_block_t *dataPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to enclosure components to get generation count
    dataPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (dataPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, dataPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    *edalLocale = dataPtr->edalLocale;
    return FBE_EDAL_STATUS_OK;

} // end of fbe_edal_getEdalLocale


/*!*************************************************************************
 *                        @fn fbe_edal_find_first_edal_match_U8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the first matching component index for respective components
 *      from the edal block.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param attribute - attribute for which we want to match index.
 *      @param component - component type for which we will match.
 *      @param start_index - starting index for the components
 *      @param check_value - value to be compare to find match.
 *
 *  RETURN VALUES/ERRORS:
 *      @return matching_index - If success returns matching component index, or Invalid value. 
 *
 *  NOTES:
 *      All callers should be checking the returned value.
 *
 *  HISTORY:
 *    05-Mar-2010: Dipak - Moved the code from fbe_base_enclosure_find_first_edal_match_U8
 *                                      function to here for creating this function.
 *
 **************************************************************************/
fbe_u32_t
fbe_edal_find_first_edal_match_U8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_enclosure_component_types_t component,
                                  fbe_u32_t start_index,
                                  fbe_u8_t check_value)

{
    fbe_u32_t                   component_count, i, matching_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                    edal_value;
    fbe_edal_status_t           edal_status;

    edal_status = 
        fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getU8(enclosureComponentBlockPtr,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 *                        @fn fbe_edal_find_first_edal_match_bool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will get the first matching component index for respective components
 *      from the edal block.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param attribute - attribute for which we want to match index.
 *      @param component - component type for which we will match.
 *      @param start_index - starting index for the components
 *      @param check_value - value to be compare to find match.
 *
 *  RETURN VALUES/ERRORS:
 *      @return matching_index - If success returns matching component index, or Invalid value. 
 *
 *  NOTES:
 *      All callers should be checking the returned value.
 *
 *  HISTORY:
 *    05-Mar-2010: Naizhong - Created
 *
 **************************************************************************/
fbe_u32_t
fbe_edal_find_first_edal_match_bool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_enclosure_component_types_t component,
                                    fbe_u32_t start_index,
                                    fbe_bool_t check_value)
{
    fbe_u32_t                   component_count, i, matching_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t                  edal_value;
    fbe_edal_status_t           edal_status;

    edal_status = 
        fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr,
                                           component,
                                           &component_count);
    /* find the type */
    if (edal_status == FBE_EDAL_STATUS_OK)
    {
        /* let loop through the rest of the components to see if there's a match */
        for (i = start_index; i<component_count; i++)
        {
            edal_status = 
                fbe_edal_getBool(enclosureComponentBlockPtr,
                                 attribute,
                                 component,
                                 i,
                                 &edal_value);
            if ((edal_status == FBE_EDAL_STATUS_OK) && (edal_value == check_value))
            {
                matching_index = i;  /* if we have found match */
                break; /* found */
            }
        }
    }

    return (matching_index);
}

/*!*************************************************************************
 *  @fn fbe_edal_get_component_size
 **************************************************************************
 * @brief
 *      This function returns the component size for the specified 
 *      enclosure type and component type.
 *
 * @param enclosure_type(Input) - the specified enclosure type.
 * @param component_type(Input) - the specified component type.
 * @param number_of_components(Output) - the pointer to the returned number of components.
 *
 * @return  fbe_edal_status_t    
 *
 *  NOTES:
 *
 *  HISTORY:
 *    03-Mar-2010: PHE - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_get_component_size(fbe_enclosure_types_t  enclosure_type, 
                            fbe_enclosure_component_types_t component_type,
                            fbe_u32_t * component_size_p)                       
{
    fbe_edal_status_t status = FBE_EDAL_STATUS_OK;
        
    switch(enclosure_type)
    {
        case FBE_ENCL_PE:
            status = processor_enclosure_get_component_size(component_type, component_size_p);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, unhandled enclosure type %d.\n", 
                __FUNCTION__, enclosure_type);

            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    
    return status;
}
/*!*****************************************************************************
 * @fn fbe_edal_is_pe_component(fbe_enclosure_component_types_t component_type)
 *******************************************************************************
 * @brief
 *  This function used to checked the given component type belongs to Processor
 *  enclosure
 *
 * @param component_type : Component Type
 * 
 * @return fbe_bool_t TRUE : if component of PE type
 *
 * @author
 *  16-July - 2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_bool_t
fbe_edal_is_pe_component(fbe_enclosure_component_types_t component_type)
{
    fbe_bool_t is_pe_component = FALSE;
    
    is_pe_component = processor_enclosure_is_component_type_supported(component_type);
    
    return is_pe_component;
}
// End of file fbe_enclosure_data_access.c
