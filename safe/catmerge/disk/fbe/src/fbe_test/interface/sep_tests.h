#ifndef SEP_TESTS_H
#define SEP_TESTS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    sep_tests.h
 ***************************************************************************
 *
 * @brief   This file contains the function prototypes for the FBE test suites
 *          for both simulation and hardware tests.  New tests should be added
 *          here accordingly.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_neit.h"
#include "sep_utils.h"
#include "kms_dll.h"

extern fbe_service_control_entry_t sep_control_entry;
extern fbe_io_entry_function_t sep_io_entry;

extern fbe_service_control_entry_t physical_package_control_entry;
extern fbe_io_entry_function_t physical_package_io_entry;

#define SEP_EXPECTED_ERROR 0
void sep_config_01(void);
void sep_config_load(const fbe_test_packages_t *load_packages);
void sep_config_load_esp_sep_and_neit(void);
void sep_config_load_sep_and_neit(void);
void sep_config_load_sep_and_neit_no_esp(void);
void sep_config_load_neit(void);
void sep_config_load_sep(void);
void sep_config_load_sep_with_time_save(fbe_bool_t dualsp);
void sep_config_load_package_params(fbe_sep_package_load_params_t *sep_params_p,
                                    fbe_neit_package_load_params_t *neit_params_p, 
                                    const fbe_test_packages_t *load_packages);
void sep_config_load_sep_and_neit_params(fbe_sep_package_load_params_t *sep_params_p,
                                         fbe_neit_package_load_params_t *neit_params_p);
void sep_config_load_esp_sep_and_neit_params(fbe_sep_package_load_params_t *sep_params_p,
                                         fbe_neit_package_load_params_t *neit_params_p);
void sep_config_fill_load_params(fbe_sep_package_load_params_t *sep_params_p);
void neit_config_fill_load_params(fbe_neit_package_load_params_t *neit_params_p);

void sep_config_load_sep_and_neit_both_sps(void);
void sep_config_load_esp_sep_and_neit_both_sps(void);
void sep_config_load_packages_both_sps(const fbe_test_packages_t *load_packages, fbe_bool_t load_balance);
void sep_config_load_sep_and_neit_load_balance_both_sps(void);
void sep_config_load_sep_and_neit_load_balance_both_sps_with_sep_params(fbe_sep_package_load_params_t *spa_sep_params_p, 
                                                                        fbe_sep_package_load_params_t *spb_sep_params_p);
void sep_config_load_esp_sep_and_neit_load_balance_both_sps(void);
void sep_config_load_balance_load_sep(void);
void sep_config_load_balance_load_sep_with_time_save(fbe_bool_t dualsp);
void sep_config_load_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, 
                              fbe_sep_package_load_params_t *sep_params, const fbe_test_packages_t *load_packages);
void sep_config_load_esp_and_sep_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, fbe_sep_package_load_params_t *sep_params);
void sep_config_load_esp_and_sep_packages_with_sep_params(fbe_packet_t *packet, fbe_semaphore_t *sem, fbe_sep_package_load_params_t *sep_params);
void sep_config_load_sep_packages(fbe_packet_t *packet, fbe_semaphore_t *sem, fbe_sep_package_load_params_t *sep_params);
void sep_config_load_neit_package(void);
void sep_config_load_setup_package_entries(const fbe_test_packages_t *set_packages);
void sep_config_load_esp_sep_and_neit_setup_package_entries(void);
void sep_config_load_sep_and_neit_setup_package_entries(void);


void sep_config_load_kms(fbe_kms_package_load_params_t *kms_params_p);
void sep_config_load_kms_both_sps(fbe_kms_package_load_params_t *kms_params_p);

/******************************
 *  FUNCTION PROTOTYPES
 ******************************/

/* Populates the sep suite */
mut_testsuite_t * fbe_test_create_sep_configuration_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_copy_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_normal_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_power_savings_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_rebuild_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_services_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_sniff_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_sparing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_special_ops_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_encryption_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_verify_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_zeroing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_extent_pool_test_suite(mut_function_t startup, mut_function_t teardown);

/* Populates the dual SP test suite */
mut_testsuite_t * fbe_test_create_sep_dualsp_configuration_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_copy_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_normal_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_power_savings_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_rebuild_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_services_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_sniff_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_sparing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_special_ops_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_encryption_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_verify_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_zeroing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * fbe_test_create_sep_dualsp_extent_pool_test_suite(mut_function_t startup, mut_function_t teardown);

/* Populates the hw test suite */
mut_testsuite_t * hw_fbe_test_create_sep_configuration_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_normal_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_power_savings_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_rebuild_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_services_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_sniff_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_sparing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_special_ops_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_verify_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_zeroing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_performance_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_configuration_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_degraded_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_disk_errors_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_normal_io_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_power_savings_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_rebuild_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_services_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_sniff_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_sparing_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_special_ops_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_verify_test_suite(mut_function_t startup, mut_function_t teardown);
mut_testsuite_t * hw_fbe_test_create_sep_dualsp_zeroing_test_suite(mut_function_t startup, mut_function_t teardown);

/* Actual tests */
void simple_init_test(void);
void simple_write_test(void);

extern char * pinga_drive_validation_short_desc;
extern char * pinga_drive_validation_long_desc;
void pinga_drive_validation_test(void);
void pinga_drive_validation_setup(void);
void pinga_drive_validation_cleanup(void);

extern char * plankton_short_desc;
extern char * plankton_long_desc;
void plankton_test(void);
void plankton_test_setup(void);
void plankton_test_cleanup(void);

extern char * patrick_star_short_desc;
extern char * patrick_star_long_desc;
void patrick_star_test(void);
void patrick_star_test_setup(void);
void patrick_star_test_cleanup(void);

extern char * squidward_short_desc;
extern char * squidward_long_desc;
void squidward_setup(void);
void squidward_test(void);
void squidward_cleanup(void);

extern char * super_hv_short_desc;
extern char * super_hv_long_desc;
void super_hv_setup(void);
void super_hv_test(void);
void super_hv_cleanup(void);
/* Dual SP */
void super_hv_dualsp_test(void);
void super_hv_dualsp_setup(void);
void super_hv_dualsp_cleanup(void);
fbe_status_t fbe_test_physical_package_tests_config_unload(void);

extern char * elmo_short_desc;
extern char * elmo_long_desc;
void elmo_test(void);
void elmo_setup(void);
void elmo_block_opcode_test(void);
void elmo_cleanup(void);
void elmo_dualsp_test(void);
void elmo_dualsp_setup(void);
void elmo_dualsp_cleanup(void);

extern char * zoe_short_desc;
extern char * zoe_long_desc;
void zoe_test(void);
void zoe_block_opcode_test(void);
void zoe_setup(void);
void zoe_cleanup(void);
void zoe_dualsp_test(void);
void zoe_dualsp_setup(void);
void zoe_dualsp_cleanup(void);

extern char * grover_short_desc;
extern char * grover_long_desc;
void grover_test(void);
void grover_block_opcode_test(void);
void grover_setup(void);
void grover_cleanup(void);
void grover_dualsp_test(void);
void grover_dualsp_setup(void);
void grover_dualsp_cleanup(void);

extern char * snuffy_short_desc;
extern char * snuffy_long_desc;
void snuffy_test(void);
void snuffy_block_opcode_test(void);
void snuffy_setup(void);
void snuffy_cleanup(void);
void snuffy_dualsp_test(void);
void snuffy_dualsp_setup(void);
void snuffy_dualsp_cleanup(void);

extern char * rick_short_desc;
extern char * rick_long_desc;
void rick_test(void);
void rick_setup(void);
void rick_cleanup(void);

extern char * telly_short_desc;
extern char * telly_long_desc;
void telly_test(void);
void telly_block_opcode_test(void);
void telly_setup(void);
void telly_cleanup(void);
void telly_dualsp_test(void);
void telly_dualsp_setup(void);
void telly_dualsp_cleanup(void);

extern char * prairie_dawn_short_desc;
extern char * prairie_dawn_long_desc;
void prairie_dawn_test(void);
void prairie_dawn_block_opcode_test(void);
void prairie_dawn_setup(void);
void prairie_dawn_cleanup(void);
void prairie_dawn_dualsp_test(void);
void prairie_dawn_dualsp_setup(void);
void prairie_dawn_dualsp_cleanup(void);

