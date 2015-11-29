/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file patrick_star_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for ensuring LUN state change notification
 *   is propogated upstream correctly.
 *
 * @version
 *   3/2010 - Created.  rundbs
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_test_configurations.h"               
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_transport.h"
#include "pp_utils.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "VolumeAttributes.h"


/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * patrick_star_short_desc = "LU State/Attribute Change Notification\n";
char * patrick_star_long_desc = "\
The Patrick Star scenario focuses on ensuring the LUN state/attribute change notification is sent \n\
from the LUN to BVD.  This test runs on in both Simulation and Hardware environment.\n"\
"\n"\
"-raid_test_level 0 \n\
\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Tests cover 520 block sizes.\n\
        - Tests cover one Raid-5 RG and one LUN in that RG.\n\
\n\
STEP 2: Get LUN, BVD, and Two Disks.\n\
        - Get LUN object ID \n\
        - Get BVD object ID \n\
        - Get Disk in Position 1 that will be used in this test\n\
        - Get Disk in Position 2 that will be used in this test\n\
\n\
STEP 3: Remove First Drive \n\
        - Remove a Disk in Position 1 that obtains from STEP 2\n\
        - Wait for BVD to aware of the corresponding LUN attribute change to degrade\n\
\n\
STEP 4: Remove Second Drive \n\
        - Remove a Disk in Position 2 that obtains from STEP 2\n\
        - Wait for the LUN's failed lifecycle state \n\
        - Wait for BVD to aware of the corresponding LUN attribute change to broken (unattached)\n\
\n\
STEP 5: Insert Both Drives \n\
        - Insert both drives back in \n\
        - Wait for both BVD and LUN objects to be in READY state.\n\
        - Wait for BVD to aware of the corresponding LUN attribute change to enable \n\
          (no longer unattached nor degraded)\n\
\n\
STEP 6: Remove First Drive again, then reboot SEP and test LUN attribute is set for degraded.\n\
\n\
Description last updated: 5/29/2012.\n";


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/


/*!*******************************************************************
 * @def PATRICK_STAR_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def PATRICK_STAR_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_MAX_RG_WIDTH               16



/*!*******************************************************************
 * @def PATRICK_STAR_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in the RAID Group
 * !@TODO: remove this define when a utility function is available to create a RG only
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_LUNS_PER_RAID_GROUP        1

/*!*******************************************************************
 * @def PATRICK_STAR_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define PATRICK_STAR_CHUNKS_PER_LUN 3


/*!*******************************************************************
 * @def PATRICK_STAR_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_RAID_GROUP_ID        0


/*!*******************************************************************
 * @def PATRICK_STAR_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_NS_TIMEOUT        60000 /*wait maximum of 60 seconds*/


/*!*******************************************************************
 * @def PATRICK_STAR_TEST_POLLING_INTERVAL
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define PATRICK_STAR_TEST_POLLING_INTERVAL  100 /*ms*/


