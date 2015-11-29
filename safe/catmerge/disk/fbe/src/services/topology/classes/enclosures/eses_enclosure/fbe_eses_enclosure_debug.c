/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_debug.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the debugging related functions.
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
#include "fbe_eses_enclosure_debug.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "fbe/fbe_enclosure.h"

/**************************************************************************
*          fbe_eses_enclosure_xlate_eses_comp_type()                  
***************************************************************************
* DESCRIPTION
*   Translate the version descriptor component type field into a
*   fbe_enclosure_fw_target_t type.
*
* PARAMETERS
*   component (in) - version descriptor component enum
*   fw_target (out) - fbe eses target type (used by shim)
*
* RETURN VALUES
*   fbe_status_t - failure when target not found
*
* HISTORY
*   09-Oct-208 gb - Created
***************************************************************************/
fbe_status_t fbe_eses_enclosure_xlate_eses_comp_type(ses_comp_type_enum component,
                                                         fbe_enclosure_fw_target_t * fw_target)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(component)
    {
    case SES_COMP_TYPE_EXPANDER_FW_EMA :
        *fw_target = FBE_FW_TARGET_LCC_EXPANDER;
        break;
    case SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW :
        *fw_target = FBE_FW_TARGET_LCC_BOOT_LOADER;
        break;
    case SES_COMP_TYPE_INIT_STR :
        *fw_target = FBE_FW_TARGET_LCC_INIT_STRING;
        break;
    case SES_COMP_TYPE_FPGA_IMAGE :
        *fw_target = FBE_FW_TARGET_LCC_FPGA;
        break;
    case SES_COMP_TYPE_PS_FW :
        *fw_target = FBE_FW_TARGET_PS;
        break;
    case SES_COMP_TYPE_LCC_MAIN :
        *fw_target = FBE_FW_TARGET_LCC_MAIN;
        break;
    case SES_COMP_TYPE_COOLING_FW :
        *fw_target = FBE_FW_TARGET_COOLING;
        break;
    case SES_COMP_TYPE_SPS_FW :
        *fw_target = FBE_FW_TARGET_SPS_PRIMARY;
        break;
    case SES_COMP_TYPE_SPS_SEC_FW :
        *fw_target = FBE_FW_TARGET_SPS_SECONDARY;
        break;
    case SES_COMP_TYPE_SPS_BAT_FW :
        *fw_target = FBE_FW_TARGET_SPS_BATTERY;
        break;
    default:
        // Check if something which is unknown.
        // not supported
        *fw_target = FBE_FW_TARGET_INVALID;
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;
} // fbe_eses_enclosure_xlate_eses_comp_type


/*!*************************************************************************
 * @fn fbe_eses_printSendControlPage
 *                    fbe_eses_enclosure_t *eses_enclosure,
 *                    fbe_u8_t *controlPagePtr)
 **************************************************************************
 *
 *  @brief
 *      This function print/trace the contents of an ESES Control buffer
 *      (only set elements/components)
 *
 *  @param    eses_enclosure - pointer to an ESES enclosure object
 *  @param    controlPagePtr - pointer to an ESES Control buffer
 *
 *  @return   None
 *
 *  NOTES:
 *
 *  HISTORY:
 *    16-Dec-2008: Joe Perry - Created
 *
 **************************************************************************/
