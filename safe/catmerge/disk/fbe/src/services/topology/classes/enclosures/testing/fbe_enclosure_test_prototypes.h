#ifndef FBE_ENCLOSURE_TEST_PROTOTYPES_H
#define FBE_ENCLOSURE_TEST_PROTOTYPES_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_enclosure_test_prototypes.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains testing prototypes for the enclosure
 *  object.
 *
 * HISTORY
 *   7/16/2008:  Created. bphilbin - Thanks Rob!
 *
 ***************************************************************************/
#include "sas_viper_enclosure_private.h"
#include "fbe_enclosure_data_access_public.h"

/* fbe_sas_viper_enclosure_test_monitor.c */
VOID fbe_sas_viper_enclosure_test_create_object_function(void);
void fbe_sas_viper_enclosure_test_lifecycle(void);
void fbe_sas_viper_enclosure_test_publicDataAccess(void);
void fbe_sas_viper_enclosure_test_edalFlexComp(void);
/* eses-fbe_eses_main.c */
void test_fbe_eses_build_receive_diagnostic_results_cdb(void);
void test_fbe_eses_build_send_diagnostic_cdb(void);

/*fbe_sas_viper_enclosure_read.c*/
void test_fbe_sas_viper_enclosure_ps_extract_status(void);
void test_fbe_sas_viper_enclosure_connector_extract_status(void);
void test_fbe_sas_viper_enclosure_array_dev_slot_extract_status(void);
void test_fbe_sas_viper_enclosure_exp_phy_extract_status(void);
void test_fbe_sas_viper_enclosure_cooling_extract_status(void);
void test_fbe_sas_viper_enclosure_temp_sensor_extract_status(void);
void test_fbe_eses_enclosure_handle_scsi_cmd_response(void);

/*fbe_all_enclosure_functions_test.c*/
extern fbe_enclosure_component_block_t * 
fbe_sas_viper_init_component_block_with_block_size(fbe_sas_viper_enclosure_t * viperEnclPtr, 
                                                   fbe_u8_t *buffer,
                                                   fbe_u32_t edal_block_count,
                                                   fbe_u32_t memory_chunk_size);

#endif /* FBE_ENCLOSURE_TEST_PROTOTYPES_H */
/****************************************
 * end fbe_enclosure_test_prototypes.h
 ****************************************/