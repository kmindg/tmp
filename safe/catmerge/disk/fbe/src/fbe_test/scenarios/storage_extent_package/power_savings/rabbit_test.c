/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file rabbit_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of hibernation with rebuild on dual SP.  
 *
 * @version
 *   12/01/2009 - Created. Jean Montes
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_sim_server.h"                 //  for fbe_api_sim_server_...


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* rabbit_short_desc = "RAID Group Hibernation and Rebuild - Dual SP";
char* rabbit_long_desc =
    "\n"
    "\n"
    "The Rabbit Scenario tests interactions between hibernation and rebuild on\n"
    "dual-SP, for a RAID-5.  It tests that the RAID group will continue rebuilding\n"
    "if a hibernate request is sent to it, and only hibernate after the rebuild\n" 
    "completes.\n"
    "\n"
    "Dependencies:\n"
    "   - RAID group object supports hibernation in dual-SP mode (Woozle \n"
    "     scenario)\n"
    "   - RAID group object supports hibernation and rebuild in single-SP mode\n"
    "     (Pooh scenario)\n"
    "   - RAID group object supports rebuild (Diego scenario)\n"
    "   - RAID group object supports degraded I/O for a RAID-5 (Big Bird  \n"
    "     scenario)\n"
    "\n"
    "Starting Config:\n"
    "Starting Config:\n"
    "    [PP] 1 - armada boards\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 3 - viper enclosure\n"
    "    [PP] 60 - SAS drives (PDOs)\n"
    "    [PP] 60 - logical drives (LDs)\n"
    "    [SEP] 60 - provision drives (PVDs)\n"
    "    [SEP] 1 - RAID object (RAID-5)\n"
    "    [SEP] 1 - LUN object \n"
    "\n"
    "\n"
    "STEP 1: Bring up SP A and SP B with the initial topology\n"
    "\n"
    "STEP 2: Configure a Hot Spare drive\n"
    "\n"
    "STEP 3: Set idle time to hibernate to 5 seconds\n" 
    "\n"
    "STEP 4: Start I/O to all of the LUNs on SP A and SP B\n"
    "   - Use rdgen to write (continuously) to each LUN\n"
    "\n"    
    "STEP 5: Remove one of the drives in the RG (drive A)\n"
    "    - Remove the physical drive (PDO-A)\n"
    "    - Verify that VD-A is now in the FAIL state\n"  
    "    - Verify that the RAID object is in the READY state\n"
    "\n"
    "STEP 6: Verify that the Hot Spare swaps in for drive A\n"
    "    - Verify that VD-A is now in the READY state\n"  
    "\n"
    "STEP 7: Verify that rebuild is in progress\n" 
    "    - Verify that the RAID object updates the rebuild checkpoint as needed\n"  
    "\n"
    "STEP 8: Stop I/O to all the LUNs on both SPs\n" 
    "\n"
    "STEP 9: Wait 6 seconds and verify the RAID object does not hibernate\n"
    "    - Verify that the RAID object is in the READY state\n"
    "\n"
    "STEP 10: Wait for the rebuild to complete\n" 
    "    - Verify that the Needs Rebuild chunk count is 0\n"
    "    - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END\n"
    "\n"
    "STEP 11: Wait 6 seconds and verify the RAID object enters hibernate\n"
    "    - Verify that the RAID object is in the HIBERNATE state\n"
    "\n"
    "STEP 12: Cleanup\n"
    "    - Destroy objects\n"
    "\n"
    "\n"
    "*** Rabbit Phase II ********************************************************* \n"
    "\n"
    "- Test all RAID types\n"
    "\n"
    "\n"
    "Description last updated: 10/27/2011.\n"
    "\n"
    "\n";


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//  Configuration parameters 
#define RABBIT_TEST_RAID_GROUP_WIDTH          3         // number of drives in the RG 
#define RABBIT_TEST_RAID_GROUP_DATA_DRIVES    (RABBIT_TEST_RAID_GROUP_WIDTH - 1)      
                                                        // number of data drives in the RG for a RAID 5
#define RABBIT_TEST_LUNS_PER_RAID_GROUP       1         // number of LUNs in the RG 
#define RABBIT_TEST_ELEMENT_SIZE              128       // element size 
#define RABBIT_TEST_VD_CAPACITY_IN_BLOCKS     0x1000    // virtual drive capacity 
#define RABBIT_TEST_RG_CAPACITY_IN_BLOCKS     (RABBIT_TEST_VD_CAPACITY_IN_BLOCKS * \
                                               RABBIT_TEST_RAID_GROUP_DATA_DRIVES)
                                                        // capacity of RG based on cap/drive and number of drives 
#define RABBIT_TEST_RAID_GROUP_CHUNK_SIZE     (8 * FBE_RAID_SECTORS_PER_ELEMENT) 
                                                        // number of blocks in a raid group bitmap chunk

//  Define the bus-enclosure-disk for the disks in the RG 
#define RABBIT_TEST_PORT                      0         // port (bus) 0 
#define RABBIT_TEST_ENCLOSURE                 0         // enclosure 0 
#define RABBIT_TEST_SLOT_1                    4         // slot for first disk in the RG: VD1  - 0-0-4 
#define RABBIT_TEST_SLOT_2                    5         // slot for second disk in the RG: VD2 - 0-0-5 
#define RABBIT_TEST_SLOT_3                    6         // slot for third disk in the RG: VD3  - 0-0-6 

//  Set of disks to be used - disks 0-0-4 through 0-0-6
fbe_test_raid_group_disk_set_t fbe_rabbit_disk_set[RABBIT_TEST_RAID_GROUP_WIDTH] = 
{
    {RABBIT_TEST_PORT, RABBIT_TEST_ENCLOSURE, RABBIT_TEST_SLOT_1},     // 0-0-4
    {RABBIT_TEST_PORT, RABBIT_TEST_ENCLOSURE, RABBIT_TEST_SLOT_2},     // 0-0-5
    {RABBIT_TEST_PORT, RABBIT_TEST_ENCLOSURE, RABBIT_TEST_SLOT_3}      // 0-0-6
};  

//  Configuration setup 
enum rabbit_config_raid5_e          
{
    RABBIT_PVD1_ID  = 0,                                // PVDs and VDs for 3 disks 
    RABBIT_PVD2_ID,
    RABBIT_PVD3_ID,
    RABBIT_VD1_ID,
    RABBIT_VD2_ID,
    RABBIT_VD3_ID,
    RABBIT_RAID_ID,                                     // one RG always
    RABBIT_LUN1_ID,                                     // one LUN 
    RABBIT_MAX_ID                                       // not sure why max is > LUN1_ID 
};


//  Number of physical objects in the test configuration.  This is needed for determining if drive has been 
//  fully removed (objects destroyed) or fully inserted (objects created)
fbe_u32_t   rabbit_test_number_physical_objects_g;


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  rabbit_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Rabbit test.  The test does the 
 *   following:
 *
 *   - Creates a RAID-5 configuration (indirectly via the rabbit_setup function)
 *   - TBD
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void rabbit_test(void)
{

    //  Return 
    return;

} //  End rabbit_test()


/*!****************************************************************************
 *  rabbit_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Rabbit test.  It creates the objects 
 *   for a RAID-5 RG based on the defined configuraton.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void rabbit_setup(void)
{  
    //  Return
    return;

} //  End rabbit_setup()


