/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * bliskey.c
 ***************************************************************************
 *
 * DESCRIPTION
 *
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"

/*! @def SOBO_4_LIFECYCLE_NOTIFICATION_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a notification
 */
#define BLISKEY_LIFECYCLE_NOTIFICATION_TIMEOUT  16000 /*wait maximum of 16 seconds*/

static fbe_status_t bliskey_run(void)
{
	fbe_u32_t port = 0;
	fbe_u32_t encl = 0;
	fbe_u32_t drive = 5;
	fbe_u32_t               object_handle;
	fbe_status_t status;
		
	/* register the notification */
    fbe_test_pp_util_lifecycle_state_ns_context_t ns_context; 
    ns_context.timeout_in_ms = BLISKEY_LIFECYCLE_NOTIFICATION_TIMEOUT;
    fbe_test_pp_util_register_lifecycle_state_notification(&ns_context);
	
	status = fbe_api_get_physical_drive_object_id_by_location(port, encl, drive, &object_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
	status = fbe_api_physical_drive_enter_glitch_drive(object_handle, 10);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
	/* wait for physical drive ready notification */
    ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = FBE_LIFECYCLE_STATE_READY;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);
	
	///* Make sure that pdo is in ready state. */
    status = fbe_test_pp_util_verify_pdo_state(port, 
			 encl, 
			 drive, 
			 FBE_LIFECYCLE_STATE_READY, 
			 BLISKEY_LIFECYCLE_NOTIFICATION_TIMEOUT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
	fbe_test_pp_util_unregister_lifecycle_state_notification(&ns_context);
	return FBE_STATUS_OK;

}

void bliskey(void)
{
	fbe_status_t run_status;

	run_status = bliskey_run();
	
	MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);
}
