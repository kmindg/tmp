/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008 - 2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_process_pages.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file process the ESES pages to populate the 
 *  configuration and mapping information.   
 *  
 * @ingroup fbe_enclosure_class
 *
 * HISTORY:
 *  05-Aug-2008 PHE - Created.                  
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_eses.h"
#include "fbe_eses_enclosure_config.h"
#include "fbe_eses_enclosure_private.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_sas_enclosure_debug.h"

/**************************
 * GLOBALS 
 **************************/

extern const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_comp_type_hdlr_table[]; 


/**************************
 * FORWARD DECLARATIONS 
 **************************/
static fbe_enclosure_status_t fbe_eses_enclosure_parse_subencl_descs(fbe_eses_enclosure_t * eses_enclosure,
                                                              ses_pg_config_struct *config_page_p);

static fbe_enclosure_status_t fbe_eses_enclosure_parse_type_descs(fbe_eses_enclosure_t * eses_enclosure,
                                                     ses_pg_config_struct *config_page_p);

static fbe_enclosure_status_t fbe_eses_enclosure_parse_version_desc(fbe_eses_enclosure_t * eses_enclosure_p,
                                                     ses_subencl_desc_struct *subencl_desc_p);

static fbe_bool_t fbe_eses_is_min_fw_rev(fbe_eses_enclosure_t *eses_enclosurep,fbe_u32_t side_id);

static fbe_enclosure_status_t fbe_eses_enclosure_type_desc_set_edal_attr(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u8_t group_id,
                                           fbe_u8_t elememt,
                                           fbe_u8_t elem_index);

static fbe_enclosure_status_t fbe_eses_enclosure_process_array_dev_slot_addl_elem_stat(
                                           fbe_eses_enclosure_t * eses_enclosure,
                                           ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p);

static  fbe_enclosure_status_t fbe_eses_enclosure_process_sas_exp_addl_elem_stat(
                                       fbe_eses_enclosure_t * eses_enclosure,
                                       ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p);

static fbe_u8_t fbe_eses_enclosure_is_new_connector(fbe_eses_enclosure_t * eses_enclosure, 
                                                      fbe_u8_t connector_elem_index);

static fbe_enclosure_status_t fbe_eses_enclosure_process_connector_phy_desc(fbe_eses_enclosure_t * eses_enclosure, 
                                                  fbe_sas_address_t expander_sas_address,
                                                  ses_sas_exp_phy_desc_struct * phy_desc_p,
                                                  fbe_bool_t process_entire_connector,
                                                  fbe_u8_t phy_index);

static fbe_enclosure_status_t fbe_eses_enclosure_update_shutdown_info(
                                                    fbe_eses_enclosure_t * eses_enclosure, 
                                                    ses_pg_emc_encl_stat_struct * emc_specific_status_page_p);
                                          
static fbe_enclosure_status_t fbe_eses_enclosure_process_sas_conn_info(
                                                     fbe_eses_enclosure_t * eses_enclosure, 
                                                     fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) ;

static fbe_enclosure_status_t fbe_eses_enclosure_process_trace_buf_info(
                                                         fbe_eses_enclosure_t * eses_enclosure, 
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p);

static fbe_enclosure_status_t
fbe_eses_enclosure_process_ps_info(fbe_eses_enclosure_t * eses_enclosure,
                                   fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) ;

static fbe_enclosure_status_t
fbe_eses_enclosure_process_sps_info(fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) ;

static fbe_enclosure_status_t 
fbe_eses_enclosure_process_input_power_sample(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t    ps_component_index,
                                              fbe_bool_t  sample_valid, 
                                              fbe_u16_t   input_power_sample); 

static fbe_bool_t 
fbe_eses_enclosure_is_input_power_update_needed(fbe_eses_enclosure_t * eses_enclosure);


static fbe_enclosure_status_t
fbe_eses_enclosure_process_general_info(fbe_eses_enclosure_t * eses_enclosure,
                                        fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) ;

static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_expander(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p);

static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_drive_slot(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p,
    fbe_bool_t *battery_backed_updated);

static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_temp_sensor(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p);

static fbe_bool_t 
fbe_eses_enclosure_is_TS_temp_update_required(fbe_eses_enclosure_t * eses_enclosure);

static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_esc_electronics(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p);

static fbe_enclosure_status_t fbe_eses_enclosure_process_eep_mode_page(
                                    fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_eses_pg_eep_mode_pg_t * eep_mode_pg_p);

static fbe_enclosure_status_t fbe_eses_enclosure_process_eenp_mode_page(
                                    fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_eses_pg_eenp_mode_pg_t * eenp_mode_pg_p);

static fbe_enclosure_status_t 
fbe_eses_enclosure_parse_buffer_descs(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_enclosure_component_types_t component_type,
                                      fbe_u8_t component_index,
                                      ses_buf_desc_struct *bufferDescPtr);

static fbe_enclosure_status_t fbe_eses_enclosure_process_slot_statistics(
                                          fbe_eses_enclosure_t * eses_enclosure, 
                                          fbe_u8_t slot_index,
                                          fbe_eses_device_slot_stats_t * slot_elem_statistics_p);

static fbe_enclosure_status_t
fbe_eses_enclosure_process_unused_connector(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_u8_t connector_id);

static fbe_enclosure_status_t 
fbe_eses_enclosure_subencl_desc_get_subencl_slot(fbe_eses_enclosure_t *eses_enclosure_p,
                                        ses_subencl_desc_struct *subencl_desc_p,
                                        fbe_u8_t *subencl_slot_p);

static fbe_enclosure_status_t fbe_eses_enclosure_subencl_text_to_ps_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                    fbe_char_t * subencl_text_p,
                                                    fbe_u8_t subencl_text_len,
                                                    fbe_u8_t * ps_slot_p);

static fbe_enclosure_status_t 
fbe_eses_enclosure_subencl_save_prod_rev_level(fbe_eses_enclosure_t * eses_enclosure,
                                               ses_subencl_desc_struct *subencl_desc_p,
                                               fbe_u8_t component_index);

static fbe_enclosure_status_t 
fbe_eses_enclosure_convert_subencl_prod_rev_level_to_fw_rev(fbe_eses_enclosure_t * eses_enclosure,
                                                            ses_subencl_desc_struct *subencl_desc_p,
                                                            fbe_u8_t *pFirmwareRev);

static fbe_status_t fbe_eses_enclosure_get_adjusted_lcc_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                      fbe_u8_t *pSubenclProductId,
                                                      fbe_u8_t originalLccSlot,
                                                      fbe_u8_t * pAdjustedLccSlot);

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_configuration_page(fbe_eses_enclosure_t * eses_enclosure,                                                                
*                                                               fbe_u8_t * config_page)              
***************************************************************************
* @brief
*   This function processes the config page to build the element group and
*   the element list. This is done by parsing the subenclosure descriptor
*   list and the type descriptor list.
*
* @param  eses_enclosure - The pointer to the eses enclosure.
* @param  config_page - The pointer to the configuration page.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
*   15-Sep-2008 hgd - Added support for cooling elements.
*   02-Oct-208 gb - split this function into multiple functions
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_configuration_page(fbe_eses_enclosure_t * eses_enclosure,                                                                
                                                                fbe_u8_t * config_page)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_DEBUG_HIGH,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s entry\n", __FUNCTION__ );

    if(eses_enclosure->enclosureConfigBeingUpdated == FALSE)
    {
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Process_configuration_page, start of setting mapping.\n");
                            
    }

    // Initialize the configuration and mapping related info.
    if((encl_status = fbe_eses_enclosure_init_config_info(eses_enclosure)) != FBE_ENCLOSURE_STATUS_OK)
    {   
        // Fail to initialize the configuration and mapping info. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "process_configuration_page, init_config_info failed, encl_status: 0x%x.\n", 
                          encl_status);
        return encl_status;
    }

    // get the generation code
    eses_enclosure->eses_generation_code = fbe_eses_get_pg_gen_code((ses_common_pg_hdr_struct *)config_page);

    /* Set the generation code */
    encl_status = fbe_base_enclosure_edal_setU32((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_GENERATION_CODE, // Attribute.
                                        FBE_ENCL_ENCLOSURE,  // component type.
                                        0, // only 1 enclosure
                                        eses_enclosure->eses_generation_code);  // The value to be set to.

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // parse each subenclosure descriptor
    encl_status = fbe_eses_enclosure_parse_subencl_descs(eses_enclosure, 
                                                             (ses_pg_config_struct *)config_page);

    /*if((encl_status != FBE_ENCLOSURE_STATUS_OK) &&
        (encl_status != FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED))*/
    if(!ENCL_STAT_OK(encl_status))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "process_configuration_page, parse_subencl_descs failed, encl_status: 0x%x.\n", 
                          encl_status);
        return encl_status;
    }

    // parse each type descriptor
    encl_status = fbe_eses_enclosure_parse_type_descs(eses_enclosure, 
                                                     (ses_pg_config_struct *)config_page);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "process_configuration_page, parse_type_descs failed, encl_status: 0x%x.\n", 
                         encl_status);
    }

    return encl_status;

} // End of function fbe_eses_enclosure_process_configuration_page

/*!*************************************************************************
* @fn static fbe_enclosure_status_t fbe_eses_enclosure_temp_sensor_reinit_side_id(fbe_eses_enclosure_t * eses_enclosure, 
*                                                                                 fbe_eses_subencl_side_id_t side_id)
***************************************************************************
* @brief
*   This function sets the LCC Temp Sensor Side Id in config info.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  side_id (Input) - side id for temp sennsor.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   08-Sept-2014 RLP - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_temp_sensor_reinit_side_id(fbe_eses_enclosure_t * eses_enclosure, 
                                                                            fbe_eses_subencl_side_id_t side_id)
{
    fbe_edal_status_t      edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_u32_t              temp_sensor_index = 0;
    fbe_u8_t               number_of_components = 0;
    fbe_edal_general_comp_handle_t      specific_component_data_ptr = NULL; 
    fbe_u8_t  number_of_lccs_with_temp_sensor = fbe_eses_enclosure_get_number_of_lccs_with_temp_sensor(eses_enclosure);
    fbe_u8_t  number_of_temp_sensors_per_lcc = fbe_eses_enclosure_get_number_of_temp_sensors_per_lcc(eses_enclosure);  

    if (number_of_lccs_with_temp_sensor == 1)
    {

        status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_ENCL_TEMP_SENSOR, // Component type
                                                          &number_of_components); 

        if(status != FBE_STATUS_OK)
        {
            status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                             FBE_ENCL_TEMP_SENSOR,
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

        for(temp_sensor_index = 0; temp_sensor_index < number_of_components; temp_sensor_index ++)
        {


            if(temp_sensor_index < number_of_lccs_with_temp_sensor * number_of_temp_sensors_per_lcc)
            {

                status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_TEMP_SENSOR,  // Component type.
                                                                    temp_sensor_index, // Component index.
                                                                    &specific_component_data_ptr);
                if(status != FBE_STATUS_OK)
                {
                    return FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED;
                }

                /* Set the FBE_ENCL_COMP_SIDE_ID attribute in EDAL. */
                edalStatus = eses_enclosure_access_setU8(specific_component_data_ptr,
                                                        FBE_ENCL_COMP_SIDE_ID, //Attribute.
                                                        FBE_ENCL_TEMP_SENSOR,//Component type.
                                                        side_id); // side_id

                if(!EDAL_STAT_OK(edalStatus))
                {
                    return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
                }
            }
        }// End of - for(i = 0; i < number_of_components; i ++)
    }

    return FBE_ENCLOSURE_STATUS_OK;

} // End of function fbe_eses_enclosure_temp_sensor_reinit_side_id

/*!*************************************************************************
* @fn fbe_eses_enclosure_parse_subencl_descs(fbe_eses_enclosure_t * eses_enclosure,
*                                                  ses_pg_config_struct *config_page_p)                 
***************************************************************************
* @brief
*   This function processes the subenclosure descriptors in the 
*   configuration page. The parsed data gets copied to the EDAL area.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  config_pg_p (Input) - pointer to the configuration page
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
*   15-Sep-2008 hgd - Added support for cooling elements.
*   01-Oct-2008 GB - Moved from fbe_eses_enclosure_process_configuration_page.
*   11-Nov-2008 PHE - Consolidated the code to use the common functions.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_parse_subencl_descs(fbe_eses_enclosure_t * eses_enclosure,
                                                              ses_pg_config_struct *config_page_p)
{
    fbe_u8_t subencl = 0; // Loop through all the subenclosures.  
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t total_num_subencls = 0;
    ses_subencl_desc_struct * subencl_desc_p = NULL;
    fbe_enclosure_component_types_t component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u8_t component_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                *bufferDescCount;
    ses_buf_desc_struct     *bufferDescPtr;
    fbe_u8_t                index;
    fbe_u8_t                subtype;
    fbe_u8_t                num_ps_subelements;
    fbe_u8_t                num_fan_components = 0;
    fbe_u8_t                cooling_subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t                ps_slot = 0;
    fbe_u8_t                adjustedLccSlot = 0;
    /*********
     * BEGIN
     *********/
    // Get the pointer to the first subencl descriptor.
    subencl_desc_p = &config_page_p->subencl_desc;

    // Get the total number of subenclosures.
    // (Adding 1 for the primary subenclosure).
    total_num_subencls = config_page_p->num_secondary_subencls + 1;

    // Process the subenclosure descriptor list.
    for(subencl = 0; subencl < total_num_subencls; subencl ++)           
    {
        switch(subencl_desc_p->subencl_type) 
        {
            case SES_SUBENCL_TYPE_PS:               
                component_type = FBE_ENCL_POWER_SUPPLY;
                // added for temp debug help
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_INFO,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "parse_subencl_descs, Powersupply side %d\n", 
                                                           subencl_desc_p->side);

                encl_status = fbe_eses_enclosure_subencl_desc_get_subencl_slot(eses_enclosure, subencl_desc_p, &ps_slot);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }
                
                if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
                {
                    num_ps_subelements = 1;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_DEBUG_LOW,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "parse_subencl_descs, forcing ps_subelements to 1\n");
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                               FBE_TRACE_LEVEL_DEBUG_LOW,
                                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                               "parse_subencl_descs, setting ps_subelements to %d\n",
                                                               num_ps_subelements);
                }

                component_index = ps_slot * num_ps_subelements; //Map side Id into the first element in each bucket.

                /* Save the PS subenclosure product revision level
                 * Please note this needs to be done before parsing the version descriptor.
                 * We are not sure whether the CDES poulated this field correctly before Viking. 
                 * If we do this after parsing version descriptor,
                 * it could overwrite the version from the version descriptor for the PS when
                 * there is only one PS individual element saved in the EDAL.
                 */
                encl_status = fbe_eses_enclosure_subencl_save_prod_rev_level(eses_enclosure,
                                                                            subencl_desc_p,
                                                                            component_index);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                  return encl_status;
                }

                // parse the power supply version descriptor list
                encl_status = fbe_eses_enclosure_parse_version_desc(eses_enclosure, subencl_desc_p);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }
                // save the PS serial number
                encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_SERIAL_NUMBER,
                                            FBE_ENCL_POWER_SUPPLY,
                                            component_index,
                                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                            subencl_desc_p->subencl_ser_num);

               if (encl_status != FBE_ENCLOSURE_STATUS_OK )
               {
                  return encl_status;
               }

               /* Save the PS subenclosure product id.
                * If this subenclosure has downloadable firmware, this field matches one of the 
                * SUBENCLOSURE PRODUCT IDENTIFICATION fields in the microcode file header
                * This is the primary way the client can determine whether an image file is 
                * suitable for download to this subenclosure.
                */
               encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_PRODUCT_ID,
                                            FBE_ENCL_POWER_SUPPLY,
                                            component_index,
                                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                            subencl_desc_p->subencl_prod_id);

               if (encl_status != FBE_ENCLOSURE_STATUS_OK )
               {
                  return encl_status;
               }
               
               break;
                
            case SES_SUBENCL_TYPE_LCC:            
                component_type = FBE_ENCL_LCC;
                // we need to record side id first. Because we need this to get the local side id
                // for other functions such as fbe_eses_enclosure_parse_version_desc() 
                if (subencl_desc_p->subencl_id == SES_SUBENCL_ID_PRIMARY)
                {
                    // set the enclosure side id.
                    // Should the component index (4th arg) be 0? as while getting encl side,
                    // I made change to have component_index as always 0.
                    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                                FBE_ENCL_ENCLOSURE,      // Component type.
                                                                0,
                                                                subencl_desc_p->side % ESES_SUPPORTED_SIDES);  //total 2 sides
                    /*if((encl_status != FBE_ENCLOSURE_STATUS_OK) &&
                        (encl_status != FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED))*/
                    if (encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_INFO,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "parse_subencl_descs, encl side id change detected. new value: 0x%x.\n", 
                                                           subencl_desc_p->side % ESES_SUPPORTED_SIDES);
                    }

                    if (!ENCL_STAT_OK(encl_status))
                    {
                        return encl_status;
                    }

                    fbe_eses_enclosure_temp_sensor_reinit_side_id(eses_enclosure, 
                                                                  subencl_desc_p->side % ESES_SUPPORTED_SIDES);

                }

                fbe_eses_enclosure_get_adjusted_lcc_slot(eses_enclosure, 
                                                         &subencl_desc_p->subencl_prod_id[0],
                                                         subencl_desc_p->side,
                                                         &adjustedLccSlot);

                component_index = fbe_eses_enclosure_get_next_available_lcc_index(eses_enclosure);
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_LOCATION,  // Attribute.
                                                                FBE_ENCL_LCC,      // Component type.
                                                                component_index,
                                                                adjustedLccSlot);
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                                FBE_ENCL_LCC,      // Component type.
                                                                component_index,
                                                                subencl_desc_p->side % ESES_SUPPORTED_SIDES);  //total 2 sides
                         

                // parse the LCC version descriptor list 
                encl_status = fbe_eses_enclosure_parse_version_desc(eses_enclosure, subencl_desc_p);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }

                if (subencl_desc_p->subencl_id == SES_SUBENCL_ID_PRIMARY)
                {
                    // check for minimum fw rev
                    if (!fbe_eses_is_min_fw_rev(eses_enclosure,subencl_desc_p->side))
                    {
                        encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure,
                                                                       component_type,
                                                                       component_index,
                                                                       FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
                                                                       0);
                    }
                    // trace out the error if we can't handle the error
                    if (!ENCL_STAT_OK(encl_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_ERROR,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                                           "%s can't set FAIL condition, status: 0x%X\n",
                                                           __FUNCTION__, encl_status);
                    }

                }
                // save the LCC serial number
                encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                         FBE_ENCL_SUBENCL_SERIAL_NUMBER,
                                         FBE_ENCL_LCC,
                                         component_index,
                                          FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                          subencl_desc_p->subencl_ser_num);

                 if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                 {
                      return encl_status;
                 } 

                /* Save subencl product id */
                encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                         FBE_ENCL_SUBENCL_PRODUCT_ID,
                                         FBE_ENCL_LCC,
                                         component_index,
                                         FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                         subencl_desc_p->subencl_prod_id);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }
                
                // Now we set fan side id of this LCC subenclosure.
                 /* NOTE - The following code does not look corret to me - PINGHE.
                 * It just sets the side id of all the cooling components which are LCC internal 
                 * to the local LCC side id.
                 * Since this info is not used anywhere in the code yet, I did not try to fix it.
                 */ 
                if (subencl_desc_p->subencl_id == SES_SUBENCL_ID_PRIMARY)
                {              
                    // Now we set fan side id of this LCC subenclosure. 
                    component_type = FBE_ENCL_COOLING_COMPONENT;
                    encl_status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure, component_type, &num_fan_components);
                    for (index = 0; index < num_fan_components; index++)
                    {
                         /* Get the FBE_ENCL_COMP_SUBTYPE attribute in EDAL. */
                         encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_SUBTYPE, //Attribute.
                                                    component_type,//Component type.
                                                    index,
                                                    &cooling_subtype);

                        if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                        {
                            return encl_status;
                        }

                         /* Get the FBE_ENCL_COMP_SUBTYPE attribute in EDAL. */
                         encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_SUBTYPE, //Attribute.
                                                    component_type,//Component type.
                                                    index,
                                                    &cooling_subtype);

                        if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                        {
                            return encl_status;
                        }

                        // We only deal with FAN in LCC subenclosure. 
                        if (cooling_subtype == FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL)
                        {
                            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                                        component_type,      // Component type.
                                                                        index,
                                                                        subencl_desc_p->side % ESES_SUPPORTED_SIDES);  //total 2 sides
                            if (!ENCL_STAT_OK(encl_status))
                            {
                                return encl_status;
                            }
                        }
                    }

                    component_type = FBE_ENCL_LCC;
                }
                break;
            
            case SES_SUBENCL_TYPE_CHASSIS:                    
                component_type = FBE_ENCL_ENCLOSURE;
                component_index = 0; // Only 1 enclosure.
                break;

            case SES_SUBENCL_TYPE_UPS:                    
                component_type = FBE_ENCL_SPS;
                component_index = 0; // Only 1 SPS.

                // Save the SPS serial number
                encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_SERIAL_NUMBER,
                                            FBE_ENCL_SPS,
                                            component_index,
                                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                            subencl_desc_p->subencl_ser_num);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                   return encl_status;
                }
                /* Save the SPS subenclosure product id.
                * If this subenclosure has downloadable firmware, this field matches one of the 
                * SUBENCLOSURE PRODUCT IDENTIFICATION fields in the microcode file header
                * This is the primary way the client can determine whether an image file is 
                * suitable for download to this subenclosure.
                */
                if (subencl_desc_p->subencl_id != FBE_ENCLOSURE_VALUE_INVALID)
                {
                    encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_SUBENCL_PRODUCT_ID,
                                                FBE_ENCL_SPS,
                                                component_index,
                                                FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                                subencl_desc_p->subencl_prod_id);
                
                
                    if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                    {
                       return encl_status;
                    }
                }
               
                /* Save the SPS side id. */
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                            FBE_ENCL_SPS,      // Component type.
                                                            component_index,
                                                            subencl_desc_p->side);  
                if (!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }

                // parse the SPS version descriptor list 
                encl_status = fbe_eses_enclosure_parse_version_desc(eses_enclosure, subencl_desc_p);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }

                break;

            case SES_SUBENCL_TYPE_COOLING:
                component_type = FBE_ENCL_COOLING_COMPONENT;

                /* use the component index for the overall element to store subenelcosure related info */
                encl_status = 
                        fbe_eses_enclosure_map_cooling_elem_to_component_index(eses_enclosure,
                                                                            SES_SUBENCL_TYPE_COOLING,
                                                                            0,
                                                                            subencl_desc_p->side,
                                                                            &component_index,
                                                                            &subtype);
                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }

                // parse the cooling module version descriptor list 
                encl_status = fbe_eses_enclosure_parse_version_desc(eses_enclosure, subencl_desc_p);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK )
                {
                    return encl_status;
                }
                // save the fan serial number
                encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_SERIAL_NUMBER,
                                            FBE_ENCL_COOLING_COMPONENT,
                                            component_index,
                                            FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                            subencl_desc_p->subencl_ser_num);

               if (encl_status != FBE_ENCLOSURE_STATUS_OK )
               {
                  return encl_status;
               }
               /* Save the Cooling Module subenclosure product id.
                * If this subenclosure has downloadable firmware, this field matches one of the 
                * SUBENCLOSURE PRODUCT IDENTIFICATION fields in the microcode file header
                * This is the primary way the client can determine whether an image file is 
                * suitable for download to this subenclosure.
                */
               if (subencl_desc_p->subencl_id != FBE_ENCLOSURE_VALUE_INVALID)
               {
                   encl_status = fbe_base_enclosure_edal_setStr((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_SUBENCL_PRODUCT_ID,
                                                FBE_ENCL_COOLING_COMPONENT,
                                                component_index,
                                                FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                                subencl_desc_p->subencl_prod_id);
               }

               if (encl_status != FBE_ENCLOSURE_STATUS_OK )
               {
                  return encl_status;
               }

               break;


            default:
                component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
                component_index = FBE_ENCLOSURE_VALUE_INVALID;
                break;
        }// End of - switch(subencl_desc_p->subencl_type) 

        if(component_type != FBE_ENCL_INVALID_COMPONENT_TYPE)
        {
            // Set the subenclosure id.
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SUB_ENCL_ID,  // Attribute.
                                                        component_type,      // Component type.
                                                        component_index,
                                                        subencl_desc_p->subencl_id);
            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            // get the number of buffer descriptors
            bufferDescCount = fbe_eses_get_num_buf_descs_p(subencl_desc_p);

            // get the address of the first buffer desc
            bufferDescPtr = fbe_eses_get_first_buf_desc_p(subencl_desc_p);

            // loop through all buffer descriptors
            for (index = 0; index < *bufferDescCount; index++, bufferDescPtr++)
            {
                encl_status = fbe_eses_enclosure_parse_buffer_descs(eses_enclosure, 
                                                                    component_type,
                                                                    component_index,
                                                                    bufferDescPtr);
            }

        }// End of - if(component_type != FBE_ENCL_INVALID_COMPONENT_TYPE)

        // Get the pointer to the next subenclosure descriptor.
        subencl_desc_p = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_p);

    } // End of Processing the subenclosure descriptor list 

    return FBE_ENCLOSURE_STATUS_OK;

} // fbe_eses_enclosure_parse_subencl_descs


