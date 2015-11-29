#include "sep_tests.h"
#include "mut.h"
#include "fbe_test_common_utils.h"

#define SEP_SUITE_NAME_LENGTH 75

#define SEP_SUITE_NAME(m_name, m_buffer)(_snprintf(m_buffer, SEP_SUITE_NAME_LENGTH, "sep_%s_test_suite", m_name))
#define SEP_EXTENDED_SUITE_NAME(m_name, m_buffer)(_snprintf(m_buffer, SEP_SUITE_NAME_LENGTH, "extended_test_sep_%s_test_suite", m_name))

#define SEP_DUAL_SP_SUITE_NAME(m_name, m_buffer)(_snprintf(m_buffer, SEP_SUITE_NAME_LENGTH, "sep_dualsp_%s_test_suite", m_name))
#define SEP_EXTENDED_DUAL_SP_SUITE_NAME(m_name, m_buffer)(_snprintf(m_buffer, SEP_SUITE_NAME_LENGTH, "extended_test_sep_dualsp_%s_test_suite", m_name))

mut_testsuite_t * fbe_test_create_sep_configuration_test_suite(mut_function_t startup, mut_function_t teardown) 
{ 
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("configuration", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("configuration", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, pinga_drive_validation_test, pinga_drive_validation_setup, pinga_drive_validation_cleanup,
                                  pinga_drive_validation_short_desc, pinga_drive_validation_long_desc)   

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, plankton_test, plankton_test_setup, plankton_test_cleanup,
        plankton_short_desc, plankton_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, patrick_star_test, patrick_star_test_setup, patrick_star_test_cleanup,
        patrick_star_short_desc, patrick_star_long_desc)
    /* Database service tests */
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, beibei_test, beibei_setup, beibei_cleanup,
                                  beibei_test_short_desc, beibei_test_long_desc);

    /*! @todo Due to the fact that this test issues copy operations but the 
     *        rules have changed:
     *          o Cannot start a copy operation unless there is a LU bound
     *          o Cannot start a copy operation on a degraded raid group
     *        Therefore this test is currently disabled.
     * Tests the PVD Pool-id cases 
     */ 
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dumku_test, dumku_setup, dumku_destroy,
                                  dumku_short_desc, dumku_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, hulk_test, hulk_setup, hulk_destroy,
                                  hulk_short_desc, hulk_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, shrek_test, shrek_test_setup, shrek_test_cleanup, 
                                  shrek_short_desc, shrek_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, transformers_test, transformers_test_setup, transformers_test_cleanup, 
                                  transformers_short_desc, transformers_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, 
                                  larry_t_lobster_test,
                                  larry_t_lobster_test_setup,
                                  larry_t_lobster_test_cleanup, 
                                  larry_t_lobster_short_desc,
                                  larry_t_lobster_long_desc)

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

    /* this is moved to dualsp suite. Reason: This doesn't have a single SP version to verify. Need to create one. */
