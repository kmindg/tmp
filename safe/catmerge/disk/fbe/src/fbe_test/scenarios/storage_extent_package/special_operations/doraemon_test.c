/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file doraemon_test.c
 ****************************************************************************
 *
 * @brief
 *  This file contains test for special handling of system drives.
 *
 * @todo
 *  - Add test cases for mark NR failure, If NR succeeds then commit to DB failure.
 *  - If required add a case for HS to larger drive and then user copy to original size.
 *  - 
 *
 ***************************************************************************/

#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"
#include "mut.h"   
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_metadata.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "neit_utils.h"
#include "EmcPAL_Misc.h"


char * doraemon_short_desc = "test special handling for system drives";
char * doraemon_long_desc =
    "\n"
    "\n"
    "The Doraemon Scenario tests that for System RG no sparing occurs.\n"
    "Sparing happens only for User RGs on System drives.\n"
    "\n"
    "\
    /* Case 1 - System RG Drive Insert Remove : (Tested with Same & Drive drive insert.) \
    doraemon_system_rg_remove_insert_tests() \
    1.1 - Remove drive and insert drive with No reboot. \
    1 - Remove a randomly selected system drive. \
    2 - Write a pattern to PSM LUN. \
    3 - Insert system drive. \
    4 - Let PSM rebuild complete. \
    5 - Read and validate pattern from drive inserted drive. \
 \
    1.2 - Pull drive, reboot and insert drive when system up. \
    1 - Remove a randomly selected system drive. \
    2 - Write a pattern to PSM LUN. \
    3 - Reboot the SP. \
    3 - Insert system drive. \
    4 - Let PSM rebuild complete. \
    5 - Read and validate pattern from inserted drive. \
 \
    1.3 - Pull, Shutdown SP, Insert drive when system down, bringup. \
    1 - Remove a randomly selected system drive. \
    2 - Write a pattern to PSM LUN. \
    3 - Shutdown the SP. \
    4 - Insert system drive when system is down. \
    5 - Bringup the SP. \
    6 - Let PSM rebuild complete. \
    7 - Read and validate pattern from inserted drive. \
 \
    Case 2 - User RG Drive Insert Remove : (Tested with Same & Drive drive insert.) \
    doraemon_user_rg_remove_insert_tests() \
    1 - Create raid groups with LUs of different types. (One type in one iteration) \
    2 - Remove a randomly selected system drive. \
    3 - Wait for HS to kick in. \
    4 - Wait for rebuild on HS to complete. \
    5 - Insert system drive. \
    6 - Initiate user copy back operation. \
    7 - Destroy the raid group. \
 \
    Case 3 - User copy rejection test. \
    doraemon_proactive_copy_rejection_test() \
    1 - Create a raid group on non system disks. \
    2 - Remove a randomly selected disk from this RG. \
    3 - Wait for HS to kick in. \
    4 - Let the HS rebuild. \
    5 - Initiate a user copy from the HS drive to a randomly selected system drive. \
    6 - User copy shall fail as the RG was not created in system drives. \
    7 - Destroy the raid group. \
 \
    Case 4 - Spare selection test. \
    doraemon_spare_selection_test() \
    1 - Create a raid group on system drives. \
    2 - Remove all drives larger than or equal to the system drive size. \
    3 - Remove a randomly selected system drive. \
    4 - No hot spare should swap as there is no suitable sized disk available. \
    5 - Destroy the raid group. \
 \
    Case 5 - PVD reinit failure test. \
    doraemon_pvd_validation_failure_test() \
    1 - Create a raid group on system drives. \
    2 - Remove all dries larger than or euqal to system drive size. \
    3 - Remove a randomly selected drive 'X' wait for RL to activate. \
    4 - Remove drive 'Y' next to earlier removed drive. \
    5 - Isert a new drive in slot of drive 'Y'. \
    6 - The PVD for this new drive fail to init. \
    7 - Original original drives in slot 'X' & 'Y'. \
    8 - Both PVDs should become Ready and Raid group too becoms ready. \
    9 - Destroy the raid group. \
 \
    Case 6 - Drive movement test. \
    doraemon_drive_moevement_test() \
    1 - Create a raid group on non system drives. \
    2 - Remove a randomly selected system drive 'X'. \
    3 - Remove a ramdomly selected drive 'Y' from user RG. \
    4 - Insert drive 'X' into slot 'Y'. \
    5 - The PVD fails to become ready as it is a DB drive in user slot. \
    6 - Insert drive 'Y' into slot 'X'. \
    7 - The drive gets consumed in DB slot and PVD comes ready. \
    8 - Remove drive 'X' earlier inserted and reinsert it. \
    9 - This time drive 'X' PVD becomes ready as now this drive is no longer required for DB. \
    10 - Destroy the raid group. \
 \
Description last updated: 05/25/2012.\n";

/*!*******************************************************************
 * @def DORAEMON_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define DORAEMON_MAX_WIDTH                  16

/*!*******************************************************************
 * @def DORAEMON_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define DORAEMON_LUNS_PER_RAID_GROUP        3

/*!*******************************************************************
 * @def DORAEMON_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Chunks per LUNs.
 *********************************************************************/
#define DORAEMON_CHUNKS_PER_LUN             2

/*!*******************************************************************
 * @def DORAEMON_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.  
 *
 *********************************************************************/
#define DORAEMON_FIRST_POSITION_TO_REMOVE   0

/*!*******************************************************************
 * @def DORAEMON_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The second array position to remove a drive for.
 *
 *********************************************************************/
#define DORAEMON_SECOND_POSITION_TO_REMOVE  1


#define DORAEMON_TEST_DRIVE_COUNT           30
#define DORAEMON_DRIVES_IN_ENCL_0           6

#define DORAEMON_NUM_OF_TRIP_MIRROR_DRIVES  3

#define DORAEMON_IO_BLOCK_COUNT             64

#define DORAEMON_MAX_DRIVE_HANDLES          5

#define DORAEMON_SYSTEM_DRIVE_CAPACITY      (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)


