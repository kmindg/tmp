/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp_test.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the common utility functions for the flexports related
 *  esp tests.
 *
 * @version
 *   7/18/2012 - Created. bphilbin
 *
 ***************************************************************************/



fbe_status_t fbe_test_load_fp_config(SPID_HW_TYPE platform_type);
fbe_status_t fbe_fp_test_load_required_config(SPID_HW_TYPE platform_type);
void fp_test_load_dynamic_config(SPID_HW_TYPE platform_type, fbe_status_t (test_config_function(void)));
void fp_test_start_physical_package(void);
void fbe_test_esp_restart(void);
void fp_test_set_persist_ports(fbe_bool_t persist_flag);
void fbe_test_esp_unload(void);
void fbe_test_esp_reload_with_existing_terminator_config(void);

void fbe_test_startEspAndSep(fbe_sim_transport_connection_target_t targetSp, SPID_HW_TYPE platformType);

void fbe_test_reg_set_persist_ports_true(void);

typedef enum fbe_test_common_config_t{
    FBE_ESP_TEST_FP_CONIG,
    FBE_ESP_TEST_SIMPLE_CONFIG,
    FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG,
    FBE_ESP_TEST_SIMPLE_VIKING_CONFIG,
    FBE_ESP_TEST_SIMPLE_CAYENNE_CONFIG,
    FBE_ESP_TEST_SIMPLE_NAGA_CONFIG,
    FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG,
    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
    FBE_ESP_TEST_LAPA_RIOS_CONIG,
    FBE_ESP_TEST_NAXOS_VIPER_CONIG,
    FBE_ESP_TEST_NAXOS_VOYAGER_CONIG,
    FBE_ESP_TEST_MIXED_CONIG,
    FBE_ESP_TEST_BASIC_CONIG,
    FBE_ESP_TEST_CHAUTAUQUA_ANCHO_CONFIG,
    FBE_ESP_TEST_NAXOS_ANCHO_CONIG,
    FBE_ESP_TEST_NAXOS_TABASCO_CONIG,
    FBE_ESP_TEST_SIMPLE_MIRANDA_CONFIG,  // 25 drives Oberon.
    FBE_ESP_TEST_SIMPLE_CALYPSO_CONFIG,  // 25 drives Hyperion.
}fbe_test_common_config;

typedef struct fbe_esp_test_common_config_info_s{
    fbe_test_common_config config;
    fbe_bool_t config_init_terminator;
    fbe_bool_t config_load_pp;
    fbe_u32_t num_ojbects;
}fbe_esp_test_common_config_info_t;

void fbe_test_startEspAndSep_with_common_config(fbe_sim_transport_connection_target_t target_sp,
                                    fbe_test_common_config config,
                                    SPID_HW_TYPE platform_type,
                                    void specl_config_func(void),
                                    void reg_config_func(void));

void fbe_test_startEsp_with_common_config(fbe_sim_transport_connection_target_t target_sp,
                                    fbe_test_common_config config,
                                    SPID_HW_TYPE platform_type,
                                    void specl_config_func(void),
                                    void reg_config_func(void));

void fbe_test_startEspAndSep_with_common_config_dualSp(fbe_test_common_config config,
                                                       SPID_HW_TYPE platform_type,
                                                       void specl_config_func(void),
                                                       void reg_config_func(void));

void fbe_test_startEsp_with_common_config_dualSp(fbe_test_common_config config,
                                                       SPID_HW_TYPE platform_type,
                                                       void specl_config_func(void),
                                                       void reg_config_func(void));
