#ifndef ESP_TESTS_H
#define ESP_TESTS_H

#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_devices.h"

#define ESP_EXPECTED_ERROR 0

extern fbe_class_id_t test_esp_class_table[];


// populates the esp suite.  Add all new test creation to this function
mut_testsuite_t * fbe_test_create_esp_test_suite(mut_function_t startup, mut_function_t teardown);

extern char * ratbert_short_desc;
extern char * ratbert_long_desc;
void ratbert_test(void);

extern char * dogbert_short_desc;
extern char * dogbert_long_desc;
void dogbert_test(void);

extern char * spiderman_short_desc;
extern char * spiderman_long_desc;
void spiderman_test(void);
void spiderman_setup(void);
void spiderman_destroy(void);

extern char * travis_bickle_short_desc;
extern char * travis_bickle_long_desc;
void travis_bickle_oberon_test(void);
void travis_bickle_hyperion_test(void);
void travis_bickle_oberon_setup(void);
void travis_bickle_hyperion_setup(void);
void travis_bickle_triton_test(void);
void travis_bickle_triton_setup(void);
void travis_bickle_destroy(void);

extern char * dilbert_short_desc;
extern char * dilbert_long_desc;
void dilbert_test(void);
void dilbert_setup(void);
void dilbert_destroy(void);

extern char * norma_desmond_short_desc;
extern char * norma_desmond_long_desc;
void norma_desmond_oberon_test(void);
void norma_desmond_oberon_setup(void);
void norma_desmond_hyperion_test(void);
void norma_desmond_hyperion_setup(void);
void norma_desmond_triton_test(void);
void norma_desmond_triton_setup(void);

extern char * cinderella_short_desc;
extern char * cinderella_long_desc;
void cinderella_test(void);
void cinderella_setup(void);
void cinderella_destroy(void);

extern char * fp1_ikkyusan_short_desc;
extern char * fp1_ikkyusan_long_desc;
void fp1_ikkyusan_test(void);
void fp1_ikkyusan_setup(void);
void fp1_ikkyusan_destroy(void);

extern char * vito_corleone_short_desc;
extern char * vito_corleone_long_desc;
void vito_corleone_test(void);
void vito_corleone_setup(void);
void vito_corleone_destroy(void);

extern char * master_cylinder_short_desc;
extern char * master_cylinder_long_desc;
void master_cylinder_test(void);
void master_cylinder_setup(void);

extern char * baloo_short_desc;
extern char * baloo_long_desc;
void baloo_test(void);
void baloo_setup(void);
void baloo_destroy(void);

extern char * calvin_short_desc;
extern char * calvin_long_desc;
void calvin_test(void);
void calvin_setup(void);
void calvin_destroy(void);

extern char * susie_derkins_short_desc;
extern char * susie_derkins_long_desc;
void susie_derkins_test(void);
void susie_derkins_setup(void);
void susie_derkins_destroy(void);

extern char * hobbes_short_desc;
extern char * hobbes_long_desc;
void hobbes_test(void);
void hobbes_setup(void);
void hobbes_destroy(void);

extern char * rosalyn_short_desc;
extern char * rosalyn_long_desc;
void rosalyn_test(void);
void rosalyn_setup(void);
void rosalyn_destroy(void);

extern char * dirty_harry_short_desc;
extern char * dirty_harry_long_desc;
void dirty_harry_test(void);
void dirty_harry_setup(void);

extern char * forrest_gump_short_desc;
extern char * forrest_gump_long_desc;
void forrest_gump_test(void);
void forrest_gump_setup(void);
void forrest_gump_destroy(void);

extern char * cool_hand_luke_short_desc;
extern char * cool_hand_luke_long_desc;
void cool_hand_luke_test(void);
void cool_hand_luke_setup(void);
void cool_hand_luke_destroy(void);

extern char * popeye_doyle_short_desc;
extern char * popeye_doyle_long_desc;
void popeye_doyle_megatron_test(void);
void popeye_doyle_megatron_setup(void);
void popeye_doyle_argonaut_test(void);
void popeye_doyle_argonaut_setup(void);
void popeye_doyle_argonaut_voyager_test(void);
void popeye_doyle_argonaut_voyager_setup(void);
void popeye_doyle_destroy(void);

