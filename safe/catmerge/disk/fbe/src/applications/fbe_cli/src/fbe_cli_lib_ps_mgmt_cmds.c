/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_ps_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for info related to Power Suppy
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  02/25/2011:  Rashmi Sawale - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_ps_mgmt_cmds.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "generic_utils_lib.h"

/*************************************************************************
 *          @fn fbe_cli_cmd_psmgmt ()                                    *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_psmgmt performs the execution of the PSMGMT command
 *   Power Suppy Mgmt - Displays power supply status
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   03/17/2011 Created - Rashmi Sawale
 *
 *************************************************************************/

void fbe_cli_cmd_psmgmt(fbe_s32_t argc,char ** argv)
{
    /****************************
    **    VARIABLE DECLARATION   **
    ****************************/
    fbe_bool_t                        bus_flag = FBE_FALSE;
    fbe_bool_t                        encl_flag = FBE_FALSE;
    fbe_bool_t                        slot_flag = FBE_FALSE;
    fbe_device_physical_location_t    location = {0};
    fbe_status_t                      status;

    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)))
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", PSMGMT_USAGE);
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
                fbe_cli_printf("Please provide Slot position #.\n");
                return;
            }
            location.slot = atoi(*argv);
            if(!fbe_cli_verify_ps_slot_num_on_encl(location.bus, location.enclosure, location.slot))
            {
               return;
            }
            slot_flag = FBE_TRUE;
            argc--;
            argv++;
        }
        else if(strcmp(*argv, "-set_expected_ps_type") == 0)
        {
            argc--;
            argv++;  //move to the next argument
            if(argc == 0)
            {
                fbe_cli_printf("Please provide Expected PS Type Octane/Other/None.\n");
                return;
            }
            if(strcmp(*argv, "Octane") == 0)
            {
                status = fbe_api_esp_ps_mgmt_set_expected_ps_type(FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE);
                if(status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("Successfully set expected PS type to Octane.\n");
                    return;
                }
                else
                {
                    fbe_cli_printf("Failed to set expected PS type to Octane status 0x%x.\n", status);
                    return;
                }
            }
            else if(strcmp(*argv, "Other") == 0)
            {
                status = fbe_api_esp_ps_mgmt_set_expected_ps_type(FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER);
                if(status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("Successfully set expected PS type to Other.\n");
                    return;
                }
                else
                {
                    fbe_cli_printf("Failed to set expected PS type to Other status 0x%x.\n", status);
                    return;
                }
            }
            else if(strcmp(*argv, "None") == 0)
            {
                status = fbe_api_esp_ps_mgmt_set_expected_ps_type(FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE);
                if(status == FBE_STATUS_OK)
                {
                    fbe_cli_printf("Successfully set expected PS type to None.\n");
                    return;
                }
                else
                {
                    fbe_cli_printf("Failed to set expected PS type to None status 0x%x.\n", status);
                    return;
                }
            }
            else
            {
                fbe_cli_printf("Please provide Expected PS Type Octane/Other/None.\n");
                return;
            }
                
            
        }
        else // invalid arguments
        {
            fbe_cli_printf("Invalid arguments. Please check the command. \n");
            fbe_cli_printf("%s", PSMGMT_USAGE);
            return;
        }
    }

    if(bus_flag == FBE_TRUE && encl_flag == FBE_TRUE && slot_flag == FBE_TRUE)
    {
        fbe_cli_print_specific_PS_status(location.bus, location.enclosure, location.slot);
    }
    else if(bus_flag == FBE_TRUE && encl_flag == FBE_TRUE)
    {
        fbe_cli_print_all_PS_slots_status_on_encl(location.bus, location.enclosure);
    }
    else if(bus_flag == FBE_TRUE)
    {
        fbe_cli_print_all_PS_status_on_bus(location.bus);
    }
    else 
    {
       fbe_cli_print_all_PS_status();
    }

    return;
}

/************************************************************************
 **                       End of fbe_cli_cmd_psmgmt ()                 **
 ************************************************************************/

