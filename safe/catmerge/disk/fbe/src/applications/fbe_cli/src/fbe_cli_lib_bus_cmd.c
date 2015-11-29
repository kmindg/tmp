/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_bus_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains bus command definitions.
 *
 * @version
 *  21/Feb/2012 - Created. Eric Zhou
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe_cli_bus_cmd.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "speeds.h"
#include "fbe_cli_private.h"

/*************************
 *   FUNCTION PROTOTYPES
 *************************/
void fbe_cli_print_businfo(fbe_u32_t bus);
void fbe_cli_get_speed_string(fbe_u32_t speed, char **stringBuf);
void fbe_cli_get_all_speeds_string(fbe_u32_t speed, char *stringBuf);
static void fbe_cli_print_bus_info(fbe_esp_module_io_port_info_t *io_port_info);
static void fbe_cli_print_bus_info_by_role(fbe_port_role_t port_role, fbe_u64_t device_type);

/*!*************************************************************************
* @void fbe_cli_cmd_bus_info(fbe_s32_t argc,char ** argv)
**************************************************************************
*
*  @brief
*  This function will get and print bus info.
*
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  21-Feb-2012 Eric Zhou - Created.
* **************************************************************************/
void fbe_cli_cmd_bus_info(fbe_s32_t argc,char ** argv)
{
    fbe_u32_t beBusCount = 0;
    fbe_status_t status = FBE_STATUS_OK;

    /* Determines if we should target this command at specific bus. */
    
    status = fbe_api_esp_get_max_be_bus_count(&beBusCount);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when getting max backend bus count, error code: %d.\n", __FUNCTION__,  status);
        return;
    }

    if (beBusCount == 0)
    {
        fbe_cli_printf("%s: No back end bus found\n", __FUNCTION__);
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
                fbe_cli_printf("%s", BUSINFO_USAGE);
                return;
            }
        }
        else
        {
            fbe_cli_printf("Invalid arguments \n");
            fbe_cli_printf("%s", BUSINFO_USAGE);
            return;
        }
        argc--;
        argv++;
    }

    fbe_cli_printf("\n******************************bus information********************************\n");
    fbe_cli_printf("Role  Num  LinkState  currentSpd   capableSpeeds   numOfEncl  maxEncl  maxEnclExceed\n");
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_BE, FBE_DEVICE_TYPE_MEZZANINE);
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_BE, FBE_DEVICE_TYPE_BACK_END_MODULE);
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_BE, FBE_DEVICE_TYPE_IOMODULE);
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_FE, FBE_DEVICE_TYPE_MEZZANINE);
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_FE, FBE_DEVICE_TYPE_BACK_END_MODULE);
    fbe_cli_print_bus_info_by_role(FBE_PORT_ROLE_FE, FBE_DEVICE_TYPE_IOMODULE);

    return;
}
/****************************
 * end fbe_cli_cmd_bus_info()
 ****************************/

/*!*************************************************************************
* @void fbe_cli_print_businfo(fbe_u32_t bus)
**************************************************************************
*
*  @brief
*  This function will print bus info.
*
*  @param    bus - the bus no.
*
*  @return   none
*
*  NOTES:
*
*  HISTORY:
*  21-Feb-2012 Eric Zhou - Created.
* **************************************************************************/
void fbe_cli_print_bus_info(fbe_esp_module_io_port_info_t *io_port_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_esp_bus_info_t busInfo = {0};
    char * currentBusSpeedString = "UNKNOWN";
    char capableBusSpeedString[255] = {0};
    
    fbe_cli_get_speed_string(io_port_info->io_port_info.link_info.link_speed, &currentBusSpeedString);
    fbe_cli_get_all_speeds_string(io_port_info->io_port_info.supportedSpeeds, capableBusSpeedString);
    
    if (io_port_info->io_port_logical_info.port_role == FBE_PORT_ROLE_BE) 
    {
        status = fbe_api_esp_get_bus_info(io_port_info->io_port_logical_info.logical_number, &busInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when getting info for bus %d, error code: %d.\n", __FUNCTION__, io_port_info->io_port_logical_info.logical_number, status);
            return;
        }

        fbe_cli_printf("%4s %3d %11s %10s  %15s  %9d  %7d  %6s\n", 
                       fbe_module_mgmt_port_role_to_string(io_port_info->io_port_logical_info.port_role), 
                       io_port_info->io_port_logical_info.logical_number, 
                       fbe_module_mgmt_convert_link_state_to_string(io_port_info->io_port_info.link_info.link_state),
                       currentBusSpeedString, 
                       capableBusSpeedString, 
                       busInfo.numOfEnclOnBus, 
                       busInfo.maxNumOfEnclOnBus, 
                       busInfo.maxNumOfEnclOnBusExceeded ? "YES" : "NO");
    }
    else
    {
        fbe_cli_printf("%4s %3d %11s %10s  %15s\n", 
                       fbe_module_mgmt_port_role_to_string(io_port_info->io_port_logical_info.port_role), 
                       io_port_info->io_port_logical_info.logical_number, 
                       fbe_module_mgmt_convert_link_state_to_string(io_port_info->io_port_info.link_info.link_state),
                       currentBusSpeedString, 
                       capableBusSpeedString);

    }
}
/****************************
 * end fbe_cli_print_businfo()
 ****************************/

 /*!**************************************************************
 * fbe_cli_print_port_info_by_role()
 ****************************************************************
 * @brief
 *      Displays all information for all ports of BE/FE roles 
 *
 *  @param :
 *      fbe_port_role_t              port_role
 *      fbe_u32_t                    which_sp
 *      fbe_u32_t                       device_type
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *  18-Jan-2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_bus_info_by_role(fbe_port_role_t port_role, fbe_u64_t device_type)
{
    fbe_u32_t                         slot_num = 0;
    fbe_u32_t                         port_num = 0;
    fbe_u32_t                         max_slots = 0;
    fbe_esp_module_io_port_info_t     io_port_info = {0};
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_status_t                      port_status = FBE_STATUS_OK;
    fbe_esp_module_limits_info_t       limits_info = {0};
    SP_ID                             sp_id = 0;
    
    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,(fbe_base_env_sp_t *)&sp_id); /* SAFEBUG */
       
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);

    if(status != FBE_STATUS_OK)
    {
        return;
    }

    
    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_slic_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_MEZZANINE)
    {
        max_slots = limits_info.discovered_hw_limit.num_mezzanine_slots;
    }
    else if(device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        max_slots = limits_info.discovered_hw_limit.num_bem;
    }


    for(slot_num = 0; slot_num <max_slots; slot_num++)
    {
        for(port_num = 0; port_num< limits_info.discovered_hw_limit.num_ports; port_num++)
        {
            io_port_info.header.sp = sp_id;
            io_port_info.header.type = device_type;
            io_port_info.header.slot = slot_num;
            io_port_info.header.port = port_num;

            port_status = fbe_api_esp_module_mgmt_getIOModulePortInfo (&io_port_info);
            if(port_status == FBE_STATUS_GENERIC_FAILURE)
            {
                 continue;
            }
            if(io_port_info.io_port_logical_info.port_role == port_role)
            {
                 fbe_cli_print_bus_info(&io_port_info);
            }
        }
    }
}

/****************************
 * end fbe_cli_lib_bus_cmd.c
 ****************************/
