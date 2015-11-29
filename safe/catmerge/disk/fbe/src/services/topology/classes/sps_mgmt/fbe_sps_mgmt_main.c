/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sps_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the SPS
 *  Management object. This includes the create and
 *  destroy methods for the SPS management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup sps_mgmt_class_files
 * 
 * @version
 *   02-Mar-2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_sps_mgmt_private.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_registry.h"
#include "fbe_base_package.h"
#include "fbe_base_environment_debug.h"

/* Class methods forward declaration */
fbe_status_t fbe_sps_mgmt_load(void);
fbe_status_t fbe_sps_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sps_mgmt_class_methods = { FBE_CLASS_ID_SPS_MGMT,
                                                    fbe_sps_mgmt_load,
                                                    fbe_sps_mgmt_unload,
                                                    fbe_sps_mgmt_create_object,
                                                    fbe_sps_mgmt_destroy_object,
                                                    fbe_sps_mgmt_control_entry,
                                                    fbe_sps_mgmt_event_entry,
                                                    NULL,
                                                    fbe_sps_mgmt_monitor_entry
                                                    };

#define EspSimulateBobKey        "EspSimulateBob"

/*!***************************************************************
 * fbe_sps_mgmt_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SPS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sps_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_sps_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_sps_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_unload()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SPS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sps_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_create_object()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_sps_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_sps_mgmt_t * sps_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sps_mgmt = (fbe_sps_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sps_mgmt, FBE_CLASS_ID_SPS_MGMT);
    fbe_sps_mgmt_init(sps_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sps_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_destroy_object()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_sps_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sps_mgmt_t * sps_mgmt;

    sps_mgmt = (fbe_sps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)sps_mgmt, sps_mgmt->arraySpsInfo);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)sps_mgmt, sps_mgmt->arrayBobInfo);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)sps_mgmt, sps_mgmt->spsEirData);

    if(sps_mgmt->arraySpsFupInfo != NULL) 
    {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)sps_mgmt, sps_mgmt->arraySpsFupInfo);
        sps_mgmt->arraySpsFupInfo = NULL;
    }

    /* TODO - Deregister FBE API notifications, free up
     * any memory that was allocted here 
     */
    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_sps_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context)
{
    fbe_sps_mgmt_t * sps_mgmt;

    sps_mgmt = (fbe_sps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sps_mgmt_event_entry()
 ******************************************/


/*!***************************************************************
 * fbe_sps_mgmt_setSpsNameString()
 ****************************************************************
 * @brief
 *  This function determine which SPS this is (SPSA or SPSB) by
 *  getting the SPID.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 * @param spsIndex - index to an SPS.
 *
 * @return None
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_setSpsNameString (fbe_sps_mgmt_t *sps_mgmt, fbe_u8_t enclIndex, fbe_u8_t spsIndex)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_board_mgmt_misc_info_t      boardMiscInfo;

    strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "UNKN", FBE_SPS_MGMT_SPS_STRING_LEN-1);
    sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString[FBE_SPS_MGMT_SPS_STRING_LEN-1] = '\0';

    // get SPID info
    status = fbe_api_board_get_misc_info(sps_mgmt->object_id, &boardMiscInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Misc Info, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        if (((boardMiscInfo.spid == SP_A) && (spsIndex == FBE_LOCAL_SPS)) ||
            ((boardMiscInfo.spid == SP_B) && (spsIndex == FBE_PEER_SPS)))
        {
            if (enclIndex == FBE_SPS_MGMT_PE)
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "PE_SPSA", 7);
            }
            else if (enclIndex == FBE_SPS_MGMT_DAE0)
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "DAE0_SPSA", 9);
            }
            else
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "INV_SPSA", 8);
            }
        }
        else if (((boardMiscInfo.spid == SP_B) && (spsIndex == FBE_LOCAL_SPS)) ||
            ((boardMiscInfo.spid == SP_A) && (spsIndex == FBE_PEER_SPS)))
        {
            if (enclIndex == FBE_SPS_MGMT_PE)
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "PE_SPSB", 7);
            }
            else if (enclIndex == FBE_SPS_MGMT_DAE0)
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "DAE0_SPSB", 9);
            }
            else
            {
                strncpy(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsNameString, "INV_SPSB", 8);
            }
        }
    }

}
/******************************************
 * end fbe_sps_mgmt_setSpsNameString()
 ******************************************/


/*!***************************************************************
 * fbe_sps_mgmt_setBobNameString()
 ****************************************************************
 * @brief
 *  This function determine which SPS this is (SPSA or SPSB) by
 *  getting the SPID.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 * @param spsIndex - index to an SPS.
 *
 * @return None
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_setBobNameString (fbe_sps_mgmt_t *sps_mgmt, fbe_u8_t enclIndex, fbe_u8_t bobIndex)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_device_physical_location_t          location = {0};

    strncpy(sps_mgmt->arrayBobInfo->bobNameString[bobIndex], "UNKN", FBE_SPS_MGMT_BOB_STRING_LEN-1);
    sps_mgmt->arrayBobInfo->bobNameString[bobIndex][FBE_SPS_MGMT_BOB_STRING_LEN-1] = '\0';

    location.bus = 0;
    location.enclosure = 0;
    location.slot = bobIndex;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string for BoB.\n", 
                              __FUNCTION__); 

        return status;
    }

    strncpy(sps_mgmt->arrayBobInfo->bobNameString[bobIndex], deviceStr, FBE_SPS_MGMT_BOB_STRING_LEN);
    sps_mgmt->arrayBobInfo->bobNameString[bobIndex][FBE_SPS_MGMT_BOB_STRING_LEN-1] = '\0';
    return  FBE_STATUS_OK;
}

/******************************************
 * end fbe_sps_mgmt_setBobNameString()
 ******************************************/

/*!***************************************************************
 * @fn      fbe_sps_mgmt_getRegistrySimulateBob()
 ****************************************************************
 * @brief
 *  This function reads the Registry parameter for simulating
 *  BoB's in ESP.
 *
 * @param simulateBob_p : Buffer to store registry value.
 * 
 * @return fbe_status_t  
 *
 * @author
 *  20-Aug-2012:    Joe Perry - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sps_mgmt_getRegistrySimulateBob(fbe_u32_t *simulateBob_p)
{
    fbe_u32_t espSimulateBob = FALSE;   
    fbe_u8_t defaultSimulateBob =0;
    fbe_status_t status;

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               EspSimulateBobKey, 
                               &espSimulateBob,
                               sizeof(espSimulateBob),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultSimulateBob,
                               0,
                               FALSE);
    if(status == FBE_STATUS_OK)
    {
        *simulateBob_p = espSimulateBob;
    }
	
    fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s, status %d, *simulateBob_p %d\n",
                            __FUNCTION__, status, *simulateBob_p);
    	
    return status;
}
/***********************************
    end fbe_sps_mgmt_getRegistrySimulateBob()    
************************************/

/*!***************************************************************
 * fbe_sps_mgmt_init()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_init(fbe_sps_mgmt_t * sps_mgmt)
{
    fbe_u8_t                                spsIndex, bobIndex;
    fbe_u8_t                                enclIndex, psIndex;
    fbe_status_t                            status;
    fbe_board_mgmt_platform_info_t          platformInfo;
    fbe_u32_t                               persistent_storage_data_size = 0;
    
    fbe_topology_class_trace(FBE_CLASS_ID_SPS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    // allocate buffers for SPS & BoB info
    sps_mgmt->arraySpsInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)sps_mgmt, 
                                                             sizeof(fbe_array_sps_info_t));
    
    sps_mgmt->arrayBobInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)sps_mgmt, 
                                                             sizeof(fbe_array_bob_info_t));

    sps_mgmt->arraySpsFupInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)sps_mgmt, 
                                                             sizeof(fbe_array_sps_fup_info_t));
    if(sps_mgmt->arraySpsFupInfo != NULL) 
    {
        fbe_zero_memory(sps_mgmt->arraySpsFupInfo, sizeof(fbe_array_sps_fup_info_t));
    }
    

    /*
     * SPS-BOB usage differs based on platform types
     */

    // need to know platform info (CPU Moduled)
    status = fbe_api_board_get_platform_info(sps_mgmt->object_id, &platformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get platform info, status: 0x%X",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    switch (platformInfo.hw_platform_info.midplaneType)
    {
    case TELESTO_MIDPLANE:
    case SINOPE_MIDPLANE:
    case MIRANDA_MIDPLANE:
    case RHEA_MIDPLANE:
        // Moon platforms only have BBU's & we can get count from PhyPkg
        sps_mgmt->total_sps_count = 0;
        sps_mgmt->sps_per_side = 0;
        sps_mgmt->encls_with_sps = 0;
        sps_mgmt->total_bob_count = FBE_SPS_MGMT_BBU_COUNT_TBD;     // will get actual count at Discovery time
        break;
        
    case RATCHET_MIDPLANE:
    case PROTEUS_MIDPLANE:
        // 4 SPS's (2 for xPE, 2 for DAE0)
        sps_mgmt->total_sps_count = 4;
        sps_mgmt->sps_per_side = 2;
        sps_mgmt->encls_with_sps = 2;
        sps_mgmt->total_bob_count = 0;
        break;
    case STRATOSPHERE_MIDPLANE:
#ifdef C4_INTEGRATED
    case STEELJAW_MIDPLANE:
    case RAMHORN_MIDPLANE:
    case KNOT_MIDPLANE:
    case BOXWOOD_MIDPLANE:
    case CITADEL_MIDPLANE:
    case BUNKER_MIDPLANE:
#endif /* C4_INTEGRATED - C4HW */
        if (platformInfo.is_single_sp_system)
        {
            // 1 BOB's
            sps_mgmt->total_sps_count = 0;
            sps_mgmt->sps_per_side = 0;
            sps_mgmt->encls_with_sps = 0;
            sps_mgmt->total_bob_count = 1;
        }
        else
        {
            // 2 BOB's
            sps_mgmt->total_sps_count = 0;
            sps_mgmt->sps_per_side = 0;
            sps_mgmt->encls_with_sps = 0;
            sps_mgmt->total_bob_count = 2;
        }
        break;
    case MERIDIAN_MIDPLANE:
        sps_mgmt->total_sps_count = 0;
        sps_mgmt->sps_per_side = 0;
        sps_mgmt->encls_with_sps = 0;
        sps_mgmt->total_bob_count = 0;
        break;

/* A case for single SP BC will need to be added here with a total bob_count of 1. */

    default:
        // most arrays use 2 SPS's
        sps_mgmt->total_sps_count = 2;
        sps_mgmt->sps_per_side = 1;
        sps_mgmt->encls_with_sps = 1;
        sps_mgmt->total_bob_count = 0;
        break;
    }
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s midplaneType %d, spsCnt %d, bobCnt %d\n",
                          __FUNCTION__,
                          platformInfo.hw_platform_info.midplaneType,
                          sps_mgmt->total_sps_count,
                          sps_mgmt->total_bob_count);

    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
        {
            fbe_sps_mgmt_setSpsNameString(sps_mgmt, enclIndex, spsIndex);
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsCablingStatus = FBE_SPS_CABLING_UNKNOWN;
        }
    }

    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psInserted = FALSE;
        }
    }

    for (bobIndex = 0; bobIndex < FBE_BOB_MAX_COUNT; bobIndex++)
    {
        fbe_sps_mgmt_setBobNameString(sps_mgmt, enclIndex, bobIndex);
    }

    // setup SPS Persistent Storage
    persistent_storage_data_size = sizeof(fbe_sps_bbu_test_info_t);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sps_bbu_test_info_t) < FBE_PERSIST_DATA_BYTES_PER_ENTRY);
    fbe_base_environment_set_persistent_storage_data_size((fbe_base_environment_t *) sps_mgmt, 
                                                          persistent_storage_data_size);

    /* Set the condition to initialize persistent storage and read the data from disk. */
    status = fbe_lifecycle_set_cond(&fbe_sps_mgmt_lifecycle_const,
                                    (fbe_base_object_t*)sps_mgmt,
                                    FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE);

    sps_mgmt->arraySpsInfo->spsTestTimeDetected = FALSE;

    sps_mgmt->arraySpsInfo->spsBatteryOnlineCount = 0;
    sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFup = FBE_FALSE;
    sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount = 0;

    // set to failed until we know SPS status
    sps_mgmt->arraySpsInfo->spsCacheStatus = FBE_CACHE_STATUS_FAILED;

    // init EIR info (allocate memory for it)
    sps_mgmt->spsEirData = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)sps_mgmt, sizeof(fbe_sps_mgmt_eir_data_t));
    fbe_set_memory(sps_mgmt->spsEirData, 0, sizeof(fbe_sps_mgmt_eir_data_t));
    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        sps_mgmt->arraySpsInfo->psEnclCounts[enclIndex] = 0;
        for (spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
        {
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus = SPS_STATE_UNKNOWN;
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsModulePresent = FALSE;
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsBatteryPresent = FALSE;
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].testInProgress = FALSE;
            sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].lccFwActivationInProgress = FALSE;
    
            sps_mgmt->spsEirData->spsCurrentInputPower[enclIndex][spsIndex].status = FBE_ENERGY_STAT_INVALID;
            sps_mgmt->spsEirData->spsAverageInputPower[enclIndex][spsIndex].status = FBE_ENERGY_STAT_INVALID;
            sps_mgmt->spsEirData->spsMaxInputPower[enclIndex][spsIndex].status = FBE_ENERGY_STAT_INVALID;
        }
    }
    // init BBU Energy Requirements based on platform
    for (bobIndex = 0; bobIndex < FBE_BOB_MAX_COUNT; bobIndex++)
    {
        fbe_zero_memory(&sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement,
                        sizeof(BATTERY_ENERGY_REQUIREMENTS));
        sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryState = FBE_BATTERY_STATUS_UNKNOWN;
        sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryInserted = FALSE;
        switch (platformInfo.hw_platform_info.cpuModule)
        {
        case BEACHCOMBER_CPU_MODULE:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = TRUE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.format = 
                BATTERY_ENERGY_REQUIREMENTS_FORMAT_POWER_AND_TIME;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.batteryID = bobIndex;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.powerAndTime.power = 
                FBE_BEACHCOMBER_BBU_POWER_INIT_VAULE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.powerAndTime.time = 
                FBE_BEACHCOMBER_BBU_TIME_INIT_VAULE;
            break;
        case JETFIRE_CPU_MODULE:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = TRUE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.format = 
                BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.batteryID = bobIndex;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.energy = 
                FBE_JETFIRE_BBU_ENERGY_INIT_VAULE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.power = 
                FBE_JETFIRE_BBU_MAX_LOAD_INIT_VAULE;
            break;
        case OBERON_CPU_MODULE:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = TRUE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.format = 
                BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.batteryID = bobIndex;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.energy = 
                FBE_OBERON_BBU_ENERGY_INIT_VAULE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.power = 
                FBE_OBERON_BBU_MAX_LOAD_INIT_VAULE;
            break;
        case HYPERION_CPU_MODULE:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = TRUE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.format = 
                BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.batteryID = bobIndex;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.energy = 
                FBE_HYPERION_BBU_ENERGY_INIT_VAULE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.power = 
                FBE_HYPERION_BBU_MAX_LOAD_INIT_VAULE;
            break;
        case TRITON_CPU_MODULE:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = TRUE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.format = 
                BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.batteryID = bobIndex;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.energy = 
                FBE_TRITON_BBU_ENERGY_INIT_VAULE;
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].defaultEnergyRequirement.data.energyAndPower.power = 
                FBE_TRITON_BBU_MAX_LOAD_INIT_VAULE;
            break;
        default:
            sps_mgmt->arrayBobInfo->bob_info[bobIndex].setEnergyRequirementSupported = FALSE;
            break;
        }
    }

    // set other items
    sps_mgmt->arraySpsInfo->simulateSpsInfo = FALSE;
    sps_mgmt->arrayBobInfo->simulateBobInfo = FALSE;
    sps_mgmt->arrayBobInfo->localDiskBatteryBackedSet = FALSE;
    sps_mgmt->arrayBobInfo->peerDiskBatteryBackedSet = FALSE;
