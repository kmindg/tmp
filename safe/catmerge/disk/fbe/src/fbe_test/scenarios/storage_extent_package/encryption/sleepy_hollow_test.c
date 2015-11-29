/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    sleepy_hollow.c
 ***************************************************************************
 *
 * @brief   This file test encryption with I/O to the vault raid group.
 *          Currently the vault raid group is the only private (system)
 *          raid group that is encrypted.  If other private groups are
 *          encrypted they should be added below.  
 *
 * @author
 *  01/14/2014  Ron Proulx  - Created. 
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_utils.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"                
#include "fbe_private_space_layout.h"               
#include "sep_rebuild_utils.h"    
#include "sep_test_background_ops.h"                  
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_lun.h"
#include "fbe_test.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "neit_utils.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* sleepy_hollow_short_desc = "Test encryption with I/O on system(vault) raid group";
char* sleepy_hollow_long_desc =
    "\n"
    "\n"
    "This test boots without encryption then enables encryption.\n"
    "\n"
    "STEP 1: Boot system without encryption and write background pattern\n"
    "\n"
    "STEP 2: Enable encryption for the system\n"
    "\n"
    "STEP 3: Validate that encryption completes on the vault\n"
    "\n"
    "STEP 4: Validate that the vault has been written with zeros\n" 
    "\n"
    "\n"
    
"\n"
"Desription last updated:   01/15/2014\n"    ;

/*!*******************************************************************
 * @def     SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief   Currently this test only supports (1) LUN per raid group.
 *          In the case of the vault there is only 1 LUN.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP  1

/*!*******************************************************************
 * @def     SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST
 *********************************************************************
 * @brief   This is the number of physical chunks on the vault to
 *          test.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST    3

/*!*******************************************************************
 * @def     SLEEPY_HOLLOW_RANDOM_IO_SIZE
 *********************************************************************
 * @brief   Considering the size of the private LUNs (large) each I/O
 *          spread across the LUN should be small.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_RANDOM_IO_SIZE            128

/*!*******************************************************************
 * @def SLEEPY_HOLLOW_THREAD_COUNT
 *********************************************************************
 * @brief Lightly loaded test to allow rekey to proceed.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_THREAD_COUNT              1

/*!*******************************************************************
 * @def     SLEEPY_HOLLOW_NUM_RAID_GROUPS_TO_TEST
 *********************************************************************
 * @brief   Current number of raid groups in array
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_NUM_RAID_GROUPS_TO_TEST   1

/*!*******************************************************************
 * @def     SLEEPY_HOLLOW_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_FIXED_PATTERN     FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def SLEEPY_HOLLOW_BLOCK_COUNT
 *********************************************************************
 * @brief Total count of blocks to check/set with rdgen.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_BLOCK_COUNT 0x10000

/*!*******************************************************************
 * @def SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS
 *********************************************************************
 * @brief Time we will have vault wait after I/O before encryption starts.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS 10000


/*!*******************************************************************
 * @var sleepy_hollow_raid_group_config_qual
 *********************************************************************
 * @brief   This is the array of configurations we will loop over.
 *
 * @note    Currently only the vault LUN will be encrypted (because it
 *          contains user data).
 *
 * @note    The capacity of the LUN is huge.
 *
 *********************************************************************/
