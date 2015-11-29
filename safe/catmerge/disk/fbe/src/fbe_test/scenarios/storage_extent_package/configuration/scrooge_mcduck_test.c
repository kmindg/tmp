
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file scrooge_mcduck_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for drive performance tier type required for 
 *   cache exanpansion
 *
 * @version
 *   1/2014 - Created.  Deanna Heng
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "sep_tests.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "sep_utils.h"
#include "fbe_database.h"
#include "pp_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "sep_test_io.h"
#include "sep_rebuild_utils.h"
#include "fbe_raid_geometry.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe_test.h"
#include "fbe/fbe_api_board_interface.h"
#include "sep_hook.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char *scrooge_mcduck_test_short_desc = "Tests that the performance tier type in a raid group is propogated properly.";
char *scrooge_mcduck_test_long_desc =
    "\n"
    "\n"
    "This test verifies that the vault raid group reports the correct drive tier\n"
    "when the system is brought up with different drive configurations\n"
    "\n"
    "This test also verifies the transition of 10K to 15K in the vault raid group\n"
    "\n"
    "This test also verifies that if a 10K drive is placed in a 15K vault, the 10K \n"
    "drive will get shot. The drive is processed and shot again after a reboot\n"
    "\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
/*!*******************************************************************
 * @def SCROOGE_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 1 encl + 4 pdo) 
 *
 *********************************************************************/
#define SCROOGE_MCDUCK_TEST_NUMBER_OF_PHYSICAL_OBJECTS 7
#define SCROOGE_MCDUCK_TEST_NUM_VAULT 4
#define SCROOGE_MCDUCK_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY
#define SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY //+ TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY
#define SCROOGE_MCDUCK_TEST_WAIT_SEC 120
#define SCROOGE_MCDUCK_TEST_WAIT_MSEC 30000
#define SCROOGE_MCDUCK_TEST_LUNS_PER_RAID_GROUP 1
#define SCROOGE_MCDUCK_CHUNKS_PER_LUN_TO_TEST    2


/************************* 
 * EXTERNS
 *************************/
extern void sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, fbe_u32_t death_reason);


/* this was taken from fbe_test_pp_utils */
#define GET_SAS_DRIVE_ADDRESS(BN, EN, SN) \
    (0x5000097B80000000 + ((fbe_u64_t)(BN) << 16) + ((fbe_u64_t)(EN) << 8) + (SN))

/* This is a set of parameters for the Scrooge McDuck test
 */
typedef struct scrooge_mcduck_vault_drive_details_s
{
    fbe_sas_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    fbe_u32_t encl_number;      /* The unique enclosure number */
    fbe_u32_t slot_number;
    fbe_block_size_t block_size;
    fbe_sas_address_t sas_address;
} scrooge_mcduck_vault_drive_details_t;

/*!***************************************************************
 * @fn fbe_scrooge_mcduck_subtest_t
 * 
 * @brief This function runs within a bringup test context
 *
 ******************************************************************/
typedef void (*fbe_scrooge_mcduck_subtest_t)(void *context);

/* the set of drives on bring up and what the value of drive tier should be
 */
typedef struct scrooge_mcduck_bringup_test_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t line;                     /*!< Line number of test case. */
    fbe_drive_performance_tier_type_np_t drive_tier;  /*!< Drive tier value */
    scrooge_mcduck_vault_drive_details_t vault_drives[SCROOGE_MCDUCK_TEST_NUM_VAULT];  /*!< Description of vault drives. */
    fbe_scrooge_mcduck_subtest_t         scrooge_subtest_fn_p;

}scrooge_mcduck_bringup_test_case_t;

/* Mapping to real drive types to use for this test
 */
typedef enum scrooge_mcduck_drive_type_s
{
    SCROOGE_MCDUCK_INVALID_DRIVE = 0,
    SCROOGE_MCDUCK_7K_DRIVE = FBE_SAS_NL_DRIVE_SIM_520,
    SCROOGE_MCDUCK_15K_DRIVE = FBE_SAS_DRIVE_SIM_520, //FBE_SAS_DRIVE_CHEETA_15K,
    SCROOGE_MCDUCK_FLASH_DRIVE = FBE_SAS_DRIVE_SIM_520_FLASH_HE,
    SCROOGE_MCDUCK_10K_DRIVE = FBE_SAS_DRIVE_COBRA_E_10K,
} scrooge_mcduck_drive_type_t;

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
static void scrooge_mcduck_shutdown_sps(void);
static void scrooge_mcduck_poweron_sps(void);
static void scrooge_mcduck_pull_drive(fbe_u32_t port_number,
                                      fbe_u32_t enclosure_number,
                                      fbe_u32_t slot_number,
                                      fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                      fbe_api_terminator_device_handle_t *drive_handle_p_spb);
static void scrooge_mcduck_insert_blank_new_drive_extended(fbe_u32_t port_number,
                                                           fbe_u32_t enclosure_number,
                                                           fbe_u32_t slot_number,
                                                           fbe_lba_t capacity,
                                                           fbe_sas_drive_type_t drive_type,
                                                           fbe_block_size_t block_size,
                                                           fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                                           fbe_api_terminator_device_handle_t *drive_handle_p_spb,
                                                           fbe_sas_address_t *sas_address_p);
static void scrooge_mcduck_wait_for_object_state(fbe_object_id_t object_id,
                                                 fbe_lifecycle_state_t expected_lifecycle_state,
                                                 fbe_package_id_t package_id);
static void scrooge_mcduck_verify_lowest_drive_tier(fbe_object_id_t rg_object_id,
                                                    fbe_drive_performance_tier_type_np_t drive_tier);
static void scrooge_mcduck_test_set_vault_attribute_on_user_rg(fbe_object_id_t rg_object_id);
static void scrooge_mcduck_test_create_fake_vault_rg(fbe_test_rg_configuration_t *rg_config_p);
static void scrooge_mcduck_replace_drive_with_new_drive(fbe_u32_t port,
                                                        fbe_u32_t encl,
                                                        fbe_u32_t slot,
                                                        fbe_lba_t capacity,
                                                        scrooge_mcduck_drive_type_t drive_type,
                                                        fbe_sas_address_t *sas_address_p);
