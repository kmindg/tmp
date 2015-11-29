/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file foghorn_leghorn_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of non configured drive zeroing with failures.
 *
 * @version
 *   11/30/2011 - Created. Deanna Heng
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
#include "sep_rebuild_utils.h"
#include "sep_zeroing_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* kit_cloudkicker_short_desc = "Zeroing of swapped in permanant spares";
char* kit_cloudkicker_long_desc =
    "\n"
    "\n"
    "This scenario validates background disk zeroing operation for swapped in \n"
    "permanant spares that are either completely zeroed or not fully zeroed on \n"
    "dual SP.\n"
    "\n"
    "\n"
    "******* Kit Cloudkicker *******************************************************\n"
    "\n"
    "\n"
    "Dependencies:\n"
    "    - LD object should support Disk zero IO.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 SAS drives (PDOs)\n"
    "    [PP] 1 logical drives (LDs)\n"
    "    [SEP] 1 provision drives (PVDs)\n"
    "\n"
    "STEP A: Setup - Bring up the initial topology for local SP\n"
    "    - Insert a new enclosure having one non configured drives\n"
    "    - Create a PVD object\n"
    "    - Create a RAID GROUP with a LUN\n"
    "    - Create a spare drive\n"
    "\n"
    "TEST 1: Dual SP test - Verify zeroing operation on nonzeroed permanent spare when replacing a nonzeroed disk\n"
    "\n"
    "STEP 1: Set up the pvd and the permanent spare for zeroing\n"
    "    - Check that PVD object should be in READY state\n"
    "    - Set the zeroing checkpoint to the default offset\n"
    "    - Disable Zeroing on the PVD and permanent spare\n"
    "    - Set up hook on the permanent spare to verify if zeroing occurs\n"
    "    - Set up hook on rg for rebuild checking\n"
    "STEP 2: Remove the PVD and wait for permanent spare to swap in\n"
    "    - verify zeroing stopped on the removed drive\n"
    "STEP 3: Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "    - Get the spare edge information. Confirm desired HS has been swapped in.\n"
    "    - Verify that  virtual drive is in the READY state.\n"
    "STEP 4: Initiate disk zeroing/Enable background Zeroing on the permanent spare\n"
    "STEP 5: Verify the disk zeroing operation of the permanent spare\n"
    "    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
    "      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
    "    - Remove the zeroing hook on the permanent spare\n"
    "STEP 6: Disk zeroing complete\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
    "STEP 7: Rebuild complete\n"
    "    - Check that the rebuild hook is hit for the raid group\n"
    "    - Remove the rebuild hook\n"
    "    - Verify that the rebuild completed\n"
    "STEP 8: Reinsert the removed drive\n"
    "\n"
    "TEST 2: Dual SP test - Verify zeroing operation for nonzeroed permanent spare swap during disk zeroing\n"
    "\n"
    "STEP 1: Set up the pvd and the permanent spare for zeroing\n"
    "    - Check that PVD object should be in READY state\n"
    "    - Set the zeroing checkpoint to the default offset\n"
    "    - Disable Zeroing on the PVD and permanent spare\n"
    "    - Set up hook on the permanent spare to verify if zeroing occurs\n"
    "    - Set up hook on PVD to ensure the disk is zeroing when the swap occurs\n"
    "    - Set up hook on rg for rebuild checking\n"
    "    - Manually start zeroing on PVD and hit the zeroing hook to hold it in zeroing\n"
    "STEP 2: Remove the PVD and wait for permanent spare to swap in\n"
    "    - Verify zeroing stopped on the removed drive\n"
    "    - Remove the hook for the PVD\n"
    "STEP 3: Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "    - Get the spare edge information. Confirm desired HS has been swapped in.\n"
    "    - Verify that  virtual drive is in the READY state.\n"
    "STEP 4: Initiate disk zeroing/Enable background Zeroing on the permanent spare\n"
    "STEP 5: Verify the disk zeroing operation of the permanent spare\n"
    "    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
    "      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
    "    - Remove the zeroing hook on the permanent spare\n"
    "STEP 6: Disk zeroing complete\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
    "STEP 7: Rebuild complete\n"
    "    - Check that the rebuild hook is hit for the raid group\n"
    "    - Remove the rebuild hook\n"
    "    - Verify that the rebuild completed\n"
    "STEP 8: Reinsert the removed drive\n"
    "\n"
    "TEST 3: Dual SP test - Verify zeroing operation for zeroed permanent spare swap\n"
    "\n"
    "STEP 1: Set up the pvd and the permanent spare for zeroing\n"
    "    - Check that PVD object should be in READY state\n"
    "    - Set the zeroing checkpoint to the default offset\n"
    "    - Disable Zeroing on the PVD and permanent spare\n"
    "    - Manually start zeroing on PVD and permanent spare and wait for zeroing to finish\n"
    "    - Set up hook on the permanent spare to later check that zeroing does not occur\n"
    "    - Reenable zeroing on the permanent spare\n"
    "    - Set up hook on rg for rebuild checking\n"
    "STEP 2: Remove the PVD and wait for permanent spare to swap in\n"
    "STEP 3: Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "    - Get the spare edge information. Confirm desired HS has been swapped in.\n"
    "    - Verify that  virtual drive is in the READY state.\n"
    "STEP 4: Verify that the permanent spare did not rezero\n"
    "    - Since the permanent spare had already been zeroed, we should not rezero. Check that the hook is not hit\n"
    "    - Remove the zeroing hook on the permanent spare\n"
    "STEP 5: Disk zeroing complete\n"
    "    - Remove the hook on the permanent spare\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
    "STEP 6: Rebuild complete\n"
    "    - Check that the rebuild hook is hit for the raid group\n"
    "    - Remove the rebuild hook\n"
    "    - Verify that the rebuild completed\n"
    "STEP 7: Reinsert the removed drive\n"
    "\n"
    "TEST 4: Dual SP test - Verify zeroing operation for zeroed permanent spare swap during disk zeroing\n"
    "\n"
    "STEP 1: Set up the pvd and the permanent spare for zeroing\n"
    "    - Check that PVD object should be in READY state\n"
    "    - Set the zeroing checkpoint to the default offset\n"
    "    - Disable Zeroing on the PVD\n"
    "    - Manually start zeroing on permanent spare and wait for zeroing to finish\n"
    "    - Renable background zeroing for permanent spare\n"
    "    - Set up hook on the permanent spare to verify zeroing does not occur\n"
    "    - Set up hook on PVD to ensure the disk is zeroing when the swap occurs\n"
    "    - Set up hook on rg for rebuild checking\n"
    "    - Manually start zeroing on PVD and hit the zeroing hook to hold it in zeroing\n"
    "STEP 2: Remove the PVD and wait for permanent spare to swap in\n"
    "    - Verify zeroing stopped on the removed drive\n"
    "    - Remove the zeroing hook for the PVD\n"
    "STEP 3: Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "    - Get the spare edge information. Confirm desired HS has been swapped in.\n"
    "    - Verify that  virtual drive is in the READY state.\n"
    "STEP 4: Verify that the permanent spare did not rezero\n"
    "    - Since the permanent spare had already been zeroed, we should not rezero. Check that the hook is not hit\n"
    "    - Remove the zeroing hook on the permanent spare\n"
    "STEP 5: Disk zeroing complete\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
    "STEP 6: Rebuild complete\n"
    "    - Check that the rebuild hook is hit for the raid group\n"
    "    - Remove the rebuild hook\n"
    "    - Verify that the rebuild completed\n"
    "STEP 7: Reinsert the removed drive\n"
    "\n";