/*!*************************************************************
 * fbe_cli_print_specific_PS_status()
 ****************************************************************
 * @brief
 *      Displays PS status of specific component.
 *
 *  @param :
 *      fbe_u8_t                  bus_num
 *      fbe_u8_t                  encl_num
 *      fbe_u8_t                  slot_num
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_specific_PS_status(fbe_u8_t bus_num, fbe_u8_t encl_num ,fbe_u8_t slot_num)
{
    fbe_status_t                      psStatus = FBE_STATUS_OK;
    fbe_esp_ps_mgmt_ps_info_t         psInfo;
    fbe_u32_t                         revIndex; 
    
    if(bus_num == FBE_XPE_PSEUDO_BUS_NUM && encl_num == FBE_XPE_PSEUDO_ENCL_NUM) 
    {
        fbe_cli_printf("\nXPE PS Slot %d Info:\n", slot_num);
    }
    else
    {
        fbe_cli_printf("\nBus %d Enclosure %d PS Slot %d Info:\n", bus_num, encl_num, slot_num);
    }

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    
    psInfo.location.bus = bus_num;
    psInfo.location.enclosure = encl_num;
    psInfo.location.slot = slot_num;

    psStatus = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    if(psStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get PS Info, status %d\n", psStatus);
    }
    else
    {
        fbe_cli_printf("  Slot on SP Blade        :    %d\n", psInfo.psInfo.slotNumOnSpBlade);
        fbe_cli_printf("  Associated SP           :    %s(%d)\n", fbe_cli_lib_SP_ID_to_string(psInfo.psInfo.associatedSp),psInfo.psInfo.associatedSp);
        fbe_cli_printf("  State                   :    %s\n", fbe_ps_mgmt_decode_ps_state(psInfo.psInfo.state));
        fbe_cli_printf("  SubState                :    %s\n", fbe_ps_mgmt_decode_ps_subState(psInfo.psInfo.subState));
        fbe_cli_printf("  ENV Interface Status    :    %s\n", fbe_base_env_decode_env_status(psInfo.psInfo.envInterfaceStatus));
        fbe_cli_printf("  inProcessorEncl         :    %s\n",(psInfo.psInfo.inProcessorEncl ? "YES" : "NO"));
        fbe_cli_printf("  isLocalFru              :    %s\n",(psInfo.psInfo.isLocalFru ? "YES" : "NO"));
        fbe_cli_printf("  Associated SPS          :    %s\n", (psInfo.psInfo.associatedSps ? "SPS_B" : "SPS_A"));
        fbe_cli_printf("  Inserted                :    %s\n",(psInfo.psInfo.bInserted ? "YES": "NO"));
        fbe_cli_printf("  Logical Fault           :    %s\n",(psInfo.psLogicalFault ? "YES": "NO"));
        fbe_cli_printf("  General Fault           :    %s\n", fbe_base_env_decode_status(psInfo.psInfo.generalFault));
        fbe_cli_printf("  InternalFanFault        :    %s\n", fbe_base_env_decode_status(psInfo.psInfo.internalFanFault));
        fbe_cli_printf("  Ambient Overtemp Fault  :    %s\n", fbe_base_env_decode_status(psInfo.psInfo.ambientOvertempFault));
        fbe_cli_printf("  DC Present              :    %s\n", fbe_base_env_decode_status(psInfo.psInfo.DCPresent));
        fbe_cli_printf("  AC FAIL                 :    %s\n", fbe_base_env_decode_status(psInfo.psInfo.ACFail));
        fbe_cli_printf("  AC DC Input             :    %s\n",fbe_ps_mgmt_decode_ACDC_input(psInfo.psInfo.ACDCInput));
        fbe_cli_printf("  Input Power Status      :    0x%x\n",psInfo.psInfo.inputPowerStatus);
        fbe_cli_printf("  Input Power             :    %d\n",psInfo.psInfo.inputPower);
        fbe_cli_printf("  PS Input Power          :    %d\n",psInfo.psInfo.psInputPower);
        fbe_cli_printf("  PS Margin Test Results  :    %d\n",psInfo.psInfo.psMarginTestResults);

        fbe_cli_printf("  Firmware Rev Valid      :    %s\n",(psInfo.psInfo.firmwareRevValid ? "YES" : "NO"));
        fbe_cli_printf("  Downloadable            :    %s\n",(psInfo.psInfo.bDownloadable ? "YES" : "NO"));
        fbe_cli_printf("  Firmware Revision       :    %s\n", psInfo.psInfo.firmwareRev);

        for(revIndex = 0; revIndex < FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS; revIndex ++ ) 
        {
            if(psInfo.psInfo.psRevInfo[revIndex].firmwareRevValid) 
            {
                fbe_cli_printf("  Detailed Programmable Info  :    %d\n", revIndex);
                fbe_cli_printf("      Firmware Rev Valid      :    %s\n",(psInfo.psInfo.psRevInfo[revIndex].firmwareRevValid ? "YES" : "NO"));
                fbe_cli_printf("      Downloadable            :    %s\n",(psInfo.psInfo.psRevInfo[revIndex].bDownloadable ? "YES" : "NO"));
                fbe_cli_printf("      Firmware Revision       :    %s\n", psInfo.psInfo.psRevInfo[revIndex].firmwareRev);
            }
        }
        
        fbe_cli_printf("  Sub Encl Product ID     :    %s\n",psInfo.psInfo.subenclProductId);
        fbe_cli_printf("  PS uniqueId             :    0x%X(%s)\n", psInfo.psInfo.uniqueId, decodeFamilyFruID(psInfo.psInfo.uniqueId));
        fbe_cli_printf("  uniqueIDFinal           :    %s\n", (psInfo.psInfo.uniqueIDFinal ? "YES" : "NO"));
        fbe_cli_printf("  PS Supported            :    %s\n", (psInfo.psInfo.psSupported ? "YES" : "NO"));
        fbe_cli_printf("  Data Stale              :    %s\n", (psInfo.dataStale ? "YES" : "NO"));
        fbe_cli_printf("  Env Intf Failure        :    %s\n", (psInfo.environmentalInterfaceFault ? "YES" : "NO"));
        fbe_cli_printf("  Res PROM Read Failure   :    %s\n", (psInfo.psInfo.resumePromReadFailed ? "YES" : "NO"));
        fbe_cli_printf("  FW Update Failure       :    %s\n", (psInfo.psInfo.fupFailure ? "YES" : "NO"));
        fbe_cli_printf("  FaultReg Fault          :    %s\n", (psInfo.psInfo.isFaultRegFail ? "YES" : "NO"));
        fbe_cli_printf("  Expected PE PS Type     :    %s\n", fbe_ps_mgmt_decode_expected_ps_type(psInfo.expected_ps_type));
        fbe_cli_printf("-------------------------------------------------------------------\n");   
    }
    
    return; 
}
/******************************************************
 * end fbe_cli_print_specific_PS_status() 
 ******************************************************/