static void scrooge_mcduck_replace_position_with_new_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t position,
                                                           fbe_drive_performance_tier_type_t drive_tier);
static void scrooge_mcduck_validate_drive_killed_message_present(fbe_u32_t bus,
                                                                 fbe_u32_t encl,
                                                                 fbe_u32_t slot);
static void scrooge_mcduck_wait_for_debug_hook(fbe_object_id_t object_id,
                                               fbe_u32_t state,
                                               fbe_u32_t substate,
                                               fbe_u32_t check_type,
                                               fbe_u32_t action,
                                               fbe_u64_t val1,
                                               fbe_u64_t val2);
static void scrooge_mcduck_wait_for_system_drives_rebuilt(fbe_u32_t position);
static void scrooge_mcduck_test_10K_to_15K_transition(fbe_test_rg_configuration_t *rg_config_p);
static void scrooge_mcduck_run_bringup_test_case(scrooge_mcduck_bringup_test_case_t *test_case_p);
static void scrooge_mcduck_run_bringup_test_cases(fbe_test_rg_configuration_t *rg_config_p,
                                                  void *context);
static void scrooge_mcduck_run_vault_drive_replacement_subtest(void *context);
static void scrooge_mcduck_vault_kill_drive_15K_subtest(void *context);
static void scrooge_mcduck_extended_cache_disabled_subtest(void *context);
static void scrooge_mcduck_degraded_vault_startup_subtest(void *context);
static void scrooge_mcduck_degraded_vault_in_specialize_subtest(void *context);
static void scrooge_mcduck_test_load_physical_config(scrooge_mcduck_bringup_test_case_t *test_case_p);

/* This user raid group is used to imitate the vault raid group 
 * This was done since the system drives took too long to rebuild
 */
static fbe_test_rg_configuration_t scrooge_mcduck_rg_config[] =
{
    /*! @note Although all fields must be valid only the system id is used to populate the test config+
     *                                                                                                |
     *                                                                                                v    */
    /*width capacity    raid type                   class               block size  system id                               bandwidth.*/
    { 4,     0xF004800,  FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,       FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,   0,
        { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 2 }, { 0, 0, 3 } }
    },
    { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/* Table containing Scrooge McDuck test cases
 */
static scrooge_mcduck_bringup_test_case_t scrooge_mcduck_bringup_test_cases[] =
{

    {
        "All 10K drives in the vault",
        __LINE__,
        FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K,
        {
            {   SCROOGE_MCDUCK_10K_DRIVE,
                0, 0, 0, 520, 0
            },
            {   SCROOGE_MCDUCK_10K_DRIVE,
                0, 0, 1, 520, 0
            },
            {   SCROOGE_MCDUCK_10K_DRIVE,
                0, 0, 2, 520, 0
            },
            {   SCROOGE_MCDUCK_10K_DRIVE,
                0, 0, 3, 520, 0
            },
        },
        scrooge_mcduck_run_vault_drive_replacement_subtest,
    },
    {
        "All 15K drives in the vault",
        __LINE__,
        FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K,
        {
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 0, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 1, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 2, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 3, 520, 0
            },
        },
        scrooge_mcduck_vault_kill_drive_15K_subtest,
    },
    {
        "Mix of 10K and 15K drives in the vault",
        __LINE__,
        FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K,
        {
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 0, 520, 0
            },
            {   SCROOGE_MCDUCK_10K_DRIVE,
                0, 0, 1, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 2, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 3, 520, 0
            },
        },
        scrooge_mcduck_degraded_vault_startup_subtest,
    },
    {
        "7K drives in the vault",
        __LINE__,
        FBE_DRIVE_PERFORMANCE_TIER_TYPE_7K,
        {
            {   SCROOGE_MCDUCK_7K_DRIVE,
                0, 0, 0, 520, 0
            },
            {   SCROOGE_MCDUCK_7K_DRIVE,
                0, 0, 1, 520, 0
            },
            {   SCROOGE_MCDUCK_7K_DRIVE,
                0, 0, 2, 520, 0
            },
            {   SCROOGE_MCDUCK_7K_DRIVE,
                0, 0, 3, 520, 0
            },
        },
        scrooge_mcduck_degraded_vault_in_specialize_subtest,
    },
    {
        "Mix of 15K and flash drives in the vault",
        __LINE__,
        FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K,
        {
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 0, 520, 0
            },
            {   SCROOGE_MCDUCK_15K_DRIVE,
                0, 0, 1, 520, 0
            },
            {   SCROOGE_MCDUCK_FLASH_DRIVE,
                0, 0, 2, 520, 0
            },
            {   SCROOGE_MCDUCK_FLASH_DRIVE,
                0, 0, 3, 520, 0
            },
        },
        scrooge_mcduck_extended_cache_disabled_subtest,
    },
    /* terminator */
    {
        0, 0, FBE_U16_MAX,
    },
};

/*!**************************************************************
 * scrooge_mcduck_shutdown_sps()
 ****************************************************************
 * @brief
 *  Shutdown both sps.
 *
 * @return None.   
 *
 * @author
 *  01/31/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_shutdown_sps(void)
{
    fbe_sim_transport_connection_target_t   original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep();

    fbe_api_sim_transport_set_target_server(original_target);
}


/*!**************************************************************
 * scrooge_mcduck_poweron_sps()
 ****************************************************************
 * @brief
 *  Power on both sps.
 *
 * @return None.   
 *
 * @author
 *  01/31/2014 - Created. Deanna Heng 
 ****************************************************************/
static void scrooge_mcduck_poweron_sps(void)
{
    fbe_sim_transport_connection_target_t   original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_esp_sep_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_esp_sep_and_neit();
    fbe_api_sim_transport_set_target_server(original_target);

}