fbe_test_rg_configuration_t sleepy_hollow_raid_group_config_qual[] = 
{
    /*! @note Although all fields must be valid only the system id is used to populate the test config+
     *                                                                                                |
     *                                                                                                v    */
    /*width capacity    raid type                   class               block size  system id                               bandwidth.*/
    {5,     0xF004800,  FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,       FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,   0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @def SLEEPY_HOLLOW_NUM_OF_RDGEN_CONTEXTS
 *********************************************************************
 * @brief Number of test contexts to create and test against.
 *
 *********************************************************************/
#define SLEEPY_HOLLOW_NUM_OF_RDGEN_CONTEXTS (SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP * SLEEPY_HOLLOW_NUM_RAID_GROUPS_TO_TEST)

/***************************************** 
 * FORWARD FUNCTION DECLARATIONS
 *****************************************/

/*!**************************************************************
 * sleepy_hollow_validate_verify_status()
 ****************************************************************
 * @brief
 *  Ensure there were no errors in verify.
 *
 * @param object_id -               
 *
 * @return None. 
 *
 * @author
 *  1/21/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_validate_verify_status(fbe_object_id_t object_id)
{
    fbe_lun_verify_report_t verify_report;
    fbe_u32_t error_count;

    fbe_test_sep_util_lun_get_verify_report(object_id, &verify_report);

    fbe_test_sep_util_sum_verify_results_data(&verify_report.current, &error_count);

    if (error_count != 0) {
        mut_printf(MUT_LOG_TEST_STATUS, "count of %d errors found", error_count);
        MUT_FAIL();
    }
}
/******************************************
 * end sleepy_hollow_validate_verify_status()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_vault_encryption_test()
 ****************************************************************
 * @brief
 *  Test that the vault properly handles encryption.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_encryption_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t end_lba;
    fbe_object_id_t rg_object_id;
    fbe_database_lun_info_t lun_info;

    mut_printf(MUT_LOG_TEST_STATUS, "== starting %s ==", __FUNCTION__);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*! @note Need to validate that this doesn't take a long time !!*/
    //mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for zeroing to complete...", __FUNCTION__);
    //sleepy_hollow_wait_for_zeroing(rg_config_p);
    //mut_printf(MUT_LOG_TEST_STATUS, "%s Zeroing complete...", __FUNCTION__);

    /* Write a background pattern
     */
    //sleepy_hollow_write_and_verify_background_pattern(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "Seed the vault with data.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lun_info.lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    end_lba = lun_info.capacity - SLEEPY_HOLLOW_BLOCK_COUNT;
    /* Validate that all the data is now zeros*/
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS completed; status: 0x%x. ==", status);


    /* The vault should automatically be encrypted when system encryption is enabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Vault Encryption to complete. ==");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption Rekey. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "== Encryption rekey completed successfully. ==");

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate encryption waited %d msec before starting ==",
               SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Kick off verify to validate there are no errors at beginning. ==");
    fbe_test_sep_util_lun_clear_verify_report(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);
    status = fbe_api_lun_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_USER_RW,
                                         FBE_FALSE, /* entire LUN */
                                         0, SLEEPY_HOLLOW_BLOCK_COUNT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for verify to finish ==");
    status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sleepy_hollow_validate_verify_status(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);

    mut_printf(MUT_LOG_TEST_STATUS, "== Kick off verify to validate there are no errors at end. ==");
    fbe_test_sep_util_lun_clear_verify_report(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);
    status = fbe_api_lun_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_USER_RW,
                                         FBE_FALSE, /* entire LUN */
                                         (lun_info.capacity / rg_info.num_data_disk) - SLEEPY_HOLLOW_BLOCK_COUNT,
                                         SLEEPY_HOLLOW_BLOCK_COUNT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for verify to finish ==");
    status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sleepy_hollow_validate_verify_status(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate that data was zeroed on Vault. == ");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");

    mut_printf(MUT_LOG_TEST_STATUS, "== Seed the vault with data. This is so we do ZOD and put real data on disk. ==");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate vault has encrypted data. ==");

    fbe_test_rg_validate_lbas_encrypted(rg_config_p, 
                                        0,/* lba */ 
                                        64, /* Blocks to check */ 
                                        64 /* Blocks encrypted */);
/*    
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "== Encryption disable status %d ==", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    mut_printf(MUT_LOG_TEST_STATUS, "== complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end sleepy_hollow_vault_encryption_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_vault_encryption_zero_test()
 ****************************************************************
 * @brief
 *  Test that after encryption if we let the PVD finish zeroing, 
 *  then the vault is properly zeroed.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/19/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_encryption_zero_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_id;
    fbe_database_lun_info_t lun_info;

    mut_printf(MUT_LOG_TEST_STATUS, "== starting %s ==", __FUNCTION__);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for zeroing to complete...", __FUNCTION__);
    fbe_test_rg_wait_for_zeroing(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Zeroing complete...", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "Seed the vault with data.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS completed; status: 0x%x. ==", status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for zeroing (from encryption) to complete...", __FUNCTION__);
    fbe_test_rg_wait_for_zeroing(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Zeroing complete...", __FUNCTION__);

    /* The vault should automatically be encrypted when system encryption is enabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Vault Encryption to complete. ==");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption Rekey. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "== Encryption rekey completed successfully. ==");
    
    lun_info.lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Kick off verify to validate there are no errors at beginning. ==");
    fbe_test_sep_util_lun_clear_verify_report(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);
    status = fbe_api_lun_initiate_verify(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, 
                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                         FBE_LUN_VERIFY_TYPE_USER_RW,
                                         FBE_FALSE, /* entire LUN */
                                         0, (lun_info.capacity / rg_info.num_data_disk));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for verify to finish ==");
    status = fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, FBE_TEST_WAIT_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sleepy_hollow_validate_verify_status(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate that data was zeroed on Vault. == ");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");

    fbe_test_rg_validate_lbas_encrypted(rg_config_p, 
                                        0,/* lba */ 
                                        64, /* Blocks to check */ 
                                        64 /* Blocks encrypted */);
/*    
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "== Encryption disable status %d ==", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    mut_printf(MUT_LOG_TEST_STATUS, "== complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end sleepy_hollow_vault_encryption_zero_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_vault_delayed_encryption_test()
 ****************************************************************
 * @brief
 *  Test that the vault does not start encryption if I/O is in progress.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/21/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_delayed_encryption_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t end_lba;
    fbe_object_id_t rg_object_id;
    fbe_database_lun_info_t lun_info;
    fbe_time_t start_time;
    fbe_u32_t elapsed_ms;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_u32_t num_luns;

    mut_printf(MUT_LOG_TEST_STATUS, "== starting %s ==", __FUNCTION__);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Seed the vault with data.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lun_info.lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    end_lba = lun_info.capacity - SLEEPY_HOLLOW_BLOCK_COUNT;
    /* Validate that all the data is now zeros*/
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Start I/O before starting encryption.");
 
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, 0x1000, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_RDGEN_PEER_OPTIONS_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_use_rg_hooks(rg_config_p,
                                   FBE_U32_MAX,    /* No DS objects */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Stop I/O to vault ==");

    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);

    mut_printf(MUT_LOG_TEST_STATUS, "== Run more I/O to reset the counter again== ");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    start_time = fbe_get_time();

    mut_printf(MUT_LOG_TEST_STATUS, "== Run more I/O to reset the counter == ");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure Vault encryption did not start == ");
    status = fbe_test_use_rg_hooks(rg_config_p,
                                   FBE_U32_MAX,    /* No DS objects */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Vault encryption to start ==");
    status = fbe_test_use_rg_hooks(rg_config_p,
                                   FBE_U32_MAX,    /* No DS objects */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_WAIT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate encryption waited %d msec before starting ",
               SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    elapsed_ms = fbe_get_elapsed_milliseconds(start_time);
    if (elapsed_ms < SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS) {
        mut_printf(MUT_LOG_TEST_STATUS, "Elapsed ms %d < expected elapsed %d",
                   elapsed_ms, SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
        MUT_FAIL();
    }

    status = fbe_test_use_rg_hooks(rg_config_p,
                                   FBE_U32_MAX,    /* No DS objects */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                   FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_DELETE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* The vault should automatically be encrypted when system encryption is enabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Vault Encryption to complete.");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption Rekey. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey completed successfully.");

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate encryption waited before starting ");
    
    mut_printf(MUT_LOG_TEST_STATUS, "Validate that data was zeroed on Vault.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    mut_printf(MUT_LOG_TEST_STATUS, "== complete %s ==", __FUNCTION__);

    return;
}
/******************************************
 * end sleepy_hollow_vault_delayed_encryption_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_encryption_vault_in_use_test()
 ****************************************************************
 * @brief
 *  Test that the vault properly handles when the cache is still
 *  using the vault.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_encryption_vault_in_use_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t end_lba;
    fbe_object_id_t rg_object_id;
    fbe_persistent_memory_set_params_t params;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_database_lun_info_t lun_info;
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== starting %s ==", __FUNCTION__);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Seed the vault with data.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lun_info.lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    end_lba = lun_info.capacity - SLEEPY_HOLLOW_BLOCK_COUNT;
    /* Validate that all the data is now zeros*/
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Mark vault as in use so encryption does not run ==");
    fbe_api_persistent_memory_init_params(&params);
    params.b_vault_busy = FBE_TRUE;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure the encryption does not run. ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_BUSY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_BUSY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_BUSY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Mark vault as not in use, so encryption runs. == ");
    fbe_api_persistent_memory_init_params(&params);
    params.b_vault_busy = FBE_FALSE;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* The vault should automatically be encrypted when system encryption is enabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for vault automatic Encryption ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey completed successfully.");

    mut_printf(MUT_LOG_TEST_STATUS, "Validate that data was zeroed on Vault.");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, SLEEPY_HOLLOW_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of automatic encryption failed\n");

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    mut_printf(MUT_LOG_TEST_STATUS, "== complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end sleepy_hollow_encryption_vault_in_use_test()
 ******************************************/
void sleepy_hollow_restart_sp(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_bool_t b_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    if (b_dualsp) {
    } else {
        mut_printf(MUT_LOG_LOW, " == Shutdown target SP SP A == ");
        fbe_api_sim_transport_destroy_client(FBE_SIM_SP_A);
        fbe_test_sp_sim_stop_single_sp(FBE_TEST_SPA);
        fbe_test_boot_sp(rg_config_p, FBE_SIM_SP_A);
        sep_config_load_kms(NULL); 
    }
    fbe_test_sep_reset_encryption_enabled();
}
/*!**************************************************************
 * sleepy_hollow_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 5 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                    status;

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    fbe_test_set_rg_vault_encrypt_wait_time(SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    
    sleepy_hollow_vault_encryption_test(rg_config_p);

    fbe_test_reset_rg_vault_encrypt_wait_time();
    return;
}
/******************************************
 * end sleepy_hollow_test_rg_config()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_delayed_encryption_test_rg()
 ****************************************************************
 * @brief
 *  Run a vault encryption test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_delayed_encryption_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                    status;

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    fbe_test_set_rg_vault_encrypt_wait_time(SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    sleepy_hollow_vault_delayed_encryption_test(rg_config_p);

    fbe_test_reset_rg_vault_encrypt_wait_time();
    return;
}
/******************************************
 * end sleepy_hollow_delayed_encryption_test_rg()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_vault_in_use_test_rg()
 ****************************************************************
 * @brief
 *  Run a vault encryption test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_in_use_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                    status;

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    fbe_test_set_rg_vault_encrypt_wait_time(SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    sleepy_hollow_encryption_vault_in_use_test(rg_config_p);

    fbe_test_reset_rg_vault_encrypt_wait_time();
    return;
}
/******************************************
 * end sleepy_hollow_vault_in_use_test_rg()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_vault_zero_test_rg()
 ****************************************************************
 * @brief
 *  Run a vault encryption zeroing test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_zero_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                    status;

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    fbe_test_set_rg_vault_encrypt_wait_time(SLEEPY_HOLLOW_VAULT_ENCRYPT_WAIT_TIME_MS);
    sleepy_hollow_vault_encryption_zero_test(rg_config_p);

    fbe_test_reset_rg_vault_encrypt_wait_time();
    return;
}
/******************************************
 * end sleepy_hollow_vault_zero_test_rg()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_test()
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
 *  01/15/2014  Ron Proulx  - Created from ichabod_crane_test.
 *
 ****************************************************************/

void sleepy_hollow_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];
    
    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, sleepy_hollow_test_rg_config,
                                                         SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                         SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                         FBE_FALSE /* don't save config destroy config */);
    params.b_encrypted = FBE_FALSE;
    params.b_skip_create = FBE_TRUE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end sleepy_hollow_test()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_vault_in_use_test()
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
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_in_use_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, sleepy_hollow_vault_in_use_test_rg,
                                                         SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                         SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                         FBE_FALSE /* don't save config destroy config */);
    params.b_encrypted = FBE_FALSE;
    params.b_skip_create = FBE_TRUE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end sleepy_hollow_vault_in_use_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_vault_zero_test()
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
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_zero_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sleepy_hollow_vault_zero_test_rg(rg_config_p, NULL);
    return;
}
/******************************************
 * end sleepy_hollow_vault_zero_test()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_vault_delay_encryption_test()
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
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_delay_encryption_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, sleepy_hollow_delayed_encryption_test_rg,
                                                         SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                         SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                         FBE_FALSE /* don't save config destroy config */);
    params.b_encrypted = FBE_FALSE;
    params.b_skip_create = FBE_TRUE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end sleepy_hollow_vault_delay_encryption_test()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *   *  01/15/2014  Ron Proulx  - Created from ichabod_test.
 *
 ****************************************************************/

