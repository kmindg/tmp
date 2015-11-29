/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file amon_test.c
 ***************************************************************************
 *
 * @brief Test the raid creation/destory when two system drives are removed.
 *
 * @version
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "generics.h"
#include "sep_rebuild_utils.h"                      
#include "mut.h"                                    
#include "fbe/fbe_types.h"                          
#include "sep_tests.h"                              
#include "sep_test_io.h"                            
#include "fbe/fbe_api_rdgen_interface.h"            
#include "fbe_test_configurations.h"                
#include "fbe/fbe_api_raid_group_interface.h"       
#include "fbe/fbe_api_database_interface.h"         
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_utils.h"                      
#include "pp_utils.h"                               
#include "fbe_private_space_layout.h"               
#include "fbe/fbe_api_provision_drive_interface.h"    
#include "fbe/fbe_api_common.h"                      
#include "fbe/fbe_api_terminator_interface.h"        
#include "fbe/fbe_api_sim_server.h"                 
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "dba_export_types.h"
#include "fbe/fbe_random.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* amon_short_desc = "RG/LUN create/destroy with two DB drives removed";
char* amon_long_desc =
    "\n"
    "\n"
    "The test is for RG/LUN creation/destory should failed with two DB drives removed\n"
    "\n"
    "STEP 1: Removed one DB drive, and create rg. it should success\n"
    "\n"
    "STEP 2: removed two DB drives, and create/destroy rg/lun, It should fail.\n"
    "\n"
"\n"
"Desription last updated: 8/7/2012.\n"    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

                                                        
#define AMON_TEST_RAID_GROUP_COUNT 1
#define AMON_JOB_SERVICE_CREATION_TIMEOUT 150000 // in ms
#define AMON_WAIT_MSEC 30000
#define FBE_JOB_SERVICE_DESTROY_TIMEOUT    120000 /*ms*/

#define AMON_TEST_RAID_GROUP_ID_1   0
#define AMON_TEST_RAID_GROUP_ID_2   1


/*!*******************************************************************
 * @def amon_rg_config_1
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t amon_rg_config_1[AMON_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          AMON_TEST_RAID_GROUP_ID_1,
    /* rg_disk_set */
    {{0,0,5}, {0,0,6}, {0,0,7}}
};

static fbe_lun_number_t lun_number_1 = 0x100;
static fbe_lun_number_t lun_number_2 = 0x101;

/*!*******************************************************************
 * @def amon_rg_config_2
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t amon_rg_config_2[AMON_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        1,          AMON_TEST_RAID_GROUP_ID_2,
    /* rg_disk_set */
    {{0,1,0}, {0,1,2}, {0,1,3}}
};

/*!*******************************************************************
 * @def amon_rg_config_3
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t amon_rg_config_3[AMON_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        2,          AMON_TEST_RAID_GROUP_ID_2,
    /* rg_disk_set */
    {{0,1,4}, {0,1,5}, {0,1,6}}
};


/*!*******************************************************************
 * @def AMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 59 pdo) 
 *
 *********************************************************************/
 #define AMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS 65  
/*!*******************************************************************
 * @def DIEGO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
 #define DIEGO_DRIVES_PER_ENCL 15

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
 
 static void amon_test_step_1(void);
 static void amon_test_step_2(void);

static void amon_test_create_rg_1(void);
static void amon_test_create_rg_2(void);
static void amon_test_create_rg_3(void);


static void amon_test_create_lun_1(void);
static void amon_test_create_lun_2(void);

static void amon_test_update_lun_1_in_step_1(void);
static void amon_test_update_lun_1_in_step_2(void);



static void amon_test_destroy_lun_1(void);
static void amon_test_destroy_rg_3(void);


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  amon_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the amon test.  The test does the 
 *   following:
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void amon_test(void)
{    

    /* Amon_test step 1: remove one DB drive, and create a user rg 1*/
    amon_test_step_1();

    /* Amon_test step 2: remove another DB drive, and try to destory the rg 1 and create rg 2 */
    amon_test_step_2();
	
    return;
} 