//  MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, eliot_rosewater_test,
//          eliot_rosewater_setup, eliot_rosewater_destroy,
//          eliot_rosewater_short_desc, eliot_rosewater_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite,spongebob_test, spongebob_setup, spongebob_cleanup,
                                  spongebob_short_desc, spongebob_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sherlock_hemlock_test, sherlock_hemlock_setup, sherlock_hemlock_cleanup,
        sherlock_hemlock_short_desc, sherlock_hemlock_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, diesel_test, diesel_setup, diesel_cleanup,
                                  diesel_short_desc, diesel_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, smurfs_test, smurfs_test_setup, smurfs_test_cleanup, 
                                  smurfs_short_desc, smurfs_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, chromatic_phase1_test, chromatic_setup, chromatic_cleanup, 
                                  chromatic_test_short_desc, chromatic_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, chromatic_phase2_test, chromatic_setup, chromatic_cleanup, 
                                      chromatic_test_short_desc, chromatic_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, snake_ica_test, snake_ica_setup, snake_ica_test_cleanup, 
                                      snake_ica_test_short_desc, snake_ica_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, peep_test, peep_setup, peep_cleanup, 
                                  peep_short_desc, peep_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kungfu_panda_test, kungfu_panda_setup, kungfu_panda_cleanup, 
                                  kungfu_panda_short_desc, kungfu_panda_long_desc)

    // Test became obsolete after adding logic for missing system RG/LUN recreation on DB booting path,
    // So commenting out, by Jian @ March 16th, 2013
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ezio_test, ezio_setup, ezio_cleanup, 
    //                              ezio_test_short_desc, ezio_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, arwen_test, arwen_setup, arwen_cleanup, 
                                  arwen_short_desc, arwen_long_desc)


    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, amon_test, amon_setup, amon_cleanup, 
                                  amon_short_desc, amon_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, oh_smiling_test, oh_smiling_setup, oh_smiling_cleanup,
                                  oh_smiling_short_desc, oh_smiling_long_desc)                              
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_riding_hood_test, red_riding_hood_setup, red_riding_hood_cleanup, 
                                  red_riding_hood_short_desc, red_riding_hood_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, gravity_falls_test, gravity_falls_setup, gravity_falls_cleanup, 
                                  gravity_falls_short_desc, gravity_falls_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, thomas_test, thomas_setup, thomas_cleanup,
                                  thomas_short_desc, thomas_long_desc)

    // Test became obsolete.. so commenting out as discussed with Zhipeng.
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, phoenix_test, phoenix_setup, phoenix_cleanup, 
    //                              phoenix_short_desc, phoenix_long_desc)
    // 

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, jigglypuff_drive_validation_test, jigglypuff_drive_validation_setup, jigglypuff_drive_validation_cleanup,
                                  jigglypuff_drive_validation_short_desc, jigglypuff_drive_validation_long_desc)   

    return sep_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("degraded_io", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("degraded_io", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

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
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_simultaneous_insert_and_remove_test, ernie_setup_520, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_single_degraded_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_single_degraded_zero_abort_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_shutdown_test, rosita_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, chitti_test, chitti_setup, chitti_cleanup, 
                                  chitti_short_desc, chitti_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, lurker_singlesp_test, lurker_singlesp_setup, lurker_singlesp_cleanup,
                                  lurker_short_desc, lurker_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, frank_the_pug_test, frank_the_pug_setup, frank_the_pug_cleanup,
                                  frank_the_pug_short_desc, frank_the_pug_long_desc)

    return sep_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("disk_errors", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("disk_errors", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, parinacota_test, parinacota_setup, parinacota_cleanup,
                                  parinacota_short_desc, parinacota_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, percy_test, percy_setup, percy_destroy, 
                                  percy_short_desc, percy_long_desc);

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
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, strider_test, strider_setup, strider_cleanup,
                                  strider_short_desc, strider_long_desc)

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

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, pippin_test, pippin_setup, pippin_cleanup,
                                  pippin_short_desc, pippin_long_desc)
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
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bob_the_builder_test_520, bob_the_builder_setup_520, bob_the_builder_cleanup,
                                  bob_the_builder_short_desc, bob_the_builder_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bob_the_builder_test_4k, bob_the_builder_setup_4k, bob_the_builder_cleanup,
                                  bob_the_builder_short_desc, bob_the_builder_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_jay_test_r5, agent_jay_setup_r5, agent_jay_cleanup,
                                  agent_jay_short_desc, agent_jay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_jay_test_r3, agent_jay_setup_r3, agent_jay_cleanup,
                                  agent_jay_short_desc, agent_jay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_jay_test_r6_single_degraded, agent_jay_setup_r6, agent_jay_cleanup,
                                  agent_jay_short_desc, agent_jay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_jay_test_r6_double_degraded, agent_jay_setup_r6, agent_jay_cleanup,
                                  agent_jay_short_desc, agent_jay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_kay_test_r5, agent_kay_setup_r5, agent_kay_cleanup,
                                  agent_kay_short_desc, agent_kay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_kay_test_r3, agent_kay_setup_r3, agent_kay_cleanup,
                                  agent_kay_short_desc, agent_kay_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, agent_kay_test_r6, agent_kay_setup_r6, agent_kay_cleanup,
                                  agent_kay_short_desc, agent_kay_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, baymax_test, baymax_setup, baymax_cleanup,
                                  baymax_short_desc,baymax_long_desc)

    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_normal_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("normal_io", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("normal_io", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, super_hv_test,
                                  super_hv_setup, fbe_test_physical_package_tests_config_unload,
                                  super_hv_short_desc, super_hv_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, elmo_test, elmo_setup, elmo_cleanup,
                                  elmo_short_desc, elmo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, zoe_test, zoe_setup, zoe_cleanup,
                                  zoe_short_desc, zoe_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, grover_test, grover_setup, grover_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, snuffy_test, snuffy_setup, snuffy_cleanup,
                                  snuffy_short_desc, snuffy_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rick_test, rick_setup, rick_cleanup,
                                  rick_short_desc, rick_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, telly_test, telly_setup, telly_cleanup,
                                  telly_short_desc, telly_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, prairie_dawn_test, prairie_dawn_setup, prairie_dawn_cleanup,
                                  prairie_dawn_short_desc, prairie_dawn_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, elmo_block_opcode_test, elmo_setup, elmo_cleanup,
                                  elmo_short_desc, elmo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, zoe_block_opcode_test, zoe_setup, zoe_cleanup,
                                  zoe_short_desc, zoe_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, grover_block_opcode_test, grover_setup, grover_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, snuffy_block_opcode_test, snuffy_setup, snuffy_cleanup,
                                  snuffy_short_desc, snuffy_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, telly_block_opcode_test, telly_setup, telly_cleanup,
                                  telly_short_desc, telly_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, prairie_dawn_block_opcode_test, prairie_dawn_setup, prairie_dawn_cleanup,
                                  prairie_dawn_short_desc, prairie_dawn_long_desc)
    /*! @todo Need to update database to use 4K aligned requests. 
     */
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ultra_hd_test, ultra_hd_setup, ultra_hd_cleanup,
    //                              ultra_hd_short_desc, ultra_hd_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, meriadoc_brandybuck_test, meriadoc_brandybuck_setup, meriadoc_brandybuck_cleanup,
                                  meriadoc_brandybuck_short_desc, meriadoc_brandybuck_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, handy_manny_test, handy_manny_setup, handy_manny_cleanup,
        handy_manny_short_desc, handy_manny_long_desc)
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
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, herry_monster_test, herry_monster_setup, herry_monster_cleanup,
                                  herry_monster_test_short_desc, herry_monster_test_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, launchpad_mcquack_test, launchpad_mcquack_setup, launchpad_mcquack_cleanup,
                                  launchpad_mcquack_short_desc, launchpad_mcquack_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, eye_of_vecna_test, eye_of_vecna_setup, eye_of_vecna_cleanup,
                                  eye_of_vecna_short_desc, eye_of_vecna_long_desc);

    /* This can not be run with python, because it consumes a lot of memory */
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, performance_test, performance_test_init, performance_test_destroy,
                              performance_test_short_desc, performance_test_long_desc);
    
    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_power_savings_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("power_savings", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("power_savings", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, amber_test, amber_setup, fbe_test_sep_util_destroy_neit_sep_physical,
                                  amber_short_desc, amber_long_desc);

    /*! @todo Temporarily disable weerd_test which reproduce the 
     *        copyto/power saving issue. 
     */
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, weerd_test, weerd_setup, weerd_cleanup,
                                  weerd_short_desc, weerd_long_desc);

    // This test is doing NOTHING
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, aurora_test, aurora_test_init, aurora_test_destroy, aurora_short_desc, aurora_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, boomer_test, boomer_test_setup, boomer_test_cleanup,
                                  boomer_short_desc, boomer_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, boomer_4_new_drive_test, boomer_4_new_drive_test_setup, boomer_4_new_drive_test_cleanup,
                                  boomer_4_new_drive_short_desc, boomer_4_new_drive_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dr_jekyll_test, 
            dr_jekyll_config_init, dr_jekyll_config_destroy, 
            dr_jekyll_short_desc, dr_jekyll_long_desc); 
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, pooh_test, pooh_setup, pooh_cleanup, 
                                  pooh_short_desc, pooh_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rabbit_test, NULL, NULL, 
                                  rabbit_short_desc, rabbit_long_desc)  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, eeyore_test, NULL, NULL, 
                                  eeyore_short_desc, eeyore_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, whoopsy_doo_test, whoopsy_doo_setup, whoopsy_doo_cleanup,
                                  whoopsy_doo_short_desc, whoopsy_doo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, yeti_test, yeti_setup, yeti_cleanup,
                                  yeti_short_desc, yeti_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, gaggy_test, gaggy_test_init, gaggy_test_destroy,
                                gaggy_short_desc, gaggy_long_desc);

    // Not implemented yet
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bogel_test, bogel_test_init, bogel_test_destroy,
    //                            bogel_short_desc, bogel_long_desc);
    return sep_test_suite;

}
mut_testsuite_t * fbe_test_create_sep_rebuild_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("rebuild", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("rebuild", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, diego_test, diego_setup, diego_cleanup, 
                                  diego_short_desc, diego_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, flexo_test, flexo_test_setup, flexo_test_cleanup,
                                  flexo_test_short_desc, flexo_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, swiper_test, NULL, NULL, 
                                  swiper_short_desc, swiper_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, boots_test, boots_setup, boots_cleanup, 
                                  boots_short_desc, boots_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kishkashta_test, kishkashta_test_init, kishkashta_test_destroy,
                                  kishkashta_short_desc, kishkashta_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, doodle_test, doodle_setup, doodle_cleanup, 
                                  doodle_short_desc, doodle_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dora_test, dora_setup, dora_cleanup,
                                  dora_short_desc, dora_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, banio_test, banio_setup, banio_cleanup,
                                  banio_short_desc, banio_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, momo_test, momo_setup, momo_cleanup,
                                  momo_short_desc, momo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, samwise_gamgee_test, samwise_gamgee_setup, samwise_gamgee_cleanup,
                                  samwise_gamgee_short_desc, samwise_gamgee_long_desc)
    return sep_test_suite;

}
mut_testsuite_t * fbe_test_create_sep_services_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("services", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("services", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, batman_test, batman_setup, batman_destroy,
                                  batman_short_desc, batman_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, betty_lou_test, betty_lou_setup, betty_lou_cleanup,
                                  betty_lou_short_desc, betty_lou_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ovejita_singlesp_test, ovejita_setup, ovejita_cleanup,
                                  ovejita_short_desc, ovejita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, harry_potter_test, harry_potter_setup, harry_potter_cleanup,
                                  harry_potter_short_desc, harry_potter_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, hermione_test, hermione_setup, hermione_cleanup,
                                  hermione_short_desc, hermione_long_desc)

/*!@todo DE730 comment out sailor_moon test */
//    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sailor_moon_test, sailor_moon_setup, sailor_moon_cleanup,
//                                  sailor_moon_short_desc, sailor_moon_long_desc)

    // this test fails because it's interaction with the DB serice.  Since DB service now uses the persist service, this test is less critical.
  //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, phineas_and_ferb_test, phineas_and_ferb_test_init, phineas_and_ferb_test_destroy,
                              //phineas_and_ferb_short_desc, phineas_and_ferb_long_desc);

    //ARS-384563: MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scrappydoo_test, scrappydoo_setup, scrappydoo_cleanup,
    //                              scrappydoo_short_desc, scrappydoo_long_desc) 
    #if 0/*CMS DISBALED*/
     MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ponyo_test, ponyo_setup, ponyo_destroy,
                                  ponyo_short_desc, ponyo_long_desc)

     MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, salvador_dali_test, salvador_dali_setup, salvador_dali_cleanup,
                                  salvador_dali_short_desc, salvador_dali_long_desc)
    #endif

    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_sniff_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("sniff", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("sniff", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, howdy_doo_test, howdy_doo_setup, howdy_doo_cleanup,
        howdy_doo_short_desc, howdy_doo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, daphne_test, daphne_setup, daphne_cleanup,
        daphne_short_desc, daphne_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dixie_doo_test, dixie_doo_setup, dixie_doo_cleanup,
        dixie_doo_short_desc, dixie_doo_long_desc)


    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scoop_test, scoop_test_init, scoop_test_destroy,
                                  scoop_short_desc, scoop_long_desc);
    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_sparing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("sparing", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("sparing", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kilgore_trout_test, kilgore_trout_setup, kilgore_trout_cleanup,
                                  kilgore_trout_short_desc, kilgore_trout_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, shaggy_test, shaggy_setup, shaggy_cleanup,
                                  shaggy_short_desc, shaggy_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scoobert_test, scoobert_setup, scoobert_cleanup,
                                  scoobert_short_desc, scoobert_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scoobert_4k_test, scoobert_4k_setup, scoobert_4k_cleanup,
                                  scoobert_short_desc, scoobert_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, freddie_test,
                                  freddie_setup, freddie_cleanup, freddie_short_desc, freddie_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, velma_test, 
                                  velma_setup, velma_cleanup, velma_short_desc, velma_long_desc)

    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_copy_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("copy", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("copy", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, diabolicaldiscdemon_test, diabolicaldiscdemon_setup, diabolicaldiscdemon_cleanup,
                                  diabolicaldiscdemon_short_desc, diabolicaldiscdemon_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, robi_test, robi_setup, robi_cleanup,
                                  robi_short_desc, robi_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, dooby_doo_test, dooby_doo_setup, dooby_doo_cleanup,
                                  dooby_doo_short_desc, dooby_doo_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scooby_doo_test, scooby_doo_setup, scooby_doo_cleanup,
                                  scooby_doo_short_desc, scooby_doo_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_herring_raid1_test, red_herring_raid1_setup, red_herring_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_herring_raid10_test, red_herring_raid10_setup, red_herring_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_herring_raid3_test, red_herring_raid3_setup, red_herring_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_herring_raid5_test, red_herring_raid5_setup, red_herring_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, red_herring_raid6_test, red_herring_raid6_setup, red_herring_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, vincent_van_ghoul_test, vincent_van_ghoul_setup, vincent_van_ghoul_cleanup,
                                  vincent_van_ghoul_short_desc, vincent_van_ghoul_long_desc)

    /*! @note The following tests are not functional. 
     */

    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, flim_flam_test, flim_flam_setup, flim_flam_cleanup,
    //                              flim_flam_short_desc, flim_flam_long_desc)


    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_special_ops_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("special_ops", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("special_ops", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sir_topham_hatt_test, sir_topham_hatt_setup, sir_topham_hatt_destroy,
                                  sir_topham_hatt_short_desc, sir_topham_hatt_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, biff_test, biff_setup, biff_cleanup,
        biff_short_desc, biff_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, homer_test, homer_setup, homer_cleanup,
        homer_short_desc, homer_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sully_test, sully_setup, sully_cleanup,
        sully_short_desc, sully_long_desc)

/*!@todo DE729 comment out doreamon test */
#if 0    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, doraemon_user_test, doraemon_setup, doraemon_cleanup,
        doraemon_short_desc, doraemon_long_desc)    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, doraemon_system_test, doraemon_setup, doraemon_cleanup,
        doraemon_short_desc, doraemon_long_desc)    
#endif
        
    return sep_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_encryption_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("encryption", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("encryption", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_drive_removal_test, ichabod_drive_removal_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_drive_removal_paged_corrupt_test, ichabod_drive_removal_setup, 
                                  ichabod_cleanup_with_errs, ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_shutdown_test, ichabod_shutdown_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_read_only_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_check_pattern_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_degraded_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_zero_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ichabod_crane_unconsumed_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)


    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mike_the_knight_normal_scrubbing_test, mike_the_knight_setup, mike_the_knight_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mike_the_knight_scrubbing_consumed_test, mike_the_knight_setup, mike_the_knight_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mike_the_knight_destroy_rg_before_encryption, mike_the_knight_setup, mike_the_knight_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mike_the_knight_normal_scrubbing_drive_pull_test, mike_the_knight_setup, mike_the_knight_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, mike_the_knight_scrubbing_garbage_collection_test, mike_the_knight_setup, mike_the_knight_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid0_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid1_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid3_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid5_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid6_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_raid10_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, headless_horseman_vault_test, headless_horseman_setup, headless_horseman_cleanup,
        headless_horseman_short_desc, headless_horseman_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sleepy_hollow_test, sleepy_hollow_setup, sleepy_hollow_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sleepy_hollow_vault_in_use_test, sleepy_hollow_setup, sleepy_hollow_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sleepy_hollow_vault_delay_encryption_test, sleepy_hollow_setup, sleepy_hollow_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sleepy_hollow_vault_zero_test, sleepy_hollow_setup_sim_psl, sleepy_hollow_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, katrina_test, katrina_setup, katrina_cleanup,
                                  katrina_short_desc, katrina_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, sesame_open_test, sesame_open_setup, sesame_open_cleanup,
                                  sesame_open_short_desc, sesame_open_long_desc)
 #endif
                           
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, diabolicaldiscdemon_encryption_test, diabolicaldiscdemon_encrypted_setup, diabolicaldiscdemon_encrypted_cleanup,
                                  diabolicaldiscdemon_short_desc, diabolicaldiscdemon_long_desc)
    /*! @todo Seeing a failure where the database gets out of sync with kms after a reboot. */
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, vincent_van_ghoul_encrypted_test, vincent_van_ghoul_encrypted_setup, vincent_van_ghoul_encrypted_cleanup,
    //                              vincent_van_ghoul_short_desc, vincent_van_ghoul_long_desc)
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, big_bird_single_degraded_encryption_test, big_bird_encryption_setup, big_bird_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, daffy_duck_test, daffy_duck_setup, daffy_duck_cleanup,
                                 daffy_duck_short_desc,daffy_duck_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bert_single_degraded_encryption_test, bert_encryption_setup, bert_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_single_degraded_encryption_test, ernie_encryption_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ernie_double_degraded_encryption_test, ernie_encryption_setup, ernie_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, rosita_single_degraded_encryption_test, rosita_encryption_setup, rosita_cleanup,
                                  rosita_short_desc, rosita_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_test, evie_setup, evie_cleanup,
                                  evie_short_desc, evie_long_desc)
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_vault_encryption_in_progress_test, evie_setup, evie_cleanup,
                                  evie_short_desc, evie_long_desc)
