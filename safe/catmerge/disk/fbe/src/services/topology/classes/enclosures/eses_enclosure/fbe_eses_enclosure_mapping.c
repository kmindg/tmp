/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_mapping.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the configuration and mapping related
 *  functions used by the ESES enclosure object. 
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   17-Nov-2008 PHE - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe/fbe_api_board_interface.h"

/**************************
 * GLOBALS 
 **************************/
extern const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_comp_type_hdlr_table[]; 


/**************************
 * FORWARD DECLARATION 
 **************************/
static fbe_enclosure_status_t fbe_eses_enclosure_verify_comp_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                                   fbe_u8_t group_id,
                                                                   fbe_u8_t component_index,
                                                                   fbe_eses_subencl_side_id_t side_id,
                                                                   fbe_u8_t subtype,
                                                                   fbe_u8_t elem);

/*!*************************************************************************
* @fn fbe_eses_enclosure_init_config_info(fbe_eses_enclosure_t * eses_enclosure)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for all the components in the enclosure. 
*
* @param      eses_enclosure - The pointer to the ESES enclosure.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   04-Sep-2008 PHE - Created.
*   15-Sep-2008 hgd - Added code for cooling components.
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_init_config_info(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u8_t                        number_of_components = 0;
    fbe_u8_t                        component_index = 0;
    fbe_enclosure_component_types_t component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_edal_general_comp_handle_t  specific_component_data_ptr = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_edal_status_t               edalStatus;
    fbe_enclosure_status_t          encl_status;    
    const fbe_eses_enclosure_comp_type_hdlr_t * handler;
    fbe_bool_t                      compInserted;
    fbe_u32_t                       port = 0; // for loop.
    
    /********
     * BEGIN
     ********/

    // mark enclosure data as unstable while it is being updated
    eses_enclosure->enclosureConfigBeingUpdated = TRUE;
   
    eses_enclosure->eses_generation_code = FBE_U32_MAX;

   for(port = 0; port < FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT; port ++)
   {
       eses_enclosure->primary_port_entire_connector_index[port] = FBE_ENCLOSURE_VALUE_INVALID;  
   }
    
    // Initialize the EDAL data used by the configuration.
    for (component_type = 0; component_type < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; component_type++)
    {
        status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                          component_type,  // Component type
                                                          &number_of_components); 

        if(status != FBE_STATUS_OK)
        {
            status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                             component_type,
                                             0xFF,
                                             FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                             0);
            // trace out the error if we can't handle the error
            if(!ENCL_STAT_OK(status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "%s can't set FAIL condition, status: 0x%X\n",
                                    __FUNCTION__, status);
            }

            return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
        }        

        for(component_index = 0; component_index < number_of_components; component_index ++)
        {
            status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
            if(status != FBE_STATUS_OK)
            {
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                 component_type,
                                                 component_index,
                                                 FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                 0);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
            }

            /* Init FBE_ENCL_COMP_ELEM_INDEX to SES_ELEM_INDEX_NONE. */
            edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                            FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                            component_type,    // Component type
                                            SES_ELEM_INDEX_NONE); // The value to be set to

            if(!EDAL_STAT_OK(edalStatus))
            {
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                    component_type,
                                                    component_index,
                                                    FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                    0);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }

            /* Init FBE_ENCL_COMP_ADDL_STATUS to FBE_ENCLOSURE_VALUE_INVALID. */
            edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                        FBE_ENCL_COMP_ADDL_STATUS,  // Attribute
                                                        component_type,  // Component type
                                                        FBE_ENCLOSURE_VALUE_INVALID);  // The value to be set to
            if(!EDAL_STAT_OK(edalStatus))
            {
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                    component_type,
                                                    component_index,
                                                    FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                    0);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }

            /* If a subenclosure is not installed, here is all that ESES guarantees:
            * (1) The subenclosure may or may not appear in the config page 
            * (2) If the subenclosure appears in the config page, it will have a blank SUBENCLOSURE VENDOR IDENTIFICATION.
            * (3) If the subenclosure appears, it may or may not have elements 
            * (4) It's possible that some elements will appear and some will not appear
            * (5) If an element appears it will have status Not Installed 
            *
            * So we need to initialize the FBE_ENCL_COMP_INSERTED attribute to FALSE
            * and the configuration related EDAL attributes.
            */

            /* Copy current COMP_INSERTED attribute to COMP_INSERTED_PRIOR_CONFIG */
            edalStatus = eses_enclosure_access_getBool(specific_component_data_ptr,
                                                        FBE_ENCL_COMP_INSERTED,  // Attribute
                                                        component_type,          // Component type
                                                        &compInserted); // The value to be set to

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }

            edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                                        FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG,  // Attribute
                                                        component_type,    // Component type
                                                        compInserted); // The value to be set to

            if(!EDAL_STAT_OK(edalStatus))
            {
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                    component_type,
                                                    component_index,
                                                    FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                    0);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }

            /* Init FBE_ENCL_COMP_INSERTED attribute to FALSE. */
            edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                            FBE_ENCL_COMP_INSERTED,  // Attribute
                                            component_type,    // Component type
                                            FALSE); // The value to be set to

            if(!EDAL_STAT_OK(edalStatus))
            {
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                    component_type,
                                                    component_index,
                                                    FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                    0);
                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                        "%s can't set FAIL condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }

                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }
            
            handler = fbe_eses_enclosure_get_comp_type_hdlr(fbe_eses_enclosure_comp_type_hdlr_table, component_type);
            
            /* NULL pointer for handler shoud not be considered as a fault.
            * It could be possible that we did not implement the status extraction method
            * for the particular element type. 
            */
            if((handler != NULL) && (handler->init_comp_specific_config_info != NULL))
            {
                // Init the component specific config or mapping related attributes.
                encl_status = handler->init_comp_specific_config_info(eses_enclosure, 
                                                       component_type, 
                                                       component_index);
                
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                     component_type,
                                                     component_index,
                                                     FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                     0);
                    // trace out the error if we can't handle the error
                    if(!ENCL_STAT_OK(status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                            "%s can't set FAIL condition, status: 0x%X\n",
                                            __FUNCTION__, status);
                    }

                    return encl_status;
                }
            }

        }// End of - for(i = 0; i < number_of_components; i ++)
    } // End of for loop for all the component types.
 
    return FBE_ENCLOSURE_STATUS_OK;
}// end of fbe_eses_enclosure_init_config_info()

/*!*************************************************************************
* @fn fbe_eses_enclosure_chassis_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
*                                            fbe_enclosure_component_types_t component_type,
*                                            fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific Enclosure (chassis) component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_chassis_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_enclosure_status_t              encl_status;
    
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

   /* Chassis is a subenclosure. 
    * Init FBE_ENCL_COMP_SUB_ENCL_ID attribute to FBE_ENCLOSURE_VALUE_INVALID. 
    */
    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute
                                    component_type,    // Component type
                                    component_index,
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    return encl_status;
}// End of - fbe_eses_enclosure_chassis_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_lcc_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
*                                             fbe_enclosure_component_types_t component_type,
*                                             fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific LCC component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_lcc_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;
    fbe_edal_fw_info_t                  fw_info;

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }
            
   /* The LCC is a subenclosure.
    * Clear FBE_ENCL_COMP_SUB_ENCL_ID attribute. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* invalidate fw_info */
    fw_info.valid = FALSE;
    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_ENCL_COMP_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_LCC_EXP_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_LCC_BOOT_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_LCC_INIT_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_LCC_FPGA_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* clear the side id for the LCC.
    * Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                        FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                        component_type,//Component type.
                                        FBE_ENCLOSURE_VALUE_INVALID); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_COMP_IS_LOCAL attribute to FALSE. It will be updated while processing the config page. */
    edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                                FBE_ENCL_COMP_IS_LOCAL,  // Attribute
                                                component_type,    // Component type
                                                FALSE); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_lcc_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_array_dev_slot_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
