
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file larry_t_lobster_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for binding new LUNs.
 *
 * @version
 *   12/15/2009 - Created. Susan Rundbaken (rundbs)
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
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"
//#include "fbe/fbe_cmi_interface.h"
//#include "fbe/fbe_api_cmi_interface.h"


/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* larry_t_lobster_short_desc = "LUN Bind operation";
char* larry_t_lobster_long_desc =
    "\n"
    "\n"
    "The Larry T Lobster scenario tests binding new LUNs to\n"
    "an existing RAID Group on single or dual sp settings.\n"
    "\n"
    "Dependencies:\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 3 SAS drives (PDOs)\n"
    "    [PP] 3 logical drives (LDs)\n"
    "    [SEP] 3 provision drives (PVDs)\n"
    "    [SEP] 3 virtual drives (VDs)\n"
    "    [SEP] 1 RAID object (RAID-5)\n"
    "    [SEP] 1 Virtual RAID object (VR)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Create a virtual drive (VD) with an I/O edge to PVD\n"
    "        - Verify that PVD and VD are both in the READY state\n"
    "    - Create a RAID object(RO1) and attach edges to the virtual drives\n"
    "\n" 
    "STEP 2: Send a LUN Bind request using fbe_api\n"
    "   - Test with valid and invalid input parameters\n"
    "      - invalid RG ID\n" 
    "      - invalid LUN capacity\n" 
    "      - not enough space in RG\n"
    "   - Test with aligned and non-aligned capacity as input\n"
    "STEP 3: verifying the LUN object is created with the valid input\n"
    "        and not created with invalid input:\n"
    "       - Database Service has recorded object in its tables:\n" 
    "           - LUN ID\n" 
    "           - FBE Object ID\n"
    "           - LUN name\n"
    "           - RG ID\n"
    "           - LUN count\n"
    "           - imported capacity\n"
    "       - event log has the correct tracing\n"
    "STEP n: repeat STEP 2&3 with different configuration defined in larry_t_lobster_luns[]\n"
    "\n"
    "DualSP mode: repeat the aboved test on different SP defined in larry_t_lobster_dualsp_pass_config[]\n" 
    "\n"
    "Description last updated: 10/3/2011.\n";        
    
typedef struct larry_t_lobster_lun_s{
    fbe_api_lun_create_t    fbe_lun_create_req;
    fbe_bool_t              expected_to_fail;
    fbe_object_id_t         lun_object_id;
}larry_t_lobster_lun_t;

typedef struct larry_t_lobster_dualsp_pass_config_s{
    fbe_sim_transport_connection_target_t creation_sp;
    fbe_sim_transport_connection_target_t destroy_sp;
    fbe_sim_transport_connection_target_t reboot_sp;
}larry_t_lobster_dualsp_pass_config_t;

#define LARRY_T_LOBSTER_MAX_PASS_COUNT 8

static larry_t_lobster_dualsp_pass_config_t larry_t_lobster_dualsp_pass_config[LARRY_T_LOBSTER_MAX_PASS_COUNT] =
{
    {FBE_SIM_SP_A, FBE_SIM_SP_A, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/ 
    {FBE_SIM_SP_B, FBE_SIM_SP_B, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_A, FBE_SIM_SP_B, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_B, FBE_SIM_SP_A, FBE_SIM_INVALID_SERVER},/*FBE_SIM_INVALID_SERVER means no reboot*/
    {FBE_SIM_SP_A, FBE_SIM_SP_A, FBE_SIM_SP_A}, 
    {FBE_SIM_SP_B, FBE_SIM_SP_B, FBE_SIM_SP_B},
    {FBE_SIM_SP_A, FBE_SIM_SP_B, FBE_SIM_SP_A},
    {FBE_SIM_SP_B, FBE_SIM_SP_A, FBE_SIM_SP_B},

};
/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo) 
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of 520 blocks in the physical drive.
 *        6TB Capacity (512 bps blocks) =
 *           2048 (1MB_blocks) * 1024 * 1024 * 6 =  0x300000000
 *           
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY             (0x300000000)  /*6TB*/ 



/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_RAID_GROUP_ID          0

#define LARRY_T_LOBSTER_INVALID_TEST_RAID_GROUP_ID  5


/*!*******************************************************************
 * @var larry_t_lobster_disk_set_520
 *********************************************************************
 * @brief This is the disk set for the 520 RG (520 byte per block SAS)
 *
 *********************************************************************/
static fbe_u32_t larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT][LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH][3] = 
{
    /* width = 3 */
    { {0,0,3}, {0,0,4}, {0,0,5}}
};


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_LUN_COUNT        7


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_LUN_NUMBER
 *********************************************************************
 * @brief  Lun number used by this test
 *
 *********************************************************************/
