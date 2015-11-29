/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_main.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the main function for the logical drive unit tests.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "mut.h"

/* We need to pull this in order to pull in the functions to load the physical and logical 
 * packages. 
 */
#include "../../../../fbe_test/lib/package_config/fbe_test_package_config.c"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 ** Imported functions.
 ******************************/

extern void fbe_logical_drive_add_unit_tests(mut_testsuite_t * suite_p);
extern void fbe_logical_drive_add_io_tests(mut_testsuite_t * suite_p);

/** \fn int __cdecl main (int argc , char ** argv)
 *  \details 
            This is the main function of the fbe_logical_drive Test Suite
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
    fbe_status_t fbe_logical_drive_unit_tests(void);
    fbe_status_t status = fbe_logical_drive_unit_tests();
#endif

    mut_testsuite_t *unit_suite_p;
    mut_testsuite_t *io_suite_p;

#include "fbe/fbe_emcutil_shell_maincode.h"
    mut_init(argc, argv);

    unit_suite_p = MUT_CREATE_TESTSUITE("fbe_logical_drive_unit_test_suite");
    io_suite_p = MUT_CREATE_TESTSUITE("fbe_logical_drive_io_tests_suite");

    fbe_logical_drive_add_unit_tests(unit_suite_p);
    fbe_logical_drive_add_io_tests(io_suite_p);

    MUT_RUN_TESTSUITE(unit_suite_p);

    MUT_RUN_TESTSUITE(io_suite_p);
    
    exit(0);
}
/**************************************
 * end main()
 **************************************/
/*****************************************
 * end fbe_logical_drive_test_main.c
 *****************************************/