#endif
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_drive_relocation_test, evie_setup, evie_cleanup,
                                  evie_short_desc, evie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_encryption_mode_test, evie_setup, evie_cleanup,
                                  evie_short_desc, evie_long_desc)
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_bind_unbind_test, evie_setup, evie_cleanup,
                                  evie_short_desc, evie_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, kalia_test, kalia_setup, kalia_cleanup, 
                                  kalia_short_desc, kalia_long_desc)

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, nearly_headless_nick_test, nearly_headless_nick_setup, 
                                  nearly_headless_nick_cleanup,
                                  nearly_headless_nick_short_desc, nearly_headless_nick_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, clifford_crc_error_encrypted_test, clifford_encryption_setup, clifford_cleanup,
                                  clifford_crc_error_test_short_desc, clifford_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bob_the_builder_encrypted_test, bob_the_builder_encrypted_setup, bob_the_builder_encrypted_cleanup,
                                  bob_the_builder_short_desc, bob_the_builder_long_desc)

    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_verify_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("verify", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("verify", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, clifford_test, clifford_setup, clifford_cleanup,
                                  clifford_short_desc, clifford_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, clifford_crc_error_test, clifford_setup, clifford_cleanup,
                                  clifford_crc_error_test_short_desc, clifford_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, cleo_test, cleo_setup, cleo_cleanup,
                                  cleo_short_desc, cleo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, hamburger_test, hamburger_setup, hamburger_cleanup,
                                  hamburger_short_desc, hamburger_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, bilbo_baggins_test, bilbo_baggins_setup, bilbo_baggins_cleanup,
                                  bilbo_baggins_short_desc, bilbo_baggins_long_desc)
    return sep_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_zeroing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("zeroing", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("zeroing", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scooby_dum_test, scooby_dum_setup,  scooby_dum_cleanup, 
                                  scooby_dum_short_desc, scooby_dum_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, foghorn_leghorn_test, foghorn_leghorn_setup,  foghorn_leghorn_cleanup,
                                  foghorn_leghorn_short_desc, foghorn_leghorn_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, squidward_test, squidward_setup, squidward_cleanup,
                                  squidward_short_desc, squidward_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, yabbadoo_test, yabbadoo_setup, yabbadoo_cleanup,
                                  yabbadoo_short_desc, yabbadoo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, jingle_bell_test, jingle_bell_setup, jingle_bell_cleanup,
                                  jingle_bell_short_desc, jingle_bell_long_desc)                              
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, ugly_duckling_test, ugly_duckling_setup, ugly_duckling_cleanup,
                                  ugly_duckling_short_desc, ugly_duckling_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, doremi_test, doremi_setup, doremi_cleanup,
                                  doremi_short_desc, doremi_long_desc)                                 
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, evie_quiesce_test, evie_quiesce_test_setup, evie_quiesce_test_cleanup,
                                  evie_short_desc, evie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, hiro_hamada_test, hiro_hamada_setup, hiro_hamada_cleanup,
                                  hiro_hamada_short_desc, hiro_hamada_long_desc)
    return sep_test_suite; 
}