/*!*************************************************************************
 *   @fn fbe_eses_enclosure_get_adjusted_lcc_slot(fbe_base_enclosure_t * pBaseEncl,
 *                                             fbe_u8_t *pSubenclProductId,
 *                                             fbe_u32_t originalLccSlot,
 *                                             fbe_u32_t * pAdjustedLccSlot)   
 **************************************************************************
 *  @brief
 *      This function will get the adjust lcc slot.
 *      The terminator uses slot 0 and 1 for the subencl LCC.
 *      However, on real hardware, we see slot 2 and 3 used for the subencl LCC
 *      So this function adjusts the LCC slot so that it will be OK for both terminator or hardware.
 *      After the terminator fixs this issue, this function can be removed.
 *
 *  @param pBaseEncl - The pointer to the fbe_base_enclosure_t.
 *  @param pSubenclProductId - The pointer to the subEncl Product Id.
 *  @param originalLccSlot - The LCC slot saved in the EDAL.
 *  @param pAdjustedLccSlot(OUTPUT) - The pointer to the LCC slot adjusted.
 * 
 *  @return fbe_status_t always return FBE_STATUS_OK.
 * 
 *  @version
 *    04-Feb-2014: PHE - Created. 
 **************************************************************************/
static fbe_status_t fbe_eses_enclosure_get_adjusted_lcc_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                      fbe_u8_t *pSubenclProductId,
                                                      fbe_u8_t originalLccSlot,
                                                      fbe_u8_t * pAdjustedLccSlot)
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s, originalLccSlot %d.\n", 
                         __FUNCTION__, originalLccSlot);  


    if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VOYAGER_EE_PRODUCT_ID_STR, 11) == 0)
    {
        if((originalLccSlot == 0) || (originalLccSlot == 1))
        {
            *pAdjustedLccSlot = (originalLccSlot == 0) ? 2 : 3;
        }
        else
        {
            *pAdjustedLccSlot = originalLccSlot;
        }
    }
    else
    {
        *pAdjustedLccSlot = originalLccSlot;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_parse_type_descs(fbe_eses_enclosure_t * eses_enclosure,
*                                         ses_pg_config_struct *config_page_p)                  
***************************************************************************
* @brief
*   This function processes the type descriptors in the 
*   configuration page. The parsed data gets copied to the EDAL area.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  config_pg_p (Input) - pointer to the configuration page
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred. 
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
*   15-Sep-2008 hgd - Added support for cooling elements.
*   01-Oct-2008 GB - Moved from fbe_eses_enclosure_process_configuration_page.
*   11-Nov-2008 PHE - Consolidated the code to use the common function.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_parse_type_descs(fbe_eses_enclosure_t * eses_enclosure,
                                                     ses_pg_config_struct *config_page_p)
{
    fbe_u8_t            element = 0;
    fbe_u8_t            elem_index = 0;
    fbe_u8_t            group_id = 0;
    fbe_u16_t           offset_in_ctrl_stat_pg = 0;   
    fbe_u8_t            total_num_groups = 0;
    fbe_u8_t            subencl_count = 0;
    fbe_eses_elem_group_t   * elem_group = NULL;
    ses_subencl_desc_struct * subencl_desc_p = NULL;
    ses_type_desc_hdr_struct * elem_type_desc_hdr_p = NULL; 
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    /********
     * BEGIN
     ********/
  
    // Get the pointer to the first subencl descriptor.
    subencl_desc_p = &config_page_p->subencl_desc;

    // get to the end of the subenclosure descriptor list
    for(subencl_count = 0; subencl_count <= config_page_p->num_secondary_subencls; subencl_count ++ )
    {
        // add up the total number of type descriptor headers
        total_num_groups += subencl_desc_p->num_elem_type_desc_hdrs;

        // next desc
        subencl_desc_p = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_p);
    }

    //process the memory allocate for elem group
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    //the elem group number may different from the previous,
    //so we release the memory elem group at first
    if(elem_group != NULL)
    {
        fbe_memory_ex_release(elem_group);
        elem_group = NULL;
        fbe_eses_enclosure_set_elem_group(eses_enclosure, elem_group);
    }

    //allocate memory for elem_group
    elem_group = fbe_memory_ex_allocate( total_num_groups * sizeof(fbe_eses_elem_group_t) );
    if(elem_group == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_WARNING,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "%s, failed allocating element group\n",
                __FUNCTION__);
        return FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
    }

    // Initialize the element group informatio.
    fbe_zero_memory(elem_group,
            sizeof(fbe_eses_elem_group_t) * total_num_groups);
    fbe_eses_enclosure_set_elem_group(eses_enclosure, elem_group);

    // Set the number of actual elem groups.
    fbe_eses_enclosure_set_number_of_actual_elem_groups(eses_enclosure, total_num_groups);

    // After exiting the preceeding loop, subencl_desc_p should
    // be pointing to the first type descriptor

    // Process the type descriptor header list.
    elem_type_desc_hdr_p = (ses_type_desc_hdr_struct *)subencl_desc_p;
 
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);  

    offset_in_ctrl_stat_pg = FBE_ESES_PAGE_HEADER_SIZE; // first group_byte_offset in the control/status page.

    for(group_id = 0; group_id < total_num_groups; group_id ++)
    {
        /* Populate the element group.*/
        fbe_eses_elem_group_set_elem_type(elem_group, group_id, elem_type_desc_hdr_p->elem_type); 
        fbe_eses_elem_group_set_num_possible_elems(elem_group, group_id, elem_type_desc_hdr_p->num_possible_elems);
        fbe_eses_elem_group_set_subencl_id(elem_group, group_id, elem_type_desc_hdr_p->subencl_id);
        fbe_eses_elem_group_set_byte_offset(elem_group, group_id, offset_in_ctrl_stat_pg);
        
        if(elem_type_desc_hdr_p->num_possible_elems == 0)
        {
            fbe_eses_elem_group_set_first_elem_index(elem_group, group_id, SES_ELEM_INDEX_NONE); 
        }
        else
        {
            fbe_eses_elem_group_set_first_elem_index(elem_group, group_id, elem_index); 
        }
        
        /* The element 0 is the overall element. The individual element starts from 1. 
         * The reason to start from element 0 is that 
         * we also save the overall element info as the container in EDAL for some component types. 
         */  
        for(element = 0; element <= elem_type_desc_hdr_p->num_possible_elems; element++)
        {
            encl_status = fbe_eses_enclosure_type_desc_set_edal_attr(eses_enclosure,
                                        group_id,
                                        element,
                                        elem_index);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            // Prepare to handle the next element.
            offset_in_ctrl_stat_pg += FBE_ESES_CTRL_STAT_ELEM_SIZE;

            if(element > 0)
            {
                /* The element 0 is the overall element. The overall element does not have the element index. 
                 * So only increment the element index for the element greater than 0.
                 */
                elem_index ++;
            }
        } 
     
        // Get the pointer to the next type descriptor header.
        elem_type_desc_hdr_p = (ses_type_desc_hdr_struct *)((fbe_u8_t *)elem_type_desc_hdr_p + FBE_ESES_ELEM_TYPE_DESC_HEADER_SIZE);
    }

    return FBE_ENCLOSURE_STATUS_OK;

} // fbe_eses_enclosure_parse_type_descs


/*!*************************************************************************
* @fn fbe_eses_enclosure_type_desc_set_edal_attr(fbe_eses_enclosure_t *eses_enclosure,
*                                           fbe_u8_t group_id,
*                                           fbe_u8_t element,
*                                           fbe_u8_t elem_index)                 
***************************************************************************
* @brief
*   This function saves the configration info parsed from the type descriptor
*   of the configuration page to the EDAL.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  group_id(Input) - The element group id.
* @param  element (Input) - The order of the element in the element group. 
* @param  elem_index - The element index of the element. 
* 
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*   11-Nov-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_type_desc_set_edal_attr(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u8_t group_id,
                                           fbe_u8_t element,
                                           fbe_u8_t elem_index)
{
    fbe_eses_elem_group_t          * elem_group = NULL;  
    fbe_u8_t                         component_index = FBE_ENCLOSURE_VALUE_INVALID;  
    fbe_enclosure_component_types_t  component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                         elem_index_in_edal = SES_ELEM_INDEX_NONE;
    fbe_u8_t                         subencl_id = 0;
    fbe_bool_t                       is_local_connector = FBE_FALSE;
    fbe_bool_t                       is_local = FBE_FALSE;

    /********
     * BEGIN
     ********/
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s entry \n", __FUNCTION__);

    // Get the component type in EDAL for this element group.
    encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure, group_id, &component_type);
                                                        
    if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
    {
        // Do not need to set the edal attributes for this element.
        return FBE_ENCLOSURE_STATUS_OK; 
    }
    else if(encl_status != FBE_ENCLOSURE_STATUS_OK) 
    {
        return encl_status;
    }

    switch(component_type)
    {                
        case FBE_ENCL_POWER_SUPPLY: 
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_TEMP_SENSOR:
        case FBE_ENCL_EXPANDER:
        case FBE_ENCL_LCC:
        case FBE_ENCL_ENCLOSURE:
        case FBE_ENCL_DISPLAY:
        case FBE_ENCL_CONNECTOR:
        case FBE_ENCL_DRIVE_SLOT:
        case FBE_ENCL_SPS:
        case FBE_ENCL_SSC:
            // Get the component index in EDAL for this element.
            encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                group_id,
                                element,
                                &component_index);

            if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
            {
                // Do not need to set the edal attributes for this element.
                return FBE_ENCLOSURE_STATUS_OK; 
            }
            else if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status; 
            }  

            if(element == 0)
            {
                elem_index_in_edal = SES_ELEM_INDEX_NONE;
            }
            else
            {
                elem_index_in_edal = elem_index;
            }

            // Set the FBE_ENCL_COMP_ELEM_INDEX attribute.
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  // Attribute.
                                                    component_type,         // Component type.
                                                    component_index,     // Component index.
                                                    elem_index_in_edal);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            } 

            if(component_type == FBE_ENCL_CONNECTOR)
            {
                elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
 
                /* Get the subencl_id of this element group. */      
                subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);
            
                /* Generally we will get 0 &1 as subenclid for viper and other enclosure
                   but for voyager it will come 0-3. So blow code will set is_local in pair of 
                   odd/even */

                is_local_connector = ((subencl_id % 2) == 0) ? FBE_TRUE : FBE_FALSE;

                /* Set the FBE_ENCL_COMP_IS_LOCAL attribute in EDAL. */
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_IS_LOCAL,  // Attribute
                                                            component_type,    // Component type
                                                            component_index,  //Index of the component 
                                                            is_local_connector); // The value to be set to

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }

                if((component_type == FBE_ENCL_CONNECTOR) && is_local_connector)
                {
                    /* Set the FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA attribute to TRUE. The function also
                    * sets FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT to FALSE
                    * so that the sas connector info element in EMC enclosure control page can be written
                    * after processing the configuration change. 
                    * Need to write the sas connector info element for all the LOCAL connectors which include
                    * all the individual lanes and all the entire connectors. 
                    */
                    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,  // Attribute
                                                            component_type,    // Component type
                                                            component_index,  //Index of the component 
                                                            TRUE); // The value to be set to

                    if(!ENCL_STAT_OK(encl_status))
                    {
                        return encl_status;
                    }
                }
            }
            else if(component_type == FBE_ENCL_LCC)
            {
                elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
 
                /* Get the subencl_id of this element group. */      
                subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);       

                /* Generally we will get 0 &1 as subenclid for viper and other enclosure
                   but for voyager it will come 0-3. So blow code will set is_local in pair of 
                   odd/even */

                is_local = ((subencl_id % 2) == 0) ? FBE_TRUE : FBE_FALSE;

                /* Set the FBE_ENCL_COMP_IS_LOCAL attribute in EDAL. */
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_IS_LOCAL,  // Attribute
                                                            component_type,    // Component type
                                                            component_index,  //Index of the component 
                                                            is_local); // The value to be set to

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }
            }
            break;

        case FBE_ENCL_EXPANDER_PHY:         
            /* Don't need to set up the configuration in EDAL here.  
             * The element index of the phy will be populated while extracting its status from the status page. 
             * The phys will be saved in the order of phy descriptors on the additional status page in EDAL.
             * If the phy is not connected to anything, no need to save the info for it.
             */
        default:
            // Don't need to set up the configuration in EDAL.           
            break;

    }// End of - switch(elem_type_desc_hdr_p->elem_type)

    return FBE_ENCLOSURE_STATUS_OK;
}// End of function fbe_eses_enclosure_type_desc_set_edal_attr   