fbe_test_rg_configuration_t patrick_star_raid_group_config[] =
{

    /* width, capacity                 raid type,      class,                  block size      RAID-id.                bandwidth.*/
    {3,       0x4000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,   PATRICK_STAR_TEST_RAID_GROUP_ID,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/* LUN info used to create a LUN */
typedef struct patrick_star_test_create_info_s
{
    fbe_raid_group_type_t     raid_type; 
    fbe_raid_group_number_t   raid_group_id;
    fbe_block_transport_placement_t placement;
    fbe_lun_number_t                wwid;
    fbe_lba_t                       capacity;
    fbe_bool_t                      ndb_b;
    fbe_bool_t                      noinitialverify_b;
    fbe_lba_t                       ndb_addroffset;
} patrick_star_test_create_info_t;


typedef  struct  patrick_start_test_context_s
{
    fbe_semaphore_t     sem;
    fbe_object_id_t     object_id;
    fbe_bool_t          lun_degraded_flag;
} patrick_star_test_context_t;

static fbe_api_rdgen_context_t 	patrick_star_test_rdgen_contexts[5];

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
void patrick_star_run_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
static void patrick_star_test_pull_drive(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
                                         fbe_api_terminator_device_handle_t* drive_handle);
static void patrick_star_test_reinsert_drive(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
                                             fbe_api_terminator_device_handle_t drive_handle,
                                             fbe_object_id_t pvd_id);
static void patrick_star_test_lun_degraded_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context);
static void patrick_star_test_wait_lun_degraded_notification(patrick_star_test_context_t* lun_degraded_context);
static void patrick_star_test_register_lun_degraded_notification(fbe_object_id_t object_id, 
                                         patrick_star_test_context_t* context, 
                                         fbe_notification_registration_id_t* registration_id);
static void patrick_star_test_unregister_lun_degraded_notification(fbe_object_id_t object_id, 
                                         patrick_star_test_context_t* context, 
                                         fbe_notification_registration_id_t registration_id);
static void patrick_star_test_degraded_lu_attribute(fbe_test_rg_configuration_t *rg_config_p);
static void patrick_star_test_degraded_bind_lu_attribute(fbe_test_rg_configuration_t *rg_config_p);

static fbe_status_t patrick_star_test_run_io_load(fbe_object_id_t lun_object_id);
static void patrick_star_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks);
/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!**************************************************************
 * patrick_star_test_pull_drive()
 ****************************************************************
 * @brief
 *  This function pulls the specified drive.  This means the
 *  the drive is removed, but its data is preserved, i.e.
 *  logout the drive. The handle for the drive is returned.
 *
 * @param   bus             - bus of drive to pull (IN)
 * @param   enclosure       - enclosure of drive to pull (IN)
 * @param   slot            - slot of drive to pull (IN)
 * @param   drive_handle_p  - handle of drive pulled (OUT)
 *
 * @return None.
 *
 ****************************************************************/
static void patrick_star_test_pull_drive(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
                                         fbe_api_terminator_device_handle_t* drive_handle_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove a drive: bus %d, encl %d, slot %d ===\n", bus, enclosure, slot);

    status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, drive_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} /* End patrick_star_test_pull_drive() */


/*!**************************************************************
 * patrick_star_test_reinsert_drive()
 ****************************************************************
 * @brief
 *  This function reinserts the specified drive.
 *
 * @param   bus             - bus of drive to reinsert (IN)
 * @param   enclosure       - enclosure of drive to reinsert (IN)
 * @param   slot            - slot of drive to reinsert (IN)
 * @param   drive_handle    - handle of drive reinsert (IN)
 * @param   pvd_object_id   - PVD object ID of drive reinsert (IN)
 *
 * @return None.
 *
 ****************************************************************/
static void patrick_star_test_reinsert_drive(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
                                             fbe_api_terminator_device_handle_t drive_handle,
                                             fbe_object_id_t pvd_id)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    
    /* Reinsert drive */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Reinsert a drive: bus %d, encl %d, slot %d ===\n", bus, enclosure, slot);
    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, drive_handle);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for pdo to be ready */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for drive to be ready ===\n");
    status = fbe_test_pp_util_verify_pdo_state(bus, enclosure, slot, FBE_LIFECYCLE_STATE_READY, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait until the PVD object transition to READY state */
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;

} /* End patrick_star_test_reinsert_drive() */



/*!****************************************************************************
 *  patrick_star_run_test
 ******************************************************************************
 *
 * @brief
 *  Test the functionality that when RG goes in degraded or broken state, BVD changes 
 *  the attributes corresponding LUN state change.
 *
 *
 * @param rg_config_p - Current configuration.          
 *
 * @return  None 
 *
 *****************************************************************************/
