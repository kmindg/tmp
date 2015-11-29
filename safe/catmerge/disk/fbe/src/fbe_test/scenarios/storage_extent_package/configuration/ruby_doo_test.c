
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file ruby_doo_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for the interaction between verify and lun unbind.
 *
 * @version
 *   3/9/2010 - Created.  guov
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_random.h"

char * ruby_doo_short_desc = "LU unbind and RG destroyed while background verify is in progress";
char * ruby_doo_long_desc =
"\n"
"\n"
"The Rubydoo Scenario tests the ability to unbind LUN and destroy RG when background verify\n"
"is in progress.\n"
"\n"
"Starting Config: \n"
"\t[PP] armada board\n"
"\t[PP] SAS PMC port\n"
"\t[PP] viper enclosure\n"
"\t[PP] fifteen SAS drives \n"
"\t[PP] fifteen logical drives \n"
"\t[SEP] fifteen provisioned drives \n"
"\t[SEP] fifteen virtual drives\n"
"\t[SEP] one RAID Objects (Raid-5)\n"
"\t[SEP] one LUN Objects\n"
"\n"
"STEP 1: Create the initial topology.\n"
"\t- Create and verify the initial physical package config.\n"
"\t- Create virtual drives and attach edges to provisioned drives.\n"
"\t- Create a five drive raid-5 raid group object and attach edges from raid-5 to the virtual drives.\n"
"\t- Bind one LUNs to the raid-5 object.\n"
"STEP 2: Initiate a background verify(read-only).\n"
"\t- Issue an fbe_api call to LUN object to start background verify.\n"
"STEP 3: Unbind the LUN while verify is in progress.\n"
"\t- Verify the background verify is in progress thru bgverify status.\n"
"\t- Issue an fbe_api call to LUN object to unbind the lun.\n"
"STEP 4: Destroy RG while verify is in progress\n"
"\t- Verify the background verify is in progress thru bgverify status.\n"
"\t- Issue an fbe_api call to RG object to destroy the RG.\n"
"\n"
"Description last updated: 10/03/2011.\n";

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def RUBY_DOO_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of RGs to create
 *
 *********************************************************************/
#define RUBY_DOO_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def RUBY_DOO_TEST_MAX_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RAID_GROUP
 *
 *********************************************************************/
#define RUBY_DOO_TEST_MAX_RAID_GROUP_WIDTH               5


/*!*******************************************************************
 * @def RUBY_DOO_TEST_RAID_GROUP_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of RAID_GROUPs
 *
 *********************************************************************/
#define RUBY_DOO_TEST_RAID_GROUP_ELEMENT_SIZE           128 


/*!*******************************************************************
 * @def RUBY_DOO_TEST_RAID_GROUP_ELEMENTS_PER_PARITY
 *********************************************************************
 * @brief Legacy elements per parity 
 *
 *********************************************************************/
#define RUBY_DOO_TEST_RAID_GROUP_ELEMENTS_PER_PARITY    8


/*!*******************************************************************
 * @def RUBY_DOO_TEST_RAID_GROUP_CHUNK_SIZE
 *********************************************************************
 * @brief Number of blocks in a raid group bitmap chunk
 *
 *********************************************************************/
#define RUBY_DOO_TEST_RAID_GROUP_CHUNK_SIZE      FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH

#define RUBY_DOO_TEST_MAX_RG_SIZE 200 * 0x8000 * 3


///*!*******************************************************************
// * @def RUBY_DOO_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS
// *********************************************************************
// * @brief Capacity of Virtual Drive; based on 32 MB sim drive
// *
// *********************************************************************/
//#define RUBY_DOO_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS   0xE000


/*!*******************************************************************
 * @def RUBY_DOO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 1 encl + 15 pdo) 
 *
 *********************************************************************/
#define RUBY_DOO_TEST_NUMBER_OF_PHYSICAL_OBJECTS 18 


