/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file diesel_test.c
 ***************************************************************************
 *
 * @brief   This scenario test enable/disable background operation functionality.
 *
 * @version
 *  08/22/2011  Amit Dhaduk  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * diesel_short_desc = "Enable/Disable Background Operations";
char * diesel_long_desc = "\
The Diesel scenario focuses on performing and verifying the enable/disable functionality for all \n\
Background Operations as well as individual Background Operation (Metadata Rebuild, Rebuild, \n\
Error Verify, Read/Write Verify, Read-only Verify, Disk Zeroing, and Sniff Verify).\n\
The enable/disable functionality of the Background Operations will be tested with the RAID Group and \n\
Provision Drive Object IDs. This test runs on in simulation environment.\n"\
"\n"\
"-raid_test_level 0 \n\
\n\
STEP 1.0: Configure all raid groups and LUNs.\n\
        - Tests cover 520 block sizes.\n\
        - Tests cover one Raid-5 RG and one LUN per RG.\n\
\n\
STEP 2.0: Enable/Disable Background Operations for RAID Group Object ID.\n\
        - Get RG object ID \n\
        - Verify that ENABLE is set by default for the following Background Operations:  \n\
            Rebuild, Metadata Rebuild, Error Verify, Read/Write Verify, and Read-only Verify.\n\
        - Disable the following RAID Group Background Operations and verify that they are disable:\n\
            All, Rebuild, Metadata Rebuild, Error Verify, Read/Write Verify, and Read-only Verify.\n\
        - Enable the following RAID Group Background Operations and verify that they are enable: \n\
            All, Rebuild, Metadata Rebuild, Error Verify, Read/Write Verify, and Read-only Verify.\n\
\n\
STEP 2.1: Enable/Disable Background Operations for Provision Object ID.\n\
        - Get Provision Drive object ID \n\
        - Verify that ENABLE is set by default for the Sniff and Disk Zeroing Background Operations. \n\
        - Disable the following RAID Group Background Operations and verify that they are disable:\n\
            All, Sniff, and Disk Zeroing.\n\
        - Enable the following RAID Group Background Operations and verify that they are enable: \n\
            All, Sniff, and Disk Zeroing.\n\
\n\
STEP 3.0: Enable/Disable Metadata Rebuild for RAID Group Object ID.\n\
        - Get RG object ID. \n\
        - Get VD object ID. \n\
        - Disable Metadata Rebuild.\n\
        - Remove a drive from Postion 1. \n\
        - Wait for VD object to transition in fail state. \n\
        - Configure extra Hot Spare. \n\
        - Wait for the Hot Spare drive to swap in. \n\
        - Verify that Metadata Rebuild is disabled. \n\
        - Verify that no progress being made for Rebuild checkpoint. \n\
        - Enable Metadata Rebuild.\n\
        - Wait for Rebuild to complete. \n\
\n\
STEP 4.0: Enable/Disable Rebuild for RAID Group Object ID.\n\
        - Get RG object ID. \n\
        - Get VD object ID. \n\
        - Disable Rebuild.\n\
        - Remove a drive from Postion 0. \n\
        - Wait for VD object to transition in fail state. \n\
        - Insert the pull drive back in. \n\
        - Wait for VD object to transition in ready state. \n\
        - Verify that Rebuild is disabled. \n\
        - Verify that no progress being made for Rebuild checkpoint. \n\
        - Enable Rebuild.\n\
        - Wait for Rebuild to complete. \n\
\n\
STEP 5.0: Enable/Disable Error Verify for RAID Group Object ID.\n\
        - Get RG object ID. \n\
        - Disable Error Verify.\n\
        - Send a user initiated request to start Error Verify. \n\
        - Wait for some times. \n\
        - Confirm Error Verify is disabled. \n\
        - Verify that no progress being made for Error Verify checkpoint. \n\
        - Enable Error Verify.\n\
        - Wait for Error Verify to complete. \n\
\n\
STEP 5.1: Enable/Disable Read/Write Verify for RAID Group Object ID.\n\
        - Get RG object ID. \n\
        - Disable Read/Write Verify.\n\
        - Send a user initiated request to start Read/Write Verify. \n\
        - Wait for some times. \n\
        - Confirm Read/Write Verify is disabled. \n\
        - Verify that no progress being made for Read/Write Verify checkpoint. \n\
        - Enable Read/Write Verify.\n\
        - Wait for Read/Write Verify to complete. \n\
\n\
STEP 5.2: Enable/Disable Read-only Verify for RAID Group Object ID.\n\
        - Get RG object ID. \n\
        - Disable Read-only Verify.\n\
        - Send a user initiated request to start Read-only Verify. \n\
        - Wait for some times. \n\
        - Confirm Read-only Verify is disabled. \n\
        - Verify that no progress being made for Read-only Verify checkpoint. \n\
        - Enable Read-only Verify.\n\
        - Wait for Read-only Verify to complete. \n\
\n\
STEP 6.0: Enable/Disable Sniff for Provision Drive Object ID.\n\
        - Get PVD object ID for the first drive. \n\
        - Set the Sniff checkpoint at 0. \n\
        - Clear all Sniff reports. \n\
        - Get Sniff report. \n\
        - Disable Sniff Verify.\n\
        - Start Sniff. \n\
        - Wait for some times. \n\
        - Confirm Sniff is disabled. \n\
        - Get Sniff status. \n\
        - Verify that no progress being made for Sniff Verify checkpoint. \n\
        - Enable Sniff Verify.\n\
        - Wait for Sniff Verify to complete. \n\
        - Disable verify on this provision drive. \n\
\n\
STEP 7.0: Enable/Disable Disk Zeroing for Provision Drive Object ID.\n\
        - Get PVD object ID for the first drive. \n\
        - Disable Disk Zeroing.\n\
        - Set the Disk Zeroing checkpoint at 0. \n\
        - Verify that it is set to 0. \n\
        - Wait for some times. \n\
        - Confirm Disk Zeroing is disabled. \n\
        - Verify that no progress being made for Disk Zeroing checkpoint. \n\
        - Enable Disk Zeroing.\n\
        - Wait for Disk Zeroing to complete. \n\
\n\
Description last updated: 10/05/2011.\n";


