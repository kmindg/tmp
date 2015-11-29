/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cato_fong_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests the Gen3 Power Supply firmware upgrade.
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
char * cato_fong_short_desc = "Cato Fong: Power Supply firmware download test";
char * cato_fong_long_desc ="\
Test Gen3 Power Supply firmware upgrade\n\
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
STEP 2: Remove/Insert a power supply\n\
        - Using TAPI remove the power supply from 2nd enclosure on the loop\n\
        - Verify power supply is removed\n\
        - Using TAPI insert the power supply in 2nd enclosure on the loop\n\
STEP 3: Verify that firmware upgrade occurs to the power supply in the 2nd enclosure on the loop\n";

/*!**************************************************************
 * cato_fong_test() 
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
 *  12/04/2009 - Created. GB
 *
 ****************************************************************/
void cato_fong_test(void)
{
    return;
}
/******************************************
 * end cato_fong_test()
 ******************************************/
/*************************
 * end file cato_fong_test.c
 *************************/
