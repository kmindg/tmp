/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_cooling_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE CLI functions for cooling management.
 *
 * @ingroup fbe_cli
 *
 * @revision
 * 05-April-2011:  - created Satwik R. Dharmadhikari
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_cooling_mgmt_cmds.h"
#include "fbe_cooling_mgmt_util.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "generic_utils_lib.h"

static void fbe_cli_print_all_fan_info(void);
static void fbe_cli_print_fan_info_on_encl(fbe_device_physical_location_t * pLocation);
static void fbe_cli_print_specific_fan_info(fbe_device_physical_location_t * pLocation);

/*************************************************************************
 *                            @fn fbe_cli_cmd_coolingmgmt ()             *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_coolingmgmt performs the execution of the 'coolingmgmt' command
 *   which displays the fan information.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   05-April-2011 Created by Satwik R. Dharmadhikari
 *
 *************************************************************************/
void fbe_cli_cmd_coolingmgmt(fbe_s32_t argc,char ** argv)
{
    fbe_bool_t                        bus_flag = FBE_FALSE;
    fbe_bool_t                        encl_flag = FBE_FALSE;
    fbe_bool_t                        slot_flag = FBE_FALSE;
    fbe_device_physical_location_t    location = {0};


    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)))
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", COOLINGMGMT_USAGE);
        return;
    }
    while(argc > 0)
    {
        if((strcmp(*argv, "-b")) == 0)
        {
            argc--;
            argv++;             // move to the BUS number
            if(argc == 0)
            {
                fbe_cli_printf("Please provide bus #.\n");
                return;
            }

            location.bus = atoi(*argv);
            if(location.bus >= FBE_API_PHYSICAL_BUS_COUNT && location.bus !=FBE_XPE_PSEUDO_BUS_NUM)
            {
                fbe_cli_printf("Invalid Bus number, it should be between 0 to %d.\n",FBE_API_PHYSICAL_BUS_COUNT - 1);
                return;
            }
            bus_flag = FBE_TRUE;
            argc--;
            argv++;
        } 
        else if(strcmp(*argv, "-e") == 0)
        {
            argc--;
            argv++;     // move to the ENCLOSURE number
            if(argc == 0)
            {
                fbe_cli_printf("Please provide enclosure position #.\n");
                return;
            }
            location.enclosure = atoi(*argv);
            encl_flag = FBE_TRUE;
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-s") == 0)
        {
            argc--;
            argv++;  //move to the SLOT number
            if(argc == 0)
            {
                fbe_cli_printf("Please provide Fan Slot position #.\n");
                return;
            }
            location.slot = atoi(*argv);
            slot_flag = FBE_TRUE;
            argc--;
            argv++;
        }
        else // invalid arguments
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", COOLINGMGMT_USAGE);
            return;
        }
    }
            
	fbe_cli_printf("Stand Alone Fan Status only (no status for Fans internal to other components)\n\n");


    if(bus_flag == FBE_TRUE && encl_flag == FBE_TRUE && slot_flag == FBE_TRUE)
    {
        fbe_cli_print_specific_fan_info(&location);
    }
    else if(bus_flag == FBE_TRUE && encl_flag == FBE_TRUE)
    {
        fbe_cli_print_fan_info_on_encl(&location);
    }
    else 
    {
        fbe_cli_print_all_fan_info();
    }
   
    return;
}

/*!**************************************************************
 * fbe_cli_print_all_fan_info()
 ****************************************************************
 * @brief
 *      Prints all the fan info in the system.
 *
 *  @param :
 *      
 *
 *  @return:
 *      None.
 *
 * @author
 *  08-Aug-2011:  PHE - Created.
 ****************************************************************/
static void fbe_cli_print_all_fan_info(void)
{
    fbe_u32_t                         bus = 0;
    fbe_u32_t                         encl = 0;
    fbe_u32_t                         enclCount = 0;
    fbe_device_physical_location_t    location = {0};
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_status_t                      status = FBE_STATUS_OK;
   
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
    }

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        fbe_cli_print_fan_info_on_encl(&location);
    }

    for (bus = 0; bus < FBE_API_PHYSICAL_BUS_COUNT; bus ++)
    {
        location.bus = bus;

        status = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCount);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to get encl count on bus %d, status:%d\n", bus, status);
            continue;
        }
        
        for ( encl = 0; encl < enclCount; encl++ )
        {
            location.enclosure = encl;
            fbe_cli_print_fan_info_on_encl(&location);
        }
    }

    return;
}


/*!**************************************************************
 * fbe_cli_print_fan_info_on_encl()
 ****************************************************************
 * @brief
 *      prints fan info on the encl.
 *
 * @param pLocation - The pointer to the location of the enclosure
 *
 * @return:
 *      None.
 *
 * @author
 *  05-April-2011:  Created by Satwik R. Dharmadhikari
 ****************************************************************/
static void fbe_cli_print_fan_info_on_encl(fbe_device_physical_location_t * pLocation)
{
    
    fbe_u32_t                                slot = 0;
    fbe_esp_encl_mgmt_encl_info_t            enclInfo = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    status = fbe_api_esp_encl_mgmt_get_encl_info(pLocation, &enclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl Info, status:%d\n", status);
        return;
    }
 
    if(enclInfo.fanCount == 0) 
    {
        if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
        {
            fbe_cli_printf("No Fan in xPE\n");
        }
        else
        {
            fbe_cli_printf("No Fan in Bus %d Encl %d\n", pLocation->bus, pLocation->enclosure);
        }
    }
    else
    {
         for(slot = 0; slot < enclInfo.fanCount; slot ++)
        {
            pLocation->slot = slot;
    
            fbe_cli_print_specific_fan_info(pLocation);
        } 
    }

    return;
}