/*!*******************************************************************
 * @def DIESEL_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing complete
 *
 *********************************************************************/
#define DIESEL_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def DIESEL_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define DIESEL_TEST_RAID_GROUP_COUNT 1

#define DIESEL_TEST_LUNS_PER_RAID_GROUP  2

#define DIESEL_TEST_CHUNKS_PER_LUN  5

/*!*******************************************************************
 * @def DIESEL_MAX_WAIT_TIME_SECS
 *********************************************************************
 * @brief Maximum time in seconds to wait for a verify operation to complete.
 *        At present there are about 75 chunks to verify so it takes at least
 *        75 seconds to complete the sniff.
 *********************************************************************/
#define DIESEL_MAX_WAIT_TIME_SECS  120

/*!*******************************************************************
 * @def DIESEL_MAX_WAIT_TIME_MSECS
 *********************************************************************
 * @brief Maximum time in mili seconds to wait for a verify operation to complete.
 *********************************************************************/
#define DIESEL_MAX_WAIT_TIME_MSECS  DIESEL_MAX_WAIT_TIME_SECS * 1000

/*!*******************************************************************
 * @var diesel_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t diesel_rg_configuration[] = 
{
     /* width,  capacity          raid type,                  class,        block size  RAID-id.  bandwidth.*/
    {3,         0xE000,  FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,        0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
void diesel_run_tests(fbe_test_rg_configuration_t *rg_config_p ,void * context_p);
void diesel_background_operation_rebuild_test(fbe_test_rg_configuration_t *rg_config_p);
void diesel_background_operation_metadata_rebuild_test(fbe_test_rg_configuration_t *rg_config_p);
void diesel_background_operation_raid_group_verify_test(fbe_test_rg_configuration_t *rg_config_p);
void diesel_wait_for_raid_group_verify_completion(fbe_object_id_t rg_object_id, fbe_lun_verify_type_t verify_type);
void diesel_background_operation_sniff_test(fbe_test_rg_configuration_t *rg_config_p);
void diesel_wait_for_sniff_completion(fbe_object_id_t in_pvd_object_id,fbe_u32_t       in_timeout_sec );
void diesel_background_operation_general_test(fbe_test_rg_configuration_t *rg_config_p);
void diesel_background_operation_disk_zeroing_test(fbe_test_rg_configuration_t *rg_config_p);



/*!****************************************************************************
 *  diesel_run_tests
 ******************************************************************************
 *
 * @brief
 *   Run background operation enable/disable tests.
 *
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p   - context for the given test.
 *
 * @author
 *  08/22/2011 - Created. Amit Dhaduk
 *
 *****************************************************************************/
void diesel_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    mut_printf(MUT_LOG_LOW, "=== Diesel test start - test enable/disable background operations ===\n");	

    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;
        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            diesel_background_operation_general_test(rg_config_p);

            diesel_background_operation_metadata_rebuild_test(rg_config_p );
            
            diesel_background_operation_rebuild_test(rg_config_p );

            diesel_background_operation_raid_group_verify_test(rg_config_p);

            diesel_background_operation_disk_zeroing_test(rg_config_p);
            
            diesel_background_operation_sniff_test(rg_config_p);

        }
    }
    mut_printf(MUT_LOG_LOW, "=== Diesel test complete ===\n");	
    return;
}
/******************************************
 * end diesel_run_tests()
 ******************************************/