extern char * ultra_hd_short_desc;
extern char * ultra_hd_long_desc;
void ultra_hd_test(void);
void ultra_hd_setup(void);
void ultra_hd_cleanup(void);
void ultra_hd_dualsp_test(void);
void ultra_hd_dualsp_setup(void);
void ultra_hd_dualsp_cleanup(void);

extern char * meriadoc_brandybuck_short_desc;
extern char * meriadoc_brandybuck_long_desc;
void meriadoc_brandybuck_test(void);
void meriadoc_brandybuck_setup(void);
void meriadoc_brandybuck_cleanup(void);

extern char * daffy_duck_short_desc;
extern char * daffy_duck_long_desc;
void daffy_duck_test(void);
void daffy_duck_setup(void);
void daffy_duck_cleanup(void);

extern char * big_bird_short_desc;
extern char * big_bird_long_desc;
void big_bird_single_degraded_zero_abort_test(void);
void big_bird_single_degraded_test(void);
void big_bird_shutdown_test(void);
void big_bird_setup(void);
void big_bird_cleanup(void);
void big_bird_dualsp_single_degraded_test(void);
void big_bird_dualsp_single_degraded_zero_abort_test(void);
void big_bird_dualsp_shutdown_test(void);
void big_bird_dualsp_setup(void);
void big_bird_dualsp_cleanup(void);

void big_bird_single_degraded_encryption_test(void);
void big_bird_encryption_setup(void);
void big_bird_dualsp_encryption_setup(void);
void big_bird_dualsp_single_degraded_encryption_test(void);

extern char * oscar_short_desc;
extern char * oscar_long_desc;
void oscar_degraded_test(void);
void oscar_degraded_zero_abort_test(void);
void oscar_shutdown_test(void);
void oscar_setup(void);
void oscar_cleanup(void);
void oscar_dualsp_degraded_test(void);
void oscar_dualsp_degraded_zero_abort_test(void);
void oscar_dualsp_shutdown_test(void);
void oscar_dualsp_setup(void);
void oscar_dualsp_cleanup(void);


extern char * bert_short_desc;
extern char * bert_long_desc;
void bert_single_degraded_zero_abort_test(void);
void bert_single_degraded_test(void);
void bert_shutdown_test(void);
void bert_setup(void);
void bert_cleanup(void);
void bert_dualsp_single_degraded_test(void);
void bert_dualsp_single_degraded_zero_abort_test(void);
void bert_dualsp_shutdown_test(void);
void bert_dualsp_setup(void);
void bert_dualsp_cleanup(void);

void bert_encryption_setup(void);
void bert_single_degraded_encryption_test(void);
void bert_dualsp_encryption_setup(void);
void bert_dualsp_single_degraded_encryption_test(void);

extern char * ernie_short_desc;
extern char * ernie_long_desc;
void ernie_single_degraded_zero_abort_test(void);
void ernie_single_degraded_test(void);
void ernie_double_degraded_zero_abort_test(void);
void ernie_double_degraded_test(void);
void ernie_shutdown_test(void);
void ernie_setup_520(void);
void ernie_simultaneous_insert_and_remove_test(void);
void ernie_setup(void);
void ernie_cleanup(void);
void ernie_dualsp_single_degraded_test(void);
void ernie_dualsp_single_degraded_zero_abort_test(void);
void ernie_dualsp_double_degraded_test(void);
void ernie_dualsp_double_degraded_zero_abort_test(void);
void ernie_dualsp_shutdown_test(void);
void ernie_dualsp_setup(void);
void ernie_dualsp_cleanup(void);

void ernie_encryption_setup(void);
void ernie_single_degraded_encryption_test(void);
void ernie_double_degraded_encryption_test(void);
void ernie_dualsp_encryption_setup(void);
void ernie_dualsp_single_degraded_encryption_test(void);
void ernie_dualsp_double_degraded_encryption_test(void);

extern char * rosita_short_desc;
extern char * rosita_long_desc;
void rosita_single_degraded_zero_abort_test(void);
void rosita_single_degraded_test(void);
void rosita_shutdown_test(void);
void rosita_setup(void);
void rosita_cleanup(void);
void rosita_dualsp_single_degraded_test(void);
void rosita_dualsp_single_degraded_zero_abort_test(void);
void rosita_dualsp_shutdown_test(void);
void rosita_dualsp_setup(void);
void rosita_dualsp_cleanup(void);

void rosita_encryption_setup(void);
void rosita_single_degraded_encryption_test(void);
void rosita_dualsp_encryption_setup(void);
void rosita_dualsp_single_degraded_encryption_test(void);

extern char * gimli_short_desc;
extern char * gimli_long_desc;
void gimli_setup(void);
void gimli_cleanup(void);
void gimli_dualsp_test(void);
void gimli_dualsp_setup(void);
void gimli_dualsp_cleanup(void);

extern char * frank_the_pug_short_desc;
extern char * frank_the_pug_long_desc;
void frank_the_pug_test(void);
void frank_the_pug_setup(void);
void frank_the_pug_cleanup(void);
void frank_the_pug_dualsp_test(void);
void frank_the_pug_dualsp_setup(void);
void frank_the_pug_dualsp_cleanup(void);

extern char * handy_manny_short_desc;
extern char * handy_manny_long_desc;
void handy_manny_test(void);
void handy_manny_setup(void);
void handy_manny_cleanup(void);

extern char * lefty_short_desc;
extern char * lefty_long_desc;
void lefty_raid5_test(void);
void lefty_raid3_test(void);
void lefty_raid6_test(void);
void lefty_raid1_test(void);
void lefty_raid0_test(void);
void lefty_raid10_test(void);
void lefty_setup(void);
void lefty_cleanup(void);

extern char * pippin_short_desc;
extern char * pippin_long_desc;
void pippin_test(void);
void pippin_setup(void);
void pippin_cleanup(void);
void pippin_dualsp_test(void);
void pippin_dualsp_setup(void);
void pippin_dualsp_cleanup(void);


extern char * squeeze_short_desc;
extern char * squeeze_long_desc;
void squeeze_raid1_test(void);
void squeeze_raid0_test(void);
void squeeze_raid10_test(void);
void squeeze_raid5_test(void);
void squeeze_raid3_test(void);
void squeeze_raid6_test(void);
void squeeze_raid1_setup(void);
void squeeze_raid0_setup(void);
void squeeze_raid10_setup(void);
void squeeze_raid5_setup(void);
void squeeze_raid3_setup(void);
void squeeze_raid6_setup(void);
void squeeze_cleanup(void);

extern char * stretch_short_desc;
extern char * stretch_long_desc;
void stretch_raid1_test(void);
void stretch_raid0_test(void);
void stretch_raid10_test(void);
void stretch_raid5_test(void);
void stretch_raid3_test(void);
void stretch_raid6_test(void);
void stretch_raid1_setup(void);
void stretch_raid0_setup(void);
void stretch_raid10_setup(void);
void stretch_raid5_setup(void);
void stretch_raid3_setup(void);
void stretch_raid6_setup(void);
void stretch_cleanup(void);

extern char * abby_cadabby_short_desc;
extern char * abby_cadabby_long_desc;
void abby_cadabby_raid1_test(void);
void abby_cadabby_raid10_test(void);
void abby_cadabby_raid5_test(void);
void abby_cadabby_raid3_test(void);
void abby_cadabby_raid6_test(void);
void abby_cadabby_raid1_setup(void);
void abby_cadabby_raid10_setup(void);
void abby_cadabby_raid6_setup(void);
void abby_cadabby_raid3_setup(void);
void abby_cadabby_raid5_setup(void);
void abby_cadabby_cleanup(void);
void abby_cadabby_dualsp_raid1_test(void);
void abby_cadabby_dualsp_raid10_test(void);
void abby_cadabby_dualsp_raid5_test(void);
void abby_cadabby_dualsp_raid3_test(void);
void abby_cadabby_dualsp_raid6_test(void);
void abby_cadabby_dualsp_raid1_setup(void);
void abby_cadabby_dualsp_raid10_setup(void);
void abby_cadabby_dualsp_raid6_setup(void);
void abby_cadabby_dualsp_raid3_setup(void);
void abby_cadabby_dualsp_raid5_setup(void);
void abby_cadabby_dualsp_cleanup(void);

