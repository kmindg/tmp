/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file freddie_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the failed spare tests:
 *      o No spares available then new spare is inserted into system
 *      o Drive swaps in but then fails during rebuild
 *
 * HISTORY
 *  09/21/2012  Ron Proulx  - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_database_interface.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_event.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"
#include "fbe_test_common_utils.h"
#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "sep_hook.h"

char * freddie_short_desc = "This scenario test failures during drive sparing and the associated notifications.";
char * freddie_long_desc =
    "\n"
    "\n"
    "The Freddie Scenario tests what happens when a hot-spare drive fails.\n"
    "\n"
    "It covers the following cases:\n"
    "\t- Hot spare drive is removed while a rebuild is in progress.\n"
    "\t- The replacement/original drive is removed while an equalize is in progress.\n"
    "\t- Hot spare drive is removed while an equalize is in progress.\n"
    "\n"
    "Dependencies: \n"
    "\t- The Kilgore Trout scenario is a prerequisite to this scenario.\n"
    "\t- The Shaggy scenario is a prerequisite to this scenario.\n"
    "\t- The Diego scenario is a prerequisite to this scenario.\n"
    "\t- The FlimFlam scenario is a prerequisite to this scenario.\n"
    "\n"
    "Starting Config: \n"
    "\t[PP] an armada board\n"
    "\t[PP] a SAS PMC port\n"
    "\t[PP] a viper enclosure\n"
    "\t[PP] four SAS drives (PDO-A to PDO-D)\n"
    "\t[PP] four logical drives (LDO-A to LDO-D)\n"
    "\t[SEP] four provision drives (PVD-A to PVD-D)\n"
    "\t[SEP] two virtual drives (VD-A and VD-B)\n"
    "\t[SEP] a RAID-1 object (RG)\n"
    "\t[SEP] a LU object (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Create the provision drives with I/O edges to the logical drives.\n"
    "\t- Verify that the provision drives are in the READY state.\n"
    "\t- Create virtual drives (VD-A and VD-B) with I/O edges to PVD-A and PVD-B.\n"
    "\t- Verify that VD-A and VD-B are in the READY state.\n"
    "\t- Create a RAID-1 object (RG) with I/O edges to VD-A and VD-B.\n"
    "\t- Verify that RG is in the READY state.\n"
    "\t- Create an LU object (LU) with an I/O edge to RG.\n"
    "STEP 2: Fail a hot spare drive during a rebuild.\n"
    "\t- Perform rdgen writes to the LU.\n"
    "\t- Remove PDO-B.\n"
    "\t- Verify that VD-B is in the READY state and now has an I/O edge to PVD-C.\n"
    "\t- Verify that RG is generating rebuild I/O requests on VD-B.\n"
    "\t- Disable the sparing queue in the job service.\n"
    "\t- Remove PDO-C.\n"
    "\t- Verify that RG stopped/cleared the rebuild of VD-B.\n"
    "\t- Verify that RG is degraded.\n"
    "STEP 3: Fail the original drive during a equalize.\n"
    "\t- Enable the sparing queue in the job service.\n"
    "\t- Verify that VD-B is in the READY state and now has an I/O edge to PVD-D.\n"
    "\t- Verify that RG correctly rebuilt VD-B.\n"
    "\t- Reinsert PDO-B.\n"
    "\t- Verify that VD-B now has an I/O edge to PVD-B.\n"
    "\t- Verify that VD-B is generating equalize I/O requests on PVD-B.\n"
    "\t- Remove PDO-B.\n"
    "\t- Verify that VD-B is in the READY state but now does not have an I/O edge to PVD-B.\n"
    "\t- Verify that VD-B has terminated the equalize to PVD-B.\n"
    "STEP 4: Fail a hot spare drive during a equalize.\n"
    "\t- Reinsert PDO-B.\n"
    "\t- Verify that VD-B now has an I/O edge to PVD-B.\n"
    "\t- Verify that VD-B is generating equalize I/O requests on PVD-B.\n"
    "\t- Disable the sparing queue in the job service.\n"
    "\t- Remove PDO-D.\n"
    "\t- Verify that VD-B is in the ACTIVATE state.\n"
    "\t- Verify that RG is degraded.\n"
    ;


/*!*******************************************************************
 * @def FREDDIE_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the qual test. 
 *
 *********************************************************************/
#define FREDDIE_TEST_LUNS_PER_RAID_GROUP        3

/*********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FREDDIE_TEST_CHUNKS_PER_LUN             3

/*!*******************************************************************
 * @def FREDDIE_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief raid group count for all test levels
 *
 *********************************************************************/
#define FREDDIE_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def FREDDIE_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of our raid groups.
 *
 *********************************************************************/
#define FREDDIE_ELEMENT_SIZE                    1024

