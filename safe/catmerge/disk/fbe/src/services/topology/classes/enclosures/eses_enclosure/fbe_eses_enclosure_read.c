/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_read.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file parses the ESES element status for 
 *  each element type and saves the information to the ESES enclosure 
 *  object. 
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   24-Jul-2008 PHE - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"

void fbe_eses_enclosure_printUpdatedComponent(fbe_eses_enclosure_t *eses_enclosure,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t index);

static fbe_status_t 
fbe_eses_enclosure_update_fw_status_for_removal(fbe_eses_enclosure_t * pEsesEncl, 
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t componentIndex);

static fbe_enclosure_status_t 
fbe_eses_enclosure_debounce_lcc_fault_if_needed(fbe_eses_enclosure_t * pEsesEncl, 
                                      fbe_u32_t lccComponentIndex,
                                      fbe_bool_t *pLccFaulted);

/*currently commented this declaration to write MUT for this function
   redeclared in eses_enclosure_config.h */
   
/*static fbe_enclosure_status_t fbe_eses_enclosure_array_dev_slot_extract_status(
                                              fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);*/

/*static fbe_enclosure_status_t fbe_eses_enclosure_exp_phy_extract_status(
                                              fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);*/

/*static fbe_enclosure_status_t fbe_eses_enclosure_connector_extract_status(
                                              fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

/*currently commented this declaration to write MUT for this function
   redeclared in eses_enclosure_config.h */

/*static fbe_enclosure_status_t fbe_eses_enclosure_ps_extract_status(
                                              fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);*/
                                              
/*static fbe_enclosure_status_t fbe_eses_enclosure_cooling_extract_status(
                                              fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);
*/

/*==================================================================
 | ELEMENT TYPE  |  Type of Element   |   status parsing           |
 |               |                    |   function implemented?    |
 ===================================================================
 | 02h           | Power Supply       |   Yes                      |
 -------------------------------------------------------------------
 | 03h           | Cooling            |   Yes                      | 
 -------------------------------------------------------------------
 | 04h           | Temperature Sensor |   Yes                      |
 -------------------------------------------------------------------
 | 07h           | ESC Electronics    |   Yes                       |
 -------------------------------------------------------------------
 | 0Bh           | UPS                |   No                       |
 -------------------------------------------------------------------
 | 0Ch           | Display            |   Yes                      |
 -------------------------------------------------------------------
 | 0Eh           | Enclosure          |   Yes                      |
 -------------------------------------------------------------------
 | 10h           | Language           |   No                       |
 -------------------------------------------------------------------
 | 17h           | Array Device Slot  |   Yes                      |
 -------------------------------------------------------------------  
 | 18h           | SAS Expander       |   No                       |
 -------------------------------------------------------------------
 | 19h           | SAS Connector      |   Yes                      |
 -------------------------------------------------------------------
 | 81h           | Expander Phy       |   Yes                      |
 ------------------------------------------------------------------*/
/* The status and control handler for drive. */
fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_array_dev_slot_hdlr = {
                                FBE_ENCL_DRIVE_SLOT,
                                fbe_eses_enclosure_array_dev_slot_init_config_info,
                                fbe_eses_enclosure_array_dev_slot_extract_status,
                                fbe_eses_enclosure_array_dev_slot_setup_control,
                                fbe_eses_enclosure_array_dev_slot_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_exp_phy_hdlr = {
                                FBE_ENCL_EXPANDER_PHY,
                                fbe_eses_enclosure_exp_phy_init_config_info,
                                fbe_eses_enclosure_exp_phy_extract_status,
                                fbe_eses_enclosure_exp_phy_setup_control,
                                fbe_eses_enclosure_exp_phy_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_connector_hdlr = {
                                FBE_ENCL_CONNECTOR,
                                fbe_eses_enclosure_connector_init_config_info,
                                fbe_eses_enclosure_connector_extract_status,
                                fbe_eses_enclosure_connector_setup_control,
                                fbe_eses_enclosure_connector_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_ps_hdlr = {
                                FBE_ENCL_POWER_SUPPLY,
                                fbe_eses_enclosure_ps_init_config_info,
                                fbe_eses_enclosure_ps_extract_status,
                                fbe_eses_enclosure_ps_setup_control,
                                fbe_eses_enclosure_ps_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_cooling_hdlr = {
                                FBE_ENCL_COOLING_COMPONENT,
                                fbe_eses_enclosure_cooling_init_config_info,
                                fbe_eses_enclosure_cooling_extract_status,
                                fbe_eses_enclosure_cooling_setup_control,
                                fbe_eses_enclosure_cooling_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_temp_sensor_hdlr = {
                                FBE_ENCL_TEMP_SENSOR,
                                fbe_eses_enclosure_temp_sensor_init_config_info,
                                fbe_eses_enclosure_temp_sensor_extract_status,
                                fbe_eses_enclosure_temp_sensor_setup_control,
                                fbe_eses_enclosure_temp_sensor_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_chassis_hdlr = {
                                FBE_ENCL_ENCLOSURE,
                                fbe_eses_enclosure_chassis_init_config_info,
                                fbe_eses_enclosure_encl_extract_status,
                                fbe_eses_enclosure_encl_setup_control,
                                fbe_eses_enclosure_encl_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_lcc_hdlr = {
                                FBE_ENCL_LCC,
                                fbe_eses_enclosure_lcc_init_config_info,
                                fbe_eses_enclosure_encl_extract_status,
                                fbe_eses_enclosure_encl_setup_control,
                                fbe_eses_enclosure_encl_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_display_hdlr = {
                                FBE_ENCL_DISPLAY,
                                fbe_eses_enclosure_display_init_config_info,
                                fbe_eses_enclosure_display_extract_status,
                                fbe_eses_enclosure_display_setup_control,
                                fbe_eses_enclosure_display_copy_control
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_expander_hdlr = {
                                FBE_ENCL_EXPANDER,
                                fbe_eses_enclosure_expander_init_config_info,
                                fbe_eses_enclosure_expander_extract_status,
                                NULL,
                                NULL
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_sps_hdlr = {
                                FBE_ENCL_SPS,
                                fbe_eses_enclosure_sps_init_config_info,
                                fbe_eses_enclosure_sps_extract_status,
                                NULL,
                                NULL
                                };

fbe_eses_enclosure_comp_type_hdlr_t fbe_eses_enclosure_ssc_hdlr = {
                                FBE_ENCL_SSC,
                                NULL,
                                fbe_eses_enclosure_esc_elec_extract_status,
                                NULL,
                                NULL
                                };

/* The eses enclsoure component type handler table. */
const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_comp_type_hdlr_table[] = {  
                                &fbe_eses_enclosure_array_dev_slot_hdlr,
                                &fbe_eses_enclosure_exp_phy_hdlr,
                                &fbe_eses_enclosure_connector_hdlr,
                                &fbe_eses_enclosure_ps_hdlr,
                                &fbe_eses_enclosure_cooling_hdlr,
                                &fbe_eses_enclosure_temp_sensor_hdlr,
                                &fbe_eses_enclosure_chassis_hdlr,
                                &fbe_eses_enclosure_lcc_hdlr,
                                &fbe_eses_enclosure_display_hdlr,
                                &fbe_eses_enclosure_expander_hdlr,
                                &fbe_eses_enclosure_sps_hdlr,
                                &fbe_eses_enclosure_ssc_hdlr,
                                NULL};

/* Edal error handler function */
fbe_enclosure_status_t 
fbe_eses_enclosure_handle_edal_error(fbe_eses_enclosure_t *eses_enclosure,
                              fbe_u32_t component,
                              fbe_u32_t index,
                              fbe_u32_t flt_sym,
                              fbe_u32_t addl_status);


char * fbe_edal_print_AdditionalStatus(fbe_u8_t additionalStatus);


/*!*************************************************************************
* @fn fbe_eses_enclosure_array_dev_slot_extract_status(fbe_eses_enclosure_t* eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                  
***************************************************************************
* @brief
*       This function parses the drive status for each individual array device slot 
*       status element in the element group. 
*
*       This function avoids requesting the DRIVE_SLOT_NUMBER when the
*       num_possible_elems is zero. This means that we will not get the status
*       for the overall element which we assume that no one needs. We only process
*       the individual elements.
*
* @param   eses_enclosure - The pointer to the enclosure.
* @param   group_id - The element group id.
* @param   group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_array_dev_slot_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t*           eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_port_number_t               port_number = 0xFF;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;    
    fbe_eses_elem_group_t         * elem_group = NULL;
    fbe_u8_t                        slot_index = 0;
    fbe_u8_t                        slot_num = 0;
    fbe_bool_t                      driveInserted = FALSE;
    fbe_bool_t                      driveFaulted = FALSE;
    fbe_bool_t                      faultLedOn = FALSE;
    fbe_bool_t                      statusValid = FALSE;    
    fbe_bool_t                      driveInsertMasked = FALSE;
    fbe_bool_t                      statusChanged = FALSE;
    ses_stat_elem_array_dev_slot_struct * stat_ptr = NULL;
    fbe_u32_t                       drive_ins_bitmask = 0x00000000;
    fbe_u32_t                       drive_rmv_bitmask = 0x00000000;
    fbe_u32_t                       drive_bit;

    /*********
     * BEGIN
     *********/
    fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)eses_enclosure, &port_number);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s entry \n", __FUNCTION__);

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
  
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    if (num_possible_elems > 0)
    {
        // get the first slot number
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                    FBE_ENCL_DRIVE_SLOT,        // Component type
                                                    0,                       // drive component index
                                                    &slot_num);          // phy component index
    }

    /* Elem 0 is the overall element and the individual element starts from 1. 
     * We are starting at one and thus assume that no caller is interested in 
     * the overall element.
     */
    for (elem = 1 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &slot_index);

        /* fbe_eses_enclosure_get_comp_index() - returns mapping_failed */

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        /* fbe_eses_enclosure_get_comp_index() - returns mapping_failed */
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        } 

        stat_ptr = (ses_stat_elem_array_dev_slot_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
    
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            case SES_STAT_CODE_OK:                
            case SES_STAT_CODE_CRITICAL:     
            case SES_STAT_CODE_UNAVAILABLE:
                driveInserted = TRUE;
                driveFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_UNRECOV:
                driveInserted = TRUE;
                driveFaulted = TRUE;
                statusValid = TRUE; 
                break;

            case SES_STAT_CODE_NOT_INSTALLED:                     
                driveInserted = FALSE;
                driveFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_UNSUPP:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNKNOWN:            
            default:     
                driveInserted = FALSE;
                driveFaulted = TRUE;
                statusValid = FALSE;
                
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, elem: %d, unknown element status code: 0x%x.\n", 
                            __FUNCTION__, elem,
                            stat_ptr->cmn_stat.elem_stat_code);
                
                break;
        }// End of - switch(stat_ptr->cmn_stat.elem_stat_code)

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            statusValid);
                
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
          
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, can't handle edal error. status: 0x%x\n", 
                                     __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        if(!statusValid)
        {
            // Skip this element and switch to the next element of this component.
            continue;
        }

        // Set the drive inserted-ness.  This includes checking whether the Insert bit
        // has been masked off
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_DRIVE_INSERT_MASKED,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        &driveInsertMasked);
        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && driveInsertMasked)
        {
            driveInserted = FALSE;
        }
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            driveInserted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, can't handle edal error. status: 0x%x\n", 
                                __FUNCTION__, encl_error_status); 

            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        }
        if(encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
        {
            statusChanged = TRUE;
            if(driveInserted)
            {
                /* We should reset it everytime to 0x00000001 */
                drive_bit = 0x00000001;

                /* if slot num is 0 and drive is inserted then no need to shift bits
                  * We can directly use drive_bit that is 0x00000001*/  
                if(slot_index != 0)
                {
                    drive_bit = (drive_bit << (slot_index));
                }
                drive_ins_bitmask = (drive_ins_bitmask | drive_bit);
            }
            else
            {
               /* We should reset it everytime to 0x00000001 */
                drive_bit = 0x00000001;

               if(slot_index != 0)
               {
                    drive_bit = (drive_bit << (slot_index));
               }
               drive_rmv_bitmask  = (drive_rmv_bitmask | drive_bit);
            }

            
        }
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            driveFaulted);
        
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, can't handle edal error. status: 0x%x\n", 
                                      __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        /* This function has to be called before updating the FBE_ENCL_COMP_POWERED_OFF attribute in EDAL. */
        encl_status = fbe_eses_enclosure_slot_status_update_power_cycle_state(eses_enclosure,
                                                                              slot_index,
                                                                              stat_ptr->dev_off);
      
        if(!ENCL_STAT_OK(encl_error_status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                  FBE_TRACE_LEVEL_ERROR,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                  "%s, slot_status_update_power_cycle_state failed, encl_status: 0x%x.\n", 
                                  __FUNCTION__, encl_status);

            return encl_status; 
        }
                  
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_POWERED_OFF,
                                                FBE_ENCL_DRIVE_SLOT,
                                                slot_index,
                                                (stat_ptr->dev_off));
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_POWERED_OFF);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, can't handle edal error. status: 0x%x\n", 
                                      __FUNCTION__, encl_error_status);
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }   
      
        
        // process the Drive Fault LED related fields
        if (stat_ptr->fault_requested || stat_ptr->fault_sensed)
        {
            faultLedOn = TRUE;
        }
        else
        {
            faultLedOn = FALSE;
        }
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_FAULT_LED_ON,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        faultLedOn);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULT_LED_ON);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, can't handle edal error. status: 0x%x\n", 
                                      __FUNCTION__, encl_error_status);
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }
        // process the Mark Drive related fields
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_MARKED,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        stat_ptr->ident);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DRIVE_SLOT,
                                                                     slot_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_MARKED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_eses_enclosure_slot_update_user_req_power_ctrl(eses_enclosure, slot_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
           fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                  "slot: %d, slot_extract_status,update_user_req_power_ctrl failed,encl_status:0x%x.\n", 
                                  slot_index, encl_status); 
            
            return encl_status; 
        }

        encl_status = fbe_eses_enclosure_slot_update_device_off_reason(eses_enclosure, slot_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
           fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                  "slot: %d, slot_extract_status,update_device_off_reason failed,encl_status:0x%x.\n", 
                                  slot_index, encl_status); 
            
            return encl_status; 
        }

       /*
        * Check for state change & trace if detected
        */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_DRIVE_SLOT,
                                                 slot_index);


    } // End of - for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    if(statusChanged == TRUE)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "First Disk:%d, drive_ins_bitmask:0x%08x, drive_rmv_bitmask:0x%08x.\n", 
                            (slot_num), drive_ins_bitmask,drive_rmv_bitmask);
    }
    return FBE_ENCLOSURE_STATUS_OK; 
}// End of function fbe_eses_enclosure_array_dev_slot_extract_status

