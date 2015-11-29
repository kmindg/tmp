/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cinderella_test.c
 ***************************************************************************
 *
 * @brief
 *  This file detects the presence of drive and handle this
 *  appropriately
 *
 * @version
 *   03/16/2010 - Created. Ashok Tamilarasan
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
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_common_transport.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void cinderella_test_verify_get_all_drives_function(void);

char * cinderella_short_desc = "New Drive Detection on boot up";
char * cinderella_long_desc ="\
This tests validates the drive management object consumes the drive exposed by the Physical Package \n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper drive\n\
        [PP] 3 SAS drives/drive\n\
        [PP] 3 logical drives/drive\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
 STEP 2: Validate the starting of Drive Management Object\n\
        - Verify that drive Management Object is started\n\
          by checking the lifecycle state\n\
 STEP 3: Validate that drive object gets the data from the physical package\n\
        - Verify that Drive Management Object sees 3 Viper Drives\n\
        - Issue FBE API from the test code to drive management object to \n\
          validate that data(type, serial number, location) stored in the \n\
          drive management object is correct";

/*!**************************************************************
 * cinderella_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/12/2009 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
void cinderella_test(void)
{
    fbe_esp_drive_mgmt_drive_info_t drive_info;
    fbe_u32_t bus, enclosure, drive;
    fbe_u32_t status;

    for (bus = 0; bus < CHAUTAUQUA_MAX_PORTS; bus ++)
    {
        for ( enclosure = 0; enclosure < CHAUTAUQUA_MAX_ENCLS; enclosure++ )
        {
            for (drive = 0; drive < CHAUTAUQUA_MAX_DRIVES; drive++)
            {
                mut_printf(MUT_LOG_LOW, "Getting Drive Info for Bus:%d Encl: %d Slot: %d...\n", bus, enclosure, drive);
                drive_info.location.bus = bus;
                drive_info.location.enclosure = enclosure;
                drive_info.location.slot = drive;
    
        
                status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_info);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_DRIVE_TYPE_SAS, drive_info.drive_type, "Drive Type does not match\n");
            }
        }
    }

	/*let's also test the interfaces that get all the drives at once*/
	cinderella_test_verify_get_all_drives_function();
	
    mut_printf(MUT_LOG_LOW, "Cinderella test passed...\n");
    return;
}
/******************************************
 * end cinderella_test()
 ******************************************/
void cinderella_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
}

void cinderella_destroy(void)
{

    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
}

static void cinderella_test_verify_get_all_drives_function(void)
{
	fbe_status_t 						status;
	fbe_u32_t 							total_drives, actual_drives;
	fbe_esp_drive_mgmt_drive_info_t *	all_drive_info = NULL;

	mut_printf(MUT_LOG_LOW, "=== testing getting all drives info at once ===");

	status = fbe_api_esp_drive_mgmt_get_total_drives_count(&total_drives);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "=== Found %d drives ===", total_drives);

	all_drive_info = (fbe_esp_drive_mgmt_drive_info_t *)fbe_api_allocate_memory(sizeof(fbe_esp_drive_mgmt_drive_info_t) * total_drives);

	/*some invalid pattern inside*/
	fbe_set_memory(all_drive_info, 0xff, sizeof(fbe_esp_drive_mgmt_drive_info_t) * total_drives);

	status = fbe_api_esp_drive_mgmt_get_all_drives_info(all_drive_info, total_drives, &actual_drives);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(actual_drives == total_drives);

	/*let's check a few fileds on the first drive*/
	MUT_ASSERT_TRUE(all_drive_info->inserted == FBE_TRUE);
    MUT_ASSERT_TRUE(all_drive_info->block_size != 0);
	MUT_ASSERT_TRUE(all_drive_info->gross_capacity != 0);
	MUT_ASSERT_TRUE(all_drive_info->drive_type != FBE_DRIVE_TYPE_INVALID);
	MUT_ASSERT_TRUE(all_drive_info->location.bus != 0xff);
	MUT_ASSERT_TRUE(all_drive_info->location.enclosure != 0xff);
	MUT_ASSERT_TRUE(all_drive_info->location.slot != 0xff);

	fbe_api_free_memory(all_drive_info);
}
/*************************
 * end file cinderella_test.c
 *************************/