*                                            fbe_enclosure_component_types_t component_type,
*                                            fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific Drive component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                                    fbe_enclosure_component_types_t component_type,
                                                    fbe_u8_t component_index)
{
    fbe_enclosure_status_t              encl_status;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

    /* Clear the slot number of the drive. */
    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_DRIVE_SLOT_NUMBER,  // Attribute
                                    component_type,    // Component type
                                    component_index,
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to  
       
    /* Clear the phy index of the drive. */
    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_DRIVE_PHY_INDEX,  // Attribute
                                    component_type,    // Component type
                                    component_index,
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to  
       
    return encl_status;
}// End of - fbe_eses_enclosure_array_dev_slot_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_exp_phy_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific Expander Phy component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ );

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }

    /* Init FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX attribute to SES_ELEM_INDEX_NONE. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,  // Attribute
                                    component_type,    // Component type
                                    SES_ELEM_INDEX_NONE); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_EXP_PHY_ID attribute to FBE_ENCLOSURE_VALUE_INVALID. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_EXP_PHY_ID,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_exp_phy_init_config_info


/*!*************************************************************************
* @fn fbe_eses_enclosure_connector_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific connector component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_connector_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_enclosure_component_types_t component_type,
                                              fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;   

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s entry\n", __FUNCTION__ );


    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }

    /* Init FBE_ENCL_COMP_IS_LOCAL attribute to FALSE. It will be updated while processing the config page. */
    edalStatus = eses_enclosure_access_setBool(specific_component_data_ptr,
                                                FBE_ENCL_COMP_IS_LOCAL,  // Attribute
                                                component_type,    // Component type
                                                FALSE); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_COMP_CONTAINER_INDEX attribute to FBE_ENCLOSURE_CONTAINER_INDEX_INVALID.
     * It will be updated while processing the additional status page for the local connectors. 
     */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                    FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute
                                                    component_type,    // Component type
                                                    FBE_ENCLOSURE_CONTAINER_INDEX_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS attribute to FBE_SAS_ADDRESS_INVALID. 
     * It will be updated while processing the additional status page for the local connectors. 
     */
    edalStatus = eses_enclosure_access_setU64(specific_component_data_ptr,
                                    FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,  // Attribute
                                    component_type,    // Component type
                                    FBE_SAS_ADDRESS_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_CONNECTOR_PHY_INDEX attribute to FBE_ENCLOSURE_VALUE_INVALID. 
     * It will be updated while processing the additional status page for the local connectors. 
     */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_CONNECTOR_PHY_INDEX,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to   

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;

}// End of - fbe_eses_enclosure_connector_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_expander_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific expander component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_expander_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_enclosure_component_types_t component_type,
                                             fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;   
    fbe_u8_t                            number_of_expanders_per_lcc = 0;
    fbe_u8_t                            number_of_lccs = 0;
    fbe_eses_subencl_side_id_t          side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );


    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }
   
    number_of_expanders_per_lcc = fbe_eses_enclosure_get_number_of_expanders_per_lcc(eses_enclosure); 
    number_of_lccs = fbe_eses_enclosure_get_number_of_lccs(eses_enclosure);

    if(component_index < number_of_lccs * number_of_expanders_per_lcc)
    {
        side_id = (component_index / number_of_expanders_per_lcc);
    }                       
    else 
    {
        side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    }   
  
    /* Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                            FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                            component_type,//Component type.
                                            side_id); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Init FBE_ENCL_EXP_SAS_ADDRESS to FBE_SAS_ADDRESS_INVALID. */
    edalStatus = eses_enclosure_access_setU64(specific_component_data_ptr,
                                    FBE_ENCL_EXP_SAS_ADDRESS,  // Attribute
                                    component_type,//Component type.
                                    FBE_SAS_ADDRESS_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
   
    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_expander_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_sps_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific SPS component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   02-Aug-2012     Joe Perry - Created.
*   21-Sep-2012 PHE - Init the subenclId and sideId to FBE_ENCLOSURE_VALUE_INVALID
*                     and invalidate the 3 firmware rev info.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_sps_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;
    fbe_edal_fw_info_t                  fw_info;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }
            
   /* The SPS is a subenclosure.
    * Init the FBE_ENCL_COMP_SUB_ENCL_ID attribute in EDAL 
    * to FBE_ENCLOSURE_VALUE_INVALID. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* invalidate fw_info */
    fw_info.valid = FALSE;
    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_ENCL_COMP_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_SPS_SECONDARY_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_SPS_BATTERY_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

   /* Init the FBE_ENCL_COMP_SIDE_ID attribute in EDAL 
    * to FBE_ENCLOSURE_VALUE_INVALID. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                        FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                        component_type,//Component type.
                                        FBE_ENCLOSURE_VALUE_INVALID); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_sps_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_ps_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific power supply. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_ps_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_enclosure_component_types_t component_type,
                                       fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;  
    fbe_eses_subencl_side_id_t          side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_edal_fw_info_t                  fw_info;
    fbe_u8_t                            num_ps_subelements;
    fbe_u8_t                            container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;

    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );


    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }
    
   /* The power supply is a subenclosure.
    * Clear FBE_ENCL_COMP_SUB_ENCL_ID attribute. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /*
     * Init writable buffer id (for resume prom write) to FBE_ENCLOSURE_VALUE_INVALID.   
     * By doing this, we are able to know whether this PS has the buffer for resume prom write or not
     * (i.e. the PS resume prom is writable or not)
     */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                    FBE_ENCL_BD_BUFFER_ID_WRITEABLE,  // Attribute
                                    component_type,    // Component type
                                    FBE_ENCLOSURE_VALUE_INVALID); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* invalidate fw_info */
    fw_info.valid = FALSE;
    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_ENCL_COMP_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

   /* The component index is the side id for the power supply.
    * Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. 
    */
    if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
    {
        num_ps_subelements = 1;
    }
    side_id = component_index / num_ps_subelements; 
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                        FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                        component_type,//Component type.
                                        side_id); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    if(fbe_eses_enclosure_is_ps_overall_elem_saved(eses_enclosure))
    {
        /* We save overall and individual elements in EDAL for all the enclosure power supplies*/
        container_index = (component_index % num_ps_subelements == 0) ? 
                                FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED : 
                                (side_id * num_ps_subelements);
    }
    else
    {
        /* We don't save the PS overall element for other PS. 
         * So container_index will be FBE_ENCLOSURE_CONTAINER_INDEX_INVALID for others.
         */
        container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    }

    /* Set the FBE_ENCL_COMP_CONTAINER_INDEX attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                component_type,         // Component type.
                                                container_index);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_ps_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_cooling_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific cooling component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL;  
    fbe_u8_t                            number_of_cooling_components_per_ps = 0;
    fbe_u8_t                            number_of_cooling_components_on_chassis = 0;
    fbe_u8_t                            number_of_cooling_components_external = 0;
    fbe_u8_t                            number_of_cooling_components_on_lcc= 0;
    fbe_u8_t                            total_number_of_cooling_component = 0;
    fbe_u8_t                            start_index_of_cooling_component_on_chassis = 0;
    fbe_u8_t                            start_index_of_cooling_component_external = 0;
    fbe_u8_t                            start_index_of_cooling_component_on_lcc = 0;
    fbe_u8_t                            number_of_power_supplies = 0;
    fbe_eses_subencl_side_id_t          side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_enclosure_cooling_subtype_t     cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t                            container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    fbe_edal_fw_info_t                  fw_info;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }

    total_number_of_cooling_component = fbe_eses_enclosure_get_number_of_cooling_components(eses_enclosure);
    number_of_cooling_components_per_ps = fbe_eses_enclosure_get_number_of_cooling_components_per_ps(eses_enclosure); 
    number_of_cooling_components_on_chassis = fbe_eses_enclosure_get_number_of_cooling_components_on_chassis(eses_enclosure);
    number_of_cooling_components_external = fbe_eses_enclosure_get_number_of_cooling_components_external(eses_enclosure);
    number_of_cooling_components_on_lcc = fbe_eses_enclosure_get_number_of_cooling_components_on_lcc(eses_enclosure);
    number_of_power_supplies = fbe_eses_enclosure_get_number_of_power_supplies(eses_enclosure);

    start_index_of_cooling_component_on_chassis = number_of_power_supplies * number_of_cooling_components_per_ps;
    start_index_of_cooling_component_external = start_index_of_cooling_component_on_chassis + 
                                                    number_of_cooling_components_on_chassis;
    start_index_of_cooling_component_on_lcc = start_index_of_cooling_component_external + 
                                                    number_of_cooling_components_external*FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT;
    if(component_index < start_index_of_cooling_component_on_chassis)
    {
        // This cooling element is inside of the power supply. 
        cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_PS_INTERNAL;
        side_id = (component_index / number_of_cooling_components_per_ps);
        container_index = (component_index % number_of_cooling_components_per_ps == 0) ? 
                            FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED : 
                            (side_id * number_of_cooling_components_per_ps);
    }                       
    else if(component_index < start_index_of_cooling_component_external)                              
    {
        // This cooling element is on the chassis.
        cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_CHASSIS;
        side_id = FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE;
        container_index = ((component_index - start_index_of_cooling_component_on_chassis) %
                            number_of_cooling_components_on_chassis == 0)?
                            FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED :
                            start_index_of_cooling_component_on_chassis;

    } 
    else if(component_index < start_index_of_cooling_component_on_lcc)                              
    {
        // This cooling element is external.
        cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_STANDALONE;
        side_id = (component_index - start_index_of_cooling_component_external)/
                    (FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT);
        container_index = (((component_index - start_index_of_cooling_component_external) %
                            FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT) == 0)?
                            FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED :
                            (component_index - 1);  // only one overall, one inidividual

    } 
    else if(component_index < total_number_of_cooling_component)                              
    {
        // This cooling element in LCC sub encll.
        cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL;
        // Will set it later with LCC side ID in fbe_eses_enclosure_parse_subencl_descs()
        side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
        container_index = (((component_index - start_index_of_cooling_component_on_lcc) %
                            number_of_cooling_components_on_lcc) == 0)?
                            FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED :
                            start_index_of_cooling_component_on_lcc;  // only one overall, one inidividual

    } 
    else
    {
        cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID; 
        side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
        container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    }   
  
    /* Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                            FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                            component_type,//Component type.
                                            side_id); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Set the FBE_ENCL_COMP_SUBTYPE attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_SUBTYPE, //Attribute.
                                                component_type,//Component type.
                                                cooling_subtype);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Set the FBE_ENCL_COMP_CONTAINER_INDEX attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                component_type,         // Component type.
                                                container_index);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* invalidate fw_info */
    fw_info.valid = FALSE;
    edalStatus = eses_enclosure_access_setStr(specific_component_data_ptr,
                                    FBE_ENCL_COMP_FW_INFO,  // Attribute
                                    component_type,    // Component type
                                    sizeof(fw_info),
                                    (char *)&fw_info); // The value to be set to

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

   /* cooling could be a subenclosure. 
    * Init FBE_ENCL_COMP_SUB_ENCL_ID attribute to FBE_ENCLOSURE_VALUE_INVALID. 
    */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute.
                                                component_type,         // Component type.
                                                FBE_ENCLOSURE_VALUE_INVALID);
    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_cooling_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_temp_sensor_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific temp sensor component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                                fbe_enclosure_component_types_t component_type,
                                                fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL; 
    fbe_u8_t                            number_of_temp_sensors_per_lcc = 0;
    fbe_u8_t                            number_of_temp_sensors_on_chassis = 0;
    fbe_u8_t                            number_of_lccs_with_temp_sensor = 0;
    fbe_eses_subencl_side_id_t          side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_enclosure_temp_sensor_subtype_t temp_sensor_subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_INVALID;
    fbe_u8_t                            container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }
   
    number_of_temp_sensors_per_lcc = fbe_eses_enclosure_get_number_of_temp_sensors_per_lcc(eses_enclosure);  
    number_of_temp_sensors_on_chassis = fbe_eses_enclosure_get_number_of_temp_sensors_on_chassis(eses_enclosure);
    number_of_lccs_with_temp_sensor = fbe_eses_enclosure_get_number_of_lccs_with_temp_sensor(eses_enclosure);


    /* 
     * We organize the temp sensor info in the edal block in two
     * groups.  The first group is for the temp sensors on the LCCs. 
     * The second group is for the chassis.  The LCCs are assumed to 
     * be distributed in two groups.  The first 
     */

    if(component_index < number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc)
    {
        // This temp sensor element is on the LCC. 
        temp_sensor_subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_LCC;

        if (number_of_lccs_with_temp_sensor == 1)
        {
            // side id will be initialized later after we know the side ID
            side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
            container_index = (component_index % number_of_temp_sensors_per_lcc == 0) ? 
                                FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED : 
                                0;
        }
        else
        {
            side_id = (component_index / number_of_temp_sensors_per_lcc);
            container_index = (component_index % number_of_temp_sensors_per_lcc == 0) ? 
                                FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED : 
                                (side_id * number_of_temp_sensors_per_lcc);
        }
    }                       
    else if(component_index < number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc + 
                       number_of_temp_sensors_on_chassis)
    {
        // This temp sensor element is on the chassis.
        temp_sensor_subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_CHASSIS;
        side_id = FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE;
        container_index = (component_index == number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc) ? 
                            FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED : 
                            (number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc);      
    } 
    else
    {
        temp_sensor_subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_INVALID;        
        side_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
        container_index = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    }
  
    /* Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                            FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                            component_type,//Component type.
                                            side_id); // side_id

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Set the FBE_ENCL_COMP_SUBTYPE attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_SUBTYPE, //Attribute.
                                                component_type,//Component type.
                                                temp_sensor_subtype);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    /* Set the FBE_ENCL_COMP_CONTAINER_INDEX attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                component_type,         // Component type.
                                                container_index);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_temp_sensor_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_display_init_config_info(
*                   fbe_eses_enclosure_t * eses_enclosure,
*                   fbe_enclosure_component_types_t component_type,
*                   fbe_u8_t component_index)
***************************************************************************
* @brief
*       This function initializes the configuration and mapping information
*       for the specific display component. 
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  component_type - The EDAL component type.
* @param  component_index - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   12-Mar-2009 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_display_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index)
{
    fbe_status_t                        status;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL; 
    fbe_u8_t                            number_of_two_digit_displays_per_lcc = 0;
    fbe_u8_t                            number_of_one_digit_displays_per_lcc = 0;
    fbe_enclosure_display_subtype_t     display_subtype = FBE_ENCL_DISPLAY_SUBTYPE_INVALID;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s entry\n", __FUNCTION__ );

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type.
                                                        component_index, // Component index.
                                                        &specific_component_data_ptr);
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    }    

    number_of_two_digit_displays_per_lcc = fbe_eses_enclosure_get_number_of_two_digit_displays(eses_enclosure);
    number_of_one_digit_displays_per_lcc = fbe_eses_enclosure_get_number_of_one_digit_displays(eses_enclosure);
                     
    if(component_index < number_of_two_digit_displays_per_lcc * FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY)
    {
        display_subtype = FBE_ENCL_DISPLAY_SUBTYPE_TWO_DIGIT;
    }
    else if(component_index < number_of_two_digit_displays_per_lcc * FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY +
                              number_of_one_digit_displays_per_lcc * FBE_ESES_NUM_ELEMS_PER_ONE_DIGIT_DISPLAY)
    {
        display_subtype = FBE_ENCL_DISPLAY_SUBTYPE_ONE_DIGIT;
    }
    else
    {
        display_subtype = FBE_ENCL_DISPLAY_SUBTYPE_INVALID;
    }
    
    /* Set FBE_ENCL_COMP_SUBTYPE attribute in EDAL. */
    edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                        FBE_ENCL_COMP_SUBTYPE, //Attribute.
                                        component_type,//Component type.
                                        display_subtype);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_display_init_config_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_next_available_phy_index()                  