/*!*************************************************************************
* @fn fbe_eses_enclosure_exp_phy_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                  
***************************************************************************
* @brief
*       This function parses the expander phy status for each individual
*       expander phy status element. It also fills in the drive_bypassed
*       status with the status of phy_disabled. 
*
* @param    eses_enclosure - The pointer to the enclosure.
* @param    group_id - The element group id.
* @param    group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_exp_phy_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t*           eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;    
    fbe_eses_elem_group_t         * elem_group = NULL;
    fbe_u8_t                        elem_index = SES_ELEM_INDEX_NONE;
    fbe_u8_t                        phy_index = 0;
    fbe_u8_t                        slot_index = 0;  // The drive slot index.
    fbe_bool_t                      phy_disabled = FALSE;
    fbe_bool_t                      phy_inserted = FALSE;    
    fbe_bool_t                      drv_bypassed = FALSE;
    fbe_bool_t                      statusValid = FALSE;   
    ses_stat_elem_exp_phy_struct  * stat_ptr = NULL;

    /********
     * BEGIN
     ********/

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 

    /*
     * Only process the phys which belongs to the primary subenclosure(i.e. 0).
     */
    if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) != SES_SUBENCL_ID_PRIMARY)
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }

    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Elem 0 is the overall element and the individual element starts from 1. We only care about the individual phy elements. */
    for (elem = 1 ; elem <= num_possible_elems ; elem ++)
    {
        stat_ptr = (ses_stat_elem_exp_phy_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));  

        if(fbe_eses_enclosure_exp_elem_index_phy_id_to_phy_index(eses_enclosure, 
            stat_ptr->exp_index, stat_ptr->phy_id, &phy_index) == FBE_ENCLOSURE_STATUS_OK)
        {   
            elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id) + elem - 1;    
            
            // Set the element index for this phy. 
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ELEM_INDEX,
                                                        FBE_ENCL_EXPANDER_PHY,
                                                        phy_index,
                                                        elem_index);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                        FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_ERROR,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, can't handle edal error. status: 0x%x\n", 
                              __FUNCTION__, encl_error_status);   
                
            }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
                
            // Set if the phy is enabled for this phy. 
            switch(stat_ptr->cmn_stat.elem_stat_code)
            {
                case SES_STAT_CODE_OK:
                case SES_STAT_CODE_NONCRITICAL:
                    phy_inserted = TRUE;
                    phy_disabled = FALSE;             
                    drv_bypassed = FALSE;
                    statusValid = TRUE;
                    break;
                    
                case SES_STAT_CODE_CRITICAL:                    
                case SES_STAT_CODE_UNRECOV:                    
                    phy_inserted = TRUE;
                    phy_disabled = TRUE;              
                    drv_bypassed = TRUE;
                    statusValid = TRUE;
                    break;

                case SES_STAT_CODE_NOT_INSTALLED:
                    phy_inserted = FALSE;
                    phy_disabled = TRUE;
                    drv_bypassed = TRUE;
                    statusValid = TRUE;
                    break;
            
                case SES_STAT_CODE_UNAVAILABLE:
                    phy_inserted = TRUE;
                    phy_disabled = TRUE;
                    drv_bypassed = TRUE;
                    statusValid = TRUE;
                    break;

                case SES_STAT_CODE_UNSUPP: 
                case SES_STAT_CODE_UNKNOWN:
                default:                        
                    phy_inserted = FALSE;
                    phy_disabled = TRUE;              
                    drv_bypassed = TRUE;
                    statusValid = FALSE;
            
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_WARNING,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, unknown element status code: 0x%x.\n", 
                                   __FUNCTION__,  
                                   stat_ptr->cmn_stat.elem_stat_code);

                    
                    break;   
            } // End of - switch(stat_ptr->cmn_stat.elem_stat_code) 

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_EXPANDER_PHY,
                                                            phy_index,
                                                            statusValid);
                
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_STATUS_VALID);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, can't handle edal error. status: 0x%x\n", 
                                   __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_EXPANDER_PHY,
                                                            phy_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
                
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_ADDL_STATUS);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, can't handle edal error. status: 0x%x\n", 
                                   __FUNCTION__,  encl_error_status); 
                                  
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            // Skip this element and switch to the next.
            if(!statusValid)
            {
                continue;
            }
                
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_EXP_PHY_DISABLED,
                                                                FBE_ENCL_EXPANDER_PHY,
                                                                phy_index,
                                                                phy_disabled);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_DISABLED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, can't handle edal error. status: 0x%x\n", 
                                   __FUNCTION__, encl_error_status);  
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }       

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_INSERTED,
                                                                FBE_ENCL_EXPANDER_PHY,
                                                                phy_index,
                                                                phy_inserted);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_INSERTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, can't handle edal error. status: 0x%x\n", 
                                  __FUNCTION__,  encl_error_status);
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            // Check if the phy is connected to any drive slot.
            if(fbe_eses_enclosure_phy_index_to_drive_slot_index(eses_enclosure,
                            phy_index, &slot_index) == FBE_ENCLOSURE_STATUS_OK)
            {
                // This phy is attached to a drive slot.                                         
                // Set the drive bypassed attribute with the value of !phy_enabled.
                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_DRIVE_BYPASSED,
                                                                FBE_ENCL_DRIVE_SLOT,
                                                                slot_index,
                                                                drv_bypassed);
                if (!ENCL_STAT_OK(encl_status))
                {
                    encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                            FBE_ENCL_EXPANDER_PHY,
                                                                            phy_index,
                                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                            FBE_ENCL_DRIVE_BYPASSED);
                    if(!ENCL_STAT_OK(encl_error_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't handle edal error. status: 0x%x\n", 
                                            __FUNCTION__,  encl_error_status);  
                        
                    }
                    // stop processing and return what we originally failed for (encl_stat)
                    return encl_status;   
                }
            }
        
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_EXP_PHY_LINK_READY,
                                                FBE_ENCL_EXPANDER_PHY,
                                                phy_index,
                                                stat_ptr->link_rdy);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_LINK_READY);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);  

                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXP_PHY_READY,
                                                    FBE_ENCL_EXPANDER_PHY,
                                                    phy_index,
                                                    stat_ptr->phy_rdy);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_READY);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);    
                                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_EXP_PHY_FORCE_DISABLED,
                                                        FBE_ENCL_EXPANDER_PHY,
                                                        phy_index,
                                                        stat_ptr->force_disabled);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_FORCE_DISABLED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXP_PHY_SPINUP_ENABLED,
                                                    FBE_ENCL_EXPANDER_PHY,
                                                    phy_index,
                                                    stat_ptr->spinup_enabled);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_SPINUP_ENABLED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }                

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD,
                                                    FBE_ENCL_EXPANDER_PHY,
                                                    phy_index,
                                                    stat_ptr->sata_spinup_hold);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);    
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_EXP_PHY_CARRIER_DETECTED,
                                                    FBE_ENCL_EXPANDER_PHY,
                                                    phy_index,
                                                    stat_ptr->carrier_detect);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_EXPANDER_PHY,
                                                                        phy_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_EXP_PHY_CARRIER_DETECTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);   
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_eses_enclosure_phy_update_disable_reason(eses_enclosure, phy_index);

            if (!ENCL_STAT_OK(encl_status)) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "phy_index: %d, phy_extract_status,update_disable_reason failed,encl_status:0x%x.\n", 
                                    phy_index, encl_status); 
                
           
                return encl_status;
            }

           /*
            * Check for state change & trace if detected
            */
            fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                     FBE_ENCL_EXPANDER_PHY,
                                                     phy_index);

        }// End of - valid phy.

    } // End of - for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return FBE_ENCLOSURE_STATUS_OK; 
}// End of function fbe_eses_enclosure_exp_phy_extract_status