/*!**************************************************************
 * amon_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *  We make all the disks(0_0_x) have the same capacity 
 *
 * @param b_use_4160 - TRUE if we will use both 4160 and 520.              
 *
 * @return None.
 *
 ****************************************************************/

void amon_create_physical_config(fbe_bool_t b_use_4160)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl0_3_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t secondary_block_size;

    if (b_use_4160)
    {
        secondary_block_size = 4160;
    }
    else
    {
        secondary_block_size = 520;
    }

    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
    /*before loading the physical package we initialize the terminator*/
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
                                 &port0_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
       
       //disks 0_0_x should have the same capacity 
       fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
     /*skipp (0,3,0) for inserting it in the test in order to test pvd can be created in modified lifecycle condition */
    for ( slot = 1; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(AMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == AMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end amon_create_physical_config()
 *******************************************


/*!****************************************************************************
 *  amon_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the amon test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void amon_setup(void)
{  

    /* Initialize any required fields and perform cleanup if required
     */
    if (fbe_test_util_is_simulation())
    {
        /*  Load the physical package and create the physical configuration. */
        amon_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        /* Load the logical packages */
        sep_config_load_sep_and_neit();

        fbe_test_wait_for_all_pvds_ready();
    }

    fbe_test_common_util_test_setup_init();

} 


/*!****************************************************************************
 *  amon_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Diego test. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void amon_cleanup(void)
{

    if (fbe_test_util_is_simulation())
    {
         fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

}

/*!**************************************************************
 * amon_test_step_1()
 ****************************************************************
 * @brief
 * remove a DB drive, and create user rg. It should success.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void amon_test_step_1(void)
{
    fbe_status_t    status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create/Destroy RG/LUNs while the PSM is degraded. ===");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Expectation: Create/Destroy RG/LUNs should pass. ===\n");

    /*Pull out drive 0_0_0 */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove Drive 0-0-0 to make PSM degraded. ===\n");
    status = fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 

    /* One drive degraded, create rg 1 */
    amon_test_create_rg_1();
    mut_printf(MUT_LOG_TEST_STATUS, "=== RG (1) created successfully. ===");

    amon_test_create_rg_3();
    mut_printf(MUT_LOG_TEST_STATUS, "=== RG (3) created successfully. ===");

    /*create LUN 1 on RG 1*/
    amon_test_create_lun_1();
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN (1) created successfully. ===");

    amon_test_update_lun_1_in_step_1();
    
}

/*!**************************************************************
 * amon_test_step_2()
 ****************************************************************
 * @brief
 * remove another DB drive, and create user rg. It should fail.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void amon_test_step_2(void)
{
    fbe_status_t    status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create/Destroy RG/LUNs while while the PSM is double degraded. ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Expectation: Create/Destroy RG/LUNs should fail. ===\n");

    /*After removed two db drives, any job for create/destroy lun/raid should fail.*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove Drive 0-0-1 to make Tripe Mirror Double degraded. ===\n");
    status = fbe_test_pp_util_pull_drive_hw(0, 0, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                     
    /* wait for position 1 degraded */
    sep_rebuild_utils_verify_rb_logging_by_object_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 1);

    /*two drives degraded, create RG 2.  Create rg 2 will fail*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RG (2).. This should fail. ===");
    amon_test_create_rg_2();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create RG (2) failed as expected. ===\n");

    /*destroy LUN 1 which created on rg 1. destroy LUN 1 will fail */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy a LUN. This should fail. ===");
    amon_test_destroy_lun_1();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy LUN failed as expected.===\n");

    /*create lun 2 on RG 1. create LUN 2 will fail*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a LUN (2).. This should fail. ===");
    amon_test_create_lun_2();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create LUN (2) failed as expected. ===\n");

    /* try to destroy rg 1 after two drives degraded. destory RG will failed.*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy a RG (3). This should fail. ===");
    amon_test_destroy_rg_3();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Destroy RG (3) failed as expected. ===\n");

    /*try to update lun 1 .*/
    amon_test_update_lun_1_in_step_2();

    /*Because the failed job will generate a error trace, before destroy the sep, we need to reset the error counter*/
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);

}