***************************************************************************
* @brief
*       This function gets the phy index which is available to use.
*       It should be called after the ESES configuration page is processed.
*
* @param    sas_viper_enclosure (Input) - The pointer to the sas viper enclosure.
*
* @return   fbe_u8_t 
*       value - the unused phy index is found. 
*       FBE_ENCLOSURE_VALUE_INVALID - no available.
*
* NOTES
*
* HISTORY
*  09-Sep-2008 PHE - Created.
*  22-May-2009 NC  - Changed to only for phy_index
***************************************************************************/
fbe_u8_t fbe_eses_enclosure_get_next_available_phy_index(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u8_t component_index ; 

    /**********
     * BEGIN
     **********/
    component_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXP_PHY_ID,  //attribute
                                                    FBE_ENCL_EXPANDER_PHY,  // Component type
                                                    0, //starting index
                                                    FBE_ENCLOSURE_VALUE_INVALID);   //unused

    return component_index; 
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_next_available_lcc_index()                  
***************************************************************************
* @brief
*       This function gets the lcc index which is available to use.
*
* @param    eses_enclosure (Input) - The pointer to the eses enclosure.
*
* @return   fbe_u8_t 
*       value - the unused lcc index is found. 
*       FBE_ENCLOSURE_VALUE_INVALID - no available.
*
* NOTES
*
* HISTORY
*  29-July-2009 NC  - created
***************************************************************************/
fbe_u8_t fbe_eses_enclosure_get_next_available_lcc_index(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u8_t component_index ; 

    /**********
     * BEGIN
     **********/
    component_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_SUB_ENCL_ID,  //attribute
                                                    FBE_ENCL_LCC,  // Component type
                                                    0, //starting index
                                                    FBE_ENCLOSURE_VALUE_INVALID);   //unused

    return component_index; 
}
  