/*!*******************************************************************
 * @def RUBY_DOO_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define RUBY_DOO_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

/*!*******************************************************************
 * @def RUBY_DOO_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define RUBY_DOO_TEST_RAID_GROUP_ID        0

/*!*******************************************************************
 * @def RUBY_DOO_TEST_LUN_NUMBER
 *********************************************************************
 * @brief RAID LUN number used by this test
 *
 *********************************************************************/
#define RUBY_DOO_TEST_LUN_NUMBER        123


/*!*******************************************************************
 * @def RUBY_DOO_TEST_BLOCK_SIZE_512
 *********************************************************************
 * @brief 512 block size
 *
 *********************************************************************/
#define RUBY_DOO_TEST_BLOCK_SIZE_512        512

/*!*******************************************************************
 * @def RUBY_DOO_TEST_BLOCK_SIZE_520
 *********************************************************************
 * @brief 520 block size
 *
 *********************************************************************/
#define RUBY_DOO_TEST_BLOCK_SIZE_520        520

/*!*******************************************************************
 * @def RUBY_DOO_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define RUBY_DOO_TEST_NS_TIMEOUT        30000 /*wait maximum of 10 seconds*/

/*!*******************************************************************
 * @var ruby_doo_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ruby_doo_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {RUBY_DOO_TEST_MAX_RAID_GROUP_WIDTH,RUBY_DOO_TEST_MAX_RG_SIZE, FBE_RAID_GROUP_TYPE_RAID5,    FBE_CLASS_ID_PARITY,    520,RUBY_DOO_TEST_RAID_GROUP_ID,0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
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
typedef enum ruby_doo_test_enclosure_num_e
{
    RUBY_DOO_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    RUBY_DOO_TEST_ENCL2_WITH_SAS_DRIVES,
} ruby_doo_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum ruby_doo_test_drive_type_e
{
    SAS_DRIVE,
} ruby_doo_test_drive_type_t;

/* This is a set of parameters for the test drive selection.
 */
typedef struct enclosure_drive_details_s
{
    ruby_doo_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    ruby_doo_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        RUBY_DOO_TEST_ENCL1_WITH_SAS_DRIVES,
        RUBY_DOO_TEST_BLOCK_SIZE_520
    },
};

/* Count of rows in the table.
 */
#define RUBY_DOO_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void ruby_doo_test_load_physical_config(void);
static void ruby_doo_test_load_logical_config(void);
void ruby_doo_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
fbe_status_t ruby_doo_test_create_raid_group(fbe_test_rg_configuration_t *rg_config_ptr);
fbe_status_t ruby_doo_test_create_lun(fbe_test_rg_configuration_t *rg_config_ptr, 
                                      fbe_lun_number_t lun_number,
                                      fbe_bool_t initial_verify);