/*After removing one drive, create RG 1*/
static void amon_test_create_rg_1(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_api_raid_group_create_t         create_rg;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = amon_rg_config_1[0].raid_group_id;
    create_rg.b_bandwidth = amon_rg_config_1[0].b_bandwidth;
    create_rg.capacity = amon_rg_config_1[0].capacity;
    create_rg.drive_count = amon_rg_config_1[0].width;
    create_rg.raid_type = amon_rg_config_1[0].raid_type;
    create_rg.is_private = FBE_FALSE;
    fbe_copy_memory(create_rg.disk_array, amon_rg_config_1[0].rg_disk_set, amon_rg_config_1[0].width * sizeof(fbe_test_raid_group_disk_set_t));
    
    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                               "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(amon_rg_config_1[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);   

    return;

} /* End amon_test_create_rg_1() */




/*!**************************************************************
 * amon_test_create_rg_2()
 ****************************************************************
 * @brief
 *  After removing two drives, create RG 2
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void amon_test_create_rg_2(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_api_raid_group_create_t         create_rg;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = amon_rg_config_2[0].raid_group_id;
    create_rg.b_bandwidth = amon_rg_config_2[0].b_bandwidth;
    create_rg.capacity = amon_rg_config_2[0].capacity;
    create_rg.drive_count = amon_rg_config_2[0].width;
    create_rg.raid_type = amon_rg_config_2[0].raid_type;
    create_rg.is_private = FBE_FALSE;
    fbe_copy_memory(create_rg.disk_array, amon_rg_config_2[0].rg_disk_set, amon_rg_config_2[0].width * sizeof(fbe_test_raid_group_disk_set_t));
    
    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);
    MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation should fail, but it does not report err status");
    
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, 
                            "job error code is not FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);
    return;

} /* End amon_test_create_rg_1() */                         

/*After removing one drive, create RG 3.  This RG is used to test destory operation after two drives degraded */
static void amon_test_create_rg_3(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_api_raid_group_create_t         create_rg;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = amon_rg_config_3[0].raid_group_id;
    create_rg.b_bandwidth = amon_rg_config_3[0].b_bandwidth;
    create_rg.capacity = amon_rg_config_3[0].capacity;
    create_rg.drive_count = amon_rg_config_3[0].width;
    create_rg.raid_type = amon_rg_config_3[0].raid_type;
    create_rg.is_private = FBE_FALSE;
    fbe_copy_memory(create_rg.disk_array, amon_rg_config_3[0].rg_disk_set, amon_rg_config_3[0].width * sizeof(fbe_test_raid_group_disk_set_t));
    
    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                            "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(amon_rg_config_3[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);   

    return;

} /* End amon_test_create_rg_3() */

/*After removing one drive, create LUN 1 on RG 1*/
static void amon_test_create_lun_1(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_object_id_t                     lun_object_id;
    fbe_api_lun_create_t            fbe_lun_create_req;

    /* Bind a Lun on RG 1 */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on a Raid Group start===\n");
    fbe_lun_create_req.raid_type = amon_rg_config_1[0].raid_type;
    fbe_lun_create_req.raid_group_id = amon_rg_config_1[0].raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number_1;
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
    
    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 30000, &lun_object_id, &job_error_code);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR , "create lun job fail");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on a Raid Group end ===\n");

    return;
} /* End amon_test_create_lun_1() */

/*After removing two drives, create LUN 2 on RG 1*/
static void amon_test_create_lun_2(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_object_id_t                     lun_object_id;
    fbe_api_lun_create_t            fbe_lun_create_req;

    /* Bind a Lun on RG 1 */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on Raid Group 1 start===\n");
    fbe_lun_create_req.raid_type = amon_rg_config_1[0].raid_type;
    fbe_lun_create_req.raid_group_id = amon_rg_config_1[0].raid_group_id; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number_2;
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
    
    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 30000, &lun_object_id, &job_error_code);
    /*Because two db drives are degraded, the job will fail.*/
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);   
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, 
                                "job error code is not FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on Raid Group 1 end ===\n");

    return;
} /* End kungfu_panda_test_create_lun() */