/*!*************************************************************************
* @fn fbe_eses_enclosure_parse_version_desc(fbe_eses_enclosure_t * eses_enclosure_p,
*                                           ses_subencl_desc_struct *subencl_desc_p)              
***************************************************************************
* @brief
*   This function processes the version descriptor list for a given 
*   subenclosure descriptor in the Configuration Diagnostic Page. 
*   The parsed data gets copied to the EDAL area.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  subencl_desc_p (Input) - pointer to the subenclosure descriptor
*
* @return  fbe_enclosure_status_t 
*    FBE_ENCLOSURE_STATUS_OK - no error.
*    otherwise - an error occurred.
*
* NOTES
*
* HISTORY
*  01-Oct-2008 GB - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_parse_version_desc(fbe_eses_enclosure_t * eses_enclosure_p,
                                                     ses_subencl_desc_struct *subencl_desc_p)
{
    ses_ver_desc_struct     *version_desc_p;
    fbe_enclosure_fw_target_t   fw_target;
    fbe_edal_general_comp_handle_t generalDataPtr;
    fbe_eses_encl_fw_component_info_t *fw_info_p = NULL;
    fbe_u8_t                item = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_edal_status_t       edalStatus = FBE_EDAL_STATUS_OK;
    char                    revcopy[FBE_ESES_FW_REVISION_SIZE+1];
    char                    compcopy[FBE_ESES_FW_IDENTIFIER_SIZE+1];
    fbe_u8_t component_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                local_side_id = 0;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_edal_fw_info_t              fw_info;
    fbe_u8_t                subencl_slot = 0;
    fbe_u8_t                rev_id = 0;

    // set up the local variable to be used
    version_desc_p = subencl_desc_p->ver_desc;

    // get out local side id
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure_p,
                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute
                                                FBE_ENCL_ENCLOSURE, 
                                                0,
                                                &local_side_id); 
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // for each version descriptor in this subenclosure
    for( item = 0; item < subencl_desc_p->num_ver_descs; item++, version_desc_p++)
    {
        // translate the component type from the version descriptor
        fbe_eses_enclosure_xlate_eses_comp_type(version_desc_p->comp_type,&fw_target);
        //mapp the fw_target to index with side
        fbe_edal_get_fw_target_component_type_attr(fw_target,&fw_comp_type,&fw_comp_attr);

        encl_status = fbe_eses_enclosure_subencl_desc_get_subencl_slot(eses_enclosure_p, 
                                                                       subencl_desc_p, 
                                                                       &subencl_slot);

        if (encl_status != FBE_ENCLOSURE_STATUS_OK )
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                           "parse_version_desc: failed to get subencl slot, target %d,component type %d(%d).\n",
                           fw_target, fw_comp_type, version_desc_p->comp_type);

            continue;
        }
        if((fw_comp_type == FBE_ENCL_POWER_SUPPLY) && 
           (fbe_eses_enclosure_is_ps_overall_elem_saved(eses_enclosure_p)))
        {
            /* we need to add 1 to skip the overall element.
             */
            rev_id = item + 1;
        }

        encl_status = fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosure_p, 
                                                                          fw_comp_type, 
                                                                          subencl_slot, 
                                                                          rev_id,
                                                                          &component_index);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                           "parse_version_desc: skip unsupported firmware target %d, component type %d(%d)\n", 
                           fw_target, fw_comp_type, version_desc_p->comp_type);
            continue;
        }

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                          FBE_TRACE_LEVEL_INFO,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure_p),
                          "parse_version_desc: %s slot(%d)\n", 
                          fbe_eses_enclosure_fw_targ_to_text((fbe_enclosure_fw_target_t)fw_target), 
                          subencl_slot);
        // get  a pointer to the specified firmware target information for our local side
        // we are going to store fan info for all positions.
        // SPS has the side id as 31. So we have to put all of its the fw targets here
        // as special cases. - PINGHE
        // Adding (fw_target == FBE_FW_TARGET_PS). Viking has 4 PS. Even though
        // Currently SPA would do upgrade for PS in slot 0 and slot 1.
        // SPB would do upgrade for PS in slot 2 and slot 3. We still save the 
        // subenclosure id and some other info for all 4 PS.
        if (((subencl_desc_p->side%ESES_SUPPORTED_SIDES) == local_side_id)||
            (fw_target == FBE_FW_TARGET_PS) ||
            (fw_target == FBE_FW_TARGET_COOLING) || 
            (fw_target == FBE_FW_TARGET_SPS_PRIMARY) ||
            (fw_target == FBE_FW_TARGET_SPS_SECONDARY) ||
            (fw_target == FBE_FW_TARGET_SPS_BATTERY))
        {
            status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                                    fw_target, 
                                                    subencl_slot, 
                                                    &fw_info_p);

            if (fw_info_p == NULL)
            {
                // error on translation
                // output error info
                continue;
            }
            // copy the version descriptor fields into the eses enclosure object
            fw_info_p->fwEsesComponentSubEnclId = subencl_desc_p->subencl_id;
            fw_info_p->fwEsesComponentType = version_desc_p->comp_type;
            fw_info_p->fwEsesComponentElementIndex = version_desc_p->elem_index;
        }

        // the revision is in the public area, so need different access method
        status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure_p,
                                                               fw_comp_type,
                                                               component_index,
                                                               &generalDataPtr);
        if (status == FBE_STATUS_OK)
        {
            fbe_zero_memory(fw_info.fwRevision, FBE_EDAL_FW_REV_STR_SIZE);

            if (eses_enclosure_p->eses_version_desc == ESES_REVISION_1_0)
            {
                // fill in data
                fbe_copy_memory(fw_info.fwRevision,  
                                &version_desc_p->cdes_rev.cdes_1_0_rev.comp_rev_level, 
                                FBE_ESES_FW_REVISION_1_0_SIZE);
            }
            else if (eses_enclosure_p->eses_version_desc == ESES_REVISION_2_0)
            {
                // fill in data
                fbe_copy_memory(fw_info.fwRevision,  
                                &version_desc_p->cdes_rev.cdes_2_0_rev.comp_rev_level, 
                                FBE_ESES_FW_REVISION_2_0_SIZE);
            }
            
            fw_info.valid = TRUE;
            fw_info.downloadable = version_desc_p->downloadable;
            // copy the fw revision
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                          FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure_p),
                          "parse_version_desc:save edal rev:%s, index:%d\n", fw_info.fwRevision,component_index);
            edalStatus = eses_enclosure_access_setStr(generalDataPtr,
                                                       fw_comp_attr,
                                                       fw_comp_type,
                                                       sizeof(fbe_edal_fw_info_t),
                                                       (char *)&fw_info);
        }

        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        }

        /* For CDES 2, there is no bundled revision for LCC. So
         * we just copy the expander firmware (i.e. primary firmware) revision
         * as the bundled revision. So that our code would not break
         * while querying the bundled revision.
         */
         // Copy LCC CDES FW revision to other location too
        if ((eses_enclosure_p->eses_version_desc == ESES_REVISION_2_0) && 
            (fw_comp_type == FBE_ENCL_LCC) && 
            (fw_comp_attr == FBE_LCC_EXP_FW_INFO))
        {
            // copy the fw revision
            edalStatus = eses_enclosure_access_setStr(generalDataPtr,
                                                       FBE_ENCL_COMP_FW_INFO,
                                                       FBE_ENCL_LCC,
                                                       sizeof(fbe_edal_fw_info_t),
                                                       (char *)&fw_info);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
            }
        }

        // trace the fw revision and component
        if (eses_enclosure_p->eses_version_desc == ESES_REVISION_1_0)
        {
            fbe_zero_memory(revcopy, FBE_ESES_FW_REVISION_1_0_SIZE + 1);
            fbe_copy_memory(revcopy, 
                            version_desc_p->cdes_rev.cdes_1_0_rev.comp_rev_level, 
                            FBE_ESES_FW_REVISION_1_0_SIZE);
            fbe_zero_memory(compcopy, FBE_ESES_FW_IDENTIFIER_SIZE + 1);
            fbe_copy_memory(compcopy, version_desc_p->cdes_rev.cdes_1_0_rev.comp_id, FBE_ESES_FW_IDENTIFIER_SIZE);
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure_p),
                              "parse_version_desc(1.0): component:%s rev:%s downloadable %d\n", 
                              compcopy, 
                              revcopy, 
                              version_desc_p->downloadable);
        }
        else if (eses_enclosure_p->eses_version_desc == ESES_REVISION_2_0)
        {
            fbe_zero_memory(revcopy, FBE_ESES_FW_REVISION_2_0_SIZE + 1);
            fbe_copy_memory(revcopy, 
                            version_desc_p->cdes_rev.cdes_2_0_rev.comp_rev_level, 
                            FBE_ESES_FW_REVISION_2_0_SIZE);
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure_p),
                              "parse_version_desc(2.0): rev:%s downloadable %d\n", 
                              revcopy, 
                              version_desc_p->downloadable);
        }

    }

    return FBE_ENCLOSURE_STATUS_OK;

} // fbe_eses_enclosure_parse_version_desc


/*!*************************************************************************
* @fn fbe_eses_is_min_fw_rev
*
***************************************************************************
* @brief
*   Get the current firmware budle revision from EDAL and compare
*   it against a minimum revision level. If the minimum revision level
*   is not met, trace it and return an enclosure status error.
*
* @param  eses_enclosurep (Input) - The pointer to the eses enclosure.
*
* @return  TRUE - meets minimum fw rev
*          FALSE - fw is below rev
*
* HISTORY
*  01-Oct-2008 GB - Created.
***************************************************************************/
fbe_bool_t fbe_eses_is_min_fw_rev(fbe_eses_enclosure_t *eses_enclosurep,fbe_u32_t side_id)
{
    fbe_s32_t                       result;
    fbe_edal_general_comp_handle_t  general_datap = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_edal_status_t               edal_status = FBE_EDAL_STATUS_ERROR;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_EDAL_ERROR;
    char                            fwRev[FBE_EDAL_FW_REV_STR_SIZE+1];
    fbe_u8_t                        index;
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_edal_fw_info_t              fw_info;
    char *                          min_rev_string;


    fbe_zero_memory(fwRev, FBE_EDAL_FW_REV_STR_SIZE+1);

    fbe_edal_get_fw_target_component_type_attr(FBE_FW_TARGET_LCC_MAIN,&fw_comp_type,&fw_comp_attr);
    encl_status = fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosurep,
                                                        fw_comp_type, 
                                                        side_id, 
                                                        0,  // Temp solution. need to be changed -- PINGHE
                                                        &index);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosurep,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosurep),
                    "min_fw_rev: can't map FW_LCC_MAIN component type %d\n", 
                    fw_comp_type);
        return FALSE;
    }
    // the revision and identifier are in the public area, access through edal lib function
    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosurep,
                                                           fw_comp_type,
                                                           index,
                                                           &general_datap);
    if (status == FBE_STATUS_OK)
    {
        // get the fw revision in edal
        edal_status = eses_enclosure_access_getStr(general_datap,
                                                fw_comp_attr,
                                                fw_comp_type,
                                                sizeof(fbe_edal_fw_info_t),
                                                (char *)&fw_info); // copy the string here
        if ((edal_status == FBE_EDAL_STATUS_OK) && (fw_info.valid))
        {
            fbe_copy_memory(fwRev, fw_info.fwRevision, FBE_EDAL_FW_REV_STR_SIZE);
            encl_status = FBE_ENCLOSURE_STATUS_OK;
        }
        else
        {
            // force rev be invalid (not supported)
            *((fbe_u16_t *)fwRev) = 0xffff;
        }
    }
    else
    {
        encl_status = FBE_ENCLOSURE_STATUS_EDAL_FAILED;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosurep,
                                  FBE_TRACE_LEVEL_WARNING,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosurep),
                                  "fbe_eses_is_min_fw_rev: fwrev access failed!\n");
    }

    if (encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        // compare this string with the minimal level
        // only care about major/minor, so skip the last byte in the actual rev
        if((eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)||
           (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE))
        {
            min_rev_string = " 004";
        } 
        if((eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_ANCHO))
        {
            /* AR 0633061: The format of the string has changed for new enclosure ENCL_CLEANUP */
            *((fbe_u16_t *)fwRev) = 0x0000;
            min_rev_string = "";    
        }
        else if((eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP)||
           (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP))
        {
            return TRUE;    // TODO ENCL_CLEANUP - Need to support new revisioning format for the CDES 2.0 enclosures.
        }
        else if ((eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_RHEA) ||
                 (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_MIRANDA) ||
                 (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CALYPSO))
        {
            /* AR 0633061: The format of the string has changed for new enclosure ENCL_CLEANUP */
            //ENCL_CLEANUP
            return TRUE; /*TODO: FIXME*/
            *((fbe_u16_t *)fwRev) = 0x0000;
            min_rev_string = "";    
        } 
        else if (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_TABASCO) 
        {
             /* AR 0633061: The format of the string has changed for new enclosure ENCL_CLEANUP */ 
             return TRUE;
        } 
        else 
        {
            min_rev_string = " 103";
        }
        result = fbe_compare_string(fwRev, FBE_EDAL_FW_REV_STR_SIZE-1, min_rev_string, FBE_EDAL_FW_REV_STR_SIZE-1, FALSE);
        if( (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP) ||
            (eses_enclosurep->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP) )

        {  // TODO ENCL_CLEANUP - Need to support new revisioning format for the CDES 2.0 enclosures.
            return TRUE;
        }
        if ((result < 0) || (*((fbe_u16_t *)fwRev) == 0xffff))
        {
            encl_status = FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosurep,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosurep),
                                               "fbe_eses_is_min_fw_rev: INSTALLED ENCL FIRMWARE(%s) IS BELOW MIN REV(%s)!!\n",
                                               fwRev,
                                               min_rev_string);
        }
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosurep,
                                  FBE_TRACE_LEVEL_WARNING,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosurep),
                                  "fbe_eses_is_min_fw_rev: fwrev edal access failed!\n");
    }

    if (encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        return TRUE;
    }
    return FALSE;

} // fbe_eses_is_min_fw_rev


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_addl_status_page(
*                                    fbe_eses_enclosure_t * eses_enclosure,
*                                    fbe_u8_t * addl_status_page)                  
***************************************************************************
* @brief
*       This function process the ESES additional status page. It can only
*       be called after processing the ESES configuration page.
*       It will process the drive array slot additional status elements
*       and sas expander additional status elements.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  addl_status_page - The pointer to the additional status page.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_addl_status_page(
                                    fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_u8_t * addl_status_page)
{
    fbe_u8_t     elem_index = 0;
    fbe_u8_t     group_id = 0;
    fbe_u16_t    byte_offset = 0;
    ses_common_pg_hdr_struct * addl_status_page_p = NULL;
    ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p = NULL;
    fbe_eses_elem_group_t * elem_group = NULL; 
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
       
    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_INFO,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "%s entry\n", __FUNCTION__ );

    addl_status_page_p = (ses_common_pg_hdr_struct * )addl_status_page;

    addl_elem_stat_desc_p = fbe_eses_get_first_addl_elem_stat_desc_p(addl_status_page);
    
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);  

    /* 
     * The first loop is to get the drive slot number and element index mapping.
     */
    for(byte_offset = FBE_ESES_PAGE_HEADER_SIZE;
        byte_offset < fbe_eses_get_pg_size((ses_common_pg_hdr_struct * )addl_status_page_p);)
    {
        //Get the element index for this additional element status descriptor.
        elem_index = addl_elem_stat_desc_p->elem_index;
 
        group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index); 
                
  
        if(fbe_eses_elem_group_get_elem_type(elem_group, group_id) == SES_ELEM_TYPE_ARRAY_DEV_SLOT)
        {
            // Peer LCC does not have any array device slot elements.
            // So it must be on local LCC.  
            encl_status = fbe_eses_enclosure_process_array_dev_slot_addl_elem_stat(eses_enclosure, addl_elem_stat_desc_p);
        }
        byte_offset += fbe_eses_get_addl_elem_stat_desc_len(addl_elem_stat_desc_p); 


        // Get the pointer to the next additional element descriptor.
        addl_elem_stat_desc_p = fbe_eses_get_next_addl_elem_stat_desc_p(addl_elem_stat_desc_p);
        

    } // End of the first loop.

    /*
     * Get the pointer to the first additional element status descriptor 
     * in the additional status page.
     */
    addl_elem_stat_desc_p = fbe_eses_get_first_addl_elem_stat_desc_p(addl_status_page);

    /*
     * The second loop is to set the phy index for each drive slot. 
     * The reason that we have to do the loop the second time is that 
     * we are not sure if the array device slot additional status element 
     * is before the SAS expander additional status element in the 
     * addtional status page and we need
     * to loop through array device slot additional status element first to 
     * get the drive slot and its element index mapping before we can set 
     * the phy index for each drive slot. 
     */
    for(byte_offset = FBE_ESES_PAGE_HEADER_SIZE;
        byte_offset < fbe_eses_get_pg_size(addl_status_page_p);)
    {
        //Get the element index for this additional element status descriptor.
        elem_index = addl_elem_stat_desc_p->elem_index;
 
        group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, elem_index); 
     
        if(fbe_eses_elem_group_get_elem_type(elem_group, group_id) == SES_ELEM_TYPE_SAS_EXP)           
        {
           /* 
            * This is additional status element for sas expander.
            */
            encl_status = fbe_eses_enclosure_process_sas_exp_addl_elem_stat(eses_enclosure, 
                                                addl_elem_stat_desc_p);                                 
        }
        byte_offset += fbe_eses_get_addl_elem_stat_desc_len(addl_elem_stat_desc_p); 

        // Get the pointer to the next additional element descriptor.
        addl_elem_stat_desc_p = fbe_eses_get_next_addl_elem_stat_desc_p(addl_elem_stat_desc_p);

   }  // End of the second loop.  
         
 
    return encl_status;         
       
}// End of function fbe_eses_enclosure_process_addl_status_page

/**************************************************************************
* @fn fbe_eses_enclosure_process_array_dev_slot_addl_elem_stat(
*                             fbe_eses_enclosure_t * eses_enclosure,
*                             ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p)              
***************************************************************************
* @brief
*       This function gets called while processing the additional status page
*       for the array slot device. It finds the corresponding slot number 
*       for the array device slot element index and then sets the sas address 
*       for the array device slot.
*       
* @param   eses_enclosure  - The pointer to the eses enclosure.
* @param   addl_elem_stat_desc_p - The pointer to the array device slot 
*                              additional element status descriptor.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - error is found.
*
* NOTES
*
* HISTORY
*   06-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_array_dev_slot_addl_elem_stat(
                                           fbe_eses_enclosure_t * eses_enclosure,
                                           ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p)
{
    ses_array_dev_slot_prot_spec_info_struct * prot_spec_info_p = NULL;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t slot_index;
    fbe_u32_t slot_number;
#if (MOCKUP_VOYAGER)
    fbe_u32_t comp_id;
#endif

    // This is additional element status for array drive slot. 
    // Get the pointer to the protocal specific information.         
    prot_spec_info_p = (ses_array_dev_slot_prot_spec_info_struct *)(fbe_eses_get_prot_spec_info_p(addl_elem_stat_desc_p));

    if(prot_spec_info_p->desc_type != FBE_ESES_DESC_TYPE_ARRAY_DEVICE_SLOT) 
    {
        return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }
   
   /* 
    * It is possible that we don't need to describe any device phys. 
    * If so, the number of phy descriptors is 0.
    * So, we add the check here to see if num_phy_descs is 1 not 0.
    */  
    if(prot_spec_info_p->num_phy_descs == 1)
    {           
        // Match the element index for the drive slot.
        // The element index is recorded while processing type descriptor.
        slot_index = 
            fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                        FBE_ENCL_DRIVE_SLOT,  // Component type
                                                        0, //starting index
                                                        addl_elem_stat_desc_p->elem_index); 
        if (slot_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "process_array_dev_slot_addl_elem_stat, slot element index %d undefined.\n",
                                addl_elem_stat_desc_p->elem_index); 
            encl_status = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
        }
        else 
        {
            // record slot number.
            slot_number = prot_spec_info_p->dev_slot_num;
#if (MOCKUP_VOYAGER)
            // HACK FOR MOCKUP VOYAGER
            // For the mockup we map EE0 to 0-14 and EE1 to 30-44. 
            // The Vipers only have 15 drives not 30 but we still want
            // the EE1 to be in the correct range.
            if ((comp_id = (((fbe_base_enclosure_t *) eses_enclosure)->component_id)) != 0) 
            {
                if ((comp_id == 3)&&(slot_number<30))
                {
                    slot_number += ((((fbe_base_enclosure_t *) eses_enclosure)->component_id) - 2) * 30;
                }
            }
#endif

            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                  FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                  FBE_ENCL_DRIVE_SLOT,
                                                  slot_index,
                                                  slot_number);
        }
    } //End of - if(prot_spec_info_p->num_phy_descs == 1)    
    
    return encl_status;

} // End of fbe_eses_enclosure_process_array_dev_slot_addl_elem_stat



