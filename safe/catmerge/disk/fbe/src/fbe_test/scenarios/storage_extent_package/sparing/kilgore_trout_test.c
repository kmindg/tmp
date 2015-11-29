#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"    //  for fbe_api_database_lookup_raid_group_by_number
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe_test_package_config.h"              
#include "fbe/fbe_api_common.h"                  
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_job_service.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_job_service_interface.h" 
#include "fbe_test_common_utils.h"


char * kilgore_trout_short_desc = "drive sparing infrastructure test";
char * kilgore_trout_long_desc =
"\n"
"\n"
"The Kilgore Trout Scenario tests the config service infrastructure needed for drive sparing.\n"
"It covers the following cases:\n"
"\t- It verifies that sparing requests are held pending if a spare drive is not available.\n"
"\t- It verifies that config service policy can control when new drives become spare drives.\n"
"\t- It verifies that an unassigned drive can be designated as a hot spare.\n"
"\n"
"Dependencies: \n"
"\t- The virtual drive must provide drive sparing information.\n"
"\t- The config service must queue drive sparing requests.\n"
"\t- The config service must maintain a drive sparing policy.\n"
"\t- The config service must provide a control command for assigning a hot spare.\n"
"\n"
"Starting Config: \n"
"\t[PP] an armada board\n"
"\t[PP] a SAS PMC port\n"
"\t[PP] a viper enclosure\n"
"\t[PP] two SAS drives (PDO-A and PDO-B)\n"
"\t[PP] two logical drives (LDO-A and LDO-B)\n"
"\t[SEP] two provision drives (PVD-A and PVD-B)\n"
"\t[SEP] two virtual drives (VD-A and VD-B)\n"
"\t[SEP] a RAID-1 object, connected to the two VDs\n"
"\n"
"STEP 1: Bring up the initial topology.\n"
"\t- Create and verify the initial physical package config.\n"
"\t- Create a RAID-5 object with I/O edge to the virtual drives.\n"
"\t- Verify that RAID-5 object is in the READY state.\n"
"STEP 2: Make VD-A need a spare while queues are blocked.\n"
"\t- Configure PVD-D as a hot spare via a config service control command.\n"
"\t- Disable the sparing queue in the job service\n"
"\t- Remove the physical drive (PDO-A).\n"
"\t- Verify that VD-A is now in the FAIL state.\n"
"\t- Verify that VD-A can still supply the information needed for drive sparing.\n"
"\t- Verify that the job service has a pending request for a spare drive for VD-A.\n"
"STEP 3: Verify that the job service still has a pending job request\n"
"\t- Verify that the job service still has a pending request for a spare drive for VD-A.\n"
"STEP 4: Verify Hot spare processing after queues are re-enabled.\n"
"\t- Enable the sparing queue in the job service\n"
"\t- Verify that the job service processed the pending sparing request.\n"
"\t- Verify that VD-A now has an edge to PVD-D and is in the READY state.\n"
"STEP 5: Wait for rebuild of VD-A to complete.\n"
"\t- Verify that VD-A rebuilt completely.\n"
"\t- Verify that position 0 is no longer rebuild logging.\n"
"\t- Verify that the job service has a pending request for a spare drive for VD-A.\n"
"\t  (sent from the RAID-1 object).\n"
"STEP 6: Make VD-A need a spare again (and again there is no spare.\n"
"\t- Disable the sparing queue in the job service\n"
"\t- Remove the physical drive (PDO-B).\n"
"\t- Verify that VD-B is now in the FAIL state.\n"
"\t- Verify that the job service has a pending request for a spare drive for VD-A(sent from the RAID-5 object).\n"
"STEP 7: Cancel the pending spare request.\n"
"\t- Delete the RAID-1 object from the SEP config.\n"
"\t- Verify that the job service does not have a pending request for a spare drive for VD-A.\n"
;

/*!*******************************************************************
 * @def KILGORE_TROUT_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define KILGORE_TROUT_RAID_GROUP_WIDTH      3

#define KILGORE_TROUT_PVD_SIZE TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

#define KILGORE_TROUT_NUMBER_RAID_GROUPS   5

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a function
 *         register
 */
#define KILGORE_TROUT_TIMEOUT  5000 /*wait maximum of 5 seconds*/

#define KILGORE_TROUT_TEST_LUNS_PER_RAID_GROUP      3
#define KILGORE_TROUT_TEST_CHUNKS_PER_LUN           3 

/*!*******************************************************************
 * @def KILGORE_TROUT_RAID_GROUP_CAPACITY_IN_BLOCKS
 *********************************************************************
 * @brief raid group capacity.
 *********************************************************************/
#define KILGORE_TROUT_RAID_GROUP_CAPACITY_IN_BLOCKS (KILGORE_TROUT_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS * KILGORE_TROUT_DATA_ELEMENTS_PER_MIRROR_STRIPE) 

/*****************************************
 * GLOBAL DATA
 *****************************************/

/* Number of physical objects in the test configuration. 
*/
fbe_u32_t   kilgore_trout_test_number_physical_objects_g;

/*!*******************************************************************
 * @enum kilgore_trout_sep_object_ids_e
 *********************************************************************
 * @brief Enumeration of the logical objects used by Kilgore Trout test
 * 
 *********************************************************************/
enum kilgore_trout_sep_object_ids_e {
    KILGORE_TROUT_RAID_ID = 21,
    KILGORE_TROUT_MAX_RAIG_GROU_ID = 22
};
/*!*******************************************************************
 * @var kilgore_trout_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t kilgore_trout_raid_group_config[] = 
{
        /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {KILGORE_TROUT_RAID_GROUP_WIDTH,FBE_LBA_INVALID,FBE_RAID_GROUP_TYPE_RAID5,FBE_CLASS_ID_PARITY,520,KILGORE_TROUT_RAID_ID,0},

        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


fbe_test_raid_group_disk_set_t kilgore_troutDiskSet[7] = { 
	{0, 0 , 4},
	{0, 0 , 5},
	{0, 0 , 6},
    {0, 0 , 7},
    {0, 0 , 8},
    {0, 0 , 9},
    {0, 0 , 10}
};

fbe_test_raid_group_disk_set_t kilgore_trout_removed_drive;
fbe_api_terminator_device_handle_t kilgore_trout_removed_drive_info;

/*!*******************************************************************
 * @enum kilgore_trout_logical_drives_e
 *********************************************************************
 * @brief Enumeration of the physical objects used by Kilgore Trout test
 * 
 *********************************************************************/
