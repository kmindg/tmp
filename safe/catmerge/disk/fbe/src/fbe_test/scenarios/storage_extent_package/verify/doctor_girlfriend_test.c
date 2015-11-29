/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file doctor_girlfriend_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test for verify for system power failure during
 *  non cached writes
 *
 *
 ***************************************************************************/

#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_background_ops.h"
#include "pp_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test.h"
#include "EmcPAL_Misc.h"
#include "fbe/fbe_api_event_log_interface.h"


char * doctor_girlfriend_short_desc = "Test automatic full verify of dirty LUNs (RAID-5)";
char * doctor_girlfriend_long_desc =
    "\n"
    "\n"
    "The Hamburger scenario tests that when a RAID group goes to SHUTDOWN with outstanding\n"
    "non-cached write I/O, that it verifies the entire extent of any LUN that had\n"
    "outstanding non-cached writes when the RAID group comes back up.\n"
    "\n"
    "Dependencies:\n"
    "    - Persistent meta-data storage.\n"
    "    - The LUN object must support marking itself as 'LUN dirty' while non-cached writes are outstanding.\n"
    "    - The raid object must support verify-before write operations and any required verify meta-data.\n"
    " \n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 2 SAS drives (PDO)\n"
    "    [PP] 2 logical drives (LD)\n"
    "    [SEP] 2 provision drives (PD)\n"
    "    [SEP] 5 virtual drives (VD)\n"
    "    [SEP] 1 raid object (RAID-1)\n"
    "    [SEP] 3 lun objects (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology on one SP.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create a raid object with I/O edges attached to all VDs.\n"
    "    - Create each lun object with an I/O edge attached to the RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Shutdown RG during non-cached write I/O\n"
    "    - Start non-cached write I/O only to the first LUN object\n"
    "    - Initiate proactive copy\n"
    "    - Shutdown the RG by pulling multiple drives\n"
    "    - Stop I/O\n"
    "    - Restore drives in RG\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Verify that the RAID object verifies the first LUN extent.\n"
    "\n"
    ;



/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_RAID_GROUP_COUNT   1

/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_RAID_GROUP_CHUNK_SIZE
 *********************************************************************
 * @brief Number of blocks in a raid group bitmap chunk.
 *
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_RAID_GROUP_CHUNK_SIZE  (2 * FBE_RAID_SECTORS_PER_ELEMENT)

/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP    1

/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def DOCTOR_GIRLFRIEND_MAX_VERIFY_WAIT_TIME_SECS
 *********************************************************************
 * @brief Maximum time in seconds to wait for a verify operation to complete.
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_MAX_VERIFY_WAIT_TIME_SECS  120  

/*!*******************************************************************
 * @def BIG_BIRD_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define DOCTOR_GIRLFRIEND_MAX_IO_SIZE_BLOCKS (DOCTOR_GIRLFRIEND_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 


#define DOCTOR_GIRLFRIEND_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY 1
#define DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_SPARE 2
#define DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE 0
#define DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE 1

#define DOCTOR_GIRLFRIEND_CHUNKS_PER_LUN            0x5

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/


static void doctor_girlfriend_raid5_verify_test(fbe_u32_t index,
                                        fbe_test_rg_configuration_t*);

void doctor_girlfriend_write_background_pattern(void);

static void doctor_girlfriend_wait_for_lun_verify_completion(fbe_object_id_t in_lun_object_id,
                                                     fbe_u32_t in_lun_number,
                                                     fbe_lun_verify_report_t*   in_verify_report_p,
                                                     fbe_object_id_t in_rg_object_id);
                                                     

void doctor_girlfriend_get_lun_verify_status(fbe_object_id_t in_object_id,
                                    fbe_lun_get_verify_status_t* out_verify_status);

static void doctor_girlfriend_validate_verify_results(fbe_u32_t in_index,
                                              fbe_test_rg_configuration_t*);

static void doctor_girlfriend_start_io(void);

static void doctor_girlfriend_remove_and_reinsert_drives(fbe_test_rg_configuration_t*);

static void doctor_girlfriend_wait_for_verify_completion(fbe_object_id_t            in_lun_object_id,
                                                 fbe_u32_t                  in_lun_number,
                                                 fbe_lun_verify_report_t*   in_verify_report_p,
                                                 fbe_object_id_t            in_rg_object_id);

static void doctor_girlfriend_clear_verify_reports(fbe_test_rg_configuration_t* in_current_rg_config_p);
void doctor_girlfriend_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

static void doctor_girlfriend_reinsert_sp(fbe_sim_transport_connection_target_t which_sp, fbe_test_rg_configuration_t* in_current_rg_config_p);
static void doctor_girlfriend_remove_sp(fbe_sim_transport_connection_target_t which_sp);

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*!*******************************************************************
 * @var doctor_girlfriend_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t doctor_girlfriend_raid_group_config[] =  
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,         0x400000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

/*!*******************************************************************
 * @var fbe_DOCTOR_GIRLFRIEND_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fbe_doctor_girlfriend_test_context_g[DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP * DOCTOR_GIRLFRIEND_RAID_GROUP_COUNT];


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 * doctor_girlfriend_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void doctor_girlfriend_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * doctor_girlfriend_dualsp_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ******************************************************************************/

