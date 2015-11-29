/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/*!**************************************************************************
 *  @file fbe_enclosure_data_access_set.c
 ***************************************************************************
 *
 * DESCRIPTION:
 * @brief
 *      This module is a library that provides access functions (set)
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
 *                        @fn fbe_edal_setComponentOverallStatus
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the overall Status for a specific component
 *      This function will go through the chain.
 *
 *  PARAMETERS:
 *      @param enclosurePtr - pointer to some type of enclosure object
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param newStatus - new value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    18-Dec-2008: NC - Created
 *    17-Mar-2010: PHE - Modified for the component type in multiple blocks.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setComponentOverallStatus(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t component,
                                   fbe_u32_t newStatus)
{
    fbe_edal_status_t               status;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_enclosure_component_block_t *localBlockPtr = NULL;


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

    localBlockPtr = componentBlockPtr;

    while(localBlockPtr != NULL)
    {
        status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(localBlockPtr,
                                                            &componentTypeDataPtr,
                                                            component);
    if (status == FBE_EDAL_STATUS_OK)
    {
        componentTypeDataPtr->componentOverallStatus = newStatus;
    }

        localBlockPtr = localBlockPtr->pNextBlock;
    }

    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_setComponentOverallStatus


/*!*************************************************************************
 *  @fn fbe_edal_incrementComponentOverallStateChange
 **************************************************************************
 *  @brief
 *      This function increments componentOverallStateChangeCount
 *      for the specified component type in the EDAL block chain.
 *
 *  @param enclosureComponentBlockPtr - pointer to the first block.
 *  @param componentType - specific component type(PS, Drive, ...)
 *
 *  @return fbe_edal_status_t -
 *
 *  NOTES:
 *
 *  HISTORY:
 *    17-Mar-2010: PHE - Created.
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_incrementComponentOverallStateChange(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t componentType)
{
    fbe_edal_status_t               status;
    fbe_enclosure_component_t       *componentTypeDataPtr = NULL;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_enclosure_component_block_t *localBlockPtr = NULL;

    
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

    localBlockPtr = componentBlockPtr;

    while(localBlockPtr != NULL)
    {
        status = enclosure_access_getSpecifiedComponentTypeDataInLocalBlock(localBlockPtr,
                                                                &componentTypeDataPtr,
                                                                componentType);
        if (status == FBE_EDAL_STATUS_OK)
        {
            componentTypeDataPtr->componentOverallStateChangeCount ++;
        }

        localBlockPtr = localBlockPtr->pNextBlock;
    }

    return FBE_EDAL_STATUS_OK;

}   // end of fbe_edal_incrementComponentOverallStateChange

/*!*************************************************************************
 *                        @fn fbe_edal_updateBlockPointers
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will update the pointers to additional Block pointers
 *      within the enclosure data.  This is need when EDAL data is copied
 *      to another buffer (pointers will point to previous buffer addresses).
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status before
 *      using the enclosureComponentBlockPtr contents.
 *
 *  HISTORY:
 *    17-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_updateBlockPointers(fbe_edal_block_handle_t enclosureComponentBlockPtr)
{
    fbe_enclosure_component_block_t *compBlockPtr;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to Base enclosure to get Base enclosure type
    compBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (compBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, compBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }

    // update the pointers to next block
    while (compBlockPtr->pNextBlock != NULL)
    {
        compBlockPtr->pNextBlock =
            (fbe_enclosure_component_block_t *)((fbe_u8_t *)compBlockPtr + compBlockPtr->blockSize);
        compBlockPtr = compBlockPtr->pNextBlock;
    }
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_updateBlockPointers


/*!*************************************************************************
 *                        @fn fbe_edal_connectorControl
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will update expander phy's "disable" flag status for 
 *      respective connector.
 * 
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param connector_id - Connector ID
 *      @param disable -      action to be perform on  expander phy's "disable" flag
 *                            for respective connector
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of operation
 *
 *  HISTORY:
 *    01-June-2011: Naizhong - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_connectorControl(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t   connector_id,
                          fbe_bool_t disable)
{
    fbe_enclosure_component_block_t *compBlockPtr;
    fbe_edal_status_t      edal_status = FBE_EDAL_STATUS_OK;
    fbe_u32_t              matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t               phy_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t             is_entire_connector = FBE_FALSE;
    fbe_bool_t             is_local = FBE_FALSE;

    // validate pointer & buffer
    if (enclosureComponentBlockPtr == NULL)
    {
        enclosure_access_log_error("EDAL:%s, Null Block Pointer\n", __FUNCTION__);
        return FBE_EDAL_STATUS_NULL_BLK_PTR;
    }
    // cast to Base enclosure to get Base enclosure type
    compBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    if (compBlockPtr->enclosureBlockCanary != EDAL_BLOCK_CANARY_VALUE)
    {
        enclosure_access_log_error("EDAL:%s, Invalid Block Canary, expected 0x%x, actual 0x%x\n", 
            __FUNCTION__, EDAL_BLOCK_CANARY_VALUE, compBlockPtr->enclosureBlockCanary);
        return FBE_EDAL_STATUS_INVALID_BLK_CANARY;
    }


    /* The connectors saved in EDAL is in the order of sides, side A followed by side B.
     * All the local connectors are in the will be in contiguous order.
     * First, find the first local connector index. Other local connectors follow.
     */
    matching_conn_index = 
        fbe_edal_find_first_edal_match_bool(enclosureComponentBlockPtr,
                                            FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                            FBE_ENCL_CONNECTOR,  // Component type
                                            0, //starting index
                                            FBE_TRUE);

    if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        /* There must be expansion port among the local connectors.
         * Second, find the first matching connector id.
         */
        matching_conn_index = 
            fbe_edal_find_first_edal_match_U8(enclosureComponentBlockPtr,
                                            FBE_ENCL_CONNECTOR_ID,  //attribute
                                            FBE_ENCL_CONNECTOR,  // Component type
                                            matching_conn_index, //starting index
                                            connector_id);

        while (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
        {
            // Check if this is an entire connector.
            edal_status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                            FBE_ENCL_COMP_IS_LOCAL,  // Attribute.
                                            FBE_ENCL_CONNECTOR,  // Component type
                                            matching_conn_index,     // Component index.
                                            &is_local);
            if(!EDAL_STAT_OK(edal_status))
            {
                return edal_status;
            }
            if (is_local == FBE_FALSE)
            {
                // we finished search our local side, we've done!
                // let's break out
                break;
            }

            // Check if this is an entire connector.
            edal_status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                            FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute.
                                            FBE_ENCL_CONNECTOR,  // Component type
                                            matching_conn_index,     // Component index.
                                            &is_entire_connector);

            if(!EDAL_STAT_OK(edal_status))
            {
                return edal_status;
            }
            /* skip the whole connector*/
            if (is_entire_connector==FBE_FALSE)
            {
                edal_status = fbe_edal_getU8(enclosureComponentBlockPtr,
                                            FBE_ENCL_CONNECTOR_PHY_INDEX, // Attribute.
                                            FBE_ENCL_CONNECTOR,  // component type.
                                            matching_conn_index,
                                            &phy_index);  // The value to be set to.
                if(!EDAL_STAT_OK(edal_status))
                {
                    return edal_status;
                }

                // The phy index for the entire connector is set to FBE_ENCLOSURE_VALUE_INVALID. 
                // So need to check the phy index here.
                if(phy_index != FBE_ENCLOSURE_VALUE_INVALID)
                {  
                    //Enable the phy whether it is disabled or enabled. If it is already enabled, the state will not be affected in any way.
                    edal_status = fbe_edal_setBool(enclosureComponentBlockPtr,
                                            FBE_ENCL_EXP_PHY_DISABLE,
                                            FBE_ENCL_EXPANDER_PHY,
                                            phy_index,
                                            disable);
                }
            }
            /* Find the next matching connector index */

            matching_conn_index = 
                fbe_edal_find_first_edal_match_U8(enclosureComponentBlockPtr,
                                                FBE_ENCL_CONNECTOR_ID,  //attribute
                                                FBE_ENCL_CONNECTOR,  // Component type
                                                matching_conn_index + 1, //starting index with next connector.
                                                connector_id);
        }// while loop
    }

    return edal_status;

}

