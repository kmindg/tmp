/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_sp_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the spstat(sp) command.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   12/31/2011:  dongz - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <string.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_api_transport.h"

#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe_api_esp_sps_mgmt_interface.h"
#include "fbe_api_esp_ps_mgmt_interface.h"
#include "fbe_api_esp_board_mgmt_interface.h"
#include "fbe_cli_lib_board_mgmt_cmds.h"
#include "fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe_api_esp_drive_mgmt_interface.h"
#include "fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_api_enclosure_interface.h"
#include "fbe_cli_sp_cmd.h"
#include "fbe_encl_mgmt_utils.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_cmi.h"


/**************************************************************************
*  slot stat title print format talbe
**************************************************************************/
drive_print_format_t drive_print_format_table[] = {
        {FBE_ESP_ENCL_TYPE_VOYAGER, 12},
        {FBE_ESP_ENCL_TYPE_VIKING, 20},
        {FBE_ESP_ENCL_TYPE_CAYENNE, 12},
        {FBE_ESP_ENCL_TYPE_NAGA, 20},
        {FBE_ESP_ENCL_TYPE_UNKNOWN, 0}
};

/*!***************************************************************
 * @fn fbe_cli_display_hw_cache_status()
 ****************************************************************
 * @brief
 *  This function get and display the hw cache info.
 *  output format:
 *    HW Cache Status: OK/FAILED/DEGRADED
 *    Reason: NA/PS/SPS/FAN/SUITCASE/ENCL
 *
 *    If the status is OK. Put NA as the Reason. If the status is Degraded or Failed,
 *    call fbe_api_esp_sps_mgmt_getCacheStatus, if it is failed or degraded, put SPS
 *    call fbe_api_esp_sps_mgmt_getPsCacheStatus, if it is failed or degraded, put PS
 *    call fbe_api_esp_encl_mgmt_getCacheStatus, if it is failed or degraded, put ENCL
 *    call fbe_api_esp_board_mgmt_getCacheStatus, if it is failed or degraded, put SUITCASE
 *    call fbe_api_esp_cooling_mgmt_getCacheStatus, if it is failed or degraded, put FAN
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_hw_cache_status(void)
{
    fbe_common_cache_status_info_t  espCacheStatusInfo;
    fbe_common_cache_status_t cache_status;
    fbe_status_t status;

    //display hw cache status
    status = fbe_api_esp_common_getCacheStatus(&espCacheStatusInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get hw cache status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    fbe_cli_printf("HW Cache Status: %s  ", fbe_cli_print_HwCacheStatus(espCacheStatusInfo.cacheStatus));
    fbe_cli_printf("Reason:");

    switch(espCacheStatusInfo.cacheStatus)
    {
        case FBE_CACHE_STATUS_OK:
            fbe_cli_printf(" NA\n");
            break;

        default:
            status = fbe_api_esp_sps_mgmt_getCacheStatus(&cache_status);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get sps cache status, error code: %d.\n", __FUNCTION__, status);
                return;
            }
            if(cache_status == espCacheStatusInfo.cacheStatus)
                fbe_cli_printf(" SPS/BBU");
            status = fbe_api_esp_ps_mgmt_getCacheStatus(&cache_status);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get ps cache status, error code: %d.\n", __FUNCTION__, status);
                return;
            }
            if(cache_status == espCacheStatusInfo.cacheStatus)
            {
                fbe_cli_printf(" PS");
            }
            status = fbe_api_esp_encl_mgmt_getCacheStatus(&cache_status);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get encl cache status, error code: %d.\n", __FUNCTION__, status);
                return;
            }
            if(cache_status == espCacheStatusInfo.cacheStatus)
            {
                fbe_cli_printf(" ENCL");
            }
            status = fbe_api_esp_board_mgmt_getCacheStatus(&cache_status);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get suitcase cache status, error code: %d.\n", __FUNCTION__, status);
                return;
            }
            if(cache_status == espCacheStatusInfo.cacheStatus)
            {
                fbe_cli_printf(" SUITCASE");
            }
            status = fbe_api_esp_cooling_mgmt_getCacheStatus(&cache_status);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get cool cache status, error code: %d.\n", __FUNCTION__, status);
                return;
            }
            if(cache_status == espCacheStatusInfo.cacheStatus)
            {
                fbe_cli_printf(" FAN");
            }
    }
    fbe_cli_printf("\n");
}

/*!***************************************************************
 * @fn fbe_cli_display_sp_physical_memory(fbe_esp_board_mgmt_board_info_t *board_info)
 ****************************************************************
 * @brief
 *  This function get and display physical memory on all SP blades.
 *  output format:
 *    Physical memory: SPA(x), SPB(x)
 *
 * @param
 *    board_info
 *
 * @return
 *    None
 *
 * @version
 *  18-July-2012: Chengkai Hu - Created.
 *
 ****************************************************************/
static void fbe_cli_display_sp_physical_memory(fbe_esp_board_mgmt_board_info_t *board_info)
{
    if (board_info->isSingleSpSystem)
    {
        fbe_cli_printf("Physical memory: SPA(%lu MB)\n", 
            (unsigned long)board_info->spPhysicalMemorySize[SP_A]);
    }
    else
    {
        fbe_cli_printf("Physical memory: SPA(%lu MB), SPB(%lu MB)\n", 
            (unsigned long)board_info->spPhysicalMemorySize[SP_A],
            (unsigned long)board_info->spPhysicalMemorySize[SP_B]);
    }
}