extern char * strider_short_desc;
extern char * strider_long_desc;
void strider_test(void);
void strider_setup(void);
void strider_cleanup(void);

extern char * mumford_the_magician_test_short_desc;
extern char * mumford_the_magician_test_long_desc;
void mumford_the_magician_test(void);
void mumford_the_magician_setup(void);
void mumford_the_magician_cleanup(void);

extern char * guy_smiley_test_short_desc;
extern char * guy_smiley_test_long_desc;
void guy_smiley_test(void);
void guy_smiley_setup(void);
void guy_smiley_cleanup(void);

extern char * cookie_monster_short_desc;
extern char * cookie_monster_long_desc;
void cookie_monster_raid0_test(void);
void cookie_monster_raid1_test(void);
void cookie_monster_raid10_test(void);
void cookie_monster_raid5_test(void);
void cookie_monster_raid3_test(void);
void cookie_monster_raid6_test(void);
void cookie_monster_raid0_setup(void);
void cookie_monster_raid1_setup(void);
void cookie_monster_raid10_setup(void);
void cookie_monster_raid6_setup(void);
void cookie_monster_raid3_setup(void);
void cookie_monster_raid5_setup(void);
void cookie_monster_cleanup(void);
void cookie_monster_dualsp_raid0_test(void);
void cookie_monster_dualsp_raid1_test(void);
void cookie_monster_dualsp_raid10_test(void);
void cookie_monster_dualsp_raid5_test(void);
void cookie_monster_dualsp_raid3_test(void);
void cookie_monster_dualsp_raid6_test(void);
void cookie_monster_dualsp_raid0_setup(void);
void cookie_monster_dualsp_raid1_setup(void);
void cookie_monster_dualsp_raid10_setup(void);
void cookie_monster_dualsp_raid6_setup(void);
void cookie_monster_dualsp_raid3_setup(void);
void cookie_monster_dualsp_raid5_setup(void);
void cookie_monster_dualsp_cleanup(void);


extern char * turner_test_short_desc;
extern char * turner_test_long_desc;
void turner_test(void);
void turner_setup(void);
void turner_cleanup(void);


extern char * dusty_test_short_desc;
extern char * dusty_test_long_desc;
void dusty_test(void);
void dusty_setup(void);
void dusty_cleanup(void);

extern char * kermit_short_desc;
extern char * kermit_long_desc;
void kermit_raid0_test(void);
void kermit_raid1_test(void);
void kermit_raid3_test(void);
void kermit_raid5_test(void);
void kermit_raid6_test(void);
void kermit_raid10_test(void);
void kermit_setup(void);
void kermit_cleanup(void);

extern char * headless_horseman_short_desc;
extern char * headless_horseman_long_desc;
void headless_horseman_raid0_test(void);
void headless_horseman_raid1_test(void);
void headless_horseman_raid3_test(void);
void headless_horseman_raid5_test(void);
void headless_horseman_raid6_test(void);
void headless_horseman_raid10_test(void);
void headless_horseman_vault_test(void);
void headless_horseman_setup(void);
void headless_horseman_cleanup(void);
void headless_horseman_dualsp_raid0_test(void);
void headless_horseman_dualsp_raid1_test(void);
void headless_horseman_dualsp_raid3_test(void);
void headless_horseman_dualsp_raid5_test(void);
void headless_horseman_dualsp_raid6_test(void);
void headless_horseman_dualsp_raid10_test(void);
void headless_horseman_reboot_dualsp_raid0_test(void);
void headless_horseman_reboot_dualsp_raid1_test(void);
void headless_horseman_reboot_dualsp_raid3_test(void);
void headless_horseman_reboot_dualsp_raid5_test(void);
void headless_horseman_reboot_dualsp_raid6_test(void);
void headless_horseman_reboot_dualsp_raid10_test(void);
void headless_horseman_dualsp_setup(void);
void headless_horseman_dualsp_cleanup(void);
void headless_horseman_dualsp_large_rg_reboot_test(void);
void headless_horseman_dualsp_large_rg_setup(void);

void headless_horseman_dualsp_large_r10_reboot_test(void);
void headless_horseman_dualsp_large_r10_rg_setup(void);

extern char * sir_topham_hatt_short_desc;
extern char * sir_topham_hatt_long_desc;
void sir_topham_hatt_test(void);
void sir_topham_hatt_setup(void);
void sir_topham_hatt_destroy(void);

extern char * minion_short_desc;
extern char * minion_long_desc;
void minion_test(void);
void minion_reboot_test(void);
void minion_setup(void);
void minion_cleanup(void);

extern char * biff_short_desc;
extern char * biff_long_desc;
void biff_test(void);
void biff_setup(void);
void biff_cleanup(void);
void biff_dualsp_test(void);
void biff_dualsp_setup(void);
void biff_dualsp_cleanup(void);

extern char * kipper_short_desc;
extern char * kipper_long_desc;
void kipper_health_check_dualsp_test(void);
void kipper_dualsp_setup(void);
void kipper_dualsp_cleanup(void);

extern char * pingu_slot_limit_short_desc;
extern char * pingu_slot_limit_long_desc;
void pingu_slot_limit_dualsp_test(void);
void pingu_slot_limit_dualsp_setup(void);
void pingu_slot_limit_dualsp_cleanup(void);


extern char * ovejita_short_desc;
extern char * ovejita_long_desc;
void ovejita_singlesp_test(void);
void ovejita_test(void);
void ovejita_setup(void);
void ovejita_cleanup(void);

/* Dual SP */
void ovejita_dualsp_test(void);
void ovejita_dualsp_setup(void);
void ovejita_dualsp_cleanup(void);

extern char * betty_lou_short_desc;
extern char * betty_lou_long_desc;
void betty_lou_test(void);
void betty_lou_setup(void);
void betty_lou_cleanup(void);

extern char * sherlock_hemlock_short_desc;
extern char * sherlock_hemlock_long_desc;
void sherlock_hemlock_test(void);
void sherlock_hemlock_setup(void);
void sherlock_hemlock_cleanup(void);

extern char * clifford_crc_error_test_short_desc;
extern char * clifford_short_desc;
extern char * clifford_long_desc;
void clifford_test(void);
void clifford_crc_error_encrypted_test(void);
void clifford_crc_error_test(void);
void clifford_setup(void);
void clifford_encryption_setup(void);
void clifford_cleanup(void);

extern char * clifford_dualsp_short_desc;
extern char * clifford_dualsp_long_desc;
void clifford_dualsp_test(void);
void clifford_dualsp_setup(void);
void clifford_dualsp_cleanup(void);

extern char * cleo_short_desc;
extern char * cleo_long_desc;
void cleo_test(void);
void cleo_setup(void);
void cleo_cleanup(void);

extern char * hamburger_short_desc;
extern char * hamburger_long_desc;
void hamburger_test(void);
void hamburger_setup(void);
void hamburger_cleanup(void);

extern char * nearly_headless_nick_short_desc;
extern char * nearly_headless_nick_long_desc;
void nearly_headless_nick_setup(void);
void nearly_headless_nick_dualsp_setup(void);
void nearly_headless_nick_test(void);
void nearly_headless_nick_dualsp_test(void);
void nearly_headless_nick_cleanup(void);
void nearly_headless_nick_dualsp_cleanup(void);

extern char * doctor_girlfriend_short_desc;
extern char * doctor_girlfriend_long_desc;
void doctor_girlfriend_test(void);
void doctor_girlfriend_setup(void);
void doctor_girlfriend_cleanup(void);

extern char * hamburger_dualsp_short_desc;
extern char * hamburger_dualsp_long_desc;
void hamburger_dualsp_test(void);
void hamburger_dualsp_setup(void);
void hamburger_dualsp_cleanup(void);

extern char * kilgore_trout_short_desc;
extern char * kilgore_trout_long_desc;
void kilgore_trout_test(void);
void kilgore_trout_config_init(void);
void kilgore_trout_config_destroy(void);
void kilgore_trout_cleanup(void);
void kilgore_trout_setup(void);