enum kilgore_trout_logical_drives_e {
    KILGORE_TROUT_LDO_A_PORT_NUMBER = 0,
    KILGORE_TROUT_LDO_A_ENCLOSURE_NUMBER = 0,
    KILGORE_TROUT_LDO_A_SLOT_NUMBER = 4,
    KILGORE_TROUT_LDO_B_PORT_NUMBER = 0,
    KILGORE_TROUT_LDO_B_ENCLOSURE_NUMBER = 0,
    KILGORE_TROUT_LDO_B_SLOT_NUMBER = 5,
    KILGORE_TROUT_LDO_C_PORT_NUMBER = 0,
    KILGORE_TROUT_LDO_C_ENCLOSURE_NUMBER = 0,
    KILGORE_TROUT_LDO_C_SLOT_NUMBER = 6,
    KILGORE_TROUT_LDO_D_PORT_NUMBER = 0,
    KILGORE_TROUT_LDO_D_ENCLOSURE_NUMBER = 0,
    KILGORE_TROUT_LDO_D_SLOT_NUMBER = 7
};

/*!*******************************************************************
 * @enum kilgore_trout_logical_drives_e
 *********************************************************************
 * @brief Enumeration of the logical objects used by Kilgore Trout test
 * 
 *********************************************************************/
enum kilgore_trout_sep_object_numbers_e {
    KILGORE_TROUT_PVD_A_ID = 0, 
    KILGORE_TROUT_PVD_B_ID = 1, 
    KILGORE_TROUT_PVD_C_ID = 2, 
    KILGORE_TROUT_PVD_D_ID = 3, 
    KILGORE_TROUT_VD_A_ID = 4,
    KILGORE_TROUT_VD_B_ID = 5,
    KILGORE_TROUT_VD_C_ID = 6,
    KILGORE_TROUT_VD_D_ID = 7,
    KILGORE_TROUT_RAID1_ID = 8,
    KILGORE_TROUT_LUN1_ID = 9, /* Just a single LUN.*/
    KILGORE_TROUT_MAX_ID = 10
};

static fbe_object_id_t kilgore_trout_object_ids[KILGORE_TROUT_MAX_ID];

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/* Perform the removal of a drive and verify that the 
 * drive-related objects change state accordingly
 */
static void kilgore_trout_remove_drive_and_verify(fbe_u32_t                           in_port_num,
                                                  fbe_u32_t                           in_encl_num,
                                                  fbe_u32_t                           in_slot_num,
                                                  fbe_u32_t                           in_position,
                                                  fbe_u32_t                           in_num_objects,
                                                  fbe_api_terminator_device_handle_t* out_drive_info_p);

/* Perform the re-insertion of a drive and verify that the 
 * drive-related objects change state accordingly
 */
static void kilgore_trout_reinsert_drive_and_verify(fbe_u32_t in_port_num,
        fbe_u32_t                           in_encl_num,
        fbe_u32_t                           in_slot_num,
        fbe_u32_t                           in_position,
        fbe_api_terminator_device_handle_t* in_drive_info_p);

/* completion function for the interface with the job service, 
 * use by the job service to ack back calls from Kilgore Trout tests
 */
static fbe_status_t 
kilgore_trout_request_completion_function(fbe_packet_t *, fbe_packet_completion_context_t);

/* sends packets to Job service 
*/
static void kilgore_trout_send_control_packet_and_wait(fbe_packet_t *in_packet_p,
        fbe_object_id_t                        in_object_id,
        fbe_payload_control_operation_opcode_t in_op_code,
        fbe_payload_control_buffer_t           in_data_p,
        fbe_payload_control_buffer_length_t    in_data_size);

static fbe_status_t fbe_kilgore_trout_get_virtual_drive_info(fbe_object_id_t object_id, 
        fbe_spare_drive_info_t *spare_drive_info_p);

/* completion function for spare info call to topology service 
*/
static fbe_status_t fbe_kilgore_trout_get_virtual_drive_info_completion(fbe_packet_t *in_packet_p, 
        fbe_packet_completion_context_t in_context);

/* test completion function for interaction with job service
*/
static fbe_status_t kilgore_trout_test_completion_function(
        fbe_packet_t                    *in_packet_p, 
        fbe_packet_completion_context_t in_context);

/* Kilgore Trout and Job service test calls */
fbe_status_t fbe_kilgore_trout_build_control_element(
        fbe_job_queue_element_t         *job_queue_element,
        fbe_job_service_control_t       control_code,
        fbe_object_id_t                 object_id,
        fbe_u8_t                        *command_data_p,
        fbe_u32_t                       command_size);

fbe_status_t kilgore_trout_disable_recovery_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t kilgore_trout_enable_recovery_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t kilgore_trout_add_element_to_queue(fbe_object_id_t in_object_id);
fbe_status_t kilgore_trout_find_object_in_job_service_recovery_queue(fbe_object_id_t in_object_id);
fbe_status_t kilgore_trout_remove_element_from_recovery_queue(fbe_object_id_t in_object_id);

fbe_status_t kilgore_trout_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p);

fbe_status_t kilgore_trout_get_number_of_element_in_recovery_queue(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       *num_of_objects);

fbe_status_t kilgore_trout_wait_for_step_to_complete(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms);

fbe_status_t kilgore_trout_wait_until_job_gets_in_recovery_q(fbe_object_id_t in_object_id, 
                                                             fbe_u32_t       timeout_ms);
void kilgore_trout_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);


/* Kilgore Trout step functions */

/* Bring up the initial topology.*/
static void kilgore_trout_step1(void);

/* Make VD-A need a spare, but there is no spare available. */
static void kilgore_trout_step2(fbe_test_rg_configuration_t *rg_config_p);

/* Add a new drive that is initially not available for sparing */
static void kilgore_trout_step3(void);

/* Configure the new drive as a hot spare. */
static void kilgore_trout_step4(void);

/* Wait for rebuild of VD-A to complete */
static void kilgore_trout_step5(void);

/* Make VD-B need a spare a(and again there is no spare available) */
static void kilgore_trout_step6(void);

/* Cancel the pending spare request. */
static void kilgore_trout_step7(void);

/* Job service tests */
static void kilgore_trout_step7(void);
fbe_status_t kilgore_trout_reinsert_removed_drive(void);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/
void kilgore_trout_setup(void)
{
   if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &kilgore_trout_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);

        
    }

   // fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;


}

void kilgore_trout_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== destroy configuration ===\n");
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
}


