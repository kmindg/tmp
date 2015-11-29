/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file daphne_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of unconsumed PVDs remapping and then
 *  getting consumed as part of a raid group.
 *
 * @version
 *  7/11/2012 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * daphne_short_desc = "Unconsumed sniff media errors on PVDs that then get bound";
char * daphne_long_desc = "\
The daphne scenario tests what happens when an unconsumed PVD gets media errors and.\n\
does the expected remapping of those media errors.  Then it makes sure that once\n\
those PVDs are consumed into raid groups, there are no unexpected errors.\n\
\n\
STEP 1) Inject media errors for sniff to find and then remap.\n\
STEP 2) Wait for the sniff to pass the point where the errors were injected.\n\
STEP 3) Create a raid group and LUNs on the drive that remapped. \n\
STEP 4) wait for zeroing to finish and the initial verify to run.\n\
STEP 5) make sure there were no errors found during the verify. \n\
\n\
Description last updated: 7/11/2012.\n";

enum {
    DAPHNE_TEST_LUNS_PER_RAID_GROUP = 3,
    DAPHNE_MAX_WAIT_SEC = 120,
    DAPHNE_MAX_WAIT_MSEC = (DAPHNE_MAX_WAIT_SEC * 1000),
    DAPHNE_CHUNKS_PER_LUN = 4,
    DAPHNE_MAX_IO_BLOCKS = 128,
};

fbe_api_logical_error_injection_record_t daphne_error_record[] = 
{
    {                                           
        0x4, 0x10, 0x10000, 0xc, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
    }
};

/*!*******************************************************************
 * @var daphne_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t daphne_raid_group_config[] = 
{
    /* width,                       capacity    raid type,                  class,       block size RAID-id. bandwidth. */
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_CLASS_ID_INVALID, 520, 0, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
void daphne_inject_media_errors(fbe_object_id_t pvd_object_id)
{
    fbe_status_t                                        status;
    fbe_provision_drive_get_verify_status_t verify_status;

    mut_printf( MUT_LOG_TEST_STATUS, "== starting == \n");

    fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
    fbe_test_sep_util_provision_drive_get_verify_status(pvd_object_id, &verify_status);
    MUT_ASSERT_INT_EQUAL(verify_status.verify_enable, FBE_FALSE);

    fbe_test_sep_util_provision_drive_clear_verify_report( pvd_object_id );

    status = fbe_api_logical_error_injection_enable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
}

void fbe_test_sep_util_wait_for_sniff(fbe_object_id_t pvd_object_id, fbe_u32_t passes, fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_u32_t total_time_ms = 0;
    fbe_provision_drive_verify_report_t verify_report;

    while (total_time_ms < timeout_ms)
    {
        status = fbe_test_sep_util_provision_drive_get_verify_report( pvd_object_id, &verify_report );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_report.pass_count == passes)
        {
            return;
        }
        fbe_api_sleep(500);
        total_time_ms += 500;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Sniff did not complete in %d ms\n", timeout_ms);
    MUT_FAIL();
}

void fbe_test_sep_util_wait_for_sniff_chkpt(fbe_object_id_t pvd_object_id, fbe_lba_t chkpt, fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_u32_t total_time_ms = 0;
    fbe_provision_drive_get_verify_status_t verify_status;

    while (total_time_ms < timeout_ms)
    {
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (verify_status.verify_checkpoint > chkpt)
        {
            return;
        }
        fbe_api_sleep(500);
        total_time_ms += 500;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Sniff did not complete in %d ms\n", timeout_ms);
    MUT_FAIL();
}
void daphne_start_sniff(fbe_object_id_t pvd_object_id)
{
    fbe_provision_drive_get_verify_status_t verify_status;

    fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id, 0);
    fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
    fbe_test_sep_util_provision_drive_get_verify_status(pvd_object_id, &verify_status);
    MUT_ASSERT_INT_EQUAL(verify_status.verify_enable, FBE_TRUE);
    return;
}

