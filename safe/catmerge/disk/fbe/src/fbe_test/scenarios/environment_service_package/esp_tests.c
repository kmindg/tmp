#include "esp_tests.h"
#include "mut.h"
#include "fbe_test_common_utils.h"

mut_testsuite_t * fbe_test_create_esp_test_suite(mut_function_t startup, mut_function_t teardown) {

    mut_testsuite_t *esp_test_suite = NULL;

    if (fbe_test_get_default_extended_test_level() == 0)
    {
        esp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("esp_test_suite",startup, teardown);
    }
    else
    {
        esp_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("extended_esp_test_suite",startup, teardown);
    }
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, ratbert_test, NULL, NULL,
                                  ratbert_short_desc, ratbert_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dogbert_test, NULL, NULL,
                                  dogbert_short_desc, dogbert_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, spiderman_test, spiderman_setup, spiderman_destroy,
                                  spiderman_short_desc, spiderman_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, travis_bickle_oberon_test, travis_bickle_oberon_setup, travis_bickle_destroy,
                                  travis_bickle_short_desc, travis_bickle_long_desc);
// ENCL_CLEANUP
/*
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, travis_bickle_hyperion_test, travis_bickle_hyperion_setup, travis_bickle_destroy,
                                  travis_bickle_short_desc, travis_bickle_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, travis_bickle_triton_test, travis_bickle_triton_setup, travis_bickle_destroy,
                                  travis_bickle_short_desc, travis_bickle_long_desc);
*/
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dilbert_test, dilbert_setup, dilbert_destroy,
                                  dilbert_short_desc, dilbert_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, norma_desmond_oberon_test, norma_desmond_oberon_setup, spiderman_destroy,
                                  norma_desmond_short_desc, norma_desmond_long_desc);
// ENCL_CLEANUP
/*
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, norma_desmond_hyperion_test, norma_desmond_hyperion_setup, spiderman_destroy,
                                  norma_desmond_short_desc, norma_desmond_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, norma_desmond_triton_test, norma_desmond_triton_setup, spiderman_destroy,
                                  norma_desmond_short_desc, norma_desmond_long_desc);
*/
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, cinderella_test, cinderella_setup, cinderella_destroy,
                                  cinderella_short_desc, cinderella_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, vito_corleone_test, vito_corleone_setup, vito_corleone_destroy,
                                  vito_corleone_short_desc, vito_corleone_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, master_cylinder_test, master_cylinder_setup, spiderman_destroy,
                                  master_cylinder_short_desc, master_cylinder_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, sir_topham_hatt_test, sir_topham_hatt_setup, sir_topham_hatt_destroy,
                                  sir_topham_hatt_short_desc, sir_topham_hatt_long_desc);
#if 0
    /* The current PVD design requests any download request to be finished or aborted.
     * This test does not count in the new requirement. Disable them for now.
     */
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, thomas_the_tank_engine_test, sir_topham_hatt_setup, sir_topham_hatt_destroy,
                                  thomas_the_tank_engine_short_desc, thomas_the_tank_engine_long_desc);
