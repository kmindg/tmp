
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file launchpad_mcquack_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of delayed IO
 *
 * @version
 *   1/23/2012 - Created. Deanna Heng
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_rebuild_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * launchpad_mcquack_short_desc = "Delay an IO packet up and down the stack";
char * launchpad_mcquack_long_desc ="\
The Launchpad McQuack Test performs tests with delayed IOs.\n\
\n\
Dependencies:\n\
        - Debug Hooks for Delaying an IO.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 15 SAS drives\n\
        [PP] 15 SATA drives\n\
        [PP] 30 logical drive\n\
        [SEP] 12 provision drive\n\
        [SEP] 12 virtual drive\n\
        [SEP] x LUNs\n\
\n\
STEP 1: Bring 1 SP up (SPA).\n\
STEP 2: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 3: Create a delay io record for the logical error injection service\n\
        - Delay IO on edges going down: lun->rg or rg->vd or vd->pvd\n\
        - Delay IO on edges going up: rg->lun or vd->rg or pvd->vd\n\
STEP 4: Remove either 1 or 2 drives.\n\
STEP 5: Disable logical error injection on appropriate objects to release the IO that was held.\n\
STEP 6: Verify that all operations are completed successfully.\n\
STEP 7: Cleanup\n\
        - Destroy objects\n";

/*!*******************************************************************
 * @def LAUNCHPAD_MCQUACK_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define LAUNCHPAD_MCQUACK_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def LAUNCHPAD_MCQUACK_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define LAUNCHPAD_MCQUACK_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def LAUNCHPAD_MCQUACK_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait an object to be ready
 *
 *********************************************************************/
#define LAUNCHPAD_MCQUACK_WAIT_MSEC 1000 * 120

/*!*******************************************************************
 * @var launchpad_mcquack_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/

fbe_test_rg_configuration_array_t launchpad_mcquack_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
{
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        //{16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,     520,            2,         0},
        //{5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    512,            3,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end launchpad_mcquack_raid_group_config_qual()
 **************************************/

#define LAUNCHPAD_MCQUACK_DELAY_MSECS 5000

/* General LEI Record to create to delay the IO going down the stack
 */
fbe_api_logical_error_injection_record_t record_down =        
              { 0x1,  /* pos_bitmap */
                0x10, /* width */
                0x0,  /* lba */
                0x1,  /* blocks */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,      /* error type */
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
                0x0,  /* error count */
                LAUNCHPAD_MCQUACK_DELAY_MSECS, /* error limit is msecs to delay */
                0x0,  /* skip count */
                0x15, /* skip limit */
                0x1,  /* error adjcency */
                0x0,  /* start bit */
                0x0,  /* number of bits */
                0x0,  /* erroneous bits */
                0x0, /* crc detectable */
                0}; /* crc detectable */

/* General LEI Record to create to delay the IO going up the stack
 */
fbe_api_logical_error_injection_record_t record_up =        
              { 0x1,  /* pos_bitmap */
                0x10, /* width */
                0x0,  /* lba */
                0x1,  /* blocks */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP,      /* error type */
                //FBE_LOGICAL_ERROR_INJECTION_MODE_DELAY_UP, /* error mode */
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0,  /* error count */
                LAUNCHPAD_MCQUACK_DELAY_MSECS, /* error limit is msecs to delay */
                0x0,  /* skip count */
                0x15, /* skip limit */
                0x1,  /* error adjcency */
                0x0,  /* start bit */
                0x0,  /* number of bits */
                0x0,  /* erroneous bits */
                0x0, /* crc detectable */
                0};

static fbe_api_rdgen_context_t launchpad_mcquack_test_contexts[10];
fbe_u32_t context_index = 0;