/*!*************************************************************************
* @fn fbe_eses_enclosure_process_sas_exp_addl_elem_stat(
*                     fbe_eses_enclosure_t * eses_enclosure,
*                     ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p)                  
***************************************************************************
* @brief
*       This function gets called while processing the additional status page
*       for SAS expander additional element status descriptor. 
*              
* @param  eses_enclosure  - The pointer to the eses enclosure.
* @param  addl_elem_stat_desc_p - The pointer to the sas expander 
*                              additional element status descriptor.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - error is found.
*
* NOTES
*
* HISTORY
*   06-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_sas_exp_addl_elem_stat(
                                       fbe_eses_enclosure_t * eses_enclosure,
                                       ses_addl_elem_stat_desc_hdr_struct * addl_elem_stat_desc_p)
{    
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t i = 0; // Used to loop through all the phy descriptors.
    fbe_u8_t expander_index = 0;
    fbe_u8_t phy_index = 0;
    fbe_u8_t slot_index = 0;          
    fbe_u8_t group_id = 0;
    fbe_sas_address_t expander_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_eses_elem_group_t * elem_group = NULL;
    ses_sas_exp_prot_spec_info_struct * prot_spec_info_p = NULL;     
    ses_sas_exp_phy_desc_struct * phy_desc_p = NULL;
    ses_elem_type_enum elem_type = SES_ELEM_TYPE_INVALID;
 
    /********
     * BEGIN
     ********/
    // Get the pointer to the protocal specific information.         
    prot_spec_info_p = (ses_sas_exp_prot_spec_info_struct *)(fbe_eses_get_prot_spec_info_p(addl_elem_stat_desc_p));

    if(prot_spec_info_p->desc_type != FBE_ESES_DESC_TYPE_SAS_EXP_ESC_ELEC) 
    {
        return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }    

    if((encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, FBE_ENCL_EXPANDER,
         addl_elem_stat_desc_p->elem_index, &expander_index)) != FBE_ENCLOSURE_STATUS_OK)
    {
        // Fail to find the mapping between element index and expander component index. 
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s, unavailable to get expander index, encl_status: 0x%x.\n", 
                         __FUNCTION__, encl_status);  

        
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                              FBE_ENCL_COMP_ELEM_INDEX,  // Attribute
                                              FBE_ENCL_EXPANDER,    // Component type
                                              expander_index,  //Index of the component 
                                              addl_elem_stat_desc_p->elem_index); // The value to be set to

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    expander_sas_address = swap64(prot_spec_info_p->sas_address);

    encl_status = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_EXP_SAS_ADDRESS,  // Attribute
                                               FBE_ENCL_EXPANDER,    // Component type
                                               expander_index,  //Index of the component 
                                               expander_sas_address); // The value to be set to

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);  
    group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, addl_elem_stat_desc_p->elem_index);

    if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) != SES_SUBENCL_ID_PRIMARY)
    {
        // This is peer expander. We don't process any phy descriptors for the peer expander.
        return FBE_ENCLOSURE_STATUS_OK;
    }

    // It comes here because this is the local expander additional status element.
    // Get the pointer to the protocal specific information.         
    prot_spec_info_p = (ses_sas_exp_prot_spec_info_struct *)(fbe_eses_get_prot_spec_info_p(addl_elem_stat_desc_p));

    // Get the pointer to the first Expander Phy descriptor.
    phy_desc_p = fbe_eses_get_first_sas_exp_phy_desc_p(prot_spec_info_p);

    for(i = 0; i < prot_spec_info_p->num_exp_phy_descs; i++)
    { 
       /*
        * Check if the phy is attached to something. Do not save any information for the phy
        * that is not attached to anything.
        * Only save the phy info on the local expander, but save the connector info on both Local and Peer LCCs.
        * If the phy is attached to a single lane of the connector,
        * other element index is the element index of the entire connector.
        * If the phy is attached to an array device slot, other element index
        * is the element index of the array device slot.
        */ 
        
        if((phy_desc_p->conn_elem_index != SES_ELEM_INDEX_NONE) ||
            (phy_desc_p->other_elem_index != SES_ELEM_INDEX_NONE))
        {
            /* The phy on the local expander is attached to something. 
             * Get the phy_index. The element index is used to find the side id. 
             * Since only the info for phys on the local LCC is saved, we do not need a valid element index of 
             * the phy to look for the next available phy index. So SES_ELEM_INDEX_NONE is used as one of the inputs.
             */
            phy_index = fbe_eses_enclosure_get_next_available_phy_index(eses_enclosure);
            if(phy_index==FBE_ENCLOSURE_VALUE_INVALID) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "process_sas_exp_addl_elem_stat, unavailable phy index.\n"); 
                return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
            }

            /* Set the PHY_ID attribute for the phy. */
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                      FBE_ENCL_EXP_PHY_ID,  // Attribute
                                                      FBE_ENCL_EXPANDER_PHY,    // Component type
                                                      phy_index,  //Index of the component 
                                                      i); // The value to be set to
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "process_sas_exp_addl_elem_stat, phy_index:%d phy_id:%d.\n", phy_index, i); 

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }
                                 
            /* Set the EXPANDER_ELEM_INDEX attribute for the phy. */
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                      FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,  // Attribute
                                                      FBE_ENCL_EXPANDER_PHY,    // Component type
                                                      phy_index,  //Index of the component 
                                                      addl_elem_stat_desc_p->elem_index); // The value to be set to

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }



                group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, phy_desc_p->other_elem_index);

            /* we are skipping invalid group_id */
                if(group_id != FBE_ENCLOSURE_VALUE_INVALID)
                {
                    elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);
                    switch (elem_type)
                    {

                       case SES_ELEM_TYPE_ARRAY_DEV_SLOT:
                        /* This phy is attached to an array drive slot. */
                        if((encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, FBE_ENCL_DRIVE_SLOT, 
                            phy_desc_p->other_elem_index, &slot_index)) != FBE_ENCLOSURE_STATUS_OK)
                        {
                            // Fail to find the mapping between element index and expander component index. 
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, unavailable to get slot index, phy_index %d, encl_status: 0x%x.\n", 
                                            __FUNCTION__, phy_desc_p->other_elem_index, encl_status);  
                            return encl_status;
                        }

                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, phy_index %d, slot_index %d.\n", 
                                            __FUNCTION__, phy_index, slot_index);

                        /* The drive slot_index is found. Set the FBE_ENCL_DRIVE_PHY_INDEX for the slot.*/
                        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                              FBE_ENCL_DRIVE_PHY_INDEX,  // Attribute
                                                              FBE_ENCL_DRIVE_SLOT,    // Component type
                                                              slot_index,  //Index of the component 
                                                              phy_index); // The value to be set to   

                    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return encl_status;
                    }
                    break;

                case SES_ELEM_TYPE_SAS_CONN:
                    /* This phy is attached to a single lane of the connector.
                * conn_elem_index is the element index of the individual lane.
                * other_elem_index is the element index of the entire conector.
                */        
                /* First, process the entire connector. */
                encl_status = fbe_eses_enclosure_process_connector_phy_desc(eses_enclosure, 
                          expander_sas_address, phy_desc_p, TRUE, FBE_ENCLOSURE_VALUE_INVALID);
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "process_connector_phy_desc for entire connector failed, encl_status: 0x%x.\n", 
                                      encl_status);
                    return encl_status;
                }
                
                /* Second, process the individual lane of the connector. */
                encl_status = fbe_eses_enclosure_process_connector_phy_desc(eses_enclosure, 
                         expander_sas_address, phy_desc_p, FALSE, phy_index);          
                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "uprocess_connector_phy_desc for single lane failed, encl_status: 0x%x.\n", 
                                     encl_status);
                    return encl_status;
                }
                    break;

                default:
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, skip element index %d, type %d.\n", 
                                        __FUNCTION__, phy_desc_p->other_elem_index, elem_type);  
                    break;
                }// end of switch
            }
            else
            {
                // Do nothing.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unfound group id %d.\n", 
                                    __FUNCTION__, phy_desc_p->other_elem_index);  
            }
        }
        
        // Get the pointer to the next phy descriptor.
        phy_desc_p = fbe_eses_get_next_sas_exp_phy_desc_p(phy_desc_p);
                
    } // End of - for(i = 0; i < prot_spec_info_p->num_exp_phy_descs; i++)

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - function fbe_eses_enclosure_process_sas_exp_addl_elem_stat

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_connector_phy_desc(fbe_eses_enclosure_t * eses_enclosure, 
*                                                   fbe_sas_address_t expander_sas_address,
*                                                   ses_sas_exp_phy_desc_struct * phy_desc_p,
*                                                   fbe_bool_t process_entire_connector,
*                                                   fbe_u8_t phy_index)                
***************************************************************************
* @brief
*       This function fills in the attributes for the connector got from 
*       the phy descriptor in the sas expander additional status element. 
*              
* @param  eses_enclosure  - The pointer to the eses enclosure.
* @param  expander_sas_address - The sas address of the expander that the phy descriptor belongs to.
* @param  phy_desc_p - The phy descriptor in the sas expander additional status element.
* @param  process_entire_connector - Process the entire connector or the single lane in the phy desciptor.
* @param  phy_index - The component index of the phy attached to the connector.index used by EDAL to access the information of phys.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - error is found.
*
* NOTES
*
* HISTORY
*   07-Jan-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_connector_phy_desc(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_sas_address_t expander_sas_address,
                                                  ses_sas_exp_phy_desc_struct * phy_desc_p,
                                                  fbe_bool_t process_entire_connector,
                                                  fbe_u8_t phy_index)

{
    fbe_u8_t connector_elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t container_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "process_connector_phy_desc, entry phy_index %d.\n", 
                                 phy_index);
    if(process_entire_connector)
    {
        connector_elem_index = phy_desc_p->other_elem_index;
    }
    else
    {
        connector_elem_index = phy_desc_p->conn_elem_index;
    }

    // Check if it is a new connector.
    connector_index = fbe_eses_enclosure_is_new_connector(eses_enclosure, connector_elem_index);
    if(connector_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        /* Set the sas address of the expander which contains the connector.*/
        encl_status = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,  // Attribute
                                            FBE_ENCL_CONNECTOR,    // Component type
                                            connector_index,  //Index of the component 
                                            expander_sas_address); // The value to be set to

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }     

        /* Set the FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR attribute. */
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute
                                            FBE_ENCL_CONNECTOR,    // Component type
                                            connector_index,  //Index of the component 
                                            process_entire_connector); // The value to be set to

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }  

        /* Get entire connector index. */
        if(process_entire_connector)
        {
            // This is an entire connector.
            container_index = FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED;
        }
        else
        {
            encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, FBE_ENCL_CONNECTOR,
                phy_desc_p->other_elem_index, &entire_connector_index);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                // Fail to find the mapping between element index and entire connector index. 
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "process_connector_phy_desc, entire_connector_index unavailable, encl_status: 0x%x.\n", 
                                 encl_status);
                return encl_status;
            }
            container_index = entire_connector_index;
        }

        /* Set FBE_ENCL_COMP_CONTAINER_INDEX in EDAL, the component index of the entire connector
         * that the individual lane belongs to. 
         * If the connector is already an entire connector, 
         * its container component index is its own component index.
         */
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute
                                            FBE_ENCL_CONNECTOR,    // Component type
                                            connector_index,  //Index of the component 
                                            container_index); // The value to be set to

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        } 
        
          
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "process_connector_phy_desc, conn_index %d phy_index %d.\n", 
                                 connector_index, phy_index);

        // Set the phy index for connector.
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_PHY_INDEX,  // Attribute
                                                    FBE_ENCL_CONNECTOR,    // Component type
                                                    connector_index,  //Index of the component 
                                                    phy_index); // The value to be set to

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }   
    
    }// End of - if(connector_index != FBE_ENCLOSURE_VALUE_INVALID)
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "process_connector_phy_desc, conn_index invalid phy_index %d.\n", 
                                 phy_index);

    }

    return encl_status;
}// End of function fbe_eses_enclosure_process_connector_phy_desc  


/*!*************************************************************************
* @fn fbe_u8_t fbe_eses_enclosure_is_new_connector(fbe_eses_enclosure_t * eses_enclosure, 
*                                                        fbe_u8_t elem_index)               
***************************************************************************
* @brief
*       This function checks if the specified connector element index
*       has been processed already or not. If it is not saved yet, then
*       it is a new connector that needs to be processed.
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  elem_index (Input) - The connector element index.
*
* @return   fbe_u8_t 
*       FBE_ENCLOSURE_VALUE_INVALID - It is NOT a new connector.
*       others - It is a new connector.
*      
* NOTES
*      The input connector element index could be the element index of an individual lane or 
*      an entire connector.
* 
* HISTORY
*  04-Sep-2008 PHE - Created.
***************************************************************************/
static fbe_u8_t fbe_eses_enclosure_is_new_connector(fbe_eses_enclosure_t * eses_enclosure, 
                                                      fbe_u8_t elem_index)
                                                                      
{
    fbe_u32_t matching_index;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t  container_index;

    matching_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    0, //starting index
                                                    elem_index);   

    if(matching_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_CONTAINER_INDEX,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    matching_index,
                                                    &container_index);   
        if ((encl_status != FBE_ENCLOSURE_STATUS_OK)  ||
            (container_index == FBE_ENCLOSURE_CONTAINER_INDEX_INVALID))
        {
            // If container index is not valid, it's new, do nothing
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_DEBUG_LOW,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "is_new_connector, elem_index:%d encl_status:0x%x container_index:0x%x.\n", 
                                     elem_index, encl_status, container_index);

            matching_index = FBE_ENCLOSURE_VALUE_INVALID; 
        }
    }  

    return (fbe_u8_t)matching_index;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_status_page(                  
*                     fbe_eses_enclosure_t * eses_enclosure, 
*                     fbe_u8_t * status_page)              
***************************************************************************
* @brief
*
* DESCRIPTION
*       This function loops through all the groups to call the extract status
*       method of each group to process the enclosure status read from 
*       the status page. 
*
* @param   eses_enclosure - The pointer to the enclosure.
*          status_page - The pointer to the status page.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - error is found. 
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_status_page(fbe_eses_enclosure_t * eses_enclosure, 
                                                         fbe_u8_t * status_page)
{
    fbe_u8_t group_id = 0;
    fbe_u8_t num_possible_elems = 0;
    fbe_u8_t elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u16_t group_byte_offset = 0;
    fbe_u16_t elem_byte_offset = 0;
    const fbe_eses_enclosure_comp_type_hdlr_t * handler;
    fbe_enclosure_component_types_t component_type;
    fbe_eses_elem_group_t * elem_group = NULL;

    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    
    /********
     * BEGIN
     ********/


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_DEBUG_HIGH,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "%s entry \n", __FUNCTION__ );
   
    // get the page length
    eses_enclosure->eses_status_control_page_length = 
        fbe_eses_get_pg_size((ses_common_pg_hdr_struct *)status_page);

    /* Get the elem_group so that we will know the configuration while 
     * processing the status page. 
     */

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
  
    for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++ )
    {  
        group_byte_offset = fbe_eses_elem_group_get_group_byte_offset(elem_group, group_id);
        
        if(group_byte_offset == 0)
        {  
            return FBE_ENCLOSURE_STATUS_CONFIG_INVALID; 
        }
            
        elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);

        // Check whether the element type is valid or not. 
        if(elem_type == SES_ELEM_TYPE_INVALID)
        {
            return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;
        }

        num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);
        elem_byte_offset = fbe_eses_elem_group_get_elem_byte_offset(elem_group, group_id, num_possible_elems);
    
        if(elem_byte_offset > eses_enclosure->eses_status_control_page_length)
        {
            return FBE_ENCLOSURE_STATUS_CONFIG_INVALID;
        } 
 
        encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure, group_id, &component_type);
        
        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        handler = fbe_eses_enclosure_get_comp_type_hdlr(fbe_eses_enclosure_comp_type_hdlr_table, component_type);
            
        /* NULL pointer for handler shoud not be considered as a fault.
         * It could be possible that we did not implement the status extraction method
         * for the particular element type. 
         */
        if((handler != NULL) && (handler->extract_status != NULL))
        {
            encl_status = handler->extract_status(eses_enclosure, 
                                   group_id, 
                                  (fbe_eses_ctrl_stat_elem_t *)(status_page + group_byte_offset));
            
             if(!ENCL_STAT_OK(encl_status) &&
               (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED))
            {
                return encl_status;
            }
        }
      
    } // End of the loop.
        
    return FBE_ENCLOSURE_STATUS_OK;

}// End of function fbe_eses_enclosure_process_status_page

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_comp_type_hdlr(const fbe_eses_enclosure_comp_type_hdlr_t  * hdlr_table[],
*                                 fbe_enclosure_component_types_t component_type)                  
***************************************************************************
* @brief
*       This function loops through the handler table to find the handler for
*       the specified element type. 
*
* @param  hdlr_table - The pointer to the pointer of the handler table. 
* @param  component_type - The EDAL component type.
*
* @return  fbe_eses_enclosure_comp_type_hdlr_t
*       NULL - The handler for the specified element type does not exist, 
*       otherwise - returns the handler for the specified element type.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_get_comp_type_hdlr(
                    const fbe_eses_enclosure_comp_type_hdlr_t  * hdlr_table[],
                    fbe_enclosure_component_types_t component_type)
{   
    fbe_u8_t i; // Used for loop through the handler table.

    const fbe_eses_enclosure_comp_type_hdlr_t * handler = NULL;
    
    for(i = 0; hdlr_table[i] != NULL; i++)
    {
        handler = hdlr_table[i];
        if(handler->comp_type == component_type) {
            return handler;
        }
    }
    return NULL;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_emc_specific_page(
*                                        fbe_eses_enclosure_t * eses_enclosure, 
*                                        ses_pg_emc_encl_stat_struct * emc_specific_status_page_p)               
***************************************************************************
* @brief
*     This function processes the EMC specific status page.
*     (1) Parse the sas connector info elements to get the primary_port_entire_connector_index.
*     (2) Parse the trace buffer info elements to check if there is any trace buffer saved.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   emc_specific_status_page - The pointer to the EMC specific status page.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_emc_specific_page(
                                          fbe_eses_enclosure_t * eses_enclosure, 
                                          ses_pg_emc_encl_stat_struct * emc_specific_status_page_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_common_pg_hdr_struct * pg_hdr_p = NULL;
    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p = NULL;
    fbe_u32_t page_size = 0;
    fbe_u32_t byte_offset = 0;
    
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_DEBUG_LOW,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "%s entry \n", __FUNCTION__ );
    
    pg_hdr_p = (ses_common_pg_hdr_struct *)emc_specific_status_page_p;
   
    encl_status = fbe_eses_enclosure_update_shutdown_info(eses_enclosure, emc_specific_status_page_p);
    
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "process_shutdown_info failed, encl_status: 0x%x.\n", 
                         encl_status);        

        return encl_status;
    }

    info_elem_group_hdr_p = fbe_eses_get_first_info_elem_group_hdr_p(emc_specific_status_page_p);
    
    FBE_ESES_GET_PAGE_SIZE(page_size, pg_hdr_p);

    for(byte_offset = FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET; byte_offset < page_size;)
    {
        switch(info_elem_group_hdr_p->info_elem_type)
        {
            case FBE_ESES_INFO_ELEM_TYPE_SAS_CONN:
                // This is SAS Connector information element group.
                encl_status = fbe_eses_enclosure_process_sas_conn_info(eses_enclosure, info_elem_group_hdr_p);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "process_sas_conn_info failed, encl_status: 0x%x.\n", 
                                     encl_status);
                    return encl_status;
                }
                break;

            case FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF:
                // This is trace buffer information element group.
                encl_status = fbe_eses_enclosure_process_trace_buf_info(eses_enclosure, info_elem_group_hdr_p);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "process_trace_buf_info failed, encl_status: 0x%x.\n", 
                                    encl_status);
                }
                break;

            case FBE_ESES_INFO_ELEM_TYPE_PS:
                // This is Power Supply information element group.
                encl_status = fbe_eses_enclosure_process_ps_info(eses_enclosure, info_elem_group_hdr_p);

                if((encl_status != FBE_ENCLOSURE_STATUS_OK) && (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "process_ps_info failed, encl_status: 0x%x.\n", 
                                    encl_status);
                }
                break;

            case FBE_ESES_INFO_ELEM_TYPE_SPS:
                // This is SPS information element group.
                encl_status = fbe_eses_enclosure_process_sps_info(eses_enclosure, info_elem_group_hdr_p);

                if((encl_status != FBE_ENCLOSURE_STATUS_OK) && (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "process_sps_info failed, encl_status: 0x%x.\n", 
                                    encl_status);
                }
                break;

            case FBE_ESES_INFO_ELEM_TYPE_GENERAL:
                // This is General information element group.
                encl_status = fbe_eses_enclosure_process_general_info(eses_enclosure, info_elem_group_hdr_p);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "process_general_info failed, encl_status: 0x%x.\n", 
                                    encl_status);
                }
                break;

            case FBE_ESES_INFO_ELEM_TYPE_ENCL_TIME:
            case FBE_ESES_INFO_ELEM_TYPE_DRIVE_POWER:
            default:
                // We don't need to process these information element groups for now.
                break;
        }// End of - switch(info_elem_group_hdr->info_elem_type)
       
        byte_offset += (FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE + 
                        info_elem_group_hdr_p->num_info_elems * info_elem_group_hdr_p->info_elem_size);

        info_elem_group_hdr_p = fbe_eses_get_next_info_elem_group_hdr_p(info_elem_group_hdr_p);
    }// End of - for loop.

    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_process_emc_specific_page

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_general_info(fbe_eses_enclosure_t * eses_enclosure, 
*                                          fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p)                
***************************************************************************
* @brief
*       This function processes the General information elements 
*       in the EMC specific status page.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  info_elem_group_hdr_p - The pointer to the info element group header.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   26-Jan-2009 - Rajesh V. Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_process_general_info(fbe_eses_enclosure_t * eses_enclosure,
                                        fbe_eses_info_elem_group_hdr_t  * info_elem_group_hdr_p) 
{
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_general_info_elem_cmn_struct    *general_info_elem_p = NULL;
    fbe_u8_t                            general_info_elem;
    fbe_u8_t                            num_general_info_elems;  
    fbe_eses_elem_group_t               *elem_group = NULL;
    fbe_u8_t                            group_id = FBE_ENCLOSURE_VALUE_INVALID;  
    ses_elem_type_enum                  elem_type = SES_ELEM_TYPE_INVALID;
    fbe_bool_t                          battery_backed_updated=FALSE;
    fbe_bool_t                          emc_spec_ctrl_needed=FALSE;
    fbe_status_t                        status;

    /*********
     * BEGIN 
     *********/

    general_info_elem_p = fbe_eses_get_first_general_info_elem_p(info_elem_group_hdr_p);
    num_general_info_elems = info_elem_group_hdr_p->num_info_elems;

    for(general_info_elem = 0; general_info_elem < num_general_info_elems; general_info_elem++)  
    {
        elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
        /* Get the group id */
        group_id = fbe_eses_enclosure_elem_index_to_group_id(eses_enclosure, 
                                                             general_info_elem_p->elem_index);
        /* Get the element type of this element group. */
        elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);
        
        switch(elem_type)
        {
            case SES_ELEM_TYPE_TEMP_SENSOR:
                encl_status = fbe_eses_enclosure_process_general_info_elem_for_temp_sensor(
                                    eses_enclosure,
                                    general_info_elem_p);


                if( !ENCL_STAT_OK(encl_status) && (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED) )
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "general_info_elem_for_temp_sensor failed,elem_index:%d,encl_status: 0x%x.\n", 
                                    general_info_elem_p->elem_index, encl_status);
                }
                break;

            case SES_ELEM_TYPE_SAS_EXP:
                encl_status = fbe_eses_enclosure_process_general_info_elem_for_expander(
                                    eses_enclosure,
                                    general_info_elem_p);

                if(!ENCL_STAT_OK(encl_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "general_info_elem_for_expander failed,elem_index:%d,encl_status: 0x%x.\n", 
                                    general_info_elem_p->elem_index, encl_status);
                }
                break;

            case SES_ELEM_TYPE_ARRAY_DEV_SLOT:
                encl_status = fbe_eses_enclosure_process_general_info_elem_for_drive_slot(
                                    eses_enclosure,
                                    general_info_elem_p,
                                    &battery_backed_updated);

                if( !ENCL_STAT_OK(encl_status) && (encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED) )
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "general_info_elem_for_drive_slot failed,elem_index:%d,encl_status: 0x%x.\n", 
                                    general_info_elem_p->elem_index, encl_status);
                }
                if (battery_backed_updated == TRUE)
                {
                    emc_spec_ctrl_needed=TRUE;
                }
                break;

            case SES_ELEM_TYPE_ESC_ELEC:
                encl_status = fbe_eses_enclosure_process_general_info_elem_for_esc_electronics(
                                        eses_enclosure,
                                        general_info_elem_p);

                if(!ENCL_STAT_OK(encl_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_WARNING,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "general_info_elem_for_esc_electronics failed,elem_index:%d,encl_status: 0x%x.\n", 
                                    general_info_elem_p->elem_index, encl_status);
                }
                break;

            default:
                break;
        }
        
        general_info_elem_p = fbe_eses_get_next_general_info_elem_p(general_info_elem_p);     
    }

    // use emc specific trl page to update the value to expander.
    if (emc_spec_ctrl_needed == TRUE)
    {
        emc_spec_ctrl_needed = FALSE;
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)eses_enclosure, 
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);
         if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "Fail in setting EMC_SPECIFIC_CONTROL_NEEDED condition to update drive slot battery backed info, status: 0x%x.\n",
                                        status);
                   
             }
    }

    return(FBE_ENCLOSURE_STATUS_OK);
}

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_general_info_elem_for_expander(
        fbe_eses_enclosure_t * eses_enclosure,
        ses_general_info_elem_cmn_struct *general_info_elem_p)                
