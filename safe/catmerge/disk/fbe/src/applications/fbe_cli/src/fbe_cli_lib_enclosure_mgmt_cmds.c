/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_enclosure_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE CLI functions for enclosure management.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  01/17/2011:  Trupi Ghate - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_enclosure_mgmt_cmds.h"
#include "fbe/fbe_shim_flare_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe_cli_private.h"
#include "fbe_encl_mgmt_utils.h"

static void fbe_cli_print_specific_encl_info(fbe_device_physical_location_t * pLocation);
static void fbe_cli_print_encl_info_on_bus(fbe_device_physical_location_t * pLocation);
static void fbe_cli_print_all_encl_info(void);

/*************************************************************************
 *                            @fn fbe_cli_cmd_enclmgmt ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_enclmgmt performs the execution of the 'enclmgmt' command
 *   which displays the enclosure information.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   17-March-2011 Created - Trupti Ghate
 *
 *************************************************************************/
void fbe_cli_cmd_enclmgmt(fbe_s32_t argc,char ** argv)
{
    fbe_bool_t                         bus_flag = FBE_FALSE;
    fbe_bool_t                         encl_flag = FBE_FALSE;
    fbe_device_physical_location_t     location = {0};

    /*****************
    **    BEGIN    **
    *****************/

    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)))
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", ENCLMGMT_USAGE);
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
            if(location.bus >= FBE_PHYSICAL_BUS_COUNT && location.bus !=FBE_XPE_PSEUDO_BUS_NUM)
            {
                fbe_cli_printf("Invalid Bus number, it should be between 0 to %d.\n",FBE_PHYSICAL_BUS_COUNT - 1);
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
        else // invalid arguments
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", ENCLMGMT_USAGE);
            return;
        }
    }

    if(bus_flag == FBE_TRUE && encl_flag == FBE_TRUE)
    {
        // When bus # enclosure # has specified as 'enclmgmt -b # -e #'
        fbe_cli_print_specific_encl_info(&location);
    }
    else if(bus_flag == FBE_TRUE)
    {
        // When only bus # has specified as 'enclmgmt -b #'
        fbe_cli_print_encl_info_on_bus(&location);
    }
    else 
    {
        // When no options are specified in 'enclmgmt' command.
        fbe_cli_print_all_encl_info();
    }

    return;
}


/************************************************************************
 **                       End of fbe_cli_cmd_enclmgmt ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_print_specific_encl_info()
 ****************************************************************
 * @brief
 *      Prints the enclosure information.
 *
 *  @param :
 *      pLocation - The pointer to the location of the enclosures.
 *
 *  @return:
 *      None.
 *
 * @author
 *  17-March-2011:  Created. Trupti Ghate
 *
 ****************************************************************/