/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_index_to_subencl_id(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t elem_index,
*                                              fbe_u8_t *side_id_p)
***************************************************************************
* @brief
*       This routine gets the subenclosure id based on the element index. It should be 
*       called after parsing the type descriptors in the configuration page.
*
* @param      eses_enclosure - The pointer to the enclosure.
* @param      elem_index - The element index of the component.
* @param      side_id_p (out) - The pointer to the returned side id.
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the subenclosure id is found.
*       otherwise - the subenclosure id is not found.
*
* NOTES
*
* HISTORY
*   10-Nov-2008 PHE - Created.
****************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_subencl_id(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t elem_index,
                                                                fbe_u8_t *subencl_id_p)
{
    fbe_u8_t group_id = FBE_ENCLOSURE_VALUE_INVALID;  
    fbe_eses_elem_group_t *elem_group = NULL;

    /*******
     * BEGIN
     *******/
    

    group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index); 

    if(group_id >= fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_ERROR,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s, invalid group_id: %d, elem_index: %d.\n",                          
                          __FUNCTION__, group_id, elem_index);  

        return FBE_ENCLOSURE_STATUS_MAPPING_FAILED; 
    }      
 
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    *subencl_id_p = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);
    return FBE_ENCLOSURE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_component_type_subencl_id_to_side_id(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t subencl_id,
*                                              fbe_u8_t *side_id_p)
***************************************************************************
* @brief
*       This routine gets the side id based on the component type of the subenclosure 
*       and the subenclosure id. It should be called after parsing the subenclosure 
*       descriptors in the configuration page.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   component_type - The component type of the subenclosure which contains the element.
* @param   subencl_id - The subenclosure id.
* @param   side_id_p (out) - The pointer to the returned side id.
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the component side id is found.
*       otherwise - the component side id is not found or not applicable.
*
* NOTES
*   If the side id concept is not applicable to the subenclosure, for example, the chassis, the
*   side id will be 0.
*
* HISTORY
*   10-Nov-2008 PHE - Created.
****************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_component_type_subencl_id_to_side_id(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_enclosure_component_types_t component_type,
                                                                fbe_u8_t subencl_id,
                                                                fbe_u8_t *side_id_p)
{
    fbe_u32_t                matching_index;
    fbe_enclosure_status_t   encl_status = FBE_ENCLOSURE_STATUS_OK;

    /*******
     * BEGIN
     *******/    
    // Initialize to FBE_ENCLOSURE_VALUE_INVALID.
    *side_id_p = FBE_ENCLOSURE_VALUE_INVALID;

    switch(component_type)
    {
        case FBE_ENCL_ENCLOSURE:
            matching_index = 
            fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SUB_ENCL_ID,  //attribute
                                                        component_type,  // Component type
                                                        0, //starting index
                                                        subencl_id);


            if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
            {
               /*
                * The component is found. 
                */
                *side_id_p = FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE;
            }
            else
            {
                // Do not ktrace because it is normal that we can not find the subenclosure id with the specified component type.
                // The subenclosure id could be for some other component type.
                encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;                  
            }
            break;

        case FBE_ENCL_LCC:
        case FBE_ENCL_POWER_SUPPLY:
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_SPS:
            matching_index = 
            fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SUB_ENCL_ID,  //attribute
                                                        component_type,  // Component type
                                                        0, //starting index
                                                        subencl_id);


            if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
            {
                /*
                 * The component is found. Get the side id from thecomponent index.
                 */
                encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *) eses_enclosure,
                                              FBE_ENCL_COMP_SIDE_ID,
                                              component_type,
                                              matching_index,
                                              side_id_p);
                if(!ENCL_STAT_OK(encl_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_ERROR,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s, failed to access side id for component_type: %s index: %d.\n",                          
                          __FUNCTION__, enclosure_access_printComponentType(component_type),
                          matching_index);
                    
                    encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
                }               
            }
            else
            {
                // Do not ktrace because it is normal that we can not find the subenclosure id with the specified component type.
                // The subenclosure id could be for some other component type.
                encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;   
            }
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, not needed in EDAL, component_type: %s, subencl_id: %d.\n", 
                              __FUNCTION__, enclosure_access_printComponentType(component_type),
                              subencl_id);  
            
            encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            break;  
    }// End of - switch(component_type)

    return encl_status;
}// End of function fbe_eses_enclosure_component_type_subencl_id_to_side_id

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_local_lcc_side_id(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t *local_lcc_side_id_p)
***************************************************************************
* @brief
*       This routine returns the side id of the local LCC.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   local_lcc_side_id_p (out) - The pointer to the returned local lcc side id.
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the local lcc side id is found.
*       otherwise - the local lcc side id is not found.
*
* NOTES
*
* HISTORY
*   10-Nov-2008 PHE - Created.
****************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_local_lcc_side_id(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t *local_lcc_side_id_p)
{
    
    return(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure,
                                                                FBE_ENCL_LCC,
                                                                SES_SUBENCL_ID_PRIMARY,
                                                                local_lcc_side_id_p));   
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_subencl_id_to_side_id_component_type(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t subencl_id,
*                                              fbe_u8_t *side_id_p,
*                                              fbe_enclosure_component_types_t *subencl_component_type_p)
***************************************************************************
* @brief
*       This routine gets the side id and subenclosure component type based on 
*       the specified subenclosure id. It should be called after parsing 
*       the subenclosure descriptors in the configuration page.
*
* @param      eses_enclosure - The pointer to the enclosure.
* @param      subencl_id - The subenclosure id.
* @param      side_id_p (out) - The pointer to the returned side id.
* @param      subencl_component_type_p (out) - The pointer to the returned subenclosure component type.
* 
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the component side id is found.
*       otherwise - the component side id is not found or not applicable.
*
* NOTES
*
* HISTORY
*   10-Nov-2008 PHE - Created.
****************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_subencl_id_to_side_id_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t subencl_id,
                                                                fbe_u8_t *side_id_p,
                                                                fbe_enclosure_component_types_t *subencl_component_type_p)
{

    /*******
     * BEGIN
     *******/    
    *side_id_p = FBE_ENCLOSURE_VALUE_INVALID;
    *subencl_component_type_p = FBE_ENCLOSURE_VALUE_INVALID;
    
    // We don't know what component type the subenclosure is. So we check all the possible subenclosures
    // to find the matching subenclosure id.
    if(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure, FBE_ENCL_LCC, // Component type
                               subencl_id,  side_id_p) == FBE_ENCLOSURE_STATUS_OK)
    {
        // The side id has been found. Set the subenclosure component type.
        *subencl_component_type_p = FBE_ENCL_LCC;
        return FBE_ENCLOSURE_STATUS_OK;
    }
    else if(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure, FBE_ENCL_POWER_SUPPLY, // Component type
                              subencl_id, side_id_p) == FBE_ENCLOSURE_STATUS_OK)
    {       
        // The side id has been found.Set the subenclosure component type.
        *subencl_component_type_p = FBE_ENCL_POWER_SUPPLY;
        return FBE_ENCLOSURE_STATUS_OK;
    }
    else if(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure, FBE_ENCL_COOLING_COMPONENT, // Component type
                              subencl_id, side_id_p) == FBE_ENCLOSURE_STATUS_OK)
    {
        // The side id has been found.Set the subenclosure component type.
        *subencl_component_type_p = FBE_ENCL_COOLING_COMPONENT;
        return FBE_ENCLOSURE_STATUS_OK;
    }
    else if(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure, FBE_ENCL_ENCLOSURE, // Component type
                              subencl_id, side_id_p) == FBE_ENCLOSURE_STATUS_OK)
    {
        // The side id has been found.Set the subenclosure component type.
        *subencl_component_type_p = FBE_ENCL_ENCLOSURE;
        return FBE_ENCLOSURE_STATUS_OK;
    }
    else if(fbe_eses_enclosure_component_type_subencl_id_to_side_id(eses_enclosure, FBE_ENCL_SPS, // Component type
                              subencl_id, side_id_p) == FBE_ENCLOSURE_STATUS_OK)
    {
        // The side id has been found.Set the subenclosure component type.
        *subencl_component_type_p = FBE_ENCL_SPS;
        return FBE_ENCLOSURE_STATUS_OK;
    }
    else
    {
        // The component side id is not found or the concept is not applicable to the component.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_INFO,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "%s, not needed in EDAL, subencl_id %d.\n", 
                          __FUNCTION__,  subencl_id); 
        
    }    
    return FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;   
}// End of function fbe_eses_enclosure_subencl_id_to_side_id_component_type

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_comp_index(fbe_eses_enclosure_t *eses_enclosure,
*                                               fbe_u8_t group_id,
*                                               fbe_u8_t elem,
*                                               fbe_u8_t *comp_index_p)         
***************************************************************************
* @brief
*       This function returns 
*         (1) the EDAL component index for a particular element. 
*             It also verifies the element with the platform limits. 
*             The elemenent groups could have the same element type but different subenclosure. 
*             So we need to look at the subenclosure component type to decide 
*             what the component index should be.
*         (2) the EDAL component index of the container which contains the particular element. 
*             If the container does not exist in EDAL or itself is a container, 
*             returns FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED.
*             Note the container could be the overall element which contains
*             its individual element or the entire connector which contains its individual connector.
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    group_id - The ESES element group id.
* @param    elem - The order of the element in the ESES group. 
*                     0 is the overall element. The individual element starts from 1.
* @param    comp_index_p (out)- The EDAL component index.
*
* @return  fbe_enclosure_status_t
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED - This element does not need to be saved in EDAL.
*    otherwise - an error occurred.
*
* NOTES
*    This function can not be used to get the phy_index in EDAL.
*    For drives, it can only be called after the parsing the additional status page.
*    We ignore the additional elements beyond platform limit.
*
* HISTORY
*   12-Nov-2008 PHE - Created.
*   06-Fbe-2009 PHE - Modified to get the component index instead of the first component index.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_comp_index(fbe_eses_enclosure_t *eses_enclosure,
                                                                  fbe_u8_t group_id,
                                                                  fbe_u8_t elem,
                                                                  fbe_u8_t *comp_index_p)
{
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_elem_group_t          * elem_group = NULL;
    ses_elem_type_enum               elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u8_t                         subencl_id = SES_SUBENCL_ID_NONE;
    fbe_u8_t                         side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_component_types_t  component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_enclosure_component_types_t  subencl_component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u8_t                         number_of_temp_sensors_per_lcc = 0;
    fbe_u8_t                         number_of_temp_sensors_on_chassis = 0;
    fbe_u8_t                         number_of_lccs_with_temp_sensor = 0;
    fbe_u8_t                         number_of_expanders_per_lcc = 0;
    fbe_u8_t                         number_of_displays_per_lcc = 0;
    fbe_u8_t                         number_of_two_digit_displays_per_lcc = 0;
    fbe_u8_t                         nth_group = 0;
    fbe_u8_t                         number_of_connectors_per_lcc = 0;
    fbe_u8_t                         number_of_actual_elem_group = 0;
    fbe_u8_t                         num_possible_elems = 0;
    fbe_u8_t                         elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t                         subtype = 0;
    fbe_u8_t                         num_ps_subelements;

    /********
     * BEGIN
     ********/
    *comp_index_p = FBE_ENCLOSURE_VALUE_INVALID;

    //Sanity check.
    number_of_actual_elem_group = fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure);

    if(group_id > number_of_actual_elem_group)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                              FBE_TRACE_LEVEL_WARNING,
                                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                              "get_comp_index failed, invalid group id:%d, actual:%d.\n",
                                              group_id, number_of_actual_elem_group);
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;    
    }

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    /* Get the element type of this element group. */
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* The reason to check it here because we want to print out the elem_type, elem and subencl_comp_type in trace. */
    if(elem > num_possible_elems)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "get_comp_index failed, group_id:%d, invalid elem:%d, num_possible_elems:%d.\n",
                                      group_id, elem, num_possible_elems); 
       
        return FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
    }
    
    /* Get the element type of this element group. */
    elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);

    /* Get the subenclosure id of this element group. */
    subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);  
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_DEBUG_LOW,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "get_comp_index, elem:%d, subencl_id:%d.\n",
                                      elem_type, subencl_id);    
    
    /* Get the subenclosure component type for this element group. */
    encl_status = fbe_eses_enclosure_subencl_id_to_side_id_component_type(eses_enclosure,
                                             subencl_id, 
                                             &side_id,
                                             &subencl_component_type);

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {   
        switch(elem_type)
        {
            case SES_ELEM_TYPE_PS:
                if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
                {
                    num_ps_subelements = 1;
                }

                if(fbe_eses_enclosure_is_ps_overall_elem_saved(eses_enclosure))
                {
                    /* For Viking/Naga PS, there are 3 subelems per PS in EDAL 
                     * (one for overall and two for individual elements.
                     */
                    if((subencl_component_type == FBE_ENCL_POWER_SUPPLY) && (elem <= num_ps_subelements))
                    {
                        *comp_index_p = side_id * num_ps_subelements + elem;  
                    }
                    else
                    {
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    }
                }
                else
                {
                    if(elem == 0)
                    {
                        // We don't save the overall element info in EDAL for power supplies.
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    }
                    else if((subencl_component_type == FBE_ENCL_POWER_SUPPLY) && (elem <= num_ps_subelements))
                    {
                        // For the power supply, the side id times the
                        // number of sub-elements per PS get us into the
                        // correct bucket. That is the A side or B side
                        //
                        // To this we add the elem number - 1.  We subtract 1 since the 
                        // first element is 0 which we want to ignore. So for example
                        // the element numbers for Voyager PS are:
                        //  0 - overall, 
                        //  1 - first PS subelement
                        //  2 - second PS subelement
                        // 
                        // The EDAL for the PS subelements has room for 2 per side. Thus we
                        // subtract 1 from elem getting us 0 or 1 and we add this to the 
                        // base of the bucket. (i.e. 0 or 2).
                        // 
                        *comp_index_p = side_id * num_ps_subelements + (elem - 1);  
                    }
                    else
                    {
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    }
                }
                break;

          case SES_ELEM_TYPE_ENCL: 
                if(elem == 0)
                {
                    // We don't save the overall element info in EDAL for enclosure elements.
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                else if((subencl_component_type == FBE_ENCL_LCC) && (elem == 1))
                {
                    // For LCC, the side id is used as the component index in EDAL.
                    // Because there is only maximum 1 LCC each side.
                    *comp_index_p = 
                        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SUB_ENCL_ID,  //attribute
                                                        FBE_ENCL_LCC,  // Component type
                                                        0, //starting index
                                                        subencl_id);
                    if(FBE_ENCLOSURE_VALUE_INVALID == *comp_index_p)
                    {
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    }
                }
                else if((subencl_component_type == FBE_ENCL_ENCLOSURE) && (elem == 1))
                {
                    // For Chassis, the component index is 0 in EDAL. There is only 1 enclosoure 
                    *comp_index_p = 0;
                }
                else
                {
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                break;

            case SES_ELEM_TYPE_COOLING:
                encl_status = 
                    fbe_eses_enclosure_map_cooling_elem_to_component_index(eses_enclosure,
                                                                           subencl_component_type,
                                                                           elem,
                                                                           side_id,
                                                                           comp_index_p,
                                                                           &subtype);
                break;

            case SES_ELEM_TYPE_TEMP_SENSOR:
                /* 
                 * For temp sensor, the components whose subenclosure is the LCCA will be saved first, 
                 * following by the components on the LCCB.  Then a temp sensor on LCCC and then on LCCD 
                 * if they exist.  Finally, the components whose subenclosure is Chassis are saved. 
                 * The overall element will be the first one in each group.
                 */
                number_of_temp_sensors_per_lcc = fbe_eses_enclosure_get_number_of_temp_sensors_per_lcc(eses_enclosure);
                number_of_temp_sensors_on_chassis = fbe_eses_enclosure_get_number_of_temp_sensors_on_chassis(eses_enclosure);
                number_of_lccs_with_temp_sensor = fbe_eses_enclosure_get_number_of_lccs_with_temp_sensor(eses_enclosure);
                if((subencl_component_type == FBE_ENCL_LCC) && 
                    (elem < number_of_temp_sensors_per_lcc))
                {
                    if (number_of_lccs_with_temp_sensor == 1) //(Local LCC only)
                    {
                        *comp_index_p = elem; 
                        subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_LCC;
                    }
                    else
                    {
                        *comp_index_p = side_id * number_of_temp_sensors_per_lcc + elem; 
                        subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_LCC;
                    }
                }
                else if((subencl_component_type == FBE_ENCL_ENCLOSURE) && 
                        (elem < number_of_temp_sensors_on_chassis))
                {
                    //The component index of the temp sensor on the Chassis in EDAL is after those temp sensors on the LCCs.                    
                    *comp_index_p = number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc + elem;
                    subtype = FBE_ENCL_TEMP_SENSOR_SUBTYPE_CHASSIS;
                }
                else
                {
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }

                break;

            case SES_ELEM_TYPE_DISPLAY:   
                // Get the display characters per lcc. 
                number_of_displays_per_lcc = fbe_eses_enclosure_get_number_of_display_chars(eses_enclosure);
                number_of_two_digit_displays_per_lcc = fbe_eses_enclosure_get_number_of_two_digit_displays(eses_enclosure);
      
                if((elem == 0) || (subencl_id != SES_SUBENCL_ID_PRIMARY))
                {
                    /* We don't save the overall element info or the element info in the secondary subenclosure 
                     * in EDAL for display elements.
                     */
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;  
                }
                else if(subencl_component_type == FBE_ENCL_LCC)
                {  
                    /* Viper and Derringer have two displays: a two-digit display in one color and a one-digit display in another.
                     * We should use the number of possible elements in the displays to distinguish between them. 
                     */
                    
                    encl_status = fbe_eses_enclosure_get_nth_group_among_same_type(eses_enclosure,
                                                                    group_id,
                                                                    &nth_group);

                    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return encl_status;
                    }

                    if((num_possible_elems == FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY) &&
                       (elem <= FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY) &&
                       (elem <= number_of_displays_per_lcc)) 
                    {   
                        *comp_index_p = nth_group * FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY + (elem - 1);
                        subtype = FBE_ENCL_DISPLAY_SUBTYPE_TWO_DIGIT;
                    }
                    else if((num_possible_elems == FBE_ESES_NUM_ELEMS_PER_ONE_DIGIT_DISPLAY) &&
                            (elem <= FBE_ESES_NUM_ELEMS_PER_ONE_DIGIT_DISPLAY) &&
                            (elem <= number_of_displays_per_lcc)) 
                    {
                        *comp_index_p = (number_of_two_digit_displays_per_lcc * FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY) +
                                        (nth_group * FBE_ESES_NUM_ELEMS_PER_ONE_DIGIT_DISPLAY) + (elem - 1);
                        subtype = FBE_ENCL_DISPLAY_SUBTYPE_ONE_DIGIT;
                    }
                    else
                    {
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    }
                }
                else
                {
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;  
                }
                break;

             case SES_ELEM_TYPE_SAS_EXP:
                // For expander, the components whose subenclosure is the LCC on the side A 
                // is saved first following by the components on side B in EDAL.
                number_of_expanders_per_lcc = fbe_eses_enclosure_get_number_of_expanders_per_lcc(eses_enclosure);

                if(elem == 0)
                {
                    // We don't save the overall element info in EDAL for sas expanders.
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                else if((subencl_component_type == FBE_ENCL_LCC) &&
                        (elem <= number_of_expanders_per_lcc))
                {
                    // LCCs are the only subenclosures which contain expanders.
                    *comp_index_p = side_id * number_of_expanders_per_lcc + elem - 1;
                }
                else
                {
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                break;

            case SES_ELEM_TYPE_SAS_CONN:
                number_of_connectors_per_lcc = fbe_eses_enclosure_get_number_of_connectors_per_lcc(eses_enclosure);

                if(elem == 0)
                {
                    // We don't save the overall element info in EDAL for sas connectors.
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;  
                }
                else if((subencl_component_type == FBE_ENCL_LCC) &&
                       (elem <= number_of_connectors_per_lcc))
                {
                    // LCCs are the only subenclosures which contain connectors.
                    *comp_index_p = side_id * number_of_connectors_per_lcc + elem - 1;
                }
                else
                {
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                break;

            case SES_ELEM_TYPE_ARRAY_DEV_SLOT: 
                if(elem == 0)
                {
                    // We don't save the overall element info in EDAL for device slot.
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                else if (( elem-1) >= fbe_eses_enclosure_get_number_of_slots(eses_enclosure)) 
                {
                    // We only support number of drives defined in leaf class.
                    encl_status = FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
                }
                else 
                {
                    /* the order which the slot is described in type descriptor is used as index */
                    *comp_index_p = elem - 1;
                }
            
                break;                
            
            case SES_ELEM_TYPE_EXP_PHY:
                
                if(elem == 0)
                {
                    // We don't save the overall element info in EDAL for sas expanders.
                    encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                }
                else 
                {
                    component_type = FBE_ENCL_EXPANDER_PHY;
                    elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id) + elem - 1;    
                    
                    encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure,
                                                                                    component_type,
                                                                                    elem_index,
                                                                                    comp_index_p);
                }
            
                break;                

        case SES_ELEM_TYPE_UPS:
            if(elem == 0)
            {
                // We don't save the overall element info in EDAL for sas expanders.
                encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            }
            else 
            {
                /* the order which the slot is described in type descriptor is used as index */
                *comp_index_p = elem - 1;
            }
            break;

        case SES_ELEM_TYPE_ESC_ELEC:
            if((subencl_component_type == FBE_ENCL_ENCLOSURE) && (elem == 1))
            {
                *comp_index_p = 0;
            }
            else
            {
                encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            }
            break;
            
        case SES_ELEM_TYPE_ALARM:
        case SES_ELEM_TYPE_LANG:            
        default:      
            encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            break;

        }// End of - switch(elem_type)
    }// End of - if(encl_status == FBE_ENCLOSURE_STATUS_OK)

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        encl_status = fbe_eses_enclosure_verify_comp_index(eses_enclosure, group_id, *comp_index_p, side_id, subtype, elem);
    
        if ((encl_status != FBE_ENCLOSURE_STATUS_OK) &&
            (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED))
        {
            //This happens due to the coding error(ERROR level).
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "get_comp_index, verify_comp_index failed, group_id:%d, elem:%d, comp_index:%d.\n",
                                           group_id, elem, *comp_index_p);   
            
            
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "elem_type:%s, subencl_comp_type:%s, encl_status:0x%x.\n",                                             
                                          elem_type_to_text(elem_type),
                                          enclosure_access_printComponentType(subencl_component_type),
                                          encl_status);  
            
        }
    }
    else if(encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
    {
        //This happens because the physical package does not support the new ESES element(WARNING level). 
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "get_comp_index failed, elem_type:%s, elem:%d.\n",
                                      elem_type_to_text(elem_type), 
                                      elem);
        

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "get_comp_index failed, subencl_comp_type:%s.\n",
                                     enclosure_access_printComponentType(subencl_component_type));
        
    }

    return encl_status; 
} 
        
