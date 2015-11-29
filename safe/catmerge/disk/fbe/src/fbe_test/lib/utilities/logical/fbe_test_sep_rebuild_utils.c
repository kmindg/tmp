/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_rebuild_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the rebuild functionality of the
 *  Storage Extent Package (SEP).
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "sep_rebuild_utils.h"
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "sep_hook.h"                               //  hook APIs
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config
#include "fbe/fbe_api_cmi_interface.h"              // To check peer status

//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//  Slot of the next Hot Spare that is available for use.  One enclosure is allocated for Hot Spares and any
//  test can configure a HS in the next availabe slot.
fbe_u32_t   sep_rebuild_utils_next_hot_spare_slot_g;

//  Number of physical objects in the test configuration.  This is needed for determining if drive has been
//  fully removed (objects destroyed) or fully inserted (objects created).
fbe_u32_t   sep_rebuild_utils_number_physical_objects_g;

//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!**************************************************************************
 *  sep_rebuild_utils_write_bg_pattern
 ***************************************************************************
 * @brief
 *   This function writes a data pattern to the LUNs/RAID groups specified
 *   by the given context.
 *
 * @param in_test_context_p     - pointer to context, which is an array with
 *                                an entry for each LUN.  It is empty when
 *                                passed in.
 * @param in_element_size       - element size
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_write_bg_pattern(
                                fbe_api_rdgen_context_t*    in_test_context_p,
                                fbe_u32_t                   in_element_size)
{

    //  Set up the context to do a write (fill) operation
    fbe_test_sep_io_setup_fill_rdgen_test_context(in_test_context_p, FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LUN,
         FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_LBA_INVALID, in_element_size);

    //  Write a background pattern to all of the LUNs in the entire configuration
    fbe_api_rdgen_test_context_run_all_tests(in_test_context_p, FBE_PACKAGE_ID_NEIT, 1);

    //  Make sure there were no errors
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);

    //  Return
    return;

}   // End sep_rebuild_utils_write_bg_pattern()


/*!**************************************************************************
 *  sep_rebuild_utils_read_bg_pattern
 ***************************************************************************
 * @brief
 *   This function reads a data pattern from the LUNs/RAID groups specified
 *   by the given context.
 *
 * @param in_test_context_p     - pointer to context, which is an array with
 *                                an entry for each LUN
 * @param in_element_size       - element size
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_read_bg_pattern(
                                fbe_api_rdgen_context_t*    in_test_context_p,
                                fbe_u32_t                   in_element_size)
{

    //  Set up the context to do a read and check operation
    fbe_test_sep_io_setup_fill_rdgen_test_context(in_test_context_p, FBE_OBJECT_ID_INVALID, FBE_CLASS_ID_LUN,
         FBE_RDGEN_OPERATION_READ_CHECK, FBE_LBA_INVALID, in_element_size);

    //  Read the data and check it, for all of the LUNs in the configuration
    fbe_api_rdgen_test_context_run_all_tests(in_test_context_p, FBE_PACKAGE_ID_NEIT, 1);

    //  Make sure there were no errors
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);

    //  Return
    return;

}   // End sep_rebuild_utils_read_bg_pattern()

/*!**************************************************************************
 *  sep_rebuild_utils_check_for_degraded
 ***************************************************************************
 * @brief
 *   This function checks to ensure the rebuild check point has been reset
 *   to 0.  It is passed a RG configuration data structure from which it determines the object id.
 *   Then it calls a function to actually check for the checkpoint.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 * @param in_tmo_ms                 - How long to wait for degraded in milliseconds
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_check_for_degraded(fbe_test_rg_configuration_t  *in_rg_config_p,
                                          fbe_u32_t                     in_position,
                                          fbe_u32_t                     in_tmo_ms)
{
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                       adjusted_position;              //  disk position, adjusted if need for RAID-10
    fbe_status_t                    status;                         //  fbe status
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command
    fbe_u32_t                       wait_interval_ms = 1000;
    fbe_u32_t                       total_wait_time_ms = 0;
    fbe_u32_t                       timeout_ms;
    fbe_bool_t                      b_raid_group_degraded = FBE_FALSE;
    fbe_bool_t                      b_checkpoint_zero = FBE_FALSE;
    fbe_bool_t                      b_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t this_sp;       
    fbe_sim_transport_connection_target_t other_sp;       
    fbe_sim_transport_connection_target_t active_sp;     

    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);
   
    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {    
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
           &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    // Configure dualsp mode
    //
    this_sp = fbe_api_sim_transport_get_target_server();
    active_sp = this_sp;
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_dualsp_mode == FBE_TRUE)                                                    &&
        (raid_group_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)    )
    {
        active_sp = other_sp;
        fbe_api_sim_transport_set_target_server(active_sp);
    }

    /* Generate proper timeout
     */
    if (in_tmo_ms < wait_interval_ms)
    {
        timeout_ms = wait_interval_ms + wait_interval_ms;
    }
    else
    {
        timeout_ms = in_tmo_ms / wait_interval_ms;
        timeout_ms = (timeout_ms * wait_interval_ms) + wait_interval_ms;
    }

    /* While we either validated that we are degraded or the timeout is reached.
     */
    while ((b_raid_group_degraded == FBE_FALSE) &&
           (total_wait_time_ms <= timeout_ms)      )
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%x ==",
                raid_group_object_id, adjusted_position, (unsigned int)raid_group_info.rebuild_checkpoint[adjusted_position]);
    
        if (raid_group_info.rebuild_checkpoint[adjusted_position] == 0)
        {
            MUT_ASSERT_TRUE(raid_group_info.b_rb_logging[in_position] == FBE_TRUE);
            if (b_checkpoint_zero == FBE_TRUE)
            {
                b_raid_group_degraded = FBE_TRUE;
                break;
            }
            else
            {
                b_checkpoint_zero = FBE_TRUE;
            }
        }

        /* Always wait a second time */
        fbe_api_sleep(wait_interval_ms);
        total_wait_time_ms += wait_interval_ms;
    }

    /* Validate that the raid group went and stayed degraded
     */
    if (b_raid_group_degraded == FBE_FALSE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: rg obj: 0x%x did not go/stay degraded after: %d seconds\n", 
                   __FUNCTION__, raid_group_object_id, (total_wait_time_ms / 1000));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    // Change back to original SP
    //
    if ((b_dualsp_mode == FBE_TRUE) &&
        (active_sp != this_sp)         )
    {
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    //  Return
    return;

}   // End sep_rebuild_utils_check_for_degraded()

