/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file marcus_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test LCC, PS and Fan counts and info
 *  in various platforms. If this is XPE platform, test for XPE and DAE0.
 *  If this is the DPE platform, test for DPE.
 *
 * @version
 *   24-Jan-2011 PHE - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_sim_server.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * marcus_short_desc = "Test LCC, PS and Fan counts in various platforms";
char * marcus_long_desc =
    "\n"
    "\n"
    "The edward and bella scenario tests LCC, PS and Fan counts and info in various platforms.\n"
    "It includes: \n"
    "    - Test LCC, PS and FAN counts and info in XPE and DAE0 for XPE platforms;\n"
    "    - Test LCC, PS and FAN counts and info in DPE platforms;\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] Spitfire board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 3 viper enclosures\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "\n"  
    "STEP 2: Verify the LCC, PS and FAN counts and info in XPE and DAE0 or DPE based on the platform type.\n"
    "STEP 3: Shutdown the Terminator/Physical package/ESP.\n"
    "STEP 4: Repeat step 1-3 for various platforms.\n"
    ;

typedef enum
{
    MARCUS_DPE,
    MARCUS_XPE,
    MARCUS_DAE,
} marcus_encl_type;

typedef struct marcus_test_encl_param_s{
    SPID_HW_TYPE        platformType; 
    marcus_encl_type    enclType;
    fbe_u32_t           lccCount; 
    fbe_u32_t           psCount; 
    fbe_u32_t           fanCount; 
    fbe_u32_t           bbuCount;
    fbe_u32_t           driveSlotCount;
}marcus_test_encl_param_t;

marcus_test_encl_param_t marcus_test_encl_table[] = 
{
    /* platform type, enclType, LCC count, PS Count, FAN count, BBU count, drive slot count*/
    {SPID_OBERON_1_HW_TYPE,   MARCUS_DPE, 0, 2, 10, 2, 12},  // DPE enclosure. We have not translate the FAN status for DPE in base board yet. 
    {SPID_HYPERION_1_HW_TYPE, MARCUS_DPE, 0, 4, 12, 4, 25},  // DPE enclosure. We have not translate the FAN status for DPE in base board yet. 
    {SPID_TRITON_1_HW_TYPE,   MARCUS_XPE, 0, 4, 14, 6, 0},   // XPE enclosure
    {SPID_TRITON_1_HW_TYPE,   MARCUS_DAE, 2, 2,  0, 0, 15},  // DAE0
};

#define MARCUS_TEST_MAX_ENCL_TABLE_INDEX     (sizeof(marcus_test_encl_table)/sizeof(marcus_test_encl_table[0]))

void marcus_test_verify_lcc_info(fbe_u32_t index,
                                  fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_encl_mgmt_lcc_info_t    lccInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Verify LCC(%d_%d_%d) Info ===", 
               pLocation->bus, pLocation->enclosure, pLocation->slot);
    status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(lccInfo.inserted, TRUE);
   
    return;
}

void marcus_test_verify_ps_info(fbe_u32_t index,
                                  fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_ps_mgmt_ps_info_t       getPsInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Verify PS(%d_%d_%d) info ===", 
               pLocation->bus, pLocation->enclosure, pLocation->slot);
    getPsInfo.location = *pLocation;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.bInserted, TRUE);
    
    return;
}


void marcus_test_verify_fan_info(fbe_u32_t index,
                                  fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_cooling_mgmt_fan_info_t fanInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Verify FAN(%d_%d_%d) info ===", 
               pLocation->bus, pLocation->enclosure, pLocation->slot);
    status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(fanInfo.inserted, FBE_MGMT_STATUS_TRUE);

    return;
}


void marcus_test_verify_bbu_info(fbe_u32_t index,
                                 fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_sps_mgmt_bobStatus_t            bobStatus;
    fbe_status_t                    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Verify BBU(%d_%d_%d) info ===", 
               pLocation->bus, pLocation->enclosure, pLocation->slot);
    fbe_zero_memory(&bobStatus, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    bobStatus.bobIndex = index;
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bobStatus);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(bobStatus.bobInfo.batteryInserted, TRUE);
    
    return;
}