void patrick_star_run_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                     lun_object_id;
    fbe_object_id_t                     bvd_object_id;
    fbe_u32_t                           current_time = 0;
    fbe_volume_attributes_flags         attribute;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p_1 = NULL;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p_2 = NULL;
    fbe_u32_t position_to_remove_1 = 0;
    fbe_u32_t position_to_remove_2 = 1;
    fbe_lun_number_t lun_number;
    fbe_notification_registration_id_t  registration_id;
    patrick_star_test_context_t         context;


    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Patrick Star Test ===\n");

    lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;

    /* Get the object ID for the LUN; will use it later */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Get the object ID for BVD; will use it later */
    status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);

    /* get the disk detail for first position */
    drive_to_remove_p_1 = &rg_config_p->rg_disk_set[position_to_remove_1];

    /* get the disk detail for second position */
    drive_to_remove_p_2 = &rg_config_p->rg_disk_set[position_to_remove_2];

    /* register */
    patrick_star_test_register_lun_degraded_notification(lun_object_id, &context, &registration_id);

    /* Pull one drive used by the LUN */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p_1->bus, 
                                             drive_to_remove_p_1->enclosure, 
                                             drive_to_remove_p_1->slot,
                                             &rg_config_p->drive_handle[position_to_remove_1]);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p_1->bus, 
                                                drive_to_remove_p_1->enclosure, 
                                                drive_to_remove_p_1->slot);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Ensure the BVD knows that the LUN is now degraded */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get LUN attribute change === \n");
    fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT) 
    {
        /* Get event from BVD object */
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED)
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }

    MUT_ASSERT_TRUE(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED);

    patrick_star_test_wait_lun_degraded_notification(&context);
    /* when attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED is true, we will get the notification 
       and context.lun_degraded_flag will be set to true. */
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, context.lun_degraded_flag);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s BVD is VOL_ATTR_RAID_PROTECTION_DEGRADED(successful)==\n", __FUNCTION__);

    /* Pull another drive used by the LUN */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p_2->bus, 
                                             drive_to_remove_p_2->enclosure, 
                                             drive_to_remove_p_2->slot,
                                             &rg_config_p->drive_handle[position_to_remove_2]);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p_2->bus, 
                                                drive_to_remove_p_2->enclosure, 
                                                drive_to_remove_p_2->slot);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for LUN to go into the fail state */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for LUN to go into the fail state === \n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     PATRICK_STAR_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Ensure the BVD knows that the LUN is now broken */
    // Commented out references of VOL_ATTR_UNATTACHED. AR 582312
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get LUN state change === \n");
//    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
//    {
        /* Get event from BVD object */
//        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
//        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

//        if (attribute & VOL_ATTR_UNATTACHED)
//        {
//            break;
//        }
//        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
//        fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
//   }

//    MUT_ASSERT_TRUE(attribute & VOL_ATTR_UNATTACHED);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s BVD is FBE_PATH_STATE_BROKEN(successful)==\n", __FUNCTION__);

    /* Reinsert drives */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_remove_p_1->bus, 
                                                 drive_to_remove_p_1->enclosure, 
                                                 drive_to_remove_p_1->slot,
                                                 rg_config_p->drive_handle[position_to_remove_1]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_pp_util_reinsert_drive(drive_to_remove_p_2->bus, 
                                                 drive_to_remove_p_2->enclosure, 
                                                 drive_to_remove_p_2->slot,
                                                 rg_config_p->drive_handle[position_to_remove_2]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_remove_p_1->bus, 
                                                    drive_to_remove_p_1->enclosure, 
                                                    drive_to_remove_p_1->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_remove_p_2->bus, 
                                                    drive_to_remove_p_2->enclosure, 
                                                    drive_to_remove_p_2->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==\n", __FUNCTION__);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     PATRICK_STAR_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==\n", __FUNCTION__);

    /* Ensure the BVD knows that the LUN is now enabled */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get LUN state change === \n");
    // Commented out references of VOL_ATTR_UNATTACHED. AR 582312
//    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
//    {
        /* Get attr from BVD object */
//        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
//        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

//        if (!(attribute & VOL_ATTR_UNATTACHED))
//        {
//            break;
//        }
//        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
//        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
//    }

//    MUT_ASSERT_TRUE(!(attribute & VOL_ATTR_UNATTACHED));

    mut_printf(MUT_LOG_TEST_STATUS, "== %s BVD is FBE_PATH_STATE_ENABLED(successful)==\n", __FUNCTION__);

    /* Ensure the BVD knows that the LUN is no longer degraded */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get LUN attribute change === \n");
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (!(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }

    MUT_ASSERT_TRUE(!(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED));
    patrick_star_test_wait_lun_degraded_notification(&context);
    /* when attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED is false, we will also get the notification 
       and context.lun_degraded_flag will be set to false. */
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, context.lun_degraded_flag);
    patrick_star_test_unregister_lun_degraded_notification(lun_object_id, &context, registration_id);

    patrick_star_test_degraded_lu_attribute(rg_config_p);
    patrick_star_test_degraded_bind_lu_attribute(rg_config_p);
    return;
 
} /* End patrick_star_test() */