/*!*******************************************************************
 * @def KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def KIT_CLOUDKICKER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define KIT_CLOUDKICKER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def KIT_CLOUDKICKER_ZERO_WAIT_TIME
 *********************************************************************
 * @brief Maximum wait time for zeroing
 *
 *********************************************************************/
#define KIT_CLOUDKICKER_ZERO_WAIT_TIME 200000

/*!*******************************************************************
 * @def KIT_CLOUDKICKER_RTL0_NUM_TEST_CASES_TO_RUN
 *********************************************************************
 * @brief This is the number of test cases to run for raid test level 0
 *
 *********************************************************************/
#define KIT_CLOUDKICKER_RTL0_NUM_TEST_CASES_TO_RUN 3

/*!*******************************************************************
 * @var kit_cloudkicker_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/
fbe_test_rg_configuration_t kit_cloudkicker_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];

/*!*******************************************************************
 * @enum kit_cloudkicker_test_index_e
 *********************************************************************
 * @brief This is an test index enum for the kit_cloudkicker.
 *
 *********************************************************************/
typedef enum kit_cloudkicker_test_index_e
{
    KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE = 0,
    KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING = 1,
    KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE = 2,
    KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING = 3,

    /*!@note Add new test index here.*/
    KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST,
}
kit_cloudkicker_test_index_t;


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/
void kit_cloudkicker_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

void kit_cloudkicker_swap_in_nonzeroed_permanent_spare(fbe_test_rg_configuration_t *rg_config_p);
void kit_cloudkicker_swap_in_nonzeroed_permanent_spare_during_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p);
void kit_cloudkicker_swap_in_zeroed_permanent_spare(fbe_test_rg_configuration_t *rg_config_p);
void kit_cloudkicker_swap_in_zeroed_permanent_spare_during_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p);