/*!***************************************************************
 * @fn fbe_cli_display_sps_status(SP_ID target_sp, SP_ID current_sp)
 ****************************************************************
 * @brief
 *  This function get and display the sps status info.
 *  output format:
 *    SPS A:  (1.2KW) OK
 *    SPS B:  (1.2KW) OK
 *
 * @param
 *    target_sp the sps info we need to oouput is on target_sp
 *    current_sp fbecli is run on current_sp
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_sps_status(fbe_device_physical_location_t *spsLocation)
{
    fbe_status_t status;
    fbe_esp_sps_mgmt_spsStatus_t sps_status_info;
    fbe_esp_sps_mgmt_spsManufInfo_t sps_manuf_info;
    fbe_bool_t simulateSps;

    fbe_set_memory(&sps_status_info, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    fbe_set_memory(&sps_manuf_info, 0, sizeof(fbe_esp_sps_mgmt_spsManufInfo_t));
    sps_manuf_info.spsLocation = sps_status_info.spsLocation  = *spsLocation;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&sps_status_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get sps status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    status = fbe_api_esp_sps_mgmt_getManufInfo(&sps_manuf_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get sps manufacturing status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    status = fbe_api_esp_sps_mgmt_getSimulateSps(&simulateSps);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get simulateSps status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    if (sps_status_info.spsModuleInserted)
    {
        if (sps_status_info.spsBatteryInserted)
        {
            fbe_cli_printf("SPS%s (%s) status: %s, cabling: %s, faults: Mod %s, Batt %s, %s",
                           spsLocation->slot == SP_A?"A":"B",
                           sps_manuf_info.spsManufInfo.spsModuleManufInfo.spsModelString,
                           decodeSpsState(sps_status_info.spsStatus),
                           fbe_cli_print_SpsCablingStatus(sps_status_info.spsCablingStatus),
                           sps_status_info.spsFaultInfo.spsModuleFault ? "FLT" : "OK",
                           sps_status_info.spsFaultInfo.spsBatteryFault ? "FLT" : "OK",
                           fbe_cli_print_SpsFaults(&sps_status_info.spsFaultInfo));
        }
        else
        {
            fbe_cli_printf("SPS%s (%s) : Battery Not Present",
                    spsLocation->slot == SP_A?"A":"B",
                    sps_manuf_info.spsManufInfo.spsModuleManufInfo.spsModelString);
        }
    }
    else
    {
        fbe_cli_printf("SPS%s (%s) status: %s",
                spsLocation->slot == SP_A?"A":"B",
                "",
                "Not Present");
    }

    if (simulateSps)
    {
        fbe_cli_printf(" , SIMULATED SPS\n");
    }
    else
    {
        fbe_cli_printf("\n");
    }
}

void fbe_cli_displayBbuStatus(fbe_u32_t bbuCount)
{
    fbe_u32_t                       bbuIndex;
    fbe_status_t                    status;
    fbe_esp_sps_mgmt_bobStatus_t    bbu_status_info;

    fbe_cli_printf("  BBU\t");
    for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
    {
        if (bbuIndex < (bbuCount/2))
        {
            fbe_cli_printf("A%d\t\t", bbuIndex); 
        }
        else
        {
            fbe_cli_printf("B%d\t\t", (bbuIndex - (bbuCount/2))); 
        }
    }

    fbe_cli_printf("\n\t");
    for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
    {
        fbe_set_memory(&bbu_status_info, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t)); 
        bbu_status_info.bobIndex = bbuIndex;
        status = fbe_api_esp_sps_mgmt_getBobStatus(&bbu_status_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error in get bbu status, error code: %d\n", __FUNCTION__, status);
            return;
        }

        if (!bbu_status_info.bobInfo.batteryInserted)
        {
            fbe_cli_printf("---\t");
        }
        else if (bbu_status_info.bobInfo.batteryFaults == FBE_BATTERY_FAULT_NO_FAULT)
        {
            fbe_cli_printf("OK(%d.%d)\t",
                           bbu_status_info.bobInfo.firmwareRevMajor,
                           bbu_status_info.bobInfo.firmwareRevMinor);
       }
        else
        {
            fbe_cli_printf("FLT(%d.%d)\t",
                           bbu_status_info.bobInfo.firmwareRevMajor,
                           bbu_status_info.bobInfo.firmwareRevMinor);
        }
    }

    fbe_cli_printf("\n");

}   // end of fbe_cli_displayBbuStatus

/*!***************************************************************
 * @fn fbe_cli_display_bbu_status(SP_ID target_sp, SP_ID current_sp)
 ****************************************************************
 * @brief
 *  This function get and display the BBU status info.
 *
 * @param
 *    target_sp the sps info we need to oouput is on target_sp
 *    current_sp fbecli is run on current_sp
 *
 * @return
 *  None
 *
 * @version
 *  28-Jun-2012:    Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_display_bbu_status(fbe_u32_t bbuIndex,
                                fbe_u32_t bbuCount)
{
    fbe_status_t                    status;
    fbe_esp_sps_mgmt_bobStatus_t    bbu_status_info;
    fbe_bool_t                      simulateSps;
    fbe_u32_t                       bbuSideIndex;

    fbe_set_memory(&bbu_status_info, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    bbu_status_info.bobIndex = bbuIndex;
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bbu_status_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get bbu status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    status = fbe_api_esp_sps_mgmt_getSimulateSps(&simulateSps);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get simulateSps status, error code: %d\n", __FUNCTION__, status);
        return;
    }

    if (bbu_status_info.bobInfo.associatedSp == SP_A)
    {
        bbuSideIndex = bbuIndex;
    }
    else
    {
        bbuSideIndex = bbuIndex - (bbuCount/2);
    }
    if(bbu_status_info.bobInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        fbe_cli_printf("BBU%s%d inserted %d, state: %s, faultEnvStatus:%s",
                       (bbu_status_info.bobInfo.associatedSp == SP_A ? "A":"B"),
                       bbuSideIndex,
                       bbu_status_info.bobInfo.batteryInserted,
                       bbu_status_info.bobInfo.batteryInserted ?
                       fbe_sps_mgmt_getBobStateString(bbu_status_info.bobInfo.batteryState) : "N/A",
                       fbe_base_env_decode_env_status(bbu_status_info.bobInfo.envInterfaceStatus));
    }
    else
    {
        fbe_cli_printf("BBU%s%d inserted %d, state: %s, fault:%s",
                       (bbu_status_info.bobInfo.associatedSp == SP_A ? "A":"B"),
                       bbuSideIndex,
                       bbu_status_info.bobInfo.batteryInserted,
                       bbu_status_info.bobInfo.batteryInserted ?
                       fbe_sps_mgmt_getBobStateString(bbu_status_info.bobInfo.batteryState) : "N/A",
                       bbu_status_info.bobInfo.batteryInserted ?
                       fbe_sps_mgmt_getBobFaultString(bbu_status_info.bobInfo.batteryFaults) : "N/A");
    }

    if (simulateSps)
    {
        fbe_cli_printf(" , SIMULATED BBU\n");
    }
    else
    {
        fbe_cli_printf("\n");
    }
}

/*!***************************************************************
 * @fn fbe_cli_display_management_module_info(SP_ID target_sp)
 ****************************************************************
 * @brief
 *  This function get and display the DPE/XPE mgmt info.
 *
 * @param
 *    target_sp - SPA or SPB
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_get_management_module_info(SP_ID target_sp, char *buffer)
{
    fbe_esp_module_limits_info_t limits_info;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t mgmt_comp_info;
    fbe_u32_t slot;
    fbe_status_t status;
    char *buffer_ptr;

    fbe_set_memory(&limits_info, 0, sizeof(fbe_esp_module_limits_info_t));
    status = fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get limits info, error code: %d\n", __FUNCTION__, status);
        return;
    }

    for(slot = 0; slot < limits_info.discovered_hw_limit.num_mgmt_modules; slot++)
    {
        fbe_set_memory(&mgmt_comp_info, 0, sizeof(fbe_esp_module_mgmt_get_mgmt_comp_info_t));
        mgmt_comp_info.phys_location.sp = target_sp;
        mgmt_comp_info.phys_location.slot = slot;

        status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: ERROR in get mgmt comp info, error code: %d.\n", __FUNCTION__, status);
            return;
        }
        if(!mgmt_comp_info.mgmt_module_comp_info.bInserted)
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: Removed\t", decodeSpId(target_sp));
        }
        else if(mgmt_comp_info.mgmt_module_comp_info.generalFault == FBE_MGMT_STATUS_TRUE)
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: FLT\t", decodeSpId(target_sp));
        }
        else if(mgmt_comp_info.mgmt_module_comp_info.generalFault == FBE_MGMT_STATUS_NA)
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: NA\t", decodeSpId(target_sp));
        }
        else if(mgmt_comp_info.mgmt_module_comp_info.generalFault == FBE_MGMT_STATUS_FALSE)
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: OK\t", decodeSpId(target_sp));
        }
        else if(mgmt_comp_info.mgmt_module_comp_info.isFaultRegFail == FBE_TRUE)
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: FaultReg Fault\t", decodeSpId(target_sp));
        }
        else
        {
            buffer_ptr = buffer + strlen(buffer);
            _snprintf(buffer_ptr, 128, "%s: UNKNOWN\t", decodeSpId(target_sp));
        }
    }
}

void fbe_cli_display_viking_ps_status(fbe_device_physical_location_t *location,
                                      fbe_u32_t ps_count)
{
    fbe_esp_ps_mgmt_ps_info_t ps_info[FBE_CLI_SP_MAX_PS_COUNT];
    fbe_status_t status;
    fbe_u32_t slot;

    for(slot = 0; slot < ps_count; slot++)
    {
        fbe_set_memory(&ps_info[slot], 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        fbe_copy_memory(&ps_info[slot].location, location, sizeof(fbe_device_physical_location_t));
        ps_info[slot].location.slot = slot;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&ps_info[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get ps info fail, status: %d.\n", __FUNCTION__, status);
            return;
        }
    }

    fbe_cli_printf("  PS ");
    for( slot = 0; slot < ps_count; slot++ )
    {
        // translate the slot numbers to A0/A1/B0/B1
        // slot:   0  1  2  3
        // label: A1 A0 B1 B0
        switch(slot)
        {
            case 0:
            fbe_cli_printf("A1:");
            break;
            case 1:
            fbe_cli_printf("A0:");
            break;
            case 2:
            fbe_cli_printf("B1:");
            break;
            case 3:
            fbe_cli_printf("B0:");
            break;
            default:
            fbe_cli_printf("??:");
            break;
        }

        if( !ps_info[slot].psInfo.bInserted )
        {
            fbe_cli_printf("---  ");
        }
        else
        {
            if(ps_info[slot].psLogicalFault)
            {
                fbe_cli_printf("FLT ");
            }
            else 
            {
                fbe_cli_printf("OK ");
            }
            
            if(ps_info[slot].psInfo.uniqueId == DAE_AC_1080) 
            {
                fbe_cli_printf("(Rev:");
                fbe_cli_printf("%s) ",(char *)(&ps_info[slot].psInfo.firmwareRev));
            }
            else
            {
                fbe_cli_printf("(Rev: %s) ", ps_info[slot].psInfo.firmwareRev);
            }
        }
    }

    fbe_cli_printf("\n");
    
    return;
} //fbe_cli_display_viking_ps_status
                                       
/*!***************************************************************
 * @fn fbe_cli_display_dpe_dae_power_supply_status(
 *                fbe_device_physical_location_t *location,
 *                fbe_u32_t ps_count)
 ****************************************************************
 * @brief
 *    This function get the PS status info for DAE\DPE\XPE and call the
 *    related format output function.
 *
 *    for the DAE PS info, call fbe_cli_display_dae_power_supply_status
 *    for DPE/XPE PS info, call fbe_cli_display_sp_power_supply_status
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    ps_count - PS number
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_dpe_dae_ps_status(fbe_device_physical_location_t *location,
                                       fbe_u32_t ps_count,
                                       fbe_esp_encl_type_t encl_type)
{
    fbe_esp_ps_mgmt_ps_info_t ps_info[FBE_CLI_SP_MAX_PS_COUNT];
    fbe_u8_t slot;
    fbe_status_t status;
    fbe_u8_t revIndex = 0;

    fbe_u32_t loop_tmp;
    char ps_index_string[FBE_CLI_SP_CMD_STRING_LEN];
    char ps_stat_string[FBE_CLI_SP_CMD_STRING_LEN] = {0};
    char *ps_index_string_ptr, *ps_stat_string_ptr;

    // split off the viking and naga for an easier to read format
    if((FBE_ESP_ENCL_TYPE_VIKING == encl_type) ||
       (FBE_ESP_ENCL_TYPE_NAGA == encl_type ))
    {
        fbe_cli_display_viking_ps_status(location, ps_count);
        return;
    }
    for(slot = 0; slot < ps_count; slot++)
    {
        fbe_set_memory(&ps_info[slot], 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        fbe_copy_memory(&ps_info[slot].location, location, sizeof(fbe_device_physical_location_t));
        ps_info[slot].location.slot = slot;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&ps_info[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get ps info fail, status: %d.\n", __FUNCTION__, status);
            return;
        }
    }

    fbe_zero_memory(ps_index_string, FBE_CLI_SP_CMD_STRING_LEN);
    fbe_zero_memory(ps_stat_string, FBE_CLI_SP_CMD_STRING_LEN);
    ps_index_string_ptr = ps_index_string;

    for( loop_tmp = 0; loop_tmp < ps_count; loop_tmp++ )
    {
        if (ps_count > 2)
        {
            // translate the slot numbers to A0/A1/B0/B1
            // slot:   0  1  2  3
            // label: A1 A0 B1 B0
            if(loop_tmp < (ps_count/2))
            {
                *(ps_index_string_ptr++) = 'A';
            }
            else
            {
                *(ps_index_string_ptr++) = 'B';
            }
            
            if (loop_tmp % 2)
            {
                *(ps_index_string_ptr++) = '0';
            }
            else
            {
                *(ps_index_string_ptr++) = '1';
            }
        }
        else
        {
            //the ps index output format is like PS A/B/C:
            *(ps_index_string_ptr++) = 'A' + loop_tmp;
        }
        *(ps_index_string_ptr++) = '\t';
        *(ps_index_string_ptr++) = '\t';

        if( !ps_info[loop_tmp].psInfo.bInserted )
        {
            strncat(ps_stat_string, "---  ", sizeof(ps_stat_string)-strlen(ps_stat_string)-1);
        }
        else
        {
            if(ps_info[loop_tmp].psLogicalFault)
            {
                strncat(ps_stat_string, "FLT ", sizeof(ps_stat_string)-strlen(ps_stat_string)-1);
            }
            else 
            {
                strncat(ps_stat_string, "OK ", sizeof(ps_stat_string)-strlen(ps_stat_string)-1);
            }
            
            if(ps_info[loop_tmp].psInfo.uniqueId == DAE_AC_1080) 
            {
            
                *(ps_index_string_ptr++) = '(';
    
                for(revIndex = 0; revIndex < FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS; revIndex ++) 
                {
                    if(ps_info[loop_tmp].psInfo.psRevInfo[revIndex].firmwareRevValid)
                    {
                        if(revIndex != 0) 
                        {
                            strncat(ps_stat_string, ",", sizeof(fbe_u8_t));
                        }
    
                        strncat(ps_stat_string, 
                                &ps_info[loop_tmp].psInfo.psRevInfo[revIndex].firmwareRev[0],
                                (FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1)*sizeof(fbe_u8_t));
                    }
                }
                strncat(ps_stat_string, ")\t", 2*sizeof(fbe_u8_t));
            }
            else
            {
                ps_stat_string_ptr = ps_stat_string + strlen(ps_stat_string);
                _snprintf(ps_stat_string_ptr, 128, "(%s)\t", ps_info[loop_tmp].psInfo.firmwareRev);
            }
        }
    }
    
    if( ps_count != 0 )
    {
        //remove the backslash at the end of ps_index_string
        ps_index_string[strlen(ps_index_string) - 1] = '\0';

        fbe_cli_printf("  PS\t%s\n\t%s\n", ps_index_string, ps_stat_string);
    }
    return;
}

/*!***************************************************************
 * @fn fbe_cli_display_xpe_power_supply_status(
 *                fbe_device_physical_location_t *location,
 *                fbe_u32_t ps_count)
 ****************************************************************
 * @brief
 *    This function get the PS status info for DAE\DPE\XPE and call the
 *    related format output function.
 *
 *    for the DAE PS info, call fbe_cli_display_dae_power_supply_status
 *    for DPE/XPE PS info, call fbe_cli_display_sp_power_supply_status
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    ps_count - PS number
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_xpe_ps_status(fbe_device_physical_location_t *location,
                                                 fbe_u32_t ps_count)
{
    fbe_esp_ps_mgmt_ps_info_t ps_info[FBE_CLI_SP_MAX_PS_COUNT];
    fbe_u8_t slot;
    fbe_status_t status;

    fbe_u32_t loop_tmp;

    char flt_string[FBE_CLI_SP_CMD_STRING_LEN];

    for(slot = 0; slot < ps_count; slot++)
    {
        fbe_set_memory(&ps_info[slot], 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        fbe_copy_memory(&ps_info[slot].location, location, sizeof(fbe_device_physical_location_t));
        ps_info[slot].location.slot = slot;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&ps_info[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get ps info fail, status: %d.\n", __FUNCTION__, status);
            return;
        }
    }

    for( loop_tmp = 0; loop_tmp < ps_count; loop_tmp++ )
    {
        fbe_zero_memory(flt_string, FBE_CLI_SP_CMD_STRING_LEN);

        if( ps_count == 2 )
        {
            //if there are two ps, the output is SPA:...\n SPB:... \n
            if( loop_tmp == 0 )
            {
                fbe_cli_printf("    PSA: ");
            }
            else
            {
                fbe_cli_printf("    PSB: ");
            }
        }
        else
        {
            //if the ps number is bigger than 2, the output is SPA0:...\n PSA1:...\n PSB0:...\n ...
            if( loop_tmp < ps_count/2 )
            {
                fbe_cli_printf("    PSA%d: ", loop_tmp );
            }
            else
            {
                fbe_cli_printf("    PSB%d: ", loop_tmp - ps_count/2 );
            }
        }

        //start print the ps status
        if( !ps_info[loop_tmp].psInfo.bInserted )
        {
            fbe_cli_printf("---\n");
        }
        else
        {
            switch(ps_info[loop_tmp].psInfo.generalFault)
            {
                case FBE_MGMT_STATUS_TRUE:
                    fbe_cli_printf("FLT ");
                    strncat(flt_string, "genFlt ", sizeof(flt_string)-strlen(flt_string)-1);
                    break;
                case FBE_MGMT_STATUS_FALSE:
                    fbe_cli_printf("OK ");
                    break;
                case FBE_MGMT_STATUS_NA:
                    fbe_cli_printf("NA ");
                    break;
                case FBE_MGMT_STATUS_UNKNOWN:
                    fbe_cli_printf("UNK ");
                    break;
                default:
                    fbe_cli_printf("???");
                    break;
            }

            //build the fault info string
            if(ps_info[loop_tmp].psInfo.internalFanFault == FBE_MGMT_STATUS_TRUE)
            {
                strncat(flt_string,"internalFanFlt ", sizeof(flt_string)-strlen(flt_string)-1);
                break;
            }
            if(ps_info[loop_tmp].psInfo.ambientOvertempFault == FBE_MGMT_STATUS_TRUE)
            {
                strncat(flt_string, "ambientOvertempFlt ", sizeof(flt_string)-strlen(flt_string)-1);
            }
            if(ps_info[loop_tmp].psInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                strncat(flt_string, "envInterfaceStatusFlt ", sizeof(flt_string)-strlen(flt_string)-1);
            }
            if(!ps_info[loop_tmp].psInfo.psSupported)
            {
                strncat(flt_string, "Unsupported ", sizeof(flt_string)-strlen(flt_string)-1);
            }
            //remove the blank at the end of string
            flt_string[strlen(flt_string) - 1] = '\0';

            //if the fault string is not empty, print the fault info
            if(strlen(flt_string) != 0)
            {
                fbe_cli_printf("(%s)", flt_string);
            }

            fbe_cli_printf("%s\n", ps_info[loop_tmp].psInfo.firmwareRev);
        }
    }
    fbe_cli_printf("\n");
}

/*!*************************************************************************
 * @fn fbe_cli_lib_sp_decode_fan_state
 **************************************************************************
 *
 *  @brief
 *      This function retun drive stat into text format.
 *
 *  @param   slot_flags_t  *driveDataPtr
 *
 *  @return    char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-Mar-2009: Dipak Patel- created.
 *
 **************************************************************************/