/*!**************************************************************
 * scrooge_pull_drive()
 ****************************************************************
 * @brief
 *  Pull a specified drive from array
 *
 * @param port_number - drive location.
 * @param enclosure_number - drive location.
 * @param slot_number - drive location.
 * @param drive_handle_p_spa - the handle of the drive on SPA
 * @param drive_handle_p_spb - the handle of the drive on SPB
 *
 * @return None.   
 *
 * @author
 *  01/31/2014 - Created. Deanna Heng, Taken from Sabbath
 ****************************************************************/
static void scrooge_mcduck_pull_drive(fbe_u32_t port_number,
                                      fbe_u32_t enclosure_number,
                                      fbe_u32_t slot_number,
                                      fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                      fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_object_id_t                         pdo = FBE_OBJECT_ID_INVALID;

    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, slot_number, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_pull_drive(port_number, enclosure_number, slot_number, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "scrooge_mcduck_pull_drive: fail to pull drive from SPA");
    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for PDO: 0x%x to be destroyed", __FUNCTION__, pdo);
    status = fbe_api_wait_till_object_is_destroyed(pdo, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, slot_number, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_pull_drive(port_number, enclosure_number, slot_number, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "scrooge_mcduck_drive: fail to pull drive from SPB");
    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for PDO: 0x%x to be destroyed", __FUNCTION__, pdo);
    status = fbe_api_wait_till_object_is_destroyed(pdo, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * scrooge_mcduck_insert_blank_new_drive_extended()
 ****************************************************************
 * @brief
 *  Insert a blank new drive into array
 *
 * @param port_number - drive location where to insert.
 * @param enclosure_number - drive location where to insert.
 * @param slot_number - drive location where to insert.
 * @param capacity - the capacity of the new drive
 * @param drive_type - the type of the new drive
 * @param block_size - block size of the drive
 * @param drive_handle_p_spa - the handle of the newly inserted drive on SPA
 * @param drive_handle_p_spb - the handle of the newly inserted drive on SPB
 * @param sas_address_p - pointer for the new sas address of the inserted drive
 *
 * @return None.   
 *
 * @author
 *  01/31/2014 - Created. Deanna Heng Taken from Sabbath
 ****************************************************************/
static void scrooge_mcduck_insert_blank_new_drive_extended(fbe_u32_t port_number,
                                                           fbe_u32_t enclosure_number,
                                                           fbe_u32_t slot_number,
                                                           fbe_lba_t capacity,
                                                           fbe_sas_drive_type_t drive_type,
                                                           fbe_block_size_t block_size,
                                                           fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                                           fbe_api_terminator_device_handle_t *drive_handle_p_spb,
                                                           fbe_sas_address_t *sas_address_p)
{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t   blank_new_sas_address;


    original_target = fbe_api_sim_transport_get_target_server();

    /* Note: Always insert the drive on SP B first. Since we always bring up sp A first we need
      to make sure the drive is inserted on the passive side so that when the active side sends the command
      to shoot the drive, the drive is ready to be shot on the passive side. */
    blank_new_sas_address = fbe_test_pp_util_get_unique_sas_address(); //GET_SAS_DRIVE_ADDRESS(port_number, enclosure_number, slot_number);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number,
                                                      block_size, capacity,
                                                      blank_new_sas_address, drive_type, drive_handle_p_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "scrooge_mcduck_insert_blank_new_drive_extended: fail to insert drive from SPB");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_insert_sas_drive_extend(port_number, enclosure_number, slot_number,
                                                      block_size, capacity,
                                                      blank_new_sas_address, drive_type, drive_handle_p_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "scrooge_mcduck_insert_blank_new_drive_extended: fail to insert drive from SPA");

    *sas_address_p = blank_new_sas_address;
    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * scrooge_mcduck_wait_for_sep_object_state()
 ****************************************************************
 * @brief
 *  Wait for a SEP object to become the specified lifecycle state
 *
 * @param object_id     
 * @param expected_life_cycle_state - state to wait for     
 * @param package_id 
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_wait_for_object_state(fbe_object_id_t object_id,
                                                 fbe_lifecycle_state_t expected_lifecycle_state,
                                                 fbe_package_id_t package_id)
{
    fbe_sim_transport_connection_target_t   original_target;
    fbe_status_t status = FBE_STATUS_OK;

    original_target = fbe_api_sim_transport_get_target_server();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_wait_for_object_lifecycle_state(object_id, expected_lifecycle_state, SCROOGE_MCDUCK_TEST_WAIT_MSEC, package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_wait_for_object_lifecycle_state(object_id, expected_lifecycle_state, SCROOGE_MCDUCK_TEST_WAIT_MSEC, package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * scrooge_mcduck_verify_lowest_drive_tier()
 ****************************************************************
 * @brief
 *  Verify that the raid group reports the specified drive tier
 *
 * @param rg_object_id     
 * @param drive_tier - expected drive tier reported  
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_verify_lowest_drive_tier(fbe_object_id_t rg_object_id,
                                                    fbe_drive_performance_tier_type_np_t drive_tier)
{
    fbe_status_t status;
    fbe_get_lowest_drive_tier_t rg_get_lowest_drive_tier = { 0 };

    status = fbe_api_raid_group_get_lowest_drive_tier(rg_object_id,
                                                      &rg_get_lowest_drive_tier);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(drive_tier, rg_get_lowest_drive_tier.drive_tier);
    mut_printf(MUT_LOG_TEST_STATUS, "Verified performance tier type is %d\n", drive_tier);
}

/*!**************************************************************
 * scrooge_mcduck_test_create_fake_vault_rg()
 ****************************************************************
 * @brief
 *  and set the vault attribute on the raid group
 *  
 * @param rg_object_id       
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_test_set_vault_attribute_on_user_rg(fbe_object_id_t rg_object_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    status  = fbe_api_raid_group_set_raid_attributes(rg_object_id,
                                                     FBE_RAID_ATTRIBUTE_VAULT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}


/*!**************************************************************
 * scrooge_mcduck_test_create_fake_vault_rg()
 ****************************************************************
 * @brief
 *  Create a raid group and lun, and set the vault attribute on the raid group
 *  Masks a user raid group as the vault raid group
 *
 * @param rg_config_p - raid group configuration under test          
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_test_create_fake_vault_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_api_raid_group_create_t         create_rg;
    fbe_api_lun_create_t    		    fbe_lun_create_req;
    fbe_object_id_t         		    lu_id = FBE_OBJECT_ID_INVALID;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = rg_config_p->raid_group_id;
    create_rg.b_bandwidth = rg_config_p->b_bandwidth;
    create_rg.capacity = rg_config_p->capacity;
    create_rg.drive_count = rg_config_p->width;
    create_rg.raid_type = rg_config_p->raid_type;
    create_rg.is_private = FBE_FALSE;
    fbe_copy_memory(create_rg.disk_array, rg_config_p->rg_disk_set, rg_config_p->width * sizeof(fbe_test_raid_group_disk_set_t));

    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR,
                             "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    /*let's bind a lun on this RG*/
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = rg_config_p->raid_type;
    fbe_lun_create_req.raid_group_id = rg_config_p->raid_group_id;
    fbe_lun_create_req.lun_number = 0;
    fbe_lun_create_req.capacity = 0x4000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_TRUE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_FALSE, 15000, &lu_id, &job_error_code);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    scrooge_mcduck_test_set_vault_attribute_on_user_rg(rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "Set Vault Attribute on Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

}

/*!**************************************************************
 * scrooge_mcduck_replace_position_with_new_drive()
 ****************************************************************
 * @brief
 *  Replace a drive at specified location in rg_config_p with drive of 
 *  specified drive tier and capcity
 *
 * @param port - 
 * @param encl -   
 * @param slot -
 * @param capacity -    
 * @param drive_type -            
 * @param sas_address_p - pointer for the new sas address of the inserted drive
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_replace_drive_with_new_drive(fbe_u32_t port,
                                                        fbe_u32_t encl,
                                                        fbe_u32_t slot,
                                                        fbe_lba_t capacity,
                                                        scrooge_mcduck_drive_type_t drive_type,
                                                        fbe_sas_address_t *sas_address_p)
{
    fbe_api_terminator_device_handle_t  drive_handle_spa;
    fbe_api_terminator_device_handle_t  drive_handle_spb;
    fbe_api_terminator_device_handle_t  drive_handler_new_drive_spa;
    fbe_api_terminator_device_handle_t  drive_handler_new_drive_spb;

    scrooge_mcduck_pull_drive(port, encl, slot,
                              &drive_handle_spa,
                              &drive_handle_spb);
    scrooge_mcduck_insert_blank_new_drive_extended(port, encl, slot,
                                                   capacity, drive_type, 520,
                                                   &drive_handler_new_drive_spa,
                                                   &drive_handler_new_drive_spb,
                                                   sas_address_p);

}

/*!**************************************************************
 * scrooge_mcduck_replace_position_with_new_drive()
 ****************************************************************
 * @brief
 *  Replace a drive at specified position in rg_config_p with the 
 *  specified drive tier. Verify that a new pvd is created for this
 *  new drive
 *
 * @param rg_config_p - the raid group config under test   
 * @param position - drive position to replace    
 * @param drive_tier - new drive tier replacement            
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_replace_position_with_new_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t position,
                                                           fbe_drive_performance_tier_type_t drive_tier)
{
    fbe_object_id_t pvd_id_spa = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_id_spb = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_u32_t bus = rg_config_p->rg_disk_set[position].bus;
    fbe_u32_t encl = rg_config_p->rg_disk_set[position].enclosure;
    fbe_u32_t slot = rg_config_p->rg_disk_set[position].slot;
    fbe_sas_address_t sas_address;

    original_target = fbe_api_sim_transport_get_target_server();

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    scrooge_mcduck_replace_drive_with_new_drive(bus, encl, slot,
                                                SCROOGE_MCDUCK_TEST_DRIVE_CAPACITY,
                                                drive_tier, &sas_address);

    fbe_api_sleep(5000); // allow database to process
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_id_spb);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_id_spa);
    fbe_api_sim_transport_set_target_server(original_target);

    MUT_ASSERT_INT_EQUAL(pvd_id_spb, pvd_id_spa);
    scrooge_mcduck_wait_for_object_state(pvd_id_spa,
                                         FBE_LIFECYCLE_STATE_READY,
                                         FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, pvd_id_spa);

}

/*!**************************************************************
 * scrooge_mcduck_validate_drive_killed_message_present()
 ****************************************************************
 * @brief
 *  Checks event log to figure out whether an event log message 
 *  with given attributes is present or not. Verify that the pdo
 *  is failed with the expected death reason
 *
 * @param bus   - bus
 * @param encl  - enclosure
 * @param slot  - slot
 * @param serial_number - expected serial number of drive
 * 
 * @return 
 *
 * @author
 *  2/10/2014 - Created - Deanna Heng
 *
 ****************************************************************/
static void scrooge_mcduck_validate_drive_killed_message_present(fbe_u32_t bus,
                                                                 fbe_u32_t encl,
                                                                 fbe_u32_t slot)
{

    /* Example:
     *   0xe167802f   Bus 0 Enclosure 0 Disk 0 taken offline. SN:6000097B80000012 TLA:005050084PWR Rev:C5B0 (0x22d0014) Reason:VaultCfg Action:Replace.
     */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_physical_drive_information_t physical_drive_info = { 0 };
    fbe_object_id_t pdo = FBE_OBJECT_ID_INVALID;
    fbe_sim_transport_connection_target_t   original_target;
    const fbe_u8_t *death_reason_str = NULL;
    fbe_object_death_reason_t death_reason = FBE_DEATH_REASON_INVALID;

    original_target = fbe_api_sim_transport_get_target_server();
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sleeping_beauty_verify_event_log_drive_offline(bus, encl, slot, &physical_drive_info, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION);
    scrooge_mcduck_wait_for_object_state(pdo,
                                         FBE_LIFECYCLE_STATE_FAIL,
                                         FBE_PACKAGE_ID_PHYSICAL);
    status = fbe_api_get_object_death_reason(pdo,
                                             &death_reason,
                                             &death_reason_str,
                                             FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION, death_reason);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sleeping_beauty_verify_event_log_drive_offline(bus, encl, slot, &physical_drive_info, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION);
    scrooge_mcduck_wait_for_object_state(pdo,
                                         FBE_LIFECYCLE_STATE_FAIL,
                                         FBE_PACKAGE_ID_PHYSICAL);
    status = fbe_api_get_object_death_reason(pdo,
                                             &death_reason,
                                             &death_reason_str,
                                             FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION, death_reason);


    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
* scrooge_mcduck_wait_for_debug_hook()
****************************************************************
* @brief
*  Wait for the hook to be hit on either sp
*
* @param object_id - object id to wait for
*        state - the state in the monitor 
 *       substate - the substate in the monitor           
 *
* @return 
 *
* @author
*  3/11/2014 - Created. Deanna Heng
*
****************************************************************/
static void scrooge_mcduck_wait_for_debug_hook(fbe_object_id_t object_id,
                                               fbe_u32_t state,
                                               fbe_u32_t substate,
                                               fbe_u32_t check_type,
                                               fbe_u32_t action,
                                               fbe_u64_t val1,
                                               fbe_u64_t val2)
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sim_transport_connection_target_t   target_sp;

    original_target = fbe_api_sim_transport_get_target_server();
    target_sp = original_target;

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = object_id;
    hook.check_type = check_type;
    hook.action = action;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. objid: 0x%x state: %d substate: %d.",
               object_id, state, substate);

    while (current_time < timeout_ms)
    {
        //target_sp = (target_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
        //fbe_api_sim_transport_set_target_server(target_sp);
        //mut_printf(MUT_LOG_TEST_STATUS, "Check hook on sp %d.", target_sp);
        status = fbe_api_scheduler_get_debug_hook(&hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (hook.counter != 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook objid: 0x%x state: %d substate: %d. counter 0x%llx",
                       object_id, state, substate, hook.counter);

            fbe_api_sim_transport_set_target_server(original_target);
            return;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: object 0x%x did not hit state %d substate %d in %d ms!\n",
               __FUNCTION__, object_id, state, substate, timeout_ms);

    MUT_ASSERT_TRUE(FBE_FALSE);
}


/*!**************************************************************
 * scrooge_mcduck_wait_for_system_drives_rebuilt()
 ****************************************************************
 * @brief
 *  Wait for all the system raid groups to finish rebuilding
 *
 * @param position - position being rebuilt  
 *
 * @return None.   
 *
 * @author
 *  3/12/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_wait_for_system_drives_rebuilt(fbe_u32_t position) 
{
    fbe_u32_t   index;
    for (index = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST; index <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST; index++)
    {
        if (position == 3 &&
            ((index == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR) ||
             (index == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_CBE_TRIPLE_MIRROR) ||
             (index == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG)))
        {
            continue;
        }

        sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(index, position);
        sep_rebuild_utils_wait_for_rb_comp_by_obj_id(index, position);
    }
}

/*!**************************************************************
 * scrooge_mcduck_test_10K_to_15K_transition()
 ****************************************************************
 * @brief
 *  Verify that a user raid group reports 0 for drive tier. 
 *  Mask the user raid group as the vault rg and replace 10K drives
 *  with 15K drives verifying that the correct drive tier is reported.
 *  The raid group should blindly report its lowest drive tier since no
 *  drives are being shot
 *
 * @param rg_config_p - the raid group config to run the test with              
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_test_10K_to_15K_transition(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           position = 0;


    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5); /* 5 second hotspare timeout */
    scrooge_mcduck_test_create_fake_vault_rg(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID);

    /* this will update the drive tier in the rg nonpaged */
    scrooge_mcduck_replace_position_with_new_drive(rg_config_p, rg_config_p->width - 1, SCROOGE_MCDUCK_10K_DRIVE);
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, rg_config_p->width - 1);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, rg_config_p->width - 1);
    sep_rebuild_utils_check_bits(rg_object_id);

    do
    {
        scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K);
        scrooge_mcduck_replace_position_with_new_drive(rg_config_p, position, SCROOGE_MCDUCK_15K_DRIVE);
        sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, 0);
        sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);
        sep_rebuild_utils_check_bits(rg_object_id);
        position = position + 1;
    } while (position < rg_config_p->width);

    /* Verify that the transition happens after all drives in the raid group are 15K 
     */
    scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);

    /* Verfity that the drive tier does not change when a 10K is inserted in a 15K rg 
     */
    scrooge_mcduck_replace_position_with_new_drive(rg_config_p, 0, SCROOGE_MCDUCK_10K_DRIVE);
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, 0);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, 0);
    sep_rebuild_utils_check_bits(rg_object_id);
    scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K);
}