void fbe_eses_printSendControlPage(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_u8_t *controlPagePtr)
{
    ses_common_pg_hdr_struct    *headerPtr;
    ses_cmn_ctrl_struct         *commonControlPtr;
    ses_ctrl_elem_array_dev_slot_struct *driveControlPtr;
    ses_ctrl_elem_exp_phy_struct        *phyControlPtr;
    ses_ctrl_elem_encl_struct           *lccControlPtr;
    ses_ctrl_elem_encl_struct           *enclControlPtr;
    ses_ctrl_elem_display_struct        *displayControlPtr;
    ses_ctrl_elem_sas_conn_struct       *ConnectorControlPtr ;
    ses_ps_info_elem_struct             *psControlPtr;
    fbe_u8_t                    maxNumberOfComponents;
    fbe_enclosure_status_t      enclStatus;
    fbe_status_t                status;
    fbe_u8_t                    elemIndex;
    fbe_u32_t                   index;
    fbe_u16_t                   bufferOffset;
    fbe_enclosure_number_t      enclosure_number = 0;
    fbe_enclosure_component_types_t componentType;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)eses_enclosure, 
                                            &enclosure_number);

    headerPtr = (ses_common_pg_hdr_struct *)controlPagePtr;
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "prntSndCntrlPg: CntrlPgHdr: PgCd 0x%x, PgLen 0x%x, GenCd 0x%x\n", 
                           headerPtr->pg_code,
                           fbe_eses_get_pg_size(headerPtr),
                           fbe_eses_get_pg_gen_code(headerPtr));

    // Parse the rest of the buffer looking for valid data

    // loop through all component types
    for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)
    {
        status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                        componentType, 
                                                        &maxNumberOfComponents);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "prntSndCntrlPg, Error on fbe_edal_getSpecificComponentCount, status %d\n",
                                status);
            return;
        }
        // loop through all components
        for (index = 0; index < maxNumberOfComponents; index++)
        {
            enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t*)eses_enclosure,
                                                       FBE_ENCL_COMP_ELEM_INDEX,
                                                       componentType,
                                                       index,
                                                       &elemIndex);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "prntSndCntrlPg, Error on fbe_base_enclosure_edal_getU8, enclStatus %d\n",
                                      enclStatus);
                return;
            }

            if(elemIndex == SES_ELEM_INDEX_NONE) 
            {
                /* The overall element does not have the element index.
                 * Its default value is 255. So move on to the next component.
                 */
                continue;
            }

            bufferOffset = fbe_eses_elem_index_to_byte_offset(eses_enclosure, elemIndex);
            commonControlPtr = (ses_cmn_ctrl_struct *)(controlPagePtr + bufferOffset);
            if (commonControlPtr->select)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                      FBE_TRACE_LEVEL_INFO,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "prntSndCntrlPg, %s, index %d\n",
                                      enclosure_access_printComponentType(componentType), 
                                      index);
                switch (componentType)
                {
                case FBE_ENCL_DRIVE_SLOT:
                    driveControlPtr = (ses_ctrl_elem_array_dev_slot_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "  RqstFlt %d, RqstIdent %d, DevOff %d\n", 
                                          driveControlPtr->rqst_fault,
                                          driveControlPtr->rqst_ident,
                                          driveControlPtr->dev_off);
                    break;
                case FBE_ENCL_EXPANDER_PHY:
                    phyControlPtr = (ses_ctrl_elem_exp_phy_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "  Disable : %d\n", phyControlPtr->cmn_stat.disable);
                    break;
                case FBE_ENCL_LCC:
                    lccControlPtr = (ses_ctrl_elem_encl_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "  RqstFail %d, RqstId %d, PwrCycle Rqst %d, Delay %d, PwrOffDur %d\n", 
                                          lccControlPtr->rqst_failure,
                                          lccControlPtr->rqst_ident,
                                          lccControlPtr->power_cycle_rqst,
                                          lccControlPtr->power_cycle_delay,
                                          lccControlPtr->power_off_duration);
                    break;
                case FBE_ENCL_ENCLOSURE:
                    enclControlPtr = (ses_ctrl_elem_encl_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "  RqstFail %d, RqstIdent %d\n", 
                                          enclControlPtr->rqst_failure,
                                          enclControlPtr->rqst_ident);
                    break;
                case FBE_ENCL_DISPLAY:
                    displayControlPtr = (ses_ctrl_elem_display_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "  Display Mode %d, Char %d (%c), Ident %d\n", 
                                          displayControlPtr->display_mode,
                                          displayControlPtr->display_char, 
                                          (displayControlPtr->display_char == 0) ? '0' : displayControlPtr->display_char,
                                          displayControlPtr->rqst_ident);

                    // Temporary trace to track down DIMS 239447
                    if (displayControlPtr->display_char == 0 || displayControlPtr->display_mode == 0) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "%s DISPLAY_ERROR: post setup, Mode is 0x%x Display Char 0x%x\n",
                                          __FUNCTION__, displayControlPtr->display_mode, displayControlPtr->display_char);
                    }
                    break;
                 case FBE_ENCL_CONNECTOR:
                    ConnectorControlPtr = (ses_ctrl_elem_sas_conn_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "Clear swap bit %d ,Request Ident %d, Request Fail %d ", 
                                          ConnectorControlPtr->cmn_stat.rst_swap,
                                          ConnectorControlPtr->rqst_ident,
                                          ConnectorControlPtr->rqst_fail );
                    break ;    
                case FBE_ENCL_POWER_SUPPLY:
                    psControlPtr = (ses_ps_info_elem_struct *)commonControlPtr;
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                          FBE_TRACE_LEVEL_INFO,
                                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                          "PS Margin Test Mode 0x%x", 
                                          psControlPtr->margining_test_mode);
                    break ;    
                case FBE_ENCL_COOLING_COMPONENT:
                case FBE_ENCL_TEMP_SENSOR:
                   break ;
                case FBE_ENCL_EXPANDER:
                default:
                    break;
                }
            }
        }  // index loop
    }  // all types
}   // end of fbe_eses_printSendControlPage


