/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_enclosure_test_main.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the main function for the enclosure tests.
 *
 * HISTORY
 *   7/14/2008:  Created. bphilbin -- Thanks Rob!
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "mut.h"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 ** Imported functions.
 ******************************/

extern void fbe_enclosure_add_unit_tests(mut_testsuite_t * suite_p);

/** \fn int __cdecl main (int argc , char ** argv)
 *  \details 
            This is the main function of the fbe_enclosure Test Suite
    \param
            argc argument count
            argv argument array 
 */
int __cdecl main (int argc , char ** argv)
{
    /* Occasionally we might like to execute without MUT.
     * In this case we'll use the below function to simply execute tests.
     */
#if 0
    fbe_status_t fbe_enclosure_unit_tests(void);
    fbe_status_t status = fbe_enclosure_unit_tests();
#endif

    mut_testsuite_t *unit_suite_p;

#include "fbe/fbe_emcutil_shell_maincode.h"
    mut_init(argc, argv);

    unit_suite_p = MUT_CREATE_TESTSUITE("fbe_enclosure_unit_test_suite");
    
    fbe_enclosure_add_unit_tests(unit_suite_p);

    MUT_RUN_TESTSUITE(unit_suite_p);

    exit(0);
}
/*****************************************
 * end fbe_enclosure_test_main.c
 *****************************************/
