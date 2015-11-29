/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file swiper_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of rebuild due to a "bad drive signature" / 
 *   "cru sig error".  
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

char* swiper_short_desc = "Rebuild due to 'Bad Drive Signature'";
char* swiper_long_desc =
    "\n"
    "\n"
    "The Swiper Scenario tests that if a healthy RAID-5 RAID group is shutdown/ \n"  
    "taken offline, and a drive is replaced, that drive is marked Needs Rebuild. \n"  
    "It also tests that the rebuild is performed when the RAID group is brought \n"
    "back online.\n"    
    "\n"
    "THIS TEST HAS NOT BEEN IMPLEMENTED YET\n"
    "\n"
    "Planned steps:\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Write data to the LUN\n"
    "   - Use rdgen to write a seed pattern to the LUN\n"
    "   - Wait for writes to complete before proceeding\n"
    "\n"
    "STEP 3: Remove the viper enclosure\n" 
    "   - Remove the enclosure\n"
    "   - Verify that the 4 PVDs are in the FAIL state \n"      
    "   - Verify that the 3 VDs are in the FAIL state  \n"     
    "   - Verify that the RAID object is in the FAIL state \n"
    "   - Verify that the LUN object is in the FAIL state\n"
    "\n"
    "STEP 4: Replace drive A with the unbound drive (drive D)\n"
    "   - VD-A should connect to PVD-D\n"
    "\n"
    "STEP 5: Connect the viper enclosure\n"
    "   - Connect the enclosure\n"
    "\n"
    "STEP 6: Verify Needs Rebuild is processed and VD-A becomes READY\n"
    "   - Verify that the rebuild checkpoint is set to 0 for drive A\n"
    "   - Verify that Needs Rebuild is set for drive A  \n"       
    "   - Verify that VD-A is now in the READY state \n"    
    "   - Verify that the RAID object is in the READY state\n"
    "   - Verify that the LUN object is in the READY state\n"
    "\n"
    "STEP 7: Verify completion of the rebuild operation\n" 
    "   - Verify that the Needs Rebuild chunk count is 0\n"
    "   - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END\n"
    "\n"
    "STEP 8: Verify that the rebuilt data is correct\n" 
    "   - Use rdgen to validate that the rebuilt data is correct\n"
    "\n"
    "STEP 9: Cleanup\n"
    "   - Destroy objects\n"
    "\n"
    "\n"
    "*** Swiper Phase II ****************************************************** \n"
    "\n"
    "- Test multiple LUNs in the RG with I/O to each\n"
    "- Test all RAID types\n"
    "\n"
    "Description last updated: 10/28/11 \n"
    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//  Configuration parameters 
#define SWIPER_TEST_RAID_GROUP_WIDTH          3         // number of drives in the RG 
#define SWIPER_TEST_RAID_GROUP_DATA_DRIVES    (SWIPER_TEST_RAID_GROUP_WIDTH - 1)      
                                                        // number of data drives in the RG for a RAID 5
#define SWIPER_TEST_LUNS_PER_RAID_GROUP       1         // number of LUNs in the RG 
#define SWIPER_TEST_ELEMENT_SIZE              128       // element size 
#define SWIPER_TEST_VD_CAPACITY_IN_BLOCKS     0x1000    // virtual drive capacity 
#define SWIPER_TEST_RG_CAPACITY_IN_BLOCKS     (SWIPER_TEST_VD_CAPACITY_IN_BLOCKS * \
                                               SWIPER_TEST_RAID_GROUP_DATA_DRIVES)
                                                        // capacity of RG based on cap/drive and number of drives 
#define SWIPER_TEST_RAID_GROUP_CHUNK_SIZE     (8 * FBE_RAID_SECTORS_PER_ELEMENT) 
                                                        // number of blocks in a raid group bitmap chunk

//  Define the bus-enclosure-disk for the disks in the RG 
#define SWIPER_TEST_PORT                      0         // port (bus) 0 
#define SWIPER_TEST_ENCLOSURE                 0         // enclosure 0 
#define SWIPER_TEST_SLOT_1                    4         // slot for first disk in the RG: VD1  - 0-0-4 
#define SWIPER_TEST_SLOT_2                    5         // slot for second disk in the RG: VD2 - 0-0-5 
#define SWIPER_TEST_SLOT_3                    6         // slot for third disk in the RG: VD3  - 0-0-6 

//  Set of disks to be used - disks 0-0-4 through 0-0-6
fbe_test_raid_group_disk_set_t fbe_swiper_disk_set[SWIPER_TEST_RAID_GROUP_WIDTH] = 
{
    {SWIPER_TEST_PORT, SWIPER_TEST_ENCLOSURE, SWIPER_TEST_SLOT_1},     // 0-0-4
    {SWIPER_TEST_PORT, SWIPER_TEST_ENCLOSURE, SWIPER_TEST_SLOT_2},     // 0-0-5
    {SWIPER_TEST_PORT, SWIPER_TEST_ENCLOSURE, SWIPER_TEST_SLOT_3}      // 0-0-6
};  

//  Configuration setup 
enum swiper_config_raid5_e          
{
    SWIPER_PVD1_ID  = 0,                                // PVDs and VDs for 3 disks 
    SWIPER_PVD2_ID,
    SWIPER_PVD3_ID,
    SWIPER_VD1_ID,
    SWIPER_VD2_ID,
    SWIPER_VD3_ID,
    SWIPER_RAID_ID,                                     // one RG always
    SWIPER_LUN1_ID,                                     // one LUN 
    SWIPER_MAX_ID                                       // not sure why max is > LUN1_ID 
};


//  Number of physical objects in the test configuration.  This is needed for determining if drive has been 
//  fully removed (objects destroyed) or fully inserted (objects created)
fbe_u32_t   swiper_test_number_physical_objects_g;


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  swiper_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Swiper test.  The test does the 
 *   following:
 *
 *   - Creates a RAID-5 configuration (indirectly via the swiper_setup function)
 *   - TBD
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void swiper_test(void)
{

    //  Return 
    return;

} //  End swiper_test()


/*!****************************************************************************
 *  swiper_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Swiper test.  It creates the objects 
 *   for a RAID-5 RG based on the defined configuraton.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void swiper_setup(void)
{  
                                    
    //  Return
    return;

} //  End swiper_setup()