extern char * velma_short_desc;
extern char * velma_long_desc;
void velma_test(void);
void velma_setup(void);
void velma_cleanup(void);

extern char * freddie_short_desc;
extern char * freddie_long_desc;
void freddie_test(void);
void freddie_setup(void);

void scoobert_test(void);
extern char * scoobert_short_desc;
extern char * scoobert_long_desc;
void scoobert_setup(void);
void scoobert_cleanup(void);

void scoobert_4k_test(void);
extern char * scoobert_short_desc;
extern char * scoobert_long_desc;
void scoobert_4k_setup(void);
void scoobert_4k_cleanup(void);


extern char * yabbadoo_short_desc;
extern char * yabbadoo_long_desc;
void yabbadoo_setup(void);
void yabbadoo_test(void);
void yabbadoo_cleanup(void);

void bvd_interface_load_test(void);
void fbe_api_setup_test(void);

void shaggy_test(void);
extern char * shaggy_short_desc;
extern char * shaggy_long_desc;
void shaggy_setup(void);
void shaggy_cleanup(void);
/* Dual SP */
void shaggy_dualsp_test(void);
void shaggy_dualsp_setup(void);
void shaggy_dualsp_cleanup(void);

extern char * mougli_short_desc;
extern char * mougli_long_desc;
void mougli_setup(void);
void mougli_cleanup(void);
/* Dual SP */
void mougli_dualsp_test(void);
void mougli_dualsp_setup(void);
void mougli_dualsp_cleanup(void);

void freddie_test(void);
extern char * freddie_short_desc;
extern char * freddie_long_desc;
void freddie_setup(void);
void freddie_cleanup(void);
/* Dual SP */
void freddie_dualsp_test(void);
void freddie_dualsp_setup(void);
void freddie_dualsp_cleanup(void);

void diabolicaldiscdemon_test(void);
void diabolicaldiscdemon_encryption_test(void);
extern char * diabolicaldiscdemon_short_desc;
extern char * diabolicaldiscdemon_long_desc;
void diabolicaldiscdemon_setup(void);
void diabolicaldiscdemon_cleanup(void);
void diabolicaldiscdemon_encrypted_setup(void);
void diabolicaldiscdemon_encrypted_cleanup(void);
/* Dual SP */
void diabolicaldiscdemon_encryption_dualsp_test(void);
void diabolicaldiscdemon_timeout_errors_dualsp_test(void);
void diabolicaldiscdemon_dualsp_test(void);
void diabolicaldiscdemon_dualsp_setup(void);
void diabolicaldiscdemon_dualsp_cleanup(void);
void diabolicaldiscdemon_encrypted_dualsp_setup(void);
void diabolicaldiscdemon_encrypted_dualsp_cleanup(void);

void dooby_doo_test(void);
extern char * dooby_doo_short_desc;
extern char * dooby_doo_long_desc;
void dooby_doo_setup(void);
void dooby_doo_cleanup(void);
/* Dual SP */
void dooby_doo_dualsp_test(void);
void dooby_doo_dualsp_setup(void);
void dooby_doo_dualsp_cleanup(void);

void scooby_doo_test(void);
extern char * scooby_doo_short_desc;
extern char * scooby_doo_long_desc;
void scooby_doo_setup(void);
void scooby_doo_cleanup(void);
/* Dual SP */
void scooby_doo_dualsp_test(void);
void scooby_doo_dualsp_setup(void);
void scooby_doo_dualsp_cleanup(void);

void red_herring_raid1_test(void);
void red_herring_raid3_test(void);
void red_herring_raid5_test(void);
void red_herring_raid6_test(void);
void red_herring_raid10_test(void);
extern char * red_herring_short_desc;
extern char * red_herring_long_desc;
void red_herring_raid1_setup(void);
void red_herring_raid3_setup(void);
void red_herring_raid5_setup(void);
void red_herring_raid6_setup(void);
void red_herring_raid10_setup(void);
void red_herring_cleanup(void);
/* Dual SP */
void red_herring_dualsp_raid1_test(void);
void red_herring_dualsp_raid3_test(void);
void red_herring_dualsp_raid5_test(void);
void red_herring_dualsp_raid6_test(void);
void red_herring_dualsp_raid10_test(void);
void red_herring_dualsp_raid1_setup(void);
void red_herring_dualsp_raid3_setup(void);
void red_herring_dualsp_raid5_setup(void);
void red_herring_dualsp_raid6_setup(void);
void red_herring_dualsp_raid10_setup(void);
void red_herring_dualsp_cleanup(void);

void flim_flam_test(void);
extern char * flim_flam_short_desc;
extern char * flim_flam_long_desc;
void flim_flam_setup(void);
void flim_flam_cleanup(void);
/* Dual SP */
void flim_flam_dualsp_test(void);
void flim_flam_dualsp_setup(void);
void flim_flam_dualsp_cleanup(void);

extern char * vincent_van_ghoul_short_desc;
extern char * vincent_van_ghoul_long_desc;
void vincent_van_ghoul_test(void);
void vincent_van_ghoul_setup(void);
void vincent_van_ghoul_cleanup(void);
void vincent_van_ghoul_encrypted_test(void);
void vincent_van_ghoul_encrypted_setup(void);
void vincent_van_ghoul_encrypted_cleanup(void);
/* Dual SP */
void vincent_van_ghoul_dualsp_test(void);
void vincent_van_ghoul_dualsp_setup(void);
void vincent_van_ghoul_dualsp_cleanup(void);
void vincent_van_ghoul_dualsp_encrypted_test(void);
void vincent_van_ghoul_dualsp_encrypted_setup(void);
void vincent_van_ghoul_dualsp_encrypted_cleanup(void);

extern char * homer_short_desc;
extern char * homer_long_desc;
void homer_test(void);
void homer_setup(void);
void homer_cleanup(void);

extern char * skippy_doo_short_desc;
extern char * skippy_doo_long_desc;
void skippy_doo_test(void);
void skippy_doo_setup(void);
void skippy_doo_cleanup(void);

extern char * howdy_doo_short_desc;
extern char * howdy_doo_long_desc;
void howdy_doo_test(void);
void howdy_doo_setup(void);
void howdy_doo_cleanup(void);

extern char * daphne_short_desc;
extern char * daphne_long_desc;
void daphne_test(void);
void daphne_setup(void);
void daphne_cleanup(void);

extern char * ugly_duckling_short_desc;
extern char * ugly_duckling_long_desc;
void ugly_duckling_test(void);
void ugly_duckling_setup(void);
void ugly_duckling_cleanup(void);

extern char * doremi_short_desc;
extern char * doremi_long_desc;
void doremi_test(void);
void doremi_setup(void);
void doremi_cleanup(void);

extern char * frodo_baggins_short_desc;
extern char * frodo_baggins_long_desc;
void frodo_baggins_test(void);
void frodo_baggins_setup(void);
void frodo_baggins_cleanup(void);

extern char * bilbo_baggins_short_desc;
extern char * bilbo_baggins_long_desc;
void bilbo_baggins_test(void);
void bilbo_baggins_setup(void);
void bilbo_baggins_cleanup(void);
void bilbo_baggins_dualsp_test(void);
void bilbo_baggins_dualsp_setup(void);
void bilbo_baggins_dualsp_cleanup(void);

extern char * samwise_gamgee_short_desc;
extern char * samwise_gamgee_long_desc;
void samwise_gamgee_test(void);
void samwise_gamgee_setup(void);
void samwise_gamgee_cleanup(void);
void samwise_gamgee_dualsp_test(void);
void samwise_gamgee_dualsp_setup(void);
void samwise_gamgee_dualsp_cleanup(void);

extern char * dixie_doo_short_desc;
extern char * dixie_doo_long_desc;
void dixie_doo_test(void);
void dixie_doo_setup(void);
void dixie_doo_cleanup(void);

extern char * bob_the_builder_short_desc;
extern char * bob_the_builder_long_desc;
void bob_the_builder_test_520(void);
void bob_the_builder_setup_520(void);
void bob_the_builder_test_4k(void);
void bob_the_builder_setup_4k(void);
void bob_the_builder_cleanup(void);

