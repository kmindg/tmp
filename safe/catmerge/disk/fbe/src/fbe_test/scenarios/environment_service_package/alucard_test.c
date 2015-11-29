/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file eva_01_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify B side configuration page for Voyager and Derringer.
 *
 * @version
 *   10/25/2011 - Created. dongz
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "pp_utils.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"

#include "fbe_test_configurations.h"

#include "fbe_test_esp_utils.h"
#include "fp_test.h"
#include "sep_tests.h"

/*************************
 *   GLOBALS
 *************************/

char * alucard_short_desc = "Test esp array midplane wwn seed support feature.";
char * alucard_long_desc ="\
Alucard 01\n\
        -A character in Castlevania. Son of Duke Dracula. \n\
\n\
Using Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
   1) initializes the terminator\n\
   2) loads the test configuration\n\
            a) inserts a board\n\
            b) inserts a port\n\
            c) inserts an enclosure to port 0\n\
            d) inserts 12 sas drives to port 0 encl 0\n\
\n\
   3) Set user modified wwn seed to default val.\n\
   4) Start the physical package\n\
   5) Start esp\n\
   6) Set array midplane wwn seed\n\
   7) validate array midplane rp wwn seed and user modified wwn seed flag\n\
   8) Set array midplane wwn seed and set user modified wwn seed flag\n\
   9) validate array midplane rp wwn seed and user modified wwn seed flag\n";

void alucard_test_setup(void);
fbe_status_t fbe_test_load_alucard_config(void);

/*!**************************************************************
 * alucard_test()
 ****************************************************************
 * @brief
 *  Test esp array midplane wwn seed support feature.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/25/2012 - Created. dongz
 *
 ****************************************************************/
void alucard_test()
{
    fbe_status_t status;
    fbe_u32_t wwn_seed = 0, get_wwn_seed;
    fbe_bool_t user_modify_wwn_seed;

    mut_printf(MUT_LOG_LOW, "=== alucard_test start ===");

    mut_printf(MUT_LOG_LOW, "=== alucard_test set rp wwn seed without set user modify flag ===");
    wwn_seed = 12345;
    status = fbe_api_esp_encl_mgmt_set_array_midplane_rp_wwn_seed(&wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //wait resume prom info update
    mut_printf(MUT_LOG_LOW, "=== waiting 10 seconds for resume prom write to be done===");
    fbe_api_sleep (10000);

    status = fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(&get_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(wwn_seed == get_wwn_seed);

    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == FALSE);

    mut_printf(MUT_LOG_LOW, "=== alucard_test set rp wwn seed & set user modify flag ===");

    wwn_seed = 54321;
    status = fbe_api_esp_encl_mgmt_user_modify_wwn_seed(&wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //wait resume prom info update
    mut_printf(MUT_LOG_LOW, "=== waiting 10 seconds for resume prom write to be done===");
    fbe_api_sleep (10000);

    status = fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(&get_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(wwn_seed == get_wwn_seed);

    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == TRUE);

    mut_printf(MUT_LOG_LOW, "=== alucard_test clear user modify flag ===");

    status = fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(&user_modify_wwn_seed);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(user_modify_wwn_seed == FALSE);


    mut_printf(MUT_LOG_LOW, "=== alucard_test success! ===");
}
/******************************************
 * end alucard_test()
 ******************************************/

/*!**************************************************************
 * alucard_test_setup()
 ****************************************************************
 * @brief
 *  Test esp array midplane wwn seed support feature.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   08/25/2012 - Created.  Dongz
 *
 ****************************************************************/
void alucard_test_setup()
{
    fbe_status_t status;
    fbe_bool_t default_wwn_seed_modified_flag = FALSE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //debug only
    mut_printf(MUT_LOG_LOW, "=== configured terminator ===\n");

    status = fbe_test_load_alucard_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    status = fbe_test_startPhyPkg(TRUE, FP_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== phy pkg is started ===\n");

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    fbe_test_esp_registry_write(ESP_REG_PATH,
                       FBE_ENCL_MGMT_USER_MODIFIED_WWN_SEED_INFO_KEY,
                       FBE_REGISTRY_VALUE_DWORD,
                       &default_wwn_seed_modified_flag,
                       sizeof(fbe_bool_t));

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}
/******************************************
 * end alucard_test_setup()
 ******************************************/

/*!**************************************************************
 * fbe_test_load_alucard_config()
 ****************************************************************
 * @brief
 *  Setup with mustang default settings, and create one port,
 *  one enclosure, and 15 drives.
 *  copied from fbe_test_load_fflewddur_fflam_config
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   08/25/2012 - Created. dongz
 *
 ****************************************************************/
fbe_status_t fbe_test_load_alucard_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t sas_port;
    fbe_terminator_sas_encl_info_t sas_encl;

    fbe_api_terminator_device_handle_t port_handle;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_api_terminator_device_handle_t drive_handle;

    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports < FP_MAX_PORTS; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number = 3 + no_of_ports;
        sas_port.portal_number = 5 + no_of_ports;
        sas_port.sas_address = 0x5000097A7800903F + no_of_ports;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        for ( no_of_encls = 0; no_of_encls < FP_MAX_ENCLS; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
            sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            for (slot = 0; slot < FP_MAX_DRIVES; slot ++)
            {
                if(no_of_encls < 2 || (no_of_encls == 2 && slot >=2 && slot <= 13))
                {
                    status  = fbe_test_pp_util_insert_sas_drive(no_of_ports,
                                                                 no_of_encls,
                                                                 slot,
                                                                 520,
                                                                 0x10B5C730,
                                                                 &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
    }
    return status;
}

/*!**************************************************************
 * alucard_test_destroy()
 ****************************************************************
 * @brief
 *  Test esp array midplane wwn seed support feature.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   08/25/2012 - Created. dongz
 *
 ****************************************************************/
void alucard_test_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end alucard_test_destroy()
 ******************************************/

/*************************
 * end file alucard_test.c
 *************************/