/*!****************************************************************************
 *  patrick_star_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Patrick Star test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void patrick_star_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &patrick_star_raid_group_config[0];

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,patrick_star_run_test,
                                   PATRICK_STAR_TEST_LUNS_PER_RAID_GROUP,
                                   PATRICK_STAR_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end patrick_star_test()
 ******************************************/


/*!****************************************************************************
 *  patrick_star_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Patrick Star test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void patrick_star_test_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Patrick Star test ===\n");

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        fbe_u32_t num_raid_groups;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &patrick_star_raid_group_config[0];

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_count(rg_config_p);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    return;

} /* End patrick_star_test_setup() */


/*!****************************************************************************
 *  patrick_star_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Patrick Star test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void patrick_star_test_cleanup(void)
{  

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Patrick Star test ===\n");

    if (fbe_test_util_is_simulation())
    {
    /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} /* End patrick_star_test_cleanup() */

static void  patrick_star_test_lun_degraded_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context)
{
    patrick_star_test_context_t*  lun_degraded_context = (patrick_star_test_context_t*)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    if(update_obj_msg->object_id == lun_degraded_context->object_id)
    {
        lun_degraded_context->lun_degraded_flag = update_obj_msg->notification_info.notification_data.lun_degraded_flag;
        fbe_semaphore_release(&lun_degraded_context->sem, 0, 1, FALSE); 
    }
}

/*!**************************************************************
 * patrick_star_test_wait_lun_degraded_notification()
 ****************************************************************
 * @brief
 *  Wait for the lun_degraded notification sent from the bvd.
 *
 * @param lun_degraded_context              
 *
 * @return None.
 *
 * @author
 *  05/23/2012 - Created. Vera Wang
 *
 ****************************************************************/