extern char * bob_the_builder_short_desc;
extern char * bob_the_builder_long_desc;
void bob_the_builder_encrypted_test(void);
void bob_the_builder_encrypted_setup(void);
void bob_the_builder_encrypted_cleanup(void);

extern char * scoop_short_desc;
extern char * scoop_long_desc;
void scoop_test(void);
void scoop_test_init(void);
void scoop_test_destroy(void);

extern char * aurora_short_desc;
extern char * aurora_long_desc;
void aurora_test(void);
void aurora_test_init(void);
void aurora_test_destroy(void);

extern char * dora_short_desc;
extern char * dora_long_desc;
void dora_test(void);
void dora_setup(void);
void dora_cleanup(void);
void dora_dualsp_test(void);
void dora_dualsp_setup(void);
void dora_dualsp_cleanup(void);

extern char * banio_short_desc;
extern char * banio_long_desc;
void banio_test(void);
void banio_setup(void);
void banio_cleanup(void);


extern char * tico_short_desc;
extern char * tico_long_desc;
void tico_test(void);
void tico_setup(void);
void tico_cleanup(void);
void tico_dualsp_test(void);
void tico_dualsp_setup(void);
void tico_dualsp_cleanup(void);

extern char * mimi_short_desc;
extern char * mimi_long_desc;
void mimi_dualsp_test(void);
void mimi_dualsp_setup(void);
void mimi_dualsp_cleanup(void);


extern char * momo_short_desc;
extern char * momo_long_desc;
void momo_test(void);
void momo_setup(void);
void momo_cleanup(void);



extern char * diego_short_desc;
extern char * diego_long_desc;
void diego_test(void);
void diego_setup(void);
void diego_cleanup(void);

extern char * boots_short_desc;
extern char * boots_long_desc;
void boots_test(void);
void boots_setup(void);
void boots_cleanup(void);

extern char * swiper_short_desc;
extern char * swiper_long_desc;
void swiper_test(void);
void swiper_setup(void);

extern char * pooh_short_desc;
extern char * pooh_long_desc;
void pooh_test(void);
void pooh_setup(void);
void pooh_cleanup(void);

extern char * rabbit_short_desc;
extern char * rabbit_long_desc;
void rabbit_test(void);
void rabbit_setup(void);

extern char * eeyore_short_desc;
extern char * eeyore_long_desc;
void eeyore_test(void);
void eeyore_setup(void);

extern char * woozle_short_desc;
extern char * woozle_long_desc;
void woozle_test(void);
void woozle_setup(void);
void woozle_destroy(void);

void weerd_test(void);
extern char * weerd_short_desc;
extern char * weerd_long_desc;
void weerd_setup(void);
void weerd_cleanup(void);
/* Dual SP */
void weerd_dualsp_test(void);
void weerd_dualsp_setup(void);
void weerd_dualsp_cleanup(void);

extern char * bogel_short_desc;
extern char * bogel_long_desc;
void bogel_test(void);
void bogel_setup(void);
/* Dual SP */
void bogel_dualsp_test(void);
void bogel_dualsp_setup(void);
void bogel_dualsp_cleanup(void);

extern char * amber_short_desc;
extern char * amber_long_desc;
void amber_test(void);
void amber_setup(void);
void amber_cleanup(void);

extern char * larry_t_lobster_short_desc;
extern char * larry_t_lobster_long_desc;
void larry_t_lobster_test(void);
void larry_t_lobster_test_setup(void);
void larry_t_lobster_test_cleanup(void);
/* Dual SP */
void larry_t_lobster_dualsp_test(void);
void larry_t_lobster_dualsp_setup(void);
void larry_t_lobster_dualsp_cleanup(void);

extern char * marina_short_desc;
extern char * marina_long_desc;
/* Dual SP */
void marina_dualsp_test(void);
void marina_dualsp_setup(void);
void marina_dualsp_cleanup(void);

extern char * bubble_bass_short_desc;
extern char * bubble_bass_long_desc;
void bubble_bass_test(void);
void bubble_bass_test_setup(void);
void bubble_bass_test_cleanup(void);

extern char * kalia_short_desc;
extern char * kalia_long_desc;
void kalia_dualsp_test(void);
void kalia_dualsp_setup(void);
void kalia_dualsp_cleanup(void);
void kalia_test(void);
void kalia_setup(void);
void kalia_cleanup(void);

extern char * kalia_kms_short_desc;
extern char * kalia_kms_long_desc;

extern char * ichabod_short_desc;
extern char * ichabod_long_desc;
void ichabod_crane_test(void);
void ichabod_crane_drive_removal_test(void);
void ichabod_crane_shutdown_test(void);
void ichabod_crane_drive_removal_paged_corrupt_test(void);
void ichabod_crane_check_pattern_test(void);
void ichabod_crane_read_only_test(void);
void ichabod_crane_degraded_test(void);
void ichabod_crane_zero_test(void);
void ichabod_crane_unconsumed_test(void);
void ichabod_crane_system_drive_test(void);
void ichabod_crane_system_drive_createrg_encrypt_reboot_test(void);
void ichabod_crane_rg_create_key_error_test(void);
void ichabod_crane_rg_create_system_drives_key_error_test(void);
void ichabod_crane_key_error_io_test(void);
void ichabod_crane_validate_key_error_test(void);
void ichabod_crane_validate_key_error_system_drive_test(void);
void ichabod_crane_incorrect_key_test(void);
void ichabod_crane_incorrect_key_one_drive_test(void);
void ichabod_setup(void);
void ichabod_drive_removal_setup(void);
void ichabod_shutdown_setup(void);
void ichabod_cleanup(void);
void ichabod_cleanup_with_errs(void);
void ichabod_crane_dualsp_test(void);
void ichabod_crane_dualsp_drive_removal_test(void);
void ichabod_crane_dualsp_zero_test(void);
void ichabod_crane_dualsp_unconsumed_test(void);
void ichabod_crane_dualsp_system_drive_test(void);
void ichabod_crane_dualsp_check_pattern_test(void);
void ichabod_crane_dualsp_read_only_test(void);
void ichabod_crane_dualsp_slf_test(void);
void ichabod_dualsp_setup(void);
void ichabod_drive_removal_dualsp_setup(void);
void ichabod_dualsp_cleanup(void);

extern char * mike_the_knight_short_desc;
extern char * mike_the_knight_long_desc;
void mike_the_knight_setup(void);
void mike_the_knight_cleanup(void);
void mike_the_knight_dualsp_setup(void);
void mike_the_knight_dualsp_reboot_setup(void);
void mike_the_knight_dualsp_cleanup(void);
void mike_the_knight_scrubbing_consumed_test(void);
void mike_the_knight_dualsp_scrubbing_consumed_test(void);
void mike_the_knight_normal_scrubbing_test(void);
void mike_the_knight_dualsp_normal_scrubbing_test(void);
void mike_the_knight_destroy_rg_before_encryption(void);
void mike_the_knight_dualsp_destroy_rg_before_encryption(void);
void mike_the_knight_scrubbing_garbage_collection_test(void);
void mike_the_knight_dualsp_scrubbing_garbage_collection_test(void);
void mike_the_knight_normal_scrubbing_drive_pull_test(void);
void mike_the_knight_dualsp_normal_scrubbing_drive_pull_test(void);
void mike_the_knight_dualsp_reboot_test(void);

extern char * sleepy_hollow_short_desc;
extern char * sleepy_hollow_long_desc;
void sleepy_hollow_test(void);
void sleepy_hollow_vault_delay_encryption_test(void);
void sleepy_hollow_vault_zero_test(void);
void sleepy_hollow_vault_in_use_test(void);
void sleepy_hollow_setup(void);
void sleepy_hollow_setup_sim_psl(void);
void sleepy_hollow_cleanup(void);
void sleepy_hollow_dualsp_test(void);
void sleepy_hollow_vault_in_use_dualsp_test(void);
void sleepy_hollow_delayed_encryption_dualsp_test(void);
void sleepy_hollow_vault_zero_dualsp_test(void);
void sleepy_hollow_dualsp_setup(void);
void sleepy_hollow_dualsp_setup_sim_psl(void);
void sleepy_hollow_dualsp_cleanup(void);

