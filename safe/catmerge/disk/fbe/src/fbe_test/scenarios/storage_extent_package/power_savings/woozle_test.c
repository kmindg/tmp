/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file woozle_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests for raid group hibernation.
 *
 * @version
 *   12/07/2009 - Created. Moe Valois
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_test_common_utils.h"  
#include "fbe/fbe_random.h"


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char * woozle_short_desc = "Power savings capabilities (DUAL SP)";
char * woozle_long_desc =
    "\n"
    "\n"
	"The Woozle scenario tests that the objects synchronously hibernates on both SPs\n"
    "only when I/O from all attached LUNs stops.\n"
"\n"
"Dependencies:\n"
"    - Metadata Service infrastructure.\n"
"    - Hibernation infrastructure.\n"
"    - LUN object supports hibernation (Aurora scenario).\n"
"    - VR object supports hibernation (TBD scenario).\n"
"\n"
"Starting Config:\n"
"    [PP] armada board\n"
"    [PP] SAS PMC port\n"
"    [PP] viper enclosure\n"
"    [PP] 5 SAS drives (PDO)\n"
"    [PP] 5 logical drives (LD)\n"
"    [SEP] 5 provision drives (PVD1 - PVD5)\n"
"    [SEP] 5 virtual drives (VD1 - VD5)\n"
"    [SEP] 1 raid object (RAID-0)\n"
"    [SEP] 2 lun objects (LU1-LU2)\n"
"\n"
"STEP 1: Bring up the initial topology on both SPs.\n"
"STEP 2: Enable power saving on acitve side, make sure it is reflect on passive\n"
"STEP 3: Set up RG power saving parameters on active side, make sure it is reflected on passive\n"
"STEP 4: Wait for objects on active side to save power\n"
"STEP 5: Verify objects save power"
"STEP 6: Verify passive side saves power too.\n"
"STEP 7: Start IO on active SP\n"
"STEP 8: Verify IO woke up both active and passive sides\n"
"STEP 9: Wait for objects on both sides to save power again\n"
"STEP 10: Disbale system power saving on active\n"
"STEP 11: Verify passive side is disabled too\n"
"STEP 12: Wait for objects on boths ide to get out of power savings\n"
"    Last update 10/21/2011\n"
    ;



//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//-----------------------------------------------------------------------------
//  FUNCTIONS:

//  Configuration parameters 
#define WOOZLE_TEST_RAID_GROUP_COUNT          	1       // number of RAID groups.
#define WOOZLE_TEST_LUNS_PER_RAID_GROUP       	1       // number of LUNs in the RG 
#define WOOZLE_TEST_ELEMENT_SIZE              	128     // element size 
#define WOOZLE_RG_ID							0

//  Define the bus-enclosure-disk for the disks in the RG 
#define WOOZLE_TEST_PORT                      0       // port (bus) 0 
#define WOOZLE_TEST_ENCLOSURE                 0       // enclosure 0 
#define WOOZLE_TEST_SLOT_1                    4       // slot for first disk in the RG: VD1  - 0-0-4 
#define WOOZLE_TEST_SLOT_2                    5       // slot for second disk in the RG: VD2 - 0-0-5 
#define WOOZLE_TEST_SLOT_3                    6       // slot for third disk in the RG: VD3  - 0-0-6 

#define WOOZLE_TEST_ENCL_MAX 1
#define WOOZLE_TEST_NUMBER_OF_PHYSICAL_OBJECTS 18 /* (1 board + 1 port + 1 encl + 15 pdo) */
#define WOOZLE_JOB_SERVICE_CREATION_TIMEOUT   150000

static fbe_sim_transport_connection_target_t 	active_connection_target = FBE_SIM_INVALID_SERVER;
static fbe_sim_transport_connection_target_t 	passive_connection_target = FBE_SIM_INVALID_SERVER;
static fbe_lun_number_t    						lun_number = 123;
static fbe_api_rdgen_context_t 					woozle_test_rdgen_contexts[5];

static void woozle_test_load_physical_config(void);