/*!*************************************************************************
 *                         @fn fbe_edal_setBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a Boolean attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param newValue - new Boolean attribute value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t index,
                 fbe_bool_t newValue)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setBool_direct(generalDataPtr,
                                         componentBlockPtr->enclosureType,  
                                         attribute,
                                         component,
                                         newValue);
    }
    return status;
} // end of fbe_edal_setBool

/*!*************************************************************************
 *                         @fn fbe_edal_setBool_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a u8 attribute value from an
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
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setBool_direct(fbe_edal_component_data_handle_t generalDataPtr,
                        fbe_enclosure_types_t  enclosureType,  
                        fbe_base_enclosure_attributes_t attribute,
                        fbe_enclosure_component_types_t component,
                        fbe_bool_t newValue)
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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setBool(generalDataPtr,
                                                   attribute,
                                                   component,
                                                   newValue);
            break;

         case FBE_ENCL_PE:
            status = processor_enclosure_access_setBool(generalDataPtr, 
                                                        attribute, 
                                                        component, 
                                                        newValue);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_setBool_direct

/*!*************************************************************************
 *                         @fn fbe_edal_setU8
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a U8 attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param newValue - new U8 attribute value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
               fbe_base_enclosure_attributes_t attribute,
               fbe_enclosure_component_types_t component,
               fbe_u32_t index,
               fbe_u8_t newValue)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setU8_direct(generalDataPtr,
                                       componentBlockPtr->enclosureType,  
                                       attribute,
                                       component,
                                       newValue);
    }
    return status;
} // end of fbe_edal_setU8

/*!*************************************************************************
 *                         @fn fbe_edal_setU8_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a u8 attribute value from an
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
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU8_direct(fbe_edal_component_data_handle_t generalDataPtr,
                      fbe_enclosure_types_t  enclosureType,  
                      fbe_base_enclosure_attributes_t attribute,
                      fbe_enclosure_component_types_t component,
                      fbe_u8_t newValue)

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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setU8(generalDataPtr,
                                             attribute,
                                             component,
                                             newValue);
            break;

        case FBE_ENCL_PE:
           status = processor_enclosure_access_setU8(generalDataPtr, 
                                                     attribute, 
                                                     component, 
                                                     newValue);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_setU8_direct

/*!*************************************************************************
 *                         @fn fbe_edal_setU16
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a U16 attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param newValue - new U16 attribute value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u16_t newValue)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setU16_direct(generalDataPtr,
                                        componentBlockPtr->enclosureType,  
                                        attribute,
                                        component,
                                        newValue);
    }
    return status;
} // end of fbe_edal_setU16


/*!*************************************************************************
 *                         @fn fbe_edal_setU16_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a u16 attribute value from an
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
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU16_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u16_t newValue)
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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setU16(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
            break;

        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;            
    }
    return status;
} // end of fbe_edal_setU16_direct

/*!*************************************************************************
 *                         @fn fbe_edal_setU32
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a U32 attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param newValue - new U32 attribute value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU32(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t newValue)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);

    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setU32_direct(generalDataPtr,
                                         componentBlockPtr->enclosureType,  
                                         attribute,
                                         component,
                                         newValue);
    }
    return status;
} // end of fbe_edal_setU32

/*!*************************************************************************
 *                         @fn fbe_edal_setU32_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a u32 attribute value from an
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
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU32_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u32_t newValue)
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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setU32(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
            break;

        case FBE_ENCL_PE:
           status = processor_enclosure_access_setU32(generalDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      newValue);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;    
            break;
    }
    return status;
} // end of fbe_edal_setU32_direct
/*!*************************************************************************
 *                         @fn fbe_edal_setU64
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a U64 attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param newValue - new U64 attribute value
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    28-Jul-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u64_t newValue)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);
    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setU64_direct(generalDataPtr,
                               componentBlockPtr->enclosureType,  
                               attribute,
                               component,
                               newValue);
    }
    return status;
} // end of fbe_edal_setU64

/*!*************************************************************************
 *                         @fn fbe_edal_setU64_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a u64 attribute value from an
 *      Enclosure Object for a debugger extension
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param newValue - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setU64_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u64_t newValue)
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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setU64(generalDataPtr,
                                              attribute,
                                              component,
                                              newValue);
            break;
    
        case FBE_ENCL_PE:
           status = processor_enclosure_access_setU64(generalDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      newValue);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure type %d is not supported\n", 
                __FUNCTION__, 
                enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }

    return status;
} // end of fbe_edal_setU64_direct


/*!*************************************************************************
 *                         @fn fbe_edal_setBuffer
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
 *      @param returnStringPtr - input string value 
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
fbe_edal_setBuffer(fbe_edal_block_handle_t enclosureComponentBlockPtr,
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
                processor_enclosure_access_setBuffer(generalDataPtr,
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

}   // end of fbe_edal_setBuffer

/*!*************************************************************************
 *                         @fn fbe_edal_setStr
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a Str attribute value in an
 *      Enclosure Object
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
 *      @param index - number of the Drive component to access (drive in slot 5)
 *      @param length - string length to get/copy
 *      @param returnStringPtr - input string value 
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    06-Jan-2008: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setStr(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t length,
                char *returnStringPtr)
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
                                                            component,
                                                            index,
                                                            &generalDataPtr);
    if (status == FBE_EDAL_STATUS_OK)
    {
        status = fbe_edal_setStr_direct(generalDataPtr,
                                        componentBlockPtr->enclosureType,  
                                        attribute,
                                        component,
                                        length,
                                        returnStringPtr);
    }
    return status;
} // end of fbe_edal_setStr


/*!*************************************************************************
 *                         @fn fbe_edal_setStr_direct
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a Str attribute value in an
 *      Enclosure Object.
 *
 *  PARAMETERS:
 *      @param generalDataPtr - pointer to an enclosure data block
 *      @param enclosureType - enum for enclosure type
 *      @param attribute - enum for attribute to get
 *      @param component - specific component to get (PS, Drive, ...)
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
 *    20-May-2009: Nayana Chaudhari - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setStr_direct(fbe_edal_component_data_handle_t generalDataPtr,
                       fbe_enclosure_types_t  enclosureType,  
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_enclosure_component_types_t component,
                       fbe_u32_t length,
                       char * returnStringPtr)
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
        case FBE_ENCL_SAS_VOYAGER_ICM:
        case FBE_ENCL_SAS_VOYAGER_EE:
        case FBE_ENCL_SAS_VIKING_IOSXP:
        case FBE_ENCL_SAS_VIKING_DRVSXP:
        case FBE_ENCL_SAS_CAYENNE_IOSXP:
        case FBE_ENCL_SAS_CAYENNE_DRVSXP:
        case FBE_ENCL_SAS_NAGA_IOSXP:
        case FBE_ENCL_SAS_NAGA_DRVSXP:
        case FBE_ENCL_SAS_PINECONE:
        case FBE_ENCL_SAS_STEELJAW:
        case FBE_ENCL_SAS_RAMHORN:
        case FBE_ENCL_SAS_ANCHO:
        case FBE_ENCL_SAS_TABASCO:
        case FBE_ENCL_SAS_CALYPSO:
        case FBE_ENCL_SAS_MIRANDA:
        case FBE_ENCL_SAS_RHEA:
            status = eses_enclosure_access_setStr(generalDataPtr,
                                                  attribute,
                                                  component,
                                                  length,
                                                  returnStringPtr);
            break;
    
        case FBE_ENCL_PE:
           status = processor_enclosure_access_setStr(generalDataPtr, 
                                                      attribute, 
                                                      component, 
                                                      length,
                                                      returnStringPtr);
           break;
    
        default:
            enclosure_access_log_error("EDAL:%s, Enclosure %d is not supported\n", 
                __FUNCTION__, enclosureType);
            status = FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE;
            break;
    }
    return status;
} // end of fbe_edal_setStr_direct


/*!****************************************************************************
 * @fn      fbe_edal_convertIntegerToDisplayStrings(fbe_u32_t displayValue,
 *
 * @brief
 * This function converts a display value (integer) into two display
 * characters (ASCII).
 *
 * @param displayChar0 - upper digit to display (tens place)
 * @param displayChar1 - lower digit to display (ones place)
 * @param displayValue - pointer to a u32 which will hold the result.
 *
 * @return
 *  FBE_EDAL_STATUS_OK if the value is in range (i.e. 0-99 inclusive).
 *
 * HISTORY
 *
 *****************************************************************************/