// JAP - temporary workaround for SPS/BoB shortages
    status = fbe_sps_mgmt_getRegistrySimulateBob(&sps_mgmt->arrayBobInfo->simulateBobInfo);
    // set SPS simulation to this too
    sps_mgmt->arraySpsInfo->simulateSpsInfo = sps_mgmt->arrayBobInfo->simulateBobInfo;
// JAP - temporary workaround for SPS/BoB shortages

    //Temoporary workaround for Kittyhawk on megatron
    #ifdef C4_INTEGRATED
    if (platformInfo.hw_platform_info.cpuModule == MEGATRON_CPU_MODULE)
    {
        sps_mgmt->arraySpsInfo->simulateSpsInfo = TRUE;
    }
    #endif

    // set SPS Test PS count based on array type & config
    if (sps_mgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        sps_mgmt->arraySpsInfo->spsTestConfig = FBE_SPS_TEST_CONFIG_DPE;
    }
    else if (sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
    {
        // may change at discovery time if DAE0 is a Voyager
        sps_mgmt->arraySpsInfo->spsTestConfig = FBE_SPS_TEST_CONFIG_XPE;
    }
    else
    {
        sps_mgmt->arraySpsInfo->spsTestConfig = FBE_SPS_TEST_CONFIG_UNKNOWN;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sps_mgmt_init()
 ******************************************/ 
