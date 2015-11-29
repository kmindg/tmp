/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_zeroing_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the zeroing functionality of the
 *  Storage Extent Package (SEP).
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "sep_zeroing_utils.h"
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config

//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

//-----------------------------------------------------------------------------
//  FUNCTIONS:

/*!****************************************************************************
 *  sep_zeroing_utils_check_disk_zeroing_status
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing complete status with reading checkpoint
 *   in loop. It exits when disk zeroing reach at given checkpoint or time
 *   out occurred.
 *
 * @param object_id             - provision drive object id
 * @param timeout_ms            - max timeout value to check for disk zeroing  complete
 * @param zero_checkpoint       - wait till disk zeroing reach at this checkpoint
 *
 * @return fbe_status_t              fbe status
 *
 *****************************************************************************/

fbe_status_t sep_zeroing_utils_check_disk_zeroing_status(fbe_object_id_t object_id,
                                            fbe_u32_t   timeout_ms,
                                            fbe_lba_t expect_checkpoint)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t               current_time = 0;
    fbe_lba_t               curr_checkpoint;

    while (current_time < timeout_ms){

		/* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Zero Checkpoint failed with status %d\n", __FUNCTION__, status);
            return status;
        }

        /* check if disk zeroing reach at given checkpoint */
        if ((curr_checkpoint >= expect_checkpoint && curr_checkpoint != FBE_LBA_INVALID) ||
        	(curr_checkpoint == expect_checkpoint && expect_checkpoint == FBE_LBA_INVALID)){
            mut_printf(MUT_LOG_TEST_STATUS,
		       "=== disk zeroing wait complete, chkpt 0x%llx\n",
		       (unsigned long long)curr_checkpoint);
            return status;
        }
        current_time = current_time + SEP_ZEROING_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (SEP_ZEROING_UTILS_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%llx, %d ms!\n",
                  __FUNCTION__, object_id, (unsigned long long)curr_checkpoint,
		  timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/***************************************************************
 * end sep_zeroing_utils_check_disk_zeroing_status()
 ***************************************************************/

/*!****************************************************************************
 *  sep_zeroing_utils_is_zeroing_in_progress
 ******************************************************************************
 *
 * @brief
 *   This function checks if disk zeroing is in progress with reading checkpoint.
 *
 * @param object_id             - provision drive object id
 * @param zeroing_in_progress   - bool for whether or not zeroing is in progress
 *
 * @return fbe_status_t              fbe status
 *
 *****************************************************************************/

fbe_status_t sep_zeroing_utils_is_zeroing_in_progress(fbe_object_id_t object_id, fbe_bool_t *zeroing_in_progress)
{
	fbe_status_t status;
	fbe_lba_t curr_checkpoint;

	/* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);

    if (status == FBE_STATUS_GENERIC_FAILURE){
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Zero Checkpoint failed with status %d\n", __FUNCTION__, status);
		return status;
	}

    if (curr_checkpoint == FBE_LBA_INVALID)
    {
    	*zeroing_in_progress = FBE_FALSE;
    }
    else
    {
    	*zeroing_in_progress = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/***************************************************************
 * end sep_zeroing_utils_is_zeroing_in_progress()
 ***************************************************************/

/*!****************************************************************************
 *  sep_zeroing_utils_wait_for_disk_zeroing_to_start
 ******************************************************************************
 *
 * @brief
 *   This function waits for disk zeroing  to start.  It exits when disk zeroing
 *   starts or time out occurred.
 *
 * @param object_id             - provision drive object id
 * @param timeout_ms            - max timeout value to check for disk zeroing  complete
 *
 * @return fbe_status_t              fbe status
 *
 *****************************************************************************/

fbe_status_t sep_zeroing_utils_wait_for_disk_zeroing_to_start(fbe_object_id_t object_id,
                                            fbe_u32_t   timeout_ms)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t               current_time = 0;
    fbe_lba_t               curr_checkpoint;

    while (current_time < timeout_ms){

		/* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);
            return status;
        }

        /* check if disk zeroing reach at given checkpoint */
        if (curr_checkpoint > 0 && curr_checkpoint != FBE_LBA_INVALID){
            mut_printf(MUT_LOG_TEST_STATUS,
		       "=== disk zeroing started, chkpt 0x%llx\n",
		       (unsigned long long)curr_checkpoint);
            return status;
        }
        current_time = current_time + SEP_ZEROING_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (SEP_ZEROING_UTILS_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%llx, %d ms!\n",
                  __FUNCTION__, object_id, (unsigned long long)curr_checkpoint,
		  timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/***************************************************************
 * end sep_zeroing_utils_wait_for_disk_zeroing_to_start()
 ***************************************************************/

/*!****************************************************************************
 *  sep_zeroing_utils_check_disk_zeroing_stopped
 ******************************************************************************
 *
 * @brief
 *   This function checks that disk zeroing  has stopped.
 *
 * @param object_id             - provision drive object id
 *
 * @return fbe_status_t              fbe status
 *
 *****************************************************************************/

fbe_status_t sep_zeroing_utils_check_disk_zeroing_stopped(fbe_object_id_t object_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t               current_time = 0;
    fbe_lba_t               curr_checkpoint;
    fbe_lba_t               expect_checkpoint;

    /* get the current disk zeroing checkpoint */
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &expect_checkpoint);

	if (status == FBE_STATUS_GENERIC_FAILURE){
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);
		return status;
	}

    while (current_time < 10000){

		/* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);
            return status;
        }

        MUT_ASSERT_UINT64_EQUAL(curr_checkpoint, expect_checkpoint);

        current_time = current_time + SEP_ZEROING_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (SEP_ZEROING_UTILS_POLLING_INTERVAL);
    }

    return FBE_STATUS_OK;
}
/***************************************************************
 * end sep_zeroing_utils_check_disk_zeroing_stopped()
 ***************************************************************/

/*!**************************************************************
 * sep_zeroing_utils_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Raid Group for zeroing tests.
 *
 * @param rg_config_p - the RG config to create.
 *
 * @return None.
 *
 * @author
 *   10/11/2011 - Created. Jason White
 ****************************************************************/

void sep_zeroing_utils_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_object_id_t                             rg_object_id_from_job;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
    									 30000,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rg_object_id_from_job, rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     20000, FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group: %d\n", rg_config_p->raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End sep_zeroing_utils_create_rg() */

/*!****************************************************************************
 *  sep_zeroing_utils_create_lun
 ******************************************************************************
 *
 * @brief
 *  This function creates LUNs for zeroing tests.
 *
 * @param   IN: is_ndb - Non-destructive flag.
 *
 * @return  None
 *
 * @author
 *   10/11/2011 - Created. Jason White
 *****************************************************************************/
void sep_zeroing_utils_create_lun(zeroing_test_lun_t *lun_p,
										 fbe_u32_t           lun_num,
										 fbe_object_id_t     rg_obj_id)
{
    fbe_status_t   status;
    fbe_job_service_error_type_t    job_error_type;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUN ===\n");

    /* Set the Lun number */
    lun_p->fbe_lun_create_req.lun_number = lun_num;

    lun_p->fbe_lun_create_req.raid_group_id = rg_obj_id;
    lun_p->fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    lun_p->fbe_lun_create_req.capacity = 0x2000;
    lun_p->fbe_lun_create_req.addroffset = FBE_LBA_INVALID;
    lun_p->fbe_lun_create_req.ndb_b = FBE_FALSE;
    lun_p->fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    /* We need to set wwn, instead of using the zero value */
    lun_p->fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    lun_p->fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

	status = fbe_api_create_lun(&lun_p->fbe_lun_create_req,
								FBE_TRUE,
								30000,
								&lun_p->lun_object_id,
                                &job_error_type);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "Created LUN: %d\n", lun_p->fbe_lun_create_req.lun_number);

    return;

}/* End sep_zeroing_utils_create_lun() */

fbe_status_t sep_zeroing_utils_wait_for_hook (fbe_scheduler_debug_hook_t *hook_p, fbe_u32_t wait_time)
{
	fbe_u32_t current_time = 0;
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	while (current_time < wait_time){

		status = fbe_api_scheduler_get_debug_hook(hook_p);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		if (status == FBE_STATUS_GENERIC_FAILURE){
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get hook failed with status %d\n", __FUNCTION__, status);
			return status;
		}

		if (hook_p->counter > 0)
		{
			return FBE_STATUS_OK;
		}

		current_time = current_time + SEP_ZEROING_UTILS_POLLING_INTERVAL;
		fbe_api_sleep (SEP_ZEROING_UTILS_POLLING_INTERVAL);
	}

	fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
				  "%s: TIMEOUT %d ms!\n",
				  __FUNCTION__, wait_time);

	return FBE_STATUS_GENERIC_FAILURE;
}

