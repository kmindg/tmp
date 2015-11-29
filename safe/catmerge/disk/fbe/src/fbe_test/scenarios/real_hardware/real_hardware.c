/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    real_hardware.c
 ***************************************************************************
 *
 * @brief   This file contains the MUT test suite definitions for the FBE tests
 *          to run on hardware. The tests are divided into suites similar to the
 *          fbe_test suites that run in simulation.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "hw_tests.h"
#include "sep_tests.h"

/* 
 *  The functions below create the various test suites for running FBE tests on hardware.
 *  Any new tests should be added to the appropriate suite.
 *  
 *  The suites are modeled after the test suites in fbe_test that runs in simulation. See sep_tests.c for details.
 *  
 *  @!TODO: Now that the FBE tests are being converted directly to run on hardware, the test suite creation functions
 *          can be defined in one place and used by both fbe_test and fbe_hw_test. With that, this file can go away with
 *          sep_tests.c used for all FBE test suite creation. In fact, the fbe_test_scenarios_real_hardware library, of
 *          which real_hardware.c is a part, can eventually go away.
 */

mut_testsuite_t * hw_fbe_test_create_sep_configuration_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_configuration_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, plankton_test, plankton_test_setup, plankton_test_cleanup,
                                  plankton_short_desc, plankton_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, patrick_star_test, patrick_star_test_setup, patrick_star_test_cleanup,
                                  patrick_star_short_desc, patrick_star_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, 
                                  bubble_bass_test,
                                  bubble_bass_test_setup,
                                  bubble_bass_test_cleanup, 
                                  bubble_bass_short_desc,
                                  bubble_bass_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ruby_doo_test, ruby_doo_setup, ruby_doo_test_cleanup, 
                                  ruby_doo_short_desc, ruby_doo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rabo_karabekian_test,
                                  rabo_karabekian_config_init, rabo_karabekian_config_destroy,
                                  rabo_karabekian_short_desc, rabo_karabekian_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, billy_pilgrim_test,
                                  billy_pilgrim_config_init, billy_pilgrim_config_destroy,
                                  billy_pilgrim_short_desc, billy_pilgrim_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, spongebob_test, spongebob_setup, spongebob_cleanup,
                                  spongebob_short_desc, spongebob_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sherlock_hemlock_test, sherlock_hemlock_setup, sherlock_hemlock_cleanup,
                                  sherlock_hemlock_short_desc, sherlock_hemlock_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_degraded_io_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, big_bird_single_degraded_test, big_bird_setup, big_bird_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, big_bird_single_degraded_zero_abort_test, big_bird_setup, big_bird_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, big_bird_shutdown_test, big_bird_setup, big_bird_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, oscar_degraded_test, oscar_setup, oscar_cleanup,
                                  oscar_short_desc, oscar_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, oscar_degraded_zero_abort_test, oscar_setup, oscar_cleanup,
                                  oscar_short_desc, oscar_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, oscar_shutdown_test, oscar_setup, oscar_cleanup,
                                  oscar_short_desc, oscar_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bert_single_degraded_test, bert_setup, bert_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bert_single_degraded_zero_abort_test, bert_setup, bert_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bert_shutdown_test, bert_setup, bert_cleanup,
                                  bert_short_desc, bert_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_single_degraded_test, ernie_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_single_degraded_zero_abort_test, ernie_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_double_degraded_test, ernie_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_double_degraded_zero_abort_test, ernie_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_shutdown_test, ernie_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_single_degraded_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_single_degraded_zero_abort_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_shutdown_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_disk_errors_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, parinacota_test, parinacota_setup, parinacota_cleanup,
                                  parinacota_short_desc, parinacota_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, abby_cadabby_raid1_test, abby_cadabby_raid1_setup, abby_cadabby_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, abby_cadabby_raid10_test, abby_cadabby_raid10_setup, abby_cadabby_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, abby_cadabby_raid5_test, abby_cadabby_raid5_setup, abby_cadabby_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, abby_cadabby_raid3_test, abby_cadabby_raid3_setup, abby_cadabby_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, abby_cadabby_raid6_test, abby_cadabby_raid6_setup, abby_cadabby_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid0_test, cookie_monster_raid0_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid1_test, cookie_monster_raid1_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid10_test, cookie_monster_raid10_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid5_test, cookie_monster_raid5_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid3_test, cookie_monster_raid3_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cookie_monster_raid6_test, cookie_monster_raid6_setup, cookie_monster_cleanup,
        cookie_monster_short_desc, cookie_monster_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid0_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid1_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid3_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid5_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid6_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kermit_raid10_test, kermit_setup, kermit_cleanup,
        kermit_short_desc, kermit_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lefty_raid0_test, lefty_setup, lefty_cleanup,
                                  lefty_short_desc, lefty_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lefty_raid1_test, lefty_setup, lefty_cleanup,
                                  lefty_short_desc, lefty_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lefty_raid5_test, lefty_setup, lefty_cleanup,
                                  lefty_short_desc, lefty_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lefty_raid3_test, lefty_setup, lefty_cleanup,
                                  lefty_short_desc, lefty_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lefty_raid6_test, lefty_setup, lefty_cleanup,
                                  lefty_short_desc, lefty_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mumford_the_magician_test, mumford_the_magician_setup, mumford_the_magician_cleanup,
                                  mumford_the_magician_test_short_desc, mumford_the_magician_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, guy_smiley_test, guy_smiley_setup, guy_smiley_cleanup,
                                  guy_smiley_test_short_desc,guy_smiley_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, turner_test, turner_setup, turner_cleanup,
                                  turner_test_short_desc, turner_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dusty_test, dusty_setup, dusty_cleanup,
                                  dusty_test_short_desc, dusty_test_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_normal_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_normal_io_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, elmo_test, elmo_setup, elmo_cleanup,
                                  elmo_short_desc, elmo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, zoe_test, zoe_setup, zoe_cleanup,
                                  zoe_short_desc, zoe_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, grover_test, grover_setup, grover_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, grover_block_opcode_test, grover_setup, grover_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, snuffy_test, snuffy_setup, snuffy_cleanup,
                                  snuffy_short_desc, snuffy_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, telly_test, telly_setup, telly_cleanup,
                                  telly_short_desc, telly_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, prairie_dawn_test, prairie_dawn_setup, prairie_dawn_cleanup,
                                  prairie_dawn_short_desc, prairie_dawn_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid0_test, squeeze_raid0_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid1_test, squeeze_raid1_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid10_test, squeeze_raid10_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid5_test, squeeze_raid5_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid3_test, squeeze_raid3_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squeeze_raid6_test, squeeze_raid6_setup, squeeze_cleanup,
        squeeze_short_desc, squeeze_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid0_test, stretch_raid0_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid1_test, stretch_raid1_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid10_test, stretch_raid10_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid5_test, stretch_raid5_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid3_test, stretch_raid3_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, stretch_raid6_test, stretch_raid6_setup, stretch_cleanup,
        stretch_short_desc, stretch_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, baby_bear_test, baby_bear_setup, baby_bear_cleanup,
                                  baby_bear_short_desc, baby_bear_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, eye_of_vecna_test, eye_of_vecna_setup, eye_of_vecna_cleanup,
                                  eye_of_vecna_short_desc, eye_of_vecna_long_desc)
	
    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_power_savings_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_power_savings_test_suite",startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, amber_test, amber_setup, amber_cleanup,
                                  amber_short_desc, amber_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, pooh_test, pooh_setup, pooh_cleanup, 
                                  pooh_short_desc, pooh_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dr_jekyll_test, 
            dr_jekyll_config_init, dr_jekyll_config_destroy, 
            dr_jekyll_short_desc, dr_jekyll_long_desc);  
	MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, gaggy_test, gaggy_test_init, gaggy_test_destroy,
                                gaggy_short_desc, gaggy_long_desc);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_rebuild_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_rebuild_test_suite",startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, diego_test, diego_setup, diego_cleanup, 
                                  diego_short_desc, diego_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, boots_test, boots_setup, boots_cleanup, 
                                  boots_short_desc, boots_long_desc)
	MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kishkashta_test, kishkashta_test_init, kishkashta_test_destroy,
                                  kishkashta_short_desc, kishkashta_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dora_test, dora_setup, dora_cleanup,
                                  dora_short_desc, dora_long_desc)
    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_services_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_services_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, batman_test, batman_setup, batman_destroy,
                                  batman_short_desc, batman_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, betty_lou_test, betty_lou_setup, betty_lou_cleanup,
                                  betty_lou_short_desc, betty_lou_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ovejita_test, ovejita_setup, ovejita_cleanup,
                                  ovejita_short_desc, ovejita_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_sniff_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_sniff_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, skippy_doo_test, skippy_doo_setup, skippy_doo_cleanup,
                                  skippy_doo_short_desc, skippy_doo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, howdy_doo_test, howdy_doo_setup, howdy_doo_cleanup,
                                  howdy_doo_short_desc, howdy_doo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dixie_doo_test, dixie_doo_setup, dixie_doo_cleanup,
                                  dixie_doo_short_desc, dixie_doo_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_sparing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_sparing_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kilgore_trout_test,
                                  kilgore_trout_setup, kilgore_trout_cleanup,
                                  kilgore_trout_short_desc, kilgore_trout_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, shaggy_test, shaggy_setup, shaggy_cleanup,
                                  shaggy_short_desc, shaggy_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scoobert_test, scoobert_setup, scoobert_cleanup,
                                  scoobert_short_desc, scoobert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scoobert_4k_test, scoobert_4k_setup, scoobert_4k_cleanup,
                                  scoobert_short_desc, scoobert_long_desc)
    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_special_ops_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_special_ops_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, homer_test, homer_setup, homer_cleanup,
                                  homer_short_desc, homer_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_verify_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_verify_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cleo_test, cleo_setup, cleo_cleanup,
                                  cleo_short_desc, cleo_long_desc)
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, hamburger_test, hamburger_setup, hamburger_cleanup,
    //                              hamburger_short_desc, hamburger_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_zeroing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_zeroing_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squidward_test, squidward_setup, squidward_cleanup,
                                  squidward_short_desc, squidward_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_performance_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_performance_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, king_minos_test, king_minos_setup, king_minos_cleanup,
                                  king_minos_short_desc, king_minos_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, king_aeacus_test, king_minos_setup, king_minos_cleanup,
                                  king_minos_short_desc, king_minos_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, king_rhadamanthus_test, king_minos_setup, king_minos_cleanup,
                                  king_minos_short_desc, king_minos_long_desc)


    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_configuration_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_configuration_test_suite",startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, eliot_rosewater_dualsp_test, eliot_rosewater_dualsp_setup, eliot_rosewater_dualsp_cleanup,
                                    big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, woozle_test, woozle_setup, woozle_destroy, 
                                  woozle_short_desc, woozle_long_desc)                                
    return sep_dualsp_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_degraded_io_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_single_degraded_test,
                                   big_bird_dualsp_setup, big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_single_degraded_zero_abort_test, 
                                  big_bird_dualsp_setup, 
                                  big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_shutdown_test, 
                                  big_bird_dualsp_setup, big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bert_dualsp_single_degraded_test, bert_dualsp_setup, bert_dualsp_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bert_dualsp_single_degraded_zero_abort_test, bert_dualsp_setup, bert_dualsp_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bert_dualsp_shutdown_test, bert_dualsp_setup, bert_dualsp_cleanup,
                                  bert_short_desc, bert_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_single_degraded_test, ernie_dualsp_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_single_degraded_zero_abort_test, ernie_dualsp_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_double_degraded_test, ernie_dualsp_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_double_degraded_zero_abort_test, ernie_dualsp_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_shutdown_test, ernie_dualsp_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)

    return sep_dualsp_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_disk_errors_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_normal_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_normal_io_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, elmo_dualsp_test, elmo_dualsp_setup, elmo_dualsp_cleanup,
                                  elmo_short_desc, elmo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, grover_dualsp_test, grover_dualsp_setup, grover_dualsp_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, telly_dualsp_test, telly_dualsp_setup, telly_dualsp_cleanup,
                                  telly_short_desc, telly_long_desc)                              
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, zoe_dualsp_test, zoe_dualsp_setup, zoe_dualsp_cleanup,
                                  zoe_short_desc, zoe_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, prairie_dawn_dualsp_test, prairie_dawn_dualsp_setup, prairie_dawn_dualsp_cleanup,
                                  prairie_dawn_short_desc, prairie_dawn_long_desc)
                              
    return sep_dualsp_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_power_savings_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_power_savings_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_rebuild_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_rebuild_test_suite",startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, dora_dualsp_test, dora_dualsp_setup, dora_dualsp_cleanup,
                                  dora_short_desc, dora_long_desc)

    return sep_dualsp_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_services_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_services_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_sniff_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_sniff_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_sparing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_sparing_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_special_ops_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_special_ops_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_verify_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_verify_test_suite",startup, teardown);

    return sep_test_suite;
}

mut_testsuite_t * hw_fbe_test_create_sep_dualsp_zeroing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("sep_dualsp_zeroing_test_suite",startup, teardown);

    return sep_test_suite;
}