void launchpad_mcquack_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void launchpad_mcquack_rg_vd_edge_test_down(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_rg_vd_edge_test_up(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_rg_vd_edge_test_down_broken(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_rg_vd_edge_test_up_broken(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_lun_rg_edge_test_down(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_lun_rg_edge_test_up(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_vd_pvd_edge_test_down(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_vd_pvd_edge_test_up(fbe_test_rg_configuration_t *rg_config_p);
void launchpad_mcquack_delay_io_no_faults(fbe_test_rg_configuration_t *rg_config_p);


/*!**************************************************************
 * launchpad_mcquack_run_tests()
 ****************************************************************
 * @brief
 *  Run peer join tests for each raid group in the config
 *
 * @param rg_config_p - raid group configuration to run the tests against
 *        context - TBD              
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context)
{
    launchpad_mcquack_delay_io_no_faults(rg_config_p);

    launchpad_mcquack_lun_rg_edge_test_down(&rg_config_p[0]);
    launchpad_mcquack_rg_vd_edge_test_down(&rg_config_p[0]);
    launchpad_mcquack_vd_pvd_edge_test_down(&rg_config_p[0]);

    launchpad_mcquack_vd_pvd_edge_test_up(&rg_config_p[1]);
    launchpad_mcquack_rg_vd_edge_test_up(&rg_config_p[1]);
    launchpad_mcquack_lun_rg_edge_test_up(&rg_config_p[1]);

    launchpad_mcquack_rg_vd_edge_test_up_broken(&rg_config_p[1]);
    launchpad_mcquack_rg_vd_edge_test_down_broken(&rg_config_p[0]);

}
/***************************************************************
 * end launchpad_mcquack_run_tests()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_rg_vd_edge_test_up()
 ****************************************************************
 * @brief
 *  Delay the packet on the rg-vd edge while completing
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_rg_vd_edge_test_up(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");

    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                          &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&record_up);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;//FBE_RDGEN_LBA_SPEC_FIXED; //FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                              number_physical_objects, &drive_info);
    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);

    context_index++;
    return;
}
/***************************************************************
 * end launchpad_mcquack_rg_vd_edge_test_up()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_rg_vd_edge_test_up_broken()
 ****************************************************************
 * @brief
 *  Delay the packet on the rg-vd edge while completing and break the rg
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_rg_vd_edge_test_up_broken(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace_0 = 0;
	fbe_api_terminator_device_handle_t  drive_info_0;
    fbe_u32_t							drive_pos_to_replace_1 = 1;
	fbe_api_terminator_device_handle_t  drive_info_1;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
         */
        fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                   &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);
        fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, LAUNCHPAD_MCQUACK_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&record_up);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace_0,
                                              number_physical_objects, &drive_info_0);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace_1,
                                              number_physical_objects, &drive_info_1);
    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(1, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace_0,
                                                number_physical_objects, &drive_info_0);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace_1,
                                                number_physical_objects, &drive_info_1);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace_0);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace_1);
    context_index++;
    return;
}
/***************************************************************
 * end launchpad_mcquack_rg_vd_edge_test_up_broken()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_rg_vd_edge_test_down()
 ****************************************************************
 * @brief
 *  Delay the packet on the rg-vd edge while going down the stack
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_rg_vd_edge_test_down(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    //fbe_api_rdgen_get_request_info_t *info_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
         */
        fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                   &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&record_down);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                              number_physical_objects, &drive_info);

    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);

    context_index++;
    return;
}
/***************************************************************
 * end launchpad_mcquack_rg_vd_edge_test_down()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_rg_vd_edge_test_down_broken()
 ****************************************************************
 * @brief
 *  Delay the packet on the rg-vd edge while going down the stack.
 *  Break the rg before continuing packet completion
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_rg_vd_edge_test_down_broken(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace_0 = 0;
	fbe_api_terminator_device_handle_t  drive_info_0;
    fbe_u32_t							drive_pos_to_replace_1 = 1;
	fbe_api_terminator_device_handle_t  drive_info_1;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                   &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);
    fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                            FBE_LIFECYCLE_STATE_READY, LAUNCHPAD_MCQUACK_WAIT_MSEC,
                                            FBE_PACKAGE_ID_SEP_0);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&record_down);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace_0,
                                              number_physical_objects, &drive_info_0);

    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace_1,
                                              number_physical_objects, &drive_info_1);
    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(1, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace_0,
                                                number_physical_objects, &drive_info_0);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_replace_1,
                                              number_physical_objects, &drive_info_1);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace_0);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace_1);

    context_index++;

    return;
}
/***************************************************************
 * end launchpad_mcquack_rg_vd_edge_test_down_broken()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_lun_rg_edge_test_down()
 ****************************************************************
 * @brief
 *  Delay the packet on the lun-rg edge while going down the stack
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_lun_rg_edge_test_down(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    //fbe_api_rdgen_get_request_info_t *info_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                          &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    record_down.lba = 0x800;
    status = fbe_api_logical_error_injection_create_record(&record_down);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0x800;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    /*status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                              lun_object_id,
                                              FBE_CLASS_ID_INVALID,
                                              FBE_PACKAGE_ID_SEP_0,
                                              FBE_RDGEN_OPERATION_READ_ONLY,
                                              FBE_RDGEN_LBA_SPEC_FIXED,
                                              0x800,
                                              1);*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                              number_physical_objects, &drive_info);

    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);

    record_down.lba = 0x0;
    context_index++;

    return;
}
/***************************************************************
 * end launchpad_mcquack_lun_rg_edge_test_down()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_lun_rg_edge_test_up()
 ****************************************************************
 * @brief
 *  Delay the packet on the lun-rg edge while completing
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_lun_rg_edge_test_up(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    //fbe_api_rdgen_get_request_info_t *info_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                          &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    record_up.lba = 0x800;
    status = fbe_api_logical_error_injection_create_record(&record_up);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0x800;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                              number_physical_objects, &drive_info);

    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);

    record_up.lba = 0x0;
    context_index++;

    return;
}
/***************************************************************
 * end launchpad_mcquack_lun_rg_edge_test_up()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_vd_pvd_edge_test_down()
 ****************************************************************
 * @brief
 *  Delay the packet on the vd-pvd edge while going down the stack
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_vd_pvd_edge_test_down(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t vd_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                          &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);
    //  Get the VD's object id
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos_to_replace, &vd_object_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_create_record(&record_down);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_object(vd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0x0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_no_check(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                            number_physical_objects,
                                            &drive_info);

    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_object(vd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);

    context_index++;
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/***************************************************************
 * end launchpad_mcquack_vd_pvd_edge_test_down()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_vd_pvd_edge_test_up()
 ****************************************************************
 * @brief
 *  Delay the packet on the vd-pvd edge while completing
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_vd_pvd_edge_test_up(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t vd_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 1;
	fbe_api_terminator_device_handle_t  drive_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");

    /* Now run the test for this test case.
     */
    fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                          &lun_object_id);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);
    //  Get the VD's object id
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_create_record(&record_up);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_object(vd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0xFFFFFFFF;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0x0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_api_sleep(1000);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_no_check(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                            number_physical_objects,
                                            &drive_info);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_object(vd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_semaphore_wait_ms(&context_p->semaphore, LAUNCHPAD_MCQUACK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    //  Reinsert the first drive to fail and verify drive state changes
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);
    context_index++;

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/***************************************************************
 * end launchpad_mcquack_vd_pvd_edge_test_up()
 ***************************************************************/

/*!**************************************************************
 * launchpad_mcquack_delay_io_no_faults()
 ****************************************************************
 * @brief
 *  Delay the packet on the rg-vd edge while going down the stack
 *
 * @param rg_config_p - raid group configuration to run the tests against    
 *
 * @return None.   
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void launchpad_mcquack_delay_io_no_faults(fbe_test_rg_configuration_t *rg_config_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t * context_p = &launchpad_mcquack_test_contexts[context_index];
    fbe_api_rdgen_send_one_io_params_t params; 
    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Lauchpad McQuack Test =====\n");


    /* Now run the test for this test case.
         */
        fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                   &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);

    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_create_record(&record_down);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Send an IO to lun %d==", __FUNCTION__, lun_object_id);
    /* Send I/O for hitting the media error.
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0xFFFFFFFF;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
    params.pattern = FBE_RDGEN_LBA_SPEC_FIXED;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                              lun_object_id,
                                              FBE_CLASS_ID_INVALID,
                                              FBE_PACKAGE_ID_SEP_0,
                                              FBE_RDGEN_OPERATION_WRITE_ONLY,
                                              FBE_RDGEN_PATTERN_LBA_PASS,
                                              0,
                                              1,
                                              FBE_RDGEN_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_api_sleep(1000);

    /* Stop logical error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sleep(2000);

    MUT_ASSERT_INT_EQUAL(0, context_p->start_io.statistics.error_count);

    context_index++;
    return;
}
/***************************************************************
 * end launchpad_mcquack_delay_io_no_faults()
 ***************************************************************/


/*!****************************************************************************
 * launchpad_mcquack_test()
 ******************************************************************************
 * @brief
 *  Run permanent sparing zeroing tests on raid group configs
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 ******************************************************************************/
void launchpad_mcquack_test(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED testing_level = fbe_sep_test_util_get_raid_testing_extended_level();    

    /* rtl 1 and rtl 0 are the same for now */
            fbe_test_run_test_on_rg_config(&launchpad_mcquack_raid_group_config_qual[0][0],
                                           NULL,launchpad_mcquack_run_tests,
                                           LAUNCHPAD_MCQUACK_LUNS_PER_RAID_GROUP,
                                           LAUNCHPAD_MCQUACK_CHUNKS_PER_LUN);

    return;
}
/***************************************************************
 * end launchpad_mcquack_test()
 ***************************************************************/

/*!****************************************************************************
 *  launchpad_mcquack_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the launchpad_mcquack test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *****************************************************************************/
void launchpad_mcquack_setup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(&launchpad_mcquack_raid_group_config_qual[0][0]);
        
        //darkwing_duck_create_random_config(&launchpad_mcquack_raid_group_config_random[0], raid_group_count);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&launchpad_mcquack_raid_group_config_qual[0][0]);

        /* Setup the physical config for the raid groups
         */
        elmo_create_physical_config_for_rg(&launchpad_mcquack_raid_group_config_qual[0][0], 
                                           raid_group_count);
        sep_config_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);        
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/***************************************************************
 * end launchpad_mcquack_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  launchpad_mcquack_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the launchpad_mcquack test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  1/23/2012 - Created. Deanna Heng
 *****************************************************************************/
void launchpad_mcquack_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/***************************************************************
 * end launchpad_mcquack_dualsp_cleanup()
 ***************************************************************/