/*!*************************************************************************
* @fn fbe_status_t fbe_eses_enclosure_connector_extract_status(
*                                             fbe_eses_enclosure_t * eses_enclosure,
*                                             fbe_u8_t group_id,
*                                             fbe_eses_ctrl_stat_elem_t * group_overall_status)                
***************************************************************************
* @brief
*       This function parses the sas connector status for each individual 
*       sas connector status element in the element group. 
*
* @param      eses_enclosure - The pointer to the enclosure.
* @param      group_id - The element group id.
* @param      group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_connector_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t*           eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;    
    fbe_eses_elem_group_t         * elem_group = NULL;
    fbe_u8_t                        connector_index = 0;
    fbe_bool_t                      connector_disabled_changed = FALSE;
    fbe_bool_t                      connector_insert_changed = FALSE;
    fbe_bool_t                      connectorInsertMasked = FALSE;
    fbe_bool_t                      connectorInserted = FALSE;
    fbe_bool_t                      connectorDegraded = FALSE;    
    fbe_bool_t                      connector_disabled = FALSE;  
    fbe_bool_t                      statusValid = FALSE;
    fbe_bool_t                      connectorFaulted = TRUE;
    ses_stat_elem_sas_conn_struct * stat_ptr = NULL;

    /********
     * BEGIN
     ********/    

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry\n", __FUNCTION__ ); 

    

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 

#if 0 
    /* HACK - PINGHE - need to be removed after peer expander additional status is available from expander firmware. */
    if(fbe_eses_elem_group_get_subencl_id(elem_group, group_id) != SES_SUBENCL_ID_PRIMARY)
    {
        return FBE_ENCLOSURE_STATUS_OK;
    }