/*!**************************************************************************
 *  sep_rebuild_utils_check_for_reb_restart
 ***************************************************************************
 * @brief
 *   This function checks to ensure the rebuild check point has been reset
 *   to 0.  It is passed a RG configuration data structure from which it determines the object id.
 *   Then it calls a function to actually check for the checkpoint.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_check_for_reb_restart(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position)
{
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                       adjusted_position;              //  disk position, adjusted if need for RAID-10
    fbe_status_t                    status;                         //  fbe status
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
            &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    //  Get the raid group information
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW,
           "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==",
            raid_group_object_id, adjusted_position,
        (unsigned long long)raid_group_info.rebuild_checkpoint[adjusted_position]);

    if (raid_group_info.rebuild_checkpoint[adjusted_position] != 0)
    {
        MUT_FAIL_MSG("The rebuild checkpoint is not equal to 0!\n");
    }

    //  Return
    return;

}   // End sep_rebuild_utils_check_for_reb_restart()


/*!**************************************************************************
 *  sep_rebuild_utils_check_for_reb_in_progress
 ***************************************************************************
 * @brief
 *   This function checks to ensure the rebuild check point has been reset
 *   to 0.  It is passed a RG configuration data structure from which it determines the object id.
 *   Then it calls a function to actually check for the checkpoint.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_check_for_reb_in_progress(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position)
{
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                       adjusted_position;              //  disk position, adjusted if need for RAID-10
    fbe_status_t                    status;                         //  fbe status
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
            &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    //  Get the raid group information
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==",
            raid_group_object_id, adjusted_position,
            (unsigned long long)raid_group_info.rebuild_checkpoint[adjusted_position]);

    if (raid_group_info.rebuild_checkpoint[adjusted_position] == 0)
    {
        MUT_FAIL_MSG("The rebuild is not in progress! (checkpoint = 0)\n");
    }

    //  Return
    return;

}   // End sep_rebuild_utils_check_for_reb_in_progress()

/*!**************************************************************************
 *  sep_rebuild_utils_get_reb_checkpoint
 ***************************************************************************
 * @brief
 *   This function gets the rebuild check point.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 * @param out_checkpoint            - the rebuild checkpoint
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_get_reb_checkpoint(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position,
                                    fbe_lba_t*                      out_checkpoint)
{
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                       adjusted_position;              //  disk position, adjusted if need for RAID-10
    fbe_status_t                    status;                         //  fbe status
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
            &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    //  Get the raid group information
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==",
            raid_group_object_id, adjusted_position,
            (unsigned long long)raid_group_info.rebuild_checkpoint[adjusted_position]);

    *out_checkpoint = raid_group_info.rebuild_checkpoint[adjusted_position];

    //  Return
    return;

}   // End sep_rebuild_utils_get_reb_checkpoint()

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_comp
 ***************************************************************************
 * @brief
 *   This function waits for the rebuild to complete.  It is passed a RG
 *   configuration data structure from which it determines the object id.
 *   Then it calls a function to actually wait for the rebuild.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_wait_for_rb_comp(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position)
{
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                   adjusted_position;              //  disk position, adjusted if need for RAID-10


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
            &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    //  Wait for the rebuild to complete
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(raid_group_object_id, adjusted_position);

    //  Return
    return;

}   // End sep_rebuild_utils_wait_for_rb_comp()


/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_comp_by_obj_id
 ***************************************************************************
 * @brief
 *   This function waits for the rebuild to complete.
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_wait_for_rb_comp_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position)
{

    fbe_status_t                    status;                         //  fbe status
    fbe_u32_t                       count;                          //  loop count
    fbe_u32_t                       max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command


    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    //  Loop until the the rebuild checkpoint is set to the logical end marker for the drive that we are rebuilding
    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%llx ==", 
                   in_raid_group_object_id, in_position,
           (unsigned long long)raid_group_info.rebuild_checkpoint[in_position]);

        //  If the rebuild checkpoint is set to the logical marker for the given disk, then the rebuild has
        //  finished - return here
        if (raid_group_info.rebuild_checkpoint[in_position] == FBE_LBA_INVALID)
        {
            return;
        }

        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }

    //  The rebuild operation did not finish within the time limit - assert
    MUT_ASSERT_TRUE(0);

    //  Return (should never get here)
    return;

}   // End sep_rebuild_utils_wait_for_rb_comp_by_obj_id()

/*!**************************************************************************
 *  sep_rebuild_utils_check_bits
 ***************************************************************************
 * @brief
 *   This function checks the bits after a rebuild completes.
 *
 * @param in_raid_group_object_id   - raid group's object id
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_check_bits(fbe_object_id_t in_raid_group_object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_raid_group_get_paged_info_t     paged_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the current and peer sp.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the paged bits on this SP
     */
    status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If the rebuild is done, no chunks should be needing a rebuild.
     */
    if (paged_info.num_nr_chunks > 0)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s rg obj: 0x%x there are %lld chunks marked NR on %s",
                   __FUNCTION__, in_raid_group_object_id,
                   (unsigned long long)paged_info.num_nr_chunks,
                   (this_sp == FBE_SIM_SP_A) ? "SPA" : "SPB");
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return;
    }

    /* In the case of a dual sp test we will also check the peer since
     * some of this information can be cached on the peer.  We want to make sure it is correct there also.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode() &&
        cmi_info.peer_alive                         )
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(this_sp);
        if (paged_info.num_nr_chunks > 0)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s rg obj: 0x%x there are %lld chunks marked NR on %s",
                   __FUNCTION__, in_raid_group_object_id,
                   (unsigned long long)paged_info.num_nr_chunks,
                   (this_sp == FBE_SIM_SP_A) ? "SPA" : "SPB");
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s rg obj: 0x%x there are no chunks marked NR",
               __FUNCTION__, in_raid_group_object_id);
    return;
} // End sep_rebuild_utils_check_bits()

/*!****************************************************************************
 *  sep_rebuild_utils_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the rebuild related tests.  It creates the
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        //  Load the physical package and create the physical configuration.  We are leveraging the function
        //  from the Elmo test to do so.
        elmo_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        //  Load the logical packages
        sep_config_load_sep_and_neit();
    }

    //  Initialize the next hot spare slot to 0
    sep_rebuild_utils_next_hot_spare_slot_g = 0;

    //  Return
    return;

} //  End sep_rebuild_utils_setup()

/*!****************************************************************************
 *  sep_rebuild_utils_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the rebuild related tests for dual-SP
 *   testing.  It creates the physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_dualsp_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        //  Set the target server to SPA
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        //  Load the physical package and create the physical configuration.  We are leveraging the function
        //  from the Elmo test to do so.
        elmo_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        //  Set the target server to SPB
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        //  Load the physical package and create the physical configuration.  We are leveraging the function
        //  from the Elmo test to do so.
        elmo_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        //  Load the logical packages on both SPs; setting target server to SPA for consistency during testing
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    //  Initialize the next hot spare slot to 0
    sep_rebuild_utils_next_hot_spare_slot_g = 0;

    //  The following utility function does some setup for hardware; noop for simulation
    fbe_test_common_util_test_setup_init();

    //  Return
    return;

} //  End sep_rebuild_utils_dualsp_setup()

/*!****************************************************************************
 *  sep_rebuild_utils_setup_hot_spare
 ******************************************************************************
 *
 * @brief
 *   This function creates a Hot Spare and waits for it to become ready.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_setup_hot_spare(
                                            fbe_u32_t           in_port_num,
                                            fbe_u32_t           in_encl_num,
                                            fbe_u32_t           in_slot_num)
{
    //  Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    fbe_test_sep_util_configure_drive_as_hot_spare(in_port_num, in_encl_num, in_slot_num);

    //  Log a trace
    mut_printf(MUT_LOG_TEST_STATUS, "%s: %d-%d-%d Success\n", __FUNCTION__, in_port_num, in_encl_num, in_slot_num);

    //  Return
    return;

} //  End sep_rebuild_utils_setup_hot_spare()

/*!****************************************************************************
 *  sep_rebuild_utils_start_rb_logging_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   Test that rebuild logging is set for a drive when the drive is removed from
 *   a RAID-5, RAID-3, or RAID-1 RG (when the RG is enabled).
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - position of disk to remove
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_start_rb_logging_r5_r3_r1(
                        fbe_test_rg_configuration_t*            in_rg_config_p,
                        fbe_u32_t                               in_position,
                        fbe_api_terminator_device_handle_t*     out_drive_info_p)
{

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing first drive in the RG\n", __FUNCTION__);

    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove a drive in the RG and verify the drive state changes
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_drive_info_p);

    //  Verify that the RAID and LUN objects are in the READY state
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

    //  Verify that rebuild logging is set for the drive and that the rebuild checkpoint is 0
    sep_rebuild_utils_verify_rb_logging(in_rg_config_p, in_position);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the READY state
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

        //  Verify that rebuild logging is set for the drive and that the rebuild checkpoint is 0
        sep_rebuild_utils_verify_rb_logging(in_rg_config_p, in_position);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End sep_rebuild_utils_start_rb_logging_r5_r3_r1()

/*!****************************************************************************
 *  sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   Test that the RG is enabled/ready and rebuild logging is cleared when the
 *   first drive to be removed is re-inserted in a RAID-5, RAID-3 or RAID-1.
 *
 * @param in_rg_config_p    - pointer to the RG configuration info
 * @param in_position           - position of disk to remove
 * @param in_drive_info_p   - pointer to structure with the "drive info" for the
 *                            drive
 *
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_u32_t                               in_position,
                            fbe_api_terminator_device_handle_t*     in_drive_info_p)
{

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinserting first drive in the RG\n", __FUNCTION__);

    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

    //  Reinsert the first drive to fail and verify drive state changes
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p,
                                                in_position,
                                                sep_rebuild_utils_number_physical_objects_g,
                                                in_drive_info_p);

    //  Verify that the RAID and LUN objects are in the READY state
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

    //  Verify that rebuild logging is cleared for the drive
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the READY state
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

        //  Verify that rebuild logging is cleared for the drive
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1()


/*!****************************************************************************
 *  sep_rebuild_utils_triple_mirror_r6_second_drive_removed
 ******************************************************************************
 *
 * @brief
 *   Test that rebuild logging is set for a drive when it is the second drive to
 *   be removed from a triple mirror RG or RAID-6 RG.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_triple_mirror_r6_second_drive_removed(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_api_terminator_device_handle_t*     out_drive_info_p)
{

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing second drive in a triple mirror\n", __FUNCTION__);

    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove a drive in the RG and verify the drive state changes
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_drive_info_p);

    //  Verify that the RAID and LUN objects are in the READY state
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

    //  Verify that rebuild logging is set for the drive and that the rebuild checkpoint is 0
    sep_rebuild_utils_verify_rb_logging(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the READY state
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

        //  Verify that rebuild logging is set for the drive and that the rebuild checkpoint is 0
        sep_rebuild_utils_verify_rb_logging(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End sep_rebuild_utils_triple_mirror_r6_second_drive_removed()


/*!****************************************************************************
 *  sep_rebuild_utils_triple_mirror_r6_second_drive_restored
 ******************************************************************************
 *
 * @brief
 *   Test that the RG is enabled/ready and rebuild logging is cleared when the
 *   second drive to be removed is re-inserted in a triple mirror RAID-1
 *   or RAID-6 RG.
 *
 * @param in_rg_config_p    - pointer to the RG configuration info
 * @param in_drive_info_p   - pointer to structure with the "drive info" for the
 *                            drive
 *
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_triple_mirror_r6_second_drive_restored(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_api_terminator_device_handle_t*     in_drive_info_p)
{

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinserting second drive in a triple mirror\n", __FUNCTION__);

    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

    //  Reinsert the second drive to fail and verify drive state changes
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p,
                                                SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS,
                                                sep_rebuild_utils_number_physical_objects_g,
                                                in_drive_info_p);

    //  Verify that the RAID and LUN objects are in the READY state
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

    //  Verify that rebuild logging is cleared for the drive
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the READY state
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

        //  Verify that rebuild logging is cleared for the drive
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End sep_rebuild_utils_triple_mirror_r6_second_drive_restored()

void sep_rebuild_utils_remove_drive_by_location(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot,
                                                fbe_api_terminator_device_handle_t * out_drive_info_p)
{
    fbe_status_t                        status;
    fbe_object_id_t                     pvd_object_id;
    
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s:remove the drive port: %d encl: %d slot: %d",
                __FUNCTION__, bus, encl, slot);

     status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            encl,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (fbe_test_util_is_simulation())
    {
        /* remove the drive. */
        status = fbe_test_pp_util_pull_drive(bus,
                                             encl,
                                             slot,
                                             out_drive_info_p); 
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;

            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();

            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }

            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            /* remove the drive. */
            status = fbe_test_pp_util_pull_drive(bus,
                                                 encl,
                                                 slot,
                                                 out_drive_info_p);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        /* remove the drive on actual hardware. */
        status = fbe_test_pp_util_pull_drive_hw(bus,
                                                encl,
                                                slot);

     }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* verify that the pvd and vd objects are in the FAIL state. */
    shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    return;
}
/******************************************************************************
 * end shaggy_remove_drive()
 ******************************************************************************/