void sleepy_hollow_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, sleepy_hollow_test_rg_config,
                                                             SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                             SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                             FBE_FALSE);
        params.b_encrypted = FBE_FALSE;
        params.b_skip_create = FBE_TRUE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end sleepy_hollow_dualsp_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_delayed_encryption_dualsp_test()
 ****************************************************************
 * @brief
 *  Vault delayed in starting encryption test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *   2/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_delayed_encryption_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;
    
    rg_config_p = &sleepy_hollow_raid_group_config_qual[0]; 

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, sleepy_hollow_delayed_encryption_test_rg,
                                                             SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                             SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                             FBE_FALSE);
        params.b_encrypted = FBE_FALSE;
        params.b_skip_create = FBE_TRUE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end sleepy_hollow_delayed_encryption_dualsp_test()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_vault_in_use_dualsp_test()
 ****************************************************************
 * @brief
 *  Vault already in use when encryption is started.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *   2/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_vault_in_use_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;
    
    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, sleepy_hollow_vault_in_use_test_rg,
                                                             SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                             SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                             FBE_FALSE);
        params.b_encrypted = FBE_FALSE;
        params.b_skip_create = FBE_TRUE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end sleepy_hollow_vault_in_use_dualsp_test()
 ******************************************/
/*!**************************************************************
 * sleepy_hollow_vault_zero_dualsp_test()
 ****************************************************************
 * @brief
 *  Vault already in use when encryption is started.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 ****************************************************************/