/*!*************************************************************************
* @fn fbe_eses_enclosure_map_cooling_elem_to_component_index(fbe_eses_enclosure_t * eses_enclosure, 
*                                                            fbe_enclosure_component_types_t subencl_component_type,
*                                                            fbe_u8_t elem,
*                                                            fbe_u8_t *comp_index_p,
*                                                            fbe_u8_t *subtype);
***************************************************************************
* @brief
*       This function create component index in edal for cooling component.
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    subencl_component_type - subenclosure type.
* @param    element   - element within a group. 
* @param    side_id   - location of the component. 
* @param    comp_index_p - location in edal.
* @param    subtype - subtype associated with the component.
*
* @return  fbe_enclosure_status_t
*    Always return FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*   
* HISTORY
*   29-Apr-2010 NChiu - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_map_cooling_elem_to_component_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_enclosure_component_types_t subencl_component_type,
                                                       fbe_u8_t elem,
                                                       fbe_u8_t side_id,
                                                       fbe_u8_t *comp_index_p,
                                                       fbe_u8_t *subtype)
{
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                         number_of_cooling_components_on_chassis = 0;
    fbe_u8_t                         number_of_cooling_components_external = 0;
    fbe_u8_t                         number_of_cooling_components_per_ps = 0;
    fbe_u8_t                         number_of_cooling_components_on_lcc = 0;
    fbe_u8_t                         number_of_power_supplies = 0;

    /* 
    * For cooling component, the components whose subenclosure is the power supply on the side A 
    * is saved first following by the component on side B in EDAL.
    * The overall element will the first one in each group.
    */
    number_of_cooling_components_external = fbe_eses_enclosure_get_number_of_cooling_components_external(eses_enclosure);
    number_of_cooling_components_per_ps = fbe_eses_enclosure_get_number_of_cooling_components_per_ps(eses_enclosure);
    number_of_cooling_components_on_chassis = fbe_eses_enclosure_get_number_of_cooling_components_on_chassis(eses_enclosure);
    number_of_power_supplies = fbe_eses_enclosure_get_number_of_power_supplies(eses_enclosure);
    number_of_cooling_components_on_lcc = fbe_eses_enclosure_get_number_of_cooling_components_on_lcc(eses_enclosure);
    if((subencl_component_type == FBE_ENCL_POWER_SUPPLY) && 
            (elem < number_of_cooling_components_per_ps))
    {
        *comp_index_p = side_id * number_of_cooling_components_per_ps + elem;
        *subtype = FBE_ENCL_COOLING_SUBTYPE_PS_INTERNAL;
    }
    else if((subencl_component_type == FBE_ENCL_ENCLOSURE) && 
            (elem < number_of_cooling_components_on_chassis))
    {
        //The component index of the cooling on the Chassis in EDAL is after those on the PSs.                    
        *comp_index_p = number_of_power_supplies * number_of_cooling_components_per_ps + elem;  
        *subtype = FBE_ENCL_COOLING_SUBTYPE_CHASSIS;
    }
    else if((subencl_component_type == FBE_ENCL_COOLING_COMPONENT) && 
            (elem < FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT))
    {
        //The component index of the external cooling in EDAL is after the chassis.
        *comp_index_p = number_of_power_supplies * number_of_cooling_components_per_ps + 
                        number_of_cooling_components_on_chassis + 
                        side_id * FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT + elem;  
        *subtype = FBE_ENCL_COOLING_SUBTYPE_STANDALONE;
    }
    else if((subencl_component_type == FBE_ENCL_LCC) && 
            (elem < number_of_cooling_components_on_lcc))
    {
        //The component index of the lcc internal cooling in EDAL is after external cooling.
        *comp_index_p = number_of_power_supplies * number_of_cooling_components_per_ps + 
                        number_of_cooling_components_on_chassis + 
                        number_of_cooling_components_external * FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT +
                        + elem;  
        *subtype = FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL;
    }
    else
    {
        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
    }
     
    return (encl_status);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_map_fw_target_type_to_component_index(fbe_eses_enclosure_t * eses_enclosure, 
*                                                    fbe_enclosure_fw_target_t fw_target, 
*                                                    fbe_u8_t slot_id,
*                                                    fbe_u8_t rev_id, 
*                                                    fbe_u8_t *component_index
***************************************************************************
* @brief
*       This function maps a firmware target to the component index in edal.
*   The mapping is the same as in fbe_eses_enclosure_get_comp_index().
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    fw_target - firmware id.
* @param    slot_id   - The location of the hardware 
* @param    rev_id   -  The rev id. 
* @param    component_index - location in edal.
*
* @return  fbe_enclosure_status_t
*
* NOTES
*   Some device such as Viking PS contains two MCUs. Each MCU has its own rev.
* HISTORY
*   29-Apr-2010 NChiu - Created.
*   13-Dec-2013 PHE - add the mcu as the input.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_map_fw_target_type_to_component_index(fbe_eses_enclosure_t * eses_enclosure,
                                                    fbe_enclosure_component_types_t fw_comp_type, 
                                                    fbe_u8_t slot_id, 
                                                    fbe_u8_t rev_id,
                                                    fbe_u8_t *component_index)
{
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t subtype = 0; // a place holder.
    fbe_u8_t                         num_ps_subelements;

    switch (fw_comp_type)
    {
        // We assuming the firmware revision storage matching the side id.
        //  component      index
        //   LCC A           0
        //   LCC B           1
        //   PS A            0
        //   PS B            1

    case FBE_ENCL_LCC:
        *component_index = 
            (fbe_u8_t)fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SIDE_ID,  //attribute
                                                        FBE_ENCL_LCC,  // Component type
                                                        0, //starting index
                                                        slot_id % ESES_SUPPORTED_SIDES); // this won't do anything if the input is sideId
        if (*component_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            encl_status = FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
        }
        break;
    case FBE_ENCL_POWER_SUPPLY:
        if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
        {
            num_ps_subelements = 1;
        }

        if(fbe_eses_enclosure_is_ps_overall_elem_saved(eses_enclosure))
        {
            /* We save overall and two individual elements in EDAL for Viking/Naga PS
             */
            *component_index = slot_id * num_ps_subelements + rev_id;
        }
        else
        {
            *component_index = slot_id * num_ps_subelements;
        }
        break;
    case FBE_ENCL_COOLING_COMPONENT:
        encl_status = 
                fbe_eses_enclosure_map_cooling_elem_to_component_index(eses_enclosure,
                                                                        fw_comp_type,
                                                                        0,   // use the overall element
                                                                        slot_id,
                                                                        component_index,
                                                                        &subtype);
        break;
    case FBE_ENCL_SPS:
        /* We can not use the slot id to find the SPS component index.
         * ESES uses side 31 for SPS. So we saved side id 31 for SPS in EDAL.
         * The client might use 0 or 1 for slot_id.
         * That is why we just return component index 0 since currently there is only 1 SPS reported by ESES. 
         */
        *component_index = 0;
        break;

    default:
        encl_status = FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
        break;
    }

    return (encl_status);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_nth_group_among_same_type(fbe_eses_enclosure_t * eses_enclosure, 
*                                                     fbe_u8_t group_id,
*                                                     fbe_u8_t * number_of_same_type_groups_processed_p)     
***************************************************************************
* @brief
*       This function gets the order of the specified group among
*       other groups which have the same characteristics. 
*       If it is the first group, the output will be 0. 
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    group_id - The ESES element group id.
* @param    nth_group_p - The pointer to the 
*                        number of same type groups which have smaller group id.
*
* @return  fbe_enclosure_status_t
*    Always return FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*   
* HISTORY
*   24-Feb-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_nth_group_among_same_type(
                                                       fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_u8_t group_id,
                                                       fbe_u8_t * nth_group_p)
{ 
    fbe_eses_elem_group_t          * elem_group = NULL;
    ses_elem_type_enum               elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u8_t                         subencl_id = SES_SUBENCL_ID_NONE;
    fbe_u8_t                         num_possible_elems = 0;
    ses_elem_type_enum               elem_type_i = SES_ELEM_TYPE_INVALID;
    fbe_u8_t                         subencl_id_i = SES_SUBENCL_ID_NONE;
    fbe_u8_t                         num_possible_elems_i = 0;
    fbe_u8_t                         number_of_groups = 0;
    fbe_u8_t                         i = 0 ; // for loop.
    
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);
    subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    for(i = 0; i < group_id; i ++)
    {
        elem_type_i = fbe_eses_elem_group_get_elem_type(elem_group, i);
        subencl_id_i = fbe_eses_elem_group_get_subencl_id(elem_group, i);
        num_possible_elems_i = fbe_eses_elem_group_get_num_possible_elems(elem_group, i);
        if((elem_type == elem_type_i) &&
            (subencl_id == subencl_id_i) &&
            (num_possible_elems == num_possible_elems_i))
        {
            number_of_groups ++;
        }
    }

    *nth_group_p = number_of_groups;

    return FBE_ENCLOSURE_STATUS_OK;
}
                    