/*!*******************************************************************
 * @var doraemon_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t doraemon_raid_group_config[] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0, {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, }, },
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0, {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, {0,0,4},}, },
//    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            2,          0, {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, }, },
    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0, {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, }, },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

fbe_test_rg_configuration_t doraemon_no_system_drive_raid_group_config = 
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0, {{0,1,0}, {0,1,1}, {0,1,2}, {0,1,3}, {0,1,4}, }, };

fbe_test_rg_configuration_t doraemon_all_system_drive_raid_group_config = 
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0, {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, }, };


fbe_api_terminator_device_handle_t doraemon_drive_handle[DORAEMON_MAX_DRIVE_HANDLES];
fbe_api_terminator_device_handle_t doraemon_peer_drive_handle[DORAEMON_MAX_DRIVE_HANDLES];

fbe_u32_t   rand_slot_tm;
fbe_u32_t   rand_slot_sys;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static void doraemon_test_run_io(fbe_object_id_t      object_id,
                                 fbe_lba_t            start_lba,
                                 fbe_u32_t            blocks,
                                 fbe_block_size_t     block_size,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_data_pattern_flags_t pattern);


/*****************************************
 * External Function prototypes
 *****************************************/
void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                        fbe_edge_index_t spare_edge_index,
                                                        fbe_test_raid_group_disk_set_t * spare_location_p);

extern void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);


/* @todo PJP fix this to enumerate the PSL and get suitable LU when lun_info.flags
 * is made available.
 */
fbe_private_space_layout_object_id_t doraemon_get_system_lu_oid(void)
{
    return FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_DDBS;
}


