/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file oh_smiling_test.c
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *  3/7/2012 - Created. He Wei
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
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_base_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_random.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * oh_smiling_short_desc = "User Private Raid Group/LUN Test";
char * oh_smiling_long_desc =
"\n"
"   The oh_smiling_test tests User Private Raid/LUN configure feature\n"
" \n"
"   oh_smiling is test for adding user private attribute \n"
"   to LUN and RG.The new attribute is persisted to Database. \n"
"   It is intialized at LUN/RG creation. The value can be get by FBE_API.\n"
"\n"
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
"   STEP 1: Create Raid Group/LUN: \n"
"        - Create User Private Raid Goup RG-1\n"
"        - Create Non-User Private Raid Goup RG-2\n"
"        - Create Private LUN-1 on RG-1\\n"
"        - Create Non-Private LUN-2 on RG-2\n"
"   STEP 2: Check the user private attribute value on previous Raid Group/LUN: \n"
"        - Reboot system\n"
"        - Check LUN-1 is Private\n"
"        - Check LUN-2 is Non-Private\n"
"        - Check RG-1 is Private\n"
"        - Check RG-2 is Non-Private\n"
"\n";


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def OH_SMILING_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define OH_SMILING_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def OH_SMILING_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define OH_SMILING_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY



/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum oh_smiling_test_enclosure_num_e
{
    OH_SMILING_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    OH_SMILING_TEST_ENCL2_WITH_SAS_DRIVES,
    OH_SMILING_TEST_ENCL3_WITH_SAS_DRIVES,
    OH_SMILING_TEST_ENCL4_WITH_SAS_DRIVES
} oh_smiling_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum oh_smiling_test_drive_type_e
{
    SAS_DRIVE
} oh_smiling_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    oh_smiling_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    oh_smiling_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        OH_SMILING_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        OH_SMILING_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        OH_SMILING_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        OH_SMILING_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define OH_SMILING_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

#define OH_SMILING_TEST_WAIT_MSEC          30000
#define OH_SMILING_TEST_LUNS_PER_RAID_GROUP  2
#define OH_SMILING_TEST_RG_TIMEOUT        60000 /*wait maximum of 60 seconds*/
#define OH_SMILING_TEST_LUN_TIMEOUT        25000 /*wait maximum of 25 seconds*/

/*!*******************************************************************
 * @var oh_smiling_rg_configuration
 *********************************************************************
 * @brief This is oh_smiling_rg_configuration, raid group configuration for creation
 *
 *********************************************************************/
static fbe_api_raid_group_create_t oh_smiling_rg_configuration[] = 
{
    {3,3,1800,0,FBE_RAID_GROUP_TYPE_RAID5,
     FBE_TRI_FALSE,FBE_FALSE,5000,   /*private*/
        {
            {0,0,5},
            {0,0,6},
            {0,0,7},
        },
     FBE_TRUE,
     FBE_FALSE
    },
    {4,3,1800,0,FBE_RAID_GROUP_TYPE_RAID5,
     FBE_TRI_FALSE,FBE_FALSE,5000,   /*not private*/
        {
            {0,0,8},
            {0,0,9},
            {0,0,10},
        },
     FBE_FALSE,
     FBE_FALSE
    },
};

static void oh_smiling_test_create_rg(void);
static void oh_smiling_test_system_reboot(void);
static void oh_smiling_create_lun(fbe_raid_group_number_t   in_raid_group_id,
                                          fbe_raid_group_type_t     in_raid_type,
                                          fbe_lun_number_t          in_lun_number,
                                          fbe_bool_t                in_user_private,
                                          fbe_object_id_t*          out_lun_object_id);
static void oh_smiling_load_physical_config(void);


/*!**************************************************************
 * oh_smiling_load_physical_config()
 ****************************************************************
 * @brief
 *  load physical config on setup.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/7/2012 - Created. He Wei
 ****************************************************************/
static void oh_smiling_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[OH_SMILING_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < OH_SMILING_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < OH_SMILING_TEST_ENCL_MAX; enclosure++)
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
    status = fbe_api_wait_for_number_of_objects(OH_SMILING_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 
                                                20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == OH_SMILING_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} 
/* End oh_smiling_test_load_physical_config() */


/*!****************************************************************************
 * oh_smiling_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/7/2012 - Created. He Wei
 ******************************************************************************/
void oh_smiling_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/* End oh_smiling_cleanup() */



/*!****************************************************************************
 * oh_smiling_test()
 ******************************************************************************
 * @brief
 *  Run lun zeroing test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/7/2012 - Created. He Wei
 ******************************************************************************/