void doctor_girlfriend_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}

/*!****************************************************************************
 * doctor_girlfriend_test()
 ******************************************************************************
 * @brief
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ******************************************************************************/
void doctor_girlfriend_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &doctor_girlfriend_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,doctor_girlfriend_run_tests,
                                   DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP,
                                   DOCTOR_GIRLFRIEND_CHUNKS_PER_LUN);
    return;    

}

/*!****************************************************************************
 * doctor_girlfriend_dualsp_test()
 ******************************************************************************
 * @brief
 *
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ******************************************************************************/
void doctor_girlfriend_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &doctor_girlfriend_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,doctor_girlfriend_run_tests,
                                   DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP,
                                   DOCTOR_GIRLFRIEND_CHUNKS_PER_LUN);

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;    

}


/*!**************************************************************
 * doctor_girlfriend_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Hamburger test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *
 ****************************************************************/
void doctor_girlfriend_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &doctor_girlfriend_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, DOCTOR_GIRLFRIEND_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

        elmo_load_config(rg_config_p, DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP, DOCTOR_GIRLFRIEND_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;

}   // End doctor_girlfriend_setup()

/*!**************************************************************
 * doctor_girlfriend_dualsp_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Hamburger test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ****************************************************************/
void doctor_girlfriend_dualsp_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t  raid_group_count = 0;
        //const fbe_char_t *raid_type_string_p = NULL;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &doctor_girlfriend_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
        //fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);

        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;

}   // End doctor_girlfriend_dualsp_setup()

/*!**************************************************************
 * doctor_girlfriend_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the doctor_girlfriend test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void doctor_girlfriend_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t            index;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   num_raid_groups = 0;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    num_raid_groups = DOCTOR_GIRLFRIEND_RAID_GROUP_COUNT;

    mut_printf(MUT_LOG_LOW, "=== Hamburger test start ===\n");

    // STEP 2: Write initial data pattern to all configured LUNs.
    // This is necessary because bind is not working yet.
    doctor_girlfriend_write_background_pattern();

    // Perform the write verify test on all raid groups.
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
        
            mut_printf(MUT_LOG_TEST_STATUS, "doctor_girlfriend verify test iteration %d of %d.\n", 
                        index+1, num_raid_groups);
    
            doctor_girlfriend_raid5_verify_test(index, rg_config_p);
        }
    }

    return;

}   // End doctor_girlfriend_test()


/*!**************************************************************************
 * doctor_girlfriend_raid5_verify_test
 ***************************************************************************
 * @brief
 *  This function performs the write verify test. It does the following
 *  on a raid group:
 *  - Writes an initial data pattern.
 *  - Writes correctable and uncorrectable CRC error patterns on two VDs.
 *  - Performs a write verify operation.
 *  - Validates the error counts in the verify report.
 *  - Performs another write verify operation.
 *  - Validates that there are no new error counts in the verify report.
 * 
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void doctor_girlfriend_raid5_verify_test(fbe_u32_t in_index,
                                        fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_status_t                status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   raid_group_count    = 0;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "==== Testing automatic dirty verify during PACO ====");

    raid_group_count = fbe_test_get_rg_array_length(in_current_rg_config_p);
        
    doctor_girlfriend_start_io();

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Forcing drive into proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(in_current_rg_config_p,
                                                              raid_group_count,
                                                              DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_SPARE,
                                                              FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                              NULL /* The system selects the destination drives*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__);

    EmcutilSleep(1000);

    doctor_girlfriend_clear_verify_reports(in_current_rg_config_p);
    doctor_girlfriend_remove_and_reinsert_drives(in_current_rg_config_p);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(in_current_rg_config_p, 
                                                                          raid_group_count,
                                                                          DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_SPARE,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_FALSE, /* Don't wait for swap out*/
                                                                          FBE_TRUE   /* Skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    doctor_girlfriend_validate_verify_results(in_index, in_current_rg_config_p);

    doctor_girlfriend_clear_verify_reports(in_current_rg_config_p);

    return;

}   // End doctor_girlfriend_raid5_verify_test()


/*!**************************************************************************
 * doctor_girlfriend_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param void
 *
 * @return void
 *
 ***************************************************************************/
