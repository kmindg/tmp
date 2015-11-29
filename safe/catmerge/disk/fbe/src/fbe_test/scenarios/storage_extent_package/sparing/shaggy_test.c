/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file shaggy_drive_sparing_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains a shaggy drive sparing test.
 *
 * HISTORY
 *   7/30/2009:  Created. Dhaval Patel.
 *   3/04/2010:  Modified. Anuj Singh. To support table driven methodology.
 *   8/15/2012:  Updated to validation proper notification(s). Ron Proulx
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
#include "sep_hook.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test_common_utils.h"
#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "fbe/fbe_api_encryption_interface.h"

char * shaggy_short_desc = "This scenario test basic drive sparing and the associated notifications.";
char * shaggy_long_desc =
"\n"
"\tHardware Configuration:\n"  
"\t1 port, 1 viper enclosure and 5 SAS drives.\n"
"\tAssumptions:\n"
"\t - Single SP only.\n"
"\tScenario Sequence:\n"
"\t1.   This is a MUT driven test.\n"
"\t2.   Create the hardware config using the terminator api calls.\n"
"\t3.   Create a raid 0 raid group\n" 
"\t\ta. Raid group attributes:\n"
"\t\t\ti.   width: 5 drives\n" 
"\t\t\tii.  capacity: 0x6000 blocks.\n"
"\t\tb. Create provisioned drives and attach edges to logical drives.\n"
"\t\tc. Create virtual drives and attach edges to provisioned drives.\n"
"\t\td. Create raid 0 raid group object and attach edges from raid 0 to virtual drives.\n"
"\t4.   Create and attach 3 LUNs to the raid 0 object.\n"
"\t\ta. Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
"\t5.   Remove one of the SAS drive from RAID group using terminator API.\n"
"\t\ta.   Verify that PDO and LDO object associated with this drive get destroyed.\n"
"\t\tb.   Verify that PVD, VD, RAID, LUN object associated with this drive changes it's state to ACTIVATE.\n"
"\t\tc.   Verify that edge between PVD and LDO gets detached for this drive.\n"
"\t6.   Insert the same SAS drive back using terminator API.\n"
"\t\ta.   Verify that PDO and LDO object associated with this drive becomes READY.\n"
"\t\tb.   Verify that shaggy test receives notification when LDO becomes ready.\n"
"\t7.   Shaggy test in response to notification will create edge between corresponding PDO and LDO object.\n"
"\t\ta.   Verify that PVD, VD, RAID and LUN object associated with this drive changes its state to READY.\n";

/*!*******************************************************************
 * @def SHAGGY_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the qual test. 
 *
 *********************************************************************/
#define SHAGGY_TEST_LUNS_PER_RAID_GROUP                 1

/*********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SHAGGY_TEST_CHUNKS_PER_LUN                      3

/*!*******************************************************************
 * @def SHAGGY_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief raid group count for the sas 520 raid group. 
 *
 *********************************************************************/
#define SHAGGY_TEST_RAID_GROUP_COUNT                    5

/*!*******************************************************************
 * @def SHAGGY_HS_SAS_520_COUNT
 *********************************************************************
 * @brief hot spare count for the sas 520 drive.
 *
 *********************************************************************/
#define SHAGGY_HS_SAS_520_COUNT                         14

/*!*******************************************************************
 * @def SHAGGY_TEST_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SHAGGY_TEST_WAIT_SEC 60

#define SHAGGY_TEST_MAX_RETRIES     80     // retry count - number of times to loop

/*!*******************************************************************
 * @def SHAGGY_TEST_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SHAGGY_TEST_WAIT_MSEC 1000 * SHAGGY_TEST_WAIT_SEC

/*!*******************************************************************
 * @var shaggy_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t shaggy_raid_group_config[SHAGGY_TEST_RAID_GROUP_COUNT + 1] = 
{
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        //{5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},

        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            2,          0},

        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},

        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            4,          0},

        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            5,          0},

        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},

        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/* shaggy rdgen context. */
fbe_api_rdgen_context_t   shaggy_test_context[SHAGGY_TEST_LUNS_PER_RAID_GROUP * 2];


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void shaggy_test_double_fault_permanent_sparing_r6_r10_r1(fbe_test_rg_configuration_t *rg_config_p);
                                                          
void shaggy_test_single_fault_permanent_sparing(fbe_test_rg_configuration_t *rg_config_p);

void shaggy_test_sparing_after_reboot(fbe_test_rg_configuration_t *rg_config_p, fbe_test_rg_configuration_t *whole_rg_config_p);

void shaggy_test_get_next_random_drive_position(fbe_test_rg_configuration_t * rg_config_p,
                                                fbe_u32_t * position_p);

void shaggy_test_raid10_get_mirror_position(fbe_u32_t orig_pos,
                                            fbe_u32_t width,
                                            fbe_u32_t * mirror_position_p);

static void shaggy_test_configure_hs_reboot(fbe_test_rg_configuration_t * rg_config_p, 
                                            fbe_u32_t unused_drive_index);
static void shaggy_test_validate_snake_river_NDU_case(fbe_test_rg_configuration_t * rg_config_p);
static void shaggy_test_wait_for_drive_reinsert(fbe_test_hs_configuration_t * hs_config_p,
                                                fbe_test_rg_configuration_t *rg_config_p);
static void shaggy_test_immediate_spare_faulted_drive(fbe_test_rg_configuration_t *rg_config_p);
static void shaggy_test_sparing_encryption_reboot_test(fbe_test_rg_configuration_t *rg_config_p);
void shaggy_run_tests(fbe_test_rg_configuration_t *rg_config_p,void * context_p);
static void shaggy_test_use_unused_drive_as_spare(fbe_test_rg_configuration_t * rg_config_p);
fbe_status_t shaggy_test_config_hotspares(fbe_test_raid_group_disk_set_t *disk_set_p,
                                                     fbe_u32_t no_of_spares);

void shaggy_test_fill_removed_disk_set(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                       fbe_u32_t * position_p,
                                       fbe_u32_t number_of_drives);
void shaggy_test_reinsert_removed_disks(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                        fbe_u32_t * position_p,
                                        fbe_u32_t    number_of_drives);

void shaggy_test_setup_hot_spare(fbe_test_hs_configuration_t * hs_config_p);

/*!****************************************************************************
 * shaggy_setup()
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
void shaggy_setup(void)
{
     
    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &shaggy_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();

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
 * end shaggy_setup()
 ******************************************************************************/

void shaggy_cleanup(void)
{
    /* Cleanup  notifications*/
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test */);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * shaggy_restart_sep()
 ******************************************************************************
 * @brief
 *  Sparing after reboot test.
 *
 * @return None.
 *
 * @author
 *  09/25/2012 - Created. Prahlad Purohit
 ******************************************************************************/
static void shaggy_restart_sep(fbe_test_rg_configuration_t * rg_config_ptr)
{
    fbe_status_t    status;
    fbe_u32_t       num_raid_groups;
    fbe_u32_t       rg_index;
    fbe_u32_t       lun_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_test_rg_configuration_t * rg_config_p = NULL;


    /* Shutdown the SEP */
    fbe_test_sep_util_destroy_sep();

    /* restart the SEP */
    sep_config_load_sep();
    
    /* confirm the RG and LUNs are in the READY state
     * much of this code comes from fbe_test_sep_util_create_raid_group_configuration() 
     */
    fbe_api_sleep(1000);

    /* get the number of RGs*/    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    /* for each configured RG, make sure it is enabled */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* get the object ID of the RG */
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
        
            /* verify the raid group get to ready state in reasonable time */
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                             FBE_LIFECYCLE_STATE_READY, 120000,
                                                             FBE_PACKAGE_ID_SEP_0);
            mut_printf(MUT_LOG_TEST_STATUS, "%s RG Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                              __FUNCTION__, rg_object_id,  rg_config_p->raid_type);

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }

    /* for each configured RG, make sure its LUNs are enabled */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            for (lun_index = 0; lun_index < rg_config_p->number_of_luns_per_rg; lun_index++)
            {
                /* find the object id of the lun - unless the lun id was defaulted .... we have no way of knowing the lun
                 id or the object id in that case */
                if (rg_config_p->logical_unit_configuration_list[lun_index].lun_number != FBE_LUN_ID_INVALID )
                {
                    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                                        &lun_object_id);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
        
                    /* verify the lun get to ready state in reasonable time. */
                    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                          lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                                          6000,
                                                                                          FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                                      __FUNCTION__, lun_object_id,  rg_config_p->logical_unit_configuration_list[lun_index].raid_type);
                    /* verify the lun connect to BVD in reasonable time. */
                    status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                               lun_object_id,
                                                                               6000);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x connects to BVD",
                                      __FUNCTION__, lun_object_id);
                }
            }
        }
    }
}
/*************************** 
 * end shaggy_restart_sep()
 ***************************/

/*!****************************************************************************
 * shaggy_restart_dualsp_sep()
 ******************************************************************************
 * @brief
 *  Sparing after reboot test.
 *
 * @return None.
 *
 * @author
 *  09/25/2012 - Created. Prahlad Purohit
 ******************************************************************************/