void oh_smiling_test(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_raid_group_create_t     *rg_request = oh_smiling_rg_configuration;
    fbe_object_id_t                 rg_id[2];
    fbe_object_id_t                 lun_id[2];
    fbe_lun_number_t                lun_num[2] = {119, 110};
    fbe_database_lun_info_t         lun_info;
    fbe_api_raid_group_get_info_t   raid_info;
    fbe_job_service_error_type_t    job_error_type;
    
    mut_printf(MUT_LOG_LOW, "=== Oh_smiling test start ===\n"); 

    mut_printf(MUT_LOG_LOW, "Create Private RG-1\n");
    status = fbe_api_create_rg(&rg_request[0], FBE_TRUE, OH_SMILING_TEST_RG_TIMEOUT, &rg_id[0], &job_error_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "Create Non-Private RG-2\n");
    status = fbe_api_create_rg(&rg_request[1], FBE_TRUE, OH_SMILING_TEST_RG_TIMEOUT, &rg_id[1], &job_error_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "Create Private LUN-1 on RG-1\n");
    oh_smiling_create_lun(rg_request[0].raid_group_id, rg_request[0].raid_type,
                          lun_num[0], FBE_TRUE, &lun_id[0]);
    
    mut_printf(MUT_LOG_LOW, "Create Non-Private LUN-2 on RG-2\n");
    oh_smiling_create_lun(rg_request[1].raid_group_id, rg_request[1].raid_type,
                          lun_num[1], FBE_FALSE, &lun_id[1]);

    mut_printf(MUT_LOG_LOW, "Reboot...\n");
    oh_smiling_test_system_reboot();

    mut_printf(MUT_LOG_LOW, "Check LUN-1 is Private\n");
    /* Verify LUN get to ready state in reasonable time before we get lun_info. */
    status = fbe_api_wait_for_object_lifecycle_state(lun_id[0], 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    lun_info.lun_object_id = lun_id[0];
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(lun_info.user_private, FBE_TRUE);

    mut_printf(MUT_LOG_LOW, "Check LUN-2 is Non-Private\n");
    /* Verify LUN get to ready state in reasonable time before we get lun_info. */
    status = fbe_api_wait_for_object_lifecycle_state(lun_id[1], 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    lun_info.lun_object_id = lun_id[1];
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(lun_info.user_private, FBE_FALSE);
    
    mut_printf(MUT_LOG_LOW, "Check RG-1 is Private\n");
    /* Verify LUN get to ready state in reasonable time before we get lun_info. */
    status = fbe_api_wait_for_object_lifecycle_state(rg_id[0], 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_raid_group_get_info(rg_id[0], &raid_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(raid_info.user_private, FBE_TRUE);

    mut_printf(MUT_LOG_LOW, "Check RG-2 is Non-Private\n");
    /* Verify LUN get to ready state in reasonable time before we get lun_info. */
    status = fbe_api_wait_for_object_lifecycle_state(rg_id[1], 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_raid_group_get_info(rg_id[1], &raid_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(raid_info.user_private, FBE_FALSE);
    
    mut_printf(MUT_LOG_LOW, "=== Oh_smiling test complete ===\n");
    return;
}
/******************************************
 * end oh_smiling_test()
 ******************************************/

/*!****************************************************************************
 *  oh_smiling_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the oh_smiling test.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  3/7/2012 - Created. He Wei
 *****************************************************************************/

void oh_smiling_setup(void)
{

    mut_printf(MUT_LOG_LOW, "=== oh_smiling setup ===\n");    

    if (fbe_test_util_is_simulation())
    {
        /* Load the physical configuration */
        oh_smiling_load_physical_config();

        /* Load sep and neit packages */
        sep_config_load_sep_and_neit();
    }
    
    return; 
}
/******************************************
 * end oh_smiling_setup()
 ******************************************/

/*!**************************************************************
 * oh_smiling_test_system_reboot()
 ****************************************************************
 * @brief
 *  Reboot sep and neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/7/2012 - Created. He Wei
 ****************************************************************/
static void oh_smiling_test_system_reboot()
{
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(OH_SMILING_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/******************************************
 * end oh_smiling_test_system_reboot()
 ******************************************/
 
/*!****************************************************************************
 *  oh_smiling_create_lun
 ******************************************************************************
 *
 * @brief
 *  This function creates LUNs for oh_smiling_test.  
 *
 * @param   in_raid_group_id - raid group number
 * @param   in_raid_type - raid group type
 * @param   in_lun_number - lun number
 * @param   in_user_private - user private flag
 * @param   out_lun_object_id - output LUN object id
 *
 * @return  None 
 *
 * @author
 *   3/8/2012 - Created. He Wei 
 *****************************************************************************/
static void oh_smiling_create_lun(fbe_raid_group_number_t   in_raid_group_id,
                                          fbe_raid_group_type_t     in_raid_type,
                                          fbe_lun_number_t          in_lun_number,
                                          fbe_bool_t                in_user_private,
                                          fbe_object_id_t*          out_lun_object_id)
{
    fbe_status_t   status;
    fbe_api_lun_create_t fbe_lun_create_req;
    fbe_job_service_error_type_t job_error_type;


    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUN: %d ===\n", in_lun_number);

    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = in_raid_type;
    fbe_lun_create_req.raid_group_id = in_raid_group_id; 
    fbe_lun_create_req.lun_number = in_lun_number;
    fbe_lun_create_req.capacity = 500;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.user_private = in_user_private;
    /* The wwn of create LUN should be unique */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
    
    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 
                                OH_SMILING_TEST_LUN_TIMEOUT, out_lun_object_id,
                                &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Create LUN Passed: %d\n", fbe_lun_create_req.lun_number);
 
    return;
}
/******************************************
 * end oh_smiling_create_lun()
 ******************************************/

/*************************
 * end file oh_smiling_test.c
 *************************/


