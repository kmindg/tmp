/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions used by the 
 *  ESES enclosure object.
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   11-Nov-2008 PHE - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_transport_memory.h"
#include "fbe_base_environment_debug.h"
#include "generic_utils_lib.h"

static fbe_status_t fbe_eses_enclosure_fan_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t prom_id, 
                                                     fbe_u32_t *index);

static fbe_status_t fbe_eses_enclosure_fan_side_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t fan_side_id, 
                                                     fbe_u32_t *index);

/*!*************************************************************************
*  @fn fbe_eses_enclosure_get_fw_target_addr(fbe_eses_enclosure_t * eses_enclosure_p,
*                                                        fbe_enclosure_fw_target_t target, 
*                                                        fbe_u8_t side,
*                                                        fbe_eses_encl_fw_component_info_t **fw_info_p)                  
***************************************************************************
* @brief
*   Translate the input parameters into the address of the specified
*   firmware component revision information stored within 
*   fbe_eses_enclosure_t.
*
* @param  eses_enclosure_p (in) - pointr to eses enclosure data
* @param  target (in) -  fbe target enum
* @param  side (in) - eses side
* @param  fw_info_p (out) - pointer to target revision information
*
* @return  fbe_status_t - failure if target not found
* @note Please update fbe_eses_enclosure_init_enclFwInfo for the priority 
*       if this funcition gets changed.
*
* HISTORY
*   09-Oct-208 gb - Created
***************************************************************************/
fbe_status_t fbe_eses_enclosure_get_fw_target_addr(fbe_eses_enclosure_t * eses_enclosure_p,
                                                        fbe_enclosure_fw_target_t target, 
                                                        fbe_u8_t side,
                                                        fbe_eses_encl_fw_component_info_t **fw_info_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    // only the non-error case will set it to some good value.
    *fw_info_p = NULL;

    // get a pointer to the array item location
    switch (target)
    {
    case FBE_FW_TARGET_LCC_EXPANDER:
    case FBE_FW_TARGET_LCC_BOOT_LOADER:
    case FBE_FW_TARGET_LCC_INIT_STRING:
    case FBE_FW_TARGET_LCC_FPGA:
    case FBE_FW_TARGET_LCC_MAIN:
    case FBE_FW_TARGET_SPS_PRIMARY:
    case FBE_FW_TARGET_SPS_SECONDARY:
    case FBE_FW_TARGET_SPS_BATTERY:
          *fw_info_p = &eses_enclosure_p->enclFwInfo->subencl_fw_info[target];
          (*fw_info_p)->enclFupComponent = target;
          (*fw_info_p)->enclFupComponentSide = side;
          status = FBE_STATUS_OK;
        break;
    case FBE_FW_TARGET_COOLING:
        if (side < FBE_ESES_MAX_SUPPORTED_EXTERNAL_COOLING_UNIT)
        {
            *fw_info_p = &eses_enclosure_p->enclFwInfo->subencl_fw_info[target+side];
            (*fw_info_p)->enclFupComponent = target;
            (*fw_info_p)->enclFupComponentSide = side;
            status = FBE_STATUS_OK;
        }
        break;

    case FBE_FW_TARGET_PS:
        if (side < FBE_ESES_MAX_SUPPORTED_PS)
        {
            *fw_info_p = &eses_enclosure_p->enclFwInfo->subencl_fw_info[target+side];
            (*fw_info_p)->enclFupComponent = target;
            (*fw_info_p)->enclFupComponentSide = side;
            status = FBE_STATUS_OK;
        }
        break;

    default:
        // target not translated
        // the status and fw_info_p already set for error case.
        break;
    }
    return status;
} // fbe_eses_enclosure_get_fw_target_addr


/*!*************************************************************************
* @fn fbe_eses_enclosure_is_cmd_retry_needed(fbe_eses_enclosure_t * eses_enclosure,
*                                                 fbe_eses_ctrl_opcode_t opcode,
*                                                 fbe_enclosure_status_t encl_status)                 
***************************************************************************
* @brief
*       This function decides if the command is needed to fail or retry or no action.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   opcode - The ESES control opcode.
* @param   encl_status - The enclosure status.
* 
* @return  fbe_cmd_retry_code_t
*   FBE_CMD_NO_ACTION -- The command does not fail or does not need to be retried.
*   FBE_CMD_RETRY -- The command needs to be retried.
*   FBE_CMD_FAIL -- The command needs to be failed.
*
* NOTES
*
* HISTORY
*   22-Oct-2008 PHE - Created.
*   03-Dec_2008 PHE - Added the input paramter opcode.
*   13-May-2009 NCHAUDHARI - modified to return failed condition
***************************************************************************/
fbe_cmd_retry_code_t fbe_eses_enclosure_is_cmd_retry_needed(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_eses_ctrl_opcode_t opcode,
                                                  fbe_enclosure_status_t encl_status)
{
    fbe_cmd_retry_code_t    cmd_retry_needed = FBE_CMD_NO_ACTION;
    fbe_lifecycle_state_t   lifecycle_state; 
    fbe_status_t            status; 
    fbe_enclosure_status_t  enclosure_status;

    switch(opcode)
    {
        case FBE_ESES_CTRL_OPCODE_MODE_SENSE:
        case FBE_ESES_CTRL_OPCODE_MODE_SELECT:
            switch(encl_status)
            {
                case FBE_ENCLOSURE_STATUS_OK:
                    cmd_retry_needed = FBE_CMD_NO_ACTION;
                    break;
                case FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED:
                case FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED:
                    fbe_base_enclosure_set_enclosure_fault_info(
                                    (fbe_base_enclosure_t *) eses_enclosure,
                                    0,
                                    0,
                                    FBE_ENCL_FLTSYMPT_MODE_CMD_UNSUPPORTED,
                                    opcode);
                    cmd_retry_needed = FBE_CMD_NO_ACTION;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, mode page or encl func unsupported. status: 0x%x cmd status:0x%x\n", 
                                __FUNCTION__, encl_status, cmd_retry_needed); 
                    break;
                case FBE_ENCLOSURE_STATUS_PARAMETER_INVALID:
                case FBE_ENCLOSURE_STATUS_EDAL_FAILED:
                case FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED:
                case FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED:
                    cmd_retry_needed = FBE_CMD_FAIL;
                    break;
                case FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST:
                    if ( opcode == FBE_ESES_CTRL_OPCODE_MODE_SELECT)
                    {
// JAP - temporary fix
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, ModeSelect with Illegal Request - DO NOT KEEP RE-TRYING\n", 
                                    __FUNCTION__); 
                        cmd_retry_needed = FBE_CMD_NO_ACTION;
                        eses_enclosure->sps_dev_supported = 0;
// JAP - temporary fix
                    }
                    else
                    {
                        cmd_retry_needed = FBE_CMD_FAIL;

                    }
                    break;
                default:
                    // retry count has exceeded, we set the fault symptom and 
                    // we do not want to retry anymore.
                    if(eses_enclosure->mode_sense_retry_cnt > 
                        eses_enclosure->properties.number_of_command_retry_max)
                    {
                        // set fault symptom in enclosure fault info
                        fbe_base_enclosure_set_enclosure_fault_info(
                                    (fbe_base_enclosure_t *) eses_enclosure,
                                    0,
                                    0,
                                    FBE_ENCL_FLTSYMPT_MODE_CMD_UNSUPPORTED,
                                    opcode);
                        // set the attribute in edal that mode page is not supported
                        enclosure_status = fbe_base_enclosure_edal_setBool(
                            (fbe_base_enclosure_t *)eses_enclosure,
                            ((opcode == FBE_ESES_CTRL_OPCODE_MODE_SELECT) ? FBE_ENCL_MODE_SELECT_UNSUPPORTED : FBE_ENCL_MODE_SENSE_UNSUPPORTED), // Attribute.
                            FBE_ENCL_ENCLOSURE, // component type.
                            0, // only 1 enclosure
                            TRUE); // The value to be set to.
                        if(!ENCL_STAT_OK(enclosure_status))
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, Can't set FBE_ENCL_MODE_SELECT_UNSUPPORTED. status: 0x%x\n", 
                                __FUNCTION__, enclosure_status); 
                        
                        }
                        cmd_retry_needed = FBE_CMD_NO_ACTION;
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, mode page unsupported as retry count exceeded. status: 0x%x cmd status:0x%x\n", 
                                __FUNCTION__, encl_status, cmd_retry_needed); 
                    }
                    else
                    {
                        cmd_retry_needed = FBE_CMD_RETRY;
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, default case, retry count:0x%x. status: 0x%x cmd status:0x%x\n", 
                                __FUNCTION__, eses_enclosure->mode_sense_retry_cnt, encl_status, cmd_retry_needed); 
                    }
                    break;
            }
            break;
        case FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION:
        case FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE:
        case FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS:
        case FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS: 
        case FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA:
        case FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY:
            // These pages have to be handled successfully so that the enclosure can go to the ready state.
            switch(encl_status)
            {
                case FBE_ENCLOSURE_STATUS_OK:
                case FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED:
                    cmd_retry_needed = FBE_CMD_NO_ACTION;
                    break;
                case FBE_ENCLOSURE_STATUS_PARAMETER_INVALID:
                case FBE_ENCLOSURE_STATUS_EDAL_FAILED:
                case FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED:
                case FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED:
                case FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED:
                    cmd_retry_needed = FBE_CMD_FAIL;
                    break;
                default:
                    cmd_retry_needed = FBE_CMD_RETRY;
                    break;
            }// End of - switch(encl_status)
            break;

        case FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS:
            if ((encl_status == FBE_ENCLOSURE_STATUS_BUSY) ||
                (encl_status == FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED))
            {
                cmd_retry_needed = FBE_CMD_RETRY;
            }
            else
            {
                cmd_retry_needed = FBE_CMD_NO_ACTION;
            }
            break;

        case FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE:
            // Image download retries will be done for:
            //  1) FBE_ENCLOSURE_STATUS_BUSY
            //  2) FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED
            //  3) Not busy and first page and no retries attempted
            //  4) FBE_ENCLOSURE_STATUS_PARAMETER_INVALID and stage of fw download  -- The code does not seem to retry here.
            // For other conditions CM will control retry attempts
            // of the download/upgrade sequence.
            if ((encl_status == FBE_ENCLOSURE_STATUS_BUSY) || (encl_status == FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED))
            {
                cmd_retry_needed = FBE_CMD_RETRY;
            }
            else if ((eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt == 0) &&
                (eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred == 0))
            {
                // retry if not busy status and this is the first page and no retries have occurred
                cmd_retry_needed = FBE_CMD_RETRY;
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, DOWNLOAD_FIRMWARE, 1st page enclstatus:0x%x retry_needed:%d\n", 
                                __FUNCTION__, encl_status, cmd_retry_needed); 
            }
            else
            {
                cmd_retry_needed = FBE_CMD_NO_ACTION;
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, DOWNLOAD_FIRMWARE, enclstatus:0x%x retry_needed:%d\n", 
                                __FUNCTION__, encl_status, cmd_retry_needed); 

            }// End of - else (eses_enclosure->enclCurrentFupInfo)
            break;

        case FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL:
            status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const, 
                (fbe_base_object_t*)eses_enclosure, 
                &lifecycle_state); 
            // We won't retry set_emc_specific_ctrl if enclosure is not ready or we fail  
            // to get lifecycle state. 
            if((lifecycle_state != FBE_LIFECYCLE_STATE_READY) &&
            (status == FBE_STATUS_OK))
            { 
                cmd_retry_needed = FBE_CMD_NO_ACTION;
                eses_enclosure->update_lifecycle_cond |= FBE_ESES_UPDATE_COND_EMC_SPECIFIC_CONTROL_NEEDED;
                break;
            }
            // if lifecycle cond is good(ready), we want to fall back thru the swich of enclosure status,
            // hence don't break here.

        case FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL:
        case FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS:
        default:
            switch(encl_status)
            {
                case FBE_ENCLOSURE_STATUS_OK:
                case FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED:
                case FBE_ENCLOSURE_STATUS_PARAMETER_INVALID:
                case FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED:
                case FBE_ENCLOSURE_STATUS_HARDWARE_ERROR:
                    cmd_retry_needed = FBE_CMD_NO_ACTION;
                    break;
                case FBE_ENCLOSURE_STATUS_EDAL_FAILED:
                case FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED:
                case FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED:
                    cmd_retry_needed = FBE_CMD_FAIL;
                    break;
                default:
                    cmd_retry_needed = FBE_CMD_RETRY;
                    break;
            }// End of - switch(encl_status)
            break;
    }// End of - switch(opcode)

     if(cmd_retry_needed != FBE_CMD_NO_ACTION)
     {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "cmd retry needed, opcode: 0x%x,encl_status:0x%x.\n",
                               opcode, encl_status);
         
     }
     
     return cmd_retry_needed;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_is_time_sync_needed()                  
***************************************************************************
* @brief
*       This function decides if it is the time to sync up the expander time
*       with the system time.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* 
* @return  fbe_bool_t.
*   FBE_TRUE -- The time sync is needed.
*   FBE_FALSE -- The time sync is not needed.
*
* NOTES
*   The EMA may not roll over the day, month or year correctly because the ESES spec
*   states the client can not expect that the EMA knows how many days are in a month 
*   or year. To insure proper synchronization,
*   we need to re-synchronize each time the day changes.
*
* HISTORY
*   17-Dec-2008 PHE - Created.
***************************************************************************/
fbe_bool_t fbe_eses_enclosure_is_time_sync_needed(fbe_eses_enclosure_t * eses_enclosure)
{

    fbe_system_time_t system_time;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;

    fbe_get_system_time(&system_time);

    // Check if this is a new day.
    if(eses_enclosure->day == (fbe_u8_t)system_time.day)
    {
        return FBE_FALSE;
    }
    else
    {
        // Update the day saved in the enclosue object with the day of the system time.
        eses_enclosure->day = (fbe_u8_t)system_time.day;

        /* Set the FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA attribute to TRUE. 
         * The function also sets the FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT to FALSE at the same time.
        * so that the encl time info element in EMC enclosure control page can be written
        * when the day is out of sync. 
        */
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,  // attribute
                                            FBE_ENCL_ENCLOSURE,  // component type
                                            0,  // component index
                                            TRUE);

        if(!ENCL_STAT_OK(encl_status))
        {
            return FBE_FALSE;
        }

        return FBE_TRUE;
    }
}// End of function fbe_eses_enclosure_is_time_sync_needed

/*!*************************************************************************
 *   @fn fbe_eses_enclosure_prom_id_to_buf_id()                  
 ***************************************************************************
 *
 * DESCRIPTION
 *       @brief
 *      This function gets the buffer id for the Resume Prom id.
 *
 * PARAMETERS
 *       @prom_id - Resume Prom id
 *
 * RETURN VALUES
 *       @return fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   10-Feb-2009     AS - Created.
 *   16-Oct-2009   Prasenjeet - Added an argument to indicate a request type.
 *                         For write request type , retrieved the buf_id from the
 *                         FBE_ENCL_BD_BUFFER_ID_WRITEABLE attrbitue.
 ***************************************************************************/
fbe_status_t fbe_eses_enclosure_prom_id_to_buf_id(fbe_eses_enclosure_t *eses_enclosure, 
                                                  fbe_u8_t prom_id, fbe_u8_t *buf_id, fbe_enclosure_resume_prom_req_type_t request_type)
{
    fbe_enclosure_component_types_t component;
    fbe_u32_t                       index = 0;
    fbe_base_enclosure_attributes_t attribute;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                        num_ps_subelements = 0;


    switch (prom_id)
    {
    case FBE_ENCLOSURE_MIDPLANE_RESUME_PROM:
        component = FBE_ENCL_ENCLOSURE;
        index = 0;
        break;

        case FBE_ENCLOSURE_LCC_A_RESUME_PROM:
        case FBE_ENCLOSURE_LCC_B_RESUME_PROM:
        case FBE_ENCLOSURE_EE_LCC_A_RESUME_PROM:
        case FBE_ENCLOSURE_EE_LCC_B_RESUME_PROM:
            status = fbe_eses_enclosure_lcc_prom_id_to_index(eses_enclosure,
                                                              prom_id,
                                                              &index);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, failed to get index for prom_id:%d\n",
                                    __FUNCTION__, prom_id);

                return (FBE_STATUS_GENERIC_FAILURE);
            }

            component = FBE_ENCL_LCC;
            break;

    case FBE_ENCLOSURE_PS_A_RESUME_PROM:
        component = FBE_ENCL_POWER_SUPPLY;
        index = 0;
        break;
        
    case FBE_ENCLOSURE_PS_B_RESUME_PROM:
            // For enclosures with more then 1 PS per side, we need to determine how many 
            // subelements there are (per side) so we can skip them to get to the B side. 
            //
            // For enclosures that don't support more then 1 PS, the number of sub-elements will be 0.
            // In that case we set the num_ps_subelements to 1. This allows the leaf class to ignore
            // the sub-element field. The eses class always inits this field to 0. The leaf class must
            // overwrite it with the correct value.
            //
            if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
            {
                num_ps_subelements = 1;
            }
            component = FBE_ENCL_POWER_SUPPLY;
            index = num_ps_subelements;
            break;
        
        case FBE_ENCLOSURE_FAN_1_RESUME_PROM:
        case FBE_ENCLOSURE_FAN_2_RESUME_PROM:
        case FBE_ENCLOSURE_FAN_3_RESUME_PROM:
            status = fbe_eses_enclosure_fan_prom_id_to_index(eses_enclosure,
                                                            prom_id,
                                                            &index);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, failed to get index for prom_id:%d\n",
                                    __FUNCTION__, prom_id);
                
                return (FBE_STATUS_GENERIC_FAILURE);
            }
            component = FBE_ENCL_COOLING_COMPONENT;
        break;
    default:
        // print error info
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s, Invalid resume prom id:%d\n",
                                __FUNCTION__, prom_id);
        
         return(FBE_STATUS_GENERIC_FAILURE);
        break;
    }

   if(request_type == FBE_ENCLOSURE_WRITE_RESUME_PROM)
   {
            attribute = FBE_ENCL_BD_BUFFER_ID_WRITEABLE;
    }
   else
   {
            attribute = FBE_ENCL_BD_BUFFER_ID;
   }

    if ( FBE_ENCLOSURE_STATUS_OK != fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        attribute,
                                                        component,
                                                        index,
                                                        buf_id))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s, Can't get buffer id, resume prom:%d\n",
                               __FUNCTION__, prom_id);
         
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    return(FBE_STATUS_OK);
}