#endif

    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Elem 0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &connector_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);
               
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;    
        }  

        stat_ptr = (ses_stat_elem_sas_conn_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
        if(stat_ptr->cmn_stat.swap)    
        {
            /* Set the clear swap bit */     
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_CLEAR_SWAP,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        TRUE);
            if(!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            FBE_ENCL_CONNECTOR,
                                                            connector_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_ENCL_COMP_CLEAR_SWAP);

                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__,  encl_error_status);  
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
            else if(encl_status ==  FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
            {
                /* The value of the swap bit has been changed. Set the condition to 
                 * read the EMC specific page to update the attached sas address
                 * for the connectors.
                 */
                encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                &fbe_eses_enclosure_lifecycle_const,
                                (fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);

                if(!ENCL_STAT_OK(encl_status))            
                {
                    /* Failed to set the emc specific status condition. */
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "Connector_extract_status, can't set EMC specific status unknown condition, status: 0x%x.\n",
                                        encl_status);

                    encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR,
                                                                    connector_index,
                                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                    FBE_ENCL_COMP_CLEAR_SWAP);
                    if(!ENCL_STAT_OK(encl_error_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't handle edal error. status: 0x%x\n", 
                                            __FUNCTION__, encl_error_status);  
                        
                    }
                    // stop processing and return what we originally failed for (encl_stat)
                    return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                }   

                encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_WRITE_DATA,
                                                                FBE_ENCL_CONNECTOR,
                                                                connector_index,
                                                                TRUE);

                if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                FBE_ENCL_CONNECTOR,
                                                                connector_index,
                                                                fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                FBE_ENCL_COMP_WRITE_DATA);

                    if(!ENCL_STAT_OK(encl_error_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't handle edal error. status: 0x%x\n", 
                                            __FUNCTION__, encl_error_status);  
                    }
                    return encl_status;
                }
                                    
                /* Send dignostic command */
                encl_status = fbe_base_enclosure_set_lifecycle_cond(
                                &fbe_eses_enclosure_lifecycle_const,
                                (fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

                if(!ENCL_STAT_OK(encl_status))            
                {
                    /* Failed to set the specific status condition. */
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "connector_extract_status, cannot set the EXPANDER CONTROL condition : 0x%x.\n",
                                        encl_status);
                        
                    encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR,
                                                                    connector_index,
                                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                    FBE_ENCL_COMP_WRITE_DATA);
                    if(!ENCL_STAT_OK(encl_error_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, can't handle edal error. status: 0x%x\n", 
                                            __FUNCTION__, encl_error_status);  
                        
                    }
                    // stop processing and return what we originally failed for (encl_stat)
                    return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
                } 
            }
        }

        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            case SES_STAT_CODE_OK:
                connectorInserted = TRUE;
                connectorFaulted = FALSE;
                connector_disabled = FALSE;                        
                connectorDegraded = FALSE;                        
                statusValid = TRUE;
                break;
                
            case SES_STAT_CODE_CRITICAL: 
                connectorInserted = TRUE;
                connectorFaulted = TRUE;
                connector_disabled = TRUE;                        
                connectorDegraded = FALSE;                        
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_NOT_INSTALLED: 
                connectorInserted = FALSE;
                connectorFaulted = FALSE;
                connector_disabled = FALSE;                        
                connectorDegraded = FALSE;                        
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_NONCRITICAL: 
                connectorInserted = TRUE;
                connector_disabled = FALSE;                        
                connectorDegraded = TRUE;
                connectorFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_UNAVAILABLE:
                connectorInserted = TRUE;
                connector_disabled = FALSE;                        
                connectorDegraded = FALSE;
                connectorFaulted = FALSE;
                statusValid = TRUE;
                break;  

                     
            case SES_STAT_CODE_UNKNOWN:
                connectorInserted = TRUE;
                connector_disabled = TRUE;                        
                connectorDegraded = FALSE;                        
                connectorFaulted = TRUE;
                statusValid = TRUE;
                break;                       

            case SES_STAT_CODE_UNSUPP: 
            case SES_STAT_CODE_UNRECOV: 
            default:  
                connectorInserted = FALSE;
                connector_disabled = TRUE;
                connectorDegraded = FALSE;
                connectorFaulted = FALSE;
                statusValid = FALSE;
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                    __FUNCTION__,  elem, 
                                    stat_ptr->cmn_stat.elem_stat_code);
                
                break;   
        }  

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_STATUS_VALID,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        statusValid);
            
        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ADDL_STATUS,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        stat_ptr->cmn_stat.elem_stat_code);
            
        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status);   
                                  
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        // process the Connector LED related fields
        //----------------------------------------------------------------------------------------------

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_FAULT_LED_ON,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        stat_ptr->fail);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULT_LED_ON);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, can't handle edal error. status: 0x%x\n", 
                                      __FUNCTION__, encl_error_status);
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }
        // process the Mark Drive related fields
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_MARKED,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        stat_ptr->ident);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_MARKED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }
        //-------------------------------------------------------------------------------------------

        // Set the new value for FBE_ENCL_CONNECTOR_DISABLED attribute.
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_DISABLED,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    connector_disabled);

        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_CONNECTOR_DISABLED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        if(encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
        {
            connector_disabled_changed = TRUE;
        }            

        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_INSERT_MASKED,
                                                        FBE_ENCL_CONNECTOR,
                                                        connector_index,
                                                        &connectorInsertMasked);
        if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && connectorInsertMasked)
        {
            connectorInserted = FALSE;
        }
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_INSERTED,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    connectorInserted);

        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        }

        // A cable was inserted so get a refreshed page
        if(encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
        {
            connector_insert_changed = TRUE;
        }            


        /* For newly inserted connector, we need to debounce the degraded status */
        if (connectorDegraded &&   // degraded
            ((encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED) &&  // insert status changed
             connectorInserted))  // currently inserted
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "debounce degraded connector index %d, status code %d.\n",
                                    connector_index, 
                                    stat_ptr->cmn_stat.elem_stat_code);
            connectorDegraded = FALSE;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_DEGRADED,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    connectorDegraded);

        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_CONNECTOR_DEGRADED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);  
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_FAULTED,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    connectorFaulted);
        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status);    
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_CONNECTOR_TYPE,
                                                    FBE_ENCL_CONNECTOR,
                                                    connector_index,
                                                    stat_ptr->conn_type);
        if(!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_CONNECTOR,
                                                                     connector_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_CONNECTOR_TYPE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status);    
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

       /*
        * Check for state change & trace if detected
        */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_CONNECTOR,
                                                 connector_index);

    } // End of - for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    // Check if we need to set the condition to read the emc specific status page again.
    if(connector_disabled_changed || connector_insert_changed)
    {
        /* The value of the connector enabled attribute has been changed.
        * Set the condition to read the EMC specific page to update the attached sas address
        * for the connectors.
        */
        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                        &fbe_eses_enclosure_lifecycle_const,
                        (fbe_base_enclosure_t *)eses_enclosure,
                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);

        if(!ENCL_STAT_OK(encl_status))            
        {
            /* Failed to set the emc specific status condition. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "connector_extract_status, can't set emc specific status unknown condition, status: 0x%x.\n",
                                encl_status);
            

            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            FBE_ENCL_CONNECTOR,
                                                            connector_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }   
    }

    return FBE_ENCLOSURE_STATUS_OK; 
}// End of function fbe_eses_enclosure_connector_extract_status


/*!*************************************************************************
* @fn fbe_eses_enclosure_esc_elec_extract_status(                  
*                    fbe_object_handle_t object_handle, 
*                    fbe_packet_t * packet)                  
***************************************************************************
* @brief
*   Parse the status element to get the SSC status. 
*
* @param      eses_enclosure - The pointer to the enclosure.
* @param      group_id - The element group id.
* @param      group_overall_status - The pointer to the group overall status.
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
* @note
*    Even if right now we only need to get the status for the SSC (System Status Card). 
*    This function could be used for other component type as well if modified properly. 
*    So fbe_eses_enclosure_esc_elec_extract_status is used as the function name (PHE).
*     
* HISTORY
*   17-Dec-2013 GB - Created.
*   13-Jan-2014 PHE - Saved the status code and status valid bit.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_esc_elec_extract_status(void *enclosure,
                                                             fbe_u8_t group_id,
                                                             fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t            *eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;
    fbe_u8_t                        component_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t                      sscInserted = FALSE;
    fbe_bool_t                      sscFaulted = FALSE;
    fbe_bool_t                      statusValid = FALSE; 
    fbe_eses_elem_group_t           * elem_group;
    ses_stat_elem_encl_struct       *stat_ptr = NULL;
   

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry\n", __FUNCTION__); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
    
    // get the number of elelments for this group to use for loop count limit
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    // Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        // Get the component index for this element
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                group_id,
                                                elem,
                                                &component_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and the original failure reason (encl_stat)
            return encl_status;      
        }        
        
        stat_ptr = (ses_stat_elem_encl_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
        
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            case SES_STAT_CODE_OK:
                sscInserted = TRUE;
                sscFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNRECOV:     
                sscInserted = TRUE;
                sscFaulted = TRUE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_NOT_INSTALLED:                     
                sscInserted = FALSE;
                sscFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_UNSUPP:
            case SES_STAT_CODE_UNKNOWN:
            case SES_STAT_CODE_UNAVAILABLE:                
            default:                    
                sscInserted = FALSE;
                sscFaulted = TRUE;
                statusValid = FALSE;

                // Change the trace level to debug low until the expander firmware goes above 2.23.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_DEBUG_LOW,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                         __FUNCTION__, elem,
                                         stat_ptr->cmn_stat.elem_stat_code);

                break;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_SSC,
                                                            component_index,
                                                            statusValid);
                
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_SSC,
                                                            component_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
          
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, can't handle edal error. status: 0x%x\n", 
                                     __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        // copy the insert flag
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                       FBE_ENCL_COMP_INSERTED,
                                                       FBE_ENCL_SSC,
                                                       component_index,
                                                       sscInserted);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, Can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // copy the fault flag
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                       FBE_ENCL_COMP_FAULTED,  // attribute
                                                       FBE_ENCL_SSC,  // component type
                                                       component_index,     // loop index for each
                                                       sscFaulted);         // from the above case statement

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        } 

        // copy the insert flag
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                       FBE_ENCL_COMP_INSERTED,
                                                       FBE_ENCL_SSC,
                                                       component_index,
                                                       sscInserted);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SSC,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, Can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Check for state change & trace if detected
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_SSC,
                                                 component_index);

    }// End of - for (elem = 0 ; elem <= num_possible_elems ; elem ++)
 

    return FBE_ENCLOSURE_STATUS_OK;       
}// End of function fbe_eses_enclosure_esc_elec_extract_status


/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_ps_extract_status(
*                                             fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                
***************************************************************************
* @brief
*       This function parses the power supply status for each individual 
*       power supply status element. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
*   27-Aug-2008 hgd - added code for AC fail
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_ps_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;    
    fbe_eses_elem_group_t         * elem_group = NULL;
    fbe_u8_t                        ps_index = 0;
    fbe_bool_t                      ps_inserted = FALSE;
    fbe_bool_t                      ps_powered_off = FALSE;
    fbe_bool_t                      ps_faulted = FALSE;
    fbe_bool_t                      ps_supported = FALSE;
    fbe_bool_t                      statusValid = FALSE;   
    ses_stat_elem_ps_struct       * stat_ptr = NULL;

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ ); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Elem 0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &ps_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_POWER_SUPPLY,
                                                                     ps_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__, encl_error_status);    
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }  
        
        stat_ptr = (ses_stat_elem_ps_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
     
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            //  hgd note - following code is based on a current ps info
            //  struct The data layout may change when the eses class
            //  enclosure object is defined.
            case SES_STAT_CODE_OK:
                ps_inserted = TRUE;
                ps_faulted = FALSE;
                ps_powered_off = FALSE;                     
                ps_supported = TRUE;
                statusValid = TRUE;      
            break;     

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
                ps_inserted = TRUE;
                ps_faulted = TRUE;
                ps_powered_off = FALSE;                     
                ps_supported = TRUE;
                statusValid = TRUE;
            break;
        
            case SES_STAT_CODE_UNRECOV:             
                ps_inserted = TRUE;
                ps_faulted = TRUE;       
                ps_powered_off = FALSE;                     
                ps_supported = FALSE;
                statusValid = TRUE;
            break;

            case SES_STAT_CODE_NOT_INSTALLED:               
                ps_inserted = FALSE;
                ps_faulted = FALSE;      
                ps_powered_off = TRUE;                     
                ps_supported = FALSE;
                statusValid = TRUE;
            break;          

            case SES_STAT_CODE_UNAVAILABLE:             
                ps_inserted = TRUE;
                ps_faulted = TRUE;       
                ps_powered_off = TRUE;                     
                ps_supported = TRUE;
                statusValid = TRUE;
            break;          

            case SES_STAT_CODE_UNKNOWN:
               ps_inserted = TRUE;
               ps_faulted = TRUE;   
               ps_powered_off = TRUE;                     
               ps_supported = FALSE;
               statusValid = TRUE;
            break;
        
            case SES_STAT_CODE_UNSUPP: 
            default:
               ps_inserted = FALSE;
               ps_faulted = TRUE;   
               ps_powered_off = TRUE;                     
               ps_supported = FALSE;
               statusValid = FALSE;

            
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, elem %d, unknown element status code: 0x%x, groupId %d.\n", 
                                    __FUNCTION__,  
                                    elem,
                                    stat_ptr->cmn_stat.elem_stat_code,
                                    group_id);               
               break;   
           }

           encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_STATUS_VALID,
                                                               FBE_ENCL_POWER_SUPPLY,
                                                               ps_index,
                                                               statusValid);
                    
           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't handle edal error. status: 0x%x\n", 
                                    __FUNCTION__,  encl_error_status);  
                
            }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
           }

           encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_ADDL_STATUS,
                                                               FBE_ENCL_POWER_SUPPLY,
                                                               ps_index,
                                                               stat_ptr->cmn_stat.elem_stat_code);
                    
           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_ADDL_STATUS);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
               return encl_status;
           }

            // Skip this element and switch to the next.
            if(!statusValid)
            {
                continue;
            }

           encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        ps_index,
                                                        ps_inserted);

           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_INSERTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
           }
           else if((encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED) && (!ps_inserted))
           {  
               /* The power supply was removed. 
                * Update the Firmware upgrade status if 
                * there is firmware upgrade going on for this power supply.
                */
               fbe_eses_enclosure_update_fw_status_for_removal(eses_enclosure, FBE_ENCL_POWER_SUPPLY, ps_index);
           }


           encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_FAULTED,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        ps_index,
                                                        ps_faulted);

           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_FAULTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
           }       
                                                        
           encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_POWERED_OFF,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        ps_index,
                                                        ps_powered_off);
        
           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_POWERED_OFF);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
           }        

           encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_PS_SUPPORTED,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        ps_index,
                                                        ps_supported);

           if (!ENCL_STAT_OK(encl_status))
           {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_PS_SUPPORTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
           }
        /*
         *  Note that within this object and for outside clients, the
         *  status should be checked in the following order.
         *  First check the inserted value, if this is FALSE, use this
         *  value and stop.  Do not use other ps attributes.
         *  If inserted is true, then okay to use other values.
         *
         *  And now, this code will break that rule.  AC fail will be 
         *  set to the value returned by the expander.  So, in this
         *  case, the Viper object will be a pass through for AC fail,
         *  with no intelligent interpretation, i.e. no check of inserted.
         *  Other ideas considered are to let AC fail float, i.e. not
         *  set any value, or to always set it to TRUE, if the PS is not
         *  inserted. 
         */
        if ( stat_ptr->ac_fail )
        {
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_PS_AC_FAIL,
                                                    FBE_ENCL_POWER_SUPPLY,
                                                    ps_index,
                                                    TRUE);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_PS_AC_FAIL);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
        }
        else
        {
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_PS_AC_FAIL,
                                                    FBE_ENCL_POWER_SUPPLY,
                                                    ps_index,
                                                    FALSE);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_POWER_SUPPLY,
                                                                        ps_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_PS_AC_FAIL);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
        }

        /*
         * Check for state change & trace if detected
         */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_POWER_SUPPLY,
                                                 ps_index);

    } // End of  -  for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return FBE_ENCLOSURE_STATUS_OK; 
}// End of function fbe_eses_enclosure_ps_extract_status