/*!*******************************************************************
 * @def FREDDIE_TEST_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define FREDDIE_TEST_WAIT_SEC                   60

#define FREDDIE_TEST_MAX_RETRIES                80     // retry count - number of times to loop

/*!*******************************************************************
 * @def FREDDIE_TEST_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define FREDDIE_TEST_WAIT_MSEC 1000 * FREDDIE_TEST_WAIT_SEC

/*!*******************************************************************
 * @def FREDDIE_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define FREDDIE_MAX_WIDTH                       16

/*!*******************************************************************
 * @def FREDDIE_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Number of blocks in small io
 *
 *********************************************************************/
#define FREDDIE_SMALL_IO_SIZE_BLOCKS            1024  

/*!*******************************************************************
 * @def FREDDIE_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define FREDDIE_MAX_IO_SIZE_BLOCKS              (FREDDIE_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @var freddie_io_msec_short
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `short' period of time.
 *
 *********************************************************************/
#define FREDDIE_SHORT_IO_TIME_SECS              5
static fbe_u32_t freddie_io_msec_short = (FREDDIE_SHORT_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var freddie_io_msec_long
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `long' period of time.
 *
 *********************************************************************/
#define FREDDIE_LONG_IO_TIME_SECS               30
static fbe_u32_t freddie_io_msec_long = (FREDDIE_LONG_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var freddie_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t freddie_threads = 2;

/*!*******************************************************************
 * @var freddie_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t freddie_test_contexts[FREDDIE_TEST_LUNS_PER_RAID_GROUP * FREDDIE_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @var freddie_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t freddie_raid_group_config[FREDDIE_TEST_RAID_GROUP_COUNT + 1] = 
{
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,         0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void freddie_run_tests(fbe_test_rg_configuration_t *rg_config_p,void * context_p);

/*!*******************************************************************
 * @var freddie_reboot_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t freddie_reboot_params = { 0 };

/*!**************************************************************
 * freddie_set_io_seconds()
 ****************************************************************
 * @brief
 *  Set the io seconds for this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/19/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void freddie_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= FREDDIE_LONG_IO_TIME_SECS)
    {
        freddie_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        freddie_io_msec_long  = ((fbe_random() % FREDDIE_LONG_IO_TIME_SECS) + 1) * 1000;
    }
    freddie_io_msec_short = ((fbe_random() % FREDDIE_SHORT_IO_TIME_SECS) + 1) * 1000;
    return;
}
/******************************************
 * end freddie_set_io_seconds()
 ******************************************/

/*!**************************************************************
 * freddie_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_write_background_pattern
 *
 ****************************************************************/
static void freddie_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &freddie_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing background pattern ==", 
               __FUNCTION__);

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            FREDDIE_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            FREDDIE_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end freddie_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          freddie_start_io()
 *****************************************************************************
 *
 * @brief   Start rdgen with the operation type, number of threads and block
 *          size requested.  If this is a dualsp test, start dualsp I/O.
 *
 * @param   ran_abort_msecs - Random time to inject aborts.  If aborts are 
 *              diabled - FBE_TEST_RANDOM_ABORT_TIME_INVALID
 * @param   rdgen_operation - The rdgen operation to run (usually w/r/c)
 * @param   threads - The number of threads to run
 * @param   io_size_blocks - The maximum I/O size to use
 * @param   b_only_run_on_sp_selected - If FBE_TRUE and dualsp mode is enabled
 *              only run the I/O on the SP indicated by sp_to_run_on.
 * @param   sp_to_run_on - If b_only_run_on_sp_selected is True, then only run
 *              I/O on this SP.
 *
 * @return  None
 *
 * @author
 *  10/31/2012  Ron Proulx  - Updated from scooby_doo_start_io
 *
 *****************************************************************************/
static void freddie_start_io(fbe_test_random_abort_msec_t ran_abort_msecs,
                             fbe_rdgen_operation_t rdgen_operation,
                             fbe_u32_t threads,
                             fbe_u32_t io_size_blocks,
                             fbe_bool_t b_only_run_on_sp_selected,
                             fbe_sim_transport_connection_target_t sp_to_run_on)

{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_bool_t                  b_ran_aborts_enabled = FBE_FALSE;
    fbe_bool_t                  b_dualsp_io = FBE_FALSE;
    fbe_sim_transport_connection_target_t this_sp; 
    fbe_sim_transport_connection_target_t other_sp; 

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start I/O ==", __FUNCTION__);

    /* Setup the abort mode
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  

    /* Configure the dualsp mode as required.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        /* If `b_only_run_on_sp_selected' only run on the selected SP.
         */
        if (b_only_run_on_sp_selected == FBE_TRUE)
        {
            /* Validate the `sp to run on'.
             */
            if ((sp_to_run_on != FBE_SIM_SP_A) &&
                (sp_to_run_on != FBE_SIM_SP_B)    )
            {
                /* Fail the request.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "== %s sp_to_run_on: %d invalid ==", __FUNCTION__, sp_to_run_on);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
                return;
            }

            /* Set the target server and configure the mode to single SP only.
             */
            b_dualsp_io = FBE_FALSE;
            status = fbe_api_sim_transport_set_target_server(sp_to_run_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_io_configure_dualsp_io_mode(b_dualsp_io, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            /* Else enable dualsp I/O.
             */
            b_dualsp_io = FBE_TRUE;
            status = fbe_test_sep_io_configure_dualsp_io_mode(b_dualsp_io, FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    
        /* Start the I/O.
         */
        fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           b_dualsp_io  /* Peer I/O  */);
    }
    else
    {
        /* Else don't configure dualsp mode.
         */
        fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           FBE_FALSE  /* Peer I/O not supported */);
    }

    /* inject random aborts if we are asked to do so
     */
    if (ran_abort_msecs!= FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, ran_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If we changed SPs change it back.
     */
    if (b_only_run_on_sp_selected == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
/******************************************
 * end freddie_start_io()
 ******************************************/

/*!***************************************************************************
 *          freddie_reboot()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t freddie_reboot(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_preserve_data = FBE_TRUE;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are in dualsp mode (the normal mode on the array), shutdown
     * the current SP to transfer control to the other SP.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        /* Mark `peer dead' so that we don't wait for notifications from the 
         * peer.
         */
        status = fbe_test_common_notify_mark_peer_dead(b_is_dualsp_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If both SPs are enabled shutdown this SP and validate that the other 
         * SP takes over.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp: %d shutdown SP: %d make current SP: %d ==", 
                   __FUNCTION__, b_is_dualsp_mode, this_sp, other_sp);

        /* Only use the `quick' destroy if `preserve data' is not set.
         */
        if (b_preserve_data == FBE_TRUE)
        {
            fbe_test_sep_util_destroy_neit_sep();
        }
        else
        {
            fbe_api_sim_transport_destroy_client(this_sp);
            fbe_test_sp_sim_stop_single_sp(this_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);
        }

        /* Set the transport server to the correct SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(this_sp, other_sp);
		/* In simulation it takes a very long time to flush jornal */
		/*
		04/02/13 13:41:56.819 INFO LIB  RAID    139 : wr_log hdr vd: s_id 0xb0, pos: 0x1, hdr st: 0, lba 0x0 cnt 0x0 ts: 0x0 0x0 wr_bm 0x0 
		04/02/13 13:41:56.962 INFO LIB  RAID    139 : wr_log hdr vd: s_id 0xb1, pos: 0x1, hdr st: 0, lba 0x0 cnt 0x0 ts: 0x0 0x0 wr_bm 0x0 
		04/02/13 13:41:57.005 INFO LIB  RAID    139 : wr_log hdr vd: s_id 0xb2, pos: 0x1, hdr st: 0, lba 0x0 cnt 0x0 ts: 0x0 0x0 wr_bm 0x0 
		04/02/13 13:41:57.157 INFO LIB  RAID    139 : wr_log hdr vd: s_id 0xb3, pos: 0x1, hdr st: 0, lba 0x0 cnt 0x0 ts: 0x0 0x0 wr_bm 0x0 
		*/
		/* Let's say 150 ms. per slot. * 256 = 38400 ms.
		*/
		fbe_api_sleep(60000);
    }
    else
    {
        /* Else simply reboot this SP. (currently no specific reboot params for NEIT).
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot current SP: %d ==", 
                   __FUNCTION__, this_sp);
        status = fbe_test_common_reboot_this_sp(sep_params_p, NULL /* No params for NEIT*/); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }  

    return status;
}
/******************************************
 * end freddie_reboot()
 ******************************************/

/*!***************************************************************************
 *          freddie_reboot_cleanup()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t freddie_reboot_cleanup(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are single SP mode reboot this SP.
     */
    if (b_is_dualsp_mode == FBE_FALSE)
    {
        /* If we are in single SP mode there currently is no cleanup neccessary.
         */
    }
    else
    {
        /* Mark the peer `alive' so that we wait for notifications on both SPs.
         */
        status = fbe_test_common_notify_mark_peer_alive(b_is_dualsp_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Else we are in dualsp mode. Boot the `other' SP and use/set the same
         * debug hooks that we have for this sp.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot other SP: %d ==", 
                   __FUNCTION__, other_sp);
        status = fbe_test_common_boot_sp(other_sp, sep_params_p, NULL /* No neit params*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }  

    return status;
}
/******************************************
 * end freddie_reboot_cleanup()
 ******************************************/

/*!**************************************************************
 * freddie_remove_sp()
 ****************************************************************
 * @brief
 *  Remove SP
 *
 *
 *
 * @return  status
 *
 *
 ****************************************************************/
static fbe_status_t freddie_remove_sp(fbe_sim_transport_connection_target_t which_sp)
{
    fbe_status_t                            status;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_is_dualsp_test;
    
    /* Get `this' and other SP.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    this_sp = which_sp;
    if (b_is_dualsp_test) {
        other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    } else {
        other_sp = this_sp;
    }

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    status = fbe_api_sim_transport_set_target_server(which_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_destroy_client(which_sp);
    fbe_test_sp_sim_stop_single_sp(which_sp == FBE_SIM_SP_A? FBE_TEST_SPA: FBE_TEST_SPB);

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(other_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    this_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(this_sp, other_sp);

    return status;
}
/******************************************
 * end freddie_remove_sp()
 ******************************************/

/*!**************************************************************
 * freddie_reinsert_sp()
 ****************************************************************
 * @brief
 *  Remove SP
 *
 *
 *
 * @return  status
 *
 *
 ****************************************************************/
static fbe_status_t freddie_reinsert_sp(fbe_sim_transport_connection_target_t which_sp, fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_status_t                    status;
    fbe_test_rg_configuration_t *   rg_config_p = NULL;
    fbe_u32_t num_raid_groups;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    rg_config_p = in_current_rg_config_p;

    fbe_test_base_suite_startup_single_sp(which_sp);

    status = fbe_api_sim_transport_set_target_server(which_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

    sep_config_load_sep_and_neit();

    //elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);

    return status;
}
/******************************************
 * end freddie_reinsert_sp()
 ******************************************/

/*!****************************************************************************
 * freddie_setup()
 ******************************************************************************
 * @brief
 *  Setup for a drive sparing test.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  4/10/2010 - Created. Dhaval Patel
 * 5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
void freddie_setup(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       num_raid_groups = 0;
   
    if (fbe_test_util_is_simulation())
    {
        /* Raid group config for all test levels.
         */
        rg_config_p = &freddie_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();

		/* Jornal flashing is very slow in a simulation.
			So we need to increase the rdgen timeout 
		*/
		fbe_api_rdgen_set_default_timeout(3 * 60000); /* 3 min timeout */

        /* After sep is loaded setup notifications.
         */
        fbe_test_common_setup_notifications(FBE_FALSE /* This is a single SP test*/);

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
        
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************************************************
 * end freddie_setup()
 ******************************************************************************/

/*!****************************************************************************
 * freddie_cleanup()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void freddie_cleanup(void)
{
    /* Destroy semaphore
     */
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************************************************
 * end freddie_cleanup()
 ******************************************************************************/

/*!****************************************************************************
 * freddie_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void freddie_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    
    /* Raid group config for all test levels.
     */
    rg_config_p = &freddie_raid_group_config[0];

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,freddie_run_tests,
                                   FREDDIE_TEST_LUNS_PER_RAID_GROUP,
                                   FREDDIE_TEST_CHUNKS_PER_LUN);
    return;    

}
/******************************************************************************
 * end freddie_test()
 ******************************************************************************/

/*!****************************************************************************
 * freddie_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void freddie_dualsp_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;

    /* Raid group config for all test levels.
     */
    rg_config_p = &freddie_raid_group_config[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,freddie_run_tests,
                                   FREDDIE_TEST_LUNS_PER_RAID_GROUP,
                                   FREDDIE_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end freddie_dualsp_test()
 ******************************************************************************/

/*!**************************************************************
 * freddie_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a Permanent Sparing  test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  5/30/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/
void freddie_dualsp_setup(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       num_raid_groups = 0;

    if (fbe_test_util_is_simulation())
    {
        /* Raid group config for all test levels.
         */
        rg_config_p = &freddie_raid_group_config[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();


		/* Jornal flashing is very slow in a simulation.
			So we need to increase the rdgen timeout 
		*/
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_rdgen_set_default_timeout(3 * 60000); /* 3 min timeout */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_rdgen_set_default_timeout(3 * 60000); /* 3 min timeout */

		/* The dafault target is SP_A */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		
        /* After sep is loaded setup notifications
         */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dual SP test*/);
       
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end freddie_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * freddie_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the freddie test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/30/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/

void freddie_dualsp_cleanup(void)
{
    /* Destroy semaphore
     */
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test*/);

    fbe_test_sep_util_print_dps_statistics();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
}
/******************************************
 * end freddie_dualsp_cleanup()
 ******************************************/

/*!***************************************************************************
 *          freddie_test_insert_spare_after_no_spares_available()
 *****************************************************************************
 *
 * @brief   Remove all spares remove drive from raid group, validate
 *          `no spare' job status then `create' a spare and validate
 *          spare swaps-in and is rebuilt.  
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   ran_abort_msecs - Determines if random aborts should be injected or not
 * @param   rdgen_operation - The type of rdgen operation to run (typically
 *              w/r/c).
 *
 * @return  None.
 *
 * @author
 * 09/21/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void freddie_test_insert_spare_after_no_spares_available(fbe_test_rg_configuration_t *const rg_config_p,
                                                                fbe_test_random_abort_msec_t ran_abort_msecs,
                                                                fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = 1;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   drives_to_remove = 1;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_bool_t                  b_event_found = FBE_FALSE;
    fbe_notification_info_t     notification_info;
    fbe_u32_t                   raid_type;

    /* Determine the raid group type value.
     */
    raid_type = (fbe_u32_t)rg_config_p->raid_type;
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        raid_type = 10;
    }

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start raid type: RAID-%d threads: %d max blocks: %d==", 
               __FUNCTION__, raid_type, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS);
    freddie_start_io(ran_abort_msecs, rdgen_operation, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS,
                     FBE_FALSE,             /* Allow I/O on both SPs*/
                     FBE_SIM_INVALID_SERVER /* Allow I/O on both SPs*/);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 2: Disable automatic sparing
     */
    status = fbe_api_provision_drive_disable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the vd information for the position to remove.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);

    /*! @note The virtual drive purposefully doesn't generate notification for
     *        errors that are not terminal (i.e. any error with `PRESENTLY' in
     *        will NOT generate a notification).  Therefore do NOT wait for
     *        the `FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE' 
     *        notification.
     */

    /* Step 3: Remove (1) drive from the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE,        /* We will be re-inserting this drive */
                               remove_delay,    /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);


    /* Step 6: Validate `no spare' event
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_WARN_SPARE_NO_SPARES_AVAILABLE,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Step 7: Set a debug hook when the old drive is swapped out.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Enable automatic sparing.
     */
    status = fbe_api_provision_drive_enable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Wait for the debug hook
     */
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0,
                                                 0); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Set the `permanent spare replacing' notification
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                  FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                  FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                  FBE_STATUS_OK,
                                                  FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Remove the debug hook
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Wait for a spare to be swapped in.  Since the `spare trigger
     *          interval' is (10) second it will take at least 10 seconds after
     *          reporting `no spare' before we swap-in a spare.  So wait (20)
     *          seconds.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                           30000, 
                                           &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13: Validate `permanent spare replacing' event
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Step 14: Wait for the rebuilds to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 15: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Step 16: Re-insert any removed drives
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Re-insert the removed drive */);
    fbe_api_sleep(1000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive - complete==", __FUNCTION__, drives_to_remove);

    fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
    fbe_test_wait_for_all_pvds_ready();

    /* Always refresh the extra disk info.
     */

    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    return;
}
/***********************************************************
 * end freddie_test_insert_spare_after_no_spares_available()
 ***********************************************************/