/*!*************************************************************************
 *   @fn fbe_eses_enclosure_dl_release_buffers()                  
 ***************************************************************************
 * DESCRIPTION
 *  @brief
 *  Release fbe memory from the sg list.
 *
 * PARAMETERS
 * @param eses_enclosure - for tracing purposes
 * @param memory_start_address - start address of the memory chunk to exclude (sg list)
 * @param dl_item_p - pointer to the first sg element to check
 *
 * RETURN VALUES
 *  @void
 *
 * HISTORY
 *   31-Mar-2009    GB - Created.
 ***************************************************************************/
void fbe_eses_enclosure_dl_release_buffers(fbe_eses_enclosure_t * eses_enclosure,
                                           fbe_u8_t *memory_start_address, 
                                           fbe_sg_element_t *dl_item_p)
{
    // null pointer indicates end of list
    for( ; dl_item_p->address != NULL; dl_item_p++)
    {
        // if the address is not within the sg list buffer, release it
        if ((dl_item_p->address < memory_start_address) || 
            (dl_item_p->address >= (memory_start_address + FBE_MEMORY_CHUNK_SIZE)))
        {
            fbe_transport_release_buffer(dl_item_p->address);

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s, Memory released, buffer %64p.\n", 
                                __FUNCTION__,
                                dl_item_p->address);

            dl_item_p->address = NULL;
        }
    }
    return;
} // end fbe_eses_enclosure_dl_release_buffers

/*!**************************************************************
 * @fn fbe_eses_enclosure_print_prvt_data(
 *                         void * enclPrvtData, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print private data of eses
 *  enclosure.
 *
 * @param enclPrvtData - pointer to the void.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_eses_enclosure_print_prvt_data(void * enclPrvtData,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context)
{
  fbe_u32_t port = 0; // for loop.
  
  fbe_eses_enclosure_t * esesPrvtData = (fbe_eses_enclosure_t *)enclPrvtData;

  trace_func(trace_context, "ESES Private Data:\n"); 
  
  fbe_eses_enclosure_print_encl_serial_number(esesPrvtData->encl_serial_number, trace_func, trace_context);
  for(port = 0; port< FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT; port ++) 
  {
      fbe_eses_enclosure_print_primary_port_entire_connector_index(esesPrvtData->primary_port_entire_connector_index[port], trace_func, trace_context);
  }

  fbe_eses_enclosure_print_enclosureConfigBeingUpdated(esesPrvtData->enclosureConfigBeingUpdated, trace_func, trace_context);
  fbe_eses_enclosure_print_outstanding_write(esesPrvtData->outstanding_write, trace_func, trace_context);
  fbe_eses_enclosure_print_emc_encl_ctrl_outstanding_write(esesPrvtData->emc_encl_ctrl_outstanding_write, trace_func, trace_context);
  fbe_eses_enclosure_print_mode_select_needed(esesPrvtData->mode_select_needed, trace_func, trace_context);
  fbe_eses_enclosure_print_test_mode_status(esesPrvtData->test_mode_status, trace_func, trace_context);
  fbe_eses_enclosure_print_test_mode_rqstd(esesPrvtData->test_mode_rqstd, trace_func, trace_context);
  fbe_eses_enclosure_print_fw_activating_status(esesPrvtData->inform_fw_activating,
                                                trace_func, trace_context);
  fbe_eses_enclosure_print_enclosure_type(esesPrvtData->sas_enclosure_type,trace_func, trace_context);

  /* print sas private data */
  fbe_sas_enclosure_print_prvt_data((void *)&(esesPrvtData->sas_enclosure), trace_func, trace_context);
}
/*!*************************************************************************
* @fn fbe_eses_enclosure_first_status_read_completed(fbe_eses_enclosure_t * eses_enclosure,
*                                              fbe_bool_t * first_status_read_completed_p)
***************************************************************************
* @brief
*       This function checkes whether the processing of the first status page has completed or not.
*       since the enclosure is added.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   first_status_read_completed_p - The pointer to the flag which indicates whether
*                          the first status read has completed.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   06-Apr-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_first_status_read_completed(fbe_eses_enclosure_t * eses_enclosure,
                                               fbe_bool_t * first_status_read_completed_p)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u64_t               timeOfLastGoodStatusPage = 0;
    
    encl_status = fbe_base_enclosure_edal_getU64((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE,
                                                    FBE_ENCL_ENCLOSURE,
                                                    0,
                                                    &timeOfLastGoodStatusPage);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }
    
    if(timeOfLastGoodStatusPage == 0)
    {
        *first_status_read_completed_p = FALSE;
    }
    else
    {
        *first_status_read_completed_p = TRUE;
    }

    return encl_status;
}// End of function fbe_eses_enclosure_first_status_read_completed

/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_device_off_reason(
*                      fbe_eses_enclosure_t * eses_enclosure,
*                      fbe_u8_t slot_index)
***************************************************************************
* @brief
*       This function updated the EDAL drive slot device off reason
*       based on the status read.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*   FBE_ENCL_POWER_OFF_REASON_POWER_ON does not necessarily mean 
*   the drive is already powered on.
*   It could be possible that the power on has not happened yet.
*   Other device off reasons do not necessarily mean the drive is already powered off.
*   It could be possible that the power off has not happened yet. 
*   They need to be qualified by the device off status.
*
* HISTORY
*   06-Apr-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_slot_update_device_off_reason(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              slot_powered_off = FALSE;
    fbe_bool_t              drive_inserted = FALSE;
    fbe_u8_t                slot_addl_status = 0;
    fbe_bool_t              first_status_read_completed = FALSE;
    fbe_u8_t                curr_device_off_reason = FBE_ENCL_POWER_OFF_REASON_INVALID;
    fbe_u8_t                device_off_reason = FBE_ENCL_POWER_OFF_REASON_INVALID;
        
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWERED_OFF,
                                        FBE_ENCL_DRIVE_SLOT,
                                        slot_index,
                                        &slot_powered_off);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ADDL_STATUS,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        &slot_addl_status);
            
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status =  fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            &drive_inserted);
    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }
    
    encl_status = fbe_eses_enclosure_first_status_read_completed(eses_enclosure, &first_status_read_completed);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_DRIVE_DEVICE_OFF_REASON,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        &curr_device_off_reason);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    if(slot_powered_off)
    {
        if(slot_addl_status == SES_STAT_CODE_UNRECOV)
        {
            /* The EMA powered off the drive. So far the only reason that 
            * the EMA powers off the drive is overtemp.
            */
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_HW;
        }
        else if(!drive_inserted || !first_status_read_completed) 
        {
            /* The drive is removed OR
            * the processing of the first status page has not completed. 
            * Change the device off reason to FBE_ENCL_POWER_OFF_REASON_NONPERSIST
            * so that the drive could be powered on 
            * when the drive is re-inserted or 
            * when the enclosure is re-connected or
            * when the SP gets rebooted.
            */
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_NONPERSIST;
        }
        else if((curr_device_off_reason == FBE_ENCL_POWER_OFF_REASON_INVALID) ||
                (curr_device_off_reason == FBE_ENCL_POWER_OFF_REASON_POWER_ON))
        {   
            /* The device off could be due to the usurper command from local LCC or peer LCC.
             * Update the reason.
             */
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_PERSIST;
        }
        else
        {
            /* No need to change. */ 
            device_off_reason = curr_device_off_reason;            
        }
    }
    else
    {
        // The slot is powered on.
        if((curr_device_off_reason == FBE_ENCL_POWER_OFF_REASON_POWER_SAVE) || 
           (curr_device_off_reason == FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE))
        {
            /* We don't want to overwrite the reason in case the device off has not taken into effect yet. 
             * It will be updated if the discovery op is received to power on the drive.
             */
            device_off_reason = curr_device_off_reason;
        }
        else
        {
            device_off_reason = FBE_ENCL_POWER_OFF_REASON_POWER_ON;
        }
    }

       
    // Update the device off reason. 
    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_DRIVE_DEVICE_OFF_REASON,  //Attribute
                                        FBE_ENCL_DRIVE_SLOT, // Component type
                                        slot_index, // Component index
                                        device_off_reason);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    return encl_status;
}// End of function fbe_eses_enclosure_slot_update_device_off_reason

/*!*************************************************************************
* @fn fbe_eses_enclosure_phy_update_disable_reason(
*                           fbe_eses_enclosure_t * eses_enclosure,
*                           fbe_u8_t phy_index)
***************************************************************************
* @brief
*       This function updated the EDAL phy disable reason
*       based on the status read.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   phy_index - The phy index in EDAL.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
*
* HISTORY
*   06-Apr-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_phy_update_disable_reason(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t phy_index)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              phy_disabled = FALSE;
    fbe_bool_t              phy_force_disabled = FALSE;
    fbe_u8_t                phy_addl_status = SES_STAT_CODE_OK;
    fbe_bool_t              comp_inserted = TRUE;
    fbe_u8_t                comp_index = 0;
    fbe_bool_t              first_status_read_completed = FALSE;
    fbe_u8_t                curr_phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID;
    fbe_u8_t                phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID;
    fbe_u8_t                connector_id; 
    fbe_bool_t            * connector_disable_list = NULL;
    fbe_bool_t              connector_disable_in_list = FALSE;  // have to init to FALSE.
    fbe_bool_t              comp_insert_masked = FALSE;

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                        FBE_ENCL_EXP_PHY_DISABLED,  //Attribute
                        FBE_ENCL_EXPANDER_PHY, // Component type
                        phy_index, // Component index
                        &phy_disabled);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    // Check if the EMA disabled the phy.
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EXP_PHY_FORCE_DISABLED,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        &phy_force_disabled);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }
    
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_ADDL_STATUS,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        &phy_addl_status);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_eses_enclosure_first_status_read_completed(eses_enclosure, &first_status_read_completed);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EXP_PHY_DISABLE_REASON,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        &curr_phy_disable_reason);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    if(fbe_eses_enclosure_phy_index_to_drive_slot_index(eses_enclosure,
                            phy_index, &comp_index) == FBE_ENCLOSURE_STATUS_OK)
    {
        // This phy is attached to a drive slot.                                         
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        comp_index,
                                                        &comp_inserted);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_DRIVE_INSERT_MASKED,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        comp_index,
                                                        &comp_insert_masked);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

    }
    else if(fbe_eses_enclosure_phy_index_to_connector_index(eses_enclosure,
                            phy_index, &comp_index) == FBE_ENCLOSURE_STATUS_OK)
    {
        // This phy is attached to a connector.                                         
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_INSERTED,
                                                        FBE_ENCL_CONNECTOR,
                                                        comp_index,
                                                        &comp_inserted);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_CONNECTOR_INSERT_MASKED,
                                                        FBE_ENCL_CONNECTOR,
                                                        comp_index,
                                                        &comp_insert_masked);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        connector_disable_list = fbe_eses_enclosure_get_connector_disable_list_ptr(eses_enclosure);

        if(connector_disable_list != NULL)
        {
            encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_ID,
                                                            FBE_ENCL_CONNECTOR,
                                                            comp_index,
                                                            &connector_id);
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }

            connector_disable_in_list = connector_disable_list[connector_id]? TRUE : FALSE;
        }
    }

    if(phy_disabled)
    {
        if(phy_force_disabled ||
            (phy_addl_status == SES_STAT_CODE_CRITICAL) ||
            (phy_addl_status == SES_STAT_CODE_UNRECOV))
        {
            phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_HW; 
        }
        else if(connector_disable_in_list) 
        {
            phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_PERSIST;            
        }
        else if(!first_status_read_completed) 
        {
            /* The processing of the first status page has not completed. 
            * Change the phy disable reason to FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST
            * so that the phy will be enabled 
            * when the drive which the phy is connected to is inserted or
            * when the connector which the phy is connected to is inserted or
            * when the SP gets rebooted.
            */
            phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST;
        }
        else if(!comp_inserted && !comp_insert_masked)
        {
            /* The component(drive or connector) which the phy is connected to is removed.
            * Change the phy disable reason to FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST
            * so that the phy will be enabled when the drive gets inserted or the connector gets connected. 
            */
            phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST; 
        }
        else if((curr_phy_disable_reason == FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID) ||
               (curr_phy_disable_reason == FBE_ESES_ENCL_PHY_DISABLE_REASON_ENABLED))
        {
            /* The phy disable could be due to the usurper command from local LCC, 
             * For example, simulated physical removal.
             */
            phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_PERSIST;
        }
        else
        {
            // No need to change. 
            phy_disable_reason = curr_phy_disable_reason;
        }

        
    }
    else 
    {
        /* The phy is enabled. */
        phy_disable_reason = FBE_ESES_ENCL_PHY_DISABLE_REASON_ENABLED;
    }

    // Update the phy disable reason. 
    encl_status = fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_EXP_PHY_DISABLE_REASON,  //Attribute
                                        FBE_ENCL_EXPANDER_PHY, // Component type
                                        phy_index, // Component index
                                        phy_disable_reason);

    if(encl_status == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_INFO,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "phyIndex %d,disableReason old %d, new %d,compIns %d,InsMasked %d,forceDisable %d,addStatus %d,disabled %d.\n",
                                    phy_index, curr_phy_disable_reason, phy_disable_reason, comp_inserted, comp_insert_masked, phy_force_disabled, phy_addl_status, phy_disabled);

    }

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status; 
    }

    return encl_status;
}// End of function fbe_eses_enclosure_phy_update_disable_reason


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_update_user_req_power_ctrl(
*                           fbe_eses_enclosure_t * eses_enclosure,
*                           fbe_u8_t phy_index)
***************************************************************************
* @brief
*       This function updated the EDAL drive user request power control attribute
*       based on the status read.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*   The EDAL attribute FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL is used to indicate which value will
*   be used for device off bit while building the ENCL control page.
*
*   The reason to have this attribute is because while the drive power cycle is in progress, 
*   we don't want any usurper command such as turning on the drive fault led to the same slot
*   to bring the drive power back because the drive power on command 
*   takes the priority over the power cycle command.
*
* HISTORY
*   20-Oct-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_slot_update_user_req_power_ctrl(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t slot_index)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              slot_powered_off = FALSE;  // read status
    fbe_bool_t              device_off = FALSE; // write value
        
    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWERED_OFF,
                                        FBE_ENCL_DRIVE_SLOT,
                                        slot_index,
                                        &slot_powered_off);

    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_DRIVE_DEVICE_OFF,
                                            FBE_ENCL_DRIVE_SLOT,
                                            slot_index,
                                            &device_off);

    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    /* Check whether the current powered off status is as desired. 
    * If it is already as desired, clear the FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL attribute.
    * If not, no change to the FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL attribute is needed. 
    * The reason to do so is as follows:
    * If a new encl control page needs to be issued from the usurper 
    * such as turning on the drive fault led to the same slot, 
    * we will build the device off bit in the new encl control page 
    * based on the FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL bit.
    * if the FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL attribute is cleared, 
    * the new encl control page will use the current powered off status. 
    * Otherwise, it will use the value of the FBE_ENCL_DRIVE_DEVICE_OFF attribute. 
    */
    if(slot_powered_off == device_off)
    {
        /* The current powered off status is already as desired. 
         * Clear the user request power control bit.
         */
        encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    slot_index,
                                                    FALSE);
    }

    return encl_status;
        
}// End of function fbe_eses_enclosure_slot_update_user_req_power_ctrl


