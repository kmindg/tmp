/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dogbert_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests the ability of ERP to register for and
 *  handle asynchronous event notification of state changes from
 *  physical package
 *
 * @version
 *   11/10/2009 - Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * dogbert_short_desc = "Asynch event notification and handling of state changes";
char * dogbert_long_desc ="\
This test the ability to register for enclosure state changes and handle them in ERP\n\
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
STEP 2: Removal of Enclosure\n\
        - Using TAPI remove the first enclosure on the loop\n\
STEP 3: Verification\n\
        - Verify that all the three enclosure object goes to a destroyed state\n\
        - Verfiy that ERP also removes the three enclosure from the loop\n\
STEP 4: Addition of Enclosure\n\
        - Using TAPI add the enclosures back on the loop\n\
Step 5: Verify that ERP is notified of all three enclosures and is added back on the loop\n";

/*!**************************************************************
 * dogbert_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test the async notification and
 *  handling
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/12/2009 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void dogbert_test(void)
{
    return;
}
/******************************************
 * end dogbert_test()
 ******************************************/
/*************************
 * end file dogbert_test.c
 *************************/