static void shaggy_restart_dualsp_sep(fbe_test_rg_configuration_t * rg_config_ptr)
{
    fbe_status_t    status;
    fbe_u32_t       num_raid_groups;
    fbe_u32_t       rg_index;
    fbe_u32_t       lun_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_test_rg_configuration_t * rg_config_p = NULL;
    fbe_sim_transport_connection_target_t this_sp, peer_sp;

    this_sp = fbe_api_sim_transport_get_target_server();
    peer_sp = (this_sp == FBE_SIM_SP_B)? FBE_SIM_SP_A : FBE_SIM_SP_B;

    /* shutdown sep on this sp. */
    fbe_test_sep_util_destroy_sep();

    /* shutdown sep on peer sp. */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fbe_test_sep_util_destroy_sep();

    /* restart sep on both sps. */
    sep_config_load_sep_with_time_save(FBE_TRUE);    

    fbe_api_sim_transport_set_target_server(this_sp);    
    
    /* confirm the RG and LUNs are in the READY state
     * much of this code comes from fbe_test_sep_util_create_raid_group_configuration() 
     */
    fbe_api_sleep(1000);

    /* get the number of RGs*/    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    /* for each configured RG, make sure it is enabled */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* get the object ID of the RG */
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
        
            /* verify the raid group get to ready state in reasonable time */
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                             FBE_LIFECYCLE_STATE_READY, 120000,
                                                             FBE_PACKAGE_ID_SEP_0);
            mut_printf(MUT_LOG_TEST_STATUS, "%s RG Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                              __FUNCTION__, rg_object_id,  rg_config_p->raid_type);

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }

    /* for each configured RG, make sure its LUNs are enabled */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            for (lun_index = 0; lun_index < rg_config_p->number_of_luns_per_rg; lun_index++)
            {
                /* find the object id of the lun - unless the lun id was defaulted .... we have no way of knowing the lun
                 id or the object id in that case */
                if (rg_config_p->logical_unit_configuration_list[lun_index].lun_number != FBE_LUN_ID_INVALID )
                {
                    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                                        &lun_object_id);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
        
                    /* verify the lun get to ready state in reasonable time. */
                    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                          lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                                          60000,
                                                                                          FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                                      __FUNCTION__, lun_object_id,  rg_config_p->logical_unit_configuration_list[lun_index].raid_type);
                    /* verify the lun connect to BVD in reasonable time. */
                    status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                               lun_object_id,
                                                                               60000);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x connects to BVD",
                                      __FUNCTION__, lun_object_id);
                }
            }
        }
    }
}
/**********************************
 * end shaggy_restart_dualsp_sep()
 **********************************/
 
/*!****************************************************************************
 * shaggy_test()
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
void shaggy_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &shaggy_raid_group_config[0];
    
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,shaggy_run_tests,
                                   SHAGGY_TEST_LUNS_PER_RAID_GROUP,
                                   SHAGGY_TEST_CHUNKS_PER_LUN);
    return;    

}
/******************************************************************************
 * end shaggy_test()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_dualsp_test()
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
void shaggy_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &shaggy_raid_group_config[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,shaggy_run_tests,
                                   SHAGGY_TEST_LUNS_PER_RAID_GROUP,
                                   SHAGGY_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end shaggy_dualsp_test()
 ******************************************************************************/


/*!**************************************************************
 * shaggy_dualsp_setup()
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
void shaggy_dualsp_setup(void)
{

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        
        rg_config_p = &shaggy_raid_group_config[0];
        

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

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
 * end shaggy_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * shaggy_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the shaggy test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/30/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/

void shaggy_dualsp_cleanup(void)
{
    /* Destroy semaphore
     */
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test */);

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
 * end shaggy_dualsp_cleanup()
 ******************************************/


/*!****************************************************************************
 * shaggy_run_tests()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  4/10/2010 - Created. Dhaval Patel
 *   5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
void shaggy_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   num_raid_groups = 0;
    fbe_u32_t                   index = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);

    /* Disable automatic sparing.
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* For all raid groups.
     */
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        /* Only run test on enabled raid groups.
         */
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {

            /* 1) normal case:single fault raid group permanent sparing test. 
             */
            shaggy_test_single_fault_permanent_sparing(rg_config_p);

            /* Always refresh the extra disk info.
             */
            fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr, num_raid_groups);

            /* 2) normal case:double fault raid group permanent sparing test. 
             */
            shaggy_test_double_fault_permanent_sparing_r6_r10_r1(rg_config_p);
            
            /* We do not require to run these tests  for all raid groups.
             * We can select any one from the available list.
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5)
            {
                /* 3) Test make sure that no permanent sparing is take place if 
                 *    the same drive reinserted. 
                 */
                shaggy_test_wait_for_drive_reinsert(NULL, rg_config_p);

                /* 4) Test permanent spare is immediately swapped in for a
                 *    faulted drive.
                 */
                shaggy_test_immediate_spare_faulted_drive(rg_config_p);
                   
                /* 5) test the Automatic configuration of PVD as Spare. 
                 */
                shaggy_test_use_unused_drive_as_spare(rg_config_p);
                
                /* This is required only on Simulation */
                if (fbe_test_util_is_simulation())
                {
                    /* refresh extra drive info*/
                    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr,num_raid_groups);

                    /* 6) Test to check sparing of un-inited PVD.
                     */
                    shaggy_test_sparing_after_reboot(rg_config_p, rg_config_ptr);

                    /* refresh extra drive info*/
                    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr,num_raid_groups);

                    /* 7) test the hot spare configuration over reboot. */
                    shaggy_test_configure_hs_reboot(rg_config_ptr,0);

#if 0 /* Not applicable for Unity platform */
                    /* Test the case of pre-snake and snake river on each SP and
                     * swap in failing
                    */
                    if (fbe_test_sep_util_get_dualsp_test_mode())
                    {
                        /* The test case below is going to induce some error traces and clear them and so 
                         * first make sure there are no error traces */
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
                        shaggy_test_validate_snake_river_NDU_case(rg_config_p);
                    }
#endif
                }
                
            }
            fbe_test_sep_util_unconfigure_all_spares_in_the_system();
        }
    }
    fbe_api_sleep(5000); /* 5 Sec. delay */
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);

    /* Done.
     */
    return;
}
/*********************************** 
 * end shaggy_run_tests()
 ***********************************/

