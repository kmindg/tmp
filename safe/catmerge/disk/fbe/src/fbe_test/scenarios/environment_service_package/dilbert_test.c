/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dilbert_test.c
 ***************************************************************************
 *
 * @brief
 *  This file detects the presence of enclosure and handle this
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
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "esp_tests.h"
#include "physical_package_tests.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * dilbert_short_desc = "New Enclosure Detection on boot up";
char * dilbert_long_desc ="\
This tests validates the enclosure management object consumes the enclosure exposed by the Physical Package \n\
\n\
\n\
Starting Config:\n\
        [PP] chautauqua_config\n\
        [PP] armada board\n\
        [PP] 2 SAS PMC ports\n\
        [PP] 8 viper enclosure/bus\n\
        [PP] 15 SAS drives/enclosure\n\
        [PP] 15 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
 STEP 2: Validate the starting of Enclosure Management Object\n\
        - Verify that enclosure Management Object is started\n\
          by checking the lifecycle state\n\
 STEP 3: Validate that enclosure object gets the data from the physical package\n\
        - Verify that Enclosure Management Object sees 3 Viper Enclosures\n\
        - Issue FBE API from the test code to enclosure management object to \n\
          validate that data(type, serial number, location) stored in the \n\
          enclosure management object is correct";

/*!**************************************************************
 * dilbert_test() 
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
void dilbert_test(void)
{
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_device_physical_location_t location;
    fbe_u32_t bus, encl;
    fbe_u32_t status;

    for (bus = 0; bus < CHAUTAUQUA_MAX_PORTS; bus ++)
    {
        for ( encl = 0; encl < CHAUTAUQUA_MAX_ENCLS; encl++ )
        {
            mut_printf(MUT_LOG_LOW, "Getting Encl Info for Bus:%d Encl: %d...\n", bus, encl);
            location.bus = bus;
            location.enclosure = encl;
    
            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_TYPE_VIPER, encl_info.encl_type, "Enclosure Type does not match\n");
        }
    }
    mut_printf(MUT_LOG_LOW, "Dilbert test passed...\n");

    return;
}
/******************************************
 * end dilbert_test()
 ******************************************/

void dilbert_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,
                                        NULL,
                                        NULL);
}

void dilbert_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
}
/*************************
 * end file dilbert_test.c
 *************************/

