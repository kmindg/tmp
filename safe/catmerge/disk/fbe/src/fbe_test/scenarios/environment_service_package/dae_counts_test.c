
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dae_counts_test.c
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
#include "fbe/fbe_api_esp_common_interface.h"
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

char * dae_counts_test_short_desc = "Test LCC, PS and Fan counts for various DAE types";
char * dae_counts_test_long_desc =
    "\n"
    "\n"
    "Verify LCC, PS and Fan counts and info for various DAE types.\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] Prometheus board \n"
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
    "STEP 2: For each DAE, verify the LCC, PS and FAN counts and info.\n"
    "STEP 3: Shutdown the Terminator/Physical package/ESP.\n"
    "STEP 4: Repeat step 1-3 for various platforms.\n"
    ;

extern mixed_encl_config_type_t encl_config_list[];

typedef struct dae_counts_test_encl_param_s{
    fbe_sas_enclosure_type_t    enclType;
    fbe_u32_t                   lccCount; 
    fbe_u32_t                   psCount; 
    fbe_u32_t                   fanCount; 
    fbe_u32_t                   driveSlotCount;
} dae_counts_test_encl_param_t;

dae_counts_test_encl_param_t dae_counts_test_encl_table[] = 
{
    // dae type,           LCC count, PS Count, FAN count, drive slot count*/
    {FBE_SAS_ENCLOSURE_TYPE_VIPER, 2, 2, 0, 15},
    {FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP, 2, 4, 10, 120},
    {FBE_SAS_ENCLOSURE_TYPE_DERRINGER, 2, 2, 0, 25},
    {FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM, 4, 2, 3, 60},
    {FBE_SAS_ENCLOSURE_TYPE_ANCHO, 2, 2, 0, 15},
    {FBE_SAS_ENCLOSURE_TYPE_TABASCO, 2, 2, 0, 25},
    {FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP, 2, 4, 10, 120},
    {FBE_SAS_ENCLOSURE_TYPE_PINECONE, 2, 2, 0, 12},
    //{FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP, 2, 2, 3, 60},
    
};

#define DAE_COUNTS_TEST_MAX_ENCL_TABLE_INDEX     (sizeof(dae_counts_test_encl_table)/sizeof(dae_counts_test_encl_table[0]))

fbe_u32_t dae_type_to_counts_table_index(fbe_sas_enclosure_type_t enclType)
{
    fbe_u32_t   limit;
    fbe_u32_t   i;
    limit = DAE_COUNTS_TEST_MAX_ENCL_TABLE_INDEX;
    for(i=0;i<limit;i++)
    {
        if(enclType == dae_counts_test_encl_table[i].enclType)
        {
            break;
        }
    }
    return(i);
}

void dae_test_verify_lcc_info(fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_encl_mgmt_lcc_info_t    lccInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(lccInfo.inserted, TRUE);
    return;
}

void dae_test_verify_ps_info(fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_ps_mgmt_ps_info_t       getPsInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    getPsInfo.location = *pLocation;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//  The following line needs to be re-enabled after the terminator support for Viking is available. 
//   MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.bInserted, TRUE);
    
    return;
}


void dae_test_verify_fan_info(fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_cooling_mgmt_fan_info_t fanInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);
//  The following line needs to be re-enabled after the terminator support for Viking is available. 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_EQUAL(fanInfo.inserted, FBE_MGMT_STATUS_TRUE);

    return;
}