void sleepy_hollow_vault_zero_dualsp_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &sleepy_hollow_raid_group_config_qual[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sleepy_hollow_vault_zero_test_rg(rg_config_p, NULL);
#if 0
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, sleepy_hollow_vault_in_use_test_rg,
                                                             SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP,
                                                             SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST,
                                                             FBE_FALSE);
        params.b_encrypted = FBE_FALSE;
        params.b_skip_create = FBE_TRUE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
#endif
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end sleepy_hollow_vault_zero_dualsp_test()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void sleepy_hollow_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &sleepy_hollow_raid_group_config_qual[0]; //extended[0];
        }
        else
        {
            rg_config_p = &sleepy_hollow_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup.
         * We are using the system drives so do NOT randomize.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        /* This test uses the existing system drives but it is similar to
         * use the common load method (and not use the drives).
         */
        elmo_load_config(rg_config_p, 
                         SLEEPY_HOLLOW_TEST_LUNS_PER_RAID_GROUP, 
                         SLEEPY_HOLLOW_CHUNKS_PER_LUN_TO_TEST);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    fbe_test_disable_background_ops_system_drives();

    /* load KMS */
    sep_config_load_kms(NULL);
    
    return;
}
/**************************************
 * end sleepy_hollow_setup()
 **************************************/