/*!****************************************************************************
 *  sep_rebuild_utils_remove_drive_and_verify
 ******************************************************************************
 *
 * @brief
 *    This function removes a drive.  First it waits for the logical and physical
 *    drive objects to be destroyed.  Then checks the object states of the
 *    PVD and VD for that drive are Activate.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_remove_drive_and_verify(
                                fbe_test_rg_configuration_t*        in_rg_config_p,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p)
{
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             raid_group_object_id;               // raid group's object id
    fbe_object_id_t             pvd_object_id;                      // pvd's object id
    fbe_object_id_t             vd_object_id;                       // vd's object id
    fbe_sim_transport_connection_target_t   local_sp;               // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id


    //  Get the ID of the local SP. 
    local_sp = fbe_api_sim_transport_get_target_server();

    //  Get the ID of the peer SP. 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    //  Get the PVD object id before we remove the drive
    status = fbe_api_provision_drive_get_obj_id_by_location(in_rg_config_p->rg_disk_set[in_position].bus,
                                                            in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                            in_rg_config_p->rg_disk_set[in_position].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the drive
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                             in_rg_config_p->rg_disk_set[in_position].enclosure,
                                             in_rg_config_p->rg_disk_set[in_position].slot,
                                             out_drive_info_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            //  Set the target server to the peer and remove the drive there. 
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                                 in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                 in_rg_config_p->rg_disk_set[in_position].slot,
                                                 out_drive_info_p);

            //  Set the target server back to the local SP. 
            fbe_api_sim_transport_set_target_server(local_sp);
        }
        
        //  Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
        //  value that is passed in. Note that drives are removed differently on hardware; this check applies
        //  to simulation only.
        status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);       
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                                in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                in_rg_config_p->rg_disk_set[in_position].slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            //  Set the target server to the peer and remove the drive there. 
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                                    in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                    in_rg_config_p->rg_disk_set[in_position].slot);

            //  Set the target server back to the local SP.
            fbe_api_sim_transport_set_target_server(local_sp);
        }        
    }

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
               in_rg_config_p->rg_disk_set[in_position].bus,
               in_rg_config_p->rg_disk_set[in_position].enclosure,
               in_rg_config_p->rg_disk_set[in_position].slot);

    //  Get the RG's object id.  We need to this get the VD's object it.
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  Get the VD's object id
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "RG: 0x%X, PVD: 0x%X, VD: 0x%X\n", 
               raid_group_object_id, pvd_object_id, vd_object_id);

    //  Verify that the PVD and VD objects are in the FAIL state
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        //  Set the target server to the peer and do verification there.
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the PVD and VD objects are in the FAIL state
        sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
        sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

        //  Set the target server back to the local SP.
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} // End sep_rebuild_utils_remove_drive_and_verify()

/*!****************************************************************************
 *  sep_rebuild_utils_remove_hs_drive
 ******************************************************************************
 *
 * @brief
 *    This function removes a hs drive.  First it waits for the logical and physical
 *    drive objects to be destroyed.  Then checks the object states of the
 *    PVD and VD for that drive are Activate.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_remove_hs_drive(
                                fbe_u32_t                           bus,
                                fbe_u32_t                           enclosure,
                                fbe_u32_t                           slot,
                                fbe_object_id_t                     raid_group_object_id,
                                fbe_u32_t                           in_position,
                                fbe_u32_t                           in_num_objects,
                                fbe_api_terminator_device_handle_t* out_drive_info_p)
{
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             pvd_object_id;                      // pvd's object id
    fbe_object_id_t             vd_object_id;                       // vd's object id


    //  Get the PVD object id before we remove the drive
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the drive
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, out_drive_info_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        //  Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
        //  value that is passed in. Note that drives are removed differently on hardware; this check applies to
        //  simulation only.
        status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    //  Get the VD's object id
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);

    //  Verify that the PVD and VD objects are in the FAIL state
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    //  Return
    return;

} // End sep_rebuild_utils_remove_hs_drive()

/*!****************************************************************************
 *  sep_rebuild_utils_reinsert_drive_and_verify
 ******************************************************************************
 *
 * @brief
 *   This function re-inserts a drive.  First it waits for the logical and physical
 *   drive objects to be created.  Then it checks the object states of the
 *   PDO, LDO, PVD, and VD are Ready.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param in_drive_info_p       - pointer to structure with the "drive info" for the
 *                                drive
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_reinsert_drive_and_verify(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_u32_t                           in_position,
                                        fbe_u32_t                           in_num_objects,
                                        fbe_api_terminator_device_handle_t* in_drive_info_p)
{

    fbe_sim_transport_connection_target_t   local_sp;                       // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                        // peer SP id


    //  Get the ID of the local SP.
    local_sp = fbe_api_sim_transport_get_target_server();

    //  Get the ID of the peer SP. 
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    //  Insert the drive
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_reinsert_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                        in_rg_config_p->rg_disk_set[in_position].enclosure,
                                        in_rg_config_p->rg_disk_set[in_position].slot,
                                        *in_drive_info_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            //  Set the target server to the peer and reinsert the drive there. 
            fbe_api_sim_transport_set_target_server(peer_sp);

            fbe_test_pp_util_reinsert_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                            in_rg_config_p->rg_disk_set[in_position].enclosure,
                                            in_rg_config_p->rg_disk_set[in_position].slot,
                                            *in_drive_info_p);

            //  Set the target server back to the local SP. 
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        fbe_test_pp_util_reinsert_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                           in_rg_config_p->rg_disk_set[in_position].enclosure,
                                           in_rg_config_p->rg_disk_set[in_position].slot);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            //  Set the target server to the peer and reinsert the drive there. 
            fbe_api_sim_transport_set_target_server(peer_sp);

            fbe_test_pp_util_reinsert_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                               in_rg_config_p->rg_disk_set[in_position].enclosure,
                                               in_rg_config_p->rg_disk_set[in_position].slot);

            //  Set the target server back to the local SP. 
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted Drive: %d_%d_%d\n", 
               in_rg_config_p->rg_disk_set[in_position].bus,
               in_rg_config_p->rg_disk_set[in_position].enclosure,
               in_rg_config_p->rg_disk_set[in_position].slot);

    sep_rebuild_utils_verify_reinserted_drive(in_rg_config_p, in_position, in_num_objects, in_drive_info_p);

    //  Return
    return;

} // End sep_rebuild_utils_reinsert_drive_and_verify()


/*!****************************************************************************
 *  sep_rebuild_utils_verify_reinserted_drive
 ******************************************************************************
 *
 * @brief
 *   This function waits for the logical and physical
 *   drive objects to be created.  Then it checks the object states of the
 *   PDO, LDO, PVD, and VD are Ready.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 * @param in_drive_info_p       - pointer to structure with the "drive info" for the
 *                                drive
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_verify_reinserted_drive(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_u32_t                           in_position,
                                        fbe_u32_t                           in_num_objects,
                                        fbe_api_terminator_device_handle_t* in_drive_info_p)
{
    fbe_status_t                        status;                             // fbe status
    fbe_object_id_t                     raid_group_object_id;               // raid group's object id
    fbe_object_id_t                     vd_object_id;                       // vd's object id
    fbe_sim_transport_connection_target_t   local_sp;                       // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                        // peer SP id

    //  Get the ID of the local SP.
    local_sp = fbe_api_sim_transport_get_target_server();

    //  Get the ID of the peer SP.
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    //  Verify the PDO and LDO are created by waiting for the number of physical objects to be equal to the
    //  value that is passed in.  Note that drives are removed differently on hardware; this check applies
    //  to simulation only.
    status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Verify the PDO and LDO are in the READY state
    fbe_test_pp_util_verify_pdo_state(in_rg_config_p->rg_disk_set[in_position].bus,
                                in_rg_config_p->rg_disk_set[in_position].enclosure,
                                in_rg_config_p->rg_disk_set[in_position].slot,
                                FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);

    // Verify that PVD is in the READY state
    fbe_test_sep_drive_verify_pvd_state(in_rg_config_p->rg_disk_set[in_position].bus,
                                                            in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                            in_rg_config_p->rg_disk_set[in_position].slot, 
                                                            FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);

    //  Get the RG's object id.  We need to this get the VD's object it.
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  Get the VD's object id
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);

    //  Verify that the VD object is in the READY state
    sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);  

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        //  Set the target server to the peer and do verification there. 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify the PDO and LDO are in the READY state
        fbe_test_pp_util_verify_pdo_state(in_rg_config_p->rg_disk_set[in_position].bus,
                                    in_rg_config_p->rg_disk_set[in_position].enclosure,
                                    in_rg_config_p->rg_disk_set[in_position].slot,
                                    FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);

        //  Verify that the PVD and VD objects are in the READY state
        fbe_test_sep_drive_verify_pvd_state(in_rg_config_p->rg_disk_set[in_position].bus,
                                                                in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                                in_rg_config_p->rg_disk_set[in_position].slot, 
                                                                FBE_LIFECYCLE_STATE_READY, SEP_REBUILD_UTILS_WAIT_MSEC);

        sep_rebuild_utils_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);  

        //  Set the target server back to the local SP. 
        fbe_api_sim_transport_set_target_server(local_sp);
    }
}

/*!**************************************************************************
 *  sep_rebuild_utils_verify_rb_logging
 ***************************************************************************
 * @brief
 *   This function waits for rebuild logging to be set and the checkpoint to be
 *   0. It is passed a RG configuration data structure from which it determines
 *   the RG object id. Then it calls a function to actually check the values.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_verify_rb_logging(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        fbe_u32_t                       in_position)
{
    fbe_object_id_t                     raid_group_object_id;           //  raid group's object id


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  If the RAID group is a R10 RG, rebuild logging will be set in the underlying mirror object,
    //  not the striper object.  For R10, need to get the object ID of the appropriate mirror object
    //  and adjust the position for that object.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_object_id_t     mirror_raid_group_object_id;
        fbe_u32_t           position;

        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &mirror_raid_group_object_id, &position);

        raid_group_object_id    = mirror_raid_group_object_id;
        in_position             = position;
    }

    //  Wait for rebuild logging to be set
    sep_rebuild_utils_verify_rb_logging_by_object_id(raid_group_object_id, in_position);

    //  Return
    return;

}   // End sep_rebuild_utils_verify_rb_logging()

/*!**************************************************************************
 *  sep_rebuild_utils_verify_raid_groups_rb_logging
 ***************************************************************************
 * @brief
 *   This function waits for rebuild logging to be set and the checkpoint to be
 *   0. It is passed a RG configuration data structure from which it determines
 *   the RG object id. Then it calls a function to actually check the values.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 *
 * @return status
 *
 ***************************************************************************/
fbe_status_t sep_rebuild_utils_verify_raid_groups_rb_logging(fbe_test_rg_configuration_t *in_rg_config_p,
                                                     fbe_u32_t  raid_group_count,
                                                     fbe_u32_t  in_position)
{
    fbe_test_rg_configuration_t    *current_rg_config_p = in_rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id

    // For all raid groups
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        //  Get the RG object id from the RG number
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &raid_group_object_id);
    
        //  If the RAID group is a R10 RG, rebuild logging will be set in the underlying mirror object,
        //  not the striper object.  For R10, need to get the object ID of the appropriate mirror object
        //  and adjust the position for that object.
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_object_id_t     mirror_raid_group_object_id;
            fbe_u32_t           position;
    
            sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(current_rg_config_p, in_position, &mirror_raid_group_object_id, &position);
    
            raid_group_object_id    = mirror_raid_group_object_id;
            in_position             = position;
        }
    
        //  Wait for rebuild logging to be set
        sep_rebuild_utils_verify_rb_logging_by_object_id(raid_group_object_id, in_position);

        // goto next raid group
        current_rg_config_p++;
    }

    //  Return
    return FBE_STATUS_OK;

}   // End sep_rebuild_utils_verify_raid_groups_rb_logging()

/*!**************************************************************************
 *  sep_rebuild_utils_verify_rb_logging_by_object_id
 ***************************************************************************
 * @brief
 *   This function waits for rebuild logging to be set and the checkpoint to be
 *   0.
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_verify_rb_logging_by_object_id(
                                        fbe_object_id_t                 in_raid_group_object_id,
                                        fbe_u32_t                       in_position)
{

    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t       raid_group_info;                //  rg's information from "get info" command


    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    //  Loop until the drive has been marked rebuild logging and the rebuild checkpoint is 0
    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  Check if rebuild logging is set and the rebuild checkpoint is 0
        //! @todo - temporary change while rebuild logging is using non-paged MD service but rebuild checkpoint
        //  is not yet
        if ((raid_group_info.b_rb_logging[in_position] == FBE_TRUE) ) //&&
        //    (raid_group_info.rebuild_checkpoint[in_position] == 0))
        {
            return;
        }

        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }

    //  rebuild logging was not marked within the time limit - assert
    MUT_ASSERT_TRUE(0);

    //  Return (should never get here)
    return;

}   // End sep_rebuild_utils_verify_rb_logging_by_object_id()


/*!**************************************************************************
 *  sep_rebuild_utils_verify_not_rb_logging
 ***************************************************************************
 * @brief
 *   This function verifies that rebuild logging is not set.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_verify_not_rb_logging(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        fbe_u32_t                       in_position)
{

    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t       raid_group_info;                //  rg's information from "get info" command
    fbe_object_id_t                     raid_group_object_id;           //  raid group's object id


    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify rebuild logging NOT set on position %d in the RG\n", __FUNCTION__, in_position);

    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  If the RAID group is a R10 RG, rebuild logging will be set in the underlying mirror object,
    //  not the striper object.  For R10, need to get the object ID of the appropriate mirror object
    //  and adjust the position for that object.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_object_id_t     mirror_raid_group_object_id;
        fbe_u32_t           position;

        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &mirror_raid_group_object_id, &position);

        raid_group_object_id    = mirror_raid_group_object_id;
        in_position             = position;
    }

    //  Loop until the rebuild logging has been cleared
    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  Check if rebuild logging is clear
        if (raid_group_info.b_rb_logging[in_position] == FBE_FALSE)
        {
            return;
        }

        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }

    //  rebuild logging was not cleared within the time limit - assert
    MUT_ASSERT_TRUE(0);

    //  Return (should never get here)
    return;

}   // End sep_rebuild_utils_verify_not_rb_logging()

/*!**************************************************************************
 *  sep_rebuild_utils_verify_no_raid_groups_are_rb_logging
 ***************************************************************************
 * @brief
 *   This function verifies that rebuild logging is not set.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return status
 *
 ***************************************************************************/