void kit_cloudkicker_setup_pvd_for_zeroing(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
										   fbe_object_id_t * pvd_obj_id, fbe_api_provision_drive_info_t * pvd_info);

void kit_cloudkicker_get_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint, fbe_bool_t was_hit);
void kit_cloudkicker_set_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);
void kit_cloudkicker_delete_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);
void kit_cloudkicker_wait_for_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint);

void kit_cloudkicker_set_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p); 
void kit_cloudkicker_check_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p); 
void kit_cloudkicker_remove_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p); 
void kit_cloudkicker_check_rebuild_performed(fbe_test_rg_configuration_t* rg_config_p, fbe_u32_t drive_pos_to_replace);

/*!****************************************************************************
 * kit_cloudkicker_run_tests()
 ******************************************************************************
 * @brief
 *  Run permanent sparing zeroing tests on raid group config
 *
 * @param   rg_config_p - rg config to run the test against
 *          context_p - pointer to test case index
 *
 * @return  None
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p) {
    kit_cloudkicker_test_index_t    test_index = KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       index = 0;
    fbe_bool_t                      is_bgz_enabled = FBE_FALSE;
    fbe_object_id_t     		    pvd_object_id = FBE_OBJECT_ID_INVALID;
    test_index = * ((kit_cloudkicker_test_index_t *)context_p);
    
    mut_printf(MUT_LOG_LOW, "=== KIT CLOUDKICKER TEST - Zeroing with permanent spares Test %d ===\n", test_index);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    	/* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                            rg_config_p->rg_disk_set[0].enclosure,
                                                            rg_config_p->rg_disk_set[0].slot, 
															&pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_base_config_is_background_operation_enabled(pvd_object_id, 
                        FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &is_bgz_enabled);

    if (!is_bgz_enabled) {
        mut_printf(MUT_LOG_LOW, "Background zeroing is NOT enabled. The test will NOT run.\n");
        return;
    }

    for(index = 0; index < num_raid_groups; index++)
    {
        if(fbe_test_rg_config_is_enabled(rg_config_p)) {
            switch(test_index)
            {
                case KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE:
                    mut_printf(MUT_LOG_LOW, "=== KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE ===\n");
                    kit_cloudkicker_swap_in_nonzeroed_permanent_spare(&rg_config_p[index]);
                    break;

                case KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING:
                    mut_printf(MUT_LOG_LOW, "=== KIT_CLOUDKICKER_SWAP_IN_NONZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING ===\n");
                    kit_cloudkicker_swap_in_nonzeroed_permanent_spare_during_disk_zeroing(&rg_config_p[index]);
                    break;

                case KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE:
                    mut_printf(MUT_LOG_LOW, "=== KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE ===\n");
                    kit_cloudkicker_swap_in_zeroed_permanent_spare(&rg_config_p[index]);
                    break;

                case KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING:
                    mut_printf(MUT_LOG_LOW, "=== KIT_CLOUDKICKER_SWAP_IN_ZEROED_PERMANENT_SPARE_DURING_DISK_ZEROING ===\n");
                    kit_cloudkicker_swap_in_zeroed_permanent_spare_during_disk_zeroing(&rg_config_p[index]);
                    break;

                default:
                    mut_printf(MUT_LOG_LOW, "=== kit cloudkicker Invalid test index: %d===\n", test_index);
                    break;
            }
        }
    }
}
/***************************************************************
 * end kit_cloudkicker_run_tests()
 ***************************************************************/

