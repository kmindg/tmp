/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file eeyore_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains background verify tests with hibernation of the raid 
 *   object.
 *
 * @version
 *   12/07/2009 - Created. Moe Valois
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe_test_package_config.h"                //  for fbe_test_load_sep()
#include "fbe/fbe_api_common.h"                     //  for fbe_api_set_simulation_io_and_control_entries


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* eeyore_short_desc = "RAID Group Hibernation and Verify";
char* eeyore_long_desc =
    "\n"
    "\n"
    "The Eeyore scenario tests interactions between hibernation and verify.\n"
    "It tests that the RAID object will continue verifying if a hibernate\n"
    "request is sent to it, and hibernate on both SPs after the verify completes.\n"
    "\n"
    "Dependencies:\n"
    "    - Metadata Service infrastructure.\n"
    "    - Hibernation infrastructure.\n"
    "    - RAID other objects support hibernation (Aurora scenario)\n"
    "    - RAID object supports verify (Clifford scenario)\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada boards\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 4 SAS drives (PDOs)\n"
    "    [PP] 4 logical drives (LDs)\n"
    "    [SEP] 4 provision drives (PVDs)\n"
    "    [SEP] 3 virtual drives (VDs)\n"
    "    [SEP] 1 RAID object (RAID-5)\n"
    "    [SEP] 3 LUN objects \n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Set idle time to hibernate to 5 seconds\n" 
    "\n"
    "STEP 3: Start I/O to all of the LUNs\n"
    "   - Use rdgen to write (continuously) to each LUN\n"
    "\n"    
    "STEP 4: Initiate a raid group verify operation\n" 
    "    - Verify that the RAID object starts verify I/O\n"  
    "\n"
    "STEP 5: Stop I/O to all LUNs\n" 
    "\n"
    "STEP 6: Wait 6 seconds and verify the RAID object does not hibernate\n"
    "    - Verify that the LUN objects is in the HIBERNATE state\n"
    "    - Verify that the RAID object on both SPs is in the READY state\n"
    "\n"
    "STEP 7: Wait for the verify to complete\n" 
    "    - Verify that the verify operation completed\n"
    "\n"
    "STEP 8: Wait 6 seconds and verify the RAID object enters hibernate\n"
    "    - Verify that the RAID object on both SPs is in the HIBERNATE state\n"
    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  eeyore_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Eeyore test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void eeyore_setup(void)
{  
    return;

} //  End eeyore_setup()


/*!****************************************************************************
 *  eeyore_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Eeyore test.  The test does the 
 *   following:
 *
 *   - TBD
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void eeyore_test(void)
{
    return;

} //  End eeyore_test()