char * fbe_cli_lib_sp_decode_fan_state( fbe_esp_cooling_mgmt_fan_info_t *pFanInfo)
{
   char *string = NULL;

    if(pFanInfo->inserted != FBE_MGMT_STATUS_TRUE)
    {
        string = "---";
    }
    else
    {
        if(pFanInfo->fanFaulted == FBE_MGMT_STATUS_TRUE)
        {
            string = "FLT";
        }
        else if(pFanInfo->fanDegraded == FBE_MGMT_STATUS_TRUE)
        {
            string = "DEG";
        }
        else
        {
            string = "OK ";
        }
    }
   return(string);
} //fbe_cli_lib_sp_decode_fan_state


/*!***************************************************************
 * @fn fbe_cli_display_specificFanStatus(
 *             fbe_esp_cooling_mgmt_fan_info_t *pFanInfo,
 *             fbe_u32_t fan_count,
 *             fbe_fan_location_t fanLocation)
 ****************************************************************
 * @brief
 *    This function displays specific FAN status for DPE's.
 *
 * @param
 *    *pFanInfo - pointer to DPE Fan info
 *    fan_count - Fan count
 *    fanLocation - where Fans are located (Front, Back, SP)
 *
 * @return
 *  None
 *
 * @version
 *  27-Oct-2014: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_display_specificFanStatus(fbe_esp_cooling_mgmt_fan_info_t *pFanInfo,
                                       fbe_u32_t fan_count,
                                       fbe_fan_location_t fanLocation)
{
    fbe_u32_t loop_tmp;
    char fan_index_string[FBE_CLI_SP_CMD_STRING_LEN];
    char fan_stat_string[FBE_CLI_SP_CMD_STRING_LEN];
    char *fan_index_string_ptr;
    char *fan_stat_string_prt;
    fbe_esp_cooling_mgmt_fan_info_t *pTempFanInfo = pFanInfo;

    fbe_zero_memory(fan_index_string, FBE_CLI_SP_CMD_STRING_LEN);
    fbe_zero_memory(fan_stat_string, FBE_CLI_SP_CMD_STRING_LEN);
    fan_index_string_ptr = fan_index_string;

    for( loop_tmp = 0; loop_tmp < fan_count; loop_tmp++ )
    {
        if (fanLocation != pTempFanInfo->fanLocation)
        {
            pTempFanInfo++;
            continue;
        }
        //the fan index output format is like FAN A/B/C or A0/A1/B0/B1
        if ((pTempFanInfo->associatedSp != SP_A) && (pTempFanInfo->associatedSp != SP_B))
        {
            *(fan_index_string_ptr++) = 'A' + loop_tmp;
            *(fan_index_string_ptr++) = '/';
        }
        else
        {
            *(fan_index_string_ptr++) = 'A' + pTempFanInfo->associatedSp;
            *(fan_index_string_ptr++) = '0' + pTempFanInfo->slotNumOnSpBlade;
        }
        // if Fan has Resume PROM (FW), leave more space for revision
        if (pTempFanInfo->resumePromSupported)
        {
            *(fan_index_string_ptr++) = '\t';
            *(fan_index_string_ptr++) = '\t';
        }
        else
        {
            *(fan_index_string_ptr++) = ' ';
            *(fan_index_string_ptr++) = ' ';
        }
        if(pTempFanInfo->inserted != FBE_MGMT_STATUS_TRUE)
        {
            strncat(fan_stat_string, "---  ", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
        }
        else
        {
            if(pTempFanInfo->fanFaulted == FBE_MGMT_STATUS_TRUE)
            {
                strncat(fan_stat_string, "FLT ", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            else if(pTempFanInfo->fanDegraded == FBE_MGMT_STATUS_TRUE)
            {
                strncat(fan_stat_string, "DEG ", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            else
            {
                strncat(fan_stat_string, "OK  ", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            fan_stat_string_prt = fan_stat_string + strlen(fan_stat_string);
            if (pTempFanInfo->resumePromSupported &&
                (strcmp(pTempFanInfo->firmwareRev, "") > 0))
            {
                _snprintf(fan_stat_string_prt, 128, "(%s)\t", pTempFanInfo->firmwareRev);
            }
        }
        pTempFanInfo++;
    }

    if( fan_count != 0 )
    {
        //remove the backslash at the end of fan_index_string
        fan_index_string[strlen(fan_index_string) - 1] = '\0';

        if (fanLocation == FBE_FAN_LOCATION_FRONT)
        {
            fbe_cli_printf("  FAN %s\n      %s\n", fan_index_string, fan_stat_string);
        }
        else if (fanLocation == FBE_FAN_LOCATION_BACK)
        {
            fbe_cli_printf("  BM FAN   %s\n           %s\n", fan_index_string, fan_stat_string);
        }
    }

}   // end of fbe_cli_display_specificFanStatus


/*!***************************************************************
 * @fn fbe_cli_display_dpeFanStatus(
 *             fbe_esp_cooling_mgmt_fan_info_t *pFanInfo,
 *             fbe_u32_t fan_count)
 ****************************************************************
 * @brief
 *    This function processes FAN status for DPE's.
 *
 * @param
 *    *pFanInfo - pointer to DPE Fan info
 *    fan_count - Fan count
 *
 * @return
 *  None
 *
 * @version
 *  27-Oct-2014: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_display_dpeFanStatus(fbe_esp_cooling_mgmt_fan_info_t *pFanInfo,
                                  fbe_u32_t fan_count)
{
    fbe_u32_t loop_tmp;
    fbe_esp_cooling_mgmt_fan_info_t *pTempFanInfo = pFanInfo;
    fbe_u32_t   frontFanCount = 0, backFanCount = 0;

    // determine how many Front & Back fans there are
    for (loop_tmp = 0; loop_tmp < fan_count; loop_tmp++)
    {
        if (pTempFanInfo->fanLocation == FBE_FAN_LOCATION_FRONT)
        {
            frontFanCount++;
        }
        else if (pTempFanInfo->fanLocation == FBE_FAN_LOCATION_BACK)
        {
            backFanCount++;
        }
        pTempFanInfo++;
    }

    fbe_cli_display_specificFanStatus(pFanInfo, fan_count, FBE_FAN_LOCATION_FRONT);
    if (backFanCount > 0)
    {
        fbe_cli_display_specificFanStatus(pFanInfo, fan_count, FBE_FAN_LOCATION_BACK);
    }

}   // end of fbe_cli_display_dpeFanStatus


/*!***************************************************************
 * @fn fbe_cli_display_dae_dpe_fan_status(
 *             fbe_device_physical_location_t *location,
 *             fbe_u32_t fan_count)
 ****************************************************************
 * @brief
 *    This function get the FAN status info for DAE\DPE\XPE and call the
 *    related format output function.
 *
 *    for the DAE FAN info, call fbe_cli_display_dae_fan_status
 *    for DPE/XPE FAN info, call fbe_cli_display_sp_fan_status
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    fan_count - FAN number
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_dae_dpe_fan_status(fbe_device_physical_location_t *location,
                                        fbe_u32_t fan_count,
                                        fbe_esp_encl_type_t encl_type)
{
    fbe_device_physical_location_t local_location;
    fbe_esp_cooling_mgmt_fan_info_t fan_info[FBE_CLI_SP_MAX_FAN_COUNT];
    fbe_u8_t slot;
    fbe_status_t status;

    fbe_u32_t loop_tmp;
    char fan_index_string[FBE_CLI_SP_CMD_STRING_LEN];
    char fan_stat_string[FBE_CLI_SP_CMD_STRING_LEN];
    char *fan_index_string_ptr;
    char *fan_stat_string_prt;

    for(slot = 0; slot < fan_count; slot++)
    {
        fbe_set_memory(&fan_info[slot], 0, sizeof(fbe_esp_cooling_mgmt_fan_info_t));
        fbe_copy_memory(&local_location, location, sizeof(fbe_device_physical_location_t));
        local_location.slot = slot;

        status = fbe_api_esp_cooling_mgmt_get_fan_info(&local_location, &fan_info[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get fan info. error code: %d.\n", __FUNCTION__, status);
            return;
        }
    }
    if ( (FBE_ESP_ENCL_TYPE_VIKING == encl_type) ||
         (FBE_ESP_ENCL_TYPE_NAGA == encl_type)  || 
         (FBE_ESP_ENCL_TYPE_CAYENNE == encl_type) )

    {
        // viking labels fans 0-9
        fbe_cli_printf("\n  Fan");
        for (loop_tmp = 0; loop_tmp < fan_count; loop_tmp++)
        {
            fbe_cli_printf("%3d ",loop_tmp);
        }
        fbe_cli_printf("\n      "); 

        for (loop_tmp = 0; loop_tmp < fan_count; loop_tmp++)
        {
            fbe_cli_printf("%s ",fbe_cli_lib_sp_decode_fan_state(&fan_info[loop_tmp]));
        }
        fbe_cli_printf("\n");

        return;
    }
    else if ((FBE_ESP_ENCL_TYPE_CALYPSO == encl_type) ||
             (FBE_ESP_ENCL_TYPE_RHEA == encl_type) ||
             (FBE_ESP_ENCL_TYPE_MIRANDA == encl_type) ||
             (FBE_ESP_ENCL_TYPE_RAMHORN == encl_type) ||
             (FBE_ESP_ENCL_TYPE_STEELJAW == encl_type) )
    {
        fbe_cli_display_dpeFanStatus(&fan_info[0], fan_count);
        return;
    }

    fbe_zero_memory(fan_index_string, FBE_CLI_SP_CMD_STRING_LEN);
    fbe_zero_memory(fan_stat_string, FBE_CLI_SP_CMD_STRING_LEN);
    fan_index_string_ptr = fan_index_string;

    for( loop_tmp = 0; loop_tmp < fan_count; loop_tmp++ )
    {
        //the fan index output format is like FAN A/B/C or A0/A1/B0/B1
        if (fan_info[loop_tmp].associatedSp != SP_A && fan_info[loop_tmp].associatedSp != SP_B)
        {
            *(fan_index_string_ptr++) = 'A' + loop_tmp;
            *(fan_index_string_ptr++) = '/';
        }
        else
        {
            *(fan_index_string_ptr++) = 'A' + fan_info[loop_tmp].associatedSp;
            *(fan_index_string_ptr++) = '0' + fan_info[loop_tmp].slotNumOnSpBlade;
            *(fan_index_string_ptr++) = '/';
        }

        if(fan_info[loop_tmp].inserted != FBE_MGMT_STATUS_TRUE)
        {
            strncat(fan_stat_string, "---  ", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
        }
        else
        {
            if(fan_info[loop_tmp].fanFaulted == FBE_MGMT_STATUS_TRUE)
            {
                strncat(fan_stat_string, "FLT", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            else if(fan_info[loop_tmp].fanDegraded == FBE_MGMT_STATUS_TRUE)
            {
                strncat(fan_stat_string, "DEG", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            else
            {
                strncat(fan_stat_string, "OK", sizeof(fan_stat_string)-strlen(fan_stat_string)-1);
            }
            fan_stat_string_prt = fan_stat_string + strlen(fan_stat_string);
            if (!fan_info[loop_tmp].resumePromSupported || 
                (strcmp(fan_info[loop_tmp].firmwareRev, "") == 0))
            {
                _snprintf(fan_stat_string_prt, 128, " (Rev: N/A)  ");
            }
            else
            {
                _snprintf(fan_stat_string_prt, 128, " (Rev: %s)  ", fan_info[loop_tmp].firmwareRev);
            }
        }
    }

    if( fan_count != 0 )
    {
        //remove the backslash at the end of fan_index_string
        fan_index_string[strlen(fan_index_string) - 1] = '\0';

        fbe_cli_printf("  FAN %s: %s\n", fan_index_string, fan_stat_string);
    }

    return;
} //fbe_cli_display_dae_dpe_fan_status

/*!***************************************************************
 * @fn fbe_cli_display_xpe_fan_status(
 *             fbe_device_physical_location_t *location,
 *             fbe_u32_t fan_count)
 ****************************************************************
 * @brief
 *    This function get the FAN status info for DAE\DPE\XPE and call the
 *    related format output function.
 *
 *    for the DAE FAN info, call fbe_cli_display_dae_fan_status
 *    for DPE/XPE FAN info, call fbe_cli_display_sp_fan_status
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    fan_count - FAN number
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_xpe_fan_status(fbe_device_physical_location_t *location,
                                        fbe_u32_t fan_count)
{
    fbe_device_physical_location_t local_location;
    fbe_esp_cooling_mgmt_fan_info_t fan_info[FBE_CLI_SP_MAX_FAN_COUNT];
    fbe_u8_t slot;
    fbe_status_t status;

    fbe_u32_t loop_tmp;

    for(slot = 0; slot < fan_count; slot++)
    {
        fbe_set_memory(&fan_info[slot], 0, sizeof(fbe_esp_cooling_mgmt_fan_info_t));
        fbe_copy_memory(&local_location, location, sizeof(fbe_device_physical_location_t));
        local_location.slot = slot;

        status = fbe_api_esp_cooling_mgmt_get_fan_info(&local_location, &fan_info[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get fan info. error code: %d.\n", __FUNCTION__, status);
            return;
        }
    }

    fbe_cli_printf("   ");
    for( loop_tmp = 0; loop_tmp < fan_count; loop_tmp++ )
    {
        if (fan_info[loop_tmp].associatedSp != SP_A && fan_info[loop_tmp].associatedSp != SP_B)
        {
            fbe_cli_printf(" %c: ", 'A' + loop_tmp);
        }
        else
        {
            fbe_cli_printf(" %c%d ", 'A' + fan_info[loop_tmp].associatedSp, fan_info[loop_tmp].slotNumOnSpBlade);
        }
    }
    fbe_cli_printf("\n   ");
    for( loop_tmp = 0; loop_tmp < fan_count; loop_tmp++ )
    {
        if(fan_info[loop_tmp].fanFaulted == FBE_MGMT_STATUS_TRUE)
        {
            fbe_cli_printf(" FLT");
        }
        else if(fan_info[loop_tmp].fanDegraded == FBE_MGMT_STATUS_TRUE)
        {
            fbe_cli_printf(" DEG");
        }
        else if(fan_info[loop_tmp].fanFaulted == FBE_MGMT_STATUS_UNKNOWN)
        {
            fbe_cli_printf(" UNK");
        }
        else if(fan_info[loop_tmp].inserted == FBE_MGMT_STATUS_FALSE)
        {
            fbe_cli_printf(" ---");
        }
        else
        {
            fbe_cli_printf(" OK ");
        }
    }
    fbe_cli_printf("\n");
}

/*!***************************************************************
 * @fn fbe_cli_display_ssc_status(fbe_device_physical_location_t *location,
 *                                fbe_u8_t ssc_count)
 ****************************************************************
 * @brief
 *    This function display the status of SSC in the enclosure.
 *
 * @param
 *    pLocation - the pointer to the location of the target enclosure
 *    ssc count - SSC count
 *
 * @return
 *  None
 *
 * @version
 *  27-Dec-2013: PHE - Created.
 *
 ****************************************************************/