#endif

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, baloo_test, baloo_setup, baloo_destroy,
                                  baloo_short_desc, baloo_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, calvin_test, calvin_setup, calvin_destroy,
                                  calvin_short_desc, calvin_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, susie_derkins_test, susie_derkins_setup, susie_derkins_destroy,
                                  susie_derkins_short_desc, susie_derkins_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, hobbes_test, hobbes_setup, hobbes_destroy,
                                  hobbes_short_desc, hobbes_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, rosalyn_test, rosalyn_setup, rosalyn_destroy,
                                  rosalyn_short_desc, rosalyn_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dirty_harry_test, dirty_harry_setup, spiderman_destroy,
                                  dirty_harry_short_desc, dirty_harry_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, forrest_gump_test, forrest_gump_setup, forrest_gump_destroy,
                                  forrest_gump_short_desc, forrest_gump_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, cool_hand_luke_test, cool_hand_luke_setup, cool_hand_luke_destroy,
                                  cool_hand_luke_short_desc, cool_hand_luke_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, sleeping_beauty_test, sleeping_beauty_setup, sleeping_beauty_destroy, 
                                  sleeping_beauty_short_desc, sleeping_beauty_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, vikram_and_bethaal_test, vikram_and_bethaal_setup, vikram_and_bethaal_destroy,
                                  vikram_and_bethaal_short_desc, vikram_and_bethaal_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, tarzan_test, tarzan_setup, tarzan_destroy,
                                  tarzan_short_desc, tarzan_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, presto_test, presto_setup, presto_destroy,
                                  presto_short_desc, presto_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, chanakya_test, chanakya_setup, chanakya_destroy,
                                  chanakya_short_desc, chanakya_long_desc); 
    
    /* Firmware upgrade test scenarios start */
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, edward_and_bella_test, edward_and_bella_setup, edward_and_bella_destroy,
                                  edward_and_bella_short_desc, edward_and_bella_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, curious_george_test, curious_george_setup, curious_george_destroy,
                                  curious_george_short_desc, curious_george_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, vavoom_test, vavoom_setup, vavoom_destroy,
                                  vavoom_short_desc, vavoom_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, garnett_test, garnett_setup, garnett_destroy,
                                  garnett_short_desc, garnett_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, paul_pierce_test, paul_pierce_setup, paul_pierce_destroy,
                                  paul_pierce_short_desc, paul_pierce_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, poindexter_test, poindexter_setup, poindexter_destroy,
                                  poindexter_short_desc, poindexter_long_desc);   

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, edward_the_scissorhands_test, edward_the_scissorhands_setup, edward_the_scissorhands_destroy,
                                  edward_the_scissorhands_short_desc, edward_the_scissorhands_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, grandma_wolf_test, grandma_wolf_setup, grandma_wolf_destroy,
                                  grandma_wolf_short_desc, grandma_wolf_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, tintin_test, tintin_setup, tintin_destroy,
                                  tintin_short_desc, tintin_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, felix_the_cat_test, felix_the_cat_setup, felix_the_cat_destroy,
                                  felix_the_cat_short_desc, felix_the_cat_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, grandma_robot_test, grandma_robot_setup, grandma_robot_destroy,
                                  grandma_robot_short_desc, grandma_robot_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, viking_simple_fup_test, viking_simple_fup_setup, viking_simple_fup_destroy,
                                  viking_simple_fup_short_desc, viking_simple_fup_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dumbo_test, dumbo_setup, dumbo_destroy,
                                  dumbo_short_desc, dumbo_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, corey_combine_test, corey_combine_setup, corey_combine_destroy,
                                  corey_combine_short_desc, corey_combine_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, johnny_tractor_test, johnny_tractor_setup, johnny_tractor_destroy,
                                  johnny_tractor_short_desc, johnny_tractor_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, allie_gator_test, allie_gator_setup, allie_gator_destroy,
                                  allie_gator_short_desc, allie_gator_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, wall_e_test, wall_e_setup, wall_e_destroy,
                                  wall_e_short_desc, wall_e_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, peter_rabbit_test, peter_rabbit_setup, peter_rabbit_destroy,
                                  peter_rabbit_short_desc, peter_rabbit_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, cdes2_ps_fup_test, cdes2_ps_fup_setup, cdes2_ps_fup_destroy,
                                  cdes2_ps_fup_short_desc, cdes2_ps_fup_long_desc);

    /* Firmware upgrade test scenarios end. */

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, marcus_oberon_test, marcus_oberon_setup, marcus_destroy,
                                  marcus_short_desc, marcus_long_desc);
//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, marcus_hyperion_test, marcus_hyperion_setup, marcus_destroy,
//                                  marcus_short_desc, marcus_long_desc);
// ENCL_CLEANUP
//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, marcus_triton_test, marcus_triton_setup, marcus_destroy,
//                                  marcus_short_desc, marcus_long_desc);

