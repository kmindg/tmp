/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_unit_test_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main function for the raid mirror unit tests.
 *
 * @version
 *   9/11/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_emcutil_shell_include.h"

/******************************
 ** Imported functions.
 ******************************/
extern fbe_u32_t fbe_raid_get_iots_size(void);
extern fbe_u32_t fbe_raid_get_siots_size(void);
extern fbe_u32_t fbe_raid_get_fruts_size(void);
extern fbe_u32_t fbe_raid_get_common_size(void);
extern fbe_u32_t fbe_raid_get_packet_size(void);
extern fbe_u32_t fbe_raid_get_payload_ex_size(void);
extern fbe_u32_t fbe_raid_get_vrts_size(void);
extern fbe_u32_t fbe_raid_get_vcts_size(void);
extern void      fbe_raid_group_add_io_tests(mut_testsuite_t * const suite_p);
extern void      fbe_mirror_add_unit_tests(mut_testsuite_t * const suite_p);

/*!**************************************************************
 * main()
 ****************************************************************
 * @brief
 *  This is the main entry point of the raid mirror unit test.
 *
 * @param argc - argument count
 * @argv  argv - argument array
 *
 * @return int 0   
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/

int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *unit_suite_p;
    //mut_testsuite_t *io_suite_p;
    
#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);
#if 0
    printf("fbe_raid_iots_t size: %d\n", fbe_raid_get_iots_size());
    printf("fbe_raid_siots_t size: %d\n", fbe_raid_get_siots_size());
    printf("fbe_raid_fruts_t size: %d\n", fbe_raid_get_fruts_size());
    printf("fbe_raid_common_t size: %d\n", fbe_raid_get_common_size());
    printf("fbe_raid_vcts_t size: %d\n", fbe_raid_get_vcts_size());
    printf("fbe_raid_vrts_t size: %d\n", fbe_raid_get_vrts_size());

    printf("fbe_packet_t size: %d\n", fbe_raid_get_packet_size());
    printf("fbe_payload_ex_t size: %d\n", fbe_raid_get_payload_ex_size());
    printf("xor_command_size: %d\n", sizeof(fbe_xor_command_t));
#endif
    
    /* I/O tests are now executed on the fbe_test environment.
     * The simple mirror Read and Write tests are called `Zoe', etc.
     */
    //io_suite_p = MUT_CREATE_TESTSUITE("fbe_mirror_io_tests_suite");
    //fbe_mirror_add_io_tests(io_suite_p);
    //MUT_RUN_TESTSUITE(io_suite_p);
    
    /* raid mirror unit tests.
     */
    unit_suite_p = MUT_CREATE_TESTSUITE("fbe_mirror_unit_test_suite");
    fbe_mirror_add_unit_tests(unit_suite_p);
    MUT_RUN_TESTSUITE(unit_suite_p);

    exit(0);
}
/**************************************
 * end main()
 **************************************/

/*****************************************
 * end fbe_mirror_unit_test_main.c
 *****************************************/
