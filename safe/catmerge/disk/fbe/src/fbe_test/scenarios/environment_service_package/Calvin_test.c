/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file Calvin_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests if all the related services are getting
 *  started correctly and FBE API getting initialized
 *
 * @version
 *   05/05/2010 - Created. Dan McFarland
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * calvin_short_desc = "Calvin_Test: ESP Energy Information Reporting Infrastructure Test";
char * calvin_long_desc ="\
Calvin: \n\
        Star of the comic strip Calvin and Hobbes.  A must read for everyone if you want to understand the mind\n\
        of a 6 year old boy.\n\
        Famous quote: 'Boy I love summer vacation.  I can feel my brain beginning to atrophy already!'\n\
\n\
        This tests validates that the eir service is getting started and the FBE API initialized correctly. \n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n"
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate the starting of Service\n\
        - Verify that all the services are getting initalized\n\
        - Verify that FBE API object map is initiaized\n\
        - Verify that all the classes mentioned above are initialized and loaded";



/*!**************************************************************
 * calvin_test()
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   05/05/2010 - Created. Dan McFarland
 *
 ****************************************************************/

void calvin_test(void)
{
    fbe_status_t status;
    fbe_eir_data_t fbe_eir_data;

    status = fbe_api_esp_eir_getData( &fbe_eir_data );
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_api_sleep (6000);

    status = fbe_api_esp_eir_getData( &fbe_eir_data );
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return;
}
/******************************************
 * end calvin_test()
 ******************************************/
void calvin_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        NULL);
}

void calvin_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
}
/*************************
 * end file calvin_test.c
 *************************/