/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_status_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_u8_t new_slot_powered_off)
***************************************************************************
* @brief
*       This function gets called while processing the status page.
*       It updates the EDAL power cycle related attributes
*       based on the new power down count read from the EMC statistics status page.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The drive slot index in edal.
* @param   new_power_down_count- The new new_power_down_count 
*                                         read from the EMC statistics status page.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*   It is a simple way to detect the power cycle completion.
*
* HISTORY
*   01-Sep-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_slot_status_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_bool_t new_slot_powered_off)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t slot_powered_off = FALSE;
    fbe_bool_t power_cycle_pending = FALSE;

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_POWERED_OFF,
                                    FBE_ENCL_DRIVE_SLOT,
                                    slot_index,
                                    &slot_powered_off);   

    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }


    encl_status =  fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                                                FBE_ENCL_DRIVE_SLOT,
                                                                slot_index,
                                                                &power_cycle_pending);

    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    if(power_cycle_pending && 
       slot_powered_off && 
       !new_slot_powered_off)
    {
        encl_status =  fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        FALSE);
        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }

        encl_status =  fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_POWER_CYCLE_COMPLETED,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            TRUE);
        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }     
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_slot_statistics_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_u8_t new_power_down_count)
***************************************************************************
* @brief
*       This function gets called while processing the EMC statistics status page and 
*       the drive power cycle is pending.
*       It updates the EDAL power cycle related attributes
*       based on the new power down count read from the EMC statistics status page.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
* @param   slot_index - The index of the drive slot to update.
* @param   new_power_down_count- The new new_power_down_count 
*                                         read from the EMC statistics status page.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*   This is to catch the power cycle action when we missed the power off state from power on->power off->power on while 
*   reading the status page.
*
* HISTORY
*   01-Sep-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_slot_statistics_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_u8_t new_power_down_count)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                old_power_down_count = 0;     
    fbe_bool_t              write_data_found = FALSE;
    fbe_bool_t              data_sent = FALSE;
    fbe_bool_t              slot_powered_off = FALSE;

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            &write_data_found);

    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
                                                FBE_ENCL_DRIVE_SLOT,
                                                slot_index,
                                                &data_sent);

    if(!ENCL_STAT_OK(encl_status))
    {
        return encl_status;
    }

    if(write_data_found && !data_sent)
    {
       /* The power cycle command has not been issued yet. 
        * Update the drive slot power down count in EDAL 
        * so that it can compared with the power down count after the power cycle command.
        * For each power cycle command from the drive object through the discovery edge,
        * It will come to this if block first because the get EMC statistics status condition
        * is before the EMC specific control needed condition.
        */
        encl_status =  fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_POWER_DOWN_COUNT,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    slot_index,
                                                    new_power_down_count);
        
        return encl_status;
    }
    else
    {
        // The power cycle command has been issued. Check whether the drive slot has powered on. 
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_POWERED_OFF,
                                        FBE_ENCL_DRIVE_SLOT,
                                        slot_index,
                                        &slot_powered_off);

        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }
        
        if(slot_powered_off)
        {
            /* The drive slot has NOT powered on yet. Just return.
             * The purpose of adding this check is to handle the case that 
             * the drive becomes powered off due to the power off command or some other reasons
             * before the power cycle command is sent.
             * If you send a power cycle command to a drive already in the off state, 
             * the power cycle command will be ignored.  
             * The drive will remain off, and the power off count will not be incremented.
             * So the drive will be stuck at the power cycle pending state. 
             * This is what we want.
             */ 
            return encl_status;
        }
        else
        {
            // The drive slot has powered on. Check if there is any power down count change. 
            encl_status =  fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_POWER_DOWN_COUNT,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        &old_power_down_count);

            if(!ENCL_STAT_OK(encl_status))
            {
                return encl_status;
            }

            if(new_power_down_count != old_power_down_count)
            {
               /* Power cycle happened( the drive is powered on and power down count changed.
                * The new_power_down_count could be less than old_power_down_count
                * because the counter wraps when it reaches its maximum value.
                */
                encl_status =  fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                                                    FBE_ENCL_DRIVE_SLOT,
                                                                    slot_index,
                                                                    FALSE);
                if(!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }

                encl_status =  fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_POWER_CYCLE_COMPLETED,
                                                                    FBE_ENCL_DRIVE_SLOT,
                                                                    slot_index,
                                                                    TRUE);
                if(!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }
                encl_status =  fbe_base_enclosure_edal_setU8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_POWER_DOWN_COUNT,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        slot_index,
                                                        new_power_down_count);

                if(!ENCL_STAT_OK(encl_status))
                {
                    return encl_status;
                }
            
            }// End of - if(new_power_down_count != old_power_down_count)
        }
    }    

    return encl_status;
}// End of function fbe_eses_enclosure_slot_statistics_update_power_cycle_state

/*!*************************************************************************
* @fn fbe_eses_enclosure_ride_through_handle_slot_power_cycle(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_u8_t new_power_down_count)
***************************************************************************
* @brief
*       This function gets called while the reset ride through starts. 
*       If the power cycle command for the slot has been sent out 
*       but the power cycle pending and power cycle completed flags 
*       have not been updated before the reset ride through starts,
*       clear the power cycle pending flag so that the drive object 
*       will re-issue the power cycle command.
*
* @param   eses_enclosure - The pointer to the ESES enclosure.
*
* @return  fbe_enclosure_status_t.
*   FBE_ENCLOSURE_STATUS_OK -- no error.
*   otherwise -- error is found.
*
* NOTES
*
* HISTORY
*   01-Sep-2009 PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_ride_through_handle_slot_power_cycle(fbe_eses_enclosure_t * eses_enclosure)                                                  
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              power_cycle_pending = FALSE;
    fbe_u32_t               slot_index = 0;

    for(slot_index = 0 ; slot_index < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); slot_index ++)
    {
        encl_status =  fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                                                        FBE_ENCL_DRIVE_SLOT,
                                                                        slot_index,
                                                                        &power_cycle_pending);

        if(!ENCL_STAT_OK(encl_status))
        {
            return encl_status;
        }

        if(power_cycle_pending) 
        {
            // Set the FBE_ENCL_COMP_POWER_CYCLE_PENDING attribute to FALSE.
            encl_status =  fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_POWER_CYCLE_PENDING,
                                                                    FBE_ENCL_DRIVE_SLOT,
                                                                    slot_index,
                                                                    FALSE);

            if(!ENCL_STAT_OK(encl_status))
            {
                return encl_status;
            }

            // The power cycle will be re-issued by the drive object. So clear the power cycle write related attributes.
            encl_status = fbe_base_enclosure_edal_setBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            slot_index,
                                                            FALSE);

            if(!ENCL_STAT_OK(encl_status))
            {
                return encl_status;
            }
        }
    }// End of - for loop

    return encl_status;
    
}// End of function fbe_eses_enclosure_ride_through_handle_slot_power_cycle

/*!***************************************************************
 * @fn fbe_eses_stats_ses_elem_type_to_component_type
 ****************************************************************
 * @brief
 *
 * @param  ses_type - the ses element type to translate
 *
 * @return  fbe_enclosure_component_types_t
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_component_types_t fbe_eses_stats_ses_elem_type_to_component_type(ses_elem_type_enum ses_type)
{
    fbe_enclosure_component_types_t comp_type;

    switch (ses_type)
    {
    case SES_ELEM_TYPE_PS :
        comp_type = FBE_ENCL_POWER_SUPPLY;
        break;
    case SES_ELEM_TYPE_COOLING :
        comp_type = FBE_ENCL_COOLING_COMPONENT;
        break;
    case SES_ELEM_TYPE_TEMP_SENSOR :
        comp_type = FBE_ENCL_TEMP_SENSOR;
        break;
    case SES_ELEM_TYPE_EXP_PHY :
        comp_type = FBE_ENCL_EXPANDER_PHY;
        break;
    case SES_ELEM_TYPE_ARRAY_DEV_SLOT :
        comp_type = FBE_ENCL_DRIVE_SLOT;
        break;
    case SES_ELEM_TYPE_SAS_EXP :
        comp_type = FBE_ENCL_EXPANDER;
        break;
    case SES_ELEM_TYPE_UPS :
        comp_type = FBE_ENCL_SPS;
        break;
    default:
        comp_type = FBE_ENCL_INVALID_COMPONENT_TYPE;
        break;
    }
    return(comp_type);
} // fbe_eses_stats_ses_elem_type_to_component_type


/*!***************************************************************
 * @fn fbe_eses_enclosure_pop_phy_info
 ****************************************************************
 * @brief
 *  This function finds out if a phy is used as connector phy or
 *  drive slot phy, and then pop the info into the buffer pointed
 *  by common_infop.
 *
 * @param  *eses_enclosure - pointer to the enclosure data (in)
 * @param  phy_component_index - phy component index (in)
 * @param  *common_infop - fbe_enclosure_mgmt_stats_resp_id_info_t (out)
 *
 * @return  void
 *
 * HISTORY:
 *  23-Sep-2011 GB - Created. 
 ****************************************************************/
void fbe_eses_enclosure_pop_phy_info(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_u8_t phy_component_index,
                                      fbe_enclosure_mgmt_stats_resp_id_info_t *common_infop)
{   
    fbe_enclosure_status_t           encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                         phy_id = FBE_ENCLOSURE_VALUE_INVALID;
                        
    /* Get the phy id.*/
    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                  FBE_ENCL_EXP_PHY_ID,  // Attribute
                                                  FBE_ENCL_EXPANDER_PHY,    // Component type
                                                  phy_component_index,
                                                  &phy_id); // output

    if(encl_status == FBE_ENCLOSURE_STATUS_OK)
    {
        common_infop->slot_or_id = phy_id;
    }
    else
    {
        common_infop->slot_or_id = FBE_ENCLOSURE_VALUE_INVALID; 
    }

    // get the slot number
    if (fbe_eses_enclosure_phy_index_to_drive_slot_index(eses_enclosure, 
                     phy_component_index, &common_infop->drv_or_conn_num) == FBE_ENCLOSURE_STATUS_OK)
    {
        common_infop->drv_or_conn = SES_ELEM_TYPE_ARRAY_DEV_SLOT;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "fbe_eses_enclosure_pop_phy_info EXPPHY PhyId:%d, SlotId:%d\n",
                        common_infop->slot_or_id,
                        common_infop->drv_or_conn_num);
    }
    //find the connector number
    else if (fbe_eses_enclosure_phy_index_to_connector_index(eses_enclosure, 
                    phy_component_index, &common_infop->drv_or_conn_num)== FBE_ENCLOSURE_STATUS_OK)
    {
        common_infop->drv_or_conn = SES_ELEM_TYPE_SAS_CONN;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "fbe_eses_enclosure_pop_phy_info EXPPHY PhyId:%d, ConnId:%d\n",
                        common_infop->slot_or_id,
                        common_infop->drv_or_conn_num);
    }
    else
    {
        common_infop->drv_or_conn = SES_ELEM_TYPE_INVALID;
        common_infop->drv_or_conn_num = FBE_ENCLOSURE_VALUE_INVALID;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                        "fbe_eses_enclosure_pop_phy_info EXPPHY PhyId:%d, no slot or conn found\n",
                        common_infop->slot_or_id);
    }
    return;
}// fbe_eses_enclosure_pop_phy_info