void fbe_cli_display_ssc_status(fbe_device_physical_location_t * pLocation,
                                fbe_u8_t ssc_count)
{
    fbe_u32_t slot = 0;
    fbe_esp_encl_mgmt_ssc_info_t    sscInfo = {0};
    fbe_status_t status = FBE_STATUS_OK;

    for(slot = 0; slot < ssc_count; slot++)
    {
        status = fbe_api_esp_encl_mgmt_get_ssc_info(pLocation, &sscInfo);
        if (status == FBE_STATUS_OK)
        {
            if(sscInfo.inserted)
            {
                fbe_cli_printf("  SSC: %s\n",
                               sscInfo.faulted?"FLT":"OK ");
            }
            else
            {
                // not inserted, continue to next
                fbe_cli_printf("  SSC: ---\n");
            }
        }
        else
        {
            fbe_cli_printf(" Failed to get SSC info, status %d.\n",status);
        }
    }

    return;
}

/*!***************************************************************
 * @fn fbe_cli_display_lcc_status(fbe_device_physical_location_t *location,
 *                                fbe_u8_t lcc_count)
 ****************************************************************
 * @brief
 *    This function display the status of fans in dae.
 *
 *    the format like
 *      LCC0:   OK(rev: )     Is Local:   NO
 *      LCC1:   OK(rev: )     Is Local:   YES
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    lcc_count - LCC number
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_lcc_status(fbe_device_physical_location_t *location,
                                fbe_u8_t lcc_count,
                                fbe_esp_encl_type_t encl_type)
{
    fbe_u32_t       count;
    SP_ID           spId = 0;
    fbe_u8_t        slot;
    fbe_status_t    status;
    fbe_esp_encl_mgmt_lcc_info_t lcc_info;
    fbe_esp_encl_mgmt_lcc_info_t subencl0_lcc_info;
    fbe_esp_encl_mgmt_lcc_info_t subencl1_lcc_info;
    fbe_esp_encl_mgmt_lcc_info_t subencl2_lcc_info;
    fbe_esp_encl_mgmt_lcc_info_t subencl3_lcc_info;

    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,(fbe_base_env_sp_t *)&spId); /* SAFEBUG */
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ( "%s:Failed to get SPID status:%d\n",
            __FUNCTION__, status);
        return;
    }
    
    for(slot = 0; slot < lcc_count; slot++)
    {
        fbe_set_memory(&lcc_info, 0, sizeof(fbe_esp_encl_mgmt_lcc_info_t));

        location->slot = slot;

        status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &lcc_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get lcc info. error code: %d.\n", __FUNCTION__, status);
            return;
        }

        if((encl_type == FBE_ESP_ENCL_TYPE_VIKING) ||
           (encl_type == FBE_ESP_ENCL_TYPE_CAYENNE) ||
           (encl_type == FBE_ESP_ENCL_TYPE_NAGA))
        {
            if(lcc_info.isLocal)
            {
                // local LCC label
                if(spId == SP_A)
                {
                    count = 'A';
                }
                else
                {
                    count = 'B';
                }
            }
            else
            {
                // peer LCC label
                if(spId == SP_A)
                {
                    count = 'B';
                }
                else
                {
                    count = 'A';
                }
            }
            if(lcc_info.inserted)
            {
                fbe_cli_printf("  LCC %c: %s %s",
                              count,
                              lcc_info.faulted?"FLT":"OK ",
                              lcc_info.isLocal?"Local\n":"Peer\n");
            }
            else
            {
                // not inserted, continue to next
                fbe_cli_printf("  LCC %c: --- %s",
                              count,
                              lcc_info.isLocal?"Local\n":" Peer\n");
                continue;
            
            }
        }
        else
        {
            fbe_cli_printf("  LCC %d: ", slot);
        }

        if( !lcc_info.inserted )
        {
            fbe_cli_printf("---\n");
        }
        else
        {
            //if this is a voyager enclosure
            if(encl_type == FBE_ESP_ENCL_TYPE_VOYAGER)
            {
                //the first two lcc are icm
                if(slot <2)
                {
                    fbe_cli_printf("%s (rev:%s) %s, ICM\n",
                            lcc_info.faulted?"FLT":"OK",
                            lcc_info.firmwareRev,
                            lcc_info.isLocal?"Local":" Peer");
                }
                else
                {
                    //the others are EE, we need to display the ver info in both two expanders.
                    location->componentId = 4;
                    status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl0_lcc_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("%s: Error when get subencl0 lcc info, componentId 4, status: 0x%X.\n", __FUNCTION__, status);
                        return;
                    }
                    location->componentId = 5;
                    status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl1_lcc_info);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("%s: Error when get subencl1 lcc info, componentId 5, status: 0x%X.\n", __FUNCTION__, status);
                        return;
                    }
                    fbe_cli_printf("%s (rev:%s, %s) %s, EE_LCC\n",
                            lcc_info.faulted?"FLT":"OK",
                            subencl0_lcc_info.firmwareRev,
                            subencl1_lcc_info.firmwareRev,
                            lcc_info.isLocal?"Local":" Peer");

                    //set the component Id back
                    location->componentId = 0;
                }
            }
            else if(encl_type == FBE_ESP_ENCL_TYPE_VIKING)
            {
                fbe_cli_printf("    Revs IOSXP: %s, ",lcc_info.firmwareRev);
                //the others are DRVSXP, we need to display the ver info in all four DRVSXP.
                location->componentId = 2;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl0_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl0 lcc info, componentId 2, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }
                location->componentId = 3;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl1_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl1 lcc info, componentId 3, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }
                //the others are DRVSXP, we need to display the ver info in both two expanders.
                location->componentId = 4;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl2_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl0 lcc info, componentId 4, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }
                location->componentId = 5;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl3_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl1 lcc info, componentId 5, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }

                fbe_cli_printf("DRVSXP: %s, %s, %s, %s\n",
                        subencl0_lcc_info.firmwareRev,
                        subencl1_lcc_info.firmwareRev,
                        subencl2_lcc_info.firmwareRev,
                        subencl3_lcc_info.firmwareRev);

                //set the component Id back
                location->componentId = 0;
            }
            else if(encl_type == FBE_ESP_ENCL_TYPE_CAYENNE)
            {
                fbe_cli_printf("    Revs IOSXP: %s, ",lcc_info.firmwareRev);
                
                //the others are DRVSXP, we need to display the ver info in the DRVSXP.
                location->componentId = 4;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl0_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl0 lcc info, componentId 4, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }

                fbe_cli_printf("DRVSXP: %s\n",
                        subencl0_lcc_info.firmwareRev);

                //set the component Id back
                location->componentId = 0;
            }
            else if(encl_type == FBE_ESP_ENCL_TYPE_NAGA)
            {
                fbe_cli_printf("    Revs IOSXP: %s, ",lcc_info.firmwareRev);
                //the others are DRVSXP, we need to display the ver info in all two DRVSXP.
                location->componentId = 4;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl0_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl0 lcc info, componentId 4, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }
                location->componentId = 5;
                status = fbe_api_esp_encl_mgmt_get_lcc_info(location, &subencl1_lcc_info);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error when get subencl1 lcc info, componentId 5, status: 0x%X.\n", __FUNCTION__, status);
                    return;
                }
               

                fbe_cli_printf("DRVSXP: %s, %s\n",
                        subencl0_lcc_info.firmwareRev,
                        subencl1_lcc_info.firmwareRev);

                //set the component Id back
                location->componentId = 0;
            }
            else
            {
                fbe_cli_printf("%s (CDES-%d, rev:%s) %s\n",
                               lcc_info.faulted?"FLT":"OK",
                               lcc_info.esesRevision,
                               lcc_info.firmwareRev,
                               lcc_info.isLocal?"Local":" Peer");

            }
        }
    }
}

