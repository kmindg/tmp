/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file figment_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test that validates flashing the enclosure and
 *  drive slot LEDs.
 *
 * @version
 *   02/27/2012 - Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "esp_tests.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "physical_package_tests.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * figment_short_desc = "Enclosure and drive fault LED Flashing";
char * figment_long_desc ="\
This tests validates flashing the enclosure and drive fault LEDs in an enclosure \n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
 STEP 2: Validate the starting of Enclosure Management Object\n\
        - Verify that enclosure Management Object is started\n\
          by checking the lifecycle state\n\
 STEP 3: Flash LEDs";

#define FIGMENT_TEST_MAX_RETRIES 20

/*!**************************************************************
 * figment_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/23/2012 - Created. bphilbin
 *
 ****************************************************************/
void figment_test(void)
{
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_device_physical_location_t location;
    fbe_u32_t bus, encl;
    fbe_u32_t status;
    fbe_esp_encl_mgmt_set_led_flash_t flashCommand;
    fbe_u32_t slot;
    fbe_u32_t retries;
    fbe_bool_t ok_to_continue;


    bus = 0;
    encl = 1;
    /*
     * Flash all the drive LEDs in enclosure 1
     */
    flashCommand.location.bus = bus;
    flashCommand.location.enclosure = encl;
    flashCommand.ledControlType = FBE_ENCLOSURE_FLASH_ALL_DRIVE_SLOTS;
    flashCommand.enclosure_led_flash_on = FBE_TRUE;
    
    status = fbe_api_esp_encl_mgmt_flash_leds(&flashCommand);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    
    

    /*
     * Check that the drive slot LEDs are flashing
     */
    location.bus = bus;
    location.enclosure = encl;
    

    for(retries=0;retries<FIGMENT_TEST_MAX_RETRIES;retries++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
        ok_to_continue = TRUE;

        for(slot = 0; slot < encl_info.driveSlotCount; slot++)
        {
            if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_MARKED)
            {
                ok_to_continue = FALSE;
                break;
            }
        }
        if(ok_to_continue)
        {
            break;
        }
    }

    for(slot = 0; slot < encl_info.driveSlotCount; slot++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Slot: %d LED Status 0x%x\n", slot, encl_info.enclDriveFaultLeds[slot]);
        MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_MARKED);
    }


    /*
     * Stop flashing the drive LEDs in enclosure 3
     */
    flashCommand.location.bus = bus;
    flashCommand.location.enclosure = encl;
    flashCommand.ledControlType = FBE_ENCLOSURE_FLASH_ALL_DRIVE_SLOTS;
    flashCommand.enclosure_led_flash_on = FBE_FALSE;

    status = fbe_api_esp_encl_mgmt_flash_leds(&flashCommand);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for(retries=0;retries<FIGMENT_TEST_MAX_RETRIES;retries++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
        ok_to_continue = TRUE;

        for(slot = 0; slot < encl_info.driveSlotCount; slot++)
        {
            if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_OFF)
            {
                ok_to_continue = FALSE;
                break;
            }
        }
        if(ok_to_continue)
        {
            break;
        }
    }
    for(slot = 0; slot < encl_info.driveSlotCount; slot++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Slot: %d LED Status 0x%x\n", slot, encl_info.enclDriveFaultLeds[slot]);
        MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_OFF);
    }



    /*
     * Turn off all LEDs in enclosure 2
     */

    encl = 2;

    /*
     * Flash some of the drive LEDs in enclosure 2
     */

    flashCommand.location.bus = bus;
    flashCommand.location.enclosure = encl;
    flashCommand.ledControlType = FBE_ENCLOSURE_FLASH_DRIVE_SLOT_LIST;
    fbe_zero_memory(flashCommand.slot_led_flash_on, (sizeof(fbe_bool_t) * FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS));
    flashCommand.slot_led_flash_on[3] = FBE_TRUE;
    flashCommand.slot_led_flash_on[5] = FBE_TRUE;
    location.enclosure = encl;

    status = fbe_api_esp_encl_mgmt_flash_leds(&flashCommand);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    
    for(retries=0;retries<FIGMENT_TEST_MAX_RETRIES;retries++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);

        for(slot = 0; slot < encl_info.driveSlotCount; slot++)
        {
            if((slot == 5) || (slot == 3)) // only slots 3 and 5 should be marked
            {
                if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_MARKED)
                {
                    ok_to_continue = FALSE;
                    break;
                }
            }
            else if((slot == 0) || (slot == 1) || slot == encl_info.driveSlotCount-1) //  slots 0, 1 and last should not be inserted
            {
                if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_INVALID)
                {
                    ok_to_continue = FALSE;
                    break;
                }
            }
            else if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_OFF)
            {
                ok_to_continue = FALSE;
                break;
            }

        }
        if( (encl_info.enclDriveFaultLeds[2] == FBE_LED_STATUS_OFF) &&
            (encl_info.enclDriveFaultLeds[4] == FBE_LED_STATUS_OFF) &&
            (encl_info.enclDriveFaultLeds[3] == FBE_LED_STATUS_MARKED) &&
            (encl_info.enclDriveFaultLeds[5] == FBE_LED_STATUS_MARKED))
        {
            break;
        }
    }
    

    // check that just slots 3 and 5 are marked
    for(slot = 2; slot < encl_info.driveSlotCount-1; slot++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Slot: %d LED Status 0x%x\n", slot, encl_info.enclDriveFaultLeds[slot]);
        if((slot == 5) || (slot == 3)) // only slots 3 and 5 should be marked
        {
            MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_MARKED);
        }
        else  // all other slots should be off
        {
            MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_OFF);
        }

    }


    /*
     * Stop flashing the drive LEDs in enclosure 2
     */
    flashCommand.location.bus = bus;
    flashCommand.location.enclosure = encl;
    flashCommand.ledControlType = FBE_ENCLOSURE_FLASH_ALL_DRIVE_SLOTS;
    flashCommand.enclosure_led_flash_on = FBE_FALSE;

    status = fbe_api_esp_encl_mgmt_flash_leds(&flashCommand);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for(retries=0;retries<FIGMENT_TEST_MAX_RETRIES;retries++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
        ok_to_continue = TRUE;

        for(slot = 2; slot < encl_info.driveSlotCount-1; slot++)
        {
            if(encl_info.enclDriveFaultLeds[slot] != FBE_LED_STATUS_OFF)
            {
                ok_to_continue = FALSE;
            }
        }
        if(ok_to_continue)
        {
            break;
        }
    }
    for(slot = 0; slot < encl_info.driveSlotCount; slot++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Slot: %d LED Status 0x%x\n", slot, encl_info.enclDriveFaultLeds[slot]);
        if((slot == 0) || (slot == 1) || (slot == encl_info.driveSlotCount-1)) //  slots 0, 1 and last should not be inserted
        {
            MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_INVALID);
        }
        else
        {
            MUT_ASSERT_TRUE(encl_info.enclDriveFaultLeds[slot] == FBE_LED_STATUS_OFF);
        }
    }

    /*
     * Flash the fault LED for enclosure 2
     */
    flashCommand.location.bus = bus;
    flashCommand.location.enclosure = encl;
    flashCommand.ledControlType = FBE_ENCLOSURE_FLASH_ENCL_FLT_LED;
    flashCommand.enclosure_led_flash_on = FBE_TRUE;

    status = fbe_api_esp_encl_mgmt_flash_leds(&flashCommand);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    location.enclosure = encl;

    for(retries=0;retries<FIGMENT_TEST_MAX_RETRIES;retries++)
    {
        fbe_api_sleep(3000);
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
        if(encl_info.enclFaultLedStatus == FBE_LED_STATUS_MARKED)
        {
            break;
        }
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "Encl: %d LED Status 0x%x\n", encl, encl_info.enclFaultLedStatus);
    MUT_ASSERT_TRUE(encl_info.enclFaultLedStatus == FBE_LED_STATUS_MARKED);

    mut_printf(MUT_LOG_LOW, "figment test passed...\n");
    return;
}
/******************************************
 * end figment_test()
 ******************************************/

void figment_setup(void)
{
    fbe_status_t status;
    fbe_u32_t index = 0;
    fbe_u32_t maxEntries = 0;
    fbe_test_params_t * pTest = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    /* Load the terminator, the physical package with the chautauqua config
     * and verify the objects in the physical package.
     */
    maxEntries = fbe_test_get_naxos_num_of_tests() ;
    for(index = 0; index < maxEntries; index++)
    {
        /* Load the configuration for test */
        pTest =  fbe_test_get_naxos_test_table(index);
        if(pTest->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
        {
            status = naxos_load_and_verify_table_driven(pTest);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);
}

void figment_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file figment_test.c
 *************************/