fbe_edal_status_t
fbe_edal_convertIntegerToDisplayStrings(fbe_u32_t displayValue,
                                        fbe_u8_t *displayChar0,
                                        fbe_u8_t *displayChar1)
{

    if (displayValue > 99)
    {
        return FBE_EDAL_STATUS_ERROR;
    }

    *displayChar0 = (displayValue / 10) + FBE_EDAL_CHAR_0;
    *displayChar1 = (displayValue - ((displayValue / 10) * 10)) + FBE_EDAL_CHAR_0;

    return FBE_EDAL_STATUS_OK;

} // end of fbe_edal_convertIntegerToDisplayStrings


/*!*************************************************************************
 *                         @fn fbe_edal_setDisplayValue
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the Display Character value 
 *      from an Enclosure Object based on location.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param displayType - enum for type of Display (Bus or Enclosure)
 *      @param numericValue - boolean for whether this is a numeric value (convert)
 *      @param flashDisplay - boolean for whether to flash display
 *      @param newValue - U32 to pass value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    18-Mar-2009: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setDisplayValue(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_edal_display_type_t displayType,
                         fbe_bool_t numericValue,
                         fbe_bool_t flashDisplay,
                         fbe_u32_t newValue)
{
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_component_block_t *componentBlockPtr;
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

    /*
     * Determine which Display is desired
     */
    switch (displayType)
    {
    case FBE_EDAL_DISPLAY_BUS:
        // setup first character of Bus Display (character, mode, flash)
        if (numericValue)
        {
            // convert passed in number to ASCII
            status = fbe_edal_convertIntegerToDisplayStrings(newValue,
                                                             &displayValue0,
                                                             &displayValue1);
            if (status != FBE_EDAL_STATUS_OK)
            {
                return status;
            }
        }
        else
        {
            // copy passed in value to both displays
            displayValue0 = newValue;
            displayValue1 = newValue;
        }
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_0,
                                displayValue0);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_MODE,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_0,
                                SES_DISPLAY_MODE_DISPLAY_CHAR);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_DISPLAY,
                                  FBE_EDAL_DISPLAY_BUS_0,
                                  flashDisplay);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }

        // setup second character of Bus Display (character, mode, flash)
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_1,
                                displayValue1);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_MODE,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_BUS_1,
                                SES_DISPLAY_MODE_DISPLAY_CHAR);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_DISPLAY,
                                  FBE_EDAL_DISPLAY_BUS_1,
                                  flashDisplay);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        break;

    case FBE_EDAL_DISPLAY_ENCL:
        // setup character of Enclosure Display (character, mode, flash)
        if (numericValue)
        {
            // convert passed in number to ASCII
            status = fbe_edal_convertIntegerToDisplayStrings(newValue,
                                                             &displayValue0,
                                                             &displayValue1);
            if (status != FBE_EDAL_STATUS_OK)
            {
                return status;
            }
        }
        else
        {
            // copy passed in value to both displays
            displayValue1 = newValue;
        }

        // Temporary trace to track down DIMS 239447
        if (displayValue1 == 0)
        {
            enclosure_access_log_error("EDAL:%s, DISPLAY_ERROR setDisplayValue displayValue1 is 0x%x and should be an ASCII char\n", 
                __FUNCTION__, displayValue1);
            // We will continue to run as though nothing happened. 
        }
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_CHARACTER,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_ENCL_0,
                                displayValue1);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setU8(enclosureComponentBlockPtr,
                                FBE_DISPLAY_MODE,
                                FBE_ENCL_DISPLAY,
                                FBE_EDAL_DISPLAY_ENCL_0,
                                SES_DISPLAY_MODE_DISPLAY_CHAR);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        status = fbe_edal_setBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_DISPLAY,
                                  FBE_EDAL_DISPLAY_ENCL_0,
                                  flashDisplay);
        if (status != FBE_EDAL_STATUS_OK)
        {
            return status;
        }
        break;

    default:
        enclosure_access_log_error("EDAL:%s, Unsupported Display type %d\n", 
            __FUNCTION__, displayType);
        return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;
        break;
    }

    return status;

} // end of fbe_edal_setDisplayValue