mut_testsuite_t * fbe_test_create_sep_performance_test_suite(mut_function_t startup, mut_function_t teardown){
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("performance", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("performance", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, king_minos_test, king_minos_setup, king_minos_cleanup,
                                  king_minos_short_desc, king_minos_long_desc);

    return sep_test_suite;
}



mut_testsuite_t * fbe_test_create_sep_dualsp_configuration_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("configuration", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("configuration", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, larry_t_lobster_dualsp_test, larry_t_lobster_dualsp_setup, larry_t_lobster_dualsp_cleanup,
                                  larry_t_lobster_short_desc, larry_t_lobster_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, eliot_rosewater_dualsp_test, eliot_rosewater_dualsp_setup, eliot_rosewater_dualsp_cleanup, 
                                  eliot_rosewater_short_desc, eliot_rosewater_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, darkwing_duck_test, darkwing_duck_setup, darkwing_duck_cleanup,
                                  darkwing_duck_short_desc, darkwing_duck_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, peep_dualsp_test, peep_dualsp_setup, peep_dualsp_cleanup,
                                  peep_short_desc, peep_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, pokemon_dualsp_test, pokemon_dualsp_setup, pokemon_dualsp_cleanup,
                                  pokemon_short_desc, pokemon_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, chromatic_dualsp_test, chromatic_dualsp_setup, chromatic_dualsp_cleanup, 
                              chromatic_test_short_desc, chromatic_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, kungfu_panda_dualsp_test, kungfu_panda_dualsp_setup, kungfu_panda_dualsp_cleanup, 
                              kungfu_panda_short_desc, kungfu_panda_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ezio_dualsp_test, ezio_dualsp_setup, ezio_dualsp_cleanup, 
                                   ezio_dualsp_test_short_desc, ezio_dualsp_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, arwen_dualsp_test, arwen_dualsp_setup, arwen_dualsp_cleanup, 
                                  arwen_dualsp_short_desc, arwen_dualsp_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, vinda_dualsp_test, vinda_dualsp_setup, vinda_dualsp_cleanup, 
                                  vinda_dualsp_short_desc, vinda_dualsp_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, hamburger_dualsp_test, hamburger_dualsp_setup, hamburger_dualsp_cleanup,
                                  hamburger_short_desc, hamburger_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cassie_dualsp_test, cassie_dualsp_setup, cassie_dualsp_cleanup,
                                  cassie_dualsp_test_short_desc, cassie_dualsp_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, crayon_shin_chan_dualsp_test, crayon_shin_chan_dualsp_setup, crayon_shin_chan_dualsp_cleanup,
                                  crayon_shin_chan_dualsp_test_short_desc, crayon_shin_chan_dualsp_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_riding_hood_dualsp_test, red_riding_hood_dualsp_setup, red_riding_hood_dualsp_cleanup,
                                  red_riding_hood_short_desc, red_riding_hood_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, skippy_doo_test, skippy_doo_setup, skippy_doo_cleanup,
        skippy_doo_short_desc, skippy_doo_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test, sabbath_setup, sabbath_cleanup, 
                                  sabbath_test_short_desc, sabbath_test_long_desc)
                                  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test2, sabbath_setup, sabbath_cleanup, 
                                  sabbath_test_short_desc, sabbath_test_long_desc)
                                  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_move_drive_when_array_is_online, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_move_drive_when_array_is_offline_test1, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_move_drive_when_array_is_offline_test_bootflash_mode, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_move_drive_when_array_is_offline_test2, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_move_drive_when_array_is_offline_test3, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_drive_replacement_policy_online_test, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_drive_replacement_policy_offline_test, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sabbath_test_drive_replacement_policy_error_handling_offline_test, sabbath_setup, sabbath_cleanup,
                                  sabbath_test_short_desc, sabbath_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, broken_Rose_test, sabbath_setup, sabbath_cleanup,
                                  broken_Rose_test_short_desc, broken_Rose_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, dark_archon_test, sabbath_setup, sabbath_cleanup,
                                  dark_archon_test_short_desc, dark_archon_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, repairman_test, repairman_setup, repairman_cleanup,
                                  repairman_test_short_desc, repairman_test_long_desc)   

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, wreckit_ralph_dualsp_test_job_destroy_rg_test1, wreckit_ralph_dualsp_setup, wreckit_ralph_dualsp_cleanup,
                                  wreckit_ralph_short_desc, wreckit_ralph_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, wreckit_ralph_dualsp_test_job_destroy_rg_test2, wreckit_ralph_dualsp_setup, wreckit_ralph_dualsp_cleanup,
                                  wreckit_ralph_short_desc, wreckit_ralph_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, wreckit_ralph_dualsp_test_job_create_rg_test1, wreckit_ralph_dualsp_setup, wreckit_ralph_dualsp_cleanup,
                                  wreckit_ralph_short_desc, wreckit_ralph_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, wreckit_ralph_dualsp_test_job_create_rg_test2, wreckit_ralph_dualsp_setup, wreckit_ralph_dualsp_cleanup,
                                  wreckit_ralph_short_desc, wreckit_ralph_long_desc) 

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, dumbledore_dualsp_test, dumbledore_dualsp_setup, dumbledore_dualsp_cleanup, 
                                  dumbledore_short_desc, dumbledore_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, gravity_falls_dualsp_test, gravity_falls_dualsp_setup, gravity_falls_dualsp_cleanup, 
                                  gravity_falls_short_desc, gravity_falls_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, scrooge_mcduck_test, scrooge_mcduck_setup, scrooge_mcduck_cleanup, 
                                  scrooge_mcduck_test_short_desc, scrooge_mcduck_test_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, thomas_dualsp_test, thomas_dualsp_setup, thomas_dualsp_cleanup,
                                  thomas_short_desc, thomas_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, defiler_dualsp_test, defiler_dualsp_setup, defiler_dualsp_cleanup,
                                  defiler_short_desc, defiler_long_desc)

    return sep_dualsp_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_dualsp_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("degraded_io", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("degraded_io", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_single_degraded_test, big_bird_dualsp_setup, big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_single_degraded_zero_abort_test, big_bird_dualsp_setup, big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_shutdown_test, big_bird_dualsp_setup, big_bird_dualsp_cleanup,
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

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, rosita_dualsp_single_degraded_test, rosita_dualsp_setup, rosita_dualsp_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, rosita_dualsp_single_degraded_zero_abort_test, rosita_dualsp_setup, rosita_dualsp_cleanup,
                                  rosita_short_desc, rosita_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, rosita_dualsp_shutdown_test, rosita_dualsp_setup, rosita_dualsp_cleanup,
                                  rosita_short_desc, rosita_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, oscar_dualsp_degraded_test, oscar_dualsp_setup, oscar_dualsp_cleanup,
                                  oscar_short_desc, oscar_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, oscar_dualsp_degraded_zero_abort_test, oscar_dualsp_setup, oscar_dualsp_cleanup,
                                  oscar_short_desc, oscar_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, oscar_dualsp_shutdown_test, oscar_dualsp_setup, oscar_dualsp_cleanup,
                                  oscar_short_desc, oscar_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, chitti_dualsp_test, chitti_dualsp_setup, chitti_dualsp_cleanup,
                                  chitti_short_desc, chitti_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, fozzie_dualsp_test, fozzie_dualsp_setup, fozzie_dualsp_cleanup,
                                  fozzie_short_desc, fozzie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, edgar_the_bug_dualsp_test, edgar_the_bug_dualsp_setup,
                                  edgar_the_bug_dualsp_cleanup, edgar_the_bug_short_desc, edgar_the_bug_long_desc)
    /*! @note Currently we don't encounter any aborted memory requests.*/
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, frank_the_pug_dualsp_test, frank_the_pug_dualsp_setup, frank_the_pug_dualsp_cleanup,
    //                              frank_the_pug_short_desc, frank_the_pug_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, gimli_dualsp_test, gimli_dualsp_setup, gimli_dualsp_cleanup,
                                  gimli_short_desc, gimli_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, smelly_cat_dualsp_test, smelly_cat_dualsp_setup, smelly_cat_dualsp_cleanup,
                                  smelly_cat_short_desc, smelly_cat_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, vulture_dualsp_test, vulture_dualsp_setup, vulture_dualsp_cleanup,
                                  vulture_short_desc, vulture_long_desc)

    return sep_dualsp_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_dualsp_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("disk_errors", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("disk_errors", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, abby_cadabby_dualsp_raid5_test, abby_cadabby_dualsp_raid5_setup, abby_cadabby_dualsp_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, abby_cadabby_dualsp_raid3_test, abby_cadabby_dualsp_raid3_setup, abby_cadabby_dualsp_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, abby_cadabby_dualsp_raid6_test, abby_cadabby_dualsp_raid6_setup, abby_cadabby_dualsp_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, abby_cadabby_dualsp_raid1_test, abby_cadabby_dualsp_raid1_setup, abby_cadabby_dualsp_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, abby_cadabby_dualsp_raid10_test, abby_cadabby_dualsp_raid10_setup, abby_cadabby_dualsp_cleanup,
                                  abby_cadabby_short_desc, abby_cadabby_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid5_test, cookie_monster_dualsp_raid5_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid3_test, cookie_monster_dualsp_raid3_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid6_test, cookie_monster_dualsp_raid6_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid0_test, cookie_monster_dualsp_raid0_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid1_test, cookie_monster_dualsp_raid1_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, cookie_monster_dualsp_raid10_test, cookie_monster_dualsp_raid10_setup, cookie_monster_dualsp_cleanup,
                                  cookie_monster_short_desc, cookie_monster_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, rescue_rangers_dualsp_test, rescue_rangers_dualsp_setup, rescue_rangers_dualsp_cleanup,
                                  rescue_rangers_short_desc, rescue_rangers_long_desc)
    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_normal_io_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("normal_io", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("normal_io", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, super_hv_dualsp_test, 
                                  super_hv_dualsp_setup, super_hv_dualsp_cleanup,
                                  super_hv_short_desc, super_hv_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, elmo_dualsp_test, elmo_dualsp_setup, elmo_dualsp_cleanup,
                                  elmo_short_desc, elmo_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, zoe_dualsp_test, zoe_dualsp_setup, zoe_dualsp_cleanup,
                                  zoe_short_desc, zoe_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, grover_dualsp_test, grover_dualsp_setup, grover_dualsp_cleanup,
                                  grover_short_desc, grover_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, snuffy_dualsp_test, snuffy_dualsp_setup, snuffy_dualsp_cleanup,
                                  snuffy_short_desc, snuffy_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, telly_dualsp_test, telly_dualsp_setup, telly_dualsp_cleanup,
                                  telly_short_desc, telly_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, prairie_dawn_dualsp_test, prairie_dawn_dualsp_setup, prairie_dawn_dualsp_cleanup,
                                  prairie_dawn_short_desc, prairie_dawn_long_desc)
    /*! @todo Need to update database to use 4K aligned requests. 
     */
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ultra_hd_dualsp_test, ultra_hd_dualsp_setup, ultra_hd_dualsp_cleanup,
    //                              ultra_hd_short_desc, ultra_hd_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, pippin_dualsp_test, pippin_dualsp_setup, pippin_dualsp_cleanup,
                                  pippin_short_desc, pippin_long_desc)

    /* This can not be run with python, because it consumes a lot of memory
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, performance_test, performance_test_init, performance_test_destroy,
                              performance_test_short_desc, performance_test_long_desc);
    */
    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_power_savings_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("power_savings", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("power_savings", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, woozle_test, woozle_setup, woozle_destroy, 
                                  woozle_short_desc, woozle_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, weerd_dualsp_test, weerd_dualsp_setup, weerd_dualsp_cleanup, 
                                  weerd_short_desc, weerd_long_desc)

    return sep_dualsp_test_suite;

}
mut_testsuite_t * fbe_test_create_sep_dualsp_rebuild_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("rebuild", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("rebuild", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, dora_dualsp_test, dora_dualsp_setup, dora_dualsp_cleanup,
                                  dora_short_desc, dora_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, tico_dualsp_test, tico_dualsp_setup, tico_dualsp_cleanup,
                                  tico_short_desc, tico_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mimi_dualsp_test, mimi_dualsp_setup, mimi_dualsp_cleanup,
                                  mimi_short_desc, mimi_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, zoidberg_dualsp_test, zoidberg_dualsp_setup, zoidberg_dualsp_cleanup,
                                  zoidberg_short_desc, zoidberg_long_desc)  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bender_dualsp_test, bender_dualsp_setup, bender_dualsp_cleanup,
                                  bender_test_short_desc, bender_test_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, marina_dualsp_test, marina_dualsp_setup, marina_dualsp_cleanup,
                                  marina_short_desc, marina_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, doodle_dualsp_test, doodle_dualsp_setup, doodle_dualsp_cleanup,
                                  doodle_short_desc, doodle_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, captain_planet_dualsp_test, captain_planet_dualsp_setup, captain_planet_dualsp_cleanup,
                                  captain_planet_short_desc, captain_planet_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, splinter_dualsp_test, splinter_dualsp_setup, splinter_dualsp_cleanup,
                                  splinter_short_desc, splinter_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, shredder_dualsp_test, shredder_dualsp_setup, shredder_dualsp_cleanup,
                                  shredder_short_desc, shredder_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, samwise_gamgee_dualsp_test, samwise_gamgee_dualsp_setup, samwise_gamgee_dualsp_cleanup,
                                  samwise_gamgee_short_desc, samwise_gamgee_long_desc);
#if 0
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, tico_dualsp_test, tico_dualsp_setup, tico_dualsp_cleanup,
                                  tico_short_desc, tico_long_desc)