fbe_lba_t   doraemon_get_system_lu_offset_on_pvd(void)
{
    static fbe_lba_t    system_lu_offset = 0;
    fbe_status_t        status;
    fbe_private_space_layout_object_id_t system_lu_oid;
    fbe_private_space_layout_lun_info_t  lun_info;
    fbe_private_space_layout_region_t    region_info;

    if(0 == system_lu_offset)
    {
        // Get object id of system lun we want to use.
        system_lu_oid = doraemon_get_system_lu_oid();
    
        // Get the RG offset on disk and and LU offset in RG.
        status = fbe_private_space_layout_get_lun_by_object_id(system_lu_oid, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_private_space_layout_get_region_by_raid_group_id(lun_info.raid_group_id, &region_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        MUT_ASSERT_INT_EQUAL(region_info.raid_info.raid_type, FBE_RAID_GROUP_TYPE_RAID1);

        system_lu_offset = region_info.starting_block_address + lun_info.raid_group_address_offset;
    }

    return system_lu_offset;
}



static void doraemon_remove_drive(fbe_u32_t     bus,
                                  fbe_u32_t     encl,
                                  fbe_u32_t     slot,
                                  fbe_bool_t    pull_drive,
                                  fbe_u32_t     drive_index,
                                  fbe_bool_t    verify_pvd)
{
    fbe_status_t                        status;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_object_id_t                     pvd_object_id;

    if(verify_pvd)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    

    if(pull_drive)
    {
        MUT_ASSERT_TRUE(drive_index < DORAEMON_MAX_DRIVE_HANDLES)

        if(fbe_test_util_is_simulation())
        {
            if(fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_sim_transport_connection_target_t this_sp, peer_sp;

                this_sp = fbe_api_sim_transport_get_target_server();
                peer_sp = (this_sp == FBE_SIM_SP_B)? FBE_SIM_SP_A : FBE_SIM_SP_B;

                fbe_api_sim_transport_set_target_server(peer_sp);

                status = fbe_test_pp_util_pull_drive(bus, encl, slot, &doraemon_peer_drive_handle[drive_index]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                fbe_api_sim_transport_set_target_server(this_sp);

                /*! @todo - ARS 488569
                 * Temporary added delay between pull system drive on both SPs.
                 * We have open task where HomeWrecker code perform read from one SP and write 
                 * on other SP concurrently for reinsert system drive which lead both SP panic for IO Collision.
                 * Following delay code needs to remove once we complete the above task where HomeWrecker IO needs
                 * to synchronize across SP using CMI channel as decided by architecture folks.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "%s pull drive %d_%d_%d", __FUNCTION__, bus, encl, slot);
                /*if((bus==0) && (encl==0) && (slot<4))
                {
                    fbe_api_sleep(3000);
                }*/
            }

            status = fbe_test_pp_util_pull_drive(bus, encl, slot, &doraemon_drive_handle[drive_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            status = fbe_test_pp_util_pull_drive_hw(bus, encl, slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);           
        }
    }
    else
    {
        MUT_ASSERT_TRUE(drive_index == -1)


        if(fbe_test_util_is_simulation())
        {
            if(fbe_test_sep_util_get_dualsp_test_mode())
            {

                fbe_sim_transport_connection_target_t this_sp, peer_sp;

                this_sp = fbe_api_sim_transport_get_target_server();
                peer_sp = (this_sp == FBE_SIM_SP_B)? FBE_SIM_SP_A : FBE_SIM_SP_B;

                fbe_api_sim_transport_set_target_server(peer_sp);
                status = fbe_api_terminator_get_drive_handle(bus, encl, slot, &drive_handle);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                fbe_api_terminator_remove_sas_drive(drive_handle);

                fbe_api_sim_transport_set_target_server(this_sp);

                /*! @todo - ARS 488569
                 * Temporary added delay between remove system drive on both SPs.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "%s remove drive %d_%d_%d", __FUNCTION__, bus, encl, slot);
                /*if((bus==0) && (encl==0) && (slot<4))
                {
                    fbe_api_sleep(3000);
                }*/
                
            }

            status = fbe_api_terminator_get_drive_handle(bus, encl, slot, &drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            fbe_api_terminator_remove_sas_drive(drive_handle);
        }
        else
        {
                MUT_FAIL_MSG("Doraemon test is not completely supported on Hardware");
        }
       

    }

    if(verify_pvd)
    {
        // Wait for PVD to become failed.
        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    }
    
}


static void doraemon_insert_drive(fbe_u32_t     bus,
                                  fbe_u32_t     encl,
                                  fbe_u32_t     slot,
                                  fbe_bool_t    pull_drive,
                                  fbe_u32_t     drive_index,
                                  fbe_lba_t     drive_capacity,
                                  fbe_bool_t    verify_pvd)
{
    fbe_status_t                        status;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_terminator_sas_drive_info_t sas_drive;

    if(pull_drive)
    {
        MUT_ASSERT_TRUE(drive_index < DORAEMON_MAX_DRIVE_HANDLES)
        MUT_ASSERT_TRUE(drive_capacity == 0)

        if(fbe_test_util_is_simulation())
        {
            if(fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_sim_transport_connection_target_t this_sp, peer_sp;

                this_sp = fbe_api_sim_transport_get_target_server();
                peer_sp = (this_sp == FBE_SIM_SP_B)? FBE_SIM_SP_A : FBE_SIM_SP_B;

                fbe_api_sim_transport_set_target_server(peer_sp);

                status = fbe_test_pp_util_reinsert_drive(bus, encl, slot, doraemon_peer_drive_handle[drive_index]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                fbe_api_sim_transport_set_target_server(this_sp);

                /*! @todo - ARS 488569
                 * Temporary added delay between reinserting system drive on both SPs.
                 * We have open task where HomeWrecker code perform read from one SP and write 
                 * on other SP concurrently for reinsert system drive which lead both SP panic for IO Collision.
                 * Following delay code needs to remove once we complete the above task where HomeWrecker IO needs
                 * to synchronize across SP using CMI channel as decided by architecture folks.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "%s reinsert drive %d_%d_%d", __FUNCTION__, bus, encl, slot);
                /*if((bus==0) && (encl==0) && (slot<4))
                {
                    fbe_api_sleep(3000);
                }*/

            }
          
            status = fbe_test_pp_util_reinsert_drive(bus, encl, slot, doraemon_drive_handle[drive_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(bus, encl, slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        MUT_ASSERT_TRUE(drive_index == -1)
        MUT_ASSERT_FALSE(drive_capacity == 0)

        if(fbe_test_util_is_simulation())
        {
            if(fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_sim_transport_connection_target_t this_sp, peer_sp;

                this_sp = fbe_api_sim_transport_get_target_server();
                peer_sp = (this_sp == FBE_SIM_SP_B)? FBE_SIM_SP_A : FBE_SIM_SP_B;

                fbe_api_sim_transport_set_target_server(peer_sp);

                status = fbe_test_pp_util_get_unique_sas_drive_info(bus, encl, slot,
                                                                  520, 
                                                                  &drive_capacity,
                                                                  &drive_handle,
                                                                  &sas_drive);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                status = fbe_test_pp_util_insert_unique_sas_drive_for_dualsp(bus, encl, slot,
                                                                  520, 
                                                                  drive_capacity,
                                                                  &drive_handle,
                                                                  sas_drive);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


                fbe_api_sim_transport_set_target_server(this_sp);
                /*! @todo - ARS 488569
                 * Temporary added delay between inserting system drive on both SPs.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "%s insert NEW drive in %d_%d_%d", __FUNCTION__, bus, encl, slot);
                /*if((bus==0) && (encl==0) && (slot<4))
                {
                    fbe_api_sleep(3000);
                }*/
                
            }

            if(fbe_test_sep_util_get_dualsp_test_mode())
            {
                status = fbe_test_pp_util_insert_unique_sas_drive_for_dualsp(bus, encl, slot,
                                                                  520, 
                                                                  drive_capacity,
                                                                  &drive_handle,
                                                                  sas_drive);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            
            else
            {
                status = fbe_test_pp_util_insert_unique_sas_drive(bus, encl, slot,
                                                  520, 
                                                  drive_capacity,
                                                  &drive_handle);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            
        }
        else
        {
            MUT_FAIL_MSG("Doraemon test is not completely supported on Hardware");
        }

    }

    if(verify_pvd)
    {
        // Wait for PVD to become ready.
        status = fbe_test_sep_drive_verify_pvd_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}


void doreamon_wait_for_rb_to_progress(fbe_object_id_t in_raid_group_object_id,
                                      fbe_u32_t       in_position,
                                      fbe_lba_t       offset_lba)
{

    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;

    max_retries = 1200;

    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%x ==", 
            in_raid_group_object_id, in_position, (unsigned int)raid_group_info.rebuild_checkpoint[in_position]);
        
        if (raid_group_info.rebuild_checkpoint[in_position] == FBE_LBA_INVALID)
        {
            return;
        }

        if((raid_group_info.rebuild_checkpoint[in_position] < raid_group_info.paged_metadata_start_lba) &&
            (raid_group_info.rebuild_checkpoint[in_position] > offset_lba))
        {
            return;
        }

        fbe_api_sleep(500);
    }

    MUT_ASSERT_TRUE(FALSE);
}


void doraemon_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                     fbe_u32_t num_4160_drives,
                                                     fbe_block_count_t drive_capacity)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t num_objects = 1; /* start with board */
    fbe_u32_t encl_number = 0;
    fbe_u32_t encl_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;
    fbe_u32_t num_first_enclosure_drives = DORAEMON_DRIVES_IN_ENCL_0;

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port_handle);

    /* We start off with:
     *  1 board
     *  1 port
     *  1 enclosure
     *  plus one pdo for each of the first 10 drives.
     */
    num_objects = 3 + ( num_first_enclosure_drives); 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + ( num_520_drives);
    num_objects += num_4160_enclosures + ( num_4160_drives);

    /* First enclosure has 4 drives for system rgs.
     */
    fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
    for (slot = 0; slot < num_first_enclosure_drives; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity, &drive_handle);
    }

    /* We inserted one enclosure above, so we are starting with encl #1.
     */
    encl_number = 1;

    /* Insert enclosures and drives for 520.  Start at encl number one.
     */
    for ( encl_index = 0; encl_index < num_520_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);

        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_520_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, drive_capacity, &drive_handle);
            num_520_drives--;
            slot++;
        }
        encl_number++;
    }
    /* Insert enclosures and drives for 4160.
     * We pick up the enclosure number from the prior loop. 
     */
    for ( encl_index = 0; encl_index < num_4160_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_4160_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity, &drive_handle);
            num_4160_drives--;
            slot++;
        }
        encl_number++;
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == num_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    return;
}


static void doraemon_write_pattern_remove_drive(fbe_object_id_t object_id,
                                                fbe_u32_t       bus,
                                                fbe_u32_t       encl,
                                                fbe_u32_t       slot,
                                                fbe_bool_t      is_same_drive)
{
    /* For same drive pull/re-insert we write the pattern to LU before removing 
     * the drive. For removing a drive and inserting a different drive case we 
     * write the pattern after removing the drive.
     */
    if(is_same_drive)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Writing zero pattern to LU", __FUNCTION__);
        // Write zero pattern to the LU.
        doraemon_test_run_io(object_id,
                             0,
                             DORAEMON_IO_BLOCK_COUNT,
                             FBE_METADATA_BLOCK_SIZE,
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO,
                             FBE_DATA_PATTERN_FLAGS_INVALID);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Writing pattern to LU", __FUNCTION__);

        // Write data pattern to the LU.
        doraemon_test_run_io(object_id,
                             0,
                             DORAEMON_IO_BLOCK_COUNT,
                             FBE_METADATA_BLOCK_SIZE,
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                             FBE_DATA_PATTERN_FLAGS_INVALID);
    }

    // pull a system drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in %d_%d_%d", __FUNCTION__, bus, encl, slot);
    doraemon_remove_drive(bus, encl, slot, is_same_drive, (is_same_drive)? 0 : -1, TRUE);

    if(is_same_drive)
    {
        // Write data pattern to the LU.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Writing pattern to LU", __FUNCTION__);

        doraemon_test_run_io(object_id,
                             0,
                             DORAEMON_IO_BLOCK_COUNT,
                             FBE_METADATA_BLOCK_SIZE,
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                             FBE_DATA_PATTERN_FLAGS_INVALID);
    }
}


void doraemon_wait_for_system_rg_rebuild(fbe_u32_t   position)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t index;
    fbe_private_space_layout_region_t region;

    mut_printf(MUT_LOG_TEST_STATUS, "Test all private space RGs ...\n");
    for (index = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST; index <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST; index++)
    {
        status = fbe_private_space_layout_get_region_by_object_id(index, &region);
        if(status == FBE_STATUS_OK) {
            doreamon_wait_for_rb_to_progress(index, 
                                             position, 
                                             doraemon_get_system_lu_offset_on_pvd() + DORAEMON_IO_BLOCK_COUNT);
        }
        else {
            //EmcpalDebugBreakPoint();
        }
    }
}


static void doraemon_system_rg_no_reboot_remove_insert_tests(fbe_bool_t is_same_drive)
{
    fbe_u32_t                          rand_slot = rand_slot_tm;

    mut_printf(MUT_LOG_TEST_STATUS, "==== Drive Remove Insert Test (Only Sys RG 1) Pull and insert %s drive with no reboot. ===", is_same_drive? "Same" : "Different");

    // Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);


    // Write a pattern and remove a random system drive.
    doraemon_write_pattern_remove_drive(doraemon_get_system_lu_oid(),
                                        0, 0, rand_slot,
                                        is_same_drive);

    // Wait for 5 seconds to give time for a HS to swap in if it wants. Though it shouldn't swap.
    fbe_api_sleep(5000);

    
    // Reinsert system drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                          (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, TRUE);

    // Wait for system RGs to rebuild.
    doraemon_wait_for_system_rg_rebuild(rand_slot);

    // Read the pattern from PSM LU to validate that the pattern matches.
    doraemon_test_run_io(doraemon_get_system_lu_oid(), 
                         0,
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);

    // Read the pattern using PVD to validate that the pattern matches.
    doraemon_test_run_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + rand_slot, 
                         doraemon_get_system_lu_offset_on_pvd(),
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Pattern read from 0_0_%d OK", __FUNCTION__, rand_slot);

    mut_printf(MUT_LOG_TEST_STATUS, "==== End Drive Remove Insert Test (Only Sys RG 1) Pull and insert %s drive with no reboot. ===", is_same_drive? "Same" : "Different");    
}


static void doraemon_system_rg_remove_reboot_insert_tests(fbe_bool_t is_same_drive)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_u32_t                          rand_slot = rand_slot_tm;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Remove Insert Test (Only Sys RG 2) Pull drive, Reboot & then insert %s drive. ===", is_same_drive? "Same" : "Different");

    // Write a pattern and remove a random system drive.
    doraemon_write_pattern_remove_drive(doraemon_get_system_lu_oid(),
                                        0, 0, rand_slot,
                                        is_same_drive);

    // Wait for 5 seconds to give time for a HS to swap in if it wants. Though it shouldn't swap.
    fbe_api_sleep(5000);    

    // Reboot SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Rebooting SEP", __FUNCTION__);    
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();

    // Reinsert system drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                          (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, TRUE);

    // Wait for the system raid groups to rebuild the drive.
    doraemon_wait_for_system_rg_rebuild(rand_slot);

    // Read the pattern from PSM LU to validate that the pattern matches.
    doraemon_test_run_io(doraemon_get_system_lu_oid(), 
                         0,
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);

    // Read the pattern using PVD to validate that the pattern matches.
    doraemon_test_run_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + rand_slot, 
                         doraemon_get_system_lu_offset_on_pvd(),
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Pattern read from 0_0_%d OK", __FUNCTION__, rand_slot);


    // For different drive case do a reboot and validate the PVD.
    if(!is_same_drive)
    {
        // Reboot SEP.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Rebooting SEP", __FUNCTION__);    
        fbe_test_sep_util_destroy_neit_sep();
        sep_config_load_sep_and_neit();

        // Wait for PVD to become ready.
        status = fbe_test_sep_drive_verify_pvd_state(0, 0, rand_slot, FBE_LIFECYCLE_STATE_READY, 30000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }


    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Remove Insert Test (Only Sys RG 2) Pull drive, Reboot & then insert %s drive. ===", is_same_drive? "Same" : "Different");
}


static void doraemon_system_rg_remove_shutdown_insert_bringup_tests(fbe_bool_t is_same_drive)
{
    fbe_u32_t                          rand_slot = rand_slot_tm;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Remove Insert Test (Only Sys RG 3) Pull drive, Shutdown SP, Insert %s drive and bringup system. ===", is_same_drive? "Same" : "Different");

    // Write a pattern and remove a random system drive.
    doraemon_write_pattern_remove_drive(doraemon_get_system_lu_oid(),
                                        0, 0, rand_slot,
                                        is_same_drive);

    // Wait for 5 seconds to give time for a HS to swap in if it wants. Though it shouldn't swap.
    fbe_api_sleep(5000);

    // Shutdown SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Shutting down SEP", __FUNCTION__);
    fbe_test_sep_util_destroy_neit_sep();

    // Reinsert system drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                          (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, FALSE);

    // Reloading SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reloading SEP", __FUNCTION__);    
    sep_config_load_sep_and_neit();

    // Wait for the system raid groups to rebuild the drive.
    doraemon_wait_for_system_rg_rebuild(rand_slot);


    // Read the pattern from PSM LU to validate that the pattern matches.
    doraemon_test_run_io(doraemon_get_system_lu_oid(), 
                         0,
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);

    // Read the pattern using PVD to validate that the pattern matches.
    doraemon_test_run_io(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + rand_slot, 
                         doraemon_get_system_lu_offset_on_pvd(),
                         DORAEMON_IO_BLOCK_COUNT,
                         FBE_METADATA_BLOCK_SIZE,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                         FBE_DATA_PATTERN_FLAGS_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Pattern read from 0_0_%d OK", __FUNCTION__, rand_slot);

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Drive Remove Insert Test (Only Sys RG 3) Pull drive, Shutdown SP, Insert %s drive and bringup system. ===", is_same_drive? "Same" : "Different");
}


static void doraemon_user_rg_no_reboot_remove_insert_tests(fbe_bool_t is_same_drive)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   index = 0;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             src_pvd_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_test_rg_configuration_t *rg_config_p = doraemon_raid_group_config;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   dest_index;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_u32_t                       rand_slot = rand_slot_sys;
    fbe_database_raid_group_info_t  rg_info;

    mut_printf(MUT_LOG_TEST_STATUS, "==== Drive Remove Insert Test (User RG 1) Pull and insert %s drive with no reboot. ===", is_same_drive? "Same" : "Different");

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    rg_count = (sizeof(doraemon_raid_group_config) / sizeof(fbe_test_rg_configuration_t)) - 1;

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      rg_count,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    // Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    while(doraemon_raid_group_config[index].width !=FBE_U32_MAX)
    {
        rg_config_p = &doraemon_raid_group_config[index];
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

        // pull system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_remove_drive(0, 0, rand_slot, is_same_drive, (is_same_drive)? 0 : -1, TRUE);

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

        // Get hotspare position VD object id.
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rand_slot, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        // Wait for hotspare to swap in.
        status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         30000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s: HS swap-in, VD obj-0x%x is in ready state! ==", __FUNCTION__, vd_object_id);

        // Wait for the Raid group to rebuild the drive.
        sep_rebuild_utils_wait_for_rb_comp_by_obj_id(rg_object_id, rand_slot);

        // Reinsert system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                              (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, TRUE);

        // Get source and destination pvd ids for user copy.
        status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);         
        src_pvd_object_id = rg_info.pvd_list[rand_slot];           

        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, rand_slot, &dest_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_test_copy_determine_dest_index(&vd_object_id, 1, &dest_index);

        // Initiate copy back operation.
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy back rg: 0x%x src_pvd: 0x%x dest_pvd: 0x%x==", rg_object_id, src_pvd_object_id, dest_pvd_object_id);

        /* trigger the copy back operation  */
        status = fbe_api_copy_to_replacement_disk(src_pvd_object_id, dest_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index, &spare_drive_location);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);        

        // Destroy RG.
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

        index++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "==== End Drive Remove Insert Test (User RG 1) Pull and insert %s drive with no reboot. ===", is_same_drive? "Same" : "Different");    
}