static larry_t_lobster_lun_t larry_t_lobster_luns [LARRY_T_LOBSTER_TEST_LUN_COUNT] = {
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,00},
        {0xD3,0xE4,0xF0,0x00}, 
        123, 
        0x1000, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_FALSE},
     FBE_FALSE},
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,01},
        {0xD3,0xE4,0xF1,0x00}, 
        234, 
        0x555, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_FALSE}, 
    FBE_FALSE},
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,02},
        {0xD3,0xE4,0xF2,0x00}, 
        456, 
        0x1000, 
        FBE_BLOCK_TRANSPORT_FIRST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_TRUE},
    FBE_FALSE},
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID,
        {01,02,03,04,05,06,07,03},
        {0xD3,0xE4,0xF3,0x00},
        456,
        0x1000,
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID,
        FBE_FALSE,
        FBE_FALSE}, 
    FBE_TRUE}, /* this lun create will fail, due to duplicate lun number */
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,03},
        {0xD3,0xE4,0xF3,0x00}, 
        678, 
        FBE_LBA_INVALID / 2,
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID,
        FBE_FALSE,
        FBE_FALSE}, 
    FBE_TRUE},/* this lun create will fail, due to capacity excceeds RG cap */
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, 
        {01,02,03,04,05,06,07,03},
        {0xD3,0xE4,0xF3,0x00}, 789, 
        FBE_LBA_INVALID,
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID,
        FBE_FALSE,
        FBE_FALSE},
    FBE_TRUE},/* this lun create will fail, due to capacity is invalid */
    {{FBE_RAID_GROUP_TYPE_RAID5, 
        LARRY_T_LOBSTER_INVALID_TEST_RAID_GROUP_ID,
        {01,02,03,04,05,06,07,03},
        {0xD3,0xE4,0xF3,0x00}, 
        999,
        0x1000, 
        FBE_BLOCK_TRANSPORT_BEST_FIT,
        FBE_LBA_INVALID, 
        FBE_FALSE,
        FBE_FALSE}, 
    FBE_TRUE},/* this lun create will fail, due to invalid RG id*/
};

/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_NS_TIMEOUT        90000 /*wait maximum of 90 seconds*/


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
extern void fbe_test_check_launch_cli(void);

static void larry_t_lobster_test_load_physical_config(void);
static void larry_t_lobster_test_create_logical_config(void);
static void larry_t_lobster_test_calculate_imported_capacity(void);
static void larry_t_lobster_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p);
static void larry_t_lobster_verify_database_lun_config_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  in_fbe_lun_create_req_p);
static void larry_t_lobster_verify_lun_creation(larry_t_lobster_lun_t * lun_config, fbe_u32_t expected_lun_count);
static void larry_t_lobster_reboot_sp(void);


/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  larry_t_lobster_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Larry T Lobster test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_test(void)
{
    fbe_status_t                        status;
    fbe_u32_t                           i, expected_lun_count;
    fbe_job_service_error_type_t        job_error_type;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Larry T Lobster Test ===\n");

    mut_pause();
    fbe_test_check_launch_cli();

    /* get the number of LUNs in the system for verification use */
    status = fbe_test_sep_util_get_user_lun_count(&expected_lun_count);

    for (i = 0; i < LARRY_T_LOBSTER_TEST_LUN_COUNT; i++)
    {
        status = fbe_api_create_lun(&larry_t_lobster_luns[i].fbe_lun_create_req, 
                                    FBE_TRUE, 
                                    LARRY_T_LOBSTER_TEST_NS_TIMEOUT, 
                                    &larry_t_lobster_luns[i].lun_object_id,
                                    &job_error_type);
        if (larry_t_lobster_luns[i].expected_to_fail)
        {
            MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun expected to fail, but it succeeded!!!"); 
            if (larry_t_lobster_luns[i].fbe_lun_create_req.lun_number == 456)
            {
                MUT_ASSERT_INT_EQUAL(job_error_type, FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE);
            }
        }else{
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun failed!!!"); 
            expected_lun_count++;
        }

        /*verify the lun create result */
        larry_t_lobster_verify_lun_creation(&larry_t_lobster_luns[i], expected_lun_count);
    }

    return;

}/* End larry_t_lobster_test() */


