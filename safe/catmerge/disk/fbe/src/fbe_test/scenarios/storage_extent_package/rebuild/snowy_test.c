/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file snowy_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a LU ownership test for LU objects.
 *
 * @version
 *   9/14/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * snowy_short_desc = "SP failover logic verification";
char * snowy_long_desc =
"SP Failover Scenario (snowy) tests the ability of the system to continue monitor activities when one of the SP's failed. \n"
"It covers the following cases: \n"
"- A PVD is not connected to the VD and perform zeroing. The scenario verifies that PVD zeroing checkpoint is advancing.  \n"
"        The scenario is also verifies that PVD zeroing checkpoint continue to advance after ACTIVE SP was shut down. \n"
"- A VD is not connected to the RAID and perform proactive copy. The scenario verifies that VD proactive copy checkpoint is advancing.  \n"
"        The scenario is also verifies that VD proactive copy checkpoint continue to advance after ACTIVE SP was shut down. \n"
"- A RAID is performing rebuild. The scenario verifies that RAID rebuild checkpoint is advancing.  \n"
"        The scenario is also verifies that RAID rebuild checkpoint continue to advance after ACTIVE SP was shut down. \n"
"Starting Config: \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] two SAS drives (PDO-0 and PDO-1)) \n"
"        [PP] two logical drives (LD-0 and LD-1) \n"
"        [SEP] two provision drives (PVD-0 and PVD-1) \n"
"        [SEP] two virtual drives (VD-0 and VD-1) \n"
"        [SEP] raid object (mirror) \n"
"        [SEP] two LUN objects (LUN-0 and LUN-1) \n"
"STEP 1: Bring up the initial topology. \n"
"        - Create and verify the initial physical package config. \n"
"STEP 2: Single PVD test. \n"
"        - Create PVD object \n"
"        - Attach PVD to LDO \n"
"        - Verify that zeroing checkpoint advanced after 15 seconds. \n"
"        - Shut down SPA (active SP; may be simulated via cmi_shim). \n"
"        - Verify that zeroing checkpoint advanced after 15 seconds. \n"
"STEP 2: Single VD test. \n"
"        - Create two PVD objects \n"
"        - Attach PVD's to LDO's \n"
"        - Create VD object and instruct it to perorm proactive copy \n"
"        - Verify that proactive copy checkpoint advanced after 15 seconds. \n"
"        - Shut down SPA (active SP; may be simulated via cmi_shim). \n"
"        - Verify that proactive copy checkpoint advanced after 15 seconds. \n"
"STEP 2: Single RAID test. \n"
"        - Create two PVD objects \n"
"        - Attach PVD's to LDO's \n"
"        - Create two VD objects \n"
"        - Create RAID mirror object and instruct it to perform rebuild of VD1 \n"
"        - Verify that rebuild checkpoint advanced after 15 seconds. \n"
"        - Shut down SPA (active SP; may be simulated via cmi_shim). \n"
"        - Verify that rebuild checkpoint advanced after 15 seconds. \n";

void snowy_test(void)
{
    return;
}

/*************************
 * end file snowy_test.c
 *************************/