void dae_test_verify_drive_info(fbe_device_physical_location_t * pLocation) 
{
    fbe_esp_drive_mgmt_drive_info_t getDriveInfo = {0};
    fbe_status_t                    status = FBE_STATUS_OK;


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
 * dae_counts_test_verify_encl_attr 
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
static fbe_status_t dae_counts_test_verify_encl_attr(void)
{
    fbe_sas_enclosure_type_t        enclType=FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_u64_t                       driveMask=0;
    fbe_u32_t                       enclNumber=0;
    fbe_u32_t                       enclNumberLimit;
    fbe_u32_t                       index = 0;
    fbe_u32_t                       slot = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       deviceCount = 0;
    fbe_device_physical_location_t  location = {0};
    fbe_esp_encl_mgmt_encl_info_t   enclInfo = {0};
    fbe_esp_board_mgmt_board_info_t boardInfo = {0};
    fbe_esp_drive_mgmt_drive_info_t getDriveInfo = {0};



    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
                    boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    // get the number of configured enclosures from the setup function
    enclNumberLimit = mixed_config_get_encl_config_list_size();
    for (enclNumber = 0; enclNumber < enclNumberLimit; enclNumber++)
    {
        // update the encl number (bus remains constant at 0)
        location.enclosure = enclNumber;
        location.slot = 0;

        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        // get the test array index for this enclosure type
        enclType = mixed_config_get_enclosure_type(encl_config_list[enclNumber]);
        mut_printf(MUT_LOG_LOW, "Encl:%d_%d (%s)",
                   location.bus, 
                   location.enclosure,
                   fbe_sas_encl_type_to_string(enclType));

        index = dae_type_to_counts_table_index(enclType);
        if(index >= DAE_COUNTS_TEST_MAX_ENCL_TABLE_INDEX)
        {
            mut_printf(MUT_LOG_LOW, "Encl:%d_%d fbe_sas_encl_type:%d is not supported",
                       location.bus, location.enclosure, enclType);
            continue;
        }

        mut_printf(MUT_LOG_LOW, "  lccCount act:%d exp:%d", enclInfo.lccCount,dae_counts_test_encl_table[index].lccCount);
        MUT_ASSERT_TRUE_MSG(enclInfo.lccCount == dae_counts_test_encl_table[index].lccCount, 
                            "LCC count is incorrect!");

        status = fbe_api_esp_encl_mgmt_get_lcc_count(&location, &deviceCount);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE_MSG(deviceCount == dae_counts_test_encl_table[index].lccCount, 
                            "LCC count is incorrect when using fbe_api_esp_encl_mgmt_get_lcc_count!");

        for(slot = 0; slot < enclInfo.lccCount; slot ++) 
        {
            location.slot = slot;
            dae_test_verify_lcc_info(&location);
        }
    
        mut_printf(MUT_LOG_LOW, "  psCount act:%d, exp:%d", enclInfo.psCount, dae_counts_test_encl_table[index].psCount);
        MUT_ASSERT_TRUE_MSG(enclInfo.psCount == dae_counts_test_encl_table[index].psCount, 
                            "PS count is incorrect!");

        status = fbe_api_esp_encl_mgmt_get_ps_count(&location, &deviceCount);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE_MSG(deviceCount == dae_counts_test_encl_table[index].psCount, 
                            "PS count is incorrect when using fbe_api_esp_encl_mgmt_get_ps_count!");
    
        for(slot = 0; slot < enclInfo.psCount; slot ++) 
        {
            location.slot = slot;
            dae_test_verify_ps_info(&location);
        }
    
        mut_printf(MUT_LOG_LOW, "  fanCount act:%d, exp:%d", enclInfo.fanCount, dae_counts_test_encl_table[index].fanCount);
        MUT_ASSERT_TRUE_MSG(enclInfo.fanCount == dae_counts_test_encl_table[index].fanCount, 
                            "FAN count is incorrect!");

        status = fbe_api_esp_encl_mgmt_get_fan_count(&location, &deviceCount);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE_MSG(deviceCount == dae_counts_test_encl_table[index].fanCount, 
                            "FAN count is incorrect when using fbe_api_esp_encl_mgmt_get_fan_count!");
    
        for(slot = 0; slot < enclInfo.fanCount; slot ++) 
        {
            location.slot = slot;
            dae_test_verify_fan_info(&location);
        }
    
        mut_printf(MUT_LOG_LOW, "  driveSlotCount act:%d, exp:%d", enclInfo.driveSlotCount, dae_counts_test_encl_table[index].driveSlotCount);
        MUT_ASSERT_TRUE_MSG(enclInfo.driveSlotCount == dae_counts_test_encl_table[index].driveSlotCount, 
                            "driveSlot count is incorrect!");
    
        status = fbe_api_esp_encl_mgmt_get_drive_slot_count(&location, &deviceCount);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE_MSG(deviceCount == dae_counts_test_encl_table[index].driveSlotCount, 
                            "driveSlot count is incorrect when using fbe_api_esp_encl_mgmt_get_drive_slot_count!");

        driveMask = mixed_config_get_drive_mask(encl_config_list[enclNumber]);
        for(slot = 0; slot < enclInfo.driveSlotCount; slot ++) 
        {
            location.slot = slot;
            if (driveMask & BIT(slot))
            {
                dae_test_verify_drive_info(&location);
            }
            else
            {
                getDriveInfo.location = location;
                status = fbe_api_esp_drive_mgmt_get_drive_info(&getDriveInfo);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_EQUAL(getDriveInfo.inserted, FALSE);
            }
        }
    } // for each encl
   
    return FBE_STATUS_OK;
} //dae_counts_test_verify_encl_attr

void dae_counts_test_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_MIXED_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void dae_counts_test(void)
{
    fbe_status_t status=FBE_STATUS_OK;

    /* Wait util there is no firmware upgrade in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");
//    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
//    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    status = dae_counts_test_verify_encl_attr();
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed DAE Counts Test!");
    
    // need a sleep here to allow esp to complete any unfinished work (resume prom read, etc), otherwise destroy will be attempted while
    // work is in progress and cause memory leak
    fbe_api_sleep (9000);

    return;
    
}
void dae_counts_test_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy();
    return;
}

/*************************
 * end 
 *************************/