/*!***************************************************************
 * @fn fbe_eses_enclosure_get_elem_info
 ****************************************************************
 * @brief
 *  This function can be used to walk through the returned ESES
 *  statistics page to determine what type of (element) data is 
 *  being referenced.
 *
 *  Search the enclosure element groups to find the specified
 *  matching element offset (not element index). Fill in
 *  the fbe_enclosure_mgmt_stats_resp_id_info_t struct and 
 *  set overall_elemnt TRUE when target is not an individual
 *  element.
 *
 * @param  *eses_enclosure - pointer to the enclosure data (in)
 * @param   target_offset - the element offset to find (in)
 * @param  *common_infop - fbe_enclosure_mgmt_stats_resp_id_info_t (out)
 * @param  *overall_element - boolean, indicates overall element type (out)
 *
 * @return  void
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
void fbe_eses_enclosure_get_elem_info(fbe_eses_enclosure_t * eses_enclosure, 
                                fbe_u8_t target_offset,
                                fbe_enclosure_mgmt_stats_resp_id_info_t *common_infop,
                                fbe_bool_t *overall_element)
{
    fbe_u8_t                group_id;
    fbe_u8_t                element_index;
    fbe_u8_t                fbe_comp_type;
    fbe_u8_t                phy_component_index;
    fbe_u8_t                devslot_component_index;
    //fbe_enclosure_status_t  encl_status;
    fbe_eses_elem_group_t   *groupp;

    // offset 0 is associated with the overall element of group 0
    //   group 0 (offset 0) overall elem
    //           (offset 1) individual elem (element index 0)
    //           (offset 2) individual elem (element index 1)
    //           (offset 3) individual elem (element index 2)
    //           (offset 4) individual elem (element index 3)
    //   group 1 (offset 5 ) overall elem
    //           (offset 6) individual elem (element index 4)   = group0(number of elements)
    //           (offset 7) individual elem (element index 5)
    //   group 2 (offset 8) overall elem
    //           (offset 9) individual elem (element index 6)   = group0(number of elements) + group1(number of elements)
    //           (offset 10) individual elem (element index 7)
    //           (offset 11) individual elem (element index 8)
    //

    *overall_element = FALSE;
    common_infop->element_type = SES_ELEM_TYPE_INVALID;
    group_id = fbe_eses_elem_group_get_elem_offset_group_id(eses_enclosure, target_offset, overall_element);
    groupp = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    fbe_base_object_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s TargetOffset:0x%x, GroupId:0x%x, Overall:%d\n", 
                            __FUNCTION__, target_offset, group_id, *overall_element);

    if (group_id != FBE_ENCLOSURE_VALUE_INVALID)
    {
        common_infop->element_type = fbe_eses_elem_group_get_elem_type(groupp, group_id);
        fbe_comp_type = (fbe_u8_t) fbe_eses_stats_ses_elem_type_to_component_type(common_infop->element_type);

        if (*overall_element == FALSE)
        {

            element_index = fbe_eses_elem_group_elem_offset_to_elem_index(groupp, target_offset, group_id);
            if (common_infop->element_type == SES_ELEM_TYPE_EXP_PHY)
            {
                // get the component index
                phy_component_index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ELEM_INDEX,   //attribute
                                                        fbe_comp_type,              // Component type
                                                        0,                          // starting component index  ??
                                                        element_index);             // element index to match


                common_infop->slot_or_id = FBE_ENCLOSURE_VALUE_INVALID;
                if (phy_component_index != FBE_ENCLOSURE_VALUE_INVALID)
                {
                    // get drive slot or connector number for this phy
                    fbe_eses_enclosure_pop_phy_info(eses_enclosure, phy_component_index, common_infop);
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "get_elem_info EXPPHY TargetOffset:0x%x, ElemIndex:%d, PhyId:%d\n",
                                    target_offset,
                                    element_index, 
                                    common_infop->slot_or_id);
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                    "get_elem_info EXPPHY TargetOffset:0x%x, ElemIndex:%d, PhyId cannot be retreived.\n",
                                    target_offset,
                                    element_index);
                }
            }
            else if (common_infop->element_type == SES_ELEM_TYPE_ARRAY_DEV_SLOT)
            { 
                // get the component index
                devslot_component_index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_ELEM_INDEX,   //attribute
                                                        fbe_comp_type,              // Component type
                                                        0,                          // starting component index  ??
                                                        element_index);             // element index to match
                common_infop->slot_or_id = devslot_component_index;
            }
            else
            {
                common_infop->slot_or_id = element_index;
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_get_elem_info TargetOffset:0x%x, Elemtype:0x%x, ElemIndex:0x%x, Slot-Id:%d\n",
                                target_offset, 
                                common_infop->element_type, 
                                element_index, 
                                common_infop->slot_or_id);

            }
#if 0
            // get the side - init the side to 0 and ignore the 
            // return status since not all components have a side attribute
            common_infop->side = 0;
            encl_status = fbe_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                          FBE_ENCL_COMP_SIDE_ID,
                                          fbe_comp_type,
                                          common_infop->slot_or_id,      // component index
                                          &common_infop->side);          // return value
#endif
        }
    }
    else
    {
            // not found
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_enclosure_get_elem_info TargetOffset:0x%x\n",
                            target_offset);
    }

    return;
} // fbe_eses_enclosure_get_elem_info


/*!***************************************************************
 * @fn fbe_eses_enclosure_copy_statistics
 ****************************************************************
 * @brief
 *  This function copies data from the eses receive diag 
 *  statistics page into a response buffer. Data is copied for
 *  a single element index. Special identification data (used 
 *  by the client)is also copied to the user reposnse buffer.
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  sourcep - points to the data in the eses page that will get copied from
 * @param  *destp - points to the response buffer where the data gets copied to
 * @param  *common_infop - information common to each entry (copied from)
 * @param  *bytes_available - the space available in the response buffer (in/out)
 * @param  *size - returns the number of bytes copied (out)
 *
 * @return  FBE_ENCLOSURE_STATUS_PARAMETER_INVALID for overall 
 *          element or unknown element type
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_copy_statistics(fbe_eses_enclosure_t *eses_enclosure,
                                                          fbe_eses_page_statistics_u *sourcep, // source data
                                                          fbe_enclosure_mgmt_statistics_data_t *destp,  //destination pointer
                                                          fbe_enclosure_mgmt_stats_resp_id_info_t *common_infop,
                                                          fbe_u32_t *bytes_available,
                                                          fbe_u32_t *size) // num bytes copied to response buf
{
    fbe_u32_t   stats_data_bytes = 0;
    fbe_enclosure_status_t status = FBE_ENCLOSURE_STATUS_OK;

    // init before using
    *size = 0;

    switch (common_infop->element_type)
    {
    case SES_ELEM_TYPE_PS :
    case SES_ELEM_TYPE_COOLING :
    case SES_ELEM_TYPE_TEMP_SENSOR :
    case SES_ELEM_TYPE_EXP_PHY :
    case SES_ELEM_TYPE_ARRAY_DEV_SLOT :
    case SES_ELEM_TYPE_SAS_EXP :
        // Do not use the struct size element type struct size to calculate 
        // the bytes copied. Instead use the actual page length returned in
        // the eses page.
        // size = hdr size + page length + respbuf id info
        stats_data_bytes = (sizeof(ses_common_statistics_field_t) + sourcep->common.stats_len);
        *size = stats_data_bytes + sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t);
        if (*size <= *bytes_available)
        {
            // copy the id info into the response buffer
            // this is common to each item
            destp->stats_id_info.element_type = common_infop->element_type;
            destp->stats_id_info.slot_or_id = common_infop->slot_or_id;
            destp->stats_id_info.drv_or_conn = common_infop->drv_or_conn;
            destp->stats_id_info.drv_or_conn_num = common_infop->drv_or_conn_num;
            //  destp->stats_id_info.side = common_infop->side;

            // copy the statistics data from the eses page
            fbe_copy_memory(&(destp->stats_data), (fbe_u8_t *)sourcep, stats_data_bytes);

            // reduce the available space for the buffer being filled 
            *bytes_available -= *size;

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_copy_statistics Siz:0x%x com:0x%llx, len:0x%x, info:0x%llx\n",
                                *size,
                                (unsigned long long)sizeof(ses_common_statistics_field_t),
                                sourcep->common.stats_len,
                                (unsigned long long)sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t));

        }
        else
        {
            *size = 0;
            *bytes_available = 0;
        }
        break;

    default :
        // unknown item, don't copy any bytes to the response buffer
        *size = 0;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_copy_statistics:ElementType Not Supported 0x%x\n", 
                                common_infop->element_type);

        status = FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
        break;
    } // end switch

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_enclosure_copy_statistics: Index:0x%x ElemType:0x%x SubEnclId:%d\n",
                            sourcep->common.elem_offset,
                            common_infop->element_type, 
                            common_infop->slot_or_id);

    return status;
} // fbe_eses_enclosure_copy_statistics

/*!***************************************************************
 * @fn fbe_eses_find_element_offset_stats
 ****************************************************************
 * @brief
 *  Search the eses data page for the specified element offset.
 *  The start location of the search is supplied.
 *
 * @param  target_offset - the element offset to translate (in)
 * @param  *datap - where the search starts (in)
 * @param  page_length - number of bytes in the page
 *
 * @return  fbe_eses_page_statistics_u * - the location of the 
 *          target offset or NULL if not found
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_eses_page_statistics_u *fbe_eses_find_element_offset_stats(fbe_u8_t target_offset, fbe_u8_t *datap, fbe_u32_t page_length)
{
    fbe_u16_t   overall_count = 0;
    fbe_u8_t    item_bytes = 0;

    // set the counts for the first item 
    item_bytes = ((fbe_eses_page_statistics_u *)datap)->common.stats_len + sizeof(ses_common_statistics_field_t);
    overall_count = item_bytes;
    // loop until the page data is exhausted
    while (overall_count <= page_length)
    {
        // check for the target element offset
        if (((fbe_eses_page_statistics_u *)datap)->common.elem_offset == target_offset)
        {
            // found
            return((fbe_eses_page_statistics_u*)datap);
            break;
        }

        // next item
        datap += item_bytes;

        // count the space used by the item
        item_bytes = ((fbe_eses_page_statistics_u *)datap)->common.stats_len + sizeof(ses_common_statistics_field_t);
        overall_count += item_bytes;
    }

    return(NULL);

} // end fbe_eses_find_element_offset_stats

/*!***************************************************************
 * @fn fbe_is_fbe_element_type_match
 ****************************************************************
 * @brief
 *  Check the element type at the element offset provided. Return
 *  TRUE if the element type matches the type from the input param.
 *
 * @param  *eses_enclosure - enclosure object
 * @param  element_offset - find the type of this element offset
 * @param  target_element_type
 *
 * @return  fbe_bool_t - TRUE when the elemenet offset contains
 *      the target element type
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_bool_t fbe_is_fbe_element_type_match(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_u8_t element_offset, 
                                   fbe_enclosure_element_type_t target_element_type)
{
    fbe_enclosure_element_type_t element_type;


    element_type = fbe_get_fbe_element_type(eses_enclosure, element_offset);
    if (target_element_type == element_type)
    {
        return TRUE;
    }
    return FALSE;

} //fbe_is_fbe_element_type_match

/*!***************************************************************
 * @fn fbe_get_fbe_element_type
 ****************************************************************
 * @brief
 *  Given an element offset, find the element offset.
 *
 * @param  *eses_enclosure - enclosure object
 * @param  element_offset - find the type of this element offset
 *
 * @return  fbe_enclosure_element_type_t - the fbe enclosure element type
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_element_type_t fbe_get_fbe_element_type(fbe_eses_enclosure_t *eses_enclosure, fbe_u8_t element_offset)
{
    fbe_u8_t                        group_id;
    fbe_eses_elem_group_t           *groupp;
    fbe_bool_t                      overall = FALSE;
    fbe_enclosure_element_type_t    element_type = FBE_ELEMENT_TYPE_INVALID;

    //element_offset = ((fbe_eses_page_statistics_u *)statsp)->common.elem_index;
    group_id = fbe_eses_elem_group_get_elem_offset_group_id(eses_enclosure, element_offset, &overall);
    groupp = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);
    if (group_id != FBE_ENCLOSURE_VALUE_INVALID)
    {
        element_type = fbe_eses_elem_group_get_elem_type(groupp, group_id);
        //fbe_comp_type = (fbe_u8_t) fbe_eses_stats_ses_elem_type_to_component_type(common_infop->element_type);
    }
    return(element_type);
} // fbe_get_fbe_element_type

/*!***************************************************************
 * @fn fbe_eses_get_truncated_bytes
 ****************************************************************
 * @brief
 *  Determine the amount of space that is needed in the response 
 *  buffer in order to parse the remaining statistics data.
 *  The start location of the search is supplied and the page bytes
 *  processed upt to that point are supplied.
 *
 * @param  *eses_enclosure - the enclosure object
 * @param  *statsp - where the search starts
 * @param  page_bytes_processed - subtotal of the page bytes processed 
 * @param  page_size - total number of bytes in the eses page
 * @param   type - specifies the element type for the statistics data
 *
 * @return  fbe_u32_t * - the amount of space (bytes) needed in 
 *                      the response buffer.
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_u32_t fbe_eses_get_truncated_bytes(fbe_eses_enclosure_t *eses_enclosure,
                                       fbe_u8_t *statsp, 
                                       fbe_u32_t page_bytes_processed, 
                                       fbe_u32_t page_size, 
                                       fbe_enclosure_element_type_t stats_element_type)
{
    fbe_u32_t   page_bytes;
    fbe_u32_t   item_bytes;
    fbe_u8_t    element_offset;
    fbe_u32_t   truncated_bytes = 0;

    // init the count
    page_bytes = page_bytes_processed;

    // loop through the remaining items in the eses statistics page
    while (page_bytes<page_size)
    {
        // count the space used by the item
        item_bytes = ((fbe_eses_page_statistics_u *)statsp)->common.stats_len + sizeof(ses_common_statistics_field_t);
        // add it to the subtotal ot total page bytes
        page_bytes += item_bytes;

        // point to the next item
        statsp += item_bytes;

        // FBE_ELEMENT_TYPE_INVALID means process all, otherwise only process the specified type
        if (FBE_ELEMENT_TYPE_INVALID == stats_element_type)
        {
            // subtotal the truncated count
            // The truncated count includes additional bytes that are placed 
            // in the response buffer as part of the formatted parsed output.
            truncated_bytes += item_bytes + sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t);
        }
        else
        {
            // only add the count if this is a desired element type
            element_offset = ((fbe_eses_page_statistics_u *)statsp)->common.elem_offset;
            if (TRUE == fbe_is_fbe_element_type_match(eses_enclosure, element_offset, stats_element_type))
            {
                // subtotal the truncated count
                // The truncated count includes additional bytes that are placed 
                // in the response buffer as part of the formatted parsed output.
                truncated_bytes += item_bytes + sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t);
            }
        }
    }
    return(truncated_bytes);
} // end fbe_eses_get_truncated_bytes

/*!***************************************************************
 * @fn fbe_eses_parse_single_stats_element_type
 ****************************************************************
 * @brief
 *  Process data based on the element type. Since the eses data 
 *  is presented using overall elements and the edal element data 
 *  only represents individual elements, the following must occur:
 *  1) for each group containing the specified element type
 *      a) determine the element offset for the first individual 
 *      element in the group
 *      b) for each individual element in the group:
 *          1) locate the element offset stats in the eses page
 *          2) copy the stats to the response page
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *statsp - eses page data
 * @param  *destp - response buffer data
 * @param  element_type - the target element type
 * @param  *avail_response_space - the space remaining in the response buffer
 * @param  *data_length - the number of bytes copied
 * @param  page_size - the size of the eses page
 *
 * @return  FBE_ENCLOSURE_STATUS_OK
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t  fbe_eses_parse_single_stats_element_type(fbe_eses_enclosure_t *eses_enclosure,
                                           ses_pg_emc_statistics_struct *statsp,
                                           fbe_u8_t *destp,
                                           fbe_u8_t element_type,
                                           fbe_u32_t *avail_response_space,
                                           fbe_u32_t *data_length,
                                           fbe_u32_t  page_size)
{
    fbe_u8_t                element_offset;
    fbe_u8_t                group_id;
    fbe_u8_t                index;                      // loop control
    fbe_u8_t                total_in_group;             // loop control
    fbe_u8_t                first_element_index;             // loop control
    fbe_u8_t                fbe_comp_type;
    fbe_u8_t                exp_phy_id = 0;
    fbe_u8_t                exp_phy_index = 0;
    fbe_eses_elem_group_t   *elem_group;                // 
    fbe_eses_page_statistics_u *sourcep;
    fbe_u32_t               bytes_copied;               // number of bytes copied into the response buffer
    fbe_u32_t               element_size = 0;           // size of the element that was copied
    fbe_bool_t              more_goups = TRUE;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_mgmt_stats_resp_id_info_t common_info;

    bytes_copied = 0;
    group_id = 0;

    fbe_zero_memory(&common_info, sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t));
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    /* find element groups that contain the specified element type */
    while(more_goups)
    {
        // find element group for this element type
        // group_id specifies the starting group number for get_group_type
        group_id = fbe_eses_elem_group_get_group_type(eses_enclosure, element_type, group_id);
        if (group_id == FBE_ENCLOSURE_VALUE_INVALID)
        {
            // no more groups of this type
            more_goups = FALSE;
            break;
        }

        total_in_group = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);
        if (total_in_group == 0)
        {
            // this group is empty, next iteration
            group_id++;
            continue;

        }
        // If there are more than 0 items in the group, we should get valid
        // element offset and first item.
        element_offset = fbe_eses_elem_group_get_first_indiv_elem_offset(elem_group, group_id);
        first_element_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);

        // get the stats for each element in this group
        for (index = first_element_index;
            index < (first_element_index + total_in_group);
            index++, element_offset++)
        {
            // search the page data for the target element offset
            // statsp is kept intact so we always search the entire page
            sourcep = fbe_eses_find_element_offset_stats(element_offset, (fbe_u8_t *)(&statsp->statistics), page_size);
            if (sourcep == NULL)
            {
                // not found - each index for the group should be present (we should be able to break here), 
                // but, check the next index anyway
                continue;
            }

            // update the common info for the element
            common_info.element_type = element_type;
            //common_info.side = 0;

            // get the slot if one is associated with this element type
            if (element_type == SES_ELEM_TYPE_EXP_PHY)
            {
                /* elem_index -> exp_phy_index */
                exp_phy_index = 
                fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                            FBE_ENCL_EXPANDER_PHY,  // Component type
                                                            0, //starting index
                                                            index);

                if (exp_phy_index != FBE_ENCLOSURE_VALUE_INVALID)
                {
                    /* exp_phy_index -> exp_phy_id */
                    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_EXP_PHY_ID, //attribute
                                                                        FBE_ENCL_EXPANDER_PHY,// Component type
                                                                        exp_phy_index, // Component index
                                                                        &exp_phy_id);

                    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        return encl_status;
                    }
                    else
                    {
                        common_info.slot_or_id = exp_phy_id;
                    }
                }
                else
                {
                    return FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
                }
            }
            else 
            {
                // translate the element type to edal component type
                fbe_comp_type = (fbe_u8_t) fbe_eses_stats_ses_elem_type_to_component_type(common_info.element_type);
                common_info.slot_or_id = index;
            }

            // copy the found data
            encl_status = fbe_eses_enclosure_copy_statistics(eses_enclosure, 
                                                     sourcep,
                                                     (fbe_enclosure_mgmt_statistics_data_t*)destp,
                                                     &common_info,
                                                     avail_response_space,
                                                     &bytes_copied);
            if (*avail_response_space == 0)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_single_stats_element_type:ResponseBuffer 0 BytesRemainging!!!\n");
                break;
            }

            *data_length += bytes_copied;
            // adjust the destination pointer
            destp += bytes_copied;
            // adjust the bytes processed in the page data
            // get the element size for this element
            // used for adjusting the source pointer and counting the bytes in the page
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_single_stats_element_type:bytescopied:0x%x elemsize:0x%x, nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                            bytes_copied, element_size, destp, statsp);

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_single_stats_element_type:AvailRspBuf:0x%x, SumBytesCopied:0x%x\n",
                            *avail_response_space,
                            *data_length);

        } // for each element index within the group

        // next group
        group_id++;

    } // while more_groups


    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_single_stats_element_type:bytescopied:0x%x, nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                            *data_length, destp, statsp);

    return FBE_ENCLOSURE_STATUS_OK;
} // fbe_eses_parse_single_stats_element_type