void ruby_doo_validate_verify_on_rebind(fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t ruby_doo_validate_verify_pass_count(fbe_object_id_t lun_object_id, fbe_u32_t pass_count);
fbe_status_t ruby_doo_wait_for_verify_checkpoint(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint);
fbe_status_t ruby_doo_wait_for_initial_verify(fbe_object_id_t lun_object_id);

/*!****************************************************************************
 * ruby_doo_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the ruby_doo test.  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  15/07/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void ruby_doo_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = NULL;


    rg_config_p = &ruby_doo_raid_group_config[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);


    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, ruby_doo_run_test);


    return;

}/* End plankton_test() */


/*!****************************************************************************
 *  ruby_doo_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Ruby Doo test.  
 *
 *
 * @param  
 *
 * rg_config_ptr    - pointer to raid group 
 * context_pp       - pointer to context
 *
 * @return  None 
 * 
 * @Modified
 * 15/07/2011 - Created. Vishnu Sharma 
 *
 *****************************************************************************/
void ruby_doo_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_status_t                  status;
    fbe_object_id_t               lun_object_id;
    fbe_object_id_t               rg_object_id;
    fbe_api_lun_destroy_t         fbe_lun_destroy_req;
    fbe_u32_t                     num_raid_groups = 0;
    fbe_u32_t                     index = 0;
    fbe_test_rg_configuration_t   *rg_config_p = NULL;
    fbe_api_rg_get_status_t       bgverify_status;
    fbe_api_lun_get_status_t      lun_bgverify_status;
    fbe_lba_t                   verify_checkpoint;
    fbe_u32_t                   i;
    fbe_job_service_error_type_t job_error_type;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting ruby_doo Test ===\n");

    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {

            /* Wait for pvds to be created */
            fbe_test_sep_util_wait_for_pvd_creation(rg_config_p->rg_disk_set,rg_config_p->width,10000);

            /*Create the RAID group*/
            status = ruby_doo_test_create_raid_group(rg_config_p);         
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Create the LUN */
            status = ruby_doo_test_create_lun(rg_config_p, RUBY_DOO_TEST_LUN_NUMBER, FBE_FALSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* find the object id of the lun*/
            status = fbe_api_database_lookup_lun_by_number(RUBY_DOO_TEST_LUN_NUMBER, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* find the object id of the raid group*/
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "=== Issue BV on LUN  ===\n");

            /* Send an Error verify control op to the LUN.
             * This is needed since the error verify is higher priority than the zero. 
             */ 
            status = fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                                           FBE_LUN_VERIFY_TYPE_ERROR,
                                                           FBE_TRUE, /* Verify the entire lun */ 
                                                           FBE_LUN_VERIFY_START_LBA_LUN_START,   
                                                           FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);    
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Wait for verify to start.
             */
            status = fbe_test_sep_util_wait_for_verify_start(lun_object_id, 
                                                             FBE_LUN_VERIFY_TYPE_ERROR,
                                                             &lun_bgverify_status);                
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            verify_checkpoint = lun_bgverify_status.checkpoint;

            /* Now wait for the verify to make progress
             */
            for (i = 0; i < 50; i++)
            {
                status = fbe_api_lun_get_bgverify_status(lun_object_id,&lun_bgverify_status,FBE_LUN_VERIFY_TYPE_ERROR);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (verify_checkpoint < lun_bgverify_status.checkpoint)
                {
                    break;
                }
                fbe_api_sleep(200);
            }

            MUT_ASSERT_TRUE(i < 50);

            mut_printf(MUT_LOG_TEST_STATUS, "=== BV Started successfully on the  LUN  ===\n");

            /* Destroy a LUN */
            fbe_lun_destroy_req.lun_number = RUBY_DOO_TEST_LUN_NUMBER;

            mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

            status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  RUBY_DOO_TEST_NS_TIMEOUT, &job_error_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

            //fbe_api_sleep(10000);
            /*Test the API for BG Verify checkpoint percentage complete for RG*/
            status = fbe_api_raid_group_get_bgverify_status(rg_object_id,&bgverify_status,FBE_LUN_VERIFY_TYPE_ERROR);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            verify_checkpoint = bgverify_status.checkpoint;

            for (i = 0; i < 50; i++)
            {
                status = fbe_api_raid_group_get_bgverify_status(rg_object_id,&bgverify_status,FBE_LUN_VERIFY_TYPE_ERROR);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (verify_checkpoint <= bgverify_status.checkpoint)
                {
                    break;
                }
                fbe_api_sleep(200);
            }

            MUT_ASSERT_TRUE(i < 50);

            /* Destroy a RAID group with user provided configuration. */
            status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");

            ruby_doo_validate_verify_on_rebind(rg_config_p);
        }
    }

    return;
}

void ruby_doo_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set up for RubyDoo test ===\n");

    if (fbe_test_util_is_simulation())
    {
        ruby_doo_test_load_physical_config();
        ruby_doo_test_load_logical_config();
    }
    return;
}