static void fbe_cli_print_specific_encl_info(fbe_device_physical_location_t * pLocation)
{
    fbe_esp_encl_mgmt_encl_info_t     enclInfo;
    fbe_esp_encl_mgmt_lcc_info_t    lccInfo = {0};
    fbe_esp_encl_mgmt_ssc_info_t    sscInfo = {0};
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t    slot = 0;
    char * max_encl_speed_string = "UNKNOWN";
    char * curr_encl_speed_string = "UNKNOWN";
    fbe_bool_t enclPresent = FBE_FALSE;

    if(pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM && pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM) 
    {
        fbe_cli_printf("\nXPE Info:\n");
    }
    else
    {
        fbe_cli_printf("\nBus %d Enclosure %d Info:\n", pLocation->bus, pLocation->enclosure);
    }

    status = fbe_api_esp_encl_mgmt_get_encl_presence(pLocation, &enclPresent);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl presence, status:%d\n", status);
        return;
    }

    fbe_cli_printf("  enclPresent:%s\n", enclPresent == FBE_TRUE ? "YES" : "NO");

    status = fbe_api_esp_encl_mgmt_get_encl_info(pLocation, &enclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl info, status:%d\n", status);
        return;
    }

    fbe_cli_printf("  isProcessorEncl:%s, Encl Type:%s\n",
                   (enclInfo.processorEncl == FBE_TRUE) ? "Yes" : "No",
                   fbe_encl_mgmt_get_encl_type_string(enclInfo.encl_type));

    if(enclInfo.sscCount > 0)
    {
        status = fbe_api_esp_encl_mgmt_get_ssc_info(pLocation, &sscInfo);
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("  SSC Inserted: %s, Faulted:%s\n",
                           (sscInfo.inserted == FBE_TRUE ? "Yes" : "No"),
                           (sscInfo.faulted == FBE_TRUE ? "Yes" : "No"));
        }
        else
        {
            fbe_cli_printf(" Failed to get SSC info, status %d.\n",status);
        }
    }

    fbe_cli_printf("  enclFaultLedStatus:%s\n",fbe_base_env_decode_led_status(enclInfo.enclFaultLedStatus));
    fbe_cli_printf("  enclFaultLedReason:%s\n",fbe_base_env_decode_encl_fault_led_reason(enclInfo.enclFaultLedReason));
    fbe_cli_printf("  enclState:%s, enclFaultSymptom:%s\n",
                    fbe_encl_mgmt_decode_encl_state(enclInfo.enclState),
                    fbe_encl_mgmt_decode_encl_fault_symptom(enclInfo.enclFaultSymptom));

    if(!enclInfo.processorEncl)
    {
        fbe_cli_printf("  enclDisplay Bus %c%c Enc %c\n", enclInfo.display.busNumber[0],
                   enclInfo.display.busNumber[1], enclInfo.display.enclNumber);
    }

    fbe_cli_printf("  dataStale:%s, environmentalInterfaceFault:%s\n",
                   (enclInfo.dataStale == FBE_TRUE? "Yes": "No"),
                   (enclInfo.environmentalInterfaceFault == FBE_TRUE? "Yes": "No"));

    fbe_cli_printf("  LCC Count:%d\n",enclInfo.lccCount);
    
    for(slot = 0; slot < enclInfo.lccCount; slot ++) 
    {
        pLocation->slot = slot;
        status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf(" Failed to get LCC info for LCC %d, status:%d\n", slot, status);
            continue;
        }

        fbe_cli_printf("  LCC%d Inserted:%s, Faulted:%s, lccComponentFaulted %s\n",
                       slot,
                       (lccInfo.inserted == FBE_TRUE? "Yes": "No"),
		       (lccInfo.faulted == FBE_TRUE? "Yes" : "No"),
                       (lccInfo.lccComponentFaulted == FBE_TRUE? "Yes" : "No"));
        fbe_cli_printf("        state:%s, subState:%s\n",
                       fbe_encl_mgmt_decode_lcc_state(lccInfo.state),
			           fbe_encl_mgmt_decode_lcc_subState(lccInfo.subState));
        fbe_cli_printf("        dataStale:%s, environmentalInterfaceFault:%s\n",
                       (lccInfo.dataStale == FBE_TRUE? "Yes": "No"),
			           (lccInfo.environmentalInterfaceFault == FBE_TRUE? "Yes" : "No"));
        fbe_cli_printf("        resumePromReadFailed:%s, fupFailure:%s\n",
                       (lccInfo.resumePromReadFailed == FBE_TRUE? "Yes": "No"),
			           (lccInfo.fupFailure == FBE_TRUE? "Yes" : "No"));
    }

    fbe_cli_printf("  PS Count:%d, Fan Count:%d, Drive Count:%d\n",
                   enclInfo.psCount, enclInfo.fanCount, enclInfo.driveSlotCount);
    fbe_cli_printf("  Drive Per Bank:%d, SP Count:%d, dimmCount:%d, ssdCount: %d ",
                   enclInfo.driveSlotCountPerBank, enclInfo.spCount, enclInfo.dimmCount, enclInfo.ssdCount);
    if (enclInfo.bbuCount > 0)
    {
        fbe_cli_printf("BBU Count:%d\n", enclInfo.bbuCount);
    }
    else if (enclInfo.spsCount > 0)
    {
        fbe_cli_printf("SPS Count:%d\n", enclInfo.spsCount);
    }
    else
    {
        fbe_cli_printf("\n");
    }

    fbe_cli_get_speed_string(enclInfo.maxEnclSpeed, &max_encl_speed_string);
    fbe_cli_get_speed_string(enclInfo.currentEnclSpeed, &curr_encl_speed_string);
    fbe_cli_printf("  maxEnclSpeed:%s, currentEnclSpeed:%s, crossCable:%s\n",
                    max_encl_speed_string, curr_encl_speed_string,
                   (enclInfo.crossCable == FBE_TRUE? "Yes" : "No"));
    fbe_cli_printf("  shutdownReason:%s\n",fbe_encl_mgmt_get_enclShutdownReasonString(enclInfo.shutdownReason));
    fbe_cli_printf("  overTempWarning:%s, overTempFailure:%s\n",
                   (enclInfo.overTempWarning == FBE_TRUE? "Yes": "No"),
                   (enclInfo.overTempFailure == FBE_TRUE? "Yes": "No"));
    
    fbe_cli_printf("-------------------------------------------------------------------\n");

    return;
}