/*!***************************************************************
 * @fn fbe_eses_parse_all_stats
 ****************************************************************
 * @brief
 *  Process all the statistics data in the eses page.
 *
 *  For each element item in the eses page:
 *      1) read the element offset and get the element type
 *      2) copy the stats to the response page
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *statsp - eses page data
 * @param  *destp - response buffer data
 * @param  *avail_response_space - the space remaining in the response buffer
 * @param  *data_length - the number of bytes copied + bytes truncated
 * @param  page_size - the size of the eses page
 *
 * @return  FBE_ENCLOSURE_STATUS_OK
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_parse_all_stats(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u8_t  *statsp,// points to fbe_eses_page_statistics_u
                                           fbe_u8_t  *destp,
                                           fbe_u32_t *avail_response_space,
                                           fbe_u32_t *data_length,
                                           fbe_u32_t  page_size)
{
    fbe_u8_t    element_offset;     // element offset read from the eses page data item
    fbe_u32_t   bytes_copied;       // number of bytes copied into the response buffer
    fbe_u32_t   element_size = 0;   // size of the element that was copied
    fbe_bool_t  overall_element;    // the item represents an overall element
    fbe_u32_t   page_bytes_processed = 0;    // the bytes processed from the eses page
    fbe_enclosure_mgmt_stats_resp_id_info_t common_info;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;


    fbe_zero_memory(&common_info, sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t));
    while (page_bytes_processed < page_size)
    {
        // get the element offset from the eses returned data
        element_offset = ((fbe_eses_page_statistics_u *)statsp)->common.elem_offset;

        // get the element type, slot or comp id, side, overall_element
        fbe_eses_enclosure_get_elem_info(eses_enclosure,
                                               element_offset,
                                               &common_info,
                                               &overall_element);

        // clear each time before using
        bytes_copied = 0;

        if (!overall_element)
        {
            // copy the data for this element
            encl_status = fbe_eses_enclosure_copy_statistics(eses_enclosure, 
                                                         (fbe_eses_page_statistics_u *)statsp, // source
                                                         (fbe_enclosure_mgmt_statistics_data_t*)destp,  // dest
                                                         &common_info,
                                                         avail_response_space,  // gets written to
                                                         &bytes_copied);        // gets written to

            // not enough space left in the destination buffer
            if (*avail_response_space == 0)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_all_stats:ResponseBuffer 0 BytesRemaining!!!\n");

                *data_length += fbe_eses_get_truncated_bytes(eses_enclosure, 
                                                            statsp, 
                                                            page_bytes_processed, 
                                                            page_size, 
                                                            FBE_ELEMENT_TYPE_INVALID); //common_info.element_type);
                break;
            }
        }

        // Need the element size to adjust the source pointer and for 
        // counting the bytes processed in the eses page.
        element_size = ((fbe_eses_page_statistics_u *)statsp)->common.stats_len + sizeof(ses_common_statistics_field_t);

        *data_length += bytes_copied;
        // adjust the destination pointer
        destp += bytes_copied;
        // adjust the bytes processed in the page data
        page_bytes_processed += element_size;
        // point to next element in the page data
        statsp += element_size;
        // source_stats should be pointing to the next element, get the next offset
        element_offset = ((fbe_eses_page_statistics_u *)statsp)->common.elem_offset;

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_all_stats:bytescopied:0x%x elemsize:0x%x, nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                            bytes_copied, element_size, destp, statsp);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_all_stats:AvailRspBuf:0x%x, SumBytesCopied:0x%x, PgBytesDone:0x%x\n",
                            *avail_response_space,
                            *data_length,
                            page_bytes_processed);
    }
    return FBE_ENCLOSURE_STATUS_OK;
} // fbe_eses_parse_all_stats

/*!***************************************************************
 * @fn fbe_eses_slot_index_to_element_index
 ****************************************************************
 * @brief
 *  Get the element index for the associated slot. The mmethod
 *  used to get he element index depends on the element type.
 *
 * @param  *enclosure - pointer to the enclosure data
 * @param  slot - the phy slot index to use (in)
 * @param  element_type - the element type referencing the slot
 * @param  *element_index - the element index for the phy slot (out)
 *
 * @return  fbe_enclosure_status_t passed back from the search functions
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_slot_index_to_element_index(fbe_eses_enclosure_t *enclosure, 
                                                      fbe_u8_t slot, 
                                                      fbe_u8_t element_type,
                                                      fbe_u8_t *element_index)
{
  //  fbe_u8_t                component_index;
    fbe_enclosure_status_t  encl_status;

    if (element_type == SES_ELEM_TYPE_EXP_PHY)
    {
        // phy_index -> element index. 
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)enclosure,
                                                FBE_ENCL_COMP_ELEM_INDEX,   // attribute
                                                FBE_ENCL_EXPANDER_PHY,      // Component type
                                                slot,            // phy component index
                                                element_index);
    }
    else
    {
        // type == SES_ELEM_TYPE_ARRAY_DEV_SLOT
        encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)enclosure,
                                                FBE_ENCL_COMP_ELEM_INDEX,   // attribute
                                                FBE_ENCL_DRIVE_SLOT,        // Component type
                                                slot,                       // drive component index
                                                element_index);
    }
    return encl_status;

} // fbe_eses_slot_index_to_element_index

/*!***************************************************************
 * @fn fbe_eses_element_index_to_element_offset
 ****************************************************************
 * @brief
 *  Get the element offset associated with the supplied element index.
 *
 * @param  *enclosure - pointer to the enclosure data
 * @param  target_index - the element index to reference (in)
 * @param  *element_offset - the found element offset (out)
 *
 * @return  FBE_ENCLOSURE_STATUS_OK
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_element_index_to_element_offset(fbe_eses_enclosure_t *eses_enclosure, 
                                                   fbe_u8_t target_index, 
                                                   fbe_u8_t *element_offset)
{
    fbe_u8_t                group_id = 0;
    fbe_u8_t                first_elem_index = 0;
    fbe_u8_t                num_possible_elems = 0;
    fbe_eses_elem_group_t * elem_group = NULL;

    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    // Go through each item of the group info up to the one containing the target
    // element index. Count each group that contains 0 elements. When the target is
    // found, calculate the element offset.
    for(group_id = 0; group_id < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); group_id ++)
    {
        first_elem_index = fbe_eses_elem_group_get_first_elem_index(elem_group, group_id);
        num_possible_elems = fbe_eses_elem_group_get_num_possible_elems(elem_group, group_id);

        if((target_index >= first_elem_index) &&
           (target_index < first_elem_index + num_possible_elems))
        {
            *element_offset = target_index + (group_id + 1);
            break;
        }
    }

    return FBE_ENCLOSURE_STATUS_OK;

} // fbe_eses_element_index_to_element_offset


/*!***************************************************************
 * @fn fbe_eses_parse_stats_by_slot
 ****************************************************************
 * @brief
 *  Parse the eses statistics page by phy slot position.
 *  For each phy slot:
 *      - get the element index associated with the phy slot
 *      - translate the element index into element offset
 *      - search the eses data page for the element offset
 *      - copy the element item statistics into the reponse buffer
 *
 * @param  *enclosure - pointer to the enclosure data
 * @param  *eses_page_op - the control operation
 * @param  *statsp - the eses data page
 * @param  *destp - the response buffer data
 * @param  *bytes_remaining - the space remaining in the response buffer
 * @param  *data_length - the number of bytes copied
 * @param  limit - the byte size limit of the source data
 *
 * @return  FBE_ENCLOSURE_STATUS_OK
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_parse_stats_by_slot(fbe_eses_enclosure_t *enclosure,
                                fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                fbe_eses_page_statistics_u *statsp,
                                fbe_u8_t *destp, //fbe_enclosure_mgmt_statistics_data_t *destp,
                                fbe_u32_t *avail_response_space,
                                fbe_u32_t *data_length,
                                fbe_u32_t limit)
{
    fbe_u32_t                    slot;           // loop control
    fbe_u8_t                    last_slot;      // loop control
    fbe_u8_t                    element_offset; // determined target
    fbe_u8_t                    element_index;
    fbe_u8_t                    max_slot_num;
    fbe_u32_t                   bytes_copied;   // number of bytes copied into the response buffer
    fbe_u8_t                    group_number;
    fbe_eses_elem_group_t       *element_group = NULL;
    fbe_u32_t                   element_size = 0;   // size of the element that was copied
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                    *sourcep = NULL;
    fbe_enclosure_mgmt_stats_resp_id_info_t common_info;


    // init some variables
    fbe_zero_memory(&common_info, sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t));
    element_index = 0;
    sourcep = (fbe_u8_t *)statsp;
    last_slot = eses_page_op->cmd_buf.element_statistics.last;

    // type does not change so it can be filled now
    if ((eses_page_op->cmd_buf.element_statistics.type != FBE_ELEMENT_TYPE_EXP_PHY) &&
        (eses_page_op->cmd_buf.element_statistics.type != FBE_ELEMENT_TYPE_DEV_SLOT))
    {
        return FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST;
    }

    common_info.element_type = eses_page_op->cmd_buf.element_statistics.type;
    // limit the number of slots by the number of elements in the group
    group_number = fbe_eses_elem_group_get_group_type(enclosure, common_info.element_type, 0);
    element_group = fbe_eses_enclosure_get_elem_group_ptr(enclosure);
    // subtract 1 from the max number of slots to get the max slot number (slot numbers start at 0)
    max_slot_num = (fbe_eses_elem_group_get_num_possible_elems(element_group, group_number) - 1);

    if (last_slot > max_slot_num)
    {
        last_slot = max_slot_num;
    }
    // for each slot/phy number specified in the command
    for (slot = eses_page_op->cmd_buf.element_statistics.first; 
        slot<=last_slot; 
        slot++)
    {

        // find the element index for this slot/phy
        encl_status = fbe_eses_slot_index_to_element_index(enclosure, slot, common_info.element_type, &element_index);

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            continue;
        }

        if (element_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            continue;
        }

        // translate the index into an element offset
        encl_status = fbe_eses_element_index_to_element_offset(enclosure, element_index, &element_offset);

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            continue;
        }

        // search the eses data for this element offset data
        // statsp is kept intact so we always search the entire page
        sourcep = (fbe_u8_t *)fbe_eses_find_element_offset_stats(element_offset, (fbe_u8_t *)statsp, limit);
        if (sourcep == NULL)
        {
            // not found
            continue;
        }

        if (eses_page_op->cmd_buf.element_statistics.type == FBE_ELEMENT_TYPE_EXP_PHY)
        {
            // get drive slot or connector number for this phy
            fbe_eses_enclosure_pop_phy_info(enclosure, slot, &common_info);
            fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) enclosure),
                            "fbe_eses_parse_stats_by_slot EXPPHY ElemOffset:0x%x, ElemIndex:%d, PhyId:%d\n",
                            element_offset,
                            element_index, 
                            slot);
        }
        else
        {
            common_info.slot_or_id = slot;
        }

        // copy the eses data to the response buffer
        encl_status = fbe_eses_enclosure_copy_statistics(enclosure,
                                                          (fbe_eses_page_statistics_u *)sourcep,  // source data
                                                          (fbe_enclosure_mgmt_statistics_data_t *)destp, // destination pointer
                                                          &common_info, // more source data
                                                          avail_response_space,
                                                          &bytes_copied);           // this is never an overall element

        if (*avail_response_space == 0)
        {
            fbe_base_object_trace((fbe_base_object_t*)enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "ParseAllStats:ResponseBuffer 0 BytesRemainging!!!\n");
            break;
        }

        // get the element size for this element
        // used for adjusting the source pointer and counting the bytes in the page
        element_size = ((fbe_eses_page_statistics_u *)statsp)->common.stats_len + sizeof(ses_common_statistics_field_t);
        *data_length += bytes_copied;
        // adjust the destination pointer
        destp += bytes_copied;
        // point to next element in the page data
        sourcep += element_size;

        fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) enclosure),
                            "fbe_eses_parse_stats_by_slot:bytescopied:0x%x elemsize:0x%x, nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                            bytes_copied, element_size, destp, sourcep);

        fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) enclosure),
                            "fbe_eses_parse_stats_by_slot:AvailRspBuf:0x%x, SumBytesCopied:0x%x\n",
                            *avail_response_space,
                            *data_length);

    }
    return(encl_status);
} //fbe_eses_parse_stats_by_slot

/*!***************************************************************
 * @fn fbe_eses_parse_statistics_page
 ****************************************************************
 * @brief
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *eses_page_op - the control operation
 * @param  *sourcep - the eses data page
 *
 * @return  fbe_enclosure_status_t
 *
 * HISTORY:
 *  23-Apr-2009 GB - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_parse_statistics_page(fbe_eses_enclosure_t *eses_enclosure,
                        fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,    // details of the command (element type, slot, etc)
                        ses_pg_emc_statistics_struct * sourcep)
{
    fbe_u32_t                   stats_bytes;    // counts the statistics page bytes processed
    fbe_u32_t                   limit;          // the returned eses page length
    fbe_eses_page_statistics_u  *eses_statsp;   // points into the page data, page header and statistics element items
    fbe_u8_t                    *destp;         // the response buffer where the parsed data gets copied to
    fbe_u32_t                   bytes_remaining; // space remaining in the response buffer
    fbe_u32_t                   data_length = 0; // indicates the length of the response data copied to the user/client buffer
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;

    stats_bytes = 0;

    // swap eses pg_len before using
    limit = swap16(sourcep->hdr.pg_len);
    limit -= FBE_ESES_GENERATION_CODE_SIZE;    // actual bytes = pagelength - sizeof(generation_code)

    // first element item in the returned page data
    eses_statsp = (fbe_eses_page_statistics_u *)&sourcep->statistics.common; //remove?

    destp = eses_page_op->response_buf_p;
    // where parsed data will go
    destp = (fbe_u8_t *)&(((fbe_enclosure_mgmt_statistscs_response_page_t *)destp)->data);

    // size of the response buffer
    bytes_remaining = eses_page_op->response_buf_size;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_statistics_page:nxtdestp:0x%p, nxtsourcep:0x%p rspbufsiz:0x%x\n", 
                             destp, eses_statsp, bytes_remaining);

    if ((FBE_ELEMENT_TYPE_DEV_SLOT == eses_page_op->cmd_buf.element_statistics.type) ||
        (FBE_ELEMENT_TYPE_EXP_PHY == eses_page_op->cmd_buf.element_statistics.type))
    {
        // sort by slot number
        fbe_eses_parse_stats_by_slot(eses_enclosure,
                               eses_page_op,
                               &sourcep->statistics, //(fbe_u8_t *)eses_statsp,//fbe_eses_page_statistics_u
                               destp, // this can be derived from eses_page_page_op
                               &bytes_remaining,
                               &data_length,
                               limit);
    }
    else if (FBE_ELEMENT_TYPE_ALL == eses_page_op->cmd_buf.element_statistics.type)
    {
        encl_status = fbe_eses_parse_all_stats(eses_enclosure,
                                          (fbe_u8_t *)&sourcep->statistics,
                                          destp,
                                          &bytes_remaining,
                                          &data_length,
                                          limit);
    }
    else
    {
        // specific element type
        encl_status = fbe_eses_parse_single_stats_element_type(eses_enclosure,
                                                     sourcep,
                                                     destp, // where the data goes
                                                     eses_page_op->cmd_buf.element_statistics.type,
                                                     &bytes_remaining,
                                                     &data_length,
                                                     limit);
    }

    // include the number of bytes copied in the response data
    //destp = eses_page_op->response_buf_p;
    eses_page_op->required_response_buf_size = data_length;
    //((fbe_enclosure_mgmt_statistscs_response_page_t*)destp)->data_length = data_length;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_statistics_page:TotalBytebytesCopied:0x%x\n", 
                            ((fbe_enclosure_mgmt_statistscs_response_page_t*)destp)->data_length);

    return(encl_status);
} // fbe_eses_parse_statistics_page

/*!***************************************************************
 * @fn fbe_eses_enclosure_copy_threshold
 ****************************************************************
 * @brief
 *  This function copies data from the eses receive diag 
 *  threshold page into a response buffer. Data is copied for
 *  a single element index. Special identification data (used 
 *  by the client)is also copied to the user reposnse buffer.
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *destp - points to the response buffer where the data gets copied to
 * @param  *common_infop - information common to each entry (copied from)
 * @param  *bytes_available - the space available in the response buffer (in/out)
 * @param  *size - returns the number of bytes copied (out)
 *
 * @return  FBE_ENCLOSURE_STATUS_PARAMETER_INVALID for overall 
 *          element or unknown element type
 *
 * HISTORY:
 *  23-Apr-2009 Prasenjeet - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_enclosure_copy_threshold(fbe_eses_enclosure_t *eses_enclosure,
                                                          fbe_enclosure_mgmt_exp_threshold_info_t *destp,  //destination pointer
                                                          fbe_enclosure_mgmt_exp_threshold_info_t *common_infop,
                                                          fbe_u32_t *bytes_available,
                                                          fbe_u32_t *size) // num bytes copied to response buf
{
    fbe_enclosure_status_t status = FBE_ENCLOSURE_STATUS_OK;

    // init before using
    *size = 0;

    switch (common_infop->elem_type)
    {
      case SES_ELEM_TYPE_TEMP_SENSOR :

        // Do not use the struct size element type struct size to calculate 
        // the bytes copied. Instead use the actual page length returned in
        // the eses page.       
       
        *size = sizeof(fbe_enclosure_mgmt_exp_threshold_info_t);
        if (*size <= *bytes_available)
        {
            // copy the id info into the response buffer
            // this is common to each item
            destp->elem_type = common_infop->elem_type;
            destp->exp_overall_info.high_critical_threshold = common_infop->exp_overall_info.high_critical_threshold;
            destp->exp_overall_info.high_warning_threshold = common_infop->exp_overall_info.high_warning_threshold;
            destp->exp_overall_info.low_critical_threshold = common_infop->exp_overall_info.low_critical_threshold;
            destp->exp_overall_info.low_warning_threshold = common_infop->exp_overall_info.low_warning_threshold;

            // reduce the available space for the buffer being filled 
            *bytes_available -= *size;

            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_copy_threshold Siz:0x%x \n",
                                *size);
        }
        else
        {
            *size = 0;
            *bytes_available = 0;
        }
        break;
      default :
        // unknown item, don't copy any bytes to the response buffer
        *size = 0;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_enclosure_copy_threshold:ElementType Not Supported 0x%x\n", 
                                common_infop->elem_type);

        status = FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
        break;
    } // end switch

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_enclosure_copy_threshold:  ElemType:0x%x\n",
                            common_infop->elem_type);
    return status;
} // fbe_eses_enclosure_copy_threshold

/*!***************************************************************
 * @fn fbe_eses_parse_single_thresh_element_type
 ****************************************************************
 * @brief
 *  Process data based on the element type. 
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *threshp - eses page data
 * @param  *destp - response buffer data
 * @param  element_type - the target element type
 * @param  *avail_response_space - the space remaining in the response buffer
 * @param  *data_length - the number of bytes copied
 * @param  page_size - the size of the eses page
 *
 * @return  FBE_ENCLOSURE_STATUS_OK
 *
 * HISTORY:
 *  21-Sep-2009 Prasenjeet - Created. 
 ****************************************************************/
fbe_enclosure_status_t  fbe_eses_parse_single_thresh_element_type(fbe_eses_enclosure_t *eses_enclosure,
                                           ses_pg_emc_threshold_struct *threshp,
                                           fbe_enclosure_mgmt_exp_threshold_info_t *destp,
                                           fbe_u8_t componentType,
                                           fbe_u32_t *avail_response_space,
                                           fbe_u32_t *data_length,
                                           fbe_u32_t  page_size)
{
    fbe_u16_t              byte_offset;//element_offset;
    fbe_u8_t                group_id;
    fbe_u32_t               bytes_copied;               // number of bytes copied into the response buffer
    fbe_bool_t              more_goups = TRUE;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_mgmt_exp_threshold_info_t common_info;
    fbe_expander_threshold_overall_info_t *thresh_overall_elem_stat = NULL;
    fbe_eses_elem_group_t          * elem_group = NULL;
    ses_elem_type_enum               elem_type = SES_ELEM_TYPE_INVALID;
    fbe_u8_t                subencl_id = SES_SUBENCL_ID_NONE;
    fbe_u8_t                side_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_component_types_t  subencl_component_type = FBE_ENCL_INVALID_COMPONENT_TYPE;

    bytes_copied = 0;
    group_id = 0;

    fbe_zero_memory(&common_info, sizeof(fbe_enclosure_mgmt_exp_threshold_info_t));
    elem_group = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure);

    encl_status = fbe_eses_enclosure_component_type_to_element_type(eses_enclosure,
                                                componentType,
                                                &elem_type);

    /* find element groups that contain the specified element type */
    while(more_goups)
    {
        // find element group for this element type
        // group_id specifies the starting group number for get_group_type
        group_id = fbe_eses_elem_group_get_group_type(eses_enclosure, elem_type, group_id);
        if (group_id == FBE_ENCLOSURE_VALUE_INVALID)
        {
            // no more groups of this type
            more_goups = FALSE;
            break;
        }

        /* Get the byte offset */
        byte_offset = fbe_eses_elem_group_get_group_byte_offset(elem_group, group_id);

        /* Get the element type of this element group. */
        //elem_type = fbe_eses_elem_group_get_elem_type(elem_group, group_id);

        /* Get the subenclosure id of this element group. */
        subencl_id = fbe_eses_elem_group_get_subencl_id(elem_group, group_id);     
        
        /* Get the subenclosure component type for this element group. */
        encl_status = fbe_eses_enclosure_subencl_id_to_side_id_component_type(eses_enclosure,
                                                 subencl_id, 
                                                 &side_id,
                                                 &subencl_component_type);
        if(encl_status == FBE_ENCLOSURE_STATUS_OK)
        {
            switch(elem_type)
            {
                case SES_ELEM_TYPE_TEMP_SENSOR:
                    // Right now we are considering only TEMP_SENSOR_SUBTYPE_CHASSIS i.e. FBE_ENCL_ENCLOSURE
                    if(subencl_component_type == FBE_ENCL_ENCLOSURE) 
                    {
                        thresh_overall_elem_stat = (fbe_expander_threshold_overall_info_t *) ( ((char *)threshp) + byte_offset);
                        if(thresh_overall_elem_stat !=NULL)
                        {
                          common_info.elem_type = elem_type;
                          common_info.exp_overall_info.high_critical_threshold = thresh_overall_elem_stat->high_critical_threshold;
                          common_info.exp_overall_info.high_warning_threshold = thresh_overall_elem_stat->high_warning_threshold;
                          common_info.exp_overall_info.low_critical_threshold = thresh_overall_elem_stat->low_critical_threshold;
                          common_info.exp_overall_info.low_warning_threshold = thresh_overall_elem_stat->low_warning_threshold;

                        // copy the found data
                        encl_status = fbe_eses_enclosure_copy_threshold(eses_enclosure, 
                                                                             (fbe_enclosure_mgmt_exp_threshold_info_t*)destp,
                                                                             &common_info,
                                                                             avail_response_space,
                                                                             &bytes_copied);
                        if (*avail_response_space == 0)
                        {
                         fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "fbe_eses_parse_single_thresh_element_type:ResponseBuffer 0 BytesRemainging!!!\n");
                         break;
                        }
                        *data_length += bytes_copied;
                        // adjust the destination pointer
                        destp += bytes_copied;  
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                                    "fbe_eses_parse_single_thresh_element_type:bytescopied:0x%x , nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                                                    bytes_copied, destp, threshp);
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                                    "fbe_eses_parse_single_thresh_element_type:AvailRspBuf:0x%x, SumBytesCopied:0x%x\n",
                                                    *avail_response_space,
                                                    *data_length);
                        }
                    }
                    break;
                    default:
                        encl_status = FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED;
                    break;
            }
        }
        // next group
        group_id++;
    } // while more_groups

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_single_thresh_element_type:bytescopied:0x%x, nxtdestp:0x%p, nxtsourcep:0x%p\n", 
                            *data_length, destp, threshp);

    return FBE_ENCLOSURE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_eses_parse_threshold_page
 ****************************************************************
 * @brief
 *
 * @param  *eses_enclosure - pointer to the enclosure data
 * @param  *eses_page_op - the control operation
 * @param  *sourcep - the eses data page
 *
 * @return  fbe_enclosure_status_t
 *
 * HISTORY:
 *  17-Sep-2009 Prasenjeet - Created. 
 ****************************************************************/