void daphne_wait_for_sniff(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;
    fbe_provision_drive_verify_report_t verify_report;
    fbe_api_logical_error_injection_get_object_stats_t pvd_stats;
    fbe_api_logical_error_injection_get_stats_t stats;

    /* Wait for a pass to complete. 
     */ 
    fbe_test_sep_util_wait_for_sniff_chkpt(pvd_object_id, 0x12000, FBE_TEST_WAIT_TIMEOUT_MS);
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    status = fbe_api_logical_error_injection_disable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    status = fbe_api_logical_error_injection_get_object_stats(&pvd_stats, pvd_object_id);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    mut_printf(MUT_LOG_TEST_STATUS,
               "errors encountered pvd obj: 0x%x reads: %lld, writes: %lld",
               pvd_object_id, (long long)pvd_stats.num_read_media_errors_injected, (long long)pvd_stats.num_write_verify_blocks_remapped);
    
    status = fbe_test_sep_util_provision_drive_get_verify_report(pvd_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS,
               "pvd obj: 0x%x errors detected - recovered reads: %d, unrecovered reads: %d",
               pvd_object_id, verify_report.totals.recov_read_count, verify_report.totals.unrecov_read_count);

    /* Restore the sector trace level to it's default.
     */
    //fbe_test_sep_util_restore_sector_trace_level();

    return;
}
/*!**************************************************************
 * daphne_inject_errors()
 ****************************************************************
 * @brief
 *  Run the daphne test
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void daphne_inject_errors(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t pvd_object_id;
    fbe_u32_t pos_index;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t pvd_count = 0;

    /* Wait for zeroing to complete on all PVDs. 
     * Start error injection on all PVDs. 
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for (pos_index = 0; pos_index < current_rg_config_p->width; pos_index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[pos_index].bus,
                                                                    current_rg_config_p->rg_disk_set[pos_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[pos_index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);
            daphne_inject_media_errors(pvd_object_id);
            pvd_count++;
        }
        current_rg_config_p++;
    }
 
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_create_record(&daphne_error_record[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats( &stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, pvd_count);

    /* Allow the sniff proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for (pos_index = 0; pos_index < current_rg_config_p->width; pos_index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[pos_index].bus,
                                                                    current_rg_config_p->rg_disk_set[pos_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[pos_index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            daphne_start_sniff(pvd_object_id);
        }
        current_rg_config_p++;
    }

    /* Wait for sniff to complete.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for (pos_index = 0; pos_index < current_rg_config_p->width; pos_index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[pos_index].bus,
                                                                    current_rg_config_p->rg_disk_set[pos_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[pos_index].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            daphne_wait_for_sniff(pvd_object_id);
        }
        current_rg_config_p++;
    }

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end daphne_inject_errors()
 **************************************/
/*!**************************************************************
 * daphne_run_tests()
 ****************************************************************
 * @brief
 *  Run the daphne test
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void daphne_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    /* Wait for the initial verify to succeed. 
     * It should not get errors since the remap of media errors on the unconsumed 
     * PVD should have written zeros. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);
    return;
}
/**************************************
 * end daphne_run_tests()
 **************************************/
/*!**************************************************************
 * daphne_test()
 ****************************************************************
 * @brief
 *  Run daphne test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void daphne_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &daphne_raid_group_config[0];

    daphne_inject_errors(rg_config_p, NULL);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, daphne_run_tests,
                                   DAPHNE_TEST_LUNS_PER_RAID_GROUP,
                                   DAPHNE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end daphne_test()
 ******************************************/
/*!**************************************************************
 * daphne_setup()
 ****************************************************************
 * @brief
 *  Setup for a daphne test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
void daphne_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &daphne_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, DAPHNE_CHUNKS_PER_LUN * 2);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end daphne_setup()
 **************************************/

/*!**************************************************************
 * daphne_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the daphne test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void daphne_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();
    }
    return;
}
/******************************************
 * end daphne_cleanup()
 ******************************************/
/*************************
 * end file daphne_test.c
 *************************/


