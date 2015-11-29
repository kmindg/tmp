/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file doodle_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test for raid group downstream health verify and mark 
 *  rebuild logging in activate state.
 *
 *
 ***************************************************************************/


#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"   

char * doodle_short_desc = "Test RAID downstream health in activate state and mark rl if needed.";
char * doodle_long_desc =
    "\n"
    "\n"
    "The doodle Scenario tests that when a RAID group is coming up and it is in activate state, it\n"
    "evaluates the downstream health and marks rebuild logging if needed.\n"
    "\n"
    "Dependencies:\n"
    "    - Persistent meta-data storage.\n"    
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 5 SAS drives (PDO)\n"
    "    [PP] 5 logical drives (LD)\n"
    "    [SEP] 5 provision drives (PD)\n"
    "    [SEP] 5 virtual drives (VD)\n"
    "    [SEP] 1 raid object (RAID)\n"
    "    [SEP] 3 lun objects (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create a raid object with I/O edges attached to all VDs.\n"
    "    - Create each lun object with an I/O edge attached to the RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Shutdown RAID group\n"
    "    - Pull enough drives so that RG goes broken\n"
    "    - Make sure that Rebuild logging is not set\n"
    "    - Bring up the RG degraded \n"
    "    - Verify that Rebuild logging is set for missing drives in activate state\n"
    "\n"
    "STEP 3: Testing of powerup timer\n"
    "    - Pull enough drives so that RG goes broken\n" 
    "    - Make sure that Rebuild logging is not set\n"
    "    - Bring up the drives one by one \n"
    "    - Make sure that the rebuild logging get set \n"
    "      if the drives does not power up before the powerup timer expires\n"
    "\n"    
    "STEP 4: Handling of events in activate state\n"
    "    - Pull enough drives so that RG goes broken\n"
    "    - Make sure that Rebuild logging is not set\n"
    "    - Bring up the RG degraded\n"
    "    - While we are in activate state, pull drives\n"
    "      and make sure that event handling sets various conditions\n"
    "      in activate rotary\n"
    "\n"
    "STEP 5: drive remove during SP down\n"
    "    - Shutdown the SP\n"
    "    - Remove a drive\n"
    "    - Boot up the SP\n"
    "    - Verify that Rebuild logging is set for missing drives in activate state\n"    
    "\n"
    "STEP 6: SP reboot before mark NR\n"
    "    - Pull a drive so that RG goes degraded\n"
    "    - configure a drive as spare so it gets swapped in permanently.\n"
    "    - reboot the SP before NR mark start\n"    
    "    - When SP comes up, verify that entire disk marked as NR\n"
    "\n"
    "STEP 7: Destroy raid groups and LUNs.\n"
    " \n\
    Description last updated: 10/28/2011.\n";

/*!*******************************************************************
 * @def DOODLE_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define DOODLE_MAX_WIDTH 16

/*!*******************************************************************
 * @def DOODLE_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define DOODLE_RAID_GROUP_COUNT   5

/*!*******************************************************************
 * @def DOODLE_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define DOODLE_LUNS_PER_RAID_GROUP    3

/*!*******************************************************************
 * @def DOODLE_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS
 *********************************************************************
 * @brief Capacity of the virtual drives.
 *********************************************************************/
/* Treat the drive capacity as small to end up with a raid group
 * capacity that is relatively small also. 
 * For comparison, a typical 32 mb sim drive is about 0xF000 blocks. 
 */
#define DOODLE_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/*!*******************************************************************
 * @def DOODLE_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.  This should
 *        not be the position to inject the error on!!
 *
 *********************************************************************/
#define DOODLE_FIRST_POSITION_TO_REMOVE   0

/*!*******************************************************************
 * @def DOODLE_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define DOODLE_SECOND_POSITION_TO_REMOVE  1

/*!*******************************************************************
 * @def DOODLE_THIRD_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 3thrd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define DOODLE_THIRD_POSITION_TO_REMOVE  2

/*!*******************************************************************
 * @def DOODLE_TEST_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define DOODLE_TEST_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def DOODLE_TEST_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define DOODLE_TEST_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @def DOODLE_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define DOODLE_WAIT_MSEC 50000


/*!*******************************************************************
 * @def DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MIN
 *********************************************************************
 *  @brief 
 *  Minimum wait time to test raid group activate degraded wait timer expire duration
 *  This wait time is with reference of FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC.
 *
 *********************************************************************/
#define DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MIN  3


/*!*******************************************************************
 * @def DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MAX
 *********************************************************************
 *  @brief 
 *  Maximum wait time to test raid group activate degraded wait timer expire duration
 *  This wait time is with reference of FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC.
 *
 *********************************************************************/
#define DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MAX  20



#define DOODLE_CHUNKS_PER_LUN  2

/*!*******************************************************************
 * @var doodle_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t doodle_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var doodle_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t doodle_raid_group_config_extended[] = 
{
    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},

    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0},

    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            2,          0},

    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            3,          0},

    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            4,          0},
      
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


typedef enum doodle_test_case_enum_e 
{
    DOODLE_MARK_RL_IN_ACTIVE_STATE = 0,
    DOODLE_POWER_UP_TIMER_TEST,
    DOODLE_SET_CONDITION_IN_ACTIVE_STATE,
    DOODLE_DRIVE_REMOVED_DURING_SHUTDOWN_MARK_RL,
    DOODLE_SP_RESTART_BEFORE_NR_MARK,
    DOODLE_SP_RESTART_DURING_REBUILD,
    DOODLE_DRIVE_RETURNS_BEFORE_VERIFY,
    DOODLE_TC_COUNT,
} doodle_test_case_enum_t;

/*!*************************************************
 * @struct doodle_test_case_s
 ***************************************************
 * @brief doodle test case structure
 *
 ***************************************************/
typedef struct doodle_test_case_s
{
    doodle_test_case_enum_t test_case;  /* The name of the test case */
    fbe_object_id_t rg_object_id; /* Object to insert events on */
    fbe_u32_t num_to_remove; /* Number of drives to remove in the rg */
    fbe_u32_t drive_slots_to_remove[3]; /* The slot position to rebuild in the rg*/
    fbe_api_terminator_device_handle_t drive_info[3]; /*drive info for removed drives*/
}
doodle_test_case_t;
/**************************************
 * end doodle_test_case_s
 **************************************/
 