// Jetfire was removed from power supply handling so this test does not pass any longer
//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, skeletor_jetfire_m4_test, skeletor_jetfire_m4_setup, skeletor_destroy,
//                                  skeletor_short_desc, skeletor_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, popeye_doyle_megatron_test, popeye_doyle_megatron_setup, popeye_doyle_destroy,
                                  popeye_doyle_short_desc, popeye_doyle_long_desc); //kostring not touched

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, tinkle_test, tinkle_setup, tinkle_destroy,
                                  tinkle_short_desc, tinkle_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, ravana_fault_reg_mt_test, ravana_fault_reg_megatron_setup, ravana_fault_reg_destroy,
                                  ravana_fault_reg_megatron_short_desc, ravana_fault_reg_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, ravana_fault_reg_jf_test, ravana_fault_reg_jetfire_setup, ravana_fault_reg_destroy,
                                  ravana_fault_reg_jetfire_short_desc, ravana_fault_reg_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, ravana_ext_peer_boot_test, ravana_ext_peer_boot_megatron_setup, ravana_ext_peer_boot_destroy,
                                  ravana_ext_peer_boot_megatron_short_desc, ravana_ext_peer_boot_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, tenali_raman_test, tenali_raman_setup, tenali_raman_destroy,
                                  tenali_raman_short_desc, tenali_raman_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, vibhishana_test, vibhishana_setup, vibhishana_destroy,
                                  vibhishana_short_desc, vibhishana_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, vayu_test, vayu_setup, vayu_destroy,
                                  vayu_short_desc, vayu_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, bigwig_test, bigwig_setup, bigwig_destroy,
                                  bigwig_short_desc, bigwig_long_desc);

    /* ESP Module Management Tests */
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp1_ikkyusan_test, fp1_ikkyusan_setup, fp1_ikkyusan_destroy,
                                  fp1_ikkyusan_short_desc, fp1_ikkyusan_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp2_sudsakorn_test, fp2_sudsakorn_setup, fp2_sudsakorn_destroy,
                                  fp2_sudsakorn_short_desc, fp2_sudsakorn_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp3_rakka_test, fp3_rakka_setup, fp3_rakka_destroy,
                                  fp3_rakka_short_desc, fp3_rakka_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp4_reki_test, fp4_reki_setup, fp4_reki_destroy,
                                  fp4_reki_short_desc, fp4_reki_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp5_kuu_test, fp5_kuu_setup, fp5_kuu_destroy,
                                  fp5_kuu_short_desc, fp5_kuu_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp6_nemu_test, fp6_nemu_setup, fp6_nemu_destroy,
                                  fp6_nemu_short_desc, fp6_nemu_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp7_venger_test, fp7_venger_setup, fp7_venger_destroy,
                                  fp7_venger_short_desc, fp7_venger_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp7_venger_test2, fp7_venger_test2_setup, fp7_venger_destroy,
                                  fp7_venger_test2_short_desc, fp7_venger_test2_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp7_venger_test3, fp7_venger_test3_setup, fp7_venger_destroy,
                                  fp7_venger_test3_short_desc, fp7_venger_test3_long_desc);

//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp8_twinkle_twinkle_test, fp8_twinkle_twinkle_setup, fp8_twinkle_twinkle_destroy,
//                                  fp8_twinkle_twinkle_short_desc, fp8_twinkle_twinkle_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp9_fflewddur_fflam_test, fp9_fflewddur_fflam_setup, fp9_fflewddur_fflam_destroy,
                                  fp9_fflewddur_fflam_short_desc, fp9_fflewddur_fflam_long_desc);
                                                                  
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp10_funshine_bear_test, fp10_funshine_bear_setup, fp10_funshine_bear_destroy,
                                  fp10_funshine_bear_short_desc, fp10_funshine_bear_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp11_jorgen_von_strangle_test, fp11_jorgen_von_strangle_setup, fp11_jorgen_von_strangle_destroy,
                                  fp11_jorgen_von_strangle_short_desc, fp11_jorgen_von_strangle_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp12_snarf_test, fp12_snarf_setup, fp12_snarf_destroy,
                                  fp12_snarf_short_desc, fp12_snarf_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp13_orko_test, fp13_orko_setup, fp13_orko_destroy,
                                  fp13_orko_short_desc, fp13_orko_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp14_tick_tock_test, fp14_tick_tock_setup, fp14_tick_tock_destroy,
                                  fp14_tick_tock_short_desc, fp14_tick_tock_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp15_mumra_test, fp15_mumra_setup, fp15_mumra_destroy,
                                  fp15_mumra_short_desc, fp15_mumra_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, fp16_jackalman_test, fp16_jackalman_setup, fp16_jackalman_destroy,
                                  fp16_jackalman_short_desc, fp16_jackalman_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, drive_remove_insert_test, drive_remove_insert_test_setup, drive_remove_insert_test_destroy,
                                  drive_remove_insert_short_desc, drive_remove_insert_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dae_counts_test, dae_counts_test_setup, dae_counts_test_destroy,
                                  dae_counts_test_short_desc, dae_counts_test_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, pandavas_oberon_test, pandavas_oberon_setup, pandavas_destroy,
                                  pandavas_short_desc, pandavas_long_desc);