/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_cooling_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)               
***************************************************************************
* @brief
*       This function parses the cooling status for each individual cooling 
*       status element in the element group. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   15-Sep-2008 hgd - Created.
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_cooling_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                        num_possible_elems = 0;
    fbe_u8_t                        elem = 0;    
    fbe_eses_elem_group_t         * elem_group = NULL;
    fbe_u8_t                        cooling_index = 0;
    fbe_u8_t                        fan_fault_count = 0;
    fbe_bool_t                      cool_inserted = FALSE;
    fbe_bool_t                      cool_faulted = FALSE;
    fbe_bool_t                      cool_power_off = FALSE; 
    fbe_bool_t                      statusValid = FALSE;    
    ses_stat_elem_cooling_struct  * stat_ptr = NULL; 

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s entry\n", __FUNCTION__ ); 
    

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Elem 0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &cooling_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_COOLING_COMPONENT,
                                                                     cooling_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, can't handle edal error. status: 0x%x\n", 
                                     __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        }   
    
        stat_ptr = (ses_stat_elem_cooling_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));         

        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            //  hgd note - following code is based on a current cooling info
            //  struct The data layout may change when the eses class
            //  enclosure object is defined.
            case SES_STAT_CODE_OK:
                cool_inserted = TRUE;
                cool_faulted   = FALSE;
                cool_power_off = FALSE;      
                statusValid = TRUE;      
                break;     

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
                cool_inserted = TRUE;
                cool_faulted   = TRUE;
                cool_power_off = FALSE;      
                statusValid = TRUE;      
                fan_fault_count++;
                break;          
         
            case SES_STAT_CODE_NOT_INSTALLED:               
                cool_inserted = FALSE;
                cool_faulted   = FALSE;
                cool_power_off = TRUE;       
                statusValid = TRUE;      
                break;

            case SES_STAT_CODE_UNKNOWN:
                cool_inserted = TRUE;
                cool_faulted = TRUE;
                cool_power_off = TRUE;       
                statusValid = TRUE;
                break;
            
            case SES_STAT_CODE_UNSUPP:          
            case SES_STAT_CODE_UNRECOV:
            case SES_STAT_CODE_UNAVAILABLE:
            default:
                cool_inserted = FALSE;
                cool_faulted = TRUE;
                cool_power_off = TRUE;       
                statusValid = FALSE;
            
                // Change the trace level to debug low until the terminator provides the support to the overall element.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_DEBUG_LOW,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                     __FUNCTION__,  elem,
                                     stat_ptr->cmn_stat.elem_stat_code);
                        
                break;   
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_STATUS_VALID,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               cooling_index,
                                                               statusValid);
                    
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_COOLING_COMPONENT,
                                                                        cooling_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_STATUS_VALID);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);    
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }

            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_ADDL_STATUS,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               cooling_index,
                                                               stat_ptr->cmn_stat.elem_stat_code);
                        
            if (encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
            {
                      fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                                                         FBE_TRACE_LEVEL_INFO,
                                                                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                                                         "%s, status changed for: CI = %d, ESC: %s\n", 
                                                                                         __FUNCTION__, cooling_index, 
                                                                                         fbe_edal_print_AdditionalStatus(stat_ptr->cmn_stat.elem_stat_code));
            }
 
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_COOLING_COMPONENT,
                                                                        cooling_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_ADDL_STATUS);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }   

            // Skip this element and switch to the next.
            if(!statusValid)
            {
                continue;
            }

            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_COOLING_COMPONENT,
                                                        cooling_index,
                                                        cool_inserted);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_COOLING_COMPONENT,
                                                                        cooling_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_INSERTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);    
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }             
           
            //  hgd - To Do: If status == FBE_STATUS_OK_VALUE_CHANGED, set local
            //  bool - cumlative_change = TRUE;  Repeat as needed below.
            //  FBE_STATUS_OK_VALUE_CHANGED is a new return value, not yet
            //  implemented.
            
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_POWERED_OFF,
                                                        FBE_ENCL_COOLING_COMPONENT,
                                                        cooling_index,
                                                        cool_power_off);
                
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_COOLING_COMPONENT,
                                                                        cooling_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_POWERED_OFF);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
                
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_FAULTED,
                                                        FBE_ENCL_COOLING_COMPONENT,
                                                        cooling_index,
                                                        cool_faulted);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                        FBE_ENCL_COOLING_COMPONENT,
                                                                        cooling_index,
                                                                        fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                        FBE_ENCL_COMP_FAULTED);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);  
                   
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }   
       
        /*
         *  Note that within this object and for outside clients, the
         *  status should be checked in the following order.
         *  First check the inserted value, if this is FALSE, use this
         *  value and stop.  Do not use other cooling attributes.
         *  If inserted is true, then okay to use other values.
         */
        /*
         *  hgd - Todo Check if more than one fan is reporting a fault.  If so, then
         *  set attribute in all fans.
         *
         *  if ( fan_fault_count > 1 )
         *  {
         *      status = eses_enclosure_access_setBool(generalDataPtr,
         */

       /*
        * Check for state change & trace if detected
        */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_COOLING_COMPONENT,
                                                 cooling_index);

    } // End of  - for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return encl_status; 
}// End of function  -  fbe_eses_enclosure_cooling_extract_status()

/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_temp_sensor_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                 
***************************************************************************
* @brief
*       This function parses temp sensor status for the overall temp sensor status element
*       and each individual temp sensor status element in the element group. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   15-Sep-2008 hgd - Created.
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_temp_sensor_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t              encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            num_possible_elems = 0;
    fbe_u8_t                            elem = 0;    
    fbe_eses_elem_group_t             * elem_group = NULL;
    fbe_u8_t                            temp_sensor_index = 0;
    fbe_u8_t                            previousMaxTemperature;
    fbe_bool_t                          ts_ot_warning = FALSE;
    fbe_bool_t                          ts_ot_failure = FALSE; 
    fbe_bool_t                          ts_inserted = FALSE;
    fbe_bool_t                          ts_powered_off = FALSE; 
    fbe_bool_t                          ts_faulted = FALSE;
    fbe_bool_t                          statusValid = FALSE; 
    ses_stat_elem_temp_sensor_struct  * stat_ptr = NULL;

    /*********
     * BEGIN 
     *********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry\n", __FUNCTION__ ); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &temp_sensor_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        }  
        
        stat_ptr = (ses_stat_elem_temp_sensor_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));

        switch(stat_ptr->cmn_stat.elem_stat_code)
        {            
            case SES_STAT_CODE_OK:
                ts_inserted = TRUE;
                ts_powered_off = FALSE;
                ts_faulted = FALSE;
                statusValid = TRUE; 
                ts_ot_warning = FALSE;
                ts_ot_failure = FALSE;                    
                break;

            case SES_STAT_CODE_CRITICAL:
                ts_inserted = TRUE;
                ts_powered_off = FALSE;
                ts_faulted = FALSE;
                statusValid = TRUE;
                ts_ot_warning = FALSE;
                ts_ot_failure = TRUE;
                break;

            case SES_STAT_CODE_NONCRITICAL:
                ts_inserted = TRUE;
                ts_powered_off = FALSE;
                ts_faulted = FALSE;
                statusValid = TRUE;
                ts_ot_warning = TRUE;
                ts_ot_failure = FALSE;
                break;

            case SES_STAT_CODE_NOT_INSTALLED:  
                ts_inserted = FALSE;
                ts_powered_off = TRUE;
                ts_faulted = TRUE;
                statusValid = TRUE;
                ts_ot_warning = FALSE;
                ts_ot_failure = FALSE;     
                break;

            case SES_STAT_CODE_UNKNOWN:  
                ts_inserted = TRUE;
                ts_powered_off = FALSE; // cant say for sure.
                ts_faulted = TRUE;
                statusValid = TRUE;
                ts_ot_warning = FALSE;
                ts_ot_failure = FALSE;    
                break;

            case SES_STAT_CODE_UNSUPP:
            case SES_STAT_CODE_UNRECOV:     
            case SES_STAT_CODE_UNAVAILABLE:                    
            default:
                // Note: For sensors the codes UNSUPP, UNRECOV, UNAVAILABLE
                // are not defined in the ESES spec. 
                ts_inserted = FALSE;
                ts_powered_off = TRUE;
                ts_faulted = TRUE;
                statusValid = FALSE; 
                ts_ot_warning = FALSE;
                ts_ot_failure = FALSE; 
                 
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_WARNING,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, elem: %d, unknown element status code: 0x%x, groupId %d.\n", 
                                         __FUNCTION__, elem,
                                         stat_ptr->cmn_stat.elem_stat_code,
                                         group_id);
                break;
        }
        
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_TEMP_SENSOR,
                                                            temp_sensor_index,
                                                            statusValid);
                
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "%s, can't handle edal error. status: 0x%x\n", 
                                          __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        //updated FBE_ENCL_COMP_ADDL_STATUS with element status codes.
        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ADDL_STATUS,
                                                    FBE_ENCL_TEMP_SENSOR,
                                                    temp_sensor_index,
                                                    stat_ptr->cmn_stat.elem_stat_code);
                
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }   

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_INSERTED,
                                                FBE_ENCL_TEMP_SENSOR,
                                                temp_sensor_index,
                                                ts_inserted);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }       

    
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_POWERED_OFF,
                                            FBE_ENCL_TEMP_SENSOR,
                                            temp_sensor_index,
                                            ts_powered_off);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_POWERED_OFF);
            if(!ENCL_STAT_OK(encl_error_status))
            {
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);    
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }       

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_FAULTED,
                                            FBE_ENCL_TEMP_SENSOR,
                                            temp_sensor_index,
                                            ts_faulted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
               fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }       
    
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_TEMP_SENSOR_OT_WARNING,
                                            FBE_ENCL_TEMP_SENSOR,
                                            temp_sensor_index,
                                            ts_ot_warning);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_TEMP_SENSOR_OT_WARNING);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);    
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }       

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_TEMP_SENSOR_OT_FAILURE,
                                            FBE_ENCL_TEMP_SENSOR,
                                            temp_sensor_index,
                                            ts_ot_failure);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_TEMP_SENSOR,
                                                                     temp_sensor_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_TEMP_SENSOR_OT_FAILURE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        if(statusValid && ts_inserted && !ts_faulted)
        {
            // check temperature and record max value
            encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE,
                                                        FBE_ENCL_TEMP_SENSOR,
                                                        temp_sensor_index,
                                                        &previousMaxTemperature);
            if(!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                         FBE_ENCL_TEMP_SENSOR,
                                                                         temp_sensor_index,
                                                                         fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                         FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                             "%s, can't handle edal error. status: 0x%x\n", 
                                             __FUNCTION__, encl_error_status); 
                    
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;   
            }
            else if (stat_ptr->temp > previousMaxTemperature)
            {
                encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE,
                                                            FBE_ENCL_TEMP_SENSOR,
                                                            temp_sensor_index,
                                                            stat_ptr->temp);
                if(!ENCL_STAT_OK(encl_status))
                {
                    encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                            FBE_ENCL_TEMP_SENSOR,
                                                                            temp_sensor_index,
                                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                            FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE);
                    if(!ENCL_STAT_OK(encl_error_status))
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                 FBE_TRACE_LEVEL_ERROR,
                                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                 "%s, can't handle edal error. status: 0x%x\n", 
                                                 __FUNCTION__, encl_error_status);    
                    
        
                    }
                    // stop processing and return what we originally failed for (encl_stat)
                    return encl_status;   
                }
            }
        }

       /*
        * Check for state change & trace if detected
        */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_TEMP_SENSOR,
                                                 temp_sensor_index);

    } // End of  - for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return encl_status; 
}// End of function  -  fbe_eses_enclosure_temp_sensor_extract_status()

