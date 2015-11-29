/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_raid_library_unit_test_main.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the main function for the raid library unit tests.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
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
extern fbe_u32_t fbe_raid_get_iots_size(void);
extern fbe_u32_t fbe_raid_get_siots_size(void);
extern fbe_u32_t fbe_raid_get_fruts_size(void);
extern fbe_u32_t fbe_raid_get_common_size(void);
extern fbe_u32_t fbe_raid_get_packet_size(void);
extern fbe_u32_t fbe_raid_get_payload_ex_size(void);
extern void      fbe_raid_library_add_unit_tests(mut_testsuite_t * const suite_p);

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
 *  5/21/2009 - Created. rfoley
 *
 ****************************************************************/

int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *unit_suite_p;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    printf("fbe_raid_iots_t size: %d\n", fbe_raid_get_iots_size());
    printf("fbe_raid_siots_t size: %d\n", fbe_raid_get_siots_size());
    printf("fbe_raid_fruts_t size: %d\n", fbe_raid_get_fruts_size());
    printf("fbe_raid_common_t size: %d\n", fbe_raid_get_common_size());
    printf("fbe_packet_t size: %d\n", fbe_raid_get_packet_size());
    printf("fbe_payload_ex_t size: %d\n", fbe_raid_get_payload_ex_size());

    unit_suite_p = MUT_CREATE_TESTSUITE("fbe_raid_library_unit_test_suite");

    fbe_raid_library_add_unit_tests(unit_suite_p);
    MUT_RUN_TESTSUITE(unit_suite_p);

    exit(0);
}
/**************************************
 * end main()
 **************************************/

/*****************************************
 * end fbe_raid_library_unit_test_main.c
 *****************************************/
