/***************************************************************************
 * Copyright (C) EMC Corporation 2008 - 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_enclosure_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the enclosure object.
 *
 * @ingroup fbe_cli
 *
 * HISTORY
 *   11/11/2008:  bphilbin - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_eses.h"
#include "../fbe/src/lib/enclosure_data_access/edal_base_enclosure_data.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_utils.h"
#include "../fbe/interface/fbe_base_object_trace.h"
#include "../fbe/interface/fbe_base_enclosure_debug.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_limits.h"
#include "csx_ext.h"
#define FBE_ESES_PAGE_SIZE 4     //since min eses page size is set to 4 by default
#define FBE_STRING_IN_CMD_WAIT_TIME 3000 //3 sec timeout for string in command before giving up re-try.
#define FBE_STRING_IN_CMD_INTERVAL_TIME 250 //250ms interval for string in cmd retry.

#define FBE_ENCL_COMPONENT_ID_ALL 0xffff

#define FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX 5


static char * fbe_cli_encl_decode_encl_cmd_type(fbe_cli_encl_cmd_type_t encl_cmd_type);
static fbe_status_t fbe_cli_encl_list_all_trace_buf(fbe_object_id_t object_id);
static fbe_status_t fbe_cli_scsi_list_all_vpd_pages(fbe_object_id_t object_id);
static fbe_status_t fbe_cli_encl_print_trace_buf_status(fbe_u8_t * response_buf_p, fbe_u32_t response_buf_size);
static fbe_status_t fbe_cli_encl_trace_buf_ctrl(fbe_object_id_t object_id, fbe_u8_t buf_id, fbe_enclosure_mgmt_trace_buf_op_t buf_op);
static fbe_status_t fbe_cli_encl_get_buf_size(fbe_object_id_t object_id, 
                                              fbe_u8_t buf_id, 
                                              fbe_u32_t * buf_capacity_p,
                                              fbe_enclosure_status_t *enclosure_status);
static fbe_status_t fbe_cli_encl_save_buf_data_to_file(fbe_object_id_t object_id, fbe_u8_t buf_id,
        const char * filename, fbe_u8_t output_to_file, char *temp);
static fbe_status_t fbe_cli_encl_send_raw_rcv_diag_cmd(fbe_object_id_t object_id, 
                                                       fbe_enclosure_raw_rcv_diag_pg_t raw_diag_page,
                                                       fbe_enclosure_status_t *enclosure_status);
static fbe_status_t fbe_cli_encl_send_raw_scsi_cmd(fbe_object_id_t object_id, 
                                                   fbe_u8_t evpd, 
                                                   fbe_u8_t raw_inquiry_vpd_pg,
                                                   fbe_enclosure_status_t *enclosure_status);
static void fbe_cli_print_bytes(char *data, fbe_u8_t size);

fbe_base_enclosure_led_behavior_t
convertStringToLedAction(const char *ledActionString);
char *
convertLedActionToString(const fbe_base_enclosure_led_behavior_t ledAction);

static fbe_status_t fbe_cli_encl_get_slot_to_phy_mapping(fbe_object_id_t object_id, 
                                                    fbe_enclosure_slot_number_t slot_num_start, 
                                                    fbe_enclosure_slot_number_t slot_num_end,
                                                    fbe_enclosure_status_t *enclosure_status);
static fbe_status_t fbe_cli_encl_print_slot_to_phy_mapping(fbe_enclosure_slot_number_t slot_num_start,
                                                           fbe_enclosure_slot_number_t slot_num_end,
                                                           fbe_u8_t * response_buf_p, 
                                                           fbe_u32_t response_buf_size);
void fbe_cli_printFwRevs(fbe_edal_block_handle_t baseCompBlockPtr);
void fbe_cli_printSpecificDriveData(fbe_edal_block_handle_t baseCompBlockPtr);    
fbe_edal_status_t fbe_cli_fill_enclosure_drive_info(fbe_edal_block_handle_t baseCompBlockPtr,slot_flags_t *driveStat,fbe_u32_t *component_count);
void fbe_cli_print_drive_info(slot_flags_t *driveStat, fbe_u8_t max_slot );
void fbe_cli_print_drive_Led_info(fbe_led_status_t *pEnclDriveFaultLeds, fbe_u8_t max_slot );
void fbe_cli_print_enclstatinfo(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids);
static fbe_status_t
fbe_cli_encl_send_string_in_cmd(fbe_object_id_t object_id,
                                fbe_enclosure_status_t *enclosure_status,
                                fbe_u32_t *response_buf_size,
                                fbe_bool_t printFlag);
static void fbe_cli_print_string(char *data, fbe_u8_t size);
static fbe_status_t fbe_cli_get_threshold_in(fbe_object_id_t object_id,
                                             fbe_enclosure_component_types_t componentType);
void fbe_cli_display_enclosure_threshold_in( fbe_enclosure_component_types_t componentType,
                                            fbe_u32_t response_data_size,
                                            fbe_u8_t *sourcep);
static void fbe_cli_send_exp_cmds(fbe_u32_t bus, fbe_u32_t encl_pos, fbe_u32_t component_id, fbe_u32_t object_handle);
fbe_status_t   fbe_cli_fire_enclbufcommands(
                                            fbe_object_id_t object_id,
                                            fbe_cli_encl_cmd_type_t encl_cmd_type,
                                            fbe_u32_t port,
                                            fbe_u32_t encl_pos, fbe_u32_t comp_id);
static fbe_status_t fbe_cli_encl_print_e_log(fbe_object_id_t object_id, fbe_u8_t buf_id, char *temp);
static fbe_status_t
read_and_output_buffer(fbe_u8_t buf_id, fbe_u32_t buf_capacity, fbe_object_id_t object_id, FILE *fp, const char *filename);
static void fbe_cli_cmd_encledal_print_encl_data(fbe_s32_t specified_port,
                                          fbe_s32_t specified_encl_pos,
                                          fbe_s32_t specified_componentId,
                                          fbe_bool_t  ComponentIdFlag,
                                          fbe_enclosure_component_types_t specified_componentType,
                                          fbe_u32_t specified_componentIndex,
                                          fbe_bool_t faultDataRequested);
static void fbe_cli_cmd_encledal_print_pe_data(fbe_enclosure_component_types_t specified_componentType,
                                        fbe_u32_t specified_componentIndex);

/*!*************************************************************************
 * @fn fbe_cli_cmd_display_encl_data
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function will output the enclosure data blob for the specified
 *      enclosure.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Nov-2008: bphilbin - created.
 *
 **************************************************************************/
void fbe_cli_cmd_display_encl_data(int argc , char ** argv)
{
    fbe_s32_t specified_port = -1;
    fbe_s32_t specified_encl_pos = -1;
    fbe_s32_t specified_componentId = -1;
    fbe_enclosure_component_types_t specified_componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u32_t specified_componentIndex = 0;
    fbe_bool_t      faultDataRequested = FBE_FALSE;
    fbe_bool_t print_pe_data = FBE_FALSE;
    fbe_bool_t print_encl_data = FBE_FALSE;
    fbe_bool_t  ComponentIdFlag = FALSE;

    if (argc > 9) 
    {
        fbe_cli_error("%s", ENCL_EDAL_USAGE);
        return;
    }
    if (argc == 0)
    {
        // Special case where we will print everything. Make sure we print 
        // subenclosures as well (i.e. -cid all)
        specified_componentId = FBE_ENCL_COMPONENT_ID_ALL;
        ComponentIdFlag = TRUE;
        print_encl_data = TRUE;
        print_pe_data = TRUE;
    }

    /* Parse the command line. */
    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            specified_port = (fbe_u32_t)strtoul(*argv, 0, 0);
            print_encl_data = TRUE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            specified_encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
            print_encl_data = TRUE;
        }
        else if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            fbe_cli_printf("%s", ENCL_EDAL_USAGE);
            return;
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            if(strcmp(*argv, "all") == 0)
            {
                specified_componentId = FBE_ENCL_COMPONENT_ID_ALL;
            }
            else
            {
                specified_componentId = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            ComponentIdFlag = TRUE;
            print_encl_data = TRUE;
        }
        else if(strcmp(*argv, "-pe") == 0)
        {
            print_pe_data = TRUE;
            if(argc == 2 || argc > 4)
            {
                fbe_cli_error("Invalid arguments\n %s \n",ENCL_EDAL_USAGE);
                return;
            }
        }
        /* process Component Type & index */
        else if(strcmp(*argv, "-c") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            specified_componentType = fbe_cli_convertStringToComponentType(argv);
            if(specified_componentType == FBE_ENCL_INVALID_COMPONENT_TYPE)
            {
                fbe_cli_error("Invalid component type\n %s", ENCL_EDAL_USAGE);
                return;
            }
            // check for index (default to all if none specified)
            argc--;
            argv++;
            if(argc == 0)
            {
                specified_componentIndex = FBE_ENCL_COMPONENT_INDEX_ALL;
            }
            else
            {
                specified_componentIndex = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
        }
        else if(strcmp(*argv, "-fault") == 0)
        {
            argc--;
            argv++;
            faultDataRequested = TRUE;
            print_encl_data = TRUE;
        }
        else if(strcmp(*argv, "-a") == 0)
        {
            print_pe_data = TRUE;
            print_encl_data = TRUE;
        }
        else
        {
            fbe_cli_error("Invalid arguments\n %s", ENCL_EDAL_USAGE);
            return;
        }

        argc--;
        argv++;

    }   // end of while

   
    if(print_encl_data) 
    {
        fbe_cli_cmd_encledal_print_encl_data(specified_port,
                                            specified_encl_pos,
                                            specified_componentId,
                                            ComponentIdFlag,
                                            specified_componentType,
                                            specified_componentIndex,
                                            faultDataRequested);
    }

    if(print_pe_data) 
    {
        fbe_cli_cmd_encledal_print_pe_data(specified_componentType,
                                           specified_componentIndex);
    }

    return;
}

/*!*************************************************************************
 * @fn fbe_cli_cmd_encledal_print_encl_data(fbe_s32_t specified_port,
 *                                          fbe_s32_t specified_encl_pos,
 *                                          fbe_s32_t specified_componentId,
 *                                          fbe_bool_t  ComponentIdFlag,
 *                                          fbe_enclosure_component_types_t specified_componentType,
 *                                          fbe_u32_t specified_componentIndex,
 *                                          fbe_bool_t faultDataRequested)
 **************************************************************************
 *
 *  @brief
 *      This function will output the EDAL data reported by the enclosure objects.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Nov-2008: bphilbin - created.
 *    10-May-2013: PHE - Moved from fbe_cli_cmd_display_encl_data
 *
 **************************************************************************/
static void fbe_cli_cmd_encledal_print_encl_data(fbe_s32_t specified_port,
                                          fbe_s32_t specified_encl_pos,
                                          fbe_s32_t specified_componentId,
                                          fbe_bool_t  ComponentIdFlag,
                                          fbe_enclosure_component_types_t specified_componentType,
                                          fbe_u32_t specified_componentIndex,
                                          fbe_bool_t faultDataRequested)
{
    fbe_u32_t port = 0, encl_pos = 0;
    fbe_u32_t port_object_handle = FBE_OBJECT_ID_INVALID;
    fbe_u32_t object_handle = FBE_OBJECT_ID_INVALID;
    fbe_u32_t comp_object_handle = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_object_mgmt_get_enclosure_info_t  *enclosure_info_ptr = NULL;
    fbe_base_object_mgmt_get_enclosure_info_t *comp_enclosure_info_ptr = NULL;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;
    fbe_u32_t i;

    for (port = 0; port < FBE_API_PHYSICAL_BUS_COUNT; port++)
    {
        if (specified_port >= 0 && specified_port != port)
        {
            // Skip over this port since it is not the specified port
            continue;
        }
        status = fbe_api_get_port_object_id_by_location(port, &port_object_handle);
        if(status != FBE_STATUS_OK)
        {
            continue;
        }
        
        if(port_object_handle != FBE_OBJECT_ID_INVALID)
        {
            for(encl_pos = 0; encl_pos < FBE_API_ENCLOSURES_PER_BUS; encl_pos++)
            {
                if (specified_encl_pos >= 0 && specified_encl_pos != encl_pos)
                {
                    // Skip over this encl_pos since it is not the specified encl_pos
                    continue;
                }
                // Now get the enclosure object. This gets the ICM in case of Voyager
                status = fbe_api_get_enclosure_object_ids_array_by_location(port,encl_pos, &enclosure_object_ids);

                if(status != FBE_STATUS_OK)
                {
                    continue;
                }
          
                if(enclosure_object_ids.enclosure_object_id == FBE_OBJECT_ID_INVALID)
                {
                    break;
                }
                else
                {   
                    object_handle = enclosure_object_ids.enclosure_object_id;
                    /*check if the memory is already allocated*/
                    if(enclosure_info_ptr == NULL)
                    {
                        /*allocate memory*/
                        status = fbe_api_enclosure_setup_info_memory(&enclosure_info_ptr);
                        if(status != FBE_STATUS_OK)
                        {
                            return;
                        }
                    }
                    else
                    {
                        /*If the memory is already allaocted we will just zero out*/
                        fbe_zero_memory(enclosure_info_ptr, sizeof(*enclosure_info_ptr));
                    }
                    //Get the enclosure component data blob via the API.
                    status = fbe_api_enclosure_get_info(object_handle, enclosure_info_ptr);

                    if(status != FBE_STATUS_OK)
                    {
                        continue;
                    }
                    fbe_cli_printf("\n*****************************************************************************\n");
                    fbe_cli_printf("Bus:%d Enclosure:%d\n",port,encl_pos);
                    fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)enclosure_info_ptr);
                    fbe_edal_setCliContext(TRUE);
                    fbe_edal_setTraceLevel(FBE_TRACE_LEVEL_INFO);
                    if (specified_componentType != FBE_ENCL_INVALID_COMPONENT_TYPE)
                    {
                        fbe_edal_printSpecificComponentData((void *)enclosure_info_ptr,
                            specified_componentType,
                            specified_componentIndex,
                            TRUE,
                            fbe_cli_trace_func);          // trace full data
                    }
                    else if (faultDataRequested)
                    {
                        fbe_edal_printFaultedComponentData((void *)enclosure_info_ptr, fbe_cli_trace_func); 
                    }
                    else //If faultData is not requested then print all data.
                    {
                        fbe_edal_printAllComponentData((void *)enclosure_info_ptr, fbe_cli_trace_func); 
                    }
                    fbe_edal_setCliContext(FALSE);
                    
                    if(ComponentIdFlag && enclosure_object_ids.component_count >0)
                    {
                        for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS; i++)
                        {
                            if ((specified_componentId != FBE_ENCL_COMPONENT_ID_ALL) && 
                                (specified_componentId >= 0 && specified_componentId != i))
                            {
                                continue;
                            }
                            comp_object_handle = enclosure_object_ids.comp_object_id[i];

                            if (comp_object_handle == FBE_OBJECT_ID_INVALID)
                            {
                                continue;
                            }

                            if(comp_enclosure_info_ptr == NULL)
                            {
                                status = fbe_api_enclosure_setup_info_memory(&comp_enclosure_info_ptr);
                                if(status != FBE_STATUS_OK)
                                {
                                    return;
                                }
                            }
                            else
                            {
                                fbe_zero_memory(comp_enclosure_info_ptr, sizeof(*comp_enclosure_info_ptr));
                            }

                            //Get the enclosure component data blob via the API.
                            status = fbe_api_enclosure_get_info(comp_object_handle,comp_enclosure_info_ptr);

                            if(status != FBE_STATUS_OK)
                            {
                                continue;
                            }

                            fbe_cli_printf("\n*************************************\n");
                            fbe_cli_printf("Bus:%d Enclosure:%d Component_id %d\n",port, encl_pos, i);
                            fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)comp_enclosure_info_ptr);
                            fbe_edal_setCliContext(TRUE);
                            fbe_edal_setTraceLevel(FBE_TRACE_LEVEL_INFO);
                            if (specified_componentType != FBE_ENCL_INVALID_COMPONENT_TYPE)
                            {
                                fbe_edal_printSpecificComponentData((void *)comp_enclosure_info_ptr,
                                    specified_componentType,
                                    specified_componentIndex,
                                    TRUE,
                                    fbe_cli_trace_func);          // trace full data
                            }
                            else if (faultDataRequested)
                            {
                                fbe_edal_printFaultedComponentData((void *)comp_enclosure_info_ptr, fbe_cli_trace_func); 
                            }
                            else //If faultData is not requested then print all data.
                            {
                                fbe_edal_printAllComponentData((void *)comp_enclosure_info_ptr, fbe_cli_trace_func); 
                            }
                            
                            fbe_edal_setCliContext(FALSE);
                        }
                    }
                } 
            }
        }
    }

    if(enclosure_info_ptr != NULL)
    {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
    }
    if(comp_enclosure_info_ptr != NULL)
    {
        fbe_api_enclosure_release_info_memory(comp_enclosure_info_ptr);
    }
   
    return;
}


/*!*************************************************************************
 * @fn fbe_cli_cmd_encledal_print_pe_data(fbe_enclosure_component_types_t specified_componentType,
 *                                        fbe_u32_t specified_componentIndex)
 **************************************************************************
 *
 *  @brief
 *      This function will output the EDAL data reported by the board object.
 *
 *  @param    specified_componentType - Specified component type.
 *  @param    specified_componentIndex - Specified component
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Nov-2008: bphilbin - created.
 *    10-May-2013: PHE - Moved from fbe_cli_cmd_display_encl_data
 *
 **************************************************************************/
static void fbe_cli_cmd_encledal_print_pe_data(fbe_enclosure_component_types_t specified_componentType,
                                        fbe_u32_t specified_componentIndex)
{
    fbe_object_id_t objectId;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_board_mgmt_get_pe_info_t *pe_info;
    fbe_edal_block_handle_t pe_ControlInfo;

    /* Get the PE data*/
    pe_info = (fbe_board_mgmt_get_pe_info_t *)malloc(sizeof(fbe_board_mgmt_get_pe_info_t));
    if(pe_info == NULL)
    {
        fbe_cli_error("Failed to allocate memory for pe info.\n");
        return;
    }

    pe_ControlInfo = (fbe_edal_block_handle_t)pe_info;

    /* Get the PE data*/
    status = fbe_api_get_board_object_id(&objectId);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in getting board object id, Status: %d \n", status);
        free(pe_info);
        return;
    }

    status = fbe_api_board_get_pe_info(objectId, pe_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving Edal for Processor Encl, Status: %d \n", status);
        free(pe_info);
        return;
    }

    fbe_cli_printf("\n*****************************************************************************\n");
    fbe_cli_printf("Processor Enclosure Data\n");
    /* Print Processor Encl component data*/
    fbe_edal_updateBlockPointers(pe_ControlInfo);
    fbe_edal_setCliContext(TRUE);
    fbe_edal_setTraceLevel(FBE_TRACE_LEVEL_INFO);
    if(specified_componentType != FBE_ENCL_INVALID_COMPONENT_TYPE)
    {
        if(!fbe_edal_is_pe_component(specified_componentType))
        {
            fbe_cli_error("This is not Processor Enclosure component\n %s\n", ENCL_EDAL_USAGE);
        }
        else
        {
            fbe_edal_printSpecificComponentData((void *)pe_info,
                                                specified_componentType,
                                                specified_componentIndex,
                                                TRUE,
                                                fbe_cli_trace_func);          // trace full data
        }
    }
    else
    {
        fbe_edal_printAllComponentData((void *)pe_info, fbe_cli_trace_func);
    }
    fbe_edal_setCliContext(FALSE);
    free(pe_info);

    return;
}

/*!*************************************************************************
 * @fn fbe_cli_cmd_enclosure_envchange
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function will bypass or unbypass the specified drive.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    16-Dec-2008: bphilbin - created.
 *    20-Jan -2011: Vinay Patki - edited.
 *                  changed the way of specifying the disk or enclosure
 *                  in ENVCHANGE command. It now accepts the disk in b_e_d
 *                  and enclosure in B_E format.
 *
 **************************************************************************/