/*!*************************************************************************
* @fn fbe_eses_enclosure_verify_comp_index(fbe_eses_enclosure_t * eses_enclosure, 
*                                          fbe_u8_t group_id,
*                                          fbe_u8_t component_index,
*                                          fbe_eses_subencl_side_id_t side_id,
*                                          fbe_u8_t subtype,
*                                          fbe_u8_t elem)       
***************************************************************************
* @brief
*       This function verifies whether the component index is valid.
*       The values of the attributes to be verified in this function 
*       should already be initialized in the component specific init handler.
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    group_id - The ESES element group id.
* @param    component_index - The EDAL component index.
* @param    side_id - The ESES side id.
* @param    subtype - The component subenclosure type.
* @param    elem - The order of the element in the elem group.
*
* @return  fbe_enclosure_status_t
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*   
* HISTORY
*   10-Feb-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_verify_comp_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                                   fbe_u8_t group_id,
                                                                   fbe_u8_t component_index,
                                                                   fbe_eses_subencl_side_id_t side_id,
                                                                   fbe_u8_t subtype,
                                                                   fbe_u8_t elem)
                                                                  
{
    fbe_u8_t number_of_components = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_component_types_t  component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t initial_side_id = 0;
    fbe_u8_t initial_subtype = 0;
    fbe_u8_t initial_container_index = 0;

    /********
     * BEGIN
     ********/
        
    encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure, group_id, &component_type);
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "verify_comp_index, get_comp_type failed, group_id:%d, comp_index:%d.\n",
                                     group_id, component_index);

        return encl_status;  
    }
                                                            
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                        component_type,  // Component type
                                                        &number_of_components); 

    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
    } 

    if(component_index >= number_of_components)
    {
        // This is the coding error. ESES size and EDAL size do not match.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "verify_comp_index, group_id:%d, comp_type:%s.\n",
                                     group_id, enclosure_access_printComponentType(component_type));

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "verify_comp_index, invalid comp_index:%d, num_of_comps:%d.\n",
                                      component_index, number_of_components);

        return FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;        
    } 

    // Check if the side id matches the one saved in the EDAL.
    switch(component_type)
    {
        case FBE_ENCL_POWER_SUPPLY:
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_TEMP_SENSOR:
        case FBE_ENCL_EXPANDER:
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute
                                                component_type, 
                                                component_index,
                                                &initial_side_id); 

            if((encl_status == FBE_ENCLOSURE_STATUS_OK) && (initial_side_id != side_id))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_WARNING,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "verify_comp_index, group_id:%d, comp_type:%s.\n",
                                             group_id, enclosure_access_printComponentType(component_type));

                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "verify_comp_index, mismatch side_id: %d, initial_side_id: %d.\n",
                                             side_id, initial_side_id);

                return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;               
            }
            break;

        default:
            break;
    }

    // Check if the component subtype matches the one saved in the EDAL.
    switch(component_type)
    {
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_TEMP_SENSOR:
        case FBE_ENCL_DISPLAY:
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_SUBTYPE,  // Attribute
                                                component_type, 
                                                component_index,
                                                &initial_subtype); 

            if((encl_status == FBE_ENCLOSURE_STATUS_OK) && (initial_subtype != subtype))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_WARNING,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                             "verify_comp_index, group_id:%d, comp_type:%s.\n",
                                              group_id, enclosure_access_printComponentType(component_type));

                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                             "verify_comp_index, mismatch subtype: %d, initial_subtype: %d.\n",
                                             subtype, initial_subtype);

                return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;               
            }
            break;

        default:
            break;
    }
    
    if(elem == 0)
    {
        // This is an overall element. 
        // Check if the container index matches the one saved in the EDAL.
        switch(component_type)
        {
            case FBE_ENCL_COOLING_COMPONENT:
            case FBE_ENCL_TEMP_SENSOR:
                encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute
                                                    component_type, 
                                                    component_index,
                                                    &initial_container_index); 

                if((encl_status == FBE_ENCLOSURE_STATUS_OK) && 
                   (initial_container_index != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                 FBE_TRACE_LEVEL_WARNING,
                                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "verify_comp_index, group_id:%d, comp_type:%s.\n",
                                                group_id, enclosure_access_printComponentType(component_type));

                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                 FBE_TRACE_LEVEL_ERROR,
                                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                 "verify_comp_index, invalid initial_container_index: %d for elem 0.\n",
                                                 initial_container_index);

                    return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;              
                }
                break;

            default:
                break;
        }
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_index_to_component_index(fbe_eses_enclosure_t * eses_enclosure,
*                                fbe_enclosure_component_types_t component_type,
*                                fbe_u8_t elem_index,
*                                fbe_u8_t *component_index_p)                
***************************************************************************
* @brief
*       This function gets the component index which is the order the component
*       saved in EDAL based on the input component type and the element index.
*       It should be called after configuration page and additional page are processed.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  component_type (Input)- The target component type in EDAL.
* @param  component_index_p (output) - The pointer to the returned component index.
*
* @return 
*      fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - the component index is found.
*        FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED - The element data is not saved in EDAL.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*  12-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_component_index(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_enclosure_component_types_t component_type,
                                                     fbe_u8_t elem_index,
                                                     fbe_u8_t *component_index_p)
{
    fbe_u32_t               matching_index;

    /*******
     * BEGIN
     *******/    
    // Initialize to FBE_ENCLOSURE_VALUE_INVALID.
    *component_index_p = FBE_ENCLOSURE_VALUE_INVALID;  

    if(elem_index == SES_ELEM_INDEX_NONE)
    {
        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;  
    }
    switch(component_type)
    {
        case FBE_ENCL_DRIVE_SLOT:
        case FBE_ENCL_EXPANDER_PHY:
        case FBE_ENCL_CONNECTOR:
        case FBE_ENCL_EXPANDER:
        case FBE_ENCL_POWER_SUPPLY:
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_TEMP_SENSOR:
        case FBE_ENCL_LCC:
        case FBE_ENCL_ENCLOSURE:
        case FBE_ENCL_DISPLAY:
        case FBE_ENCL_SPS:
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, unsupported component type: %s, elem_index: %d.\n", 
                                            __FUNCTION__,  
                                            enclosure_access_printComponentType(component_type),
                                            elem_index);
            
            return FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
            break;    
    }// End of - switch(elem_type)

    matching_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                    component_type,  // Component type
                                                    0, //starting index
                                                    elem_index);   

    if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        *component_index_p = (fbe_u8_t) matching_index;      
        return FBE_ENCLOSURE_STATUS_OK; 
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_DEBUG_LOW,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "elem_index_to_comp_index mapping failed, comp_type:%s, elem_index:%d.\n", 
                                     enclosure_access_printComponentType(component_type),
                                     elem_index);  

        return FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
    }
} // End of function fbe_eses_enclosure_elem_index_to_component_index

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_comp_type(fbe_eses_enclosure_t * eses_enclosure,
*                                           fbe_u8_t group_id,
*                                           fbe_enclosure_component_types_t * component_type_p)      
***************************************************************************
* @brief
*       This function returns the EDAL component type for a particular element group. 
*
* @param    eses_enclosure - The pointer to the eses enclosure.
* @param    group_id - The ESES element group id.
* @param    component_type_p (out)- The pointer to the EDAL component type.
*
* @return  fbe_enclosure_status_t
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES   
*
* HISTORY
*   06-Fbe-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_comp_type(fbe_eses_enclosure_t * eses_enclosure,
                                                             fbe_u8_t group_id,
                                                             fbe_enclosure_component_types_t * component_type_p)
{
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_elem_group_t          * elem_group = NULL;
    ses_elem_type_enum               elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u8_t                         subencl_id = SES_SUBENCL_ID_NONE;
    fbe_u8_t                         side_id = 0;
    fbe_enclosure_component_types_t  subencl_component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;

    /********
     * BEGIN
     ********/
   *component_type_p = FBE_ENCL_INVALID_COMPONENT_TYPE;
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    /* (1) Get the element type of this element group. */
    elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);

    /* (2) Get the subenclosure id of this element group. */      
    subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);
            
    /* (3) subencl_id -> subencl_component_type */
    encl_status = fbe_eses_enclosure_subencl_id_to_side_id_component_type(eses_enclosure,
                            subencl_id,
                            &side_id,         
                            &subencl_component_type);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    /* (4) elem_type and subencl_component_type -> component_type. */
    encl_status = fbe_eses_enclosure_elem_type_to_component_type(eses_enclosure, 
                                    elem_type, 
                                    subencl_component_type,
                                    component_type_p);
    return encl_status;
}// End of function fbe_eses_enclosure_get_comp_type