/*!**************************************************************
 * fbe_cli_print_specific_fan_info()
 ****************************************************************
 * @brief
 *      Prints the individual fan information.
 *
 * @param pLocation - The pointer to the location of the fan
 *
 * @return:
 *      None.
 *
 * @author
 *  05-April-2011:  Created by Satwik R. Dharmadhikari
 ****************************************************************/
static void fbe_cli_print_specific_fan_info(fbe_device_physical_location_t * pLocation)
{
    fbe_esp_cooling_mgmt_fan_info_t    fanInfo = {0};
    fbe_status_t                       status = FBE_STATUS_OK;

    if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM) 
    {
        fbe_cli_printf("\nXPE Fan Slot %d Info:\n", pLocation->slot);
    }
    else
    {
        fbe_cli_printf("\nBus %d Enclosure %d Fan Slot %d Info:\n", 
                       pLocation->bus, pLocation->enclosure, pLocation->slot);
       
    }

    status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);
    if (status != FBE_STATUS_OK)
	{
		fbe_cli_printf("  Failed to get fan info,status:%d\n", status);
    }
	else
	{
	    fbe_cli_printf("  InProcessorEnclosure\t\t : %s\n",(fanInfo.inProcessorEncl == FBE_TRUE ? "Yes" : "No"));
        fbe_cli_printf("  AssociatedSp\t\t\t : %s\n",(fanInfo.associatedSp == SP_A)? "SP_A" : ((fanInfo.associatedSp == SP_B)?"SP_B":"N/A"));
        if (fanInfo.associatedSp == SP_A || fanInfo.associatedSp == SP_B)
        {
            fbe_cli_printf("  SlotNumOnSpBlade\t\t : %d\n", fanInfo.slotNumOnSpBlade);
        }
        else
        {
            fbe_cli_printf("  SlotNumOnSpBlade\t\t : N/A\n");
        }
        fbe_cli_printf("  Environment Interface Fault\t : %s\n", fbe_base_env_decode_env_status(fanInfo.envInterfaceStatus));
        fbe_cli_printf("  Fan Inserted\t\t\t : %s\n", fbe_base_env_decode_status(fanInfo.inserted));
        switch (fanInfo.fanLocation)
        {
        case FBE_FAN_LOCATION_FRONT:
            fbe_cli_printf("  Fan Location\t\t\t : Front\n"); 
            break;
        case FBE_FAN_LOCATION_BACK:
            fbe_cli_printf("  Fan Location\t\t\t : Back\n"); 
            break;
        case FBE_FAN_LOCATION_SP:
            fbe_cli_printf("  Fan Location\t\t\t : SP Suitcase\n"); 
            break;
        default:
            fbe_cli_printf("  Fan Location\t\t\t : N/A-Unknown\n"); 
            break;
        }
        fbe_cli_printf("  Fan State\t\t\t : %s\n", fbe_cooling_mgmt_decode_fan_state(fanInfo.state));
        fbe_cli_printf("  Fan SubState\t\t\t : %s\n", fbe_cooling_mgmt_decode_fan_subState(fanInfo.subState));
        fbe_cli_printf("  Fan UniqueID\t\t\t : %s\n", decodeFamilyFruID(fanInfo.uniqueId));
        fbe_cli_printf("  Fan Fault Status\t\t : %s\n", fbe_base_env_decode_status(fanInfo.fanFaulted));
        fbe_cli_printf("  Fan Degraded Status\t\t : %s\n", fbe_base_env_decode_status(fanInfo.fanDegraded));
        fbe_cli_printf("  MultipleFanModuleFault Status\t : %s\n", fbe_base_env_decode_status(fanInfo.multiFanModuleFaulted));
        fbe_cli_printf("  Resume PROM supported\t\t : %s\n", fbe_base_env_decode_status(fanInfo.resumePromSupported));
        fbe_cli_printf("  Firmware Updatable\t\t : %s\n", fbe_base_env_decode_status(fanInfo.fwUpdatable));
        fbe_cli_printf("  Firmware Revision\t\t : %s\n", fanInfo.firmwareRev);
        fbe_cli_printf("  Data Stale\t\t\t : %s\n", fbe_base_env_decode_status(fanInfo.dataStale));
        fbe_cli_printf("  Env Intf Fault\t\t : %s\n", fbe_base_env_decode_status(fanInfo.environmentalInterfaceFault));
        fbe_cli_printf("  Resum Prom Read Fail\t\t : %s\n", fbe_base_env_decode_status(fanInfo.resumePromReadFailed));
        fbe_cli_printf("  FW Update Failure\t\t : %s\n", fbe_base_env_decode_status(fanInfo.fupFailure));
        fbe_cli_printf("  FaultReg Fault\t\t : %s\n\n", fanInfo.isFaultRegFail?"YES" : "NO");
    }

    fbe_cli_printf("-------------------------------------------------------------------\n");
    return;
}


/*End of file fbe_cli_cooling_mgmt_cmds.c*/