void fbe_cli_cmd_enclosure_envchange(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_u32_t slot = 0, testMode = 0;
    fbe_cli_encl_cmd_type_t encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_object_mgmt_get_enclosure_info_t enclosure_info;
    fbe_base_object_mgmt_encl_dbg_ctrl_t enclosure_encl_debug_control;
    fbe_base_object_mgmt_drv_dbg_ctrl_t enclosure_drive_debug_control;
    fbe_port_dbg_ctrl_t port_debug_control;
    fbe_base_object_mgmt_drv_power_ctrl_t  enclosure_drive_power_control;
    fbe_base_object_mgmt_drv_power_action_t power_on_off_action = FBE_DRIVE_POWER_ACTION_NONE;    
    fbe_enclosure_mgmt_ctrl_op_t expTestModeControl;
    fbe_base_object_mgmt_exp_ctrl_t expDebugControl;
    fbe_enclosure_scsi_error_info_t scsi_error_info;
    fbe_job_service_bes_t disk_location = {0};    // for storing disk location given by user
    fbe_job_service_be_t encl_location = {0};    // for storing enclosure location given by user
    fbe_bool_t is_disk_given = FBE_FALSE;
    fbe_bool_t is_encl_given = FBE_FALSE;
    fbe_edal_block_handle_t    enclosureControlInfo = &enclosure_info;
    fbe_edal_status_t        edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t           driveCount;
    fbe_bool_t connector_disable = FBE_FALSE;
    fbe_u8_t   connector_id = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_object_id_t                     parent_object_id;
    fbe_bool_t bus_flag = FBE_FALSE;
    fbe_bool_t encl_flag = FBE_FALSE;
    fbe_bool_t slot_flag = FBE_FALSE;
    fbe_bool_t cru_insert = FBE_FALSE;
    char response[20];

    /*
     * There should be a minimum of arguments.
     */
    if(argc == 0)
    {
        fbe_cli_printf("%s", ENVCHANGE_USAGE);
        return;
    }
    else if ((argc > 8 )||(argc < 3))
    {
        fbe_cli_printf("enclosure envchange invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", ENVCHANGE_USAGE);
        return;
    }    

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        /*
        * Check the command type
        */
        if(strcmp(*argv, "-r") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            cru_insert = FBE_FALSE; //removal
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CRU_DEBUG_CTRL;
        }
        else if(strcmp(*argv, "-a") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            cru_insert = FBE_TRUE; //insertion
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CRU_DEBUG_CTRL;
        }
        else if(strcmp(*argv, "-destroy") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_DESTROY_OBJECT;
        }
        else if(strcmp(*argv, "-lasterror") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_LAST_ERROR;
        }
        else if(strcmp(*argv, "-power_on_off") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("enclosure envchange, too few arguments \n");
                return;
            }
            if(strcmp(*argv, "0") == 0)
                power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_DOWN;
            else if(strcmp(*argv, "1") == 0)
                power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_UP;
            else if(strcmp(*argv, "2") == 0)
                power_on_off_action = FBE_DRIVE_POWER_ACTION_POWER_CYCLE;
            else
            {
                fbe_cli_error("enclosure envchange invalid argument, for this power_on_off switch %s\n", *argv);
                return;
            }
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL;
        }
        /*
         * Check the command arguments
         */
        /* This format will be depreciated */
        else if (strcmp(*argv, "-disk") == 0)
        {
            /* Check if enclosure has been mentioned along with disk*/
            if(is_encl_given == FBE_TRUE)
            {
                fbe_cli_error("%s:\tPlease specify either Disk (B_E_D) or Encl (B_E). %d\n", __FUNCTION__, status);
                return;
            }

            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("disk argument is expected\n");
                return;
            }

            /* FBECLI - TODO*/

            /* Validate that the disk numbers are given correctly*/
            /* Convert to B_E_D format*/
            /* Check for case where no drive is specified and return*/

            status = fbe_cli_convert_diskname_to_bed(argv[0], &disk_location);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s:\tSpecified disk number %s is invalid. %d\n", __FUNCTION__, *argv, status);
                return;
            }

            bus_flag = FBE_TRUE;
            encl_flag = FBE_TRUE;
            slot_flag = FBE_TRUE;
            port = disk_location.bus;
            encl_pos = disk_location.enclosure;
            slot = disk_location.slot;
        }
        /* This format will be depreciated */
        else if (strcmp(*argv, "-encl") == 0)
        {
            /* Check if disk has been mentioned along with enclosure*/
            if(is_disk_given == FBE_TRUE)
            {
                fbe_cli_error("%s:\tPlease specify either Disk (B_E_D) or Encl (B_E). %d\n", __FUNCTION__, status);
                return;
            }

            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Enclosure argument is expected\n");
                return;
            }

            /* FBECLI - TODO*/

            /* Validate that the enclosure numbers are given correctly*/
            /* Convert to B_E format*/
            /* Check for case where no enclosure is specified and return*/

            status = fbe_cli_convert_encl_to_be(argv[0], &encl_location);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s:\tSpecified encl number %s is invalid. %d\n", __FUNCTION__, *argv, status);
                return;
            }

            bus_flag = FBE_TRUE;
            encl_flag = FBE_TRUE;
            port = encl_location.bus;
            encl_pos = encl_location.enclosure;
        }
        else if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("Please provide bus #.\n");
                return;
            }

            port = atoi(*argv);
            if(port >= FBE_PHYSICAL_BUS_COUNT && port !=FBE_XPE_PSEUDO_BUS_NUM)
            {
                fbe_cli_error("Invalid Bus number, it should be between 0 to %d.\n", FBE_PHYSICAL_BUS_COUNT - 1);
                return;
            }
            disk_location.bus = port;
            encl_location.bus = port;
            bus_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("Please provide enclosure position #.\n");
                return;
            }
            encl_pos = atoi(*argv);
            if(encl_pos >= FBE_ENCLOSURES_PER_BUS && encl_pos !=FBE_XPE_PSEUDO_ENCL_NUM)
            {
                fbe_cli_error("Invalid Enclosure number, it should be between 0 to %d.\n", FBE_ENCLOSURES_PER_BUS - 1);
                return;
            }
            disk_location.enclosure = encl_pos;
            encl_location.enclosure = encl_pos;
            encl_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-s") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("Please provide slot position #.\n");
                return;
            }
            slot = atoi(*argv);
            disk_location.slot = slot;
            slot_flag = FBE_TRUE;
        }
        else if(strcmp(*argv, "-resetexp") == 0)
        {
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_EXP_RESET;
        }
        else if(strcmp(*argv, "-testmode") == 0)
        {
            fbe_zero_memory(&expTestModeControl, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
            if((argc-1)>0 && csx_isdigit(**(argv+1)))
            {
                argc--;
                argv++;
                if(strcmp(*argv, "0") == 0)
                {
                    testMode = 0;
                }
                else if(strcmp(*argv, "1") == 0)
                {
                    testMode = 1;
                }
                else
                {
                    fbe_cli_error("Invalid testmode argument %s, it should be 0 or 1.\n", *argv);
                    return;
                }
            }
            else
            {
                expTestModeControl.cmd_buf.testModeInfo.testModeAction = FBE_EXP_TEST_MODE_STATUS;
            }
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_TEST_MODE;
        }
        else if(strcmp(*argv, "-disable") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            connector_disable = FBE_TRUE;
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL;
        }
        else if(strcmp(*argv, "-enable") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_error("enclosure envchange invalid argument, too many commands %s\n", *argv);
                return;
            }
            connector_disable = FBE_FALSE;
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL;
        }       
        else if(strcmp(*argv, "-conn") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("enclosure envchange, too few arguments \n");
                return;
            }
            connector_id = (fbe_u8_t)strtoul(*argv, 0, 0);
        }
        else
        {
            fbe_cli_error("enclosure envchange invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    // Check the location
    if(bus_flag && encl_flag)
    {
        if(slot_flag)
        {
            is_disk_given = FBE_TRUE;
        }
        else
        {
            is_encl_given = FBE_TRUE;
        }
    }
    else
    {
        fbe_cli_error("enclosure envchange, too few arguments.\n");
        return;
    }

    /* Determine whether it is an enclosure or a disk insertion/removal */
    if(encl_cmd_type == FBE_CLI_ENCL_CMD_TYPE_CRU_DEBUG_CTRL)
    {
        if(is_disk_given)
        {
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_DRIVE_DEBUG_CTRL;
        }
        else
        {
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL;
        }
    }

    if(encl_cmd_type == FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL && !is_disk_given)
    {
        fbe_cli_error("enclosure envchange, too few arguments.\n");
        return;
    }

    if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL)
    {
        /*
         * Get the object ID for the specified enclosure
         */
        status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nenvchange failed to get enclosure object handle status: %d\n",
                status);
            return;
        }
        if(object_handle_p == FBE_OBJECT_ID_INVALID)
        {
            fbe_cli_error("\nenvchange failed to get enclosure object handle\n");
            return;
        }

        status = fbe_api_enclosure_get_info(object_handle_p, &enclosure_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("\ndisplay enclosure data failed to get enclosure data, status: 0x%x\n",
                status);
            return;
        }
        fbe_edal_updateBlockPointers(enclosureControlInfo);
        edalStatus = fbe_edal_getSpecificComponentCount(enclosureControlInfo,
                                                        FBE_ENCL_DRIVE_SLOT,
                                                        &driveCount);
        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            fbe_cli_error("\nFailed to get the slot count info, edalStatus: 0x%x\n", edalStatus);
            return;
        }
    }
    
    switch(encl_cmd_type)
    {
        case FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL:
            if ( connector_id == FBE_ENCLOSURE_VALUE_INVALID)
            {
                fbe_cli_error("Invalid connector id\n");
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            // update local edal data
            edalStatus = fbe_edal_connectorControl((fbe_edal_block_handle_t)&enclosure_info,
                                                    connector_id,
                                                    connector_disable);
            if (!EDAL_STAT_OK(edalStatus))
            {
                fbe_cli_error("Unable to control connector id %d, edal status %d\n",
                    connector_id, edalStatus);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }

            // send control
            status = fbe_api_enclosure_setControl(object_handle_p, (fbe_base_object_mgmt_set_enclosure_control_t *)&enclosure_info);

            break;

        case FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL:
            if(encl_pos == 0)
            {
                if(port == 0 && !cru_insert)
                {
                    fbe_cli_printf("Removal of bus 0 enclosure 0 may cause panic.\nDo you want to continue(y/n)?\n");

                    fgets(response, sizeof(response), stdin);
                    if((response[0] == 'N') || (response[0] == 'n'))
                    {
                        return;
                    }
                }

                status = fbe_api_get_port_object_id_by_location_and_role(port, FBE_PORT_ROLE_BE, &object_handle_p);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nencldbg failed to get port object handle status: %d\n",
                        status);
                    return;
                }
                if(object_handle_p == FBE_OBJECT_ID_INVALID)
                {
                    fbe_cli_error("\nencldbg failed to get port object handle\n");
                    return;
                }
                port_debug_control.insert_masked = cru_insert ? FBE_FALSE : FBE_TRUE;
                status = fbe_api_port_set_port_debug_control(object_handle_p, &port_debug_control);
            }
            else
            {
                encl_location.enclosure = encl_pos - 1;
                status = fbe_api_get_enclosure_object_id_by_location(encl_location.bus, encl_location.enclosure, &object_handle_p);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nencldbg failed to get enclosure object handle status: %d\n",
                        status);
                    return;
                }
                if(object_handle_p == FBE_OBJECT_ID_INVALID)
                {
                    fbe_cli_error("\nencldbg failed to get enclosure object handle\n");
                    return;
                }
                fbe_zero_memory(&enclosure_encl_debug_control, sizeof(fbe_base_object_mgmt_encl_dbg_ctrl_t));
                enclosure_encl_debug_control.enclDebugAction
                    = cru_insert ? FBE_ENCLOSURE_ACTION_INSERT : FBE_ENCLOSURE_ACTION_REMOVE;
                status = fbe_api_enclosure_send_encl_debug_control(object_handle_p, &enclosure_encl_debug_control);
            }
            break;

        case FBE_CLI_ENCL_CMD_TYPE_DRIVE_DEBUG_CTRL:
            /* The command that was issued was one of the drive debug control commands. */
            fbe_zero_memory(&enclosure_drive_debug_control, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
            enclosure_drive_debug_control.driveCount = driveCount;
            enclosure_drive_debug_control.driveDebugAction[slot]
                = cru_insert ? FBE_DRIVE_ACTION_INSERT : FBE_DRIVE_ACTION_REMOVE;

            status = fbe_api_enclosure_get_disk_parent_object_id(disk_location.bus, 
                                                                 disk_location.enclosure, 
                                                                 disk_location.slot,
                                                                 &parent_object_id);
            if (status == FBE_STATUS_OK)
            {
                status = fbe_api_enclosure_send_drive_debug_control(parent_object_id, &enclosure_drive_debug_control);
            }
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed drive debug control:%d_%d_%d ParentObjId:0x%x status:0x%x\n",
                               disk_location.bus, disk_location.enclosure, disk_location.slot, parent_object_id, status);
            }
            break;

        case FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL:
            /* The command that was issued was one of the drive power control commands. */
            fbe_zero_memory(&enclosure_drive_power_control, sizeof(fbe_base_object_mgmt_drv_power_ctrl_t));
            enclosure_drive_power_control.driveCount = driveCount;
            enclosure_drive_power_control.drivePowerAction[slot] = power_on_off_action;

            status = fbe_api_enclosure_get_disk_parent_object_id(disk_location.bus, 
                                                                 disk_location.enclosure, 
                                                                 disk_location.slot,
                                                                 &parent_object_id);

            if (status == FBE_STATUS_OK)
            {
                status = fbe_api_enclosure_send_drive_power_control(parent_object_id, &enclosure_drive_power_control);
            }
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed drive power control:%d_%d_%d ParentObjId:0x%x status:0x%x\n",
                              disk_location.bus, disk_location.enclosure, disk_location.slot, parent_object_id, status);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    
        case FBE_CLI_ENCL_CMD_TYPE_DESTROY_OBJECT:
            /* The command is to destroy the enclosure object. */
            status = fbe_api_destroy_object(object_handle_p, FBE_PACKAGE_ID_PHYSICAL);
            break;

        case FBE_CLI_ENCL_CMD_TYPE_EXP_RESET:
            // The command is to Reset the Expander
            fbe_zero_memory(&expDebugControl, sizeof(fbe_base_object_mgmt_exp_ctrl_t));
            expDebugControl.expanderAction = FBE_EXPANDER_ACTION_COLD_RESET;
            expDebugControl.powerCycleDelay = 0;
            expDebugControl.powerCycleDuration = 0;
            status = fbe_api_enclosure_send_expander_control(object_handle_p, &expDebugControl);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nenvchange failed to reset Expander %d, encl %d, status: %d\n",
                    port, encl_pos, status);
                return;
            }
            break;

        case FBE_CLI_ENCL_CMD_TYPE_TEST_MODE:
            if (!expTestModeControl.cmd_buf.testModeInfo.testModeAction == FBE_EXP_TEST_MODE_STATUS)
            {
                // The command is to modify the Expander's Test Mode setting
                if (testMode == 0)
                {
                    expTestModeControl.cmd_buf.testModeInfo.testModeAction = FBE_EXP_TEST_MODE_DISABLE;
                }
                else if (testMode == 1)
                {
                    expTestModeControl.cmd_buf.testModeInfo.testModeAction = FBE_EXP_TEST_MODE_ENABLE;
                }
                else
                {
                    fbe_cli_error("\nenvchange, testmode invalid %d\n",
                        testMode);
                    return;
                }
            }
            status = fbe_api_enclosure_send_exp_test_mode_control(object_handle_p, &expTestModeControl);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nenvchange failed to send control to bus %d, encl %d, status: %d\n",
                    port, encl_pos, status);
                return;
            }
            else if (expTestModeControl.cmd_buf.testModeInfo.testModeAction == FBE_EXP_TEST_MODE_STATUS)
            {
                fbe_cli_printf("Current TestMode value is %s\n",
                    (expTestModeControl.cmd_buf.testModeInfo.testModeStatus == 0) ? "DISABLED" : "ENABLED");
            }
            break;

        case FBE_CLI_ENCL_CMD_TYPE_LAST_ERROR:
            /* This command is to retrieve last error on the enclosure object */
            status = fbe_api_enclosure_get_scsi_error_info(object_handle_p, &scsi_error_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nenvchange failed to get scsi error info, encl %d, status: %d\n",
                    encl_pos, status);
            }

            // print scsi error info
            fbe_cli_printf("scsi_error_code:0x%x  scsi_status:0x%x   payload_request_status:0x%x\n",
                scsi_error_info.scsi_error_code, scsi_error_info.scsi_status, scsi_error_info.payload_request_status);
            fbe_cli_printf("sense key:%d           addl sense code:%d  addl sense code qual:%d\n", 
                scsi_error_info.sense_key, scsi_error_info.addl_sense_code, scsi_error_info.addl_sense_code_qualifier);

            break;

        default:
            fbe_cli_error("\nenvchange unhandled %s cmd, status: 0x%x.\n",
                fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), status);
            return;
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nenvchange failed to send %s cmd, status: 0x%x.\n",
            fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), status);
        return;
    }

    fbe_cli_printf("\nenvchange successfully issued %s cmd, status: 0x%x.\n",
        fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), status);
    
    return;
}
/**************************************
 * end fbe_cli_cmd_enclosure_envchange()
 **************************************/

/*!*************************************************************************
 * @fn fbe_cli_cmd_eses_pass_through(int argc , char ** argv)
 **************************************************************************
 *  @brief
 *      This function will process all the command types related to 
 *  pass through receive diag command.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    6-Mar-2009: NC - Created.
 *
 **************************************************************************/
void fbe_cli_cmd_eses_pass_through(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_enclosure_raw_rcv_diag_pg_t raw_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID;   
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    /*
     * There should be a minimum of arguments.
     */
    if(argc != 6)
    {
        fbe_cli_printf("\nencl_rec_diag: Invalid number of arguments %d.\n", argc);
        fbe_cli_printf("%s", ENCL_REC_DIAG_USAGE);
        return;
    }

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        /*
         * Check the command arguments
         */
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_rec_diag: Please provide bus #.\n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_rec_diag: Please provide enclosure #.\n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-cmd") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_rec_diag: Please provide the eses page.\n");
                return;
            }
        
            if(strcmp(*argv, "cfg") == 0)
            {
                if(raw_diag_page != FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_rec_diag: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_CFG;
            }
            else if(strcmp(*argv, "stat") == 0)
            {
                if(raw_diag_page != FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_rec_diag: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STATUS;
            }
            else if(strcmp(*argv, "addl_stat") == 0)
            {
                if(raw_diag_page != FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_rec_diag: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_ADDL_STATUS;
            }
            else if(strcmp(*argv, "emc_encl") == 0)
            {
                if(raw_diag_page != FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_rec_diag: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_EMC_ENCL_STATUS;
            }
            else 
            {
                fbe_cli_printf("\nencl_rec_diag: Invalid commands %s.\n", *argv);
                return;
            }            
   
        }        
        else
        {
            fbe_cli_printf("\nencl_rec_diag: Invalid argument %s.\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    /* 
     * Get the object ID for the specified enclosure
     */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nencl_rec_diag: Failed to get enclosure object handle status: %d,\n", status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nencl_rec_diag: Invalid enclosure object handle.\n");
        return;
    }

    if(raw_diag_page == FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID)
    {
        fbe_cli_printf("\nencl_rec_diag: no page specified.\n");
        return;
    }            

    status = fbe_cli_encl_send_raw_rcv_diag_cmd(object_handle_p, raw_diag_page, &enclosure_status);

    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("\nencl_rec_diag: Failed to send cmd %d to bus %d, encl %d, status: 0x%x.\n",
            raw_diag_page, port, encl_pos, status);
        return;
    }

    fbe_cli_printf("\nencl_rec_diag: cmd %d sent to bus %d, encl %d, status: 0x%x.\n",
        raw_diag_page, port, encl_pos, status);
    return;
}


/*!*************************************************************************
 * @fn fbe_cli_cmd_enclbuf(int argc , char ** argv)
 **************************************************************************
 *  @brief
 *      This function will process all the command types related to enclosure buffers.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    30-Jan-2009: PHE - Created.
 *
 **************************************************************************/
void fbe_cli_cmd_enclbuf(int argc , char ** argv)
{
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_cli_encl_cmd_type_t encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_INVALID;   
    fbe_u8_t buf_id = FBE_ENCLOSURE_BUF_ID_UNSPECIFIED;
    fbe_enclosure_mgmt_trace_buf_op_t buf_op = FBE_ENCLOSURE_TRACE_BUF_OP_INVALID;
    char filename[FBE_CLI_ENCL_FILE_NAME_MAX_LEN + 1];
    size_t filename_len = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t  output_to_file = FALSE;
    fbe_u8_t  temp[50] ={0} ;

    /*
     * There should be a minimum of arguments.
     */
    if(argc == 0)
    {
        fbe_cli_printf("%s", ENCLBUF_USAGE);
        return;
    }
    else if ((argc > 9 )||(argc < 5))
    {
        fbe_cli_printf("\nenclbuf: Invalid number of arguments %d.\n", argc);
        fbe_cli_printf("%s", ENCLBUF_USAGE);
        return;
    }    

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        /*
         * Check the command arguments
         */
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide bus #.\n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide enclosure #.\n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-bid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide the buffer id.\n");
                return;
            }
            buf_id = (fbe_u8_t)strtoul(*argv, 0, 0);
        }

        /*
        * Check the command type
        */
        
        else if(strcmp(*argv, "-listtrace") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_printf("\nenclbuf: Too many commands %s.\n", *argv);
                return;
            }            
            
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_LIST_TRACE_BUF;
        }
        else if(strcmp(*argv, "-savetrace") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_printf("\nenclbuf: Too many commands %s.\n", *argv);
                return;
            }

            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_SAVE_TRACE_BUF;
        }
        else if(strcmp(*argv, "-cleartrace") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_printf("\nenclbuf: Too many commands %s.\n", *argv);
                return;
            }

            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF;
        }
        else if(strcmp(*argv, "-getsize") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_printf("\nenclbuf: Too many commands %s.\n", *argv);
                return;
            }
            
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_GET_BUF_SIZE;
        }
        else if(strcmp(*argv, "-getdata") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_INVALID)
            {
                fbe_cli_printf("\nenclbuf: Too many commands %s.\n", *argv);
                return;
            }
                   
            encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA;
        }
        else if(strcmp(*argv, "-f") == 0)
        {
            if(encl_cmd_type != FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA)
            {
                fbe_cli_printf("\nenclbuf: %s does not apply to this command type.\n", *argv);
                return;
            }
            
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide the filename.\n");
                return;
            }
            filename_len = strlen(*argv);
            if(filename_len > FBE_CLI_ENCL_FILE_NAME_MAX_LEN)
            {
                fbe_cli_printf("\nenclbuf: The file name is too long, max: %d.\n", FBE_CLI_ENCL_FILE_NAME_MAX_LEN);
                return;
            }
            memset(filename, 0, FBE_CLI_ENCL_FILE_NAME_MAX_LEN + 1); 
            memcpy(filename, *argv, filename_len);          
            output_to_file = TRUE;
        }        
        else
        {
            fbe_cli_printf("\nenclbuf: Invalid argument %s.\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    /* 
     * Get the object ID for the specified enclosure
     */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nenclbuf: Failed to get enclosure object handle status: %d,\n", status);
        return;
    }
    if(object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nenclbuf: Invalid enclosure object id.\n");
        return;
    }
   
    switch(encl_cmd_type)
    {
        case FBE_CLI_ENCL_CMD_TYPE_LIST_TRACE_BUF:
            /* The command is to list out all the trace buffer and their status. */
            if(buf_id != FBE_ENCLOSURE_BUF_ID_UNSPECIFIED)
            {
                fbe_cli_printf("\nenclbuf: the buffer id is ignored.\n");
            } 
            status = fbe_cli_encl_list_all_trace_buf(object_id);            
            break;        

        case FBE_CLI_ENCL_CMD_TYPE_SAVE_TRACE_BUF:
            /* The command is to control the trace buffer. */
            if(buf_id != FBE_ENCLOSURE_BUF_ID_UNSPECIFIED)
            {
                fbe_cli_printf("\nenclbuf: Save to the oldest buffer if the platform does not support specified buffer id.\n");
            } 
            buf_op = FBE_ENCLOSURE_TRACE_BUF_OP_SAVE;
            status = fbe_cli_encl_trace_buf_ctrl(object_id, buf_id, buf_op);
            break;

        case FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF:
            /* The command is to clear the trace buffer. */
            if(buf_id != FBE_ENCLOSURE_BUF_ID_UNSPECIFIED)
            {
                fbe_cli_printf("\nenclbuf: Clear all saved trace buffers if the platform does not support specified buffer id.\n");
            } 
            buf_op = FBE_ENCLOSURE_TRACE_BUF_OP_CLEAR;
            status = fbe_cli_encl_trace_buf_ctrl(object_id, buf_id, buf_op);
            break;

        case FBE_CLI_ENCL_CMD_TYPE_GET_BUF_SIZE:
            /* The command is to get the capacity of the buffer. */
            if(buf_id == FBE_ENCLOSURE_BUF_ID_UNSPECIFIED)
            {
                fbe_cli_printf("\nenclbuf: Please provide the buffer id.\n");
                return;
            }            
            status = fbe_cli_encl_get_buf_size(object_id, buf_id, NULL, &enclosure_status);
            break;
   
        case FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA:
            /* The command is to save the buffer data a file */
            if(buf_id == FBE_ENCLOSURE_BUF_ID_UNSPECIFIED)
            {
                fbe_cli_printf("\nenclbuf: Please provide the buffer id.\n");
                return;
            }
            _snprintf(temp,sizeof(temp),"bus:%d encl_pos:%d active trace bid %d ", port, encl_pos, buf_id);
            status = fbe_cli_encl_save_buf_data_to_file(object_id, buf_id, &filename[0], output_to_file, temp);
            break;

        default:
            fbe_cli_printf("\nenclbuf: unhandled %s cmd to bus %d, encl %d, buf_id %d status: 0x%x.\n",
                fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), port, encl_pos, buf_id, status);
            return;
    }

    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("\nenclbuf: Failed to send %s cmd to bus %d, encl %d, buf_id %d, status: 0x%x.\n",
            fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), port, encl_pos, buf_id, status);
    }
    else
    {
       fbe_cli_printf("\nenclbuf: Successfully issued %s cmd to bus %d, encl %d, buf_id %d, status: 0x%x.\n",
            fbe_cli_encl_decode_encl_cmd_type(encl_cmd_type), port, encl_pos, buf_id, status);
    }
    
    return;
}

/*!*************************************************************************
 * @fn fbe_cli_cmd_enclosure_setled
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function will bypass or unbypass the specified drive.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Jan-2009: Joe Perry - created.
 *
 **************************************************************************/