static fbe_test_rg_configuration_t woozle_rg_config[] = 
{
    /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    {3,     0xA000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        0,          0},  
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!**************************************************************
 * woozle_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Woozle test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void woozle_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
    fbe_test_discovered_drive_locations_t drive_locations;

    /*init the raid group configuration*/
    fbe_test_sep_util_init_rg_configuration(woozle_rg_config);

    /*find all power save capable drive information from disk*/
    fbe_test_sep_util_discover_all_power_save_capable_drive_information(&drive_locations);

    /*fill up the drive information into raid group configuration*/
    fbe_test_sep_util_setup_rg_config(woozle_rg_config, &drive_locations);
		
    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(woozle_rg_config);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(woozle_rg_config[0].job_number,
                                         WOOZLE_JOB_SERVICE_CREATION_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(woozle_rg_config[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 35000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End woozle_test_create_rg() */

static void woozle_logical_setup(void)
{  
    
    fbe_status_t                    status;
	fbe_api_lun_create_t        	fbe_lun_create_req;
	fbe_object_id_t					lu_id;
    fbe_job_service_error_type_t    job_error_type;

	/* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
	fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
	fbe_lun_create_req.raid_group_id = WOOZLE_RG_ID; /*BUBBLE_BASS_TEST_RAID_GROUP_ID is 0 so this is what we use*/
	fbe_lun_create_req.lun_number = lun_number;
	fbe_lun_create_req.capacity = 0x1000;
	fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
	fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
	fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

	status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 100000, &lu_id, &job_error_type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	

	mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);


} 

static void woozle_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
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

static void woozle_test_setup_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
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

static void woozle_test_write_background_pattern(fbe_block_count_t element_size, fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &woozle_test_rdgen_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUN
     */
    woozle_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                    0x1000, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK
     */
    woozle_test_setup_fill_rdgen_test_context( context_p,
                                                    lun_object_id,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_RDGEN_OPERATION_READ_CHECK,
                                                    0x1000, /* use max capacity */
                                                    128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;

} /* end woozle_test_write_background_pattern() */

static fbe_status_t woozle_test_run_io_load(fbe_object_id_t lun_object_id, fbe_u32_t timeout_in_sec)
{
	fbe_api_rdgen_context_t*    context_p = &woozle_test_rdgen_contexts[0];
    fbe_status_t                status;


    /* Write a background pattern; necessary until LUN zeroing is fully supported */
    woozle_test_write_background_pattern(128, lun_object_id);

    /* Set up for issuing reads forever
     */
    woozle_test_setup_rdgen_test_context(  context_p,
                                                lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
                                                FBE_LBA_INVALID,    /* use capacity */
                                                0,      /* run forever*/ 
                                                3,      /* threads per lun */
                                                520);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_thread_delay(timeout_in_sec);/*let IO run for a while*/

	status = fbe_api_rdgen_stop_tests(context_p, 1);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;

}

/*!**************************************************************
 * woozle_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Woozle test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void woozle_test_load_physical_config(void)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[WOOZLE_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < WOOZLE_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < WOOZLE_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
			if (slot >=0 && slot <= 3) {
				fbe_test_pp_util_insert_sas_drive(0, enclosure, slot, 520, 
												  TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, 
												  &drive_handle);
			}else{
				fbe_test_pp_util_insert_sas_drive(0, enclosure, slot, 520, 
												  TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, 
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
    status = fbe_api_wait_for_number_of_objects(WOOZLE_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == WOOZLE_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End woozle_test_load_physical_config() */

/*!****************************************************************************
 *  woozle_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Woozle test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void woozle_test_setup(void)
{
    /* Only load config in simulation.
       */
    if (fbe_test_util_is_simulation())
    {
	     /* we do the setup on SPB side*/
	    mut_printf(MUT_LOG_LOW, "=== woozle_test configurations ===");
	    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Bubble Bass test ===\n");

	    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
	    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	    /* Load the physical configuration */
	    woozle_test_load_physical_config();
	    /* Load sep and neit packages */
	    sep_config_load_sep_and_neit();

	    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
	    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	    /* Load the physical configuration */
	    woozle_test_load_physical_config();
	    /* Load sep and neit packages */
	    sep_config_load_sep_and_neit();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;

} /* End woozle_test_setup() */


/*!****************************************************************************
 *  woozle_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Woozle test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void woozle_setup(void)
{  
    fbe_cmi_service_get_info_t spa_cmi_info;
    fbe_cmi_service_get_info_t spb_cmi_info;

    mut_printf(MUT_LOG_LOW, "=== whoozle setup ===\n");   
    woozle_test_setup();
	
    do {
        fbe_api_sleep(100);
        /* We have to figure out whitch SP is ACTIVE */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_api_cmi_service_get_info(&spa_cmi_info);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_api_cmi_service_get_info(&spb_cmi_info);

        if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
            active_connection_target = FBE_SIM_SP_A;
            passive_connection_target = FBE_SIM_SP_B;
        }

        if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
            active_connection_target = FBE_SIM_SP_B;
            passive_connection_target = FBE_SIM_SP_A;
        }
    }while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);

    /* we do the setup on ACTIVE side*/
    mut_printf(MUT_LOG_LOW, "=== woozle Loading Config on ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_set_target_server(active_connection_target);

    mut_printf(MUT_LOG_LOW, "=== Creating RG-0 ===");
    woozle_test_create_rg();
    mut_printf(MUT_LOG_LOW, "=== Creating LUN ===");
    woozle_logical_setup();

    return;
} //  End woozle_setup()