/*!***************************************************************
 * @fn fbe_cli_display_virtual_lcc_status(
 *      fbe_esp_encl_type_t encl_type)
 ****************************************************************
 * @brief
 *    This function display the status of virtual lcc in DPE. 
 *    DPE does not have real LCC, but we still get lcc info reported
 *    from expander chip. So we call it a virtual LCC. 
 *
 *    the format like
 *    Virtual LCC 0: OK (rev: 116 )  Peer
 *    Virtual LCC 1: OK (rev: 116 ) Local
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    lcc_count - LCC number
 *
 * @return
 *  None
 *
 * @version
 *  5-Mar-2013: Rui Chang - Created.
 *  18-Dec-2014: PHE - Fixed the issue that we did not get the BM expander firmware revision correctly.
 *                     We need to go to module_mgmt to get the BM LCC status not from board_mgmt.
 *
 ****************************************************************/
void fbe_cli_display_virtual_lcc_status(fbe_esp_encl_type_t encl_type)
{
    fbe_lcc_info_t * pLccInfo = NULL;
    fbe_u8_t sp = 0;
    fbe_u8_t spCount;
    fbe_status_t status;
    fbe_esp_board_mgmt_board_info_t board_info = {0};
    fbe_esp_module_io_module_info_t bemInfo = {0};

    
    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, error code: %d.\n", __FUNCTION__, status);
        return;
    }

    // Only interested in LCC's on SP or seperate backend module 
    if (board_info.lccInfo[SP_A].lccActualHwType != FBE_DEVICE_TYPE_SP &&
        board_info.lccInfo[SP_A].lccActualHwType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        return;
    }

    if (board_info.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    if (board_info.lccInfo[SP_A].lccActualHwType == FBE_DEVICE_TYPE_SP)
    {
        fbe_cli_printf("  SP Expanders\t"); 

        for(sp = 0; sp < spCount ; sp++)
        {
            pLccInfo = &board_info.lccInfo[sp];
            
            if( !pLccInfo->inserted )
            {
                fbe_cli_printf("---\n");
            }
            else
            {
                fbe_cli_printf("Exp%d(%s): %s (%s)\t\t",
                               sp,
                               (pLccInfo->isLocal?"Local":" Peer"),
                                pLccInfo->faulted?"FLT":"OK",
                                pLccInfo->firmwareRev);

            }
        }
    }
    else if (board_info.lccInfo[SP_A].lccActualHwType == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        fbe_cli_printf("  BM Expanders\t"); 

        for(sp = 0; sp < spCount ; sp ++)
        {
            bemInfo.header.sp = sp;
            bemInfo.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
            bemInfo.header.slot = 0;

            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&bemInfo);

            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when getting BM(SP:%d, SLOT: %d) info, status: 0x%X.\n", 
                               __FUNCTION__, bemInfo.header.sp, bemInfo.header.slot, status);
            }
            else 
            {
                pLccInfo = &bemInfo.io_module_phy_info.lccInfo;

                if( !pLccInfo->inserted )
                {
                    fbe_cli_printf("---\n");
                }
                else
                {
                    fbe_cli_printf("Exp%d(%s): %s (%s)\t\t",
                                   sp,
                                   (pLccInfo->isLocal?"Local":" Peer"),
                                    pLccInfo->faulted?"FLT":"OK",
                                    pLccInfo->firmwareRev);
                }
            }
        } // End of - for(sp = 0; sp < spCount ; sp ++)
    }
    
    fbe_cli_printf("\n");
}

/*!****************************************************************************
 * @fn      fbe_u32_t fbe_cli_print_slot_stat_header(
 *                       fbe_u32_t bank_width, fbe_u32_t num_slots)
 *****************************************************************************
 * @brief
 *  This function prints the title for spstat output
 *
 * @param
 *       bank_width - how many slots per bank.
 *       num_slots - number of slots in the enclosure.
 *
 * @return number of slots to print out per line
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *  25-Sep-2013: Lin Lou - Modified.
 *
 *****************************************************************************/
fbe_u32_t fbe_cli_print_slot_stat_header(fbe_u32_t bank_width, fbe_u32_t num_slots)
{
    if (bank_width == 0)
    {
        if (num_slots == 25)
        {
            fbe_cli_printf("  Slot    0     1     2     3     4     5     6     7     8     9    10    11    12    13    14\n");
            fbe_cli_printf("  Slot   15    16    17    18    19    20    21    22    23    24\n");
            return 15;
        }
        else if (num_slots == 15)
        {
            fbe_cli_printf("  Slot    0     1     2     3     4     5     6     7     8     9    10    11    12    13    14\n");
            return 15;
        }
        else if (num_slots == 12)
        {
            fbe_cli_printf("  Slot    0     1     2     3     4     5     6     7     8     9    10    11\n");
            return 12;
        }
    }
    else if (bank_width == 12)
    {
        fbe_cli_printf("  Slot   0   1   2   3   4   5   6   7   8   9  10  11\n");
        return 12;
    }
    else if (bank_width == 10)
    {
        fbe_cli_printf("  Slot   0   1   2   3   4   5   6   7   8   9\n");
        return 10;
    }
    else if (bank_width == 20)
    {
        fbe_cli_printf(" Slot   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19\n");
        return bank_width;
    }
    return 15;
}

/*!**************************************************************************
 * @fn fbe_cli_display_slot_status(fbe_device_physical_location_t *location,
 *                                 fbe_u8_t slot_count,
 *                                 fbe_esp_encl_type_t encl_type,
 *                                 fbe_led_status_t  *enclDriveFaultLeds)
 ****************************************************************************
 * @brief
 *    This function display the status of slots in dae.
 *
 *    the format is:
 *            header
 *     index  state
 *    and the header is
 *
 * @param
 *    location - the location info for the target DAE/DPE/XPE
 *    slot_count - slot number
 *    encl_type - enclosure type
 *    enclDriveFaultLeds - pointer to enclosure drive fault led setting
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *  21-Jan-2013:     Michael Li - Added to get drive insertedness from both DMO and EMO
 *  25-April-2013:   Christina Chiang - Passed the current enclosure drive fault led setting 
 *
 ****************************************************************/
void fbe_cli_display_slot_status(fbe_device_physical_location_t *location,
                                        fbe_u8_t slot_count,
                                        fbe_esp_encl_type_t encl_type,
                                        fbe_led_status_t  *enclDriveFaultLeds)
{
    fbe_u32_t bank_width = 0;
    fbe_u8_t slot;
    fbe_u32_t loop_tmp;
    fbe_status_t status;
    //fbe_lifecycle_state_t drive_state_in_dmo = FBE_LIFECYCLE_STATE_INVALID;
    char row1_buf[FBE_CLI_SP_CMD_STRING_LEN];
    char *row1_buf_ptr;
    fbe_u32_t visible_enclosure_slots;
    fbe_u32_t decade_counter;
    char insertedness_str[FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN];   //returned drive insertedness state in 3 letter abbreviation
        
    loop_tmp = 0;
    for(loop_tmp = 0;drive_print_format_table[loop_tmp].encl_type != FBE_ESP_ENCL_TYPE_UNKNOWN; loop_tmp++)
    {
        if(drive_print_format_table[loop_tmp].encl_type == encl_type)
        {
            bank_width = drive_print_format_table[loop_tmp].bank_width;
            break;
        }
    }

    visible_enclosure_slots = fbe_cli_print_slot_stat_header(bank_width, slot_count);

    decade_counter = 0;
    /* loop thru all disks in the enclosure */
    row1_buf_ptr = row1_buf;
    *row1_buf_ptr = '\0';

    //get drive insertedness
    fbe_zero_memory(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN);
    for(slot = 0; slot < slot_count; slot++)
    {
        //get drive insertedness
        location->slot = slot;
        status = fbe_api_esp_encl_mgmt_get_drive_insertedness_state(location,
                                                                    (char *)&insertedness_str[0],                                                            
                                                                    NULL, 
                                                                    enclDriveFaultLeds[slot]);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get drive insertedness state fail, slot: %d.\n", __FUNCTION__, slot);
            return;
        }
        if (bank_width == 0)
        {
            row1_buf_ptr += _snprintf(row1_buf_ptr, 128, "%6s", insertedness_str);
        }
        else
        {
            row1_buf_ptr += _snprintf(row1_buf_ptr, 128, "%4s", insertedness_str);
        }

        // We check to see if we want to terminate the row. This is either when we are on a
        // mod of rowsize. Or at the last index to cover case when last row is not == rowsize.
        if (slot > 1 && (((slot + 1) % visible_enclosure_slots) == 0) ||
            (slot == slot_count - 1))
        {
            /* print out the first half of the disk information */
            fbe_cli_printf("%4d  %s\n", decade_counter, row1_buf);
            decade_counter += visible_enclosure_slots;
            row1_buf_ptr = row1_buf;
            *row1_buf_ptr = '\0';
        }
    }
}