fbe_enclosure_status_t fbe_eses_parse_threshold_page(fbe_eses_enclosure_t *eses_enclosure,
                        fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,    // details of the command (element type, etc)
                        fbe_u8_t * sourcep)
{
    fbe_u32_t                   thresh_bytes;    // counts the threshold page bytes processed
    fbe_u32_t                   limit;          // the returned eses page length
    fbe_u8_t                    *destp;         // the response buffer where the parsed data gets copied to
    fbe_u32_t                   bytes_remaining; // space remaining in the response buffer
    fbe_u32_t                   data_length = 0; // indicates the length of the response data copied to the user/client buffer
    fbe_enclosure_status_t      encl_status = FBE_ENCLOSURE_STATUS_OK;

    thresh_bytes = 0;

    // swap eses pg_len before using
    limit = swap16(((ses_pg_emc_threshold_struct *)sourcep)->hdr.pg_len);
    limit -= FBE_ESES_GENERATION_CODE_SIZE;    // actual bytes = pagelength - sizeof(generation_code)

    // where parsed data will go
    destp = eses_page_op->response_buf_p;

    // size of the response buffer
    bytes_remaining = eses_page_op->response_buf_size;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_threshold_page:ComponentType:0x%x nxtdestp:0x%p, rspbufsiz:0x%x\n", 
                             eses_page_op->cmd_buf.threshold_in.componentType,destp,  bytes_remaining);

     // specific element type
     encl_status = fbe_eses_parse_single_thresh_element_type(eses_enclosure,
                                                     (ses_pg_emc_threshold_struct *)sourcep,
                                                     (fbe_enclosure_mgmt_exp_threshold_info_t *)destp, // where the data goes
                                                     eses_page_op->cmd_buf.threshold_in.componentType,
                                                     &bytes_remaining,
                                                     &data_length,
                                                     limit);

    // include the number of bytes copied in the response data
    //destp = eses_page_op->response_buf_p;
    eses_page_op->required_response_buf_size = data_length;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                            "fbe_eses_parse_threshold_page:TotalBytebytesCopied:0x%x\n", 
                            eses_page_op->required_response_buf_size);
    return(encl_status);
} // fbe_eses_parse_threshold_page

/**************************************************************************
* @fn fbe_eses_get_chunk_and_add_to_list()
***************************************************************************
* DESCRIPTION
* @brief
*   Allocate a buffer and assign it to an sg element item. Buffers are 
*   sized to FBE_ESES_MEMORY_CHUNK_SIZE. This function allocates a single 
*   buffer and should be called multiple times for sizes greater 
*   than FBE_ESES_MEMORY_CHUNK_SIZE.
*   
* PARAMETERS
*   @param eses_enclosure - 
*   @param sgp - points to sg element to use
*   @param request_size - number of bytes requested needed.
*
* RETURN VALUES
*   @return FBE_ENCLOSURE_STATUS_OK when good
*
* HISTORY
*   31-Mar-2009 GB - Created.
***************************************************************************/
fbe_enclosure_status_t fbe_eses_get_chunk_and_add_to_list(fbe_eses_enclosure_t * eses_enclosure, 
                                                          fbe_sg_element_t *sgp, 
                                                          fbe_u32_t request_size)
{
    fbe_u8_t *buffer;

    // verify input parameters
    if ((request_size > FBE_ESES_MEMORY_CHUNK_SIZE) || (sgp == NULL))
    {
        return(FBE_ENCLOSURE_STATUS_PARAMETER_INVALID);
    }

    // allocate a memory chunk
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)
    {
        return(FBE_ENCLOSURE_STATUS_MEM_ALLOC_FAILED);
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                                "%s, Memory allocated, buffer %64p.\n", 
                                __FUNCTION__,
                                buffer);

    // fill in the sg element info
    fbe_sg_element_init(sgp,
                        request_size,
                        buffer);

    return(FBE_ENCLOSURE_STATUS_OK);

} // end fbe_eses_get_chunk_and_add_to_list


/**************************************************************************
* @fn fbe_eses_enclosure_sg_list_bytes_left()
***************************************************************************
* DESCRIPTION
*   @brief
*   Determine the number of free bytes in the memory chunk that contains
*   the sg list. Input parameters supply he number of items and the 
*   item count that pertains to this memory chunk.
*
* PARAMETERS
*   @param number_of_items - number of items in the sg list
*   @param bytes_used - sum of item.count oertaining to this memory chunck
*
* RETURN VALUES
*   @return bytes_remaining - number of available bytes in the buffer
*
* HISTORY
*   31-Mar-2009 GB - Created.
*   13-Apr-2009 PHE - Changed to a more generic function name. 
***************************************************************************/
fbe_u32_t fbe_eses_enclosure_sg_list_bytes_left(fbe_u32_t number_of_items, fbe_u32_t bytes_used)
{
    fbe_u32_t   bytes_remaining = 0;

    // space used by sg item + the bytes used (should equal the sum of the count fields)
    bytes_used = (sizeof(fbe_sg_element_t) * number_of_items) + bytes_used;
    if (bytes_used < FBE_ESES_MEMORY_CHUNK_SIZE)
    {
        bytes_remaining = FBE_ESES_MEMORY_CHUNK_SIZE - bytes_used;
    }
    return(bytes_remaining);
} // end fbe_eses_enclosure_sg_list_bytes_left


/**************************************************************************
* @fn fbe_eses_enclosure_copy_data_to_sg()
***************************************************************************
* DESCRIPTION
*   @brief
*   Given the start of a sg list and a data address, copy data 
*   into a sg list. The sg list itme content control the copy. The sg 
*   list items must be valid.
*   
* PARAMETERS
*   @param sg_p - sg list item to hold the data
*   @param data_p - the start if the data to copy
*
* HISTORY
*   31-Mar-2009 GB - Created.
***************************************************************************/
void fbe_eses_enclosure_copy_data_to_sg(fbe_sg_element_t *sg_p, fbe_u8_t *data_p)
{
    fbe_u8_t    *sourcep;
    fbe_u32_t   bytes_copied = 0;

    // for each sg item
    while(sg_p->count != 0)
    {
        // copy the data into the space referenced from the sg address
        // image pointer to copy from 
        sourcep = data_p + bytes_copied;
        fbe_copy_memory(sg_p->address, sourcep, sg_p->count);

        // update the byte count copied
        bytes_copied += sg_p->count;

        // next sg item
        sg_p++;
    }
    return;
} // end fbe_eses_enclosure_copy_data_to_sg


/****************************************************************
 * @fn fbe_eses_enclosure_print_scsi_opcode(
 *                         fbe_u32_t flt_symptom)
 ****************************************************************
 * @brief
 *  This function prints scsi operation code
 *  enclosure.
 *
 * @param flt_symptom - opcode to be printed.
 *
 * @return
 *   opcode string
 *
 * HISTORY:
 *   05/25/2009:  Created. Nayana Chaudhari
 *
 ****************************************************************/
char* fbe_eses_enclosure_print_scsi_opcode(fbe_u32_t opcode)
{
    char * opcode_str;

    switch (opcode)
    {
        case FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION:
            opcode_str = (char *)("Get Config Page");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS:
            opcode_str = (char *)("Get Encl Status Page");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS:
            opcode_str = (char *)("Get Addl Status Page");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS:
            opcode_str = (char *)("Get EMC Specific Status Page");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS:
            opcode_str = (char *)("Get Trace Buffer Info Status Command");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS:
            opcode_str = (char *)("Get Mc Download Status Page");
            break;
        case FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL:
            opcode_str = (char *)("Set EMC Specific Control Page");
            break;
        case FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL:
            opcode_str = (char *)("Set Trace Buffer Info Control Page");
            break;
        case FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE:
            opcode_str = (char *)("Mc Download Control Page");
            break;
        case FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL:
            opcode_str = (char *)("Set Encl Control Page");
            break;
        case FBE_ESES_CTRL_OPCODE_READ_BUF:
            opcode_str = (char *)("Read Buffer Command");
            break;
        /*case FBE_ESES_CTRL_OPCODE_DEBUG_CMD:
            opcode_str = (char *)("Debug Command");
            break;*/
        case FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD:
            opcode_str = (char *)("STROUT Diag Control Page");
            break;
        case FBE_ESES_CTRL_OPCODE_MODE_SENSE:
            opcode_str = (char *)("Mode Sense Command");
            break;
        case FBE_ESES_CTRL_OPCODE_MODE_SELECT:
            opcode_str = (char *)("Mode Select Command");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA:
            opcode_str = (char *)("Get Inquiry Data Command");
            break;
        case FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY:
            opcode_str = (char *)("Validate Identity Command");
            break;
        case FBE_ESES_CTRL_OPCODE_READ_RESUME:
            opcode_str = (char *)("Read Resume PROM command");
            break;
        case FBE_ESES_CTRL_OPCODE_WRITE_RESUME:
            opcode_str = (char *)("Write Resume PROM Command");
            break;
        case FBE_ESES_CTRL_OPCODE_RAW_RCV_DIAG:
            opcode_str = (char *)("Raw Receive Diag Command");
            break;
        case FBE_ESES_CTRL_OPCODE_RAW_INQUIRY:
            opcode_str = (char *)("Raw Inquiry Command");
            break;
        case FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD:
            opcode_str = (char *)("EMC Stats Command");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS:
            opcode_str = (char *)("Get EMC Stats Status Page");
            break;
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD:
            opcode_str = (char *)("Threshold In Page");
            break;          
        case FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD:
            opcode_str = (char *)("Threshold Out Diagnostic Control Page");
            break;            
        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION:
            opcode_str = (char *)("Tunnel Get Configuration Page");
            break;            
        case FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE:
            opcode_str = (char *)("Tunnel Download Firmware");
            break;            
        case FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS:
            opcode_str = (char *)("Tunnel Get Download Status");
            break;            
        case FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS:
            opcode_str = (char *)("Receive Diag Page 0x83");
            break;  
        case FBE_ESES_CTRL_OPCODE_SPS_EEPROM_CMD:
            opcode_str = (char *)("SPS EEPROM CMD");
            break;  
        case FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE:
            opcode_str = (char *)("Get SAS Encl Type");
            break;
        case FBE_ESES_CTRL_OPCODE_GET_RP_SIZE:
            opcode_str = (char *)("Get RP Size");
            break;           
        case FBE_ESES_CTRL_OPCODE_INVALID:
        default:
            opcode_str = (char *)("Invalid Opcode");
            break;
    }
    return opcode_str;
}

/****************************************************************
 * @fn fbe_eses_enclosure_print_power_action(
 *                         fbe_u32_t power_action)
 ****************************************************************
 * @brief
 *  This function prints power action string
 *  enclosure.
 *
 * @param power_action - power action to be printed.
 *
 * @return
 *   power action string
 *
 * HISTORY:
 *   10/08/2012:  Created. Zhou Eric
 *
 ****************************************************************/
char* fbe_eses_enclosure_print_power_action(fbe_u32_t power_action)
{
    char * power_action_str;

    switch (power_action)
    {
        case FBE_DRIVE_POWER_ACTION_POWER_DOWN:
            power_action_str = (char *)("Power Down");
            break;
        case FBE_DRIVE_POWER_ACTION_POWER_UP:
            power_action_str = (char *)("Power Up");
            break;
        case FBE_DRIVE_POWER_ACTION_POWER_CYCLE:
            power_action_str = (char *)("Power Cycle");
            break;
        default:
            power_action_str = (char *)("Invalid power action");
            break;
    }
    return power_action_str;
}

/****************************************************************
 * @fn fbe_eses_enclosure_lcc_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
 *                                             fbe_u8_t prom_id,
 *                                             fbe_u32_t *index)
 ****************************************************************
 * @brief
 *  This function gets the location(index) for lcc for respective resume
 *  prom id.
 *
 * @param eses_enclosure - pointer to enclosure.
 * @param prom_id - Resume prom ID for which we want to get index.
 * @param *index - Will be filled in this function for respective resume prom ID.
 *
 * @return
 *   status : FBE_STATUS_OK incase of success.
 *
 * HISTORY:
 *   09/09/2010:  Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_eses_enclosure_lcc_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t prom_id,
                                                     fbe_u32_t *index)
{
    fbe_u8_t lcc_location = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    switch(prom_id)
    {
        case FBE_ENCLOSURE_LCC_A_RESUME_PROM:
            /* Currently we are giving hardcoded value for lcc_location.
                          We can create enum which can be shared by fbe and cm
                          for lcc_location. Will consider it for clean up. */
            lcc_location = 0;
            break;
        case FBE_ENCLOSURE_LCC_B_RESUME_PROM:
            lcc_location = 1;
            break;
        case FBE_ENCLOSURE_EE_LCC_A_RESUME_PROM:
            lcc_location = 2;           
            break;
        case FBE_ENCLOSURE_EE_LCC_B_RESUME_PROM:
            lcc_location = 3;
            break;                 
    }

    
    *index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *) eses_enclosure, 
                                                FBE_ENCL_COMP_LOCATION,
                                                FBE_ENCL_LCC,
                                                0, 
                                                lcc_location);
    if(*index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        return (FBE_STATUS_OK);
    }
    
    return status;
}
/****************************************************************
 * @fn fbe_eses_enclosure_fan_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
 *                                             fbe_u8_t prom_id,
 *                                             fbe_u32_t *index)
 ****************************************************************
 * @brief
 *  This function gets the location(index) for external fans for respective resume
 *  prom id.
 *
 * @param eses_enclosure - pointer to enclosure.
 * @param prom_id - Resume prom ID for which we want to get index.
 * @param *index - Will be filled in this function for respective resume prom ID.
 *
 * @return
 *   status : FBE_STATUS_OK incase of success.
 *
 * HISTORY:
 *   09/09/2010:  Created. Dipak Patel
 *
 ****************************************************************/
static fbe_status_t fbe_eses_enclosure_fan_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t prom_id, 
                                                     fbe_u32_t *index)
{
    fbe_u8_t fan_module = 0, edal_fan_side_id = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t cooling_subtype,container_index;
    fbe_u32_t fan_index = 0;
    fbe_u8_t max_fan = 0;
    
    switch(prom_id)
    {
        case FBE_ENCLOSURE_FAN_1_RESUME_PROM:
            fan_module = 0;
            break;
        case FBE_ENCLOSURE_FAN_2_RESUME_PROM:
            fan_module = 1;
            break;
        case FBE_ENCLOSURE_FAN_3_RESUME_PROM:
            fan_module = 2;           
            break;              
    }    

     /* Get Max Fan */
     status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure, 
                                                FBE_ENCL_COOLING_COMPONENT,
                                                &max_fan);        

    for(fan_index = 0; fan_index < max_fan; fan_index++)
    {
        status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_SUBTYPE,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &cooling_subtype);            

        /* Check if it is a external */
        if(cooling_subtype == FBE_ENCL_COOLING_SUBTYPE_STANDALONE)
        {
            status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_CONTAINER_INDEX,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &container_index);               
            

            /* Check if it is a overall element */
            if(container_index == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED)
            {

                status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_SIDE_ID,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &edal_fan_side_id);
                
                
                if(fan_module == edal_fan_side_id)
                {
                    /* Found the match */
                    *index = fan_index;
                    return (FBE_STATUS_OK);
                }             
            }
        }
    }   
    
    return status;
}


/****************************************************************
 * @fn fbe_eses_enclosure_fan_side_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
 *                                             fbe_u8_t fan_side_id,
 *                                             fbe_u32_t *index)
 ****************************************************************
 * @brief
 *  This function gets the location(index) for external fans for respective resume
 *  prom id.
 *
 * @param eses_enclosure - pointer to enclosure.
 * @param fan_side_id - fan side id for which we want to get index.
 * @param *index - Will be filled in this function for respective resume prom ID.
 *
 * @return
 *   status : FBE_STATUS_OK incase of success.
 *
 * HISTORY:
 *   09/09/2010:  Created. Dipak Patel
 *   09/01/2011: PHE - Changed fbe_eses_enclosure_fan_prom_id_to_index
 *                     to fbe_eses_enclosure_fan_side_id_to_index
 *
 ****************************************************************/
static fbe_status_t fbe_eses_enclosure_fan_side_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t fan_side_id, 
                                                     fbe_u32_t *index)
{
    fbe_u8_t edal_fan_side_id = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t cooling_subtype,container_index;
    fbe_u32_t fan_index = 0;
    fbe_u8_t max_fan = 0;

     /* Get Max Fan */
     status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure, 
                                                FBE_ENCL_COOLING_COMPONENT,
                                                &max_fan);        

    for(fan_index = 0; fan_index < max_fan; fan_index++)
    {
        status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_SUBTYPE,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &cooling_subtype);            

        /* Check if it is a external */
        if(cooling_subtype == FBE_ENCL_COOLING_SUBTYPE_STANDALONE)
        {
            status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_CONTAINER_INDEX,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &container_index);               
            

            /* Check if it is a overall element */
            if(container_index == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED)
            {

                status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                      FBE_ENCL_COMP_SIDE_ID,
                                      FBE_ENCL_COOLING_COMPONENT,
                                      fan_index,
                                      &edal_fan_side_id);
                
                
                if(fan_side_id == edal_fan_side_id)
                {
                    /* Found the match */
                    *index = fan_index;
                    return (FBE_STATUS_OK);
                }             
            }
        }
    }   
    
    return status;
}