/******************************************************
 * end fbe_cli_print_encl_info() 
 ******************************************************/
 
/*!**************************************************************
 * fbe_cli_print_encl_info_on_bus()
 ****************************************************************
 * @brief
 *      Prints all the enclosure's information.
 *
 *  @param :
 *      pLocation - The pointer to the location of the bus.
 *
 *  @return:
 *      None
 *
 * @author
 *  17-March-2011:  PHE - Created. 
 *
 ****************************************************************/
static void fbe_cli_print_encl_info_on_bus(fbe_device_physical_location_t * pLocation)        
{
    fbe_u32_t      encl = 0;
    fbe_u32_t      enclCount = 0;
    fbe_status_t   status = FBE_STATUS_OK;

    status = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(pLocation, &enclCount);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Enclosure count on bus %d, status: 0x%x.\n", pLocation->bus, status);
        return;
    }

    if(enclCount == 0)
    {
        fbe_cli_printf("No enclosures on bus %d\n",pLocation->bus);
    }
    else
    {
        for ( encl = 0; encl < enclCount; encl++ )
        {
            pLocation->enclosure = encl;
            fbe_cli_print_specific_encl_info(pLocation);
        }
    }

    return;
}

/*!**************************************************************
 * fbe_cli_print_all_encl_info()
 ****************************************************************
 * @brief
 *      Prints all the enclosure's information.
 *
 *  @param :
 *      None
 *
 *  @return:
 *      None
 *
 * @author
 *  17-March-2011:  Created. Trupti Ghate
 *
 ****************************************************************/
static void fbe_cli_print_all_encl_info(void)        
{
    fbe_u32_t                         bus = 0;
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_device_physical_location_t    location = {0};
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
        fbe_cli_print_specific_encl_info(&location);
    }

    if(boardInfo.platform_info.enclosureType == VPE_ENCLOSURE_TYPE) 
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        fbe_cli_print_specific_encl_info(&location);
    }

    for (bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus ++)
    {
        location.bus = bus;
        fbe_cli_print_encl_info_on_bus(&location);
    }

    return;
}

/******************************************************
 * end fbe_cli_print_all_encl_info()
******************************************************/

/*!**************************************************************
 * fbe_cli_cmd_enclmgmt_system_id_info()
 ****************************************************************
 * @brief
 *      Get and set system id info.
 *
 *  @param :
 *      pLocation - The pointer to the location of the bus.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @author
 *  17-Sep-2012:  Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_cmd_system_id(fbe_s32_t argc,char ** argv)
{
    fbe_esp_encl_mgmt_modify_system_id_info_t modify_system_info = {0};
    fbe_esp_encl_mgmt_system_id_info_t      system_info = {0};
    fbe_status_t status;
    fbe_bool_t read_sys_id = FALSE;
    fbe_bool_t write_sys_id = FALSE;

    /*****************
    **    BEGIN    **
    *****************/

    if (argc == 0 || strcmp(*argv, "-help") == 0 || strcmp(*argv, "-h") == 0)
    {
        fbe_cli_printf("%s", SYSTEMID_USAGE);
        return;
    }

    while(argc > 0)
    {
        if(strcmp(*argv, "-r") == 0)
        {
            read_sys_id = TRUE;
            break;
        }
        if(strcmp(*argv, "-umodify") == 0)
        {
            write_sys_id = TRUE;
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-sn") == 0)
        {
            argc--;
            argv++;
            if(argc <= 0)
            {
                fbe_cli_printf("Please provide the serial number.\n");
                return;
            }
            fbe_copy_memory(modify_system_info.serialNumber, *argv, RESUME_PROM_PRODUCT_SN_SIZE);
            modify_system_info.changeSerialNumber = TRUE;

            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-pn") == 0)
        {
            argc--;
            argv++;
            if(argc <= 0)
            {
                fbe_cli_printf("Please provide the part number.\n");
                return;
            }
            fbe_copy_memory(modify_system_info.partNumber, *argv, RESUME_PROM_PRODUCT_PN_SIZE);
            modify_system_info.changePartNumber = TRUE;

            argc--;
            argv++;
        }
        else
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", SYSTEMID_USAGE);
            return;
        }
    }

    if(read_sys_id)
    {
        //read and show system id info
        status = fbe_api_esp_encl_mgmt_get_system_id_info(&system_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Error when getting system id info: %d\n", status);
            return;
        }

        fbe_cli_printf("get sn: %.16s, pn: %.16s, rev: %.3s\n",
                system_info.serialNumber,
                system_info.partNumber,
                system_info.revision);
    }
    else if(write_sys_id)
    {
        fbe_cli_printf("set ");
        if(modify_system_info.changeSerialNumber)
        {
            fbe_cli_printf("%.16s ", modify_system_info.serialNumber);
        }
        if(modify_system_info.changePartNumber)
        {
            fbe_cli_printf("%.16s", modify_system_info.partNumber);
        }
        fbe_cli_printf("\n");

        status = fbe_api_esp_encl_mgmt_modify_system_id_info(&modify_system_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("error when setting system id info: %d\n", status);
            return;
        }
        fbe_cli_printf("success.\n");
    }
    else
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", SYSTEMID_USAGE);
    }

    return;
}
/******************************************************
 * end fbe_cli_cmd_enclmgmt_system_id_info()
******************************************************/

