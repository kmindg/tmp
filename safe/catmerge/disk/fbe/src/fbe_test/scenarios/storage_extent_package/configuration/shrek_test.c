
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file shrek_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for testing the transaction abort
 *   function of the Database Service
 *
 * @version
 *   6/2011 - Created.  sonalik
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_random.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * shrek_short_desc = "tests transaction rollback functionality in the database service";
char * shrek_long_desc =
    "\n"
    "\n"
    "The Shrek test tests if the rollback transaction in the database service works as expected.\n"
    "\n"   
    "Starting Config:\n"
    "\t [PP] armada board\n"
    "\t [PP] SAS PMC port\n"
    "\t [PP] viper enclosure\n"
    "\t [PP] 5 SAS drives (PDO)\n"
    "\t [PP] 5 logical drives (LD)\n"
    "\t [SEP] 5 provision drives (PD)\n"
    "\t [SEP] 5 virtual drives (VD)\n"
    "\t [SEP] 1 raid object (RAID)\n"   
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "\t- Create a raid object with I/O edges attached to all VDs.\n"    
    "\t- Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2:Start a transaction for lun creation and abort it. The LUN object should get destroyed.\n" 
    "STEP 3:Start a transaction Enabling/Disabling power save option in RG to test the UPDATE RG functionality "
    "       and abort the transaction.The RG should regain its original settings for power save.\n" 
    "STEP 4:Start a transaction changing the system  global information and abort the transaction.The system "
    "       should retain its original values.\n" 
    "STEP 5:Start a transaction for RG and VD destroy and abort it.The RG and VD object should get recreated.\n" 
    "\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def SHREK_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define SHREK_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def SHREK_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define SHREK_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def SHREK_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define SHREK_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

/*!*******************************************************************
 * @def SHREK_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define SHREK_TEST_RAID_GROUP_ID        0


/*!*******************************************************************
 * @def SHREK_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SHREK_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @var SHREK_abort_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t shrek_rg_configuration[SHREK_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        SHREK_TEST_RAID_GROUP_ID, 0,
    /* rg_disk_set */
    {{0,0,4}, {0,0,5}, {0,0,6}}
};


/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum shrek_test_enclosure_num_e
{
    SHREK_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    SHREK_TEST_ENCL2_WITH_SAS_DRIVES,
    SHREK_TEST_ENCL3_WITH_SAS_DRIVES,
    SHREK_TEST_ENCL4_WITH_SAS_DRIVES
} shrek_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum shrek_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} shrek_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    shrek_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    shrek_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        SHREK_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SHREK_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SHREK_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SHREK_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define SHREK_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void shrek_test_load_physical_config(void);
static void shrek_test_create_rg(void);
static void shrek_test_create_lun(void);
static void shrek_test_create_object_rollback(void);
static void shrek_test_destroy_object_rollback(void);
static void shrek_test_modify_object_rollback(void);
static void shrek_test_modify_global_info_rollback(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/
/*!****************************************************************************
 *  shrek_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the SHREK test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void shrek_test(void)
{
    /*create a RG using job service*/
    shrek_test_create_rg();
    shrek_test_create_object_rollback();
    shrek_test_modify_object_rollback();
	shrek_test_modify_global_info_rollback();
    shrek_test_destroy_object_rollback();
    return;

} /* End shrek_test() */


/*!**************************************************************
 * shrek_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the shrek_test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void shrek_test_create_rg(void)
{
    fbe_status_t                                status;    
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;
    fbe_object_id_t                            object_id;
    fbe_object_id_t                            object_id_from_job;
   
   
    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(shrek_rg_configuration);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(shrek_rg_configuration[0].job_number,
                                         SHREK_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             shrek_rg_configuration[0].raid_group_id, &object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_id_from_job,object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", object_id);

    return;

} /* End shrek_test_create_rg() */

static void shrek_test_create_lun(void)
{
    fbe_status_t                       status;
    fbe_api_lun_create_t               fbe_lun_create_req; 
    fbe_object_id_t                    object_id;
    fbe_job_service_error_type_t       job_error_type;

    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type        = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id    = SHREK_TEST_RAID_GROUP_ID;
    fbe_lun_create_req.lun_number       = 123;
    fbe_lun_create_req.capacity         = 0x1000;
    fbe_lun_create_req.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b            = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset       = FBE_LBA_INVALID;
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

	status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, SHREK_TEST_NS_TIMEOUT, &object_id, &job_error_type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

	/* Get object ID for LUN just created */
	status = fbe_api_database_lookup_lun_by_number(123, &object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__, object_id);

    return;

}  /* End shrek_test_create_lun() */