/*!*************************************************************************
 * @fn printElemGroupInfo
 *                    fbe_eses_enclosure_t *eses_enclosure)
 **************************************************************************
 *
 *  @brief
 *      This function print/trace the Element Group info off an Enclosure.
 *
 *  @param    eses_enclosure - pointer to an ESES enclosure object
 *
 *  @return   None
 *
 *  NOTES:
 *
 *  HISTORY:
 *    16-Dec-2008: Joe Perry - Created
 *
 **************************************************************************/
void printElemGroupInfo(fbe_eses_enclosure_t *eses_enclosure)
{
    fbe_eses_elem_group_t   *elemGroupPtr;
    fbe_u32_t               index;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "Element Group Info :\n" );

    // loop through the entries
    elemGroupPtr = fbe_eses_enclosure_get_elem_group_ptr(eses_enclosure); 
    for (index = 0; index < fbe_eses_enclosure_get_number_of_actual_elem_groups(eses_enclosure); index++)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "   index %d\n", index);
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "  elem_type          : 0x%x\n", elemGroupPtr->elem_type);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "  subencl_id         : 0x%x\n", elemGroupPtr->subencl_id);
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "  first_elem_index   : 0x%x\n", elemGroupPtr->first_elem_index);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "  num_possible_elems : %d\n", elemGroupPtr->num_possible_elems);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "  byte_offset        : 0x%x\n", elemGroupPtr->byte_offset);

        elemGroupPtr++;
        if (elemGroupPtr->byte_offset == 0)
        {
            break;
        }
    }
}   // end of printElemGroupInfo

/*!*************************************************************************
 * @fn fbe_eses_enclosure_fw_targ_to_text
 *           (fbe_enclosure_fw_target_t fw_target)
 **************************************************************************
 *
 *  @brief
 *  This function will translate fbe_enclosure_fw_target_t to string.
 *
 *  @param    fbe_enclosure_fw_target_t
 *
 *  @return     char * - string
 *
 *  HISTORY:
 *    17-Mar-2009: GB- created.
 **************************************************************************/
char * fbe_eses_enclosure_fw_targ_to_text(fbe_enclosure_fw_target_t fw_target)
{
    char * fwTargString;

    switch(fw_target)
    {
    case FBE_FW_TARGET_LCC_EXPANDER:
        fwTargString = "LCC EXPANDER";
        break;	
    
    case FBE_FW_TARGET_LCC_BOOT_LOADER:
        fwTargString = "BOOT LOADER";
        break;	
   
    case FBE_FW_TARGET_LCC_INIT_STRING:
        fwTargString = "INIT STRING";
        break;

    case FBE_FW_TARGET_LCC_FPGA:
        fwTargString = "CDEF";
        break;
    
    case FBE_FW_TARGET_PS:
        fwTargString = "PS";
        break;	
   
    case FBE_FW_TARGET_COOLING:
        fwTargString = "FAN";
        break;	

    case FBE_FW_TARGET_LCC_MAIN:
        fwTargString = "MAIN";
        break;	

    case FBE_FW_TARGET_SPS_PRIMARY:
        fwTargString = "SPS PRIMARY";
        break;

    case FBE_FW_TARGET_SPS_SECONDARY:
        fwTargString = "SPS SECONDARY";
        break;

    case FBE_FW_TARGET_SPS_BATTERY:
        fwTargString = "SPS BATTERY";
        break;

    case FBE_FW_TARGET_INVALID:
        fwTargString = "INVALID";
        break;

    default:
        // not supported
        fwTargString = "None";
        break;
    }
    return(fwTargString);
} // fbe_eses_enclosure_fw_targ_to_text