/*!****************************************************************************
 *  larry_t_lobster_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Larry T Lobster test. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_test_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Larry T Lobster test ===\n");

    /* Load the physical config on the target server
     */    
    larry_t_lobster_test_load_physical_config();

    /* Load the SEP package on the target server
     */
	sep_config_load_sep();

    fbe_test_wait_for_all_pvds_ready();

    /* Create a test logical configuration; will be created on active side by this function */
    larry_t_lobster_test_create_logical_config();

    return;

} /* End larry_t_lobster_test_setup() */


/*!****************************************************************************
 *  larry_t_lobster_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Larry T Lobster test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_test_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Larry T Lobster test ===\n");

    fbe_test_sep_util_destroy_sep_physical();

    return;

} /* End larry_t_lobster_test_cleanup() */


/*
 *  The following functions are Larry T Lobster test functions. 
 *  They test various features of the LUN Object focusing on LUN creation (bind).
 */


/*!****************************************************************************
 *  larry_t_lobster_test_calculate_imported_capacity
 ******************************************************************************
 *
 * @brief
 *   This is the test function for calculating the imported capacity of a LUN.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void larry_t_lobster_test_calculate_imported_capacity(void)
{
    fbe_api_lun_calculate_capacity_info_t  capacity_info;
    fbe_status_t                           status;
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== Calculate imported capacity ===\n");

    /* set a random capacity for test purpose */
    capacity_info.exported_capacity = 0x100000;

    /* calculate the imported capacity */
    status = fbe_api_lun_calculate_imported_capacity(&capacity_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== exported capacity: %llu ===\n",
	       (unsigned long long)capacity_info.exported_capacity);
    mut_printf(MUT_LOG_TEST_STATUS, "=== imported capacity: %llu ===\n",
	       (unsigned long long)capacity_info.imported_capacity);

    return;

}  /* End larry_t_lobster_test_calculate_imported_capacity() */


/*
 * The following functions are utility functions used by this test suite
 */


/*!**************************************************************
 * larry_t_lobster_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Larry T Lobster test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void larry_t_lobster_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      encl0_1_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;


    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

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
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 4; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 4; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End larry_t_lobster_test_load_physical_config() */


/*!**************************************************************
 * larry_t_lobster_test_create_logical_config()
 ****************************************************************
 * @brief
 *  Create the Larry T Lobster test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void larry_t_lobster_test_create_logical_config(void)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_u32_t                                           iter;
    fbe_api_raid_group_create_t                         fbe_raid_group_create_req;
    fbe_job_service_error_type_t                        job_error_type;

    
    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create the Logical Configuration ===\n");

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");

    fbe_zero_memory(&fbe_raid_group_create_req, sizeof(fbe_api_raid_group_create_t));
    fbe_raid_group_create_req.b_bandwidth = 0;
    fbe_raid_group_create_req.capacity = FBE_LBA_INVALID; /* not specifying max number of LUNs for test */
    //fbe_raid_group_create_req.explicit_removal = FBE_FALSE;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = LARRY_T_LOBSTER_TEST_RAID_GROUP_ID;
    fbe_raid_group_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_raid_group_create_req.drive_count = 3;
    for (iter = 0; iter < 3; ++iter)    {
        fbe_raid_group_create_req.disk_array[iter].bus = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][0];
        fbe_raid_group_create_req.disk_array[iter].enclosure = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][1];
        fbe_raid_group_create_req.disk_array[iter].slot = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][2];
    }
    status = fbe_api_create_rg(&fbe_raid_group_create_req, FBE_TRUE, LARRY_T_LOBSTER_TEST_NS_TIMEOUT, &rg_object_id, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    status = fbe_api_create_rg(&fbe_raid_group_create_req, FBE_TRUE, LARRY_T_LOBSTER_TEST_NS_TIMEOUT, &rg_object_id, &job_error_type);
    MUT_ASSERT_INT_EQUAL(job_error_type, FBE_JOB_SERVICE_ERROR_RAID_GROUP_ID_IN_USE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); 

    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End larry_t_lobster_test_create_logical_config() */

static void larry_t_lobster_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  *in_fbe_lun_create_req_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;
    fbe_object_id_t rg_obj_id;
	fbe_object_id_t object_id;

    lun_info.lun_object_id = in_lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_lun_get_info failed!");

    /* check rd group obj_id*/
    status = fbe_api_database_lookup_raid_group_by_number(in_fbe_lun_create_req_p->raid_group_id, &rg_obj_id);
    MUT_ASSERT_INTEGER_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INTEGER_EQUAL_MSG(rg_obj_id, lun_info.raid_group_obj_id, "Lun_get_info return mismatch rg_obj_id");

    MUT_ASSERT_INTEGER_EQUAL_MSG((lun_info.offset % lun_info.raid_info.lun_align_size),0, "Lun_get_info return mismatch offset" );
    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->capacity, lun_info.capacity, "Lun_get_info return mismatch capacity" );
    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->raid_type, lun_info.raid_info.raid_type, "Lun_get_info return mismatch raid_type" );
    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->world_wide_name, 
                                (char *)&lun_info.world_wide_name, 
                                sizeof(lun_info.world_wide_name),
                                "lun_get_info return mismatch WWN!");

    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->user_defined_name, 
                                (char *)&lun_info.user_defined_name, 
                                sizeof(lun_info.user_defined_name),
                                "lun_get_info return mismatch user_defined_name!");

	/*make sure the LUN WWN can be used to track the LUN*/
	status = fbe_api_database_lookup_lun_by_wwid(in_fbe_lun_create_req_p->world_wide_name, &object_id);
	MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_database_lookup_lun_by_wwid failed!");
	MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(object_id, FBE_OBJECT_ID_INVALID, "failed to get object id");
	/* Verify lun.rotational_rate is 0 since rg/lun is built on top of SAS/SATA drive instead of flash drive.*/
    MUT_ASSERT_INTEGER_EQUAL_MSG(lun_info.rotational_rate, 0, "Lun_get_info return mismatch rotational rate.");
}