extern char * katrina_short_desc;
extern char * katrina_long_desc;
void katrina_test(void);
void katrina_setup(void);
void katrina_cleanup(void);
void katrina_dualsp_test(void);
void katrina_dualsp_setup(void);
void katrina_dualsp_cleanup(void);

//extern char * shaggy_encryption_short_desc;
//extern char * shaggy_encryption_long_desc;
void shaggy_encryption_dualsp_test(void);
void shaggy_encryption_dualsp_setup(void);
void shaggy_encryption_dualsp_cleanup(void);

extern char * sesame_open_short_desc;
extern char * sesame_open_long_desc;
void sesame_open_test(void);
void sesame_open_setup(void);
void sesame_open_cleanup(void);

extern char * evie_short_desc;
extern char * evie_long_desc;
void evie_test(void);
void evie_setup(void);
void evie_cleanup(void);
void evie_dualsp_test(void);
void evie_dualsp_setup(void);
void evie_dualsp_cleanup(void);
void evie_vault_encryption_in_progress_test(void);
void evie_drive_relocation_test(void);
void evie_drive_relocation_dualsp_test(void);
void evie_reboot_and_proactive_dualsp_test(void);
void evie_encryption_mode_test(void);
void evie_bind_unbind_test(void);

void evie_quiesce_test(void);
void evie_quiesce_test_setup(void);
void evie_quiesce_test_cleanup(void);

extern char * chitti_short_desc;
extern char * chitti_long_desc;
void chitti_test(void);
void chitti_setup(void);
void chitti_cleanup(void);
void chitti_dualsp_test(void);
void chitti_dualsp_setup(void);
void chitti_dualsp_cleanup(void);

extern char * peep_short_desc;
extern char * peep_long_desc;
void peep_test(void);
void peep_setup(void);
void peep_cleanup(void);
void peep_dualsp_test(void);
void peep_dualsp_setup(void);
void peep_dualsp_cleanup(void);

extern char * ruby_doo_short_desc;
extern char * ruby_doo_long_desc;
void ruby_doo_test(void);
void ruby_doo_setup(void);
void ruby_doo_test_cleanup(void);

extern char * danny_din_short_desc;
extern char * danny_din_long_desc;
void danny_din_singlesp_test(void);
void danny_din_test(void);
void danny_din_setup(void);
void danny_din_destroy(void);

/* Dual SP */
void danny_din_dualsp_test(void);
void danny_din_dualsp_setup(void);
void danny_din_dualsp_destroy(void);

extern char * rabo_karabekian_short_desc;
extern char * rabo_karabekian_long_desc;
void rabo_karabekian_test(void);
void rabo_karabekian_config_init(void);
void rabo_karabekian_config_destroy(void);

extern char * scooby_dum_short_desc;
extern char * scooby_dum_long_desc;
void scooby_dum_test(void);
void scooby_dum_setup(void);
void scooby_dum_cleanup(void);

extern char * foghorn_leghorn_short_desc;
extern char * foghorn_leghorn_long_desc;
void foghorn_leghorn_test(void);
void foghorn_leghorn_setup(void);
void foghorn_leghorn_cleanup(void);

extern char * hiro_hamada_short_desc;
extern char * hiro_hamada_long_desc;
void hiro_hamada_test(void);
void hiro_hamada_setup(void);
void hiro_hamada_cleanup(void);

extern char * barnyard_dawg_short_desc;
extern char * barnyard_dawg_long_desc;
void barnyard_dawg_dualsp_test(void);
void barnyard_dawg_dualsp_setup(void);
void barnyard_dawg_dualsp_cleanup(void);

extern char * henery_the_baby_chicken_hawk_short_desc;
extern char * henery_the_baby_chicken_hawk_long_desc;
void henery_the_baby_chicken_hawk_dualsp_test(void);
void henery_the_baby_chicken_hawk_dualsp_setup(void);
void henery_the_baby_chicken_hawk_dualsp_cleanup(void);

/* Dual SP */
void scooby_dum_dualsp_setup(void);
void scooby_dum_dualsp_test(void);
void scooby_dum_dualsp_cleanup(void);

extern char * kit_cloudkicker_short_desc;
extern char * kit_cloudkicker_long_desc;
void kit_cloudkicker_dualsp_test(void);
void kit_cloudkicker_dualsp_setup(void);
void kit_cloudkicker_dualsp_cleanup(void);

extern char * kozma_prutkov_short_desc;
extern char * kozma_prutkov_long_desc;
/* Dual SP */
void kozma_prutkov_dualsp_test(void);
void kozma_prutkov_test_dualsp_init(void);
void kozma_prutkov_test_dualsp_destroy(void);

extern char * billy_pilgrim_short_desc;
extern char * billy_pilgrim_long_desc;
void billy_pilgrim_test(void);
void billy_pilgrim_config_init(void);
void billy_pilgrim_config_destroy(void);

extern char * batman_short_desc;
extern char * batman_long_desc;
void batman_test(void);
void batman_setup(void);
void batman_destroy(void);

extern char * boomer_short_desc;
extern char * boomer_long_desc;
void boomer_test(void);
void boomer_test_setup(void);
void boomer_test_cleanup(void);

extern char * boomer_4_new_drive_short_desc;
extern char * boomer_4_new_drive_long_desc;
void boomer_4_new_drive_test(void);
void boomer_4_new_drive_test_setup(void);
void boomer_4_new_drive_test_cleanup(void);

extern char * dr_jekyll_short_desc;
extern char * dr_jekyll_long_desc;
void dr_jekyll_test(void);
void dr_jekyll_config_init(void);
void dr_jekyll_config_destroy(void);

extern char * whoopsy_doo_short_desc;
extern char * whoopsy_doo_long_desc;
void whoopsy_doo_setup(void);
void whoopsy_doo_test(void);
void whoopsy_doo_cleanup(void);

extern char * yeti_short_desc;
extern char * yeti_long_desc;
void yeti_setup(void);
void yeti_test(void);
void yeti_cleanup(void);

extern char * eliot_rosewater_short_desc;
extern char * eliot_rosewater_long_desc;
//void eliot_rosewater_test(void);
//void eliot_rosewater_setup(void);
//void eliot_rosewater_destroy(void);

/* Dual SP */
void eliot_rosewater_dualsp_test(void);
void eliot_rosewater_dualsp_setup(void);
void eliot_rosewater_dualsp_cleanup(void);

extern char * kishkashta_short_desc;
extern char * kishkashta_long_desc;
void kishkashta_test(void);
void kishkashta_test_init(void);
void kishkashta_test_destroy(void);

extern char * doodle_short_desc;
extern char * doodle_long_desc;
void doodle_test(void);
void doodle_setup(void);
void doodle_cleanup(void);
void doodle_dualsp_test(void);
void doodle_dualsp_setup(void);
void doodle_dualsp_cleanup(void);

extern char * phineas_and_ferb_short_desc;
extern char * phineas_and_ferb_long_desc;
void phineas_and_ferb_test(void);
void phineas_and_ferb_test_init(void);
void phineas_and_ferb_test_destroy(void);

extern char * baby_bear_short_desc;
extern char * baby_bear_long_desc;
void baby_bear_test(void);
void baby_bear_setup(void);
void baby_bear_cleanup(void);

extern char * gaggy_short_desc;
extern char * gaggy_long_desc;
void gaggy_test(void);
void gaggy_test_init(void);
void gaggy_test_destroy(void);

extern char * herry_monster_test_short_desc;
extern char * herry_monster_test_long_desc;
void herry_monster_test(void);
void herry_monster_setup(void);
void herry_monster_cleanup(void);

extern char * performance_test_short_desc;
extern char * performance_test_long_desc;
void performance_test(void);
void performance_test_init(void);
void performance_test_destroy(void);

extern char * bender_test_short_desc;
extern char * bender_test_long_desc;
void bender_dualsp_test(void);
void bender_dualsp_setup(void);
void bender_dualsp_cleanup(void);