/*!**************************************************************
 * diesel_background_operation_general_test()
 ****************************************************************
 * @brief
 *  This function tests general enable/disable background operations
 *  functionality.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_general_test(fbe_test_rg_configuration_t *rg_config_p)
{

    fbe_object_id_t     rg_object_id;
    fbe_object_id_t     pvd_object_id;
    fbe_status_t        status;
    fbe_bool_t          is_enabled;
    fbe_object_id_t     vd_object_id;
    fbe_u32_t           rg_position_to_test = 1;
   
    /* get the raid group object id */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get provision drive object id for first drive */
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &pvd_object_id);

    mut_printf(MUT_LOG_LOW, "=== enable/disable background operation general test start ===\n");	

    /* Raid group object - background operation test */

    /* check that by default following background operations are enabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);


    /* test enable/disable all background operations in raid group object */
    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ALL, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ALL, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    /* test enable/disable individual background operations in raid group object */
    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    fbe_api_base_config_disable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(rg_object_id , FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY);
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    /* provision drive object - background operation test */

    /* check that by default sniff background operation is enabled */
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    /* check that by default background zeroing operations is enabled */
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &is_enabled);

    /*!@todo Need to uncomment following test code once BGZ is enabled by default in the tree */
    // MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);


    /* test enable/disable all background operations in provision drive object */
    fbe_api_base_config_disable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    /* test enable/disable individual background operation in provision drive object */
    fbe_api_base_config_disable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);

    fbe_api_base_config_enable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_disable_background_operation(pvd_object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);

    /*********************
     * Virutal Drive Tests
     *********************/
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, rg_position_to_test, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    fbe_api_base_config_enable_background_operation(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_disable_background_operation(vd_object_id , FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD);
    fbe_api_base_config_is_background_operation_enabled(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_METADATA_REBUILD, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);
    fbe_api_base_config_enable_background_operation(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY);
    fbe_api_base_config_is_background_operation_enabled(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, is_enabled);
    fbe_api_base_config_disable_background_operation(vd_object_id , FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY);
    fbe_api_base_config_is_background_operation_enabled(vd_object_id, FBE_VIRTUAL_DRIVE_BACKGROUND_OP_COPY, &is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, is_enabled);

    mut_printf(MUT_LOG_LOW, "=== enable/disable background operation general test complete ===\n");	

    return;
}


