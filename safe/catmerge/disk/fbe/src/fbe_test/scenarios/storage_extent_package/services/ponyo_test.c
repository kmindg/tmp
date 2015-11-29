/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file ponyo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for the CMS service interfaces
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_cms_exerciser_interface.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * ponyo_short_desc = "This scenario will test the interfaces used by CMS clients";
char * ponyo_long_desc ="\
This includes:\n\
        - Exclusive sllocation/Commit/Free for SG and contiguos allocations\n\
\n\
Dependencies:\n\
        - CMS service must be active.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 15 SAS drive\n\
        [PP] 15 logical drive\n\
        [SEP] 15 provisioned drives\n\
\n\
STEP 1: Bring up the initial topology + NEIT\n\
STEP 2: Run The exclusive allocation test\n\
Last Update 12/02/11\n";


static void ponyo_test_end_callback_function (update_object_msg_t * update_object_msg, void *context);

/********************************************************************************************************************************/

void ponyo_setup(void)
{
	/*just bring up a plain system*/
	fbe_test_rg_configuration_t *rg_config_p = NULL;
	/* create a r5 rg */
    sep_standard_logical_config_get_rg_configuration(FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_R5, &rg_config_p);
	sep_standard_logical_config_create_rg_with_lun(rg_config_p, 0);  /* just create a RG */
    return;
}

void ponyo_test(void)
{
	fbe_cms_exerciser_exclusive_alloc_test_t 			test_info;
	fbe_status_t										status;
	fbe_notification_registration_id_t  				reg_id;
	fbe_semaphore_t 									sem;
	fbe_cms_exerciser_get_interface_test_results_t  	test_results;

	fbe_semaphore_init(&sem, 0, 1);

	mut_printf(MUT_LOG_TEST_STATUS, " %s:Starting Exclusive Allocation Test...", __FUNCTION__);

	test_info.allocations_per_thread = 1;
	test_info.cpu_mask = 0x1;
	test_info.msec_delay_between_allocations = 0;
	test_info.threads_per_cpu = 1;

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_CMS_TEST_DONE,
																  FBE_PACKAGE_NOTIFICATION_ID_NEIT,
																  FBE_TOPOLOGY_OBJECT_TYPE_ALL ,
																  ponyo_test_end_callback_function,
																  &sem,
																  &reg_id);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    

	status  = fbe_api_cms_exerciser_start_exclusive_allocation_test(&test_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

	mut_printf(MUT_LOG_TEST_STATUS, " %s:Test running, pleas wait!", __FUNCTION__);
	status = fbe_semaphore_wait_ms(&sem, 60000);/*wait up to a minute*/
	MUT_ASSERT_TRUE(status != FBE_STATUS_TIMEOUT);
	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);

	mut_printf(MUT_LOG_TEST_STATUS, " %s:Exclusive Allocation Test passed, getting results...", __FUNCTION__);
	status = fbe_api_cms_exerciser_get_exclusive_allocation_results(&test_results);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(test_results.passed == FBE_TRUE);

	status = fbe_api_notification_interface_unregister_notification(ponyo_test_end_callback_function, reg_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_semaphore_destroy(&sem);

	mut_printf(MUT_LOG_TEST_STATUS, " %s:CMS Interfaces test passed!", __FUNCTION__);

}

void ponyo_destroy(void)
{
    /* Destroy and unload: sep, neit and physical.
     */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}


static void ponyo_test_end_callback_function (update_object_msg_t * update_object_msg, void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;
    
    fbe_semaphore_release(sem, 0, 1 ,FALSE);
}