/*!*************************************************************************
 *                         @fn fbe_edal_incrementOverallStateChange
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will increment the overall State Change count.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of check State Change increment
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    29-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_incrementOverallStateChange(fbe_edal_block_handle_t enclosureComponentBlockPtr)
{
    fbe_enclosure_component_block_t *localBlockPtr;

    localBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;

    while(localBlockPtr != NULL)
    {
        localBlockPtr->overallStateChangeCount++;

        localBlockPtr = localBlockPtr->pNextBlock;
    }

    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_incrementOverallStateChange


/*!*************************************************************************
 *                         @fn fbe_edal_incrementGenrationCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will increment the generation count of EDAL.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of check State Change increment
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    06-Jan-2010: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_incrementGenrationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr)
{
    fbe_enclosure_component_block_t *componentBlockPtr;

    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;
    componentBlockPtr->generation_count++;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_incrementGenrationCount

/*!*************************************************************************
 *                         @fn fbe_edal_clearStateChanges
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will clear all StateChange flags in the Enclosure Data.
 *      This function goes through the chain.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of State Change clearing
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    29-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_clearStateChanges(fbe_edal_block_handle_t enclosureComponentBlockPtr)
{
    fbe_u32_t                       componentIndex, index;
    fbe_u32_t                       overallComponentCount, componentCount;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_ERROR;
    fbe_edal_status_t               returnStatus = FBE_EDAL_STATUS_OK;
    fbe_enclosure_component_types_t componentType;

    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;

    while (componentBlockPtr!=NULL)
    {
        edalStatus = fbe_edal_getNumberOfComponents(enclosureComponentBlockPtr,
                                                    &overallComponentCount);
        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_STATUS_FAILED;
        }
                                                
        for (componentIndex = 0; componentIndex < overallComponentCount; componentIndex++)
        {
            edalStatus = fbe_edal_getComponentType(enclosureComponentBlockPtr, 
                                                   componentIndex,
                                                   &componentType);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                continue;       // skip to the next component
            }
            edalStatus = fbe_edal_getSpecificComponentCount(enclosureComponentBlockPtr, 
                                                        componentType,
                                                        &componentCount);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                returnStatus = edalStatus;      // save bad status
                continue;                              // skip to the next component
            }

            // clear the State Change flag of each individual component
            for (index = 0; index < componentCount; index++)
            {
                edalStatus = fbe_edal_setBool(enclosureComponentBlockPtr,
                                        FBE_ENCL_COMP_STATE_CHANGE,
                                        componentType,
                                        index,
                                        FALSE);
                if (edalStatus != FBE_EDAL_STATUS_OK)
                {
                    returnStatus = edalStatus;      // save bad status
                } 
            }   // for index
        }   // for componentIndex

        // check if there's a next block
        componentBlockPtr = componentBlockPtr->pNextBlock;
    }

    return returnStatus;

} // end of fbe_edal_clearStateChanges


/**************************************************************************
 *                         @fn fbe_edal_checkForWriteData
 **************************************************************************
 *
 *  DESCRIPTION:
 *      @brief
 *      This function will check if there is any data that needs to be
 *      sent to the enclosure (via ESES Control).
 *      This function goes through the chain.
 *      This function updates overall component status.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to component block
 *      @param writeDataAvailable - pointer to boolean to return whether any changes
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    09-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_checkForWriteData(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                           fbe_bool_t *writeDataAvailable)
{
    fbe_enclosure_component_types_t componentType; 
    fbe_u32_t                       componentIndex; 
    fbe_u32_t                       componentCount;
    fbe_enclosure_component_block_t *componentBlockPtr;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_ERROR;
    fbe_bool_t                      writeDataFound;
    fbe_u8_t                        enclPos;

    *writeDataAvailable = FALSE;
    componentBlockPtr = (fbe_enclosure_component_block_t *)enclosureComponentBlockPtr;

    edalStatus = fbe_edal_getU8(enclosureComponentBlockPtr,
                                FBE_ENCL_POSITION,
                                FBE_ENCL_ENCLOSURE,
                                0,
                                &enclPos);
    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        return edalStatus;
    }
//    enclosure_access_log_info("checkForWriteData, Enclosure Position %d\n", enclPos);

    for(componentType = 0; componentType < FBE_ENCL_MAX_COMPONENT_TYPE; componentType++)
    {
        edalStatus = fbe_edal_getSpecificComponentCount(componentBlockPtr, 
                                                        componentType,
                                                       &componentCount);

        if(edalStatus != FBE_EDAL_STATUS_OK) 
        {
            return edalStatus;
            }

            // check if any of this component type changed
            writeDataFound = FALSE;
        for (componentIndex = 0; componentIndex < componentCount; componentIndex ++)
            {
                edalStatus = fbe_edal_getBool(componentBlockPtr,
                                            FBE_ENCL_COMP_WRITE_DATA,
                                        componentType,
                                        componentIndex,
                                            &writeDataFound);
                // if state change, record it & clear flag
                if (edalStatus == FBE_EDAL_STATUS_OK)
                {
                    if (writeDataFound)
                    {
                        *writeDataAvailable = TRUE;
                    enclosure_access_log_debug("EDAL: checkForWriteData, %s index %d has data to be written.\n",
                        enclosure_access_printComponentType(componentType), 
                        componentIndex);

                    fbe_edal_setComponentOverallStatus(componentBlockPtr, 
                                                       componentType, 
                                                       FBE_ENCL_COMPONENT_TYPE_STATUS_WRITE_NEEDED);
                    }
                } 
                else if (edalStatus == FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED)
                {
                    edalStatus = FBE_EDAL_STATUS_OK;           // this is OK, override status
                    break;
                }
                else
                {
                enclosure_access_log_error("EDAL: checkForWriteData, %s index %d has no Write Support, edalStatus 0x%x.\n",
                    enclosure_access_printComponentType(componentType),
                    componentIndex,
                        edalStatus);
                }

            }   // for index
    }// End of - for(componentType = 0; componentType < FBE_ENCL_MAX_COMPONENT_TYPE; componentType++)

    return FBE_EDAL_STATUS_OK;
}
        
    
/*!*************************************************************************
 *                         @fn fbe_edal_setDriveBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set an Drive Boolean attribute value to 
 *      a chained edal blocks based on Drive slot number.
 *      This function will try to match the drive slot from all the chained 
 *      EDAL blocks.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive_slot - slot number of the Drive 
 *      @param newValue - Boolean value to set attribute to
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    19-May-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setDriveBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                      fbe_base_enclosure_attributes_t attribute,
                      fbe_u32_t drive_slot,
                      fbe_bool_t newValue)
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
    status = fbe_edal_setBool_direct(generalDataPtr,
                                    componentBlockPtr->enclosureType,  
                                    attribute,
                                    FBE_ENCL_DRIVE_SLOT,
                                    newValue);

    return status;
} // end of fbe_edal_setDriveBool


/*!*************************************************************************
 *                         @fn fbe_edal_setDrivePhyBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set an Expander PHY Boolean attribute value from an
 *      Enclosure Object based on Drive number.
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param drive - number of the Drive whose PHY attribute is wanted
 *      @param newValue - Boolean value to set attribute to
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Get operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    24-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setDrivePhyBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_u32_t drive,
                         fbe_bool_t newValue)
{
    fbe_u8_t            phyIndex = 0;
    fbe_edal_status_t   status = FBE_EDAL_STATUS_ERROR;
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

    status = fbe_edal_setBool(enclosureComponentBlockPtr,
                              attribute,
                              FBE_ENCL_EXPANDER_PHY,
                              phyIndex,
                              newValue);

    return status;

} // end of fbe_edal_setDrivePhyBool


/*!*************************************************************************
 *                         @fn fbe_edal_setConnectorBool
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set a Connector attribute value from
 *      an Enclosure Object based on connector type (primary,
 *      expansion).
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to an enclosure data block
 *      @param attribute - enum for attribute to get
 *      @param connectorType - type of connector that is wanted
 *      @param returnValuePtr - pointer to a Boolean to pass attribute value in
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *
 *  HISTORY:
 *    5-24-2010: Dan McFarland - Created
 *
 **************************************************************************/