/*!****************************************************************************
 * kilgore_trout_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25/07/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void kilgore_trout_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &kilgore_trout_raid_group_config[0];
    
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,kilgore_trout_run_test,
                                   KILGORE_TROUT_TEST_LUNS_PER_RAID_GROUP,
                                   KILGORE_TROUT_TEST_CHUNKS_PER_LUN);
    return;    

}



/*!**************************************************************
 * kilgore_trout_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the kilgore Trout
 * test: drive sparing infrastructure test, plus Job service tests
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 10/27/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_run_test(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t num_raid_groups = 0;
    fbe_u32_t index = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    mut_printf(MUT_LOG_LOW, "=== Kilgore Trout Test starts ===\n");

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {

            //kilgore_trout_step1(); /* Bring up the initial topology. Create a Raid 5 type object */
            kilgore_trout_step2(rg_config_p); /* Make VD-A need a spare, but there is no spare available. */

            kilgore_trout_step3(); /* Verify that the job service still has a pending job request */
            kilgore_trout_step4(); /* Configure the new drive as a hot spare. */
            kilgore_trout_step5(); /* Wait for mirror rebuild to complete */

            /* reinsert the removed drives before quit*/
            kilgore_trout_reinsert_removed_drive();

            /* the last part of this test causes a new HS request to be generated, but it does not
             * get executed, job service code must now provide a way to search by job number for HS
             * requests */
            //  kilgore_trout_step6(); /* Make VD-B need a spare (and again there is no spare available) */
            //  kilgore_trout_step7(); /* Cancel the pending spare request. */
         }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Kilgore Trout Test completed successfully ===",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * kilgore_trout_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the kilgore_trout test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 *  Create and verify the initial physical package config
 *
 *  a) - Create and verify the initial physical package config.
 *  b) - Create a RAID-5 object with I/O edge to the virtual drives.
 *  c) - Verify that RAID-5 object is in the READY state.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void kilgore_trout_step1(void)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         provision_index = 0;
    fbe_object_id_t 					              raid_group_id;
    fbe_object_id_t 					              raid_group_id_from_job;
	fbe_job_service_error_type_t 					  job_error_code;
    fbe_status_t 									  job_status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 1 START ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start(step 1) - call Job Service API to create initial topology\n",
            __FUNCTION__);

    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));
    
    fbe_raid_group_create_request.drive_count = KILGORE_TROUT_RAID_GROUP_WIDTH;
    fbe_raid_group_create_request.raid_group_id = KILGORE_TROUT_RAID_ID;
	
	for (provision_index = 0;  provision_index < fbe_raid_group_create_request.drive_count;  
            provision_index++)
    {
        fbe_raid_group_create_request.disk_array[provision_index].bus = 
            kilgore_troutDiskSet[provision_index].bus;
        fbe_raid_group_create_request.disk_array[provision_index].enclosure = 
            kilgore_troutDiskSet[provision_index].enclosure;
        fbe_raid_group_create_request.disk_array[provision_index].slot = 
            kilgore_troutDiskSet[provision_index].slot;
    }

    /* The test assumes the Raid Group creation takes the whole size exported
     * by the provision drive. Part 2 will address raid group specific sizes
     */ 
    fbe_raid_group_create_request.b_bandwidth           = FBE_TRUE; 
    fbe_raid_group_create_request.capacity              = FBE_LBA_INVALID; 
    fbe_raid_group_create_request.raid_type             = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = KILGORE_TROUT_TIMEOUT*10;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fbe_api_job_service_raid_group_create failed");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
										 KILGORE_TROUT_TIMEOUT,
										 &job_error_code,
										 &job_status,
										 &raid_group_id_from_job);

	MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
	MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(raid_group_id_from_job, raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id,
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end(step 1) - call Job Service API to create initial topology, END\n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 1 END === \n");
}

/*!**************************************************************
 * step2()
 ****************************************************************
 * @brief
 *   Make VD-A need a spare, while queues are blocked
 *
 *   a) Configure PVD-D as a hot spare via a config service control command
 *   b) Disable the sparing queue in the job service
 *   c) Remove the physical drive (PDO-A)
 *   d) Verify that VD-A is now in the FAIL state
 *   e) Verify that the job service has a pending request for a spare
 *   drive for VD-A
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step2(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status; 
    fbe_api_terminator_device_handle_t out_drive_info; 
    fbe_object_id_t                    rg_object_id; // PVD D object id
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 2 START === \n");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start(step 2) - Make VD-A need a spare, while queues are blocked \n",
            __FUNCTION__);

	/* Speed up VD hot spare */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(10); /* 10 second hotspare timeout */

    /* Get the number of physical objects created for this test.  
     * This number is used when checking if a drive has been removed
     * or has been reinserted. */
    status = fbe_api_get_total_objects(&kilgore_trout_test_number_physical_objects_g,
                                       FBE_PACKAGE_ID_PHYSICAL);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Configuring PVD-D as hot-spare");

    /* a)Configure PVD-D as a hot spare via a config service control command */
    /* Get the object id of the unused PVD */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus, 
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot, 
                                                            &kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_configure_drive_as_hot_spare(rg_config_p->extra_disk_set[0].bus, 
                                                   rg_config_p->extra_disk_set[0].enclosure,
                                                   rg_config_p->extra_disk_set[0].slot);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: object %d now a hot spare\n",
            __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID]);

    shaggy_verify_sep_object_state(kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID], FBE_LIFECYCLE_STATE_READY);
 

    /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id,0, &kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    

    /* b) Disable the sparing queue in the job service */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Request Job service to block queues \n", __FUNCTION__);

    status = kilgore_trout_disable_recovery_queue_processing(kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't disable Job Service queue processing");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Job service queues now blocked \n", __FUNCTION__);

    /* c) Remove the physical drive (PDO-A) 
     * d) Verify that VD-A is now in the FAIL state */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: removing PDO-A\n", __FUNCTION__);
    kilgore_trout_remove_drive_and_verify(rg_config_p->rg_disk_set[0].bus, 
                                          rg_config_p->rg_disk_set[0].enclosure,
                                          rg_config_p->rg_disk_set[0].slot,
                                          0,
                                          kilgore_trout_test_number_physical_objects_g - 1,
                                          &out_drive_info);

    if (fbe_test_util_is_simulation())
    {
        /* verify object has been destroyed */
        shaggy_verify_pdo_state (rg_config_p->rg_disk_set[0].bus, 
                                 rg_config_p->rg_disk_set[0].enclosure,
                                 rg_config_p->rg_disk_set[0].slot,
                                 FBE_LIFECYCLE_STATE_DESTROY,
                                 FBE_TRUE); /* FBE_TRUE is passed to verify object is destroyed. */

    }

    /* the spare service will not immediately generate a Hot Spare request, so must give it sometime */
    kilgore_trout_wait_until_job_gets_in_recovery_q(FBE_OBJECT_ID_INVALID, 30000);

    /* Verify that the job service has a pending request for a spare
     * drive for VD-A */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verifying request for hot spare object id %d is in job service queue\n",
            __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    /* hot spare requests have invalid id, search for single object request in queue */
    status = kilgore_trout_find_object_in_job_service_recovery_queue(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "object not in job service queues");            
    mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d is in queue\n", 
            __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 2) - Make VD-A need a spare, while queues are blocked \n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 2 END ===\n");

    kilgore_trout_removed_drive = rg_config_p->rg_disk_set[0];
    kilgore_trout_removed_drive_info = out_drive_info;

    return;
}