extern char * sleeping_beauty_short_desc;
extern char * sleeping_beauty_long_desc;
void sleeping_beauty_test(void);
void sleeping_beauty_setup(void);
void sleeping_beauty_destroy(void);

extern char * vikram_and_bethaal_short_desc;
extern char * vikram_and_bethaal_long_desc;
void vikram_and_bethaal_test(void);
void vikram_and_bethaal_setup(void);
void vikram_and_bethaal_destroy(void);

extern char * presto_short_desc;
extern char * presto_long_desc;
void presto_test(void);
void presto_setup(void);
void presto_destroy(void);

extern char * fp2_sudsakorn_short_desc;
extern char * fp2_sudsakorn_long_desc;
void fp2_sudsakorn_test(void);
void fp2_sudsakorn_setup(void);
void fp2_sudsakorn_destroy(void);
void fbe_test_esp_common_destroy(void);
void fbe_test_esp_common_destroy_dual_sp(void);
void fbe_test_esp_common_destroy_all(void);
void fbe_test_esp_common_destroy_all_dual_sp(void);

extern char * fp3_rakka_short_desc;
extern char * fp3_rakka_long_desc;
void fp3_rakka_test(void);
void fp3_rakka_setup(void);
void fp3_rakka_destroy(void);

extern char * edward_and_bella_short_desc;
extern char * edward_and_bella_long_desc;
void edward_and_bella_test(void);
void edward_and_bella_setup(void);
void edward_and_bella_destroy(void);
void edward_and_bella_test_single_device_fup(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation);
void edward_and_bella_test_multiple_enclosure_fup(fbe_u64_t deviceType);

extern char * felix_the_cat_short_desc;
extern char * felix_the_cat_long_desc;
void felix_the_cat_test(void);
void felix_the_cat_setup(void);
void felix_the_cat_destroy(void);
fbe_status_t felix_the_cat_load_dae_count(void);

extern char * viking_simple_fup_short_desc;
extern char * viking_simple_fup_long_desc;
void viking_simple_fup_test(void);
void viking_simple_fup_setup(void);
void viking_simple_fup_destroy(void);

extern char * curious_george_short_desc;
extern char * curious_george_long_desc;
void curious_george_test(void);
void curious_george_setup(void);
void curious_george_destroy(void);

extern char * vavoom_short_desc;
extern char * vavoom_long_desc;
void vavoom_test(void);
void vavoom_setup(void);
void vavoom_destroy(void);

extern char * garnett_short_desc;
extern char * garnett_long_desc;
void garnett_test(void);
void garnett_setup(void);
void garnett_destroy(void);

extern char * paul_pierce_short_desc;
extern char * paul_pierce_long_desc;
void paul_pierce_test(void);
void paul_pierce_setup(void);
void paul_pierce_destroy(void);

extern char * poindexter_short_desc;
extern char * poindexter_long_desc;
void poindexter_test(void);
void poindexter_setup(void);
void poindexter_destroy(void);

extern char * edward_the_scissorhands_short_desc;
extern char * edward_the_scissorhands_long_desc;
void edward_the_scissorhands_test(void);
void edward_the_scissorhands_setup(void);
void edward_the_scissorhands_destroy(void);

extern char * grandma_wolf_short_desc;
extern char * grandma_wolf_long_desc;
void grandma_wolf_test(void);
void grandma_wolf_setup(void);
void grandma_wolf_destroy(void);

extern char * grandma_robot_short_desc;
extern char * grandma_robot_long_desc;
void grandma_robot_test(void);
void grandma_robot_setup(void);
void grandma_robot_destroy(void);

extern char * marcus_short_desc;
extern char * marcus_long_desc;
void marcus_oberon_test(void);
void marcus_hyperion_test(void);
void marcus_triton_test(void);
void marcus_oberon_setup(void);
void marcus_hyperion_setup(void);
void marcus_triton_setup(void);
void marcus_destroy(void);