void doctor_girlfriend_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &fbe_doctor_girlfriend_test_context_g[0];

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* First fill the raid group with a pattern.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            DOCTOR_GIRLFRIEND_ELEMENT_SIZE);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "doctor_girlfriend background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End doctor_girlfriend_write_background_pattern()


/*!**************************************************************
 * doctor_girlfriend_start_io()
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Degrade the raid group.
 *  Allow I/O to continue degraded.
 *  Bring drive back and allow it to rebuild.
 *  Stop I/O.
 *
 * @param None.               
 *
 * @return None.
 *
 *
 ****************************************************************/

static void doctor_girlfriend_start_io(void)
{
    fbe_api_rdgen_context_t *context_p = &fbe_doctor_girlfriend_test_context_g[0];
    fbe_status_t status;

    fbe_bool_t non_cache_io = TRUE;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

    fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_WRITE_ONLY,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0, /* run forever*/ 
                                       8, /* threads */
                                       DOCTOR_GIRLFRIEND_MAX_IO_SIZE_BLOCKS,
                                       FBE_FALSE, /* Don't inject aborts */
                                       FBE_FALSE  /* Peer I/O not supported */);

    if(non_cache_io)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                            FBE_RDGEN_OPTIONS_NON_CACHED);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end doctor_girlfriend_start_io()
 ******************************************/


/*!**************************************************************
 * doctor_girlfriend_remove_and_reinsert_drives()
 ****************************************************************
 * @brief
 *  Remove drives and reinsert drives
 *
 *
 *
 * @return None.
 *
 *
 ****************************************************************/