// ENCL_CLEANUP
//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, pandavas_hyperion_test, pandavas_hyperion_setup, pandavas_destroy,
//                                  pandavas_short_desc, pandavas_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, eva_01_derringer_test, eva_01_derringer_test_setup, eva_01_test_destroy,
                                  eva_01_short_desc, eva_01_long_desc); //kostring not touched
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, eva_01_voyager_test, eva_01_voyager_test_setup, eva_01_test_destroy,
                                  eva_01_short_desc, eva_01_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, randle_mcmurphy_test, randle_mcmurphy_setup, randle_mcmurphy_destroy,
                                  randle_mcmurphy_short_desc, randle_mcmurphy_long_desc);

//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, elmer_gantry_test, elmer_gantry_setup, elmer_gantry_destroy,
//                                  elmer_gantry_short_desc, elmer_gantry_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, elmer_gantry_oberon_test, elmer_gantry_oberon_setup, elmer_gantry_destroy,
                                  elmer_gantry_short_desc, elmer_gantry_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, liberty_valance_viper_test, liberty_valance_viper_setup, liberty_valance_destroy,
                                  liberty_valance_short_desc, liberty_valance_long_desc);
//    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, liberty_valance_voyager_test, liberty_valance_voyager_setup, liberty_valance_destroy,
//                                  liberty_valance_short_desc, liberty_valance_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, achilles_test, achilles_setup, achilles_destroy,
                                  achilles_short_desc, achilles_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, percy_test, percy_setup, percy_destroy, 
                                  percy_short_desc, percy_long_desc);

        MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, po_test, po_setup, po_destroy,
                                  po_short_desc, po_long_desc);

        MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, captain_queeg_megatron_test, captain_queeg_megatron_setup, captain_queeg_destroy,
                                  captain_queeg_short_desc, captain_queeg_long_desc);
        MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, captain_queeg_jetfire_test, captain_queeg_jetfire_setup, captain_queeg_destroy,
                                  captain_queeg_short_desc, captain_queeg_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, anduin_lothar_test, anduin_lothar_test_setup, anduin_lothar_test_destroy,
        anduin_lothar_short_desc, anduin_lothar_long_desc);  //kostring not touched

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, guti_jetfire_test, guti_jetfire_setup, guti_destroy,
                                  guti_short_desc, guti_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, guti_megatron_test, guti_megatron_setup, guti_destroy,
                                  guti_short_desc, guti_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, bamboo_bmc_jetfire_test, bamboo_bmc_jetfire_setup, bamboo_bmc_destroy,
                                  bamboo_bmc_short_desc, bamboo_bmc_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, bamboo_bmc_megatron_test, bamboo_bmc_megatron_setup, bamboo_bmc_destroy,
                                  bamboo_bmc_short_desc, bamboo_bmc_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, component_accessible_test, component_accessible_setup, component_accessible_destroy,
                                  component_accessible_short_desc, component_accessible_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, birbal_test, birbal_setup, birbal_destroy,
                                  birbal_short_desc, birbal_long_desc);
    /* End of ESP Module Management Tests */

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, figment_test, figment_setup, figment_destroy,
                                  figment_short_desc, figment_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, alucard_test, alucard_test_setup, alucard_test_destroy,
            alucard_short_desc, alucard_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dracula_test_part1, NULL, NULL,
            dracula_short_desc, dracula_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dracula_test_part2, NULL, NULL,
            dracula_short_desc, dracula_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dracula_test_part3, NULL, NULL,
            dracula_short_desc, dracula_long_desc);
    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, dracula_test_part4, NULL, NULL,
            dracula_short_desc, dracula_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, colossus_test2, colossus_load_esp, colossus_cleanup,
                                  colossus_short_desc, colossus_long_desc);

    MUT_ADD_TEST_WITH_DESCRIPTION(esp_test_suite, beast_man_test, beast_man_test_load_test_env, beast_man_destroy,
                                  beast_man_short_desc, beast_man_long_desc);

    
    return esp_test_suite;
}