#if 0
/****************************************************************
 * @fn fbe_eses_enclosure_verify_cabling(fbe_eses_enclosure_t * eses_enclosure,
 *                                    fbe_u8_t *illegal_cable_exist);
 ****************************************************************
 * @brief
 *  This function checks if there any cable installed in the disabled ports.
 *
 * @param eses_enclosure - IN, pointer to enclosure.
 * @param *illegal_cable_exist - OUT, connector id which has the problem,
 *                                    0xFF - no illegal cable; 
 *
 * @return
 *   status : status of the operation.
 *
 * HISTORY:
 *   03/03/2011:  Created. NCHIU
 *
 ****************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_verify_cabling(fbe_eses_enclosure_t * eses_enclosure,
                                  fbe_u8_t *illegal_cable_exist)
{
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t               connector_id = FBE_ENCLOSURE_VALUE_INVALID ;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t             is_cable_inserted = FALSE;
    fbe_bool_t             *connector_disable_list;

    *illegal_cable_exist = FBE_ENCLOSURE_VALUE_INVALID;

    // only need to check if we need to disable some port
    connector_disable_list = fbe_eses_enclosure_get_connector_disable_list_ptr(eses_enclosure);
    if (connector_disable_list!=NULL)
    {
        for (connector_id = 0; connector_id < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure) ; connector_id++)
        {
            // check for connectors to be disabled
            if (connector_disable_list[connector_id])
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

                    if (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
                    {
                        /* Check if it's inserted, 
                         * all connectors with the same connector id should have the same insert status.
                         */
                        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                            FBE_ENCL_CONNECTOR,  // Component type
                                                            matching_conn_index,     // Component index.
                                                            &is_cable_inserted);

                        if(!ENCL_STAT_OK(encl_status))
                        {
                            break;
                        }
                        if (is_cable_inserted)
                        {
                            *illegal_cable_exist = connector_id;
                            break;
                        }
                    }
                } // local connector
            }
        } // for all connectors
    }
    return encl_status;
}
#endif

/****************************************************************
 * @fn fbe_eses_enclosure_setup_connector_control(fbe_eses_enclosure_t * eses_enclosure,
 *                                    fbe_u8_t conn_id,
 *                                    fbe_bool_t disable);
 ****************************************************************
 * @brief
 *  This function set up connector control to disable/enable the
 *  phys for the specified connector.
 *
 * @param eses_enclosure - pointer to enclosure.
 * @param conn_id - connector id
 * @param disable - action, TRUE - disable; FALSE - enable
 *
 * @return
 *   status : FBE_ENCLOSURE_STATUS_OK in case of success.
 *
 * HISTORY:
 *   08/22/2012:  Created. Lin Lou
 *
 ****************************************************************/
fbe_enclosure_status_t
fbe_eses_enclosure_setup_connector_control(fbe_eses_enclosure_t * eses_enclosure,
                                     fbe_u8_t   connector_id,
                                     fbe_bool_t disable)
{
    fbe_enclosure_status_t ret_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t               phy_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t             is_entire_connector = FBE_FALSE;
    fbe_bool_t             is_local = FBE_FALSE;

    // Sanity check.
    if (connector_id >= fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure))
    {
        ret_status = FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
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
                                                        FBE_TRUE);

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
                encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_COMP_IS_LOCAL,  // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                                    matching_conn_index,     // Component index.
                                                                    &is_local);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    ret_status = encl_status;
                    break;
                }
                if (is_local == FBE_FALSE)
                {
                    // Only process local side connector.
                    break;
                }
                encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,  // Attribute.
                                    FBE_ENCL_CONNECTOR,  // Component type
                                    matching_conn_index,     // Component index.
                                    &is_entire_connector);

                if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                {
                    ret_status = encl_status;
                    break;
                }
                /* skip the whole connector.
                 * If disable is FALSE, we don't try to enable the phy which the connector is connected to.
                 * it will be evaluated by the condition FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY
                 * to see whether we can enable the phy or not.
                 */
                if (!is_entire_connector)
                {
                    encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_CONNECTOR_PHY_INDEX, // Attribute.
                                                FBE_ENCL_CONNECTOR,  // component type.
                                                matching_conn_index,
                                                &phy_index);  // The value to be set to.
                    if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                    {
                        ret_status = encl_status;
                        break;
                    }
                    // The phy index for the entire connector is set to FBE_ENCLOSURE_VALUE_INVALID.
                    // So need to check the phy index here.
                    if(phy_index != FBE_ENCLOSURE_VALUE_INVALID)
                    {
                        //Enable the phy whether it is disabled or enabled. If it is already enabled, the state will not be affected in any way.
                        encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_EXP_PHY_DISABLE,
                                                FBE_ENCL_EXPANDER_PHY,
                                                phy_index,
                                                disable);

                        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
                        {
                            ret_status = encl_status;
                        }
                    }
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

    if(ret_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "control connector failed, conn_id: %d, ret_status: 0x%x.\n",
                                 connector_id,
                                 ret_status);
    }

    return ret_status;
} // end of fbe_eses_enclosure_setup_connector_control


/*!*************************************************************************
 *   @fn fbe_eses_enclosure_get_buf_id()
 ***************************************************************************
 *
 * DESCRIPTION
 *       @brief
 *      This function gets the buffer id for the Resume Prom Device.
 *
 * PARAMETERS
 *       @deviceType (IN) - Device Type
 *       @pLocation (IN) - Location of the device.
 *       @buf_id (OUT) - Corresponding buf_id for the deviceType.
 *
 * RETURN VALUES
 *       @return fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   05-Jan-2011     Arun S - Created.
 ***************************************************************************/
fbe_status_t fbe_eses_enclosure_get_buf_id(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u64_t deviceType,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u8_t *buf_id,
                                           fbe_enclosure_resume_prom_req_type_t request_type)
{
    fbe_enclosure_component_types_t component;
    fbe_u32_t                       index = 0;
    fbe_base_enclosure_attributes_t attribute;
    fbe_u8_t                        num_ps_subelements = 0;
    fbe_status_t                    status = FBE_STATUS_OK;

    switch (deviceType)
    {
       case FBE_DEVICE_TYPE_ENCLOSURE:
       case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            component = FBE_ENCL_ENCLOSURE;
            index = 0;
            break;

        case FBE_DEVICE_TYPE_LCC:
            component = FBE_ENCL_LCC;
            index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *) eses_enclosure,
                                                FBE_ENCL_COMP_LOCATION,
                                                FBE_ENCL_LCC,
                                                0,
                                                pLocation->slot);
            if(index == FBE_ENCLOSURE_VALUE_INVALID)
            {
                return (FBE_STATUS_GENERIC_FAILURE);
            }
            break;

        case FBE_DEVICE_TYPE_PS:
            component = FBE_ENCL_POWER_SUPPLY;
            // For enclosures with more then 1 PS per side, we need to determine how many
            // subelements there are (per side) so we can skip them to get to the B side.
            //
            // For enclosures that don't support more then 1 PS, the number of sub-elements will be 0.
            // In that case we set the num_ps_subelements to 1. This allows the leaf class to ignore
            // the sub-element field. The eses class always inits this field to 0. The leaf class must
            // overwrite it with the correct value.
            //
            if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
            {
                num_ps_subelements = 1;
            }

            index = pLocation->slot * num_ps_subelements;
            
            break;

        case FBE_DEVICE_TYPE_FAN:
            component = FBE_ENCL_COOLING_COMPONENT;
            status = fbe_eses_enclosure_fan_side_id_to_index(eses_enclosure,
                                                            pLocation->slot,
                                                            &index);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, failed to get index for fan side id:%d\n",
                                    __FUNCTION__, pLocation->slot);

                return (FBE_STATUS_GENERIC_FAILURE);
            }
            break;

        case FBE_DEVICE_TYPE_SPS:
            component = FBE_ENCL_SPS;
            index = 0;
            break;

        case FBE_DEVICE_TYPE_SSC:
            component = FBE_ENCL_SSC;
            index = 0;
            break;


        default:
            // print error info
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, Invalid Device: %s\n",
                                               __FUNCTION__,
                                               fbe_base_env_decode_device_type(deviceType));

             return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    if(request_type == FBE_ENCLOSURE_WRITE_RESUME_PROM)
    {
         attribute = FBE_ENCL_BD_BUFFER_ID_WRITEABLE;
    }
    else if(request_type == FBE_ENCLOSURE_READ_RESUME_PROM)
    {
            attribute = FBE_ENCL_BD_BUFFER_ID;
    }
    else if((deviceType == FBE_DEVICE_TYPE_SPS) && (request_type == FBE_ENCLOSURE_WRITE_ACTIVE_RAM))
    {
         attribute = FBE_ENCL_BD_ACT_RAM_BUFFER_ID_WRITEABLE;
    }
    else if((deviceType == FBE_DEVICE_TYPE_SPS) && (request_type == FBE_ENCLOSURE_READ_ACTIVE_RAM))
    {
         attribute = FBE_ENCL_BD_ACT_RAM_BUFFER_ID;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure), 
                                           "%s, Invalid Req Type: %d\n", 
                                           __FUNCTION__, 
                                           request_type); 

         return FBE_STATUS_GENERIC_FAILURE;
    }

    if(deviceType == FBE_DEVICE_TYPE_SSC)
    {
        // temporary until ssc gets added to edal
        *buf_id = 2;
    }
    else if (FBE_ENCLOSURE_STATUS_OK != fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        attribute,
                                                        component,
                                                        index,
                                                        buf_id))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, Can't get buffer id, Device: %s\n",
                                           __FUNCTION__,
                                           fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, get buffer id %d, index %d, Device: %s.\n",
                                               __FUNCTION__,
                                               *buf_id, index,  
                                               fbe_base_env_decode_device_type(deviceType));
    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *   @fn fbe_eses_enclosure_get_buf_size(fbe_eses_enclosure_t *eses_enclosure,
 *                                       fbe_u64_t deviceType,
 *                                       fbe_device_physical_location_t *pLocation,
 *                                       fbe_u32_t *buf_size,
 *                                       fbe_enclosure_resume_prom_req_type_t request_type)
 ***************************************************************************
 * @brief
 *      This function gets the buffer size for the Resume Prom Device.
 *
 * @param deviceType (IN) - Device Type
 * @param pLocation (IN) - Location of the device.
 * @param pBufSize (OUT) - The pointer to the buffer size.
 * @param request_type (IN) - Request type.
 * 
 * @return fbe_status_t.
 *
 * NOTES
 *
 * HISTORY
 *   11-Dec-2013    PHE - Created.
 ***************************************************************************/
fbe_status_t fbe_eses_enclosure_get_buf_size(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u64_t deviceType,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t *pBufSize,
                                           fbe_enclosure_resume_prom_req_type_t request_type)
{
    fbe_enclosure_component_types_t component;
    fbe_u32_t                       index = 0;
    fbe_base_enclosure_attributes_t attribute;
    fbe_u8_t                        num_ps_subelements = 0;

    switch (deviceType)
    {
    case FBE_DEVICE_TYPE_PS:
            component = FBE_ENCL_POWER_SUPPLY;
            // For enclosures with more then 1 PS per side, we need to determine how many
            // subelements there are (per side) so we can skip them to get to the B side.
            //
            // For enclosures that don't support more then 1 PS, the number of sub-elements will be 0.
            // In that case we set the num_ps_subelements to 1. This allows the leaf class to ignore
            // the sub-element field. The eses class always inits this field to 0. The leaf class must
            // overwrite it with the correct value.
            //
            if ((num_ps_subelements = fbe_eses_enclosure_get_number_of_power_supply_subelem(eses_enclosure)) == 0)
            {
                num_ps_subelements = 1;
            }

            index = pLocation->slot * num_ps_subelements;
            
            break;

        default:
            // print error info
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, unhandled Device: %s\n",
                                               __FUNCTION__,
                                               fbe_base_env_decode_device_type(deviceType));

             return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    if(request_type == FBE_ENCLOSURE_READ_RESUME_PROM)
    {
            attribute = FBE_ENCL_BD_BUFFER_SIZE;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure), 
                                           "%s, Invalid Req Type: %d\n", 
                                           __FUNCTION__, 
                                           request_type); 

         return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_ENCLOSURE_STATUS_OK != fbe_base_enclosure_edal_getU32_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                        attribute,
                                                        component,
                                                        index,
                                                        pBufSize))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, Can't get buffer size, index %d, Device: %s\n",
                                           __FUNCTION__, index,
                                           fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, get buffer size %d, index %d, Device: %s.\n",
                                               __FUNCTION__,
                                               *pBufSize, index,  
                                               fbe_base_env_decode_device_type(deviceType));
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_eses_enclosure_get_expansion_port_connector_id(
 *                                     fbe_eses_enclosure_t * eses_enclosure,
 *                                     fbe_u8_t *connector_id)
 ****************************************************************
 * @brief:
 *    This function gets the expansion port connector id.
 *
 * @param  eses_enclosure (INPUT)- The pointer to the eses enclosure.
 * @param  connector_id_p (OUTPUT) - The pointer to the expansion port connector id.
 *
 * @return  fbe_enclosure_status_t
 *         FBE_ENCLOSURE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    08/21/2012: Lin Lou - Created.
 *    31-Jan-2014: PHE - Updated to support 8x configuration (2 expansion ports).
 *
 ****************************************************************/
fbe_enclosure_status_t
fbe_eses_enclosure_get_expansion_port_connector_id(fbe_eses_enclosure_t * eses_enclosure,
                                                   fbe_u8_t * connector_id_p,
                                                   fbe_u8_t max_ports)
{
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_MAPPING_FAILED;
    fbe_enclosure_status_t returnEnclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t               matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t             is_local = FBE_FALSE;
    fbe_u8_t               connector_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t               saved_connector_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u8_t               connectorType = FBE_ESES_CONN_TYPE_UNKNOWN;
    fbe_u64_t              attachedSasAddress = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t               port_count = 0;

    *connector_id_p  = FBE_ENCLOSURE_VALUE_INVALID;

    matching_conn_index =
        fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                        FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                        FBE_ENCL_CONNECTOR,  // Component type
                                        0, //starting index
                                        FBE_TRUE);

    if (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        matching_conn_index =
            fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                            FBE_ENCL_CONNECTOR,
                                            matching_conn_index,
                                            FBE_FALSE);
        port_count = 0;

        while (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
        {
            // Check if this is a local connector.
            enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                  FBE_ENCL_COMP_IS_LOCAL,
                                                                  FBE_ENCL_CONNECTOR,
                                                                  matching_conn_index,
                                                                  &is_local);

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
                break;
            }
            if (is_local == FBE_FALSE)
            {
                // Searching for local side connector finished.
                break;
            }

            // Check the connector type.
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_TYPE,
                                                                    FBE_ENCL_CONNECTOR,
                                                                    matching_conn_index,
                                                                    &connectorType);
            if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
                break;
            }
            // voyager enclosure has the connector id 0-5. 0,1,2,3 are external connectors
            // and their attachedSubenclId is FBE_ESES_SUBENCL_SIDE_ID_INVALID.
            // So we need to check whether the connector should be disabled or not.
            if (connectorType != FBE_ESES_CONN_TYPE_INTERNAL_CONN)
            {
                enclStatus = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS,
                                                                        FBE_ENCL_CONNECTOR,
                                                                        matching_conn_index,
                                                                        &attachedSasAddress);
                if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
                {
                    returnEnclStatus = enclStatus;      // save a bad status to return
                    break;
                }

                if((attachedSasAddress != 0) && (attachedSasAddress != FBE_SAS_ADDRESS_INVALID)) 
                {
                    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                        FBE_ENCL_CONNECTOR_ID,
                                                                        FBE_ENCL_CONNECTOR,
                                                                        matching_conn_index,
                                                                        &connector_id);
                    if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
                    {
                        returnEnclStatus = enclStatus;      // save a bad status to return
                        break;
                    }

                    if(connector_id != saved_connector_id) 
                    {
                        *connector_id_p = connector_id;
                        saved_connector_id = connector_id;
                        connector_id_p++;
                        port_count++;
                        if (port_count >= max_ports) break;
                    }
                }
            }
            /* Find the next expansion connector index */
            matching_conn_index =
                fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                                            FBE_ENCL_CONNECTOR,
                                                            matching_conn_index+1,
                                                            FBE_FALSE);
        }// while loop
    }

    if(returnEnclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "get expansion port connector id failed, returnEnclStatus: 0x%x.\n",
                                 returnEnclStatus);
    }

    return returnEnclStatus;
} // End of - fbe_eses_enclosure_get_expansion_port_connector_id

/*!***************************************************************
 * @fn fbe_eses_enclosure_get_connector_type(
 *                                     fbe_eses_enclosure_t * eses_enclosure,
 *                                     fbe_u8_t *connector_id)
 ****************************************************************
 * @brief:
 *    This function gets the connector type for the specified connector id.
 *
 * @param  eses_enclosure (INPUT)- The pointer to the eses enclosure.
 * @param  connectorId (INPUT) - The connector id.
 * @param  isLocalConnector (INPUT) - Is local connector or not.
 * @param  pConnectorType (OUTPUT) - The pointer to the connector type.
 * 
 * @return  fbe_enclosure_status_t
 *         FBE_ENCLOSURE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    Jan-27-2014: PHE - Created.
 *
 ****************************************************************/
fbe_enclosure_status_t
fbe_eses_enclosure_get_connector_type(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_u8_t connectorId,
                                      fbe_bool_t isLocalConnector,
                                      fbe_u8_t * pConnectorType)
{
    fbe_u8_t               number_of_components = 0;
    fbe_u32_t              connectorIndex = 0;
    fbe_u8_t               tempConnectorId = 0;
    fbe_bool_t             tempIsLocalConnector = FBE_FALSE;
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;

    *pConnectorType  = FBE_ESES_CONN_TYPE_UNKNOWN;

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_ENCL_CONNECTOR,
                                                          &number_of_components); 
    if(status != FBE_STATUS_OK)
    {
        return FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND;
    }

    for(connectorIndex = 0; connectorIndex < number_of_components; connectorIndex ++)
    {
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_CONNECTOR_ID,  // Attribute.
                                                                FBE_ENCL_CONNECTOR,  // Component type.
                                                                connectorIndex,      // Component index.
                                                                &tempConnectorId);       // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            continue;
        }

        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_IS_LOCAL,  // Attribute.
                                                                FBE_ENCL_CONNECTOR,  // Component type.
                                                                connectorIndex,      // Component index.
                                                                &tempIsLocalConnector);       // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            continue;
        }

        if((tempConnectorId == connectorId) && 
           (tempIsLocalConnector == isLocalConnector))
        {
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                  FBE_ENCL_CONNECTOR_TYPE,
                                                                  FBE_ENCL_CONNECTOR,
                                                                  connectorIndex,
                                                                  pConnectorType);
            return enclStatus;
        }
    }
    
    return FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND;
} // End of - fbe_eses_enclosure_get_connector_type