/*!*************************************************************************
* @fn fbe_eses_enclosure_encl_extract_status(                  
*                    fbe_object_handle_t object_handle, 
*                    fbe_packet_t * packet)                  
***************************************************************************
* @brief
*       This function parses each individual enclosure status element in the element group. 
*       The containing subenclosure for the element group can be the local LCC, 
*       the peer LCC or the chassis.
*
* @param      eses_enclosure - The pointer to the enclosure.
* @param      group_id - The element group id.
* @param      group_overall_status - The pointer to the group overall status.
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   27-Oct-2008 PHE - Created.
*
***************************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_encl_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t              * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t              encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            num_possible_elems = 0;
    fbe_u8_t                            elem = 0;
    fbe_eses_elem_group_t             * elem_group = NULL;
    fbe_enclosure_component_types_t     component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u8_t                            component_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t                          encl_inserted = FALSE;
    fbe_bool_t                          encl_faulted = FALSE;
    fbe_bool_t                          statusValid = FALSE; 
    fbe_bool_t                          check_shutdown_info_needed = FALSE;
    ses_stat_elem_encl_struct         * stat_ptr = NULL;
   

    /********
     * BEGIN
     ********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry\n", __FUNCTION__); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    /* Get the component type of this group. */
    encl_status = fbe_eses_enclosure_get_comp_type(eses_enclosure,
                                                   group_id,                                               
                                                   &component_type);
    if (!ENCL_STAT_OK(encl_status))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "encl_extract_status,get_comp_type failed,elem:%d,encl_status:0x%x.\n", 
                           elem, encl_status); 

        // stop processing
        return encl_status;      
    }

    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                group_id,
                                                elem,
                                                &component_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        }        
        
        stat_ptr = (ses_stat_elem_encl_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
        
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {
            case SES_STAT_CODE_OK:
                encl_inserted = TRUE;
                encl_faulted = FALSE;
                statusValid = TRUE;         
                break;

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNRECOV:     
                encl_inserted = TRUE;
                encl_faulted = TRUE;
                statusValid = TRUE;     
                break;

            case SES_STAT_CODE_NOT_INSTALLED:                     
                encl_inserted = FALSE;
                encl_faulted = FALSE;
                statusValid = TRUE;     
                break;

            case SES_STAT_CODE_UNSUPP:
            case SES_STAT_CODE_UNKNOWN:
            case SES_STAT_CODE_UNAVAILABLE:                
            default:                    
                encl_inserted = FALSE;
                encl_faulted = TRUE;
                statusValid = FALSE;        

                // Change the trace level to debug low until the expander firmware goes above 2.23.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_DEBUG_LOW,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                         __FUNCTION__, elem,
                                         stat_ptr->cmn_stat.elem_stat_code);

                break;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_STATUS_VALID,
                                                               component_type,
                                                               component_index,
                                                               statusValid);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, Can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            component_type,
                                                            component_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
                
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }
           
        // Enclosure inserted
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_INSERTED,  // attribute
                                        component_type,     // component type
                                        component_index,
                                        encl_inserted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }
        else if((component_type == FBE_ENCL_LCC) &&
                (encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED) && 
                (!encl_inserted))
        {  
           /* The LCC was removed. 
            * Update the Firmware upgrade status if 
            * there is firmware upgrade going on for this LCC.
            */
           fbe_eses_enclosure_update_fw_status_for_removal(eses_enclosure, FBE_ENCL_LCC, component_index);
        }

        if(component_type == FBE_ENCL_LCC)
        {
            encl_status = fbe_eses_enclosure_debounce_lcc_fault_if_needed(eses_enclosure, 
                                                        component_index,
                                                        &encl_faulted);

            if(!ENCL_STAT_OK(encl_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, mask_lcc_fault_if_needed failed. encl_status 0x%X.\n", 
                                         __FUNCTION__, encl_status); 
                
            }
        }

        // Enclosure faulted
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_FAULTED,  // attribute
                                            component_type,     // component type
                                            component_index,
                                            encl_faulted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        } 

        // Fault LED on or off
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_FAULT_LED_ON,  // attribute
                                            component_type,     // component type
                                            component_index,
                                            stat_ptr->failure_indicaton);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     component_type,
                                                                     component_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULT_LED_ON);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't handle edal error. status: 0x%x\n", 
                                        __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        } 

        // Fault LED marked
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_MARKED,  // attribute
                                            component_type,     // component type
                                            component_index,
                                            stat_ptr->ident);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            component_type,
                                                            component_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_ENCL_COMP_MARKED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        } 

        if((component_type == FBE_ENCL_ENCLOSURE) && 
            (stat_ptr->cmn_stat.elem_stat_code == SES_STAT_CODE_CRITICAL))
        {
           /* Need to check shutdown info.  
            * If the shutdown was triggered by
            * an EMA-detected fault (which can occur only if DISABLE AUTO SHUTDOWN is not set in the EMC
            * ESES Non-persistent Mode Page, the ELEMENT STATUS CODE field in the Enclosure
            * status element of the Chassis subenclosure in the Enclosure Status diagnostic page 
            * will be set to Critical. 
            */
            check_shutdown_info_needed = TRUE;        
        }

        /*
         * Check for state change & trace if detected
         */
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 component_type,
                                                 component_index);


    }// End of - for (elem = 0 ; elem <= num_possible_elems ; elem ++)
 
    // Check if we need to set the condition to read the emc specific status page again.
    if(check_shutdown_info_needed)
    { 
        /* Set the condition to read the EMC specific page to check the shutdown reason. */
        encl_status = fbe_base_enclosure_set_lifecycle_cond(
                        &fbe_eses_enclosure_lifecycle_const,
                        (fbe_base_enclosure_t *)eses_enclosure,
                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN);

        if(!ENCL_STAT_OK(encl_status))            
        {
            /* Failed to set the mode unsensed condition. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                     "encl_extract_status, can't set emc specific status unknown condition, status: 0x%x.\n",
                                     status);
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            component_type,
                                                            component_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED;
        }   
    }

    return FBE_ENCLOSURE_STATUS_OK;       
}// End of function fbe_eses_enclosure_encl_extract_status

/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_display_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                 
***************************************************************************
* @brief
*       This function parses display status for the overall display status element
*       and each individual display status element in the element group. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   11-Feb-2009 Dipak Patel - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_display_extract_status(void *enclosure,
                                          fbe_u8_t group_id,
                                          fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            num_possible_elems = 0;
    fbe_u8_t                            elem = 0;    
    fbe_eses_elem_group_t             * elem_group = NULL;
    fbe_u8_t                            display_index = 0;
    fbe_bool_t                          display_inserted = FALSE;   
    fbe_bool_t                          display_faulted = FALSE;    
    fbe_u8_t                            displayModeStatus = 0;
    fbe_u8_t                            displayCharacterStatus = 0;
    fbe_u8_t                            displayCharacter = 0;
    fbe_bool_t                          statusValid = FALSE;  
    ses_stat_elem_display_struct      * stat_ptr = NULL;
    fbe_bool_t                          writeNeeded = FALSE;
    fbe_status_t                        status;

    /*********
     * BEGIN 
     *********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry \n", __FUNCTION__ ); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &display_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        } 
         
        stat_ptr = (ses_stat_elem_display_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
 
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {  
            case SES_STAT_CODE_OK:
                display_inserted = TRUE;
                display_faulted = FALSE;
                displayModeStatus = stat_ptr->display_mode_status;
                displayCharacterStatus = stat_ptr->display_char_stat;
                statusValid = TRUE;                
                break;     

            case SES_STAT_CODE_UNSUPP: 
            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNRECOV:
            case SES_STAT_CODE_NOT_INSTALLED:
            case SES_STAT_CODE_UNKNOWN:
            case SES_STAT_CODE_UNAVAILABLE:
            default:
                display_inserted = FALSE;
                display_faulted = TRUE;                     
                statusValid = FALSE;
            
                // Change the trace level to debug low until the expander firmware goes above 2.23.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_DEBUG_LOW,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                         __FUNCTION__, elem,
                                         stat_ptr->cmn_stat.elem_stat_code);
                
                break;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_DISPLAY,
                                                            display_index,
                                                            statusValid);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_DISPLAY,
                                                            display_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_DISPLAY,
                                                        display_index,
                                                        display_inserted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED,
                                                            FBE_ENCL_DISPLAY,
                                                            display_index,
                                                            display_faulted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_DISPLAY_MODE_STATUS,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    displayModeStatus);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_DISPLAY,
                                                                     display_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_DISPLAY_MODE_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_DISPLAY_CHARACTER_STATUS,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    displayCharacterStatus);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                            FBE_ENCL_DISPLAY,
                                                            display_index,
                                                            fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                            FBE_DISPLAY_CHARACTER_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);  
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                      FBE_ENCL_COMP_MARKED,
                                                      FBE_ENCL_DISPLAY,
                                                      display_index,
                                                      stat_ptr->ident);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                FBE_ENCL_DISPLAY,
                                                display_index,
                                                fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                FBE_ENCL_COMP_MARKED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
               
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        }



        // Check if the current Display does not match what we set it to earlier. If so
        // a control page write is required to set the display to the correct value.
        // ARS 390101.

        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_DISPLAY_CHARACTER,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    &displayCharacter);
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                FBE_ENCL_DISPLAY,
                                                display_index,
                                                fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                FBE_DISPLAY_CHARACTER);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
               
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;
        }

        if((displayCharacter != 0) && 
           (displayCharacterStatus != displayCharacter))
        {
            // Current Display value does not match the expected value from EDAL. Force
            // a write control to update the display.

            // This re-writing of the same value to EDAL is just to make sure the next control
            // page write uses the display character value.
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_DISPLAY_CHARACTER,
                                                        FBE_ENCL_DISPLAY,
                                                        display_index,
                                                        displayCharacter);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                    FBE_DISPLAY_CHARACTER);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                              "%s, can't handle edal error. status: 0x%x\n", 
                                             __FUNCTION__, encl_error_status);   
                   
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;
            }

            // Make sure the display mode is set to the correct value to change the display
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_DISPLAY_MODE,
                                                        FBE_ENCL_DISPLAY,
                                                        display_index,
                                                        SES_DISPLAY_MODE_DISPLAY_CHAR);
            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                    FBE_DISPLAY_MODE);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                              "%s, can't handle edal error. status: 0x%x\n", 
                                             __FUNCTION__, encl_error_status);   
                   
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;
            }

            // control page write is needed to update the display with the values in EDAL
            writeNeeded = TRUE;  

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s:Current display:0x%x different from expected value:0x%x,display index:%d\n",
                                    __FUNCTION__,  displayCharacterStatus, displayCharacter, display_index );
        }
        else if((displayCharacter == 0) && 
                (displayCharacterStatus != displayCharacter))
        {
            // The display character in EDAL is NULL and not set to any valid value. So
            // just set it to the value that the current display has. This will be useful
            // in cases where SP reboots and expander display already has correct value set and 
            // so the layers above Physical package won't set the display again. So EDAL will 
            // have NULL internally for  the display character value and wont be able to 
            // set the display correctly if for ex: a soft reset of the expander happens.

            displayCharacter = displayCharacterStatus;
            encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_DISPLAY_CHARACTER,
                                                        FBE_ENCL_DISPLAY,
                                                        display_index,
                                                        displayCharacter);

            if (!ENCL_STAT_OK(encl_status))
            {
                encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                    FBE_ENCL_DISPLAY,
                                                    display_index,
                                                    fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                    FBE_DISPLAY_CHARACTER);
                if(!ENCL_STAT_OK(encl_error_status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                             FBE_TRACE_LEVEL_ERROR,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                              "%s, can't handle edal error. status: 0x%x\n", 
                                             __FUNCTION__, encl_error_status);   
                   
                }
                // stop processing and return what we originally failed for (encl_stat)
                return encl_status;
            }
        }


        // Set condition to write out ESES Control
        if (writeNeeded)
        {
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)eses_enclosure, 
                                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                        __FUNCTION__,  status);
        
            }
        
        }

        // Check for state change & trace if detected
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_DISPLAY,
                                                 display_index);

    } // End of  -  for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return FBE_ENCLOSURE_STATUS_OK;

}// end of fbe_eses_enclosure_display_extract_status

/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_handle_edal_error(
*                                   fbe_eses_enclosure_t * eses_enclosure,
*                                   fbe_enclosure_component_types_t component,
*                                   fbe_u32_t index,
*                                   fbe_u32_t flt_sym,
*                                   fbe_u32_t addl_status)                
***************************************************************************
* @brief
*       This function is called when any edal call fails. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  component - The component type.
* @param  index - Index of element
* @param  flt_sym - fault symptom
* @param  addl_status - The additional status
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   20-Apr-2009 Nayana Chaudhari - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_handle_edal_error(fbe_eses_enclosure_t *eses_enclosure,
                              fbe_u32_t component,
                              fbe_u32_t index,
                              fbe_u32_t flt_sym,
                              fbe_u32_t addl_status)
{
    fbe_enclosure_status_t    encl_status = FBE_ENCLOSURE_STATUS_OK;


    // log the error
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, EDAL failure for component %d, fail the enclosure.\n", 
                             __FUNCTION__, component); 
    
    
    // set faults in base enclosure and also fail enclosure by setting lifecycle condition
    // to FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL

    encl_status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure, 
                                                component,
                                                index,
                                                flt_sym,
                                                addl_status);

    return encl_status;   
} // end of fbe_eses_enclosure_handle_edal_error

/*!*************************************************************************
* @fn void fbe_eses_enclosure_printUpdatedComponent(fbe_eses_enclosure_t *eses_enclosure,
                                                    fbe_enclosure_component_types_t component,
                                                    fbe_u32_t index)
***************************************************************************
* @brief
*       This function is called to determine if there was a change to
*       the specified component & will trace it if changed.
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  component - The component type.
* @param  index - Index of element
*
* @return  void
*
* NOTES
*
* HISTORY
*   15-May-2009 Joe Perry - Created.
*
***************************************************************************/
void fbe_eses_enclosure_printUpdatedComponent(fbe_eses_enclosure_t *eses_enclosure,
                                              fbe_enclosure_component_types_t component,
                                              fbe_u32_t index)
{
    fbe_bool_t              inserted, insertedPriorConfig;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;

    /*
     * If no Config Change in progress, trace component
     */
    if (!eses_enclosure->enclosureConfigBeingUpdated)
    {
        fbe_base_enclosure_edal_printUpdatedComponent((fbe_base_enclosure_t *)eses_enclosure,
                                                      component,
                                                      index);
    }
    else
    {
        /*
         * Config Change in progress, check if the Insert state changed
         */
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_COMP_INSERTED,
                                                     component,
                                                     index,
                                                     &inserted);
        if(!ENCL_STAT_OK(encl_status))
        {
            return;
        }

        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG,
                                                     component,
                                                     index,
                                                     &insertedPriorConfig);
        if(!ENCL_STAT_OK(encl_status))
        {
            return;
        }

        // if the Inserted state changed on the Config Change, trace this component
        if (inserted != insertedPriorConfig)
        {
            fbe_base_enclosure_edal_printUpdatedComponent((fbe_base_enclosure_t *)eses_enclosure,
                                                          component,
                                                          index);
            // set to current Inserted value
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG,
                                                         component,
                                                         index,
                                                         inserted);
            if(!ENCL_STAT_OK(encl_status))
            {
                return;
            }
        }
    }

}   // end of fbe_eses_enclosure_printUpdatedComponent