/*!**************************************************************
 * scrooge_mcduck_run_bringup_test_case()
 ****************************************************************
 * @brief
 *  Start up the sps with the specified vault configuration in the 
 *  test case and verify that the correct drive tier is reported
 *
 * @param test_case_p - the bringup test case to run             
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_run_bringup_test_case(scrooge_mcduck_bringup_test_case_t *test_case_p)
{
    fbe_u32_t								slot = 0;
    fbe_status_t							status = FBE_STATUS_OK;
    fbe_sas_address_t						sas_address;
    fbe_api_terminator_device_handle_t		drive_handle_spa;
    fbe_api_terminator_device_handle_t		drive_handle_spb;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", test_case_p->description_p);

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    scrooge_mcduck_shutdown_sps();
    for (slot = 0; slot < SCROOGE_MCDUCK_TEST_NUM_VAULT; slot++)
    {
        if (test_case_p->vault_drives[slot].drive_type == SCROOGE_MCDUCK_INVALID_DRIVE)
        {
            scrooge_mcduck_pull_drive(0, 0, slot, &drive_handle_spa, &drive_handle_spb);
        }
        else
        {
            scrooge_mcduck_replace_drive_with_new_drive(0, 0, slot,
                                                        SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                        test_case_p->vault_drives[slot].drive_type,
                                                        &sas_address);
            test_case_p->vault_drives[slot].sas_address = sas_address;
        }
    }
    scrooge_mcduck_poweron_sps();
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, test_case_p->drive_tier);

    if (test_case_p->scrooge_subtest_fn_p != NULL)
    {
        test_case_p->scrooge_subtest_fn_p(test_case_p);
    }

}

/*!**************************************************************
 * scrooge_mcduck_run_bringup_test_cases()
 ****************************************************************
 * @brief
 *  Loop over the test cases for correct drive tier on sp start up
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_run_bringup_test_cases(fbe_test_rg_configuration_t *rg_config_p,
                                                  void *context)
{
    scrooge_mcduck_bringup_test_case_t *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;
    while (test_case_p->drive_tier != FBE_U16_MAX)
    {
        scrooge_mcduck_run_bringup_test_case(test_case_p);
        test_case_p++;
    }
}

/*!**************************************************************
 * scrooge_mcduck_run_vault_drive_replacement_subtest()
 ****************************************************************
 * @brief
 *  Replace a drive in the vault and check the lowest drive tier
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_run_vault_drive_replacement_subtest(void *context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           bus = 0;
    fbe_u32_t                           encl = 0;
    fbe_u32_t                           slot = 0;
    fbe_u32_t                           position = 0;
    fbe_test_rg_configuration_t         *rg_config_p = &scrooge_mcduck_rg_config[0];
    scrooge_mcduck_bringup_test_case_t  *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;
    fbe_sas_address_t                   sas_address;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (position = 0; position < rg_config_p->width; position++)
    {

        bus = rg_config_p->rg_disk_set[position].bus;
        encl = rg_config_p->rg_disk_set[position].enclosure;
        slot = rg_config_p->rg_disk_set[position].slot;

        status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K);
        mut_printf(MUT_LOG_TEST_STATUS, "Inserting brand new 15K drive\n");
        scrooge_mcduck_replace_drive_with_new_drive(bus, encl, slot,
                                                    SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                    SCROOGE_MCDUCK_15K_DRIVE,
                                                    &sas_address);
        test_case_p->vault_drives[slot].drive_type = SCROOGE_MCDUCK_15K_DRIVE;
        test_case_p->vault_drives[slot].sas_address = sas_address;

        fbe_api_sleep(2000); // allow database to process
        scrooge_mcduck_wait_for_object_state(object_id,
                                             FBE_LIFECYCLE_STATE_READY,
                                             FBE_PACKAGE_ID_SEP_0);

        scrooge_mcduck_wait_for_system_drives_rebuilt(position); 

    }

    /* Verify that the transition happens after all drives in the raid group are 15K 
     */
    scrooge_mcduck_verify_lowest_drive_tier(rg_object_id, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);

}