static void larry_t_lobster_verify_database_lun_config_info(fbe_object_id_t in_lun_object_id, fbe_api_lun_create_t  in_fbe_lun_create_req_p)
{
    fbe_status_t status;
    fbe_database_get_tables_t tables;

    status = fbe_api_database_get_tables(in_lun_object_id, &tables);
    MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_database_get_tables failed!");

    /* check noinitialverify_b*/
    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p.noinitialverify_b, 
                                 ((tables.object_entry.set_config_union.lu_config.config_flags
                                  &FBE_LUN_CONFIG_NO_INITIAL_VERIFY) == FBE_LUN_CONFIG_NO_INITIAL_VERIFY), 
                                 "database_lu_config return mismatch noinitialverify" );
}

static void larry_t_lobster_verify_lun_creation(larry_t_lobster_lun_t * lun_config, fbe_u32_t expected_lun_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t active_target;
    fbe_sim_transport_connection_target_t passive_target;
    fbe_u32_t lun_count;
    fbe_sim_transport_connection_target_t current_target = fbe_api_transport_get_target_server();
    fbe_bool_t b_run_on_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    if (b_run_on_dualsp)
    {
        /* Get the active SP */
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
        /* set the passive SP */
        passive_target = (active_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

        /* verify on the active SP first */
        fbe_api_sim_transport_set_target_server(active_target);
    }
    
    status = fbe_test_sep_util_get_user_lun_count(&lun_count);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(expected_lun_count, lun_count); 
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: Found expected %d number of LUNs. \n", 
               __FUNCTION__,
               expected_lun_count);
    if ( !lun_config->expected_to_fail )
    {
        larry_t_lobster_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
        larry_t_lobster_verify_database_lun_config_info(lun_config->lun_object_id, lun_config->fbe_lun_create_req);
        /*verify event logging for lun on active side*/
        /* check that given event message is logged correctly */
        status = fbe_test_sep_util_wait_for_LUN_create_event(lun_config->lun_object_id, 
                                             lun_config->fbe_lun_create_req.lun_number); 

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if (b_run_on_dualsp)
    {
        mut_printf(MUT_LOG_LOW, "=== Verify on ACTIVE side (%s) Passed ===", active_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "=== Verify on (%s) Passed ===", current_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    }
    

    if ( b_run_on_dualsp )
    {
        /* verify on the passive SP */
        fbe_api_sim_transport_set_target_server(passive_target);
        status = fbe_test_sep_util_get_user_lun_count(&lun_count);
        MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INTEGER_EQUAL(expected_lun_count, lun_count); 
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Found expected %d number of LUNs. \n", 
                   __FUNCTION__,
                   expected_lun_count);
        if ( !lun_config->expected_to_fail )
        {
            larry_t_lobster_verify_lun_info(lun_config->lun_object_id, &lun_config->fbe_lun_create_req);
            larry_t_lobster_verify_database_lun_config_info(lun_config->lun_object_id, lun_config->fbe_lun_create_req);
            /*verify event logging for lun on passive side*/
			/* check that given event message is logged correctly */
            status = fbe_test_sep_util_wait_for_LUN_create_event(lun_config->lun_object_id, 
												lun_config->fbe_lun_create_req.lun_number); 

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        mut_printf(MUT_LOG_LOW, "=== Verify on Passive side (%s) Passed ===", passive_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    }
}


/*!****************************************************************************
 *  larry_t_lobster_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Larry T Lobster test in dualsp mode. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Larry T Lobster dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        larry_t_lobster_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        larry_t_lobster_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        fbe_test_wait_for_all_pvds_ready();
    }

    /* Create a test logical configuration */
    larry_t_lobster_test_create_logical_config();

    return;

} /* End larry_t_lobster_setup() */


/*!****************************************************************************
 *  larry_t_lobster_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Larry T Lobster test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Larry T Lobster test ===\n");

    fbe_test_sep_util_destroy_neit_sep_physical_both_sps();

    return;

} /* End larry_t_lobster_dualsp_test_cleanup() */

static void larry_t_lobster_reboot_sp(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(LARRY_T_LOBSTER_TEST_NS_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


void larry_t_lobster_dualsp_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t active_target;

    fbe_u32_t pass;
	fbe_u32_t                                 lun_number = 123;
    fbe_bool_t                                is_msg_present;
    fbe_u32_t                           i;
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    for (pass = 0 ; pass < LARRY_T_LOBSTER_MAX_PASS_COUNT; pass++) /* TODO: change to LARRY_T_LOBSTER_MAX_PASS_COUNT when entry id synch is implemented */
    {

        /* Get the active SP */
        fbe_test_sep_get_active_target_id(&active_target);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Active SP is %s ===\n", pass,
                   active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

        fbe_api_sim_transport_set_target_server(larry_t_lobster_dualsp_pass_config[pass].creation_sp);
        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Create LUNs on %s ===\n", pass,
                   larry_t_lobster_dualsp_pass_config[pass].creation_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Now run the create tests
         */
        larry_t_lobster_test();
//      fbe_status_t                        status;
//  fbe_u32_t                           i, expected_lun_count;
//
//  mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Larry T Lobster Test ===\n");
//
//  /* get the number of LUNs in the system for verification use */
//  status = fbe_test_sep_util_get_user_lun_count(&expected_lun_count);
//
//  for (i = 0; i < LARRY_T_LOBSTER_TEST_LUN_COUNT; i++)
//  {
//      status = fbe_api_create_lun(&larry_t_lobster_luns[i].fbe_lun_create_req,
//                                  FBE_TRUE,
//                                  LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
//                                  &larry_t_lobster_luns[i].lun_object_id);
//      if (larry_t_lobster_luns[i].expected_to_fail)
//      {
//          MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun expected to fail, but it succeeded!!!");
//      }else{
//          MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_create_lun failed!!!");
//          expected_lun_count++;
//      }
//
//      /*verify the lun create result */
//      larry_t_lobster_verify_lun_creation(&larry_t_lobster_luns[i], expected_lun_count);
//  }
    
        /* reboot */
        if (larry_t_lobster_dualsp_pass_config[pass].reboot_sp != FBE_SIM_INVALID_SERVER)
        {
            fbe_api_sim_transport_set_target_server(larry_t_lobster_dualsp_pass_config[pass].reboot_sp);
            mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Rebooting %s ===\n", pass,
                       larry_t_lobster_dualsp_pass_config[pass].reboot_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
            larry_t_lobster_reboot_sp();
        }

        fbe_api_sim_transport_set_target_server(larry_t_lobster_dualsp_pass_config[pass].destroy_sp);

        //verify lun creation
        for (i = 0; i < LARRY_T_LOBSTER_TEST_LUN_COUNT; i++)
        {
            if (!larry_t_lobster_luns[i].expected_to_fail)
            {
              /*verify the lun create result */
              //larry_t_lobster_verify_lun_creation(&larry_t_lobster_luns[i], expected_lun_count); 

              larry_t_lobster_verify_database_lun_config_info(larry_t_lobster_luns[i].lun_object_id, larry_t_lobster_luns[i].fbe_lun_create_req);
            }  
        }

        mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Destroy LUNs on %s ===\n", pass,
                   larry_t_lobster_dualsp_pass_config[pass].destroy_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        /* destroy */
        status = fbe_test_sep_util_destroy_all_user_luns();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
		/*Verify if user lun 123 is destroyed and the event is logged on both SPs*/
        /* check that given event message is logged correctly on active side*/
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);	
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                             &is_msg_present, 
    										 SEP_INFO_LUN_DESTROYED,										
                                             lun_number); 

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");
	
        /* check that given event message is logged correctly on passive side */
        fbe_api_sleep(LARRY_T_LOBSTER_TEST_NS_TIMEOUT);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                             &is_msg_present, 
                                             SEP_INFO_LUN_DESTROYED,									
    										 lun_number); 

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");

        /*end verify event logging*/    
    }

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

}