fbe_status_t sep_rebuild_utils_verify_no_raid_groups_are_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_u32_t position)
{
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;

    /* For all raid groups
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        sep_rebuild_utils_verify_not_rb_logging(current_rg_config_p, position);

        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    return FBE_STATUS_OK;
} // end sep_rebuild_utils_verify_no_raid_groups_are_rb_logging()


/*!**************************************************************************
 *  sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10
 ***************************************************************************
 * @brief
 *   This function returns the object ID for the mirror object in the given
 *   R10 RG that corresponds to the given disk position.  Both the object
 *   ID of the mirror object and the position within that mirror object
 *   are returned.
 *
 * @param in_rg_config_p                    - pointer to the RG configuration info
 * @param in_position                       - disk position in the RG
 * @param out_mirror_raid_group_object_id_p - pointer to object ID of R10 mirror object
 * @param out_position_p                    - pointer to disk position in the R10 mirror RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(
                                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                                    fbe_u32_t                       in_position,
                                                    fbe_object_id_t*                out_mirror_raid_group_object_id_p,
                                                    fbe_u32_t*                      out_position_p)
{
    fbe_status_t                                    status;                     //  fbe status
    fbe_object_id_t                                 raid_group_object_id;       //  raid group's object id
    fbe_api_base_config_downstream_object_list_t    downstream_object_list;     //  object list for R10 RG
    fbe_u32_t                                       index;                      //  index into object list for R10


    //  Confirm the RG type is R10
    MUT_ASSERT_INT_EQUAL(FBE_RAID_GROUP_TYPE_RAID10, in_rg_config_p->raid_type);

    //  Get the object ID of the R10 RG; this is the striper RG object ID
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_object_id);

    //  Get the downstream object list; this is the list of mirror objects for this R10 RG
    fbe_test_sep_util_get_ds_object_list(raid_group_object_id, &downstream_object_list);

    //  Get the index into the downstream object list for the mirror object corresponding
    //  to the incoming position
    index = in_position / 2;

    //  Get the object ID of the corresponding mirror object
    *out_mirror_raid_group_object_id_p = downstream_object_list.downstream_object_list[index];
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *out_mirror_raid_group_object_id_p);

    //  Get the position in mirror object
    *out_position_p = in_position % 2;

    //  Return
    return;

}   // End sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10()


/*!****************************************************************************
 *  sep_rebuild_utils_check_raid_and_lun_states
 ******************************************************************************
 *
 * @brief
 *   This function checks that the RAID group and all its LUNs are in the
 *   given lifecycle state.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_state              - expected state
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_check_raid_and_lun_states(
                        fbe_test_rg_configuration_t*        in_rg_config_p,
                        fbe_lifecycle_state_t               in_state)
{
    fbe_object_id_t                                         raid_group_object_id;   //  raid group's object id
    fbe_u32_t                                               lun_index;              //  index of current LUN
    fbe_object_id_t                                         lun_object_id;          //  object id of current LUN


    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: check state of RG and its LUNs\n", __FUNCTION__);

    //  Get the object ID of the raid group
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  Verify the raid group's state
    sep_rebuild_utils_verify_sep_object_state(raid_group_object_id, in_state);

    //  Loop through all of the LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the object ID of this LUN
        fbe_api_database_lookup_lun_by_number(
            in_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, &lun_object_id);

        //  Verify the LUN's state
        sep_rebuild_utils_verify_sep_object_state(lun_object_id, in_state);
    }

    //  Return
    return;

} //  End sep_rebuild_utils_check_raid_and_lun_states()


/*!****************************************************************************
 *  sep_rebuild_utils_verify_sep_object_state
 ******************************************************************************
 *
 * @brief
 *   This function checks that an object is in the given lifecycle state.
 *
 * @param in_object_id          - object ID
 * @param in_state              - expected state
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_verify_sep_object_state(
                        fbe_object_id_t         in_object_id,
                        fbe_lifecycle_state_t   in_state)
{
    fbe_status_t        status;                 // fbe status

    //  Check that the object ID is valid
    MUT_ASSERT_INT_NOT_EQUAL(in_object_id, FBE_OBJECT_ID_INVALID);

    //  Verify that the object is in the given state
    //  for parity RG, we has to left enough time to wait for "JOURNAL FLUSH condition" complete
    //  change the wait time from 30s to 60s
    status = fbe_api_wait_for_object_lifecycle_state(in_object_id, in_state, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: 0x%X is in expected state (%s)",
               __FUNCTION__, in_object_id, fbe_api_state_to_string(in_state));

    //  Return
    return;

} //  Ene sep_rebuild_utils_verify_sep_object_state()


/*!****************************************************************************
 *  sep_rebuild_utils_get_drive_loc_for_specific_rg_pos
 ******************************************************************************
 *
 * @brief
 *   This function is used to get the port, enclosure, slot information for the
 *   specific drive position of the RAID group.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param drive_rg_position  - position of tdrive in RAID group.
 * @param out_bus_num_p      - pointer to bus number, get filled with bus number.
 * @param out_encl_num_p     - pointer to encl number, get filled with encl number.
 * @param out_slot_num_p     - pointer to slot number, get filled with slot number.
 *
 * @return  None
 *
 *****************************************************************************/
void sep_rebuild_utils_get_drive_loc_for_specific_rg_pos(fbe_test_rg_configuration_t*    in_rg_config_p,
                                                      fbe_u32_t                       drive_rg_position,
                                                      fbe_u32_t*                      out_bus_num_p,
                                                      fbe_u32_t*                      out_encl_num_p,
                                                      fbe_u32_t*                      out_slot_num_p)
{
    fbe_status_t                        status;
    fbe_api_get_block_edge_info_t       block_edge_info;
    fbe_object_id_t                     raid_group_object_id;
    fbe_object_id_t                     vd_object_id;


    //  Get the RG's object id.  We need to this get the VD's object it.
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  Get the virtual drive object-id for the specific drive position of the RAID group.
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, drive_rg_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the block edge information of the virtual drive object to get the server object id.
    status = fbe_api_get_block_edge_info(vd_object_id, 0, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Verify whether we have valid server object id (pvd object id).
    MUT_ASSERT_INT_NOT_EQUAL(block_edge_info.server_id, FBE_OBJECT_ID_INVALID);

    //  Get the pvd object id for the specific drive rg position.
    status = fbe_api_provision_drive_get_location(block_edge_info.server_id, out_bus_num_p, out_encl_num_p, out_slot_num_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Return
    return;

} //  End sep_rebuild_utils_get_drive_loc_for_specific_rg_pos()

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_to_start
 ***************************************************************************
 * @brief
 *   This function waits for the rebuild to start.  It is passed a RG
 *   configuration data structure from which it determines the object id.
 *   Then it calls a function to actually wait for the rebuild.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_wait_for_rb_to_start(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_position)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 raid_group_object_id;           //  raid group's object id
    fbe_u32_t                       adjusted_position;              //  disk position, adjusted if need for RAID-10
    fbe_bool_t                      b_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t this_sp;       
    fbe_sim_transport_connection_target_t other_sp;       
    fbe_sim_transport_connection_target_t active_sp; 
    fbe_api_raid_group_get_info_t   raid_group_info;


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
    //  object.   Otherwise use the object id already found and the input position.
    if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(in_rg_config_p, in_position, &raid_group_object_id,
            &adjusted_position);
    }
    else
    {
        adjusted_position = in_position;
    }

    // Configure dualsp mode
    //
    this_sp = fbe_api_sim_transport_get_target_server();
    active_sp = this_sp;
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    if (b_dualsp_mode == FBE_TRUE)
    {
        status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (raid_group_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
        {
            active_sp = other_sp;
            fbe_api_sim_transport_set_target_server(active_sp);
        }
    }

    //  Wait for the rebuild to complete
    sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(raid_group_object_id, adjusted_position);

    // Change target back if required
    if ((b_dualsp_mode == FBE_TRUE) &&
        (this_sp != active_sp)         )
    {
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    //  Return
    return;

}   // End sep_rebuild_utils_wait_for_rb_to_start()

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_any_rb_to_start
 ***************************************************************************
 * @brief
 *   This function waits for a rebuild to start on any position in this rg.
 *
 * @param rg_config_p            - pointer to the RG configuration info
 * @param position_p             - disk position in the RG where rb started.
 *
 * @return void
 *
 ***************************************************************************/
fbe_status_t sep_rebuild_utils_wait_for_any_rb_to_start(fbe_test_rg_configuration_t* rg_config_p,
                                                        fbe_raid_position_t * position_p)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_object_id_t                 rg_object_id;
    fbe_raid_position_t position;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    for (count = 0; count < max_retries; count++)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        for (position = 0; position < raid_group_info.width; position++)
        {
            //  If the rebuild checkpoint is greater than 0 and not -1 (complete), then it means the rebuild is
            //  in progress on this position.  return the position.
            if ( (raid_group_info.rebuild_checkpoint[position] > 0) &&
                 (raid_group_info.rebuild_checkpoint[position] != FBE_LBA_INVALID) )
            {
                mut_printf(MUT_LOG_LOW, "== rebuild checkpoint rg_obj: 0x%x pos: 0x%x is at 0x%llx ==\n", 
                           rg_object_id, position, raid_group_info.rebuild_checkpoint[position]);
                *position_p = position;
                return FBE_STATUS_OK;
            }
        }

        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    mut_printf(MUT_LOG_LOW, "== no rebuilds found for rg_id: 0x%x ==\n", rg_object_id);
    return FBE_STATUS_GENERIC_FAILURE;
}
// End sep_rebuild_utils_wait_for_any_rb_to_start()

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_logging_clear
 ***************************************************************************
 * @brief
 *   This function waits for rebuild logging to get cleared for this rg.
 *
 * @param in_rg_config_p            - pointer to the RG configuration info
 *
 * @return void
 *
 ***************************************************************************/
fbe_status_t sep_rebuild_utils_wait_for_rb_logging_clear(fbe_test_rg_configuration_t* rg_config_p)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_object_id_t                 rg_object_id;
    fbe_raid_position_t position;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    for (count = 0; count < max_retries; count++)
    {
        fbe_bool_t b_rebuild_logging_cleared = FBE_TRUE;
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* For all positions look to see if we are rebuild logging. 
         */ 
        for (position = 0; position < raid_group_info.width; position++)
        {
            if (raid_group_info.b_rb_logging[position] == FBE_TRUE)
            {
                mut_printf(MUT_LOG_LOW, "== rb logging not cleared rg_obj: 0x%x pos: 0x%x is at chkpt: 0x%llx ==\n",
                           rg_object_id, position,
               (unsigned long long)raid_group_info.rebuild_checkpoint[position]);
                b_rebuild_logging_cleared = FBE_FALSE;
                break;
            }
        }
        if (b_rebuild_logging_cleared == FBE_TRUE)
        {
            /* No rebuild logging set, waiting is done, just return.
             */
            return FBE_STATUS_OK;
        }

        /* Continue waiting for rb logging.
         */
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    mut_printf(MUT_LOG_LOW, "== no rebuilds found for rg_id: 0x%x ==\n", rg_object_id);
    return FBE_STATUS_GENERIC_FAILURE;
}
// End sep_rebuild_utils_wait_for_rb_logging_clear()

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_logging_clear_for_position
 ***************************************************************************
 * @brief
 *   This function waits for rebuild logging to get cleared for the specified
 *   raid group object and position.
 *
 * @param   rg_object_id - The object id of the raid group to check
 * @param   position - The position in the raid group to wait for rebuild
 *                      logging to be clear.
 *
 * @return  status
 *
 ***************************************************************************/
fbe_status_t sep_rebuild_utils_wait_for_rb_logging_clear_for_position(fbe_object_id_t rg_object_id,
                                                                      fbe_u32_t position)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;

    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    for (count = 0; count < max_retries; count++)
    {
        fbe_bool_t b_rebuild_logging_cleared = FBE_TRUE;
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Check if rebuild logging is clear for this position.
         */ 
        if (raid_group_info.b_rb_logging[position] == FBE_TRUE)
        {
            mut_printf(MUT_LOG_LOW, "== rb logging not cleared rg_obj: 0x%x pos: 0x%x is at chkpt: 0x%llx ==\n",
                       rg_object_id, position,
                       (unsigned long long)raid_group_info.rebuild_checkpoint[position]);
            b_rebuild_logging_cleared = FBE_FALSE;
        }
        if (b_rebuild_logging_cleared == FBE_TRUE)
        {
            /* No rebuild logging set, waiting is done, just return.
             */
            return FBE_STATUS_OK;
        }

        /* Continue waiting for rb logging.
         */
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    mut_printf(MUT_LOG_LOW, "== no rebuilds found for rg_id: 0x%x ==\n", rg_object_id);
    return FBE_STATUS_GENERIC_FAILURE;
}
// End sep_rebuild_utils_wait_for_rb_logging_clear_for_position

/*!**************************************************************************
 *  sep_rebuild_utils_wait_for_rb_to_start_by_obj_id
 ***************************************************************************
 * @brief
 *   This function waits for the rebuild to start.
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param in_position               - disk position in the RG
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position)
{

    fbe_status_t                    status;                         //  fbe status
    fbe_u32_t                       count;                          //  loop count
    fbe_u32_t                       max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command


    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    //  Loop until the the rebuild checkpoint is set to the logical end marker for the drive that we are rebuilding
    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_LOW, "== rebuild checkpoint is at 0x%llx ==",
           (unsigned long long)raid_group_info.rebuild_checkpoint[in_position]);

        //  If the rebuild checkpoint is greater than 0 or equal to -1 (the rebuild completed) - return here
        if ( raid_group_info.rebuild_checkpoint[in_position] > 0 ||
             raid_group_info.rebuild_checkpoint[in_position] == -1 )
        {
            return;
        }

        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }

    //  The rebuild operation did not finish within the time limit - assert
    MUT_FAIL_MSG("The rebuild did not start within the time limit!\n");

    //  Return (should never get here)
    return;

}   // End sep_rebuild_utils_wait_for_rb_to_start_by_obj_id()

/*!****************************************************************************
 *  sep_rebuild_utils_get_random_drives
 ******************************************************************************
 *
 * @brief
 *   This function randomly selects the specified number of slots from the given
 *   config.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_drives_to_remove   - the number of slots to pick
 * @param out_slots_p           - pointer to the slot array
 *
 * @return None
 *
 *****************************************************************************/
void sep_rebuild_utils_get_random_drives(fbe_test_rg_configuration_t*    in_rg_config_p,
                                   fbe_u32_t                       in_drives_to_remove,
                                   fbe_u32_t*                      out_slots_p)
{
    fbe_u32_t slot;
    fbe_bool_t new_slot = FBE_TRUE;
    fbe_u32_t i;
    fbe_u32_t j;

    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if (in_drives_to_remove > in_rg_config_p->width)
    {
        MUT_FAIL_MSG("There are not enough drives in the RG to remove!\n");
    }

    for (i=0; i<in_drives_to_remove; i++)
    {
        do
        {
            /* Get a random slot */
            slot = (fbe_random() % (in_rg_config_p->width));
            out_slots_p[i] = slot;
            new_slot = FBE_TRUE;
            for (j=0; j<i; j++)
            {
                /* If we have already chosen this slot, then choose a new one. */
                if (slot == out_slots_p[j])
                {
                    new_slot = FBE_FALSE;
                    break;
                }
            }
        } while (!new_slot);
    }
} // End sep_rebuild_utils_get_random_drives()

/*!****************************************************************************
 *  sep_rebuild_utils_remove_drive_no_check
 ******************************************************************************
 *
 * @brief
 *   This function removes a drive and waits for the logical and physical
 *   drive objects to be destroyed.  It does not check the states of the VD or
 *   the PVD.
 *
 * @param in_port_num           - port/bus number of the drive to remove
 * @param in_encl_num           - enclosure number of the drive to remove
 * @param in_slot_num           - slot number of the drive to remove
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param unused                - pointer to drive handle
 *
 * @return None
 *
 *
 *****************************************************************************/
void sep_rebuild_utils_remove_drive_no_check(
                                        fbe_u32_t                           in_port_num,
                                        fbe_u32_t                           in_encl_num,
                                        fbe_u32_t                           in_slot_num,
                                        fbe_u32_t                           in_num_objects,
                                        fbe_api_terminator_device_handle_t  *unused)
{

    fbe_status_t                        status;                     // fbe status

    //  Remove the drive
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(in_port_num, in_encl_num, in_slot_num, unused);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        //  Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
        //  value that is passed in. Note that drives are removed differently on hardware; this check applies to
        //  simulation only.
        status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(in_port_num, in_encl_num, in_slot_num);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    //  Return
    return;

} // End sep_rebuild_utils_remove_drive_no_check()

/*!****************************************************************************
 * sep_rebuild_utils_reinsert_removed_drive()
 ******************************************************************************
 * @brief
 *   This function reinsert the removed  drive and waits for the logical and physical
 *   drive objects to be ready.  It does not check the states of the VD .
 *
 *
 * @param in_port_num           - port/bus number of the drive to remove
 * @param in_encl_num           - enclosure number of the drive to remove
 * @param in_slot_num           - slot number of the drive to remove
 * @param unused                - pointer to drive handle
 *
 * @return none.
 *
 ******************************************************************************/

void sep_rebuild_utils_reinsert_removed_drive(fbe_u32_t                   in_port_num,
                                              fbe_u32_t                   in_encl_num,
                                              fbe_u32_t                   in_slot_num,
                                              fbe_api_terminator_device_handle_t  *drive_handle)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Insert removed drive back. */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(in_port_num,
                                                 in_encl_num,
                                                 in_slot_num,
                                                 *drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;

            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();

            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }

            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);
            status = fbe_test_pp_util_reinsert_drive(in_port_num,
                                                     in_encl_num,
                                                     in_slot_num,
                                                     *drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(in_port_num,
                                                    in_encl_num,
                                                    in_slot_num);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    //  Print message as to know where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted Drive: %d_%d_%d\n", 
               in_port_num, in_encl_num, in_slot_num);

    /* Wait for the unused pvd object to be in ready state. */
    status = fbe_test_sep_util_wait_for_pvd_state(in_port_num,
                                                  in_encl_num,
                                                  in_slot_num,
                                                  FBE_LIFECYCLE_STATE_READY,
                                                  60000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

/*!****************************************************************************
 * sep_rebuild_utils_reinsert_removed_drive_no_check()
 ******************************************************************************
 * @brief
 *   This function reinsert the removed drive.  It does not check the states of the VD .
 *
 *
 * @param in_port_num           - port/bus number of the drive to remove
 * @param in_encl_num           - enclosure number of the drive to remove
 * @param in_slot_num           - slot number of the drive to remove
 * @param unused                - pointer to drive handle
 *
 * @return none.
 *
 ******************************************************************************/

void sep_rebuild_utils_reinsert_removed_drive_no_check(fbe_u32_t                           in_port_num,
                                                       fbe_u32_t                           in_encl_num,
                                                       fbe_u32_t                           in_slot_num,
                                                       fbe_api_terminator_device_handle_t  *drive_handle)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Insert removed drive back. */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(in_port_num,
                                                 in_encl_num,
                                                 in_slot_num,
                                                 *drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;

            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();

            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }

            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);
            status = fbe_test_pp_util_reinsert_drive(in_port_num,
                                                     in_encl_num,
                                                     in_slot_num,
                                                     *drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(in_port_num,
                                                    in_encl_num,
                                                    in_slot_num);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}


/*!****************************************************************************
 * sep_rebuild_utils_find_free_drive()
 ******************************************************************************
 * @brief
 *  Finds the next free drive . This drive will be used as  a hot spare
 *
 * @param free_drive_pos_p          - will keep the free drive position
 * @param disk_set_p                - pointer to available free disks location
 * @param hs_count                  - how many hot spares have we configured so far
 *                                    (how many free drives have we used)
 *
 * @return FBE_STATUS.
 *
 ******************************************************************************/

fbe_status_t sep_rebuild_utils_find_free_drive(fbe_test_raid_group_disk_set_t *free_drive_pos_p,
                                              fbe_test_raid_group_disk_set_t *disk_set_p,
                                              fbe_u32_t hs_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if(hs_count < FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        disk_set_p += hs_count;

        if(disk_set_p->bus == 0 &&
            disk_set_p->enclosure == 0 &&
            disk_set_p->slot == 0)
            return status;

        *free_drive_pos_p = *disk_set_p;
        status = FBE_STATUS_OK;

    }

    return status;
}

/*!**************************************************************
 * sep_rebuild_utils_validate_event_log_errors()
 ****************************************************************
 * @brief
 *  Check that the event log errors that occurred are appropriate.
 *
 * @param None.
 *
 * @return fbe_status_t
 *
 * @author
 *  8/11/2011 - Created. Jason White
 *
 ****************************************************************/

fbe_status_t sep_rebuild_utils_validate_event_log_errors(void)
{
    fbe_u32_t count = 0;
    fbe_status_t status;

    /* We should not see any uncorrectable, coherency or checksum errors.
     */
    status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR, &count);
    MUT_ASSERT_INT_EQUAL(count, 0);
    status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR, &count);
    MUT_ASSERT_INT_EQUAL(count, 0);
    status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR, &count);
    MUT_ASSERT_INT_EQUAL(count, 0);
    return status;
}
/******************************************
 * end sep_rebuild_utils_validate_event_log_errors()
 ******************************************/