/*!***************************************************************
 * @fn fbe_cli_sp_display_specific_encl(fbe_device_physical_location_t *location)
 ****************************************************************
 * @brief
 *    This function display the dae info in specific location.
 *
 * @param
 *    location - the location of the enclosure
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_sp_display_specific_encl(fbe_device_physical_location_t *location, fbe_bool_t is_dpe_encl )
{
    fbe_esp_encl_mgmt_encl_info_t   encl_info;
    fbe_status_t                    status;
    SP_ID                           sp;
    fbe_u8_t                        spsIndex;
    char                            mgmt_module_info_buffer[FBE_CLI_SP_CMD_STRING_LEN];

    status = fbe_api_esp_encl_mgmt_get_encl_info(location, &encl_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Get enclosure info fail, location: sp %d, bus %d, enclosure %d.\n", __FUNCTION__, location->sp, location->bus, location->enclosure);
        return;
    }
    if(!encl_info.enclPresent)
    {
        return;
    }

    //if this encl is the dpe encl(location 0_0), display DPE... else display DAE...
    fbe_cli_printf("%s-%s %d (Bus %d, Pos %d) -%s",
            is_dpe_encl?"DPE":"DAE",
            fbe_cli_print_EnclosureType(encl_info.encl_type),
            location->bus*10 + location->enclosure,
            location->bus, location->enclosure,
            fbe_encl_mgmt_decode_encl_state(encl_info.enclState));
    if (encl_info.ignoreUnsupportedEnclosures)
    {
        fbe_cli_printf("\t\tIGNORE UNSUPPORTED ENCLOSURES\n");
    }
    else
    {
        fbe_cli_printf("\n");
    }

    for( spsIndex = SP_A; spsIndex < encl_info.spsCount; spsIndex++ )
    {
        location->slot = spsIndex;
        fbe_cli_display_sps_status(location);
    }
    //end display sp sps status

    fbe_cli_printf("  EnclFaultLedStatus: %s\n", fbe_base_env_decode_led_status(encl_info.enclFaultLedStatus));
    fbe_cli_printf("  EnclFaultLedReason: %s\n", fbe_base_env_decode_encl_fault_led_reason(encl_info.enclFaultLedReason));
    
    // display Mgmt Modules if any, DPE only
    if (is_dpe_encl)
    {
        fbe_set_memory(mgmt_module_info_buffer, 0, sizeof(FBE_CLI_SP_CMD_STRING_LEN));
        for (sp = SP_A; sp < SP_ID_MAX; sp++) 
        {
            fbe_cli_get_management_module_info(sp, mgmt_module_info_buffer);
        }

        if(strlen(mgmt_module_info_buffer) > 0)
        {
            fbe_cli_printf("  Mgmt Modules\t");
            fbe_cli_printf("  %s", mgmt_module_info_buffer);
        }
        fbe_cli_printf("\n");
    }

    fbe_cli_display_ssc_status(location, encl_info.sscCount);

    fbe_cli_display_dpe_dae_ps_status(location, encl_info.psCount, encl_info.encl_type);

    //display sp BBU status
    if (encl_info.bbuCount > 0)
    {
        fbe_cli_displayBbuStatus(encl_info.bbuCount);
    }

    fbe_cli_display_lcc_status(location, encl_info.lccCount, encl_info.encl_type);

    if (encl_info.lccCount == 0)
    {
        fbe_cli_display_virtual_lcc_status(encl_info.encl_type);
    }

    fbe_cli_display_dae_dpe_fan_status(location, encl_info.fanCount, encl_info.encl_type);

    fbe_cli_display_slot_status(location, encl_info.driveSlotCount, encl_info.encl_type, &encl_info.enclDriveFaultLeds[0]);

    fbe_cli_printf("\n");
}

/*!***************************************************************
 * @fn fbe_cli_display_encl_info_on_bus(
 *                     fbe_device_physical_location_t *location)
 ****************************************************************
 * @brief
 *    This function display the dae info on the bus.
 *
 * @param
 *    location - include the bus number to print
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_display_encl_info_on_bus(fbe_device_physical_location_t *location, HW_ENCLOSURE_TYPE encl_type)
{
    fbe_u8_t enclosure;
    fbe_bool_t is_dpe_encl;

    for(enclosure = 0; enclosure < FBE_ENCLOSURES_PER_BUS; enclosure++)
    {
        is_dpe_encl = false;
        location->enclosure = enclosure;
        //the DPE enclosure(location 0_0) info display is different from DAE
        if(location->bus == 0 && location->enclosure == 0 && encl_type == DPE_ENCLOSURE_TYPE)
        {
            is_dpe_encl = true;
        }
        fbe_cli_sp_display_specific_encl(location, is_dpe_encl);
    }
}
/*!***************************************************************
 * @fn fbe_cli_display_cmi_info(void)
 ****************************************************************
 * @brief
 *    Get the cmi info and display it.
 *
 * @param
 *
 * @return
 *  None
 *
 * @version
 *  24-Sep-2015 : baileg1 - Created.
 *
 ****************************************************************/
void fbe_cli_display_cmi_info(void)
{
    fbe_cmi_service_get_info_t  cmi_info;
    fbe_status_t                status;

    status = fbe_api_esp_cmi_service_get_info(&cmi_info);
    if (status == FBE_STATUS_OK) 
    {
        fbe_cli_printf("CMI Info:\n");
        fbe_cli_printf("  Peer Alive: %s\n", cmi_info.peer_alive ? "Yes" : "No");
        fbe_cli_printf("  CMI State: ");
        switch ( cmi_info.sp_state) {
        case FBE_CMI_STATE_PASSIVE:
            fbe_cli_printf("Passive\n");
            break;
        case FBE_CMI_STATE_ACTIVE:
            fbe_cli_printf("Active\n");
            break;
        case FBE_CMI_STATE_BUSY:
            fbe_cli_printf("Busy\n");
            break;
        case FBE_CMI_STATE_SERVICE_MODE:
            fbe_cli_printf("Service Mode\n");
            break;
        case FBE_CMI_STATE_INVALID:
            fbe_cli_printf("Invalid\n");
            break;
        default:
            fbe_cli_error("Unknown CMI state\n");
            break;
        }
        fbe_cli_printf("\n");
    }
    else
    {
        fbe_cli_printf("Failed to get CMI info (status:%d)",status);
    }
    return;
}
/*!***************************************************************
 * @fn fbe_cli_cmd_sp(int argc , char ** argv)
 ****************************************************************
 * @brief
 *    This function display the status SP and daes plugged in this SP.
 *
 * @param
 *    argc - argument count
 *    argv - argument string
 *
 * @return
 *  None
 *
 * @version
 *  1-December-2011: Dongz - Created.
 *
 ****************************************************************/
void fbe_cli_cmd_sp(int argc , char ** argv)
{
    SP_ID                               sp, local_sp = SP_INVALID;
    HW_ENCLOSURE_TYPE                   enclosure_type;
    fbe_esp_board_mgmt_board_info_t     board_info;
    fbe_device_physical_location_t      location;
    fbe_esp_encl_mgmt_encl_info_t       encl_info;
    fbe_u8_t                            bus;
    char                                mgmt_module_info_buffer[FBE_CLI_SP_CMD_STRING_LEN];
    fbe_u32_t                           spsIndex;
    fbe_esp_sps_mgmt_spsCounts_t        spsCountInfo;
    fbe_u32_t                           i=0,cache_card_count=0, spCount=SP_ID_MAX;
    fbe_status_t                        status;
    fbe_board_mgmt_cache_card_info_t    cacheCardInfo={0};
    fbe_bool_t                          is_service_mode = FALSE;
    fbe_esp_board_mgmt_peer_boot_info_t peerBootInfo = {0};

    if (argc >= 2)
    {
        fbe_cli_printf("\n%s", SP_CMD_USAGE);
        return;
    }

    //get current SP id
    status = fbe_api_esp_common_get_sp_id(FBE_CLASS_ID_MODULE_MGMT,(fbe_base_env_sp_t *)&local_sp); /* SAFEBUG */
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ( "\n%s:Failed to get SPID status:%d\n",
            __FUNCTION__, status);
        return;
    }

    fbe_api_esp_common_get_is_service_mode(FBE_CLASS_ID_ENCL_MGMT, &is_service_mode);

    if (is_service_mode == TRUE) 
    {
        fbe_cli_printf("\n**This SP is currently running in SERVICE MODE.**\n");
    }

    //display board info
    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\n%s: Error when get board management info, error code: %d.\n", __FUNCTION__, status);
        return;
    }

    enclosure_type = board_info.platform_info.enclosureType;
    if(enclosure_type > MAX_ENCLOSURE_TYPE)
    {
        fbe_cli_printf("\nIllegal enclosure type: %d.\n", enclosure_type);
        return;
    }

    if (board_info.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    if (board_info.isSingleSpSystem)
    {
        fbe_cli_printf("\nSingle SP System\n");
        status = fbe_api_esp_board_mgmt_getCacheCardCount(&cache_card_count);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get Cache Card count, status:%d\n", status);
        }
        else
        {
            fbe_cli_printf("Cache Card Count: %d\n", cache_card_count);

            for (i=0;i<cache_card_count;i++)
            {
                cacheCardInfo.cacheCardID = i;
                status = fbe_api_esp_board_mgmt_getCacheCardStatus(&cacheCardInfo);
        
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed to get Cache Card %d status, status:%d\n", i, status);
                }
                else
                {

                    fbe_cli_printf("Cache Card %d: %s\n", i, cacheCardInfo.inserted?"PRESENT":"REMOVED");
                }
            }
        }

    }
    else
    {
        fbe_cli_printf("\n%s [%s]   PEER SP: %s\n",
                decodeSpId(local_sp),
                board_info.is_active_sp?"Active SP":"Passive SP",
                board_info.peerPresent?"PRESENT":"REMOVED");
        // print more details of peer when peer SP is not "present"
        if (!board_info.peerPresent) 
        {
            if (board_info.peerInserted) 
            {
                status = fbe_api_esp_board_mgmt_getPeerBootInfo(&peerBootInfo);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed to get peer boot info, status:%d\n", status);
                }
                else
                {
                    fbe_cli_printf("Peer Boot Info:\n");
                    fbe_cli_printf("  Peer SP App Running   : %s\n", BOOLEAN_TO_TEXT(peerBootInfo.isPeerSpApplicationRunning));
                    fbe_cli_printf("  Peer boot state       : %s(%d)\n", fbe_pbl_decodeBootState(peerBootInfo.peerBootState), peerBootInfo.peerBootState);
                    fbe_cli_printf("  BIOS/POST fail        : %s\n\n", BOOLEAN_TO_TEXT(peerBootInfo.biosPostFail));
                    fbe_cli_printf("  FSR(Fault Status Register) Info:\n");
                    fbe_cli_print_flt_reg_status(&peerBootInfo.fltRegStatus);
                }

                fbe_cli_display_cmi_info();
            }
            else
            {
                fbe_cli_printf("Peer SP not inserted!\n");
            }
        }
    }
    //end display board info

    //display hw cache status
    fbe_cli_display_hw_cache_status();

    //display physical memory
    fbe_cli_display_sp_physical_memory(&board_info);

    //display fault status
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    if (enclosure_type == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }

    fbe_zero_memory(&encl_info, sizeof(fbe_esp_encl_mgmt_encl_info_t));
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Encl info, status:%d\n", status);
        return;
    }

    //display sp sps status
    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    for( spsIndex = 0; spsIndex < spsCountInfo.spsPerEnclosure; spsIndex++ )
    {
        location.slot = spsIndex;
        fbe_cli_display_sps_status(&location);
    }
    //end display sp sps status


    //display DPE/XPE info

    fbe_set_memory(mgmt_module_info_buffer, 0, sizeof(FBE_CLI_SP_CMD_STRING_LEN));

    //fbe_cli_printf("\n%3.3s Info\n", decodeEnclosureType(enclosure_type));


    // display Mgmt Modules if any, XPE only
    if(enclosure_type == XPE_ENCLOSURE_TYPE)
    {
        fbe_set_memory(mgmt_module_info_buffer, 0, sizeof(FBE_CLI_SP_CMD_STRING_LEN));
        for (sp = SP_A; sp < SP_ID_MAX; sp++) 
        {
            fbe_cli_get_management_module_info(sp, mgmt_module_info_buffer);
        }

        if(strlen(mgmt_module_info_buffer) > 0)
        {
            fbe_cli_printf("  Mgmt Modules\t");
            fbe_cli_printf("  %s", mgmt_module_info_buffer);
        }
        fbe_cli_printf("\n");

        fbe_cli_printf("  enclFaultLedStatus:%s\n",fbe_base_env_decode_led_status(encl_info.enclFaultLedStatus));
        fbe_cli_printf("  enclFaultLedReason:%s\n",fbe_base_env_decode_encl_fault_led_reason(encl_info.enclFaultLedReason));
    }

    //get enclosure info for display ps and fan status
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    if(enclosure_type == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        //display XPE Pworer supplies
        fbe_cli_printf("  Power Supplies:\n");
        fbe_cli_display_xpe_ps_status(&location, encl_info.psCount);
        //end display Power supplies

        //display fan status
        if(encl_info.fanCount != 0)
        {
            fbe_cli_printf("  Fans:\n");
            fbe_cli_display_xpe_fan_status(&location, encl_info.fanCount);
        }
        //end display fan status
    }

    //TODO: display suit case

    //display dae info
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    for (bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus ++)
    {
        location.bus = bus;
        fbe_cli_display_encl_info_on_bus(&location, enclosure_type);
    }
    //end display dae info
}