/*!*******************************************************************
 * @var doodle_test_cases
 *********************************************************************
 * @brief This is the set of test cases to be run
 *
 *********************************************************************/
doodle_test_case_t doodle_test_cases[DOODLE_TC_COUNT] = 
{
    {DOODLE_MARK_RL_IN_ACTIVE_STATE,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_POWER_UP_TIMER_TEST,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_SET_CONDITION_IN_ACTIVE_STATE,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_DRIVE_REMOVED_DURING_SHUTDOWN_MARK_RL,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_SP_RESTART_BEFORE_NR_MARK,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_SP_RESTART_DURING_REBUILD,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    },
    {DOODLE_DRIVE_RETURNS_BEFORE_VERIFY,
     FBE_OBJECT_ID_INVALID, 
     FBE_U32_MAX,
     {0, 1, 2},
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
    }
};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static void doodle_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

static void doodle_rg_broken(fbe_test_rg_configuration_t * in_current_rg_config_p,doodle_test_case_t *test_case_p);

static void doodle_ins_drives_mark_rl_in_activate(fbe_test_rg_configuration_t * in_current_rg_config_p,doodle_test_case_t *test_case_p);
static void doodle_reboot_dualsp(void);
static void doodle_reboot_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t state, fbe_u32_t substate);
static void doodle_add_debug_hook(fbe_object_id_t rg_object_id, fbe_u32_t state, fbe_u32_t substate);
fbe_status_t doodle_wait_for_target_SP_hook(fbe_object_id_t rg_object_id,fbe_u32_t state, fbe_u32_t substate);
fbe_status_t doodle_delete_debug_hook(fbe_object_id_t rg_object_id, fbe_u32_t state, fbe_u32_t substate);
fbe_u32_t doodle_get_data_drive_count_to_shutdown_rg(fbe_test_rg_configuration_t *rg_config_p);
static void doodle_sp_restart_during_rebuild_test(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p);
fbe_status_t doodle_wait_for_target_SP_hook_with_val(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate,
                                                   fbe_u32_t check_type,
                                                   fbe_u64_t val1,
                                                   fbe_u64_t val2);

extern fbe_u32_t sep_rebuild_utils_number_physical_objects_g;
/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 *  doodle_reinsert_drive
 ******************************************************************************
 *
 * @brief
 *   This function re-inserts a drive.
 *
 * @param rg_config_p        - pointer to the RG configuration info
 * @param position_to_insert    - disk position in the RG
 * @param in_drive_info_p       - pointer to structure with the "drive info" for the
 *                                drive
 * @return None
 *
 *****************************************************************************/
static void doodle_reinsert_drive(fbe_test_rg_configuration_t * rg_config_p, fbe_u32_t position_to_insert, fbe_api_terminator_device_handle_t* in_drive_info_p)
{
    fbe_u32_t           status;
    fbe_sim_transport_connection_target_t   local_sp;                       // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                        // peer SP id


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
    
    /*  Insert the drive */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[position_to_insert].bus,
                                        rg_config_p->rg_disk_set[position_to_insert].enclosure,
                                        rg_config_p->rg_disk_set[position_to_insert].slot,
                                        *in_drive_info_p);
        
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[position_to_insert].bus,
                                            rg_config_p->rg_disk_set[position_to_insert].enclosure,
                                            rg_config_p->rg_disk_set[position_to_insert].slot,
                                            *in_drive_info_p);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(rg_config_p->rg_disk_set[position_to_insert].bus,
                                           rg_config_p->rg_disk_set[position_to_insert].enclosure,
                                           rg_config_p->rg_disk_set[position_to_insert].slot);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            fbe_test_pp_util_reinsert_drive_hw(rg_config_p->rg_disk_set[position_to_insert].bus,
                                               rg_config_p->rg_disk_set[position_to_insert].enclosure,
                                               rg_config_p->rg_disk_set[position_to_insert].slot);

            /* Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //  Print message as to know where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Reinserted Drive: %d_%d_%d", 
               rg_config_p->rg_disk_set[position_to_insert].bus,
               rg_config_p->rg_disk_set[position_to_insert].enclosure,
               rg_config_p->rg_disk_set[position_to_insert].slot);
}

/*!****************************************************************************
 *  doodle_remove_drive
 ******************************************************************************
 *
 * @brief
 *   This function removes a drive.
 *
 * @param rg_config_p           - pointer to the RG configuration info
 * @param position_to_remove    - disk position in the RG
 * @param out_drive_info_p      - pointer to structure with the "drive info" for the
 *                                drive
 * @return None
 *
 *****************************************************************************/
static void doodle_remove_drive(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_u32_t position_to_remove,
                                fbe_api_terminator_device_handle_t* out_drive_info_p)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   local_sp;               // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

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
    
    /*  Remove the drive */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position_to_remove].bus,
                                             rg_config_p->rg_disk_set[position_to_remove].enclosure,
                                             rg_config_p->rg_disk_set[position_to_remove].slot,
                                             out_drive_info_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and remove the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position_to_remove].bus,
                                                 rg_config_p->rg_disk_set[position_to_remove].enclosure,
                                                 rg_config_p->rg_disk_set[position_to_remove].slot,
                                                 out_drive_info_p);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(rg_config_p->rg_disk_set[position_to_remove].bus,
                                                rg_config_p->rg_disk_set[position_to_remove].enclosure,
                                                rg_config_p->rg_disk_set[position_to_remove].slot);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and remove the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive_hw(rg_config_p->rg_disk_set[position_to_remove].bus,
                                                    rg_config_p->rg_disk_set[position_to_remove].enclosure,
                                                    rg_config_p->rg_disk_set[position_to_remove].slot);

            /* Set the target server back to the local SP.*/
            fbe_api_sim_transport_set_target_server(local_sp);
        }       
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to know where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
               rg_config_p->rg_disk_set[position_to_remove].bus,
               rg_config_p->rg_disk_set[position_to_remove].enclosure,
               rg_config_p->rg_disk_set[position_to_remove].slot);
}