/*!****************************************************************************
 * kit_cloudkicker_swap_in_nonzeroed_permanent_spare_during_disk_zeroing()
 ******************************************************************************
 * @brief
 *  Swap in a nonzeroed permanent spare while a disk in the raid group is being zeroed
 *
 * @param  rg_config_p - rg config to run the test against
 *
 * @return None.
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_swap_in_nonzeroed_permanent_spare_during_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p) {
    fbe_status_t                        status = FBE_STATUS_OK;
	fbe_object_id_t     				pvd_object_id = FBE_OBJECT_ID_INVALID;
	fbe_api_provision_drive_info_t      pvd_info;
	fbe_object_id_t     				spare_pvd_object_id;
	fbe_api_provision_drive_info_t      spare_pvd_info;
	fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
	fbe_lba_t                           chkpt;
    fbe_sim_transport_connection_target_t current_target;

    // Create a hot spare
    status = fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);

	// Disable zeroing on the permanent spare pvd and reset the checkpoint
	kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->extra_disk_set[0].bus,
                                          rg_config_p->extra_disk_set[0].enclosure,
                                          rg_config_p->extra_disk_set[0].slot,
										  &spare_pvd_object_id,
										  &spare_pvd_info);

	// randomly select a position to remove
	drive_pos_to_replace = fbe_random() % rg_config_p->width;

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);
    
	// Disable zeroing on the pvd and reset the checkpoint
    kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id,
                                          &pvd_info);

	// set the hooks for the pvd to replace and the spare so that we can check they were zeroing
	kit_cloudkicker_set_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);	    
	kit_cloudkicker_set_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);
    kit_cloudkicker_set_rebuild_hook(rg_config_p); 

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", pvd_object_id);

    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);	    
    status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);

    /* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kit_cloudkicker_wait_for_hook(pvd_object_id, pvd_info.default_offset+0x800); 
    kit_cloudkicker_get_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800, FBE_TRUE);

    mut_printf(MUT_LOG_LOW, "=== Removing position %d in the raid group\n", drive_pos_to_replace);
    /* need to remove a drive here */
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects-=1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                          number_physical_objects, &drive_info);

    kit_cloudkicker_delete_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

    mut_printf(MUT_LOG_LOW, "=== Checking to ensure that the zeroing stopped\n");
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_stopped(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &chkpt);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //MUT_ASSERT_UINT64_EQUAL(pvd_info.default_offset+0x800, chkpt);

    // the spare pvd should've swapped in...
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, drive_pos_to_replace);

    // start disk zeroing on the pvd
    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);		
    status = fbe_api_base_config_enable_background_operation(spare_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = sep_zeroing_utils_wait_for_disk_zeroing_to_start(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kit_cloudkicker_wait_for_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800); 
    kit_cloudkicker_get_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800, FBE_TRUE);
    kit_cloudkicker_delete_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kit_cloudkicker_check_rebuild_performed(rg_config_p, drive_pos_to_replace);
    number_physical_objects+=1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_replace, 
                                                number_physical_objects, &drive_info);

    fbe_api_sim_transport_set_target_server(current_target);

}
/***************************************************************
 * end kit_cloudkicker_swap_in_nonzeroed_permanent_spare_during_disk_zeroing()
 ***************************************************************/

/*!****************************************************************************
 * kit_cloudkicker_swap_in_nonzeroed_permanent_spare()
 ******************************************************************************
 * @brief
 *  Swap in a nonzeroed permanent spare for a nonzeroed drive in a raid group 
 *
 * @param  rg_config_p - rg config to run the test against
 *
 * @return None.
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_swap_in_nonzeroed_permanent_spare(fbe_test_rg_configuration_t *rg_config_p) {
    fbe_status_t                        status = FBE_STATUS_OK;
	fbe_object_id_t     				pvd_object_id = FBE_OBJECT_ID_INVALID;
	fbe_api_provision_drive_info_t      pvd_info;
	fbe_object_id_t     				spare_pvd_object_id;
	fbe_api_provision_drive_info_t      spare_pvd_info;
	fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
	fbe_lba_t                           chkpt;
    fbe_sim_transport_connection_target_t current_target;

    // Create a hot spare
    status = fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);

	// Disable zeroing on the permanent spare pvd and reset the checkpoint
	kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->extra_disk_set[0].bus,
                                          rg_config_p->extra_disk_set[0].enclosure,
                                          rg_config_p->extra_disk_set[0].slot,
										  &spare_pvd_object_id,
										  &spare_pvd_info);

	// randomly select a position to remove
	drive_pos_to_replace = fbe_random() % rg_config_p->width;

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);
    
	// Disable zeroing on the pvd and reset the checkpoint
    kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id,
                                          &pvd_info);

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);
	kit_cloudkicker_set_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);
    kit_cloudkicker_set_rebuild_hook(rg_config_p); 


    mut_printf(MUT_LOG_LOW, "=== Removing the position %d in the raid group\n", drive_pos_to_replace);
    /* need to remove a drive here */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	number_physical_objects-=1;
	sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                              number_physical_objects, &drive_info);
    /*sep_rebuild_utils_remove_drive_no_check(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                            rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                            number_physical_objects, &drive_info);*/


    /* verify that the zeroing checkpoint did not move for the drive*/
    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);    
    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &chkpt);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(pvd_info.default_offset, chkpt);

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);

	// the spare pvd should've swapped in...
	fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, drive_pos_to_replace);

	 status = fbe_api_base_config_enable_background_operation(spare_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);

	/* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kit_cloudkicker_wait_for_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800); 
	kit_cloudkicker_get_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800, FBE_TRUE);
	kit_cloudkicker_delete_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kit_cloudkicker_check_rebuild_performed(rg_config_p, drive_pos_to_replace);

       number_physical_objects+=1;
	sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                                number_physical_objects, &drive_info);
    /*sep_rebuild_utils_reinsert_removed_drive_no_check(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                                      rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                                      rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                                      &drive_info);*/

    fbe_api_sim_transport_set_target_server(current_target);

}
/***************************************************************
 * end kit_cloudkicker_swap_in_nonzeroed_permanent_spare()
 ***************************************************************/