/*!****************************************************************************
 *          shaggy_add_pause_debug_hook()
 ******************************************************************************
 *
 * @brief   Add the requested debug hook to `pause'.
 *
 * @param   object_id - Object id for hook
 * @param   monitor_state - Monitor state to pause at
 * @param   monitor_substate - Monitor substate to pause at
 *
 * @return  status
 *
 * @author
 *  08/17/2012  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t shaggy_add_pause_debug_hook(fbe_object_id_t object_id,
                                                fbe_u32_t monitor_state, 
                                                fbe_u32_t monitor_substate)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_sp;
    fbe_bool_t                          b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Get the current target SP.
     */
    current_sp = fbe_api_transport_get_target_server();

    /* First set the hook on SPA.
     */
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    status = fbe_api_scheduler_add_debug_hook(object_id,
                                              monitor_state,
                                              monitor_substate,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we are in dualsp set the hook on SPB.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        status = fbe_api_scheduler_add_debug_hook(object_id,
                                              monitor_state,
                                              monitor_substate,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the target back to the original.
     */
    fbe_api_transport_set_target_server(current_sp);

    /* Return the execution status.
     */
    return status;
}
/***********************************
 * end shaggy_add_pause_debug_hook()
 ***********************************/

/*!****************************************************************************
 *          shaggy_wait_for_pause_debug_hook()
 ******************************************************************************
 *
 * @brief   Wait for the specified debug hook.
 *
 * @param   object_id - Object id for hook
 * @param   monitor_state - Monitor state to pause at
 * @param   monitor_substate - Monitor substate to pause at
 *
 * @return  status
 *
 * @author
 *  08/17/2012  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t shaggy_wait_for_pause_debug_hook(fbe_object_id_t object_id,
                                                fbe_u32_t monitor_state, 
                                                fbe_u32_t monitor_substate)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_sp;
    fbe_bool_t                          b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Get the current target SP.
     */
    current_sp = fbe_api_transport_get_target_server();

    /* First wait for the hook on SPA.
     */
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    status = fbe_test_wait_for_debug_hook(object_id, 
                                         monitor_state, 
                                         monitor_substate,
                                         SCHEDULER_CHECK_STATES,
                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                         0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we are in dualsp set the hook on SPB.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        status = fbe_test_wait_for_debug_hook(object_id, 
                                         monitor_state, 
                                         monitor_substate,
                                         SCHEDULER_CHECK_STATES,
                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                         0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the target back to the original.
     */
    fbe_api_transport_set_target_server(current_sp);

    /* Return the execution status.
     */
    return status;
}
/****************************************
 * end shaggy_wait_for_pause_debug_hook()
 ****************************************/

/*!****************************************************************************
 *          shaggy_remove_pause_debug_hook()
 ******************************************************************************
 *
 * @brief   Delete the requested debug hook to `pause'.
 *
 * @param   object_id - Object id for hook
 * @param   monitor_state - Monitor state to pause at
 * @param   monitor_substate - Monitor substate to pause at
 *
 * @return  status
 *
 * @author
 *  08/17/2012  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t shaggy_remove_pause_debug_hook(fbe_object_id_t object_id,
                                                fbe_u32_t monitor_state, 
                                                fbe_u32_t monitor_substate)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_sp;
    fbe_bool_t                          b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Get the current target SP.
     */
    current_sp = fbe_api_transport_get_target_server();

    /* First remove the hook on SPA.
     */
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    status = fbe_api_scheduler_del_debug_hook(object_id,
                                              monitor_state,
                                              monitor_substate,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we are in dualsp set the hook on SPB.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        status = fbe_api_scheduler_del_debug_hook(object_id,
                                              monitor_state,
                                              monitor_substate,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the target back to the original.
     */
    fbe_api_transport_set_target_server(current_sp);

    /* Return the execution status.
     */
    return status;
}
/**************************************
 * end shaggy_remove_pause_debug_hook()
 **************************************/

/*!****************************************************************************
 * shaggy_test_check_event_log_msg()
 ******************************************************************************
 *
 * @brief   Validate permanent spare message is present.
 *
 * @param   message_code - Event code to look for
 * @param   spare_drive_location - Pointer to spare drive location
 * @param   orig_drive_pvd_object_id - Object id of original drive 
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_check_event_log_msg(fbe_u32_t message_code,
                                     fbe_test_raid_group_disk_set_t *spare_drive_location,
                                     fbe_object_id_t orig_drive_pvd_object_id)
{
     fbe_status_t                    status;
    fbe_bool_t                      is_msg_present = FBE_FALSE;
    fbe_object_id_t                 selected_spare_drive_pvd_object_id;
    fbe_api_provision_drive_info_t  original_provision_drive_info;
    fbe_api_provision_drive_info_t  selected_spare_provision_drive_info;
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         


    /* set the max retries in a local variable. */
    max_retries = SHAGGY_TEST_MAX_RETRIES;

    /* Get the pvd object info. */
    fbe_api_provision_drive_get_info(orig_drive_pvd_object_id, &original_provision_drive_info);
    
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location->bus,spare_drive_location->enclosure,
                                                            spare_drive_location->slot,&selected_spare_drive_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the pvd object info. */
    fbe_api_provision_drive_get_info(selected_spare_drive_pvd_object_id, &selected_spare_provision_drive_info);

    /*  loop until the number of chunks marked NR is 0 for the drive that we are rebuilding. */
    for (retry_count = 0; retry_count < max_retries; retry_count++)
    {
    
        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                    &is_msg_present, 
                    message_code,
                    spare_drive_location->bus,
                    spare_drive_location->enclosure,
                    spare_drive_location->slot,
                    selected_spare_provision_drive_info.serial_num,
                    original_provision_drive_info.port_number,
                    original_provision_drive_info.enclosure_number,
                    original_provision_drive_info.slot_number,
                    original_provision_drive_info.serial_num); 

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (FBE_TRUE == is_msg_present)
        {
            return;
        }

        /*  wait before trying again. */
        fbe_api_sleep(SHAGGY_TEST_WAIT_SEC);
    }                     
    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");

    return;
}
/***************************************
 * end shaggy_test_check_event_log_msg()
 ***************************************/

/*!****************************************************************************
 * shaggy_test_degraded_permanent_sparing_test()
 ******************************************************************************
 *
 * @brief   Degraded permanent sparing test.
 *
 * @param   rg_config_p - Pointer to raid group test confiiguration
 * @param   position_p - pointer to array of positions to remove
 * @param   number_of_drives - Number of drive to remove
 * @param   b_is_automatic_spare_enabled    - FBE_TRUE - Automatic sparing is 
 *              enabled.  Do not configure a hot-spare.
 *                                          - FBE_FALSE - Need to configure a
 *              hot-spare.
 * @param   js_error_type_p - Pointer to job service status to update
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 * 5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
static void shaggy_test_degraded_permanent_sparing_test(fbe_test_rg_configuration_t *rg_config_p,
                                                 fbe_u32_t *position_p,
                                                 fbe_u32_t number_of_drives,
                                                 fbe_bool_t b_is_automatic_spare_enabled,
                                                 fbe_job_service_error_type_t *js_error_type_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       index = 0;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_notification_info_t         notification_info;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_object_id_t                 orig_pvd_id[2] = {0};
    fbe_object_id_t                 spare_pvd_id[2] = {0};

    /* Get the vd object-id for this position. 
     */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, *position_p, &vd_object_id);

    /* Step 1: We are attempting to validate that once a spare is available
     *         the virtual drive will immediately swap it in (without delay).
     *         But it takes some time to configure the hot-spare so we must
     *         allow some time before attempt to replace the drive. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 2: We have disabled automatic sparing and validate that the
     *         virtual drive send the `Failed' notificaiton since there
     *         are not hot-spares configured.
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                 FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                 FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL,
                                                 FBE_STATUS_OK,
                                                 FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Remove the drives.
     */
    for(index = 0; index < number_of_drives; index++)
    {
        /* get the pvd object id before we remove the drive. */
        shaggy_test_get_pvd_object_id_by_position(rg_config_p, position_p[index], &orig_pvd_id[index]);
        /* pull the specific drive from the enclosure. */
        shaggy_remove_drive(rg_config_p,
                            position_p[index],
                            &rg_config_p->drive_handle[position_p[index]]);
    }

    /* Step 4: Wait for the virtual drive to goto fail.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                          5000, 
                                          &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5: Register for the swap-in job notification.
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                 FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                 FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                 FBE_STATUS_OK,
                                                 FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* Step 6: Set a debug hook when virtual drive exits the `Failed' state
     */
    status = shaggy_add_pause_debug_hook(vd_object_id,
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Convert the required number of drives from `automatic' to
     *         hot-spare which will allow the failed drive to be 
     *         replaced.
     */
    if (b_is_automatic_spare_enabled == FBE_FALSE)
    {
        status = shaggy_test_config_hotspares(rg_config_p->extra_disk_set, number_of_drives);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 8: Verify the lifecycle state of the raid group object after drive 
     *         removal. 
     */
    shaggy_test_verify_raid_object_state_after_drive_removal(rg_config_p, position_p, number_of_drives);   

    /* Step 9: Wait for the notification from the spare library. 
     *          We expect success status.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                          30000, /* We set the replacement time to (5) seconds */
                                          &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, notification_info.notification_data.job_service_error_info.error_code);

    /* Step 10: Wait for the permanent spare to swap-in. 
     */
    for (index = 0; index < number_of_drives; index++)
    {
        shaggy_test_wait_permanent_spare_swap_in(rg_config_p, position_p[index]);
        
        spare_drive_location.bus = rg_config_p->rg_disk_set[position_p[index]].bus;
        spare_drive_location.enclosure = rg_config_p->rg_disk_set[position_p[index]].enclosure;
        spare_drive_location.slot = rg_config_p->rg_disk_set[position_p[index]].slot;

        status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                                spare_drive_location.enclosure,
                                                                spare_drive_location.slot,
                                                                &spare_pvd_id[index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        shaggy_test_check_event_log_msg(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                        &spare_drive_location,
                                        orig_pvd_id[index]);
    }

    /* Step 11: Wait for the debug hook.
     */
    status = shaggy_wait_for_pause_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Register for the swap-in job notification.
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                 FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                 FBE_NOTIFICATION_TYPE_SWAP_INFO,
                                                 FBE_STATUS_OK,
                                                 FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13: Remove the debug hook.
     */
    status = shaggy_remove_pause_debug_hook(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Wait for the notification from the virtual drive.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000,   /*! @note Although we want a small timeout (i.e. `immediate' trigger spare interval is 10 seconds.*/
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*! @note There is no job status associated with a swap notification.
     */
    MUT_ASSERT_INT_EQUAL(FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND, notification_info.notification_data.swap_info.command_type);
    MUT_ASSERT_INT_EQUAL(vd_object_id, notification_info.notification_data.swap_info.vd_object_id);
    MUT_ASSERT_INT_EQUAL(orig_pvd_id[0], notification_info.notification_data.swap_info.orig_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(spare_pvd_id[0], notification_info.notification_data.swap_info.spare_pvd_object_id);

    /* Step 15: Wait for the rebuild completion to permanent spare. 
     */
    for(index = 0; index < number_of_drives; index++)
    {
        sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position_p[index]);
    }

    sep_rebuild_utils_check_bits(rg_object_id);

    /* set the permanent spare trigger timer back to default. */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);
}
/******************************************************************************
 * end shaggy_test_degraded_permanent_sparing_test()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_single_fault_permanent_sparing()
 ******************************************************************************
 * @brief
 *  single fault permanent sparing test.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 * 5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
void shaggy_test_single_fault_permanent_sparing(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                       position[2] = {0, 1};
    fbe_u32_t                       num_drives_removed = 1;
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_raid_group_disk_set_t  removed_disk_set[2] = {0};

    mut_printf(MUT_LOG_TEST_STATUS, "**** Single fault permanent sparing test - Start. ****\n");

              
    mut_printf(MUT_LOG_TEST_STATUS, "****  Single fault permanent sparing for RAID-%d  ****",
                                     (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /* get the next random position. */
    shaggy_test_get_next_random_drive_position(rg_config_p, &position[0]);

    /* initialize the job service error. */
    job_service_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    shaggy_test_fill_removed_disk_set(rg_config_p,removed_disk_set,&position[0],num_drives_removed);

    /* start degraded permanent sparing test. */
    shaggy_test_degraded_permanent_sparing_test(rg_config_p,
                                                &position[0],
                                                num_drives_removed,
                                                FBE_FALSE,  /* Automatic sparing is not enabled*/
                                                &job_service_error_type);
    MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** Single fault permanent sparing for RAID %d completed successfully *****\n",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /*Reinsert the removed drive*/
    shaggy_test_reinsert_removed_disks(rg_config_p,removed_disk_set,&position[0],num_drives_removed);
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** Single fault permanent sparing test - Completed successfully ****\n");
    return;
}
/******************************************************************************
 * end shaggy_test_single_fault_permanent_sparing()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_sparing_after_reboot()
 ******************************************************************************
 * @brief
 *  Sparing after reboot test.
 *
 * @return None.
 *
 * @author
 *  09/25/2012 - Created. Prahlad Purohit
 ******************************************************************************/