#endif
    return sep_dualsp_test_suite;

}
mut_testsuite_t * fbe_test_create_sep_dualsp_services_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("services", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("services", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, kozma_prutkov_dualsp_test, kozma_prutkov_test_dualsp_init, 
                                  kozma_prutkov_test_dualsp_destroy, 
                                  kozma_prutkov_short_desc, kozma_prutkov_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, danny_din_dualsp_test, danny_din_dualsp_setup, danny_din_dualsp_destroy,
                                  danny_din_short_desc, danny_din_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ovejita_dualsp_test, ovejita_dualsp_setup, ovejita_dualsp_cleanup, 
                                  ovejita_short_desc, ovejita_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, hermione_dualsp_test, hermione_dualsp_setup, hermione_dualsp_cleanup,
                                  hermione_short_desc, hermione_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, snakeskin_dualsp_test, snakeskin_test_dualsp_init, 
                                  snakeskin_test_dualsp_destroy, 
                                  snakeskin_short_desc, snakeskin_long_desc);

    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_sniff_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("sniff", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("sniff", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);
    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_sparing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("sparing", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("sparing", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);


    /*Run Permanent Sparing Test on Dual SP*/
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, shaggy_dualsp_test, shaggy_dualsp_setup, shaggy_dualsp_cleanup,
                                  shaggy_short_desc, shaggy_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, freddie_dualsp_test, freddie_dualsp_setup, freddie_dualsp_cleanup,
                                  freddie_short_desc, freddie_long_desc)


    /*! @note This test is commented out on purpose.  We use it to run dualsp 
     *        simulation with large drives.
     */
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, scoobert_dualsp_test, scoobert_dualsp_setup, scoobert_dualsp_cleanup,
    //                              scoobert_short_desc, scoobert_long_desc)

    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_copy_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("copy", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("copy", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, diabolicaldiscdemon_dualsp_test, diabolicaldiscdemon_dualsp_setup, diabolicaldiscdemon_dualsp_cleanup,
                                  diabolicaldiscdemon_short_desc, diabolicaldiscdemon_long_desc)

     MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, diabolicaldiscdemon_timeout_errors_dualsp_test, diabolicaldiscdemon_dualsp_setup, diabolicaldiscdemon_dualsp_cleanup,
                                  diabolicaldiscdemon_short_desc, diabolicaldiscdemon_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bogel_dualsp_test, bogel_dualsp_setup, bogel_dualsp_cleanup,
                                  bogel_short_desc, bogel_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, dooby_doo_dualsp_test, dooby_doo_dualsp_setup, dooby_doo_dualsp_cleanup,
                                  dooby_doo_short_desc, dooby_doo_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_herring_dualsp_raid1_test, red_herring_dualsp_raid1_setup, red_herring_dualsp_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_herring_dualsp_raid10_test, red_herring_dualsp_raid10_setup, red_herring_dualsp_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_herring_dualsp_raid3_test, red_herring_dualsp_raid3_setup, red_herring_dualsp_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_herring_dualsp_raid5_test, red_herring_dualsp_raid5_setup, red_herring_dualsp_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, red_herring_dualsp_raid6_test, red_herring_dualsp_raid6_setup, red_herring_dualsp_cleanup,
                                  red_herring_short_desc, red_herring_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, vincent_van_ghoul_dualsp_test, vincent_van_ghoul_dualsp_setup, vincent_van_ghoul_dualsp_cleanup,
                                  vincent_van_ghoul_short_desc, vincent_van_ghoul_long_desc)

    /*! @todo The following dualsp tests don't function. 
     */                        
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, scooby_doo_dualsp_test, scooby_doo_dualsp_setup, scooby_doo_dualsp_cleanup,
    //                              scooby_doo_short_desc, scooby_doo_long_desc)

    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_special_ops_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("special_ops", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("special_ops", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, biff_dualsp_test, biff_dualsp_setup, biff_dualsp_cleanup,
        biff_short_desc, biff_long_desc)
    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, 
                                  kipper_health_check_dualsp_test, 
                                  kipper_dualsp_setup, 
                                  kipper_dualsp_cleanup,
                                  kipper_short_desc, 
                                  kipper_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, 
                                  pingu_slot_limit_dualsp_test, 
                                  pingu_slot_limit_dualsp_setup, 
                                  pingu_slot_limit_dualsp_cleanup,
                                  pingu_slot_limit_short_desc, 
                                  pingu_slot_limit_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, walle_cleaner_test, walle_cleaner_setup, walle_cleaner_cleanup,
                                  walle_cleaner_short_desc, walle_cleaner_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sully_test, sully_dualsp_setup, sully_dualsp_cleanup,
                                  sully_short_desc, sully_long_desc)


/*!@todo DE729 comment out doraemon test */
#if 0    
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, doraemon_user_dualsp_test, doraemon_dualsp_setup, doraemon_dualsp_cleanup,
        doraemon_short_desc, doraemon_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, doraemon_system_dualsp_test, doraemon_dualsp_setup, doraemon_dualsp_cleanup,
        doraemon_short_desc, doraemon_long_desc)
#endif        

    return sep_dualsp_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_dualsp_encryption_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("encryption", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("encryption", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, minion_test, minion_setup, minion_cleanup,
                                  minion_short_desc, minion_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, minion_reboot_test, minion_setup, minion_cleanup,
                                  minion_short_desc, minion_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, 
                                  kalia_dualsp_test,
                                  kalia_dualsp_setup,
                                  kalia_dualsp_cleanup, 
                                  kalia_short_desc,
                                  kalia_long_desc)

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_zero_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_unconsumed_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
#endif
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_system_drive_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_system_drive_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_read_only_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_check_pattern_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
#endif                                  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_dualsp_slf_test, ichabod_dualsp_setup, ichabod_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_system_drive_createrg_encrypt_reboot_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_normal_scrubbing_test, mike_the_knight_dualsp_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_scrubbing_consumed_test, mike_the_knight_dualsp_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_destroy_rg_before_encryption, mike_the_knight_dualsp_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_scrubbing_garbage_collection_test, mike_the_knight_dualsp_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_normal_scrubbing_drive_pull_test, mike_the_knight_dualsp_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, mike_the_knight_dualsp_reboot_test, mike_the_knight_dualsp_reboot_setup, mike_the_knight_dualsp_cleanup,
                                  mike_the_knight_short_desc, mike_the_knight_long_desc)
#endif
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_incorrect_key_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_incorrect_key_one_drive_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_rg_create_key_error_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_rg_create_system_drives_key_error_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_key_error_io_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_validate_key_error_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_crane_validate_key_error_system_drive_test, ichabod_setup, ichabod_cleanup,
                                  ichabod_short_desc, ichabod_long_desc)