/*!****************************************************************************
 * kit_cloudkicker_swap_in_zeroed_permanent_spare()
 ******************************************************************************
 * @brief
 *  Swap in a zeroed permanent spare for a zeroed drive in a raid group 
 *
 * @param  rg_config_p - rg config to run the test against
 *
 * @return None.
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_swap_in_zeroed_permanent_spare(fbe_test_rg_configuration_t *rg_config_p) {
    fbe_status_t                        status = FBE_STATUS_OK;
	fbe_object_id_t     				pvd_object_id = FBE_OBJECT_ID_INVALID;
	fbe_api_provision_drive_info_t      pvd_info;
	fbe_object_id_t     				spare_pvd_object_id;
	fbe_api_provision_drive_info_t      spare_pvd_info;
	fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    fbe_sim_transport_connection_target_t current_target;    

    // Create a hot spare
    status = fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);      

	// Disable zeroing on the permanent spare pvd and reset the checkpoint
	kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->extra_disk_set[0].bus,
                                          rg_config_p->extra_disk_set[0].enclosure,
                                          rg_config_p->extra_disk_set[0].slot,
										  &spare_pvd_object_id,
										  &spare_pvd_info);
    // start disk zeroing on the pvd
	status = fbe_api_base_config_enable_background_operation(spare_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	// randomly select a position to remove
	drive_pos_to_replace = fbe_random() % rg_config_p->width;

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);
    
	// Disable zeroing on the pvd and reset the checkpoint
    kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id,
                                          &pvd_info);
    status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    sep_zeroing_utils_wait_for_disk_zeroing_to_start(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	//kit_cloudkicker_set_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);
    kit_cloudkicker_set_rebuild_hook(rg_config_p); 


    mut_printf(MUT_LOG_LOW, "=== Removing the position %d in the raid group\n", drive_pos_to_replace);
    /* need to remove a drive here */
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects-=1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                          number_physical_objects, &drive_info);

    // the spare pvd should've swapped in...
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, drive_pos_to_replace);

    // Because of rebuild, the permanent spare will go through the zeroing process again 
    //kit_cloudkicker_wait_for_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800); 
    //kit_cloudkicker_get_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800, FBE_TRUE);
    //kit_cloudkicker_delete_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);    

	/* check that disk zeroing did not move */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	//kit_cloudkicker_get_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800, FBE_FALSE);
	//kit_cloudkicker_delete_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    kit_cloudkicker_check_rebuild_performed(rg_config_p, drive_pos_to_replace);

    number_physical_objects+=1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                                number_physical_objects, &drive_info);

    fbe_api_sim_transport_set_target_server(current_target);
}
/***************************************************************
 * end kit_cloudkicker_swap_in_zeroed_permanent_spare()
 ***************************************************************/

