/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_unit_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main function for the parity unit tests.
 *
 * @revision
 *  11/2/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 ** Imported functions.
 ******************************/
extern void      fbe_parity_add_unit_tests(mut_testsuite_t * const suite_p);

/*!**************************************************************
 * main()
 ****************************************************************
 * @brief
 *  This is the main entry point of the raid group unit test.
 *
 * @param argc - argument count
 * @argv  argv - argument array
 *
 * @return int 0   
 *
 * @author
 *  11/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *unit_suite_p;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    unit_suite_p = MUT_CREATE_TESTSUITE("fbe_parity_unit_test_suite");

    fbe_parity_add_unit_tests(unit_suite_p);
    MUT_RUN_TESTSUITE(unit_suite_p);
    exit(0);
}
/**************************************
 * end main()
 **************************************/

/*****************************************
 * end fbe_parity_unit_test_main.c
 *****************************************/