/*!**************************************************************
 * scrooge_mcduck_run_vault_kill_drive_15K_subtest()
 ****************************************************************
 * @brief
 *  After placing a 10K drive in a 15K drive vault, the 10K drive
 *  should get shot on both sps and again on a hard reboot.
 *
 * @param context - the test case with vault drives               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_vault_kill_drive_15K_subtest(void *context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               bus = 0;
    fbe_u32_t                               encl = 0;
    fbe_u32_t                               slot = 0;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t                       sas_address;
    scrooge_mcduck_bringup_test_case_t *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);
    original_target = fbe_api_sim_transport_get_target_server();

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* 1. raid group must start off as a 15K vault */
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);

    /* 2. replace a drive in the vault */
    mut_printf(MUT_LOG_TEST_STATUS, "Inserting brand new 10K drive\n");
    scrooge_mcduck_replace_drive_with_new_drive(bus, encl, slot,
                                                SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                SCROOGE_MCDUCK_10K_DRIVE,
                                                &sas_address);
    test_case_p->vault_drives[slot].drive_type = SCROOGE_MCDUCK_10K_DRIVE;
    test_case_p->vault_drives[slot].sas_address = sas_address;

    fbe_api_sleep(2000); // allow database to process

    /* 3. Verify that the drive got shot and event logs shows up */
    scrooge_mcduck_validate_drive_killed_message_present(bus, encl, slot);

    status = fbe_api_base_config_get_downstream_object_list(object_id,
                                                            &downstream_object_list);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(downstream_object_list.number_of_downstream_objects, 0);


    /* 4. Verify that the drive tier did not change */
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);


    /* 5. Hard reboot */
    fbe_test_base_suite_destroy_sp();
    fbe_test_base_suite_startup_post_powercycle();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    scrooge_mcduck_test_load_physical_config(test_case_p);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    scrooge_mcduck_test_load_physical_config(test_case_p);
    scrooge_mcduck_poweron_sps();

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* 6. Verify the drive gets shot again */
    scrooge_mcduck_validate_drive_killed_message_present(bus, encl, slot);

    /* 7. Verify the drive tier did not change */
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);

}