/*!**************************************************************************
 * doodle_rg_activate_set_conditions_test
 ***************************************************************************
 * @brief
 *  This function tests that raid group event handling needs to set various 
 *  conditions in the ACTIVATE state if we remove drive while raid group is
 *  in ACTIVATE state.
 * 
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void doodle_rg_activate_set_conditions_test(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;
    fbe_u32_t                          max_tries;
    fbe_lifecycle_state_t              current_state;
    fbe_u32_t                          drive_count = 0;

    max_tries = 100;
    status = fbe_api_get_object_lifecycle_state(test_case_p->rg_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (current_state != FBE_LIFECYCLE_STATE_READY)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rg object id: 0x%x is not in ready state.  state: 0x%x", 
                   test_case_p->rg_object_id, current_state);
        MUT_FAIL_MSG("Object not in expected state at start of test!");
    }
    doodle_rg_broken(rg_config_p,test_case_p);

    /* Add debug hook to avoid raid group to go into ready state */
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Add debug hook for activate eval RL.  This is needed so that we will wait until both drives are back before
     * marking rebuild logging.  It is possible with timing that the second drive takes a while to come back and we mark 
     * rl too soon.  Thus we'll wait for both drives to come up, wait for this hook and then clear it. 
     */
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);
    for(drive_count = 0; drive_count < test_case_p->num_to_remove;drive_count++)
    {
        /* Insert the drives */ 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
        sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], sep_rebuild_utils_number_physical_objects_g,
                                                    &test_case_p->drive_info[drive_count]);
    }
    /* Wait to get to eval rl in activate.  Clear the hook.
     */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id, 
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL, 
                                   FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);
    doodle_delete_debug_hook(test_case_p->rg_object_id,
                             SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,  
                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);


    // wait until the debug  hook hit
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* At this moment eval rebuild logging condition gets clear.
     * After remove drive while raid group is in activate state, event handling code
     * will set various conditions. So verify it with check that rebuild logging 
     * gets set again. It will not add any value to go and check each individual
     * condition. So lets add debug hook in raid group activate and check that eval rebuild
     * logging condition get executed.
     */

    /* Add debug hook for activate eval RL */
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Remove the drive */
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[0], sep_rebuild_utils_number_physical_objects_g,
                                              &test_case_p->drive_info[0]);

    /* wait for debug hook to hit */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL ,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Make sure we are still activate state when the rebuild logging was marked */
    fbe_api_get_object_lifecycle_state(test_case_p->rg_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_ACTIVATE, current_state);

    /* Insert the drives */ 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[0], sep_rebuild_utils_number_physical_objects_g,
                                                &test_case_p->drive_info[0]);

    /* delete debug hook RL */
    doodle_delete_debug_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* delete debug hook Rebuild activate */
    doodle_delete_debug_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,  FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Make sure we have transitioned to ready state once the hook is deleted */
    status = fbe_api_wait_for_object_lifecycle_state(test_case_p->rg_object_id, FBE_LIFECYCLE_STATE_READY, 50000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}   // End doodle_ins_drives_mark_rl_in_activate()

/*!**************************************************************************
 * doodle_powerup_timer_test
 ***************************************************************************
 * @brief
 *  This function tests powerup timer using following steps
 *      a) Pull enough drives so that RG goes broken
 *      b) Make sure that Rebuild logging is not set
 *      c) Bring up the drives one by one 
 *      d) Make sure that the rebuild logging does not get set
 *         if the drives power up before the powerup timer expires.
 * 
 * @param rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void doodle_powerup_timer_test(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                        status;
    fbe_u32_t                           drive_count =0;
    fbe_time_t                          elapsed_seconds_p = 0;
    fbe_time_t                          current_time;               // used to store the current time for comparison
    
    /* Pull enough drives so that RG goes broken and make sure that Rebuild logging is not set. */
    doodle_rg_broken(rg_config_p,test_case_p);
    
    /* Add debug hook to avoid raid group to go into ready state */
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Insert the drives */ 
    for(drive_count = 0; drive_count < test_case_p->num_to_remove -1;drive_count++)
    {
        /* Insert the drives */ 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
        doodle_reinsert_drive(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], &test_case_p->drive_info[drive_count]);
    }

    /* wait until the debug  hook hit */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Get the current time */
    current_time = fbe_get_time();

    /* Add debug hook */
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* delete debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* wait until the debug  hook hit */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);
    
    /*  Get the elapsed time since the timestamp */
    elapsed_seconds_p = fbe_get_elapsed_seconds(current_time);
    mut_printf(MUT_LOG_TEST_STATUS, "elapsed_seconds_p: %llu",
	       (unsigned long long)elapsed_seconds_p);

    /* check that missing drive get marked for RL in raid group activate after timer get expired. Raid group
     *  activate rotary has timer of FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC seconds to wait before
     *  mark drive for rebuild logging. This test code needs to verify that drive should not get marked for RL very soon
     *  or should not take longer time. Normally it could take +3/-1 seconds as there could be possible to run raid group
     *  activate rotaty in certain time cycle. 
     */

    /*!@ todo We have seen that when we run this test with background zeroing enabled, the raid activate timer takes little bit 
     *     longer time to expire(around 8 to 12 seconds) so that needs to be investigate. 
     */
    if((elapsed_seconds_p < DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MIN) ||
        (elapsed_seconds_p > DOODLE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC_MAX))
    {

        mut_printf(MUT_LOG_TEST_STATUS, "raid group activate degraded timer not expired in expected time, elapsed time:%llu Object id 0x%x",
		   (unsigned long long)elapsed_seconds_p, test_case_p->rg_object_id);
        MUT_FAIL();
    }

    /* delete debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Re-insert the drive */
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    doodle_reinsert_drive(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], &test_case_p->drive_info[drive_count]);

    /* Make sure we have transitioned to ready state once the hook is deleted */
    status = fbe_api_wait_for_object_lifecycle_state(test_case_p->rg_object_id, FBE_LIFECYCLE_STATE_READY, 50000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_count);
    return;
}   // End doodle_powerup_timer_test()


/*!**************************************************************
 * doodle_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t doodle_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = DOODLE_WAIT_MSEC;
    
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);

            status = fbe_api_sim_transport_set_target_server(current_target);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state: %d substate: %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end doodle_wait_for_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * doodle_wait_for_target_SP_hook_with_val()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor       
 *        check_type - debug hook type
 *        val1       - rebuild checkpoint
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  9/15/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t doodle_wait_for_target_SP_hook_with_val(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate,
                                                   fbe_u32_t check_type,
                                                   fbe_u64_t val1,
                                                   fbe_u64_t val2) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = DOODLE_WAIT_MSEC;
    
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = check_type;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);

            status = fbe_api_sim_transport_set_target_server(current_target);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state: %d substate: %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end doodle_wait_for_target_SP_hook()
 ******************************************/