void fbe_cli_cmd_enclosure_setled(int argc , char ** argv)
{
    fbe_u8_t            displayCharStatus = 0;
    fbe_u8_t            displayNumber;
    fbe_u8_t            max_slot;
    fbe_u32_t           slot = 0, port = 0, encl_pos = 0;
    fbe_u32_t           object_handle_p = FBE_OBJECT_ID_INVALID; 
    fbe_bool_t          enclFaultLedOn;
    fbe_bool_t          enclFaultLedMarked;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_edal_status_t   edal_status = FBE_EDAL_STATUS_OK;
    fbe_bool_t is_disk_given = FBE_FALSE;

    fbe_base_enclosure_led_behavior_t           ledAction;
    fbe_base_enclosure_led_status_control_t     ledCommandInfo;
    fbe_base_object_mgmt_get_enclosure_info_t   enclosure_info;
    fbe_base_object_mgmt_get_enclosure_info_t   *enclosure_info_ptr = &enclosure_info;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;
    fbe_edal_block_handle_t                     enclosureControlInfo = &enclosure_info;
    fbe_led_status_t EnclDriveFaultLeds[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS]; 
    fbe_board_mgmt_platform_info_t platform_info = {0};
    fbe_object_id_t board_objectId;
    fbe_encl_fault_led_info_t  enclFaultLedInfo = {0};
    fbe_api_board_enclFaultLedInfo_t dpeEnclFaultLedInfo={0};

    /*
     * There should be a minimum of arguments.
     */
    if(argc == 0)
    {
        fbe_cli_printf("%s", SETLED_USAGE);
        return;
    }

    if ((argc == 1) && ((strcmp(*argv, "-h") == 0) || (strcmp(*argv, "-help") == 0)))
    {
        fbe_cli_printf("%s", SETLED_USAGE);
        return;
    }

    if (argc < 3)
    {
        fbe_cli_printf("enclosure setled invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", SETLED_USAGE);
        return;
    }    

    /*
     * Parse the command line.
     */
    fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
    while(argc > 0)
    {
        /*
         * Check the command type
         */
        if(strcmp(*argv, "-list") == 0)
        {
            ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_STATUS;
        }

        else if(strcmp(*argv, "-enclflt") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            ledAction = convertStringToLedAction(*argv);
            ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
            ledCommandInfo.ledInfo.enclFaultLed = ledAction;
        }

        else if(strcmp(*argv, "-busdisp") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
            displayNumber = (fbe_u8_t)strtoul(*argv, 0, 0);
            // update if larger #'s added
            ledCommandInfo.ledInfo.busNumber[1] = '0' + displayNumber;
            ledCommandInfo.ledInfo.busNumber[0] = '0';
            ledCommandInfo.ledInfo.busDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;
        }

        else if(strcmp(*argv, "-encldisp") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
            displayNumber = (fbe_u8_t)strtoul(*argv, 0, 0);
            ledCommandInfo.ledInfo.enclNumber = '0' + displayNumber;
            ledCommandInfo.ledInfo.enclDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;
        }

        /*
         * Check the command arguments
         */
        else if((strcmp(*argv, "-p") == 0) || (strcmp(*argv, "-b") == 0))
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if((strcmp(*argv, "-d") == 0) || (strcmp(*argv, "-s") == 0))
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, too few arguments \n");
                return;
            }
            slot = (fbe_u32_t)strtoul(*argv, 0, 0);
            is_disk_given = FBE_TRUE;
        }
        else if(strcmp(*argv, "-a") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure setled, no action specified\n");
                return;
            }
            else if (slot == 0xffffffff)
            {
                fbe_cli_printf("enclosure setled, no drive specified\n");
                return;
            }
            else
            {
                ledAction = convertStringToLedAction(*argv);
                ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
                ledCommandInfo.ledInfo.enclDriveFaultLeds[slot] =  ledAction;
            }
        }
        else
        {
            fbe_cli_printf("enclosure setled invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    /* 
     * Get the object ID for the specified enclosure
     */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nsetled failed to get enclosure object handle status: %d\n",
            status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nsetled failed to get enclosure object handle\n");
        return;
    }

    status = fbe_api_get_board_object_id(&board_objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get board_objectId, status:%d\n", status);
        return;
    }

    status = fbe_api_board_get_platform_info(board_objectId, &platform_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get platform_info, status:%d\n", status);
        return;
    }

    /*
     * Display LED Status if appropriate
     */
    if (ledCommandInfo.ledAction == FBE_ENCLOSURE_LED_STATUS)
    {
        /*
        * Get the enclosure component data blob via the API.
        */
       status = fbe_api_enclosure_get_info(object_handle_p, &enclosure_info);
       if(status != FBE_STATUS_OK)
       {
           fbe_cli_error("\ndisplay enclosure data failed to get enclosure data status:%d\n", 
               status);
           return;
       }
    
         /* DPE's enclosure fault LED is managed in board object */
        if(port == 0 && encl_pos == 0 && platform_info.hw_platform_info.enclosureType == DPE_ENCLOSURE_TYPE)
        {
            status = fbe_api_enclosure_get_encl_fault_led_info(board_objectId, &enclFaultLedInfo);

            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("Failed to get encl fault LED info, status:%d\n", status);
                return;
            }


            fbe_cli_printf("LED Status for Bus %d, Enclosure %d :\n", port, encl_pos);

            fbe_cli_printf("    EnclosureFaultLed : ");
            if (enclFaultLedInfo.enclFaultLedStatus == FBE_LED_STATUS_ON)
            {
                fbe_cli_printf("ON\n");
            }
            else if (enclFaultLedInfo.enclFaultLedStatus == FBE_LED_STATUS_MARKED)
            {
                fbe_cli_printf("MARKED\n");
            }
            else
            {
                fbe_cli_printf("OFF\n");
            }

        }
        else
        {
            /*
             * Enclosure Data contains some pointers that must be converted
             */
            fbe_edal_updateBlockPointers(enclosureControlInfo);
    
            fbe_cli_printf("LED Status for Bus %d, Enclosure %d :\n", port, encl_pos);
    
            // enclosure Fault LED
            fbe_edal_getBool(enclosureControlInfo,
                             FBE_ENCL_COMP_FAULT_LED_ON,
                             FBE_ENCL_ENCLOSURE,
                             0,
                             &enclFaultLedOn);
            fbe_edal_getBool(enclosureControlInfo,
                             FBE_ENCL_COMP_MARKED,
                             FBE_ENCL_ENCLOSURE,
                             0,
                             &enclFaultLedMarked);
            fbe_cli_printf("    EnclosureFaultLed : ");
            if (enclFaultLedOn)
            {
                fbe_cli_printf("ON\n");
            }
            else if (enclFaultLedMarked)
            {
                fbe_cli_printf("MARKED\n");
            }
            else
            {
                fbe_cli_printf("OFF\n");
            }
    
            // Bus Displays
            edal_status = fbe_edal_getU8(enclosureControlInfo,
                           FBE_DISPLAY_CHARACTER_STATUS,
                           FBE_ENCL_DISPLAY,
                           FBE_EDAL_DISPLAY_BUS_0,
                           &displayCharStatus);
            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_cli_printf("%s failed get bus 0 status.", __FUNCTION__);
            }
            else
            {
                fbe_cli_printf("    Display, Bus : %c", displayCharStatus);
            }
            edal_status = fbe_edal_getU8(enclosureControlInfo,
                           FBE_DISPLAY_CHARACTER_STATUS,
                           FBE_ENCL_DISPLAY,
                           FBE_EDAL_DISPLAY_BUS_1,
                           &displayCharStatus);
            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_cli_printf("%s failed get bus 0 status.", __FUNCTION__);
            }
            else
            {
                fbe_cli_printf("%c", displayCharStatus);
            }
            fbe_cli_printf("\n");
            
            // Enclosure Display
            edal_status = fbe_edal_getU8(enclosureControlInfo,
                           FBE_DISPLAY_CHARACTER_STATUS,
                           FBE_ENCL_DISPLAY,
                           FBE_EDAL_DISPLAY_ENCL_0,
                           &displayCharStatus);
            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_cli_printf("%s failed get encl 0 status.", __FUNCTION__);
            }
            else
            {
                fbe_cli_printf("    Display, Encl : %c\n", displayCharStatus);
            }
        }

        status = fbe_api_get_enclosure_object_ids_array_by_location(port,encl_pos, &enclosure_object_ids);
        /* Enclosure Data contains some pointers that must be converted */
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)enclosure_info_ptr);

        edal_status = fbe_edal_getU8((fbe_edal_block_handle_t)enclosure_info_ptr,
                    FBE_ENCL_MAX_SLOTS,
                    FBE_ENCL_ENCLOSURE,
                    0,
                    &max_slot);
        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            fbe_cli_printf("%s failed get FBE_ENCL_MAX_SLOTS.", __FUNCTION__);
        }
        
        status = fbe_api_enclosure_get_encl_drive_slot_led_info(object_handle_p, EnclDriveFaultLeds); 
        if(status == FBE_STATUS_OK)
        {
            fbe_cli_print_drive_Led_info(EnclDriveFaultLeds,max_slot); 
        }
        fbe_cli_printf("\n");
    }
    else
    {
        /*
         * Issue the LED command to the enclosure
         */
        if(is_disk_given == FBE_TRUE)
        {
            status = fbe_api_enclosure_update_drive_fault_led(port, encl_pos, slot, ledAction);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nsetled failed to send LED control to bus %d, encl %d, slot %d status: %d\n",
                    port, encl_pos, slot, status);
                return;
            }
        }
        else
        {
            /* DPE's enclosure fault LED is managed in board object */
            if(port == 0 && encl_pos == 0 && platform_info.hw_platform_info.enclosureType == DPE_ENCLOSURE_TYPE)
            {
                if (ledAction == FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON)
                {
                    dpeEnclFaultLedInfo.blink_rate = LED_BLINK_ON;
                }
                else if (ledAction == FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON)
                {
                    dpeEnclFaultLedInfo.blink_rate = LED_BLINK_1HZ;
                }
                else
                {
                    dpeEnclFaultLedInfo.blink_rate = LED_BLINK_OFF;
                }

                status = fbe_api_board_set_enclFaultLED(board_objectId, &dpeEnclFaultLedInfo);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nsetled failed to send LED control to bus %d, encl %d, status: %d\n",
                        port, encl_pos, status);
                    return;
                }
            }
            else
            {
                status = fbe_api_enclosure_send_set_led(object_handle_p, &ledCommandInfo);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nsetled failed to send LED control to bus %d, encl %d, status: %d\n",
                        port, encl_pos, status);
                    return;
                }
            }
        }
    }

    return;
}

/*!*********************************************************************
 *  @fn convertStringToLedAction
 **********************************************************************
 *  @brief
 *    Converts the LED action string to LED behavior.
 *
 *  @param    ledActionString - LED action string.
 *
 *  @return
 *        Enum mentioning LED behavior
 *
 *
 **********************************************************************/
fbe_base_enclosure_led_behavior_t
convertStringToLedAction(const char *ledActionString)
{

    if ((strcmp(ledActionString, "on") == 0) ||
        (strcmp(ledActionString, "ON") == 0))
    {
        return FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON;
    }
    if ((strcmp(ledActionString, "off") == 0) ||
        (strcmp(ledActionString, "OFF") == 0))
    {
        return FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF;
    }
    if ((strcmp(ledActionString, "mark") == 0) ||
        (strcmp(ledActionString, "MARK") == 0))
    {
        return FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON;
    }
    if ((strcmp(ledActionString, "unmark") == 0) ||
        (strcmp(ledActionString, "UNMARK") == 0))
    {
        return FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF;
    }

    return FBE_ENCLOSURE_LED_BEHAVIOR_NONE;

}   // end of convertStringToLedAction

/*!*********************************************************************
 *  @fn convertLedActionToString
 **********************************************************************
 *  @brief
 *    Converts the LED behavior into LED action string.
 *
 *  @param    fbe_base_enclosure_led_behavior_t - LED behavior.
 *
 *  @return char *
 *        LED action string
 *
 *
 **********************************************************************/
char *
convertLedActionToString(const fbe_base_enclosure_led_behavior_t ledAction)
{

    switch (ledAction)
    {
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON:
            return("FLT");
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON:
            return("MRK"); 
            break;

        default:
            return("OFF");
    }
}   // end of convertStringToLedAction


/*!*********************************************************************
 *  @fn fbe_cli_encl_decode_encl_cmd_type(fbe_cli_encl_cmd_type_t encl_cmd_type)
 **********************************************************************
 *  @brief
 *    Prints the FBE CLI env command type.
 *
 *  @param    encl_cmd_type - The fbe cli enclosure command type.
 *
 *  @return char *
 *        The pointer to the string of the fbe cli env command type.
 *
 *  HISTORY:
 *    27-Jan-2009 PHE - Created.
 *
 **********************************************************************/

static char * fbe_cli_encl_decode_encl_cmd_type(fbe_cli_encl_cmd_type_t encl_cmd_type)
{
    switch (encl_cmd_type)
    {
        case FBE_CLI_ENCL_CMD_TYPE_INVALID:
            return("FBE_CLI_ENCL_CMD_TYPE_INVALID");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL:
            return("FBE_CLI_ENCL_CMD_TYPE_ENCL_DEBUG_CTRL");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_DRIVE_DEBUG_CTRL:
            return("FBE_CLI_ENCL_CMD_TYPE_DRIVE_DEBUG_CTRL"); 
            break;
        case FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL:
            return("FBE_CLI_ENCL_CMD_TYPE_DRIVE_POWER_CTRL"); 
            break;
        case FBE_CLI_ENCL_CMD_TYPE_DESTROY_OBJECT:
            return("FBE_CLI_ENCL_CMD_TYPE_DESTROY_OBJECT");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_LIST_TRACE_BUF:
            return("FBE_CLI_ENCL_CMD_TYPE_LIST_TRACE_BUF");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_SAVE_TRACE_BUF:
            return("FBE_CLI_ENCL_CMD_TYPE_SAVE_TRACE_BUF");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF:
            return("FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_GET_BUF_SIZE:
            return("FBE_CLI_ENCL_CMD_TYPE_GET_BUF_SIZE");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA:
            return("FBE_CLI_ENCL_CMD_TYPE_GET_BUF_DATA");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_EXP_RESET:
            return("FBE_CLI_ENCL_CMD_TYPE_EXP_RESET");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_TEST_MODE:
            return("FBE_CLI_ENCL_CMD_TYPE_TEST_MODE");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_LAST_ERROR:
            return ("FBE_CLI_ENCL_CMD_TYPE_LAST_ERROR");
            break;
        case FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL:
            return ("FBE_CLI_ENCL_CMD_TYPE_CONNECTOR_CONTROL");
            break;
        default:
            fbe_cli_printf("encl_cmd_type 0x%x unknown and can't be translated.\n", encl_cmd_type);
            return("UNKNOWN");
    }// End of switch
}// End of function fbe_cli_encl_decode_encl_cmd_type

/*!***************************************************************
 * @fn fbe_cli_encl_list_all_trace_buf(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  Function to list all the trace buffers and their status.
 *
 * @param  object_id - The object id of the enclosure.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  27-Jan-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_list_all_trace_buf(fbe_object_id_t object_id)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u8_t * response_buf_p = NULL;
    fbe_u32_t response_buf_size = 0;
    fbe_u8_t number_of_trace_bufs = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    /* Step 1: Get the number of trace buffers. */
    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM,
                                         &number_of_trace_bufs, 1);

    status = fbe_api_enclosure_get_trace_buffer_info(object_id, &eses_page_op, &enclosure_status);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get the number of trace buffers, status:0x%x.\n", status);
        return status;
    }   

    fbe_cli_printf("   \n=== There are totally %d trace buffers ====\n", number_of_trace_bufs);

    if(number_of_trace_bufs == 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;   
    }

    /* Step 2: Get the status of the trace buffers.
     * Allocate memory for the response buffer which will contain the trace buffer info elements retrieved 
     * based on the specified number of the trace buffers. 
     * Adding 1 bytes because the first byte is the number of the trace buffers.
     */
    response_buf_size = number_of_trace_bufs * sizeof(ses_trace_buf_info_elem_struct) + 1;
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size * sizeof(fbe_u8_t));
    
    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS,
        response_buf_p, response_buf_size);
    
    enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    status = fbe_api_enclosure_get_trace_buffer_info(object_id, &eses_page_op, &enclosure_status);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get trace buffer status, status:0x%x.\n", status);
    }
    else
    {
        status = fbe_cli_encl_print_trace_buf_status(eses_page_op.response_buf_p, eses_page_op.response_buf_size);  
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to print trace buffer status, status:0x%x.\n", status);
        }
    }

    free(response_buf_p);

    return status;
}// End of function fbe_cli_encl_list_all_trace_buf



/*!***************************************************************
 * @fn fbe_cli_encl_print_trace_buf_status(fbe_u8_t * response_buf_p,
 *                                        fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Function to print the status for all the trace buffers.
 *
 * @param  response_buf_p - The pointer to the trace buffer info elements block.
 * @param  response_buf_size - The bytes of the responser buffer.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  27-Jan-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_print_trace_buf_status(fbe_u8_t * response_buf_p, fbe_u32_t response_buf_size)
{
    ses_trace_buf_info_elem_struct  * trace_buf_info_elem_p;
    fbe_u8_t total_number_of_trace_bufs = 0;
    fbe_u8_t rqst_number_of_trace_bufs = 0;
    fbe_u8_t number_of_trace_bufs = 0;
    fbe_u8_t i = 0;

    if((response_buf_p == NULL) || (response_buf_size == 0))
    {
        fbe_cli_printf("\nNULL response_buf_p or %d response_buf_size.\n", response_buf_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    total_number_of_trace_bufs = *response_buf_p;
    
    fbe_cli_printf("\tBUFFER_ID\t\tBUFFER_ACTION\n");
    fbe_cli_printf("===================================================\n");
    trace_buf_info_elem_p = (ses_trace_buf_info_elem_struct *)(response_buf_p + 1);

    rqst_number_of_trace_bufs = (response_buf_size - 1) / FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE;
    number_of_trace_bufs = (rqst_number_of_trace_bufs < total_number_of_trace_bufs) ? rqst_number_of_trace_bufs : total_number_of_trace_bufs;

    for(i = 0; i < number_of_trace_bufs; i++)
    {
        fbe_cli_printf("\t%d\t\t%s\n", trace_buf_info_elem_p->buf_id, 
            fbe_eses_decode_trace_buf_action_status(trace_buf_info_elem_p->buf_action));
        
        trace_buf_info_elem_p = fbe_eses_get_next_trace_buf_info_elem_p(trace_buf_info_elem_p, FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE);
    }

    return FBE_STATUS_OK;
}// End of function fbe_cli_encl_print_trace_buf_status()                                      

/*!***************************************************************
 * @fn fbe_cli_encl_trace_buf_ctrl(fbe_object_id_t object_id, 
 *                                 fbe_u8_t buf_id, 
 *                                 fbe_enclosure_mgmt_trace_buf_op_t buf_op)
 ****************************************************************
 * @brief
 *  Function to print the status for all the trace buffers.
 *
 * @param  object_id - The object id of the enclosure.
 * @param  buf_id - The buffer id.
 * @param  fbe_enclosure_mgmt_trace_buf_op_t - The trace buffer operation.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  27-Jan-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_trace_buf_ctrl(fbe_object_id_t object_id, 
                                         fbe_u8_t buf_id, 
                                         fbe_enclosure_mgmt_trace_buf_op_t buf_op)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, buf_id, buf_op, NULL, 0);

    status = fbe_api_enclosure_send_trace_buffer_control(object_id, &eses_page_op);

    return status;
}

/*!***************************************************************
 * @fn fbe_cli_encl_get_buf_size(fbe_object_id_t object_id, 
 *                               fbe_u8_t buf_id, 
 *                               fbe_u32_t * buf_capacity_p)
 ****************************************************************
 * @brief
 *  Function to read the buffer capacity.
 *
 * @param  object_id - The object id of the enclosure.
 * @param  buf_id - The buffer id.
 * @param  buf_capacity_p - The pointer to the buffer capacity.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  28-Jan-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_get_buf_size(fbe_object_id_t object_id, 
                                              fbe_u8_t buf_id, 
                                              fbe_u32_t * buf_capacity_p,
                                              fbe_enclosure_status_t *enclosure_status)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u8_t * response_buf_p = NULL;
    fbe_eses_read_buf_desc_t * read_buf_desc_p = NULL;
    fbe_u32_t buf_capacity = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t retry_count;

    if(buf_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        fbe_cli_printf("\nInvalid buf_id: 0x%x.\n", buf_id);
        return FBE_STATUS_GENERIC_FAILURE; 
    }    
            
    /* Allocate memory for the response buffer which will contain the capacity of the specified trace buffer. */
    response_buf_p = (fbe_u8_t *)malloc(FBE_ESES_READ_BUF_DESC_SIZE * sizeof(fbe_u8_t));

    fbe_api_enclosure_build_read_buf_cmd(&eses_page_op, buf_id, FBE_ESES_READ_BUF_MODE_DESC, 0, response_buf_p, FBE_ESES_READ_BUF_DESC_SIZE);

    //send scsi cmd to read buffer, retry when busy.
    retry_count = 0;
    while(1)
    {
        status = fbe_api_enclosure_read_buffer(object_id, &eses_page_op, enclosure_status);
        retry_count++;

        if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to read the capacity of buffer id %d, encl busy, will retry, count: %d.\n", buf_id, retry_count);
            fbe_api_sleep(200);
        }
        else
        {
            break;
        }
    }

    if(status != FBE_STATUS_OK)
    {
        if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to read the capacity of buffer id %d, encl busy, retry count exhausted.\n", buf_id);
        }
        else
        {
            fbe_cli_printf("\nFailed to read the capacity of buffer id %d, status: 0x%x.\n", buf_id, status);
        }
    }
    else
    {
        read_buf_desc_p = (fbe_eses_read_buf_desc_t *)(response_buf_p);
        buf_capacity = fbe_eses_get_buf_capacity(read_buf_desc_p);

        fbe_cli_printf("\nThe capacity of the buffer id %d is %d.\n", buf_id, buf_capacity);

        if(buf_capacity_p != NULL)
        {
            *buf_capacity_p = buf_capacity;
        }
    }

    free(response_buf_p);

    return status;
}// End of function fbe_cli_encl_get_buf_size

/*!***************************************************************
* @fn fbe_cli_encl_save_buf_data_to_file(fbe_object_id_t object_id,
*                                   fbe_u8_t buf_id,
*                                   char * filename,
*                                   char *temp)
****************************************************************
* @brief
*   The function is to get the buffer capacity first and then save the buffer data to a file.
*   It reads 4KB data every time (The max transfer size for a read buffer command is 4KB.)
*
* @param  object_id - The object id of the enclosure.
* @param  buf_id - The buffer id.
* @param  filename - The pointer to the file name.
* @param  output_to_file - If TRUE then saved to file and if not print on the screen.
* @param  temp - The head string of the output.
*
* @return  fbe_status_t
*
* HISTORY:
*  28-Jan-2009 PHE - Created.
*  12-Aprl-2012 ZDONG - Merged the change from fbe_cli code.
*
****************************************************************/
static fbe_status_t
fbe_cli_encl_save_buf_data_to_file(fbe_object_id_t object_id, fbe_u8_t buf_id,
        const char * filename, fbe_u8_t output_to_file, char *temp)
{
    fbe_u32_t buf_capacity = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    FILE *fp = NULL;
    fbe_u8_t  temp_buff[100] = {0};


    if(buf_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        fbe_cli_printf("\nInvalid buf_id: 0x%x.\n", buf_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Read the buffer capacity.
    status = fbe_cli_encl_get_buf_size(object_id, buf_id, &buf_capacity, &enclosure_status);
    if((status != FBE_STATUS_OK) || (buf_capacity == 0))
    {
        return status;
    }

    _snprintf(temp_buff,sizeof(temp_buff),"\n\n %s object id %d buffer id %d\n\n",temp,object_id,buf_id);

    if(output_to_file)
    {
        if((fp = fopen(filename, "ab+")) == NULL)
        {
            fbe_cli_printf("\nFailed to create/open the file %s.\n", filename);
            return FBE_STATUS_GENERIC_FAILURE;   // bail out if we can't log
        }
        else if(fwrite(temp_buff, sizeof(fbe_u8_t), sizeof(temp_buff), fp) !=  sizeof(temp_buff))
        {
            fbe_cli_printf("\nFailed to write header %s.\n", filename);
        }
    }
    else
    {
        fbe_cli_printf("\n%s\n\n",temp_buff);    //write header first to the console
    }

    // Now collect and output the buffers
    status = read_and_output_buffer(buf_id, buf_capacity, object_id, fp, filename);
    if (output_to_file)
    {
        if(fclose(fp) != 0)
        {
            fbe_cli_printf("\nFailed to close the file %s.\n", filename);
        }
    }
    return status;
}// End of function fbe_cli_encl_save_buf_data_to_file

/*!***************************************************************
 * @fn fbe_cli_encl_send_raw_rcv_diag_cmd(fbe_object_id_t object_id, 
 *                                   fbe_enclosure_raw_rcv_diag_pg_t raw_diag_page
 ****************************************************************
 * @brief
 *   The function allocates 1K response buffer, and send 
 * receive diag page to the object.
 *    
 * @param  object_id - The object id of the enclosure.
 * @param  raw_diag_page - requested page.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  06-Mar-2009 NC - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_cli_encl_send_raw_rcv_diag_cmd(fbe_object_id_t object_id, 
                                   fbe_enclosure_raw_rcv_diag_pg_t raw_diag_page,
                                   fbe_enclosure_status_t *enclosure_status)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u8_t * response_buf_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t  print_offset = 0;

    /* Allocate memory for the response buffer with the size of FBE_ESES_PAGE_MAX_SIZE. */
    response_buf_p = (fbe_u8_t *)malloc(FBE_ESES_PAGE_MAX_SIZE * sizeof(fbe_u8_t));

    eses_page_op.cmd_buf.raw_rcv_diag_page.rcv_diag_page = raw_diag_page;
    eses_page_op.response_buf_p = response_buf_p;
    eses_page_op.response_buf_size = FBE_ESES_PAGE_MAX_SIZE;
    eses_page_op.required_response_buf_size = 0;

    status = fbe_api_enclosure_send_raw_rcv_diag_pg(object_id, &eses_page_op, enclosure_status);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to send pass-through receive diag page to object id %d, status: 0x%x.\n", object_id, status);  
    }
    else
    {
        // we are running with the risk of printing out staff not belong to us
        // when eses page return size is close to what we allocated.
        while (eses_page_op.required_response_buf_size>=print_offset)
        {
            fbe_cli_printf("%03x: ", print_offset);
            fbe_cli_print_bytes(&response_buf_p[print_offset], 16);
            print_offset += 16; //next 16 bytes
        }
    }
    return status;

}// End of function fbe_cli_encl_send_raw_rcv_diag_cmd