/*!**************************************************************************
 *  sep_rebuild_utils_do_io
 ***************************************************************************
 * @brief
 *   This function performs I/O of various sizes to several different LBAs,
 *   based on the start LBA passed in.  The type of I/O to perform is
 *   specified in an input parameter.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_test_context_p     - pointer to rdgen context
 * @param in_operation_type     - rdgen I/O operation type
 * @param in_start_lba          - lba at which to start I/O
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_do_io(
                                fbe_test_rg_configuration_t*    in_rg_config_p,
                                fbe_api_rdgen_context_t*        in_test_context_p,
                                fbe_rdgen_operation_t           in_operation_type,
                                fbe_lba_t                       in_start_lba)
{

    fbe_object_id_t             raid_group_object_id;           // raid group's object id
    fbe_lba_t                   next_lba;                       // next lba to write to
    fbe_block_count_t           block_count;                    // number of blocks to write
    fbe_status_t                status;                         // fbe status


    //  Get the RG object id from the RG number
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);

    //  Set up the start LBA and block count for the next I/O
    next_lba   = in_start_lba;
    block_count = 10;

    //  Perform the requested I/O and make sure there were no errors
    status = fbe_api_rdgen_send_one_io(in_test_context_p, raid_group_object_id, FBE_CLASS_ID_INVALID,
                 FBE_PACKAGE_ID_SEP_0, in_operation_type, FBE_RDGEN_PATTERN_ZEROS,
                 next_lba, block_count, FBE_RDGEN_OPTIONS_STRIPE_ALIGN, 0, 0,
                 FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);


    //  Set up the start LBA and block count for the next I/O
    next_lba   = in_start_lba + 0x100;
    block_count = 3;

    //  Perform the requested I/O
    status = fbe_api_rdgen_send_one_io(in_test_context_p, raid_group_object_id, FBE_CLASS_ID_INVALID,
                 FBE_PACKAGE_ID_SEP_0, in_operation_type, FBE_RDGEN_PATTERN_ZEROS,
                 next_lba, block_count, FBE_RDGEN_OPTIONS_STRIPE_ALIGN, 0, 0,
                 FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);


    //  Set up the start LBA and block count for the next I/O
    next_lba   = in_start_lba + 0x200;
    block_count = 1;

    //  Perform the requested I/O
    status = fbe_api_rdgen_send_one_io(in_test_context_p, raid_group_object_id, FBE_CLASS_ID_INVALID,
                 FBE_PACKAGE_ID_SEP_0, in_operation_type, FBE_RDGEN_PATTERN_ZEROS,
                 next_lba, block_count, FBE_RDGEN_OPTIONS_STRIPE_ALIGN, 0, 0,
                 FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);

    //  Return
    return;

}   // End sep_rebuild_utils_do_io()

/*!**************************************************************************
 *  sep_rebuild_utils_write_new_data
 ***************************************************************************
 * @brief
 *   This function writes new data to several chunks.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_test_context_p     - pointer to rdgen context
 * @param in_start_lba          - lba at which to start I/O
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_write_new_data(
                                fbe_test_rg_configuration_t*    in_rg_config_p,
                                fbe_api_rdgen_context_t*        in_test_context_p,
                                fbe_lba_t                       in_start_lba)
{

    //  Write the new data and check it
    sep_rebuild_utils_do_io(in_rg_config_p, in_test_context_p, FBE_RDGEN_OPERATION_WRITE_READ_CHECK, in_start_lba);

    //  Return
    return;

}   // End sep_rebuild_utils_write_new_data()


/*!**************************************************************************
 *  sep_rebuild_utils_verify_new_data
 ***************************************************************************
 * @brief
 *   This function writes new data to several chunks.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_test_context_p     - pointer to rdgen context
 * @param in_start_lba          - lba at which to start I/O
 *
 * @return void
 *
 ***************************************************************************/