#if 0 // Remove this test cases since they are obsolete.  Please use ichabod_crane test cases.
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_kms_test, ichabod_kms_setup, ichabod_kms_cleanup,
                                  ichabod_short_desc, ichabod_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ichabod_kms_dualsp_test, ichabod_kms_dualsp_setup, ichabod_kms_dualsp_cleanup,
                                  ichabod_short_desc, ichabod_long_desc);
#endif

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sleepy_hollow_dualsp_test, sleepy_hollow_dualsp_setup, sleepy_hollow_dualsp_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sleepy_hollow_delayed_encryption_dualsp_test, 
                                  sleepy_hollow_dualsp_setup, sleepy_hollow_dualsp_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sleepy_hollow_vault_in_use_dualsp_test, 
                                  sleepy_hollow_dualsp_setup, sleepy_hollow_dualsp_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, sleepy_hollow_vault_zero_dualsp_test, 
                                  sleepy_hollow_dualsp_setup_sim_psl, sleepy_hollow_dualsp_cleanup,
                                  sleepy_hollow_short_desc, sleepy_hollow_long_desc)
#endif

#if 0
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, katrina_dualsp_test, katrina_dualsp_setup, katrina_dualsp_cleanup,
                                  katrina_short_desc, katrina_long_desc)
#endif
 
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, shaggy_encryption_dualsp_test, shaggy_encryption_dualsp_setup, shaggy_encryption_dualsp_cleanup,
                                  shaggy_short_desc, shaggy_long_desc);