static void doraemon_user_rg_remove_reboot_insert_tests(fbe_bool_t is_same_drive)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   index = 0;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             src_pvd_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_test_rg_configuration_t *rg_config_p = doraemon_raid_group_config;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   dest_index;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_database_raid_group_info_t  rg_info;
    fbe_u32_t                       rand_slot = rand_slot_sys;  

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Remove Insert Test (User RG 2) Pull drive, Reboot & then insert %s drive. ===", is_same_drive? "Same" : "Different");

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    rg_count = (sizeof(doraemon_raid_group_config) / sizeof(fbe_test_rg_configuration_t)) - 1;

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      rg_count,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    while(doraemon_raid_group_config[index].width !=FBE_U32_MAX)
    {
        // Speed up VD hot spare
        fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

        rg_config_p = &doraemon_raid_group_config[index];
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

        // pull system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_remove_drive(0, 0, rand_slot, is_same_drive, (is_same_drive)? 0 : -1, TRUE);

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

        // Get hotspare position VD object id.
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rand_slot, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        // Wait for hotspare to swap in.
        status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         30000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s: HS swap-in, VD obj-0x%x is in ready state! ==", __FUNCTION__, vd_object_id);

        // Wait for the Raid group to rebuild the drive.
        sep_rebuild_utils_wait_for_rb_comp_by_obj_id(rg_object_id, rand_slot);

        // Reboot SEP.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Rebooting SEP", __FUNCTION__);    
        fbe_test_sep_util_destroy_neit_sep();
        sep_config_load_sep_and_neit();

        // Reinsert system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                              (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, TRUE);

        // Get source and destination pvd ids for user copy.
        status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);         
        src_pvd_object_id = rg_info.pvd_list[rand_slot];           

        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, rand_slot, &dest_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_test_copy_determine_dest_index(&vd_object_id, 1, &dest_index);

        // Initiate copy back operation.
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy back rg: 0x%x src_pvd: 0x%x dest_pvd: 0x%x==", rg_object_id, src_pvd_object_id, dest_pvd_object_id);

        /* trigger the copy back operation  */
        status = fbe_api_copy_to_replacement_disk(src_pvd_object_id, dest_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index, &spare_drive_location);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);        

        // Destroy RG.
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

        index++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Drive Remove Insert Test (User RG 2) Pull drive, Reboot & then insert %s drive. ===", is_same_drive? "Same" : "Different");
}