/*!**************************************************************
 * fbe_cli_print_all_PS_slots_status_on_encl()
 ****************************************************************
 * @brief
 *      Displays PS status of all PS slots on specified enclosure.
 *
 *  @param :
 *      fbe_u8_t                  bus_num
 *      fbe_u8_t                  encl_num
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_PS_slots_status_on_encl(fbe_u8_t bus_num, fbe_u8_t encl_num)
{
    fbe_status_t                      psCntStatus = FBE_STATUS_OK;
    fbe_u32_t                         psCount;
    fbe_u32_t                         ps;
    fbe_device_physical_location_t    location = {0};
    fbe_esp_ps_mgmt_ps_info_t         psInfo;

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));

    location.bus = bus_num;
    location.enclosure = encl_num;

    psCntStatus = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    if(psCntStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get PS CNT for bus: %d, encl: %d\n", 
           bus_num, encl_num);
        return;    
    }

    if(psCount == 0) 
    {
        if(bus_num == FBE_XPE_PSEUDO_BUS_NUM && encl_num == FBE_XPE_PSEUDO_ENCL_NUM) 
        {
            fbe_cli_printf("No PS in xPE\n");
        }
        else
        {
            fbe_cli_printf("No PS in bus: %d, encl: %d\n", bus_num, encl_num);
        }
    }
    else
    {
        for(ps = 0; ps<psCount; ps++)
        {
            fbe_cli_print_specific_PS_status(bus_num, encl_num, ps);
        }
    }
    return;
}
/******************************************************
 * end fbe_cli_print_all_PS_slots_status_on_encl() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_PS_status_on_bus()
 ****************************************************************
 * @brief
 *      Displays PS status of all PS slots on all Encl on specified bus.
 *
 *  @param :
 *      fbe_u8_t                  bus_num
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_PS_status_on_bus(fbe_u8_t bus_num)
{
    fbe_status_t                      enclCntStatus = FBE_STATUS_OK;
    fbe_u32_t                         enclCntOnBus = 0;
    fbe_u32_t                         encl_num;
    fbe_device_physical_location_t    location = {0};

    location.bus = bus_num;

    enclCntStatus = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCntOnBus);
    if(enclCntStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Enclosure count on bus, status: 0x%x.\n", enclCntStatus);
        return;    
    }
	
    if(enclCntOnBus == 0)
    {
        fbe_cli_printf("No Enclosure on bus %d\n",bus_num);
        return;
    }
	 
    for(encl_num = 0; encl_num<enclCntOnBus; encl_num++)
    {
        fbe_cli_print_all_PS_slots_status_on_encl(bus_num, encl_num);
    }

    return;
}
/******************************************************
 * end fbe_cli_print_all_PS_status_on_bus() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_all_PS_status()
 ****************************************************************
 * @brief
 *      Displays PS status of all PS slots on all Encl on all buses.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_print_all_PS_status(void)
{
    fbe_u32_t                         bus_num;
    fbe_u32_t                         encl_num;
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_status_t                      status = FBE_STATUS_OK;
   
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
    }

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        bus_num = FBE_XPE_PSEUDO_BUS_NUM;
        encl_num = FBE_XPE_PSEUDO_ENCL_NUM;
        fbe_cli_print_all_PS_slots_status_on_encl(bus_num, encl_num);
    }

    for(bus_num = 0; bus_num < FBE_API_PHYSICAL_BUS_COUNT; bus_num++)
    {
        fbe_cli_print_all_PS_status_on_bus(bus_num);
    }

    return;
}
/******************************************************
 * end fbe_cli_print_all_PS_status() 
 ******************************************************/
 