void shaggy_test_sparing_after_reboot(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_test_rg_configuration_t *whole_rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       position;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_test_raid_group_disk_set_t  disk_set;
    fbe_bool_t                      b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();    

    mut_printf(MUT_LOG_TEST_STATUS, "**** Sparing after reboot test - Start. ****\n");

              
    mut_printf(MUT_LOG_TEST_STATUS, "****  Sparing after reboot for RAID-%d  ****",
                                     (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /* enable auto hot sparing with 60s trigger time. */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);    
    fbe_test_sep_util_update_permanent_spare_trigger_timer(60);

    /* get the next random position. */
    shaggy_test_get_next_random_drive_position(rg_config_p, &position);

    /* Get the vd object-id for this position. 
     */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    disk_set = rg_config_p->rg_disk_set[position];

    /* remove a drive from RG. */
    shaggy_remove_drive(rg_config_p, position, &rg_config_p->drive_handle[position]); 

    /* wait for the VD to go faulted and then restart sep. */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     SHAGGY_TEST_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    if(b_is_dualsp)
    {
        shaggy_restart_dualsp_sep(whole_rg_config_p);
    }
    else
    {
        shaggy_restart_sep(whole_rg_config_p);
    }

    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     SHAGGY_TEST_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* validate that spare swapped in. */
    shaggy_test_wait_permanent_spare_swap_in(rg_config_p, position);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Sparing after reboot for RAID %d completed successfully *****\n",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /*Reinsert the removed drive*/
    shaggy_test_reinsert_removed_disks(rg_config_p, &disk_set, &position, 1);

    /* wait for rebuild to complete. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);
    sep_rebuild_utils_check_bits(rg_object_id); 

    /* disable auto hot sparing as required by rest of the test. */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);    

    mut_printf(MUT_LOG_TEST_STATUS, "**** Sparing after reboot test - Completed successfully ****");

    return;
}
/******************************************************************************
 * end shaggy_test_sparing_after_reboot()
 ******************************************************************************/