void marcus_test_verify_drive_info(fbe_u32_t index,
                                  fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_drive_mgmt_drive_info_t getDriveInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Verify Drive(%d_%d_%d) info ===", 
               pLocation->bus, pLocation->enclosure, pLocation->slot);
    getDriveInfo.location = *pLocation;
    status = fbe_api_esp_drive_mgmt_get_drive_info(&getDriveInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.inserted, TRUE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.loop_a_valid, TRUE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.loop_b_valid, FALSE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.bypass_enabled_loop_a, FALSE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.bypass_enabled_loop_b, FALSE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.bypass_requested_loop_a, FALSE);
    MUT_ASSERT_INT_EQUAL(getDriveInfo.bypass_requested_loop_b, FALSE);

    return;
}

/*!**************************************************************
 * marcus_test_verify_encl_attr(SPID_HW_TYPE platformType) 
 ****************************************************************
 * @brief
 *  Verify the encl attributes.
 *
 * @param platformType -                
 *
 * @return None.
 *
 * @author
 *  24-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
void marcus_test_verify_encl_attr(void)
{
    fbe_u32_t           index = 0;
    fbe_u32_t           slot = 0;
    fbe_device_physical_location_t location = {0};
    fbe_esp_encl_mgmt_encl_info_t enclInfo = {0};
    fbe_esp_board_mgmt_board_info_t         boardInfo = {0};
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           bbuCount = 0;
    fbe_u32_t           deviceCount = 0;

    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);

    for(index = 0; index < MARCUS_TEST_MAX_ENCL_TABLE_INDEX; index ++) 
    {
        status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
                        boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

        if(marcus_test_encl_table[index].platformType == boardInfo.platform_info.platformType)
        {
            if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
            {
                location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                mut_printf(MUT_LOG_LOW, "=== Verify XPE attributes ===");
            }
            else
            {
                location.bus = 0;
                location.enclosure = 0;
                mut_printf(MUT_LOG_LOW, "=== Verify Encl 0_0 attributes ===");
            }

            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
            mut_printf(MUT_LOG_LOW, "=== Verify LCC count ===");
            mut_printf(MUT_LOG_LOW, "lccCount %d, table %d", enclInfo.lccCount, marcus_test_encl_table[index].lccCount);
            MUT_ASSERT_TRUE_MSG(enclInfo.lccCount == marcus_test_encl_table[index].lccCount, 
                                "LCC count is incorrect!");

            status = fbe_api_esp_encl_mgmt_get_lcc_count(&location, &deviceCount);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_TRUE_MSG(deviceCount == marcus_test_encl_table[index].lccCount, 
                                "LCC count is incorrect when using fbe_api_esp_encl_mgmt_get_lcc_count!");

            for(slot = 0; slot < enclInfo.lccCount; slot ++) 
            {
                location.slot = slot;
                marcus_test_verify_lcc_info(index, &location);
            }
        
            mut_printf(MUT_LOG_LOW, "=== Verify PS count ===");
            mut_printf(MUT_LOG_LOW, "psCount %d, table %d", enclInfo.psCount, marcus_test_encl_table[index].psCount);
            MUT_ASSERT_TRUE_MSG(enclInfo.psCount == marcus_test_encl_table[index].psCount, 
                                "PS count is incorrect!");

            status = fbe_api_esp_encl_mgmt_get_ps_count(&location, &deviceCount);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_TRUE_MSG(deviceCount == marcus_test_encl_table[index].psCount, 
                                "PS count is incorrect when using fbe_api_esp_encl_mgmt_get_ps_count!");
        
            for(slot = 0; slot < enclInfo.psCount; slot ++) 
            {
                location.slot = slot;
                marcus_test_verify_ps_info(index, &location);
            }
        
            mut_printf(MUT_LOG_LOW, "=== Verify FAN count ===");
          
            mut_printf(MUT_LOG_LOW, "fanCount %d, table %d", enclInfo.fanCount, marcus_test_encl_table[index].fanCount);
            MUT_ASSERT_TRUE_MSG(enclInfo.fanCount == marcus_test_encl_table[index].fanCount, 
                                "FAN count is incorrect!");

            status = fbe_api_esp_encl_mgmt_get_fan_count(&location, &deviceCount);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_TRUE_MSG(deviceCount == marcus_test_encl_table[index].fanCount, 
                                "FAN count is incorrect when using fbe_api_esp_encl_mgmt_get_fan_count!");
        
            for(slot = 0; slot < enclInfo.fanCount; slot ++) 
            {
                location.slot = slot;
                marcus_test_verify_fan_info(index, &location);
            }
        
            mut_printf(MUT_LOG_LOW, "=== Verify BBU count ===");
          
            if (marcus_test_encl_table[index].enclType == MARCUS_DAE)
            {
                bbuCount = 0;
            }
            else
            {
                status = fbe_api_esp_sps_mgmt_getBobCount(&bbuCount); 
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            MUT_ASSERT_TRUE_MSG(bbuCount == marcus_test_encl_table[index].bbuCount, 
                                "BBU count is incorrect when using fbe_api_esp_sps_mgmt_getBobCount!");
        
            for(slot = 0; slot < bbuCount; slot ++) 
            {
                location.slot = slot;
                marcus_test_verify_bbu_info(index, &location);
            }
        
            mut_printf(MUT_LOG_LOW, "=== Verify driveSlot count ===");
            mut_printf(MUT_LOG_LOW, "driveSlotCount %d, table %d", enclInfo.driveSlotCount, marcus_test_encl_table[index].fanCount);
            MUT_ASSERT_TRUE_MSG(enclInfo.driveSlotCount == marcus_test_encl_table[index].driveSlotCount, 
                                "driveSlot count is incorrect!");
        
            status = fbe_api_esp_encl_mgmt_get_drive_slot_count(&location, &deviceCount);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_TRUE_MSG(deviceCount == marcus_test_encl_table[index].driveSlotCount, 
                                "driveSlot count is incorrect when using fbe_api_esp_encl_mgmt_get_drive_slot_count!");

            for(slot = 0; slot < enclInfo.driveSlotCount; slot ++) 
            {
                location.slot = slot;
                //marcus_test_verify_drive_info(index, &location);
            }

            break;
        }
    }
   
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
       
    return;
}


/*!**************************************************************
 * marcus_test_load_test_env()
 ****************************************************************
 * @brief
 *  This function loads all the test environment.
 *
 * @param platform_type -               
 *
 * @return fbe_status_t -   
 *
 * @author
 *   24-Jan-2011: PHE Created. 
 *
 ****************************************************************/
