/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file inspector_clouseau_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests the Energy Information Reporting capabilities of both
 *  the Physical Package & ESP.  A known set of values for both InputPower
 *  and AirInletTemp will be provided by the Terminator and it will be 
 *  verified that ESP calculates the correct stats (especially Max &
 *  Average values).
 *
 * @version
 *   11/30/2009 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * inspector_clouseau_short_desc = "Energy Info Reporting (EIR)";
char * inspector_clouseau_long_desc ="\
Tests the Energy Info Reporting (EIR) processing in both:
    Physical Package - providing the necessary values\n\
    ESP - processing of values to generate EIR statistics\n\
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
STEP 2: Generate EIR field values\n\
        - Using TAPI generate known PS InputPower & TempSensor AirInletTemp values\n\
        - These values will change every sample interval (30 secs)
STEP 3: Verification\n\
        - Verify that the ESP calculated EIR stats (Current, Max, Average) are correct\n\
STEP 4: Verify the handling of removing an enclosure\n\
        - Verify that the EIR stats & status correclty reflect an enclosure was removed\n\
STEP 5: Verify the handling of a faulted enclosure\n\
        - Verify that the EIR stats & status correclty reflect an enclosure was faulted\n";

/*!**************************************************************
 * inspector_clouseau_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test EIR statistics processing.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/30/2009 - Created. Joe Perry
 *
 ****************************************************************/

void inspector_clouseau_test(void)
{
    return;
}
/******************************************
 * end inspector_clouseau_test()
 ******************************************/
/*************************
 * end file inspector_clouseau_test.c
 *************************/


