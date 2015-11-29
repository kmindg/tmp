/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_private_space_layout.c
 *
 * @brief
 *  This file describes the Private Space Layout, and contains functions
 *  used to access it.
 *
 * @version
 *   2011-04-27 - Created. Matthew Ferson
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe/fbe_disk_block_correct.h"

static fbe_bool_t is_destroy_needed = FBE_TRUE;

/*!**************************************************************
 * fbe_disk_block_correct_initialize_user()
 ****************************************************************
 * @brief
 *  Initialize the fbe_api
 *
 * @param 
 *  None
 *
 * @return 
 *  status - fbe status
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_initialize_user(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* initilize the fbe api user */
    status  = fbe_api_common_init_user(FBE_TRUE);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init fbe api user\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
	}
	/*this part is taking care of getting notifications from jobs and letting FBE APIs know the job completed*/
	status  = fbe_api_common_init_job_notification();
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
	}

    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_destroy_fbe_api_sim()
 ****************************************************************
 * @brief
 *  Destroy the fbe_api
 *
 * @param 
 *  None
 *
 * @return 
 *  None
 *
 ****************************************************************/
void __cdecl fbe_disk_block_correct_destroy_fbe_api_user(void)
{
	fbe_api_trace(FBE_TRACE_LEVEL_INFO,"\nDestroying fbe api...");

    if(is_destroy_needed)
    {
        /* Set this to FALSE when we call this function */
        is_destroy_needed = FBE_FALSE;
        fbe_api_common_destroy_user();
        //closeOutputFile(outFileHandle);

        fflush(stdout);
    }

    return;
}