/*!**************************************************************
 * scrooge_mcduck_extended_cache_disabled_subtest()
 ****************************************************************
 * @brief
 *  After placing a 10K drive in a 15K drive vault, the 10K drive
 *  should get shot on both sps and again on a hard reboot.
 *
 * @param context - the test case with vault drives               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_extended_cache_disabled_subtest(void *context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               bus = 0;
    fbe_u32_t                               encl = 0;
    fbe_u32_t                               slot = 0;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_sim_transport_connection_target_t   original_target;

    fbe_sas_address_t                       sas_address;
    scrooge_mcduck_bringup_test_case_t *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);
    original_target = fbe_api_sim_transport_get_target_server();

    mut_printf(MUT_LOG_TEST_STATUS, "Disable the enabler\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_drive_tier_enabled(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_drive_tier_enabled(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K);

    mut_printf(MUT_LOG_TEST_STATUS, "Inserting brand new 10K drive without enabler\n");
    scrooge_mcduck_replace_drive_with_new_drive(bus, encl, slot,
                                                SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                SCROOGE_MCDUCK_10K_DRIVE,
                                                &sas_address);
    test_case_p->vault_drives[slot].drive_type = SCROOGE_MCDUCK_10K_DRIVE;
    test_case_p->vault_drives[slot].sas_address = sas_address;


    fbe_api_sleep(2000); // allow database to process
    scrooge_mcduck_wait_for_object_state(object_id,
                                         FBE_LIFECYCLE_STATE_READY,
                                         FBE_PACKAGE_ID_SEP_0);

    status = fbe_api_base_config_get_downstream_object_list(object_id,
                                                            &downstream_object_list);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(downstream_object_list.number_of_downstream_objects, 1);
    sep_rebuild_utils_wait_for_rb_to_start_by_obj_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, 0);
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K);

    mut_printf(MUT_LOG_TEST_STATUS, "Reenable the enabler\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_drive_tier_enabled(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_drive_tier_enabled(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}

/*!**************************************************************
 * scrooge_mcduck_degraded_vault_startup_subtest()
 ****************************************************************
 * @brief
 * Startup the SPs with only 3 drives in the vault
 *
 * @param context - the test case with vault drives               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_degraded_vault_startup_subtest(void *context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               bus = 0;
    fbe_u32_t                               encl = 0;
    fbe_u32_t                               slot = 3;
    fbe_api_terminator_device_handle_t      drive_handle_spa;
    fbe_api_terminator_device_handle_t      drive_handle_spb;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t						sas_address;
    scrooge_mcduck_drive_type_t				drive_type;

    scrooge_mcduck_bringup_test_case_t *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);

    original_target = fbe_api_sim_transport_get_target_server();

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, test_case_p->drive_tier);
    drive_type = test_case_p->vault_drives[slot].drive_type;


    /* Hard reboot */
    fbe_test_base_suite_destroy_sp();

    /* don't insert this drive on start up */
    test_case_p->vault_drives[slot].drive_type = SCROOGE_MCDUCK_INVALID_DRIVE;

    fbe_test_base_suite_startup_post_powercycle();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    scrooge_mcduck_test_load_physical_config(test_case_p);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    scrooge_mcduck_test_load_physical_config(test_case_p);

    scrooge_mcduck_poweron_sps();

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, test_case_p->drive_tier);

    fbe_api_sleep(2000);

    scrooge_mcduck_insert_blank_new_drive_extended(bus, encl, slot,
                                                   SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                   drive_type,
                                                   520,
                                                   &drive_handle_spa,
                                                   &drive_handle_spb,
                                                   &sas_address);
    fbe_api_sim_transport_set_target_server(original_target);
    fbe_api_sleep(2000);
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, test_case_p->drive_tier);

}

