/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file weerd_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for transition out of the hibernate due to
 *  proactive copy operation.
 *
 * @version
 *   12/09/2009 - Created. Dhaval Patel
 *   12/27/2012 - Modified. Vera Wang
 *   01/28/2013 - Updated to support multiple raid groups and dualsp.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_rebuild_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "neit_utils.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"


char * weerd_short_desc = "Transition out of the hibernate due to Proactive copy operation.";
char * weerd_long_desc =
    "\n"
    "\n"
    "The Weerd Scenario tests what happens when user issues proactive copy on hibernated VD object.\n"
    "\n"
    "It covers the following case:\n"
    "\t- User issues Proactive Copy on hibernated Virtual Drive object.\n"
    "\n"
    "Dependencies: \n"
    "\t- The Aurora scenario is a prerequisite to this scenario.\n"
    "\t- The Maggie scenario is a prerequisite to this scenario.\n"
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
    "\n"
    "STEP 2: Sends a hibernate request to LU object.\n"
    "\t- Verify that LU object changes its state to hibernate.\n"
    "\t- Verify that RAID-1 object changes its state to hibernate.\n"
    "\t- Verify that VD-A and VD-B object changes its state to hibernate.\n"
    "\t- Verify that PVD-A and PVD-B object changes its state to hibernate.\n"
    "\n"
    "STEP 3: Sends a Proactive copy request to VD-B object using FBE-API interface.\n"
    "\t- Verify that VD-B object first sends an unhibernate command to its downstream PVD-B object and changes its current state to ACTIVATE.\n"
    "\t- Verift that downstream edge between VD-B object and PVD-B object changes its state to ENABLED when PVD-B object come out of hibernate state.\n"
    "\t- Verify that VD-B changes its state to READY and come out from hibernate state.\n"
    "\t- Verify that RAID-1 object finds edge state change to ENABLED with VD-B object when VD-B object comes out of the hibernate state.\n"
    "\t- Verify that RAID-1 object wakes up and sets an check for hibernate condition.\n" 
    "\t- Verify that VD-B object continues to do proactive copy operation.\n"
    "\t- Verify that once VD-B completes proactive copy operation it also sets an check for hibernate condition.\n"
    "\t- Verify that VD-B changes its state to hibernate after configured idle time.\n"
    "\t- Verify that RAID-1 object changes its state to hibernate once VD-B changes its state back to hibernate and its idle time is greater than the configured idle time.\n"
    ;

/*!*******************************************************************
 * @def WEERD_EXTENDED_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types.
 *
 *********************************************************************/
#define WEERD_EXTENDED_TEST_CONFIGURATION_TYPES 1

/*!*******************************************************************
 * @def WEERD_QUAL_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types
 *
 *********************************************************************/
#define WEERD_QUAL_TEST_CONFIGURATION_TYPES     1

/* The number of LUNs in our raid group.
 */
#define WEERD_LUNS_PER_RAID_GROUP               1    /*! @note This MUST BE 1 */

/* Element size of our LUNs.
 */
#define WEERD_TEST_ELEMENT_SIZE                 128

#define WEERD_CHUNKS_PER_LUN                    3

/*!*******************************************************************
 * @def     WEERD_IDLE_TIME_BEFORE_HIBERNATE_SECS
 *********************************************************************
 * @brief   How long to wait (in seconds) before the object to start to
 *          enter hibernation.
 *
 *********************************************************************/
#define WEERD_IDLE_TIME_BEFORE_HIBERNATE_SECS   10
#define WEERD_LUN_IDLE_TIME_BEFORE_HIBERNATE_SECS   WEERD_IDLE_TIME_BEFORE_HIBERNATE_SECS

/*!*******************************************************************
 * @def     WEERD_WAIT_FOR_HIBERNATION_TMO_SECS
 *********************************************************************
 * @brief   How long to wait (in seconds) for any object (in either the
 *          logical or physical package) will enter hibernation.
 *          Currently there is a hard coded delay of 2 minutes (120
 *          seconds) after the idle time has been reached.
 *
 * @note    I.E. LU will hibernate in 120 seconds...
 *
 *********************************************************************/
#define WEERD_WAIT_FOR_HIBERNATION_TMO_SECS     (120 + (WEERD_IDLE_TIME_BEFORE_HIBERNATE_SECS + 60))

/*!*******************************************************************
 * @def     WEERD_WAIT_FOR_HIBERNATION_TMO_MS
 *********************************************************************
 * @brief   How long to wait (in milliseconds) for any object (in either the
 *          logical or physical package) will enter hibernation.
 *          Currently there is a hard coded delay of 2 minutes (120
 *          seconds) after the idle time has been reached.
 *
 * @note    I.E. LU will hibernate in 120 seconds...
 *
 *********************************************************************/
#define WEERD_WAIT_FOR_HIBERNATION_TMO_MS       (WEERD_WAIT_FOR_HIBERNATION_TMO_SECS * 1000)

/*!*******************************************************************
 * @def WEERD_WAIT_OBJECT_TIMEOUT_MS
 *********************************************************************
 * @brief Number of milliseconds we wait for an object.
 *        We make this relatively large since if it takes this long
 *        then we know something is very wrong.
 *
 *********************************************************************/
#define WEERD_WAIT_OBJECT_TIMEOUT_MS            60000

/*!*******************************************************************
 * @var weerd_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t weerd_raid_groups_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t weerd_raid_groups_extended[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* All raid group configurations for qual
     */
    {
        /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
        {2,         0x20000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR, 520,        0,          0},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        1,          0},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        2,          1},
        {6,         0x20000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
        {4,         0x20000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520,        4,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var weerd_raid_groups_qual
 *********************************************************************
 * @brief Test config for raid test level 0 (default test level).
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t weerd_raid_groups_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t weerd_raid_groups_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* All raid group configurations for qual
     */
    {
        /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
        {2,         0x20000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR, 520,        0,          0},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        1,          0},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        2,          1},
        {6,         0x20000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
        {4,         0x20000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520,        4,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var weerd_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_lba_t weerd_user_space_lba_to_inject_to          =   0x0ULL; /* lba 0 on first LUN*/

/*!*******************************************************************
 * @var weerd_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_block_count_t weerd_user_space_blocks_to_inject  = 0x7FFULL; /* Size of (1) chunk minus 1 */

/*!*******************************************************************
 * @var weerd_default_offset
 *********************************************************************
 * @brief This is the default user space offset.
 *
 *********************************************************************/
static fbe_lba_t weerd_default_offset                     = 0x10000ULL; /* Offset used maybe different */

/*!***************************************************************************
 * @var weerd_b_is_DE542_fixed
 *****************************************************************************
 * @brief DE542 - Due to the fact that protocol error are injected on the
 *                way to the disk, the data is never sent to or read from
 *                the disk.  Therefore until this defect is fixed b_start_io
 *                MUST be True.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_bool_t weerd_b_is_DE542_fixed                  = FBE_FALSE;

fbe_api_rdgen_context_t                     weerd_io_context[WEERD_LUNS_PER_RAID_GROUP * 2];

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t weerd_test_proactive_copy_on_hibernating_vd(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy);

static fbe_status_t weerd_test_user_copy_on_hibernating_vd(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count,
                                                           fbe_u32_t position_to_copy);

extern void diabolicdiscdemon_verify_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p,
                                                        fbe_u32_t in_element_size);

/* it gets source and destination edge index for the virtual drive object. */
extern void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                                fbe_edge_index_t * source_edge_index_p,
                                                                fbe_edge_index_t * dest_edge_index_p);

/* Wait for the proactive spare to swap-in. */
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);
/* It waits for the proactive copy to complete. */
extern void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);