***************************************************************************
* @brief
*       This function processes the given general information element for 
*           expander to extract the reset reason.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  general_info_elem_p - The pointer to general information element for
*           temperature sensor.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   09-Apr-2009 - NChiu. Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_expander(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_general_info_elem_expander_struct *general_info_elem_expander;   
    fbe_u8_t expander_component_index = 0;
    fbe_u8_t encl_side = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_status_t status;
    fbe_port_number_t port_number;
    fbe_enclosure_number_t enclosure_number;
    fbe_enclosure_component_id_t component_id;

    general_info_elem_expander = (ses_general_info_elem_expander_struct *)general_info_elem_p;

    encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, 
                                                                   FBE_ENCL_EXPANDER,
                                                                   general_info_elem_p->elem_index,
                                                                   &expander_component_index);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getEnclosureSide((fbe_base_enclosure_t *)eses_enclosure,
                                                          &encl_side);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    /* Expander is arranged by side id.  And we should only care about the local expander.
     * We are assuming if this expander goes away, the whole enclosure goes away on this side
     */
    if (encl_side == expander_component_index)
    {

        // We need to clear this reset reason if it's external reset
        if (general_info_elem_expander->reset_reason == FBE_ESES_RESET_REASON_EXTERNAL)
        {
            // Indicating this enclosure needs write
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,  // attribute
                                                FBE_ENCL_ENCLOSURE,  // component type
                                                0,  // component index
                                                TRUE);

            if(ENCL_STAT_OK(encl_status))
            {
                status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                        (fbe_base_object_t*)eses_enclosure, 
                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process expander element, can't set EMC_SPECIFIC_CONTROL_NEEDED, status: 0x%x.\n",
                                status);
           
                }
            }
        }
        else
        {
            /*
             * bphilbin - This if statement is a temporary workaround to avoid this code for voyager enclousres.  
             * We believe this can be removed for all enclosures, however given that we were so late in a release 
             * cycle it was the lesser of the evils to remove it just for voyager.  See AR 427563
             */
            if( (eses_enclosure->sas_enclosure_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM) &&
                (eses_enclosure->sas_enclosure_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE) )
            {

                /* If we come with enclosure reset reason be external reset, it means that
                 * we've previously seen it and now it's cleared.  We need to see if we 
                 * need to destroy and rebuild this object.
                 * Only check wether the destroy is needed when the object is not the component object, 
                 * i.e. the component id is zero. 
                 */
    
                component_id = fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)eses_enclosure);
                
                if((component_id == 0) &&
                   (eses_enclosure->reset_reason == FBE_ESES_RESET_REASON_EXTERNAL)) 
                {
                    // we have to be careful about enclosure 0_0
                    // port number
                    fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)eses_enclosure, &port_number);
                    // Enclosure Position
                    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, &enclosure_number);
    
                   
                    if ((port_number!=0) || (enclosure_number!=0))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "Enclosure was power cycled.  Destroy this object to allow proper clean up.\n");
    
                        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                                    &fbe_eses_enclosure_lifecycle_const,
                                                    (fbe_base_enclosure_t*)eses_enclosure, 
                                                    FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
                        if(!ENCL_STAT_OK(encl_status))     
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                    "%s can't set BO:DESTROY condition, status: 0x%X\n",
                                                    __FUNCTION__, encl_status);
    
                            return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                        }
                    }
                }
            }
        }

        eses_enclosure->reset_reason = general_info_elem_expander->reset_reason;

    }
    return (encl_status);
}


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_general_info_elem_for_temp_sensor(
        fbe_eses_enclosure_t * eses_enclosure,
        ses_general_info_elem_cmn_struct *general_info_elem_p)                
***************************************************************************
* @brief
*       This function processes the given general information element for 
*           temperature sensors.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  general_info_elem_p - The pointer to general information element for
*           temperature sensor.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   28-Jan-2009 - Rajesh V. Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_temp_sensor(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_general_info_elem_temp_sensor_struct *general_info_elem_temp_sensor;   
    fbe_u8_t temp_sensor_component_index = 0;
    fbe_u8_t        tempSensorAddStatus;
    fbe_bool_t      tempValid = FALSE;
    fbe_u16_t       temperature = 0;

    general_info_elem_temp_sensor = (ses_general_info_elem_temp_sensor_struct *)general_info_elem_p;

    encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, 
                                                                   FBE_ENCL_TEMP_SENSOR,
                                                                   general_info_elem_p->elem_index,
                                                                   &temp_sensor_component_index);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    if(fbe_eses_enclosure_is_TS_temp_update_required(eses_enclosure))
    {
        /*
         * Make sure the Temp Sensor is not faulted (if faulted, Temp is invalid
         */
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ADDL_STATUS,
                                                    FBE_ENCL_TEMP_SENSOR,
                                                    temp_sensor_component_index,
                                                    &tempSensorAddStatus);
        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }
        else if ((tempSensorAddStatus != SES_STAT_CODE_UNKNOWN) &&
            (tempSensorAddStatus != SES_STAT_CODE_NOT_INSTALLED))
        {
            tempValid = general_info_elem_temp_sensor->valid;
            temperature = swap16(general_info_elem_temp_sensor->temp);
        }


        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_TEMP_SENSOR_TEMPERATURE_VALID,
                                                     FBE_ENCL_TEMP_SENSOR,
                                                     temp_sensor_component_index,
                                                     tempValid);
        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }
    
        if(tempValid)
        {
            encl_status = fbe_base_enclosure_edal_setU16((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_TEMP_SENSOR_TEMPERATURE,
                                                         FBE_ENCL_TEMP_SENSOR,
                                                         temp_sensor_component_index,
                                                         temperature);
            if(!ENCL_STAT_OK(encl_status))
            {
                return encl_status;
            }
        }
    }

    return(FBE_ENCLOSURE_STATUS_OK);

}

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_general_info_elem_for_drive_slot(
        fbe_eses_enclosure_t * eses_enclosure,
        ses_general_info_elem_cmn_struct *general_info_elem_p)                
***************************************************************************
* @brief
*       This function processes the given general information element for 
*           drive slots.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  general_info_elem_p - The pointer to general information element for
*           drive slot.
* @param  battery_backed_updated - Output. Indicate whether battery backed info is updated. 
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   28-Aug-2012 - Rui Chang Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_drive_slot(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p,
    fbe_bool_t *battery_backed_updated)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_general_info_elem_array_dev_slot_struct *general_info_elem_drive_slot;   
    fbe_u8_t        drive_slot_component_index = 0;
    fbe_u8_t        drive_slot_number;

#if 0
    // Currently, we only care about this for jetfire
    if (eses_enclosure->sas_enclosure_type != FBE_SAS_ENCLOSURE_TYPE_FALLBACK)
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }
#endif

    general_info_elem_drive_slot  = (ses_general_info_elem_array_dev_slot_struct *)general_info_elem_p;

    encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, 
                                                                   FBE_ENCL_DRIVE_SLOT,
                                                                   general_info_elem_p->elem_index,
                                                                   &drive_slot_component_index);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_BATTERY_BACKED,  // attribute
                                                FBE_ENCL_DRIVE_SLOT,  // component type
                                                drive_slot_component_index,  // component index
                                                general_info_elem_drive_slot->battery_backed);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                FBE_ENCL_DRIVE_SLOT,
                                                drive_slot_component_index,
                                                &drive_slot_number);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    //For jetfire, we need set battery back bits correctly, for other types of enclosure, we just keep the default setting.
    if (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_FALLBACK)
    {
        //We only use first 4 drives for vaulting in jetfire, so disable others 
        if (drive_slot_number > FBE_ESES_ENCLOSURE_MAX_DRIVE_NUMBER_FOR_VAULTING)
        {
            fbe_eses_enclosure_set_drive_battery_backed_request(eses_enclosure, drive_slot_component_index, FALSE);
    
            if (general_info_elem_drive_slot->battery_backed)
            {
                *battery_backed_updated = TRUE;
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                                FBE_ENCL_DRIVE_SLOT,
                                                                drive_slot_component_index,
                                                                TRUE);
                if (!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }
        
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s Drv_slot %d, battery_backed %d, set to 0. \n", __FUNCTION__ ,drive_slot_number,general_info_elem_drive_slot->battery_backed);
            }    
        }
        else
        {
            fbe_eses_enclosure_set_drive_battery_backed_request(eses_enclosure, drive_slot_component_index, TRUE);
    
            if (!general_info_elem_drive_slot->battery_backed)
            {
                *battery_backed_updated = TRUE;
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                                FBE_ENCL_DRIVE_SLOT,
                                                                drive_slot_component_index,
                                                                TRUE);
                if (!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }
        
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s Drv_slot %d, battery_backed %d, set to 1. \n", __FUNCTION__ ,drive_slot_number,general_info_elem_drive_slot->battery_backed);
            }
        }
    }
    else
    {
        fbe_eses_enclosure_set_drive_battery_backed_request(eses_enclosure, drive_slot_component_index, general_info_elem_drive_slot->battery_backed);
    }
    return(FBE_ENCLOSURE_STATUS_OK);

}

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_general_info_elem_for_esc_electronics(
        fbe_eses_enclosure_t * eses_enclosure,
        ses_general_info_elem_cmn_struct *general_info_elem_p)                
***************************************************************************
* @brief
*       This function processes the given general information element for 
*           ESC Electronics.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  general_info_elem_p - The pointer to general information element for
*           ESC Electronics.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   08-Jan-2013 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_general_info_elem_for_esc_electronics(
    fbe_eses_enclosure_t * eses_enclosure,
    ses_general_info_elem_cmn_struct *general_info_elem_p)
{
    fbe_enclosure_status_t                         encl_status = FBE_ENCLOSURE_STATUS_OK;
    ses_general_info_elem_esc_electronics_struct  *general_info_elem_esc_electronics;   
    fbe_u8_t                                       lcc_component_index = 0;
    fbe_u8_t                                       subencl_id = 0;
    fbe_enclosure_component_types_t                componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;

    general_info_elem_esc_electronics = (ses_general_info_elem_esc_electronics_struct *)general_info_elem_p;

    fbe_eses_enclosure_elem_index_to_component_type(eses_enclosure, general_info_elem_p->elem_index, &componentType);

    if(componentType != FBE_ENCL_LCC) 
    {
        /* Viking enclosure has the ESC Electronics element for the chassis.
         * It needs to be filtered out. So add the check here.
         */
        return FBE_ENCLOSURE_STATUS_OK;
    }

    fbe_eses_enclosure_elem_index_to_subencl_id(eses_enclosure, general_info_elem_p->elem_index, &subencl_id);

    // Save ESC electronics to its subencl LCC.
    lcc_component_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_SUB_ENCL_ID,  //attribute
                                                    FBE_ENCL_LCC,  // Component type
                                                    0, //starting index
                                                    subencl_id);   

    if(lcc_component_index == FBE_ENCLOSURE_VALUE_INVALID) 
    {
        return FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND;
    }
  
    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_LCC_ECB_FAULT,
                                                 FBE_ENCL_LCC,
                                                 lcc_component_index,
                                                 general_info_elem_esc_electronics->ecb_fault);
    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_is_TS_temp_update_required()                  
***************************************************************************
* @brief
*       This function decides if the Individual Temperature Sensors 
*        temperature in base enclosure needs to be updated with the
*        latest temperature obtained from the EMC enclosure status diag page.
*
* @param   
*       eses_enclosure - The pointer to the ESES enclosure.
* 
* @return  fbe_bool_t.
*   FBE_TRUE -- temperature needs to be requested
*   FBE_FALSE -- termperature need NOT be requested.
*
* NOTES
*
* HISTORY
*   03-Nov-2009 - Rajesh V. Created.
***************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_is_TS_temp_update_required(fbe_eses_enclosure_t * eses_enclosure)
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    if(((eses_enclosure->poll_count - 1) % 
        (FBE_ESES_ENCLOSURE_TS_TEMPERATUE_UPDATE_POLL_FREQUENCY)) == 0)
    {
        return(TRUE);
    }

    return(FALSE);
    
}// End of function fbe_eses_enclosure_is_TS_temp_update_required


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_ps_info(fbe_eses_enclosure_t * eses_enclosure, 
*                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p)                
***************************************************************************
* @brief
*       This function processes the Power Supply information elements 
*       in the EMC specific status page.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  info_elem_group_hdr_p - The pointer to the info element group header.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   29-Oct-2009 - Rajesh V. Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_process_ps_info(fbe_eses_enclosure_t * eses_enclosure,
                                   fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) 
{
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            ps_info_elem;
    fbe_u8_t                            num_ps_info_elems;      
    fbe_u8_t                            ps_elem_index = 0;      // ESES elem index in status page
    fbe_u8_t                            ps_component_index = 0; // component index related to EDAL component data
    ses_ps_info_elem_struct             *ps_info_elem_p = NULL;
    
    /*********
     * BEGIN 
     *********/

    ps_info_elem_p = fbe_eses_get_first_ps_info_elem_p(info_elem_group_hdr_p);
    num_ps_info_elems = info_elem_group_hdr_p->num_info_elems;

    for(ps_info_elem = 0; ps_info_elem < num_ps_info_elems; ps_info_elem++)
    {
        ps_elem_index = ps_info_elem_p->ps_elem_index;

        encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, 
                                                                       FBE_ENCL_POWER_SUPPLY,
                                                                       ps_elem_index,
                                                                       &ps_component_index);
        if(encl_status == FBE_ENCLOSURE_STATUS_OK)
        {
            encl_status =  fbe_eses_enclosure_process_input_power_sample(eses_enclosure,
                                                                       ps_component_index,
                                                                       ps_info_elem_p->input_power_valid,
                                                                       swap16(ps_info_elem_p->input_power));
            if(!ENCL_STAT_OK(encl_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_WARNING,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process_input_power_sample failed,ps_comp_index:%d,encl_status:0x%x.\n", 
                                ps_component_index, encl_status);
            }

            // Set PS Margining Test info
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_PS_MARGIN_TEST_MODE,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         ps_component_index,
                                                         ps_info_elem_p->margining_test_mode);
           if(!ENCL_STAT_OK(encl_status))
           {
               return encl_status;
           }
           encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_PS_MARGIN_TEST_RESULTS,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        ps_component_index,
                                                        ps_info_elem_p->margining_test_results);
          if(!ENCL_STAT_OK(encl_status))
          {
              return encl_status;
          }
            
        } // Valid Power supply ESES index

        ps_info_elem_p = fbe_eses_get_next_ps_info_elem_p(ps_info_elem_p);
    }
    return(encl_status);
}

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_sps_info(fbe_eses_enclosure_t * eses_enclosure, 
*                                          fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p)                
***************************************************************************
* @brief
*       This function processes the SPS information elements 
*       in the EMC specific status page.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  info_elem_group_hdr_p - The pointer to the info element group header.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   22-Aug-2012 - Joe Perry     Created.
***************************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_process_sps_info(fbe_eses_enclosure_t * eses_enclosure,
                                   fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) 
{
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            sps_info_elem;
    fbe_u8_t                            num_sps_info_elems;      
    fbe_u8_t                            sps_elem_index = 0;      // ESES elem index in status page
    fbe_u8_t                            sps_component_index = 0; // component index related to EDAL component data
    ses_sps_info_elem_struct            *sps_info_elem_p = NULL;
    fbe_u16_t                           byteSwapValue;
    fbe_u16_t                           previousSpsStatus;
    fbe_u16_t                           previousSpsBatTime;
    fbe_bool_t                          spsInserted = FALSE;
    
    /*********
     * BEGIN 
     *********/

    sps_info_elem_p = fbe_eses_get_first_sps_info_elem_p(info_elem_group_hdr_p);
    num_sps_info_elems = info_elem_group_hdr_p->num_info_elems;

    for(sps_info_elem = 0; sps_info_elem < num_sps_info_elems; sps_info_elem++)
    {
        sps_elem_index = sps_info_elem_p->sps_elem_index;

        encl_status = fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, 
                                                                       FBE_ENCL_SPS,
                                                                       sps_elem_index,
                                                                       &sps_component_index);
        if(encl_status == FBE_ENCLOSURE_STATUS_OK)
        {
            /*
             * Get SPS Status & check if changed
             */
            // get previous SPS Status
            encl_status = fbe_base_enclosure_edal_getU16((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_SPS_STATUS,
                                                          FBE_ENCL_SPS,
                                                          sps_component_index,
                                                          &previousSpsStatus);
            if(!ENCL_STAT_OK(encl_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                   "%s, failed getting SpsStatus, encl_status %d\n", 
                                                   __FUNCTION__, encl_status);
                return encl_status;
            }

            // compare new SPS Status & save if needed
            if (sps_info_elem_p->sps_status_valid)
            {
                spsInserted = TRUE;
                byteSwapValue = swap16(sps_info_elem_p->sps_status);
                if (previousSpsStatus != byteSwapValue)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                       "%s, SpsStatus change, 0x%x to 0x%x\n", 
                                                       __FUNCTION__,
                                                       previousSpsStatus,
                                                       byteSwapValue);
                    encl_status = fbe_base_enclosure_edal_setU16((fbe_base_enclosure_t *)eses_enclosure,
                                                                  FBE_SPS_STATUS,
                                                                  FBE_ENCL_SPS,
                                                                  sps_component_index,
                                                                  byteSwapValue);
                    if(!ENCL_STAT_OK(encl_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_ERROR,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "%s, failed setting SpsStatus, encl_status %d\n", 
                                                           __FUNCTION__, encl_status);
                        return encl_status;
                    }
                }
            }
            
#if FALSE
            // SPS Inserted based off of valid bit setting
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,
                                                            FBE_ENCL_SPS,
                                                            sps_component_index,
                                                            spsInserted);
            if(!ENCL_STAT_OK(encl_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                   "%s, failed setting SPS Inserted %d, encl_status %d\n", 
                                                   __FUNCTION__, spsInserted, encl_status);
                return encl_status;
            }