void sep_rebuild_utils_verify_new_data(
                                fbe_test_rg_configuration_t*    in_rg_config_p,
                                fbe_api_rdgen_context_t*        in_test_context_p,
                                fbe_lba_t                       in_start_lba)
{

    //  Read the data and verify it
    sep_rebuild_utils_do_io(in_rg_config_p, in_test_context_p, FBE_RDGEN_OPERATION_READ_CHECK, in_start_lba);

    //  Return
    return;

}   // End sep_rebuild_utils_verify_new_data()

/*!**************************************************************
 * fbe_test_sep_util_validate_rg_is_degraded()
 ****************************************************************
 * @brief
 *  Validate that all the associated raid groups are degraded.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param expected_removed_position - Position that we expect to be
 *          removed.
 *
 * @return None.
 *
 ****************************************************************/
void fbe_test_sep_rebuild_validate_rg_is_degraded(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_u32_t raid_group_count,
                                                  fbe_u32_t expected_removed_position)
{
    fbe_u32_t rg_index;

    /* Just loop over all our raid groups, refreshing each one.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        /* Validate that we go and stay degraded.
         */
        sep_rebuild_utils_check_for_degraded(&rg_config_p[rg_index],
                                             expected_removed_position,
                                             30000);
    }
    return;
}

/*!***************************************************************************
 *  fbe_test_sep_rebuild_utils_check_bits_clear_for_position
 *****************************************************************************
 * @brief
 *   This function checks the bits after a rebuild completes.
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param position_to_validate - Position that should not be rebuilding
 *
 * @note    The `needs_rebuild_bitmask' is the bitmask of positions that have at 
 *          least (1) chunk marked `Needs Rebuild' (NR).  It is not the bitmask
 *          of positions marked rebuild logging.
 *
 * @return void
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_utils_check_bits_clear_for_position(fbe_object_id_t  in_raid_group_object_id,
                                                              fbe_u32_t position_to_validate)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_raid_group_get_paged_info_t     paged_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the current and peer sp.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Trace the start*/
    mut_printf(MUT_LOG_TEST_STATUS, "check_bits_clear: for rg obj: 0x%x pos: %d start.", 
               in_raid_group_object_id, position_to_validate);

    /* Get the paged NR information.*/
    status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate:
     *  o This position does not contain any chunks that are marked `NR'
     *  o There are no chunks marked `Need Rebuild' (NR) for the position specified
     */
    if ((((1 << position_to_validate ) & paged_info.needs_rebuild_bitmask) != 0) ||
        (paged_info.pos_nr_chunks[position_to_validate] != 0)                       )
    {
        /* There is at least (1) chunk marked NR for the validate no NR
         * position
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "check_bits_clear: for rg obj: 0x%x pos: %d has: 0x%llx chunks marked NR mask: 0x%x", 
                   in_raid_group_object_id, position_to_validate, 
                   paged_info.pos_nr_chunks[position_to_validate], paged_info.needs_rebuild_bitmask);
        MUT_FAIL_MSG("There are chunks needing rebuild for position to validate");
    }

    /* In the case of a dual sp test we will also check the peer since
     * some of this information can be cached on the peer.  We want to make sure it is correct there also.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode() &&
        cmi_info.peer_alive                         )
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate:
         *  o Rebuild logging is clear for the position specified
         *  o There are no chunks marked `Need Rebuild' (NR) for the position specified
        */
        if ((((1 << position_to_validate ) & paged_info.needs_rebuild_bitmask) != 0) ||
            (paged_info.pos_nr_chunks[position_to_validate] != 0)                       )
        {
            /* There is at least (1) chunk marked NR for the validate no NR
             * position
            */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "check_bits_clear: for rg obj: 0x%x pos: %d has: 0x%llx chunks marked NR mask: 0x%x", 
                       in_raid_group_object_id, position_to_validate, 
                       paged_info.pos_nr_chunks[position_to_validate], paged_info.needs_rebuild_bitmask);
            MUT_FAIL_MSG("There are chunks needing rebuild for position to validate");
        }

        fbe_api_sim_transport_set_target_server(this_sp);
    }

    /* Trace the completion */
    mut_printf(MUT_LOG_TEST_STATUS, "check_bits_clear: for rg obj: 0x%x pos: %d mark NR mask: 0x%x - complete.", 
               in_raid_group_object_id, position_to_validate, paged_info.needs_rebuild_bitmask);

    return;
}
/****************************************************************
 * end fbe_test_sep_rebuild_utils_check_bits_clear_for_position()
 ****************************************************************/

/*!**************************************************************************
 *  fbe_test_sep_rebuild_utils_check_bits_set_for_position
 ***************************************************************************
 * @brief
 *   This function checks the bits before a rebuild of a swapped position
 *   completes.
 *
 * @param in_raid_group_object_id   - raid group's object id
 * @param position_to_validate - Position that should be rebuilding
 * 
 * @note    The `needs_rebuild_bitmask' is the bitmask of positions that have at 
 *          least (1) chunk marked `Needs Rebuild' (NR).  It is not the bitmask
 *          of positions marked rebuild logging.
 * 
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_rebuild_utils_check_bits_set_for_position(fbe_object_id_t  in_raid_group_object_id,
                                                            fbe_u32_t position_to_validate)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_raid_group_get_paged_info_t     paged_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the current and peer sp.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Trace the start*/
    mut_printf(MUT_LOG_TEST_STATUS, "check_bits_set: for rg obj: 0x%x pos: %d start.", 
               in_raid_group_object_id, position_to_validate);

    /* Wait for rebuild logging to be cleared for position.
     */

    /* Now get the paged metadata information.
     */
    status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate:
     *  o That there is at least (1) chunk marked NR for this position
     */
    if ((((1 << position_to_validate ) & paged_info.needs_rebuild_bitmask) == 0)     ||
        (paged_info.pos_nr_chunks[position_to_validate] == 0)                        ||
        (paged_info.pos_nr_chunks[position_to_validate] != paged_info.num_nr_chunks)    )
    {
        /* There is at least (1) chunk not marked NR for the validate NR
         * position
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "check bits set: for rg obj: 0x%x pos: %d has: 0x%llx chunks marked NR of total: 0x%llx and NR mask: 0x%x", 
                   in_raid_group_object_id, position_to_validate, 
                   paged_info.pos_nr_chunks[position_to_validate], paged_info.num_nr_chunks, paged_info.needs_rebuild_bitmask);
        MUT_FAIL_MSG("There are chunks not needing rebuild for position to validate");
    }

    /* In the case of a dual sp test we will also check the peer since
     * some of this information can be cached on the peer.  We want to make sure it is correct there also.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode() &&
        cmi_info.peer_alive                         )
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_raid_group_get_paged_bits(in_raid_group_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate:
         *  o That there is at least (1) chunk marked NR for this position
         */
        if ((((1 << position_to_validate ) & paged_info.needs_rebuild_bitmask) == 0)     ||
            (paged_info.pos_nr_chunks[position_to_validate] == 0)                        ||
            (paged_info.pos_nr_chunks[position_to_validate] != paged_info.num_nr_chunks)    )
        {
            /* There is at least (1) chunk not marked NR for the validate NR
             * position
            */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "check bits set: for rg obj: 0x%x pos: %d has: 0x%llx chunks marked NR of total: 0x%llx and NR mask: 0x%x", 
                       in_raid_group_object_id, position_to_validate, 
                       paged_info.pos_nr_chunks[position_to_validate], paged_info.num_nr_chunks, paged_info.needs_rebuild_bitmask);
            MUT_FAIL_MSG("There are chunks not needing rebuild for position to validate");
        }

        fbe_api_sim_transport_set_target_server(this_sp);
    }

    /* Trace the completion */
    mut_printf(MUT_LOG_TEST_STATUS, "check_bits_set: for rg obj: 0x%x pos: %d NR chunks: 0x%llx mark NR mask: 0x%x - complete.", 
               in_raid_group_object_id, position_to_validate,  paged_info.num_nr_chunks, paged_info.needs_rebuild_bitmask);
    return;
}
/****************************************************************
 * end fbe_test_sep_rebuild_utils_check_bits_set_for_position()
 ****************************************************************/