/*!*************************************************************************
 * @fn fbe_cli_print_bytes
 *           (char *data, fbe_u8_t size)
 **************************************************************************
 *
 *  @brief
 *      This function print the input string in hex + char format
 *
 *  @param    data - input string
 *  @param    size - number of bytes to print
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Mar-09: Naizhong Chiu- created.
 *
 **************************************************************************/
static void fbe_cli_print_bytes(char *data, fbe_u8_t size)
{
    fbe_u32_t index, print_location;
    char buffer[256];
    unsigned char byte;

    fbe_zero_memory(buffer, 256);

    // we can't really print too much in one line because of buffer limit
    if (size < 50)
    {
        // print hex format
        for (index = 0 , print_location = 0; 
            index < size; 
            index ++, print_location+=3)
        {
            byte = data[index];
            fbe_sprintf(&buffer[print_location], 4, " %02x", byte);
        }

        fbe_sprintf(&buffer[size*3], 4, " | ");

        // print char format
        for (index = 0, print_location =((size+1)*3)  ; 
            index < size; 
            index ++, print_location++)
        {
            byte = data[index];
            /* make sure it's printable */
            if ((byte<' ')||(byte>'~'))
            {
                byte = '.';
            }
            fbe_sprintf(&buffer[print_location], 2, "%c", byte);
        }

        fbe_cli_printf("%s\n", buffer);
    }

}

/*!*************************************************************************
 * @fn fbe_cli_cmd_enclosure_info
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function will 
 *      1) get and print enclosure info.
 *      2) output the enclosure data blob for the specified enclosure.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    29-Jan-2009: Dipak Patel- created.
 *
 **************************************************************************/
void fbe_cli_cmd_enclosure_info(int argc, char** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_elem_group_t  *elem_group_info_ptr = NULL;
    fbe_eses_elem_group_t  *temp_ptr = NULL;
    fbe_enclosure_mgmt_max_elem_group maxElems;
    fbe_u8_t group_id = 0;
       
    /* There should be a minimum of arguments. */
    if(argc == 0)
    {
        fbe_cli_printf("%s", ENCL_ELEM_GROUP_INFO);
        return;
    }
    else if ((argc > 4)||(argc < 3))
    {
        fbe_cli_printf("enclosure envchange invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", ENCL_ELEM_GROUP_INFO);
        return;
    }

    while(argc > 0)
    {
        /* Check the command arguments */
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure envchange, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure envchange, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else
        {
            fbe_cli_printf("enclosure envchange invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;
    } // end of while(argc > 0)

    /* 
    * Get the object ID for the specified enclosure
    */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\ndisplay enclosure data failed to get enclosure object handle status: %d\n",
            status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\ndisplay enclosure data failed to get enclosure object handle\n");
        return;
    }

    fbe_zero_memory(&maxElems, sizeof(fbe_enclosure_mgmt_max_elem_group));
    status = fbe_api_enclosure_get_max_elem_group(object_handle_p, &maxElems);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nenclinfo failed to get max element groups to bus %d, encl %d, status: %d\n",
            port, encl_pos, status);
        return;
    }

    /*Sanity check if we get max element groupds or not*/
    if(maxElems.max_elem_group == 0) 
    {
        fbe_cli_printf("Max Elem Group value is exceeding valid range\n");
        return;
    }

    elem_group_info_ptr = (fbe_eses_elem_group_t *)malloc(sizeof(fbe_eses_elem_group_t) * (maxElems.max_elem_group));

    if(elem_group_info_ptr == NULL)
    {
        fbe_cli_printf("Failed to allocater memory for element group info\n");
        return;
    }

    status = fbe_api_enclosure_elem_group_info(object_handle_p, elem_group_info_ptr, maxElems.max_elem_group);      

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nenclinfo failed to get element group info to bus %d, encl %d, status: %d\n",
            port, encl_pos, status);
        free(elem_group_info_ptr);
        return;
    }
    else
    {
        fbe_cli_printf("Bus:%d Enclosure:%d\n",port,encl_pos);
        temp_ptr =  elem_group_info_ptr;
        fbe_cli_printf("\nGroup_Id   First_index   byte_offset  num_elem    subencl_id     Elem_Type\n");

        for(group_id = 0; group_id < (maxElems.max_elem_group); group_id++)
        {
            fbe_cli_printf("   \n%d",   group_id);
            fbe_cli_printf("\t\t%d",   temp_ptr->first_elem_index);
            fbe_cli_printf("\t\t%d",temp_ptr->byte_offset);
            fbe_cli_printf("\t%d",temp_ptr->num_possible_elems);
            fbe_cli_printf("\t\t%d",temp_ptr->subencl_id); 
            fbe_cli_printf("%s",elem_type_to_text(temp_ptr->elem_type));

            temp_ptr++;
        }
        fbe_cli_printf("\n");
        free(elem_group_info_ptr);
    }

    return;
}

/*!*************************************************************************
 * @fn convertStringToComponentType
 *           (char **componentString)
 **************************************************************************
 *
 *  @brief
 *      This function will convert Component Type strings to the
 *      correct enum value.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    13-Feb-2009: Joe Perry - created.
 *
 **************************************************************************/
static fbe_enclosure_component_types_t convertStringToComponentType(char **componentString)
{
    fbe_enclosure_component_types_t componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;

    if(strcmp(*componentString, "encl") == 0)
    {
        componentType = FBE_ENCL_ENCLOSURE;
    }
    else if(strcmp(*componentString, "lcc") == 0)
    {
        componentType = FBE_ENCL_LCC;
    }
    else if(strcmp(*componentString, "ps") == 0)
    {
        componentType = FBE_ENCL_POWER_SUPPLY;
    }
    else if(strcmp(*componentString, "cool") == 0)
    {
        componentType = FBE_ENCL_COOLING_COMPONENT;
    }
    else if(strcmp(*componentString, "drv") == 0)
    {
        componentType = FBE_ENCL_DRIVE_SLOT;
    }
    else if(strcmp(*componentString, "temp") == 0)
    {
        componentType = FBE_ENCL_TEMP_SENSOR;
    }
    else if(strcmp(*componentString, "conn") == 0)
    {
        componentType = FBE_ENCL_CONNECTOR;
    }
    else if(strcmp(*componentString, "exp") == 0)
    {
        componentType = FBE_ENCL_EXPANDER;
    }
    else if(strcmp(*componentString, "phy") == 0)
    {
        componentType = FBE_ENCL_EXPANDER_PHY;
    }
    else if(strcmp(*componentString, "disp") == 0)
    {
        componentType = FBE_ENCL_DISPLAY;
    }
    else if(strcmp(*componentString, "iomodule") == 0)  //Processor Enclosure component Type
    {
        componentType = FBE_ENCL_IO_MODULE;
    }
    else if(strcmp(*componentString, "ioport") == 0)
    {
        componentType = FBE_ENCL_IO_PORT;
    }
    else if(strcmp(*componentString, "bem") == 0)
    {
        componentType = FBE_ENCL_BEM;
    }
    else if(strcmp(*componentString, "mgmt") == 0)
    {
        componentType = FBE_ENCL_MGMT_MODULE;
    }
    else if(strcmp(*componentString, "mezzanine") == 0)
    {
        componentType = FBE_ENCL_MEZZANINE;
    }
    else if(strcmp(*componentString, "platform") == 0)
    {
        componentType = FBE_ENCL_PLATFORM;
    }
    else if(strcmp(*componentString, "suitcase") == 0)
    {
        componentType = FBE_ENCL_SUITCASE;
    }
    else if(strcmp(*componentString, "bmc") == 0)
    {
        componentType = FBE_ENCL_BMC;
    }
    else if(strcmp(*componentString, "misc") == 0)
    {
        componentType = FBE_ENCL_MISC;
    }
    else if(strcmp(*componentString, "fltreg") == 0)
    {
        componentType = FBE_ENCL_FLT_REG;
    }
    else if(strcmp(*componentString, "slaveport") == 0)
    {
        componentType = FBE_ENCL_SLAVE_PORT;
    }
    else if(strcmp(*componentString, "temperature") == 0)
    {
        componentType = FBE_ENCL_TEMPERATURE;
    }
    else if(strcmp(*componentString, "sps") == 0)
    {
        componentType = FBE_ENCL_SPS;
    }
    else if(strcmp(*componentString, "battery") == 0)
    {
        componentType = FBE_ENCL_BATTERY;
    }
    else if(strcmp(*componentString, "cachecard") == 0)
    {
        componentType = FBE_ENCL_CACHE_CARD;
    }
    else if(strcmp(*componentString, "dimm") == 0)
    {
        componentType = FBE_ENCL_DIMM;
    }
    else if(strcmp(*componentString, "ssd") == 0)
    {
        componentType = FBE_ENCL_SSD;
    }

    return componentType;

}   // end of convertStringToComponentType



/*!*************************************************************************
 * @fn fbe_cli_cmd_enclosure_expStringOut
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function will send the specified StringOut command to the 
 *      specified Expander.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    21-Jan-2009: Joe Perry - created.
 *
 **************************************************************************/
void fbe_cli_cmd_enclosure_expStringOut(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
    fbe_u8_t        *stringOutPtr = NULL;
    fbe_u32_t       stringLen = 0;

    /*
     * Always need port & enclosure position plus possibly one more argument
     */
    if (argc < 5)
    {
        fbe_cli_printf("enclosure expStringOut invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", EXPSTRINGOUT_USAGE);
        return;
    }

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    stringOutPtr = esesPageOp.cmd_buf.stringOutInfo.stringOutCmd;

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure expStringOut, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure expStringOut, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-cmd") == 0)
        {
            argc--;
            argv++;

            if (argc == 0)
            {
                fbe_cli_printf("Enclosure expStringOut. Too few arguments %s\n", *argv);
                return;
            }

            while (argc)
            {
                stringLen += (fbe_u32_t)strlen(*argv);
                if (stringLen > FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH)
                {
                    fbe_cli_printf("enclosure expStringOut, command too long\n");
                    return;
                }
                strncat(stringOutPtr, *argv, FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH-strlen(stringOutPtr)-2);
                stringLen++;
                argc--;
                argv++;

                if (argc == 0)
                {
                    strncat(stringOutPtr, "\n", 1);
                }
                else
                {
                    strncat(stringOutPtr, " ", 1);
                }
            }
        }
        else
        {
            fbe_cli_printf("enclosure expStringOut invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;
    } // end of while(argc > 0)

    /* Get the object ID for the specified enclosure */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nexpStringOut failed to get enclosure object handle status: %d\n",
            status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nexpStringOut failed to get enclosure object handle\n");
        return;
    }

    /* Format command & send to Enclosure object */
    status = fbe_api_enclosure_send_exp_string_out_control(object_handle_p, &esesPageOp);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nexpStringOut failed to send control to bus %d, encl %d, status: %d\n",
            port, encl_pos, status);
        return;
    }

    return;

}   // end of fbe_cli_cmd_enclosure_expStringOut

/**
 **************************************************************************/
void fbe_cli_cmd_display_dl_info(int argc , char ** argv)
{

    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t                   num_bytes = 0;
    fbe_eses_encl_fup_info_t    dl_info;

    if (argc < 4) 
    {
        fbe_cli_error("%s", ENCLDLINFO_USAGE);
        return;
    }

    // Parse the command line.
    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("dl info, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("dl info, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }

        argc--;
        argv++;

    }   // end of while

    //Get the object ID for the specified enclosure
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\ndisplay enclosure dl info failed to get enclosure object handle status: %d\n",
            status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_error("\ndisplay enclosure dl info failed to get enclosure object handle\n");
        return;
    }

    // init the struct that will return the data
    fbe_zero_memory(&dl_info, sizeof(fbe_eses_encl_fup_info_t));

    // Get the enclosure component data blob via the API.
    num_bytes = sizeof(fbe_eses_encl_fup_info_t);
    status = fbe_api_enclosure_get_dl_info(object_handle_p, &dl_info, num_bytes);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\ndisplay enclosure dl info failed to get enclosure data status:%d\n", 
            status);
        return;
    }

    // Print out the download info from base enclosure object
    fbe_cli_printf("  Image Type: %s (0x%x), Side: %d\n",
        fbe_eses_enclosure_fw_targ_to_text(dl_info.enclFupComponent),
        dl_info.enclFupComponent,
        dl_info.enclFupComponentSide);

    fbe_cli_printf("  Operation: %s (0x%x) \n",
        fbe_eses_enclosure_fw_op_to_text(dl_info.enclFupOperation),
        dl_info.enclFupOperation);

    fbe_cli_printf("    Status:    %s (0x%x)\n",
        fbe_eses_enclosure_fw_status_to_text(dl_info.enclFupOperationStatus),
        dl_info.enclFupOperationStatus);

    fbe_cli_printf("    ExtStatus: %s (0x%x)\n",
        fbe_eses_enclosure_fw_extstatus_to_text(dl_info.enclFupOperationAdditionalStatus),
        dl_info.enclFupOperationAdditionalStatus);

    fbe_cli_printf("  BytesXferred/ImageSize: 0x%x/0x%x \n",
        dl_info.enclFupBytesTransferred,
        dl_info.enclFupImageSize);

    fbe_cli_printf("  Use Tunnelling: %s \n",
        (dl_info.enclFupUseTunnelling==TRUE)?"YES":"NO");

    fbe_cli_printf("  Retry Count: %x \n",dl_info.enclFupOperationRetryCnt);
    fbe_cli_printf("  Xfer Size: %x \n",dl_info.enclFupCurrTransferSize);
    fbe_cli_printf("  Image Pointer: %p \n",dl_info.enclFupImagePointer);

    return;
}

/*!*************************************************************************
 * @fn fbe_cli_cmd_slottophy(int argc , char ** argv)
 **************************************************************************
 *  @brief
 *      This function displays the drive slot to phy mapping info.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    19-Mar-2009: PHE - Created.
 *
 **************************************************************************/
void fbe_cli_cmd_slottophy(int argc , char ** argv)
{
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_slot_number_t slot_num_start = 0;
    fbe_enclosure_slot_number_t slot_num_end = 0;
    fbe_enclosure_status_t       enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    /*
     * There should be a minimum of arguments.
     */
    if(argc == 0)
    {
        fbe_cli_printf("%s", SLOTTOPHY_USAGE);
        return;
    }

    if (argc != 7)
    {
        fbe_cli_printf("\nenclbuf: Invalid number of arguments %d.\n", argc);
        fbe_cli_printf("%s", SLOTTOPHY_USAGE);
        return;
    }    

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        /*
         * Check the command arguments
         */
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide bus #.\n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide enclosure position #.\n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-slot") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide the starting slot number and the ending slot number.\n");
                return;
            }
            slot_num_start = (fbe_u8_t)strtoul(*argv, 0, 0);

            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclbuf: Please provide the ending slot number.\n");
                return;
            }
            slot_num_end = (fbe_u8_t)strtoul(*argv, 0, 0);
        }
        else
        {
            fbe_cli_printf("\nenclbuf: Invalid argument %s.\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    /* 
     * Get the object ID for the specified enclosure
     */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get enclosure object id, status: %d.\n", status);
        return;
    }
    if(object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nInvalid enclosure object id.\n");
        return;
    }
   
    if(slot_num_start > slot_num_end)
    {
        fbe_cli_printf("\nInvalid slot numbers, start: %d, end: %d.\n", slot_num_start, slot_num_end);
        return;
    }

    status = fbe_cli_encl_get_slot_to_phy_mapping(object_id, slot_num_start, slot_num_end, &enclosure_status);    

    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("\nenclbuf: Failed to send slottophy cmd to bus %d, encl %d, "
           "slots %d -> %d, status: 0x%x.\n",
            port, encl_pos, slot_num_start, slot_num_end, status);
    }
    else
    {
       fbe_cli_printf("\nenclbuf: Successfully issued slottophy cmd to bus %d, encl %d, "
           "slots %d -> %d, status: 0x%x.\n",
            port, encl_pos, slot_num_start, slot_num_end, status);
    }
    
    return;
} // End of - fbe_cli_cmd_slottophy
 
/*!***************************************************************
 * @fn fbe_cli_encl_get_slot_to_phy_mapping(fbe_object_id_t object_id, 
 *                                       fbe_enclosure_slot_number_t slot_num_start, 
 *                                       fbe_enclosure_slot_number_t slot_num_end,
 *                                       fbe_enclosure_status_t *enclosure_status)
 ****************************************************************
 * @brief
 *  Function to get the slot to phy mapping for the specified group of drive slots.
 *
 * @param  object_id - The object id of the enclosure.
 * @param  slot_num_start - The first drive slot number to get the phy mapping for.
 * @param  slot_num_end - The last drive slot number to get the phy mapping for.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  18-Mar-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_get_slot_to_phy_mapping(fbe_object_id_t object_id, 
                                                    fbe_enclosure_slot_number_t slot_num_start, 
                                                    fbe_enclosure_slot_number_t slot_num_end,
                                                    fbe_enclosure_status_t *enclosure_status)
{
    fbe_enclosure_mgmt_ctrl_op_t  slot_to_phy_mapping_op;
    fbe_u8_t                    * response_buf_p = NULL;
    fbe_u32_t                     response_buf_size = 0;
    fbe_status_t                  status = FBE_STATUS_OK;

    if(slot_num_start > slot_num_end)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    response_buf_size = (slot_num_end - slot_num_start + 1) * sizeof(fbe_enclosure_mgmt_slot_to_phy_mapping_info_t);
    /* Allocate the memory. */
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size);
    
    /* Fill in the struct fbe_enclosure_mgmt_ctrl_op_t with slot_num_start, slot_num_end,
     * response_buf_p, response_buf_size.
     */
    fbe_api_enclosure_build_slot_to_phy_mapping_cmd(&slot_to_phy_mapping_op, slot_num_start, slot_num_end,
                                                    response_buf_p, response_buf_size);
    /* Send the operation to the physical package. */
    status = fbe_api_enclosure_get_slot_to_phy_mapping(object_id, &slot_to_phy_mapping_op, enclosure_status);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get slot to phy mapping, status:0x%x.\n", status);
    }
    else
    {
        status = fbe_cli_encl_print_slot_to_phy_mapping(slot_num_start, 
                                                        slot_num_end,
                                                        slot_to_phy_mapping_op.response_buf_p, 
                                                        slot_to_phy_mapping_op.response_buf_size);  
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to print slot to phy mapping, status:0x%x.\n", status);
        }
    }

    free(response_buf_p);

    return status;
}// End of function fbe_cli_encl_get_slot_to_phy_mapping