/*!**************************************************************
 * step3()
 ****************************************************************
 * @brief
 * Verify that the job service still has a pending job request
 *
 * a) Verify that the job service still has a pending request
 *    for a spare drive for VD-A.\n"
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step3(void)
{
    fbe_status_t status; 
    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 3 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start(step 3) - Verify that the job service still has a pending job request\n",
            __FUNCTION__);

    /* a) Add a new physical drive (PDO-D and LDO-D) where policy does
     *    not allow it to be a spare.
     *
     * b) Verify that a new provisioned drive (PVD-D) is created and
     * is in the READY state. */
    //status = fbe_api_provision_drive_get_obj_id_by_location(KILGORE_TROUT_LDO_D_PORT_NUMBER, 
    //                                                        KILGORE_TROUT_LDO_D_ENCLOSURE_NUMBER,
    //                                                        KILGORE_TROUT_LDO_D_SLOT_NUMBER,
    //                                                        &kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* c) Verify that the job service still has a pending request
     *    for a spare drive for VD-A.\n */
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Verifying again that request remains in queue for object id %d\n",
               __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    
    /* hot spare requests have invalid id, search for single object request in queue */
    status = kilgore_trout_find_object_in_job_service_recovery_queue(FBE_OBJECT_ID_INVALID);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "object id %d, not in job service queues");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d still in queue\n", __FUNCTION__,
               kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 3) - Verify that the job service still has a pending job request\n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 3 END ===\n");
    return;
}

/*!**************************************************************
 * step4()
 ****************************************************************
 * @brief
 *  Verify Hot spare processing after queues are re-enabled.
 *
 * a) Enable the sparing queue in the job service 
 * b) Verify that the config service processed the pending sparing
 *    request
 * c) Verify that VD-A now has an edge to PVD-C and is in the
 *    READY state.
 *     
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step4(void)
{  
    fbe_api_get_block_edge_info_t edge_info; 
    fbe_status_t                  status; 
    fbe_edge_index_t              edge_index; 
    fbe_object_id_t               raid_group_object_id;               // raid group's object id 
    fbe_object_id_t               vd_object_id;                       // vd's object id
    status = FBE_STATUS_OK;
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 4 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start(step 4) - Verify Hot spare processing after queues are re-enabled\n",
            __FUNCTION__);

    /* a) Enable the sparing queue in the job service */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Request to job service to enable queues\n",
            __FUNCTION__);

    status =  kilgore_trout_enable_recovery_queue_processing(kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unable to change access to Job service queues");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Job service queues now enabled\n",
            __FUNCTION__);

    /* status should be no object since object should not be in queue anymore */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verifying that hot spare request for object id %d is no longer in queue \n",
               __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    /* b) Verify that the config service processed the pending sparing request */
    /* hot spare requests have invalid id, search for single object request in queue */
    status = kilgore_trout_find_object_in_job_service_recovery_queue(FBE_OBJECT_ID_INVALID);
    if ((status == FBE_STATUS_NO_OBJECT) || (status == FBE_STATUS_OK))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d no longer in queue \n",
                   __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    }

    /* c) Verify that VD-A now has an edge to PVD-D and is in the READY state. */

    /* Verify that hot spare gets swapped-in and edge between VD object and spare PVD
     * object becomes ENABLED. */
    
    /*  Get the RG's object id.  We need to this get the VD's object it. */
    fbe_api_database_lookup_raid_group_by_number(KILGORE_TROUT_RAID_ID, &raid_group_object_id);

    /*  Get the VD's object id */
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, 0, &vd_object_id);
    
    /* a) Enable the sparing queue in the job service */
    mut_printf(MUT_LOG_TEST_STATUS, "%s:  Verify that VD-A now has an edge to PVD-D and is in the READY state\n",
            __FUNCTION__);

    edge_index = 0;
    status = fbe_api_wait_for_block_edge_path_state (vd_object_id, 
                                                     edge_index, /* Proactive spare edge index. */
                                                     FBE_PATH_STATE_ENABLED,
                                                     FBE_PACKAGE_ID_SEP_0,
                                                     10000);

    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    /* Get the spare edge information. */
    status = fbe_api_get_block_edge_info(vd_object_id, 
                                         edge_index, 
                                         &edge_info, 
                                         FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:  Verified that VD-A now has an edge to PVD-D and is in the READY state\n",
            __FUNCTION__);


    /* Verify whether PVD, VD, RAID and LUN objects are in READY
     * state or not. */
   shaggy_verify_sep_object_state(edge_info.server_id, FBE_LIFECYCLE_STATE_READY);
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);
    shaggy_verify_sep_object_state(raid_group_object_id, FBE_LIFECYCLE_STATE_READY);

    /* must wait until NR processing kicks in */
    kilgore_trout_wait_for_step_to_complete(kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID], 4000);

     mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end(step 4) - Verify Hot spare processing after queues are re-enabled\n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 4 END ===\n");
    return;
}

/*!**************************************************************
 *          step5()
 ****************************************************************
 *
 * @brief   Wait for rebuild of VD-A to complete.
 *
 *  a) Verify that VD-A rebuilt completely.
 *  b) Verify that position 0 is no longer rb logging.
 *     
 * @param - none 
 *
 * @return - none
 *
 * @author
 *  03/16/2010  Ron Proulx  - Created
 * 
 ****************************************************************/