/*!****************************************************************************
 * shaggy_test_double_fault_permanent_sparing_r6_r10_r1()
 ******************************************************************************
 * @brief
 *  double fault raid group permanent sparing test.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 * 5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
void shaggy_test_double_fault_permanent_sparing_r6_r10_r1(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                       position[2] = {0, 1};
    fbe_u32_t                       num_drives_removed = 1;
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_raid_group_disk_set_t  removed_disk_set[2] = {0};
    
    /* permanent sparing degraded - single/double fault test. */
    
    if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 ||
        rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10 )
    {
        num_drives_removed = 2;
    }
    else
    {
       /*Other type of RAID we will just return*/
        return;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** Double fault permanent sparing - Start ****\n");

    mut_printf(MUT_LOG_TEST_STATUS, "**** Double fault permanent sparing for RAID %d ****",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    shaggy_test_get_next_random_drive_position(rg_config_p, &position[0]);

    if(rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {    
        /* get the mirror position of the first randon drive. */
        shaggy_test_raid10_get_mirror_position(position[0], rg_config_p->width, &position[1]);
    }
    else
    {
        if(position[0] < (rg_config_p->width / 2)) 
        {
            position[1] = position[0] + (rg_config_p->width / 2);
        }
        else
        {
            position[1] = position[0] - (rg_config_p->width / 2);
        }
    }

    /* initialize the job service error. */
    job_service_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    shaggy_test_fill_removed_disk_set(rg_config_p,removed_disk_set,&position[0], num_drives_removed);
    
    /* start degraded permanent sparing test. */
    shaggy_test_degraded_permanent_sparing_test(rg_config_p,
                                                &position[0],
                                                num_drives_removed,
                                                FBE_FALSE,  /* Automatic sparing is not enabled*/
                                                &job_service_error_type);
    MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
    

    mut_printf(MUT_LOG_TEST_STATUS, "***** Double fault permanent sparing for RAID %d completed ****\n",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /*Reinsert the removed drive*/
    shaggy_test_reinsert_removed_disks(rg_config_p,removed_disk_set,&position[0],num_drives_removed);
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** Double fault permanent sparing - Completed ****\n");
    return;
}
/******************************************************************************
 * end shaggy_test_double_fault_permanent_sparing_r6_r10_r1()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_remove_drive()
 ******************************************************************************
 * @brief
 *  It is used to remove the specific drive in raid group.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_remove_drive(fbe_test_rg_configuration_t * rg_config_p,
                         fbe_u32_t position,
                         fbe_api_terminator_device_handle_t * out_drive_info_p)
{
    fbe_status_t                        status;
    fbe_object_id_t                     pvd_object_id;
    fbe_object_id_t                     vd_object_id;
    fbe_object_id_t                     rg_object_id;

    /* get the pvd object id before we remove the drive. */
    shaggy_test_get_pvd_object_id_by_position(rg_config_p, position, &pvd_object_id);
   
    mut_printf(MUT_LOG_TEST_STATUS, "%s:remove the drive port: %d encl: %d slot: %d",
                __FUNCTION__,
               rg_config_p->rg_disk_set[position].bus,
               rg_config_p->rg_disk_set[position].enclosure,
               rg_config_p->rg_disk_set[position].slot);

    if (fbe_test_util_is_simulation())
    {
        /* remove the drive. */
        status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position].bus,
                                             rg_config_p->rg_disk_set[position].enclosure,
                                             rg_config_p->rg_disk_set[position].slot,
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
            status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position].bus,
                                             rg_config_p->rg_disk_set[position].enclosure,
                                             rg_config_p->rg_disk_set[position].slot,
                                             &rg_config_p->peer_drive_handle[position]);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        /* remove the drive on actual hardware. */
        status = fbe_test_pp_util_pull_drive_hw(rg_config_p->rg_disk_set[position].bus,
                                   rg_config_p->rg_disk_set[position].enclosure,
                                   rg_config_p->rg_disk_set[position].slot);

     }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* get the vd object id. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    /* verify that the pvd and vd objects are in the FAIL state. */
    shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    return;
}
/******************************************************************************
 * end shaggy_remove_drive()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_remove_hs_drive()
 ******************************************************************************
 * @brief
 *  It is used to remove the host spare drive.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_remove_hs_drive(fbe_test_hs_configuration_t * hs_config_p,
                            fbe_api_terminator_device_handle_t * out_drive_info_p)
{
    fbe_status_t                        status;

    /* get the pvd object-id by location. */
    status = fbe_api_provision_drive_get_obj_id_by_location(hs_config_p->disk_set.bus,
                                                            hs_config_p->disk_set.enclosure,
                                                            hs_config_p->disk_set.slot,
                                                            &hs_config_p->hs_pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(hs_config_p->hs_pvd_object_id, FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:remove the drive, port %d, encl %d, drive %d",
                __FUNCTION__, hs_config_p->disk_set.bus, hs_config_p->disk_set.enclosure, hs_config_p->disk_set.slot);

    if (fbe_test_util_is_simulation())
    {
        /* remove the drive. */
        status = fbe_test_pp_util_pull_drive(hs_config_p->disk_set.bus,
                                             hs_config_p->disk_set.enclosure,
                                             hs_config_p->disk_set.slot,
                                             out_drive_info_p); 
    }
    else
    {

        /* remove the drive on actual hardware. */
        status = fbe_test_pp_util_pull_drive_hw(hs_config_p->disk_set.bus,
                                             hs_config_p->disk_set.enclosure,
                                             hs_config_p->disk_set.slot);

    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* verify that the pvd and vd objects are in the FAIL state. */
    shaggy_verify_sep_object_state(hs_config_p->hs_pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    return;
}
/******************************************************************************
 * end shaggy_remove_hs_drive()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_verify_raid_object_state_after_drive_removal()
 ******************************************************************************
 * @brief
 *  It is used to verify the raid group state after drive removal.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_verify_raid_object_state_after_drive_removal(fbe_test_rg_configuration_t  * rg_config_p,
                                                              fbe_u32_t * position_p,
                                                              fbe_u32_t num_drive_removal)
{
    fbe_status_t            status;
    fbe_object_id_t         rg_object_id;
    fbe_lifecycle_state_t   lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t               mirror_position;

    /* get raid group object-id. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);

    /* determine lifecycle state based on raid type and number of drive removal. */
    switch(rg_config_p->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            if(num_drive_removal >= 1)  {
               lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
            }
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            /* n - 1 failure support. */
            if(num_drive_removal < rg_config_p->width) {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_READY;
            }
            else {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
            }
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
       case FBE_RAID_GROUP_TYPE_RAID3:
            /* single drive failure raid group. */
            if(num_drive_removal == 1) {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_READY;
            }
            else {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
            }
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            /* n - 2 failure support if it is mirrored position. */
            if(num_drive_removal == 1) {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_READY;
            }
            else if(num_drive_removal == 2) {
                /* if both drives are mirror to each other than raid object remain in ready state. */
                shaggy_test_raid10_get_mirror_position(position_p[0], rg_config_p->width, &mirror_position);
                if(mirror_position == position_p[1]) {
                    lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_READY;
                }
                else {
                    lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
                }
            }
            else {
                lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
            }
            break;
       case FBE_RAID_GROUP_TYPE_RAID6:
           /* n - 2 failure support. */
           if(num_drive_removal <= 2) {
               lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_READY;
           }
           else {
               lifecycle_state_after_drive_removal = FBE_LIFECYCLE_STATE_FAIL;
           }
           break;
        default:
           status = FBE_STATUS_GENERIC_FAILURE;
           MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "RAID type is not expected");
    }

    shaggy_verify_sep_object_state(rg_object_id, lifecycle_state_after_drive_removal);
}
/******************************************************************************
 * end shaggy_test_verify_raid_object_state_after_drive_removal()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_setup_hot_spare()
 ******************************************************************************
 * @brief
 *  It is used to setup hot spare.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_setup_hot_spare(fbe_test_hs_configuration_t *hs_config_p)
{  
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_notification_info_t notification_info;

    /* get the object id of the pvd. */
    status = fbe_api_provision_drive_get_obj_id_by_location(hs_config_p->disk_set.bus,
                                                            hs_config_p->disk_set.enclosure,
                                                            hs_config_p->disk_set.slot,
                                                            &(hs_config_p->hs_pvd_object_id));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* Register for notification from the Configuration Service on this config change */
    status = fbe_test_common_set_notification_to_wait_for(hs_config_p->hs_pvd_object_id,
                                                 FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                 FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED,
                                                 FBE_STATUS_OK,
                                                 FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* create a pvd configured as a hot spare */
    fbe_test_sep_util_configure_pvd_as_hot_spare(hs_config_p->hs_pvd_object_id, NULL, 0, FBE_FALSE);

    /* wait for the notification from the Configuration Service */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                          30000, 
                                          &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the new pvd to become ready */
    status = fbe_api_wait_for_object_lifecycle_state(hs_config_p->hs_pvd_object_id,
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     20000, FBE_PACKAGE_ID_SEP_0); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:drive %d-%d-%d configured as hot spare, obj-id:0x%x.\n",
               __FUNCTION__,
               hs_config_p->disk_set.bus,
               hs_config_p->disk_set.enclosure,
               hs_config_p->disk_set.slot,
               hs_config_p->hs_pvd_object_id);

    return;
}
/******************************************************************************
 * end shaggy_test_setup_hot_spare()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_wait_permanent_spare_swap_in()
 ******************************************************************************
 * @brief
 *  It waits until permanent spare gets swap-in at specific edge index.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_wait_permanent_spare_swap_in(fbe_test_rg_configuration_t * rg_config_p,
                                              fbe_u32_t position)
{
    fbe_edge_index_t                edge_index;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_status_t                    status;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_u32_t                       bus, encl, slot;

    /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    /* get the edge index where the permanent spare gets swap-in. */
    fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);

    /* wait until spare gets swapped-in. */
    status = fbe_api_wait_for_block_edge_path_state(vd_object_id,
                                                    edge_index,
                                                    FBE_PATH_STATE_ENABLED,
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    SHAGGY_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    
    /* get the spare edge information. */
    status = fbe_api_get_block_edge_info(vd_object_id,
                                         edge_index,
                                         &edge_info,
                                         FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_location(edge_info.server_id, &bus, &encl, &slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* update the raid group configuration with swapped-in permanent spare. */
    rg_config_p->rg_disk_set[position].bus = bus;
    rg_config_p->rg_disk_set[position].enclosure = encl;
    rg_config_p->rg_disk_set[position].slot = slot;

    mut_printf(MUT_LOG_TEST_STATUS, "%s:permanent spare %d-%d-%d gets swapped-in, vd_obj_id:0x%x, hs_obj_id:0x%x",
               __FUNCTION__, bus, encl, slot, vd_object_id, edge_info.server_id);
}
/******************************************************************************
 * end shaggy_test_wait_permanent_spare_swap_in()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_wait_permanent_spare_swap_in()
 ******************************************************************************
 * @brief
 *  This test is used to verify the hot spare configuration works across the
 *  reboot.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the tests against.
 * @param   unused_drive_index - Index of unused drive
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void shaggy_test_configure_hs_reboot(fbe_test_rg_configuration_t * rg_config_p, 
                                            fbe_u32_t unused_drive_index)
{
    fbe_api_provision_drive_info_t  before_pvd_info;
    fbe_api_provision_drive_info_t  after_pvd_info;
    fbe_object_id_t                 hs_object_id;
    fbe_test_hs_configuration_t     hs_config;


    /* get the HS disk set */
    hs_config.disk_set = rg_config_p->extra_disk_set[unused_drive_index];

    /* setup a hot spare. */
    shaggy_test_setup_hot_spare(&hs_config);  // Changed from shaggy_hs_sas_520_config[0], which is a PVD already in a RG

    /* get the object id. */
    fbe_api_provision_drive_get_obj_id_by_location(hs_config.disk_set.bus,
                                                   hs_config.disk_set.enclosure,
                                                   hs_config.disk_set.slot,
                                                   &hs_object_id);

    /* get the pvd info before configuration of spare */
    fbe_api_provision_drive_get_info(hs_object_id, &before_pvd_info);

    /* restart the sp */
    shaggy_restart_sep(rg_config_p);

    shaggy_verify_sep_object_state(hs_object_id, FBE_LIFECYCLE_STATE_READY); // Add a wait here so the get info will succeed.

    /* get the pvd info after configuration of spare */
    fbe_api_provision_drive_get_info(hs_object_id, &after_pvd_info);

    /* compare the before and after reboot pvd config type. */
    MUT_ASSERT_INT_EQUAL(before_pvd_info.config_type, after_pvd_info.config_type);

}

/*!****************************************************************************
 * shaggy_test_validate_snake_river_NDU_case()
 ******************************************************************************
 * @brief
 *  This test is used to validate that sparing is rejected if one SP is running
 * snake river release and the other SP is running pre-snake release
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the tests against.
 *
 * @return None.
 *
 * @author
 *  09/09/2014 - Created. Ashok Tamilarasan
 ******************************************************************************/
static void shaggy_test_validate_snake_river_NDU_case(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_sim_transport_connection_target_t current_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t vd_object_id;
    fbe_u32_t position;
    fbe_test_raid_group_disk_set_t  disk_set;
    fbe_status_t status;
    fbe_u64_t sep_version;
    fbe_notification_info_t notification_info;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Starting..\n", __FUNCTION__);

    /* Change the peer SEP version pre-snake river to simulate a NDU case */
    /* During NDU, the Active SP will be running pre-snake and so call the passive SP
     * to set the SEP version 
     */
    current_sp = fbe_api_sim_transport_get_target_server();
    fbe_test_sep_get_active_target_id(&active_sp);
    passive_sp = ((active_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "Setting passive SP to pre-snake version...\n ");
    fbe_api_sim_transport_set_target_server(passive_sp);
    sep_version = FLARE_COMMIT_LEVEL_ROCKIES;
    fbe_api_database_set_peer_sep_version(&sep_version);
    fbe_api_sim_transport_set_target_server(active_sp);

    /* Initiate swap-ins */
    /* enable auto hot sparing with 60s trigger time. */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);    
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* get the next random position. */
    shaggy_test_get_next_random_drive_position(rg_config_p, &position);

    /* Get the vd object-id for this position. 
     */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    disk_set = rg_config_p->rg_disk_set[position];

    /* remove a drive from RG. */
    shaggy_remove_drive(rg_config_p, position, &rg_config_p->drive_handle[position]); 

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for VD Object ID 0x%08x to go to failed. \n", vd_object_id);
    /* wait for the VD to go faulted and then restart sep. */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     SHAGGY_TEST_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    /* Validate that the Swap-in fails */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                          FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                          FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                          FBE_STATUS_GENERIC_FAILURE,
                                                          FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for job service failure. \n");
    /* Wait for the notification from the spare library. 
     *          We expect error status.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, /* We set the replacement time to (5) seconds */
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED, notification_info.notification_data.job_service_error_info.error_code);

    /* Shut Down the active SP. Clear the error counters since we would have take some error traces above */
    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Shutting down SEP", __FUNCTION__);
    fbe_test_sep_util_destroy_neit_sep();        

    fbe_api_sim_transport_set_target_server(passive_sp);
    fbe_api_trace_clear_error_counter(FBE_PACKAGE_ID_SEP_0);
    /* Validate that hot spare swapped in */
     /* validate that spare swapped in. */
    shaggy_test_wait_permanent_spare_swap_in(rg_config_p, position);

    /* wait for rebuild to complete. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);
    sep_rebuild_utils_check_bits(rg_object_id); 

    fbe_api_sim_transport_set_target_server(active_sp);
    // Reloading SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reloading SEP", __FUNCTION__);    
    sep_config_load_sep_and_neit();

    fbe_api_sim_transport_set_target_server(current_sp);

}
/*!****************************************************************************
 * shaggy_test_get_pvd_object_id_by_position()
 ******************************************************************************
 * @brief
 *  It is used to get the pvd object-id for the specific position in rg.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_get_pvd_object_id_by_position(fbe_test_rg_configuration_t * rg_config_p,
                                               fbe_u32_t position,
                                               fbe_object_id_t * pvd_object_id_p)
{
    fbe_status_t status;

    /* get the pvd object-id for this position. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                            rg_config_p->rg_disk_set[position].enclosure,
                                                            rg_config_p->rg_disk_set[position].slot,
                                                            pvd_object_id_p);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *pvd_object_id_p);
}
/******************************************************************************
 * end shaggy_test_get_pvd_object_id_by_position()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_verify_sep_object_state()
 ******************************************************************************
 * @brief
 *  It is used to verify the object state.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_verify_sep_object_state(fbe_object_id_t object_id,
                                    fbe_lifecycle_state_t expected_state)
{
    fbe_status_t status;

    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* Verify the SEP objects is in expected lifecycle State  */
    status = fbe_api_wait_for_object_lifecycle_state(object_id,
                                                     expected_state,
                                                     SHAGGY_TEST_WAIT_MSEC,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: SEP object (%d) is in expected state (%s)",
               __FUNCTION__, object_id, fbe_api_state_to_string(expected_state));
    return;
}
/******************************************************************************
 * end shaggy_verify_sep_object_state()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_verify_pdo_state()
 ******************************************************************************
 * @brief
 *  It is used to verify the pdo state for the specific drive slot.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void  shaggy_verify_pdo_state(fbe_u32_t port_number,
                              fbe_u32_t enclosure_number,
                              fbe_u32_t slot_number,
                              fbe_lifecycle_state_t expected_state,
                              fbe_bool_t destroyed)
{
    fbe_object_id_t     object_id;
    fbe_status_t        status;

    /* Get the physical drive object id by location of the drive. */
    status = fbe_api_get_physical_drive_object_id_by_location(port_number,
                                                              enclosure_number,
                                                              slot_number,
                                                              &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (FBE_IS_TRUE (destroyed))
    {
        /* If destroyed set to TRUE then object id should be returned as Invalid. */
        MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: Physical Drive Object got destroyed.", __FUNCTION__);
    }
    else
    {
        /* Object id must be valid with all other lifecycle state. */
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: Physical drive object id = %d.", __FUNCTION__, object_id);

        /* Verify whether physical drive object is in expected State, it waits
         * for the few seconds if it PDO is not in expected state.
         */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                        expected_state, 
                                                        10000,
                                                        FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: PDO (%d) is in expected state (%s)",
                   __FUNCTION__, object_id, fbe_api_state_to_string(expected_state));
    }

    return;
}
/******************************************************************************
 * end shaggy_verify_pdo_state()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_get_next_random_drive_position()
 ******************************************************************************
 * @brief
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_get_next_random_drive_position(fbe_test_rg_configuration_t * rg_config_p,
                                                fbe_u32_t * position_p)
{
    fbe_u32_t rand_position;

    /* get the next random number. */
    rand_position = fbe_random();

    /* get the next random position in rand group. */
    rand_position = (rand_position % rg_config_p->width);
    *position_p = rand_position;
    return;
}
/******************************************************************************
 * end shaggy_test_get_next_random_drive_position()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_raid10_get_mirror_position()
 ******************************************************************************
 * @brief
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
void shaggy_test_raid10_get_mirror_position(fbe_u32_t orig_pos,
                                            fbe_u32_t width,
                                            fbe_u32_t * mirror_position_p)
{
    fbe_u32_t offset; 

   /* get the offset position of mirror for the raid10. */
   offset = width / 2;

   /* get the mirrored position of the first randon drive. */
   if(orig_pos < offset) {
       *mirror_position_p = orig_pos + offset;
   }
   else {
       *mirror_position_p = orig_pos - offset;
   }
   return;
}
/******************************************************************************
 * end shaggy_test_raid10_get_mirror_position()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_wait_for_drive_reinsert()
 ******************************************************************************
 * @brief
 *  This test is used to verify the reinsert of the removed drive within a minute time
 *  .
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/02/2011 - Created. Vishnu Sharma
 ******************************************************************************/