/*!***************************************************************
 * @fn fbe_cli_displaySpsMgmtCacheStatusInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed SPS_MGMT Cache Status info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displaySpsMgmtCacheStatusInfo(fbe_bool_t displayAll)
{
    fbe_common_cache_status_t           cacheStatus;
    fbe_esp_sps_mgmt_spsStatus_t        spsInfo;
    fbe_esp_sps_mgmt_bobStatus_t        bbuInfo;
    fbe_esp_board_mgmt_board_info_t     boardInfo;
    fbe_lcc_info_t                      lccInfo;
    fbe_board_mgmt_misc_info_t          miscInfo;
    fbe_esp_sps_mgmt_spsCounts_t        spsCountInfo;
    fbe_u32_t                           enclIndex;
    fbe_u32_t                           spsIndex;
    fbe_u32_t                           bbuIndex, bbuCount;
    fbe_u32_t                           lccIndex, lccCount;
    fbe_object_id_t                     objectId;
    fbe_status_t                        status;
    fbe_device_physical_location_t      location;
    fbe_bool_t                          diskBatteryBackedSet;

    // get overall CacheStatus
    status = fbe_api_esp_sps_mgmt_getCacheStatus(&cacheStatus);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\tSPS_MGMT CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }
    else
    {
        fbe_cli_printf("%s: Error when get sps_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return;
    }

    // if faulted, display detailed info
    if (displayAll || (cacheStatus != FBE_CACHE_STATUS_OK))
    {
        status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error in get sps count, status: %d\n", __FUNCTION__, status);
            return;
        }

        // if array has SPS's, display SPS info
        if (spsCountInfo.totalNumberOfSps > 0)
        {
            status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
                return;
            }

            for (enclIndex = 0; enclIndex < spsCountInfo.enclosuresWithSps; enclIndex++)
            {
                fbe_set_memory(&spsInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
                if ((boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) &&
                    (enclIndex == 0))
                {
                    spsInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
                    spsInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    fbe_cli_printf("\t\tSPE\n");
                }
                else
                {
                    fbe_cli_printf("\t\tENCL %d_%d\n",
                                   spsInfo.spsLocation.bus,
                                   spsInfo.spsLocation.enclosure);
                }
                for (spsIndex = 0; spsIndex < spsCountInfo.spsPerEnclosure; spsIndex++)
                {
                    spsInfo.spsLocation.slot = spsIndex;
                    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsInfo);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_cli_printf("%s: Error in get sps status, status: %d\n", __FUNCTION__, status);
                        return;
                    }
                    fbe_cli_printf("\t\t\tSPS %d, Status: %s, Cabling: %s, Faults: %s\n",
                                   spsInfo.spsLocation.slot,
                                   decodeSpsState(spsInfo.spsStatus),
                                   fbe_cli_print_SpsCablingStatus(spsInfo.spsCablingStatus),
                                   fbe_cli_print_SpsFaults(&spsInfo.spsFaultInfo));
                }
            }
        }

        status = fbe_api_esp_sps_mgmt_getBobCount(&bbuCount);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error in get BBU count, status: %d\n", __FUNCTION__, status);
            return;
        }

        // if array has BBU's, display BBU info
        if (bbuCount > 0)
        {
            fbe_set_memory(&bbuInfo, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
            for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
            {
                bbuInfo.bobIndex = bbuIndex;
                status = fbe_api_esp_sps_mgmt_getBobStatus(&bbuInfo);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("%s: Error in get bbu status, status: %d\n", __FUNCTION__, status);
                    return;
                }
                fbe_cli_printf("\t\tBBU %d\n", bbuIndex);
                fbe_cli_printf("\t\t\tInserted: %d, State: %s, ",
                                   bbuInfo.bobInfo.batteryInserted,
                                   fbe_sps_mgmt_getBobStateString(bbuInfo.bobInfo.batteryState));
                if (bbuInfo.bobInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
                {
                    fbe_cli_printf("Fault:EnvIntfError %s\n",
                                   fbe_base_env_decode_env_status(bbuInfo.bobInfo.envInterfaceStatus));
                }
                else
                {
                    fbe_cli_printf("Fault:%s\n",
                                   fbe_sps_mgmt_getBobFaultString(bbuInfo.bobInfo.batteryFaults));
                }
            }

            // additional BBU related status
            // get status of setting Drive BatteryBacked attribute
            status = fbe_api_get_enclosure_object_id_by_location(0,0, &objectId);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error in getEnclObjIdByLoc, status: %d\n", __FUNCTION__, status);
                return;
            }

            status = fbe_api_enclosure_get_disk_battery_backed_status(objectId, &diskBatteryBackedSet);
            if (status == FBE_STATUS_OK)
            {
                fbe_cli_printf("\t\tDiskBatteryBacked : %s\n", 
                               (diskBatteryBackedSet ? "VALID" : "INVALID"));
            }
            else
            {
                fbe_cli_printf("%s: Error in getDiskBatteryBackedStatus, status: %d\n", __FUNCTION__, status);
                return;
            }

            // get LCC info
            fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
            status = fbe_api_enclosure_get_lcc_count(objectId, &lccCount);
            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("%s: Error in getLccCount, status: %d\n", __FUNCTION__, status);
                return;
            }

            fbe_cli_printf("\t\tBase Module Drive ECB Status\n");
            for (lccIndex = 0; lccIndex < lccCount; lccIndex++)
            {
                status = fbe_api_enclosure_get_lcc_info(&location, &lccInfo);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_cli_printf("%s: Error in getLccInfo, status: %d\n", __FUNCTION__, status);
                    return;
                }
                fbe_cli_printf("\t\t\tLCC %d, ecbFault %d\n", 
                               lccIndex, lccInfo.ecbFault);
            }

            status = fbe_api_get_board_object_id(&objectId);
            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("%s: Error getBoardObjId, status: %d\n", __FUNCTION__, status);
                return;
            }
            status = fbe_api_board_get_misc_info(objectId, &miscInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_cli_printf("%s: Error in getMiscInfo, status: %d\n", __FUNCTION__, status);
                return;
            }
            fbe_cli_printf("\t\tBase Module Power ECB Status\n");
            fbe_cli_printf("\t\t\tLocalEcbFault %d, peerEcbFault %d\n",
                           miscInfo.localPowerECBFault, miscInfo.peerPowerECBFault);

        }
        
    }   // end of if (cacheStatus != FBE_CACHE_STATUS_OK)

}   // end of fbe_cli_displaySpsMgmtCacheStatusInfo

/*!***************************************************************
 * @fn fbe_cli_displayPsMgmtPsInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed PS_MGMT PS info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayPsMgmtPsInfo(fbe_device_physical_location_t *psLocation)
{
    fbe_u32_t                           psIndex, psCount;
    fbe_status_t                        status;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;

    // get PS count
    status = fbe_api_esp_ps_mgmt_getPsCount(psLocation, &psCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Get ps count fail, status: %d.\n", __FUNCTION__, status);
        return;
    }

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    psInfo.location = *psLocation;
    for (psIndex = 0; psIndex < psCount; psIndex++)
    {
        psInfo.location.slot = psIndex;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get ps info fail, status: %d.\n", __FUNCTION__, status);
            return;
        }
        if (psInfo.psInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_cli_printf("\t\t\tPS %d, Fault:EnvIntfError %s\n",
                           psIndex,
                           fbe_base_env_decode_env_status(psInfo.psInfo.envInterfaceStatus));
        }
        else
        {
            fbe_cli_printf("\t\t\tPS %d, Insert: %d, Supported %d, GenFlt: %d, AcFail %d\n",
                           psIndex,
                           psInfo.psInfo.bInserted,
                           psInfo.psInfo.psSupported,
                           psInfo.psInfo.generalFault,
                           psInfo.psInfo.ACFail);
        }

    }

}   // end of fbe_cli_displayPsMgmtPsInfo

/*!***************************************************************
 * @fn fbe_cli_displayPsMgmtCacheStatusInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed PS_MGMT Cache Status info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayPsMgmtCacheStatusInfo(fbe_bool_t displayAll)
{
    fbe_common_cache_status_t           cacheStatus;
    fbe_status_t                        status;
    fbe_device_physical_location_t      location;
    fbe_esp_board_mgmt_board_info_t     boardInfo;

    // get overall CacheStatus
    status = fbe_api_esp_ps_mgmt_getCacheStatus(&cacheStatus);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\tPS_MGMT CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }
    else
    {
        fbe_cli_printf("%s: Error when get ps_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return;
    }

    // if faulted, display detailed info
    if (displayAll || (cacheStatus != FBE_CACHE_STATUS_OK))
    {

        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
            return;
        }

        // determine if there is an xPE
        if (boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
        {
            fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
            fbe_cli_printf("\t\tSPE\n");
            fbe_cli_displayPsMgmtPsInfo(&location);
        }

        // get PS info for enclosure 0_0 (DPE or DAE0)
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        fbe_cli_printf("\t\tENCL %d_%d\n",
                       location.bus,
                       location.enclosure);
        fbe_cli_displayPsMgmtPsInfo(&location);

    }   // end of if (cacheStatus != FBE_CACHE_STATUS_OK)

}   // end of fbe_cli_displayPsMgmtCacheStatusInfo


/*!***************************************************************
 * @fn fbe_cli_displayEnclMgmtCacheStatusInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed ENCL_MGMT Cache Status info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayEnclMgmtCacheStatusInfo(fbe_bool_t displayAll)
{
    fbe_common_cache_status_t           cacheStatus;
    fbe_status_t                        status;
    fbe_device_physical_location_t      location;
    fbe_esp_board_mgmt_board_info_t     boardInfo;
    fbe_esp_encl_mgmt_encl_info_t       enclInfo;

    // get overall CacheStatus
    status = fbe_api_esp_encl_mgmt_getCacheStatus(&cacheStatus);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\tENCL_MGMT CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }
    else
    {
        fbe_cli_printf("%s: Error when get encl_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return;
    }

    // if faulted, display detailed info
    if (displayAll || (cacheStatus != FBE_CACHE_STATUS_OK))
    {

        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
            return;
        }

        // determine if there is an xPE
        if (boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
        {
            fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
            fbe_cli_printf("\t\tSPE\n");
            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Error when get encl info, status: %d.\n", __FUNCTION__, status);
                return;
            }
            fbe_cli_printf("\t\t\tShutdownReason: %s\n",
                           fbe_encl_mgmt_get_enclShutdownReasonString(enclInfo.shutdownReason));
        }

        // get PS info for enclosure 0_0 (DPE or DAE0)
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        fbe_cli_printf("\t\tENCL %d_%d\n",
                       location.bus,
                       location.enclosure);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get encl info, status: %d.\n", __FUNCTION__, status);
            return;
        }
        fbe_cli_printf("\t\t\tShutdownReason: %s\n",
                       fbe_encl_mgmt_get_enclShutdownReasonString(enclInfo.shutdownReason));

    }   // end of if (cacheStatus != FBE_CACHE_STATUS_OK)

}   // end of fbe_cli_displayEnclMgmtCacheStatusInfo

/*!***************************************************************
 * @fn fbe_cli_displayBoardMgmtSuitcaseInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed BOARD_MGMT Suitcase info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayBoardMgmtSuitcaseInfo(fbe_device_physical_location_t *suitcaseLocation)
{
    fbe_status_t                        status;
    fbe_esp_board_mgmt_suitcaseInfo_t   suitcaseInfo;
    fbe_u32_t                           suitcaseIndex, suitcaseCount;

    if ((suitcaseLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
        (suitcaseLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
    {
        fbe_cli_printf("\t\tSPE, %s\n",
                       (suitcaseLocation->sp == SP_A ? "SPA" : "SPB"));
    }
    else
    {
        fbe_cli_printf("\t\tENCL %d_%d, %s\n",
                       suitcaseLocation->bus,
                       suitcaseLocation->enclosure,
                       (suitcaseLocation->sp == SP_A ? "SPA" : "SPB"));
    }

    suitcaseInfo.location = *suitcaseLocation;
    suitcaseInfo.location.slot = 0;
    status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get suitcase info, status: %d.\n", __FUNCTION__, status);
        return;
    }

    suitcaseCount = suitcaseInfo.suitcaseCount;
    for (suitcaseIndex = 0; suitcaseIndex < suitcaseCount; suitcaseIndex++)
    {
        suitcaseInfo.location.slot = suitcaseIndex;
        status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get suitcase info, status: %d.\n", __FUNCTION__, status);
            return;
        }
        if (suitcaseInfo.suitcaseInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_cli_printf("\t\t\tSuitcase %d, EnvIntfError %s\n",
                           suitcaseIndex,
                           fbe_base_env_decode_env_status(suitcaseInfo.suitcaseInfo.envInterfaceStatus));
        }
        else
        {
            fbe_cli_printf("\t\t\tSuitcase %d, ShutdownWarning %d, ambientOvertempWarning %d\n",
                           suitcaseIndex,
                           suitcaseInfo.suitcaseInfo.shutdownWarning,
                           suitcaseInfo.suitcaseInfo.ambientOvertempWarning);
        }
    }

}   // end of fbe_cli_displayBoardMgmtSuitcaseInfo

/*!***************************************************************
 * @fn fbe_cli_displayBoardMgmtCacheStatusInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed BOARD_MGMT Cache Status info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayBoardMgmtCacheStatusInfo(fbe_bool_t displayAll)
{
    fbe_common_cache_status_t           cacheStatus;
    fbe_status_t                        status;
    fbe_esp_board_mgmt_board_info_t     boardInfo;
    fbe_device_physical_location_t      location;

    // get overall CacheStatus
    status = fbe_api_esp_board_mgmt_getCacheStatus(&cacheStatus);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\tBOARD_MGMT CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }
    else
    {
        fbe_cli_printf("%s: Error when get board_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return;
    }

    // if faulted, display detailed info
    if (displayAll || (cacheStatus != FBE_CACHE_STATUS_OK))
    {

        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
            return;
        }

        // determine if there is an xPE
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        if (boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
        {
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }

        // get Suitcase info for both SP's
        location.sp = SP_A;
        fbe_cli_displayBoardMgmtSuitcaseInfo(&location);
        location.sp = SP_B;
        fbe_cli_displayBoardMgmtSuitcaseInfo(&location);

        // display peer SP status
        fbe_cli_printf("\t\tPeer SP status, inserted : %d, present : %d\n",
                       boardInfo.peerInserted, boardInfo.peerPresent);

    }   // end of if (cacheStatus != FBE_CACHE_STATUS_OK)

}   // end of fbe_cli_displayBoardMgmtCacheStatusInfo


/*!***************************************************************
 * @fn fbe_cli_displayCoolingMgmtFanInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed COOLING_MGMT Fan info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayCoolingMgmtFanInfo(fbe_device_physical_location_t *fanLocation)
{
    fbe_status_t                        status;
    fbe_esp_cooling_mgmt_fan_info_t     fanInfo;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(fanLocation, &fanInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get fan info, status: %d.\n", __FUNCTION__, status);
        return;
    }
    if (fanInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        fbe_cli_printf("\t\t\tFan %d, EnvIntfError %s\n",
                       fanLocation->slot,
                       fbe_base_env_decode_env_status(fanInfo.envInterfaceStatus));
    }
    else
    {
        fbe_cli_printf("\t\t\tFan %d, multiFanModuleFaulted %d\n",
                       fanLocation->slot,
                       fanInfo.multiFanModuleFaulted);
    }

}   // end of fbe_cli_displayBoardMgmtSuitcaseInfo

/*!***************************************************************
 * @fn fbe_cli_displayCoolingMgmtCacheStatusInfo()
 ****************************************************************
 * @brief
 *  This function displays detailed COOLING_MGMT Cache Status info.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_displayCoolingMgmtCacheStatusInfo(fbe_bool_t displayAll)
{
    fbe_common_cache_status_t           cacheStatus;
    fbe_status_t                        status;
    fbe_esp_board_mgmt_board_info_t     boardInfo;
    fbe_esp_encl_mgmt_encl_info_t       enclInfo;
    fbe_device_physical_location_t      location;
    fbe_u32_t                           fanIndex;

    // get overall CacheStatus
    status = fbe_api_esp_cooling_mgmt_getCacheStatus(&cacheStatus);
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\tCOOLING_MGMT CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }
    else
    {
        fbe_cli_printf("%s: Error when get cooling_mgmt cacheStatus, status %d.\n", __FUNCTION__, status);
        return;
    }

    // if faulted, display detailed info
    if (displayAll || (cacheStatus != FBE_CACHE_STATUS_OK))
    {

        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error when get board management info, status: %d.\n", __FUNCTION__, status);
            return;
        }

        // determine if there is an xPE
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        if (boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
        {
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
            fbe_cli_printf("\t\tSPE\n");
        }
        else
        {
            fbe_cli_printf("\t\tENCL %d_%d\n",
                           location.bus,
                           location.enclosure);
        }

        // get fan count
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);

        // get individual fan status
        for (fanIndex = 0; fanIndex < enclInfo.fanCount; fanIndex++)
        {
            location.slot = fanIndex;
            fbe_cli_displayCoolingMgmtFanInfo(&location);
        }

    }   // end of if (cacheStatus != FBE_CACHE_STATUS_OK)

}   // end of fbe_cli_displayCoolingMgmtCacheStatusInfo

/*!***************************************************************
 * @fn fbe_cli_cmd_espcs()
 ****************************************************************
 * @brief
 *  This function displays detailed ESP Cache Status from
 *  individual ESP objects.
 *
 * @param
 *   None
 *
 * @return
 *  None
 *
 * @version
 *  22-Mar-2013: Joe Perry - Created.
 *
 ****************************************************************/
