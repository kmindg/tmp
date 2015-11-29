#ifndef SEP_ZEROING_UTILS_H
#define SEP_ZEROING_UTILS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sep_zeroing_utils.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the defines and function prototypes for the zeroing
 *   service tests.
 *
 * @version
 *   10/10/2011:  Created. Jason White.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "fbe/fbe_terminator_api.h"                 //  for fbe_terminator_sas_drive_info_t
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"

//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):

/*!*******************************************************************
 * @def SEP_ZEROING_UTILS_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing complete
 *
 *********************************************************************/
#define SEP_ZEROING_UTILS_POLLING_INTERVAL 100 /*ms*/

//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS:
//

typedef struct zeroing_test_lun_s
{
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_object_id_t         lun_object_id;
}zeroing_test_lun_t;

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES:

// Check the disk zeroing status
fbe_status_t sep_zeroing_utils_check_disk_zeroing_status(fbe_object_id_t object_id,
                                            fbe_u32_t   timeout_ms,
                                            fbe_lba_t zero_checkpoint);

// Check that disk zeroing has stopped
fbe_status_t sep_zeroing_utils_check_disk_zeroing_stopped(fbe_object_id_t object_id);

fbe_status_t sep_zeroing_utils_wait_for_disk_zeroing_to_start(fbe_object_id_t object_id,
                                            fbe_u32_t   timeout_ms);

void sep_zeroing_utils_create_rg(fbe_test_rg_configuration_t *rg_config_p);

void sep_zeroing_utils_create_lun(zeroing_test_lun_t *lun_p,
										 fbe_u32_t           lun_num,
										 fbe_object_id_t     rg_obj_id);

fbe_status_t sep_zeroing_utils_wait_for_hook (fbe_scheduler_debug_hook_t *hook_p, fbe_u32_t wait_time);

fbe_status_t sep_zeroing_utils_is_zeroing_in_progress(fbe_object_id_t object_id, fbe_bool_t *zeroing_in_progress);

#endif // SEP_ZEROING_UTILS_H
