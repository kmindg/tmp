/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file phoenix_test.c
 ***************************************************************************
 *
 * @brief Test the raid creation/destory when two system drives are removed.
 *
 * @version
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

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


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* phoenix_short_desc = "Test system object recreation and NDB system LUN";
char* phoenix_long_desc =
    "\n"
    "\n"
    "Test system object recreation and NDB system LUN\n"
    "\n"
    "STEP 1: Set system object need to be recreated (one PVD and one RAID)\n"
    "\n"
    "STEP 2: reboot the system, check the recreated object\n"
    "\n"
    "STEP 3: mark the system LUN(PSM/vault) to be NDB\n"
    "\n"
    "STEP 4: reboot the system, check the NDB lun object\n"

    "\n"
"\n"
"Desription last updated: 8/7/2012.\n"    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

                                                        
#define PHOENIX_TEST_RAID_GROUP_COUNT 1
#define PHOENIX_JOB_SERVICE_CREATION_TIMEOUT 150000 // in ms
#define PHOENIX_WAIT_MSEC 30000
#define FBE_JOB_SERVICE_DESTROY_TIMEOUT    120000 /*ms*/

#define PHOENIX_TEST_RAID_GROUP_ID_1   0

/*!*******************************************************************
 * @def phoenix_test_rdgen_io_context
 *********************************************************************
 * @brief   This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t phoenix_test_rdgen_io_context[1];


/*!*******************************************************************
 * @def phoenix_rg_config_1
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t phoenix_rg_config_1[PHOENIX_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          PHOENIX_TEST_RAID_GROUP_ID_1,
    /* rg_disk_set */
    {{0,0,5}, {0,0,6}, {0,0,7}}
};


/*!*******************************************************************
 * @def PHOENIX_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
 #define PHOENIX_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 /* should be 66, leave one disk to be inserted in create RG_2*/