void kilgore_trout_step5(void)
{  
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t raid_group_object_id;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t       pos_to_be_rebuilt = 0;

    /* Verify that position 0 (VD-A) needs to be rebuilt
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 5 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start(step 5) - Wait for rebuild of VD-A to complete \n",
               __FUNCTION__);

    //  Get the RG's object id.  We need to do this to get the VD's object it.
    fbe_api_database_lookup_raid_group_by_number(KILGORE_TROUT_RAID_ID, &raid_group_object_id);

    /* Get the raid group object id.
     */
    status = fbe_api_raid_group_get_info(raid_group_object_id, 
                                         &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:  Verifying VD-A rebuilt completely \n",
               __FUNCTION__);

    if (rg_info.rebuild_checkpoint[pos_to_be_rebuilt] == FBE_LBA_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s rebuild complete rg: 0x%x pos: %d", 
                __FUNCTION__, raid_group_object_id, pos_to_be_rebuilt);
    }
    else if (rg_info.b_rb_logging[pos_to_be_rebuilt] == FBE_TRUE)
    {
        /* validate that the rebuild checkpoint is below the raid group capacity
         * and that position 0 isn't marked rb logging (a.k.a. disabled).
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                "%s: rebuild_checkpoint[0]: 0x%llx or rb_logging[0]: 0x%x not expected\n",
                __FUNCTION__,
		(unsigned long long)rg_info.rebuild_checkpoint[pos_to_be_rebuilt],
                rg_info.b_rb_logging[pos_to_be_rebuilt]); 
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s:  Verifying VD-A rebuilt completely and rb logging for position 0 is not marked \n",
            __FUNCTION__);

    /* Now wait for position 0 to rebuild. diego will assert if it takes
     * too long to rebuild.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Waiting for position: %d to rebuild \n",
            __FUNCTION__, pos_to_be_rebuilt); 

    status = fbe_test_sep_util_wait_rg_pos_rebuild(raid_group_object_id,  
                                          pos_to_be_rebuilt, 
                                          60 /* Time to wait */);

    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    /* If we made it this fare rebuilds are complete. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: position: %d fully rebuilt\n",
            __FUNCTION__, pos_to_be_rebuilt); 
    mut_printf(MUT_LOG_TEST_STATUS, "%s:  Verified VD-A rebuilt completely and rb logging for position 0 is not marked \n",
            __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 5) - Wait for rebuild of VD-A to complete \n",
            __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 5 END ===\n");
    return;
}

/*!**************************************************************
 * step6()
 ****************************************************************
 * @brief
 *  Make VD-A need a spare again (and again there is no spare
 *  available)
 *
 * a) Disable the sparing queue in the job service\n"
 * b) Remove the physical drive (PDO-B)
 * c) Verify that VD-A is now in the FAIL state 
 * d) Verify that the job service has a pending request for 
 *    a spare drive for VD-A(sent from the RAID-1 object)
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step6(void)
{
    fbe_status_t status = FBE_STATUS_OK; 
    fbe_api_terminator_device_handle_t out_drive_info; /* drive info needed for reinsertion */   

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 6 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start(step 6) - Make VD-A need a spare again (and again there is no spare \n",
            __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: sending packet to Job service to block processing of elements\n",
            __FUNCTION__);

    status = kilgore_trout_disable_recovery_queue_processing(KILGORE_TROUT_VD_A_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't disable Job Service queue processing");

    /* a) Remove the physical drive (PDO-B)
     * b) Verify that VD-B is now in the FAIL state */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: removing PDO-B\n", __FUNCTION__);
    kilgore_trout_remove_drive_and_verify(KILGORE_TROUT_LDO_B_PORT_NUMBER, 
                                          KILGORE_TROUT_LDO_B_ENCLOSURE_NUMBER,
                                          KILGORE_TROUT_LDO_B_SLOT_NUMBER,
                                          1, /* position in RG */
                                          kilgore_trout_test_number_physical_objects_g - 1,
                                          &out_drive_info);

    shaggy_verify_pdo_state (KILGORE_TROUT_LDO_B_PORT_NUMBER,
                             KILGORE_TROUT_LDO_B_ENCLOSURE_NUMBER,
                             KILGORE_TROUT_LDO_B_SLOT_NUMBER,
                             FBE_LIFECYCLE_STATE_DESTROY,
                             FBE_TRUE); /* FBE_TRUE is passed to verify object is destroyed. */

    /* must wait hot spare request is generated */
    kilgore_trout_wait_for_step_to_complete(kilgore_trout_object_ids[KILGORE_TROUT_PVD_D_ID], 5000);

    /* c) Verify that the job service has a pending request for
     * a spare drive for VD-A(sent from the RAID-5 object) */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify job service has request for object id %d in queue\n",
               __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    /* hot spare requests have invalid id, search for single object request in queue */
    status = kilgore_trout_find_object_in_job_service_recovery_queue(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "object id not in job service queues");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d request for sparing in queue \n",
               __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 6) - No spare available for VD-A \n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 6 END ===\n");
    return;
}

/*!**************************************************************
 * step7()
 ****************************************************************
 * @brief
 *  Cancel the pending spare request
 *
 * a) Delete the RAID-5 object from the SEP config
 * b) Verify that the job service does not have a pending 
 *    request for a spare drive for VD-A 
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step7(void)
{
    fbe_status_t status = FBE_STATUS_OK; 
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 7 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start(step 7) - Cancel the pending spare request \n",
            __FUNCTION__);

    /* a) Delete the RAID-5 object from the SEP config
     * b) Verify that the job service does not have a pending
     *    request for a spare drive for VD-A */
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: sending job service request to remove element for object id %d from queue\n",
               __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    status = kilgore_trout_remove_element_from_recovery_queue(FBE_OBJECT_ID_INVALID);
    if (status == FBE_STATUS_NO_OBJECT)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d, not in queue \n",
                   __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: object id %d, removed from queue \n",
                   __FUNCTION__, kilgore_trout_object_ids[KILGORE_TROUT_VD_A_ID]);

    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 7) - Pending spare request removed from queue \n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 7 END ===\n");
    return;
}