void ruby_doo_validate_verify_on_rebind(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    fbe_api_lun_destroy_t fbe_lun_destroy_req;
    fbe_job_service_error_type_t job_error_type;
    fbe_object_id_t lun_object_id;
    fbe_u32_t i;

    /*Create the RAID group*/
    status = ruby_doo_test_create_raid_group(rg_config_p);         
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* find the object id of the raid group*/
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Prevent any verifies from starting */
    fbe_api_base_config_disable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);

    for (i = 0; i < 3; i++)
    {
        /* Create the LUN */
        status = ruby_doo_test_create_lun(rg_config_p, (RUBY_DOO_TEST_LUN_NUMBER + i), FBE_FALSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* find the object id of the lun*/
        status = fbe_api_database_lookup_lun_by_number(RUBY_DOO_TEST_LUN_NUMBER+i, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Send initial Error verify for LUN #:%d ===\n", 
                   RUBY_DOO_TEST_LUN_NUMBER+i);

        /* Send an Error verify control op to the LUN.
         * This is needed since the error verify is higher priority than the zero. 
         */ 
        status = fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                                       FBE_LUN_VERIFY_TYPE_ERROR,
                                                       FBE_TRUE, /* Verify the entire lun */ 
                                                       FBE_LUN_VERIFY_START_LBA_LUN_START,   
                                                       FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);    
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Destroy the middle LUN */
    fbe_lun_destroy_req.lun_number = RUBY_DOO_TEST_LUN_NUMBER + 1;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  RUBY_DOO_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

    /* Recreate the LUN back */
    status = ruby_doo_test_create_lun(rg_config_p, (RUBY_DOO_TEST_LUN_NUMBER + 1), FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Enable the background verifies */
    fbe_api_base_config_enable_background_operation(rg_object_id, FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Waiting for RG Checkpoint===\n");

    /* Wait for the RG checkpoint to go to Invalid */
    status = ruby_doo_wait_for_verify_checkpoint(rg_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the pass count is Zero for the LUNs */
    for (i = 0; i < 3; i++)
    {
        /* find the object id of the lun*/
        status = fbe_api_database_lookup_lun_by_number(RUBY_DOO_TEST_LUN_NUMBER+i, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Waiting for pass count for LUN #:%d ===\n", 
                   RUBY_DOO_TEST_LUN_NUMBER+i);

        /* For the first and third LUN the verify should have proceeded normally and pass count should be 1 */
        if(i == 1)
        {
            /* Make sure the passcount for the verify is Zero */
            ruby_doo_validate_verify_pass_count(lun_object_id, 0);
        }

        else
        {
            /* Make sure the passcount for the verify is 1 */
            ruby_doo_validate_verify_pass_count(lun_object_id, 1);
        }
        /* Destroy a LUN */
        fbe_lun_destroy_req.lun_number = RUBY_DOO_TEST_LUN_NUMBER + i;

        mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

        status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  RUBY_DOO_TEST_NS_TIMEOUT, &job_error_type);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

    }


}
/*!****************************************************************************
 *  ruby_doo_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the ruby_doo test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void ruby_doo_test_cleanup(void)
{  

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for RubyDoo test ===\n");

    if (fbe_test_util_is_simulation())
    {
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_sep_physical();
    }
    return;

} /* End ruby_doo_test_cleanup() */

/*!**************************************************************
 * ruby_doo_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the ruby_doo test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void ruby_doo_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[RUBY_DOO_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < RUBY_DOO_TEST_ENCL_MAX; enclosure++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < RUBY_DOO_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if (SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
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
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to load PhysicalPackage.dll!  Did you build it?");

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(RUBY_DOO_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == RUBY_DOO_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End ruby_doo_test_load_physical_config() */

/*!**************************************************************
 * ruby_doo_test_load_logical_config()
 ****************************************************************
 * @brief
 *  Configure the ruby_doo test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void ruby_doo_test_load_logical_config(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Logical Configuration ===\n");

    sep_config_load_sep();

    return;

} /* End ruby_doo_test_load_logical_config() */

/*!**************************************************************
 * ruby_doo_test_create_raid_group()
 ****************************************************************
 * @brief
 *  To create a raid group.
 *
 * @param 
 * 
 * rg_config_ptr       - Pointer to raid group configuration
 * @return status.
 *
 * @author
 *  07/15/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/

fbe_status_t ruby_doo_test_create_raid_group(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_u32_t                                           iter;
    fbe_api_job_service_raid_group_create_t             fbe_raid_group_create_req;
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;


    /* Create the SEP objects for the configuration */

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    fbe_zero_memory(&fbe_raid_group_create_req,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_req.b_bandwidth = FBE_TRUE;
    fbe_raid_group_create_req.capacity = rg_config_ptr->capacity;
    //fbe_raid_group_create_req.explicit_removal = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = rg_config_ptr->raid_group_id;
    fbe_raid_group_create_req.raid_type = rg_config_ptr->raid_type;
    fbe_raid_group_create_req.drive_count = rg_config_ptr->width;
    fbe_raid_group_create_req.wait_ready  = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec = RUBY_DOO_TEST_NS_TIMEOUT;

    for (iter = 0; iter < rg_config_ptr->width; ++iter)
    {
        fbe_raid_group_create_req.disk_array[iter].bus = rg_config_ptr->rg_disk_set[iter].bus;
        fbe_raid_group_create_req.disk_array[iter].enclosure = rg_config_ptr->rg_disk_set[iter].enclosure;
        fbe_raid_group_create_req.disk_array[iter].slot = rg_config_ptr->rg_disk_set[iter].slot;
    }
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_raid group_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_raid_group_create_req.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_req.job_number,
                                         RUBY_DOO_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
                                                         fbe_raid_group_create_req.raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * ruby_doo_test_create_lun()
 ****************************************************************
 * @brief
 *  To create a raid group.
 *
 * @param 
 * 
 * rg_config_ptr       - Pointer to raid group configuration
 * @return status.
 *
 * @author
 *  07/15/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/

fbe_status_t ruby_doo_test_create_lun(fbe_test_rg_configuration_t *rg_config_ptr, 
                                      fbe_lun_number_t lun_number, fbe_bool_t initial_verify)
{
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_status_t            status;
    fbe_job_service_error_type_t    job_error_type;

    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a LUN ===\n");
    fbe_lun_create_req.raid_type = rg_config_ptr->raid_type;
    fbe_lun_create_req.raid_group_id = rg_config_ptr->raid_group_id; 
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = 500;  /* use the drive capacity/100 for the lun cap to make verify processed quickly. */
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = initial_verify;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, (RUBY_DOO_TEST_NS_TIMEOUT), NULL, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

fbe_status_t ruby_doo_wait_for_initial_verify(fbe_object_id_t lun_object_id)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_api_lun_get_lun_info_t      get_info;

    while (current_time < timeout_ms)
    {

        status = fbe_api_lun_get_lun_info(lun_object_id, &get_info);
        if (get_info.b_initial_verify_already_run == FBE_TRUE)
        {
            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t ruby_doo_wait_for_verify_checkpoint(fbe_object_id_t rg_object_id, fbe_lba_t checkpoint)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_api_raid_group_get_info_t   get_info;

    while (current_time < timeout_ms)
    {

        status = fbe_api_raid_group_get_info(rg_object_id, &get_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (get_info.error_verify_checkpoint == checkpoint)
        {
            /* If we are waiting for a completion also make sure the event q is empty to 
             * insure there are no incoming verifies.
             */
            if ((checkpoint != FBE_LBA_INVALID) || 
                (get_info.b_is_event_q_empty == FBE_TRUE))
            {
                return status;
            }
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t ruby_doo_validate_verify_pass_count(fbe_object_id_t lun_object_id, fbe_u32_t pass_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    fbe_lun_verify_report_t   in_verify_report;

    while (current_time < timeout_ms)
    {

        status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &in_verify_report);
        if (in_verify_report.pass_count == pass_count)
        {
            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