/*!**************************************************************
 * doodle_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t doodle_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                      fbe_u32_t state, 
                                      fbe_u32_t substate) 
{
    fbe_status_t                        status = FBE_STATUS_OK;

    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: rg obj id: 0x%X state: %d, substate: %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id, 
                                              state, 
                                              substate, 
                                              NULL, 
                                              NULL, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return FBE_STATUS_OK;
}
/******************************************
 * end doodle_delete_debug_hook()
 ******************************************/
 
/*!**************************************************************
 * doodle_add_debug_hook()
 ****************************************************************
 * @brief
 *  Add the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void doodle_add_debug_hook(fbe_object_id_t rg_object_id, 
                                  fbe_u32_t state, 
                                  fbe_u32_t substate) 
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook: rg obj id 0x%X state %d substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_add_debug_hook(rg_object_id, 
                                              state, 
                                              substate, 
                                              NULL, 
                                              NULL, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end doodle_add_debug_hook()
 ******************************************/

/*!****************************************************************************
 *  doodle_reboot_sp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the doodle test. This function preset the
 *  debug hook before load sep and neit package so that SP boot up with debug hook
 *  set.
 *
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *        state - the state in the monitor 
 *        substate - the substate in the monitor 
 *
 * @return  None 
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 *****************************************************************************/
static void doodle_reboot_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t state, fbe_u32_t substate)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sep_package_load_params_t sep_params_p;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

    status = fbe_api_sim_transport_set_target_server(DOODLE_TEST_ACTIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the scheduler hook */
    sep_params_p.scheduler_hooks[0].object_id = rg_object_id;
    sep_params_p.scheduler_hooks[0].monitor_state = state;
    sep_params_p.scheduler_hooks[0].monitor_substate = substate;
    sep_params_p.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params_p.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params_p.scheduler_hooks[0].val1 = NULL;
    sep_params_p.scheduler_hooks[0].val2 = NULL;

    /* and this will be our signal to stop */
    sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

    /* shutdown the logical package - sep & neit*/
    fbe_test_sep_util_destroy_neit_sep();

    // let's wait awhile before bringing SPA back up
    fbe_api_sleep(2000);

    status = fbe_api_sim_transport_set_target_server(DOODLE_TEST_ACTIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);

    fbe_api_database_set_load_balance(FBE_TRUE);

    /* In doodle_test, it configures some particular PVDs as hot spares. In order to make configuring 
    particular PVDs as hot spares work, after reboot, we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);  

    return;
}
/******************************************
 * end doodle_reboot_sp()
 ******************************************/


/*!****************************************************************************
 *  doodle_reboot_dualsp
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the doodle test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 *****************************************************************************/
static void doodle_reboot_dualsp(void)
{
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep_both_sps();

    sep_config_load_sep_and_neit_load_balance_both_sps();
}
/******************************************
 * end doodle_reboot_dualsp()
 ******************************************/


/*!**************************************************************************
 * doodle_sp_restart_before_nr_mark_test
 ***************************************************************************
 * @brief
 * This test verify that if SP gets shutdown after permanent spare swap in complete
   and before NR mark then entire disk needs to mark for NR when SP come up.
 * This function does the following on a raid group:
 *   - Remove a disk
 *   - Add a debug hook to avoid marking of NR
 *   - configure a drive as spare to swap permanently
 *   - wait for debug hook to hit 
 *   - preset the debug hook to avoid rebuild start and then reboot the SP.
 *   - wait for the debug hook to hit
 *   - check that NR marked on entire disk
 *  
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 *
 ***************************************************************************/
static void doodle_sp_restart_before_nr_mark_test(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{

    fbe_status_t                       status;    
    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_chunk_index_t                   total_chunks;

    /*temporary not running this test case for R10 as well as dual sp*/
    if((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) || fbe_test_sep_util_get_dualsp_test_mode())
    {
        return;
    }

    /*  Set up a spare drive */
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                        rg_config_p->extra_disk_set[0].enclosure ,
                                        rg_config_p->extra_disk_set[0].slot); 

    /* get rg to the degraded mode */
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);
    
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, DOODLE_FIRST_POSITION_TO_REMOVE);

    /* wait for debug  hook to hit to confirm that RL marked for the drive */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /* Reboot SP before mark NR */
    doodle_reboot_sp(rg_config_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /*  Verify that the RAID and LUN objects are in the READY state */
    sep_rebuild_utils_check_raid_and_lun_states(rg_config_p, FBE_LIFECYCLE_STATE_READY);

    /* Wait for the debug hook to hit which was set during SP reboot to confirm that NR get marked */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    total_chunks = (raid_group_info.capacity / raid_group_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;

    status = fbe_api_raid_group_get_paged_bits(test_case_p->rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Check that NR should be mark on entire disk
     */
    if(paged_info.num_nr_chunks != total_chunks)
    {        
        mut_printf(MUT_LOG_TEST_STATUS, "doodle _nr_mark_test - expected chunk to mark NR %llu, NR marked for %llu chunk", (unsigned long long)total_chunks, (unsigned long long)paged_info.num_nr_chunks);
        mut_printf(MUT_LOG_TEST_STATUS, "rg id 0x%x, pos %d",test_case_p->rg_object_id,DOODLE_FIRST_POSITION_TO_REMOVE);

        MUT_FAIL_MSG("doodle - Fail to mark NR on entire disk");
    }

    /* delete the debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY );

    /*  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    /* Re-insert the removed drive */ 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                sep_rebuild_utils_number_physical_objects_g,
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

   
    mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted removed drive %d_%d_%d", rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].bus,
                                                                            rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].enclosure,
                                                                            rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].slot);
    fbe_test_sep_util_get_raid_group_disk_info(rg_config_p);

    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, 1);

    return;
}
/******************************************
 * end doodle_sp_restart_before_nr_mark_test()
 ******************************************/