/*!**************************************************************
 * scrooge_mcduck_degraded_vault_in_specialize_test()
 ****************************************************************
 * @brief
 * Startup the SPs with only 3 drives in the vault
 *
 * @param context - the test case with vault drives               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_degraded_vault_in_specialize_subtest(void *context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               bus = 0;
    fbe_u32_t                               encl = 0;
    fbe_u32_t                               slot = 0;
    fbe_u32_t                               position = 3;
    fbe_api_terminator_device_handle_t      drive_handle_spa;
    fbe_api_terminator_device_handle_t      drive_handle_spb;
    fbe_sim_transport_connection_target_t   original_target;
    fbe_sas_address_t                       sas_address;
    fbe_sep_package_load_params_t           sep_params;
    fbe_neit_package_load_params_t          neit_params;

    scrooge_mcduck_bringup_test_case_t *test_case_p = (scrooge_mcduck_bringup_test_case_t *)context;

    mut_printf(MUT_LOG_TEST_STATUS, "Starting test case: %s\n", __FUNCTION__);

    original_target = fbe_api_sim_transport_get_target_server();

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Shutdown */
    //fbe_test_base_suite_destroy_sp();
    scrooge_mcduck_shutdown_sps();

    /* 1. Insert 4 drives in the vault */
    for (slot = 0; slot < SCROOGE_MCDUCK_TEST_NUM_VAULT; slot++)
    {
        scrooge_mcduck_replace_drive_with_new_drive(bus, encl, slot,
                                                    SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                    SCROOGE_MCDUCK_15K_DRIVE,
                                                    &sas_address);
        test_case_p->vault_drives[slot].drive_type = SCROOGE_MCDUCK_15K_DRIVE;
        test_case_p->vault_drives[slot].sas_address = sas_address;
        test_case_p->drive_tier = FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K;

    }

    /* load sep and neit with hook*/
    fbe_zero_memory(&sep_params, sizeof(fbe_sep_package_load_params_t));
    fbe_zero_memory(&neit_params, sizeof(fbe_neit_package_load_params_t));
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);
    mut_printf(MUT_LOG_TEST_STATUS, "Insert specialize hook for vault raid group");
    sep_params.scheduler_hooks[0].object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG;
    sep_params.scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY;
    sep_params.scheduler_hooks[0].monitor_substate = FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE;
    sep_params.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params.scheduler_hooks[0].val1 = NULL;
    sep_params.scheduler_hooks[0].val2 = NULL;
    /* and this will be our signal to stop */
    sep_params.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_sleep(2000); // wait a couple seconds before starting up again

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_esp_sep_and_neit_params(&sep_params, NULL);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_esp_sep_and_neit_params(&sep_params, NULL);
    fbe_api_sim_transport_set_target_server(original_target);


    status = fbe_test_sep_util_wait_for_database_service_ready(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check that the hook is hit */
    scrooge_mcduck_wait_for_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                                       FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE,
                                       SCHEDULER_CHECK_STATES,
                                       SCHEDULER_DEBUG_ACTION_PAUSE,
                                       NULL,
                                       NULL);

    /* remove one of the vault drives */
    scrooge_mcduck_pull_drive(bus, encl, position, &drive_handle_spa, &drive_handle_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "Delete the debug hook on both SPs\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_scheduler_del_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_scheduler_del_debug_hook(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(original_target);

    /* wait for the vault raid group to be ready*/
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the vault to become ready\n");
    scrooge_mcduck_wait_for_object_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG,
                                         FBE_LIFECYCLE_STATE_READY,
                                         FBE_PACKAGE_ID_SEP_0);

    /* check that the drive tier has been initialized as invalid */
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID);

    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_reinsert_drive(bus, encl, position,
                                             drive_handle_spb);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(bus, encl, position,
                                             drive_handle_spa);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    fbe_api_sim_transport_set_target_server(original_target);

    scrooge_mcduck_wait_for_system_drives_rebuilt(position); 
    scrooge_mcduck_verify_lowest_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, test_case_p->drive_tier);

}