/*!***************************************************************
 * @fn fbe_cli_encl_print_slot_to_phy_mapping(fbe_enclosure_slot_number_t slot_num_start,
 *                                           fbe_enclosure_slot_number_t slot_num_end,
 *                                           fbe_u8_t * response_buf_p, 
 *                                           fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Function to print the slot to phy mapping info.
 *
 * @param  slot_num_start - The first drive slot number to get the phy mapping for.
 * @param  slot_num_end - The last drive slot number to get the phy mapping for.
 * @param  response_buf_p - The pointer to the block of the mapping info for requested drive slots.
 * @param  response_buf_size - The bytes of the responser buffer.
 *
 * @return  fbe_status_t 
 *      FBE_STATUS_OK - no error.
 *      otherwise - error found.
 *
 * HISTORY:
 *  19-Mar-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_encl_print_slot_to_phy_mapping(fbe_enclosure_slot_number_t slot_num_start,
                                                           fbe_enclosure_slot_number_t slot_num_end,
                                                           fbe_u8_t * response_buf_p, 
                                                           fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_slot_to_phy_mapping_info_t * mapping_info_p = NULL;
    fbe_enclosure_slot_number_t   total_number_of_slots = 0;
    fbe_enclosure_slot_number_t   rqst_number_of_slots = 0;
    fbe_enclosure_slot_number_t   number_of_slots = 0;
    fbe_enclosure_slot_number_t   slot_num = 0;

    if((response_buf_p == NULL) || (response_buf_size == 0))
    {
        fbe_cli_printf("\nNULL response_buf_p or %d response_buf_size.\n", response_buf_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    total_number_of_slots = slot_num_end - slot_num_start + 1;
    
    fbe_cli_printf("\tSLOT_NUM\tPHY_ID\t\tEXPANDER_SAS_ADDRESS\n");
    fbe_cli_printf("============================================================\n");
    mapping_info_p = (fbe_enclosure_mgmt_slot_to_phy_mapping_info_t *)(response_buf_p);

    rqst_number_of_slots = response_buf_size / sizeof(fbe_enclosure_mgmt_slot_to_phy_mapping_info_t);
    number_of_slots = (rqst_number_of_slots < total_number_of_slots) ? rqst_number_of_slots : total_number_of_slots;

    for(slot_num = slot_num_start; slot_num < slot_num_start + number_of_slots; slot_num ++)
    {
        fbe_cli_printf("\t%d\t\t%d\t\t0x%llX\n", slot_num, 
            mapping_info_p->phy_id, 
           (unsigned long long)mapping_info_p->expander_sas_address);
        
        mapping_info_p ++;
    }

    return FBE_STATUS_OK;
}// End of function fbe_cli_encl_print_slot_to_phy_mapping()                                      
/*!*************************************************************************
* @void fbe_cli_cmd_enclstat(int argc , char ** argv);
**************************************************************************
*
*  @brief
*  This function will get and print enclosure state info.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  24-Feb-2009: Dipak Patel - created.
*  30-Apr-2009: Nayana Chaudhari - include enclosure fault info.
*  02-Aug-2010: Sujay Ranjan - Added voyager support
* **************************************************************************/
void fbe_cli_cmd_enclstat(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID;
    fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT, user_port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t encl_pos = FBE_API_ENCLOSURES_PER_BUS, user_encl_pos = FBE_API_ENCLOSURES_PER_BUS;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;

    /* There should be a minimum of arguments. */
    fbe_bool_t b_target_all = FBE_TRUE;

    /* Determines if we should target this command at specific bus. */
    fbe_bool_t b_target_port = FBE_FALSE;

    /* Determines if we should target this command at specified enclosure. */
    fbe_bool_t b_target_encl = FBE_FALSE;

    /* Parse the command line. */
    while(argc > 0)
    {
        /* Check the command type */
        if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("%s", ENCLSTAT_USAGE);
                return;
            }
        }
        else if ((strcmp(*argv, "-bus") == 0) ||
                  (strcmp(*argv, "-b") == 0))
        {
            /* Filter by a specific bus.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-bus, expected bus, too few arguments \n");
                return;
            }
            user_port = (fbe_u32_t)strtoul(*argv, 0, 0);
            if( user_port >= FBE_API_PHYSICAL_BUS_COUNT)
            {
                fbe_cli_printf("Invalid bus number. Please enter bus number between 0 to %d \n", FBE_API_PHYSICAL_BUS_COUNT);
                return;
            }
            b_target_all = FBE_FALSE;
            b_target_port = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-enclosure") == 0) ||
                  (strcmp(*argv, "-e") == 0))
        {
            /* Filter by a specific enclosure.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-enclosure, expected enclosure number, too few arguments \n");
                return;
            }
            user_encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
            if( user_encl_pos >= FBE_API_ENCLOSURES_PER_BUS)
            {
                fbe_cli_printf("Invalid enclosure number. Please enter enclosure number between 0 to %d \n", FBE_API_ENCLOSURES_PER_BUS);
                return;
            }
            b_target_all = FBE_FALSE;
            b_target_encl = FBE_TRUE;
        }
        else
        {
            fbe_cli_printf("Invalid number of arguments %d\n", argc);
            fbe_cli_printf("%s", ENCLSTAT_USAGE);
            return;
        }
        argc--;
        argv++;
    }

    for (port=0;port<FBE_API_PHYSICAL_BUS_COUNT;port++)
    {
        if(b_target_port && (port != user_port))
        {
            continue;
        }
        status = fbe_api_get_port_object_id_by_location(port, &object_handle_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\failed to get port object handle status: %d\n",
                status);
            continue;
        }
        for(encl_pos=0;encl_pos<FBE_API_MAX_ALLOC_ENCL_PER_BUS;encl_pos++)
        {
            if(b_target_encl && (encl_pos != user_encl_pos))
            {
                continue;
            }
            status = fbe_api_get_enclosure_object_ids_array_by_location(port,encl_pos, &enclosure_object_ids);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\failed to get enclosure object handle status: %d\n",
                    status);
                    continue;
            }
            object_handle_p = enclosure_object_ids.enclosure_object_id;
            if(object_handle_p != FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_print_enclstatinfo(port, encl_pos, enclosure_object_ids);
            }
        }
    }
    return;
} //End of fbe_cli_cmd_enclstat

void fbe_cli_print_enclstatinfo(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids)
{
    fbe_object_id_t obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t comp_object_handle = FBE_OBJECT_ID_INVALID,i=0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info_ptr = NULL;
    fbe_base_object_mgmt_get_enclosure_info_t *comp_enclosure_info_ptr = NULL;
    fbe_lifecycle_state_t state;
    fbe_enclosure_fault_t enclosure_fault_info;
    fbe_class_id_t  class_id;
    fbe_enclosure_component_block_t *compBlockPtr;
    slot_flags_t   driveStat[FBE_API_ENCLOSURE_SLOTS] = {0};
    fbe_enclosure_component_block_t *comp_compBlockPtr;
    fbe_u32_t  component_count;
    fbe_u8_t max_slot = 0;

    obj_id = enclosure_object_ids.enclosure_object_id;
    
    if(obj_id != FBE_OBJECT_ID_INVALID)
    {
        // Get the lifecycle condition first          
        status = fbe_api_get_object_lifecycle_state (obj_id, &state, FBE_PACKAGE_ID_PHYSICAL);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nfail to get lifecycle state for :%d port: %d and enclosure:%d\n",
                status, port_num, encl_pos);
            return;
        }

        // if enclosure is failed, get and print fault information
        if (state == FBE_LIFECYCLE_STATE_FAIL)
        {
            status = fbe_api_enclosure_get_fault_info (obj_id, &enclosure_fault_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nfail to get enclosure fault info. status:%d\n",
                    status);
                return;
            }

            status = fbe_api_get_object_class_id(obj_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);        
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nfailed to get enclosure object class id:%d\n", 
                    status);
                return;
            }
            fbe_base_object_trace_class_id(class_id, fbe_cli_trace_func, NULL);
            fbe_cli_printf(" (Bus:%d Enclosure:%d) ",port_num,encl_pos);
            fbe_cli_printf("LifeCycleState:%s\n",fbe_cli_print_LifeCycleState(state));
            // if additional_status belongs to scsi errors, print faultyCompType as failed
            // opcode otherwise print it as enclosure component.
            fbe_cli_printf("FaultyCompType:%s", 
                (fbe_base_enclosure_if_scsi_error(enclosure_fault_info.additional_status) ? \
                fbe_eses_enclosure_print_scsi_opcode(enclosure_fault_info.faultyComponentType) : \
                enclosure_access_printComponentType(enclosure_fault_info.faultyComponentType)));
            fbe_cli_printf("   FaultyCompIndex:0x%x   ", enclosure_fault_info.faultyComponentIndex);
            fbe_base_enclosure_print_encl_fault_symptom(enclosure_fault_info.faultSymptom, 
                fbe_cli_trace_func, 
                NULL);
            fbe_cli_printf("\n");
        } // if state
        // otherwise, get and print enclosure information
        else
        {
            /*allocate memory*/
            status = fbe_api_enclosure_setup_info_memory(&enclosure_info_ptr);
            if(status != FBE_STATUS_OK)
            {
                return;
            }
 
            // Get the enclosure component data blob via the API.
            status = fbe_api_enclosure_get_info(obj_id, enclosure_info_ptr);
            if(status != FBE_STATUS_OK)
            {
                fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
                fbe_cli_printf("\nfailed to get enclosure data status:%d\n", status);
                return;
            }

            /* Enclosure Data contains some pointers that must be converted */
            edalStatus = fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)enclosure_info_ptr);
            if(edalStatus != FBE_EDAL_STATUS_OK)
            {
                fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
                fbe_cli_printf("\nEDAL: failed to update block pointers %p, status:0x%x\n", enclosure_info_ptr, edalStatus);
                return;
            }

            compBlockPtr = (fbe_enclosure_component_block_t *)(fbe_edal_block_handle_t)enclosure_info_ptr;

            fbe_cli_printf("%s", fbe_edal_print_Enclosuretype(compBlockPtr->enclosureType));
            fbe_cli_printf(" (Bus:%d Enclosure:%d) ", port_num, encl_pos);
            fbe_cli_printf("LifeCycleState:%s ", fbe_cli_print_LifeCycleState(state));
            fbe_cli_printFwRevs((fbe_edal_block_handle_t)enclosure_info_ptr);
            edalStatus = fbe_edal_getU8((fbe_edal_block_handle_t)enclosure_info_ptr,
                FBE_ENCL_MAX_SLOTS,
                FBE_ENCL_ENCLOSURE,
                0,
                &max_slot);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
                fbe_cli_printf("\nEDAL: could not retrieve enclosure max slots, status: 0x%x\n", edalStatus);
                return;
            }

            edalStatus = fbe_cli_fill_enclosure_drive_info((fbe_edal_block_handle_t)enclosure_info_ptr,driveStat,&component_count);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
                fbe_cli_printf("\nEDAL: failed to fill the enclosure drive info, status: 0x%x\n", edalStatus);
                return;
            }

            if(component_count != 0)
            {
                fbe_cli_print_drive_info(driveStat,max_slot);

                fbe_zero_memory(driveStat, sizeof(driveStat));
            }   
            if(max_slot > FBE_API_MAX_DRIVE_EE )
            {
                for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS && enclosure_object_ids.component_count >0; i++)
                {
                    comp_object_handle = enclosure_object_ids.comp_object_id[i];
                    if (comp_object_handle == FBE_OBJECT_ID_INVALID)
                    {
                        continue;
                    }

                    if(comp_enclosure_info_ptr == NULL)
                    {
                        status = fbe_api_enclosure_setup_info_memory(&comp_enclosure_info_ptr);
                        if(status != FBE_STATUS_OK)
                        {
                            fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
                            return;
                        }
                    }
                    else
                    {
                        fbe_zero_memory(comp_enclosure_info_ptr, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
                    }

                    //Get the enclosure component data blob via the API.
                    status = fbe_api_enclosure_get_info(comp_object_handle,comp_enclosure_info_ptr);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nfailed to get enclosure info, status:%d\n", status);
                        continue;
                    }

                    /* Enclosure Data contains some pointers that must be converted */
                    edalStatus = fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)comp_enclosure_info_ptr);
                    if (edalStatus != FBE_EDAL_STATUS_OK)
                    {
                        fbe_cli_printf("\nEDAL: failed to update block pointers %p, status:0x%x\n", enclosure_info_ptr, edalStatus);
                        continue;
                    }

                    comp_compBlockPtr = (fbe_enclosure_component_block_t *)(fbe_edal_block_handle_t)enclosure_info_ptr;
                    edalStatus = fbe_cli_fill_enclosure_drive_info((fbe_edal_block_handle_t)comp_enclosure_info_ptr, driveStat, &component_count);
                    if (edalStatus != FBE_EDAL_STATUS_OK)
                    {
                        fbe_cli_printf("\nEDAL: failed to fill the enclosure drive info, status: 0x%x\n", edalStatus);
                    }

                    // fbe_cli_print_drive_info(driveStat,component_count); 
                }
                fbe_cli_print_drive_info(driveStat,max_slot); 
            }           
        }// else state
    }
    if(enclosure_info_ptr != NULL)
    {
        fbe_api_enclosure_release_info_memory(enclosure_info_ptr);
    }
    if(comp_enclosure_info_ptr != NULL)
    {
        fbe_api_enclosure_release_info_memory(comp_enclosure_info_ptr);
    }
    return;
}

/*!*************************************************************************
*                         @fn fbe_cli_printFwRevs
**************************************************************************
*
*  DESCRIPTION:
*  @brief
*      This function will output the local LCC firmware rev.
*
*  PARAMETERS:
*      componentTypeDataPtr - pointer to the enclosure components
*
*  HISTORY:
*   28-feb-2009 : Dipak Patel - Created
*   29-Apr-2010 : NCHIU - modified to print the rev from LCC component.
*
**************************************************************************/
void
fbe_cli_printFwRevs(fbe_edal_block_handle_t baseCompBlockPtr)
{

    fbe_edal_status_t      status;
    char  fw_rev[MAX_FW_REV_STR_LEN+1];
    fbe_u8_t side_id;
    fbe_edal_fw_info_t              fw_info;
    fbe_u32_t              index;     

    fbe_zero_memory(fw_rev, MAX_FW_REV_STR_LEN + 1);           
    fbe_edal_getEnclosureSide(baseCompBlockPtr,&side_id);

    index = fbe_edal_find_first_edal_match_U8(baseCompBlockPtr,
        FBE_ENCL_COMP_SIDE_ID,  //attribute
        FBE_ENCL_LCC,  // Component type
        0, //starting index
        side_id);

    if(index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        status = fbe_edal_getStr(baseCompBlockPtr,
            FBE_ENCL_COMP_FW_INFO, 
            FBE_ENCL_LCC,
            index,
            sizeof(fw_info),
            (char *)&fw_info);

        if ((status == FBE_EDAL_STATUS_OK) && (fw_info.valid))
        {
            fbe_copy_memory(fw_rev, fw_info.fwRevision, MAX_FW_REV_STR_LEN);
            fbe_cli_printf ("BundleRev:%s",fw_rev);
        }
        else
        {
            fbe_cli_printf ("BundleRev: INVALID");
        }
    }
    else
    {
        fbe_cli_printf ("SIDE_ID: INVALID");
    }
}




/*!*************************************************************************
 *                         @fn fbe_cli_printSpecificDriveData
 **************************************************************************
 *
 *  DESCRIPTION:
 *  @brief
 *      This function will trace Specific Drive/Slot component data for fbe_cli.
 *
 *  PARAMETERS:
 *      componentTypeDataPtr - pointer to drive component;
 *
 *  RETURN VALUES/ERRORS:
 *      
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Feb-2009: Dipak Patel - Created
 *    18-Jul-2011: Vinay Patki - Edited
 *
 **************************************************************************/
void
fbe_cli_printSpecificDriveData(fbe_edal_block_handle_t baseCompBlockPtr)
{
   slot_flags_t                driveStat;
   fbe_u8_t                    index;
   fbe_edal_status_t           status;
   fbe_u32_t                   component_count;


   status = fbe_edal_getSpecificComponentCount(baseCompBlockPtr,
                                               FBE_ENCL_DRIVE_SLOT,
                                               &component_count);
   // loop through all Drive/Slots
   if(status == FBE_EDAL_STATUS_OK)
   {
       fbe_cli_printf("\nSlot :");

       for (index = 0; index < component_count; index++)
       {
          fbe_cli_printf("%3d",index);
       }

       fbe_cli_printf("\nState:");

       for (index = 0; index < component_count; index++)
       {
          fbe_zero_memory(&driveStat, sizeof(slot_flags_t));

          status = fbe_edal_getBool((void *)baseCompBlockPtr,
                                    FBE_ENCL_COMP_INSERTED,
                                    FBE_ENCL_DRIVE_SLOT,
                                    index,
                                    &(driveStat.driveInserted));

          /*When getting any flag of drive slot fails, we print ERR.
          */
          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          status = fbe_edal_getBool((void *)baseCompBlockPtr,
                                     FBE_ENCL_COMP_FAULTED,
                                     FBE_ENCL_DRIVE_SLOT,
                                     index,
                                     &(driveStat.driveFaulted));

          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          status = fbe_edal_getBool((void *)baseCompBlockPtr,
                                     FBE_ENCL_COMP_POWERED_OFF,
                                     FBE_ENCL_DRIVE_SLOT,
                                     index,
                                     &(driveStat.drivePoweredOff));

          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          status = fbe_edal_getBool((void *)baseCompBlockPtr,
                                     FBE_ENCL_DRIVE_LOGGED_IN,
                                     FBE_ENCL_DRIVE_SLOT,
                                     index,
                                     &(driveStat.driveLoggedIn));

          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          status = fbe_edal_getBool((void *)baseCompBlockPtr,
                                     FBE_ENCL_DRIVE_BYPASSED,
                                     FBE_ENCL_DRIVE_SLOT,
                                     index,
                                     &(driveStat.driveBypassed));

          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          status = fbe_edal_getDrivePhyBool((void *)baseCompBlockPtr,
                                     FBE_ENCL_EXP_PHY_READY,
                                     index,
                                     &(driveStat.phyReady));

          if(status != FBE_EDAL_STATUS_OK)
          {
              fbe_cli_printf("ERR");
              continue;
          }

          fbe_cli_printf("%s",fbe_eses_debug_decode_slot_state(&driveStat));
       }
       fbe_cli_printf("\n");  
       return;
   }

    fbe_cli_printf ("Failed to get specific drive data\n");
}

/*!*************************************************************************
 * @fn fbe_cli_cmd_raw_inquiry(int argc , char ** argv)
 **************************************************************************
 *  @brief
 *      This function will process all the command types related to 
 *  SCSI Inquiry VPD Pages command.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *    25-Mar-2009: AS - Created.
 *
 **************************************************************************/
void fbe_cli_cmd_raw_inquiry(int argc , char ** argv)
{
    fbe_u8_t evpd = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_enclosure_raw_inquiry_vpd_pg_t raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    /*
     * There should be a minimum of arguments.
     */
    if(argc != 6)
    {
        fbe_cli_printf("\nencl_inquiry: Invalid number of arguments %d.\n", argc);
        fbe_cli_printf("%s", ENCL_INQ_USAGE);
        return;
    }

    /*
     * Parse the command line.
     */
    while(argc > 0)
    {
        /*
         * Check the command arguments
         */
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_inquiry: Please provide bus #.\n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_inquiry: Please provide enclosure #.\n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-cmd") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_inquiry: Please provide the eses page.\n");
                return;
            }
        
            if(strcmp(*argv, "std") == 0)
            {
                raw_inquiry_vpd_pg = 0;
                evpd = 0;
            }
            else if(strcmp(*argv, "sup") == 0)
            {
                if(raw_inquiry_vpd_pg != FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_inquiry: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUPPORTED;
                evpd = 1;
                //fbe_cli_printf("\nencl_inquiry: Supported Inquiry VPD Page is not supported for now\n");
            }
            else if(strcmp(*argv, "unit_sno") == 0)
            {
                if(raw_inquiry_vpd_pg != FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_inquiry: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_UNIT_SERIALNO;
                evpd = 1;
                //fbe_cli_printf("\nencl_inquiry: Unit Serial Number VPD Page is not supported for now\n");
            }
            else if(strcmp(*argv, "dev_id") == 0)
            {
                if(raw_inquiry_vpd_pg != FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_inquiry: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_DEVICE_ID;
                evpd = 1;
                //fbe_cli_printf("\nencl_inquiry: Device Identifier VPD Page is not supported for now\n");
            }
            else if(strcmp(*argv, "ext") == 0)
            {
                if(raw_inquiry_vpd_pg != FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_inquiry: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_EXTENDED;
                evpd = 1;
                //fbe_cli_printf("\nencl_inquiry: Extended VPD Page is not supported for now\n");
            }
            else if(strcmp(*argv, "sub_encl") == 0)
            {
                if(raw_inquiry_vpd_pg != FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
                {
                    fbe_cli_printf("\nencl_inquiry: Too many commands %s.\n", *argv);
                    return;
                }            

                raw_inquiry_vpd_pg = FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUBENCL;
                evpd = 1;
                //fbe_cli_printf("\nencl_inquiry: Sub Enclosure VPD Page is not supported for now\n");
            }
            else 
            {
                fbe_cli_printf("\nencl_inquiry: Invalid commands %s.\n", *argv);
                return;
            }            
   
        }        
        else
        {
            fbe_cli_printf("\nencl_inquiry: Invalid argument %s.\n", *argv);
            return;
        }
        argc--;
        argv++;

    } // end of while(argc > 0)

    /* 
     * Get the object ID for the specified enclosure
     */
    status = fbe_api_get_enclosure_object_id_by_location(port, encl_pos, &object_handle_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nencl_inquiry: Failed to get enclosure object handle status: %d,\n", status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nencl_inquiry: Invalid enclosure object handle.\n");
        return;
    }

    if(raw_inquiry_vpd_pg == FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID)
    {
        fbe_cli_printf("\nencl_inquiry: no page specified.\n");
        return;
    }            

    status = fbe_cli_encl_send_raw_scsi_cmd(object_handle_p, evpd, raw_inquiry_vpd_pg, &enclosure_status);

    if(status != FBE_STATUS_OK)
    {
       fbe_cli_printf("\nencl_inquiry: Failed to send cmd %d to bus %d, encl %d, status: 0x%x.\n",
            raw_inquiry_vpd_pg, port, encl_pos, status);
    }
    
    return;
}

/*!***************************************************************
 * @fn fbe_cli_encl_send_raw_scsi_cmd(fbe_object_id_t object_id, 
 *                                   fbe_enclosure_raw_inquiry_vpd_pg_t raw_inquiry_vpd_pg
 ****************************************************************
 * @brief
 *   The function allocates 1K response buffer, and send 
 * raw inquiry page to the object.
 *    
 * @param  object_id - The object id of the enclosure.
 * @param  raw_inquiry_vpd_pg - requested page.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  25-Mar-2009 AS - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_cli_encl_send_raw_scsi_cmd(fbe_object_id_t object_id, fbe_u8_t evpd,
                                   fbe_u8_t raw_inquiry_vpd_pg,
                                   fbe_enclosure_status_t *enclosure_status)
{
    fbe_u32_t  print_offset = 0;
    fbe_u8_t *response_buf_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;

    /* Allocate memory for the response buffer with the size of FBE_SCSI_INQUIRY_DATA_SIZE. */
    response_buf_p = (fbe_u8_t *)malloc(FBE_SCSI_INQUIRY_DATA_SIZE * sizeof(fbe_u8_t));

    eses_page_op.cmd_buf.raw_scsi.raw_inquiry.page_code = raw_inquiry_vpd_pg;
    eses_page_op.cmd_buf.raw_scsi.raw_inquiry.evpd = evpd;
    eses_page_op.response_buf_p = response_buf_p;
    eses_page_op.response_buf_size = FBE_SCSI_INQUIRY_DATA_SIZE;
    eses_page_op.required_response_buf_size = 0;

    status = fbe_api_enclosure_send_raw_inquiry_pg(object_id, &eses_page_op, enclosure_status);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to send enquiry page to object id %d, status: 0x%x.\n", object_id, status);  
        return status;
    }

    // we are running with the risk of printing out staff not belong to us
    // when eses page return size is close to what we allocated.
    while (eses_page_op.required_response_buf_size >= print_offset)
    {
        fbe_cli_printf("%03x: ", print_offset);
        fbe_cli_print_bytes(&response_buf_p[print_offset], 16);
        print_offset += 16; //next 16 bytes
    }

    return status;

}// End of function fbe_cli_encl_send_raw_scsi_cmd


/*!***************************************************************
 * @fn fbe_cli_scsi_list_all_vpd_pages(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  Function to list all the SCSI VPD Pages and their status.
 *
 * @param  object_id - The object id of the enclosure.
 *
 * @return  fbe_status_t
 *
 * HISTORY:
 *  01-Apr-2009 AS - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cli_scsi_list_all_vpd_pages(fbe_object_id_t object_id)
{
#if 0 /* Activate the code only after complete implementation of VPD pages */
    fbe_u8_t number_of_vpd_pages = 0;
    fbe_u8_t *response_buf_p = NULL;
    fbe_u32_t response_buf_size = 0;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_enclosure_mgmt_raw_inquiry_cmd_t *raw_inquiry_cmd_p = NULL;

    /* Step 1: Get the number of SCSI VPD Pages. */
    /* TODO: Create function definition for fbe_api_enclosure_build_vpd_pages_cmd */
    fbe_api_enclosure_build_vpd_pages_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM,
                                         &number_of_vpd_pages, 1);

    /* TODO: Create function definition for fbe_api_enclosure_get_vpd_pages_info */
   status = fbe_api_enclosure_get_vpd_pages_info(object_id, &eses_page_op);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get the number of SCSI VPD pages, status:0x%x.\n", status);
        return status;
    }
    
    fbe_cli_printf("   \n=== There are totally %d SCSI VPD Pages ====\n", number_of_vpd_pages);

    if(number_of_vpd_pages == 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;   
    }

    /* Step 2: Get the status of the VPD Pages.
     * Allocate memory for the response buffer which will contain the VPD Page info elements retrieved 
     * based on the specified number of VPD Pages. 
     * Adding 1 bytes because the first byte is the number of the VPD Pages.
     */
    response_buf_size = number_of_vpd_pages * FBE_SCSI_INQUIRY_DATA_SIZE + 1;
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size * sizeof(fbe_u8_t));
    
    /* TODO: Create function definition for fbe_api_enclosure_build_vpd_pages_cmd */
    fbe_api_enclosure_build_vpd_pages_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS,
        response_buf_p, response_buf_size);
    
    /* TODO: Create function definition for fbe_api_enclosure_get_vpd_pages_info */
    status = fbe_api_enclosure_get_vpd_pages_info(object_id, &eses_page_op);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get SCSI VPD Page status, status:0x%x.\n", status);
    }
    else
    {
        /* TODO: Create function definition for fbe_cli_encl_print_vpd_pages_status */
        status = fbe_cli_encl_print_vpd_pages_status(eses_page_op.response_buf_p, eses_page_op.response_buf_size);  
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to print SCSI VPD Page status, status:0x%x.\n", status);
        }
    }
    

    free(response_buf_p);
#endif
    return FBE_STATUS_OK;
}// End of function fbe_cli_scsi_list_all_vpd_pages

/*!*************************************************************************
* @fn fbe_cli_cmd_enclosure_private_data
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function will 
*      1) get enclosure info.
*      2) output the enclosure private data.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES: if -b <bus no.> -e<encl no.>  are not define then it will output the private data for all encl's
*
*  HISTORY:
*    12-Apr-2009: Dipak Patel- created.
*
**************************************************************************/
void fbe_cli_cmd_enclosure_private_data(int argc, char** argv)
{
    fbe_u32_t object_handle_p= FBE_OBJECT_ID_INVALID,port , encl_pos,encl_pos_tmp =0;
    fbe_status_t status;
    fbe_base_object_mgmt_get_enclosure_prv_info_t enclosurePrvInfo;
    fbe_class_id_t  class_id;
    fbe_bool_t  encl_filter = FALSE,port_filter = FALSE;

    /*
    There should be a minimum of arguments.
    */
    if ((argc > 6)||(argc < 0))
    {
        fbe_cli_printf(" invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", ENCLPRVT_USAGE);
        return;
    }
    /*
    Parse the command line.
    */
    while(argc > 0)
    {
        /*
        Check the command type
        */
        if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("%s", ENCLPRVT_USAGE);
                return;
            }
        }
        else if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_private_data: Please provide bus #.\n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
            port_filter = TRUE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nencl_private_data: Please provide enclosure #.\n");
                return;
            }
            encl_pos_tmp = (fbe_u32_t)strtoul(*argv, 0, 0);
            encl_filter = TRUE;
        }
        argc--;
        argv++;
    }
    /*
    Get the object ID for the specified enclosure
    */
    for (port = 0;port<FBE_PHYSICAL_BUS_COUNT;port++)
    {
                status = fbe_api_get_port_object_id_by_location(port, &object_handle_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\failed to get port object handle status: %d\n",
                status);
            return;
        }
        if(object_handle_p != FBE_OBJECT_ID_INVALID)
        {
            if(encl_filter)
            {
                encl_pos = encl_pos_tmp;
            }
            else
            {
                encl_pos = 0;
            }
            for(encl_pos = 0;encl_pos<FBE_ENCLOSURES_PER_BUS;encl_pos++)
            {
                status = fbe_api_get_enclosure_object_id_by_location(port,encl_pos, &object_handle_p);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\failed to get enclosure object handle status: %d\n",
                        status);
                    return;
                }
                if(object_handle_p == FBE_OBJECT_ID_INVALID)
                {
                    break;
                }
                else
                {
                    //Get the enclosure component data blob via the API.
                    status = fbe_api_enclosure_get_prvt_info(object_handle_p, &enclosurePrvInfo);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nfailed to get enclosure private data:%d\n", 
                            status);
                        return;
                    }

                    status = fbe_api_get_object_class_id(object_handle_p, &class_id, FBE_PACKAGE_ID_PHYSICAL);

                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nfailed to get enclosure object class id:%d\n", 
                            status);
                        return;
                    }
                    if (IS_VALID_LEAF_ENCL_CLASS(class_id))
                    {
                        switch(class_id)
                        {
                        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
                            fbe_sas_viper_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;

                        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
                            fbe_sas_magnum_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);
                            break;
                        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
                            fbe_sas_citadel_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
                            fbe_sas_bunker_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE :
                            fbe_sas_derringer_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);         
                            break;
                        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE :
                            fbe_sas_tabasco_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);         
                            break;
                        case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE :
                            fbe_sas_voyager_icm_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);
                            break;
                        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE :
                            fbe_sas_viking_iosxp_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);
                            break;
                       case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE :
                            fbe_sas_naga_iosxp_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);
                            break;
                        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
                            fbe_sas_fallback_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
                            fbe_sas_boxwood_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
                            fbe_sas_knot_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
                            fbe_sas_pinecone_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;				
                        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
                            fbe_sas_ancho_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE :
                             fbe_sas_cayenne_iosxp_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);
                             break;
                        case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
                            fbe_sas_steeljaw_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
                            fbe_sas_ramhorn_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
                            fbe_sas_calypso_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
                            fbe_sas_rhea_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
                            fbe_sas_miranda_enclosure_print_prvt_data((void *)&enclosurePrvInfo, fbe_cli_trace_func, NULL);       
                            break;
                        }
                    }
                    else
                    {    
                        fbe_cli_printf("\nUnsupported class. ClassId :%d, Port:%d, Enclosure:%d\n", 
                            class_id, port, encl_pos);

                    }
                }
            }
        }
    }
    return;
}
/*!***************************************************************
 * @fn fbe_cli_print_stats_data
 ****************************************************************
 * @brief
 *  Print the statistics data for the specified element type.
 *
 * @param  *statsp - byte pointer to the union of statistics data(in)
 * @param  *data_size - pointer to the variable where size of data 
 *                      actually processed is reserved (out)
 * @param  header - indicate whether the header need to be printed
 *
 * @return  void
 *
 * HISTORY:
 *  23-Sep-2011 GB - ported from fbe_cli to here.
 ****************************************************************/