extern char * tintin_short_desc;
extern char * tintin_long_desc;
void tintin_test(void);
void tintin_setup(void);
void tintin_destroy(void);

extern char * skeletor_short_desc;
extern char * skeletor_long_desc;
void skeletor_jetfire_m4_test(void);
void skeletor_jetfire_m4_setup(void);
void skeletor_destroy(void);

extern char * fp6_nemu_short_desc;
extern char * fp6_nemu_long_desc;
void fp6_nemu_test(void);
void fp6_nemu_setup(void);
void fp6_nemu_destroy(void);

extern char * chanakya_short_desc;
extern char * chanakya_long_desc;
void chanakya_test(void);
void chanakya_setup(void);
void chanakya_destroy(void);

extern char * fp5_kuu_short_desc;
extern char * fp5_kuu_long_desc;
void fp5_kuu_test(void);
void fp5_kuu_setup(void);
void fp5_kuu_destroy(void);

extern char * fp4_reki_short_desc;
extern char * fp4_reki_long_desc;
void fp4_reki_test(void);
void fp4_reki_setup(void);
void fp4_reki_destroy(void);


extern char * fp7_venger_short_desc;
extern char * fp7_venger_long_desc;
void fp7_venger_test(void);
void fp7_venger_setup(void);
void fp7_venger_destroy(void);

extern char * fp7_venger_test2_short_desc;
extern char * fp7_venger_test2_long_desc;
void fp7_venger_test2(void);
void fp7_venger_test2_setup(void);

void fp7_venger_test3(void);
void fp7_venger_test3_setup(void);
extern char * fp7_venger_test3_short_desc;
extern char * fp7_venger_test3_long_desc;

extern char * fp8_twinkle_twinkle_short_desc;
extern char * fp8_twinkle_twinkle_long_desc;
void fp8_twinkle_twinkle_test(void);
void fp8_twinkle_twinkle_setup(void);
void fp8_twinkle_twinkle_destroy(void);

extern char * fp9_fflewddur_fflam_short_desc;
extern char * fp9_fflewddur_fflam_long_desc;
void fp9_fflewddur_fflam_test(void);
void fp9_fflewddur_fflam_setup(void);
void fp9_fflewddur_fflam_destroy(void);

extern char * fp10_funshine_bear_short_desc;
extern char * fp10_funshine_bear_long_desc;
void fp10_funshine_bear_test(void);
void fp10_funshine_bear_setup(void);
void fp10_funshine_bear_destroy(void);

extern char * fp11_jorgen_von_strangle_short_desc;
extern char * fp11_jorgen_von_strangle_long_desc;
void fp11_jorgen_von_strangle_test(void);
void fp11_jorgen_von_strangle_setup(void);
void fp11_jorgen_von_strangle_destroy(void);

extern char * fp12_snarf_short_desc;
extern char * fp12_snarf_long_desc;
void fp12_snarf_test(void);
void fp12_snarf_setup(void);
void fp12_snarf_destroy(void);

extern char * fp13_orko_short_desc;
extern char * fp13_orko_long_desc;
void fp13_orko_test(void);
void fp13_orko_setup(void);
void fp13_orko_destroy(void);

extern char * fp14_tick_tock_short_desc;
extern char * fp14_tick_tock_long_desc;
void fp14_tick_tock_test(void);
void fp14_tick_tock_setup(void);
void fp14_tick_tock_destroy(void);

extern char * fp15_mumra_short_desc;
extern char * fp15_mumra_long_desc;
void fp15_mumra_test(void);
void fp15_mumra_setup(void);
void fp15_mumra_destroy(void);

extern char * fp16_jackalman_short_desc;
extern char * fp16_jackalman_long_desc;
void fp16_jackalman_test(void);
void fp16_jackalman_setup(void);
void fp16_jackalman_destroy(void);


extern char * tinkle_short_desc;
extern char * tinkle_long_desc;
void tinkle_test(void);
void tinkle_setup(void);
void tinkle_destroy(void);


extern char * sir_topham_hatt_short_desc;
extern char * sir_topham_hatt_long_desc;
void sir_topham_hatt_test(void);
void sir_topham_hatt_setup(void);
void sir_topham_hatt_destroy(void);