/*!*************************************************************************
 * @fn fbe_eses_enclosure_fw_op_to_text
 *           (fbe_enclosure_firmware_opcode_t fw_op)
 **************************************************************************
 *
 *  @brief
 *  This function will translate fbe_enclosure_firmware_opcode_t to string.
 *
 *  @param    fbe_enclosure_firmware_opcode_t
 *
 *  @return     char * - string
 *
 *  HISTORY:
 *    17-Mar-2009: GB- created.
 **************************************************************************/
char * fbe_eses_enclosure_fw_op_to_text(fbe_enclosure_firmware_opcode_t fw_op)
{
    char * fwOpStr;

    switch(fw_op)
    {
    case FBE_ENCLOSURE_FIRMWARE_OP_NONE :
        fwOpStr = "NONE";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD :
        fwOpStr = "DOWNLOAD";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE :
        fwOpStr = "ACTIVATE";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_ABORT :
        fwOpStr = "ABORT";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS :
        fwOpStr = "GET STATUS";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION :
        fwOpStr = "NOTIFY COMPLETION";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION :
        fwOpStr = "GET PERMISSION";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION :
        fwOpStr = "RETURN PERMISSION";
        break;

    default:
        // not supported
        fwOpStr = "Not Supported";
        break;
    }
    return(fwOpStr);
} // fbe_eses_enclosure_fw_op_to_text

/*!*************************************************************************
 * @fn fbe_eses_enclosure_fw_status_to_text
 *           (fbe_enclosure_firmware_status_t dl_status)
 **************************************************************************
 *
 *  @brief
 *  This function will translate fbe_enclosure_firmware_status_t to string.
 *
 *  @param    fbe_enclosure_firmware_status_t
 *
 *  @return     char * - string
 *
 *  HISTORY:
 *    17-Mar-2009: GB- created.
 **************************************************************************/
char * fbe_eses_enclosure_fw_status_to_text(fbe_enclosure_firmware_status_t dl_status)
{
    char * statusStr;

    switch(dl_status)
    {
    case FBE_ENCLOSURE_FW_STATUS_NONE :
        statusStr = "NONE";
        break;
    case FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS :
        statusStr = "IN PROGRESS";
        break;
    case FBE_ENCLOSURE_FW_STATUS_FAIL :
        statusStr = "FAIL";
        break;
    case FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED :
        statusStr = "DOWNLOAD FAILED";
        break;
    case FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED :
        statusStr = "ACTIVATE FAILED";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_ABORT :
        statusStr = "ABORT";
        break;
    default:
        // not supported
        statusStr = "Not Supported";
        break;
    }
    return(statusStr);
} // fbe_eses_enclosure_fw_status_to_text


/*!*************************************************************************
 * @fn fbe_eses_enclosure_fw_extstatus_to_text
 *           (fbe_enclosure_firmware_ext_status_t dl_extstatus)
 **************************************************************************
 *
 *  @brief
 *  This function will translate fbe_enclosure_firmware_ext_status_t to string.
 *
 *  @param    fbe_enclosure_firmware_ext_status_t
 *
 *  @return     char * - string
 *
 *  HISTORY:
 *    17-Mar-2009: GB- created.
 **************************************************************************/
char * fbe_eses_enclosure_fw_extstatus_to_text(fbe_enclosure_firmware_ext_status_t dl_extstatus)
{
    char * extStatusStr;

    switch( dl_extstatus)
    {
    case FBE_ENCLOSURE_FW_EXT_STATUS_NONE :
        extStatusStr = "NONE";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_REQ :
        extStatusStr = "STATUS REQUESTED";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED :
        extStatusStr = "IMAGE LOADED";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN :
        extStatusStr = "UNKNOWN";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_NO_ABORT :
        extStatusStr = "ABORT";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_BUSY :
        extStatusStr = "BUSY";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE :
        extStatusStr = "PAGE ERROR";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM :
        extStatusStr = "CHECKSUM ERROR";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_TO :
        extStatusStr = "TIME OUT";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_ACTIVATE :
        extStatusStr = "ACTIVATE ERROR";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_NO_IMAGE :
        extStatusStr = "NO IMAGE";
        break;
    case FBE_ENCLOSURE_FW_EXT_STATUS_ERR_LFC :
        extStatusStr = "LIFECYCLE ERROR";
        break;
    default:
        // not supported
        extStatusStr = "Not Supported";
        break;
    }
    return(extStatusStr);
} // fbe_eses_enclosure_fw_extstatus_to_text

// End of file fbe_eses_enclosure_debug.c