/*!**************************************************************************
 * doodle_sp_restart_during_rebuild_test()
 ***************************************************************************
 * @brief
 * This test verify that if both SP gets shutdown/reboot during rebuild and when it 
 * come up, if it has valid rebuild non zero checkpoint, metadata and user data 
 * should rebuild successfully. There are possibility that Raid group persist it's
 * non paged metadata and along with it, it will persist rebuild checkpoint as well if
 * rebuild is in progress which is expected. 
 * Now if both SP rebooted or paniced in this situaltion, when it 
 * come up, first RG's metadata will mark to rebuild. So first metadata will rebuild and 
 * than user data. So will have following sequece which will test by this function.
 * 
 * - Start Rebuild
 * - Persist rebuild checkpoint explicitly in the middle of rebuild
 * (to generate scenario where RG persist it's non paged metadata during rebuild)
 * - Reboot SP while rebuild is in progress
 * - When SP come up, make sure that metadata will mark for rebuild in specialize
 * - verify that rebuild complete successfully.
 * - Ref ( AR 507123)
 *  
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 * @author
 *   09/15/2012 - Created. Amit Dhaduk
 *
 ***************************************************************************/
static void doodle_sp_restart_during_rebuild_test(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;


    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_chunk_index_t                   total_chunks;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    /* Based on the test level determine which configuration to use.
    */

    /* this test need to run only on single SP version and with RTL 0 */
    if(fbe_test_sep_util_get_dualsp_test_mode() || (test_level > 0))
    {
        return;
    }

    /* get rg to the degraded mode */
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    
    status = fbe_api_scheduler_add_debug_hook(test_case_p->rg_object_id,
            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
            FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
            0x800,
            NULL,
            SCHEDULER_CHECK_VALS_LT,
            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /*  Set up a spare drive */
    sep_rebuild_utils_setup_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                        rg_config_p->extra_disk_set[0].enclosure ,
                                        rg_config_p->extra_disk_set[0].slot); 

    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, DOODLE_FIRST_POSITION_TO_REMOVE);

    doodle_wait_for_target_SP_hook_with_val(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,SCHEDULER_CHECK_VALS_LT, 0x800, NULL); 

    /* persist rebuild checkpoint during rebuild */
    fbe_api_base_config_metadata_nonpaged_persist(test_case_p->rg_object_id);

    status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "== reb chkpt 0x%x before reboot rg object id: 0x%x pos: %d ==", 
                   (unsigned int)raid_group_info.rebuild_checkpoint[DOODLE_FIRST_POSITION_TO_REMOVE], test_case_p->rg_object_id, DOODLE_FIRST_POSITION_TO_REMOVE);

    /* Reboot SP during rebuild */
    doodle_reboot_sp(rg_config_p, SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    /*  Verify that the RAID and LUN objects are in the READY state */
    sep_rebuild_utils_check_raid_and_lun_states(rg_config_p, FBE_LIFECYCLE_STATE_READY);

    /* Wait for the debug hook to hit which was set during SP reboot to confirm that NR get marked */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY);

    status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* delete the debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY );

    /*  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, DOODLE_FIRST_POSITION_TO_REMOVE);

    total_chunks = (raid_group_info.capacity / raid_group_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;

    status = fbe_api_raid_group_get_paged_bits(test_case_p->rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If the rebuild is done, no chunks should be needing a rebuild.
     */
    if (paged_info.num_nr_chunks > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks %d bitmap 0x%x", 
                   __FUNCTION__, test_case_p->rg_object_id, (int)paged_info.num_nr_chunks, paged_info.needs_rebuild_bitmask);
        MUT_FAIL_MSG("There are chunks needing rebuild on SP A");
    }

    
    /* Re-insert the removed drive */ 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                sep_rebuild_utils_number_physical_objects_g,
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted removed drive %d_%d_%d", rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].bus,
                                                                            rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].enclosure,
                                                                            rg_config_p->rg_disk_set[test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE]].slot);
    fbe_test_sep_util_get_raid_group_disk_info(rg_config_p);
    
    return;
}
/*********************************************
 * end doodle_sp_restart_during_rebuild_test()
 *********************************************/

/*!***************************************************************************
 *          doodle_test_drive_returns_before_verify()
 *****************************************************************************
 *
 * @brief   This test re-inserts a removed drive while the SP is shutdown.
 *          It validates that the paged metadata verify runs without issue.
 *  
 * @param   rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 * @author
 *  04/03/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void doodle_test_drive_returns_before_verify(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;


    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_chunk_index_t                   total_chunks;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    /* Based on the test level determine which configuration to use.
    */

    /* this test need to run only on single SP version and with RTL 0 */
    if(fbe_test_sep_util_get_dualsp_test_mode() || (test_level > 0))
    {
        return;
    }

    /* get rg to the degraded mode */
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    
    status = fbe_api_scheduler_add_debug_hook(test_case_p->rg_object_id,
            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
            FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
            0x800,
            NULL,
            SCHEDULER_CHECK_VALS_LT,
            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Now reboot the and stop at teh paged metadata verify
     */
    doodle_reboot_sp(rg_config_p, 
                     SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                     FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Wait for the debug hook to be hit.
     */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                   FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Re-insert the removed drive
     */
    doodle_reinsert_drive(rg_config_p, 
                          test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE], 
                          &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);
    
    /* Set a debug hook when the paged verify completes.
     */
    doodle_add_debug_hook(test_case_p->rg_object_id, 
                          SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                          FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);

    /* Remove the paged verify start hook.
     */
    status = doodle_delete_debug_hook(test_case_p->rg_object_id,                   
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, 
                                      FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     
    /* Wait for the paged metadata verify to complete.
     */
    doodle_wait_for_target_SP_hook(test_case_p->rg_object_id,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                                   FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);

    /* Delete the debug hook 
     */
    doodle_delete_debug_hook(test_case_p->rg_object_id, 
                             SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
                             FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED);

    /* Validate that the raid group starts to rebuild.
     */
    status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "== reb chkpt 0x%x before reboot rg object id: 0x%x pos: %d  ==", 
                   (unsigned int)raid_group_info.rebuild_checkpoint[DOODLE_FIRST_POSITION_TO_REMOVE], test_case_p->rg_object_id, DOODLE_FIRST_POSITION_TO_REMOVE);

    /*  Verify that the RAID and LUN objects are in the READY state */
    sep_rebuild_utils_check_raid_and_lun_states(rg_config_p, FBE_LIFECYCLE_STATE_READY);

    /*  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, DOODLE_FIRST_POSITION_TO_REMOVE);

    total_chunks = (raid_group_info.capacity / raid_group_info.num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;

    status = fbe_api_raid_group_get_paged_bits(test_case_p->rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If the rebuild is done, no chunks should be needing a rebuild.
     */
    if (paged_info.num_nr_chunks > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks %d bitmap 0x%x", 
                   __FUNCTION__, test_case_p->rg_object_id, (int)paged_info.num_nr_chunks, paged_info.needs_rebuild_bitmask);
        MUT_FAIL_MSG("There are chunks needing rebuild on SP A");
    }

    return;
}
/***********************************************
 * end doodle_test_drive_returns_before_verify()
 ***********************************************/

