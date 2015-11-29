#ifndef RAID_TYPES_H

#define RAID_TYPES_H 0x00000001

/**********************************************************************

* Copyright (C) EMC Corporation 2009

* All rights reserved.

* Licensed material - property of EMC Corporation.

**********************************************************************/

/*

* Define the various raid types outside of the Flare environment.

* Other subsystems such as Auto-tiering currently use them,

* but they should not have to pull in definitions for the rest of FLARE.

*

* Created: 9/23/2009, Alan Sawyer

*/


// These definitions had better correspond to FLARE's

typedef enum SYSTEM_raid_type_tag {

            SYSTEM_RAID_TYPE_MIN = 0,

            SYSTEM_RAID_TYPE_RAIDNONE = 0,

            SYSTEM_RAID_TYPE_RAID5,

            SYSTEM_RAID_TYPE_IDISK,

            SYSTEM_RAID_TYPE_RAID1,

            SYSTEM_RAID_TYPE_RAID0,

            SYSTEM_RAID_TYPE_RAID3,

            SYSTEM_RAID_TYPE_HS,

            SYSTEM_RAID_TYPE_RAID10,

            SYSTEM_RAID_TYPE_Hi5,

            SYSTEM_RAID_TYPE_Hi5_CACHE,

            SYSTEM_RAID_TYPE_RAID6,

            SYSTEM_RAID_TYPE_MAX = SYSTEM_RAID_TYPE_RAID6

}
SYSTEM_RAID_TYPE;

#endif
