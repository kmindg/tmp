/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file spiderman_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests if all the related services are getting
 *  started correctly and FBE API getting initialized
 *
 * @version
 *   11/10/2009 - Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "esp_tests.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * spiderman_short_desc = "Environment Service package Framework";
char * spiderman_long_desc ="\
This tests validates that all the required services are getting started and the FBE API initialized correctly \n\
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
 * spiderman_test() 
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

void spiderman_test(void)
{
    return;
}
/******************************************
 * end spiderman_test()
 ******************************************/
void spiderman_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}

void spiderman_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file spiderman_test.c
 *************************/