/*!**************************************************************************
 * doodle_sp_shutdown_drive_remove_rl_mark_activate
 ***************************************************************************
 * @brief
 * This test verify that if drive gets removed after SP shutdown and before 
 * SP come up then that drive needs to mark for rebuild logging during raid group
 * activate state. 
 *  
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/

static void doodle_sp_shutdown_drive_remove_rl_mark_activate(fbe_test_rg_configuration_t * rg_config_p,doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;    
    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_u32_t                          count;
    fbe_u32_t                          max_tries;
    fbe_test_raid_group_disk_set_t *disk_set;
    fbe_sep_package_load_params_t sep_params_p;
    fbe_object_id_t         pdo_object_id;

    max_tries = 100;

    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

   /* get the disk set which we will be removing while SP is down */
    disk_set = &rg_config_p->rg_disk_set[DOODLE_FIRST_POSITION_TO_REMOVE];

    /* Get the PDO object id */
    status = fbe_api_get_physical_drive_object_id_by_location(disk_set->bus, 
                                                                disk_set->enclosure, 
                                                                disk_set->slot,
                                                                &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);

    /* shutdown the logical package - sep & neit*/
    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_test_sep_util_destroy_neit_sep_both_sps();
    }
    else
    {
        fbe_test_sep_util_destroy_neit_sep();
    }

    /* power down the drive with change the pdo object state */
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    doodle_remove_drive(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE], &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    sep_params_p.scheduler_hooks[0].object_id = test_case_p->rg_object_id;
    sep_params_p.scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE;
    sep_params_p.scheduler_hooks[0].monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START;
    sep_params_p.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params_p.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params_p.scheduler_hooks[0].val1 = NULL;
    sep_params_p.scheduler_hooks[0].val2 = NULL;

    /* and this will be our signal to stop */
    sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_sim_transport_set_target_server(DOODLE_TEST_ACTIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);

    fbe_api_database_set_load_balance(FBE_TRUE);

    status = fbe_api_wait_for_object_lifecycle_state(test_case_p->rg_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, 30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
         status = fbe_api_sim_transport_set_target_server(DOODLE_TEST_PASSIVE_SP);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        sep_config_load_sep_and_neit();

        fbe_api_database_set_load_balance(FBE_TRUE);

        status = fbe_api_sim_transport_set_target_server(DOODLE_TEST_ACTIVE_SP);
    }
    
    // Make sure missing drive is marked rebuild logging during activate.
    for(count = 0; count < max_tries; count++)
    {
        /*  Get the raid group information */
        status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(raid_group_info.b_rb_logging[DOODLE_FIRST_POSITION_TO_REMOVE] == FBE_TRUE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "RL marked for position %d",DOODLE_FIRST_POSITION_TO_REMOVE);
            break;
        }

        fbe_api_sleep(500);
    }

    /* delete the debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Insert the drives */ 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[DOODLE_FIRST_POSITION_TO_REMOVE],
                                                sep_rebuild_utils_number_physical_objects_g,
                                                &test_case_p->drive_info[DOODLE_FIRST_POSITION_TO_REMOVE]);

    /* wait for the rebuild to complete */
    fbe_test_sep_util_wait_rg_pos_rebuild(test_case_p->rg_object_id, DOODLE_FIRST_POSITION_TO_REMOVE, DOODLE_WAIT_MSEC);

    return;

}
/*******************************************************
 * end doodle_sp_shutdown_drive_remove_rl_mark_activate()
 ********************************************************/

/*!**************************************************************************
 * doodle_rg_broken
 ***************************************************************************
 * @brief
 *  This function does the following on a raid group:
 *  - Insert hook to avoid marking rebuild logging
 *  - Remove a single drive  
 *  - Removes the second drive to make the RG broken  
 *  - Make sure none of the drives are marked for rebuild logging
 *  
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void doodle_rg_broken(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;    
    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_u16_t                          index;
    fbe_u32_t                          drive_count; 
    fbe_object_id_t                    vd_object_id;

    memset(raid_group_info.b_rb_logging, FBE_FALSE, FBE_RAID_MAX_DISK_ARRAY_WIDTH*sizeof(fbe_bool_t));
    
    /* Add debug hook to prevent eval rl*/
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST);

    /* Remove required drives so that RAID goes broken    
    */
    for(drive_count = 0; drive_count < test_case_p->num_to_remove;drive_count++)
    {
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
        doodle_remove_drive(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], &test_case_p->drive_info[drive_count]);
    }

    /* Wait for VDs to fail*/
    for(drive_count = 0; drive_count < test_case_p->num_to_remove;drive_count++)
    {
        fbe_test_sep_util_get_virtual_drive_object_id_by_position(test_case_p->rg_object_id,
                                                                  drive_count,
                                                                  &vd_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL, 10000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* delete debug hook to let eval rl continue, and now rg will go broken */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST);

    status = fbe_api_wait_for_object_lifecycle_state(test_case_p->rg_object_id, FBE_LIFECYCLE_STATE_FAIL, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "RG: 0x%x is in FBE_LIFECYCLE_STATE_FAIL state", test_case_p->rg_object_id);

    /*  Get the raid group information */
    status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Make sure no drives are marked rebuild logging */
    for(index = 0; index < raid_group_info.width; index++)
    {
        MUT_ASSERT_TRUE(raid_group_info.b_rb_logging[index] == FBE_FALSE);
    }

    return;

}   // End doodle_rg_broken()

