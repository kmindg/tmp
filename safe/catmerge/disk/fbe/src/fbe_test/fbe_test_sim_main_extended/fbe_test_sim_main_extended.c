/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_sim_main_extended.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the fbe test that runs with
 *  extended testing on by default.
 * 
 *  This means that -raid_test_level 1 is on by default.
 *
 * @version
 *   9/20/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "mut.h"
#include "fbe_test.h"
#include "sep_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 *   LOCAL FUNCTION PROTOTYPES
 ******************************/

/*****************************
 *   GLOBALS
 ******************************/

/*!**************************************************************
 * main()
 ****************************************************************
 * @brief
 *  This is the main entry point for the fbe test executible
 *  with -raid_test_level 1.
 *
 * @param argc
 * @param argv
 *
 * @return int - The return value from fbe_test_main()
 *
 * @author
 *  9/20/2011 - Created. Rob Foley
 *
 ****************************************************************/
#define FBE_TEST_EXTENDED_TIMEOUT 3600
int __cdecl main (int argc , char **argv)
{
#include "fbe/fbe_emcutil_shell_maincode.h"
    /* Set the default raid test level so that we run this test with raid test level 1.
     */
    fbe_test_set_default_extended_test_level(1);
    return fbe_test_main(argc, argv, FBE_TEST_EXTENDED_TIMEOUT);
}
/******************************************
 * end main()
 ******************************************/

/****************************
 * end file fbe_test_sim_main_extended.c
 ****************************/