#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_large_rg_reboot_test, 
                                  headless_horseman_dualsp_large_rg_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_large_r10_reboot_test, 
                                  headless_horseman_dualsp_large_r10_rg_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid0_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid1_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid3_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid5_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid6_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_dualsp_raid10_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid0_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid1_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid3_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid5_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid6_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, headless_horseman_reboot_dualsp_raid10_test, 
                                  headless_horseman_dualsp_setup, headless_horseman_dualsp_cleanup,
                                  headless_horseman_short_desc, headless_horseman_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, diabolicaldiscdemon_encryption_dualsp_test, diabolicaldiscdemon_encrypted_dualsp_setup, diabolicaldiscdemon_encrypted_dualsp_cleanup,
                                  diabolicaldiscdemon_short_desc, diabolicaldiscdemon_long_desc)
    /*! @todo Seeing a failure where the database gets out of sync with kms after a reboot. */
    //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, vincent_van_ghoul_dualsp_encrypted_test, vincent_van_ghoul_dualsp_encrypted_setup, vincent_van_ghoul_dualsp_encrypted_cleanup,
    //                              vincent_van_ghoul_short_desc, vincent_van_ghoul_long_desc)

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, big_bird_dualsp_single_degraded_encryption_test, 
                                  big_bird_dualsp_encryption_setup, big_bird_dualsp_cleanup,
                                  big_bird_short_desc, big_bird_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bert_dualsp_single_degraded_encryption_test, 
                                  bert_dualsp_encryption_setup, bert_dualsp_cleanup,
                                  bert_short_desc, bert_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_single_degraded_encryption_test, 
                                  ernie_dualsp_encryption_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, ernie_dualsp_double_degraded_encryption_test, 
                                  ernie_dualsp_encryption_setup, ernie_dualsp_cleanup,
                                  ernie_short_desc, ernie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, rosita_dualsp_single_degraded_encryption_test, 
                                  rosita_dualsp_encryption_setup, rosita_dualsp_cleanup,
                                  rosita_short_desc, rosita_long_desc)
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, evie_dualsp_test, evie_dualsp_setup, evie_dualsp_cleanup,
                                  evie_short_desc, evie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, evie_drive_relocation_dualsp_test, evie_dualsp_setup, evie_dualsp_cleanup,
                                  evie_short_desc, evie_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, evie_reboot_and_proactive_dualsp_test, evie_dualsp_setup, evie_dualsp_cleanup,
                                  evie_short_desc, evie_long_desc)