/*!**************************************************************
 * scrooge_mcduck_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Load the physical config
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 ****************************************************************/
static void scrooge_mcduck_test_load_physical_config(scrooge_mcduck_bringup_test_case_t *test_case_p)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl0_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_sas_address_t                       new_sas_address;
    fbe_u32_t                               num_physical_objects = SCROOGE_MCDUCK_TEST_NUMBER_OF_PHYSICAL_OBJECTS;


    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */
                                         &port0_handle);

    /* Insert 1 enclosure to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0,
                                            0,
                                            port0_handle,
                                            &port0_encl0_handle);


    /* Insert the vault drives 
     */
    for (slot = 0; slot < SCROOGE_MCDUCK_TEST_NUM_VAULT; slot++)
    {
        if (test_case_p->vault_drives[slot].drive_type == SCROOGE_MCDUCK_INVALID_DRIVE)
        {
            num_physical_objects = num_physical_objects - 1;
        }
        else
        {

            if (test_case_p->vault_drives[slot].sas_address != 0)
            {
                new_sas_address = test_case_p->vault_drives[slot].sas_address;
            }
            else
            {
                new_sas_address = GET_SAS_DRIVE_ADDRESS(0, 0, slot);
                /* increment the unique sas address variable appropriately */
                fbe_test_pp_util_get_unique_sas_address();
                test_case_p->vault_drives[slot].sas_address = new_sas_address;
            }
            fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot,
                                                     520,
                                                     SCROOGE_MCDUCK_SYSTEM_DRIVE_CAPACITY,
                                                     new_sas_address,
                                                     test_case_p->vault_drives[slot].drive_type,
                                                     &drive_handle);
        }
    }

#if 0
    /* Insert just enough drives for the user raid group
     * Start off with 10K since the test case expects that
     */
    for (slot = SCROOGE_MCDUCK_TEST_NUM_VAULT; slot < SCROOGE_MCDUCK_TEST_NUM_VAULT + scrooge_mcduck_rg_config[0].width; slot++)
    {
        new_sas_address = GET_SAS_DRIVE_ADDRESS(0,0,slot);
        /* increment the unique sas address variable appropriately */
        fbe_test_pp_util_get_unique_sas_address();
        fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot,
                                                 520,
                                                 SCROOGE_MCDUCK_TEST_DRIVE_CAPACITY,
                                                 new_sas_address,
                                                 SCROOGE_MCDUCK_10K_DRIVE,
                                                 &drive_handle);
    }
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(num_physical_objects, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id(0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == num_physical_objects);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Use FBE Simulator PSL.");
    status = fbe_api_board_sim_set_psl(0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

}

/*!**************************************************************
  * scrooge_mcduck_test()
  ****************************************************************
  * @brief
  *   Run the test cases
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  2/06/2014 Created. Deanna Heng
  *
  ****************************************************************/
void scrooge_mcduck_test(void)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Wait for database ready */
    status = fbe_test_sep_util_wait_for_database_service(SCROOGE_MCDUCK_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(&scrooge_mcduck_rg_config[0],
                                                SCROOGE_MCDUCK_TEST_LUNS_PER_RAID_GROUP,
                                                SCROOGE_MCDUCK_CHUNKS_PER_LUN_TO_TEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(&scrooge_mcduck_rg_config[0], 1);

    status = fbe_api_database_set_drive_tier_enabled(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    scrooge_mcduck_run_bringup_test_cases(&scrooge_mcduck_rg_config[0], &scrooge_mcduck_bringup_test_cases[0]);
    //scrooge_mcduck_degraded_vault_in_specialize_subtest(&scrooge_mcduck_bringup_test_cases[0]);
    //scrooge_mcduck_test_10K_to_15K_transition(&scrooge_mcduck_rg_config[1]);
}

/*!**************************************************************
 * scrooge_mcduck_setup()
 ****************************************************************
 * @brief
 *  setup the simulation
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 *
 ****************************************************************/
void scrooge_mcduck_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Scrooge McDuck test ===\n");

    if (FBE_FALSE == fbe_test_util_is_simulation()) return;

    rg_config_p = &scrooge_mcduck_rg_config[0];
    fbe_test_sep_util_init_rg_configuration_array(&scrooge_mcduck_rg_config[0]);
    fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
    scrooge_mcduck_rg_config[0].b_use_fixed_disks = FBE_TRUE;

    fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Load the physical configuration */
    scrooge_mcduck_test_load_physical_config(&scrooge_mcduck_bringup_test_cases[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load Physical Package for SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    scrooge_mcduck_test_load_physical_config(&scrooge_mcduck_bringup_test_cases[0]);

    /* Load sep and neit packages */
    sep_config_load_esp_sep_and_neit_both_sps();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    //fbe_api_control_automatic_hot_spare(FBE_FALSE);
    //fbe_api_base_config_disable_all_bg_services();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    //fbe_api_control_automatic_hot_spare(FBE_FALSE);
    //fbe_api_base_config_disable_all_bg_services();

    return;
}


/*!**************************************************************
 * scrooge_mcduck_setup()
 ****************************************************************
 * @brief
 *  tear down
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/29/2014 - Created. Deanna Heng
 *
 ****************************************************************/
void scrooge_mcduck_cleanup(void)
{
    if (FBE_FALSE == fbe_test_util_is_simulation()) return;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for SPB ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();
}