/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_expander_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                 
***************************************************************************
* @brief
*       This function parses display status for the overall expander status element
*       and each individual expander status element in the element group. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   24-Oct-2009 UDY- Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_expander_extract_status(void *enclosure,
                                          fbe_u8_t group_id,
                                          fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            num_possible_elems = 0;
    fbe_u8_t                            elem = 0;    
    fbe_eses_elem_group_t             * elem_group = NULL;
    fbe_u8_t                            exp_index = 0;
    fbe_bool_t                          exp_inserted = FALSE;   
    fbe_bool_t                          exp_faulted = FALSE;    
    fbe_bool_t                          statusValid = FALSE;  
    ses_stat_elem_sas_exp_struct      * stat_ptr = NULL;

    /*********
     * BEGIN 
     *********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s entry \n", __FUNCTION__ ); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &exp_index);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_EXPANDER,
                                                                     exp_index,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        } 
         
        stat_ptr = (ses_stat_elem_sas_exp_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
 
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {                     
            case SES_STAT_CODE_OK:
                exp_inserted = TRUE;
                exp_faulted = FALSE;                     
                statusValid = TRUE;
            break;

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNRECOV:
                exp_inserted = TRUE;
                exp_faulted = TRUE;                     
                statusValid = TRUE;
            break;            

            case SES_STAT_CODE_NOT_INSTALLED:
                exp_inserted = FALSE;
                exp_faulted = FALSE;                     
                statusValid = TRUE;
            break;
                

            case SES_STAT_CODE_UNSUPP:         
            case SES_STAT_CODE_UNKNOWN:
            case SES_STAT_CODE_UNAVAILABLE:        
                 exp_inserted = FALSE;
                 exp_faulted = TRUE;                     
                 statusValid = FALSE;
                
             // Change the trace level to debug low until the expander firmware goes above 2.23.
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_DEBUG_LOW,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                         __FUNCTION__, elem,
                                         stat_ptr->cmn_stat.elem_stat_code);
                
                break;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_EXPANDER,
                                                            exp_index,
                                                            statusValid);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_EXPANDER,
                                                                     exp_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_EXPANDER,
                                                            exp_index,
                                                            stat_ptr->cmn_stat.elem_stat_code);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_EXPANDER,
                                                                     exp_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_EXPANDER,
                                                        exp_index,
                                                        exp_inserted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_EXPANDER,
                                                                     exp_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED,
                                                            FBE_ENCL_EXPANDER,
                                                            exp_index,
                                                            exp_faulted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_EXPANDER,
                                                                     exp_index,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Check for state change & trace if detected
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_EXPANDER,
                                                 exp_index);

    } // End of  -  for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return FBE_ENCLOSURE_STATUS_OK;

}// end of fbe_eses_enclosure_expander_extract_status

/*!*************************************************************************
* @fn fbe_enclosure_status_t fbe_eses_enclosure_sps_extract_status(
*                                              fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_u8_t group_id,
*                                              fbe_eses_ctrl_stat_elem_t * group_overall_status)                 
***************************************************************************
* @brief
*       This function parses display status for the overall SPS status element
*       and each individual expander status element in the element group. 
*
* @param  eses_enclosure - The pointer to the enclosure.
* @param  group_id - The element group id.
* @param  group_overall_status - The pointer to the group overall status,
*
* @return  fbe_enclosure_status_t.
*       FBE_ENCLOSURE_STATUS_OK - the status was extracted successfully.
*       otherwise - an error found.
*
* NOTES
*
* HISTORY
*   02-Aug-2012     Joe Perry - Created.
*
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_sps_extract_status(void *enclosure,
                                      fbe_u8_t group_id,
                                      fbe_eses_ctrl_stat_elem_t * group_overall_status)
{
    fbe_eses_enclosure_t          * eses_enclosure = (fbe_eses_enclosure_t *) enclosure;
    fbe_enclosure_status_t          encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t          encl_error_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            num_possible_elems = 0;
    fbe_u8_t                            spsIndex = 0;
    fbe_u8_t                            elem = 0;    
    fbe_eses_elem_group_t             * elem_group = NULL;
    fbe_bool_t                          statusValid = FALSE;  
    fbe_bool_t                          spsInserted = FALSE;
    fbe_bool_t                          spsFaulted = FALSE;
    ses_stat_elem_sas_sps_struct      * stat_ptr = NULL;

    /*********
     * BEGIN 
     *********/

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s entry,group_overall_status 0x%llx\n", 
                                       __FUNCTION__, (unsigned long long)group_overall_status); 

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

    /* Loop through all the elements in the group. Elem  0 is the overall element and the individual element starts from 1. */
    for (elem = 0 ; elem <= num_possible_elems ; elem ++)
    {
        /* Get the component index for this element. */
        encl_status = fbe_eses_enclosure_get_comp_index(eses_enclosure,
                                                        group_id,
                                                        elem,
                                                        &spsIndex);

        if(encl_status == FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED)
        {
            continue;
        }
        else if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SPS,
                                                                     spsIndex,
                                                                     FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
                                                                     FBE_ENCL_COMP_BASE);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;      
        } 
         
        stat_ptr = (ses_stat_elem_sas_sps_struct *)(fbe_eses_get_ctrl_stat_elem_p((fbe_u8_t *)group_overall_status, elem));
        switch(stat_ptr->cmn_stat.elem_stat_code)
        {                     
            case SES_STAT_CODE_OK:
                spsInserted = TRUE;
                spsFaulted = FALSE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_NOT_INSTALLED:
                spsInserted = FALSE;
                spsFaulted = TRUE;
                statusValid = TRUE;
                break;

            case SES_STAT_CODE_CRITICAL:
            case SES_STAT_CODE_NONCRITICAL:
            case SES_STAT_CODE_UNRECOV:
            case SES_STAT_CODE_UNSUPP:         
            case SES_STAT_CODE_UNKNOWN:
            case SES_STAT_CODE_UNAVAILABLE:        
                spsInserted = TRUE;
                spsFaulted = TRUE;
                statusValid = TRUE;
                break;

            default:
                spsInserted = FALSE;
                spsFaulted = TRUE;
                statusValid = FALSE;
                
                // Change the trace level to debug low until the expander firmware goes above 2.23.
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                   "%s, elem: %d, unknown element status code: 0x%x.\n", 
                                                   __FUNCTION__, elem,
                                                   stat_ptr->cmn_stat.elem_stat_code);
                break;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_STATUS_VALID,
                                                            FBE_ENCL_SPS,
                                                            spsIndex,
                                                            statusValid);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SPS,
                                                                     spsIndex,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_STATUS_VALID);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__,  encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,
                                                            FBE_ENCL_SPS,
                                                            spsIndex,
                                                            stat_ptr->cmn_stat.elem_stat_code);
                    
        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SPS,
                                                                     spsIndex,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_ADDL_STATUS);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Skip this element and switch to the next.
        if(!statusValid)
        {
            continue;
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_SPS,
                                                        spsIndex,
                                                        spsInserted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SPS,
                                                                     spsIndex,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_INSERTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status); 
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED,
                                                            FBE_ENCL_SPS,
                                                            spsIndex,
                                                            spsFaulted);

        if (!ENCL_STAT_OK(encl_status))
        {
            encl_error_status = fbe_eses_enclosure_handle_edal_error(eses_enclosure,
                                                                     FBE_ENCL_SPS,
                                                                     spsIndex,
                                                                     fbe_base_enclosure_translate_encl_stat_to_flt_symptom(encl_status),
                                                                     FBE_ENCL_COMP_FAULTED);
            if(!ENCL_STAT_OK(encl_error_status))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                         FBE_TRACE_LEVEL_ERROR,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                         "%s, can't handle edal error. status: 0x%x\n", 
                                         __FUNCTION__, encl_error_status);   
                
            }
            // stop processing and return what we originally failed for (encl_stat)
            return encl_status;   
        }

        // Check for state change & trace if detected
        fbe_eses_enclosure_printUpdatedComponent(eses_enclosure,
                                                 FBE_ENCL_SPS,
                                                 spsIndex);

    } // End of  -  for (elem = 0 ; elem <= num_possible_elems ; elem ++)

    return FBE_ENCLOSURE_STATUS_OK;

}// end of fbe_eses_enclosure_sps_extract_status

