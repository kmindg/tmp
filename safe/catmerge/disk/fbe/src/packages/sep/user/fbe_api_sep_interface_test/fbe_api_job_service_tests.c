/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_job_service_tests.c
 ***************************************************************************
 *
 * @brief
 *   This file includes unit tests to test the interaction of various
 *   objects with the Job Control Service.  These tests are part of
 *   the fbe_api_job_service_suite test suite.
 *
 * @version
 *   12/2009 - Created.  guov
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "fbe_api_sep_interface_test.h"
#include "fbe/fbe_api_job_service_interface.h"


/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  fbe_api_job_service_lun_create_test
 ******************************************************************************
 *
 * @brief
 *  This is the test for LUN create (bind).
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void fbe_api_job_service_lun_create_test(void)
{
    fbe_status_t status;
    fbe_api_job_service_lun_create_t fbe_lun_create_req;

    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = 5;
    fbe_lun_create_req.lun_number = 123;
    fbe_lun_create_req.capacity = 1234567;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.user_private = FBE_FALSE;
    fbe_raid_group_create_req.wait_ready = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec = 20000;

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
 
    return;

} /* End fbe_api_job_service_lun_create_test() */


/*!****************************************************************************
 *  fbe_api_job_service_lun_destroy_test
 ******************************************************************************
 *
 * @brief
 *  This is the test for LUN destroy (unbind).
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void fbe_api_job_service_lun_destroy_test(void)
{
    fbe_status_t status;
    fbe_api_job_service_lun_destroy_t   fbe_lun_destroy_req;

    fbe_lun_destroy_req.lun_number = 123;
    fbe_lun_destroy_req.wait_destroy =  FBE_FALSE;
    fbe_lun_destroy_req.destroy_timeout_msec = 20000;

    status = fbe_api_job_service_lun_destroy(&fbe_lun_destroy_req);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);

    return;

} /* End fbe_api_job_service_lun_destroy_test() */