static void doraemon_user_rg_remove_shutdown_insert_bringup_tests(fbe_bool_t is_same_drive)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   index = 0;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             src_pvd_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_test_rg_configuration_t *rg_config_p = doraemon_raid_group_config;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   dest_index;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_database_raid_group_info_t  rg_info;
    fbe_u32_t                      rand_slot = rand_slot_sys;    

    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Remove Insert Test (User RG 3) Pull drive, Shutdown SP, Insert %s drive and bringup system. ===", is_same_drive? "Same" : "Different");

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    rg_count = (sizeof(doraemon_raid_group_config) / sizeof(fbe_test_rg_configuration_t)) - 1;

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      rg_count,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    while(doraemon_raid_group_config[index].width !=FBE_U32_MAX)
    {
        // Speed up VD hot spare
        fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

        rg_config_p = &doraemon_raid_group_config[index];
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

        // pull system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_remove_drive(0, 0, rand_slot, is_same_drive, (is_same_drive)? 0 : -1, TRUE);

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

        // Get hotspare position VD object id.
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rand_slot, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        // Wait for hotspare to swap in.
        status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         30000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s: HS swap-in, VD obj-0x%x is in ready state! ==", __FUNCTION__, vd_object_id);

        // Wait for the Raid group to rebuild the drive.
        sep_rebuild_utils_wait_for_rb_comp_by_obj_id(rg_object_id, rand_slot);

        // Shutdown SEP.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Shutting down SEP", __FUNCTION__);
        fbe_test_sep_util_destroy_neit_sep();        

        // Reinsert system drive.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
        doraemon_insert_drive(0, 0, rand_slot, is_same_drive, 
                              (is_same_drive)? 0 : -1, (is_same_drive)? 0 : DORAEMON_SYSTEM_DRIVE_CAPACITY, FALSE);

        // Reloading SEP.
        mut_printf(MUT_LOG_TEST_STATUS, "%s Reloading SEP", __FUNCTION__);    
        sep_config_load_sep_and_neit();

        // Get source and destination pvd ids for user copy.
        status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);         
        src_pvd_object_id = rg_info.pvd_list[rand_slot];           

        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, rand_slot, &dest_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_test_copy_determine_dest_index(&vd_object_id, 1, &dest_index);

        // Initiate copy back operation.
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy back rg: 0x%x src_pvd: 0x%x dest_pvd: 0x%x==", rg_object_id, src_pvd_object_id, dest_pvd_object_id);

        /* trigger the copy back operation  */
        status = fbe_api_copy_to_replacement_disk(src_pvd_object_id, dest_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index, &spare_drive_location);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);        

        // Destroy RG.
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

        index++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Drive Remove Insert Test (User RG 3) Pull drive, Shutdown SP, Insert %s drive and bringup system. ===", is_same_drive? "Same" : "Different");    
}


