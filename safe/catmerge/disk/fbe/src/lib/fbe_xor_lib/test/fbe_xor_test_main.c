/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_library_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main function for the logical drive unit tests.
 *
 * @revision
 *   08/10/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe_xor_test_prototypes.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 ** Imported functions.
 ******************************/
void fbe_xor_test_add_tests(mut_testsuite_t * const suite_p);
void fbe_xor_test_checksum_add_tests(mut_testsuite_t * const suite_p);
void fbe_xor_test_parity_add_tests(mut_testsuite_t * const suite_p);
void fbe_xor_test_move_add_tests(mut_testsuite_t * const suite_p);
void fbe_xor_test_stamps_add_tests(mut_testsuite_t * const suite_p);

/*! @note Due to the fact the fact that fbe_get_package_id is required 
 *        for the base services etc and including fbe_sep.lib is not
 *        possible, the test library spoofs fbe_get_package_id().
 *
 *  @note Currently the xor library only `lives' in the SEP package  
 */
#include "fbe/fbe_package.h"
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_SEP_0;
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 * @def     cmd_options[]
 ***************************************************************************** 
 * 
 * @brief   Command line options for `fbe_test' simulation
 *
 *****************************************************************************/
typedef struct fbe_test_cmd_options_s 
{
    const char * name;
    int least_num_of_args;
    BOOL show;
    const char * description;
} fbe_test_cmd_options_t;

static fbe_test_cmd_options_t cmd_options[] = {
    {"-iterations",  1, TRUE, " "},
    {"-block_count",  1, TRUE, " "},
    {"-repeat_count", 1, TRUE, " "},
    {"-data_drives", 1, TRUE, " "},
};
/*!***************************************************************
 * fbe_xor_test_add_unit_tests()
 ****************************************************************
 * @brief
 *  This function adds the I/O unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  05/26/2009  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_xor_test_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);
    fbe_xor_test_move_add_tests(suite_p);
    fbe_xor_test_checksum_add_tests(suite_p);
    fbe_xor_test_parity_add_tests(suite_p);
    fbe_xor_test_stamps_add_tests(suite_p);
    return;
}
/* end fbe_xor_test_add_unit_tests() */

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
 *  8/10/2009 - Created. rfoley
 *
 ****************************************************************/

int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *suite_p;
    fbe_u32_t i;

#include "fbe/fbe_emcutil_shell_maincode.h"

    /* register the pre-defined user options */
    for (i = 0; i < sizeof(cmd_options) / sizeof(cmd_options[0]); ++i)
    {
        /* must be called before mut_init() */
        mut_register_user_option(cmd_options[i].name, cmd_options[i].least_num_of_args, cmd_options[i].show, cmd_options[i].description);
    }

    mut_init(argc, argv);

    suite_p = MUT_CREATE_TESTSUITE("fbe_xor_unit_test_suite");

    fbe_xor_test_add_unit_tests(suite_p);
    MUT_RUN_TESTSUITE(suite_p);

    exit(0);
}
/**************************************
 * end main()
 **************************************/

/*****************************************
 * end fbe_xor_library_test_main.c
 *****************************************/