/*!***************************************************************
 * @fn fbe_eses_enclosure_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type
 *                                                   fbe_enclosure_fw_target_t * pFwTarget)
 ****************************************************************
 * @brief:
 *    This function converts the eses comp type to fw target.
 *
 * @param  eses_comp_type (INPUT)-
 * @param  pFwTarget (OUTPUT) -
 *
 * @return  fbe_status_t
 *         FBE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    06-Oct-2012: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_eses_enclosure_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type,
                                                            fbe_enclosure_fw_target_t * pFwTarget)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(eses_comp_type)
    {
        case SES_COMP_TYPE_EXPANDER_FW_EMA:
            *pFwTarget = FBE_FW_TARGET_LCC_EXPANDER;
            break;
                
        case SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW:
           *pFwTarget = FBE_FW_TARGET_LCC_BOOT_LOADER;
           break;
        
        case SES_COMP_TYPE_INIT_STR:
           *pFwTarget = FBE_FW_TARGET_LCC_INIT_STRING;
           break;
        
        case SES_COMP_TYPE_FPGA_IMAGE:
           *pFwTarget = FBE_FW_TARGET_LCC_FPGA;
           break;
    
        case SES_COMP_TYPE_PS_FW:
           *pFwTarget = FBE_FW_TARGET_PS;
           break;
    
        case SES_COMP_TYPE_LCC_MAIN: 
           *pFwTarget = FBE_FW_TARGET_LCC_MAIN;
           break;
    
        case SES_COMP_TYPE_SPS_FW: 
           *pFwTarget = FBE_FW_TARGET_SPS_PRIMARY;
           break;               
    
        case SES_COMP_TYPE_COOLING_FW:
           *pFwTarget = FBE_FW_TARGET_COOLING;
           break;      
                         
        case SES_COMP_TYPE_SPS_SEC_FW:  
           *pFwTarget = FBE_FW_TARGET_SPS_SECONDARY;
           break;
                    
        case SES_COMP_TYPE_SPS_BAT_FW:   
           *pFwTarget = FBE_FW_TARGET_SPS_BATTERY;
           break;
                   
        case SES_COMP_TYPE_OTHER_HW_VER: 
        case SES_COMP_TYPE_OTHER_FW:  
        case SES_COMP_TYPE_BBU_FW: 
        case SES_COMP_TYPE_OTHER_EXPANDER:
        default:
            *pFwTarget = FBE_FW_TARGET_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
           break; 
    }

    return status;
}


/*!*******************************************************************************
 * @fn fbe_eses_enclosure_format_fw_rev(fbe_eses_enclosure_t *eses_enclosure,
 *                                   fbe_eses_enclosure_format_fw_rev_t * pFwRevAfterFormatting,
 *                                   fbe_u8_t * pFwRev,
 *                                   fbe_u8_t fwRevSize)
 *********************************************************************************
 * @brief
 *  Format the given firmware revision string to the major, minor and patch format.
 *
 * @param  pFirmwareRevInEsesFormat(OUT) - Pointer to the PS firmware revision in major, minor and patch format.
 * @param  pFirmwareRev(IN) - Pointer to the firmware revision string.
 * @param  firmwareRevSize - Size of the firmware revision string.
 *
 * @version
 *  07-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_status_t fbe_eses_enclosure_format_fw_rev(fbe_eses_enclosure_t *eses_enclosure,
                                    fbe_eses_enclosure_format_fw_rev_t * pFwRevAfterFormatting,
                                    fbe_u8_t * pFwRev,
                                    fbe_u8_t fwRevSize)
{
    fbe_u32_t i = 0;  // loop through all the charactors in the string of firmware revision.
    fbe_u32_t dots = 0;
    fbe_u32_t returnCount = 0;
    fbe_u32_t major = 0;
    fbe_u32_t minor = 0;
    fbe_u32_t patch = 0;
    fbe_u8_t fwRevTemp[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1] = {0};

    fbe_zero_memory(pFwRevAfterFormatting, sizeof(fbe_eses_enclosure_format_fw_rev_t));

    fbe_zero_memory(&fwRevTemp[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&fwRevTemp[0], pFwRev, FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    for(i = 0; i < fwRevSize; i++)
    {
        if(fwRevTemp[i] == '.')
        {
            dots++;
        }
        else if((fwRevTemp[i] == ' ') && (i < FBE_ESES_FW_REVISION_1_0_SIZE - 1))
        {
            /* The CDES-1 firmware rev size is 5.We don't care about the byte 4.
             * It is possible that firmware rev of CDES-1 or 
             * the CDES-2 PS firmware rev (still uses CDES-1 firmware rev format)
             * has the space in the first 5 bytes. It would cause the problem while
             * calling sscanf because sscanf would skip the leading spaces.
             * So fill out 0s for the spaces the first 4 bytes if needed. 
             */ 
            fwRevTemp[i] = '0';
        }
    }

    if(dots == 0) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d%2d", &major, &minor);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = 0;
    }
    else if(dots == 1) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d.%2d", &major, &minor);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = 0;
    }
    else if(dots == 2) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d.%2d.%2d", &major, &minor, &patch);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = patch;
    }
    else
    {
        pFwRevAfterFormatting->majorNumber = 0;
        pFwRevAfterFormatting->minorNumber = 0;
        pFwRevAfterFormatting->patchNumber = 0;

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, invalid fwRev %s, revSize %d.\n", 
                             __FUNCTION__, pFwRev, fwRevSize); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_bool_t fbe_eses_enclosure_at_lower_rev(fbe_eses_enclosure_t *eses_enclosure,
 *                               fbe_u8_t * pDestFwRev,
 *                               fbe_u8_t * pSrcFwRev,
 *                               fbe_u32_t fwRevSize)
 *********************************************************************************
 * @brief
 *  This function compares two revisions (esses 2 firmware revision format).
 *  If Rev is lower than MinAcceptableRev return TRUE. Otherwise, return FALSE.
 *
 * @param  eses_enclosure - The pointer to the fbe_eses_enclosure_t. 
 * @param  pRev - the rev to check
 * @param  pMinAcceptableRev - the low rev to compare to
 * @param  fwRevSize - The firmware revision size.
 *
 * @return FBE_TRUE - MinAcceptableRev < Rev
 *         FBE_FALSE - MinAcceptableRev >= Rev
 *
 * @version
 *  13-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_bool_t fbe_eses_enclosure_at_lower_rev(fbe_eses_enclosure_t *eses_enclosure,
                                fbe_u8_t * pRev,
                                fbe_u8_t * pMinAcceptableRev,
                                fbe_u32_t revSize)
{
    fbe_eses_enclosure_format_fw_rev_t revAfterFormatting = {0};
    fbe_eses_enclosure_format_fw_rev_t minAcceptableRev = {0};

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, entry.\n", __FUNCTION__); 

    fbe_eses_enclosure_format_fw_rev(eses_enclosure, &revAfterFormatting, pRev, revSize);
    fbe_eses_enclosure_format_fw_rev(eses_enclosure, &minAcceptableRev, pRev, revSize);
                                              
    if(revAfterFormatting.majorNumber < minAcceptableRev.majorNumber) 
    {
        return FBE_TRUE;
    }
    else if((revAfterFormatting.majorNumber == minAcceptableRev.majorNumber) &&
            (revAfterFormatting.minorNumber < minAcceptableRev.minorNumber))
    {
        return FBE_TRUE;
    }
    else if((revAfterFormatting.majorNumber == minAcceptableRev.majorNumber) &&
            (revAfterFormatting.minorNumber == minAcceptableRev.minorNumber) &&
            (revAfterFormatting.patchNumber < minAcceptableRev.patchNumber)) 
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
} // End of fbe_eses_enclosure_at_lower_rev

/*!*************************************************************************
 * @fn fbe_eses_enclosure_get_buf_id_with_unavail_buf_size(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_u32_t  * fbe_u32_t)   
 ***************************************************************************
 * @brief
 *   This function gets the buffer id which does not have the buffer size available yet.
 *
 * @param     eses_enclosure (INPUT) - The pointer to the fbe_eses_enclosure_t.
 * @param     pBufInfo (OUTPUT) - The pointer to the fbe_eses_enclosure_buf_info_t.
 *
 * @return   fbe_status_t.
 *        FBE_STATUS_OK  - the target PS was found. 
 *        FBE_STATUS_COMPONENT_NOT_FOUND - the target PS was NOT found.
 *
 * NOTES
 *       It is currently for PS only. It can be expanded to other resume prom devices 
 *       in the future if needed.
 *
 * HISTORY
 *   10-Dec-2013   PHE - Created.
 ***************************************************************************/
fbe_status_t 
fbe_eses_enclosure_get_buf_id_with_unavail_buf_size(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_eses_enclosure_buf_info_t * pBufInfo)
{
    fbe_u8_t          psIndex = 0;
    fbe_u8_t          psComponentCount = 0;
    fbe_bool_t        psInserted = FBE_FALSE;
    fbe_u8_t          psSubEnclId = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u32_t         psRpSize = 0;
    fbe_u8_t          psBufId = 0;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;

    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_POWER_SUPPLY,
                                                             &psComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s, failed to get psComponentCount, status 0x%X.\n", 
                    __FUNCTION__,
                    status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    for(psIndex = 0; psIndex< psComponentCount; psIndex ++) 
    {
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SUB_ENCL_ID,
                                                               FBE_ENCL_POWER_SUPPLY,
                                                               psIndex,
                                                               &psSubEnclId);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s, psIndex %d, failed to get psSubEnclId, enclStatus 0x%X.\n", 
                    __FUNCTION__, psIndex, enclStatus);

            continue;
        }

        if(psSubEnclId == FBE_ENCLOSURE_VALUE_INVALID) 
        {
            continue;
        }

        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_INSERTED,
                                                               FBE_ENCL_POWER_SUPPLY,
                                                               psIndex,
                                                               &psInserted);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s, psIndex %d, failed to get psInserted, enclStatus 0x%X.\n", 
                    __FUNCTION__, psIndex, enclStatus);

            continue;
        }

        if(!psInserted) 
        {
            continue;
        }

        enclStatus = fbe_base_enclosure_edal_getU32_thread_safe((fbe_base_enclosure_t *) eses_enclosure,
                                           FBE_ENCL_BD_BUFFER_SIZE, 
                                           FBE_ENCL_POWER_SUPPLY, 
                                           psIndex, 
                                           &psRpSize);

        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                    "%s, psIndex %d, failed to get psRpSize, enclStatus 0x%X.\n", 
                    __FUNCTION__, psIndex, enclStatus);

            continue;
        }

        if(psRpSize == 0)
        { 
            /* Find the buffer. Get the buffer ID. */
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                   FBE_ENCL_BD_BUFFER_ID,
                                   FBE_ENCL_POWER_SUPPLY,
                                   psIndex,
                                   &psBufId);

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s, psIndex %d, failed to get psRpSize, status 0x%X.\n", 
                        __FUNCTION__, psIndex, enclStatus);

                continue;
            }
            else
            {
                pBufInfo->componentType = FBE_ENCL_POWER_SUPPLY;
                pBufInfo->componentIndex = psIndex;
                pBufInfo->bufId = psBufId;

                return FBE_STATUS_OK;
            }
        }
    } // End of for loop

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}


/*!*************************************************************************
* @fn fbe_status_t fbe_eses_enclosure_is_eeprom_resume_valid(fbe_eses_enclosure_t * eses_enclosure,
*                                               dev_eeprom_rev0_info_struct * pEepromRev0,
*                                               fbe_bool_t * pResumeValid)              
***************************************************************************
* @brief
*       This function checks the resume in the EEPROM is valid or not.
*       Whether it is valid or not is based on the serial number. 
*
* @param       eses_enclosure - The pointer to the ESES enclosure.
* @param       pEepromRev0 - the pointer to dev_eeprom_rev0_info_struct.
* @param       pResumeValid (OUTPUT)- 
* 
* @return  fbe_status_t.
*      FBE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   22-Jan-2014     PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_is_eeprom_resume_valid(fbe_eses_enclosure_t * eses_enclosure,
                                                dev_eeprom_rev0_info_struct * pEepromRev0,
                                                fbe_bool_t * pResumeValid)
{
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    *pResumeValid = FBE_TRUE;

    if(pEepromRev0->emc_serialnum[0] == 0)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s SN not available.\n", __FUNCTION__ );

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s SP Family FRU ID: 0x%X.\n",
                            __FUNCTION__,
                            pEepromRev0->ffid);

        *pResumeValid = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_reformat_resume(fbe_eses_enclosure_t * eses_enclosure,
*                                        RESUME_PROM_STRUCTURE * pEmcStandardRp,
*                                        dev_eeprom_rev0_info_struct * pEepromRev0)               
***************************************************************************
* @brief
*       This function convert the EEPROM data into the EMC standard Resume Prom data.
*
* @param       eses_enclosure - The pointer to the ESES enclosure.
* @param       pEmcStandardRp - pointer to RESUME_PROM_STRUCTURE
* @param       pEepromRev0 - the pointer to dev_eeprom_rev0_info_struct.
*
* @return  fbe_status_t.
*      FBE_STATUS_OK - no error.
*      otherwise - error is found.
*
* NOTES
*
* HISTORY
*   10-Dec-2013     PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_enclosure_reformat_resume(fbe_eses_enclosure_t * eses_enclosure,
                                      RESUME_PROM_STRUCTURE * pEmcStandardRp,
                                      dev_eeprom_rev0_info_struct * pEepromRev0)
{
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s entry \n", __FUNCTION__ );

    fbe_copy_memory(&pEmcStandardRp->sp_family_fru_id[0],
                    &pEepromRev0->ffid,
                    RESUME_PROM_SP_FAMILY_FRU_ID_SIZE);

    fbe_copy_memory(&pEmcStandardRp->emc_tla_serial_num[0],
                    &pEepromRev0->emc_serialnum[0],
                    RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

    fbe_copy_memory(&pEmcStandardRp->emc_tla_part_num[0],
                    &pEepromRev0->emc_partnum[0],
                    RESUME_PROM_EMC_TLA_PART_NUM_SIZE);

    fbe_copy_memory(&pEmcStandardRp->emc_tla_assembly_rev[0],
                    &pEepromRev0->emc_assembly_revision[0],
                    RESUME_PROM_EMC_TLA_ASSEMBLY_REV_SIZE);

    fbe_copy_memory(&pEmcStandardRp->year_mft[0],
                    &pEepromRev0->mfg_date[0],
                    RESUME_PROM_YEAR_MANUFACTURE_SIZE);

    fbe_copy_memory(&pEmcStandardRp->month_mft[0],
                    &pEepromRev0->mfg_date[RESUME_PROM_YEAR_MANUFACTURE_SIZE],
                    RESUME_PROM_MONTH_MANUFACTURE_SIZE);

    fbe_copy_memory(&pEmcStandardRp->day_mft[0],
                    &pEepromRev0->mfg_date[(RESUME_PROM_YEAR_MANUFACTURE_SIZE + RESUME_PROM_MONTH_MANUFACTURE_SIZE)],
                    RESUME_PROM_DAY_MANUFACTURE_SIZE);

    fbe_copy_memory(&pEmcStandardRp->vendor_assembly_rev[0],
                    &pEepromRev0->vendor_revision[0],
                    RESUME_PROM_VENDOR_ASSEMBLY_REV_SIZE);

    fbe_copy_memory(&pEmcStandardRp->vendor_part_num[0],
                    &pEepromRev0->vendor_part_number[0],
                    RESUME_PROM_VENDOR_PART_NUM_SIZE);

    fbe_copy_memory(&pEmcStandardRp->loc_mft[0],
                    &pEepromRev0->mfg_location[0],
                    RESUME_PROM_LOCATION_MANUFACTURE_SIZE);

    fbe_copy_memory(&pEmcStandardRp->vendor_serial_num[0],
                    &pEepromRev0->vendor_serialnum[0],
                    RESUME_PROM_VENDOR_SERIAL_NUM_SIZE);

    fbe_copy_memory(&pEmcStandardRp->vendor_name[0],
                    &pEepromRev0->vendor_name[0],
                    RESUME_PROM_VENDOR_NAME_SIZE);

    pEmcStandardRp->resume_prom_checksum = calculateResumeChecksum(pEmcStandardRp);

    return FBE_STATUS_OK;
}


/*!*******************************************************************************
 * @fn fbe_eses_enclosure_convert_string_to_hex
 *********************************************************************************
 * @brief
 *  This function converts ascii to hex but only uses 0-9 and Aa-Za.
 *
 * @param  string - the pointer to the string.
 *
 * @return fbe_u32_t - the output hex.
 *
 * @version
 *  22-June-2010 PHE -- Created.
 *******************************************************************************/
fbe_u32_t fbe_eses_enclosure_convert_string_to_hex(char * string)
{
    fbe_u32_t output = 0;
    fbe_u32_t count = 0;
    fbe_u32_t diff = 0;

    while (count< strlen(string))
    {
        if (string[count] == 0)
        {
            // quit if 0 encountered
            break;
        }
        if (((string[count] >= '0') && (string[count] <= '9')) ||
            ((string[count] >= 'A') && (string[count] <= 'Z')) ||
            ((string[count] >= 'a') && (string[count] <= 'z')))
        {
            if((string[count] >= '0') && (string[count] <= '9'))
            {
                diff = 0x30;
            }
            else if ((string[count] >= 'A') && (string[count] <= 'Z'))
            {
                diff = 0x37;
            }
            else
            {
                diff = 0x57;
            }
            output <<= 4;
            output += (string[count] - diff);
        }
        count++;
    }
    return(output);
} // fbe_eses_enclosure_convert_string_to_hex

// End of file fbe_eses_enclosure_utils.c