static void  patrick_star_test_wait_lun_degraded_notification(patrick_star_test_context_t* lun_degraded_context)
{
    fbe_status_t  status;
    
    MUT_ASSERT_TRUE(NULL != lun_degraded_context);
    
    status = fbe_semaphore_wait_ms(&(lun_degraded_context->sem), PATRICK_STAR_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end patrick_star_test_wait_lun_degraded_notification()
 ******************************************/

/*!**************************************************************
 * patrick_star_test_register_lun_degraded_notification()
 ****************************************************************
 * @brief
 *  Register lun_degraded notification.
 *
 * @param object_id to register.(IN) 
 * @param context   notification context(OUT)
 * @param registration_id (OUT)  
 *
 * @return None.
 *
 * @author
 *  05/23/2012 - Created. Vera Wang
 *
 ****************************************************************/

static void patrick_star_test_register_lun_degraded_notification(fbe_object_id_t object_id, 
                                         patrick_star_test_context_t* context, 
                                         fbe_notification_registration_id_t* registration_id)
{
    fbe_status_t    status;

    MUT_ASSERT_TRUE(NULL != context);
    MUT_ASSERT_TRUE(NULL != registration_id);

    context->object_id = object_id;
    fbe_semaphore_init(&(context->sem), 0, 1);

    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED, 
        FBE_PACKAGE_NOTIFICATION_ID_SEP_0, FBE_TOPOLOGY_OBJECT_TYPE_LUN, 
        patrick_star_test_lun_degraded_callback_function, context, registration_id);
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end patrick_star_test_register_lun_degraded_notification()
 ******************************************/

/*!**************************************************************
 * patrick_star_test_unregister_lun_degraded_notification()
 ****************************************************************
 * @brief
 *  Unregister lun_degraded notification.
 *
 * @param object_id to register.(IN) 
 * @param context   notification context(OUT)
 * @param registration_id (OUT)                 
 *
 * @return None.
 *
 * @author
 *  05/23/2012 - Created. Vera Wang
 *
 ****************************************************************/
 
static void  patrick_star_test_unregister_lun_degraded_notification(fbe_object_id_t object_id, 
                                          patrick_star_test_context_t* context, 
                                          fbe_notification_registration_id_t registration_id)
{
    fbe_status_t  status;
    status = fbe_api_notification_interface_unregister_notification(patrick_star_test_lun_degraded_callback_function,
         registration_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_semaphore_destroy(&context->sem); /* SAFEBUG - needs destroy */
}
/******************************************
 * end patrick_star_test_unregister_lun_degraded_notification()
 ******************************************/

static void patrick_star_test_degraded_lu_attribute(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t 						status;
    fbe_database_lun_info_t 			lun_info;
	fbe_object_id_t 					bvd_object_id;
    fbe_object_id_t                     object_id;
	fbe_u32_t                   		current_time = 0;
	fbe_volume_attributes_flags         attribute;
    fbe_test_raid_group_disk_set_t*     drive_to_remove_p_1;
    fbe_lun_number_t                    lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;

    
    /* Get the object ID for the LUN; will use it later */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

	/* Get the object ID for BVD; will use it later */
    status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);

    drive_to_remove_p_1 = &rg_config_p->rg_disk_set[0];
    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove first drive again to test lun attribute is set to degraded for step 6. === \n");
    /* Pull one drive used by the LUN again to test lun attribute is set correctly when we get lun info during SEP startup. */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p_1->bus, 
                                             drive_to_remove_p_1->enclosure, 
                                             drive_to_remove_p_1->slot,
                                             &rg_config_p->drive_handle[0]);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p_1->bus, 
                                                drive_to_remove_p_1->enclosure, 
                                                drive_to_remove_p_1->slot);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Ensure the BVD knows that the LUN is now degraded */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get LUN attribute change === \n");
    fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT) 
    {
        /* Get event from BVD object */
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED)
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }

    MUT_ASSERT_TRUE(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED);

	/*run some IO to the LUN, this will make sure the VOL_ATTR_HAS_BEEN_WRITTEN
	will be present on the edge*/
	patrick_star_test_run_io_load(object_id);

	/*make sure it's there*/
	mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN attribute change === \n");
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if ((attribute & VOL_ATTR_HAS_BEEN_WRITTEN))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }
    MUT_ASSERT_TRUE((attribute & VOL_ATTR_HAS_BEEN_WRITTEN));

    fbe_zero_memory(&lun_info, sizeof(lun_info));

    /* reboot SEP*/
    if (fbe_test_util_is_simulation()) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy SEP. ===\n");
        fbe_test_sep_util_destroy_neit_sep();
        mut_printf(MUT_LOG_TEST_STATUS, "=== Reload SEP again. ===\n");
        sep_config_load_sep_and_neit();  

        /* Wait for the object to be ready, since we're just loading the package and it might take a while for the object to
         * be up. 
         */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    }
    lun_info.lun_object_id = object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE((lun_info.is_degraded == FBE_TRUE));

	/*make sure it's there after reboot as well*/
	attribute = 0;
    current_time = 0;
	mut_printf(MUT_LOG_TEST_STATUS, "=== Wait for BVD to get FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN attribute change afte reboot=== \n");
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if ((attribute & VOL_ATTR_HAS_BEEN_WRITTEN)
            && (attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }
    MUT_ASSERT_TRUE((attribute & VOL_ATTR_HAS_BEEN_WRITTEN));
    MUT_ASSERT_TRUE((attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED));

    /*resume the RG from degraded*/
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_remove_p_1->bus, 
                                                 drive_to_remove_p_1->enclosure, 
                                                 drive_to_remove_p_1->slot,
                                                 rg_config_p->drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_remove_p_1->bus, 
                                                    drive_to_remove_p_1->enclosure, 
                                                    drive_to_remove_p_1->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /*check that the LUN is not degraded now*/
    fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    
    /*make sure it's there after reboot as well*/
    fbe_zero_memory(&lun_info, sizeof(lun_info));
    attribute = 0;
    current_time = 0;
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if ((attribute & VOL_ATTR_HAS_BEEN_WRITTEN)
            && !(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }
    MUT_ASSERT_TRUE((attribute & VOL_ATTR_HAS_BEEN_WRITTEN));
    MUT_ASSERT_FALSE((attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED));

    lun_info.lun_object_id = object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_FALSE((lun_info.is_degraded == FBE_TRUE));
    
}

static fbe_status_t patrick_star_test_run_io_load(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t*    context_p = &patrick_star_test_rdgen_contexts[0];
    fbe_status_t                status;

    /* Set up for issuing reads forever
     */
    patrick_star_test_setup_rdgen_test_context(context_p,
                                         lun_object_id,
                                         FBE_CLASS_ID_INVALID,
                                         FBE_RDGEN_OPERATION_WRITE_ONLY,
                                         FBE_LBA_INVALID,    /* use capacity */
                                         0,      /* run forever*/ 
                                         3,      /* threads per lun */
                                         1024);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_thread_delay(5000);/*let IO run for a while*/

    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;

} 




static void patrick_star_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} 