/*!**************************************************************
 * fbe_cli_verify_encl_num_on_bus()
 ****************************************************************
 * @brief
 *      Verifies encl num on bus(valid/invalid).
 *
 *  @param :
 *      fbe_u8_t   bus_num
 *      fbe_u8_t   encl_num
 *
 *  @return:
 *      TRUE or FALSE
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static BOOL fbe_cli_verify_encl_num_on_bus(fbe_u8_t bus_num, fbe_u8_t encl_num)
{
    fbe_status_t                      enclCntStatus = FBE_STATUS_OK;
    fbe_u32_t                         enclCntOnBus = 0;
    fbe_device_physical_location_t    location = {0};

    location.bus = bus_num;

    enclCntStatus = fbe_api_esp_encl_mgmt_get_encl_count_on_bus(&location, &enclCntOnBus);
    if(enclCntStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Enclosure count on bus, status: 0x%x.\n", enclCntStatus);
        return FALSE;    
    }

    if(enclCntOnBus == 0)
    {
        fbe_cli_printf("No Enclosure on bus %d\n\n",bus_num);
        return FALSE;
    }
    else if(encl_num > enclCntOnBus-1)
    {
         fbe_cli_printf("Please enter enclosure number between 0 to %d for bus %d(encl cnt: %d)\n\n",enclCntOnBus-1, bus_num,enclCntOnBus);
         return FALSE;
    }
    else
    {
        return TRUE;
    }
}
/******************************************************
 * end fbe_cli_verify_encl_num_on_bus() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_verify_ps_slot_num_on_encl()
 ****************************************************************
 * @brief
 *      Verifies ps slot num on encl(valid/invalid).
 *
 *  @param :
 *      fbe_u8_t   bus_num
 *      fbe_u8_t   encl_num
 *      fbe_u8_t   slot_num
 *
 *  @return:
 *      TRUE or FALSE
 *
 * @author
 *  03/17/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static BOOL fbe_cli_verify_ps_slot_num_on_encl(fbe_u8_t bus_num, fbe_u8_t encl_num, fbe_u8_t slot_num)
{
    fbe_status_t                      psCntStatus = FBE_STATUS_OK;
    fbe_u32_t                         psCount;
    fbe_device_physical_location_t    location = {0};

    location.bus = bus_num;
    location.enclosure = encl_num;

    psCntStatus = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    if(psCntStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get PS CNT for bus: %d, encl: %d\n", bus_num, encl_num);
        return FALSE;    
    }


    if(slot_num > psCount-1)
    {
         fbe_cli_printf("Please enter PS slot number between 0 to %d for bus %d and encl %d(PS cnt: %d)\n\n",psCount-1,bus_num,encl_num,psCount);
         return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/******************************************************
 * end fbe_cli_verify_ps_slot_num_on_encl() 
 ******************************************************/