/*!***************************************************************************
 *  fbe_test_sep_rebuild_utils_wait_for_rebuild_logging_clear_for_position()
 ***************************************************************************** 
 * 
 * @brief   This function wait for rebuild logging to be clear for the position
 *          specified.
 *
 * @param   rg_object_id   - raid group's object id
 * @param   position_to_validate - Position that should be rebuilding
 * 
 * @return void
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_utils_wait_for_rebuild_logging_clear_for_position(fbe_object_id_t rg_object_id,
                                                            fbe_u32_t position_to_validate)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;

    max_retries = SEP_REBUILD_UTILS_MAX_RETRIES;

    for (count = 0; count < max_retries; count++)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* For the position specified check if rebuild logging is cleared
         */ 
        if (raid_group_info.b_rb_logging[position_to_validate] == FBE_FALSE)
        {
            /* No rebuild logging set, waiting is done, just return.
             */
            return;
        }

        /* Continue waiting for rb logging.
         */
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== rebuild logging still set for rg_id: 0x%x pos: %d ==", 
               rg_object_id, position_to_validate);
    status = FBE_STATUS_TIMEOUT;
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************************************************
 * end fbe_test_sep_rebuild_utils_wait_for_rebuild_logging_clear_for_position()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_utils_corrupt_paged_metadata()
 ***************************************************************************** 
 * 
 * @brief   This function `corrupts' the paged metadata by adding one or more
 *          positions to the NR bitmask.
 *
 * @param   rg_config_p - Pointer to raid groups configuration to corrupt
 * @param   user_space_lba - The user space LBA to corrupt the paged metadata
 *              for
 * @param   user_space_blocks - The number of user blocks in the request 
 *
 * @note    The `needs_rebuild_bitmask' is the bitmask of positions that have at 
 *          least (1) chunk marked `Needs Rebuild' (NR).  It is not the bitmask
 *          of positions marked rebuild logging.
 *
 * @return  status
 * 
 * @author
 *  03/12/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_rebuild_utils_corrupt_paged_metadata(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_lba_t user_space_lba,
                                                               fbe_block_count_t user_space_blocks)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_object_id_t                                     rg_object_id;
    fbe_api_raid_group_get_info_t                       rg_info;
    fbe_lba_t                                           chunk_size;
    fbe_chunk_index_t                                   chunk_offset;
    fbe_chunk_index_t                                   chunk_count;
    fbe_raid_group_paged_metadata_t                     chunk_info;
    fbe_raid_position_bitmask_t                         original_nr_bitmask;
    fbe_raid_position_bitmask_t                         modified_nr_bitmask;
    fbe_u32_t                                           original_number_of_positions_marked_nr;
    fbe_u32_t                                           number_of_positions_marked_nr;
    fbe_u32_t                                           number_of_positions_to_mark_nr;
    fbe_u32_t                                           position;

    /* Get info about the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the original chunk information for the requested block in the
     * user area.  Currently we limit the number of modified chunks to 1. 
     * The chunk offset is physical so we must divide by the number of 
     * data disks. 
     */
    chunk_size = rg_info.lun_align_size / rg_info.num_data_disk;
    chunk_offset = (user_space_lba / rg_info.num_data_disk) / chunk_size;
    chunk_count = (user_space_blocks >= chunk_size) ? (user_space_blocks / chunk_size) : 1;
    if (chunk_count > 1)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s changing number of modified chunks from %lld to %lld", 
                   __FUNCTION__, (unsigned long long)chunk_count, (unsigned long long)1);
        chunk_count = 1;
    }

    /* Generate set bits that will cause the error.  Add (1) more position that
     * is marked NR.
     */
    status = fbe_api_raid_group_get_paged_metadata(rg_object_id,
                                                   chunk_offset,
                                                   chunk_count,
                                                   &chunk_info,
                                                   FBE_FALSE    /* Allow data from cache*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    original_nr_bitmask = chunk_info.needs_rebuild_bits;
    modified_nr_bitmask = original_nr_bitmask;
    original_number_of_positions_marked_nr = fbe_test_common_count_bits(original_nr_bitmask);
    number_of_positions_marked_nr = original_number_of_positions_marked_nr;
    number_of_positions_to_mark_nr = rg_info.width - rg_info.num_data_disk + 1;
    position = 0;
    while((number_of_positions_marked_nr < number_of_positions_to_mark_nr) && 
          (position < rg_info.width)                                          )
    {
        if (((1 << position) & modified_nr_bitmask) == 0)
        {
            modified_nr_bitmask |= (1 << position);
            number_of_positions_marked_nr++;
        }
        position++;
    }

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "rebuild utils: corrupt paged metadata rg obj: 0x%x change chunk offset: %lld NR bitmask from: 0x%x to: 0x%x",
               rg_object_id, (unsigned long long)chunk_offset, original_nr_bitmask, modified_nr_bitmask);

    /* Update the NR bits
     */
    chunk_info.needs_rebuild_bits = modified_nr_bitmask;

    /* Now write the modified chunk information.
     */
    status = fbe_api_raid_group_set_paged_metadata(rg_object_id,
                                                   chunk_offset,
                                                   chunk_count,
                                                   &chunk_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/********************************************************* 
 * end fbe_test_sep_rebuild_utils_corrupt_paged_metadata()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_get_consumed_checkpoint_to_pause_at()
 *****************************************************************************
 *
 * @brief   Determine the raid group rebuild checkpoint (lba) that we should
 *          pause at after the desired `user /consumed' space on the associated 
 *          raid group has been rebuilt.
 *
 * @param   rg_object_id - The raid group object id to get the rebuild
 * @param   percent_user_space_rebuilt_before_pause - Percentage of user capacity
 *              on the raid group that should be copied before we pause.
 * @param   checkpoint_to_pause_at_p - Address of checkpoint to pause at
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/20/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_rebuild_get_consumed_checkpoint_to_pause_at(fbe_object_id_t rg_object_id,
                                                                     fbe_u32_t percent_user_space_rebuilt_before_pause,
                                                                     fbe_lba_t *checkpoint_to_pause_at_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t                                   upstream_capacity = 0;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;
    fbe_object_id_t                             upstream_object_id;
    fbe_api_raid_group_get_info_t               rg_info;
    fbe_u32_t                                   upstream_index;
    fbe_api_get_block_edge_info_t               edge_info;
    fbe_lba_t                                   rg_user_capacity = 0;
    fbe_lba_t                                   debug_hook_lba = FBE_LBA_INVALID;


    /* Get the raid group info.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the upstream object list and determine the `consumed' capacity.
     */
    status = fbe_api_base_config_get_upstream_object_list(rg_object_id, &upstream_object_list);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(upstream_object_list.number_of_upstream_objects >= 1);
    upstream_object_id = upstream_object_list.upstream_object_list[0];
    MUT_ASSERT_INT_NOT_EQUAL(0, upstream_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, upstream_object_id);

    /* If this is mirror under striper then we simply use the exported capacity.
     */
    if (rg_info.raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        /* Get the imported capacity of each upstream objects
         */
        for (upstream_index = 0; upstream_index < upstream_object_list.number_of_upstream_objects; upstream_index++)
        {
            /* Get the edge info for this upstream index.  We assume there is
             * only (1) downstream connection.
             */
            upstream_object_id = upstream_object_list.upstream_object_list[upstream_index];
            status = fbe_api_get_block_edge_info(upstream_object_id, 0, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Add the edge capacity to the total
             */
            upstream_capacity += edge_info.capacity;
        }
    }
    else
    {
        /* Else if this is a mirror under striper just use the exported capacity.
         */
        upstream_capacity = rg_info.raid_capacity;
    }

    /* Now convert to the physical (i.e. per drive) capacity
     */
    rg_user_capacity = upstream_capacity / rg_info.num_data_disk;

    /* Get the rebuild lba to pause at without the offset.
     */
    debug_hook_lba = ((rg_user_capacity + 99) / 100) * percent_user_space_rebuilt_before_pause;

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "fbe_test_sep_rebuild: rg obj: 0x%x consumed pause percent: %d lba: 0x%llx",
               rg_object_id, percent_user_space_rebuilt_before_pause, (unsigned long long)debug_hook_lba);

    /* Update requested value.
     */
    *checkpoint_to_pause_at_p = debug_hook_lba;

    return status;
}
/*************************************************************************
 * end fbe_test_sep_rebuild_get_consumed_checkpoint_to_pause_at()
 *************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_get_raid_group_checkpoint_to_pause_at()
 *****************************************************************************
 *
 * @brief   Determine the raid group rebuild checkpoint (lba) that we should
 *          pause at after the desired amount of raid group sapce has been
 *          rebuilt.
 *
 * @param   rg_object_id - The raid group object id to get the rebuild
 * @param   percent_user_space_rebuilt_before_pause - Percentage of raid group 
 *              capacity on the raid group that should be rebuilt before we pause.
 * @param   checkpoint_to_pause_at_p - Address of checkpoint to pause at
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/24/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_rebuild_get_raid_group_checkpoint_to_pause_at(fbe_object_id_t rg_object_id,
                                                                     fbe_u32_t percent_user_space_rebuilt_before_pause,
                                                                     fbe_lba_t *checkpoint_to_pause_at_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t               rg_info;
    fbe_lba_t                                   rg_user_capacity = 0;
    fbe_lba_t                                   debug_hook_lba = FBE_LBA_INVALID;


    /* Get the raid group info.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now convert to the physical (i.e. per drive) capacity
     */
    rg_user_capacity = rg_info.raid_capacity / rg_info.num_data_disk;

    /* Get the rebuild lba to pause at without the offset.
     */
    debug_hook_lba = ((rg_user_capacity + 99) / 100) * percent_user_space_rebuilt_before_pause;

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "fbe_test_sep_rebuild: rg obj: 0x%x raid group pause percent: %d lba: 0x%llx",
               rg_object_id, percent_user_space_rebuilt_before_pause, (unsigned long long)debug_hook_lba);

    /* Update requested value.
     */
    *checkpoint_to_pause_at_p = debug_hook_lba;

    return status;
}
/*************************************************************************
 * end fbe_test_sep_rebuild_get_raid_group_checkpoint_to_pause_at()
 *************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_generate_distributed_io()
 *****************************************************************************
 *
 * @brief   Generate a data pattern distributed across the each of the valid LUNs.
 *
 * @param   rg_config_p - Pointer to array of raid groups to validate
 * @param   raid_group_count - Number of raid groups under test
 * @param   rdgen_context_p - Pointer to rdgne context
 * @param   rdgen_op - rdgen opcode to use
 * @param   rdgen_pattern - rdgne data pattern
 * @param   requested_blocks - Number of blocks for each I/O
 *
 * @return fbe_status_t 
 *
 * @author
 *  03/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_rebuild_generate_distributed_io(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t raid_group_count,
                                                          fbe_api_rdgen_context_t *rdgen_context_p,
                                                          fbe_rdgen_operation_t rdgen_op,
                                                          fbe_rdgen_pattern_t rdgen_pattern,
                                                          fbe_u64_t requested_blocks)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       number_of_ios = 10;
    fbe_sim_transport_connection_target_t this_sp;
    fbe_sim_transport_connection_target_t other_sp;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id;
    fbe_u32_t                       lun_index;
    fbe_object_id_t                 lun_object_id;
    fbe_u32_t                       io_index;
    fbe_lba_t                       logical_capacity;
    fbe_lba_t                       io_gap;
    fbe_lba_t                       start_lba;
    fbe_u64_t                       blocks;
    fbe_api_rdgen_peer_options_t    peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    /* If dualsp send I/O thru peer
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s dual sp send I/O thru peer: %s", 
                   __FUNCTION__, (other_sp == FBE_SIM_SP_A) ? "SPA" : "SPB");
        status = fbe_api_rdgen_io_specification_set_peer_options(&rdgen_context_p->start_io.specification,
                                                                 peer_options);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* For all the raid groups specified
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Get the raid group object id.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* For each lun in the current raid group.
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            /* Get the lun object id.
             */
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Distribute the I/Os across the lun
             */
            logical_capacity = current_rg_config_p->logical_unit_configuration_list[lun_index].capacity;
            if ((number_of_ios * requested_blocks) > logical_capacity)
            {
                blocks = 1;
            }
            else
            {
                blocks = requested_blocks;
            }
            io_gap = (logical_capacity / number_of_ios);

            /* If this is the first lun for the raid group trace some information.
             */
            if (lun_index == 0)
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_rebuild: generate: %d distributed ios rg obj: 0x%x num luns: %d op: %d pat: %d blks: 0x%llx", 
                           number_of_ios, rg_object_id, current_rg_config_p->number_of_luns_per_rg,
                           rdgen_op, rdgen_pattern, requested_blocks);
            }

            /* Send (1) I/O for the size requested to the distributed area
             */
            for (io_index = 0; io_index < number_of_ios; io_index++)
            {
                /* Generate the starting lba
                 */
                start_lba = (io_index * io_gap);
                if ((start_lba + blocks) > logical_capacity)
                {
                    /* If we cannot send a single block something is wrong
                     */
                    if ((start_lba + 1) > logical_capacity)
                    {
                        status = FBE_STATUS_GENERIC_FAILURE;
                        mut_printf(MUT_LOG_TEST_STATUS, 
                                   "%s lun obj: 0x%x start_lba: 0x%llx greater than capacity: 0x%llx",
                                   __FUNCTION__, lun_object_id, 
                                   (unsigned long long)start_lba, 
                                   (unsigned long long)logical_capacity);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        return status;
                    }

                    /* Just execute a single block I/O.
                     */
                    blocks = 1;
                }
    
                /* Send the `logical' request  
                 */
                status = fbe_api_rdgen_send_one_io(rdgen_context_p, 
                                                   lun_object_id,   /* Issue one I/O to this LUN */
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   rdgen_op,
                                                   rdgen_pattern,
                                                   start_lba, /* lba */
                                                   blocks /* blocks */,
                                                   FBE_RDGEN_OPTIONS_INVALID,
                                                   0, 0, /* no expiration or abort time */
                                                   peer_options);
                fbe_test_sep_io_validate_status(status, rdgen_context_p, FBE_FALSE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            } /* end for all ios required */

        } /* end for all luns in this raid group */

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups */

    /* Clear peer options
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        status = fbe_api_rdgen_io_specification_set_peer_options(&rdgen_context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_test_sep_rebuild_generate_distributed_io()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_validate_no_quiesced_io()
 *****************************************************************************
 *
 * @brief   For the set of raid groups under test validate that there are no
 *          I/Os stuck min quiesce.
 *
 * @param   rg_config_p - Pointer to array of raid groups to validate
 * @param   raid_group_count - Number of raid groups under test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  05/09/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_rebuild_validate_no_quiesced_io(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t raid_group_count)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t this_sp;
    fbe_sim_transport_connection_target_t other_sp;
    fbe_cmi_service_get_info_t          cmi_info;
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                           rg_index;
    fbe_object_id_t                     rg_object_id;
    fbe_api_raid_group_get_io_info_t    raid_group_io_info;
    fbe_u32_t                           secs_to_wait_for_unquiesce = 60;
    fbe_u32_t                           total_secs_waited = 0;       

    /* Get local and peer SP
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* For all the raid groups specified
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Get the raid group object id.
         */
        total_secs_waited = 0;
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate on current SP
         */
        status = fbe_test_sep_util_get_raid_group_io_info(rg_object_id, &raid_group_io_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* It is Ok to be quiesced but it should not be set forever
         */
        if (raid_group_io_info.b_is_quiesced)
        {
            /* Log a message and wait up to 60 seconds for the quiesce to be
             * cleared.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s rg obj: 0x%x SP: %d currently quiesced wait: %d secs for unquiesce",
                       __FUNCTION__, rg_object_id, this_sp, (secs_to_wait_for_unquiesce - total_secs_waited));

            /* Wait for unquiesce
             */
            while((raid_group_io_info.b_is_quiesced == FBE_TRUE)   &&
                  (total_secs_waited < secs_to_wait_for_unquiesce)    )
            {
                fbe_api_sleep(1000);
                total_secs_waited++;

                status = fbe_test_sep_util_get_raid_group_io_info(rg_object_id, &raid_group_io_info);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* If quiesce is still set we are done.
             */
            if (raid_group_io_info.b_is_quiesced)
            {
                status = FBE_STATUS_TIMEOUT;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s rg obj: 0x%x SP: %d waited: %d secs for unquiesce but still set",
                           __FUNCTION__, rg_object_id, this_sp, total_secs_waited);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
        }

        /* We shouldn't be quiesced and the quiesced count should be 0.
         */
        MUT_ASSERT_TRUE(raid_group_io_info.b_is_quiesced == FBE_FALSE);
        MUT_ASSERT_TRUE(raid_group_io_info.quiesced_io_count == 0);

        /* Validate on peer
         */
        if (fbe_test_sep_util_get_dualsp_test_mode() &&
            cmi_info.peer_alive                         )
        {
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_util_get_raid_group_io_info(rg_object_id, &raid_group_io_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            
            /* It is Ok to be quiesced but it should not be set forever
             */
            if (raid_group_io_info.b_is_quiesced)
            {
                /* Log a message and wait up to 60 seconds for the quiesce to be
                 * cleared.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s rg obj: 0x%x SP: %d currently quiesced wait: %d secs for unquiesce",
                           __FUNCTION__, rg_object_id, other_sp, (secs_to_wait_for_unquiesce - total_secs_waited));
    
                /* Wait for unquiesce
                 */
                while((raid_group_io_info.b_is_quiesced == FBE_TRUE)   &&
                      (total_secs_waited < secs_to_wait_for_unquiesce)    )
                {
                    fbe_api_sleep(1000);
                    total_secs_waited++;
    
                    status = fbe_test_sep_util_get_raid_group_io_info(rg_object_id, &raid_group_io_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
    
                /* If quiesce is still set we are done.
                 */
                if (raid_group_io_info.b_is_quiesced)
                {
                    status = FBE_STATUS_TIMEOUT;
                    mut_printf(MUT_LOG_TEST_STATUS, 
                               "%s rg obj: 0x%x SP: %d waited: %d secs for unquiesce but still set",
                               __FUNCTION__, rg_object_id, other_sp, total_secs_waited);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
                }
            }
    
            /* We shouldn't be quiesced and the quiesced count should be 0.
             */
            MUT_ASSERT_TRUE(raid_group_io_info.b_is_quiesced == FBE_FALSE);
            MUT_ASSERT_TRUE(raid_group_io_info.quiesced_io_count == 0);

            /* Set target to original
             */
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        } /* end if dualsp test mode */

        /* Goto next raid group
         */
        current_rg_config_p++;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_test_sep_rebuild_validate_no_quiesced_io()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook()
 *****************************************************************************
 *
 * @brief   Set debug hook when mark NR is complete.
 *
 * @param   rg_config_p - array of rg config to run test against.
 * @param   raid_group_count - Number of raid groups to set hook for
 *
 * @return  none
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Simply set the debug hook (only on the `Active') for all the raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If not enabled don't set the hook.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Set hook when parent raid group has completed marking NR
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /*! @note For a RAID-10 we always use the first mirror. 
             */
            status = fbe_test_add_debug_hook_active(downstream_object_list.downstream_object_list[0], 
                                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                    0, 0,
                                                    SCHEDULER_CHECK_STATES,
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                    0, 0,
                                                    SCHEDULER_CHECK_STATES,
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }

    /* Return
     */
    return;
}
/**************************************************************
 * end fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook()
 *****************************************************************************
 *
 * @brief   wait for debug hook when mark NR is complete.
 *
 * @param   rg_config_p - array of rg config to run test against. 
 * @param   raid_group_count - Number of raid groups to set hook for
 *
 * @return  none
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Simply set the debug hook (only on the `Active') for all the raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If not enabled don't set the hook.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Wait for hook when parent raid group has completed marking NR
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /*! @note For a RAID-10 we always use the first mirror. 
             */
            status = fbe_test_wait_for_debug_hook_active(downstream_object_list.downstream_object_list[0], 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                     SCHEDULER_CHECK_STATES, 
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                     SCHEDULER_CHECK_STATES, 
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }

    /* Return
     */
    return;
}
/******************************************************************
 * end fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_validate_raid_group_not_marked_nr()
 *****************************************************************************
 *
 * @brief   Validate that there are no chunks marked NR for the associated
 *          raid groups.
 *
 * @param   rg_config_p - array of rg config to run test against. 
 * @param   raid_group_count - Number of raid groups to set hook for
 *
 * @return  none
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_validate_raid_group_not_marked_nr(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count)
{
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Simply set the debug hook (only on the `Active') for all the raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If not enabled don't set the hook.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Validate that no chunks were marked
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /*! @note For a RAID-10 we always use the first mirror. 
             */
            sep_rebuild_utils_check_bits(downstream_object_list.downstream_object_list[0]);
        } else {
            sep_rebuild_utils_check_bits(rg_object_id);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }

    /* Return
     */
    return;
}
/**************************************************************
 * end fbe_test_sep_rebuild_validate_raid_group_not_marked_nr()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_validate_raid_group_not_marked_nr()
 *****************************************************************************
 *
 * @brief   Validate that the there are no chunks marked NR for the position
 *          specified.
 *
 * @param   rg_config_p - array of rg config to run test against. 
 * @param   raid_group_count - Number of raid groups to set hook for
 * @param   position_to_check - The position that should not me marked NR
 *
 * @return  none
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_validate_raid_group_position_not_marked_nr(fbe_test_rg_configuration_t *rg_config_p,
                                                                     fbe_u32_t raid_group_count,
                                                                     fbe_u32_t position_to_check)
{
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Simply set the debug hook (only on the `Active') for all the raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If not enabled don't set the hook.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Validate that no chunks were marked
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /*! @note For a RAID-10 we always use the first mirror. 
             */
            fbe_test_sep_rebuild_utils_check_bits_clear_for_position(downstream_object_list.downstream_object_list[0], position_to_check);
        } else {
            fbe_test_sep_rebuild_utils_check_bits_clear_for_position(rg_object_id, position_to_check);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }

    /* Return
     */
    return;
}
/***********************************************************************
 * end fbe_test_sep_rebuild_validate_raid_group_position_not_marked_nr()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook()
 *****************************************************************************
 *
 * @brief   Delete debug hook when mark NR is complete.
 *
 * @param   rg_config_p - array of rg config to run test against. 
 * @param   raid_group_count - Number of raid groups to set hook for
 *
 * @return  none
 *
 *****************************************************************************/
void fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;

    /* Simply set the debug hook (only on the `Active') for all the raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* If not enabled don't set the hook.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the raid group object id.
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Delete hook when parent raid group has completed marking NR
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /*! @note For a RAID-10 we always use the first mirror. 
             */
            status = fbe_test_del_debug_hook_active(downstream_object_list.downstream_object_list[0], 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            status = fbe_test_del_debug_hook_active(rg_object_id, 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }

    /* Return
     */
    return;
}
/**************************************************************
 * end fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook()
 *************************************************************/


/********************************************* 
 * end of file fbe_test_sep_rebuild_utils.c
 *********************************************/