extern char * flexo_test_short_desc;
extern char * flexo_test_long_desc;
void flexo_test(void);
void flexo_test_setup(void);
void flexo_test_cleanup(void);

extern char * beibei_test_short_desc;
extern char * beibei_test_long_desc;
void beibei_test(void);
void beibei_setup(void);
void beibei_cleanup(void);

extern char * dumku_short_desc;
extern char * dumku_long_desc;
void dumku_setup(void);
void dumku_test(void);
void dumku_destroy(void);

extern char * hulk_short_desc;
extern char * hulk_long_desc;
void hulk_setup(void);
void hulk_test(void);
void hulk_destroy(void);

extern char * shrek_short_desc;
extern char * shrek_long_desc;
void shrek_test(void);
void shrek_test_setup(void);
void shrek_test_cleanup(void);

extern char * transformers_short_desc;
extern char * transformers_long_desc;
void transformers_test(void);
void transformers_test_setup(void);
void transformers_test_cleanup(void);


extern char * spongebob_short_desc;
extern char * spongebob_long_desc;
void spongebob_setup(void);
void spongebob_test(void);
void spongebob_cleanup(void);

extern char * zoidberg_short_desc;
extern char * zoidberg_long_desc;
void zoidberg_dualsp_test(void);
void zoidberg_dualsp_setup(void);
void zoidberg_dualsp_cleanup(void);

extern char * darkwing_duck_short_desc;
extern char * darkwing_duck_long_desc;
void darkwing_duck_test(void);
void darkwing_duck_setup(void);
void darkwing_duck_cleanup(void);

extern char * diesel_short_desc;
extern char * diesel_long_desc;
void diesel_setup(void);
void diesel_test(void);
void diesel_cleanup(void);

extern char * smurfs_short_desc;
extern char * smurfs_long_desc;
void smurfs_test(void);
void smurfs_test_setup(void);
void smurfs_test_cleanup(void);

extern char * harry_potter_short_desc;
extern char * harry_potter_long_desc;
void harry_potter_test(void);
void harry_potter_setup(void);
void harry_potter_cleanup(void);

extern char * hermione_short_desc;
extern char * hermione_long_desc;
void hermione_test(void);
void hermione_setup(void);
void hermione_cleanup(void);
void hermione_dualsp_test(void);
void hermione_dualsp_setup(void);
void hermione_dualsp_cleanup(void);

extern char * salvador_dali_short_desc;
extern char * salvador_dali_long_desc;
void salvador_dali_test(void);
void salvador_dali_setup(void);
void salvador_dali_cleanup(void);

extern char * captain_planet_short_desc;
extern char * captain_planet_long_desc;
void captain_planet_dualsp_test(void);
void captain_planet_dualsp_setup(void);
void captain_planet_dualsp_cleanup(void);

extern char * splinter_short_desc;
extern char * splinter_long_desc;
void splinter_dualsp_test(void);
void splinter_dualsp_setup(void);
void splinter_dualsp_cleanup(void);

extern char * shredder_short_desc;
extern char * shredder_long_desc;
void shredder_dualsp_test(void);
void shredder_dualsp_setup(void);
void shredder_dualsp_cleanup(void);

extern char * pokemon_short_desc;
extern char * pokemon_long_desc;
void pokemon_test(void);
void pokemon_dualsp_test(void);
void pokemon_dualsp_setup(void);
void pokemon_dualsp_cleanup(void);

extern char * ponyo_short_desc;
extern char * ponyo_long_desc;
void ponyo_test(void);
void ponyo_setup(void);
void ponyo_destroy(void);

extern char * kungfu_panda_short_desc;
extern char * kungfu_panda_long_desc;
void kungfu_panda_test(void);
void kungfu_panda_setup(void);
void kungfu_panda_cleanup(void);
/* Dual SP */
void kungfu_panda_dualsp_test(void);
void kungfu_panda_dualsp_setup(void);
void kungfu_panda_dualsp_cleanup(void);

extern char * king_minos_short_desc;
extern char * king_minos_long_desc;
void king_minos_test(void);
void king_aeacus_test(void);
void king_rhadamanthus_test(void);
void king_minos_setup(void);
void king_minos_cleanup(void);

extern char * eye_of_vecna_short_desc;
extern char * eye_of_vecna_long_desc;
void eye_of_vecna_test(void);
void eye_of_vecna_setup(void);
void eye_of_vecna_cleanup(void);


extern char * sailor_moon_short_desc;
extern char * sailor_moon_long_desc;
void sailor_moon_test(void);
void sailor_moon_setup(void);
void sailor_moon_cleanup(void);

/* sep_standard_configs */
void sep_standard_logical_config_create_rg_with_lun(fbe_test_rg_configuration_t *raid_group_config_p, fbe_u32_t luns_per_rg);
void sep_standard_logical_config_get_rg_configuration(fbe_test_sep_rg_config_type_t rg_type, fbe_test_rg_configuration_t **rg_config_p);

extern char * chromatic_test_short_desc;
extern char * chromatic_test_long_desc;
void chromatic_phase1_test(void);
void chromatic_phase2_test(void);
void chromatic_setup(void);
void chromatic_cleanup(void);
/* Dual SP */
void chromatic_dualsp_test(void);
void chromatic_dualsp_setup(void);
void chromatic_dualsp_cleanup(void);


extern char * snake_ica_test_short_desc;
extern char * snake_ica_test_long_desc;
void snake_ica_setup(void);
void snake_ica_test(void);
void snake_ica_test_cleanup(void);


extern char * ezio_test_short_desc;
extern char * ezio_test_long_desc;
void ezio_test(void);
void ezio_setup(void);
void ezio_cleanup(void);

extern char * ezio_dualsp_test_short_desc;
extern char * ezio_dualsp_test_long_desc;
void ezio_dualsp_test(void);
void ezio_dualsp_setup(void);
void ezio_dualsp_cleanup(void);

extern char * sabbath_test_short_desc;
extern char * sabbath_test_long_desc;
extern char * broken_Rose_test_short_desc;
extern char * broken_Rose_test_long_desc;
void sabbath_test(void);
void sabbath_test2(void);
void sabbath_test_move_drive_when_array_is_online(void);
void sabbath_test_move_drive_when_array_is_offline_test1(void);
void sabbath_test_move_drive_when_array_is_offline_test2(void);
void sabbath_test_move_drive_when_array_is_offline_test3(void);
void sabbath_test_move_drive_when_array_is_offline_test_bootflash_mode(void);
void sabbath_test_drive_replacement_policy_online_test(void);
void sabbath_test_drive_replacement_policy_offline_test(void);
void sabbath_test_drive_replacement_policy_error_handling_offline_test(void);
void broken_Rose_test(void);
void sabbath_setup(void);
void sabbath_cleanup(void);

extern char * repairman_test_short_desc;
extern char * repairman_test_long_desc;
void repairman_test(void);
void repairman_setup(void);
void repairman_cleanup(void);

extern char * scrooge_mcduck_test_short_desc;
extern char * scrooge_mcduck_test_long_desc;
void scrooge_mcduck_test(void);
void scrooge_mcduck_setup(void);
void scrooge_mcduck_cleanup(void);


extern char * arwen_short_desc;
extern char * arwen_long_desc;
void arwen_test(void);
void arwen_setup(void);
void arwen_cleanup(void);

extern char * arwen_dualsp_short_desc;
extern char * arwen_dualsp_long_desc;
void arwen_dualsp_test(void);
void arwen_dualsp_setup(void);
void arwen_dualsp_cleanup(void);

extern char * amon_short_desc;
extern char * amon_long_desc;
void amon_test(void);
void amon_setup(void);
void amon_cleanup(void);


extern char * jingle_bell_short_desc;
extern char * jingle_bell_long_desc;
void jingle_bell_setup(void);
void jingle_bell_test(void);
void jingle_bell_cleanup(void);
void jingle_bell_dualsp_setup(void);
void jingle_bell_dualsp_test(void);
void jingle_bell_dualsp_cleanup(void);

extern char * starbuck_short_desc;
extern char * starbuck_long_desc;
void starbuck_setup(void);
void starbuck_test(void);
void starbuck_cleanup(void);