void sleepy_hollow_setup_sim_psl(void)
{
    /* Use simulation psl so zeroing on system drives completes.
     */
    fbe_test_set_use_fbe_sim_psl(FBE_TRUE);
    sleepy_hollow_setup();
}
/*!**************************************************************
 * sleepy_hollow_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void sleepy_hollow_dualsp_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &sleepy_hollow_raid_group_config_qual[0]; //extended[0];
        }
        else
        {
            rg_config_p = &sleepy_hollow_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

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

    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_disable_background_ops_system_drives();

    /* Load KMS */        
    sep_config_load_kms_both_sps(NULL);

    return;
}
/**************************************
 * end sleepy_hollow_dualsp_setup()
 **************************************/
void sleepy_hollow_dualsp_setup_sim_psl(void)
{
    /* Use simulation psl so zeroing on system drives completes.
     */
    fbe_test_set_use_fbe_sim_psl(FBE_TRUE);
    sleepy_hollow_dualsp_setup();
}
/*!**************************************************************
 * sleepy_hollow_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the sleepy_hollow test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_cleanup(void)
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
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end sleepy_hollow_cleanup()
 ******************************************/

/*!**************************************************************
 * sleepy_hollow_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the sleepy_hollow test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void sleepy_hollow_dualsp_cleanup(void)
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
        fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end sleepy_hollow_dualsp_cleanup()
 ******************************************/

/*********************************
 * end file sleepy_hollow_test.c
 *********************************/