/*!****************************************************************************
 * kit_cloudkicker_swap_in_zeroed_permanent_spare_during_disk_zeroing()
 ******************************************************************************
 * @brief
 *  Swap in a nonzeroed permanent spare for a drive that is zeroing in a raid group 
 *
 * @param  rg_config_p - rg config to run the test against
 *
 * @return None.
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_swap_in_zeroed_permanent_spare_during_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p) {
    fbe_status_t                        status = FBE_STATUS_OK;
	fbe_object_id_t     				pvd_object_id = FBE_OBJECT_ID_INVALID;
	fbe_api_provision_drive_info_t      pvd_info;
	fbe_object_id_t     				spare_pvd_object_id;
	fbe_api_provision_drive_info_t      spare_pvd_info;
	fbe_u32_t           				number_physical_objects = 0;
	fbe_u32_t							drive_pos_to_replace = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    fbe_lba_t                           chkpt;
    fbe_u32_t                           index;
    fbe_sim_transport_connection_target_t current_target;    

    // Create a hot spare
    status = fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);      

	// Disable zeroing on the permanent spare pvd and reset the checkpoint
	kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->extra_disk_set[0].bus,
                                          rg_config_p->extra_disk_set[0].enclosure,
                                          rg_config_p->extra_disk_set[0].slot,
										  &spare_pvd_object_id,
										  &spare_pvd_info);
    
	status = fbe_api_base_config_enable_background_operation(spare_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	// randomly select a position to remove
	drive_pos_to_replace = fbe_random() % rg_config_p->width;

    /* diable zeroing on all the drives in the raid group and set up hooks */
    for(index=0; index < rg_config_p->width; index ++) 
    {
        if(index == drive_pos_to_replace) 
        {
            continue;
        }

        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus,
                                          rg_config_p->rg_disk_set[index].enclosure,
                                          rg_config_p->rg_disk_set[index].slot,
                                          &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);

        
        kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->rg_disk_set[index].bus,
                                          rg_config_p->rg_disk_set[index].enclosure,
                                          rg_config_p->rg_disk_set[index].slot,
                                          &pvd_object_id,
                                          &pvd_info);

        // setup the hoooks for the pvds to check for zeroing
        kit_cloudkicker_set_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);
    }

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                      rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                      rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                      &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);

	// Disable zeroing on the pvd and reset the checkpoint
    kit_cloudkicker_setup_pvd_for_zeroing(rg_config_p->rg_disk_set[drive_pos_to_replace].bus,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].enclosure,
                                          rg_config_p->rg_disk_set[drive_pos_to_replace].slot,
                                          &pvd_object_id,
                                          &pvd_info);

    // setup the hoooks for the pvds to check for zeroing
    kit_cloudkicker_set_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);
    kit_cloudkicker_set_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);    
    kit_cloudkicker_set_rebuild_hook(rg_config_p);
    

    // start disk zeroing on the pvd
	status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME);

	/* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kit_cloudkicker_wait_for_hook(pvd_object_id, pvd_info.default_offset+0x800); 
    kit_cloudkicker_get_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800, FBE_TRUE);


    mut_printf(MUT_LOG_LOW, "=== Removing the position %d in the raid group\n", drive_pos_to_replace);
    /* need to remove a drive here */
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects-=1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_pos_to_replace,
                                          number_physical_objects, &drive_info);

    kit_cloudkicker_delete_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);

	mut_printf(MUT_LOG_LOW, "=== Checking to ensure that the zeroing stopped\n");
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_stopped(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &chkpt);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //MUT_ASSERT_UINT64_EQUAL(pvd_info.default_offset+0x800, chkpt);

	// the spare pvd should've swapped in...
	fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, drive_pos_to_replace);

    // Because of rebuild, the permanent spare will go through the zeroing process again 
    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);    
    kit_cloudkicker_wait_for_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800); 
    kit_cloudkicker_get_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800, FBE_TRUE);
    kit_cloudkicker_delete_zeroing_hook(spare_pvd_object_id, spare_pvd_info.default_offset+0x800);

    /* delete all the hooks */
    for(index=0; index < rg_config_p->width; index ++) 
    {
        if(index == drive_pos_to_replace) 
        {
            continue;
        }
        
        fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus,
                                                       rg_config_p->rg_disk_set[index].enclosure,
                                                       rg_config_p->rg_disk_set[index].slot,
                                                       &pvd_object_id);

        fbe_test_sep_util_set_active_target_for_pvd(pvd_object_id);

        // setup the hoooks for the pvds to check for zeroing
        kit_cloudkicker_delete_zeroing_hook(pvd_object_id, pvd_info.default_offset+0x800);
    }

    fbe_test_sep_util_set_active_target_for_pvd(spare_pvd_object_id);    

	/* check that disk zeroing did not move */
    status = sep_zeroing_utils_check_disk_zeroing_status(spare_pvd_object_id, KIT_CLOUDKICKER_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kit_cloudkicker_check_rebuild_performed(rg_config_p, drive_pos_to_replace);
    number_physical_objects+=1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_pos_to_replace, 
                                                number_physical_objects, &drive_info);

    fbe_api_sim_transport_set_target_server(current_target);    

}
/***************************************************************
 * end kit_cloudkicker_swap_in_zeroed_permanent_spare_during_disk_zeroing()
 ***************************************************************/