static void doctor_girlfriend_remove_and_reinsert_drives(fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_status_t            status;
    fbe_api_rdgen_context_t *context_p = &fbe_doctor_girlfriend_test_context_g[0];
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Remove drives.
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive. ==\n", __FUNCTION__);
        fbe_test_sep_util_remove_drives(DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive successful. ==\n", __FUNCTION__);
    
    
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
        fbe_test_sep_util_remove_drives(DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive successful. ==\n", __FUNCTION__);
    }
    else
    {
        /* Not in simulation */
        fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;

        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive. ==\n", __FUNCTION__);
        drive_to_remove_p = &(rg_config_p->rg_disk_set[DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
        drive_to_remove_p = &(rg_config_p->rg_disk_set[DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, 20000, FBE_PACKAGE_ID_SEP_0);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==\n", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==\n", __FUNCTION__);

    /* Put the drives back in.
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive ==\n", __FUNCTION__);
        fbe_test_sep_util_insert_drives(DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive successful. ==\n", __FUNCTION__);
    
    
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive ==\n", __FUNCTION__);
        fbe_test_sep_util_insert_drives(DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive successful. ==\n", __FUNCTION__);
    }
    else
    {
        /* Not in simulation */
        fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive ==\n", __FUNCTION__);
        drive_to_insert_p = &(rg_config_p->rg_disk_set[DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive ==\n", __FUNCTION__);
        drive_to_insert_p = &(rg_config_p->rg_disk_set[DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Wait for pdo to be ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for first drive to be ready ==\n", __FUNCTION__);
    fbe_test_sep_util_wait_for_pdo_ready(DOCTOR_GIRLFRIEND_FIRST_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for first drive to be ready successful. ==\n", __FUNCTION__);


    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for second drive to be ready ==\n", __FUNCTION__);
    fbe_test_sep_util_wait_for_pdo_ready(DOCTOR_GIRLFRIEND_SECOND_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for second drive to be ready successful. ==\n", __FUNCTION__);


    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==\n", __FUNCTION__);

    fbe_test_sep_util_wait_for_all_objects_ready(in_current_rg_config_p);
    
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==\n", __FUNCTION__);

    return;

}/******************************************
 * end doctor_girlfriend_remove_and_reinsert_drives()
 ******************************************/

/*!**************************************************************
 * doctor_girlfriend_validate_verify_results()
 ****************************************************************
 * @brief
 *  Validate the background verify that was initiated for the dirty luns
 *
 * @param position_to_insert - The array index to insert.               
 *
 * @return None.
 *
 *
 ****************************************************************/

static void doctor_girlfriend_validate_verify_results(fbe_u32_t in_index,
                                              fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_object_id_t                             lun_object_id;              // LUN object to test
    fbe_u32_t                                   index;
    fbe_lun_verify_report_t                     verify_report[DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP]; // the verify report data
    fbe_test_logical_unit_configuration_t *     logical_unit_configuration_p = NULL;
    fbe_object_id_t                             rg_object_id;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);


    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (index =0; index < DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        doctor_girlfriend_wait_for_verify_completion(lun_object_id, logical_unit_configuration_p->lun_number, &verify_report[index], rg_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "coherency errors for lun %d is %d\n",index,
                   verify_report[index].current.correctable_coherency);

        // Validate that the correctable coherency error is not zero
        //MUT_ASSERT_TRUE((verify_report[index].current.correctable_coherency) != 0);

        logical_unit_configuration_p++;
    }
    return;

} // End doctor_girlfriend_validate_verify_results


/*!**************************************************************************
 * doctor_girlfriend_clear_verify_reports()
 ***************************************************************************
 * @brief
 *  This function clears the verify report
 * 
 * @param in_current_rg_config_p   - Pointer to the raid group
 *
 * @return void
 *
 ***************************************************************************/
static void doctor_girlfriend_clear_verify_reports(fbe_test_rg_configuration_t* in_current_rg_config_p)
                                                 
{
    fbe_object_id_t                             lun_object_id; 
    fbe_u32_t                                   index; 
    fbe_test_logical_unit_configuration_t *     logical_unit_configuration_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (index =0; index < DOCTOR_GIRLFRIEND_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
        logical_unit_configuration_p++;
    }

    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    return;

}   // End doctor_girlfriend_clear_verify_reports()

/*!**************************************************************
 * doctor_girlfriend_wait_for_verify_completion()
 ****************************************************************
 * @brief
 *  This function waits for the user verify operation to complete
 *  on the specified LUN.
 *
 * @param in_object_id      - pointer to LUN object
 *
 * @return void
 *
 ****************************************************************/
static void doctor_girlfriend_wait_for_verify_completion(fbe_object_id_t in_lun_object_id,
                                                     fbe_u32_t in_lun_number,
                                                     fbe_lun_verify_report_t*   in_verify_report_p,
                                                     fbe_object_id_t in_rg_object_id)
                                                    
{
    fbe_u32_t                              sleep_time;    
    fbe_bool_t                             is_msg_present;
    fbe_bool_t                             started = FBE_TRUE;
    fbe_status_t                           status;

    mut_printf(MUT_LOG_HIGH, "%s entry (LUN %d / Object 0x%X)", 
               __FUNCTION__, in_lun_number, in_lun_object_id);

    for (sleep_time = 0; sleep_time < (DOCTOR_GIRLFRIEND_MAX_VERIFY_WAIT_TIME_SECS*10); sleep_time++)
    {
        if(! started)
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                                 &is_msg_present, 
                                                 SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED,
                                                 in_lun_number,
                                                 in_lun_object_id
                                                 );

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if(is_msg_present)
            {
                mut_printf(MUT_LOG_HIGH, "%s LUN %d automatic verify started", __FUNCTION__, in_lun_number);
                started = FBE_TRUE;
            }
        }

        if(started)
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                                 &is_msg_present, 
                                                 SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE,
                                                 in_lun_number,
                                                 in_lun_object_id
                                                 );

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if(is_msg_present)
            {
                mut_printf(MUT_LOG_HIGH, "%s LUN %d automatic verify completed after %d msecs",
                           __FUNCTION__, in_lun_number, (sleep_time * 100));
                return;
            }
        }

        EmcutilSleep(100);
    }

    MUT_ASSERT_TRUE(0)
    return;

}   // End doctor_girlfriend_wait_for_verify_completion()

