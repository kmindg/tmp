/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               mock_dramcache_test_main.c
 ***************************************************************************
 *
 * DESCRIPTION: 
 *
 * FUNCTIONS:  Unit test to validate that the 
 *
 *
 * NOTES:
 *
 * HISTORY:
 *    07/13/2009  Martin Buckley Initial Version
 *
 **************************************************************************/

#include "sep_tests.h"
#include "physical_package_tests.h"
#include "esp_tests.h"

#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_terminator_interface.h"

#include "fbe/fbe_api_common.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"

#include "simulation_log.h"
#include "simulation/fbe_simulation_public_api.h"
#include "fbe/fbe_emcutil_shell_include.h"

void MUT_API setup(fbe_test_sp_type_t sp, fbe_test_sp_hw_config_t hwConfig, fbe_test_logical_config_t logicConfig) {
    fbe_test_setup(sp);
    fbe_test_initialize_hw_config(sp, hwConfig);
    fbe_test_initialize_logical_config(logicConfig);
}

void MUT_API setup_SPA_1enc_12drives_1RaidGroup(void) {
    setup(fbe_test_SPA, hw_config_1enclosure_12drives, logical_config_1raidgroup_12drives);
}

void MUT_API setup_SPB_1enc_12drives_1RaidGroup(void) {
    setup(fbe_test_SPB, hw_config_1enclosure_12drives, logical_config_1raidgroup_12drives);
}

void MUT_API setup_SPA_1enc_12drives_4RaidGroups(void) {
    setup(fbe_test_SPA, hw_config_1enclosure_12drives, logical_config_4raidgroups_12drives);
}

void MUT_API setup_SPB_1enc_12drives_4RaidGroups(void) {
    setup(fbe_test_SPB, hw_config_1enclosure_12drives, logical_config_4raidgroups_12drives);
}

void MUT_API setup_SPA_2enc_24drives_4RaidGroups(void) {
    setup(fbe_test_SPA, hw_config_2enclosures_24drives, logical_config_4raidgroups_24drives);
}

void MUT_API setup_SPB_2enc_24drives_4RaidGroups(void) {
    setup(fbe_test_SPB, hw_config_2enclosures_24drives, logical_config_4raidgroups_24drives);
}

void MUT_API setup_SPA_2enc_24drives_8RaidGroups(void) {
    setup(fbe_test_SPA, hw_config_2enclosures_24drives, logical_config_8raidgroups_24drives);
}

void MUT_API setup_SPB_2enc_24drives_8RaidGroups(void) {
    setup(fbe_test_SPB, hw_config_2enclosures_24drives, logical_config_8raidgroups_24drives);
}

void MUT_API setup_SPA_4enc_48drives_16RaidGroups(void) {
    setup(fbe_test_SPA, hw_config_4enclosures_48drives, logical_config_16raidgroups_48drives);
}

void MUT_API setup_SPB_4enc_48drives_16RaidGroups(void) {
    setup(fbe_test_SPB, hw_config_4enclosures_48drives, logical_config_16raidgroups_48drives);
}

void MUT_API setup_SPA_4enc_48drives_16RaidGroups_with_1Lun(void) {
    setup(fbe_test_SPA, hw_config_4enclosures_48drives, logical_config_16raidgroups_48drives);
    fbe_test_create_lun(0,  0, 20480);  // 10MB
    fbe_test_create_lun(1,  1, 20480);  // 10MB
    fbe_test_create_lun(2,  2, 20480);  // 10MB
    fbe_test_create_lun(3,  3, 20480);  // 10MB
    fbe_test_create_lun(4,  4, 20480);  // 10MB
    fbe_test_create_lun(5,  5, 20480);  // 10MB
    fbe_test_create_lun(6,  6, 20480);  // 10MB
    fbe_test_create_lun(7,  7, 20480);  // 10MB
    fbe_test_create_lun(8,  8, 20480);  // 10MB
    fbe_test_create_lun(9,  9, 20480);  // 10MB
    fbe_test_create_lun(10, 10, 20480);  // 10MB
    fbe_test_create_lun(11, 11, 20480);  // 10MB
    fbe_test_create_lun(12, 12, 20480);  // 10MB
    fbe_test_create_lun(13, 13, 20480);  // 10MB
    fbe_test_create_lun(14, 14, 20480);  // 10MB
    fbe_test_create_lun(15, 15, 20480);  // 10MB
}

void MUT_API setup_SPB_4enc_48drives_16RaidGroups_with_1Lun(void) {
    setup(fbe_test_SPB, hw_config_4enclosures_48drives, logical_config_16raidgroups_48drives);
    fbe_test_create_lun(0,  0, 20480);  // 10MB
    fbe_test_create_lun(1,  1, 20480);  // 10MB
    fbe_test_create_lun(2,  2, 20480);  // 10MB
    fbe_test_create_lun(3,  3, 20480);  // 10MB
    fbe_test_create_lun(4,  4, 20480);  // 10MB
    fbe_test_create_lun(5,  5, 20480);  // 10MB
    fbe_test_create_lun(6,  6, 20480);  // 10MB
    fbe_test_create_lun(7,  7, 20480);  // 10MB
    fbe_test_create_lun(8,  8, 20480);  // 10MB
    fbe_test_create_lun(9,  9, 20480);  // 10MB
    fbe_test_create_lun(10, 10, 20480);  // 10MB
    fbe_test_create_lun(11, 11, 20480);  // 10MB
    fbe_test_create_lun(12, 12, 20480);  // 10MB
    fbe_test_create_lun(13, 13, 20480);  // 10MB
    fbe_test_create_lun(14, 14, 20480);  // 10MB
    fbe_test_create_lun(15, 15, 20480);  // 10MB
}

void MUT_API cleanupA(void) {
    fbe_test_cleanup(fbe_test_SPA);
}

void MUT_API cleanupB(void) {
    fbe_test_cleanup(fbe_test_SPB);
}

void MUT_API test(void) {
    /* As needed, introduce delay */
    bool wait = FALSE;
    if(wait) {
        char buffer[100];
        printf("type cr to continue\n");
        gets(buffer);
    }
}

int __cdecl main (int argc , char **argv)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    mut_testsuite_t *test_suite;   /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);   /* before proceeding we need to initialize MUT infrastructure */

    test_suite = MUT_CREATE_TESTSUITE("test_suite")   /* testsuite is created */

    MUT_ADD_TEST(test_suite, test, setup_SPA_1enc_12drives_1RaidGroup, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_1enc_12drives_1RaidGroup, cleanupB);
    MUT_ADD_TEST(test_suite, test, setup_SPA_1enc_12drives_4RaidGroups, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_1enc_12drives_4RaidGroups, cleanupB);
    MUT_ADD_TEST(test_suite, test, setup_SPA_2enc_24drives_4RaidGroups, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_2enc_24drives_4RaidGroups, cleanupB);
    MUT_ADD_TEST(test_suite, test, setup_SPA_2enc_24drives_8RaidGroups, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_2enc_24drives_8RaidGroups, cleanupB);
    MUT_ADD_TEST(test_suite, test, setup_SPA_4enc_48drives_16RaidGroups, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_4enc_48drives_16RaidGroups, cleanupB);
    MUT_ADD_TEST(test_suite, test, setup_SPA_4enc_48drives_16RaidGroups_with_1Lun, cleanupA);
    MUT_ADD_TEST(test_suite, test, setup_SPB_4enc_48drives_16RaidGroups_with_1Lun, cleanupB);

    MUT_RUN_TESTSUITE(test_suite);
}