void fbe_cli_print_stats_data(fbe_enclosure_mgmt_statistics_data_t *statsp,
                              fbe_u32_t *data_size,
                              fbe_bool_t header)
{
    fbe_eses_page_statistics_u  *datap;
    fbe_bool_t                  update_data_size = TRUE;

    // set datap to point to the stats_data, it will be cast
    // to an eses struct specific to the element type
    datap = (fbe_eses_page_statistics_u*) &statsp->stats_data;

    *data_size = 0;
    switch (statsp->stats_id_info.element_type)
    {
    case SES_ELEM_TYPE_PS :
        if (header)
        {
            fbe_cli_printf("\n\t\tPS STATISTICS \n");
            fbe_cli_printf("ElemIndex|DCOverV|DCUnderV|Fail|OverTemp|ACFail|DCFail\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_cli_printf(" %-3d      %-7d %-8d %-4d %-8d %-6d %d\n",
                           statsp->stats_id_info.slot_or_id,
                           ((fbe_eses_ps_stats_t *)datap)->dc_over,
                           ((fbe_eses_ps_stats_t *)datap)->dc_under,
                           ((fbe_eses_ps_stats_t *)datap)->fail,
                           ((fbe_eses_ps_stats_t *)datap)->ot_fail,
                           ((fbe_eses_ps_stats_t *)datap)->ac_fail,
                           ((fbe_eses_ps_stats_t *)datap)->dc_fail);
        }
        break;

    case SES_ELEM_TYPE_COOLING :
        if (header)
        {
            fbe_cli_printf("\n\t\tCOOLING STATISTICS\n");
            fbe_cli_printf("ElemIndex | Fail\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_cli_printf(" %-3d          %d \n",
                           statsp->stats_id_info.slot_or_id,
                           ((fbe_eses_cooling_stats_t*)datap)->fail);
        }
        break;

    case SES_ELEM_TYPE_TEMP_SENSOR :
        if (header)
        {
            fbe_cli_printf("\n\t\tTEMP SENSOR STATISTICS\n");
            fbe_cli_printf("ElemIndex|OverTempFail|OverTempWarn\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_cli_printf(" %-3d      %-10d   %d\n",
                           statsp->stats_id_info.slot_or_id,
                           ((fbe_eses_temperature_stats_t*)datap)->ot_fail,
                           ((fbe_eses_temperature_stats_t*)datap)->ot_warn);
        }
        break;

    case SES_ELEM_TYPE_EXP_PHY :
        if (header)
        {
            fbe_cli_printf("\n\t\tEXP PHY STATISTICS\n");
            fbe_cli_printf("PhyId|Conn|Slot|InvldDW|Dsprty|DWSync|RstFail|CdErr|PhyChg|CRCPMon|InConnCRC\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            if (statsp->stats_id_info.drv_or_conn == SES_ELEM_TYPE_SAS_CONN)
            {
                fbe_cli_printf(" %-5d %-4d --   %-6d  %-5d  %-5d  %-6d  %-4d  %-5d  %-5d   %d\n",
                               statsp->stats_id_info.slot_or_id,
                               statsp->stats_id_info.drv_or_conn_num,
                               //WARNING: here we use swap32 to do byte order change since eses enclosure 
                               //does not do the endian conversion for eses pages passes from expander. 
                               //If there is any change in the eses enclosure for the eses page endian conversion,
                               //then the following code may need change.  
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->invalid_dword),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->disparity_error),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->loss_dword_sync),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->phy_reset_fail),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->code_violation),
                               ((fbe_eses_exp_phy_stats_t *)datap)->phy_change,
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->crc_pmon_accum),
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->in_connect_crc));
            }
            else if (statsp->stats_id_info.drv_or_conn == SES_ELEM_TYPE_ARRAY_DEV_SLOT)
            {
                fbe_cli_printf(" %-5d --   %-3d  %-6d  %-5d  %-5d  %-6d  %-4d  %-5d  %-5d   %d\n",
                               statsp->stats_id_info.slot_or_id,
                               statsp->stats_id_info.drv_or_conn_num,
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->invalid_dword),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->disparity_error),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->loss_dword_sync),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->phy_reset_fail),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->code_violation),
                               ((fbe_eses_exp_phy_stats_t *)datap)->phy_change,
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->crc_pmon_accum),
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->in_connect_crc));
            }
            else
            {
                fbe_cli_printf(" %-5d --   --   %-6d  %-5d  %-5d  %-6d  %-4d  %-5d  %-5d   %d\n",
                               statsp->stats_id_info.slot_or_id,
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->invalid_dword),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->disparity_error),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->loss_dword_sync),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->phy_reset_fail),
                               swap32(((fbe_eses_exp_phy_stats_t *)datap)->code_violation),
                               ((fbe_eses_exp_phy_stats_t *)datap)->phy_change,
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->crc_pmon_accum),
                               swap16(((fbe_eses_exp_phy_stats_t *)datap)->in_connect_crc));
            }
        }
        break;

    case SES_ELEM_TYPE_ARRAY_DEV_SLOT :
        if (header)
        {
            fbe_cli_printf("\n\t\tARRAY DEVICE SLOT STATISTICS\n");
            fbe_cli_printf(" Slot | Insert Count | Power Down\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_cli_printf(" %-4d   %-5d           %-5d \n",
                           statsp->stats_id_info.slot_or_id,
                           ((fbe_eses_device_slot_stats_t *)datap)->insert_count,
                           ((fbe_eses_device_slot_stats_t *)datap)->power_down_count);
        }
        break;

    case SES_ELEM_TYPE_SAS_EXP :
        if (header)
        {
            fbe_cli_printf("\n\t\tSAS EXP STATISTICS\n");
            fbe_cli_printf("ElemIndex | Exp Change\n");
        }
        if (statsp->stats_id_info.slot_or_id < FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_cli_printf(" %-3d        %d \n",
                           statsp->stats_id_info.slot_or_id,
                           swap16(((fbe_eses_sas_exp_stats_t *)datap)->exp_change));
        }
        break;

    default :
        // trace
        fbe_cli_printf("This element type (0x%x) is not supported for statistics gathering!\n", statsp->stats_id_info.element_type);
        *data_size = 0;
        update_data_size = FALSE;
        break;
    } // end switch

    if (update_data_size)
    {
        *data_size = (sizeof(ses_common_statistics_field_t) + datap->common.stats_len) + sizeof(fbe_enclosure_mgmt_stats_resp_id_info_t);
    }
} // fbe_cli_print_stats_data

/*!***************************************************************
 * @fn fbe_cli_print_enclosure_statistics
 ****************************************************************
 * @brief
 *  Parse the statistics page and display the data.
 *
 * @param  element_type - the element type to display (input)
 * @param  first_slot - first slot number to display (input)
 * @param  last_slot - last slot number to display (input)
 * @param  response_data_size - the response data length
 *                              from DAE (input)
 * @param  *sourcep - the response eses statistics page 
 *                    data pointer (input)
 * @param  response_buf_size -  the buffer size we pre-allocated 
 *                              for the response data.
 *
 * @return  void
 *
 * HISTORY:
 *  23-Sep-2011 GB - Ported form fbe_cli to here.
 ****************************************************************/
void fbe_cli_display_enclosure_statistics(fbe_enclosure_element_type_t element_type,
                                          fbe_enclosure_slot_number_t first_slot,
                                          fbe_enclosure_slot_number_t last_slot,
                                          fbe_u32_t response_data_size,
                                          fbe_u8_t *sourcep,    // points to the buffer returned from target
                                          fbe_u32_t response_buf_size) 
{
    fbe_u32_t               bytes_processed;
    fbe_u32_t               data_size = 0;
    fbe_u8_t                *checkp = NULL;
    fbe_u8_t                *datap = NULL;
    fbe_bool_t              header = TRUE;
    fbe_enclosure_element_type_t prev_element_type = FBE_ELEMENT_TYPE_INVALID;

    // point to the first statistics_data_t item
    datap = (fbe_u8_t *)&(((fbe_enclosure_mgmt_statistscs_response_page_t *)sourcep)->data);

    checkp = sourcep + response_data_size;
    bytes_processed = 0;
    if (response_data_size > response_buf_size)
    {
        fbe_cli_printf("EnclStats:Response Data Truncated-Allocated:0x%x, Need:0x%x\n",response_buf_size, response_data_size);
    }
    while ((bytes_processed < response_data_size) && (sourcep < checkp))
    {
        // display the data
        if (prev_element_type != ((fbe_enclosure_mgmt_statistics_data_t *)datap)->stats_id_info.element_type)
        {
            header = TRUE;
        }

        fbe_cli_print_stats_data( (fbe_enclosure_mgmt_statistics_data_t *)datap,
                                   &data_size,
                                   header);

        header = FALSE;
        prev_element_type = ((fbe_enclosure_mgmt_statistics_data_t *)datap)->stats_id_info.element_type;

        // next element item
        bytes_processed += data_size;
        datap += data_size;
        
        if (data_size == 0)
        {
            fbe_cli_printf("No data size associated with this element, stats halted!\n");
            break;
        }
    }
    return;
} // fbe_cli_print_enclosure_statistics

/*!*************************************************************************
 * @fn fbe_cli_get_statistics
 **************************************************************************
 *  @brief
 *  This function is the handler for using the "statistics" command. This
 *  parses the input and calls the function to execute hte operation.
 *
*  @param object_id - The enclosure object Id.
*  @param element_type - The element Type.
*  @param start_slot - The start slot number.
*  @param end_slot - The end slot number
 *
 *  @return   fbe_status_t
 *
 *  HISTORY:
 *    29-Sep-2011: GB - Ported from fbe_cli to here
 **************************************************************************/
static fbe_status_t fbe_cli_get_statistics(fbe_object_id_t object_id,
                                           fbe_enclosure_element_type_t element_type,
                                           fbe_enclosure_slot_number_t start_slot,
                                           fbe_enclosure_slot_number_t end_slot)
{
    fbe_enclosure_mgmt_ctrl_op_t    control_op;
    fbe_enclosure_status_t          enclosure_status;
    fbe_u8_t                        *response_bufp = NULL;
    fbe_u32_t                       response_buf_size = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       retry_count;

    if(start_slot > end_slot)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get how many memory we need 
    response_buf_size = FBE_ESES_PAGE_MAX_SIZE;
    // Allocate the memory
    response_bufp = (fbe_u8_t *)malloc(response_buf_size);
    if (response_bufp == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // Fill in the struct fbe_enclosure_mgmt_ctrl_op_t with slot_num_start, slot_num_end,
    // response_buf_p, response_buf_size.
    status = fbe_api_enclosure_build_statistics_cmd(&control_op,
                                                    element_type,
                                                    start_slot,
                                                    end_slot,
                                                    response_bufp,
                                                    response_buf_size);

    retry_count = 0;
    // Send the operation to the physical package
    while(1)
    {

        status = fbe_api_enclosure_get_statistics(object_id, &control_op, &enclosure_status);
        retry_count++;

        if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get statistics, encl busy, will retry, count: %d.\n", retry_count);
            fbe_api_sleep(200);
        }
        else
        {
            break;
        }
    }

    if(status != FBE_STATUS_OK)
    {
        if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get statistics, encl busy, retry count exhausted.\n");
        }
        else
        {
            fbe_cli_printf("\nFailed to get statistics, status:0x%x.\n", status);
        }
    }
    else
    {
        fbe_cli_display_enclosure_statistics(element_type,
                                             start_slot,
                                             end_slot,
                                             control_op.required_response_buf_size,
                                             control_op.response_buf_p,
                                             response_buf_size);
    }

    if (response_bufp!= NULL)
    {
        free(response_bufp);
    }

    return status;
} // End fbe_cli_get_statistics

/*!*************************************************************************
 * @fn fbe_cli_stringToElementType
 *           (char **elementString)
 **************************************************************************
 *
 *  @brief
 *  Given an element type string, from the parsed input, return the
 *  appropriate element type.
 *
*  @param    elementString - The element string.
 *
 *  @return   fbe_enclosure_element_type_t
 *
 *  HISTORY:
 *    29-Sep-2011: GB - Ported from fbe_cli to here
 *
 **************************************************************************/
static fbe_enclosure_element_type_t fbe_cli_stringToElementType(char **elementString)
{
    ses_elem_type_enum elementType = FBE_ELEMENT_TYPE_INVALID;

    if(strcmp(*elementString, "ps") == 0)
    {
        elementType = FBE_ELEMENT_TYPE_PS;
    }
    else if(strcmp(*elementString, "cool") == 0)
    {
        elementType = FBE_ELEMENT_TYPE_COOLING;
    }
    else if(strcmp(*elementString, "devslot") == 0)
    {
        elementType = FBE_ELEMENT_TYPE_DEV_SLOT;
    }
    else if(strcmp(*elementString, "temp") == 0)
    {
        elementType = SES_ELEM_TYPE_TEMP_SENSOR;
    }
    else if(strcmp(*elementString, "exp") == 0)
    {
        elementType = FBE_ELEMENT_TYPE_SAS_EXP;
    }
    else if(strcmp(*elementString, "phy") == 0)
    {
        elementType = FBE_ELEMENT_TYPE_EXP_PHY;
    }

    return elementType;

}   // end of fbe_cli_stringToElementType


/*!*************************************************************************
 * @fn fbe_cli_cmd_display_statistics
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *  This function is the handler for using the "statistics" command. This
 *  parses the input and calls the function to execute hte operation.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  HISTORY:
 *    29-Sep-2011: GB - Ported from fbe_cli to here
 *
 **************************************************************************/
void fbe_cli_cmd_display_statistics(int argc , char ** argv)
{
    fbe_u32_t                       object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_enclosure_element_type_t    element_type = FBE_ELEMENT_TYPE_ALL;
    fbe_enclosure_slot_number_t     first_slot = 0;
    fbe_enclosure_slot_number_t     last_slot = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_u32_t    componentId = 0;
    fbe_bool_t   componentIdflag = FALSE;
    fbe_u8_t   *tag = NULL;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;
    fbe_bool_t   printAllBus = TRUE;
    fbe_bool_t   printAllEncl = TRUE;
    fbe_u8_t   portIndex = 0;
    fbe_u8_t   enclIndex = 0;
    fbe_s8_t   compIndex = -1;

    // Parse the command line
    while(argc > 0)
    {
        if(strcmp(*argv, "-h") == 0)
        {
            fbe_cli_printf("%s", ENCLSTATISTICS_USAGE);
            return;
        }
        else if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
            printAllBus = FALSE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
            printAllEncl = FALSE;
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            componentId = (fbe_u32_t)strtoul(*argv, 0, 0);
            componentIdflag = TRUE;
        }
        // process Element Type & index
        else if(strcmp(*argv, "-t") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("encl_data, too few arguments \n");
                return;
            }
            element_type = fbe_cli_stringToElementType(argv);
            if (element_type == FBE_ELEMENT_TYPE_INVALID)
            {
                fbe_cli_error("ElementStatistics, unknown element type:%s\n",*argv);
                return;
            }

            // check for slot range (default to all if none specified)
            argc--;
            argv++;
            if(argc != 0)
            {
                if ((element_type == FBE_ELEMENT_TYPE_EXP_PHY) ||
                    (element_type == FBE_ELEMENT_TYPE_DEV_SLOT))
                {
                    if ((atoi(*argv) >= 0) && (atoi(*argv) <= 255))
                    {
                        first_slot = (fbe_u32_t)strtoul(*argv, 0, 0);
                        argc--;
                        argv++;
                        if (argc == 0)
                        {
                            fbe_cli_printf("Last slot defaulting to max value!\n");
                        }
                        else
                        {
                            if ((atoi(*argv) >= 0) && (atoi(*argv) <= 255))
                            {
                                last_slot = (fbe_u32_t)strtoul(*argv, 0, 0);
                            }
                            else
                            {
                                fbe_cli_printf("Last slot defaulting to max value(255)!\n");
                            }
                        }
                        if (first_slot > last_slot)
                        {
                            fbe_cli_printf("First Slot > Last Slot!\n");
                            return;
                        }
                    }
                    else
                    {
                        // bad first slot value
                        fbe_cli_printf("First Slot Value Out of Range:%s\n",*argv);
                        return;
                    }
                }
                else
                {
                    fbe_cli_error("Slot election only available for phy and devslot\n");
                    return;
                }
            }

        }
        else
        {
            fbe_cli_error("ElementStatistics, unknown arg:%s\n",*argv);
            return;
        }
        argc--;
        argv++;
    }   // end of while

    // loop all the ports to print the enclosures we want
    for (portIndex = 0; portIndex < FBE_PHYSICAL_BUS_COUNT; portIndex++)
    {
        // we either print enclosures on all the ports or use the specific 
        // port number given by input parameter
        if (printAllBus == TRUE)
        {
            port = portIndex;
        }
        else if (port != portIndex)
        {
            continue;
        }

        // loop all the enclosures on a port to print the one we want
        for (enclIndex = 0; enclIndex < FBE_ENCLOSURES_PER_BUS; enclIndex++)
        {
            // we either print all the enclosures or print the specific 
            // enclosure number given by input parameter
            if (printAllEncl == TRUE)
            {
                encl_pos = enclIndex;
            }
            else if (encl_pos != enclIndex)
            {
                continue;
            }

            status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl_pos, &enclosure_object_ids);

            if(status != FBE_STATUS_OK)
            {
                if (printAllBus != TRUE && printAllEncl != TRUE)
                {
                    fbe_cli_printf("\nElementStatistics failed to get enclosure object handle, status: %d\n", status);
                    return;
                }
                else
                {
                    continue;
                }
            }

            // loop all the components in an enclosure
            // we use -1 to represent the enclosure itself, and in Voyager it represents ICM
            for (compIndex = -1; compIndex < FBE_API_MAX_ENCL_COMPONENTS; compIndex++)
            {
                if (componentIdflag == FALSE)
                {
                    componentId = compIndex;
                }
                else if (componentId != compIndex)
                {
                    continue;
                }

                /* Get the component object ID for the specified EE's */
                if(componentId != -1)
                {
                    /*Get the object ID for specified component ID */
                    /*If we are targeting EE's*/ 
                    object_handle_p = enclosure_object_ids.comp_object_id[componentId];
                    tag ="componentID";    
                }
                else
                {
                    /* Get the object ID for the specified enclosure */
                    object_handle_p = enclosure_object_ids.enclosure_object_id;
                    tag = "Enclosure";
                }

                if(object_handle_p == FBE_OBJECT_ID_INVALID)
                {
                    // if you give the specific enclosure/component ID, but we did not find it
                    // then we will give you a error
                    if (printAllBus != TRUE && printAllEncl != TRUE && componentIdflag == TRUE)
                    {
                        fbe_cli_printf("\nElementStatistics failed to get %s object handle\n",tag);
                        return;
                    }
                    else
                    {
                        continue;
                    }
                }

                if (componentId != -1)
                {
                    fbe_cli_printf("\nEnclosure Statistics Data for Port %d, Enclosure %d, Component %d:\n", portIndex, enclIndex, componentId);
                }
                else
                {
                    fbe_cli_printf("\nEnclosure Statistics Data for Port %d, Enclosure %d:\n", portIndex, enclIndex);
                }

                status = fbe_cli_get_statistics(object_handle_p,
                                                element_type,
                                                first_slot,
                                                last_slot);

                if (status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nDone\n");
                }
                else
                {
                    fbe_cli_printf("\neElementStatistics: Failed get Statistics bus:%d, encl:%d, compId: %d, "
                                   "slots %d-%d, status: 0x%x.\n",
                                   port, encl_pos, componentId,  first_slot, last_slot, status);
                }
                // if this enclosure does not has component at all, we exit this loop
                if (enclosure_object_ids.component_count == 0)
                {
                    break;
                }
            } // for all component
        } // for all encl
    } // for all bus

    return;

} // fbe_cli_cmd_display_statistics

//End of file fbe_cli_src_enclosure_cmds.c

/*!*************************************************************************
* @fn fbe_cli_fill_enclosure_drive_info()
*           
**************************************************************************
*
*  @brief
*      This function will fill the drive led related info depending upon index number and fill
*       according to the slot number.
*
*  @param baseCompBlockPtr - The base component block pointer. 
*  @param driveStat - Pointer to the slot_flags_t.
*  @param -component_count - The drive component count.
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    13-May-2010: Sujay Ranjan
*
**************************************************************************/
fbe_edal_status_t  fbe_cli_fill_enclosure_drive_info(fbe_edal_block_handle_t baseCompBlockPtr,slot_flags_t *driveStat,fbe_u32_t *component_count)
{
    fbe_u8_t                    index;
    fbe_edal_status_t        status;
    fbe_u8_t   slot;

    status = fbe_edal_getSpecificComponentCount(baseCompBlockPtr,
        FBE_ENCL_DRIVE_SLOT,
        component_count);


    if(FBE_EDAL_STATUS_OK == status)
    { 

        for(index = 0;index < *component_count; index++)
        {
            // get the slot number
            status = fbe_edal_getU8((void *)baseCompBlockPtr,
                FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                FBE_ENCL_DRIVE_SLOT,        // Component type
                index,                 // drive component index
                &slot); 


            status = fbe_edal_getDriveBool((void *)baseCompBlockPtr,
                FBE_ENCL_COMP_INSERTED,
                slot,
                &(driveStat[slot].driveInserted));

            status = fbe_edal_getDriveBool((void *)baseCompBlockPtr,
                FBE_ENCL_COMP_FAULTED,
                slot,
                &(driveStat[slot].driveFaulted));

            status = fbe_edal_getDriveBool((void *)baseCompBlockPtr,
                FBE_ENCL_COMP_POWERED_OFF,
                slot,
                &(driveStat[slot].drivePoweredOff));

            status = fbe_edal_getDriveBool((void *)baseCompBlockPtr,
                FBE_ENCL_DRIVE_BYPASSED,
                slot,
                &(driveStat[slot].driveBypassed));

            status = fbe_edal_getDriveBool((void *)baseCompBlockPtr,
                FBE_ENCL_DRIVE_LOGGED_IN,
                slot,
                &(driveStat[slot].driveLoggedIn));

        }

    }
    return status;
}