/*!**************************************************************************
 * doodle_ins_drives_mark_rl_in_activate
 ***************************************************************************
 * @brief
 *  This function does the following on a raid group:
 *  - Insert hook to avoid making the RG to go ready
 *  - Insert a single drive
 *  - Make sure that rebuild logging is marked on the dead drive
 *  - Make sure we are still in activate state
 *  - Delete the hook so that RG can go ready
 * 
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void doodle_ins_drives_mark_rl_in_activate(fbe_test_rg_configuration_t * rg_config_p, doodle_test_case_t *test_case_p)
{
    fbe_status_t                       status;    
    fbe_api_raid_group_get_info_t      raid_group_info;
    fbe_u32_t                          count;
    fbe_u32_t                          max_tries;
    fbe_lifecycle_state_t              current_state;
    fbe_u32_t                          drive_count =0;
    
    max_tries = 100;

    // Add debug hook
    doodle_add_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    for(drive_count = 0; drive_count < test_case_p->num_to_remove -1;drive_count++)
    {
        /* Insert the drives */ 
        sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
        sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], sep_rebuild_utils_number_physical_objects_g,
                                                  &test_case_p->drive_info[drive_count]);
    }

    /* Make sure missing drive is marked rebuild logging */
    for(count = 0; count < max_tries; count++)
    {
        /*  Get the raid group information */
        status = fbe_api_raid_group_get_info(test_case_p->rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(raid_group_info.b_rb_logging[test_case_p->drive_slots_to_remove[drive_count]] == FBE_TRUE)
        {
            break;
        }

        fbe_api_sleep(500);
    }

    /* Timedout waiting for rebuild logging */
    if(count>=max_tries)
    {
        MUT_ASSERT_TRUE(0);
    }

    /* Make sure we are still activate state when the rebuild logging was marked */
    fbe_api_get_object_lifecycle_state(test_case_p->rg_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_ACTIVATE, current_state);

    /* delete debug hook */
    doodle_delete_debug_hook(test_case_p->rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE, FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START);

    /* Make sure we have transitioned to ready state once the hook is deleted */
    status = fbe_api_wait_for_object_lifecycle_state(test_case_p->rg_object_id, FBE_LIFECYCLE_STATE_READY, 40000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Insert the drives */ 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, test_case_p->drive_slots_to_remove[drive_count], sep_rebuild_utils_number_physical_objects_g,
                                              &test_case_p->drive_info[drive_count]);

    /* wait for the rebuild to complete */
    fbe_test_sep_util_wait_rg_pos_rebuild(test_case_p->rg_object_id, test_case_p->drive_slots_to_remove[drive_count], DOODLE_WAIT_MSEC);

    return;

}   // End doodle_ins_drives_mark_rl_in_activate()



/*!*************************************************************************
 * doodle_mark_rl_in_raid_group_activate_test
 ***************************************************************************
 * @brief
 * This test verify that raid group marks rebuild logging in activate state for
 * the missing drive before becoming ready.
 *  
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void doodle_mark_rl_in_raid_group_activate_test(fbe_test_rg_configuration_t * rg_config_p,doodle_test_case_t *test_case_p)
{

    /*Temporary returning for R10 */
    if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        return;
    }

    /* Remove drives and make sure that rg goes broken
       without marking NR            
    */
    doodle_rg_broken(rg_config_p,test_case_p);

    /* Insert drives and make sure that we mark rebuild logging
       in activate state for the missing drives before 
       becoming ready            
    */
    doodle_ins_drives_mark_rl_in_activate(rg_config_p,test_case_p);

    return;
}
/********************************************************
 * end doodle_mark_rl_in_raid_group_activate_test()
 ********************************************************/

/*!*************************************************************************
 * doodle_get_data_drive_count_to_shutdown_rg()
 ***************************************************************************
 * @brief
 *  Helper function to get the no of data drives from raid group.
 *
 * @param rg_config_p         -Pointer to raid group configuration.
 *
 * @return data_drives.       - No of data drives in the raid.
 *
 * @author
 *  10/14/2011 - Created. Vishnu Sharma
 ***************************************************************************/
fbe_u32_t doodle_get_data_drive_count_to_shutdown_rg(fbe_test_rg_configuration_t *rg_config_p) 
 { 
    fbe_u32_t data_drives = 0; 
  
    switch(rg_config_p->raid_type) 
     { 
         case FBE_RAID_GROUP_TYPE_RAID0: 
         data_drives = 1;  
         break; 
          
         case FBE_RAID_GROUP_TYPE_RAID1: 
         data_drives = (rg_config_p->width/2)+1; 
         break; 
  
         case FBE_RAID_GROUP_TYPE_RAID3: 
         case FBE_RAID_GROUP_TYPE_RAID5: 
         case FBE_RAID_GROUP_TYPE_RAID10: 
         data_drives = 2; 
         break; 
 
         case FBE_RAID_GROUP_TYPE_RAID6: 
         data_drives = 3; 
         break; 
  
         case FBE_RAID_GROUP_TYPE_UNKNOWN: 
         default: 
         mut_printf(MUT_LOG_TEST_STATUS, "%s : RAID GROUP TYPE UNKNOWN.", __FUNCTION__);
         data_drives = 0; 
    } 
  
     return data_drives; 
}
/********************************************************
 * end doodle_get_data_drive_count_to_shutdown_rg()
 ********************************************************/
 