/*!********************************************************************
 * patrick_star_test_degraded_bind_lu_attribute()
 **********************************************************************
 * @brief
 *  Test the RAID_PROTECTION_DEGRADED attribute in the following case:
 *      A LUN is bound when the RG is degraded.
 *
 * @param rg_config_p Raid group configuration
 *
 * @return None.
 *
 * @author
 *  03/30/2013 - Created. Zhipeng Hu
 *
 **********************************************************************/
static void patrick_star_test_degraded_bind_lu_attribute(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                     lun_object_id;
    fbe_object_id_t                     bvd_object_id;
    fbe_volume_attributes_flags         attribute;
    fbe_lun_number_t lun_number;
    fbe_api_lun_destroy_t           fbe_lun_destroy_req;
    fbe_job_service_error_type_t    job_error_type = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_database_lun_info_t         lun_info;
    fbe_u32_t                       current_time = 0;
    fbe_test_raid_group_disk_set_t*     drive_to_remove_p_1 = &rg_config_p->rg_disk_set[0];
    fbe_status_t    job_status;

    /* Get the object ID for BVD; will use it later */
    status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove first drive again to test lun attribute is set to degraded for step 6. === \n");
    /* Pull one drive used by the LUN again to test lun attribute is set correctly when we get lun info during SEP startup. */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p_1->bus, 
                                             drive_to_remove_p_1->enclosure, 
                                             drive_to_remove_p_1->slot,
                                             &rg_config_p->drive_handle[0]);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p_1->bus, 
                                                drive_to_remove_p_1->enclosure, 
                                                drive_to_remove_p_1->slot);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Patrick Star  Degraded Bind LU Test ===\n");    

    lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;

    mut_printf(MUT_LOG_TEST_STATUS, "1. Remove the LUN\n");
    fbe_lun_destroy_req.lun_number = lun_number;
    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, PATRICK_STAR_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(job_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                                "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    mut_printf(MUT_LOG_TEST_STATUS, "2. Create the LUN again\n");
    status = fbe_test_sep_util_create_logical_unit(&rg_config_p->logical_unit_configuration_list[0]);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                                "create LUN failed");

    status = fbe_api_common_wait_for_job(rg_config_p->logical_unit_configuration_list[0].job_number,
                                         PATRICK_STAR_TEST_NS_TIMEOUT,
                                         &job_error_type,
                                         &job_status,
                                         NULL);    
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN config creation failed");
    MUT_ASSERT_TRUE_MSG((job_error_type == FBE_JOB_SERVICE_ERROR_NO_ERROR), "LUN config creation failed");

    mut_printf(MUT_LOG_TEST_STATUS, "3. Check the LUN and BVD attribute\n");   

    /* Get the object ID for the LUN; will use it later */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
    
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE((lun_info.is_degraded == FBE_TRUE));

    
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if ((attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }
    MUT_ASSERT_TRUE((attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED));

    /*resume the degraded RG*/
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_remove_p_1->bus, 
                                                 drive_to_remove_p_1->enclosure, 
                                                 drive_to_remove_p_1->slot,
                                                 rg_config_p->drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Not in simulation */
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_remove_p_1->bus, 
                                                    drive_to_remove_p_1->enclosure, 
                                                    drive_to_remove_p_1->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /*check that the LUN is not degraded now*/
    fbe_api_sleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    
    /*make sure it's there after reboot as well*/
    fbe_zero_memory(&lun_info, sizeof(lun_info));
    attribute = 0;
    current_time = 0;
    while (current_time < PATRICK_STAR_TEST_NS_TIMEOUT)
    {
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (!(attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED))
        {
            break;
        }
        current_time = current_time + PATRICK_STAR_TEST_POLLING_INTERVAL;
        EmcutilSleep(PATRICK_STAR_TEST_POLLING_INTERVAL);
    }
    MUT_ASSERT_FALSE((attribute & VOL_ATTR_RAID_PROTECTION_DEGRADED));

    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_FALSE((lun_info.is_degraded == FBE_TRUE));

    return; 
} /* End patrick_star_test_degraded_bind_lu_attribute() */