/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_index_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
*                                fbe_u8_t elem_index,
*                                fbe_enclosure_component_types_t * component_type_p)                
***************************************************************************
* @brief
*       This function translates the ESES element index to EDAL component type.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_index (Input)- ESES element index
* @param  component_type_p (output) - The pointer to the returned EDAL component type.
*
* @return 
*      fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - the component type is found.
*        FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED - The element data is not saved in EDAL.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*  25-Jan-2010 Rajesh V. - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_u8_t elem_index,
                                                     fbe_enclosure_component_types_t * component_type_p)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_INVALID;
    fbe_u8_t group_id = 0;

    group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index);
    if(group_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        return(FBE_ENCLOSURE_STATUS_INVALID);
    }

    encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure, group_id, component_type_p);
    return(encl_status);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_phy_index_to_drive_slot_index(fbe_eses_enclosure_t * eses_enclosure, 
*                                             fbe_u8_t phy_index,
*                                             fbe_u8_t * drive_slot_index_p)               
***************************************************************************
* @brief
*       This function gets the drive slot number that the specified phy 
*       is attached to. 
*       It should be called after parsing the ESES additinal page and status page.
*
* @param    eses_enclosure (input)- The pointer to the eses enclosure.
* @param    phy_index (input) - The order that phy info is saved.  
* @param    drive_slot_index_p (output) - The pointer to the associated drive slot index.   
*
* @return
*      fbe_enclosure_status_t 
*         FBE_ENCLOSURE_STATUS_OK - the associated drive slot number is found. 
*         otherwise - an error is found.
*
* NOTES
*
* HISTORY
*   04-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_phy_index_to_drive_slot_index(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t phy_index,
                                              fbe_u8_t * drive_slot_index_p)
{
    fbe_u32_t               matching_index;
    
    /*******
     * BEGIN
     *******/
    *drive_slot_index_p = FBE_ENCLOSURE_VALUE_INVALID;
     
    matching_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_PHY_INDEX,  //attribute
                                                    FBE_ENCL_DRIVE_SLOT,  // Component type
                                                    0, //starting index
                                                    phy_index);   

    if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        // Find the element index that the phy is attached to.
        *drive_slot_index_p = (fbe_u8_t) matching_index;
        return FBE_ENCLOSURE_STATUS_OK; 
    }

    /* 
     * It is normal that we can not find a mapping between the phy index and drive slot.
     * Because the phy could be connected to something other than a drive.
     * So do not a message to indicate it is an error here.
     */
   
    return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_phy_index_to_connector_index(fbe_eses_enclosure_t * eses_enclosure, 
*                                             fbe_u8_t phy_index,
*                                             fbe_u8_t * connector_index_p)               
***************************************************************************
* @brief
*       This function gets the connector index that the specified phy 
*       is attached to. 
*       It should be called after parsing the ESES additinal page and status page.
*
* @param    eses_enclosure (input)- The pointer to the eses enclosure.
* @param    phy_index (input) - The order that phy info is saved.  
* @param    connector_index_p (output) - The pointer to the connector index.   
*
* @return
*      fbe_enclosure_status_t 
*         FBE_ENCLOSURE_STATUS_OK - the associated connector index is found. 
*         otherwise - an error is found.
*
* NOTES
*
* HISTORY
*   07-Apr-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_phy_index_to_connector_index(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t phy_index,
                                              fbe_u8_t * connector_index_p)
{
    fbe_u32_t               matching_index;
    
    /*******
     * BEGIN
     *******/
    *connector_index_p = FBE_ENCLOSURE_VALUE_INVALID;
     
    matching_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_PHY_INDEX,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    0, //starting index
                                                    phy_index);   

    if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        // Find the element index that the phy is attached to.
        *connector_index_p = (fbe_u8_t) matching_index;
        return FBE_ENCLOSURE_STATUS_OK; 
    }

    /* 
     * It is normal that we can not find a mapping between the phy index and drive slot.
     * Because the phy could be connected to something other than a drive.
     * So do not a message to indicate it is an error here.
     */
   
    return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_exp_elem_index_phy_id_to_phy_index(fbe_eses_enclosure_t  * eses_enclosure, 
*                         fbe_u8_t expander_elem_index, 
*                         fbe_u8_t phy_id, 
*                         fbe_u8_t * phy_index_p)                 
***************************************************************************
*
* DESCRIPTION
*       This function gets the phy index from the expander element index and phy id.
*       It should be called after parsing the ESES additional status page.
*
* PARAMETERS
*       eses_enclosure (Input) - The pointer to the eses enclosure.
*       expander_elem_index (Input)- The element index of the sas expander that the phy is contained.
*       phy_id (Input) - The phy identification.
*       phy_index_p (Output) - The order that the phy info is saved.
*
* RETURN VALUES
*      fbe_enclosure_status_t
*        FBE_ENCLOSURE_STATUS_OK - the associated phy index is found. 
*        otherwise - the associated phy index is not found.
*
* NOTES
*      It is expected to see the mapping failure for the phy which is not attached 
*      to anything because we don't save its information.
*
* HISTORY
*  26-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_exp_elem_index_phy_id_to_phy_index(fbe_eses_enclosure_t  * eses_enclosure, 
                fbe_u8_t expander_elem_index, fbe_u8_t phy_id, fbe_u8_t * phy_index_p)
{
    fbe_u8_t i = 0;
    fbe_u8_t component_expander_elem_index = SES_ELEM_INDEX_NONE; 
    fbe_u8_t component_phy_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


   /*********
    * BEGIN
    *********/
   *phy_index_p = FBE_ENCLOSURE_VALUE_INVALID;

    for(i = 0; i < fbe_eses_enclosure_get_number_of_expander_phys(eses_enclosure); i ++)
    {
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                  FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,  // Attribute
                                                  FBE_ENCL_EXPANDER_PHY,    // Component type
                                                  i,
                                                  &component_expander_elem_index); // output
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                  FBE_ENCL_EXP_PHY_ID,  // Attribute
                                                  FBE_ENCL_EXPANDER_PHY,    // Component type
                                                  i,
                                                  &component_phy_id); // output

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        if((component_expander_elem_index == expander_elem_index) && (component_phy_id  == phy_id))
        {
            *phy_index_p = i;      
            return FBE_ENCLOSURE_STATUS_OK; 
        } 
    }


    /* 
     * Fail to find the mapping between the expander element index, phy id and and phy_index. 
     * It is expected that the mapping is not found if the phy is not attached at all.
     * Don't want the trace every poll cycle at normal trace level. So FBE_TRACE_LEVEL_DEBUG_HIGH is used.
     */    
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, failed to find the mapping, expander_elem_index: %d, phy_id: %d.\n", 
                                 __FUNCTION__, expander_elem_index, phy_id);

    return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_expander_sas_address_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
*                                                     fbe_sas_address_t sas_address,
*                                                     fbe_u8_t *elem_index_p)                 
***************************************************************************
* @brief
*       This function gets the expander element index from the expander sas address.
*       It should be called after parsing the ESES additional status page.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  sas_address (Input)- The sas address of the expander.
* @param  elem_index_p (output) - The pointer to the sas expander element index.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - the associated sas expander element index is found. 
*          otherwise - the associated sas expander element index is not found.
*
* NOTES
*
* HISTORY
*  26-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_expander_sas_address_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_sas_address_t sas_address,
                                                     fbe_u8_t *elem_index_p)
{
    LARGE_INTEGER sas_addr;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t matching_component_index = FBE_ENCLOSURE_VALUE_INVALID;

    /********
     * BEGIN
     ********/

    *elem_index_p = SES_ELEM_INDEX_NONE;
 
    matching_component_index = fbe_base_enclosure_find_first_edal_match_U64((fbe_base_enclosure_t *) eses_enclosure,
                                                FBE_ENCL_EXP_SAS_ADDRESS,  // Attribute.
                                                FBE_ENCL_EXPANDER,  // Component type.
                                                0, // Start index.
                                                sas_address);  // Check value.

    if(matching_component_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                      FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                                      FBE_ENCL_EXPANDER,    // Component type
                                                      matching_component_index,    // Component index
                                                      elem_index_p); // The pointer to the return value        
    }
    else
    {    
        sas_addr.QuadPart = sas_address;

        // Fail to find the mapping between expander sas address and elempower supply index. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, failed to find the mapping, sas_address: 0x%8.8x_0x%8.8x.\n", 
                                 __FUNCTION__,  
                                 sas_addr.HighPart, sas_addr.LowPart);  
       
        encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    }

    return encl_status;
}// End of - fbe_eses_enclosure_expander_sas_address_to_elem_index


/*!*************************************************************************
* @fn fbe_eses_enclosure_component_type_to_element_type(fbe_eses_enclosure_t * eses_enclosure,
*                                                ses_elem_type_enum elem_type,
*                                                fbe_enclosure_component_types_t subencl_component_type,
*                                                fbe_enclosure_component_types_t * component_type)             
***************************************************************************
* @brief
*       This function translates the ESES element type to EDAL component type.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_type (Input)- The ESES element type.
* @param  subencl_component_type (Input)- The component type of the subenclosure 
*                                         which contains the element group of the element type.
* @param  component_type_p (output) - The pointer to the sas expander element index.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - no error. 
*          otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*  8-Oct-2009 Prasenjeet - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_component_type_to_element_type(fbe_eses_enclosure_t * eses_enclosure,
                                                fbe_enclosure_component_types_t component_type,
                                                ses_elem_type_enum * elem_type_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    // Initialized to SES_ELEM_TYPE_INVALID.
    *elem_type_p = SES_ELEM_TYPE_INVALID;

    switch(component_type)
    {
        case FBE_ENCL_POWER_SUPPLY : 
            *elem_type_p = SES_ELEM_TYPE_PS;
            break;

        case FBE_ENCL_COOLING_COMPONENT :
            *elem_type_p = SES_ELEM_TYPE_COOLING;
            break;

        case FBE_ENCL_TEMP_SENSOR :
            *elem_type_p = SES_ELEM_TYPE_TEMP_SENSOR;
            break;

        case FBE_ENCL_EXPANDER :
            *elem_type_p = SES_ELEM_TYPE_SAS_EXP ;
            break;

 //       case subencl_component_type : 
 //           *elem_type_p = SES_ELEM_TYPE_ENCL ;
//            break; 
        
        case FBE_ENCL_DISPLAY :
            *elem_type_p = SES_ELEM_TYPE_DISPLAY;
            break;
                 
        case FBE_ENCL_DRIVE_SLOT :
            *elem_type_p = SES_ELEM_TYPE_ARRAY_DEV_SLOT;
            break;
       
        case FBE_ENCL_CONNECTOR :  
            *elem_type_p = SES_ELEM_TYPE_SAS_CONN;
            break;

        case FBE_ENCL_EXPANDER_PHY :
            *elem_type_p = SES_ELEM_TYPE_EXP_PHY;
            break;

        case FBE_ENCL_SPS :
            *elem_type_p = SES_ELEM_TYPE_UPS;
            break;

        default:


            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, unhandled component_type: 0x%x.\n", 
                                     __FUNCTION__,  component_type);

            encl_status = FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
            break;
    } 

    return encl_status;

}// End of function fbe_eses_enclosure_component_type_to_element_type



/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_type_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
*                                                ses_elem_type_enum elem_type,
*                                                fbe_enclosure_component_types_t subencl_component_type,
*                                                fbe_enclosure_component_types_t * component_type)             
***************************************************************************
* @brief
*       This function translates the ESES element type to EDAL component type.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_type (Input)- The ESES element type.
* @param  subencl_component_type (Input)- The component type of the subenclosure 
*                                         which contains the element group of the element type.
* @param  component_type_p (output) - The pointer to the sas expander element index.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - no error. 
*          otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*  26-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_elem_type_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                ses_elem_type_enum elem_type,
                                                fbe_enclosure_component_types_t subencl_component_type,
                                                fbe_enclosure_component_types_t * component_type_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    // Initialized to FBE_ENCL_INVALID_COMPONENT_TYPE.
    *component_type_p = FBE_ENCL_INVALID_COMPONENT_TYPE;

    switch(elem_type)
    {
        case SES_ELEM_TYPE_PS: 
            *component_type_p = FBE_ENCL_POWER_SUPPLY;
            break;

        case SES_ELEM_TYPE_COOLING:
            *component_type_p = FBE_ENCL_COOLING_COMPONENT;
            break;

        case SES_ELEM_TYPE_TEMP_SENSOR:
            *component_type_p = FBE_ENCL_TEMP_SENSOR;
            break;

        case SES_ELEM_TYPE_SAS_EXP:
            *component_type_p = FBE_ENCL_EXPANDER;
            break;

        case SES_ELEM_TYPE_ENCL: 
            *component_type_p = subencl_component_type;
            break; 
        
        case SES_ELEM_TYPE_DISPLAY:
            *component_type_p = FBE_ENCL_DISPLAY;
            break;
                 
        case SES_ELEM_TYPE_ARRAY_DEV_SLOT:
            *component_type_p = FBE_ENCL_DRIVE_SLOT;
            break;
       
        case SES_ELEM_TYPE_SAS_CONN:  
            *component_type_p = FBE_ENCL_CONNECTOR;
            break;

        case SES_ELEM_TYPE_EXP_PHY:
            *component_type_p = FBE_ENCL_EXPANDER_PHY;
            break;

        case SES_ELEM_TYPE_UPS:
            *component_type_p = FBE_ENCL_SPS;
            break;

        case SES_ELEM_TYPE_ESC_ELEC:
            if(subencl_component_type == FBE_ENCL_ENCLOSURE) 
            {
            
                *component_type_p = FBE_ENCL_SSC;
            }
            else
            {
                encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            }
            break;
            
        case SES_ELEM_TYPE_ALARM:
        case SES_ELEM_TYPE_LANG:
            encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
            break;

        default:


            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, unhandled elem_type: 0x%x.\n", 
                                     __FUNCTION__,  elem_type);

            encl_status = FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED;
            break;
    } 

    return encl_status;

}// End of function fbe_eses_enclosure_elem_type_to_component_type

/*!***************************************************************
 * @fn fbe_eses_enclosure_get_connector_entire_connector_index(
 *                                     fbe_eses_enclosure_t * eses_enclosure,
 *                                     fbe_u8_t connector_id,
 *                                     fbe_u8_t * expansion_port_entire_connector_index_p)
 ****************************************************************
 * @brief:
 *    This function gets the entire connector index for a connector.
 *
 * @param  eses_enclosure (INPUT)- The pointer to the eses enclosure.
 * @param  connector_id (INPUT) - connector id
 * @param  expansion_port_entire_connector_index_p (OUTPUT) - The pointer to the expansion port entire connector index.
 *
 * @return  fbe_enclosure_status_t 
 *         FBE_ENCLOSURE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    03/04/2010: NCHIU - Created. 
 *
 ****************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_get_connector_entire_connector_index(fbe_eses_enclosure_t * eses_enclosure,
                                                   fbe_u8_t connector_id,
                                                   fbe_u8_t * expansion_port_entire_connector_index_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;

    /********
     * BEGIN
     ********/        
    *expansion_port_entire_connector_index_p = FBE_ENCLOSURE_VALUE_INVALID;

    // Sanity check.  
    if (connector_id >= fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure))
    {
        encl_status = FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
    }
    else
    {

        /* The connectors saved in EDAL is in the order of sides, side A followed by side B.
         * All the local connectors are in the will be in contiguous order.
         * First, find the first local connector index. Other local connectors follow.
         */
        matching_conn_index = 
                fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        0, //starting index
                                                        TRUE);

        if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
        {
            /* There must be expansion port among the local connectors.
             * Second, find the first matching connector id.
             */
            matching_conn_index = 
                fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_ID,  //attribute
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        matching_conn_index, //starting index
                                                        connector_id);

            if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
            {
                /* Let's get the whole connector now*/
                *expansion_port_entire_connector_index_p = 
                            fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute.
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        matching_conn_index,     // Component index.
                                                        TRUE);

                if(*expansion_port_entire_connector_index_p != FBE_ENCLOSURE_VALUE_INVALID)
                {
                    encl_status = FBE_ENCLOSURE_STATUS_OK;
                }
            }
        }
    }

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "get connector entire_connector_index failed, conn_id: %d, encl_status: 0x%x.\n", 
                                 connector_id,
                                 encl_status); 
    }

    return encl_status;

} // End of - fbe_eses_enclosure_get_connector_entire_connector_index
                   