/*!****************************************************************************
 *  woozle_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Woozle test
 *****************************************************************************/
void woozle_test(void)
{
    fbe_object_id_t                             rg_object_id, lu_object_id, object_id, pdo_object_id;
    fbe_status_t								status;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_u32_t									disk = 0; 
    fbe_lifecycle_state_t						object_state;
    fbe_lun_set_power_saving_policy_t           lun_ps_policy;
    fbe_system_power_saving_info_t				system_info;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
	rg_config_p = &woozle_rg_config[0];
	
	
    /*let's put the active SP in hibernation*/
    fbe_api_sim_transport_set_target_server(active_connection_target);
    
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*make sure LUN is ready*/
	status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    lun_ps_policy.lun_delay_before_hibernate_in_sec = 10;/*this is for 5 seconds delay before hibernation*/
    lun_ps_policy.max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000);/*we allow 1000 minutes for the drives to wake up (default for drives is 100 minutes)*/

    /* Make sure the LUN is not saving power */
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_lun_set_power_saving_policy(lu_object_id, &lun_ps_policy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(passive_connection_target);

    /*make sure LUN is ready on the peer as well*/
    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure the LUN is not saving power */
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_lun_set_power_saving_policy(lu_object_id, &lun_ps_policy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(active_connection_target);
    
	/* checking if VOL_ATTR_STANDBY_POSSIBLE is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 1000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_STANDBY_POSSIBLE is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 1000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
    
	/*and enable the power saving on the RG*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Enabling power saving on the Active RAID ===\n");
    
    status = fbe_api_database_lookup_raid_group_by_number(WOOZLE_RG_ID, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== changing power saving time of all bound objects to 20 seconds ===\n");
    
    /* Make sure the RG is not saving power */
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, 20);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);
    MUT_ASSERT_INTEGER_EQUAL(object_ps_info.configured_idle_time_in_seconds, 20);

    mut_printf(MUT_LOG_TEST_STATUS, "===Enabling power saving on RG(should propagate to all objects) ===\n");
    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	
	/* checking if VOL_ATTR_STANDBY_POSSIBLE is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_STANDBY_POSSIBLE is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);

    for (disk = 0; disk < rg_config_p->width; disk++) 
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Disable background zeroing so that drives go into power saving mode*/
         status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fbe_api_sim_transport_set_target_server(passive_connection_target);

    /* Make sure the RG is not saving power */
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    MUT_ASSERT_INTEGER_EQUAL(object_ps_info.configured_idle_time_in_seconds, 20);

    for (disk = 0; disk < rg_config_p->width; disk++) 
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Disable background zeroing so that drives go into power saving mode*/
         status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fbe_api_sim_transport_set_target_server(active_connection_target);
	
	mut_printf(MUT_LOG_TEST_STATUS, " %s: Enabling system power save \n", __FUNCTION__);
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Enabling system power save statistics. \n", __FUNCTION__);
    status  = fbe_api_enable_system_power_save_statistics();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_get_system_power_save_info(&system_info);
    MUT_ASSERT_TRUE(system_info.enabled == FBE_TRUE);
    MUT_ASSERT_TRUE(system_info.stats_enabled == FBE_TRUE);
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /*now let's wait and make sure the LU and other objects connected to it are hibernating*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting up to 120 sec. for system to power save ===\n");
	
	/* checking if VOL_ATTR_IDLE_TIME_MET  is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_IDLE_TIME_MET is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
	
    /* verify LUN can handle IO while it's waiting for hibernate timer */
    mut_printf(MUT_LOG_TEST_STATUS, " %s: hibernation interrupted by IO\n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, " %s: Running IO on LU for 20 seconds to wake up objects \n", __FUNCTION__);
    woozle_test_run_io_load(lu_object_id, 10000);
	/* checking if VOL_ATTR_IDLE_TIME_MET  is cleared*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET is cleared! \n", __FUNCTION__, lu_object_id);
		fbe_api_sim_transport_set_target_server(passive_connection_target);
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET is cleared on passived side! \n", __FUNCTION__, lu_object_id);
	fbe_api_sim_transport_set_target_server(active_connection_target);

    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified LU is NOT saving power \n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified LU is NOT saving power on passive side as well\n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /*now let's wait and make sure the LU and other objects connected to it are hibernating*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting up to 120 sec. for system to power save again ===\n");
	
	/* checking if VOL_ATTR_IDLE_TIME_MET  is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_IDLE_TIME_MET is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	    
    /* checking if VOL_ATTR_STANDBY_POSSIBLE is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 1000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_STANDBY_POSSIBLE is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_POSSIBLE, 1000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_POSSIBLE set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
    
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot,&pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 180000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified PDO is saving power \n", __FUNCTION__);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Making sure all bound objects saving power ===\n");
    status = fbe_api_get_object_lifecycle_state(lu_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    
    /*make sure LU is saving power */
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_SAVING_POWER);
    
    /*make sure RAID saving power */
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_SAVING_POWER);
    
    /*make sure VD saving power */
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_SAVING_POWER);
    
    /*make sure PVD saving power */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_SAVING_POWER);
    
    
    /*make sure it shows up on the other side, it might take a second or so to show up so let's wait a bit.*/
    fbe_api_sim_transport_set_target_server(passive_connection_target);
	status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(active_connection_target);
	   	
	/* checking if VOL_ATTR_STANDBY_TIME_MET is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_STANDBY_TIME_MET is set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 80000, lu_object_id, FBE_TRUE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_TIME_MET set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
	
	/* checking if VOL_ATTR_IDLE_TIME_MET is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_IDLE_TIME_MET is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_IDLE_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_IDLE_TIME_MET not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
    
    /*run IO, make sure the drives are not saving power and then check the other side as well
    Pay attention LU is still sleeping until we disbale the power save on the object or the array*/
    
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Running IO on LU to wake up objects \n", __FUNCTION__);
    woozle_test_run_io_load(lu_object_id, 5000);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified RG is NOT saving power \n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified RG is NOT saving power on passive side as well\n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(active_connection_target);
    
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Waiting for objects to save power again \n", __FUNCTION__);
    /*after a minute (we set it before), objects should save power again*/
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Active side saves power, checking passive side \n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(active_connection_target);
	
    /*wake up the system and make sure it works on both ends*/
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system power save \n", __FUNCTION__); 
    status = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system power save statistics.\n", __FUNCTION__);
    status = fbe_api_disable_system_power_save_statistics();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_get_system_power_save_info(&system_info);
    MUT_ASSERT_TRUE(system_info.enabled == FBE_FALSE);
    MUT_ASSERT_TRUE(system_info.stats_enabled == FBE_FALSE);
    fbe_api_sim_transport_set_target_server(active_connection_target);
    
    /*wait for objects to be ready again*/
    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Verified LUN is NOT saving power \n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(lu_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(active_connection_target);
	
	/* Disable power saving on the raid group*/
	status = fbe_api_disable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling power saving on the raid group \n", __FUNCTION__); 
	
    /*let's wait for a while since RG propagate it to the LU*/
    fbe_api_sleep(14000);
	
	/*check it's disbaled*/
    status = fbe_api_get_object_power_save_info(lu_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);
    status = fbe_api_get_object_power_save_info(rg_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);
	
	/* checking if VOL_ATTR_STANDBY_TIME_MET is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_TIME_MET not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	
	/* checking if VOL_ATTR_STANDBY_TIME_MET is not set*/
	status = fbe_test_sep_utils_wait_for_attribute(VOL_ATTR_STANDBY_TIME_MET, 80000, lu_object_id, FBE_FALSE);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X Success! VOL_ATTR_STANDBY_TIME_MET not set! \n", __FUNCTION__, lu_object_id);
	
	fbe_api_sim_transport_set_target_server(active_connection_target);
    
    /*get an id of a bound drive and wait for it to get out of power save to indicate thwo system is not at READY and we can stop the test*/
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot, &object_id);
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

} 

void woozle_destroy(void)
{
    // Only destroy config in simulation  
    if (fbe_test_util_is_simulation())
    {
	    /*start with SPB so we leave the switch at SPA when we exit*/
	    mut_printf(MUT_LOG_LOW, "=== woozle UNLoading Config from SPB ===");
	    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	    fbe_test_sep_util_destroy_neit_sep_physical();
	    
	    /*back to A*/
	    mut_printf(MUT_LOG_LOW, "=== woozle UNLoading Config from SPA ===");
	    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	    fbe_test_sep_util_destroy_neit_sep_physical();
    }
	
}