/*!**************************************************************
 * diesel_background_operation_metadata_rebuild_test()
 ****************************************************************
 * @brief
 *  This function tests enable/disable metadata rebuild 
 *  background operation.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 * 
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_metadata_rebuild_test(fbe_test_rg_configuration_t *rg_config_p)
{

    
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_object_id_t     rg_object_id;
    fbe_object_id_t     vd_object_id;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_status_t    status;
    fbe_bool_t      b_is_enabled;
    fbe_u32_t       position_to_remove = 1;
    
   
    mut_printf(MUT_LOG_LOW, "=== Metadata Rebuild background operation test started ===\n");	
    
    /* get the raid group object id */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* disable the given background operation */
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);

    /* remove the drive */
    fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position_to_remove].bus, 
        rg_config_p->rg_disk_set[position_to_remove].enclosure,
        rg_config_p->rg_disk_set[position_to_remove].slot, &drive_handle);

    /* wait for VD object to transition in fail state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                          DIESEL_MAX_WAIT_TIME_MSECS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* configure extra drive as hot spare */
    status = fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus,
                                rg_config_p->extra_disk_set[0].enclosure,
                                rg_config_p->extra_disk_set[0].slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for spare drive swap in */
    fbe_test_sep_drive_wait_permanent_spare_swap_in(rg_config_p, position_to_remove);

    /* wait for some time */
    fbe_api_sleep(10000);

    /* confirm that metadata rebuild background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);

    /* get the rebuild checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* metadata rebuild operation is disabled and becasue of that
     * rebuild also can not make progress so checking that with rebuild checkpoint.  
     * There is no any other mechanisam to verify progress of metadata rebuild. */
    if(raid_group_info.rebuild_checkpoint[position_to_remove] != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable metadata rebuild background operation");
    }

    /* enable metadata rebuild background operation */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD);

    /* wait for rebuild operation to complete */
    fbe_test_sep_util_wait_for_raid_group_to_rebuild(rg_object_id);

    mut_printf(MUT_LOG_LOW, "=== Metadata Rebuild background operation test completed ===\n");	

    return;
}
/**********************************************************
 * end diesel_background_operation_metadata_rebuild_test()
 **********************************************************/



/*!**************************************************************
 * diesel_background_operation_rebuild_test()
 ****************************************************************
 * @brief
 *  This function tests enable/disable rebuild rebuild 
 *  background operation.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 *
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_rebuild_test(fbe_test_rg_configuration_t *rg_config_p)
{

    
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_object_id_t     rg_object_id;
    fbe_object_id_t     vd_object_id;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_u32_t       drive_pos = 0; /* 0th drive position to use in this test */
    fbe_status_t    status;
    fbe_bool_t      b_is_enabled;
   
    mut_printf(MUT_LOG_LOW, "=== Rebuild background operation test started ===\n");	
    
    /* get the raid group object id */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* disable the rebuild background operation */
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    /* remove the drive */
    fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[drive_pos].bus, 
        rg_config_p->rg_disk_set[drive_pos].enclosure,
        rg_config_p->rg_disk_set[drive_pos].slot, &drive_handle);

    /* wait for VD object to transition in fail state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                          DIESEL_MAX_WAIT_TIME_MSECS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* reinsert the drive to start rebuild operation */
    fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[drive_pos].bus, 
            rg_config_p->rg_disk_set[drive_pos].enclosure,
            rg_config_p->rg_disk_set[drive_pos].slot,drive_handle );

    /* wait for VD object to transition in ready state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          DIESEL_MAX_WAIT_TIME_MSECS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* wait for some time */
    fbe_api_sleep(10000);


    /* confirm that rebuild background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);


    /* get the rebuild checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* make sure that rebuild operation is disabled with checking of rebuild checkpoint */
    if(raid_group_info.rebuild_checkpoint[drive_pos] != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable rebuild background operation");
    }

    /* enable rebuild background operation */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_REBUILD);

    /* wait for rebuild operation to complete */
    fbe_test_sep_util_wait_for_raid_group_to_rebuild(rg_object_id);

    mut_printf(MUT_LOG_LOW, "=== Rebuild background operation test completed ===\n");	

    return;
}
/**********************************************************
 * end diesel_background_operation_rebuild_test()
 **********************************************************/