static void doraemon_proactive_copy_rejection_test(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         src_pvd_object_id = FBE_OBJECT_ID_INVALID;    
    fbe_test_rg_configuration_t             *rg_config_p = &doraemon_no_system_drive_raid_group_config;
    fbe_u32_t                               dest_index;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_u32_t                               rand_slot = rand_slot_sys;
    fbe_database_raid_group_info_t          rg_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start Proactive Copy Rejection ===");

    fbe_test_sep_util_init_rg_configuration(rg_config_p);

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      1,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    // Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

    // pull a drive in RG.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in 0_1_%d", __FUNCTION__, rand_slot);
    doraemon_remove_drive(0, 1, rand_slot, TRUE, 0, TRUE);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    // Get hotspare position VD object id.
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rand_slot, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    // Wait for hotspare to swap in.
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: HS swap-in, VD obj-0x%x is in ready state! ==", __FUNCTION__, vd_object_id);

    // Wait for the Raid group to rebuild the drive.
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(rg_object_id, rand_slot);

    // Reinsert RG drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_1_%d", __FUNCTION__, rand_slot);
    doraemon_insert_drive(0, 1, rand_slot, TRUE, 0, 0, TRUE);  

    // Get source pvd id for user copy.
    status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    src_pvd_object_id = rg_info.pvd_list[rand_slot];

    // Let destination PVD id be a randomly selected system drive.
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, rand_slot, &dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_copy_determine_dest_index(&vd_object_id, 1, &dest_index);

    // Initiate copy back operation.
    mut_printf(MUT_LOG_TEST_STATUS, "== starting copy back rg: 0x%x src_pvd: 0x%x dest_pvd: 0x%x==", rg_object_id, src_pvd_object_id, dest_pvd_object_id);

    // Trigger the copy back operation it must fail.
    status = fbe_api_copy_to_replacement_disk(src_pvd_object_id, dest_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    // Destroy RG.
    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Proactive Copy Rejection ===");    
}


static void doraemon_spare_selection_test(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t             *rg_config_p = &doraemon_all_system_drive_raid_group_config;
    fbe_u32_t                               rand_slot = rand_slot_sys;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start Spare Selection Test ===");

    fbe_test_sep_util_init_rg_configuration(rg_config_p);

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      1,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    // Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

    // Remove all drives large enough to be spare candidates.
    doraemon_remove_drive(0, 0, 4, TRUE, 0, TRUE);
    doraemon_remove_drive(0, 0, 5, TRUE, 1, TRUE);

    // pull a drive in RG.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Remove drive in 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_remove_drive(0, 0, rand_slot, TRUE, 2, TRUE);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    // Get hotspare position VD object id.
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rand_slot, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    // Wait for hotspare to swap in, no hotspare should swap in.
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: No HS swap-in, VD obj-0x%x ! ==", __FUNCTION__, vd_object_id);

    // Reinsert RG drive.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Inserting drive in 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_insert_drive(0, 0, rand_slot, TRUE, 2, 0, TRUE);

    // Wait for VD to become ready.
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Wait for the Raid group to rebuild the drive.
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(rg_object_id, rand_slot);


    doraemon_insert_drive(0, 0, 5, TRUE, 1, 0, TRUE);
    doraemon_insert_drive(0, 0, 4, TRUE, 0, 0, TRUE);    
    
    // Destroy RG.
    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Spare Selection Test ===");
}


static void doraemon_pvd_validation_failure_test(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t             *rg_config_p = &doraemon_all_system_drive_raid_group_config;
    fbe_u32_t                               rand_slot = rand_slot_sys;
    fbe_u32_t                               second_slot;
    fbe_object_id_t                     pvd_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start PVD Validation Failure Test ===");
    
    // Second slot is the slot next to randomly selsected first slot.
    second_slot = rand_slot + 1;
    if(second_slot == fbe_private_space_layout_get_number_of_system_drives())
    {
        second_slot = 0;
    }

    fbe_test_sep_util_init_rg_configuration(rg_config_p);

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      1,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    // Speed up VD hot spare
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

    // Remove all drives large enough to be spare candidates.
    doraemon_remove_drive(0, 0, 4, TRUE, 0, TRUE);
    doraemon_remove_drive(0, 0, 5, TRUE, 1, TRUE);

    // Pull first drive in RG and wait for RL to start.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Remove first drive @ 0_0_%d", __FUNCTION__, rand_slot);
    doraemon_remove_drive(0, 0, rand_slot, TRUE, 2, TRUE);
    sep_rebuild_utils_verify_rb_logging(rg_config_p, rand_slot);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, second_slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Pull second drive in RG.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Remove second drive @ 0_0_%d", __FUNCTION__, second_slot);
    doraemon_remove_drive(0, 0, second_slot, TRUE, 3, TRUE);

    // Insert a new drive in slot.
    doraemon_insert_drive(0, 0, second_slot, FALSE, -1, DORAEMON_SYSTEM_DRIVE_CAPACITY, FALSE);

    // @todo PJP - Need to replace this with a hook once Ashwins changes are in.
    fbe_api_sleep(5000);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Remove new drive and insert original drive. PVD validation must succeed this time.
    doraemon_remove_drive(0, 0, second_slot, FALSE, -1, FALSE);
    doraemon_insert_drive(0, 0, second_slot, TRUE, 3, 0, TRUE);

    // Insert the first original drive.
    doraemon_insert_drive(0, 0, rand_slot, TRUE, 2, 0, TRUE); 

    doraemon_insert_drive(0, 0, 5, TRUE, 1, 0, TRUE);
    doraemon_insert_drive(0, 0, 4, TRUE, 0, 0, TRUE);    

    // Destroy RG.
    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "=== End PVD Validation Failure Test ===");
}


