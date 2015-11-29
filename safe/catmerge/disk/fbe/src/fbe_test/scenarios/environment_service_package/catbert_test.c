/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file catbert_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests the enclosure firmware upgrade processing.
 *
 * @version
 *   12/04/2009 - Created. GB
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * catbert_short_desc = "Enclosure Firmware Upgade using the fbe_api interface.";
char * catbert_long_desc ="\
Test the ability to perform firmware upgrade to the enclosure lcc and power supply.\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n"
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Validate that all services in ESP is started correctly\n\
        - Validate that ERP is able to see 3 Viper enclosures\n\
STEP 2: Verify enclosure firmware download.\n\
        - Validate EFUP is notified as each enclosure is added.\n\
        - Validate EFUP downloads image to each lcc.\n\
        - Validae EFUP activates image on each lcc.\n\
        - Validate EFUP downloads image to each ps.\n\
        - Validate EFUP activates image to each ps.\n\
STEP 4: Remove PS from Enclosure\n\
        - Using TAPI to remove a power supply from an enclosure.\n\
STEP 4: Addition of Power Supply\n\
        - Using TAPI insert the power supply back in the enclosure\n\
Step 5: Verify that EFUP is notified of PS insertion\n";

/*!**************************************************************
 * catbert_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test enclosure firmware download.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/04/2009 - Created. GB
 *
 ****************************************************************/
void catbert_test(void)
{
    return;
}
/******************************************
 * end catbert_test()
 ******************************************/
/*************************
 * end file catbert_test.c
 *************************/