static void shaggy_test_wait_for_drive_reinsert(fbe_test_hs_configuration_t * hs_config_p,
                                                fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t                       status;
    fbe_object_id_t                    pvd_id;
    fbe_object_id_t                    rg_object_id;
    fbe_object_id_t                    vd_object_id;
    fbe_spare_drive_info_t             spare_drive_info;
    fbe_object_id_t                    pvd_object_id;
      
    /* Log the test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== shaggy: Validate no spare when drive re-inserted - Spare timer: %d seconds ==",
               60);
    
    /* set the permanent sparing timer to 60 second. */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(60);

    /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &vd_object_id);

    //shaggy_test_setup_hot_spare(hs_config_p);
    
    /* get the pvd object id before we remove the drive. */
    shaggy_test_get_pvd_object_id_by_position(rg_config_p, 0, &pvd_id);
    
    /* pull the specific drive from the enclosure. */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== shaggy: Validate no spare when drive re-inserted - Pull drive ==");
    shaggy_remove_drive(rg_config_p,
                        0,
                        &rg_config_p->drive_handle[0]);
    
    /* Wait for 30 seconds.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== shaggy: Validate no spare when drive re-inserted - Wait for: %d seconds ==",
               15);
    fbe_api_sleep (15000);
    
    /* Now re-insert drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== shaggy: Validate no spare when drive re-inserted - Re-insert drive ==");
    if (fbe_test_util_is_simulation())
    {
        /* reinsert the drive. */
        status = fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[0].bus,
                                                 rg_config_p->rg_disk_set[0].enclosure,
                                                 rg_config_p->rg_disk_set[0].slot,
                                                 rg_config_p->drive_handle[0]); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

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

            /* re-insert the drive. */
            status = fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[0].bus,
                                                 rg_config_p->rg_disk_set[0].enclosure,
                                                 rg_config_p->rg_disk_set[0].slot,
                                                 rg_config_p->drive_handle[0]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(rg_config_p->rg_disk_set[0].bus,
                                           rg_config_p->rg_disk_set[0].enclosure,
                                           rg_config_p->rg_disk_set[0].slot);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY,
                                                     50000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_test_sep_util_get_virtual_drive_spare_info(vd_object_id,&spare_drive_info);
    
    /* get the object id of the pvd. */
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_info.port_number,
                                                            spare_drive_info.enclosure_number,
                                                            spare_drive_info.slot_number,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL(pvd_id, pvd_object_id);
    
    /* wait for the rebuild completion to permanent spare. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, 0);

    sep_rebuild_utils_check_bits(rg_object_id);

    /* set the permanent sparing timer to 1 second. */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== shaggy: Validate no spare when drive re-inserted - Success.  Set spare timer to %d seconds ==",
               1);

    return;
}
/************************************************** 
 * end shaggy_test_wait_for_drive_reinsert()
 **************************************************/

/*!****************************************************************************
 * shaggy_test_config_hotspares()
 ******************************************************************************
 * @brief
 *  This test is used to init the hs config
 *  .
 *
 * @param None.
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t shaggy_test_config_hotspares(fbe_test_raid_group_disk_set_t *disk_set_p,
                                          fbe_u32_t no_of_spares)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_hs_configuration_t hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t hs_index = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "No of spares required for this raid group %d.",no_of_spares); 
        
    for(hs_index = 0; hs_index < no_of_spares; hs_index ++)
    {
       
        hs_config[hs_index].block_size = 520;
        hs_config[hs_index].disk_set = *(disk_set_p + hs_index) ;
        hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
    }

    status = fbe_test_sep_util_configure_hot_spares(hs_config,no_of_spares);
    
    return status;
}
/*************************************** 
 * end shaggy_test_config_hotspares()
 ***************************************/