/* wait for the source edge candidate to swap-out. */
extern void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                             fbe_edge_index_t source_edge_index);

static void weerd_test_enable_system_power_saving(void);

static void weerd_test_verify_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p);
static void weerd_test_verify_power_saving_policy_current_sp(fbe_test_rg_configuration_t *rg_config_p);
static void weerd_test_enable_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count);
static void weerd_test_enable_power_saving_policy_current_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count);


static void weerd_test_bring_bound_objects_to_hibernating_state(fbe_test_rg_configuration_t * rg_config_p, fbe_u32_t raid_group_count);

void weerd_load_physical_config(void);
static void weerd_test_disable_system_power_saving(void);


/*!***************************************************************************
 *          weerd_validate_object_is_power_saving()
 *****************************************************************************
 *
 * @brief   There can be (especially in the dualsp environment) a small delay
 *          from the time an object enters the hibernate rotary until the 
 *          power savings state is set to `power save'.  This method will wait
 *          a short period for the power save state before reporting a failure.
 *
 * @param   object_id   - The object identifier for the object that we expect
 *              to be power saving  
 *
 * @return  status
 *
 * @author
 *  02/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t weerd_validate_object_is_power_saving(fbe_object_id_t object_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           check_interval = (3 * 1000);
    fbe_u32_t                           timeout_ms = (6 * 1000);
    fbe_base_config_object_power_save_info_t object_ps_info; 
    fbe_base_config_object_power_save_info_t peer_ps_info; 
    fbe_u32_t                           total_wait_time = 0;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_bool_t                          b_is_dualsp_mode;

    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* First get the sp id this was invoked on and out peer.
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    
    /* Get the initial values*/
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_get_object_power_save_info(object_id, &peer_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        /* Else simply mark the peer as done
         */
        peer_ps_info = object_ps_info;
        peer_ps_info.power_saving_enabled = FBE_TRUE;
        peer_ps_info.power_save_state = FBE_POWER_SAVE_STATE_SAVING_POWER;
    }

    /* Wait up to the time out period.
     */
    while (((object_ps_info.power_save_state != FBE_POWER_SAVE_STATE_SAVING_POWER) ||
            (peer_ps_info.power_save_state   != FBE_POWER_SAVE_STATE_SAVING_POWER)    ) &&
           (total_wait_time < timeout_ms)                                                  )
    {
        /* We wait for the local before waiting for the peer.
         */
        status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if ((object_ps_info.power_saving_enabled == FBE_TRUE)                      && 
            (object_ps_info.power_save_state == FBE_POWER_SAVE_STATE_SAVING_POWER)    )
        {
            /* The local is complete.  Check the peer if needed.
             */
            if (b_is_dualsp_mode == FBE_TRUE)
            {
                status = fbe_api_transport_set_target_server(other_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if ((peer_ps_info.power_saving_enabled == FBE_TRUE)                      && 
                    (peer_ps_info.power_save_state == FBE_POWER_SAVE_STATE_SAVING_POWER)    )
                {
                    /* We are done.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS,  
                               "%s obj: 0x%x both local and peer power saving",
                               __FUNCTION__, object_id);
                    return FBE_STATUS_OK;
                }
            }
            else
            {
                /* Else we are done.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,  
                           "%s obj: 0x%x both SP power saving",
                           __FUNCTION__, object_id);
                return FBE_STATUS_OK;
            }
        }

        /* Else we need to wait.
         */
        total_wait_time += check_interval;
        fbe_api_sleep(check_interval);

    } /* end while not power savings or timeout */

    /* Check for timeout.
     */
    if (total_wait_time >= timeout_ms)
    {
        /* Fail.
         */
        status = FBE_STATUS_TIMEOUT;
        if (b_is_dualsp_mode == FBE_TRUE)
        {
            mut_printf(MUT_LOG_TEST_STATUS,  
                       "%s obj: 0x%x either local: %d or peer: %d not power saving",
                       __FUNCTION__, object_id,
                       object_ps_info.power_save_state,
                       peer_ps_info.power_save_state);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        mut_printf(MUT_LOG_TEST_STATUS,  
                   "%s obj: 0x%x either local: %d not power saving",
                   __FUNCTION__, object_id,
                   object_ps_info.power_save_state);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Else return success.
     */
    return FBE_STATUS_OK;
}
/********************************************** 
 * end weerd_validate_object_is_power_saving()
 **********************************************/

/*!***************************************************************************
 *          weerd_validate_object_power_saving_disabled()
 *****************************************************************************
 *
 * @brief   There can be (especially in the dualsp environment) a small delay
 *          from the time an object exits the hibernate rotary until the 
 *          power savings state is set to `power save'.  This method will wait
 *          a short period for the power save state before reporting a failure.
 *
 * @param   object_id   - The object identifier for the object that we expect
 *              to be power saving  
 *
 * @return  status
 *
 * @author
 *  02/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t weerd_validate_object_power_saving_disabled(fbe_object_id_t object_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           check_interval = (3 * 1000);
    fbe_u32_t                           timeout_ms = (6 * 1000);
    fbe_base_config_object_power_save_info_t object_ps_info; 
    fbe_base_config_object_power_save_info_t peer_ps_info; 
    fbe_u32_t                           total_wait_time = 0;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_bool_t                          b_is_dualsp_mode;

    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* First get the sp id this was invoked on and out peer.
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    
    /* Get the initial values*/
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_get_object_power_save_info(object_id, &peer_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        /* Else simply mark the peer as done
         */
        peer_ps_info = object_ps_info;
        peer_ps_info.power_saving_enabled = FBE_FALSE;
        peer_ps_info.power_save_state = FBE_POWER_SAVE_STATE_NOT_SAVING_POWER;
    }

    /* Wait up to the time out period.
     */
    while (((object_ps_info.power_save_state != FBE_POWER_SAVE_STATE_NOT_SAVING_POWER) ||
            (peer_ps_info.power_save_state   != FBE_POWER_SAVE_STATE_NOT_SAVING_POWER)    ) &&
           (total_wait_time < timeout_ms)                                                      )
    {
        /* We wait for the local before waiting for the peer.
         */
        status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if ((object_ps_info.power_saving_enabled == FBE_FALSE)                         && 
            (object_ps_info.power_save_state == FBE_POWER_SAVE_STATE_NOT_SAVING_POWER)    )
        {
            /* The local is complete.  Check the peer if needed.
             */
            if (b_is_dualsp_mode == FBE_TRUE)
            {
                status = fbe_api_transport_set_target_server(other_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if ((peer_ps_info.power_saving_enabled == FBE_FALSE)                         && 
                    (peer_ps_info.power_save_state == FBE_POWER_SAVE_STATE_NOT_SAVING_POWER)    )
                {
                    /* We are done.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS,  
                               "%s obj: 0x%x both local and peer not power saving",
                               __FUNCTION__, object_id);
                    return FBE_STATUS_OK;
                }
            }
            else
            {
                /* Else we are done.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,  
                           "%s obj: 0x%x both SP not power saving",
                           __FUNCTION__, object_id);
                return FBE_STATUS_OK;
            }
        }

        /* Else we need to wait.
         */
        total_wait_time += check_interval;
        fbe_api_sleep(check_interval);

    } /* end while not power savings or timeout */

    /* Check for timeout.
     */
    if (total_wait_time >= timeout_ms)
    {
        /* Fail.
         */
        status = FBE_STATUS_TIMEOUT;
        if (b_is_dualsp_mode == FBE_TRUE)
        {
            mut_printf(MUT_LOG_TEST_STATUS,  
                       "%s obj: 0x%x either local: %d or peer: %d power saving",
                       __FUNCTION__, object_id,
                       object_ps_info.power_save_state,
                       peer_ps_info.power_save_state);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        mut_printf(MUT_LOG_TEST_STATUS,  
                   "%s obj: 0x%x either local: %d power saving",
                   __FUNCTION__, object_id,
                   object_ps_info.power_save_state);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Else return success.
     */
    return FBE_STATUS_OK;
}
/***************************************************
 * end weerd_validate_object_power_saving_disabled()
 ***************************************************/

/* Populate the io error record */
static fbe_status_t weerd_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t drive_pos,
                                                             fbe_u8_t requested_scsi_command_to_trigger,
                                                             fbe_protocol_error_injection_error_record_t *record_p,
                                                             fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    scsi_command_to_trigger = requested_scsi_command_to_trigger;
    fbe_object_id_t             drive_object_id = 0;
    fbe_object_id_t             pvd_object_id = 0;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                              rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                              rg_config_p->rg_disk_set[drive_pos].slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Calculate the physical lba to inject to.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    weerd_default_offset = provision_drive_info.default_offset;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (weerd_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_command_to_trigger != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_command_to_trigger, FBE_SCSI_VERIFY_16);
            scsi_command_to_trigger = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_command_to_trigger == FBE_SCSI_VERIFY_16);
    }

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = fbe_test_neit_utils_populate_protocol_error_injection_record(rg_config_p->rg_disk_set[drive_pos].bus,
                                                                          rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                                          rg_config_p->rg_disk_set[drive_pos].slot,
                                                                          record_p,
                                                                          record_handle_p,
                                                                          weerd_user_space_lba_to_inject_to,
                                                                          weerd_user_space_blocks_to_inject,
                                                                          FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                          scsi_command_to_trigger,
                                                                          FBE_SCSI_SENSE_KEY_RECOVERED_ERROR,
                                                                          FBE_SCSI_ASC_PFA_THRESHOLD_REACHED,
                                                                          FBE_SCSI_ASCQ_GENERAL_QUALIFIER,
                                                                          FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                          1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/************************************************
 * end weerd_populate_io_error_injection_record()
 ************************************************/

/*!***************************************************************************
 *          weerd_run_io_to_start_copy()
 *****************************************************************************
 *
 * @brief   If the test isn't running I/O but they want to trigger a proactive
 *          copy, this method is invoked to run the specified I/O type which
 *          will trigger the copy.
 *
 * @param   rg_config_p - Pointer to raid group array to locate pvds
 * @param   sparing_position - Position that is being copied
 * @param   source_edge_index - Source vd edge index to send I/O to
 * @param   requested_scsi_opcode_to_inject_on - SCSI operation that is being injected to
 * @param   io_size - Fixed number of blocks per I/O
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  01/09/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t weerd_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_u32_t sparing_position,
                                               fbe_edge_index_t source_edge_index,
                                               fbe_u8_t requested_scsi_opcode_to_inject_on,
                                               fbe_block_count_t io_size)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *rdgen_context_p = &weerd_io_context[0];
    fbe_u8_t                        scsi_opcode_to_inject_on = requested_scsi_opcode_to_inject_on;
    fbe_rdgen_operation_t           rdgen_operation = FBE_RDGEN_OPERATION_INVALID;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL; 
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_class_id_t                  class_id;
    fbe_lba_t                       user_space_start_lba = FBE_LBA_INVALID;
    fbe_block_count_t               blocks = 0;
    fbe_lba_t                       start_lba = FBE_LBA_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;

    /* We must inject to the same pvd block that the error was injected to.
     */
    user_space_start_lba = weerd_user_space_lba_to_inject_to;
    blocks = io_size;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (weerd_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_opcode_to_inject_on != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_opcode_to_inject_on, FBE_SCSI_VERIFY_16);
            scsi_opcode_to_inject_on = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_opcode_to_inject_on == FBE_SCSI_VERIFY_16);
    }

    /* Determine the rdgen operation based on the SCSI opcode
     */
    switch (scsi_opcode_to_inject_on)
    {
        case FBE_SCSI_READ_6:
        case FBE_SCSI_READ_10:
        case FBE_SCSI_READ_12:
        case FBE_SCSI_READ_16:
            /* Generate read only requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
            break;
        case FBE_SCSI_VERIFY_10:
        case FBE_SCSI_VERIFY_16:
            /* Generate verify only */
            rdgen_operation = FBE_RDGEN_OPERATION_VERIFY;
            break;
        case FBE_SCSI_WRITE_6:
        case FBE_SCSI_WRITE_10:
        case FBE_SCSI_WRITE_12:
        case FBE_SCSI_WRITE_16:
            /* Generate write only */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
            break;
        case FBE_SCSI_WRITE_SAME_10:
        case FBE_SCSI_WRITE_SAME_16:
            /* Generate zero requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_ZERO_ONLY;
            break;
        case FBE_SCSI_WRITE_VERIFY_10:
        case FBE_SCSI_WRITE_VERIFY_16:
            /* Generate write verify requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_VERIFY;
            break;
        default:
            /* Unsupported opcode */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported SCSI opcode: 0x%x",
                       __FUNCTION__, scsi_opcode_to_inject_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }


    /* Get the vd object id of the position to force into proactive copy
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    drive_to_spare_p = &rg_config_p->rg_disk_set[sparing_position];
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Get the pvd to issue the I/O to
     */
    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));

    /* Verify the class-id of the object to be provision drive
     */
    pvd_object_id = edge_info.server_id;
    status = fbe_api_get_object_class_id(pvd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);

    /*! @note PVD Sniff could automatically trigger copy.
     */
    if ((status != FBE_STATUS_OK)                  || 
        (class_id != FBE_CLASS_ID_PROVISION_DRIVE)    )  
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + weerd_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_PROVISION_DRIVE));
    }

    /* Get default offset to locate lba to issue request to.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    /*! @note PVD Sniff could automatically trigger copy.
     */
    if (status != FBE_STATUS_OK)
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + weerd_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    default_offset = provision_drive_info.default_offset;
    start_lba = user_space_start_lba + default_offset;

    /* Send a single I/O to each PVD.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Issue rdgen op: %d to pvd: 0x%x lba: 0x%llx blks: 0x%llx ", 
               __FUNCTION__, rdgen_operation, pvd_object_id, start_lba, blocks);
    status = fbe_api_rdgen_send_one_io(rdgen_context_p, 
                                       pvd_object_id,       /* Issue to a single pvd */
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       rdgen_operation,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       start_lba, /* lba */
                                       blocks, /* blocks */
                                       FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    /*! @note PVD Sniff could automatically trigger copy.
     */
    if (status != FBE_STATUS_OK)
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + weerd_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************
 * end weerd_run_io_to_start_copy()
 ********************************************************/

/* Trigger the proactive copy operation using I/O errors. */
static void weerd_io_error_trigger_proactive_sparing(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t        drive_pos,
                                                     fbe_edge_index_t source_edge_index,
                                                     fbe_bool_t       b_validate_eol)
{
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;
    fbe_object_id_t                                 rg_object_id;
    fbe_object_id_t                                 vd_object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_get_block_edge_info_t                   block_edge_info;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: inject scsi errors on bus:0x%x, encl:0x%x, slot:0x%x to trigger spare swap-in.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = weerd_populate_io_error_injection_record(rg_config_p,
                                                      drive_pos,
                                                      FBE_SCSI_WRITE_16,
                                                      &record,
                                                      &record_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* inject the scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Start I/O to force EOL.*/
    status = weerd_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                        FBE_SCSI_WRITE_16,
                                        weerd_user_space_blocks_to_inject);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the eol bit gets set for the pdo and it gets propogated to virtual drive object. */
    if (b_validate_eol == FBE_TRUE)
    {
        status = fbe_api_wait_for_block_edge_path_attribute(vd_object_id, source_edge_index, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE, FBE_PACKAGE_ID_SEP_0, 60000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s:EOL bit is set on the edge between vd (0x%x) and pvd (0x%x) object.",
                   __FUNCTION__, vd_object_id, block_edge_info.server_id);
    }

    /* stop injecting scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* restart the job service queue to proceed with job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);
    return;
}
/*************************************************
 * end weerd_io_error_trigger_proactive_sparing()
 *************************************************/


/*!***************************************************************************
 *          weerd_test_proactive_copy_on_hibernating_vd()
 *****************************************************************************
 *
 * @brief   Run Weerd proactive copy on hibernating vd test for the 
 *          specific RAID group.
 *
 *  The test performs the following tasks:
 *      -Enables the system power-saving mode.
 *      -Sets the power saving policies as per requirement.
 *      -Brings the bound drives into hibernating mode.
 *      -Triggers proactive copy on one of the bound disks (by error injection).
 *      -Waits for the proactive spare to swap in. 
 *      -Verifies that proactive copy completes and waits for the swap-out
 *       of the drive. 
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The raid group position to execute proactive
 *              copy on.
 *
 * @return  status
 *
 * @author
 *  01/09/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t weerd_test_proactive_copy_on_hibernating_vd(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t        *context_p = &weerd_io_context[0];
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_object_id_t                 source_object_id;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_object_id_t                 pdo_object_id;
    fbe_object_id_t                 dest_object_id;
    fbe_u32_t                       rg_index;
    fbe_bool_t                      b_is_dualsp_mode;

    /* Step 1:  Enable System power saving mode.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable system power savings ==", 
               __FUNCTION__);
    weerd_test_enable_system_power_saving();

    /* Step 2:  Set the power saving policies as per our requirement.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        weerd_test_verify_power_saving_policy(current_rg_config_p);
        /* Goto next raid group*/
        current_rg_config_p++;
    }

    /* Step 3:  Bring the bound objects to hibernating state.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Bring: %d raid groups to hibernate ==", 
               __FUNCTION__, raid_group_count);
    weerd_test_bring_bound_objects_to_hibernating_state(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Bring: raid groups to hibernate - complete ==", 
               __FUNCTION__);

    /* Step 4:  Validate that proactive copy bring objects out of hibernate
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Copy operations are not support on RAID-0
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* Goto next */
            current_rg_config_p++;
            continue;
        }

        /* Get the source drive information.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position_to_copy].bus,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].slot,
                                                                &source_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[position_to_copy].bus,
                                                                  current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                                  current_rg_config_p->rg_disk_set[position_to_copy].slot,
                                                                  &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* get the edge index of the source and destination drive based on configuration mode. 
         */
        diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Set a debug hook after we switch to mirror mode but before we start the 
         * metadata rebuild.
         */
        status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Inject an error on the associated PDO to set EOL.
         */
        weerd_io_error_trigger_proactive_sparing(current_rg_config_p, position_to_copy, source_edge_index,
                                                 FBE_FALSE  /* Don't validate EOL */);

        /* Wait for pdo and pvd to be ready then inject it again
         */
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify PDO and PVD is out saving power, pdo_obj_id:0x%x\n", __FUNCTION__, pdo_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              pdo_object_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              source_object_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait until the parent raid group is ready, otherwise it could be in 
         * ACTIVATE which will cause the proactive copy request to `temporarily'
         * fail.
         */

        /* Inject an error on the associated PDO to set EOL (again).
         */
        weerd_io_error_trigger_proactive_sparing(current_rg_config_p, position_to_copy, source_edge_index,
                                                 FBE_TRUE   /* Validate EOL */);

        /* wait for the proactive spare to swap-in. */
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &dest_drive_location);
        status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                                dest_drive_location.enclosure,
                                                                dest_drive_location.slot,
                                                                &dest_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Generate a test trace */
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s from: %d_%d_%d(0x%x) to: %d_%d_%d(0x%x) start ===",
               __FUNCTION__, current_rg_config_p->rg_disk_set[position_to_copy].bus,
               current_rg_config_p->rg_disk_set[position_to_copy].enclosure, current_rg_config_p->rg_disk_set[position_to_copy].slot,
               source_object_id, 
               dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot,
               dest_object_id);

        /* Wait until we start the metadata rebuild.
         */
        status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that NR is set on the destination drive*/
        fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

        /* Remove the rebuild start hook. */
        status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

        /* wait for the swap-out of the drive after proactive copy completion. */
        diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

        /* Refresh the raid group disk set information. */
        fbe_test_sep_util_raid_group_refresh_disk_info(current_rg_config_p, 1);

        /* Clear EOL for the orignal source disk
         */
        status = fbe_api_provision_drive_get_location(source_object_id,
                                                      &source_drive_location.bus,
                                                      &source_drive_location.enclosure,
                                                      &source_drive_location.slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_clear_disk_eol(source_drive_location.bus,
                                                  source_drive_location.enclosure,
                                                  source_drive_location.slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group*/
        current_rg_config_p++;

    } /* end initiate proactive copy on all raid groups under test*/

    /* Step 5:  Validate that the user data was correctly copied.
     */
    diabolicdiscdemon_verify_background_pattern(context_p, WEERD_TEST_ELEMENT_SIZE);
    
    /* Step 6:  Validate that all virutal drives go back to hibernation
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Copy operations are not support on RAID-0
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* Goto next */
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now let's wait and make sure the original power save capable drive 
         * is hibernating again after proactive completes. 
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              vd_object_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group*/
        current_rg_config_p++;

    } /* end validate that copy virtual drives go back to hibernation*/

    /* Step 7: Refresh the `extra disk' info for all raid groups.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s completed successfully. ****\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/***************************************************
 * end weerd_test_proactive_copy_on_hibernating_vd()
 ***************************************************/

/*!***************************************************************************
 *          weerd_test_user_copy_on_hibernating_vd()
 *****************************************************************************
 *
 * @brief   Run Weerd user copy on hibernating vd test for the specific RAID 
 *          group.
 *
 *  The test performs the following tasks:
 *      -Enables the system power-saving mode.
 *      -Sets the power saving policies as per requirement.
 *      -Brings the bound drives into hibernating mode.
 *      -Generates user copy on one of the bound disks.
 *      -Waits for the spare to swap in. 
 *      -Verifies that user copy completes and waits for the swap-out
 *       of the drive. 
 *       
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The raid group position to execute user
 *              copy on.
 *
 * @return  status
 *
 * @author
 *  5/18/2010 - Created. Dhaval Patel
 *  1/02/2013 - Modified. Vera Wang
 *  1/28/2012 - Updated. Ron Proulx
 ****************************************************************/
static fbe_status_t weerd_test_user_copy_on_hibernating_vd(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t raid_group_count,
                                                           fbe_u32_t position_to_copy)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_object_id_t                         rg_obj_id, src_obj_id, dest_obj_id, vd_obj_id;
    fbe_object_id_t                         pvd_obj_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_api_rdgen_context_t                *context_p = &weerd_io_context[0];
    fbe_u32_t                               rg_index;
    fbe_transport_connection_target_t       this_sp;
    fbe_transport_connection_target_t       other_sp;
    fbe_bool_t                              b_is_dualsp_mode;

    /* Step 1:  Enable System power saving mode.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable system power savings ==", 
               __FUNCTION__);
    weerd_test_enable_system_power_saving();

    /* Step 2:  Set the power saving policies as per our requirement.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        weerd_test_verify_power_saving_policy(current_rg_config_p);
        /* Goto next raid group*/
        current_rg_config_p++;
    }

    /* Step 3:  Bring the bound objects to hibernating state.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Bring: %d raid groups to hibernate ==", 
               __FUNCTION__, raid_group_count);
    weerd_test_bring_bound_objects_to_hibernating_state(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Bring: raid groups to hibernate - complete ==", 
               __FUNCTION__);

    /* Step 4:  Copy from a a `power savings' capable drive to a non-power savings
     *          capable drive.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Copy operations are not support on RAID-0
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* Goto next */
            current_rg_config_p++;
            continue;
        }

        /* Get the source and destination drive information.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get the virutal drive object id.
         */
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_obj_id, position_to_copy, &vd_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* The source drive is the position specified.  Validate that the source
         * drive is in power savings.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position_to_copy].bus,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].slot,
                                                                &src_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              src_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              vd_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* The destination drive is the first `extra' disk.  Disable power savings
         * for this drive and wait until it is in the Ready state.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->extra_disk_set[0].bus,
                                                                current_rg_config_p->extra_disk_set[0].enclosure,
                                                                current_rg_config_p->extra_disk_set[0].slot,
                                                                &dest_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* we need to make sure dest goes to hibernate first,otherwise, disabling power save will not cause path state change to vd. */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              src_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable power saving on pvd obj: 0x%x (%d_%d_%d) ==", 
                   __FUNCTION__, dest_obj_id,
                   current_rg_config_p->extra_disk_set[0].bus,
                   current_rg_config_p->extra_disk_set[0].enclosure,
                   current_rg_config_p->extra_disk_set[0].slot);
        status = fbe_api_disable_object_power_save(dest_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_dualsp_mode == FBE_TRUE)
        {
            this_sp = fbe_api_transport_get_target_server();
            other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
            status = fbe_api_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_disable_object_power_save(dest_obj_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              dest_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start a `User Copy' request. 
         *  o Source drive is power savings capable
         *  o Destination drive is NOT power saves capable.
         */
        mut_printf(MUT_LOG_LOW, "=== rg obj: 0x%x User copy from power saving: %d_%d_%d to non-power saving: %d_%d_%d drive===",
                   rg_obj_id,
                   current_rg_config_p->rg_disk_set[position_to_copy].bus,
                   current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                   current_rg_config_p->rg_disk_set[position_to_copy].slot,
                   current_rg_config_p->extra_disk_set[0].bus,
                   current_rg_config_p->extra_disk_set[0].enclosure,
                   current_rg_config_p->extra_disk_set[0].slot);

        /* get the edge index of the source and destination drive based on configuration mode. 
        */
        diabolicdiscdemon_get_source_destination_edge_index(vd_obj_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Set a debug hook after we switch to mirror mode but before we start the 
         * metadata rebuild.
        */
        status = fbe_api_scheduler_add_debug_hook(vd_obj_id,
                                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                  FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                                  0, 0,              
                                                  SCHEDULER_CHECK_STATES,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now start the copy operation.
         */
        status = fbe_api_copy_to_replacement_disk(src_obj_id, dest_obj_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR);

        /* Wait until we start the metadata rebuild.
         */
        status = fbe_test_wait_for_debug_hook(vd_obj_id, 
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that NR is set on the destination drive*/
        fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_obj_id, dest_edge_index);

        /* Remove the rebuild start hook. */
        status = fbe_api_scheduler_del_debug_hook(vd_obj_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_obj_id);

        /* wait for the swap-out of the drive after proactive copy completion. */
        diabolicdiscdemon_wait_for_source_drive_swap_out(vd_obj_id, source_edge_index);

        /* Before we refresh the raid group information, overwrite the 
         * `extra[0]' with the source drive location.  This will be the 
         * destination drive for the next step.
         */
        current_rg_config_p->extra_disk_set[0].bus = current_rg_config_p->rg_disk_set[position_to_copy].bus;
        current_rg_config_p->extra_disk_set[0].enclosure = current_rg_config_p->rg_disk_set[position_to_copy].enclosure;
        current_rg_config_p->extra_disk_set[0].slot = current_rg_config_p->rg_disk_set[position_to_copy].slot;

        /* Refresh the raid group disk set information. */
        fbe_test_sep_util_raid_group_refresh_disk_info(current_rg_config_p, 1);

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups copy from power savings drive to non-power savings drive */

    /* Step 6:  Validate that the user data was correctly copied.
     */
    diabolicdiscdemon_verify_background_pattern(context_p, WEERD_TEST_ELEMENT_SIZE);

    /* Step 7:  Copy from a a `non-power savings' capable drive to a `power savings'
     *          capable drive.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Copy operations are not support on RAID-0
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* Goto next */
            current_rg_config_p++;
            continue;
        }

        /* Get the source and destination drive information.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get the virtual drive object id.
         */
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_obj_id, position_to_copy, &vd_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note Even though we disable power savings on the destination drive
         *        in Step 4 above, it was automatically re-enabled when it
         *        became part of a power savings raid group.  Now manually 
         *        disable it so that we test copy from a non-pwer savings drive
         *        to a power savings drive.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position_to_copy].bus,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                                current_rg_config_p->rg_disk_set[position_to_copy].slot,
                                                                &src_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_enable_object_power_save(src_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_dualsp_mode == FBE_TRUE)
        {
            this_sp = fbe_api_transport_get_target_server();
            other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
            status = fbe_api_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_enable_object_power_save(src_obj_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              src_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              vd_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable power saving on pvd obj: 0x%x (%d_%d_%d) ==", 
                   __FUNCTION__, src_obj_id,
                   current_rg_config_p->rg_disk_set[position_to_copy].bus,
                   current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                   current_rg_config_p->rg_disk_set[position_to_copy].slot);
        status = fbe_api_disable_object_power_save(src_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_dualsp_mode == FBE_TRUE)
        {
            this_sp = fbe_api_transport_get_target_server();
            other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
            status = fbe_api_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_disable_object_power_save(src_obj_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              src_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              vd_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* The destination drive is the first `extra' disk.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->extra_disk_set[0].bus,
                                                                current_rg_config_p->extra_disk_set[0].enclosure,
                                                                current_rg_config_p->extra_disk_set[0].slot,
                                                                &dest_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that power saving is enabled for pvd obj: 0x%x (%d_%d_%d) ==", 
                   __FUNCTION__, dest_obj_id,
                   current_rg_config_p->extra_disk_set[0].bus,
                   current_rg_config_p->extra_disk_set[0].enclosure,
                   current_rg_config_p->extra_disk_set[0].slot);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              dest_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = weerd_validate_object_is_power_saving(dest_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start a `User Copy' request. 
         *  o Source drive is NOT power savings capable
         *  o Destination drive is power saves capable.
         */
        mut_printf(MUT_LOG_LOW, "=== rg obj: 0x%x User copy from non-power saving: %d_%d_%d to power saving: %d_%d_%d drive===",
                   rg_obj_id,
                   current_rg_config_p->rg_disk_set[position_to_copy].bus,
                   current_rg_config_p->rg_disk_set[position_to_copy].enclosure,
                   current_rg_config_p->rg_disk_set[position_to_copy].slot,
                   current_rg_config_p->extra_disk_set[0].bus,
                   current_rg_config_p->extra_disk_set[0].enclosure,
                   current_rg_config_p->extra_disk_set[0].slot);

        /* get the edge index of the source and destination drive based on configuration mode. 
        */
        diabolicdiscdemon_get_source_destination_edge_index(vd_obj_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Set a debug hook after we switch to mirror mode but before we start the 
         * metadata rebuild.
        */
        status = fbe_api_scheduler_add_debug_hook(vd_obj_id,
                                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                  FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                                  0, 0,              
                                                  SCHEDULER_CHECK_STATES,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* we need to wait for raid group ready, otherwise, it would reject the copy */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              rg_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now start the copy operation.
         */
        status = fbe_api_copy_to_replacement_disk(src_obj_id, dest_obj_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR);

        /* Wait until we start the metadata rebuild.
         */
        status = fbe_test_wait_for_debug_hook(vd_obj_id, 
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that NR is set on the destination drive*/
        fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_obj_id, dest_edge_index);

        /* Remove the rebuild start hook. */
        status = fbe_api_scheduler_del_debug_hook(vd_obj_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* wait for the proactive copy completion. */
        diabolicdiscdemon_wait_for_proactive_copy_completion(vd_obj_id);

        /* wait for the swap-out of the drive after proactive copy completion. */
        diabolicdiscdemon_wait_for_source_drive_swap_out(vd_obj_id, source_edge_index);

        /* Before we refresh the raid group information, overwrite the 
         * `extra[0]' with the source drive location.  This will be the
         * source drive to validate that it goes to power savings.
         */
        current_rg_config_p->extra_disk_set[0].bus = current_rg_config_p->rg_disk_set[position_to_copy].bus;
        current_rg_config_p->extra_disk_set[0].enclosure = current_rg_config_p->rg_disk_set[position_to_copy].enclosure;
        current_rg_config_p->extra_disk_set[0].slot = current_rg_config_p->rg_disk_set[position_to_copy].slot;

        /* Refresh the raid group disk set information. */
        fbe_test_sep_util_raid_group_refresh_disk_info(current_rg_config_p, 1);

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups copy form non-power savings to power savings drive*/

    /* Step 8:  Validate that the user data was correctly copied.
     */
    diabolicdiscdemon_verify_background_pattern(context_p, WEERD_TEST_ELEMENT_SIZE);

    /* Step 9:  Copy from a a `non-power savings' capable drive to a `power savings'
     *          capable drive.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Copy operations are not support on RAID-0
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* Goto next */
            current_rg_config_p++;
            continue;
        }

        /* Now validate that the source drive for the previous copy operation 
         * goes to hibernate.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate thar the original source drive is not power savings.
         * The source drive is the first `extra' disk.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->extra_disk_set[0].bus,
                                                                current_rg_config_p->extra_disk_set[0].enclosure,
                                                                current_rg_config_p->extra_disk_set[0].slot,
                                                                &pvd_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that power saving is disabled for pvd obj: 0x%x (%d_%d_%d) ==", 
                   __FUNCTION__, pvd_obj_id,
                   current_rg_config_p->extra_disk_set[0].bus,
                   current_rg_config_p->extra_disk_set[0].enclosure,
                   current_rg_config_p->extra_disk_set[0].slot);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              pvd_obj_id, 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              WEERD_WAIT_OBJECT_TIMEOUT_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = weerd_validate_object_power_saving_disabled(pvd_obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups validate power savings drive goes back to hibernation*/

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s completed successfully. ****\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/***********************************************
 * end weerd_test_user_copy_on_hibernating_vd()
 ***********************************************/

static void weerd_test_enable_system_power_saving(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    fbe_transport_connection_target_t           this_sp;
    fbe_transport_connection_target_t           other_sp;
    fbe_bool_t                                  b_is_dualsp_mode;
    
    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        this_sp = fbe_api_transport_get_target_server();
        other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status  = fbe_api_enable_system_power_save();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);
}

static void weerd_test_disable_system_power_saving(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    fbe_transport_connection_target_t           this_sp;
    fbe_transport_connection_target_t           other_sp;
    fbe_bool_t                                  b_is_dualsp_mode;
    
    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Disable system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        this_sp = fbe_api_transport_get_target_server();
        other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status  = fbe_api_disable_system_power_save();
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /*make sure it worked*/
    power_save_info.enabled = FBE_TRUE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_FALSE);
}

static void weerd_test_verify_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_bool_t                          b_is_dualsp_mode;

    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* First get the sp id this was invoked on and out peer.
     */
    
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        this_sp = fbe_api_transport_get_target_server();
        other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        weerd_test_verify_power_saving_policy_current_sp(rg_config_p);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    weerd_test_verify_power_saving_policy_current_sp(rg_config_p);
}

static void weerd_test_verify_power_saving_policy_current_sp(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_lun_set_power_saving_policy_t           lun_ps_policy;
    fbe_object_id_t                             lu_object_id;
    fbe_status_t                                status;
    fbe_u32_t                                   lun_index;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== setting power saving policy ===\n");

    for ( lun_index = 0; lun_index < WEERD_LUNS_PER_RAID_GROUP; lun_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                       &lu_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        lun_ps_policy.lun_delay_before_hibernate_in_sec = WEERD_LUN_IDLE_TIME_BEFORE_HIBERNATE_SECS ;
        lun_ps_policy.max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000);/*we allow 1000 minutes for the drives to wake up (default for drives is 100 minutes)*/
        status = fbe_api_lun_set_power_saving_policy(lu_object_id, &lun_ps_policy);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        lun_ps_policy.lun_delay_before_hibernate_in_sec = 0;
        lun_ps_policy.max_latency_allowed_in_100ns = 0;

        mut_printf(MUT_LOG_TEST_STATUS, "=== getting power saving policy ===\n");

        status = fbe_api_lun_get_power_saving_policy(lu_object_id, &lun_ps_policy);
        MUT_ASSERT_TRUE(FBE_STATUS_OK == status);
        MUT_ASSERT_TRUE(lun_ps_policy.lun_delay_before_hibernate_in_sec == WEERD_LUN_IDLE_TIME_BEFORE_HIBERNATE_SECS);
        MUT_ASSERT_TRUE(lun_ps_policy.max_latency_allowed_in_100ns == (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000));
    }
}

static void weerd_test_enable_power_saving_policy(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
{
    fbe_status_t                        status;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_bool_t                          b_is_dualsp_mode;

    /* Check if we need to check boith SPs
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* First get the sp id this was invoked on and out peer.
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        status = fbe_api_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        weerd_test_enable_power_saving_policy_current_sp(rg_config_p, raid_group_count);
        status = fbe_api_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    weerd_test_enable_power_saving_policy_current_sp(rg_config_p, raid_group_count);
}

static void weerd_test_enable_power_saving_policy_current_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
{
    fbe_status_t                            status;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_object_id_t                         object_id, rg_object_id, pvd_object_id;
    fbe_u32_t                               position;
    fbe_u32_t                               idle_time_secs = WEERD_IDLE_TIME_BEFORE_HIBERNATE_SECS;
    fbe_u32_t                               rg_index;

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Disable background zeroing */
        for (position = 0 ; position < current_rg_config_p->width; position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position].bus,
                                                                    current_rg_config_p->rg_disk_set[position].enclosure,
                                                                    current_rg_config_p->rg_disk_set[position].slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Before starting the sniff related operation disable the disk zeroing.
             */
            status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s change rg obj: 0x%x objects to %d seconds ===\n",
                   __FUNCTION__, rg_object_id, idle_time_secs);

        /* Change the raid group idle time */
        status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, idle_time_secs);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Change the pvd idle time */
        for (position = 0; position < current_rg_config_p->width; position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position].bus,
                                                                    current_rg_config_p->rg_disk_set[position].enclosure,
                                                                    current_rg_config_p->rg_disk_set[position].slot, &object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_set_object_power_save_idle_time(object_id, idle_time_secs);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_set_object_power_save_idle_time(object_id, idle_time_secs);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        mut_printf(MUT_LOG_TEST_STATUS, "===Enabling power saving on rg obj: 0x%x (should propagate to all objects) ===\n",
                   rg_object_id);
        status = fbe_api_enable_raid_group_power_save(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end enable power save on raid groups*/

}
 
/*!***************************************************************************
 *          weerd_test_proactive_copy_on_hibernating_vd()
 *****************************************************************************
 *
 * @brief   Run Weerd proactive copy on hibernating vd test for the 
 *          specific RAID group.
 *
 *  The test performs the following tasks:
 *      -Enables the system power-saving mode.
 *      -Sets the power saving policies as per requirement.
 *      -Brings the bound drives into hibernating mode.
 *      -Triggers proactive copy on one of the bound disks (by error injection).
 *      -Waits for the proactive spare to swap in. 
 *      -Verifies that proactive copy completes and waits for the swap-out
 *       of the drive. 
 *       
 *
 * @param   rg_config_p - Pointer to single (1) raid group under test
 *
 * @return  None
 *
 * @author
 *    12/27/2012 - Modified. Vera Wang
 *
 *****************************************************************************/
static void weerd_test_bring_bound_objects_to_hibernating_state(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count)
{
    fbe_status_t                            status;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_object_id_t                         object_id, rg_object_id, lu_object_id, pdo_object_id;
    fbe_u32_t                               position;
    fbe_u32_t                               rg_index;
    fbe_bool_t                              b_is_dualsp_mode;

    /* Step 1:  For all raid groups under test, disable zeroing and change power
     *          save idle time. 
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* Step 2:  Validate that all objects associated with the raid groups (LUN,
     *          vds, pvds etc) now enter power save.
     */
    weerd_test_enable_power_saving_policy(rg_config_p, raid_group_count);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /*now let's wait and make sure the LU and other objects connected to it are hibernating*/
        status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[0].bus,
                                                                  current_rg_config_p->rg_disk_set[0].enclosure,
                                                                  current_rg_config_p->rg_disk_set[0].slot,
                                                                  &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify PDO is saving power, pdo_obj_id:0x%x\n", __FUNCTION__, pdo_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              pdo_object_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified PDO is saving power, pdo_obj_id:0x%x\n", __FUNCTION__, pdo_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "Making sure all bound objects saving power\n");
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, &lu_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify LU object is saving power, lu_obj_id:0x%x\n", __FUNCTION__, lu_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              lu_object_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*make sure LU is saving power */
        status = weerd_validate_object_is_power_saving(lu_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified LU object is saving power, lu_obj_id:0x%x\n", __FUNCTION__, lu_object_id);

        /*make sure RAID saving power */
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify RAID object is saving power, rg_obj_id:0x%x\n", __FUNCTION__, rg_object_id);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                              rg_object_id, 
                                                                              FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                              WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = weerd_validate_object_is_power_saving(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified RAID object is saving power, rg_obj_id:0x%x\n", __FUNCTION__, rg_object_id);

        for (position = 0; position < current_rg_config_p->width; position++)
        {
            /*make sure VD saving power */
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify VD object is saving power, vd_obj_id:0x%x\n", __FUNCTION__, object_id);
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                                  object_id, 
                                                                                  FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                                  WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                                  FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = weerd_validate_object_is_power_saving(object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified VD object is saving power, vd_obj_id:0x%x\n", __FUNCTION__, object_id);

            /*make sure PVD saving power */
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position].bus,
                                                                    current_rg_config_p->rg_disk_set[position].enclosure,
                                                                    current_rg_config_p->rg_disk_set[position].slot, &object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, " %s: Verify PVD object is saving power, pvd_obj_id:0x%x\n", __FUNCTION__, object_id);
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(b_is_dualsp_mode,
                                                                                  object_id, 
                                                                                  FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                                                  WEERD_WAIT_FOR_HIBERNATION_TMO_MS, 
                                                                                  FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = weerd_validate_object_is_power_saving(object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified PVD object is saving power, pvd_obj_id:0x%x\n", __FUNCTION__, object_id);

        } /* end for all raid group positions*/

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end enable power save on raid groups*/

    return;
}
/**************************************************************** 
 * end weerd_test_bring_bound_objects_to_hibernating_state()
 ****************************************************************/

/*!**************************************************************
 *          weerd_test_rg_config()
 ****************************************************************
 *
 * @brief   Run I/O validataion after proactive copy.  The run I/O
 *          with proactive copy operation in progress.
 *
 * @param   rg_config_p - config to run test against. 
 * @param   test_context_p - Pointer to rdgen context              
 *
 * @return None.   
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void *test_context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &weerd_io_context[0];
    fbe_u32_t                   position_to_copy = 0;
    fbe_u32_t                   raid_group_count;

    /* Get how many raid groups are under test
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* This test uses `big_bird' to remove drives so set the copy position to
     * next position to remove (this can only be done for source drive removal).
     */
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /* write background pattern to all the luns before we start copy operation. 
     */
    diabolicdiscdemon_write_background_pattern(context_p);

    /* Initiate proactive on hibernating drives.*/
    status = weerd_test_proactive_copy_on_hibernating_vd(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initiate user copy to on hibernating drives.*/
    position_to_copy += 1;
    status = weerd_test_user_copy_on_hibernating_vd(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    weerd_test_disable_system_power_saving();    
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    /* Set the sector trace stop on error off.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    return;
}
/******************************************
 * end weerd_test_rg_config()
 ******************************************/

/*!**************************************************************
 * weerd_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        /* Extended 
         */
        raid_group_types = WEERD_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        /* Qual 
         */
        raid_group_types = WEERD_QUAL_TEST_CONFIGURATION_TYPES;
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Loop over the number of configurations we have.
     */
    for (raid_group_type_index = 0; raid_group_type_index < raid_group_types; raid_group_type_index++)
    {
        if (test_level > 0)
        {
            /* Extended
             */
            rg_config_p = &weerd_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            /* Qual
             */
            rg_config_p = &weerd_raid_groups_qual[raid_group_type_index][0];
        }

        if (raid_group_type_index + 1 >= raid_group_types) {

            /* Since this doesn't test degraded disk not additional disk are 
             * required.  All the copy disk will come from the automatic spare pool.
             */
            fbe_test_run_test_on_rg_config(rg_config_p, NULL, weerd_test_rg_config,
                                           WEERD_LUNS_PER_RAID_GROUP,
                                           WEERD_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, weerd_test_rg_config,
                                           WEERD_LUNS_PER_RAID_GROUP,
                                           WEERD_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }

    } /* for all raid group types */

    return;
}
/******************************************
 * end weerd_test()
 ******************************************/

/*!**************************************************************
 * weerd_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_dualsp_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = WEERD_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = WEERD_QUAL_TEST_CONFIGURATION_TYPES;
    }


    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Loop over the number of configurations we have.
     */
    for (raid_group_type_index = 0; raid_group_type_index < raid_group_types; raid_group_type_index++)
    {
        if (test_level > 0)
        {
            rg_config_p = &weerd_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            rg_config_p = &weerd_raid_groups_qual[raid_group_type_index][0];
        }

        if (raid_group_type_index + 1 >= raid_group_types) {
            /* Since this doesn't test degraded disk not additional disk are 
             * required.  All the copy disk will come from the automatic spare pool.
             */
            fbe_test_run_test_on_rg_config(rg_config_p, NULL, weerd_test_rg_config,
                                           WEERD_LUNS_PER_RAID_GROUP,
                                           WEERD_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, weerd_test_rg_config,
                                           WEERD_LUNS_PER_RAID_GROUP,
                                           WEERD_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }

    } /* for all raid group types */

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end weerd_dualsp_test()
 ******************************************/

/*!**************************************************************
 * weerd_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t rg_index, raid_type_count;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        if (test_level > 0)
        {
            /* Extended.
             */
            array_p = &weerd_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &weerd_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_sep_util_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
        sep_config_load_sep_and_neit();
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end weerd_setup()
 **************************************/

/*!**************************************************************
 * weerd_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t rg_index, raid_type_count;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        if (test_level > 0)
        {
            /* Extended.
             */
            array_p = &weerd_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &weerd_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_sep_util_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

    }

    return;
}
/**************************************
 * end weerd_dualsp_setup()
 **************************************/

/*!**************************************************************
 * weerd_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end weerd_cleanup()
 ******************************************/

/*!**************************************************************
 * weerd_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  01/28/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void weerd_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {

        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end weerd_dualsp_cleanup()
 ******************************************/