static void doraemon_drive_moevement_test(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t             *rg_config_p = &doraemon_no_system_drive_raid_group_config;
    fbe_u32_t                               rand_slot = rand_slot_tm;
    fbe_object_id_t                         pvd_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start Drive Movement Test ===");


    // Swap the large drive in 0_0_4 with random slot drive in encl 1.
    doraemon_remove_drive(0, 0, 4, TRUE, 0, TRUE);
    doraemon_remove_drive(0, 1, rand_slot, TRUE, 1, TRUE);
    doraemon_insert_drive(0, 0, 4, TRUE, 1, 0, TRUE);
    doraemon_insert_drive(0, 1, rand_slot, TRUE, 0, 0, TRUE);


    fbe_test_sep_util_init_rg_configuration(rg_config_p);

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      1,
                                                      DORAEMON_CHUNKS_PER_LUN, 
                                                      DORAEMON_LUNS_PER_RAID_GROUP);

    // Prevent any hot spares from kicking in.
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1000);

    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, 1);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, rand_slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    mut_printf(MUT_LOG_TEST_STATUS, "%s: removing system drive 0_0_%d\n", __FUNCTION__, rand_slot);

    // Pull a drive from the system drives.
    doraemon_remove_drive(0, 0, rand_slot, TRUE, 0, TRUE);

    // Pull a drive from the user RG on non system drives.
    doraemon_remove_drive(0, 1, rand_slot, TRUE, 1, TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Inserting system drive in 0_1_%d\n", __FUNCTION__, rand_slot);

    // Insert the pulled system drive in the RG slot.
    doraemon_insert_drive(0, 1, rand_slot, TRUE, 0, 0, FALSE);

    // Give a little time to let pvd attach if it wants to.
    fbe_api_sleep(5000);    

    // The PVD should stay in FAIL state as we have inserted a DB drive in user slot.
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Insert the removed user RG drive in system slot and it should get pvd ready.
    doraemon_insert_drive(0, 0, rand_slot, TRUE, 1, 0, TRUE);

    // Wait for the system RGs to rebuild.
    doraemon_wait_for_system_rg_rebuild(rand_slot);

    // Remove the drive in the user slot that failed to become ready and reinsert it.
    doraemon_remove_drive(0, 1, rand_slot, TRUE, 0, FALSE);
    doraemon_insert_drive(0, 1, rand_slot, TRUE, 0, 0, TRUE);

    // Destroy RG.
    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);

    // Swap the drive in 0_0_4 with random slot drive in encl 1.
    doraemon_remove_drive(0, 0, 4, TRUE, 0, TRUE);
    doraemon_remove_drive(0, 1, rand_slot, TRUE, 1, TRUE);
    doraemon_insert_drive(0, 0, 4, TRUE, 1, 0, TRUE);
    doraemon_insert_drive(0, 1, rand_slot, TRUE, 0, 0, TRUE);
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== End Drive Movement Test ===");
}


/*!**************************************************************
 * doraemon_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the doraemon test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *  05/02/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doraemon_setup(void)
{        
    mut_printf(MUT_LOG_LOW, "=== Doreamon setup ===\n");	

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;

        rg_config_p = &doraemon_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        /* Load the physical package and create the physical configuration. 
         */
        doraemon_create_physical_config_for_disk_counts(DORAEMON_TEST_DRIVE_COUNT,
                                                        0,
                                                        TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;

}