/*!****************************************************************************
 * shaggy_test_use_unused_drive_as_spare()
 ******************************************************************************
 * @brief
 *  This test is used to verify that if no spare is bind in the system and 
 *  a drive goes down then system will select a drive from unused drive pool
 *  and replace that drive with failed drive.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/05/2011 - Created. Vishnu Sharma
 
******************************************************************************/
static void shaggy_test_use_unused_drive_as_spare(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_object_id_t                                   rg_object_id;
    fbe_object_id_t                                   vd_object_id;
    fbe_api_provisional_drive_get_spare_drive_pool_t  spare_drive_pool;
    fbe_job_service_error_type_t                      job_service_error_type;
    fbe_u32_t                                         position =0;
    fbe_u32_t                                         number_of_drives = 1;
    fbe_test_raid_group_disk_set_t                    removed_disk_set[2] = {0};

    mut_printf(MUT_LOG_TEST_STATUS, "**** Automatic Configuration of PVD Test - Started successfully ****\n");
    
    /* initialize the job service error. */
    job_service_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    
     /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id,0,
                                                              &vd_object_id);

     /* set the unused drive as spare flag of virtual drive object. */
    fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id,FBE_TRUE);

    status = fbe_test_sep_util_unconfigure_all_spares_in_the_system();  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_provision_drive_get_spare_drive_pool(&spare_drive_pool);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure that no hot spare configured in the system. */
    MUT_ASSERT_INT_EQUAL(spare_drive_pool.number_of_spare,0);
    
    shaggy_test_fill_removed_disk_set(rg_config_p,removed_disk_set,&position,number_of_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Degraded permanent sparing  - Started successfully ****\n");
    
    /* start degraded permanent sparing test. */ 
    shaggy_test_degraded_permanent_sparing_test(rg_config_p,
                                                &position,
                                                number_of_drives,
                                                FBE_TRUE,   /* Automatic sparing is enabled*/
                                                &job_service_error_type);
    MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Degraded permanent sparing  - Completed successfully ****\n");
   
    /*Reinsert the removed drive*/
    shaggy_test_reinsert_removed_disks(rg_config_p,removed_disk_set,&position,1);

    /* Reset the unused drive as spare flag of virtual drive object befor 
        leaving. */
    fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id,
                                                        FBE_FALSE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** Automatic Configuration of PVD Test - Completed successfully ****\n");

}
/********************************************* 
 * end shaggy_test_use_unused_drive_as_spare()
 *********************************************/

/*!****************************************************************************
 * shaggy_reinsert_removed_drive()
 ******************************************************************************
 * @brief
 *   This function fills the removed disk array by removed drives.
 *       
 * @param rg_config_p           - pointer to raid group
 * @param removed_disk_set_p    - pointer to array
 * @param position_p            - pointer to position
 * @param number_of_drives      - number of drives removed
 * 
 * @return none.
 *
 ******************************************************************************/
void shaggy_test_fill_removed_disk_set(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                       fbe_u32_t * position_p,
                                       fbe_u32_t number_of_drives)
{

    fbe_u32_t disk_index =0;

    for (disk_index = 0;disk_index <number_of_drives;disk_index++)
    {
        removed_disk_set_p[disk_index] = rg_config_p->rg_disk_set[position_p[disk_index]];        
    }

}
/*******************************************
 * end shaggy_test_fill_removed_disk_set()
 *******************************************/

/*!****************************************************************************
 * shaggy_reinsert_removed_drive()
 ******************************************************************************
 * @brief
 *   This function reinsert the removed  drive and waits for the logical and physical
 *   drive objects to be ready.  It does not check the states of the VD .
 *     *
 * @param rg_config_p           - pointer to raid group
 * @param removed_disk_set_p    - pointer to array
 * @param position_p            - pointer to position
 * @param number_of_drives      - number of drives removed
 * 
 * @return none.
 *
 ******************************************************************************/