/*!*******************************************************************
 * @def DIEGO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
 #define DIEGO_DRIVES_PER_ENCL 15

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
static void phoenix_reboot_sp(void);
static void phoenix_test_create_rg_1(void);
static void phoenix_test_write_nonzero_pattern(fbe_object_id_t object_id);
static void phoenix_test_read_nonzero_pattern(fbe_object_id_t object_id);
static void phoenix_test_read_zero_pattern(fbe_object_id_t lun_object_id);


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  phoenix_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the phoenix test.  The test does the 
 *   following:
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void phoenix_test(void)
{    
    fbe_status_t                            status;
    fbe_database_system_object_recreate_flags_t   system_object_op_flags;
    fbe_api_terminator_device_handle_t drive_handle;

    /*********************************Test 1 ***********************************/    
    mut_printf(MUT_LOG_TEST_STATUS, "Test 1: mark PVD0, Vault RG, PSM LUN recreated\n");

    /*==== Set the recreate or NDB flags by FBE API ====*/
    /*--mark PSM LUN to be recreated with NDB flags --*/    
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM] |= FBE_SYSTEM_OBJECT_OP_NDB_LUN;
    system_object_op_flags.valid_num++;
    /*--mark PVD_0 to be recreated--*/
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    system_object_op_flags.valid_num++;
    /*--mark VAULT RG to be recreated--*/
    //system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    //system_object_op_flags.valid_num++;

    /*===== persist the flags to raw mirror ====*/
    status = fbe_api_database_persist_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "mark system object recreation flags successfully\n");

    /*==== Check the recreate or NDB flags which just set ====*/
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    status = fbe_api_database_get_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(system_object_op_flags.valid_num, 0);
    MUT_ASSERT_INT_EQUAL(system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM]&FBE_SYSTEM_OBJECT_OP_RECREATE, FBE_SYSTEM_OBJECT_OP_RECREATE);
    MUT_ASSERT_INT_EQUAL(system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM]&FBE_SYSTEM_OBJECT_OP_NDB_LUN, FBE_SYSTEM_OBJECT_OP_NDB_LUN);
    MUT_ASSERT_INT_EQUAL(system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0]&FBE_SYSTEM_OBJECT_OP_RECREATE, FBE_SYSTEM_OBJECT_OP_RECREATE);
    mut_printf(MUT_LOG_TEST_STATUS, "Check the recreation flags successfully\n");

    /*==== Before reboot, do write and read check with PSM LUN ====*/
    phoenix_test_write_nonzero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM);

       
    /*==== Reboot and wait for database service ready ====*/
    phoenix_reboot_sp();


    /*==== Verify the PVD_0 and VAULT RG are ready ====*/
    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*==== After reboot, do read check with PSM LUN ====*/
    phoenix_test_read_nonzero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM);
    mut_printf(MUT_LOG_TEST_STATUS, "write and read check pass\n");

    /*********************************Test 2***********************************/ 
    mut_printf(MUT_LOG_TEST_STATUS,"Test 2: Mark Vault LUN recreated with NDB flags\n");

    /*==== Set the recreate or NDB flags by FBE API ====*/
    /*--mark PSM LUN to be recreated with NDB flags --*/    
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    system_object_op_flags.valid_num++;
    
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    system_object_op_flags.valid_num++;    
    /*===== persist the flags to raw mirror ====*/
    status = fbe_api_database_persist_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "mark system object recreation flags successfully\n");


    /*==== Check the recreate or NDB flags which just set ====*/
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    status = fbe_api_database_get_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(system_object_op_flags.valid_num, 0);
    MUT_ASSERT_INT_EQUAL(system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN]&FBE_SYSTEM_OBJECT_OP_RECREATE, FBE_SYSTEM_OBJECT_OP_RECREATE);

    /*==== Before reboot, do write and read check with VAULT LUN ====*/
    phoenix_test_write_nonzero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);

    /*==== Reboot and wait for database service ready ====*/
    phoenix_reboot_sp();

    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN, 
                                                     FBE_LIFECYCLE_STATE_READY, 40000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /*==== After reboot, do read zero pattern check with PSM LUN ====*/
    phoenix_test_read_zero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN);
    

    mut_printf(MUT_LOG_TEST_STATUS, "write and read check pass\n");

    /*********************************Test 3***********************************/ 
    mut_printf(MUT_LOG_TEST_STATUS, "Test 3: Insert a new db drive to (0,0,0), and mark this PVD recreated\n");

    /*==== Mark PVD 0 need to be recreated ====*/
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0] |= FBE_SYSTEM_OBJECT_OP_RECREATE;
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0] |= FBE_SYSTEM_OBJECT_OP_PVD_ZERO;
    system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0] |= FBE_SYSTEM_OBJECT_OP_PVD_NR;
    system_object_op_flags.valid_num++;

    /*===== persist the flags to raw mirror ====*/
    status = fbe_api_database_persist_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "set recreation flags successfully\n");

    /*==== Check the recreate or NDB flags which just set ====*/
    fbe_zero_memory(&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    status = fbe_api_database_get_system_object_recreate_flags(&system_object_op_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(system_object_op_flags.valid_num, 0);
    MUT_ASSERT_INT_EQUAL(system_object_op_flags.system_object_op[FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0]&FBE_SYSTEM_OBJECT_OP_RECREATE, FBE_SYSTEM_OBJECT_OP_RECREATE);
    mut_printf(MUT_LOG_TEST_STATUS, "check recreation flags successfully\n");


    /*==== Before reboot, do write and read check with PSM LUN ====*/
    phoenix_test_write_nonzero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM);

    /*==== Now Reboot this SP ====*/
    /*==== First destory neit and sep ====*/
    fbe_test_sep_util_destroy_neit_sep();
    /*==== Insert a new drive to DB slot 0. The aim is to trigger homerecker to start renitilize this PVD ====*/
    /*==== Pull DB drive 0_0_0 ====*/
    status = fbe_test_pp_util_pull_drive(0, 0, 0, &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /*==== Insert a new drive to DB slot ====*/
    status = fbe_test_pp_util_insert_unique_sas_drive(0, 0, 0,
                                    520,
                                    TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                    &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Pull drive(0,0,0) and reinsert a new drive to this slot offline\n");

    /*==== BOOT UP THE sep and neit again ====*/
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(15000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /*==== After reboot, do read zero pattern check with PSM LUN ====*/
    phoenix_test_read_nonzero_pattern(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM);
    mut_printf(MUT_LOG_TEST_STATUS, "write and read check pass\n");
    /*********************** End of Test 3 ******************************/

    /*This test is obsolete, which is against the current solution of RG recreation. So there will be error
     *output. We should clear the error count*/
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail reset error counter for sep");

    return;
} 

/*!**************************************************************
 * phoenix_create_physical_config()
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

void phoenix_create_physical_config(fbe_bool_t b_use_4160)
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

    /*! @todo - reinstate configuration support for SATA drives
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @todo - reinstate configuration support for SATA drives
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
    status = fbe_api_wait_for_number_of_objects(PHOENIX_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == PHOENIX_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end phoenix_create_physical_config()
 *******************************************


/*!****************************************************************************
 *  phoenix_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the phoenix test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void phoenix_setup(void)
{  

    /* Initialize any required fields and perform cleanup if required
     */
    if (fbe_test_util_is_simulation())
    {
        /*  Load the physical package and create the physical configuration. */
        phoenix_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        /* Load the logical packages */
        sep_config_load_sep_and_neit();
    }
    fbe_test_sep_util_set_pvd_class_debug_flags(0xffff);
    fbe_test_common_util_test_setup_init();

} 


/*!****************************************************************************
 *  phoenix_cleanup
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
void phoenix_cleanup(void)
{

    if (fbe_test_util_is_simulation())
    {
         fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

}

/*After removing one drive, create RG 1*/
static void phoenix_test_create_rg_1(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
    fbe_api_raid_group_create_t         create_rg;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
    create_rg.raid_group_id = phoenix_rg_config_1[0].raid_group_id;
    create_rg.b_bandwidth = phoenix_rg_config_1[0].b_bandwidth;
    create_rg.capacity = phoenix_rg_config_1[0].capacity;
    create_rg.drive_count = phoenix_rg_config_1[0].width;
    create_rg.raid_type = phoenix_rg_config_1[0].raid_type;
    create_rg.is_private = FBE_FALSE;
    fbe_copy_memory(create_rg.disk_array, phoenix_rg_config_1[0].rg_disk_set, phoenix_rg_config_1[0].width * sizeof(fbe_test_raid_group_disk_set_t));
    
    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
    MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                               "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(phoenix_rg_config_1[0].raid_group_id, &rg_object_id);

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

} /* End phoenix_test_create_rg_1() */

static void phoenix_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(15000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!****************************************************************************
 *  phoenix_test_write_nonzero_pattern
 ******************************************************************************
 *
 * @brief
 *   write nonzero pattern to system LUN. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void phoenix_test_write_nonzero_pattern(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context = &phoenix_test_rdgen_io_context[0];
    fbe_status_t    status;

    status = fbe_api_rdgen_test_context_init(context,
                        lun_object_id,
                        FBE_CLASS_ID_INVALID,
                        FBE_PACKAGE_ID_SEP_0,
                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                        FBE_RDGEN_PATTERN_LBA_PASS,
                        1, /* we do one full sequential pass */
                        0, /* num ios not used */
                        0, /* time not used */
                        1, /* theads */
                        FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                        0, /* start lba */
                        0, /* min lba */
                        1024, /* max lba */
                        FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                        1,
                        128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context->start_io.statistics.error_count, 0);

    return;
                        
}

/*!****************************************************************************
 *  phoenix_test_read_nonzero_pattern
 ******************************************************************************
 *
 * @brief
 *   read nonzero pattern to system LUN. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void phoenix_test_read_nonzero_pattern(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context = &phoenix_test_rdgen_io_context[0];
    fbe_status_t    status;

    status = fbe_api_rdgen_test_context_init(context,
                        lun_object_id,
                        FBE_CLASS_ID_INVALID,
                        FBE_PACKAGE_ID_SEP_0,
                        FBE_RDGEN_OPERATION_READ_CHECK,
                        FBE_RDGEN_PATTERN_LBA_PASS,
                        1, /* we do one full sequential pass */
                        0, /* num ios not used */
                        0, /* time not used */
                        1, /* theads */
                        FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                        0, /* start lba */
                        0, /* min lba */
                        1024, /* max lba */
                        FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                        1,
                        128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context->start_io.statistics.error_count, 0);

    return;

}

/*!****************************************************************************
 *  phoenix_test_read_zero_pattern
 ******************************************************************************
 *
 * @brief
 *   read zero pattern to system LUN. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void phoenix_test_read_zero_pattern(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context = &phoenix_test_rdgen_io_context[0];
    fbe_status_t    status;

    status = fbe_api_rdgen_test_context_init(context,
                        lun_object_id,
                        FBE_CLASS_ID_INVALID,
                        FBE_PACKAGE_ID_SEP_0,
                        FBE_RDGEN_OPERATION_READ_CHECK,
                        FBE_RDGEN_PATTERN_ZEROS,
                        1, /* we do one full sequential pass */
                        0, /* num ios not used */
                        0, /* time not used */
                        1, /* theads */
                        FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                        0, /* start lba */
                        0, /* min lba */
                        1024, /* max lba */
                        FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                        1,
                        128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context->start_io.statistics.error_count, 0);

    return;

}