/*!**************************************************************
 * kit_cloudkicker_setup_pvd_for_zeroing()
 ****************************************************************
 * @brief
 *  disable zeroing and set the zero checkpoint to the beginning
 *
 * @param   bus - bus location of drive
 *          enclosure - enclosure location of drive    
 *          slot - slot location of drive
 *          pvd_obj_id - pointer to the pvd object id from the bus, slot, enclosure
 *          pvd_info - pointer to the pvd info
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_setup_pvd_for_zeroing(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
										   fbe_object_id_t * pvd_obj_id, fbe_api_provision_drive_info_t * pvd_info) {
	fbe_status_t        status = FBE_STATUS_OK;
	fbe_u32_t           timeout_ms = 30000;

	/* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(bus, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

	/* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus, enclosure, slot, 
															pvd_obj_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_provision_drive_get_info(*pvd_obj_id, pvd_info);

	/* Disable the default background zeroing operation since
	 * we explicitly enable a zeroing operation here.
	 */
	status = fbe_test_sep_util_provision_drive_disable_zeroing(*pvd_obj_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* Now set the checkpoint
     */ 
	status = fbe_api_provision_drive_set_zero_checkpoint(*pvd_obj_id, pvd_info->default_offset);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/***************************************************************
 * end kit_cloudkicker_setup_pvd_for_zeroing()
 ***************************************************************/

/*!**************************************************************
 * kit_cloudkicker_set_zeroing_hook()
 ****************************************************************
 * @brief
 *  Set a zeroing hook for pvd object id
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for      
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_set_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint) {
	fbe_status_t						status = FBE_STATUS_OK;

	status = fbe_api_scheduler_add_debug_hook(object_id,
    										  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    										  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    										  lba_checkpoint,
    										  NULL,
    										  SCHEDULER_CHECK_VALS_LT,
    										  SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end kit_cloudkicker_set_zeroing_hook()
 ******************************************/

/*!**************************************************************
 * kit_cloudkicker_get_zeroing_hook()
 ****************************************************************
 * @brief
 *  Check if the zeroing hook counter
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for   
 *          was_hit - flag to check if the hook was hit or not hit      
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_get_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint, fbe_bool_t was_hit) {
	fbe_scheduler_debug_hook_t          hook;
	fbe_status_t						status = FBE_STATUS_OK;

	mut_printf(MUT_LOG_TEST_STATUS,
		   "Getting Permanent Spare Debug Hook OBJ %d LBA %llu",
		   object_id, (unsigned long long)lba_checkpoint);

	hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_VALS_LT;
	hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
	hook.val1 = lba_checkpoint;
	hook.val2 = NULL;
	hook.counter = 0;

	status = fbe_api_scheduler_get_debug_hook(&hook);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
		   (unsigned long long)hook.counter);

    if (was_hit) {
        MUT_ASSERT_UINT64_NOT_EQUAL(0, hook.counter);
    } else {
        MUT_ASSERT_UINT64_EQUAL(0, hook.counter);
    }
}
/******************************************
 * end kit_cloudkicker_get_zeroing_hook()
 ******************************************/

/*!**************************************************************
 * kit_cloudkicker_delete_zeroing_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for         
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_delete_zeroing_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint) {
	fbe_status_t status = FBE_STATUS_OK;

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook OBJ %d LBA %llu",
		   object_id, (unsigned long long)lba_checkpoint);

	status = fbe_api_scheduler_del_debug_hook(object_id,
											  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
											  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
											  lba_checkpoint,
											  NULL,
											  SCHEDULER_CHECK_VALS_LT,
											  SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end kit_cloudkicker_delete_zeroing_hook()
 ******************************************/

/*!**************************************************************
 * kit_cloudkicker_wait_for_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for         
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_wait_for_hook(fbe_object_id_t object_id, fbe_lba_t lba_checkpoint) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;

    hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_VALS_LT;
	hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
	hook.val1 = lba_checkpoint;
	hook.val2 = NULL;
	hook.counter = 0;

    status = sep_zeroing_utils_wait_for_hook(&hook, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end kit_cloudkicker_wait_for_hook()
 ******************************************/
/*!**************************************************************
 * kit_cloudkicker_set_rebuild_hook()
 ****************************************************************
 * @brief
 *  Verify that the rebuild hook was hit
 *
 * @param  rg_config_p - rg to remove the rebuild hook from                
 *
 * @return None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_set_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p) {
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

	mut_printf(MUT_LOG_TEST_STATUS, "Setting Rebuild Hook for OBJ %d", rg_object_id);

    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_LOG);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end kit_cloudkicker_set_rebuild_hook()
 ******************************************/

/*!**************************************************************
 * kit_cloudkicker_check_rebuild_hook()
 ****************************************************************
 * @brief
 *  Verify that the rebuild hook was hit
 *
 * @param   rg_config_p - rg to remove the rebuild hook from            
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_check_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p) 
{
    fbe_scheduler_debug_hook_t          hook;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_VALS_LT;
    hook.action = SCHEDULER_DEBUG_ACTION_LOG;
    hook.val1 = 0;
	hook.val2 = NULL;
	hook.counter = 0;

    status = sep_zeroing_utils_wait_for_hook(&hook, KIT_CLOUDKICKER_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_scheduler_get_debug_hook(&hook);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS,
	       "Rebuild Hook counter for object id %d = %llu", rg_object_id,
	       (unsigned long long)hook.counter);
    MUT_ASSERT_UINT64_NOT_EQUAL(0, hook.counter);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
}
/******************************************
 * end kit_cloudkicker_check_rebuild_hook()
 ******************************************/

/*!**************************************************************
 * kit_cloudkicker_remove_rebuild_hook()
 ****************************************************************
 * @brief
 *  Remove the hook to the target SP
 *
 * @param   rg_config_p - the raid group config to remove the hook from            
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_remove_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p) 
{
    fbe_object_id_t                     raid_group_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);

    fbe_test_sep_util_get_active_passive_sp(raid_group_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Rebuild Debug Hook. rg: %d objid: %d", 
               rg_config_p->raid_group_id, raid_group_object_id);

    status = fbe_api_scheduler_del_debug_hook(raid_group_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_LOG);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      
}
/******************************************
 * end kit_cloudkicker_remove_rebuild_hook()
 ******************************************/
/*!**************************************************************
 * kit_cloudkicker_check_rebuild_performed()
 ****************************************************************
 * @brief
 *  Verify that a rebuild was performed and completed
 *
 * @param   rg_config_p - the raid group config to remove the hook from 
 *          drive_pos_to_replace - drive position to check rb checkpoint           
 *
 * @return  None
 *
 * @author
 *  12/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void kit_cloudkicker_check_rebuild_performed(fbe_test_rg_configuration_t* rg_config_p, fbe_u32_t drive_pos_to_replace) {
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    kit_cloudkicker_check_rebuild_hook(rg_config_p); 
    kit_cloudkicker_remove_rebuild_hook(rg_config_p); 
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos_to_replace);
    sep_rebuild_utils_check_bits(rg_object_id);
}
/******************************************
 * end kit_cloudkicker_check_rebuild_performed()
 ******************************************/

/*!****************************************************************************
 * kit_cloudkicker_test()
 ******************************************************************************
 * @brief
 *  Run permanent sparing zeroing tests on raid group configs
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  12/01/2011 - Created. Deanna Heng
 ******************************************************************************/
void kit_cloudkicker_dualsp_test(void)
{
    fbe_u32_t i = 0;  
    fbe_u32_t testing_level = fbe_sep_test_util_get_raid_testing_extended_level();    
    kit_cloudkicker_test_index_t test_index = 0;     

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);


    /* rtl 1 and rtl 0 are the same for now */
    if (testing_level > 0) {
        for(i=0; i<KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST; i++) {    
            kit_cloudkicker_test_index_t test_index = (kit_cloudkicker_test_index_t) i;  
            
            if (i + 1 >= KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST) {
                           
               fbe_test_run_test_on_rg_config_with_extra_disk(&kit_cloudkicker_raid_group_config_random[0],
                                                              &test_index,kit_cloudkicker_run_tests,
                                                              KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP,
                                                              KIT_CLOUDKICKER_CHUNKS_PER_LUN);
            }
            else {
               fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(&kit_cloudkicker_raid_group_config_random[0],
                                                              &test_index,kit_cloudkicker_run_tests,
                                                              KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP,
                                                              KIT_CLOUDKICKER_CHUNKS_PER_LUN,
                                                              FBE_FALSE);
            }
        }
    } else {
        // Randomly choose test cases to run
        for (i=0; i<KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST; i++) {
            test_index = (kit_cloudkicker_test_index_t) (fbe_random() % KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST);
            if (i + 1 >= KIT_CLOUDKICKER_PERMANENT_SPARE_ZEROING_TEST_LAST) {

               fbe_test_run_test_on_rg_config_with_extra_disk(&kit_cloudkicker_raid_group_config_random[0],
                                                              &test_index,kit_cloudkicker_run_tests,
                                                              KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP,
                                                              KIT_CLOUDKICKER_CHUNKS_PER_LUN);
            }
            else {

               fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(&kit_cloudkicker_raid_group_config_random[0],
                                                              &test_index,kit_cloudkicker_run_tests,
                                                              KIT_CLOUDKICKER_LUNS_PER_RAID_GROUP,
                                                              KIT_CLOUDKICKER_CHUNKS_PER_LUN,
                                                              FBE_FALSE);
            }
        }
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/***************************************************************
 * end kit_cloudkicker_test()
 ***************************************************************/

/*!****************************************************************************
 *  kit_cloudkicker_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the kit_cloudkicker test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  11/30/2011 - Created. Deanna Heng
 *****************************************************************************/
void kit_cloudkicker_dualsp_setup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t raid_group_count = 1;
        
        darkwing_duck_create_random_config(&kit_cloudkicker_raid_group_config_random[0], raid_group_count);

        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&kit_cloudkicker_raid_group_config_random[0]);

        /* initialize the number of extra drive required by each rg 
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(&kit_cloudkicker_raid_group_config_random[0]);

        /* Setup the physical config for the raid groups
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(&kit_cloudkicker_raid_group_config_random[0], 
                                           raid_group_count);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(&kit_cloudkicker_raid_group_config_random[0], 
                                           raid_group_count);

        sep_config_load_sep_and_neit_load_balance_both_sps();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);        
    }

    // update the permanent spare trigger timer so we don't need to wait long for the swap
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/***************************************************************
 * end kit_cloudkicker_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  kit_cloudkicker_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the kit_cloudkicker test.
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  11/30/2011 - Created. Deanna Heng
 *****************************************************************************/
void kit_cloudkicker_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/***************************************************************
 * end kit_cloudkicker_dualsp_cleanup()
 ***************************************************************/