#endif

            /*
             * Get SPS BatTime & check if changed
             */
            // get previous SPS Status
            encl_status = fbe_base_enclosure_edal_getU16((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_SPS_BATTIME,
                                                          FBE_ENCL_SPS,
                                                          sps_component_index,
                                                          &previousSpsBatTime);
            if(!ENCL_STAT_OK(encl_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                   "%s, failed getting SpsBatTime, encl_status %d\n", 
                                                   __FUNCTION__, encl_status);
                return encl_status;
            }
            // compare new SPS BatTime & save if needed
            if (sps_info_elem_p->sps_battime_valid)
            {
                byteSwapValue = swap16(sps_info_elem_p->sps_battime);
                if (previousSpsBatTime != byteSwapValue)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                       "%s, SpsBatTime change, 0x%x to 0x%x\n", 
                                                       __FUNCTION__,
                                                       previousSpsBatTime,
                                                       byteSwapValue);
                    encl_status = fbe_base_enclosure_edal_setU16((fbe_base_enclosure_t *)eses_enclosure,
                                                                 FBE_SPS_BATTIME,
                                                                 FBE_ENCL_SPS,
                                                                 sps_component_index,
                                                                 byteSwapValue);
                    if(!ENCL_STAT_OK(encl_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                           FBE_TRACE_LEVEL_ERROR,
                                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                           "%s, failed setting SpsBattime, encl_status %d\n", 
                                                           __FUNCTION__, encl_status);
                        return encl_status;
                    }
                }
            }

        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_WARNING,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, elem_index_to_comp_index failed, encl_status %d\n", 
                            __FUNCTION__, encl_status);
        }

        sps_info_elem_p = fbe_eses_get_next_sps_info_elem_p(sps_info_elem_p);
    }
    return(encl_status);
}


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_input_power_sample(fbe_eses_enclosure_t * eses_enclosure, 
*                                                    fbe_u8_t   ps_component_index,
*                                                    fbe_bool_t  sample_valid, 
*                                                    fbe_u16_t   input_power_sample)            
***************************************************************************
* @brief
*   This function processes the current input power sample and updates
*   the input power in EDAL if needed.   
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   ps_component_index - Component index of the Power Supply
* @param   sample_valid - validity of the current input power sample
* @param   input_power_sample - current input power sample
*
* @return fbe_enclosure_status_t 
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   19-Jan-2010 Rajesh V. - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_input_power_sample(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t    ps_component_index,
                                              fbe_bool_t  sample_valid, 
                                              fbe_u16_t   input_power_sample) 
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t    *valid_input_power_sample_count;
    fbe_u8_t    *invalid_input_power_sample_count;
    fbe_u32_t   *total_input_power;

    if(ps_component_index >= FBE_ESES_ENCLOSURE_MAX_PS_INFO_COLLECTED)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "ps_component_index invalid value: %d\n", 
                         ps_component_index);
        return(FBE_ENCLOSURE_STATUS_INVALID);

    }
    
    valid_input_power_sample_count =
        &eses_enclosure->ps_eir_info[ps_component_index].valid_input_power_sample_count;
    invalid_input_power_sample_count = 
        &eses_enclosure->ps_eir_info[ps_component_index].invalid_input_power_sample_count;
    total_input_power = 
        &eses_enclosure->ps_eir_info[ps_component_index].total_input_power;  

    if(sample_valid)
    {
        (*valid_input_power_sample_count)++;
        *total_input_power += input_power_sample;
    }
    else
    {
        (*invalid_input_power_sample_count)++;
    }

    // See if it is required to update the input power in EDAL
    if(fbe_eses_enclosure_is_input_power_update_needed(eses_enclosure))
    {
        fbe_u16_t   input_power;

        // Input power is marked invalid if there is atleast one invalid sample
        if(*invalid_input_power_sample_count == 0)
        {
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_PS_INPUT_POWER_STATUS,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         ps_component_index,
                                                         FBE_EDAL_ATTRIBUTE_VALID);
           if(!ENCL_STAT_OK(encl_status))
           {
               return encl_status;
           }


            input_power = ((fbe_u32_t)(*total_input_power)) / (*valid_input_power_sample_count);
            encl_status = fbe_base_enclosure_edal_setU16((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_PS_INPUT_POWER,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         ps_component_index,
                                                         input_power); 
             if(!ENCL_STAT_OK(encl_status))
           {
               return encl_status;
           }

        }
        else
        {
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_PS_INPUT_POWER_STATUS,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         ps_component_index,
                                                         FBE_EDAL_ATTRIBUTE_INVALID);
            if(!ENCL_STAT_OK(encl_status))
            {
                return encl_status;
            }
        }
        // Now that input power data in EDAL is updated, clear the PS EIR related private

        // data in enclosure object for the next sampling period.
        *valid_input_power_sample_count = 0;
        *invalid_input_power_sample_count = 0;
        *total_input_power = 0;
    }

    return(encl_status);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_is_input_power_update_needed()                  
***************************************************************************
* @brief
*       This function decides if the input power needs to be updated in
*       EDAL
*
* @param   
*       eses_enclosure - The pointer to the ESES enclosure.
* 
* @return  fbe_bool_t.
*   FBE_TRUE -- input power update required
*   FBE_FALSE -- input power update Not required.
*
* NOTES
*
* HISTORY
*   20-Jan-2010 Rajesh V. - Created.
***************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_is_input_power_update_needed(fbe_eses_enclosure_t * eses_enclosure)
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    if(((eses_enclosure->poll_count - 1) % 
        (FBE_ESES_ENCLOSURE_INPUT_POWER_UPDATE_POLL_FREQUENCY)) == 0)
    {
        return(TRUE);
    }

    return(FALSE);
    
}// End of function fbe_eses_enclosure_is_emc_specific_status_needed


/*!*************************************************************************
* @fn  fbe_eses_enclosure_update_shutdown_info(fbe_eses_enclosure_t * eses_enclosure, 
*                                              ses_pg_emc_encl_stat_struct * emc_specific_status_page_p)                
***************************************************************************
* @brief
*   This function updates the shutdown reason and whether it is a partial shutdonw or not.
*   If there is no shutdown scheduled, the shutdown reason is zero.
*   If the shutdown reason is nonzero, partial shutdown is one if the shutdown
*   affects only part of the enclosure and does not shut down the EMA.    
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   emc_specific_status_page - The pointer to the EMC specific status page.
*
* @return fbe_enclosure_status_t 
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   10-Apr-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_update_shutdown_info(fbe_eses_enclosure_t * eses_enclosure, 
                                          ses_pg_emc_encl_stat_struct * emc_specific_status_page_p)
{
    fbe_enclosure_status_t  encl_status_shutdown_reason = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t  encl_status_partial = FBE_ENCLOSURE_STATUS_OK;

   
    encl_status_shutdown_reason = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_SHUTDOWN_REASON, // Attribute
                                                            FBE_ENCL_ENCLOSURE, // Component type
                                                            0, // Component index 
                                                            emc_specific_status_page_p->shutdown_reason);


    if(!ENCL_STAT_OK(encl_status_shutdown_reason))
    {
        return encl_status_shutdown_reason;
    } 

    encl_status_partial = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_PARTIAL_SHUTDOWN, // Attribute
                                                            FBE_ENCL_ENCLOSURE, // Component type
                                                            0, // Component index 
                                                            emc_specific_status_page_p->partial);

    if(!ENCL_STAT_OK(encl_status_partial))
    {
        return encl_status_partial;
    }     

    if((encl_status_shutdown_reason == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED) || 
       (encl_status_partial == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Shutdown Reason: 0x%x, Partial shutdown: 0x%x.\n", 
                            emc_specific_status_page_p->shutdown_reason,
                            emc_specific_status_page_p->partial);
    }
    return FBE_ENCLOSURE_STATUS_OK;

}// End of function fbe_eses_enclosure_update_shutdown_info

/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_sas_conn_info(fbe_eses_enclosure_t * eses_enclosure, 
*                                               fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p)                
***************************************************************************
* @brief
* This function processes the sas connector information elements in
* the EMC specific status page to extract various elements and attributes and
* return them in the eses_enclosure via edal.
*
* @param eses_enclosure - The pointer to the enclosure.  This is
*        output which is written via edal functions.  
*
* @param info_elem_group_hdr_p - The pointer to the info element group
*        header.  This is input and should not be written by this function.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_sas_conn_info(fbe_eses_enclosure_t * eses_enclosure, 
                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) 
{
    fbe_u8_t                        elem = 0;
    ses_sas_conn_info_elem_struct * sas_conn_info_elem_p = NULL;
    fbe_u8_t                        connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          port_bypass_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_sas_address_t               serverSasAddress;
    fbe_sas_address_t               attached_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_bool_t                      is_local_connector = FALSE;
    fbe_bool_t                      is_entire_connector = FALSE;
    fbe_bool_t                      *connector_disable_list;
    fbe_bool_t                      undersized_info_elem = FALSE;
    fbe_u8_t                        attached_sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_u8_t                        sp_id = 0;
    fbe_u8_t                        comp_location = 0;
    fbe_u8_t                        primary_port_entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t                        saved_primary_port_entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u32_t                       port = 0; 
    /*********
     * BEGIN
     *********/
    /* 
     * Get the server sas address for use later on.
     */

    encl_status = fbe_base_enclosure_edal_getU64((fbe_base_enclosure_t *)eses_enclosure,
                                               FBE_ENCL_SERVER_SAS_ADDRESS,
                                               FBE_ENCL_ENCLOSURE,
                                               0, // only 1 enclosure.
                                               &serverSasAddress);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    sas_conn_info_elem_p = fbe_eses_get_first_sas_conn_info_elem_p(info_elem_group_hdr_p);


    if (sizeof(ses_sas_conn_info_elem_struct) > info_elem_group_hdr_p->info_elem_size)
    {
        // We have a difference in the size of the info element.  This
        // suggests that we are running with old CDES firmware.  The
        // problem is that we will add to the sas_conn_info_elem_p
        // based on the wrong (i.e.  smaller) size and therefore
        // become mis-aligned.  We must report an error and fail this
        // enclosure.
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "conn_info_elem struct size is larger than CDES size\n");
        undersized_info_elem = TRUE;
    }

    for(elem = 0; elem < info_elem_group_hdr_p->num_info_elems; elem ++)
    {
        if (undersized_info_elem)
        {
            attached_sub_encl_id = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
        }
        else
        {
            attached_sub_encl_id = sas_conn_info_elem_p->attached_sub_encl_id;
        }

        /* It is either an entire connector or a lane of the entire connector. */
        if( fbe_eses_enclosure_elem_index_to_component_index(eses_enclosure, FBE_ENCL_CONNECTOR,
              sas_conn_info_elem_p->conn_elem_index, &connector_index) == FBE_ENCLOSURE_STATUS_OK )
        {    
            attached_sas_address = swap64(sas_conn_info_elem_p->attached_sas_address);

            /* Save the attached sas address for either local connector or peer connector. */
            encl_status = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS,
                                                            FBE_ENCL_CONNECTOR,
                                                            connector_index,
                                                            attached_sas_address);

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }


            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                           FBE_ENCL_CONNECTOR_ID,
                                           FBE_ENCL_CONNECTOR,
                                           connector_index,
                                           sas_conn_info_elem_p->conn_id);
            
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }
            
            
            /* we want to check on the element size before accessing the new field attached_sub_encl_id. */
            // This is special code to handle attached_sub_encl_id
            // when running with CDES firmware that does not support
            // it yet.  We force the attached sub encl id to be assigned based on conn_id.
            if(attached_sub_encl_id ==  FBE_ESES_SUBENCL_SIDE_ID_INVALID)
            {
#if 0
                // Rewrite the original code to also trace that it is being used.  If CDES is working than
                // we should never see this trace and this code can be removed.

                if (sas_conn_info_elem_p->conn_id > 3)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "ERROR attached_sub_encl_id (%d) is getting assigned to conn_id (%d) / 2\n",
                                    attached_sub_encl_id, sas_conn_info_elem_p->conn_id);

                    attached_sub_encl_id = sas_conn_info_elem_p->conn_id/2;

                }

                // Original code which we think may be obsolete.
                attached_sub_encl_id = (sas_conn_info_elem_p->conn_id > 3)?
                                                              sas_conn_info_elem_p->conn_id/2:
                                                              FBE_ESES_SUBENCL_SIDE_ID_INVALID;
#endif
           }
           
           encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                          FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID,
                                          FBE_ENCL_CONNECTOR,
                                          connector_index,
                                           attached_sub_encl_id);                                           

           if (encl_status != FBE_ENCLOSURE_STATUS_OK)
           {
               return encl_status;
           }


            encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_IS_LOCAL,
                                                            FBE_ENCL_CONNECTOR,
                                                            connector_index,
                                                            &is_local_connector);

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            encl_status = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, &sp_id);

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            if(((sp_id == SP_A) && (is_local_connector)) ||
               ((sp_id == SP_B) && (!is_local_connector)))
            {
                comp_location = sas_conn_info_elem_p->conn_id;
            }
            else
            {
                comp_location = sas_conn_info_elem_p->conn_id + 
                     fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure);
            }

            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                       FBE_ENCL_COMP_LOCATION,
                                       FBE_ENCL_CONNECTOR,
                                       connector_index,
                                       comp_location);
            
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            if(is_local_connector) 
            {
               /* This is a local connector and we have not found the primary_port_entire_connector_index yet.
                * We only know the serverSasAddress for LOCAL side. So we can only know the 
                * primary port for connectors on local LCC.
                * Compare the server sas address with the attached sas address of the connector
                * to find out if the connector is a primary connector.
                */
                if ((attached_sas_address == serverSasAddress) &&
                    (serverSasAddress != FBE_SAS_ADDRESS_INVALID))
                {  
                    // Check if this is an entire connector.
                    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute.
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        connector_index,     // Component index.
                                                        &is_entire_connector);

                    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return encl_status;
                    }
                    
                    if(is_entire_connector)
                    {
                        /* It is an entire connector. */
                        primary_port_entire_connector_index = connector_index; 
                    }
                    else
                    {                
                        /* Get the primary port entire connector index. */
                        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_CONTAINER_INDEX,
                                                                    FBE_ENCL_CONNECTOR,
                                                                    connector_index,
                                                                    &primary_port_entire_connector_index);

                        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                        {
                            return encl_status;
                        }
                    }

                    if((primary_port_entire_connector_index != saved_primary_port_entire_connector_index) &&
                       (port < FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT))
                    {
                        eses_enclosure->primary_port_entire_connector_index[port] = primary_port_entire_connector_index;
                        saved_primary_port_entire_connector_index = primary_port_entire_connector_index;
                        port ++;
                    }
                }

                //Do not "break" here because we need to continue setting the attached_sas_address for all the connectors.
         
            } // End of - if(is_local_connector)&& ...
        }// End of a valid connector index.

        // Get the next sas connector information element in the EMC specific status page.
        sas_conn_info_elem_p = fbe_eses_get_next_sas_conn_info_elem_p(sas_conn_info_elem_p, 
            info_elem_group_hdr_p->info_elem_size);

    }// End of for loop. 

    /* Set the primary port attribute for all the connectors since we now know
     * the primary_port_entire_connector_index. 
     */
    encl_status = fbe_eses_enclosure_populate_connector_primary_port(eses_enclosure);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "populate_primary_port failed, encl_status: 0x%x.\n", 
                         encl_status);

        return encl_status;
    }

    /* Now, we need to check on which port we need to disable, if any. */
    connector_disable_list = fbe_eses_enclosure_get_connector_disable_list_ptr(eses_enclosure);
    if (connector_disable_list!=NULL)
    {
        // make sure not used port is disabled.
        for (connector_index = 0; connector_index < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure) ; connector_index++)
        {
            if(connector_disable_list[connector_index])
            {
                port_bypass_status = fbe_eses_enclosure_process_unused_connector(eses_enclosure,
                                                                          connector_index);
                if (port_bypass_status==FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
                {
                    /*
                    * Set condition to write out ESES Control
                    */

                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const,
                                                    (fbe_base_object_t*)eses_enclosure,
                                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, can't set COND_EXPANDER_CONTROL_NEEDED, status: 0x%x.\n",
                                                __FUNCTION__,  status);
                        encl_status = FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                        break;
                    }
                }

            }
        }
    }
    return encl_status;
   
}// End of function fbe_eses_enclosure_process_sas_conn_info

/*!*************************************************************************
* @fn  fbe_eses_enclosure_populate_connector_primary_port(fbe_eses_enclosure_t * eses_enclosure)                 
***************************************************************************
* @brief
*       This function sets FBE_ENCL_CONNECTOR_PRIMARY_PORT attribute for all the 
*       connectors.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  primary_port_entire_connector_index - 
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
*   
* NOTES
*   It can be only called after primary_port_entire_connector_index is available. 
*
* HISTORY
*   06-Jan-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_populate_connector_primary_port(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u8_t connector_index = 0;
    fbe_u8_t entire_connector_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t port = 0; // for loop.

    for(connector_index = 0; 
        connector_index < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure);
        connector_index ++)
    {
        /* Initialize the primary port attribut to FALSE for all the connectors. 
         * It will be set again if it turns out to be a primary port. 
         */
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    FALSE);

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
    }

    for (port = 0; port < FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT; port ++) 
    {
        if(eses_enclosure->primary_port_entire_connector_index[port] == FBE_ENCLOSURE_VALUE_INVALID)
        {
            break;
        }
        else if(eses_enclosure->primary_port_entire_connector_index[port] >= fbe_eses_enclosure_get_number_of_connectors(eses_enclosure))
        {
            encl_status = FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
        }
        else
        {        
            for(connector_index = 0; 
                connector_index < fbe_eses_enclosure_get_number_of_connectors(eses_enclosure);
                connector_index ++)
            {
                // Get the primary port entire connector index of the connector.
                encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_CONTAINER_INDEX,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        &entire_connector_index);
    
                if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    return encl_status;
                }
    
               /* If the entire_connector_index of the connector is equal to primary_port_entire_connector_index,
                * set the primary port attribute to TRUE.
                */
                
                if((entire_connector_index == eses_enclosure->primary_port_entire_connector_index[port]) ||
                   (connector_index == eses_enclosure->primary_port_entire_connector_index[port]))
                {
                    
                    encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                                            FBE_ENCL_CONNECTOR,
                                                            connector_index,
                                                            TRUE);
    
                    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return encl_status;
                    }
                }
            }// End of for loop 
        }
    }
    
    return FBE_ENCLOSURE_STATUS_OK;
   
}// End of function fbe_eses_enclosure_populate_connector_primary_port