/*!**************************************************************
 * fbe_cli_cmd_wwn_seed()
 ****************************************************************
 * @brief
 *      Set, get array midplane wwn seed info and user modified
 *      wwn seed flag.
 *
 *  @param :
 *      None
 *
 *  @return:
 *      None
 *
 * @author
 *  17-Sep-2012:  Created. Dongz
 *
 ****************************************************************/
void fbe_cli_cmd_wwn_seed(int argc , char ** argv)
{
    fbe_u32_t wwn_seed = 0;
    fbe_bool_t set_flag = FALSE;
    fbe_bool_t clear_flag = FALSE;
    fbe_bool_t user_modified_wwn_seed_flag;
    fbe_status_t status;
    fbe_u8_t *wwn_seed_str = NULL;

    if(argc == 0)
    {
        status = fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(&wwn_seed);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Fail to get array midplane resume prom wwn seed!\n");
        }

        status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modified_wwn_seed_flag);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Fail to get user modified wwn seed flag!\n");
        }
        fbe_cli_printf("wwn seed 0x%x, flag %d\n", wwn_seed, user_modified_wwn_seed_flag);
        return;
    }

    if(argc == 1)
    {
        if(strcmp(*argv, "-cflag") == 0)
        {
            clear_flag = TRUE;
        }
        else
        {
            fbe_cli_printf("%s", WWN_SEED_USAGE);
            return;
        }
    }
    else if(argc == 2)
    {
        if(strcmp(*argv, "-umodify") == 0)
        {
            set_flag = TRUE;
        } else if(strcmp(*argv, "-set") == 0)
        {
            set_flag = FALSE;
        }
        else
        {
            fbe_cli_printf("%s", WWN_SEED_USAGE);
            return;
        }
        argv++;
        wwn_seed_str = *argv;
        if ((*wwn_seed_str == '0') && (*(wwn_seed_str + 1) == 'x' || *(wwn_seed_str + 1) == 'X'))
        {
            wwn_seed_str += 2;
        }
        wwn_seed = fbe_atoh(wwn_seed_str);

        if(wwn_seed == 0)
        {
            fbe_cli_printf("Invalidate wwn seed!\n");
            return;
        }
    }
    else
    {
        fbe_cli_printf("%s", WWN_SEED_USAGE);
        return;
    }
    if(clear_flag)
    {
        status = fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag();
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Fail to clear user modified wwn seed flag!\n");
        }
        return;
    }

    if(set_flag)
    {
        fbe_cli_printf("set wwn seed to 0x%x, set flag.\n", wwn_seed);
        status = fbe_api_esp_encl_mgmt_user_modify_wwn_seed(&wwn_seed);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to set wwn seed!\n");
        }
    }
    else
    {
        fbe_cli_printf("set wwn seed to 0x%x.\n", wwn_seed);
        status = fbe_api_esp_encl_mgmt_set_array_midplane_rp_wwn_seed(&wwn_seed);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("Failed to set wwn seed!\n");
        }
    }
    return;
}

