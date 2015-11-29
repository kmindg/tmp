#ifndef FBE_RAID_GROUP_EXECUTOR_H
#define FBE_RAID_GROUP_EXECUTOR_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_journal.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the private defines and function prototypes for the
 *   executor code for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *  01/26/2012  Ron Proulx  - Created.
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

/* fbe_raid_group_executor.c
 */
fbe_status_t fbe_raid_group_iots_paged_metadata_update_completion(fbe_packet_t * packet_p, 
                                                                  fbe_packet_completion_context_t context);

#endif // FBE_RAID_GROUP_EXECUTOR_H