void fbe_cli_cmd_espcs(int argc , char ** argv)
{
    fbe_common_cache_status_info_t  espCacheStatusInfo;
    fbe_status_t                    status;
    fbe_bool_t                      displayAll = FALSE;
    fbe_bool_t                      is_service_mode = FALSE;

    if (argc >= 2)
    {
        fbe_cli_printf("%s", ESPCS_CMD_USAGE);
        return;
    }

    if ((argc == 1 ) && (strcmp(*argv, "-all") == 0))
    {
        displayAll = TRUE;
    }

    fbe_api_esp_common_get_is_service_mode(FBE_CLASS_ID_ENCL_MGMT, &is_service_mode);

    if (is_service_mode == TRUE) 
    {
        fbe_cli_printf("**This SP is currently running in SERVICE MODE.**\n");
    }

    fbe_cli_printf("Detailed ESP CacheStatus Info\n");

    //display hw cache status
    status = fbe_api_esp_common_getCacheStatus(&espCacheStatusInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in get hw cache status, error code: %d\n", __FUNCTION__, status);
        return;
    }
    fbe_cli_printf("Overall CacheStatus: %s\n", fbe_cli_print_HwCacheStatus(espCacheStatusInfo.cacheStatus));

    if (displayAll || (espCacheStatusInfo.cacheStatus != FBE_CACHE_STATUS_OK))
    {
        // get sps_mgmt info
        fbe_cli_displaySpsMgmtCacheStatusInfo(displayAll);

        // get ps_mgmt info
        fbe_cli_displayPsMgmtCacheStatusInfo(displayAll);

        // get encl_mgmt info
        fbe_cli_displayEnclMgmtCacheStatusInfo(displayAll);

        // get board_mgmt info
        fbe_cli_displayBoardMgmtCacheStatusInfo(displayAll);

        // get cooling_mgmt info
        fbe_cli_displayCoolingMgmtCacheStatusInfo(displayAll);

    }
    fbe_cli_printf("\n");

}   // end of fbe_cli_cmd_espcs