/*!**************************************************************
 * doodle_mark_rl_in_raid_group_activate_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the doodle test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void doodle_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
   fbe_u32_t                   index;
   fbe_u32_t                   test_index;
   fbe_test_rg_configuration_t *rg_config_p = NULL;
   fbe_u32_t                   num_raid_groups = 0;
   fbe_status_t                status;
   fbe_u32_t                   num_of_drives_to_remove =0;
   fbe_object_id_t             rg_object_id;


    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    mut_printf(MUT_LOG_LOW, "=== Doodle test start **** ===\n");

    /*  Get the number of physical objects in existence at this point in time.  This number is 
     *  used when checking if a drive has been removed or has been reinserted. 
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "doodle test started *****************\n");

    /* Perform test on all raid groups. */
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "doodle verify test raid type: %d of %d.\n", 
                        index+1, num_raid_groups);

            if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "raid type %d not enabled", rg_config_p->raid_type);
                continue;
            }

            /*  Get the RG's object id. */
            fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            num_of_drives_to_remove = doodle_get_data_drive_count_to_shutdown_rg(rg_config_p);

            if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                fbe_test_set_object_id_for_r10(&rg_object_id);
            }

            for(test_index = 0; test_index < DOODLE_TC_COUNT; test_index++) 
            {

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Doodle test case: %d of %d raid type: %d rg: %d of %d!!!", 
                           (test_index + 1), DOODLE_TC_COUNT, rg_config_p->raid_type, (index + 1), num_raid_groups);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                
                doodle_test_cases[test_index].num_to_remove = num_of_drives_to_remove;
                doodle_test_cases[test_index].rg_object_id = rg_object_id;
                
                switch(doodle_test_cases[test_index].test_case)
                {
                    case DOODLE_MARK_RL_IN_ACTIVE_STATE:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle mark RL in activate state test ============ \n");
                        doodle_mark_rl_in_raid_group_activate_test(rg_config_p,&doodle_test_cases[test_index]);
                        break;

                    case DOODLE_POWER_UP_TIMER_TEST:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle powerup timer test ============ \n");
                        doodle_powerup_timer_test(rg_config_p,&doodle_test_cases[test_index]);
                        break;

                    case DOODLE_SET_CONDITION_IN_ACTIVE_STATE:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle set conditions in raid group activate state test ================ \n");
                        doodle_rg_activate_set_conditions_test(rg_config_p,&doodle_test_cases[test_index]);
                        break;

                    case DOODLE_DRIVE_REMOVED_DURING_SHUTDOWN_MARK_RL:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle drive remove during shutdown, mark RL in activate test ============\n");
                        doodle_sp_shutdown_drive_remove_rl_mark_activate(rg_config_p,&doodle_test_cases[test_index]);
                        break;

                    case DOODLE_SP_RESTART_BEFORE_NR_MARK:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle sp restart befor mark NR test =========\n");
                        doodle_sp_restart_before_nr_mark_test(rg_config_p,&doodle_test_cases[test_index]);
                        break;
                    case DOODLE_SP_RESTART_DURING_REBUILD:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle sp restart during rebuild test =========\n");
                        doodle_sp_restart_during_rebuild_test(rg_config_p,&doodle_test_cases[test_index]);
                        break;
                    case DOODLE_DRIVE_RETURNS_BEFORE_VERIFY:
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle drive returns before verify =========\n");
                        doodle_test_drive_returns_before_verify(rg_config_p,&doodle_test_cases[test_index]);
                        break;
                    default:
                        status = FBE_STATUS_GENERIC_FAILURE;
                        mut_printf(MUT_LOG_TEST_STATUS, "doodle invalid test case: %d for index: %d",
                                   doodle_test_cases[test_index].test_case, test_index);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        return;
                }

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Doodle test case: %d of %d raid type: %d rg: %d of %d - complete !!!", 
                           (test_index + 1), DOODLE_TC_COUNT, rg_config_p->raid_type, (index + 1), num_raid_groups);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
            }
         }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "doodle test completed **********************\n");

    return;

}   
/*******************************************************
 * end doodle_run_tests()
 ********************************************************/


/*!****************************************************************************
 * doodle_test()
 ******************************************************************************
 * @brief
 *  Run raid group eval rebuild logging tests across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void doodle_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    /* Based on the test level determine which configuration to use.
    */
    if (test_level > 0)
    {
        /* Run test for normal element size.
        */
        rg_config_p = &doodle_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &doodle_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,doodle_run_tests,
                                   DOODLE_LUNS_PER_RAID_GROUP,
                                   DOODLE_CHUNKS_PER_LUN);
    return;    

}
/*******************************************************
 * end doodle_test()
 ********************************************************/

/*!**************************************************************
 * doodle_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the doodle test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void doodle_setup(void)
{        
    mut_printf(MUT_LOG_LOW, "=== doodle setup ===\n");  

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

        /* Based on the test level determine which configuration to use.
        */
        if (test_level > 0)
        {
            /* Run test for normal element size.
            */
            rg_config_p = &doodle_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &doodle_raid_group_config_qual[0];
        }
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        
        /* Load the physical package and create the physical configuration. 
         */
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

        fbe_api_database_set_load_balance(FBE_TRUE);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    return;

}   // End doodle_setup()

/*!****************************************************************************
 * doodle_cleanup()
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

void doodle_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/********************************************************
 * end doodle_cleanup()
 ********************************************************/

/*!**************************************************************
 * doodle_dualsp_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the doodle dual sp test.
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 10/04/2011 - Created. Sandeep Chaudhari.
 * 
 ****************************************************************/
void doodle_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    mut_printf(MUT_LOG_LOW, "=== Doodle Dual SP Test starts ===\n");

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Based on the test level determine which configuration to use.
    */
    if (test_level > 0)
    {
        /* Run test for normal element size.
        */
        rg_config_p = &doodle_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &doodle_raid_group_config_qual[0];
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,doodle_run_tests,
                                                   DOODLE_LUNS_PER_RAID_GROUP,
                                                   DOODLE_CHUNKS_PER_LUN);
    
    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Doodle Dual SP Test completed successfully ===", __FUNCTION__);
    return;
}
/********************************************************
 * end doodle_dualsp_test()
 ********************************************************/

/*!**************************************************************
 * doodle_dualsp_setup
 ****************************************************************
 * @brief
 *  This function inits the doodle test
 *
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 10/04/2011 - Created. Sandeep Chaudhari.
 * 
 ****************************************************************/
void doodle_dualsp_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

        /* Based on the test level determine which configuration to use.
        */
        if (test_level > 0)
        {
            /* Run test for normal element size.
            */
            rg_config_p = &doodle_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &doodle_raid_group_config_qual[0];
        }
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPA ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*  Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPB ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        
        sep_config_load_sep_and_neit_load_balance_both_sps(); 
	}

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;

}
/********************************************************
 * end doodle_dualsp_setup()
 ********************************************************/
 
/*!**************************************************************
 * doodle_dualsp_cleanup
 ****************************************************************
 * @brief
 *  This function destroys the doodle_test configuration
 *
 * @return none 
 *
 * @author
 * 10/04/2011 - Created. Sandeep Chaudhari.
 * 
 ****************************************************************/

void doodle_dualsp_cleanup()
{
    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    { 
        /*start with SPB so we leave the switch at SPA when we exit*/
        mut_printf(MUT_LOG_LOW, "=== doodle_test Unloading Config from SPB ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();
    
        /*back to A*/
        mut_printf(MUT_LOG_LOW, "=== doodle_test Unloading Config from SPA ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();   
    }

    return;
}
/********************************************************
 * end doodle_dualsp_cleanup()
 ********************************************************/