/*!**************************************************************
 * diesel_background_operation_raid_group_verify_test()
 ****************************************************************
 * @brief
 *  This function tests enable/disable raid group verify 
 *  background operations.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_raid_group_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t             rg_object_id;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_status_t    status;
    fbe_bool_t      b_is_enabled;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== Error Verify background operation test started ===\n");	

    /* disable error verify background operation */
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY);

    /* Send a user initiated request to start error verify */
    fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_ERROR);

    /* wait for some time */
    fbe_api_sleep(10000);

    /* confirm Error verify background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);


    /* verify that error verify operation is disabled using checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if(raid_group_info.error_verify_checkpoint != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable error verify operation");
    }

    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY);

    diesel_wait_for_raid_group_verify_completion(rg_object_id, FBE_LUN_VERIFY_TYPE_ERROR);

    mut_printf(MUT_LOG_LOW, "=== Error Verify background operation test completed ===\n");	


    mut_printf(MUT_LOG_LOW, "=== Read_write Verify background operation test started ===\n");	

    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY);

    /* Send a user initiated request to start read write user verify */
    fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RW);

    /* wait for some time */
    fbe_api_sleep(10000);

    /* confirm read_write verify background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);

    /* verify that read write verify operation is disabled using checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if(raid_group_info.rw_verify_checkpoint != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable read_write verify operation");
    }

    /* enable read write verify operation */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY);

    /* wait for read write verify completion */
    diesel_wait_for_raid_group_verify_completion(rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RW);

    mut_printf(MUT_LOG_LOW, "=== Read_write Verify background operation test completed ===\n");	

 
    mut_printf(MUT_LOG_LOW, "=== Read_only Verify background operation test started ===\n");	

    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY);

    /* Send a user initiated request to start read only verify */
    fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RO);

    fbe_api_sleep(10000);

    /* confirm read_only verify background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);

    /* verify that read only verify is disabled using checkpoint */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if(raid_group_info.ro_verify_checkpoint != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Failed to disable read_only verify operation");
    }

    /* enable read only verify */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY);

    /* wait for read only verify operation to complete */
    diesel_wait_for_raid_group_verify_completion(rg_object_id, FBE_LUN_VERIFY_TYPE_USER_RO);

    mut_printf(MUT_LOG_LOW, "=== Read_only Verify background operation test completed ===\n");	

    return;
}

/*!**************************************************************
 * diesel_wait_for_raid_group_verify_completion()
 ****************************************************************
 * @brief
 *  This function waits for the given verify background operation to complete. 
 *  It uses verify checkpoint to determine if given verify operation is complete 
 *  or not.
 *
 * @param rg_object_id      - pointer to raid group object
 * @param verify_type       - verify operation type
 
 *
 * @return void
 *
 ****************************************************************/
void diesel_wait_for_raid_group_verify_completion(fbe_object_id_t rg_object_id,
                                                         fbe_lun_verify_type_t verify_type)
{
    fbe_u32_t                       sleep_time;
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t   raid_group_info;


    for (sleep_time = 0; sleep_time < (DIESEL_MAX_WAIT_TIME_SECS*10); sleep_time++)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Don't report any verify as being complete if there is something 
         * incoming on the event q. 
         */
        if (raid_group_info.b_is_event_q_empty == FBE_TRUE)
        {

            switch(verify_type)
            {
                case FBE_LUN_VERIFY_TYPE_ERROR:
                    if(raid_group_info.error_verify_checkpoint == FBE_LBA_INVALID)
                    {
                        return;
                    }
                    break;

                case FBE_LUN_VERIFY_TYPE_USER_RW:
                    if(raid_group_info.rw_verify_checkpoint == FBE_LBA_INVALID)
                    {
                        return;
                    }

                    break;

                case FBE_LUN_VERIFY_TYPE_USER_RO:
                    if(raid_group_info.ro_verify_checkpoint == FBE_LBA_INVALID)
                    {
                        return;
                    }
                    break;

                default:
                    MUT_ASSERT_TRUE(0)
            }
        }

        /* Sleep 100 miliseconds */
        EmcutilSleep(100);
    }

    /*  The verify operation took too long and timed out. */
    MUT_ASSERT_TRUE(0)
    return;

}   
/**********************************************************
 * end diesel_wait_for_raid_group_verify_completion()
 **********************************************************/