/*!****************************************************************************
 * doraemon_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/02/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void doraemon_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * doraemon_user_test()
 ******************************************************************************
 * @brief
 *  Run Doreamon scenario for special handling of system drives.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/02/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void doraemon_user_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    rand_slot_sys = fbe_random() % fbe_private_space_layout_get_number_of_system_drives();

    // Remove and reinsert same system drive with user RGs bound on system drive
    doraemon_user_rg_no_reboot_remove_insert_tests(TRUE);
    doraemon_user_rg_remove_reboot_insert_tests(TRUE);
    doraemon_user_rg_remove_shutdown_insert_bringup_tests(TRUE);

    if(test_level > 0)
    {
        // Remove and reinsert a new system drive with user RGs bound on system drive
        doraemon_user_rg_no_reboot_remove_insert_tests(FALSE);
        doraemon_user_rg_remove_reboot_insert_tests(FALSE);
        doraemon_user_rg_remove_shutdown_insert_bringup_tests(FALSE);    
    }

    // Run proactive copy rejection test.
    doraemon_proactive_copy_rejection_test();

    // Run spare selection test.
    doraemon_spare_selection_test();

    // Run PVD validation failure test.
    doraemon_pvd_validation_failure_test();

    return;
}


/*!****************************************************************************
 * doraemon_system_test()
 ******************************************************************************
 * @brief
 *  Run Doreamon scenario for special handling of system drives.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/02/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void doraemon_system_test(void)
{
    rand_slot_tm = fbe_random() % DORAEMON_NUM_OF_TRIP_MIRROR_DRIVES;

    // Remove and reinsert system drive. No user RGs
    doraemon_system_rg_no_reboot_remove_insert_tests(TRUE);
    doraemon_system_rg_remove_reboot_insert_tests(TRUE);
    doraemon_system_rg_remove_shutdown_insert_bringup_tests(TRUE);

    // Remove and reinsert a new system drive. No user RGs
     doraemon_system_rg_no_reboot_remove_insert_tests(FALSE);
     doraemon_system_rg_remove_reboot_insert_tests(FALSE);
     doraemon_system_rg_remove_shutdown_insert_bringup_tests(FALSE);

    // Drive movement test.
     doraemon_drive_moevement_test(); 

    return;
}


/*!****************************************************************************
 * doraemon_user_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run dual SP doreamon scenario for special handling of system drives.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/14/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void doraemon_user_dualsp_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    doraemon_user_test();

    return;
}


/*!****************************************************************************
 * doraemon_system_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run dual SP doreamon scenario for special handling of system drives.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/14/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void doraemon_system_dualsp_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    doraemon_system_test();

    return;
}


/*!**************************************************************
 *  doraemon_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup the doraemon test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/14/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doraemon_dualsp_setup(void)
{    
    fbe_sim_transport_connection_target_t sp;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        rg_config_p = &doraemon_raid_group_config[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        /* Load the physical package and create the physical configuration. 
         */
        doraemon_create_physical_config_for_disk_counts(DORAEMON_TEST_DRIVE_COUNT,
                                                        0,
                                                        TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);

         /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        doraemon_create_physical_config_for_disk_counts(DORAEMON_TEST_DRIVE_COUNT,
                                                        0,
                                                        TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}


/*!**************************************************************
 *  doraemon_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the doraemon test for dual-SP testing.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 05/14/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doraemon_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}


/*!**************************************************************
 *  doraemon_test_run_io()
 ****************************************************************
 * @brief
 *  Issue I/O to the given object at the given LBA for the specified
 *  number of blocks.
 *
 * @param object_id - object ID of object to issue I/O to.
 * @param start_lba - starting LBA for I/O request
 * @param blocks    - number of blocks for I/O request
 * @param block_size - number of bytes per block
 * @param opcode    - type of I/O request (read or write)
 * @param pattern   - pattern for writes
 *
 * @return None.
 *
 * @note copied from hermione test.
 *
 ****************************************************************/
static void doraemon_test_run_io(fbe_object_id_t      object_id,
                                 fbe_lba_t            start_lba,
                                 fbe_u32_t            blocks,
                                 fbe_block_size_t     block_size,
                                 fbe_payload_block_operation_opcode_t opcode,
                                 fbe_data_pattern_flags_t pattern)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t * sg_list_p;
    fbe_u8_t * buffer;
    fbe_payload_block_operation_t               block_operation;
    fbe_block_transport_block_operation_info_t  block_operation_info;
    fbe_raid_verify_error_counts_t              verify_err_counts={0};
    fbe_u64_t                                   corrupt_blocks_bitmap= 0;
    fbe_data_pattern_info_t                     data_pattern_info;
    fbe_u32_t                                   sg_element_count;

    /* Allocate memory for a 2-element sg list.
     */
    buffer = fbe_api_allocate_memory(blocks * block_size + 2*sizeof(fbe_sg_element_t));
    sg_list_p = (fbe_sg_element_t *)buffer;

    /* Create sg list.
     */
    sg_list_p[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list_p[0].count = blocks * block_size;
    sg_list_p[1].address = NULL;
    sg_list_p[1].count = 0;

    sg_element_count = fbe_sg_element_count_entries(sg_list_p);    

    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         FBE_DATA_PATTERN_LBA_PASS,
                                         pattern,
                                         (fbe_u32_t)0,
                                         0x55,  /* sequence id used in seed; arbitrary */
                                         0,     /* no header words */
                                         NULL   /* no header array */);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (pattern == FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
    {
        corrupt_blocks_bitmap = (fbe_u64_t)-1;
    }

    /* For a write request, set data pattern in sg list.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
    {
        status = fbe_data_pattern_fill_sg_list(sg_list_p,
                                               &data_pattern_info,
                                               block_size,
                                               corrupt_blocks_bitmap, /* Corrupt data corrupts all blocks */
                                               sg_element_count);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    }

    /* Create appropriate payload.
     */
    fbe_payload_block_build_operation(&block_operation,
                                      opcode,
                                      start_lba,
                                      blocks,
                                      block_size,
                                      1, /* optimum block size */
                                      NULL);

    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_err_counts;

    mut_printf(MUT_LOG_TEST_STATUS, "=== sending IO Object: 0x%x Opcode: %d\n", object_id, opcode);

    status = fbe_api_block_transport_send_block_operation(object_id, 
                                                          FBE_PACKAGE_ID_SEP_0, 
                                                          &block_operation_info, 
                                                          sg_list_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    /* For a read request, validate pattern read in sg list.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        status = fbe_data_pattern_check_sg_list(sg_list_p,
                                               &data_pattern_info,
                                               block_size,
                                               corrupt_blocks_bitmap, /* Corrupt data corrupts all blocks */
                                               object_id,
                                               sg_element_count,
                                               TRUE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    }    

    return;
}