/*Tests the object creation transaction rollback */
void shrek_test_create_object_rollback(void)
{
    fbe_status_t status;
    fbe_database_transaction_info_t transaction_info = {0};
    fbe_database_control_lun_t create_lun;
    fbe_object_id_t lu_object_id,rg_object_id;
    fbe_u32_t num_luns;
	fbe_u32_t lun_count;

    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start transaction for LUN creation ===\n");
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    status = fbe_api_database_start_transaction(&transaction_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &num_luns);
	lun_count = num_luns;
  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN create! ===\n");

    create_lun.lun_number = 123;
    create_lun.lun_set_configuration.config_flags = FBE_LUN_CONFIG_NONE;
    create_lun.lun_set_configuration.capacity = 0x1000;
    create_lun.transaction_id = transaction_info.transaction_id;
    create_lun.object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_database_create_lun(create_lun);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN created successfully ===\n");

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &num_luns);
    MUT_ASSERT_INT_EQUAL(num_luns, lun_count+1);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Abort Transaction for LUN creation started===\n");
    status = fbe_api_database_abort_transaction(transaction_info.transaction_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Transaction successfully aborted!!===\n");

    /*LUN should not be visible after abort since it gets destroyed*/
    /*check in database service*/
    status = fbe_api_database_lookup_lun_by_number(123, &lu_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    /*check in topology service*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &num_luns);
    MUT_ASSERT_INT_EQUAL(num_luns, lun_count);
  
}

/*Tests object destroy transaction rollback */
void shrek_test_destroy_object_rollback(void)
{
    fbe_status_t                                status;
    fbe_database_transaction_info_t             transaction_info = {0};
    fbe_database_control_destroy_object_t       destroy_vd,destroy_raid;
    fbe_api_base_config_downstream_object_list_t downstream_raid_object_list;
    fbe_object_id_t rg_object_id;
    fbe_u32_t num_rgs,new_rg_count,num_vds,new_vd_count;

    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*find existing no of RG objects*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_rgs);
    new_rg_count = num_rgs;

    /*find existing no of VD objects*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &num_vds);
    new_vd_count = num_vds;

    /*get all the VD objects connected to RG*/
    status = fbe_api_base_config_get_downstream_object_list(rg_object_id, &downstream_raid_object_list);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*start a transaction to destroy RG and VD and abort it*/
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    status = fbe_api_database_start_transaction(&transaction_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*RG DESTROY*/
    destroy_raid.object_id = rg_object_id;
    destroy_raid.transaction_id = transaction_info.transaction_id;
    status = fbe_api_database_destroy_raid(destroy_raid);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RG destroyed successfully===\n");

    /*One RG destroyed - check in topology service*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_rgs);
    MUT_ASSERT_INT_EQUAL(num_rgs, new_rg_count-1);

    /*VD DESTROY*/
     destroy_vd.object_id = downstream_raid_object_list.downstream_object_list[0];
     destroy_vd.transaction_id = transaction_info.transaction_id;
     status = fbe_api_database_destroy_vd(destroy_vd);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     mut_printf(MUT_LOG_TEST_STATUS, "=== VD destroyed successfully===\n");

     /*one VD objects destroyed - check in topology service*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &num_vds);
    MUT_ASSERT_INT_EQUAL(num_vds, new_vd_count-1);

     mut_printf(MUT_LOG_TEST_STATUS, "=== Transaction Abort started.. ===\n");
     status = fbe_api_database_abort_transaction(transaction_info.transaction_id);

     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     mut_printf(MUT_LOG_TEST_STATUS, "=== Transaction Aborted successfully ===\n");

     /*check if RG recreated - database service*/
     status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     /*check if RG recreated - topology service*/
     status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_rgs);
     MUT_ASSERT_INT_EQUAL(num_rgs, new_rg_count);

     /*check if VD recreated - topology service*/
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0, &num_vds);
    MUT_ASSERT_INT_EQUAL(num_vds, new_vd_count);
}
/*Tests the object update config transaction rollback */
void shrek_test_modify_object_rollback(void)
{
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;
    fbe_database_control_update_raid_t          update_raid;
    fbe_database_transaction_info_t             transaction_info = {0};
    fbe_object_id_t rg_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Enabling / Disabling power saving on the RAID ===\n");
    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*make sure it's not saving power */
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     /*let's wait for a while since RG propagate it to the LU*/
    fbe_api_sleep(14000);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);

    // Update RAID config rollback transaction testing
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start transaction for RAID UPDATE ===\n");
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    status = fbe_api_database_start_transaction(&transaction_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_raid_group_by_number(0, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    update_raid.update_type = FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE;
    update_raid.transaction_id = transaction_info.transaction_id;
    update_raid.object_id = rg_object_id;

    status = fbe_api_database_update_raid_group(update_raid);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_sleep(14000);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Updated raid group successfully ===\n");

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Abort Transaction for update raid started...\n");
    status = fbe_api_database_abort_transaction(transaction_info.transaction_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Transaction aborted successfully  ===\n");
    fbe_api_sleep(14000);
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);

}
/*Tests the system global info transaction rollback */
void shrek_test_modify_global_info_rollback(void)
{
	fbe_database_power_save_t              power_save_info;
    fbe_system_power_saving_info_t  system_power_save_info;
    fbe_status_t                                status;
    fbe_database_transaction_info_t transaction_info = {0};
     
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
    status  = fbe_api_get_system_power_save_info(&system_power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(system_power_save_info.enabled == FBE_TRUE);

    /*Start transaction to disable power save and abort it*/
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    status = fbe_api_database_start_transaction(&transaction_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    power_save_info.system_power_save_info.enabled = FBE_FALSE;
    power_save_info.transaction_id = transaction_info.transaction_id;

     status = fbe_api_database_set_power_save(power_save_info);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RG power saving ===\n");

    status  = fbe_api_get_system_power_save_info(&system_power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(system_power_save_info.enabled == FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Abort Transaction RG power saving ===\n");
    status = fbe_api_database_abort_transaction(transaction_info.transaction_id);


    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Abort Transaction successfull RG power saving ===\n");

    /*check if system retains its original value*/
    status  = fbe_api_get_system_power_save_info(&system_power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(system_power_save_info.enabled == FBE_TRUE);
}

/*!****************************************************************************
 *  shrek_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the shrek test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void shrek_test_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Shrek test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    shrek_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    return;

} /* End shrek_test_setup() */


/*!****************************************************************************
 *  shrek_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Shrek test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void shrek_test_cleanup(void)
{  
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Shrek test ===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;

} /* End shrek_test_cleanup() */


static void shrek_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[SHREK_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

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
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < SHREK_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < SHREK_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
            else if(SATA_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sata_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
        }
    }

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
    status = fbe_api_wait_for_number_of_objects(SHREK_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == SHREK_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End shrek_test_load_physical_config() */