/*!*************************************************************************
*   @fn fbe_eses_enclosure_process_trace_buf_info()                  
***************************************************************************
* @brief
*       This function processes the trace buffer information elements 
*       in the EMC specific status page to set the flag to indicate
*       whether there is any saved trace buffer.  
*
* @param
*       eses_enclosure - The pointer to the enclosure.
*       info_elem_group_hdr_p - The pointer to the information element group header.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error is found.
* NOTES
*
* HISTORY
*   17-Nov-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_trace_buf_info(
                                                         fbe_eses_enclosure_t * eses_enclosure, 
                                                         fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p) 
{
    fbe_u8_t elem = 0;
    ses_trace_buf_info_elem_struct * trace_buf_info_elem_p = NULL;
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;

    /********
     * BEGIN
     ********/

    trace_buf_info_elem_p = fbe_eses_get_first_trace_buf_info_elem_p(info_elem_group_hdr_p);

    for(elem = 0; elem < info_elem_group_hdr_p->num_info_elems; elem ++)
    {
        if((trace_buf_info_elem_p->buf_action != FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE) &&
            (trace_buf_info_elem_p->buf_action != FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF))
        {
            // Set the FBE_ENCL_TRACE_PRESENT attribute to TRUE.
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_TRACE_PRESENT,
                                                    FBE_ENCL_ENCLOSURE, // Component type.
                                                    0,  // only 1 enclosure
                                                    TRUE);
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }
        }

        // Get the pointer to the next trace buffer info element.
        trace_buf_info_elem_p = fbe_eses_get_next_trace_buf_info_elem_p(trace_buf_info_elem_p, 
            info_elem_group_hdr_p->info_elem_size);
    } // End of - for(elem = 0; elem < info_elem_group_hdr_p->num_info_elems; elem ++)

    return FBE_ENCLOSURE_STATUS_OK;
}// End of function fbe_eses_enclosure_process_trace_buf_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_info_elem_group_hdr_p(fbe_eses_enclosure_t * eses_enclosure, 
*                                        ses_pg_emc_encl_stat_struct * emc_specific_status_page_p,
*                                        fbe_eses_info_elem_type_t info_elem_type,
*                                        fbe_eses_info_elem_group_hdr_t ** info_elem_group_hdr_p)                
***************************************************************************
* @brief
*       This function gets the pointer which points to the info elemement group header
*       with the specified type.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  emc_specific_status_page - The pointer to the EMC specific status page.
* @param  info_elem_type - The type of the information element type.
* @param  info_elem_group_hdr_p (Output)- The pointer to the info elemement group header.
*       
* RETURN VALUES
*       fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   05-Oct-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_info_elem_group_hdr_p(fbe_eses_enclosure_t * eses_enclosure, 
                                                ses_pg_emc_encl_stat_struct * emc_specific_status_page_p,
                                                fbe_eses_info_elem_type_t info_elem_type,
                                                fbe_eses_info_elem_group_hdr_t ** info_elem_group_hdr_p_p)
         
{
    ses_common_pg_hdr_struct * pg_hdr_p= NULL;
    fbe_eses_info_elem_group_hdr_t * info_elem_group_hdr_p = NULL;
    fbe_u32_t page_size = 0;
    fbe_u32_t byte_offset = 0;
    
    /********
     * BEGIN
     ********/
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                          "%s entry.\n", __FUNCTION__);

    pg_hdr_p = (ses_common_pg_hdr_struct *)emc_specific_status_page_p;

    FBE_ESES_GET_PAGE_SIZE(page_size, pg_hdr_p);

    info_elem_group_hdr_p = fbe_eses_get_first_info_elem_group_hdr_p(emc_specific_status_page_p);

    for(byte_offset = FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET; byte_offset < page_size;)
    {
        if(info_elem_group_hdr_p->info_elem_type == info_elem_type)
        {
            *info_elem_group_hdr_p_p = info_elem_group_hdr_p;
            return FBE_ENCLOSURE_STATUS_OK;
        }

        byte_offset += (FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE + 
                        info_elem_group_hdr_p->num_info_elems * info_elem_group_hdr_p->info_elem_size);

        info_elem_group_hdr_p = fbe_eses_get_next_info_elem_group_hdr_p(info_elem_group_hdr_p);
    }// End of - for loop.

    return FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND;

}  // End of - fbe_eses_enclosure_get_info_elem_group_hdr_p


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_mode_param_list(fbe_eses_enclosure_t * eses_enclosure,
*                                                 fbe_u8_t * mode_param_list_p)                  
***************************************************************************
* @brief
*       This function process the mode parameter list.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  mode_param_list_p - The pointer to the mode parameter list.
*
* @return   fbe_enclosure_status_t 
*       FBE_ENCLOSURE_STATUS_OK - no error.
*       otherwise - an error is found.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_mode_param_list(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_eses_mode_param_list_t * mode_param_list_p)
{
    fbe_u16_t    byte_offset = 0;
    fbe_u32_t    mode_param_list_size = 0;
    fbe_eses_mode_pg_common_hdr_t * mode_pg_common_hdr_p = NULL;    
    fbe_eses_pg_eep_mode_pg_t * eep_mode_pg_p = NULL;
    fbe_eses_pg_eenp_mode_pg_t * eenp_mode_pg_p = NULL;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
       
    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_DEBUG_HIGH,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "%s entry\n", __FUNCTION__);

    
    /*
     * Check if the size of the entire mode parameter list is less than the allocated response buffer size.
     * The fist byte that mode_param_list_p points to is the lengh of the mode parameter list (the first byte is not included),
     * so we need to add 1 for the first byte.
     */
    mode_param_list_size = swap16(mode_param_list_p->mode_data_len) + 1;

    if(mode_param_list_size > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE)
    {
         fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s, not enough memory allocated for mode param list, mode_param_list size: %d, allocated size: %llu.\n", 
                         __FUNCTION__, mode_param_list_size, (unsigned long long)FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE);  
   
        return FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;    
    }

    // Initialized mode_select_needed to 0.
    eses_enclosure->mode_select_needed = 0;

    mode_pg_common_hdr_p = fbe_eses_get_first_mode_page(mode_param_list_p);

    for(byte_offset = FBE_ESES_MODE_PARAM_LIST_HEADER_SIZE; byte_offset < mode_param_list_size; byte_offset ++)
    {
        switch(mode_pg_common_hdr_p->pg_code)
        {
            case SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG:
                // This is EMC ESES persistent mode page.
                eep_mode_pg_p = (fbe_eses_pg_eep_mode_pg_t *)mode_pg_common_hdr_p;
                encl_status = fbe_eses_enclosure_process_eep_mode_page(eses_enclosure, eep_mode_pg_p);
                break;

            case SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG:
                // This is EMC ESES non-persistent mode page.
                eenp_mode_pg_p = (fbe_eses_pg_eenp_mode_pg_t *)mode_pg_common_hdr_p;
                encl_status = fbe_eses_enclosure_process_eenp_mode_page(eses_enclosure, eenp_mode_pg_p);
                break;

            default:
                // We don't care about other mode pages so far.
                break;
        }
       
        byte_offset += (mode_pg_common_hdr_p->pg_len + FBE_ESES_MODE_PAGE_COMMON_HEADER_SIZE);
        mode_pg_common_hdr_p = fbe_eses_get_next_mode_page(mode_pg_common_hdr_p);
    }// End of - for loop.

    return encl_status;
} // End of function fbe_eses_enclosure_process_mode_param_list


/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_eep_mode_page(fbe_eses_enclosure_t * eses_enclosure,
*                                    fbe_eses_pg_eep_mode_pg_t * eep_mode_pg_p)                  
***************************************************************************
* @brief
*       This function process the EMC ESES persistent(eep) mode page.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  eep_mode_pg_p - The pointer to the EMC ESE persistent(eep) mode page.
*
* @return   fbe_enclosure_status_t 
*     Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_eep_mode_page(
                                    fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_eses_pg_eep_mode_pg_t * eep_mode_pg_p) 
{
    fbe_status_t            status = FBE_STATUS_OK;
       
    /**********
     * BEGIN
     **********/

     fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_DEBUG_HIGH,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "%s entry\n", __FUNCTION__ );
     
    // If it is in non-high availabilty environment or does not disable indicator control, 
    // mode select is needed to set it to high availability environment and disable indicator control.
    if((eep_mode_pg_p->ha_mode != FBE_ESES_ENCLOSURE_HA_MODE_DEFAULT) ||
        (eep_mode_pg_p->disable_indicator_ctrl != FBE_ESES_ENCLOSURE_DISABLE_INDICATOR_CTRL_DEFAULT))
    {

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "process_eep_mode_page, ha_mode: 0x%x, disable_indicator_ctrl: 0x%x.\n", 
                    eep_mode_pg_p->ha_mode, eep_mode_pg_p->disable_indicator_ctrl);
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                     FBE_TRACE_LEVEL_INFO,
                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                     "Mode Select needed for EEP\n");

        
        eses_enclosure->mode_select_needed |= FBE_ESES_MODE_SELECT_EEP_MODE_PAGE;
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                       (fbe_base_object_t*)eses_enclosure, 
                                       FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSELECTED);
        if(status != FBE_STATUS_OK)            
        {
            /* Failed to set the mode unselected condition. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_ERROR,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s, can't set mode unselected condition, status: 0x%x.\n",
                         __FUNCTION__, status);
            return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }
    }
    
    return FBE_ENCLOSURE_STATUS_OK;
}// End of - fbe_eses_enclosure_process_eep_mode_page
/*!*************************************************************************
* @fn  fbe_eses_enclosure_process_eenp_mode_page(fbe_eses_enclosure_t * eses_enclosure,
*                                    fbe_eses_pg_eenp_mode_pg_t * eenp_mode_pg_p)                  
***************************************************************************
* @brief
*       This function process the EMC ESES non persistent(eenp) mode page.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  eep_mode_pg_p - The pointer to the EMC ESE non persistent(eenp) mode page.
*
* @return   fbe_enclosure_status_t 
*     Always returns FBE_ENCLOSURE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   04-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_eenp_mode_page(
                                    fbe_eses_enclosure_t * eses_enclosure,
                                    fbe_eses_pg_eenp_mode_pg_t * eenp_mode_pg_p) 
{
    fbe_status_t            status = FBE_STATUS_OK;
//    fbe_u8_t                spsCount;
       
    /**********
     * BEGIN
     **********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s entry\n", __FUNCTION__ );   

    // Mode Select needed if TestMode or SpsSupport needed
    eses_enclosure->test_mode_status = eenp_mode_pg_p->test_mode;
    if ((eenp_mode_pg_p->test_mode != eses_enclosure->test_mode_rqstd) ||
        (eenp_mode_pg_p->sps_dev_supported != eses_enclosure->sps_dev_supported))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "process_eenp_mode_page, testMd: %d (%d), spsDevSupp %d (%d).\n", 
                    eenp_mode_pg_p->test_mode, 
                    eses_enclosure->test_mode_rqstd,
                    eenp_mode_pg_p->sps_dev_supported,
                    eses_enclosure->sps_dev_supported);
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                    "process_eenp_mode_page, include_drive_connectors 0x%x, retried %d.\n", 
                    eenp_mode_pg_p->include_drive_connectors,
                    eses_enclosure->mode_sense_retry_cnt);
        
        eses_enclosure->mode_select_needed |= FBE_ESES_MODE_SELECT_EENP_MODE_PAGE;
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                       (fbe_base_object_t*)eses_enclosure, 
                                       FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSELECTED);
        if(status != FBE_STATUS_OK)            
        {
            /* Failed to set the mode unselected condition. */
           fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "%s, can't set mode unselected condition, status: 0x%x.\n",
                                  __FUNCTION__, status);
            return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }
    }

    return FBE_ENCLOSURE_STATUS_OK;
}// End of function fbe_eses_enclosure_process_eenp_mode_page


/*!*************************************************************************
* @fn fbe_eses_enclosure_process_download_ucode_status(fbe_eses_enclosure_t *eses_enclosure, 
*                                                      fbe_eses_download_status_page_t  * dl_status_page_p)                  
***************************************************************************
* @brief
*   Process the microcode download status bytes from the Download Microcode
*   Status page.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       dl_status_page - The pointer to the returned download status.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008  - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_download_ucode_status(
                                        fbe_eses_enclosure_t *eses_enclosure, 
                                        fbe_eses_download_status_page_t  * dl_status_page_p)
{
    if (eses_enclosure == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Null pointer eses_enclosure\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }
    if (dl_status_page_p == NULL)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, Null pointer dl_status_page_p\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    // for receive diag page must check for enclosure busy page
    if (dl_status_page_p->page_code == SES_PG_CODE_ENCL_BUSY)
    {
        // enclosure is busy
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, Null pointer dl_status_page_p\n", 
                             __FUNCTION__);
        

        return FBE_ENCLOSURE_STATUS_OK;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_INFO,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s dlstatus:0x%x addlstatus:0x%x\n", __FUNCTION__, 
                          dl_status_page_p->dl_status.status, 
                          dl_status_page_p->dl_status.addl_status);

    // the following status is returned from the download 
    // microcode status diagnostic page (receive diag)
    switch(dl_status_page_p->dl_status.status)
    {
    case ESES_DOWNLOAD_STATUS_NONE :            // no download in  progress
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        /* Setting to FBE_ENCLOSURE_FW_STATUS_NONE and FBE_ENCLOSURE_FW_EXT_STATUS_NONE
         * indicate the activate completion.
         * We do not want do it here. Just key off the rev change to decide whether  
         * the activate complates.
         */
        break;
    case ESES_DOWNLOAD_STATUS_IMAGE_IN_USE :    // Complete - subenclosure will attempt to use image
        /*ESES_DOWNLOAD_STATUS_IMAGE_IN_USE was returned for SPS firmware upgrade 
         * before the physical package parses the configuration page to the new rev. 
         * We need to set to IN PROGRESS. Just key off the rev change to decide whether  
         * the activate complates.
         */
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        break;
    case ESES_DOWNLOAD_STATUS_IN_PROGRESS :     // Interim - download in progress
    case ESES_DOWNLOAD_STATUS_UPDATING_FLASH :  // Interim - transfer complete, updating flash
    case ESES_DOWNLOAD_STATUS_UPDATING_NONVOL : // Interim - updating non-vol with image
    case ESES_DOWNLOAD_STATUS_ACTIVATE_IN_PROGRESS: // adding temporary support until expander fw changes
        if (eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL)
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                  FBE_TRACE_LEVEL_INFO,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                  "%s: No Failure Status Returned From Expander!!\n",
                                  __FUNCTION__);
        }
        else
        {
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS;
            // don't update the extended status when in-progress
            //eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        }
        break;
    case ESES_DOWNLOAD_STATUS_NEEDS_ACTIVATE :  // Complete - needs activation or hard reset
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED;
        break;
    case ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD :// Error - page field error
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE;
        break;
    case ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM :  // Error - image checksum error, image discarded
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM;
        break;
    case ESES_DOWNLOAD_STATUS_ERROR_IMAGE :     // Error - internal error, new image required
    case ESES_DOWNLOAD_STATUS_ERROR_BACKUP :    // Error - internal error, backup image available
    case ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED : // Error - activate failed
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_ACTIVATE;
        break;
    case ESES_DOWNLOAD_STATUS_NO_IMAGE :        // Error - activation with no deferred image loaded
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_NO_IMAGE;
        break;
    default :
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_ERROR,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, Unknown ESES_DOWNLOAD_STATUS_ %d\n", 
                            __FUNCTION__, dl_status_page_p->dl_status.status);
        
        break;
    }

    return FBE_ENCLOSURE_STATUS_OK;
} //fbe_eses_enclosure_process_download_ucode_status


/*!*************************************************************************
* @fn fbe_eses_enclosure_parse_buffer_descs(fbe_eses_enclosure_t *eses_enclosure, 
*                                           fbe_enclosure_component_types_t component_type,
*                                           fbe_u8_t component_index,
(                                           ses_buf_desc_struct *bufferDescPtr)
***************************************************************************
* @brief
*   Process the Buffer Descriptor contents.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     component_type - type of Component
* @param     component_index - index to a specific component
* @param     ses_buf_desc_struct - pointer to a Buffer Descriptor.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   05-Jan-2009  - Created.  Joe Perry
*   16-Oct-2009  - Modified - Prasenjeet - return, if buf_index is 0, and store buf_id under
*                           the FBE_ENCL_BD_BUFFER_ID_WRITEABLEattribute, if writeable is set
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_parse_buffer_descs(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_enclosure_component_types_t component_type,
                                      fbe_u8_t component_index,
                                      ses_buf_desc_struct *bufferDescPtr)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
       

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s entry, buf index:%d, buf type:%d \n",
                                        __FUNCTION__,
                                       bufferDescPtr->buf_index,
                                       bufferDescPtr->buf_type);

    //simply return, if buf_index is 0
    //buf_index is 0 is local SXP EEPROM, flare should not modify
     if ((bufferDescPtr->buf_index == 0) && (bufferDescPtr->buf_type  == SES_BUF_TYPE_EEPROM))
    {
      return encl_status;
     }

    /*
     * Process based on Buffer Type
     */
    switch (bufferDescPtr->buf_type)
    {
    case SES_BUF_TYPE_EEPROM:
        /*
         * Save Buffer Descriptor attributes for Resume PROM's
         */
        //For LCC only process the descriptors for LCC read and write.
        //Ignore Mini SAS connector Descriptos for CDES2 
        if((eses_enclosure->eses_version_desc == ESES_REVISION_2_0) &&
           (component_type == FBE_ENCL_LCC) && 
           ((bufferDescPtr->buf_index != SES_BUF_INDEX_LOCAL_LCC_EEPROM) &&
            (bufferDescPtr->buf_index != SES_BUF_INDEX_PEER_LCC_EEPROM)))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, Dont process buf index %d, comp type:%d.\n",
                                               __FUNCTION__,
                                               bufferDescPtr->buf_index, component_type);
            return FBE_ENCLOSURE_STATUS_OK;
        }

        //For Enclosure only process the descriptor for Baseboard read and write.
        //Ignore Mini SAS connector Descriptos        ENCL_CLEANUP
        if((component_type == FBE_ENCL_ENCLOSURE) && 
           (bufferDescPtr->buf_index != SES_BUF_INDEX_BASEBOARD_EEPROM) )
        {
            return FBE_ENCLOSURE_STATUS_OK;
        }



          //store buf_id under the new attribute, if writeable is set.
          if(bufferDescPtr->writable == 1)
          {
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_BUFFER_ID_WRITEABLE,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);      
         }
         else
         {
                 encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_BUFFER_ID,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);
         }              
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_BD_BUFFER_TYPE,
                                                     component_type,
                                                     component_index,
                                                     bufferDescPtr->buf_type);
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_BD_BUFFER_INDEX,
                                                     component_type,
                                                     component_index,
                                                     bufferDescPtr->buf_index);
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        break;
             
    case SES_BUF_TYPE_EVENT_LOG:  
        /*
         * Save Buffer Descriptor attributes for Event Log
         */
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_BD_ELOG_BUFFER_ID,
                                                     component_type,
                                                     component_index,
                                                     bufferDescPtr->buf_id);
            
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        break;
        
    case SES_BUF_TYPE_ACTIVE_TRACE:  
          /*
           * Save Buffer Descriptor attributes for active trace buffer
           */
           encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_ACT_TRC_BUFFER_ID,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);
            
          if(encl_status != FBE_ENCLOSURE_STATUS_OK)
          {
                   return encl_status;
          }
          break;
        
    case SES_BUF_TYPE_ACTIVE_RAM:
        // Active RAM buffer only supported for SPS's
        if (component_type != FBE_ENCL_SPS)
        {
            break;
        }
        //store buf_id under the new attribute, if writeable is set.
        if (bufferDescPtr->buf_type == SES_BUF_TYPE_ACTIVE_RAM)
        {
            if(bufferDescPtr->writable == 1)
            {
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_ACT_RAM_BUFFER_ID_WRITEABLE,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);      
            }
            else
            {
                 encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_ACT_RAM_BUFFER_ID,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);
            }           
        }
        else
        {
            if(bufferDescPtr->writable == 1)
            {
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_BUFFER_ID_WRITEABLE,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);      
            }
            else
            {
                 encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_BD_BUFFER_ID,
                                                             component_type,
                                                             component_index,
                                                             bufferDescPtr->buf_id);
            }           
        }
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_BD_ACT_RAM_BUFFER_ID,
                                                    component_type,
                                                    component_index,
                                                    bufferDescPtr->buf_id);
            
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
        break;
                  
    default:
        break;
    }   // end of switch

    return FBE_ENCLOSURE_STATUS_OK;

}   // end of fbe_eses_enclosure_parse_buffer_descs