/*!*************************************************************************
* @fn fbe_cli_print_drive_info()
*           
**************************************************************************
*
*  @brief
*  This function will print the drive info for enclosure.
*
*  @param driveStat -Pointer to the slot_flags_t.  
*  @param max_slot -The max number of slots.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  13-May-2010: Sujay Ranjan
*  2-Dec-2010: Gerry Fredette - Updated so that format is consistent and indented properly.
*
**************************************************************************/
void fbe_cli_print_drive_info(slot_flags_t *driveStat,fbe_u8_t max_slot)
{
    fbe_u32_t   break_line = 0;
    fbe_u8_t     index;
    fbe_u32_t   count;


    if(max_slot < FBE_API_MAX_DRIVE_EE)
     {
        // Example output. But note that the first line is printed by the caller.
        // SAS_BUNKER_ENCLOSURE  (Bus:0 Enclosure:0)  LifeCycleState:Ready BundleRev: 109
        //  Slot    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14
        //  State RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY

        //  Slot    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24
        //  State RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY

        fbe_cli_printf("\n  Slot ");
        for (index = 0; index < max_slot; index++)
        {
            fbe_cli_printf("%3d",index);
        }
        fbe_cli_printf("\n  State");
        for (index = 0; index < max_slot; index++)
        {
            fbe_cli_printf("%s",fbe_eses_debug_decode_slot_state(&driveStat[index]));
        }
        fbe_cli_printf("\n");

    }
    else
    {
        if(max_slot < FBE_API_MAX_DRIVE_VIKING)
        {
            // Example output. But note that the first line is printed by the caller.
            // SAS_VOYAGER_ICM_ENCLOSURE  (Bus:1 Enclosure:0)  LifeCycleState:Ready BundleRev: 005
            //  Slot    0   1   2   3   4   5   6   7   8   9  10  11
            //   (a)  RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY
            //   (b)  RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY
            //   (c)  RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY
            //   (d)  RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY
            //   (e)  RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY RDY
            break_line = FBE_API_VOYAGER_BANK_WIDTH;
        }
        else
        {
            break_line = FBE_API_VIKING_BANK_WIDTH;
        }

        fbe_cli_printf("\n  Slot");

        for(index = 0; index < break_line;index ++)
        {
            fbe_cli_printf("%4d",index );
        }

        for (index = 0, count = 'a'; index < max_slot; index++)
        {
            if(!(index %break_line))
            {
                fbe_cli_printf("\n   (%c) ", count++); 
            }
            fbe_cli_printf("%s ",fbe_eses_debug_decode_slot_state(&driveStat[index]));
        }
        fbe_cli_printf("\n");
    }
}

/*!*************************************************************************
* @fn fbe_cli_print_drive_Led_info()
*           
**************************************************************************
*
*  @brief
*  This function will print the drive LED info for enclosure.
*
*  @param driveStat -Pointer to the slot_flags_t.  
*  @param max_slot -The max number of slots.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  11-Oct-2012: Randall Porteus
*
**************************************************************************/
void fbe_cli_print_drive_Led_info(fbe_led_status_t *pEnclDriveFaultLeds,fbe_u8_t max_slot)
{
    fbe_u32_t   break_line = 0;
    fbe_u8_t     index;
    fbe_u32_t   count;


    if(max_slot < FBE_API_MAX_DRIVE_EE)
     {
        // Example output. But note that the first line is printed by the caller.
        // SAS_BUNKER_ENCLOSURE  (Bus:0 Enclosure:0)  LifeCycleState:Ready BundleRev: 109
        //  DR#    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14
        //        OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF

        //  DR#    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24
        //        OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF

        fbe_cli_printf("\n  DRV# ");
        for (index = 0; index < max_slot; index++)
        {
            fbe_cli_printf("%3d ",index);
        }
        fbe_cli_printf("\n       ");
        for (index = 0; index < max_slot; index++)
        {
            fbe_cli_printf("%s",fbe_eses_debug_decode_slot_Led_state(&pEnclDriveFaultLeds[index]));   
        }
        fbe_cli_printf("\n");

    }
    else
    {
        if(max_slot < FBE_API_MAX_DRIVE_VIKING){
            // Example output. But note that the first line is printed by the caller.
            //  DR#    0   1   2   3   4   5   6   7   8   9  10  11
            //   (a)  OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF
            //   (b)  OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF
            //   (c)  OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF
            //   (d)  OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF
            //   (e)  OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF OFF
       
            break_line = FBE_API_VOYAGER_BANK_WIDTH;
        }
        else
        {
            break_line = FBE_API_VIKING_BANK_WIDTH;
        }

        fbe_cli_printf("\n  DRV# ");
        for(index = 0; index < break_line;index ++)
        {
            fbe_cli_printf("%3d ",index );
        }

        for (index = 0, count = 'a'; index < max_slot; index++)
        {
            if(!(index %break_line))
            {
                fbe_cli_printf("\n   (%c) ", count++); 
            }
            fbe_cli_printf("%s",fbe_eses_debug_decode_slot_Led_state(&pEnclDriveFaultLeds[index]));
        }
        fbe_cli_printf("\n");
    }
}

/*!*************************************************************************
* @fn fbe_cli_cmd_enclosure_expStringIn
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function will send the specified StringIn command to the
*      specified Expander.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
void fbe_cli_cmd_enclosure_expStringIn(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t componentId   = 0;
    fbe_bool_t componentIdflag = FALSE;
    fbe_u8_t   *tag = NULL;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids = {0};
    fbe_u32_t response_buf_size = 0;
    fbe_u32_t currentTime = 0;
    fbe_u8_t recievedData = FALSE;

    /*
    * Always need port & enclosure position plus possibly one more argument
    */
    if (argc < 2)
    {
        fbe_cli_printf("enclosure expStringIn invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", EXPSTRINGIN_USAGE);
        return;
    }

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    /*
    * Parse the command line.
    */
    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure expStringIn, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure expStringIn, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nenclosure expStringIn, too few arguments .\n");
                return;
            }
            componentId = (fbe_u32_t)strtoul(*argv, 0, 0);
            componentIdflag = TRUE;
        }
        else
        {
            fbe_cli_printf("enclosure expStringIn invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;
    } // end of while(argc > 0)

    status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl_pos, &enclosure_object_ids);
    /* Get the component object ID for the specified EE's */
    if(componentIdflag)
    {
        /*Get the object ID for specified component ID */
        /*If we are targeting EE's*/
        object_handle_p = enclosure_object_ids.comp_object_id[componentId];
        tag ="componentID";
    }
    else
    {
        /* Get the object ID for the specified enclosure */
        object_handle_p = enclosure_object_ids.enclosure_object_id;
        tag = "Enclosure";
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nenclosure expStringIn failed to get %s object handle status: %d\n",
            tag, status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nenclosure expStringIn failed to get %s object handle\n",tag);
        return;
    }

    while ((currentTime < FBE_STRING_IN_CMD_WAIT_TIME) && (recievedData == FALSE)) 
    {
        fbe_api_sleep(FBE_STRING_IN_CMD_INTERVAL_TIME);

        do 
        {
            status = fbe_cli_encl_send_string_in_cmd(object_handle_p, &enclosure_status, &response_buf_size, FBE_TRUE);
            if(status != FBE_STATUS_OK)
            {
                if (componentIdflag)
                {
                    fbe_cli_printf("\nexpStringIn failed to send control to bus %d, encl %d, compId %d, status: %d\n",
                                      port, encl_pos, componentId, status);
                }
                else
                {
                    fbe_cli_printf("\nexpStringIn failed to send control to bus %d, encl %d, status: %d\n", port, encl_pos, status);
                }
                return;
            }

            if ((recievedData == FALSE) && (response_buf_size >  FBE_ESES_PAGE_SIZE))
            {
                  recievedData = TRUE;
            }
        }while ((recievedData == TRUE) && ((response_buf_size >  FBE_ESES_PAGE_SIZE)));

        currentTime +=  FBE_STRING_IN_CMD_INTERVAL_TIME;
    }
    return;

}// end of fbe_cli_cmd_enclosure_expStringIn

/*!***************************************************************
* @fn fbe_cli_encl_send_string_in_cmd(fbe_object_id_t object_id,
*                                   fbe_enclosure_status_t *enclosure_status,
*                                   fbe_u32_t *response_buf_size,
*                                   fbe_bool_t printFlag)
****************************************************************
* @brief
*   The function allocates 1K response buffer, and send
* string in page to the object.
*
* @param  object_id - The object id of the enclosure.
* @param  enclosure_status - enclosure status.
*
* @return  fbe_status_t
*
* HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
****************************************************************/
static fbe_status_t
fbe_cli_encl_send_string_in_cmd(fbe_object_id_t object_id,
                                fbe_enclosure_status_t *enclosure_status,
                                fbe_u32_t *response_buf_size,
                                fbe_bool_t printFlag)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u8_t * response_buf_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t  print_offset = 0;
    fbe_u32_t  size = 0;


    /* Allocate memory for the response buffer with the size of FBE_ESES_PAGE_MAX_SIZE. */
    response_buf_p = (fbe_u8_t *)malloc(FBE_ESES_PAGE_MAX_SIZE * sizeof(fbe_u8_t));
    if (response_buf_p == NULL)
    {
        fbe_cli_printf("\nFailed to allocate response buffer\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    eses_page_op.cmd_buf.raw_rcv_diag_page.rcv_diag_page = FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STRING_IN;

    eses_page_op.response_buf_p = response_buf_p;
    eses_page_op.response_buf_size = FBE_ESES_PAGE_MAX_SIZE;
    eses_page_op.required_response_buf_size = 0;

    status = fbe_api_enclosure_send_raw_rcv_diag_pg(object_id, &eses_page_op, enclosure_status);

    if(printFlag)
    {
        fbe_cli_printf("\nAllocated size %d, required size %d\n", FBE_ESES_PAGE_MAX_SIZE,
                   eses_page_op.required_response_buf_size);
    }

    if(status == FBE_STATUS_OK)
    {
        *response_buf_size = eses_page_op.required_response_buf_size;
    }
    else
    {
        *response_buf_size = 0;
    }

    // we are running with the risk of printing out staff not belong to us
    // when eses page return size is close to what we allocated.
    while (eses_page_op.required_response_buf_size > print_offset)
    {
        size = (eses_page_op.required_response_buf_size - print_offset > 60) ?
                    60 : (eses_page_op.required_response_buf_size - print_offset);
        if(printFlag)
        {
             fbe_cli_print_string(&response_buf_p[print_offset], size);
        }

        print_offset += size; //next 60 bytes
    }

    //in fbe_cli a '\n' will be print automatically after the last command output finish,
    //however in fbecli we have to add a '\n' manually
    fbe_cli_printf("\n");

    if(response_buf_p != NULL)
    {
        free(response_buf_p);
    }

    return status;

}// End of function fbe_cli_encl_send_string_in_cmd

/*!***************************************************************
* @fn fbe_cli_print_string(char *data, fbe_u8_t size)
****************************************************************
*  @brief
*      This function print the input string in char format
*
*  @param    data - input string
*  @param    size - number of bytes to print
*
*  @return   none
*
*
* HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
****************************************************************/
static void fbe_cli_print_string(char *data, fbe_u8_t size)
{
    fbe_u32_t index, print_location;
    char byte, buffer[256];

    fbe_zero_memory(buffer, 256);

    // we can't really print too much in one line because of buffer limit
    if (size < 80)
    {
        // print char format
        for (index = 0, print_location = 0  ;
            index < size;
            index++, print_location++)
        {
            if (((data[index] < ' ')||(data[index] > '~')) && (data[index] != '\n'))
            {
                data[index] = '.';
            }

            if (data[index] == '\0')
            {
                data[index] = '\n' ;
                if(index != 0)
                {
                    if(data[index - 1] == '\n')
                    {
                        data[index - 1] = ' ';
                    }
                }
            }
            byte = data[index];
            fbe_sprintf(&buffer[print_location], 2, "%c", byte);
        }
        byte = '\0';
        fbe_cli_printf("%s", buffer);
    }
}

/*!*************************************************************************
* @fn fbe_cli_cmd_enclosure_expThresholdIn(int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function will send the specified ThresholdIn command to the
*      specified Expander.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
void fbe_cli_cmd_enclosure_expThresholdIn(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
    fbe_enclosure_component_types_t componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;
    fbe_u32_t componentId = 0;
    fbe_bool_t componentIdflag = FALSE;
    fbe_u8_t  *tag = NULL;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids = {0};

    /*
    * Always need port & enclosure position plus possibly one more argument
    */
    if (argc < 2)
    {
        fbe_cli_printf("enclosure thresholdIn invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", THRESHOLDIN_USAGE);
        return;
    }

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    /*
    * Parse the command line.
    */
    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("Enclosure ThresholdIn, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("ThresholdIn, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-t") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("ThresholdIn, too few arguments \n");
                return;
            }
            componentType = fbe_cli_convertStringToComponentType(argv);
            if(componentType == FBE_ENCL_INVALID_COMPONENT_TYPE )
            {
                fbe_cli_error("ThresholdIn, Invalid Component type:%s\n",*argv);
                return;
            }
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nThresholdIn, too few arguments #.\n");
                return;
            }
            componentId = (fbe_u32_t)strtoul(*argv, 0, 0);
            componentIdflag = TRUE;
        }
        else
        {
            fbe_cli_printf("ThresholdIn invalid argument %s\n", *argv);
            return;
        }
        argc--;
        argv++;
    } // end of while(argc > 0)

    status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl_pos, &enclosure_object_ids);
    /* Get the component object ID for the specified EE's */
    if(componentIdflag)
    {
        /*Get the object ID for specified component ID */
        /*If we are targeting EE's*/
        object_handle_p = enclosure_object_ids.comp_object_id[componentId];
        tag ="componentID";
    }
    else
    {
        /* Get the object ID for the specified enclosure */
        object_handle_p = enclosure_object_ids.enclosure_object_id;
        tag = "Enclosure";
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nThresholdIn failed to get %s object handle status: %d\n",
            tag, status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nThresholdIn failed to get %s object handle\n",tag);
        return;
    }

    /* Format command & send to Enclosure object */
    status = fbe_cli_get_threshold_in(object_handle_p, componentType);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nThresholdIn failed to send control to bus %d, encl %d compId %d, status: %d\n",
            port, encl_pos, componentId, status);
        return;
    }
    return;

}// end of fbe_cli_cmd_enclosure_expStringIn

/*!*************************************************************************
* @fn fbe_cli_get_threshold_in
**************************************************************************
*  @brief
*  This function is the handler for using the "threshol in" command. This
*  parses the input and calls the function to execute hte operation.
*
*  @param object_id - The enlosure object Id.
*  @param componentType - The enclosure component Type.
*
*  @return   fbe_status_t
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
**************************************************************************/
static fbe_status_t fbe_cli_get_threshold_in(fbe_object_id_t object_id,
                                             fbe_enclosure_component_types_t componentType)
{
    fbe_enclosure_mgmt_ctrl_op_t    control_op;
    fbe_enclosure_status_t          enclosure_status;
    fbe_u8_t                        *response_bufp = NULL;
    fbe_u32_t                       response_buf_size = 0;
    fbe_status_t                    status = FBE_STATUS_OK;

    // todo: create a function that calulates actual size
    response_buf_size = 0x500;
    // Allocate the memory
    response_bufp = (fbe_u8_t *)malloc(response_buf_size);
    if (response_bufp == NULL)
    {
        fbe_cli_printf("fbe_cli_get_threshold_in: Fail to allocated ResponseBufSize:0x%x\n",response_buf_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // response_buf_p, response_buf_size.
    status = fbe_api_enclosure_build_threshold_in_cmd(&control_op,
        componentType,
        response_bufp,
        response_buf_size);

    // Send the operation to the physical package
    status = fbe_api_enclosure_get_threshold_in(object_id, &control_op, &enclosure_status);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to get threshold, status:0x%x.\n", status);
    }
    else
    {
        fbe_cli_printf("print threshold in\n");
        fbe_cli_display_enclosure_threshold_in(componentType, //element_type,
            control_op.required_response_buf_size,
            response_bufp);
    }

    if (response_bufp!= NULL)
    {
        free(response_bufp);
    }

    return status;
} // End fbe_cli_get_threshold_in

/*!***************************************************************
* @fn fbe_cli_print_enclosure_threshold_in
****************************************************************
* @brief
*  Parse the threshold in page and display the data.
*
* @param  componentType - Component Type
* @param  response_data_size - Reponse data size
* @param  *sourcep - source pointer
*
* @return  void
*
* HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
****************************************************************/
void fbe_cli_display_enclosure_threshold_in( fbe_enclosure_component_types_t componentType, //fbe_enclosure_element_type_t element_type,
                                            fbe_u32_t response_data_size,
                                            fbe_u8_t *sourcep)    // points to the buffer returned from target
{
    fbe_u32_t               bytes_processed;
    fbe_u8_t                *checkp = NULL;
    fbe_enclosure_mgmt_exp_threshold_info_t      *datap = NULL;

    // point to the first statistics_data_t item
    datap = (fbe_enclosure_mgmt_exp_threshold_info_t *)sourcep;

    checkp = sourcep + response_data_size;
    bytes_processed = 0;
    if (response_data_size > 0x500)
    {
        fbe_cli_printf("EnclStats:Response Data Truncated-Allocated:0x500 Need:0x%x\n",response_data_size);
        return;
    }

    fbe_cli_printf("\n Component Type : 0x0%x\n", componentType);
    // we can't really print too much in one line because of buffer limit
    fbe_cli_printf("\n  \tElement Type \t\t  | HI CRI THRESH |HI WAR THRES |LOW CRI THRESH  | LOW WAR THRESH\n");

    while ((bytes_processed < response_data_size) && (sourcep < checkp) )
    {
        fbe_cli_printf("%s(0x0%x)",
            elem_type_to_text(datap->elem_type),
            datap->elem_type);

        fbe_cli_printf("  0x0%x\t\t0x0%x\t\t 0x0%x\t\t0x0%x \n",
            datap->exp_overall_info.high_critical_threshold,   //HIGH CRITICAL THRESHOLD
            datap->exp_overall_info.high_warning_threshold,  //HIGH WARNING THRESHOLD
            datap->exp_overall_info.low_critical_threshold,     //LOW WARNING THRESHOLD
            datap->exp_overall_info.low_warning_threshold); //LOW CRITICAL THRESHOLD
        // next element item
        bytes_processed += sizeof(fbe_enclosure_mgmt_exp_threshold_info_t);
    }
    return;
} // fbe_cli_print_enclosure_threshold_in

/*!*************************************************************************
* @fn fbe_cli_cmd_enclosure_expThresholdOut
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function will send the specified ThresholdOut command to the
*      specified Expander.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
void fbe_cli_cmd_enclosure_expThresholdOut(int argc , char ** argv)
{
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
    fbe_enclosure_mgmt_exp_threshold_info_t* exp_threshold_buffer;
    fbe_u32_t componentId = 0;
    fbe_bool_t componentIdflag = FALSE;
    fbe_u8_t   *tag = NULL;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids = {0};

    /*
    * Always need port & enclosure position plus possibly one more argument
    */
    if (argc < 5)
    {
        fbe_cli_printf("enclosure ThresholdOut invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", EXPTHRESHOLDOUT_USAGE);
        return;
    }

    exp_threshold_buffer = &(esesPageOp.cmd_buf.thresholdOutInfo);

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    /*
    * Parse the command line.
    */

    while(argc > 0)
    {
        if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            encl_pos = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            componentId = (fbe_u32_t)strtoul(*argv, 0, 0);
            componentIdflag = TRUE;
        }
        else if(strcmp(*argv, "-et") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }

            exp_threshold_buffer->elem_type = fbe_cli_stringToElementType(argv);
            if (exp_threshold_buffer->elem_type == FBE_ELEMENT_TYPE_INVALID)
            {
                fbe_cli_error("Threshold Out, unknown element type:%s\n",*argv);
                return;
            }

        }

        else if(strcmp(*argv, "-hct") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            exp_threshold_buffer->exp_overall_info.high_critical_threshold = (fbe_u32_t)strtoul(*argv, 0, 0);

        }
        else if(strcmp(*argv, "-hwt") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            exp_threshold_buffer->exp_overall_info.high_warning_threshold = (fbe_u32_t)strtoul(*argv, 0, 0);

        }
        else if(strcmp(*argv, "-lwt") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            exp_threshold_buffer->exp_overall_info.low_warning_threshold = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if(strcmp(*argv, "-lct") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("enclosure ThresholdOut, too few arguments \n");
                return;
            }
            exp_threshold_buffer->exp_overall_info.low_critical_threshold = (fbe_u32_t)strtoul(*argv, 0, 0);
        }

        argc--;
        argv++;
    }

    status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl_pos, &enclosure_object_ids);
    /* Get the component object ID for the specified EE's */
    if(componentIdflag)
    {
        /*Get the object ID for specified component ID */
        /*If we are targeting EE's*/
        object_handle_p = enclosure_object_ids.comp_object_id[componentId];
        tag ="componentID";
    }
    else
    {
        /* Get the object ID for the specified enclosure */
        object_handle_p = enclosure_object_ids.enclosure_object_id;
        tag = "Enclosure";
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nThresholdOut failed to get %s object handle status: %d\n", tag, status);
        return;
    }
    if(object_handle_p == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("\nThresholdOut failed to get %s object handle\n",tag);
        return;
    }

    /* Format command & send to Enclosure object */
    status = fbe_api_enclosure_send_exp_threshold_out_control(object_handle_p, &esesPageOp);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\n ThresholdOut failed to send control to bus %d, encl %d, status: %d\n",
            port, encl_pos, status);
        return;
    }

    fbe_cli_printf("\nThreshOut cmd successfully completed for bus %d, encl %d, status: %d\n", port, encl_pos, status);

    return;

}   // end of fbe_cli_cmd_enclosure_expThresholdOut()

/**************************************************************************
* @ fbe_cli_cmd_all_enclbuf
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function calls enclbuf command for all enclosures and outputs trace buffers to console.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/

void fbe_cli_cmd_all_enclbuf(int argc , char ** argv)
{
    fbe_u32_t encl_pos = 0,port = 0,i= 0, comp_object_handle = FBE_OBJECT_ID_INVALID;
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID;
    fbe_cli_encl_cmd_type_t encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;

    if ((argc > 1 ))
    {
        fbe_cli_printf("\nallenclbuf: Invalid number of arguments %d.\n", argc);
        fbe_cli_printf("%s", ALLENCLBUF_USAGE);
        return;
    }
    else
    {
        /*
        * Check the command arguments
        */
        if(argc == 1)
        {
            if(strcmp(*argv, "-h") == 0)
            {
                argc--;
                argv++;
                if(argc == 0)
                {
                    fbe_cli_printf("%s", ALLENCLBUF_USAGE);
                    return;
                }
            }
            else if(strcmp(*argv, "-clear") == 0)
            {
                /* call a function to clear all trace buffers */
                encl_cmd_type = FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF;

            }


            argc--;
            argv++;
        }

    }
    for (port=0;port<FBE_API_PHYSICAL_BUS_COUNT;port++)
    {
        status = fbe_api_get_port_object_id_by_location(port, &object_handle_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to get port object handle status: %d\n",
                status);
            continue;
        }
        if(object_handle_p != FBE_OBJECT_ID_INVALID)
        {

            for(encl_pos=0;encl_pos<FBE_API_MAX_ALLOC_ENCL_PER_BUS;encl_pos++)
            {
                status = fbe_api_get_enclosure_object_ids_array_by_location(port, encl_pos, &enclosure_object_ids);
                object_handle_p = enclosure_object_ids.enclosure_object_id;

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nFailed to get enclosure object handle status: %d\n",
                        status);
                    continue;
                }

                if(object_handle_p == FBE_OBJECT_ID_INVALID)
                {
                    break;
                }
                else
                {
                    /* fire the enclbuf command with options */
                    fbe_cli_fire_enclbufcommands(object_handle_p,encl_cmd_type,port,encl_pos, FBE_COMP_ID_INVALID);

                } // else state

                /* Check For the Edge Expander */
                for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS && enclosure_object_ids.component_count >0; i++)
                {
                    comp_object_handle = enclosure_object_ids.comp_object_id[i];

                    if (comp_object_handle == FBE_OBJECT_ID_INVALID)
                    {
                        continue;
                    }

                    /* fire the enclbuf command with options */
                    fbe_cli_fire_enclbufcommands(comp_object_handle,encl_cmd_type,port,encl_pos, i);

                }
            } // else object_handle_p
        } // for encl_pos
    } // if object_handle_p
}

/**************************************************************************
 * @fn fbe_cli_cmd_expander_cmd
 *           (int argc , char ** argv)
 **************************************************************************
 *
 *  @brief
 *      This function calls different fbe_cli commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 *  @return   none
 *
 *  NOTES:
 *
 *  HISTORY:
 *  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
 *
 **************************************************************************/