/*!*************************************************************************
 * @fn fbe_eses_enclosure_update_fw_status_for_removal(fbe_eses_enclosure_t * pEsesEncl, 
 *                                    fbe_enclosure_component_types_t componentType,
 *                                   fbe_u32_t componentIndex)                 
 ***************************************************************************
 * @brief
 *       This function updates the firmware upgrade status when the component was removed.
 *
 * @param  pEsesEncl - The pointer to the enclosure.
 * @param  componentType - 
 * @param  componentIndex -
 *
 * @return  fbe_status_t.
 *       always return FBE_STATUS_OK 
 *
 * @note
 *
 * @version
 *   07-Jul-2010 PHE- Created.
 *
 ***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_update_fw_status_for_removal(fbe_eses_enclosure_t * pEsesEncl, 
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t componentIndex)
{
    fbe_bool_t needToUpdate = FALSE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                         FBE_TRACE_LEVEL_INFO,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                         "EsesEnclUpdateFwStat4Remove, %s %d Removed,enclFupComp %d,enclFupCompSide %d.\n",                                 
                                         enclosure_access_printComponentType(componentType),
                                         componentIndex,
                                         pEsesEncl->enclCurrentFupInfo.enclFupComponent,
                                         pEsesEncl->enclCurrentFupInfo.enclFupComponentSide);   

    switch(componentType)
    {         
        case FBE_ENCL_POWER_SUPPLY:
            if((pEsesEncl->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_PS) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == componentIndex))
            {
                needToUpdate = TRUE;
            }
            break;

        case FBE_ENCL_LCC:
            if((pEsesEncl->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_LCC_MAIN) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == componentIndex))
            {
                needToUpdate = TRUE;
            }
            break;

        default:
            break;
    }

    if(needToUpdate)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                         FBE_TRACE_LEVEL_INFO,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                         "EsesEnclUpdateFwStat4Remove, %s %d Removed, aborting upgrade.\n", 
                                         enclosure_access_printComponentType(componentType),
                                         componentIndex);   
        
        pEsesEncl->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
        pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE; 
        pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 * @fn fbe_eses_enclosure_debounce_lcc_fault(fbe_eses_enclosure_t * pEsesEncl, 
 *                                           fbe_u32_t lccCompIndex,
 *                                           fbe_bool_t *pLccFaulted)               
 ***************************************************************************
 * @brief
 *       This function deboucnes the lcc fault if needed for
 *       a period of FBE_ESES_ENCLOSURE_LCC_FAULT_DEBOUNCE_TIME.
 *
 * @param  pEsesEncl(INPUT)- The pointer to the enclosure.
 * @param  lccCompIndex (OUTPUT)- 
 * @param  pLccFaulted (INPUT/OUTPUT)-
 *
 * @return  fbe_status_t.
 *       FBE_STATUS_OK if successful. Otherwise, return FBE_STATUS_GENERIC_FAILURE.
 *
 * @note
 *
 * @version
 *   15-Jul-2014 PHE- Created.
 *
 ***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_debounce_lcc_fault_if_needed(fbe_eses_enclosure_t * pEsesEncl, 
                                      fbe_u32_t lccCompIndex,
                                      fbe_bool_t *pLccFaulted)
{
    fbe_u32_t                 elapsedTimeInSec = 0;
    fbe_u64_t                 lccFaultStartTimeStamp = 0;
    fbe_bool_t                lccFaultMasked = FBE_FALSE;
    fbe_enclosure_status_t    enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                  lccSideId = 0;

    enclStatus = fbe_base_enclosure_edal_getU64((fbe_base_enclosure_t *)pEsesEncl,
                                                    FBE_LCC_FAULT_START_TIMESTAMP,
                                                    FBE_ENCL_LCC,
                                                    lccCompIndex,
                                                    &lccFaultStartTimeStamp);
    
    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                 "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, can't get lccFaultStartTimeStamp, enclStatus 0x%X.\n", 
                                 lccCompIndex, lccSideId, enclStatus);
        
        return FBE_STATUS_GENERIC_FAILURE; 

    }

    enclStatus = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)pEsesEncl,
                                                FBE_LCC_FAULT_MASKED,
                                                FBE_ENCL_LCC,
                                                lccCompIndex,
                                                &lccFaultMasked);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                 "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, can't get lccFaultMasked, enclStatus 0x%X.\n", 
                                 lccCompIndex, lccSideId, enclStatus);
        
        return FBE_STATUS_GENERIC_FAILURE; 

    }

    enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)pEsesEncl,
                                                FBE_ENCL_COMP_SIDE_ID,
                                                FBE_ENCL_LCC,
                                                lccCompIndex,
                                                &lccSideId);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                 "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, can't get lccSideId, enclStatus 0x%X.\n", 
                                 lccCompIndex, lccSideId, enclStatus);
        
        return FBE_STATUS_GENERIC_FAILURE; 

    }

    if(*pLccFaulted) 
    {
        if (lccFaultStartTimeStamp == 0)
        {
            lccFaultStartTimeStamp = fbe_get_time();
            lccFaultMasked = FBE_TRUE;

            fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                         FBE_TRACE_LEVEL_INFO,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                         "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, ignoring lcc fault the first time.\n",
                                         lccCompIndex, lccSideId);
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(lccFaultStartTimeStamp);
            if(elapsedTimeInSec >= FBE_ESES_ENCLOSURE_LCC_FAULT_DEBOUNCE_TIME) 
            {
                if(lccFaultMasked) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                             FBE_TRACE_LEVEL_INFO,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                             "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, stop ignoring lcc fault.\n",
                                             lccCompIndex, lccSideId);

                    lccFaultMasked = FBE_FALSE;
                }
            }
            else
            {
                lccFaultMasked = FBE_TRUE;

                fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                         FBE_TRACE_LEVEL_INFO,
                                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                         "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, ignoring lcc fault.\n",
                                         lccCompIndex, lccSideId);

            }
        }
    }
    else
    {
        if(lccFaultMasked) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                             FBE_TRACE_LEVEL_INFO,
                                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                             "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, lcc fault cleared.\n",
                                             lccCompIndex, lccSideId);
            lccFaultMasked = FBE_FALSE;
        }

        lccFaultStartTimeStamp = 0;
    }

    *pLccFaulted = (*pLccFaulted) && (!lccFaultMasked);
    
    enclStatus = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)pEsesEncl,
                                                FBE_LCC_FAULT_MASKED,
                                                FBE_ENCL_LCC,
                                                lccCompIndex,
                                                lccFaultMasked);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                 "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, can't set lccFaultMasked, enclStatus 0x%X.\n", 
                                 lccCompIndex, lccSideId, enclStatus);
        
        return enclStatus; 

    }

    enclStatus = fbe_base_enclosure_edal_setU64((fbe_base_enclosure_t *)pEsesEncl,
                                                FBE_LCC_FAULT_START_TIMESTAMP,
                                                FBE_ENCL_LCC,
                                                lccCompIndex,
                                                lccFaultStartTimeStamp);

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                 "DEBOUNCE_LCC_FAULT, lccCompIndex %d, lccSideId %d, can't set lccFaultStartTimeStamp, enclStatus 0x%X.\n", 
                                 lccCompIndex, lccSideId, enclStatus);
        
        return enclStatus; 

    }

    return FBE_ENCLOSURE_STATUS_OK;
}

//End of file