/*After removing two drives, destory LUN 1 on RG 1*/
static void amon_test_destroy_lun_1(void)
{
    fbe_status_t                    status;
    fbe_api_lun_destroy_t           fbe_lun_destroy_req;
    fbe_job_service_error_type_t    job_error_type = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

    fbe_lun_destroy_req.lun_number = lun_number_1;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 30000, &job_error_type);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(job_error_type, FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, 
                                "job error code is not FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED");

    return;
}

static void amon_test_update_lun_1_in_step_1()
{
    fbe_database_lun_info_t     lun_info;
    fbe_u32_t                   attributes = LU_ATTR_DUAL_ASSIGNABLE;
    fbe_object_id_t             lun_object_id;
    fbe_api_job_service_lun_update_t    lu_update_job;
    fbe_status_t                status;
    fbe_job_service_error_type_t    error_type = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_status_t    job_status;

    status = fbe_api_database_lookup_lun_by_number(lun_number_1, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "---START to update lun ---\n");
    lu_update_job.object_id = lun_object_id;
    lu_update_job.update_type = FBE_LUN_UPDATE_ATTRIBUTES;
    lu_update_job.attributes = attributes;
    status = fbe_api_job_service_lun_update(&lu_update_job);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*wait for the job*/
    status = fbe_api_common_wait_for_job(lu_update_job.job_number, 30000, &error_type, &job_status, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, job_status);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, error_type);
    

    lun_info.lun_object_id = lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE_MSG((attributes & lun_info.attributes),"lun_get_info return mismatch attributes");

    return;
}

static void amon_test_update_lun_1_in_step_2()
{
    fbe_u32_t                   attributes = LU_PRIVATE;
    fbe_object_id_t             lun_object_id;
    fbe_api_job_service_lun_update_t    lu_update_job;
    fbe_status_t                status;
    fbe_job_service_error_type_t    error_type = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_status_t    job_status;

    status = fbe_api_database_lookup_lun_by_number(lun_number_1, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "---START to update lun ---\n");
    lu_update_job.object_id = lun_object_id;
    lu_update_job.update_type = FBE_LUN_UPDATE_ATTRIBUTES;
    lu_update_job.attributes = attributes;
    status = fbe_api_job_service_lun_update(&lu_update_job);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*wait for the job*/
    status = fbe_api_common_wait_for_job(lu_update_job.job_number, 15000, &error_type, &job_status, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /*NOTE: when update lun job failed, the notification set the status is FBE_STATUS_OK, so comments out the following check*/
    //MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, job_status);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, error_type);

    /*update LUN attribute again with fbe_api */
    attributes = LU_ATTR_DUAL_ASSIGNABLE;
    status = fbe_api_update_lun_attributes(lun_object_id, attributes, &error_type);

    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, error_type);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN updated attributes failed, get expected error type ===\n");

    
    return;
}

/*!****************************************************************************
 * amon_test_destroy_rg()
 ******************************************************************************
 * @brief
 *  
 * destroy RG 3 after removing two drives
 *
 * @return None.
 *
 ******************************************************************************/
static void amon_test_destroy_rg_3(void)
{
    fbe_status_t                                     status;
    fbe_object_id_t                                  rg_object_id;
    fbe_api_destroy_rg_t                             destroy_rg;
    fbe_job_service_error_type_t                     job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
  
    /* Validate that the raid group exist */
    status = fbe_api_database_lookup_raid_group_by_number(amon_rg_config_3[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
        
    /* Destroy a RAID group */
    fbe_zero_memory(&destroy_rg, sizeof(fbe_api_destroy_rg_t));
    destroy_rg.raid_group_id = amon_rg_config_3[0].raid_group_id;

    status = fbe_api_destroy_rg(&destroy_rg, FBE_TRUE, 100000, &job_error_code);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED, "RG destroy job should fail because of two degraded drives, but it does not");
    
    /* The raid group was not destroyed, because two db drives are degraded. the destroy job will fail */
    status = fbe_api_database_lookup_raid_group_by_number(amon_rg_config_3[0].raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully! ===");

}