extern char *launchpad_mcquack_short_desc;
extern char *launchpad_mcquack_long_desc;
void launchpad_mcquack_test(void);
void launchpad_mcquack_setup(void);
void launchpad_mcquack_cleanup(void);

extern char * robi_short_desc;
extern char * robi_long_desc;
void robi_test(void);
void robi_setup(void);
void robi_cleanup(void);

extern char * oh_smiling_short_desc;
extern char * oh_smiling_long_desc;
void oh_smiling_test(void);
void oh_smiling_setup(void);
void oh_smiling_cleanup(void);

extern char * red_riding_hood_short_desc;
extern char * red_riding_hood_long_desc;
void red_riding_hood_test(void);
void red_riding_hood_setup(void);
void red_riding_hood_cleanup(void);
void red_riding_hood_dualsp_test(void);
void red_riding_hood_dualsp_setup(void);
void red_riding_hood_dualsp_cleanup(void);

extern char * phoenix_short_desc;
extern char * phoenix_long_desc;
void phoenix_test(void);
void phoenix_setup(void);
void phoenix_cleanup(void);


extern char * snakeskin_short_desc;
extern char * snakeskin_long_desc;
void snakeskin_test_dualsp_init(void);
void snakeskin_test_dualsp_destroy(void);
void snakeskin_dualsp_test(void);

extern char * agent_jay_short_desc;
extern char * agent_jay_long_desc;
void agent_jay_test(fbe_u32_t raid_type_index);
void agent_jay_setup(fbe_u32_t raid_type_index);
void agent_jay_cleanup(void);
void agent_jay_test_r5(void);
void agent_jay_test_r3(void);
void agent_jay_test_r6_single_degraded(void);
void agent_jay_test_r6_double_degraded(void);
void agent_jay_setup_r5(void);
void agent_jay_setup_r3(void);
void agent_jay_setup_r6(void);

extern char * agent_kay_short_desc;
extern char * agent_kay_long_desc;
void agent_kay_test(fbe_u32_t raid_type_index);
void agent_kay_setup(fbe_u32_t raid_type_index);
void agent_kay_cleanup(void);
void agent_kay_test_r5(void);
void agent_kay_test_r3(void);
void agent_kay_test_r6(void);
void agent_kay_setup_r5(void);
void agent_kay_setup_r3(void);
void agent_kay_setup_r6(void);

void cassie_dualsp_test(void);
extern char* cassie_dualsp_test_long_desc;
extern char* cassie_dualsp_test_short_desc;
void cassie_dualsp_setup(void);
void cassie_dualsp_cleanup(void);

extern char * doraemon_short_desc;
extern char * doraemon_long_desc;
void doraemon_system_test(void);
void doraemon_user_test(void);
void doraemon_setup(void);
void doraemon_cleanup(void);
void doraemon_user_dualsp_test(void);
void doraemon_system_dualsp_test(void);
void doraemon_dualsp_setup(void);
void doraemon_dualsp_cleanup(void);

extern char * fozzie_short_desc;
extern char * fozzie_long_desc;
void fozzie_dualsp_test(void);
void fozzie_dualsp_setup(void);
void fozzie_dualsp_cleanup(void);

extern char * crayon_shin_chan_dualsp_test_short_desc;
extern char * crayon_shin_chan_dualsp_test_long_desc;
void crayon_shin_chan_dualsp_test(void);
void crayon_shin_chan_dualsp_setup(void);
void crayon_shin_chan_dualsp_cleanup(void);

extern char * edgar_the_bug_short_desc;
extern char * edgar_the_bug_long_desc;
void edgar_the_bug_dualsp_test(void);
void edgar_the_bug_dualsp_setup(void);
void edgar_the_bug_dualsp_cleanup(void);

extern char * wreckit_ralph_short_desc;
extern char * wreckit_ralph_long_desc;
void wreckit_ralph_dualsp_test_job_destroy_rg_test1(void);
void wreckit_ralph_dualsp_test_job_destroy_rg_test2(void);
void wreckit_ralph_dualsp_test_job_create_rg_test1(void);
void wreckit_ralph_dualsp_test_job_create_rg_test2(void);
void wreckit_ralph_dualsp_setup(void);
void wreckit_ralph_dualsp_cleanup(void);

extern char * dumbledore_short_desc;
extern char * dumbledore_long_desc;
void dumbledore_dualsp_test(void);
void dumbledore_dualsp_setup(void);
void dumbledore_dualsp_cleanup(void);

extern char * rescue_rangers_short_desc;
extern char * rescue_rangers_long_desc;
void rescue_rangers_dualsp_test(void);
void rescue_rangers_dualsp_setup(void);
void rescue_rangers_dualsp_cleanup(void);

extern char * parinacota_short_desc;
extern char * parinacota_long_desc;
void parinacota_test(void);
void parinacota_setup(void);
void parinacota_cleanup(void);

extern char * percy_short_desc;
extern char * percy_long_desc;
void percy_test(void);
void percy_setup(void);
void percy_destroy(void);

extern const char *lurker_short_desc;
extern const char *lurker_long_desc;
extern void lurker_singlesp_test(void);
extern void lurker_singlesp_setup(void);
extern void lurker_singlesp_cleanup(void);
extern char * smelly_cat_short_desc;
extern char * smelly_cat_long_desc;
void smelly_cat_dualsp_test(void);
void smelly_cat_dualsp_setup(void);
void smelly_cat_dualsp_cleanup(void);

extern char * gravity_falls_short_desc;
extern char * gravity_falls_long_desc;
void gravity_falls_test(void);
void gravity_falls_setup(void);
void gravity_falls_cleanup(void);
void gravity_falls_dualsp_test(void);
void gravity_falls_dualsp_setup(void);
void gravity_falls_dualsp_cleanup(void);

extern char * thomas_short_desc;
extern char * thomas_long_desc;
void thomas_test(void);
void thomas_setup(void);
void thomas_cleanup(void);
void thomas_dualsp_test(void);
void thomas_dualsp_setup(void);
void thomas_dualsp_cleanup(void);


extern char *dark_archon_test_short_desc;
extern char * dark_archon_test_long_desc;
void dark_archon_test(void);

extern char *baymax_short_desc;
extern char *baymax_long_desc;
void baymax_test(void);
void baymax_setup(void);
void baymax_cleanup(void);

extern char * vinda_dualsp_short_desc;
extern char * vinda_dualsp_long_desc;
void vinda_dualsp_test(void);
void vinda_dualsp_setup(void);
void vinda_dualsp_cleanup(void);

extern char *vulture_short_desc;
extern char * vulture_long_desc;
void vulture_dualsp_test(void);
void vulture_dualsp_setup(void);
void vulture_dualsp_cleanup(void);

extern char * scooter_short_desc;
extern char * scooter_long_desc;
void scooter_test(void);
void scooter_setup(void);
void scooter_cleanup(void);
void scooter_dualsp_test(void);
void scooter_dualsp_setup(void);
void scooter_dualsp_cleanup(void);


extern char * gonzo_short_desc;
extern char * gonzo_long_desc;
void gonzo_test(void);
void gonzo_setup(void);
void gonzo_cleanup(void);
void gonzo_dualsp_test(void);
void gonzo_dualsp_setup(void);
void gonzo_dualsp_cleanup(void);

extern char *walle_cleaner_short_desc;
extern char *walle_cleaner_long_desc;
void walle_cleaner_setup(void);
void walle_cleaner_cleanup(void);
void walle_cleaner_test(void);

/* jigglypuff_new_drive_test */
extern char * jigglypuff_drive_validation_short_desc;
extern char * jigglypuff_drive_validation_long_desc;
void jigglypuff_drive_validation_test(void);
void jigglypuff_drive_validation_setup(void);
void jigglypuff_drive_validation_cleanup(void);

extern char * sully_short_desc;
extern char * sully_long_desc;
void sully_setup(void);
void sully_test(void);
void sully_cleanup(void);
void sully_dualsp_setup(void);
void sully_dualsp_cleanup(void);

extern char *defiler_short_desc;
extern char *defiler_long_desc;
void defiler_dualsp_setup(void);
void defiler_dualsp_cleanup(void);
void defiler_dualsp_test(void);

#endif /* SEP_TESTS_H */