/*!**************************************************************
 * diesel_background_operation_sniff_test()
 ****************************************************************
 * @brief
 *  This function tests enable/disable sniff background operation.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 *
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_sniff_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t                         pvd_object_id;
    fbe_status_t                            status;
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_bool_t                              b_is_enabled;

    mut_printf(MUT_LOG_LOW, "=== Sniff background operation test Started ===\n");	
        
    /* get provision drive object id for first drive */
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &pvd_object_id);

    /* disable zeroing on the all PVDs before start sniff test */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();

    /* disable sniff background operation */
    fbe_api_base_config_disable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);

    /* set the sniff checkpoint at 0 */
    fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id , 0);

    /* clear all verify reports */
    fbe_test_sep_util_provision_drive_clear_verify_report(pvd_object_id);
    
    fbe_test_sep_util_provision_drive_get_verify_status( pvd_object_id, &verify_status );
    if(verify_status.verify_checkpoint != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "diesel_background_operation_sniff_test sniff checkpoint set fail");
    }

    /* start sniff operation 
     * This command goes through job service and different one to control sniff operation from
     * enable/disable background operations
     */    
    fbe_test_sep_util_provision_drive_enable_verify( pvd_object_id );

    /* wait for some time */
    fbe_api_sleep(10000);

    /* confirm sniff background operation is disabled */
    fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, b_is_enabled);

    /* get the sniff operaiton status */
    fbe_test_sep_util_provision_drive_get_verify_status( pvd_object_id, &verify_status );

    /* check that sniff operation is disabled with verify the checkpoint */
    if(verify_status.verify_checkpoint != 0)
    {
        MUT_ASSERT_TRUE_MSG(FBE_FALSE, "diesel_background_operation_sniff_test failed to disable sniff");
    }

    /* enable the sniff operation */
    fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);

    /* wait for sniff operation to complete */
    diesel_wait_for_sniff_completion(pvd_object_id, DIESEL_MAX_WAIT_TIME_SECS*2);

    /* disable verify on this provision drive */
    fbe_test_sep_util_provision_drive_disable_verify( pvd_object_id );

    mut_printf(MUT_LOG_LOW, "=== Sniff background operation test Completed ===\n");	

}
/**********************************************************
 * end diesel_background_operation_sniff_test()
 **********************************************************/

/*!**************************************************************
 * diesel_background_operation_disk_zeroing_test()
 ****************************************************************
 * @brief
 *  This function tests enable/disable disk zeroing background operation.
 *
 * @param rg_config_p - Pointer to the raid group configuration table
 *
 *
 * @return void
 *
 ****************************************************************/