extern char * thomas_the_tank_engine_short_desc;
extern char * thomas_the_tank_engine_long_desc;
void thomas_the_tank_engine_test(void);


extern char * vayu_short_desc;
extern char * vayu_long_desc;
void vayu_test(void);
void vayu_setup(void);
void vayu_destroy(void);

extern char * pandavas_short_desc;
extern char * pandavas_long_desc;
void pandavas_oberon_test(void);
void pandavas_oberon_setup(void);
void pandavas_hyperion_test(void);
void pandavas_hyperion_setup(void);
void pandavas_triton_test(void);
void pandavas_triton_setup(void);
void pandavas_destroy(void);

extern char * cody_jarrett_short_desc;
extern char * cody_jarrett_long_desc;
void cody_jarrett_sentry_test(void);
void cody_jarrett_sentry_setup(void);
void cody_jarrett_argonaut_test(void);
void cody_jarrett_argonaut_setup(void);
void cody_jarrett_destroy(void);

extern char * ravana_fault_reg_megatron_short_desc;
extern char * ravana_fault_reg_jetfire_short_desc;
extern char * ravana_fault_reg_long_desc;
void ravana_fault_reg_mt_test(void);
void ravana_fault_reg_jf_test(void);
void ravana_fault_reg_megatron_setup(void);
void ravana_fault_reg_jetfire_setup(void);
void ravana_fault_reg_destroy(void);

extern char * ravana_ext_peer_boot_megatron_short_desc;
extern char * ravana_ext_peer_boot_long_desc;
void ravana_ext_peer_boot_test(void);
void ravana_ext_peer_boot_megatron_setup(void);
void ravana_ext_peer_boot_destroy(void);

extern char * tenali_raman_short_desc;
extern char * tenali_raman_long_desc;
void tenali_raman_test(void);
void tenali_raman_setup(void);
void tenali_raman_destroy(void);

extern char * vibhishana_short_desc;
extern char * vibhishana_long_desc;
void vibhishana_test(void);
void vibhishana_setup(void);
void vibhishana_destroy(void);

extern char * drive_remove_insert_short_desc;
extern char * drive_remove_insert_long_desc;
void drive_remove_insert_test(void);
void drive_remove_insert_test_setup(void);
void drive_remove_insert_test_destroy(void);

extern char * bigwig_short_desc;
extern char * bigwig_long_desc;
void bigwig_test(void);
void bigwig_setup(void);
void bigwig_destroy(void);

extern char * dae_counts_test_short_desc;
extern char * dae_counts_test_long_desc;
void dae_counts_test(void);
void dae_counts_test_setup(void);
void dae_counts_test_destroy(void);

void eva_01_derringer_test(void);
void eva_01_voyager_test(void);
void eva_01_tabasco_test(void);
void eva_01_derringer_test_setup(void);
void eva_01_voyager_test_setup(void);
void eva_01_tabasco_test_setup(void);
void eva_01_test_destroy(void);
extern char * eva_01_short_desc;
extern char * eva_01_long_desc;

extern char * randle_mcmurphy_short_desc;
extern char * randle_mcmurphy_long_desc;
void randle_mcmurphy_test(void);
void randle_mcmurphy_setup(void);
void randle_mcmurphy_destroy(void);

extern char * elmer_gantry_short_desc;
extern char * elmer_gantry_long_desc;
void elmer_gantry_test(void);
void elmer_gantry_setup(void);
void elmer_gantry_destroy(void);
void elmer_gantry_oberon_test(void);
void elmer_gantry_oberon_setup(void);

extern char * liberty_valance_short_desc;
extern char * liberty_valance_long_desc;
void liberty_valance_viper_test(void);
void liberty_valance_viper_setup(void);
void liberty_valance_voyager_test(void);
void liberty_valance_voyager_setup(void);
void liberty_valance_destroy(void);

extern char * achilles_short_desc;
extern char * achilles_long_desc;
void achilles_test(void);
void achilles_setup(void);
void achilles_destroy(void);