#ifndef C4_INTEGRATED  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, nearly_headless_nick_dualsp_test, nearly_headless_nick_dualsp_setup, 
                                  nearly_headless_nick_dualsp_cleanup,
                                  nearly_headless_nick_short_desc, nearly_headless_nick_long_desc)
#endif
    
    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_verify_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("verify", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("verify", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);


    /*! @todo Currently (03/31/2011) Clifford hangs waiting for rebuild in dualsp 
     *        private branch.  Therefore it is currently disabled 
     */
   //MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, clifford_dualsp_test, clifford_dualsp_setup, clifford_dualsp_cleanup,
   //                               clifford_dualsp_short_desc, clifford_dualsp_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, frodo_baggins_test, frodo_baggins_setup, frodo_baggins_cleanup,
                                  frodo_baggins_short_desc, frodo_baggins_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, bilbo_baggins_dualsp_test, bilbo_baggins_dualsp_setup, bilbo_baggins_dualsp_cleanup,
                                  bilbo_baggins_short_desc, bilbo_baggins_long_desc);
    return sep_dualsp_test_suite;
}
mut_testsuite_t * fbe_test_create_sep_dualsp_zeroing_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("zeroing", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("zeroing", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, scooby_dum_dualsp_test, scooby_dum_dualsp_setup, scooby_dum_dualsp_cleanup, 
                                  scooby_dum_short_desc, scooby_dum_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, henery_the_baby_chicken_hawk_dualsp_test, henery_the_baby_chicken_hawk_dualsp_setup, henery_the_baby_chicken_hawk_dualsp_cleanup,
                                  henery_the_baby_chicken_hawk_short_desc, henery_the_baby_chicken_hawk_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, barnyard_dawg_dualsp_test, barnyard_dawg_dualsp_setup,  barnyard_dawg_dualsp_cleanup,
                                  barnyard_dawg_short_desc, barnyard_dawg_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, kit_cloudkicker_dualsp_test, kit_cloudkicker_dualsp_setup, kit_cloudkicker_dualsp_cleanup, 
                                  kit_cloudkicker_short_desc, kit_cloudkicker_long_desc)
                                  
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, jingle_bell_dualsp_test, jingle_bell_dualsp_setup, jingle_bell_dualsp_cleanup, 
                                  jingle_bell_short_desc, jingle_bell_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, starbuck_test, starbuck_setup, starbuck_cleanup,
                                  starbuck_short_desc, starbuck_long_desc)

    return sep_dualsp_test_suite;
}

mut_testsuite_t * fbe_test_create_sep_extent_pool_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_SUITE_NAME("extent_pool", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_SUITE_NAME("extent_pool", &buffer[0]);
    }
    sep_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, scooter_test, scooter_setup, scooter_cleanup,
                                  scooter_short_desc, scooter_long_desc)

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_test_suite, gonzo_test, gonzo_setup, gonzo_cleanup,
                                  gonzo_short_desc, gonzo_long_desc)

    return sep_test_suite;
}


mut_testsuite_t * fbe_test_create_sep_dualsp_extent_pool_test_suite(mut_function_t startup, mut_function_t teardown)
{
    mut_testsuite_t * sep_dualsp_test_suite = NULL;
    fbe_char_t buffer[SEP_SUITE_NAME_LENGTH];
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        SEP_DUAL_SP_SUITE_NAME("extent_pool", &buffer[0]);
    }
    else
    {
        SEP_EXTENDED_DUAL_SP_SUITE_NAME("extent_pool", &buffer[0]);
    }
    sep_dualsp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED(buffer, startup, teardown);

    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, scooter_dualsp_test, scooter_dualsp_setup, scooter_dualsp_cleanup,
                                  scooter_short_desc, scooter_long_desc)
    MUT_ADD_TEST_WITH_DESCRIPTION(sep_dualsp_test_suite, gonzo_dualsp_test, gonzo_dualsp_setup, gonzo_dualsp_cleanup,
                                  gonzo_short_desc, gonzo_long_desc)
    
    return sep_dualsp_test_suite;
}