/*!***************************************************************************
 *          freddie_test_insert_spare_swaps_in_after_reboot()
 *****************************************************************************
 *
 * @brief   Validate that spare swaps-in if raid group is degraded after a reboot.  
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   ran_abort_msecs - Determines if random aborts should be injected or not
 * @param   rdgen_operation - The type of rdgen operation to run (typically
 *              w/r/c).
 *
 * @return  None.
 *
 * @author
 * 09/21/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void freddie_test_insert_spare_swaps_in_after_reboot(fbe_test_rg_configuration_t *const rg_config_p,
                                                            fbe_test_random_abort_msec_t ran_abort_msecs,
                                                            fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = 1;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   drives_to_remove = 1;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_bool_t                  b_event_found = FBE_FALSE;
    fbe_notification_info_t     notification_info;
    fbe_u32_t                   raid_type;
    fbe_bool_t                  b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t this_sp; 
    fbe_sim_transport_connection_target_t other_sp;
    fbe_sim_transport_connection_target_t active_sp; 
    fbe_sim_transport_connection_target_t passive_sp;  

    /* Get the vd information for the position to remove.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id); 

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine the raid group type value.
     */
    raid_type = (fbe_u32_t)rg_config_p->raid_type;
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        raid_type = 10;
    }

    /* Step 1: Start I/O on the peer (since this test shuts-down this SP)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start raid type: RAID-%d threads: %d max blocks: %d==", 
               __FUNCTION__, raid_type, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS);
    freddie_start_io(ran_abort_msecs, rdgen_operation,freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS,
                     FBE_TRUE,  /* Only allow I/O on the peer SP (if dualsp) */
                     passive_sp   /* Only run I/O on the passive*/);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 2: Disable automatic sparing on `this' SP
     */
    status = fbe_api_provision_drive_disable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Remove (1) drive from the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE,        /* We will be re-inserting this drive */
                               remove_delay,    /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Validate `no spare' event
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_WARN_SPARE_NO_SPARES_AVAILABLE,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);


    /* Step 6: Set the `trigger' spare timer on both SPs to 30 seconds
     *         since the timer should be updating on both.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(30);

    /* Step 7: Set the `permanent spare replacing' notification
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                  FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                  FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                  FBE_STATUS_OK,
                                                  FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8:  Reboot and validate that the permanent spare request proceeds.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP ==", __FUNCTION__);
    status = freddie_reboot(&freddie_reboot_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP - complete ==", __FUNCTION__);

    /* Step 9: Wait for the notification.  We need to add a lot of
     *          padding since the `trigger spare interval' is 10 seconds
     *          So (10) seconds to give up on copy and (10) seconds to
     *          swap-in spare.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Validate `permanent spare replacing' event
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Step 11: Validate that the job-in-progress flag is cleared.
     */
    status = fbe_test_sep_util_wait_for_swap_request_to_complete(vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12:  Perform any reboot cleanup required.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = freddie_reboot_cleanup(&freddie_reboot_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(1000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 12: Wait for the rebuilds to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* If this is a dualsp test make sure the target is `other'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 13: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* If this is a dualsp test make sure the target is `this'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 14: Re-insert any removed drives
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Re-insert the removed drive */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 15: Set the `trigger' spare timer on both SPs to 1
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
    fbe_test_wait_for_all_pvds_ready();
    /* Always refresh the extra disk info.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    return;
}
/***********************************************************
 * end freddie_test_insert_spare_swaps_in_after_reboot()
 ***********************************************************/

/*!***************************************************************************
 *          freddie_test_reboot_during_spare_complete()
 *****************************************************************************
 *
 * @brief   Validate that spare swaps-in if raid group is degraded after a reboot.  
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   ran_abort_msecs - Determines if random aborts should be injected or not
 * @param   rdgen_operation - The type of rdgen operation to run (typically
 *              w/r/c).
 *
 * @return  None.
 *
 * @author
 *  03/11/2014   Ron Proulx - Created.
 *
 *****************************************************************************/
static void freddie_test_reboot_during_spare_complete(fbe_test_rg_configuration_t *const rg_config_p,
                                                      fbe_test_random_abort_msec_t ran_abort_msecs,
                                                      fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = 1;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   drives_to_remove = 1;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_bool_t                  b_event_found = FBE_FALSE;
    fbe_notification_info_t     notification_info;
    fbe_u32_t                   raid_type;
    fbe_bool_t                  b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t this_sp; 
    fbe_sim_transport_connection_target_t other_sp;
    fbe_sim_transport_connection_target_t active_sp; 
    fbe_sim_transport_connection_target_t passive_sp;  

    /* Get the vd information for the position to remove.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);

    /* Get `this' and other SP.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;


    /* Determine the raid group type value.
     */
    raid_type = (fbe_u32_t)rg_config_p->raid_type;
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        raid_type = 10;
    }

    /* Step 1: Start I/O on the passive (since this test shuts-down this SP)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start raid type: RAID-%d threads: %d max blocks: %d==", 
               __FUNCTION__, raid_type, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS);
    freddie_start_io(ran_abort_msecs, rdgen_operation,freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS,
                     FBE_TRUE,  /* Only allow I/O on the peer SP (if dualsp) */
                     passive_sp   /* Only run I/O on the passive*/);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 2: Set the `trigger' spare timer on both SPs to 5 seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 3: Set a debug hook when the spare has been swapped in but before
     *         the commit.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                            0, 0,  
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4: Remove (1) drive from the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE,        /* We will be re-inserting this drive */
                               remove_delay,    /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Wait for the swap in complete hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "freddie validate cleanup: wait for swap complete hook vd obj: 0x%x",
               vd_object_id); 
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP,
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Set the `permanent spare replacing' notification
     */
    status = fbe_api_sim_transport_set_target_server(passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                  FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                  FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                  FBE_STATUS_OK,
                                                  FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6:  Reset (i.e. PANIC) the current SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s PANIC SP: %d ==", __FUNCTION__, this_sp);
    status = freddie_remove_sp(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s PANIC SP: %d  - complete ==", __FUNCTION__, this_sp);

    /* Step 6: Validate permanent spare swapped in
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Step 7: Wait for the notification.  We need to add a lot of
     *          padding since the `trigger spare interval' is 10 seconds
     *          So (10) seconds to give up on copy and (10) seconds to
     *          swap-in spare.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Validate `permanent spare replacing' event
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Step 9: Validate that the job-in-progress flag is cleared.
     */
    status = fbe_test_sep_util_wait_for_swap_request_to_complete(vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10:  Perform any reboot cleanup required.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = freddie_reboot_cleanup(&freddie_reboot_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(1000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 11: Wait for the rebuilds to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* If this is a dualsp test make sure the target is `other'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 13: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* If this is a dualsp test make sure the target is `this'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 14: Re-insert any removed drives
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Re-insert the removed drive */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 15: Set the `trigger' spare timer on both SPs to 1
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
    fbe_test_wait_for_all_pvds_ready();

    /* Always refresh the extra disk info.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    return;
}
/***********************************************************
 * end freddie_test_reboot_during_spare_complete()
 ***********************************************************/
/*!***************************************************************************
 * freddie_test_reboot_during_persist()
 *****************************************************************************
 * @brief   
 *  Reboot the active SP during the persist phase.
 *  Validate the peer takes over and finishes the job.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   ran_abort_msecs - Determines if random aborts should be injected or not
 * @param   rdgen_operation - The type of rdgen operation to run (typically
 *              w/r/c).
 *
 * @return  None.
 *
 * @author
 *  3/18/2014 - Created. Rob Foley
 *
 *****************************************************************************/
static void freddie_test_reboot_during_persist(fbe_test_rg_configuration_t *const rg_config_p,
                                               fbe_test_random_abort_msec_t ran_abort_msecs,
                                               fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = 1;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   drives_to_remove = 1;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_bool_t                  b_event_found = FBE_FALSE;
    fbe_u32_t                   raid_type;
    fbe_bool_t                  b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t this_sp; 
    fbe_sim_transport_connection_target_t other_sp;
    fbe_sim_transport_connection_target_t active_sp; 
    fbe_sim_transport_connection_target_t passive_sp;  

    /* Get the vd information for the position to remove.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);

    /* Get `this' and other SP.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;


    /* Determine the raid group type value.
     */
    raid_type = (fbe_u32_t)rg_config_p->raid_type;
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        raid_type = 10;
    }

    /* Step 1: Start I/O on the passive (since this test shuts-down this SP)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start raid type: RAID-%d threads: %d max blocks: %d==", 
               __FUNCTION__, raid_type, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS);
    freddie_start_io(ran_abort_msecs, rdgen_operation,freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS,
                     FBE_TRUE,  /* Only allow I/O on the peer SP (if dualsp) */
                     passive_sp   /* Only run I/O on the passive*/);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 2: Set the `trigger' spare timer on both SPs to 5 seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);
    /* 1. Add hook to pause db service during persist. */
    status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "== Database Persist Debug hook is now added ==\n");

    /* Step 4: Remove (1) drive from the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE,        /* We will be re-inserting this drive */
                               remove_delay,    /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_database_wait_hook(FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST, 
                                        FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "== database hook was triggered == \n");

    status = fbe_api_sim_transport_set_target_server(passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Step 9: Validate that the job-in-progress flag is cleared.
     */
    status = fbe_test_sep_util_wait_for_swap_request_to_complete(vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Validate permanent spare swapped in
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         FBE_TEST_WAIT_TIMEOUT_MS /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* We will boot up the other SP with drives inserted.  First insert drives for the remaining SP.
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Re-insert the removed drive */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting: %d removed drive - complete==", __FUNCTION__, drives_to_remove);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&freddie_raid_group_config[0], active_sp);

    /* Step 11: Wait for the rebuilds to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* If this is a dualsp test make sure the target is `other'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 13: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* If this is a dualsp test make sure the target is `this'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 15: Set the `trigger' spare timer on both SPs to 1
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
    fbe_test_wait_for_all_pvds_ready();

    /* Always refresh the extra disk info.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    return;
}
/***********************************************************
 * end freddie_test_reboot_during_persist()
 ***********************************************************/
/*!***************************************************************************
 *          freddie_test_validate_spare_job_cleanup_after_reboot()
 *****************************************************************************
 *
 * @brief   Validate that if the SP reboots AFTER the spare swaps-in but BEFORE
 *          the job cleanup is done, that the cleanup is called on the remaining
 *          SP.  
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   ran_abort_msecs - Determines if random aborts should be injected or not
 * @param   rdgen_operation - The type of rdgen operation to run (typically
 *              w/r/c).
 *
 * @return  None.
 *
 * @author
 *  10/03/2013   Ron Proulx - Created.
 *
 *****************************************************************************/
static void freddie_test_validate_spare_job_cleanup_after_reboot(fbe_test_rg_configuration_t *const rg_config_p,
                                                                 fbe_test_random_abort_msec_t ran_abort_msecs,
                                                                 fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = 1;
    fbe_api_rdgen_context_t    *context_p = &freddie_test_contexts[0];
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   drives_to_remove = 1;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             vd_object_id;
    fbe_u32_t                   raid_type;
    fbe_bool_t                  b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t this_sp; 
    fbe_sim_transport_connection_target_t other_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;


    /* Get the vd information for the position to remove.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);

    /* Determine the active and passive for the virtual drive.
     * Set the active SP to the active SP for the virtual drive.
     */
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();

    /* Determine the raid group type value.
     */
    raid_type = (fbe_u32_t)rg_config_p->raid_type;
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        raid_type = 10;
    }

    /* Step 1: Start I/O on the peer (since this test shuts-down this SP)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start raid type: RAID-%d threads: %d max blocks: %d==", 
               __FUNCTION__, raid_type, freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS);
    freddie_start_io(ran_abort_msecs, rdgen_operation,freddie_threads, FREDDIE_MAX_IO_SIZE_BLOCKS,
                     FBE_TRUE,  /* Only allow I/O on the peer SP (if dualsp) */
                     other_sp   /* Only run I/O on the peer*/);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 3: Remove (1) drive from the raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive start ==", __FUNCTION__, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE,        /* We will be re-inserting this drive */
                               remove_delay,    /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing: %d drive - complete==", __FUNCTION__, drives_to_remove);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Wait for the swap in complete hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "freddie validate cleanup: wait for swap complete hook vd obj: 0x%x",
               vd_object_id); 

    /* Step 6:  Reset (i.e. PANIC) the current SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s PANIC SP: %d ==", __FUNCTION__, this_sp);
    status = freddie_remove_sp(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s PANIC SP: %d  - complete ==", __FUNCTION__, this_sp);

    /* Step 7: Wait for the virtual drive to become Ready on the
     *         now active SP.
     */
    fbe_test_sep_wait_for_rg_objects_ready(rg_config_p);

    /* Step 8:  Perform any reboot cleanup required.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot SP: %d ==", __FUNCTION__, this_sp);
    status = freddie_reinsert_sp(this_sp, rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(1000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot SP: %d - complete ==", __FUNCTION__, this_sp);

    /* If this is a dualsp test make sure the target is `other'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 9: Wait for the rebuilds to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* If this is a dualsp test make sure the target is `other'.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(freddie_io_msec_short);

    /* Step 10: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Step 11: Set the `trigger' spare timer on both SPs to 1
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    /*! @note Since we `PANICed' the SP the drive locations may have changed.
     *        Therefore we cannot perform any cleanup!  This MUST be the last
     *        test for freddie!!
     */

    return;
}
/***********************************************************
 * end freddie_test_validate_spare_job_cleanup_after_reboot()
 ***********************************************************/

/*!***************************************************************************
 *          freddie_run_tests()
 *****************************************************************************
 *
 * @brief   Run spare `failure' tests for each raid type.  Since each test
 *          validates notifications etc, we only test (1) raid group at a time.
 *          I/O is running against the raid group during the test.  Automatic
 *          sparing is enabled.
 *
 * @param   rg_config_p - Pointer to array of raid groups to tests.
 * @param   
 *
 * @return None.
 *
 * @author
 *  09/21/2012  Ron Proulx  - Created from shaggy_test.
 *
 *****************************************************************************/
void freddie_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       raid_group_count = 0;
    fbe_u32_t                       rg_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_bool_t                      b_dualsp_test_mode = fbe_test_sep_util_get_dualsp_test_mode();
    
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);

    /*! @note Automatic sparing is enabled
     */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Initialize out `short' and `long' I/O times
     */
    freddie_set_io_seconds();

    /* Write the background pattern to seed all luns with background pattern
     */
    freddie_write_background_pattern();

    /* Set the sparing trigger time to a short value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* For all raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* Only run test on enabled raid groups.
         */
        if (fbe_test_rg_config_is_enabled(current_rg_config_p) == FBE_FALSE) {
            current_rg_config_p++;
            continue;
        }

        /*! @note Sparing operations (of any type) are not supported for RAID-0 raid groups.
         *        Therefore skip these tests if the raid group type is RAID-0.  
        */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) {
            /* Print a message and return.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Sparing operations not supported on RAID-0 ==",
                   __FUNCTION__);
            current_rg_config_p++;
            continue;         
        }

        /* Test 1: Remove all spares remove drive from raid group, validate
         *         `no spare' job status then `create' a spare and validate
         *         spare swaps-in and is rebuilt.
         */
        freddie_test_insert_spare_after_no_spares_available(current_rg_config_p,
                                                            FBE_TEST_RANDOM_ABORT_TIME_INVALID, /* No aborts*/
                                                            FBE_RDGEN_OPERATION_WRITE_READ_CHECK);

        /* Test 2: Validate that spare swaps-in if raid group is degraded after
         *         a reboot.
         */
        if (test_level > 0) {
            freddie_test_insert_spare_swaps_in_after_reboot(current_rg_config_p,
                                                            FBE_TEST_RANDOM_ABORT_TIME_INVALID, /* No aborts*/
                                                            FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
        }

        /*! @note These test are only significant when running dualsp. 
         */
        if (b_dualsp_test_mode == FBE_TRUE) {
            /* Test 3: Validate that spare job continues and cleans-up
             *         on passive SP if active SP reboot just as the permanent
             *         spare job completes.
             */
            freddie_test_reboot_during_persist(current_rg_config_p,
                                               FBE_TEST_RANDOM_ABORT_TIME_INVALID, /* No aborts*/
                                               FBE_RDGEN_OPERATION_WRITE_READ_CHECK);

            /* This test is similar to the one above, so commenting it out*/
            /*freddie_test_reboot_during_spare_complete(current_rg_config_p,
                                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK);*/

            /*! @note This MUST be the last test!  Do not add any more test after this 
             *        one.
             */
            /* Test 4: If this is a dualsp test and the raid type is RAID-1
             *         reboot the CMI active SP (which by definition is the virtual
             *         drive active SP) AFTER a permanent spare has swapped-in
             *         but BEFORE the job completes.
             */
            if (test_level > 0) {
                freddie_test_validate_spare_job_cleanup_after_reboot(rg_config_p,
                                                             FBE_TEST_RANDOM_ABORT_TIME_INVALID, /* No aborts*/
                                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
            }

        } /* end if dualsp test mode */

        /* Goto the next raid group under test.
         */
        current_rg_config_p++;

    } /* end for all raid groups under test*/

    fbe_api_sleep(5000); /* 5 Sec. delay */

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);


    /* Done.
     */
    return;
}
/*********************************** 
 * end freddie_run_tests()
 ***********************************/


/*************************
 * end file freddie_test.c
 *************************/
