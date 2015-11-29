#ifndef FBE_RAID_GROUP_EXPANSION_H
#define FBE_RAID_GROUP_EXPANSION_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the private defines for the raid group expansion.
 *
 * @version
 *   10/03/2013:  Created. Shay Harel
 *
 ***************************************************************************/

/*start the expansion process*/
fbe_status_t fbe_raid_group_usurper_start_expansion(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_start_expansion(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p, fbe_lba_t new_capacity);

fbe_lifecycle_status_t fbe_raid_group_change_config_active(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_change_config_passive(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_change_config_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_change_config_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
#endif /*FBE_RAID_GROUP_EXPANSION_H*/
