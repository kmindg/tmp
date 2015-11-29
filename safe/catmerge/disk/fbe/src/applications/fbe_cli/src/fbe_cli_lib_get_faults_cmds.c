/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_get_faults_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This command is designed to display all Array faults.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  05/10/2013:  Lin Lou - created
 *
 ***************************************************************************/

#include "fbe_cli_lib_get_faults_cmds.h"
#include "generic_utils_lib.h"

/*!*****************************************************************************
 * @fn fbe_cli_cmd_get_faults ()
 *******************************************************************************
 * @brief
 *  fbe_cli_cmd_get_faults shows all faults of the Array
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
void fbe_cli_cmd_get_faults(fbe_s32_t argc,char ** argv)
{
    fbe_bool_t                        fault = FBE_FALSE;
    fbe_package_id_t                  package_id = FBE_PACKAGE_ID_ESP;

    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)))
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", GET_FAULTS_USAGE);
        return;
    }
   
    while(argc > 0)
    {
        if(strcmp(*argv, "esp") == 0)
        {
            package_id = FBE_PACKAGE_ID_ESP;
            argc--;
            argv++;
        }
        else if((strcmp(*argv, "phy") == 0) ||
           (strcmp(*argv, "pp") == 0))
        {
            package_id = FBE_PACKAGE_ID_PHYSICAL;
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "sep") == 0)
        {
            package_id = FBE_PACKAGE_ID_SEP_0;
            argc--;
            argv++;
        }
        else // invalid arguments
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", GET_FAULTS_USAGE);
            return;
        }
    }

    if(package_id == FBE_PACKAGE_ID_ESP)
    {
        fault |= fbe_cli_print_esp_sp_faults();
        fault |= fbe_cli_print_esp_sp_fltReg_faults();
        fault |= fbe_cli_print_esp_suitcase_faults();

        fault |= fbe_cli_print_esp_bmc_faults();

        fault |= fbe_cli_print_esp_module_faults(FBE_DEVICE_TYPE_IOMODULE);
        fault |= fbe_cli_print_esp_module_faults(FBE_DEVICE_TYPE_MEZZANINE);
        fault |= fbe_cli_print_esp_module_faults(FBE_DEVICE_TYPE_BACK_END_MODULE);

        fault |= fbe_cli_print_esp_mgmt_module_faults();

        fault |= fbe_cli_print_esp_sps_faults();
        fault |= fbe_cli_print_esp_bbu_faults();

        fault |= fbe_cli_print_esp_encl_faults();

        if(!fault)
        {
            fbe_cli_printf("No ESP faults detected.\n");
        }
    }
    else if(package_id == FBE_PACKAGE_ID_ESP)
    {
        fbe_cli_printf("Not implemented.\n");
    }
    else if(package_id == FBE_PACKAGE_ID_SEP_0)
    {
        fbe_cli_printf("Not implemented.\n");
    }

    return;
}


/*!*****************************************************************************
 * @fn fbe_cli_print_esp_sp_faults ()
 *******************************************************************************
 * @brief
 *  print all component faults on SP, including SP itself.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_sp_faults(void)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_esp_board_mgmt_board_info_t   board_info;
    fbe_device_physical_location_t    location = {0};
    fbe_bool_t                        fault = FBE_FALSE;
    fbe_bool_t                        anyFault = FBE_FALSE;
    fbe_esp_encl_mgmt_encl_info_t     encl_info = {0};
    char                              fault_string[FAULT_STRING_LENGTH] = {0};
    SP_ID                             sp;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
        return anyFault;
    }

    /* SP Faults */
    if(board_info.lowBattery)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Low Battery", FAULT_STRING_LENGTH);
    }

    if(board_info.engineIdFault)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "engineIdFault", FAULT_STRING_LENGTH);
    }

    if(!board_info.peerPresent)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Peer Removed", FAULT_STRING_LENGTH);
    }

    if(board_info.internalCableStatus == FBE_CABLE_STATUS_MISSING)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Missing Internal SPC Cable", FAULT_STRING_LENGTH);
    }

    if(board_info.internalCableStatus == FBE_CABLE_STATUS_CROSSED)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Internal SPC Cross Cabled", FAULT_STRING_LENGTH);
    }

    /* System Serial Number Invalid */
    if(board_info.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }

    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl info, status:%d\n", status);
    }
    else if(encl_info.enclFaultLedReason & FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "System SN Invalid", FAULT_STRING_LENGTH);
    }

    if(fault)
    {
        anyFault = FBE_TRUE;
        fault_string[strlen(fault_string)-2] = '\0';
        fbe_cli_printf("SP: %s\n", fault_string);
    }

    /* SP Resume Prom Faults */
    fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
    fault = FBE_FALSE;
    for( sp = SP_A; sp < SP_ID_MAX; sp++ )
    {
        location.sp = sp;
        fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_SP, &location, fault_string);
        if(fault)
        {
            anyFault = FBE_TRUE;
            fbe_cli_printf("%s Resume Prom: %s\n", (sp==SP_A) ? "SPA" : "SPB", fault_string);
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_sp_fltReg_faults()
 *******************************************************************************
 * @brief
 *  print SP fault register faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any fault register faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_sp_fltReg_faults(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_esp_board_mgmt_peer_boot_info_t peerBootInfo = {0};
    char                                fault_string[FAULT_STRING_LENGTH] = {0};

    /* fault register fault */
    status = fbe_api_esp_board_mgmt_getPeerBootInfo(&peerBootInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get peer boot info, status:%d\n", status);
        return fault;
    }

    if(peerBootInfo.peerBootState == FBE_PEER_HUNG)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Peer Hung", FAULT_STRING_LENGTH);
    }

    if(peerBootInfo.fltRegStatus.anyFaultsFound)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Peer FltReg Fault", FAULT_STRING_LENGTH);
    }

    /*
    if(peerBootInfo.biosPostFail)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Peer BIOS Post Fail", FAULT_STRING_LENGTH);
    }
    */

    if(fault)
    {
        fault_string[strlen(fault_string)-2] = '\0';
        fbe_cli_printf("SP FltReg: %s\n", fault_string);
    }

    return fault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_suitcase_faults()
 *******************************************************************************
 * @brief
 *  print processor enclosure suitcase faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any suitcase faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_suitcase_faults(void)
{
    fbe_status_t                        status;
    fbe_esp_board_mgmt_suitcaseInfo_t   suitcaseInfo = {0};
    fbe_u32_t                           suitcaseIndex, suitcaseCount;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    char                                fault_string[FAULT_STRING_LENGTH] = {0};
    SP_ID                               sp;

    status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get suitcase info, status: %d.\n", __FUNCTION__, status);
        return anyFault;
    }

    suitcaseCount = suitcaseInfo.suitcaseCount;
    for( sp = SP_A; sp < SP_ID_MAX; sp++ )
    {
        for (suitcaseIndex = 0; suitcaseIndex < suitcaseCount; suitcaseIndex++)
        {
            fault = FBE_FALSE;
            fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);

            suitcaseInfo.location.sp = sp;
            suitcaseInfo.location.slot = suitcaseIndex;
            status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get suitcase info, status: %d.\n", __FUNCTION__, status);
                continue;
            }

            if (suitcaseInfo.suitcaseInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string,
                        fbe_base_env_decode_env_status(suitcaseInfo.suitcaseInfo.envInterfaceStatus),
                        FAULT_STRING_LENGTH);
            }

            if(suitcaseInfo.suitcaseInfo.shutdownWarning == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "shutdownWarning", FAULT_STRING_LENGTH);
            }

            if(suitcaseInfo.suitcaseInfo.ambientOvertempFault == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "ambientOvertempFault", FAULT_STRING_LENGTH);
            }

            if(suitcaseInfo.suitcaseInfo.ambientOvertempWarning == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "ambientOvertempWarning", FAULT_STRING_LENGTH);
            }

            if(suitcaseInfo.suitcaseInfo.state == FBE_SUITCASE_STATE_FAULTED)
            {
                switch (suitcaseInfo.suitcaseInfo.subState)
                {
                case FBE_SUITCASE_SUBSTATE_HW_ERR_MON_FAULT:
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, "Tap12VMissing", FAULT_STRING_LENGTH);
                    break;
                case FBE_SUITCASE_SUBSTATE_BIST_FAILURE:
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, "BistFailure", FAULT_STRING_LENGTH);
                    break;
                default:
                    break;
                }
            }

            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                fbe_cli_printf("%s Suitcase %d: %s\n", (sp==SP_A) ? "SPA" : "SPB", suitcaseIndex, fault_string);
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_bmc_faults()
 *******************************************************************************
 * @brief
 *  print all BMC faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any BMC faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_bmc_faults(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           slot;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    char                                fault_string[FAULT_STRING_LENGTH] = {0};
    SP_ID                               sp;
    fbe_esp_board_mgmt_bmcInfo_t        bmcInfo = {0};

    for( sp = SP_A; sp < SP_ID_MAX; sp++ )
    {
        for (slot = 0; slot < TOTAL_BMC_PER_BLADE; slot++)
        {
            fault = FBE_FALSE;
            fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
            fbe_zero_memory(&bmcInfo, sizeof(fbe_esp_board_mgmt_bmcInfo_t));

            bmcInfo.location.sp = sp;
            bmcInfo.location.slot = slot;
            status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to get bmc %d info, status:%d\n", slot, status);
                continue;
            }

            /*
            if (bmcInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string,
                        fbe_base_env_decode_env_status(bmcInfo.envInterfaceStatus),
                        FAULT_STRING_LENGTH);
            }
            */

            if(bmcInfo.bmcInfo.shutdownWarning)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "shutdownWarning", FAULT_STRING_LENGTH);
            }

            /*
            if(bmcInfo.bmcInfo.bistReport)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "BIST failed", FAULT_STRING_LENGTH);
            }
            */

            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                fbe_cli_printf("%s BMC %d: %s\n", (sp==SP_A) ? "SPA" : "SPB", slot, fault_string);
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_module_faults(fbe_u64_t device_type)
 *******************************************************************************
 * @brief
 *  print all module faults for given type of device.
 *
 * @param :
 *   fbe_u64_t device_type - module device type
 *
 * @return:
 *   fbe_bool_t - if any module faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_module_faults(fbe_u64_t device_type)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           slot_num, port_num;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    char                                fault_string[FAULT_STRING_LENGTH] = {0};
    char *                              device_string = NULL;
    fbe_device_physical_location_t      location = {0};
    SP_ID                               sp;
    fbe_u32_t                           max_slots = 0;
    fbe_esp_module_io_module_info_t     module_info = {0};
    fbe_esp_module_mgmt_module_status_t module_status_info = {0};
    fbe_esp_module_limits_info_t        limits_info = {0};
    fbe_esp_module_io_port_info_t       io_port_info = {0};

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get module limits info, status:%d\n", status);
        return anyFault;
    }

    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
        device_string = "IO Module";
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
        device_string = "Mezzanine";
    }
    else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_bem;
        device_string = "Base Module";
    }
    else
    {
        fbe_cli_printf("Module device type invalid.\n");
        return anyFault;
    }

    for( sp = SP_A; sp < SP_ID_MAX; sp++ )
    {
        for(slot_num = 0; slot_num < max_slots; slot_num++)
        {
            fault = FBE_FALSE;
            fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
            fbe_zero_memory(&module_info, sizeof(fbe_esp_module_io_module_info_t));

            module_info.header.sp = sp;
            module_info.header.type = device_type;
            module_info.header.slot = slot_num;

            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&module_info);

            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_printf("Failed to get module info, status:%d\n", status);
                continue;
            }

            module_status_info.header.sp = sp;
            module_status_info.header.type = device_type;
            module_status_info.header.slot = slot_num;

            status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);

            if(status == FBE_STATUS_GENERIC_FAILURE)
            {
                fbe_cli_printf("Failed to get module status info, status:%d\n", status);
                continue;
            }

            if(module_info.io_module_phy_info.inserted != FBE_MGMT_STATUS_TRUE)
            {
                continue;
            }

            if(module_status_info.state == MODULE_STATE_UNSUPPORTED_MODULE)
            {
                fault = FBE_TRUE;
                if(module_status_info.substate == MODULE_SUBSTATE_UNSUPPORTED_NOT_COMMITTED)
                {
                    fbe_cli_cat_fault_string(fault_string, "Unsupported Not Committed", FAULT_STRING_LENGTH);
                }
                else /*if(module_status_info.substate == MODULE_SUBSTATE_UNSUPPORTED_MODULE)*/
                {
                    fbe_cli_cat_fault_string(fault_string, "Unsupported", FAULT_STRING_LENGTH);
                }
            }
            else if(module_status_info.state == MODULE_STATE_FAULTED)
            {
                fault = FBE_TRUE;
                if(module_status_info.substate == MODULE_SUBSTATE_INCORRECT_MODULE)
                {
                    fbe_cli_cat_fault_string(fault_string, "Incorrect Module", FAULT_STRING_LENGTH);
                }
                else if(module_status_info.substate == MODULE_SUBSTATE_POWERED_OFF)
                {
                    fbe_cli_cat_fault_string(fault_string, "Powered Off", FAULT_STRING_LENGTH);
                }
                else if(module_status_info.substate == MODULE_SUBSTATE_POWERUP_FAILED)
                {
                    fbe_cli_cat_fault_string(fault_string, "Power Up Failed", FAULT_STRING_LENGTH);
                }
                else if(module_status_info.substate == MODULE_SUBSTATE_INTERNAL_FAN_FAULTED)
                {
                    fbe_cli_cat_fault_string(fault_string, "Internal Fan Faulted", FAULT_STRING_LENGTH);
                }
                else if(module_status_info.substate == MODULE_SUBSTATE_FAULT_REG_FAILED)
                {
                    fbe_cli_cat_fault_string(fault_string, "Fault Register Failed", FAULT_STRING_LENGTH);
                }
                else
                {
                    fbe_cli_cat_fault_string(fault_string, "Unknown Fault", FAULT_STRING_LENGTH);
                }
            }

            /* print module fault */
            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                fbe_cli_printf("%s %s %d: %s\n",
                                (sp==SP_A) ? "SPA" : "SPB",
                                device_string, slot_num, fault_string);
            }

            /* print module resume prom fault */
            location.sp = sp;
            location.slot = slot_num;
            fault = fbe_cli_any_resume_prom_fault(device_type, &location, fault_string);
            if(fault)
            {
                anyFault = FBE_TRUE;
                fbe_cli_printf("%s %s %d Resume Prom: %s\n",
                                (sp==SP_A) ? "SPA" : "SPB",
                                 device_string, slot_num, fault_string);
            }

            for(port_num = 0; port_num < limits_info.discovered_hw_limit.num_ports; port_num++)
            {
                fault = FBE_FALSE;
                fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);

                 io_port_info.header.sp = sp;
                 io_port_info.header.type = device_type;
                 io_port_info.header.slot = module_info.io_module_phy_info.slotNumOnBlade;
                 io_port_info.header.port = port_num;

                 status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&io_port_info);

                 if(status == FBE_STATUS_GENERIC_FAILURE)
                 {
                     continue;
                 }

                 if(io_port_info.io_port_logical_info.port_state == FBE_PORT_STATE_FAULTED)
                 {
                     fault = FBE_TRUE;
                     if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_INCORRECT_MODULE)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Incorrect Module", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_UNSUPPORTED_SFP)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Unsupported SFP", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_SFP_READ_ERROR)
                     {
                         fbe_cli_cat_fault_string(fault_string, "SFP Read Error", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_FAULTED_SFP)
                     {
                         fbe_cli_cat_fault_string(fault_string, "SFP Faulted", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Exceeded Max Limits", FAULT_STRING_LENGTH);
                     }
                     else
                     {
                         fbe_cli_cat_fault_string(fault_string, "Unknown Fault", FAULT_STRING_LENGTH);
                     }
                 }
                 else if(io_port_info.io_port_logical_info.port_state == FBE_PORT_STATE_UNKNOWN)
                 {
                     fault = FBE_TRUE;
                     if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_MODULE_POWERED_OFF)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Powered Off", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_MODULE_READ_ERROR)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Module Read Error", FAULT_STRING_LENGTH);
                     }
                     else if(io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_UNSUPPORTED_MODULE)
                     {
                         fbe_cli_cat_fault_string(fault_string, "Module Unsupported", FAULT_STRING_LENGTH);
                     }
                 }
            }

            /* print ports fault */
            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                fbe_cli_printf("%s %s %d port %d: %s\n",
                                (sp==SP_A) ? "SPA" : "SPB",
                                device_string, slot_num, port_num, fault_string);
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_mgmt_module_faults()
 *******************************************************************************
 * @brief
 *  print all management module faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any management module faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_mgmt_module_faults(void)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_bool_t                               fault = FBE_FALSE;
    fbe_bool_t                               anyFault = FBE_FALSE;
    char                                     fault_string[FAULT_STRING_LENGTH] = {0};
    fbe_esp_module_limits_info_t             limits_info = {0};
    fbe_esp_module_mgmt_get_mgmt_comp_info_t mgmt_comp_info = {0};
    fbe_device_physical_location_t           location = {0};
    fbe_u32_t                                slot;
    SP_ID                                    sp;

    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get limits info, status: %d\n", __FUNCTION__, status);
        return anyFault;
    }

    for( sp = SP_A; sp < SP_ID_MAX; sp++ )
    {
        for(slot = 0; slot < limits_info.discovered_hw_limit.num_mgmt_modules; slot++)
        {
            fault = FBE_FALSE;
            fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
            fbe_zero_memory(&mgmt_comp_info, sizeof(fbe_esp_module_mgmt_get_mgmt_comp_info_t));

            mgmt_comp_info.phys_location.sp = sp;
            mgmt_comp_info.phys_location.slot = slot;

            status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: ERROR in get mgmt module %d info, status: %d.\n", __FUNCTION__, slot, status);
                continue;
            }

            if(!mgmt_comp_info.mgmt_module_comp_info.bInserted)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Removed", FAULT_STRING_LENGTH);
            }
            else
            {
                if(mgmt_comp_info.mgmt_module_comp_info.generalFault == FBE_MGMT_STATUS_TRUE)
                {
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, "General Fault", FAULT_STRING_LENGTH);
                }
                if(mgmt_comp_info.mgmt_module_comp_info.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
                {
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string,
                            fbe_base_env_decode_env_status(mgmt_comp_info.mgmt_module_comp_info.envInterfaceStatus),
                            FAULT_STRING_LENGTH);
                }
            }

            /* print mgmt module fault */
            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                fbe_cli_printf("%s Mgmt Module %d: %s\n", (sp==SP_A) ? "SPA" : "SPB", slot, fault_string);
            }

            /* print mgmt module resume prom fault */
            location.sp = sp;
            location.slot = slot;
            fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_MGMT_MODULE, &location, fault_string);
            if(fault)
            {
                anyFault = FBE_TRUE;
                fbe_cli_printf("%s MGMT Module %d Resume Prom: %s\n",
                                (sp==SP_A) ? "SPA" : "SPB", slot, fault_string);
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_bbu_faults()
 *******************************************************************************
 * @brief
 *  print all BBU faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any BBU faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_bbu_faults(void)
{
    fbe_esp_sps_mgmt_bobStatus_t    bbu_status_info;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       bbuIndex, bbuCount;
    fbe_bool_t                      anyFault = FBE_FALSE;
    fbe_bool_t                      fault = FBE_FALSE;
    char                            fault_string[FAULT_STRING_LENGTH] = {0};
    fbe_device_physical_location_t  location = {0};

    status = fbe_api_esp_sps_mgmt_getBobCount(&bbuCount);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get BBU Counts, status %d\n", status);
        return anyFault;
    }
    if (bbuCount == 0)
    {
        return anyFault;
    }

    for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
    {
        fbe_set_memory(&bbu_status_info, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
        bbu_status_info.bobIndex = bbuIndex;
        status = fbe_api_esp_sps_mgmt_getBobStatus(&bbu_status_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error in get bbu status, error code: %d\n", __FUNCTION__, status);
            continue;
        }

        if(!bbu_status_info.bobInfo.batteryInserted)
        {
            fault = FBE_TRUE;
            fbe_cli_cat_fault_string(fault_string, "Removed", FAULT_STRING_LENGTH);
        }
        else
        {
            if(bbu_status_info.bobInfo.batteryFaults == FBE_BATTERY_FAULT_TEST_FAILED)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "TestFailed", FAULT_STRING_LENGTH);
            }
            else if(bbu_status_info.bobInfo.batteryFaults == FBE_BATTERY_FAULT_CANNOT_CHARGE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "CannotCharge", FAULT_STRING_LENGTH);
            }
            else if(bbu_status_info.bobInfo.batteryFaults == FBE_BATTERY_FAULT_NOT_READY)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "NotReady", FAULT_STRING_LENGTH);
            }
        }
        if(fault)
        {
            anyFault = FBE_TRUE;
            fault_string[strlen(fault_string)-2] = '\0';
            fbe_cli_printf("BBU %d: %s\n", bbuIndex, fault_string);
        }

        /* BBU Resume Prom Fault */
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        location.slot = bbuIndex;
        fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_SP, &location, fault_string);
        if(fault)
        {
            anyFault = FBE_TRUE;
            fbe_cli_printf("BBU %d Resume Prom: %s\n", bbuIndex, fault_string);
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_sps_faults()
 *******************************************************************************
 * @brief
 *  print all SPS faults.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any SPS faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_sps_faults(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      anyFault = FBE_FALSE;
    fbe_bool_t                      fault = FBE_FALSE;
    char                            fault_string[FAULT_STRING_LENGTH] = {0};
    fbe_u32_t                       enclIndex, spsIndex;
    fbe_esp_board_mgmt_board_info_t boardInfo;
    fbe_esp_sps_mgmt_spsCounts_t    spsCountInfo;
    fbe_esp_sps_mgmt_spsStatus_t    spsInfo;
    fbe_common_cache_status_t       cacheStatus;

    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get sps count, status: %d\n", __FUNCTION__, status);
        return anyFault;
    }

    if (spsCountInfo.totalNumberOfSps == 0)
    {
        return anyFault;
    }

    // get overall CacheStatus
    status = fbe_api_esp_sps_mgmt_getCacheStatus(&cacheStatus);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get sps_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return anyFault;
    }
    if(cacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN
            || cacheStatus == FBE_CACHE_STATUS_FAILED)
    {
        anyFault = FBE_TRUE;
        fbe_cli_printf("SPS: Cache %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
        return anyFault;
    }

    for (enclIndex = 0; enclIndex < spsCountInfo.enclosuresWithSps; enclIndex++)
    {
        fbe_zero_memory(&spsInfo, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if ((boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) &&
            (enclIndex == 0))
        {
            spsInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }

        for (spsIndex = 0; spsIndex < spsCountInfo.spsPerEnclosure; spsIndex++)
        {
            fault = FBE_FALSE;
            fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
            spsInfo.spsLocation.slot = spsIndex;

            status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error in get sps status, status: %d\n", __FUNCTION__, status);
                continue;
            }

            if(!spsInfo.spsModuleInserted)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Module Removed", FAULT_STRING_LENGTH);
            }
            else
            {
                if(spsInfo.dualComponentSps && !spsInfo.spsBatteryInserted)
                {
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, "Battery Removed", FAULT_STRING_LENGTH);
                }

                if(spsInfo.spsStatus == SPS_STATE_FAULTED)
                {
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, fbe_cli_print_SpsFaults(&spsInfo.spsFaultInfo), FAULT_STRING_LENGTH);
                }

                if(spsInfo.spsCablingStatus != FBE_SPS_CABLING_VALID)
                {
                    fault = FBE_TRUE;
                    fbe_cli_cat_fault_string(fault_string, fbe_cli_print_SpsCablingStatus(spsInfo.spsCablingStatus), FAULT_STRING_LENGTH);
                }
            }

            if(fault)
            {
                anyFault = FBE_TRUE;
                fault_string[strlen(fault_string)-2] = '\0';
                if ((boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) &&
                    (enclIndex == 0))
                {
                    spsInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
                    spsInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    fbe_cli_printf("SPE SPS %d: %s\n", spsIndex, fault_string);
                }
                else
                {
                    fbe_cli_printf("ENCL %d_%d SPS %d: %s\n",
                            spsInfo.spsLocation.bus,
                            spsInfo.spsLocation.enclosure,
                            spsIndex, fault_string);
                }
                continue;
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_any_resume_prom_fault()
 *******************************************************************************
 * @brief
 *  get resume prom faults string for given device.
 *
 * @param :
 *   fbe_u64_t device_type -
 *   fbe_device_physical_location_t *pLocation - device location
 *   char *fault_string - fault string (OUTPUT)
 *
 * @return:
 *   fbe_bool_t - if any resume prom faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_any_resume_prom_fault(fbe_u64_t device_type, fbe_device_physical_location_t *pLocation, char *fault_string)
{
    fbe_bool_t                                  fault = FBE_FALSE;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_base_env_get_resume_prom_info_cmd_t getResumePromInfo = {0};

    fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);

    getResumePromInfo.deviceType = device_type;
    getResumePromInfo.deviceLocation = *pLocation;

    status = fbe_api_esp_common_get_resume_prom_info(&getResumePromInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get %s resume prom info, status: 0x%X.\n",
                fbe_base_env_decode_device_type(device_type), status);
        return fault;
    }

    if(getResumePromInfo.op_status > FBE_RESUME_PROM_STATUS_READ_SUCCESS)
    {
        fault = FBE_TRUE;
        strncpy(fault_string, fbe_base_env_decode_resume_prom_status(getResumePromInfo.op_status), FAULT_STRING_LENGTH-1);
    }

    return fault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_encl_faults()
 *******************************************************************************
 * @brief
 *  print all faults for devices in the enclosure.
 *
 * @param :
 *   None
 *
 * @return:
 *   fbe_bool_t - if any faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_encl_faults(void)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_esp_board_mgmt_board_info_t   board_info;
    fbe_device_physical_location_t    location = {0};
    fbe_bool_t                        fault = FBE_FALSE;
    fbe_u8_t                          bus, encl;
    fbe_u32_t                         enclCntOnBus = 0;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return fault;
    }

    if(board_info.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        fault = fbe_cli_print_esp_specific_encl_faults(&location);
        fault |= fbe_cli_print_esp_fan_faults(&location);
        fault |= fbe_cli_print_esp_ps_faults(&location);
        fault |= fbe_cli_print_esp_drive_faults(&location);
    }

    for (bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus ++)
    {
        location.bus = bus;
        status = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCntOnBus);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get Enclosure count on bus %d, status: 0x%x.\n", bus, status);
            continue;
        }

        if(enclCntOnBus == 0)
        {
            continue;
        }

        for(encl = 0; encl<enclCntOnBus; encl++)
        {
            location.enclosure = encl;
            fault |= fbe_cli_print_esp_specific_encl_faults(&location);
            fault |= fbe_cli_print_esp_fan_faults(&location);
            fault |= fbe_cli_print_esp_ps_faults(&location);
            fault |= fbe_cli_print_esp_drive_faults(&location);
        }
    }

    return fault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_specific_encl_faults()
 *******************************************************************************
 * @brief
 *  print devices faults on the specific enclosure, including enclosure,
 *  midplane resume prom, drive midplane resume prom, connector, LCC and
 *  LCC resume prom.
 *
 * @param :
 *   fbe_device_physical_location_t *pLocation - enclosure location
 *
 * @return:
 *   fbe_bool_t - if any enclosure faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_specific_encl_faults(fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_bool_t                        fault = FBE_FALSE;
    fbe_bool_t                        anyFault = FBE_FALSE;
    fbe_esp_encl_mgmt_encl_info_t     encl_info = {0};
    char                              fault_string[FAULT_STRING_LENGTH] = {0};
    fbe_u32_t                         slot;
    fbe_esp_encl_mgmt_lcc_info_t      lccInfo = {0};
    fbe_u32_t                         connectorCount;
    fbe_esp_encl_mgmt_connector_info_t connectorInfo = {0};

    /* Enclosure Fault */
    fault = FBE_FALSE;
    fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
    status = fbe_api_esp_encl_mgmt_get_encl_info(pLocation, &encl_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Failed to get enclosure %d_%d info.\n", __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return anyFault;
    }

    if(!encl_info.enclPresent)
    {
        return anyFault;
    }

    if(encl_info.enclState == FBE_ESP_ENCL_STATE_FAILED)
    {
        fault = FBE_TRUE;
        if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL)
        {
            fbe_cli_cat_fault_string(fault_string, "Lifecycle State Fail", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_CROSS_CABLED)
        {
            fbe_cli_cat_fault_string(fault_string, "Cross Cabled", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED)
        {
            fbe_cli_cat_fault_string(fault_string, "Backend Loop Miscabled", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_LCC_INVALID_UID)
        {
            fbe_cli_cat_fault_string(fault_string, "LCC Invalid UID", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX)
        {
            fbe_cli_cat_fault_string(fault_string, "Exceeded Max", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_UNSUPPORTED)
        {
            fbe_cli_cat_fault_string(fault_string, "Unsupported Enclosure Type", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_PS_TYPE_MIX)
        {
            fbe_cli_cat_fault_string(fault_string, "PS Type Mix", FAULT_STRING_LENGTH);
        }
        else if(encl_info.enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_RP_FAULTED)
        {
            fbe_cli_cat_fault_string(fault_string, "Resume PROM Fault", FAULT_STRING_LENGTH);
        }
    }

    if(encl_info.enclFaultLedReason & FBE_ENCL_FAULT_LED_OVERTEMP_FLT)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "overTempFailure", FAULT_STRING_LENGTH);
    }

    if(encl_info.enclFaultLedReason & FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL)
    {
        fault = FBE_TRUE;
        fbe_cli_cat_fault_string(fault_string, "Sub Enclosure Lifecycle State Fail", FAULT_STRING_LENGTH);
    }

    if(fault)
    {
        anyFault = FBE_TRUE;
        fault_string[strlen(fault_string)-2] = '\0';
        fbe_cli_printf("ENCL %d_%d: %s\n", pLocation->bus, pLocation->enclosure, fault_string);
    }

    /* Midplane Resume Prom Fault */
    fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
    fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_ENCLOSURE, pLocation, fault_string);
    if(fault)
    {
        anyFault = FBE_TRUE;
        fbe_cli_printf("ENCL %d_%d Resume Prom: %s\n", pLocation->bus, pLocation->enclosure, fault_string);
    }

    /* Drive Midplane Resume Prom Fault */
    if(encl_info.driveMidplaneCount>0)
    {
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_DRIVE_MIDPLANE, pLocation, fault_string);
        if(fault)
        {
            anyFault = FBE_TRUE;
            fbe_cli_printf("ENCL %d_%d Drive Midplane Resume Prom: %s\n", pLocation->bus, pLocation->enclosure, fault_string);
        }
    }

    /* LCC Fault */
    for(slot = 0; slot < encl_info.lccCount; slot++)
    {
        fault = FBE_FALSE;
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fbe_zero_memory(&lccInfo, sizeof(fbe_esp_encl_mgmt_lcc_info_t));

        pLocation->slot = slot;
        status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf(" Failed to get LCC info for LCC %d, status:%d\n", slot, status);
            continue;
        }

        if(!lccInfo.inserted)
        {
            fault = FBE_TRUE;
            fbe_cli_cat_fault_string(fault_string, "Removed", FAULT_STRING_LENGTH);
        }

        if(lccInfo.faulted)
        {
            fault = FBE_TRUE;
            fbe_cli_cat_fault_string(fault_string, "General Fault", FAULT_STRING_LENGTH);
        }

        if(fault)
        {
            anyFault = FBE_TRUE;
            fault_string[strlen(fault_string)-2] = '\0';
            fbe_cli_printf("ENCL %d_%d LCC %d: %s\n",
                    pLocation->bus, pLocation->enclosure, pLocation->slot,
                    fault_string);
        }

        /* LCC Resume Prom Fault */
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_LCC, pLocation, fault_string);
        if(fault)
        {
            anyFault = FBE_TRUE;
            fbe_cli_printf("ENCL %d_%d LCC %d Resume Prom: %s\n",
                    pLocation->bus, pLocation->enclosure, pLocation->slot,
                    fault_string);
        }
    }

    /* Connector Fault */
    status = fbe_api_esp_encl_mgmt_get_connector_count(pLocation, &connectorCount);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf(" Failed to get connector count enclosure %d_%d, status:%d\n", pLocation->bus, pLocation->enclosure, status);
        return anyFault;
    }

    for(slot = 0; slot < connectorCount; slot ++)
    {
        fault = FBE_FALSE;
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fbe_zero_memory(&connectorInfo, sizeof(fbe_esp_encl_mgmt_connector_info_t));

        pLocation->slot = slot;

        status = fbe_api_esp_encl_mgmt_get_connector_info(pLocation, &connectorInfo);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf(" Failed to get enclosure %d_%d connector %d info, status:%d\n",
                    pLocation->bus, pLocation->enclosure, pLocation->slot, status);
            continue;
        }

        if(connectorInfo.isLocalFru && connectorInfo.cableStatus == FBE_CABLE_STATUS_DEGRADED)
        {
            fault = FBE_TRUE;
            anyFault = FBE_TRUE;
            fbe_cli_printf("ENCL %d_%d Connector %d: Cable Status Degraded\n",
                    pLocation->bus, pLocation->enclosure, pLocation->slot);
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_fan_faults()
 *******************************************************************************
 * @brief
 *  print all fan faults on specific enclosure.
 *
 * @param :
 *   fbe_device_physical_location_t *pLocation - enclosure location
 *
 * @return:
 *   fbe_bool_t - if any fan faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_fan_faults(fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    fbe_u32_t                           slot = 0;
    fbe_esp_encl_mgmt_encl_info_t       enclInfo = {0};
    fbe_esp_cooling_mgmt_fan_info_t     fanInfo = {0};
    char                                fault_string[FAULT_STRING_LENGTH] = {0};

    status = fbe_api_esp_encl_mgmt_get_encl_info(pLocation, &enclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl Info, status:%d\n", status);
        return anyFault;
    }

    if(enclInfo.fanCount == 0)
    {
        return anyFault;
    }

    for(slot = 0; slot < enclInfo.fanCount; slot ++)
    {
        fault = FBE_FALSE;
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fbe_zero_memory(&fanInfo, sizeof(fbe_esp_cooling_mgmt_fan_info_t));

        pLocation->slot = slot;
        status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get fan info,status:%d\n", status);
            continue;
        }

        if(fanInfo.inserted != FBE_MGMT_STATUS_TRUE)
        {
            fault = FBE_TRUE;
            fbe_cli_cat_fault_string(fault_string, "Removed", FAULT_STRING_LENGTH);
        }
        else
        {
            if(fanInfo.fanFaulted == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "General Fault", FAULT_STRING_LENGTH);
            }
            if(fanInfo.fanDegraded == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Fan Degraded", FAULT_STRING_LENGTH);
            }
            if(fanInfo.isFaultRegFail == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "FaultReg Fault", FAULT_STRING_LENGTH);
            }
            if(fanInfo.multiFanModuleFaulted == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Multiple Fan Fault", FAULT_STRING_LENGTH);
            }
            if(fanInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string,
                        fbe_base_env_decode_env_status(fanInfo.envInterfaceStatus),
                        FAULT_STRING_LENGTH);
            }
        }

        /* print fan fault */
        if(fault)
        {
            anyFault = FBE_TRUE;
            fault_string[strlen(fault_string)-2] = '\0';

            if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
            {
                fbe_cli_printf("xPE Fan %d: %s\n", slot, fault_string);
            }
            else
            {
                fbe_cli_printf("Encl %d_%d Fan %d: %s\n",
                        pLocation->bus, pLocation->enclosure, pLocation->slot, fault_string);
            }
        }

        /* print fan resume prom fault */
        if (fanInfo.resumePromSupported)
        {
            fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_FAN, pLocation, fault_string); 
            if(fault)
            {
                anyFault = FBE_TRUE;
                if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
                {
                    fbe_cli_printf("xPE Fan Resume Prom %d: %s\n", slot, fault_string);
                }
                else
                {
                    fbe_cli_printf("Encl %d_%d Fan Resume Prom %d: %s\n",
                            pLocation->bus, pLocation->enclosure, pLocation->slot, fault_string);
                }
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_ps_faults()
 *******************************************************************************
 * @brief
 *  print all power supply faults on specific enclosure.
 *
 * @param :
 *   fbe_device_physical_location_t *pLocation - enclosure location
 *
 * @return:
 *   fbe_bool_t - if any PS faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_ps_faults(fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    fbe_u32_t                           psCount, slot;
    fbe_esp_ps_mgmt_ps_info_t           psInfo = {0};
    char                                fault_string[FAULT_STRING_LENGTH] = {0};

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));

    status = fbe_api_esp_ps_mgmt_getPsCount(pLocation, &psCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get PS count for bus: %d, encl: %d\n",
                pLocation->bus, pLocation->enclosure);
        return anyFault;
    }

    if(psCount == 0)
    {
        return anyFault;
    }

    for(slot = 0; slot < psCount; slot++)
    {
        fault = FBE_FALSE;
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fbe_zero_memory(&psInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));

        pLocation->slot = slot;
        psInfo.location = *pLocation;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get PS Info, status %d\n", status);
            continue;
        }

        if(!psInfo.psInfo.bInserted)
        {
            fault = FBE_TRUE;
            fbe_cli_cat_fault_string(fault_string, "Removed", FAULT_STRING_LENGTH);
        }
        else
        {
            if(psInfo.psInfo.generalFault == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                if(psInfo.psInfo.ACFail == FBE_MGMT_STATUS_TRUE)
                {
                    fbe_cli_cat_fault_string(fault_string, "General Fault/ACFail", FAULT_STRING_LENGTH);
                }
                else
                {
                    fbe_cli_cat_fault_string(fault_string, "General Fault", FAULT_STRING_LENGTH);
                }
            }

            if(psInfo.psInfo.internalFanFault == FBE_MGMT_STATUS_TRUE)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "InternalFanFlt", FAULT_STRING_LENGTH);
            }
            if(psInfo.psInfo.isFaultRegFail)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "FaultReg Fault", FBE_MGMT_STATUS_TRUE);
            }

            if(psInfo.psInfo.generalFault == FBE_MGMT_STATUS_UNKNOWN)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Unknown Fault", FAULT_STRING_LENGTH);
            }

            if(psInfo.psInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string,
                        fbe_base_env_decode_env_status(psInfo.psInfo.envInterfaceStatus),
                        FAULT_STRING_LENGTH);
            }

        }

        /* print PS fault */
        if(fault)
        {
            anyFault = FBE_TRUE;
            fault_string[strlen(fault_string)-2] = '\0';

            if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
            {
                fbe_cli_printf("xPE PS %d: %s\n", slot, fault_string);
            }
            else
            {
                fbe_cli_printf("Encl %d_%d PS %d: %s\n",
                        pLocation->bus, pLocation->enclosure, pLocation->slot, fault_string);
            }
        }

        /* print PS resume prom fault */
        fault = fbe_cli_any_resume_prom_fault(FBE_DEVICE_TYPE_PS, pLocation, fault_string);
        if(fault)
        {
            anyFault = FBE_TRUE;
            if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
            {
                fbe_cli_printf("xPE PS Resume Prom %d: %s\n", slot, fault_string);
            }
            else
            {
                fbe_cli_printf("Encl %d_%d PS Resume Prom %d: %s\n",
                        pLocation->bus, pLocation->enclosure, pLocation->slot, fault_string);
            }
        }

    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_print_esp_drive_faults()
 *******************************************************************************
 * @brief
 *  print all drive faults on specific enclosure.
 *
 * @param :
 *   fbe_device_physical_location_t *pLocation - enclosure location
 *
 * @return:
 *   fbe_bool_t - if any drive faults detected
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_cli_print_esp_drive_faults(fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          fault = FBE_FALSE;
    fbe_bool_t                          anyFault = FBE_FALSE;
    fbe_u32_t                           max_slot, slot;
    char                                fault_string[FAULT_STRING_LENGTH] = {0};
    fbe_esp_drive_mgmt_drive_info_t     drive_info={0};
    fbe_object_id_t                     object_id;
    fbe_enclosure_mgmt_get_drive_slot_info_t getDriveSlotInfo = {0};
    fbe_lifecycle_state_t               lifecycle_state;
    fbe_object_death_reason_t           death_reason;
    const fbe_u8_t *                    death_reason_str = NULL;

    status = fbe_api_esp_encl_mgmt_get_drive_slot_count(pLocation, &max_slot);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get drive count for bus: %d, encl: %d\n",
                pLocation->bus, pLocation->enclosure);
        return anyFault;
    }

    if (max_slot == 0)
    {
        return anyFault;
    }

    for(slot = 0; slot < max_slot; slot++)
    {
        fault = FBE_FALSE;
        fbe_zero_memory(fault_string, FAULT_STRING_LENGTH);
        fbe_zero_memory(&drive_info, sizeof(fbe_esp_drive_mgmt_drive_info_t));

        pLocation->slot = slot;
        drive_info.location = *pLocation;

        status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s:Failed to get drive information.\n", __FUNCTION__);
            continue;
        }

        if(drive_info.inserted)
        {
            if(drive_info.state == FBE_LIFECYCLE_STATE_FAIL)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Lifecycle State Fail", FAULT_STRING_LENGTH);
            }
            else if(drive_info.state == FBE_LIFECYCLE_STATE_PENDING_FAIL)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Lifecycle State Pending Fail", FAULT_STRING_LENGTH);
            }
            else if(drive_info.state == FBE_LIFECYCLE_STATE_DESTROY)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Lifecycle State Destroy", FAULT_STRING_LENGTH);
            }

            status = fbe_api_get_physical_drive_object_id_by_location(pLocation->bus,
                                                                      pLocation->enclosure,
                                                                      pLocation->slot,
                                                                      &object_id);
            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
            {
                fbe_cli_error("\nFailed to get physical drive object handle for port: %d enclosure: %d slot: %d\n",
                        pLocation->bus, pLocation->enclosure, pLocation->slot);
                continue;
            }

            status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                        object_id, status);
                continue;
            }

            status  = fbe_api_get_object_death_reason(object_id, &death_reason, &death_reason_str, FBE_PACKAGE_ID_PHYSICAL);
            if ((status == FBE_STATUS_OK) && (death_reason != FBE_DEATH_REASON_INVALID)) {
                fault = FBE_TRUE;
                /* fbe_cli_cat_fault_string(fault_string, death_reason_str, FAULT_STRING_LENGTH); */
            }
        }
        else
        {
            status = fbe_api_enclosure_get_disk_parent_object_id(pLocation->bus,
                                                                 pLocation->enclosure,
                                                                 pLocation->slot,
                                                                 &object_id);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Can't get enclosure Object Id by Location, Port:%d, Enclosure:%d, Slot:%d",
                        pLocation->bus, pLocation->enclosure, pLocation->slot);
                continue;
            }

            getDriveSlotInfo.location = *pLocation;
            status = fbe_api_enclosure_get_drive_slot_info(object_id, &getDriveSlotInfo);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s:ESES enclosure get drive slot info error:%d\n", __FUNCTION__, status);
                continue;
            }

            if(getDriveSlotInfo.inserted)
            {
                fault = FBE_TRUE;
                fbe_cli_cat_fault_string(fault_string, "Powering Up", FAULT_STRING_LENGTH);
            }
        }

        /* print drive fault */
        if(fault)
        {
            anyFault = FBE_TRUE;
            fault_string[strlen(fault_string)-2] = '\0';

            if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
            {
                fbe_cli_printf("xPE Drive %d: %s\n", slot, fault_string);
            }
            else
            {
                fbe_cli_printf("Encl %d_%d Drive %d: %s\n",
                        pLocation->bus, pLocation->enclosure, pLocation->slot, fault_string);
            }
        }
    }

    return anyFault;
}

/*!*****************************************************************************
 * @fn fbe_cli_cat_fault_string
 *     (char *fault_string, char *fault_reason, fbe_u32_t string_length)
 *******************************************************************************
 * @brief
 *  concatenate fault reason to the fault string.
 *
 * @param :
 *   char * - fault_string
 *   char * - fault_reason
 *   fbe_u32_t - string_length
 *
 * @return:
 *   None
 *
 * @author
 *   05/10/2013 Lin Lou - Created.
 *
 ******************************************************************************/
static void fbe_cli_cat_fault_string(char *fault_string, char *fault_reason, fbe_u32_t string_length) {
    if(string_length <= (strlen(fault_string)+strlen(fault_reason)+2))
    {
        return;
    }

    strncat(fault_string, fault_reason, string_length-strlen(fault_string)-1);
    strncat(fault_string, ", ", string_length-strlen(fault_string)-1);

    return;
}