/*!**************************************************************
 * step8()
 ****************************************************************
 * @brief
 * testing of the job service 
 *
 * This function tests the Job service functionality to:
 *
 *  - add elements to queue
 *  - remove elements from queue
 *  - block queue processing
 *  - unblock queue processing
 *  - determine if a request for a specific object is in the queue
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 11/03/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

void kilgore_trout_step8(void)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t num_of_objects = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== step 8 START ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start(step 8) - Testing of the Job Service \n",
            __FUNCTION__);

    /* sending  a request to the Job service to disable queue processing */
    status = kilgore_trout_disable_recovery_queue_processing(KILGORE_TROUT_VD_A_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't disable Job Service queue processing");

    /* sending a request to the Job service to add 3 elements to the recovery queue */
    status = kilgore_trout_add_element_to_queue(KILGORE_TROUT_VD_A_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't add an element to the Job Service queue");

    status = kilgore_trout_add_element_to_queue(KILGORE_TROUT_VD_B_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't add an element to the Job Service queue");

    status = kilgore_trout_add_element_to_queue(KILGORE_TROUT_VD_C_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't add an element to the Job Service queue");

    /* sending a request to the Job service to remove an element from the recovery queue
    */
    status = kilgore_trout_remove_element_from_recovery_queue(FBE_OBJECT_ID_INVALID);
    if (status == FBE_STATUS_NO_OBJECT)
    {
        mut_printf(MUT_LOG_LOW, "=== object id %d, not in queue ===\n",
                KILGORE_TROUT_VD_B_ID);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "=== object id %d, removed from queue ===\n",
                KILGORE_TROUT_VD_B_ID);
    }
    
    /* sending a request to the Job service to enable queue processing */
    status = kilgore_trout_enable_recovery_queue_processing(KILGORE_TROUT_VD_A_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't enable Job Service queue processing");

    /* sending a request to the Job service to add an element to the recovery queue */
    status = kilgore_trout_add_element_to_queue(KILGORE_TROUT_VD_B_ID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't add an element to the Job Service queue");
#if 0
    status = kilgore_trout_find_object_in_job_service_recovery_queue(FBE_OBJECT_ID_INVALID);
    if (status == FBE_STATUS_NO_OBJECT)
    {
        mut_printf(MUT_LOG_LOW, "=== object id %d, not in queue ===\n",
                KILGORE_TROUT_VD_C_ID);
    }
#endif
    status = kilgore_trout_get_number_of_element_in_recovery_queue(KILGORE_TROUT_VD_C_ID, &num_of_objects);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "can't get number of elements in queue request");

    /* we must wait for the queue to be drained before terminating test */
    status = kilgore_trout_wait_for_step_to_complete(KILGORE_TROUT_VD_A_ID, 10000);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: end(step 8) - End of testing of the Job Service \n",
            __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== step 8 END ===\n");
    return;
}

/*!**************************************************************
 * kilgore_trout_find_object_in_job_service_recovery_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to see if a
 *  given object exists in the Job service recovery queue
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of search request
 *
 * @author
 * 11/05/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_find_object_in_job_service_recovery_queue(fbe_u32_t in_object_number)
{
    fbe_job_queue_element_t                 job_queue_element;
    fbe_status_t                            status = FBE_STATUS_OK;

    /* Setup the job service control element with appropriate values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE,
                                                      in_object_number,
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "can't init command for Job service");

    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/***************************************************************
 * end kilgore_trout_find_object_in_job_service_recovery_queue()
 **************************************************************/

/*!**************************************************************
 * kilgore_trout_disable_recovery_queue_processing
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to disable
 *  recovery queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 11/05/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_disable_recovery_queue_processing(fbe_u32_t in_object_number)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: call to disable queue \n", __FUNCTION__);
    job_queue_element.object_id = kilgore_trout_object_ids[in_object_number];

    /* Setup the job service control element with appropriate values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                      kilgore_trout_object_ids[in_object_number],
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "can't init command for Job service");

    job_queue_element.queue_access = FALSE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/*******************************************************
 * end kilgore_trout_disable_recovery_queue_processing()
 ******************************************************/

/*!**************************************************************
 * kilgore_trout_enable_recovery_queue_processing
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to enable
 *  recovery queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 11/05/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_enable_recovery_queue_processing(fbe_u32_t in_object_number)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = kilgore_trout_object_ids[in_object_number];

    /* Setup the job service control element with appropriate values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                      kilgore_trout_object_ids[in_object_number],
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "can't init command for Job service");

    job_queue_element.queue_access = TRUE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/******************************************************
 * end kilgore_trout_enable_recovery_queue_processing()
 *****************************************************/

/*!**************************************************************
 * kilgore_trout_add_element_to_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to add a
 *  test element to the job service recovery queue
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of request
 *
 * @author
 * 11/05/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_add_element_to_queue(fbe_u32_t in_object_number)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = kilgore_trout_object_ids[in_object_number];

    /* Setup the job service control element with test values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_ADD_ELEMENT_TO_QUEUE_TEST,
                                                      kilgore_trout_object_ids[in_object_number],
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_job_service_add_element_to_queue_test(&job_queue_element,
            FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/******************************************
 * end kilgore_trout_add_element_to_queue()
 *****************************************/

/*!**************************************************************
 * kilgore_trout_get_number_of_element_in_recovery_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service for the
 *  number of test objects in the recovery queue
 * 
 * @param object_id       - the ID of the target object
 * @param num_of_objects  - number of elements in queue
 *
 * @return fbe_status - status of request
 *
 * @author
 * 11/12/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_get_number_of_element_in_recovery_queue(fbe_u32_t in_object_number, 
                                                                   fbe_u32_t       *num_of_objects)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = kilgore_trout_object_ids[in_object_number];

    /* Setup the job service control element with appropriate values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE,
                                                      kilgore_trout_object_ids[in_object_number],
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "can't init command for Job service");

    status = fbe_api_job_service_process_command(&job_queue_element,
            FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE,
            FBE_PACKET_FLAG_NO_ATTRIB);

    *num_of_objects = job_queue_element.num_objects;

    return status;
}
/*************************************************************
 * end kilgore_trout_get_number_of_element_in_recovery_queue()
 ************************************************************/

/*!**************************************************************
 * kilgore_trout_remove_element_from_recovery_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to remove a
 *  test element from the job service recovery queue
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of request
 *
 * @author
 * 11/05/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_remove_element_from_recovery_queue(fbe_u32_t in_object_number)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    /* Setup the job service control element with appropriate values */
    status = fbe_kilgore_trout_build_control_element (&job_queue_element,
                                                      FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE,
                                                      in_object_number,
                                                      (fbe_u8_t *)NULL,
                                                      0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "can't init command for Job service");

    status = fbe_api_job_service_process_command(&job_queue_element,
            FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE,
            FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/********************************************************
 * end kilgore_trout_remove_element_from_recovery_queue()
 *******************************************************/


/*!**************************************************************
 * fbe_kilgore_trout_get_virtual_drive_info_completion()
 ****************************************************************
 * @brief
 * It is a completion function get the spare drive information
 * from virtal drive object.
 *
 * @param packet_p - The packet requesting this operation.
 * @param fbe_packet_completion_context_t - Packet context.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/02/2009 - Created. Jesus Medina
 *
 ****************************************************************/
static fbe_status_t fbe_kilgore_trout_get_virtual_drive_info_completion (
        fbe_packet_t                    *packet_p,
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_kilgore_trout_get_virtual_drive_info_completion()
 *****************************************************************/

/*!****************************************************************************
 *  kilgore_trout_remove_drive_and_verify
 ******************************************************************************
 *
 * @brief
 * This function removes a drive.  First it waits for the logical and physical
 * drive objects to be destroyed.  Then checks the object states of the
 * PVD and VD for that drive are Activate.
 *
 * @param in_port_num           - port/bus number of the drive to remove
 * @param in_encl_num           - enclosure number of the drive to remove
 * @param in_slot_num           - slot number of the drive to remove
 * @param in_pvd_object_id      - object id of the PVD for this drive
 * @param in_vd_object_id       - object id of the VD for this drive
 * @param out_drive_info_p      - pointer to structure that gets filled in with the 
 *                                "drive info" for the drive
 * @return None 
 *
 *****************************************************************************/

static void kilgore_trout_remove_drive_and_verify(
        fbe_u32_t                          in_port_num,
        fbe_u32_t                          in_encl_num,
        fbe_u32_t                          in_slot_num,
        fbe_u32_t                          in_position,
        fbe_u32_t                          in_num_objects,
        fbe_api_terminator_device_handle_t*  out_drive_info_p)
{
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             raid_group_object_id;               // raid group's object id 
    fbe_object_id_t             pvd_object_id;                      // pvd's object id
    fbe_object_id_t             vd_object_id;                       // vd's object id

    //  Get the PVD object id before we remove the drive 
    status = fbe_api_provision_drive_get_obj_id_by_location(in_port_num,
                                                            in_encl_num, 
                                                            in_slot_num, 
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the drive
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(in_port_num, 
                                         in_encl_num, 
                                         in_slot_num, 
                                         out_drive_info_p); 
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(in_port_num, 
                                         in_encl_num, 
                                         in_slot_num); 
    
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Removing drive - waiting for PDO and LDO to be destroyed\n", __FUNCTION__);

    //  Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the 
    //  value that is passed in. 
    if (fbe_test_util_is_simulation())
    {
        status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    //  Get the RG's object id.  We need to this get the VD's object it.
    fbe_api_database_lookup_raid_group_by_number(KILGORE_TROUT_RAID_ID, &raid_group_object_id);

    //  Get the VD's object id 
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);

    //  Verify that the PVD and VD objects are in the FAIL state
    shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    //shaggy_verify_sep_object_state(raid_group_object_id, FBE_LIFECYCLE_STATE_READY);
    //shaggy_verify_sep_object_state(kilgore_trout_object_ids[KILGORE_TROUT_LUN1_ID], FBE_LIFECYCLE_STATE_READY);
    return;
} 
/*********************************************
 * end kilgore_trout_remove_drive_and_verify()
 **********************************************/


/*!****************************************************************************
 *  kilgore_trout_reinsert_drive_and_verify
 ******************************************************************************
 *
 * @brief
 * This function reinserts a drive.  First it waits for the logical and physical
 * drive objects to be destroyed.  Then checks the object states of the
 * PVD and VD for that drive are Activate.
 *
 * @param in_port_num           - port/bus number of the drive to remove
 * @param in_encl_num           - enclosure number of the drive to remove
 * @param in_slot_num           - slot number of the drive to remove
 * @param in_pvd_object_id      - object id of the PVD for this drive
 * @param in_vd_object_id       - object id of the VD for this drive
 * @param out_drive_info_p      - pointer to structure that gets filled in with the 
 *                                "drive info" for the drive
 * @return None 
 *
 *****************************************************************************/

static void kilgore_trout_reinsert_drive_and_verify(
                                           fbe_u32_t in_port_num,
                                           fbe_u32_t in_encl_num,
                                           fbe_u32_t in_slot_num,
                                           fbe_u32_t in_position,
                                           fbe_api_terminator_device_handle_t* in_drive_info_p)
{
    fbe_status_t                        status;                             // fbe status
    fbe_object_id_t                     raid_group_object_id;               // raid group's object id 
    fbe_object_id_t                     pvd_object_id;                      // pvd's object id
    fbe_object_id_t                     vd_object_id;                       // vd's object id

    /* Insert the drive by changing the insert bit for it */

    if (fbe_test_util_is_simulation())
    {
          status =  fbe_test_pp_util_reinsert_drive(in_port_num, 
                                    in_encl_num, 
                                    in_slot_num,
                                    *in_drive_info_p);
    }
    else
    {
          status =  fbe_test_pp_util_reinsert_drive_hw(in_port_num, 
                                    in_encl_num, 
                                    in_slot_num);
    }

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Reinserting drive - waiting for PDO and LDO to be created\n", __FUNCTION__);

    /* Verify the PDO and LDO are created.  We will wait for the number
     * of physical objects to be equal to the expected number for the test, which 
     * signifies that the objects exist. */
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Reinserting drive - waiting for objects %d\n", 
            __FUNCTION__, kilgore_trout_test_number_physical_objects_g);

    status = fbe_api_wait_for_number_of_objects(kilgore_trout_test_number_physical_objects_g, 
                                                10000, 
                                                FBE_PACKAGE_ID_PHYSICAL);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Verify the PDO and LDO are in the READY state */
    shaggy_verify_pdo_state(in_port_num, in_encl_num, in_slot_num, FBE_LIFECYCLE_STATE_READY, FBE_FALSE);

    //  Get the PVD object id 
    status = fbe_api_provision_drive_get_obj_id_by_location(in_port_num, 
                                                            in_encl_num,
                                                            in_slot_num, 
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the RG's object id.  We need this get the VD's object id.
    fbe_api_database_lookup_raid_group_by_number(KILGORE_TROUT_RAID_ID, &raid_group_object_id);

    //  Get the VD's object id 
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id, in_position, &vd_object_id);

    //  Verify that the PVD and VD objects are in the READY state 
    shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);

    /* Verify that the PVD and VD objects are in the ready state */
    shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);
    return;
} 
/*********************************************
 * end kilgore_trout_reinsert_drive_and_verify()
 **********************************************/

/*!**************************************************************
 * kilgore_trout_request_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Kilgore Trout request
 * 
 * @param packet_p - packet
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

static fbe_status_t kilgore_trout_request_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end kilgore_trout_request_completion_function()
 ********************************************************/

/*!**************************************************************
 * kilgore_trout_test_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Kilgore Trout request sent to the Job Service
 * 
 * @param packet_p - packet
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

static fbe_status_t kilgore_trout_test_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end kilgore_trout_test_completion_function()
 ********************************************************/

/*!**************************************************************
 * kilgore_trout_test_set_cancel_function()
 ****************************************************************
 * @brief
 *  This function sets a cancel function for the 
 *  Kilgore Trout request sent to the Job Service
 * 
 * @param packet_p - packet
 * @param context -  Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

fbe_status_t kilgore_trout_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p)
{
    /* Set the packet cancellation and context. */
    return FBE_STATUS_OK;

} 
/*******************************************************
 * end kilgore_trout_test_set_cancel_function()
 ********************************************************/

/*!**************************************************************
 * fbe_kilgore_trout_build_control_element()
 ****************************************************************
 * @brief
 *  This function builds a test control element 
 * 
 * @param job_queue_element - element to build 
 * @param job_control_code    - code to send to job service
 * @param object_id           - the ID of the target object
 * @param command_data_p      - pointer to client data
 * @param command_size        - size of client's data
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

fbe_status_t fbe_kilgore_trout_build_control_element(
        fbe_job_queue_element_t        *job_queue_element,
        fbe_job_service_control_t      control_code,
        fbe_object_id_t                object_id,
        fbe_u8_t                       *command_data_p,
        fbe_u32_t                      command_size)
{
    fbe_job_type_t job_type = FBE_JOB_TYPE_INVALID;

    /* Context size cannot be greater than job element context size. */
    if (command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    job_queue_element->object_id = object_id;

    /* only set the job type element for requests that are queued */
    switch(control_code)
    {
        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
            job_type = FBE_JOB_TYPE_DRIVE_SWAP;
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
            job_type = FBE_JOB_TYPE_RAID_GROUP_CREATE;
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
            job_type = FBE_JOB_TYPE_RAID_GROUP_DESTROY;
            break;
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
            job_type = FBE_JOB_TYPE_LUN_CREATE;
            break;
        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
            job_type = FBE_JOB_TYPE_LUN_DESTROY;
            break;
        case FBE_JOB_CONTROL_CODE_ADD_ELEMENT_TO_QUEUE_TEST:
            job_type = FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST;
            break;
        case FBE_JOB_CONTROL_CODE_INVALID:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
    }

    job_queue_element->job_type = job_type;
    job_queue_element->timestamp = 0;
    job_queue_element->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->current_state  = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->queue_access = TRUE;
    job_queue_element->num_objects = 0;

    if (command_data_p != NULL)
    {
        fbe_copy_memory(job_queue_element->command_data, command_data_p, command_size);
    }
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_kilgore_trout_build_control_element()
 ***********************************************/

/*********************************************************************
 * kilgore_trout_wait_for_step_to_complete()
 *********************************************************************
 * @brief
 *  This function waits for all elements in the Job service queue to be
 *  processed before exiting the test
 * 
 * @param packet_p - packet
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 *    
 *********************************************************************/
fbe_status_t kilgore_trout_wait_for_step_to_complete(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms)
{
    fbe_u32_t    current_time = 0;
    fbe_status_t status;

    status = FBE_STATUS_OK;
    while (current_time < timeout_ms)
    {
        /* wait a little longer */
        current_time = current_time + 100;
        fbe_api_sleep (100);
        /* send request to job service to finish all elements before exiting test */
    }

    return status;
}
/***********************************************
 * end kilgore_trout_wait_for_step_to_complete()
 ***********************************************/

/*********************************************************************
 * kilgore_trout_wait_until_job_gets_in_recovery_q()
 *********************************************************************
 * @brief
 *  This function waits until a job is added to the recovery q for the given object
 * 
 * @param in_object_id - object id
 * @param timeout_ms - how long to wait
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 05/31/2011 - Created
 *    
 *********************************************************************/
fbe_status_t kilgore_trout_wait_until_job_gets_in_recovery_q(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms)
{
    fbe_u32_t    current_time = 0;
    fbe_status_t status;

    status = FBE_STATUS_OK;
    while (current_time < timeout_ms)
    {
        status = kilgore_trout_find_object_in_job_service_recovery_queue(in_object_id);
        if (status == FBE_STATUS_OK)
        {
            return status;
        }
        mut_printf(MUT_LOG_LOW, 
                   "%s Wait for job for obj: 0x%x - status: 0x%x\n",
                   __FUNCTION__, in_object_id, status);
        /* wait a little longer */
        current_time = current_time + 2000;
        fbe_api_sleep (2000);
        /* send request to job service to finish all elements before exiting test */
    }

    return status;
}
/***********************************************
 * end kilgore_trout_wait_for_step_to_complete()
 ***********************************************/

/*********************************************************************
 * kilgore_trout_reinsert_removed_drive()
 *********************************************************************
 * @brief
 *  This function will reinsert the removed drives
 * 
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 08/11/2011 - Vishnu Sharma
 *    
 *********************************************************************/


fbe_status_t kilgore_trout_reinsert_removed_drive(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting unused drive (%d_%d_%d). ==", 
                __FUNCTION__, kilgore_trout_removed_drive.bus,
                kilgore_trout_removed_drive.enclosure,
                kilgore_trout_removed_drive.slot);

    /* insert unused drive back */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(kilgore_trout_removed_drive.bus, 
                                        kilgore_trout_removed_drive.enclosure, 
                                        kilgore_trout_removed_drive.slot,
                                        kilgore_trout_removed_drive_info);
        
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(kilgore_trout_removed_drive.bus, 
                                        kilgore_trout_removed_drive.enclosure, 
                                        kilgore_trout_removed_drive.slot);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the unused pvd object to be in ready state */
    status = fbe_test_sep_util_wait_for_pvd_state(kilgore_trout_removed_drive.bus, 
                                kilgore_trout_removed_drive.enclosure, 
                                kilgore_trout_removed_drive.slot,
                                FBE_LIFECYCLE_STATE_READY,
                                KILGORE_TROUT_TIMEOUT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;

}

/*********************************************
 * end fbe_kilgore_trout_test file
 **********************************************/