extern char * percy_short_desc;
extern char * percy_long_desc;
void percy_test(void);
void percy_setup(void);
void percy_destroy(void);

extern char * figment_short_desc;
extern char * figment_long_desc;
void figment_test(void);
void figment_setup(void);
void figment_destroy(void);

extern char * po_short_desc;
extern char * po_long_desc;
void po_test(void);
void po_setup(void);
void po_destroy(void);

extern char * captain_queeg_short_desc;
extern char * captain_queeg_long_desc;
void captain_queeg_megatron_test(void);
void captain_queeg_megatron_setup(void);
void captain_queeg_jetfire_test(void);
void captain_queeg_jetfire_setup(void);
void captain_queeg_destroy(void);


extern char * anduin_lothar_short_desc;
extern char * anduin_lothar_long_desc;
void anduin_lothar_test(void);
void anduin_lothar_test_setup(void);
void anduin_lothar_test_destroy(void);

extern char * guti_short_desc;
extern char * guti_long_desc;
void guti_jetfire_test(void);
void guti_jetfire_setup(void);
void guti_megatron_test(void);
void guti_megatron_setup(void);
void guti_destroy(void);

extern char * alucard_short_desc;
extern char * alucard_long_desc;
void alucard_test(void);
void alucard_test_setup(void);
void alucard_test_destroy(void);
extern char * dracula_short_desc;
extern char * dracula_long_desc;
void dracula_test_part1(void);
void dracula_test_part2(void);
void dracula_test_part3(void);
void dracula_test_part4(void);

extern char * bamboo_bmc_short_desc;
extern char * bamboo_bmc_long_desc;
void bamboo_bmc_jetfire_test(void);
void bamboo_bmc_jetfire_setup(void);
void bamboo_bmc_megatron_test(void);
void bamboo_bmc_megatron_setup(void);
void bamboo_bmc_destroy(void);

extern char * component_accessible_short_desc;
extern char * component_accessible_long_desc;
void component_accessible_test(void);
void component_accessible_setup(void);
void component_accessible_destroy(void);

extern char * tarzan_short_desc;
extern char * tarzan_long_desc;
void tarzan_test(void);
void tarzan_setup(void);
void tarzan_destroy(void);

extern char * birbal_short_desc;
extern char * birbal_long_desc;
void birbal_test(void);
void birbal_setup(void);
void birbal_destroy(void);

extern char * colossus_short_desc;
extern char * colossus_long_desc;
void colossus_test2(void);
void colossus_load_esp(void);
void colossus_cleanup(void);

extern char * beast_man_short_desc;
extern char * beast_man_long_desc;
void beast_man_test(void);
void beast_man_test_load_test_env(void);
void beast_man_destroy(void);

extern char * dumbo_short_desc;
extern char * dumbo_long_desc;
void dumbo_test(void);
void dumbo_setup(void);
void dumbo_destroy(void);

extern char * johnny_tractor_short_desc;
extern char * johnny_tractor_long_desc;
void johnny_tractor_test(void);
void johnny_tractor_setup(void);
void johnny_tractor_destroy(void);

extern char * allie_gator_short_desc;
extern char * allie_gator_long_desc;
void allie_gator_test(void);
void allie_gator_setup(void);
void allie_gator_destroy(void);

extern char * corey_combine_short_desc;
extern char * corey_combine_long_desc;
void corey_combine_test(void);
void corey_combine_setup(void);
void corey_combine_destroy(void);

extern char * wall_e_short_desc;
extern char * wall_e_long_desc;
void wall_e_test(void);
void wall_e_setup(void);
void wall_e_destroy(void);

extern char * peter_rabbit_short_desc;
extern char * peter_rabbit_long_desc;
void peter_rabbit_test(void);
void peter_rabbit_setup(void);
void peter_rabbit_destroy(void);

extern char * cdes2_ps_fup_short_desc;
extern char * cdes2_ps_fup_long_desc;
void cdes2_ps_fup_test(void);
void cdes2_ps_fup_setup(void);
void cdes2_ps_fup_destroy(void);

#endif /* ESP_TESTS_H */