static void marcus_test_load_test_env(SPID_HW_TYPE platform_type)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        platform_type,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);

    if(platform_type == SPID_HYPERION_1_HW_TYPE)
    {
        /* For for mgmt module configure the mgmt port speed */
        fbe_test_esp_check_mgmt_config_completion_event();
    }
    return;
}

void marcus_oberon_test(void)
{
    mut_printf(MUT_LOG_LOW, "=== Testing Oberon platform  ===\n");
    marcus_test_verify_encl_attr();

    return;
}

void marcus_hyperion_test(void)
{
    mut_printf(MUT_LOG_LOW, "=== Testing Hyperion platform  ===\n");
    marcus_test_verify_encl_attr();

    return;
}

void marcus_triton_test(void)
{
    mut_printf(MUT_LOG_LOW, "=== Testing Triton platform  ===\n");
    marcus_test_verify_encl_attr();

    return;
}

void marcus_oberon_setup(void)
{
    marcus_test_load_test_env(SPID_OBERON_1_HW_TYPE);
    return;
}

void marcus_hyperion_setup(void)
{
    marcus_test_load_test_env(SPID_HYPERION_1_HW_TYPE);
    return;
}

void marcus_triton_setup(void)
{
    marcus_test_load_test_env(SPID_TRITON_1_HW_TYPE);
    return;
}

void marcus_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}

/*************************
 * end file marcus_test.c
 *************************/

