#ifndef DORA_TEST_H
#define DORA_TEST_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dora_test.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the defines and function prototypes for the Dora 
 *   test which are shared with the other tests in the NR/rebuild suite 
 *   (Diego, Boots, etc). 
 * 
 * @version
 *   03/01/2010:  Created. Jean Montes.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "fbe/fbe_terminator_api.h"                 //  for fbe_terminator_sas_drive_info_t


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//  Retry parameters when waiting for an action or state 
#define DORA_TEST_MAX_RETRIES               80      // retry count - number of times to loop
#define DORA_TEST_RETRY_TIME                250     // wait time in ms, in between retries 

//  Common configuration parameters 
#define DORA_TEST_LUNS_PER_RAID_GROUP       3       // number of LUNs in the RG 
#define DORA_TEST_ELEMENT_SIZE              128     // element size 
#define DORA_TEST_CHUNKS_PER_LUN            3       // number of chunks per LUN
#define DORA_TEST_RG_CAPACITY               0x32000 // raid group's capacity
#define DORA_TEST_DB_DRIVES_RG_CAPACITY     0xE000  // raid group's capacity 
#define DORA_TEST_FIRST_REMOVED_DRIVE_POS   1       // position in the rg/edge of the drive that is removed first
#define DORA_TEST_LAST_REMOVED_DRIVE_POS    0       // position in the rg/edge of the drive that is removed last
#define DORA_TEST_SECOND_REMOVED_DRIVE_POS  2       // position in the rg/edge of the drive that is removed second

//  Bus and enclosure for the disks in the RG.
//  Enclosure 0-1 will be reserved for Hot Spares. 
#define DORA_TEST_PORT                      0       // port (bus) 0 
#define DORA_TEST_ENCLOSURE_FIRST           0       // enclosure 1
#define DORA_TEST_ENCLOSURE_SECOND          1       // enclosure 2
#define DORA_TEST_ENCLOSURE_THIRD           2       // enclosure 3
#define DORA_TEST_ENCLOSURE_FOURTH          3       // enclosure 4
#define DORA_TEST_HOT_SPARE_ENCLOSURE       1       // enclosure 1 - all Hot Spares are located here

//  RAID group-specific configuration parameters 
#define DORA_TEST_NUM_RAID_GROUPS           4       // number of RGs 

#define DORA_TEST_R5_RAID_GROUP_WIDTH       3       // number of drives in the RAID-5 RG 
#define DORA_TEST_R1_RAID_GROUP_WIDTH       2       // number of drives in the RAID-1 RG 
#define DORA_TEST_TRIPLE_MIRROR_RAID_GROUP_WIDTH    3   
                                                    // number of drives in the triple mirror RAID-1 RG 
#define DORA_TEST_R6_RAID_GROUP_WIDTH       6       // number of drives in a RAID-6 RG
#define DORA_TEST_R6_RAID_GROUP_2_WIDTH     4       // number of drives in another RAID-6 RG
#define DORA_TEST_DB_DRIVES_RAID_GROUP_WIDTH 3      // number of drives in the user RG on the DB drives     
#define DORA_TEST_DUMMY_RAID_GROUP_WIDTH    2       // number of drives in dummy RG   
#define DORA_TEST_R10_RAID_GROUP_WIDTH      4       // number of drives in the RAID-10 RG  

//  Disk slots for the RAID groups and Hot Spares  
#define DORA_TEST_R5_2_SLOT_1               0       // slot for 1st disk in RAID-5: VD1 - 0-2-0 - rg 9 position 0
#define DORA_TEST_R5_2_SLOT_2               1       // slot for 2nd disk in RAID-5: VD1 - 0-2-1 - rg 9 position 1
#define DORA_TEST_R5_2_SLOT_3               2       // slot for 3rd disk in RAID-5: VD1 - 0-2-2 - rg 9 position 2
#define DORA_TEST_HS_SLOT_0_2_3             3       // slot for hot spare disk          - 0-2-3 

#define DORA_TEST_DUMMY_SLOT_1              8       // slot for 1st disk in dummy RG: 0-2-8
#define DORA_TEST_DUMMY_SLOT_2              9       // slot for 2nd disk in dummy RG: 0-2-9

#define DORA_TEST_DB_DRIVES_SLOT_0          0       // slot for 1st disk in user RG on DB drives: 0-0-0
#define DORA_TEST_DB_DRIVES_SLOT_1          1       // slot for 2nd disk in user RG on DB drives: 0-0-1
#define DORA_TEST_DB_DRIVES_SLOT_2          2       // slot for 3rd disk in user RG on DB drives: 0-0-2

//  Disk positions in the RG  
#define DORA_TEST_POSITION_0                0       // disk/edge position 0
#define DORA_TEST_POSITION_1                1       // disk/edge position 1
#define DORA_TEST_POSITION_2                2       // disk/edge position 2

 // Max time in seconds to wait for all error injections
#define DORA_TEST_MAX_ERROR_INJECT_WAIT_TIME_SECS   15  

 // Max sleep for quiesce test
#define DORA_TEST_QUIESCE_TEST_MAX_SLEEP_TIME       1000

//  Max time wait for state changes
#define DORA_TEST_WAIT_SEC                          60

//  Threads per LUN for rdgen tests
#define DORA_TEST_THREADS_PER_LUN                   8


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

#endif // DORA_TEST_H
