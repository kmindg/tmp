/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the unit test for lun.
 * 
 * @ingroup lun_unit_test_files
 * 
 * @version
 *   01/06/2010:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_lun_private.h"
#include "fbe_lun_test.h"
#include "fbe/fbe_emcutil_shell_include.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/



/*!***************************************************************
 * main()
 ****************************************************************
 * @brief
 *  This function is the main routine for the test.  
 *  Test suite is created here and tests are added to the suite.
 *  Suite is then excuted.
 *
 * @param  - argc, argv.
 *
 * @return - None 
 *
 * @author
 *  01/06/2010 - Created. guov
 *
 ****************************************************************/
int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *lun_class_test_suite;          /* pointer to testsuite structure */

    #include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                           /* before proceeding we need to initialize MUT infrastructure */

    lun_class_test_suite = MUT_CREATE_TESTSUITE("lun_class_test_suite")  /* testsuite is created */
    MUT_ADD_TEST_WITH_DESCRIPTION(lun_class_test_suite, 
                                  calculate_import_capacity_test, 
                                  NULL,
                                  NULL,
                                  calculate_import_capacity_short_desc, 
                                  calculate_import_capacity_long_desc)

    MUT_RUN_TESTSUITE(lun_class_test_suite)
}
/**************************************************************
 * end main()
 **************************************************************/

/*******************************
 * end fbe_lun_test_main.c
 *******************************/