/*!*************************************************************************
* @fn fbe_eses_enclosure_process_emc_statistics_status_page(
*                                        fbe_eses_enclosure_t * eses_enclosure, 
*                                        ses_pg_emc_encl_stat_struct * emc_specific_status_page_p)               
***************************************************************************
* @brief
*     This function processes the EMC statistics status page.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   emc_statistics_status_page_p - The pointer to the EMC statistics status page.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error found.
*
* NOTES
*
* HISTORY
*   27-Aug-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_process_emc_statistics_status_page(
                                          fbe_eses_enclosure_t * eses_enclosure, 
                                          ses_pg_emc_statistics_struct * emc_statistics_status_page_p)
{
    fbe_enclosure_status_t            encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t                         page_size = 0;
    fbe_u32_t                         byte_offset = 0;
    fbe_enclosure_component_types_t   comp_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u8_t                          comp_index = 0;
    ses_common_pg_hdr_struct        * pg_hdr_p = NULL;
    ses_common_statistics_field_t   * elem_stats_p = NULL;
    fbe_eses_device_slot_stats_t    * slot_elem_statistics_p = NULL;
    
    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__);    

    elem_stats_p = fbe_eses_get_first_elem_stats_p(emc_statistics_status_page_p);
    
    pg_hdr_p = (ses_common_pg_hdr_struct *)emc_statistics_status_page_p;

    FBE_ESES_GET_PAGE_SIZE(page_size, pg_hdr_p);

    for(byte_offset = FBE_ESES_EMC_STATS_CTRL_STAT_FIRST_ELEM_OFFSET; 
                    byte_offset < page_size;)
    {
        encl_status = fbe_eses_enclosure_elem_offset_to_comp_type_comp_index(eses_enclosure,
                                                                        elem_stats_p->elem_offset,
                                                                        &comp_type,
                                                                        &comp_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            //move on to the next elem statistics.            
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "process_emc_statistics_status_page, elem_offset_to_comp_type_comp_index failed, elem_offset %d.\n",
                        elem_stats_p->elem_offset);    
            
            return encl_status;
        }
        else
        {
            switch(comp_type)
            {
                case FBE_ENCL_DRIVE_SLOT:
                   
                    slot_elem_statistics_p = (fbe_eses_device_slot_stats_t *)elem_stats_p;

                    encl_status = fbe_eses_enclosure_process_slot_statistics(eses_enclosure, comp_index, slot_elem_statistics_p);
                        
                    if (!ENCL_STAT_OK(encl_status))
                    {
                        return encl_status;
                    }
                    
                    break;

                default:
                    break;
            }
        }// End of - if(encl_status != FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)

        byte_offset +=(FBE_ESES_EMC_STATS_CTRL_STAT_COMM_FIELD_LENGTH + elem_stats_p->stats_len);
            
        elem_stats_p = fbe_eses_get_next_elem_stats_p(elem_stats_p);
    }// End of - for loop.                 

    return encl_status;
}// End of - fbe_eses_enclosure_process_emc_statistics_status_page

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_slot_statistics(
*                                         fbe_eses_enclosure_t * eses_enclosure, 
*                                         fbe_u8_t slot_index,
*                                         fbe_eses_device_slot_stats_t * slot_elem_statistics_p)            
***************************************************************************
* @brief
*     This function process the element statistics from the EMC statistics status page
*     for the given drive slot.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   slot_index - The drive slot index in edal.
* @param   slot_elem_statistics_p - The pointer to drive element statistics.
*
* @return fbe_enclosure_status_t 
*     FBE_ENCLOSURE_STATUS_OK -- no error.
*     otherwise - error found.
*
* NOTES
*
* HISTORY
*   27-Aug-2009 PHE - Created.
***************************************************************************/
static fbe_enclosure_status_t fbe_eses_enclosure_process_slot_statistics(
                                          fbe_eses_enclosure_t * eses_enclosure, 
                                          fbe_u8_t slot_index,
                                          fbe_eses_device_slot_stats_t * slot_elem_statistics_p)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              power_cycle_pending = FALSE;

    
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                        FBE_ENCL_DRIVE_SLOT,
                                        slot_index,
                                        &power_cycle_pending);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    if(power_cycle_pending)
    {
        encl_status = fbe_eses_enclosure_slot_statistics_update_power_cycle_state(eses_enclosure, 
                                                        slot_index, 
                                                        slot_elem_statistics_p->power_down_count);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }
    }// End of - if(power_cycle_pending)

    return encl_status;

}// End of - fbe_eses_enclosure_process_slot_statistics

/****************************************************************
 * @fn fbe_eses_enclosure_process_unused_connector(fbe_eses_enclosure_t * eses_enclosure,
 *                                    fbe_u8_t conn_id);
 ****************************************************************
 * @brief
 *  This function disable/enable the phys for the specified connector
 *  This function should only be used under the monitor context.
 *
 * @param eses_enclosure - pointer to enclosure.
 * @param conn_id - connector id
 *
 * @return
 *   status : FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED incase of write needs to go out.
 *
 * HISTORY:
 *   03/03/2011:  Created. NCHIU
 *
 ****************************************************************/
static fbe_enclosure_status_t
fbe_eses_enclosure_process_unused_connector(fbe_eses_enclosure_t * eses_enclosure,
                                     fbe_u8_t   connector_id)
{
    fbe_enclosure_status_t ret_status = FBE_ENCLOSURE_STATUS_OK, encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t             is_cable_inserted = TRUE, is_illegal, is_local = FALSE;

    // Sanity check.
    if (connector_id >= fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure))
    {
        ret_status = FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
    }
    else
    {
        ret_status = fbe_eses_enclosure_setup_connector_control(eses_enclosure, connector_id, FBE_TRUE);

        if(ret_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return ret_status;
        }

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

            while (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
            {
                encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_IS_LOCAL,  // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                                    matching_conn_index,     // Component index.
                                                                    &is_local);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    ret_status = encl_status;
                }
                if (is_local == FBE_FALSE)
                {
                    // Only process local side connector.
                    break;
                }
                is_illegal = FALSE;
                // check if cable inserted
                encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        matching_conn_index,     // Component index.
                                                        &is_cable_inserted);
                if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    ret_status = encl_status;
                    break;
                }
                if (is_cable_inserted)
                {
                    is_illegal = TRUE;
                    // record the fault symptom.
                    fbe_base_enclosure_set_enclosure_fault_info((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR,
                                                    FBE_ENCL_FLTSYMPT_ILLEGAL_CABLE_INSERTED,
                                                    connector_id,
                                                    matching_conn_index);
                }

                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_ILLEGAL_CABLE,
                                                            FBE_ENCL_CONNECTOR,
                                                            matching_conn_index,
                                                            is_illegal);
                if(encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "cable inserted in connector ID:%d, index:%d is now %s\n",
                                    connector_id, matching_conn_index, is_illegal? "illegal": "legal");
                }

                if(!ENCL_STAT_OK(encl_status))
                {
                    ret_status = encl_status;
                    break;
                }
                /* Find the next matching connector index */

                matching_conn_index =
                    fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_ID,  //attribute
                                                            FBE_ENCL_CONNECTOR,  // Component type
                                                            matching_conn_index + 1, //starting index with next connector.
                                                            connector_id);
            }// while loop
        }
    }

    if(!ENCL_STAT_OK(ret_status))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "control connector failed, conn_id: %d, ret_status: 0x%x.\n",
                                 connector_id,
                                 ret_status);
    }

    return ret_status;
} // End of - fbe_eses_enclosure_process_unused_connector


/*!*************************************************************************
* @fn fbe_eses_enclosure_get_sas_encl_type_from_config_page(fbe_eses_enclosure_t * eses_enclosure,
*                                                  ses_pg_config_struct *config_page_p)                 
***************************************************************************
* @brief
*   This function processes the configuration page subenclosure descriptors
*   to get the primary subencl id. Then get the sas enclosure type based on the
*   primary subencl id. 
*
* @param  eses_enclosure (Input) - The pointer to the eses enclosure.
* @param  config_page_p (Input) - pointer to the configuration page
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   23-Nov-2013 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_get_sas_encl_type_from_config_page(fbe_eses_enclosure_t * eses_enclosure,
                                                              ses_pg_config_struct *config_page_p)
{
    fbe_u8_t subencl = 0; // Loop through all the subenclosures.  
    fbe_u8_t total_num_subencls = 0;
    ses_subencl_desc_struct * subencl_desc_p = NULL;

    /*********
     * BEGIN
     *********/
    // Get the pointer to the first subencl descriptor.
    subencl_desc_p = &config_page_p->subencl_desc;

    // Get the total number of subenclosures.
    // (Adding 1 for the primary subenclosure).
    total_num_subencls = config_page_p->num_secondary_subencls + 1;

    // Process the subenclosure descriptor list.
    for(subencl = 0; subencl < total_num_subencls; subencl ++)           
    {
        switch(subencl_desc_p->subencl_type) 
        {
            case SES_SUBENCL_TYPE_LCC:
                if (subencl_desc_p->subencl_id == SES_SUBENCL_ID_PRIMARY)
                {
                    if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_VIKING_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_VIKING_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_VIKING_ICM_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_VIKING_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_CAYENNE_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP;
                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_CAYENNE_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                           FBE_ESES_ENCL_CAYENNE_ICM_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                           FBE_ESES_ENCL_CAYENNE_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_NAGA_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_NAGA_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;
                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_NAGA_ICM_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP;

                    }
                    else if(strncmp((char *)subencl_desc_p->subencl_prod_id, 
                               FBE_ESES_ENCL_NAGA_LCC_PRODUCT_ID_STR, 16) == 0)
                    {
                        eses_enclosure->sas_enclosure_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;

                    }
                }
                break;

            default:
                break;
        }// End of - switch(subencl_desc_p->subencl_type) 


        if((eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP) ||
            (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP)||
            (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP) ||
            (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP) ||
           (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP) ||
            (eses_enclosure->sas_enclosure_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP))
        {
            break;
        }
        else
        {
            // Get the pointer to the next subenclosure descriptor.
            subencl_desc_p = fbe_eses_get_next_ses_subencl_desc_p(subencl_desc_p);
        }

    } // End of Processing the subenclosure descriptor list 

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "get_sas_encl_type_from_config_page: %s(%d), pid %s.\n", 
                    fbe_sas_enclosure_print_sasEnclosureType(eses_enclosure->sas_enclosure_type, NULL, NULL),
                    eses_enclosure->sas_enclosure_type,
                    subencl_desc_p->subencl_prod_id);


    return FBE_ENCLOSURE_STATUS_OK;

} // fbe_eses_enclosure_get_sas_encl_type_from_config_page


/*!*************************************************************************
* @fn fbe_eses_enclosure_subencl_desc_get_subencl_slot()                  
***************************************************************************
* @brief
*       This function gets the subenclosure slot based on the subencl text.
*
* @param eses_enclosure (Input) - The pointer to the eses enclosure.
* @param subencl_desc_p (Input) - The pointer to ses_subencl_desc_struct.
* @param slot_p(Output) - the subEnclosure slot.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*  03-Dec-2013 PHE  - created
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_subencl_desc_get_subencl_slot(fbe_eses_enclosure_t *eses_enclosure_p,
                                        ses_subencl_desc_struct *subencl_desc_p,
                                        fbe_u8_t *subencl_slot_p)
{
    fbe_char_t              *subencl_text_p = NULL; 
    fbe_u8_t                *subencl_text_len_p = NULL;
    fbe_enclosure_status_t   encl_status = FBE_ENCLOSURE_STATUS_OK;

    subencl_text_p = fbe_eses_get_ses_subencl_text_p(subencl_desc_p);

    subencl_text_len_p = fbe_eses_get_ses_subencl_text_len_p(subencl_desc_p);

    switch(subencl_desc_p->subencl_type) 
    {
        case SES_SUBENCL_TYPE_PS:  
            encl_status = fbe_eses_enclosure_subencl_text_to_ps_slot(eses_enclosure_p, subencl_text_p, *subencl_text_len_p, subencl_slot_p);
    
            if (encl_status != FBE_ENCLOSURE_STATUS_OK )
            {
                return encl_status;
            }

            break;

        default:
            *subencl_slot_p = subencl_desc_p->side;
            break;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_subencl_text_to_ps_slot()                  
***************************************************************************
* @brief
*       This function gets the ps slot based on the subencl text.
*
* @param eses_enclosure (Input) - The pointer to the eses enclosure.
* @param subencl_text_p (Input) - The pointer to the subencl text.
* @param subencl_text_len (Input) - The length of the subencl text.
* @param ps_slot_p(Output) - the PS slot.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*  03-Dec-2013 PHE  - created
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_subencl_text_to_ps_slot(fbe_eses_enclosure_t * eses_enclosure,
                                                    fbe_char_t * subencl_text_p,
                                                    fbe_u8_t subencl_text_len,
                                                    fbe_u8_t * ps_slot_p) 
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_DEBUG_LOW,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "subencl_text_to_ps_slot: subencl_text %s.\n", 
                subencl_text_p);
    if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A, subencl_text_len) == 0)
    {
        *ps_slot_p = 0;
    }
    else if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B, subencl_text_len) == 0)
    {
        *ps_slot_p = 1;
    }
    else if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A1, subencl_text_len) == 0)
    {
        *ps_slot_p = 0;
    }
    else if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A0, subencl_text_len) == 0)
    {
        *ps_slot_p = 1;
    }
    else if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B1, subencl_text_len) == 0)
    {
        *ps_slot_p = 2;
    }
    else if(strncmp((char *)subencl_text_p, 
                           FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B0, subencl_text_len) == 0)
    {
        *ps_slot_p = 3;
    }
    else
    {   
        *ps_slot_p = 0;

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_ERROR,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "subencl_text_to_ps_slot: subencl_text %s.\n", 
                subencl_text_p);

        return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK; 
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_subencl_save_prod_rev_level()                  
***************************************************************************
* @brief
*       This function saves the subenclosure product revision level as the overall firmware rev
*       for all the MCUs in the subenclosure.
*
* @param eses_enclosure (Input) - The pointer to the eses enclosure.
* @param subencl_text_p (Input) - The pointer to the subencl text.
* @param component_index (Input) - The EDAL component index.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*  03-Dec-2013 PHE  - created
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_subencl_save_prod_rev_level(fbe_eses_enclosure_t * eses_enclosure,
                                               ses_subencl_desc_struct *subencl_desc_p,
                                               fbe_u8_t component_index) 
{
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_edal_fw_info_t              fw_info;
    fbe_enclosure_fw_target_t       fw_target;
    fbe_edal_general_comp_handle_t  generalDataPtr = NULL;
    fbe_edal_status_t               edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                        firmwareRevInEsesFormat[FBE_ESES_FW_REVISION_SIZE + 1] = {0};
    

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "subencl_save_prod_rev_level entry.\n");

    switch(subencl_desc_p->subencl_type) 
    {
        case SES_SUBENCL_TYPE_PS:  
            fw_target = FBE_FW_TARGET_PS;
            break;

        default:
            fw_target = FBE_FW_TARGET_INVALID;
            return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
            break;
    }
   
    fbe_edal_get_fw_target_component_type_attr(fw_target,&fw_comp_type,&fw_comp_attr);

    status = fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure,
                                                           fw_comp_type,
                                                           component_index,
                                                           &generalDataPtr);

    fbe_zero_memory(fw_info.fwRevision,FBE_EDAL_FW_REV_STR_SIZE);
    if (eses_enclosure->eses_version_desc == ESES_REVISION_1_0)
    {
        // CDES1 format needs to be converted
        fbe_eses_enclosure_convert_subencl_prod_rev_level_to_fw_rev(eses_enclosure, 
                                                                subencl_desc_p,
                                                                &firmwareRevInEsesFormat[0]);
        fbe_copy_memory(fw_info.fwRevision,  &firmwareRevInEsesFormat[0], FBE_EDAL_FW_REV_STR_SIZE);
    
    }
    else
    {
        // CDES2 format is string that can be copied directly
        fbe_copy_memory(fw_info.fwRevision,  &subencl_desc_p->subencl_prod_rev_level.digits[0], SES_SUBENCL_REV_LEVEL_DIGITS); 
    }

    fw_info.valid = FBE_TRUE;
    fw_info.downloadable = FBE_TRUE;

    // copy the fw revision
    edalStatus = eses_enclosure_access_setStr(generalDataPtr,
                                               fw_comp_attr,
                                               fw_comp_type,
                                               sizeof(fbe_edal_fw_info_t),
                                               (char *)&fw_info);

    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return FBE_ENCLOSURE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_convert_subencl_prod_rev_level_to_fw_rev()                  
***************************************************************************
* @brief
*       This function converts the subenclosure product revision level to
*       the fw rev.
*
* @param eses_enclosure (Input) - The pointer to the eses enclosure.
* @param subencl_desc_p (Input) - The pointer to ses_subencl_desc_struct.
* @param pFirmwareRev(Output) - the PS slot.
*
* @return  fbe_enclosure_status_t 
*        FBE_ENCLOSURE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*   Please see the ESES spec for the format change.
*
* HISTORY
*  31-Dec-2013 PHE  - created
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_convert_subencl_prod_rev_level_to_fw_rev(fbe_eses_enclosure_t * eses_enclosure,
                                                            ses_subencl_desc_struct *subencl_desc_p,
                                                            fbe_u8_t *pFirmwareRev) 
{
    fbe_u32_t majorNumber = 0;
    fbe_u32_t minorNumber = 0;
    fbe_u32_t debugVersion = 0;
    fbe_u32_t hex = 0;
    fbe_u8_t subencl_prod_rev_level[5] = {0};
    
    fbe_copy_memory(&subencl_prod_rev_level[0], &subencl_desc_p->subencl_prod_rev_level.digits[0], 4);
    hex = fbe_eses_enclosure_convert_string_to_hex((char *)&subencl_prod_rev_level[0]);

    majorNumber = (hex & 0xF800) >> 11;
    minorNumber = (hex & 0x07F0) >> 4;
    debugVersion = (hex & 0x000F);

    pFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE] = '\0';

    if(debugVersion == 0) 
    {
        /* Convert to the string. */
        fbe_sprintf(pFirmwareRev, 
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1, 
                    "%02d%02d",
                    majorNumber,
                    minorNumber);
    }
    else
    {
        /* Convert to the string. */
        fbe_sprintf(pFirmwareRev, 
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1, 
                    "%02d%02d%c",
                    majorNumber,
                    minorNumber,
                    debugVersion -1 + 'a');
    }

    if(*pFirmwareRev == '0') 
    {
        //The majorNumber has the leading 0, replace it with ' '.
        *pFirmwareRev = ' ';
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                "convert_subencl_prod_rev_level_to_fw_rev, prodRevLevel %s, fwRev %s.\n", 
                &subencl_prod_rev_level[0],
                pFirmwareRev);

    return FBE_ENCLOSURE_STATUS_OK;
}

//End of file fbe_eses_enclosure_process_pages.c
