/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file smurfs_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test for binding RGs and LUNs on DB drives
 *
 * @version
 *  08/26/2011 - Created. Vera Wang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "mut.h"   

#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"

#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"

#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_random.h"


/*************************
 *   TEST DESCRIPTION
 *************************/
char * smurfs_short_desc = "test of binding RGs and LUNs on DB drives";
char * smurfs_long_desc ="\
The smurfs tests the ability of the system to bind raid group and LUNs on database drives\n\
                  and they will not overlap with any private space areas.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] SAS drives\n\
        [PP] logical drive\n\
        [SEP] provision drive\n\
\n\
STEP 1: Configure all raid groups and LUNs on database drives (the first 4 drives).\n\
        - Tests cover 520 block sizes.\n\
        - Tests cover one Raid-5 RG and one LUN in that RG.\n\
STEP 2: Run IO to the LUN.\n\
STEP 3: Unbind the LUN.\n\
        - Ask the configuration service to unbind the LUN we wrote to.\n\
STEP 4: Destroy RG.\n\
        - Test to verify the RG is destroyed when no LUN exists.\n\
\n\
Additional test to add:\n\
        - To create some RGs with capacities that are specified, and check they\n\
          do not overlap with the private space RGs.\n\
\n\
Description last updated: 10/06/2011.\n";


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void smurfs_test_load_physical_config(void);
static void smurfs_test_create_lun(fbe_lun_number_t lun_number);
static void smurfs_test_destroy_lun(fbe_lun_number_t lun_number);
static void smurfs_test_create_rg(void);
static fbe_status_t smurfs_test_run_io_load(fbe_object_id_t lun_object_id);

static void smurfs_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id);

static void smurfs_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks);

static void smurfs_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks);



/*!*******************************************************************
 * @def SMURFS_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SMURFS_TEST_NS_TIMEOUT        100000 /*wait maximum of 100 seconds*/

/**********************************************************************************/

#define SMURFS_TEST_ENCL_MAX 4
#define SMURFS_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 /* (1 board + 1 port + 4 encl + 60 pdo) */
static fbe_api_rdgen_context_t  smurfs_test_rdgen_contexts[5]; 

#define SMURFS_TEST_RAID_GROUP_COUNT 1
static fbe_test_rg_configuration_t smurfs_rg_config[SMURFS_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    4,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          0,
    /* rg_disk_set */
    {{0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}}
};


/*!*******************************************************************
 * @def SMURFS_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define SMURFS_TEST_DRIVE_CAPACITY             (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY * 2)

/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum smurfs_test_enclosure_num_e
{
    SMURFS_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    SMURFS_TEST_ENCL2_WITH_SAS_DRIVES,
    SMURFS_TEST_ENCL3_WITH_SAS_DRIVES,
    SMURFS_TEST_ENCL4_WITH_SAS_DRIVES
} smurfs_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum smurfs_test_drive_type_e
{
    SAS_DRIVE,
//    SATA_DRIVE
} smurfs_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    smurfs_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    smurfs_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        SMURFS_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SMURFS_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SMURFS_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        SMURFS_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define SMURDSFS_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])


/*!**************************************************************
 * smurfs_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the smurfs test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void smurfs_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[SMURFS_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot = 0;
    fbe_u32_t                               enclosure = 0;

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
    for ( enclosure = 0; enclosure < SMURFS_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < SMURFS_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
            fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number, 
                                              encl_table[enclosure].encl_number,   
                                              slot,                                
                                              encl_table[enclosure].block_size,    
                                              SMURFS_TEST_DRIVE_CAPACITY,      
                                              &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(SMURFS_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == SMURFS_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End smurfs_test_load_physical_config() */

static void smurfs_test_create_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                    status;
    fbe_api_lun_create_t            fbe_lun_create_req;
    fbe_object_id_t					lu_id;
    fbe_job_service_error_type_t    job_error_type; 
    
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = 0; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, SMURFS_TEST_NS_TIMEOUT, &lu_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully!", __FUNCTION__, lu_id);

    /* Wait for LUN to be `ready' */
    status = fbe_api_wait_for_object_lifecycle_state(lu_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X ready!", __FUNCTION__, lu_id);

    return;   
} 

static void smurfs_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       			fbe_lun_destroy_req;
    fbe_job_service_error_type_t            job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, SMURFS_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");

    return;
}


/*!**************************************************************
 * smurfs_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Boomer test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void smurfs_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(smurfs_rg_config);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(smurfs_rg_config[0].job_number,
                                         SMURFS_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(smurfs_rg_config[0].raid_group_id, &rg_object_id);

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

} /* End smurfs_test_create_rg() */


void smurfs_test(void)
{
    fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     lu_object_id;
    smurfs_test_create_rg();
    smurfs_test_create_lun(123);

    status = fbe_api_database_lookup_lun_by_number(123, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    smurfs_test_run_io_load(lu_object_id);
    /* Test to verify the RG is not destroyed when a LUN exists.. */
    //status = fbe_test_sep_util_destroy_raid_group(smurfs_rg_config[0].raid_group_id);
    //MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);   

    smurfs_test_destroy_lun(123);
 
    /* Test to verify the RG is destroyed when no LUN exists.. */
    status = fbe_test_sep_util_destroy_raid_group(smurfs_rg_config[0].raid_group_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}

void smurfs_test_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for smurf test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    smurfs_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    return;
}

void smurfs_test_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for smurfs test ===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}



static fbe_status_t smurfs_test_run_io_load(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t*    context_p = &smurfs_test_rdgen_contexts[0];
    fbe_status_t                status;


    /* Write a background pattern; necessary until LUN zeroing is fully supported */
    smurfs_test_write_background_pattern(128, lun_object_id);

    /* Set up for issuing reads forever
     */
    smurfs_test_setup_rdgen_test_context(  context_p,
                                                lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
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

static void smurfs_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
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




static void smurfs_test_write_background_pattern(fbe_element_size_t element_size, fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &smurfs_test_rdgen_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUN
     */
    smurfs_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK
     */
    smurfs_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_READ_CHECK,
                                                    FBE_LBA_INVALID, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;

} /* end smurfs_test_write_background_pattern() */

static void smurfs_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t max_lba,
                                                    fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id,
                                                class_id, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                1,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                max_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                io_size_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} 