void shaggy_test_reinsert_removed_disks(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                        fbe_u32_t * position_p,
                                        fbe_u32_t    number_of_drives)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t drive_index =0;
    
    for ( drive_index = 0; drive_index < number_of_drives; drive_index++ )
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting unused drive (%d_%d_%d). ==", 
            __FUNCTION__, removed_disk_set_p[drive_index].bus, removed_disk_set_p[drive_index].enclosure,
            removed_disk_set_p[drive_index].slot);

        /* insert unused drive back */
        if(fbe_test_util_is_simulation())
        {
            status = fbe_test_pp_util_reinsert_drive(removed_disk_set_p[drive_index].bus, 
                                            removed_disk_set_p[drive_index].enclosure, 
                                            removed_disk_set_p[drive_index].slot,
                                            rg_config_p->drive_handle[position_p[drive_index]]);
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_pp_util_reinsert_drive(removed_disk_set_p[drive_index].bus, 
                                                        removed_disk_set_p[drive_index].enclosure, 
                                                        removed_disk_set_p[drive_index].slot,
                                                        rg_config_p->peer_drive_handle[position_p[drive_index]]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(removed_disk_set_p[drive_index].bus, 
                                            removed_disk_set_p[drive_index].enclosure, 
                                            removed_disk_set_p[drive_index].slot);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* wait for the unused pvd object to be in ready state */
        status = fbe_test_sep_util_wait_for_pvd_state(removed_disk_set_p[drive_index].bus, 
                                removed_disk_set_p[drive_index].enclosure, 
                                removed_disk_set_p[drive_index].slot,
                                FBE_LIFECYCLE_STATE_READY,
                                SHAGGY_TEST_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

}
/*!***************************************************************************
 *          shaggy_test_immediate_spare_faulted_drive()
 ***************************************************************************** 
 * 
 * @brief   Validate that if a drive `Fails' (i.e. is not removed) that we do
 *          not wait the `needs replacement drive time' and instead immediately
 *          replace the failed drive (since it is not coming back).
 *
 * @return  None.
 *
 * @author
 *  08/29/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void shaggy_test_immediate_spare_faulted_drive(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       position[2] = {0, 1};
    fbe_test_raid_group_disk_set_t  removed_disk_set[2] = {0};
    fbe_u32_t                       index = 0;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_notification_info_t         notification_info;
    fbe_u32_t                       orig_bus;
    fbe_u32_t                       orig_enclosure;
    fbe_u32_t                       orig_slot;
    fbe_object_id_t                 orig_pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_object_id_t                 spare_pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                       bus;
    fbe_u32_t                       enclosure;
    fbe_u32_t                       slot;
    fbe_edge_index_t                edge_index;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_test_hs_configuration_t     hs_config;
    fbe_u32_t                       unused_drive_index = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "**** Immediately Spare Faulted Drive - Start. ****");

    mut_printf(MUT_LOG_TEST_STATUS, "**** Immediately Spare Faulted Drive for RAID-%d  ****",
                                     (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /* Step 1: We are attempting to validate that independent of the swap-in
     *         timer, the spare should swap-in immediately.  Therefore set the
     *         `need replacement drive' delay to a very large (5 minutes) value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* refresh extra drive info*/
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, 1);

    /* get the HS disk set */
    hs_config.disk_set = rg_config_p->extra_disk_set[unused_drive_index];
    status = fbe_api_provision_drive_get_obj_id_by_location(hs_config.disk_set.bus,
                                                            hs_config.disk_set.enclosure,
                                                            hs_config.disk_set.slot,
                                                            &spare_pvd_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, spare_pvd_id);

    /* setup a hot spare. */
    shaggy_test_setup_hot_spare(&hs_config);  

    /* get the next random position. */
    shaggy_test_get_next_random_drive_position(rg_config_p, &position[index]);

    shaggy_test_fill_removed_disk_set(rg_config_p,removed_disk_set,&position[index],1);

    /* start degraded permanent sparing test. */

    /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position[index], &vd_object_id);

    /* get the edge index where the permanent spare gets swap-in. */
    fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);

    /* Insert an error that results in  a `drive fault'.
     */

    /* get the pvd object id before we remove the drive. */
    shaggy_test_get_pvd_object_id_by_position(rg_config_p, position[index], &orig_pvd_id);

    /* Get the location to insert the drive fault.
     */
    orig_bus = rg_config_p->rg_disk_set[position[index]].bus;
    orig_enclosure = rg_config_p->rg_disk_set[position[index]].enclosure;
    orig_slot = rg_config_p->rg_disk_set[position[index]].slot;

    /* Step 2: Register for the swap-in job notification.
     */
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                 FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                 FBE_NOTIFICATION_TYPE_SWAP_INFO,
                                                 FBE_STATUS_OK,
                                                 FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Force a `drive fault' for this position.
     */
    status = fbe_test_sep_util_set_drive_fault(orig_bus, orig_enclosure, orig_slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Wait for the virtual drive to goto fail.
     */

    /* verify the lifecycle state of the raid group object after drive removal. */
    shaggy_test_verify_raid_object_state_after_drive_removal(rg_config_p, &position[index], 1);   

    /* We expected the permanent spare to swap-in immediately.
     */

    /* Step 4: Wait for the notification from the virtual drive.
     */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000,   /*! @note Although we want a small timeout (i.e. `immediate' trigger spare interval is 10 seconds.*/
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*! @note There is no job status associated with a swap notification.
     */
    MUT_ASSERT_INT_EQUAL(FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND, notification_info.notification_data.swap_info.command_type);
    MUT_ASSERT_INT_EQUAL(vd_object_id, notification_info.notification_data.swap_info.vd_object_id);
    MUT_ASSERT_INT_EQUAL(orig_pvd_id, notification_info.notification_data.swap_info.orig_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(spare_pvd_id, notification_info.notification_data.swap_info.spare_pvd_object_id);

    /* wait until spare gets swapped-in. */
    status = fbe_api_wait_for_block_edge_path_state(vd_object_id,
                                                    edge_index,
                                                    FBE_PATH_STATE_ENABLED,
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    10000);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    
    /* get the spare edge information. */
    status = fbe_api_get_block_edge_info(vd_object_id,
                                         edge_index,
                                         &edge_info,
                                         FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_location(edge_info.server_id, &bus, &enclosure, &slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* update the raid group configuration with swapped-in permanent spare. */
    rg_config_p->rg_disk_set[position[index]].bus = bus;
    rg_config_p->rg_disk_set[position[index]].enclosure = enclosure;
    rg_config_p->rg_disk_set[position[index]].slot = slot;

    /* validate the swap-in event */
    spare_drive_location.bus = rg_config_p->rg_disk_set[position[index]].bus;
    spare_drive_location.enclosure = rg_config_p->rg_disk_set[position[index]].enclosure;
    spare_drive_location.slot = rg_config_p->rg_disk_set[position[index]].slot;

    shaggy_test_check_event_log_msg(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                    &spare_drive_location,
                                    orig_pvd_id);

    /* wait for the rebuild completion to permanent spare. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position[index]);

    sep_rebuild_utils_check_bits(rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Immediately Spare Faulted Drive for RAID-%d completed successfully *****",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /* Clear the `drive fault' on the original drive.
     */
    status = fbe_test_sep_util_clear_drive_fault(orig_bus, orig_enclosure, orig_slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* wait for the rebuild completion to permanent spare. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position[index]);

    sep_rebuild_utils_check_bits(rg_object_id);

    /* Set the permanent spare replacement timer back to a small value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Immediately Spare Faulted Drive - Completed successfully ****");
    return;
}
/******************************************************************************
 * end shaggy_test_immediate_spare_faulted_drive()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_test_sparing_encryption_reboot_test()
 ******************************************************************************
 * @brief
 *  This function was writted to validate the fix for race condition between
 *  sparing job and database update job that happens during boot up of
 * encrypted system
 *
 * @return None.
 *
 * @notes:
 *       The idea to make sure database job for update config does not get started
 * while another job is outstanding and KMS waits in a tight loop only when the
 * config updates starts and not before. We achieve this by disabling job service
 * let the hot spare job be queued up and reboot the SP which will try to initiate
 * the database job but the hot spare one takes first and KMS handles it appropriately
 *
 * @author
 *  07/31/2014 - Created. Ashok Tamilarasan
 * 
 ******************************************************************************/
static void shaggy_test_sparing_encryption_reboot_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t rg_object_id;
    fbe_object_id_t vd_object_id;
    fbe_u32_t position;
    fbe_test_raid_group_disk_set_t  disk_set;
    fbe_status_t status;


    mut_printf(MUT_LOG_TEST_STATUS, " Starting ...%s...\n", __FUNCTION__);

    /* enable auto hot sparing with 60s trigger time. */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);    
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

        /* Disable the Job */
    fbe_test_sep_util_disable_creation_queue(FBE_OBJECT_ID_INVALID);

    /* get the next random position. */
    shaggy_test_get_next_random_drive_position(rg_config_p, &position);

    /* Get the vd object-id for this position. 
     */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    disk_set = rg_config_p->rg_disk_set[position];

    /* remove a drive from RG. */
    shaggy_remove_drive(rg_config_p, position, &rg_config_p->drive_handle[position]); 

    /* wait for the VD to go faulted and then restart sep. */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     SHAGGY_TEST_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

     mut_printf(MUT_LOG_TEST_STATUS, " %s Rebooting SPB...\n", __FUNCTION__);
    /* Reboot the peer */
    fbe_test_common_reboot_sp_neit_sep_kms(FBE_SIM_SP_B, NULL, NULL, NULL);

    /* Enable the job */
    fbe_test_sep_util_enable_creation_queue(FBE_OBJECT_ID_INVALID);

    /* Validate that hot spare swapped in */
     /* validate that spare swapped in. */
    shaggy_test_wait_permanent_spare_swap_in(rg_config_p, position);

    mut_printf(MUT_LOG_TEST_STATUS, "**** Sparing after reboot for RAID %d completed successfully *****\n",
                                    (rg_config_p->raid_type == 2) ? 10 : rg_config_p->raid_type);

    /*Reinsert the removed drive*/
    shaggy_test_reinsert_removed_disks(rg_config_p, &disk_set, &position, 1);

    /* wait for rebuild to complete. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position);
    sep_rebuild_utils_check_bits(rg_object_id); 

    /* disable auto hot sparing as required by rest of the test. */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);    


    return;
}
/******************************************************************************
 * end shaggy_test_sparing_encryption_reboot_test()
 ******************************************************************************/

/*!****************************************************************************
 * shaggy_encryption_run_tests()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  4/10/2010 - Created. Dhaval Patel
 *   5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
void shaggy_encryption_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   num_raid_groups = 0;
    fbe_u32_t                   index = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...\n", __FUNCTION__);

    /* Disable automatic sparing.
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* For all raid groups.
     */
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        /* Only run test on enabled raid groups.
         */
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {

            /* 1) normal case:single fault raid group permanent sparing test. 
             */
            shaggy_test_single_fault_permanent_sparing(rg_config_p);

            /* Always refresh the extra disk info.
             */
            fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr, num_raid_groups);

            /* 2) normal case:double fault raid group permanent sparing test. 
             */
            shaggy_test_double_fault_permanent_sparing_r6_r10_r1(rg_config_p);
            
            /* We do not require to run these tests  for all raid groups.
             * We can select any one from the available list.
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5)
            {
                /* 3) Test make sure that no permanent sparing is take place if 
                 *    the same drive reinserted. 
                 */
                shaggy_test_wait_for_drive_reinsert(NULL, rg_config_p);

                /* 4) Test permanent spare is immediately swapped in for a
                 *    faulted drive.
                 */
                shaggy_test_immediate_spare_faulted_drive(rg_config_p);
                   
                /* 5) test the Automatic configuration of PVD as Spare. 
                 */
                shaggy_test_use_unused_drive_as_spare(rg_config_p);
 
#if 0
                /* This is required only on Simulation */
                if (fbe_test_util_is_simulation())
                {
                    /* refresh extra drive info*/
                    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr,num_raid_groups);

                    /* 6) Test to check sparing of un-inited PVD.
                     */
                    shaggy_test_sparing_after_reboot(rg_config_p, rg_config_ptr);

                    /* refresh extra drive info*/
                    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_ptr,num_raid_groups);

                    /* 7) test the hot spare configuration over reboot. */
                    shaggy_test_configure_hs_reboot(rg_config_ptr,0);
                }
#endif
                
            }
            fbe_test_sep_util_unconfigure_all_spares_in_the_system();
        }
    }

    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
        /* This test validates the race condition between reboot and sparing
         * kicking in at the time of reboot
         */
        shaggy_test_sparing_encryption_reboot_test(rg_config_ptr);
    }
    fbe_api_sleep(5000); /* 5 Sec. delay */
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);

    /* Done.
     */
    return;
}
/*********************************** 
 * end shaggy_encryption_run_tests()
 ***********************************/

/*!****************************************************************************
 * shaggy_encryption_dualsp_test()
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
void shaggy_encryption_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &shaggy_raid_group_config[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,shaggy_encryption_run_tests,
                                   SHAGGY_TEST_LUNS_PER_RAID_GROUP,
                                   SHAGGY_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end shaggy_encryption_dualsp_test()
 ******************************************************************************/


/*!**************************************************************
 * shaggy_encryption_dualsp_setup()
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
void shaggy_encryption_dualsp_setup(void)
{
	fbe_status_t status;

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        
        rg_config_p = &shaggy_raid_group_config[0];
        

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

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
    
        /* After sep is loaded setup notifications
         */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dual SP test*/); 

		//fbe_api_enable_system_encryption();
        sep_config_load_kms_both_sps(NULL);

		status = fbe_test_sep_util_enable_kms_encryption();
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end shaggy_encryption_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * shaggy_encryption_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the shaggy test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/30/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/

void shaggy_encryption_dualsp_cleanup(void)
{
    /* Destroy semaphore
     */
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test */);

    fbe_test_sep_util_print_dps_statistics();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms_both_sps();

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
 * end shaggy_encryption_dualsp_cleanup()
 ******************************************/



/*************************
 * end file shaggy_test.c
 *************************/





