/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_sim_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the fbe test.
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
 *  This is the main entry point for the fbe test executible.
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
#define FBE_TEST_NOMINAL_TIMEOUT 1000

int __cdecl main (int argc , char **argv)
{
#include "fbe/fbe_emcutil_shell_maincode.h"
    /*timeout value is 1000 for regular test mode */
    return fbe_test_main(argc, argv, FBE_TEST_NOMINAL_TIMEOUT);
}
/******************************************
 * end main()
 ******************************************/

/****************************
 * end file fbe_test_sim_main.c
 ****************************/