void diesel_background_operation_disk_zeroing_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status; 
    fbe_bool_t                      b_is_enabled = FBE_FALSE;
    fbe_api_provision_drive_info_t  provision_drive_info;
	fbe_lba_t                       zero_checkpoint = 0;
	fbe_lba_t                       current_zero_checkpoint = 0;
    fbe_object_id_t                 pvd_object_id;

    mut_printf(MUT_LOG_LOW, "=== background operation zeroing test Started ===\n");	
    
    /* get provision drive object id for first drive */
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &pvd_object_id);

    /* Set the check point to the second chunk.  The checkpoint must be aligned
     * to the provision drive chunk size.
     */
    status = fbe_api_base_config_is_background_operation_enabled(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable disk zeroing background operation 
     */
    status = fbe_api_base_config_disable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the lowest allowable background zero checkpoint
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    zero_checkpoint = provision_drive_info.default_offset;
    
    /* Now set the checkpoint
     */ 
	status = fbe_api_provision_drive_set_zero_checkpoint(pvd_object_id, zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Sleep for a 5 seconds and then confirm that the checkpoint hasn't changed*/
    fbe_api_sleep(5000);
	status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id,  &current_zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_TRUE(current_zero_checkpoint == zero_checkpoint);

    /* Re-enable background zeroing if required
     */
    if (b_is_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* wait for disk zeroing operation to complete */
        status = fbe_test_sep_util_provision_drive_wait_disk_zeroing(pvd_object_id, FBE_LBA_INVALID, DIESEL_MAX_WAIT_TIME_SECS * 1000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_LOW, "=== disk zeroing background operation test Completed ===\n");	

}
/**********************************************************
 * end diesel_background_operation_disk_zeroing_test()
 **********************************************************/



/*!****************************************************************************
 *  diesel_wait_for_sniff_completion
 ******************************************************************************
 *
 * @brief
 *    This function waits the specified timeout interval for a sniff operation to
 *    complete on a provision drive.  Note that the provision drive is polled
 *    every 100 ms during the timeout interval for a completed verify pass.
 *
 * @param   in_pvd_object_id  -  provision drive to check for a verify operation
 * @param   in_timeout_sec    -  verify operation timeout interval (in seconds)
 *
 * @return  void
 *
 * @author
 *    08/22/2011 - Created. Amit Dhaduk
 *
 *****************************************************************************/

void
diesel_wait_for_sniff_completion(fbe_object_id_t pvd_object_id, fbe_u32_t   timeout_sec )
{
    fbe_u32_t                               poll_time;            /* poll time */
    fbe_u32_t                               verify_pass;          /* verify pass to wait for */
    fbe_provision_drive_verify_report_t     verify_report = {0};  /* verify report  */

    /* get verify report for specified provision drive */
    fbe_test_sep_util_provision_drive_get_verify_report( pvd_object_id, &verify_report );

    /* determine verify pass to wait for */
    verify_pass = verify_report.pass_count + 1;

    /* wait specified timeout interval for a verify pass to complete */
    for ( poll_time = 0; poll_time < (timeout_sec * 10); poll_time++ )
    {
        /* get verify report for specified provision drive */
        fbe_test_sep_util_provision_drive_get_verify_report( pvd_object_id, &verify_report );

        /* finished when next verify pass completes */
        if ( verify_report.pass_count == verify_pass )
        {
            /* verify pass completed for provision drive */
            mut_printf( MUT_LOG_TEST_STATUS,
                        "diesel: verify pass %d completed for object: 0x%x\n",
                        verify_report.pass_count, pvd_object_id
                      );

            return;
        }

        /* wait for next poll */
        fbe_api_sleep( DIESEL_POLLING_INTERVAL );
    }

    /* verify pass failed to complete for provision drive */
    mut_printf( MUT_LOG_TEST_STATUS,
                "diesel: verify pass failed to complete in %d seconds for object: 0x%x\n", 
                timeout_sec, pvd_object_id
              );

    MUT_ASSERT_TRUE( FBE_FALSE );

    return;
}   
/******************************************
 * end diesel_wait_for_sniff_completion()
 ******************************************/


/*!****************************************************************************
 * diesel_test()
 ******************************************************************************
 * @brief
 *  This is main diesel test entry point.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  08/22/2011 - Created. Amit Dhaduk
 ******************************************************************************/
void diesel_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &diesel_rg_configuration[0];

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,diesel_run_tests,
                                   DIESEL_TEST_LUNS_PER_RAID_GROUP,
                                   DIESEL_TEST_CHUNKS_PER_LUN);
     return;
}
/******************************************
 * end diesel_test()
 ******************************************/

/*!****************************************************************************
 *  diesel_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the diesel test.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  8/22/2011 - Created. Amit Dhaduk 
  *****************************************************************************/

void diesel_setup(void)
{

    mut_printf(MUT_LOG_LOW, "=== diesel setup ===\n");	

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &diesel_rg_configuration[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_load_config(rg_config_p, DIESEL_TEST_LUNS_PER_RAID_GROUP, DIESEL_TEST_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);    
    }

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);
    
    return; 
}
/******************************************
 * end diesel_setup()
 ******************************************/

/*!****************************************************************************
 * diesel_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  08/22/2011 - Created. Amit Dhaduk
 ******************************************************************************/

void diesel_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}


/*************************
 * end file diesel_test.c
 *************************/


