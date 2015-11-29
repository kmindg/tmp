/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains .
 *
 * @version
 *   8/20/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_rdgen_test_private.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_transport.h"
#include "mut.h"
#include "fbe/fbe_emcutil_shell_include.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

__cdecl main (int argc , char **argv)
{
    mut_testsuite_t *suite_p = NULL;                /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                           /* before proceeding we need to initialize MUT infrastructure */

    suite_p = MUT_CREATE_TESTSUITE("rdgen_test_suite")   /* testsuite is created */

    fbe_rdgen_test_add_unit_tests(suite_p);
    MUT_RUN_TESTSUITE(suite_p);
}

/* Needed to get everything to link. */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}

/*************************
 * end file fbe_rdgen_test_main.c
 *************************/
