/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file master_cylinder_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the PS Management Object is created by ESP Framework.
 * 
 * @version
 *   04/19/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_ps_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * master_cylinder_short_desc = "Verify the PS Management Object is created by ESP Framework";
char * master_cylinder_long_desc ="\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. PS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate PS Status from PS Mgmt\n\
        - Verify that the PS Status reported by PS Mgmt is good\n\
";

/*!**************************************************************
 * master_cylinder_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void master_cylinder_test(void)
{
    fbe_status_t                    status;
    fbe_esp_ps_mgmt_ps_info_t       psInfo;
    fbe_u8_t                        ps;
    fbe_device_physical_location_t  location = {0};
    fbe_u32_t                       psCount;

    // verify the initial PS Status
    location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;

    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "psCount %d\n", psCount); 
    MUT_ASSERT_TRUE(psCount <= FBE_ESP_PS_MAX_COUNT_PER_ENCL);

    psInfo.location = location;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    for (ps = 0; ps < psCount; ps++)
    {
        psInfo.location.slot = ps;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "ps %d, psInserted %d, fault %d, acFail %d\n", 
            ps,
            psInfo.psInfo.bInserted,
            psInfo.psInfo.generalFault,
            psInfo.psInfo.ACFail);
        MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
        MUT_ASSERT_FALSE(psInfo.psInfo.generalFault);
        MUT_ASSERT_FALSE(psInfo.psInfo.ACFail);
    }
    
    return;
}
/******************************************
 * end master_cylinder_test()
 ******************************************/
/*!**************************************************************
 * master_cylinder_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
void master_cylinder_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}
/**************************************
 * end master_cylinder_setup()
 **************************************/

/*!**************************************************************
 * master_cylinder_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the master_cylinder test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void master_cylinder_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end master_cylinder_cleanup()
 ******************************************/
/*************************
 * end file master_cylinder_test.c
 *************************/