void fbe_cli_cmd_expander_cmd(int argc , char ** argv)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t  encl_pos, port, object_handle = FBE_OBJECT_ID_INVALID;
    fbe_u32_t specified_port = 0, specified_encl = 0, specified_comp = 0,i;
    fbe_bool_t port_filter = FBE_FALSE, encl_filter = FBE_FALSE, comp_filter = FBE_FALSE, print_all = FBE_FALSE;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;

    if(argc < 2)
    {
         print_all = FBE_TRUE;    //print all
    }

    while(argc > 0)
    {
        /*
        Check the command type
        */
        if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("%s", EXPCMDS_USAGE);
                return;
            }
        }
        else if(strcmp(*argv, "-b") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nexpander_cmd: Please provide bus #.\n");
                return;
            }
            specified_port = (fbe_u32_t)strtoul(*argv, 0, 0);
            port_filter = FBE_TRUE;
        }
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nexpander_cmd: Please provide enclosure #.\n");
                return;
            }
            specified_encl = (fbe_u32_t)strtoul(*argv, 0, 0);
            encl_filter = FBE_TRUE;
        }
        else if(strcmp(*argv, "-cid") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("\nexpander_cmd: Please provide enclosure #.\n");
                return;
            }
            specified_comp = (fbe_u32_t)strtoul(*argv, 0, 0);
            comp_filter = FBE_TRUE;
        }
        argc--;
        argv++;
    }


    for (port = 0;port<FBE_PHYSICAL_BUS_COUNT;port++)
    {
        if(port_filter && (specified_port != port))
        {
            continue;
        }
        status = fbe_api_get_port_object_id_by_location(port, &object_handle);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to get object handle for bus %d,status: %d\n", port, status);
            continue;
        }

        if(object_handle != FBE_OBJECT_ID_INVALID)
        {
            for(encl_pos = 0;encl_pos<FBE_ENCLOSURES_PER_BUS;encl_pos++)
            {
                if(encl_filter && (specified_encl != encl_pos))
                {
                    continue;
                }
                status = fbe_api_get_enclosure_object_ids_array_by_location(port,encl_pos, &enclosure_object_ids);
                object_handle = enclosure_object_ids.enclosure_object_id;

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nFailed to get object handle for bus %d, encl %d, status: %d\n",
                        port, encl_pos, status);
                    continue;
                }

                if(object_handle == FBE_OBJECT_ID_INVALID)
                {
                    break;
                }
                else
                {
                    if(!comp_filter)
                    {
                        fbe_cli_send_exp_cmds(port, encl_pos, 0, object_handle);
                    }
                    if(enclosure_object_ids.component_count >0)
                    {
                        for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS; i++)
                        {
                            if (comp_filter && specified_comp != i)
                            {
                                continue;
                            }
                            object_handle = enclosure_object_ids.comp_object_id[i];
                            if(object_handle == FBE_OBJECT_ID_INVALID)
                            {
                                continue;
                            }
                            fbe_cli_send_exp_cmds(port, encl_pos, i, object_handle);
                        }
                    }
                }
            }
        }
    }
    return;
}

/**************************************************************************
* @ fbe_cli_cmd_all_enclbuf
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*   This function calls enclbuf command for all enclosures and outputs trace buffers to console.
*
*  @param object_id - The enclosure object Id.
*  @param encl_cmd_type - The enclosure command type.
*  @param port - The port number.
*  @param enclosure - The enclosure position number.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
fbe_status_t   fbe_cli_fire_enclbufcommands(fbe_object_id_t object_id,fbe_cli_encl_cmd_type_t encl_cmd_type, fbe_u32_t port,fbe_u32_t encl_pos, fbe_u32_t comp_id)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u8_t * response_buf_p = NULL;
    fbe_u32_t response_buf_size = 0;
    fbe_u8_t number_of_trace_bufs = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t  output_to_file = FALSE;
    fbe_u8_t buf_id = 0;
    fbe_u8_t index = 0;

    ses_trace_buf_info_elem_struct  * trace_buf_info_elem_p;
    fbe_u8_t total_number_of_trace_bufs = 0;
    fbe_u8_t rqst_number_of_trace_bufs = 0;
    fbe_u8_t i = 0;
    fbe_u8_t rev_level[MAX_FW_REV_STR_LEN+1] = {0};
    fbe_u8_t  temp[50] ={0} ;
    fbe_u8_t  comp_id_str[50] ={0} ;

    fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info;
    fbe_edal_block_handle_t enclosureControlInfo;
    fbe_u8_t side_id;

    fbe_u32_t retry_count = 0;

    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM,
        &number_of_trace_bufs, 1);

    //send scsi cmd to read buffer, retry when busy.
    retry_count = 0;
    while(1)
    {
        status = fbe_api_enclosure_get_trace_buffer_info(object_id, &eses_page_op, &enclosure_status);
        retry_count++;

        if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get the number of trace buffers, encl busy, will retry, count: %d.\n", retry_count);
            fbe_api_sleep(200);
        }
        else
        {
            break;
        }
    }

    if(status != FBE_STATUS_OK)
    {
        if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get the number of trace buffers, encl busy, retry count exhausted.\n");
        }
        else
        {
            fbe_cli_printf("\nFailed to get the number of trace buffers, status:0x%x.\n", status);
        }
        return status;
    }

    fbe_cli_printf("   \n=== There are totally %d trace buffers ====\n", number_of_trace_bufs);

    if(number_of_trace_bufs == 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    response_buf_size = number_of_trace_bufs * sizeof(ses_trace_buf_info_elem_struct) + 1;
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size * sizeof(fbe_u8_t));
    if (response_buf_p==NULL)
    {
        fbe_cli_printf("\nFailed to get response buffer memory.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    enclosure_info = (fbe_base_object_mgmt_get_enclosure_info_t *)malloc(sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
    if (enclosure_info==NULL)
    {
        fbe_cli_printf("\nFailed to get enclosure edal memory.\n");
        free(response_buf_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_enclosure_build_trace_buf_cmd(&eses_page_op, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS,
        response_buf_p, response_buf_size);

    enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    //send scsi cmd to read buffer, retry when busy.
    retry_count = 0;
    while(1)
    {
        status = fbe_api_enclosure_get_trace_buffer_info(object_id, &eses_page_op, &enclosure_status);
        retry_count++;

        if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get trace buffer status, encl busy, will retry, count: %d.\n", retry_count);
            fbe_api_sleep(200);
        }
        else
        {
            break;
        }
    }

    if(status != FBE_STATUS_OK)
    {
        if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
        {
            fbe_cli_printf("\nFailed to get trace buffer status, encl busy, retry count exhausted.\n");
        }
        else
        {
            fbe_cli_printf("\nFailed to get trace buffer status, status:0x%x.\n", status);
        }
        free(response_buf_p);
        free(enclosure_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    total_number_of_trace_bufs = *response_buf_p;

    trace_buf_info_elem_p = (ses_trace_buf_info_elem_struct *)(response_buf_p + 1);

    rqst_number_of_trace_bufs = (response_buf_size - 1) / FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE;
    number_of_trace_bufs = (rqst_number_of_trace_bufs < total_number_of_trace_bufs) ? rqst_number_of_trace_bufs : total_number_of_trace_bufs;

    /* let's not do the save, but retrieve the active trace buffer instead.
     * fbe_cli_encl_trace_buf_ctrl(object_id,FBE_ENCLOSURE_BUF_ID_UNSPECIFIED,FBE_ENCLOSURE_TRACE_BUF_OP_SAVE);
     */
    // Get the enclosure component data blob via the API.
    status = fbe_api_enclosure_get_info(object_id, enclosure_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nfailed to get enclosure data status:%d\n", status);
        free(response_buf_p);
        free(enclosure_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    enclosureControlInfo = enclosure_info;
    /* Enclosure Data contains some pointers that must be converted */
    fbe_edal_updateBlockPointers(enclosureControlInfo);

    fbe_edal_getEnclosureSide(enclosureControlInfo,&side_id);

    if (comp_id != FBE_COMP_ID_INVALID)
    {
        _snprintf(comp_id_str,sizeof(comp_id_str)," comp_id:%d", comp_id);
    }
    else
    {
        comp_id_str[0] = '\0'; // Pass an empty Nul terminated string for non-componenet id
    }

    index = fbe_edal_find_first_edal_match_U8(enclosureControlInfo,
            FBE_ENCL_COMP_SIDE_ID,  //attribute
            FBE_ENCL_LCC,  // Component type
            0, //starting index
            side_id);

    if(index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        edalStatus = fbe_edal_getU8((void*)enclosureControlInfo,
                            FBE_ENCL_BD_ACT_TRC_BUFFER_ID ,
                            FBE_ENCL_LCC,
                            index,
                            &buf_id);


        _snprintf(temp,sizeof(temp),"bus:%d encl_pos:%d%s active trace bid %d ", port, encl_pos, comp_id_str, buf_id);
        fbe_cli_encl_save_buf_data_to_file(object_id,buf_id, "allenclbuf_logs.txt", output_to_file, temp);  //open the file in the write and append mode
    }
    else
    {
        fbe_cli_printf ("SIDE_ID: INVALID");
    }

    for(i=0; i<number_of_trace_bufs;i++)
    {
        fbe_zero_memory(rev_level, MAX_FW_REV_STR_LEN + 1);
        strncpy(rev_level,trace_buf_info_elem_p->rev_level,MAX_FW_REV_STR_LEN);
        fbe_zero_memory(temp, sizeof(temp));
        _snprintf(temp,sizeof(temp)-1,"bus:%d encl_pos:%d%s rev:%s buf act: %s", port, encl_pos, comp_id_str,
                rev_level, fbe_eses_decode_trace_buf_action_status(trace_buf_info_elem_p->buf_action));
        fbe_cli_encl_save_buf_data_to_file(object_id,trace_buf_info_elem_p->buf_id, "allenclbuf_logs.txt", output_to_file, temp);  //open the file in the write and append mode

        trace_buf_info_elem_p = fbe_eses_get_next_trace_buf_info_elem_p(trace_buf_info_elem_p, FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE);
    }

   /* check if the user wanted to clear out the buffers */
   if( encl_cmd_type == FBE_CLI_ENCL_CMD_TYPE_CLEAR_TRACE_BUF)
    {
       //send scsi cmd to read buffer, retry when busy.
       retry_count = 0;
       while(1)
       {
           status = fbe_cli_encl_trace_buf_ctrl(object_id, FBE_ENCLOSURE_BUF_ID_UNSPECIFIED, FBE_ENCLOSURE_TRACE_BUF_OP_CLEAR); //clear out each trace buffer
           retry_count++;

           if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
           {
               fbe_cli_printf("\nFailed to clear out the trace buffers, encl busy, will retry, count: %d.\n", retry_count);
               fbe_api_sleep(200);
           }
           else
           {
               break;
           }
       }

       //if the last ret status is not ok, mean the operation failed.
       if(status != FBE_STATUS_OK)
       {
           if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
           {
               fbe_cli_printf("\nFailed to clear out the trace buffers, encl busy, retry count exhausted.\n");
           }
           else
           {
               fbe_cli_printf("\nFailed to clear out the trace buffers, status:0x%x.\n", status);
           }
       }
    }
    if(response_buf_p != NULL)
    {
        free(response_buf_p);
    }
    if (enclosure_info != NULL)
    {
        free(enclosure_info);
    }

  return FBE_STATUS_OK ;
 }

static void fbe_cli_send_exp_cmds(fbe_u32_t bus, fbe_u32_t encl_pos, fbe_u32_t component_id, fbe_u32_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t    esesPageOp;
    fbe_u8_t  *cmdArray[] = {"pphy", "pdrv", "pdevices", "pinfo"};
    fbe_u32_t cmdCount = (sizeof(cmdArray)/sizeof(*cmdArray));
    fbe_u32_t cmd, response_buf_size = 0;
    fbe_u32_t currentTime = 0;
    fbe_u8_t recievedData = FALSE;

    fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    /* flushing out the expander buffer completely */
    do
    {
        status = fbe_cli_encl_send_string_in_cmd(object_handle, &enclosure_status,&response_buf_size,FBE_FALSE);
        if(status != FBE_STATUS_OK)
        {
            if(component_id >0)
            {
                fbe_cli_printf("\nexpStringIn failed to send control to bus %d, encl %d, Component id: %d, status: %d\n",
                        bus, encl_pos, component_id, status);
            }
            else
            {
                fbe_cli_printf("\nexpStringIn failed to send control to bus %d, encl %d, status: %d\n",
                        bus, encl_pos, status);
            }
            return;
        }
    }while(response_buf_size >  FBE_ESES_PAGE_SIZE);  //since min eses page size is set to 4 by default

     /* executing expander cmds and collecting output */
    for(cmd = 0; cmd<cmdCount ; cmd++)
    {
        fbe_sprintf(esesPageOp.cmd_buf.stringOutInfo.stringOutCmd, sizeof(esesPageOp.cmd_buf.stringOutInfo.stringOutCmd), "%s\n", cmdArray[cmd]);

        /* Format command & send to Enclosure object */
        status = fbe_api_enclosure_send_exp_string_out_control(object_handle, &esesPageOp);

        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nexpStringOut failed to send control to bus %d, encl %d, status: %d\n", bus, encl_pos, status);
            return;
        }

        if(component_id >0)
        {
            fbe_cli_printf("\n(Bus: %d Enclosure: %d Component id: %d Cmd: %s)\n",bus, encl_pos, component_id, cmdArray[cmd]);
        }
        else
        {
            fbe_cli_printf("\n(Bus: %d Enclosure: %d Cmd: %s)\n",bus, encl_pos,cmdArray[cmd]);
        }

        currentTime = 0;
        recievedData = FALSE;

        while ((currentTime < FBE_STRING_IN_CMD_WAIT_TIME) && (recievedData == FALSE)) 
        {
            fbe_api_sleep(FBE_STRING_IN_CMD_INTERVAL_TIME);

            do 
            {
                status = fbe_cli_encl_send_string_in_cmd(object_handle, &enclosure_status,&response_buf_size,FBE_TRUE);

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nexpStringIn failed to send control to bus %d, encl %d, status: %d\n", bus, encl_pos, status);
                    return;
                }

                if ((recievedData == FALSE) && (response_buf_size >  FBE_ESES_PAGE_SIZE))
                {
                      recievedData = TRUE;
                }
                
            }while ((recievedData == TRUE) && ((response_buf_size >  FBE_ESES_PAGE_SIZE)));

            currentTime +=  FBE_STRING_IN_CMD_INTERVAL_TIME;
        }

        fbe_zero_memory(&esesPageOp, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    }
}

/*!*************************************************************************
* @fn fbe_cli_cmd_enclosure_e_log_command
*           (int argc , char ** argv)
**************************************************************************
*
*  @brief
*      This function will print the event log with the help of read buffer command.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
void fbe_cli_cmd_enclosure_e_log_command(int argc , char ** argv)
{
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID, port = 0, encl_pos = 0, i=0, comp_object_handle = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u8_t buf_id = 0;
    fbe_u8_t  temp[50] ={0} ;
    fbe_base_object_mgmt_get_enclosure_info_t enclosure_info;
    fbe_edal_block_handle_t enclosureControlInfo = &enclosure_info;
    fbe_u32_t index=0;
    fbe_u8_t side_id;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids;

    /* There should be a minimum of arguments. */
    if ((argc > 1)||(argc < 0))
    {
        fbe_cli_printf(" invalid number of arguments %d\n", argc);
        fbe_cli_printf("%s", EVENTLOG_USAGE);
        return;
    }

    /* Parse the command line. */
    while(argc > 0)
    {
        /* Check the command type */
        if(strcmp(*argv, "-h") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_printf("%s", EVENTLOG_USAGE);
                return;
            }
        }
        else
        {
            fbe_cli_printf(" invalid argument %d\n", argc);
            fbe_cli_printf("%s", EVENTLOG_USAGE);
            return;
        }
    }

    for (port = 0; port < FBE_API_PHYSICAL_BUS_COUNT; port++)
    {
        status = fbe_api_get_port_object_id_by_location(port, &object_id);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to get port object handle status: %d\n",status);
            continue;
        }
        if(object_id != FBE_OBJECT_ID_INVALID)
        {
            for(encl_pos = 0; encl_pos < FBE_API_ENCLOSURES_PER_BUS; encl_pos++)
            {
                status = fbe_api_get_enclosure_object_ids_array_by_location(port,encl_pos, &enclosure_object_ids);
                object_id = enclosure_object_ids.enclosure_object_id;

                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nFailed to get enclosure object handle status: %d\n", status);
                    continue;
                }
                if(object_id == FBE_OBJECT_ID_INVALID)
                {
                    break;
                }
                else
                {

                    fbe_zero_memory(&enclosure_info, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));

                    // Get the enclosure component data blob via the API.
                    status = fbe_api_enclosure_get_info(object_id, &enclosure_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("\nfailed to get enclosure data status:%d\n", status);
                        continue;
                    }

                    /* Enclosure Data contains some pointers that must be converted */
                    fbe_edal_updateBlockPointers(enclosureControlInfo);
                    _snprintf(temp,sizeof(temp),"bus:%d encl_pos:%d", port,encl_pos);

                   fbe_edal_getEnclosureSide(enclosureControlInfo,&side_id);

                    index = fbe_edal_find_first_edal_match_U8(enclosureControlInfo,
                            FBE_ENCL_COMP_SIDE_ID,  //attribute
                            FBE_ENCL_LCC,  // Component type
                            0, //starting index
                            side_id);
                    if(index != FBE_ENCLOSURE_VALUE_INVALID)
                    {
                        fbe_edal_getEnclosureSide(enclosureControlInfo,&side_id);

                        edalStatus = fbe_edal_getU8((void*)enclosureControlInfo,
                            FBE_ENCL_BD_ELOG_BUFFER_ID ,
                            FBE_ENCL_LCC,
                            index,
                            &buf_id);

                       fbe_cli_encl_print_e_log(object_id,buf_id,temp);
                    }
                    else
                    {
                        fbe_cli_printf("\nfailed to get the first matching side_id for %s\n", temp);
                    }

                    /* Check For the Edge Expander */
                    for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS && enclosure_object_ids.component_count >0 ; i++)
                    {
                        comp_object_handle = enclosure_object_ids.comp_object_id[i];

                        if (comp_object_handle == FBE_OBJECT_ID_INVALID)
                        {
                            continue;
                        }

                        fbe_zero_memory(&enclosure_info, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));

                        // Get the enclosure component data blob via the API.
                        status = fbe_api_enclosure_get_info(comp_object_handle, &enclosure_info);
                        if(status != FBE_STATUS_OK)
                        {
                            fbe_cli_printf("\nfailed to get enclosure data status:%d\n", status);
                             continue;
                        }

                        /* Enclosure Data contains some pointers that must be converted */
                        fbe_edal_updateBlockPointers(enclosureControlInfo);
                        _snprintf(temp,sizeof(temp),"bus:%d encl_pos:%d, comp_id:%d", port,encl_pos, i);

                        fbe_edal_getEnclosureSide(enclosureControlInfo,&side_id);

                        edalStatus = fbe_edal_getU8((void*)enclosureControlInfo,
                                 FBE_ENCL_BD_ELOG_BUFFER_ID ,
                                 FBE_ENCL_LCC,
                                 side_id,
                                 &buf_id);

                        fbe_cli_encl_print_e_log(comp_object_handle,buf_id,temp);

                    }
                } // else object_handle_p
            } // for encl_pos
        } // if object_handle_p
    }

    return;
}// end of fbe_cli_cmd_enclosure_expStringIn

/*!*************************************************************************
* @fn fbe_cli_encl_print_e_log
*
**************************************************************************
*
*  @brief
*      This function will print the event log with the help of read buffer command.
*
*  @param    object_id - The enclosure object Id.
*  @param    buf_id - The buffer Id.
*  @param    temp - string which contains bus, enclosure
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*    12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
**************************************************************************/
static fbe_status_t fbe_cli_encl_print_e_log(fbe_object_id_t object_id, fbe_u8_t buf_id, char *temp)
{
    fbe_u32_t buf_capacity = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t  temp_buff[100] = {0};

    if(buf_id == FBE_ENCLOSURE_VALUE_INVALID)
    {
        fbe_cli_printf("\nInvalid buf_id: 0x%x.\n", buf_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Read the buffer capacity.
    status = fbe_cli_encl_get_buf_size(object_id, buf_id, &buf_capacity, &enclosure_status);
    if((status != FBE_STATUS_OK) || (buf_capacity == 0))
    {
        return status;
    }

    _snprintf(temp_buff,sizeof(temp_buff),"\n\n %s object id %d buffer id %d\n\n",temp,object_id,buf_id);

    fbe_cli_printf("\n%s\n\n",temp_buff);    //write header first to the console

    status = read_and_output_buffer(buf_id, buf_capacity, object_id, NULL, "");
    return status;
}

/*!***************************************************************
* @fn fbe_cli_encl_save_buf_data_to_file(fbe_u32_t buf_capacity,
*                                   FILE *fp)
****************************************************************
* @brief
*   This function collects expander trace and either saves it to a file (if fp is not NULL) or
*   prints it to standard out.
*
* @param  buf_capacity - size of the buffer to collect and print
* @param  fp - File pointer to output file.  If NULL output to stdout
*
* @return  fbe_status_t
*
* HISTORY:
*  12-Aprl-2012 ZDONG - Merged from fbe_cli code.
*
****************************************************************/

static fbe_status_t
read_and_output_buffer(fbe_u8_t buf_id, fbe_u32_t buf_capacity, fbe_object_id_t object_id, FILE *fp, const char *filename)
{
    fbe_enclosure_mgmt_ctrl_op_t eses_page_op;
    fbe_u32_t bytes_read = 0;
    fbe_u32_t bytes_to_read = 0;
    fbe_u32_t xfer_count = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclosure_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t * response_buf_p = NULL;
    fbe_u32_t  byte_index = 0;
    fbe_u32_t retry_count = 0;

    response_buf_p = (fbe_u8_t *)malloc(FBE_ESES_TRACE_BUF_MIN_TRANSFER_SIZE + sizeof(fbe_u8_t) );
    if (response_buf_p == NULL)
    {
        fbe_cli_printf("\nFailed to allocate a data buffer for expander trace collection\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Read the buffer data.
    for(bytes_read = 0; bytes_read < buf_capacity;)
    {
        // Get the actual size of data that will be read this time.
        if(buf_capacity - bytes_read >= FBE_ESES_TRACE_BUF_MIN_TRANSFER_SIZE)
        {
            bytes_to_read = FBE_ESES_TRACE_BUF_MIN_TRANSFER_SIZE;
        }
        else
        {
            bytes_to_read = buf_capacity - bytes_read;
        }

        // fbe_api_enclosure_build_read_buf_cmd will zero the memory allocated.
        fbe_api_enclosure_build_read_buf_cmd(&eses_page_op, buf_id, FBE_ESES_READ_BUF_MODE_DATA,
            bytes_read,  // starting byte offset within the specified buffer.
            response_buf_p,
            bytes_to_read);

        //send scsi cmd to read buffer, retry when busy.
        retry_count = 0;
        while(1)
        {
            status = fbe_api_enclosure_read_buffer(object_id, &eses_page_op, &enclosure_status);
            retry_count++;

            if(status == FBE_STATUS_BUSY && retry_count < FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
            {
                fbe_cli_printf("\nFailed to read the data of buffer id %d, encl busy, will retry, count: %d.\n", buf_id, retry_count);
                fbe_api_sleep(200);
            }
            else
            {
                break;
            }
        }

        //if the last ret status is not ok, mean the operation failed.
        if(status != FBE_STATUS_OK)
        {
            if(status == FBE_STATUS_BUSY && retry_count >= FBE_ENCL_SEND_SCSI_CMD_RETRY_COUNT_MAX)
            {
                fbe_cli_printf("\nFailed to read the data of buffer id %d, encl busy, retry count exhausted.\n", buf_id);
            }
            else
            {
                fbe_cli_printf("\nFailed to read the data of buffer id %d, status: 0x%x.\n", buf_id, status);
            }
            break;
        }
        else if (fp)
        {
            xfer_count = eses_page_op.required_response_buf_size;
            if(fwrite(response_buf_p, sizeof(fbe_u8_t), xfer_count, fp) != xfer_count)
            {
                fbe_cli_printf("\nFailed to write to file %s.\n", filename);
                break;
            }
        }
        else
        {
            /* The buffer can have '\0' characters in it. This causes problem with
            * functions that work on NULL terminated strings. Thus replacing these '\0'
            * with blank spaces if two consecutive chars are not '\0's or '\n's.
            */
            xfer_count = eses_page_op.required_response_buf_size;
            for(byte_index = 0 ; byte_index < xfer_count; byte_index++)
            {
                // If the character is not printable, replace it with a '.'
                if (((response_buf_p[byte_index] < ' ')||(response_buf_p[byte_index]> '~')) && (response_buf_p[byte_index]!= '\n') &&
                     (response_buf_p[byte_index] != '\0'))
                {
                    response_buf_p[byte_index] = '.';
                }
                if(response_buf_p[byte_index] == '\0')
                {
                    response_buf_p[byte_index] = '\n' ;
                    if(byte_index!= 0)
                    {
                        if(response_buf_p[byte_index - 1] == '\n')
                            response_buf_p[byte_index - 1] = ' ';
                    }
                }
            }
            response_buf_p[ xfer_count] = '\0' ;
            fbe_cli_printf("%s",response_buf_p);
        }
        bytes_read += xfer_count;
        if (xfer_count == 0)
        {
            break;
        }
    }

    if(response_buf_p != NULL)
    {
        free(response_buf_p);
    }
    return status;
}