/*
fbe_edal_status_t
fbe_edal_setConnectorBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                          fbe_base_enclosure_attributes_t attribute,
                          fbe_edal_connector_type_t connectorType,
                          fbe_bool_t setValue)
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


    // search through Connectors (connectors on side A are stored first, followed by connectors on side B).
    for (connectorIndex = 0; connectorIndex < connectorCount; connectorIndex++)
    {
        status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                  FBE_ENCL_COMP_IS_LOCAL,  // <<<< Local meaning that it's on this SP, not the peer.
                                  FBE_ENCL_CONNECTOR,
                                  connectorIndex,
                                  &isLocalConnector);

        if((status == FBE_EDAL_STATUS_OK) &&isLocalConnector)
        {
            status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // <<<< 
                                    FBE_ENCL_CONNECTOR,
                                    connectorIndex,
                                    &isEntireConnector);

            if((status == FBE_EDAL_STATUS_OK) && isEntireConnector)
            {
                // get Primary Port flag from Connector & compare with parameter
                status = fbe_edal_getBool(enclosureComponentBlockPtr,
                                    FBE_ENCL_CONNECTOR_PRIMARY_PORT,  // <<<<
                                    FBE_ENCL_CONNECTOR,
                                    connectorIndex,
                                    &primaryPort);            

                if ((status == FBE_EDAL_STATUS_OK) && 
                    (((connectorType == FBE_EDAL_PRIMARY_CONNECTOR) && primaryPort) ||
                    ((connectorType ==  FBE_EDAL_EXPANSION_CONNECTOR) && !primaryPort)))
                {
                    status = fbe_edal_setBool(enclosureComponentBlockPtr,
                                            attribute,
                                            FBE_ENCL_CONNECTOR,
                                            connectorIndex,
                                            setValue);
                    return status;
                }
            }
        }
    }

    return FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND;

}   // end of fbe_edal_setConnectorBool

*/

/*!*************************************************************************
 *                        @fn fbe_edal_setGenerationCount
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the Generation Count
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param generation_count - Generation Count
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    06-Jan-2010: Dipak Patel - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setGenerationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u32_t  generation_count)
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

    dataPtr->generation_count = generation_count;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_setGenerationCount


/*!*************************************************************************
 *                        @fn fbe_edal_setEdalLocale
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will set the locale of the edal block
 *
 *  PARAMETERS:
 *      @param enclosureComponentBlockPtr - pointer to enclosure object block
 *      @param edalLocale - edal block locale
 *
 *  RETURN VALUES/ERRORS:
 *      @return fbe_edal_status_t - status of the Set operation
 *
 *  NOTES:
 *      All callers should be checking the returned status.
 *
 *  HISTORY:
 *    05-Mar-2010: NCHIU - Created
 *
 **************************************************************************/
fbe_edal_status_t
fbe_edal_setEdalLocale(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t  edalLocale)
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

    dataPtr->edalLocale = edalLocale;
    return FBE_EDAL_STATUS_OK;
} // end of fbe_edal_setEdalLocale

// End of file fbe_enclosure_data_access_set.c