/*!***************************************************************
 * @fn fbe_eses_enclosure_get_expansion_port_entire_connector_index(
 *                                     fbe_eses_enclosure_t * eses_enclosure,
 *                                     fbe_u8_t connector_id,
 *                                     fbe_u8_t * expansion_port_entire_connector_index_p)
 ****************************************************************
 * @brief:
 *    This function gets the entire connector index of the expansion port.
 *
 * @param  eses_enclosure (INPUT)- The pointer to the eses enclosure.
 * @param  eses_enclosure (INPUT)- connector id of the port.
 * @param  expansion_port_entire_connector_index_p (OUTPUT) - The pointer to the expansion port entire connector index.
 *
 * @return  fbe_enclosure_status_t 
 *         FBE_ENCLOSURE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    11/07/2008: PHE - Created. 
 *
 ****************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_get_expansion_port_entire_connector_index(fbe_eses_enclosure_t * eses_enclosure,
                                                      fbe_u8_t connector_id,
                                                      fbe_u8_t * expansion_port_entire_connector_index_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;

    /********
     * BEGIN
     ********/        
    *expansion_port_entire_connector_index_p = FBE_ENCLOSURE_VALUE_INVALID;

    /* The connectors saved in EDAL is in the order of sides, side A followed by side B.
     * All the local connectors are in the will be in contiguous order.
     * First, find the first local connector index. Other local connectors follow.
     */
    matching_conn_index = 
            fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    0, //starting index
                                                    TRUE);

    if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        /* There must be expansion port among the local connectors.
         * Second, find the first expansion port.
         */
        matching_conn_index = 
            fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_ID,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    matching_conn_index, //starting index
                                                    connector_id);

        if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
        {
            *expansion_port_entire_connector_index_p = 
                        fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    matching_conn_index,     // Component index.
                                                    TRUE);

            if(*expansion_port_entire_connector_index_p != FBE_ENCLOSURE_VALUE_INVALID)
            {
                encl_status = FBE_ENCLOSURE_STATUS_OK;
            }
        }
    }
    
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "get expansion port entire conn index failed, connector id %d, encl_status: 0x%x.\n", 
                                 connector_id,
                                 encl_status); 
    }

    return encl_status;

} // End of - fbe_eses_enclosure_get_expansion_port_entire_connector_index
                   
/**************************************************************************
* fbe_eses_enclosure_get_local_sas_exp_elem_index()
***************************************************************************
***TO BE MOVED TO ESES Enclosure Class
* DESCRIPTION
*   This function gets the element index of the local SAS Expander. 
*   There is only one local SAS Expander per LCC.
*
* PARAMETERS
*   eses_enclosure - The pointer to the sas eses enclosure.
*   elem_index_p - The pointer to the local SAS Expander element index. 
*
* RETURN VALUES
*   fbe_enclosure_status_t 
*
* NOTES
*   It can be added to the EDAL if we feel there is a need.
*
* HISTORY
*   20-Oct-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_local_sas_exp_elem_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_u8_t *elem_index_p)
{
    fbe_eses_elem_group_t * elem_group = NULL;
    fbe_u8_t              group_id = 0;
   
    /*********
     * BEGIN
     *********/
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
        
    for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++)
    {
        if((fbe_eses_elem_group_get_subencl_id(elem_group, group_id) == SES_SUBENCL_ID_PRIMARY) &&
           (fbe_eses_elem_group_get_elem_type(elem_group, group_id) == SES_ELEM_TYPE_SAS_EXP))
        {
            /* There is only one SAS Expander element on local LCC. */
            *elem_index_p = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);
            return FBE_ENCLOSURE_STATUS_OK;
        }
    }



    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_WARNING,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, fail to find the local SAS Expander element index.\n", 
                                __FUNCTION__ );

    return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
}// End of function fbe_eses_enclosure_get_local_sas_exp_elem_index


/*!*************************************************************************
* @fn fbe_eses_elem_index_to_byte_offset(fbe_eses_elem_group_t * elem_group, 
*                                            fbe_u8_t elem_index)                  
***************************************************************************
* @brief
*       This function returns the byte offset in the control/status page 
*       for the specified element index. 
*
* @param eses_enclosure - The pointer to the eses enclosure.
* @param elem_index - The element index.
*
* @return  fbe_u16_t - The byte offset in the control/status page.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_u16_t fbe_eses_elem_index_to_byte_offset(fbe_eses_enclosure_t *eses_enclosure,
                                             fbe_u8_t elem_index)
{
    fbe_u8_t group_id = 0;
    fbe_u8_t first_elem_index = 0;
    fbe_u16_t elem_byte_offset = 0;
    fbe_eses_elem_group_t * elem_group = NULL;

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index);
    first_elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);
    elem_byte_offset = fbe_eses_elem_group_get_elem_byte_offset(elem_group, group_id, elem_index - first_elem_index + 1);
    return elem_byte_offset;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_index_to_elem_offset(fbe_eses_enclosure_t * eses_enclosure,
*                                            fbe_u8_t elem_index;
*                                             fbe_u8_t * elem_offset_p)        
***************************************************************************
* @brief
*       This function translates the ESES element index to ESES element offset.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_index (Input)- The ESES element index.
* @param  elem_offset_p (output) - The pointer to the ESES element offset.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - no error. 
*          FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND - an error occurred.
*
* NOTES
*
* HISTORY
*  28-Aug-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_elem_index_to_elem_offset(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_index,
                                             fbe_u8_t * elem_offset_p)
{
    fbe_u8_t group_id;
    fbe_enclosure_status_t encl_status;

    group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index);

    if(group_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        *elem_offset_p = FBE_ENCLOSURE_VALUE_INVALID;
        encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    }
    else
    {
        *elem_offset_p = elem_index + group_id + 1;
        encl_status = FBE_ENCLOSURE_STATUS_OK;
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_offset_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
*                                             fbe_u8_t elem_index;
*                                             fbe_u8_t * elem_offset_p)        
***************************************************************************
* @brief
*       This function translates the ESES element offset to ESES element index.
*       If this is the overall element, the elem index will be SES_ELEM_INDEX_NONE.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_index (Input)- The ESES element index.
* @param  elem_offset_p (output) - The pointer to the ESES element offset.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - no error. 
*          FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND - an error occurred.
*
* NOTES
*
* HISTORY
*  28-Aug-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_elem_offset_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_offset,
                                             fbe_u8_t * elem_index_p)
{
    fbe_u8_t group_id;
    fbe_bool_t overall_elem = FALSE;
    fbe_u8_t num_possible_elems = 0;
    fbe_eses_elem_group_t * elem_group = NULL;

    *elem_index_p = SES_ELEM_INDEX_NONE;

    group_id = fbe_eses_elem_group_get_elem_offset_group_id(eses_enclosure, 
                         elem_offset,  
                         &overall_elem); 

    if(group_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    }

    // The overall element has element offset assigned to it but not element index.
    if(!overall_elem)
    {
        elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

        num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

        if(num_possible_elems == 0)
        {
            *elem_index_p = SES_ELEM_INDEX_NONE;    
        }
        else
        {
            *elem_index_p = elem_offset - group_id -1;
        }
    }

    return FBE_ENCLOSURE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_elem_offset_to_comp_type_comp_index(fbe_eses_enclosure_t * eses_enclosure,
*                                             fbe_u8_t elem_offset,
*                                             fbe_u8_t * comp_type,
*                                             fbe_u8_t * comp_index)
***************************************************************************
* @brief
*       This function translates the ESES element offset to EDAL component type and 
*       component index.
*       If this is the overall element, the elem index will be SES_ELEM_INDEX_NONE.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_offset (Input)- The ESES element offset.
* @param  comp_type_p (output) - The pointer to the EDAL component type.
* @param  comp_index_p (output) - The pointer to the EDAL component index.
*
* @return fbe_enclosure_status_t 
*          FBE_ENCLOSURE_STATUS_OK - no error. 
*          FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED - It is not present in EDAL.
*          otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*  28-Aug-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_elem_offset_to_comp_type_comp_index(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_offset,
                                             fbe_enclosure_component_types_t * comp_type_p,
                                             fbe_u8_t * comp_index_p)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                group_id = 0;
    fbe_u8_t                elem = 0;
    fbe_bool_t              overall_elem = FALSE;
    fbe_eses_elem_group_t   * elem_group = NULL;
    fbe_u8_t                elem_index = 0;
    fbe_u8_t                first_elem_index = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s entry \n", __FUNCTION__);

    group_id = fbe_eses_elem_group_get_elem_offset_group_id(eses_enclosure, 
                        elem_offset,  
                        &overall_elem); 

    if(group_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    }
    
    encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure,
                                                    group_id,
                                                    comp_type_p);

    if (!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }     

    if(overall_elem)
    {
        /* elem  is the order of the element in the ESES group. 
        * 0 is the overall element.
        */
        elem = 0;
    }
    else
    {  
        elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

        first_elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);

        encl_status = fbe_eses_enclosure_elem_offset_to_elem_index(eses_enclosure, elem_offset, &elem_index);

        if (!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }        

        /* elem  is the order of the element in the ESES group. 
        * The individual element starts from 1.
        */
        elem = elem_index - first_elem_index + 1;
    }

    encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure, group_id, elem, comp_index_p);
    return encl_status;

} // End of - fbe_eses_enclosure_elem_offset_to_comp_type_comp_index

// End of file fbe_eses_enclosure_mapping.c
