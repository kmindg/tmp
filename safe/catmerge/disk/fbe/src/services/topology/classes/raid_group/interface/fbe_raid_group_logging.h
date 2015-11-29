#ifndef FBE_RAID_GROUP_LOGGING_H
#define FBE_RAID_GROUP_LOGGING_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_logging.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the private defines and function prototypes for the
 *   event logging code for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/25/2010:  Created. Jean Montes.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//

#include "fbe_raid_group_object.h"             // for the raid group object 


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 


//  Log a message to the event log that all rebuilds/copies have completed 
fbe_status_t fbe_raid_group_logging_log_all_complete(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p, 
                                    fbe_u32_t                       in_position,
                                    fbe_u32_t                       in_bus,
                                    fbe_u32_t                       in_encl,
                                    fbe_u32_t                       in_slot);

//  Log a message to the event log that a rebuild/copy of a LUN has started or completed 
fbe_status_t fbe_raid_group_logging_break_context_log_and_checkpoint_update(fbe_raid_group_t *raid_group_p,
                                                              fbe_packet_t *packet_p,
                                                              fbe_raid_position_bitmask_t checkpoint_change_bitmask,
                                                              fbe_lba_t checkpoint_update_lba,
                                                              fbe_block_count_t checkpoint_update_blocks);

fbe_status_t fbe_raid_group_logging_log_lun_bv_start_or_complete_if_needed(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_lba_t                   in_start_lba);


//  Log a message to the event log that rebuild logging is marked on a disk in a RG
fbe_status_t fbe_raid_group_logging_log_rl_started(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_u32_t                       in_position);

//  Log a message to the event log that NR-on-demand is cleared on a disk in a RG
fbe_status_t fbe_raid_group_logging_log_rl_cleared(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_u32_t                       in_position);


#endif // FBE_RAID_GROUP_LOGGING_H
