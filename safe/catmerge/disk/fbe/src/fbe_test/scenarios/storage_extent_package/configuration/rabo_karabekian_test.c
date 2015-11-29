/*!*************************************************************************
 * @file rabo_karabekian_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for Creating Raid Groups via the
 *   Job service API
 *
 * @version
 *   01/05/2010 - Created. Jesus medina
 *
 ***************************************************************************/

#include "sep_tests.h"
#include "fbe/fbe_types.h"                          
#include "fbe_test_configurations.h"               
#include "fbe_test_package_config.h"              
#include "fbe/fbe_api_common.h"                  
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_utils.h"                  
#include "fbe/fbe_api_discovery_interface.h"
#include "sep_utils.h"                        
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_job_service.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_job_service_interface.h" 
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_database_interface.h"

#include "fbe/fbe_api_common_transport.h"
#include "pp_utils.h"                        


char * rabo_karabekian_short_desc = "API Raid Group creation with Job Service test";
char * rabo_karabekian_long_desc =
"\n"
"\n"
"The Rabo Karabekian Scenario tests the infrastructure needed for Raid Group\n"
"creation. \n"
"\n"
"This scenario tests the creation of Raid Group for the following cases:\n"
"\n"
"  - A RG creation consuming the entire virtual drive exported capacity\n"
"  - A RG creation consuming part of the virtual drives where 2 or more RGs share the VD object\n"
"  - A RG creation consuming a mix of unique PVDs and OVDs that have been consumed by VD objects\n"
"  - A RG creation consuming different VD sizes\n"
"  - A RG destroy of edges for VDs that leave holes at beginning of VD object so that other\n"
"      RG object can be created, a subsequent RG creation should consume the space\n"
"  - A RG destroy of edges for VDs that leave holes at middle of VD object so that other\n"
"      RG object can be created, a subsequent RG creation should consume the space\n"
"  - A RG destroy of edges for VDs that leave holes at end of VD object so that other\n"
"      RG object can be created, a subsequent RG creation should consume the space\n"
"\n"
"It covers the following cases:\n"
"\t- It verifies that the Create Raid Group API can successfully create Raid Groups\n"
"\t  via Job service\n"
"\t- It verifies that configuration service can offer transaction operations to update\n"
"\t  objects in memory only and on disk.\n"
"\t- It verifies that Job service creation can create Raid Groups on shared VDs\n"
"\n"
"Dependencies: \n"
"\t- The Job Service ability to queue create type requests and process these \n"
"\t  type of requests\n"
"\t- Configuration service ability to provide APIs to open and close\n"
"\t  update-in-memory transactions.\n"
"\t- Configuration service ability to persist database with the\n"
"\t  Persistence service. \n"
"\t  Persistence service API to read-in persisted objects\n"
"\n"
"Starting Config: \n"
"\t[PP] an armada board\n"
"\t[PP] a SAS PMC port\n"
"\t[PP] a viper enclosure\n"
"\t[PP] four SAS drives (PDO-A, PDO-B, PDO-C and PDO-D)\n"
"\t[PP] four logical drives (LDO-A, LDO-B, LDO-C and LDO-D)\n"
"\t[SEP] four provision drives (PVD-A, PVD-B, PVD-C and PVD-D)\n"
"\t[SEP] four virtual drives (VD-A, VD-B, VD-C and VD-D)\n"
"\t[SEP] a RAID-5 object, connected to the four VDs\n"
"\n"
"STEP 1: Bring up the initial topology.\n"
"\t- Create and verify the initial physical package config.\n"
"\t- Create five provision drives with I/O edges to the logical drives.\n"
"\t- Verify that provision drives are in the READY state.\n"
"STEP 2: call SEP API to create a single Raid Group object.\n"
"\t- Create a raid group request\n"
"\t- send the raid group request to the job service via the new RG create API.\n"
"STEP 3: Verify SEP Raid group create API performs parameter verification.\n"
"\t- for the raid group request, verify that: \n"
"\t- proper raid group type is provided\n"
"\t- proper raid size is provided\n"
"STEP 4: Verify that Job service performs parameter verification before it queues\n"
"        request.\n"
"\t- for each raid group request, verify that: \n"
"\t- proper raid group type is provided\n"
"\t- proper raid size is provided\n"
"Step 5: Verify that Job service can receive and is able to process create\n"
"\trequests.\n"
"\t-  Job service queues create requests as they arrive\n"
"\t-  Job service dequeues and process create requests\n"
"\t-  Job service properly validates packet's Raid Group Creation parameters\n"
"\t-  Job service opens a transaction to persist objects in memory with the\n"
"\t   configuration service\n"
"\t-  Job service creates required objects and edges in memory\n"
"\t-  Job service closes the transaction to persist objects in memory with the\n"
"\t   configuration service\n"
"\t-  Job service commits objects to disk via configuration service API\n"
"Step 6: Verify that the Raid components have been persisted in memory and are\n"
"\tactivated\n"
"\t- verify that SEP Raid Group Create API results in: \n"
"\t-  VD objects with I/O edges to the provision drives created on step 1.\n"
"\t-  VD objects are in the READY state.\n"
"\t-  RAID object with I/O edge to the virtual drives.\n"
"\t-  Raid object in the READY state.\n"
;

/*!*******************************************************************
 * @def RABO_KARABEKIAN_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define  RABO_KARABEKIAN_RAID_GROUP_WIDTH      4

#define RABO_KARABEKIAN_PVD_SIZE TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

#define RABO_KARABEKIAN_NUMBER_RAID_GROUPS   5

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a function
 *         register
 */
#define RABO_KARABEKIAN_TIMEOUT  60000 /* wait maximum of 1 minute */

#define RABO_KARABEKIAN_MIN_DISK_COUNT 6


/*****************************************
 * GLOBAL DATA
 *****************************************/

/* Number of physical objects in the test configuration. 
*/
fbe_u32_t   rabo_karabekian_test_number_physical_objects_g;


/*!*******************************************************************
 * @enum rabo_karabekian_sep_object_ids_e
 *********************************************************************
 * @brief Enumeration of the logical objects used by Rabo Karabekian test
 * 
 *********************************************************************/
enum rabo_karabekian_sep_object_ids_e {
    RABO_KARABEKIAN_RAID_ID = 21,
    RABO_KARABEKIAN_MAX_ID = 22
};

fbe_test_raid_group_disk_set_t rabo_karabekianDiskSet[RABO_KARABEKIAN_MIN_DISK_COUNT];

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
fbe_status_t fbe_rabo_karabekian_build_control_element(fbe_job_queue_element_t *job_queue_element,
        fbe_job_service_control_t      control_code,
        fbe_object_id_t                object_id,
        fbe_u8_t                       *command_data_p,
        fbe_u32_t                      command_size);

fbe_status_t rabo_karabekian_disable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t rabo_karabekian_enable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t rabo_karabekian_add_element_to_creation_queue(fbe_object_id_t in_object_id);
fbe_status_t rabo_karabekian_find_object_in_job_service_creation_queue(fbe_object_id_t in_object_id);
fbe_status_t rabo_karabekian_remove_element_from_creation_queue(fbe_object_id_t in_object_id);

fbe_status_t rabo_karabekian_test_set_cancel_function(fbe_packet_t* in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p);

fbe_status_t rabo_karabekian_get_number_of_element_in_creation_queue(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       *num_of_objects);

fbe_status_t rabo_karabekian_wait_for_step_to_complete(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms);

/* RG create function */
static void rabo_karabekian_remove_raid_group(fbe_raid_group_number_t rg_id); 

/* RG delete function */
static void rabo_karabekian_create_raid_group(fbe_object_id_t raid_group_id,
        fbe_lba_t capacity,
        fbe_u32_t drive_count, 
        fbe_raid_group_type_t raid_group_type);

/* Rabo Karabekian step functions */

/* Call Job Service API to create Raid Group objects. */
static void rabo_karabekian_step1(void);

/* Verify that the Raid components have been persisted */
static void rabo_karabekian_step2(void);

/* Call Job Service API to create Raid Group objects. */
static void rabo_karabekian_step3(void);

fbe_status_t rabo_karabekian_get_available_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                                   fbe_test_raid_group_disk_set_t *disk_set,fbe_u32_t no_of_drives);


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

static fbe_u32_t rabo_karabekian_num_system_stripers;
static fbe_u32_t rabo_karabekian_num_system_parities;
static fbe_u32_t rabo_karabekian_num_system_mirrors;

static void rabo_karabekian_step0(void)
{
    fbe_status_t     status;

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_parities);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_stripers);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_mirrors);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * rabo_karabekian_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the Rabo Karabekian
 * test: API Raid Group creation with Job Service test
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 01/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/
static fbe_u32_t num_system_raids;

void rabo_karabekian_test(void)
{
    fbe_object_id_t *rg_ids;
    fbe_status_t     status;
    fbe_test_discovered_drive_locations_t drive_locations;
    
    mut_printf(MUT_LOG_LOW, "=== Rabo Karabekian Test starts ===\n");
    fbe_test_sep_util_set_trace_level (FBE_TRACE_LEVEL_INFO, FBE_PACKAGE_ID_SEP_0);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    status = fbe_api_enumerate_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &rg_ids, &num_system_raids);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_free_memory(rg_ids);

    /*get the all available drive information*/
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /*Fill the global disk set array with the available drives information*/
    status = rabo_karabekian_get_available_drive_location(&drive_locations,rabo_karabekianDiskSet,RABO_KARABEKIAN_MIN_DISK_COUNT);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    rabo_karabekian_step0();

    /* Call Job Service API to create Raid Group objects. */
    //rabo_karabekian_step1();
    
    /* Verify that the Raid component has been persisted */
    //rabo_karabekian_step2();
    
    /* call to exercise RG creation for various cases */
    rabo_karabekian_step3();

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Rabo Karabekian Test completed successfully ===",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * step1()
 ****************************************************************
 * @brief
 *  This function builds and sends a Raid group creation request
 *  to the Job serivce
 *
 *  a) Create a raid group request
 *  b) send the raid group request to the job service via the
 *     new RG create API
 *
 * @param void
 *
 * @return void
 *
 * @author
 * 01/05/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void rabo_karabekian_step1(void)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         provision_index = 0;
    fbe_object_id_t                                   raid_group_id;   
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - Step 2,  call Job Service API to create Raid Group objects\n",
            __FUNCTION__);
    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_request.drive_count = RABO_KARABEKIAN_RAID_GROUP_WIDTH;
    fbe_raid_group_create_request.raid_group_id = RABO_KARABEKIAN_RAID_ID;
    
    for (provision_index = 0;  provision_index < fbe_raid_group_create_request.drive_count;  
            provision_index++)
    {
        fbe_raid_group_create_request.disk_array[provision_index].bus = 
            rabo_karabekianDiskSet[provision_index].bus;
        fbe_raid_group_create_request.disk_array[provision_index].enclosure = 
            rabo_karabekianDiskSet[provision_index].enclosure;
        fbe_raid_group_create_request.disk_array[provision_index].slot = 
            rabo_karabekianDiskSet[provision_index].slot;
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
    fbe_raid_group_create_request.ready_timeout_msec    = RABO_KARABEKIAN_TIMEOUT;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         RABO_KARABEKIAN_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id, FBE_LIFECYCLE_STATE_READY, 20000,
            FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end - Step 2,  call Job Service API to create Raid Group object, END\n",
            __FUNCTION__);
}

/*!**************************************************************
 * step2()
 ****************************************************************
 * @brief
 *
 * Verify that the Raid components have been persisted
 *
 * a) Verify that edges between PVDS and VDs have been persisted.
 * b) Verify that VDs have been persisted.\n"
 * c) Verify that edges between VDs and the Raid Object have been
 * persisted.
 * d) Verify that Raid Group object has been persisted.
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 01/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void rabo_karabekian_step2(void)
{
    fbe_status_t     status;
    fbe_object_id_t *rg_ids;
    fbe_u32_t        num_rgs;
    fbe_u32_t        rg_count_after_delete;

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - Step 2,  Verify Raid Group object has been created\n",
            __FUNCTION__);

    /* send the enumerate request to the parity object since this is Raid 5 object */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &rg_ids, &num_rgs);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL(1 + num_system_raids, num_rgs);
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: step2: raid group object created %d\n",
            __FUNCTION__,
            rg_ids[0]);
    fbe_api_free_memory(rg_ids);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 5... ===\n");

    /* Call Job Service API to destroy Raid Group object. */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &rg_ids, &rg_count_after_delete);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");
    MUT_ASSERT_INT_EQUAL(num_system_raids, rg_count_after_delete);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 deleted successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: end - Step 2,  Verify Raid Group object has been created, END\n",
            __FUNCTION__);

    fbe_api_free_memory(rg_ids);

    return;
}

/*!**************************************************************
 * step3()
 ****************************************************************
 * @brief
 *  This function creates raid groups for the following cases
 *
 *  - A RG consuming the entire VD object exported capacity
 *  - A RG consuming part of VD object exported capacity
 *  - A RG consuming part of VD object exported capacity
 *    and VD entire exported capacity
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 04/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void rabo_karabekian_step3(void)
{
    fbe_u32_t        num_stripers = 0;
    fbe_u32_t        rg_count_after_create = 0;
    fbe_status_t     status;

    mut_printf(MUT_LOG_LOW, "=== Rabo Karabekian multi Test starts ===\n");
    fbe_test_sep_util_set_trace_level (FBE_TRACE_LEVEL_INFO, FBE_PACKAGE_ID_SEP_0);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

#if 0
    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_parities);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_stripers);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &rabo_karabekian_num_system_mirrors);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Call Job Service API to create a single Raid Group type 5 object
     * RG size = fbe lba invalid */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 5... ===\n");
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+2, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 5... ===\n");
     /* Call Job Service API to destroy Raid Group object. */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");
    MUT_ASSERT_INT_EQUAL(num_parities, rabo_karabekian_num_system_parities);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 deleted successfully! ===\n");
#endif

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 5... ===\n");
    /* Call Job Service API to create Raid Group type 5 object with specific size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    RABO_KARABEKIAN_PVD_SIZE,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Second Raid Group Type 5 with invalid size ===\n");
    /* Call Job Service API to create Raid Group type 5 object with specific size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 created successfully with invalid size! ===\n");
#endif

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating second Raid Group Type 5... ===\n");
    /* Call Job Service API to create Raid Group type 5 object with specific size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    RABO_KARABEKIAN_PVD_SIZE / 3,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+2, 
            "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 created successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating third Raid Group Type 5... ===\n");
     /* Call Job Service API to create Raid Group type 5 object with specific size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+2,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+3, 
            "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== third Raid Group Type 5 created successfully! ===\n");

    /* Call Job Service API to destroy Raid Group object Rabo id+1, this test will then leave a
     * space between VDs so another RG creation test should us that space
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting second Raid Group Type 5... ===\n");
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+1); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted second Raid Group Type 5 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating second Raid Group Type 5 again with fixed size... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with another different size
     * this should use space left for RG deletion of Rabo id + 1 above call */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    RABO_KARABEKIAN_PVD_SIZE / 3,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 created successfully again with fixed size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating third Raid Group Type 5 again with invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with another different size
     * this should use space left for RG deletion of Rabo id + 2 above call */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+2,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+3, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== third Raid Group Type 5 created successfully again with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting second Raid Group... ===\n");

    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+1); 
    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

#endif
    
#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted second Raid Group Type 5 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating second Raid Group Type 5 again with fixed size... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with another different size
     * this should use space left for RG deletion of Rabo id + 1 above call */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    RABO_KARABEKIAN_PVD_SIZE / 3,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+3, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== second Raid Group Type 5 created successfully again with fixed size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting second Raid Group Type 5... ===\n");
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+1); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted second Raid Group Type 5 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating middle Raid Group Type 5  with invalid size... ===\n");
    /* Call Job Service API to create Raid Group type 5 object with specific size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+3, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== middle Raid Group Type 5 created successfully again with invalid size! ===\n");
#endif 

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating one last Raid Group Type 5 again with invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with invalid size */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+2,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+3, 
            "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Last Raid Group Type 5 created successfully with invalid size! ===\n");
#endif
    /* Call Job Service API to destroy Raid Group object Rabo id+1 and +2, this test will then leave a
     * space at the end so another RG creation should us that space
     */
#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting all Raid Groups Type 5... ===\n");

    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 
    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);
    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+1); 
    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);
    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+2); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted all Raid Groups Type 5 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 5 again with fixed size... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with size = lba invalid
     * which means amy leftover space on the VD should be now consumed
     */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    RABO_KARABEKIAN_PVD_SIZE,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully again with fixed size! ===\n");
 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a second Raid Group Type 5 with fixed size and more drives... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with size = lba invalid
     * which means amy leftover space on the VD should be now consumed
     */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+1,
                                    RABO_KARABEKIAN_PVD_SIZE/3,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+2, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully with fixed size and more drives===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a third Raid Group Type 5 with invalid size and more drives... ===\n");

    /* Call Job Service API to create Raid Group type 5 object as step 2, with size = lba invalid
     * which means amy leftover space on the VD should be now consumed
     */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID+2,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+3, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+3, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== third Raid Group Type 5 created successfully with invalid size and more drives===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting all Raid Groups Type 5... ===\n");

    /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+2, "Found unexpected number of RGs!");

       /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+1); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");
   /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID+2); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted all Raid Group Type 5 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "================ START ==================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "===    Raid Group Create Type 6 test  ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 6 invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 6 object. */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+2, 
                                    FBE_RAID_GROUP_TYPE_RAID6);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Created Raid Group Type 6 successfully with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 6... ===\n");
     /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted Raid Group Type 6 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=====  Raid Group Create Type 6 test =====\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=============== END ======================\n");

    mut_printf(MUT_LOG_TEST_STATUS, "================ START ==================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "===    Raid Group Create Type 0 test  ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 0 invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 0 object. */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+1, 
                                    FBE_RAID_GROUP_TYPE_RAID0);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0, &num_stripers);

    MUT_ASSERT_INT_EQUAL_MSG(num_stripers, rabo_karabekian_num_system_stripers+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Created Raid Group Type 0 successfully with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 0... ===\n");

    /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0, &num_stripers);

    MUT_ASSERT_INT_EQUAL_MSG(num_stripers, rabo_karabekian_num_system_stripers, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleted Raid Group Type 0 successfully ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=====  Raid Group Create Type 0 test =====\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=============== END ======================\n");
#endif
    
    mut_printf(MUT_LOG_TEST_STATUS, "================ START ==================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "===    Raid Group Create Type 10 test  ===\n");

    /* Raid 10 is a special type. should be exercised by other test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 10 invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 10 object. */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID10);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0, &num_stripers);

    MUT_ASSERT_INT_EQUAL_MSG(num_stripers, rabo_karabekian_num_system_stripers+1, 
            "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Created Raid Group Type 10 successfully with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 10... ===\n");
    /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &num_stripers);

    MUT_ASSERT_INT_EQUAL_MSG(num_stripers, rabo_karabekian_num_system_stripers, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=====  Raid Group Create Type 10 test =====\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=============== END ======================\n");

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);


#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "================ START ==================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "===    Raid Group Create Type 1 test  ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 1 invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 1 object. */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH/2, 
                                    FBE_RAID_GROUP_TYPE_RAID1);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0, &num_mirrors);

    MUT_ASSERT_INT_EQUAL_MSG(num_mirrors, rabo_karabekian_num_system_mirrors+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Created Raid Group Type 1 successfully with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 1... ===\n");
 
    /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0, &num_mirrors);

    MUT_ASSERT_INT_EQUAL_MSG(num_mirrors, rabo_karabekian_num_system_mirrors, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=====  Raid Group Create Type 1 test =====\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=============== END ======================\n");
#endif

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "================ START ==================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "===    Raid Group Create Type 3 test  ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating a Raid Group Type 3 invalid size... ===\n");

    /* Call Job Service API to create Raid Group type 3 object. */
    rabo_karabekian_create_raid_group(RABO_KARABEKIAN_RAID_ID,
                                    FBE_LBA_INVALID,
                                    RABO_KARABEKIAN_RAID_GROUP_WIDTH+1, 
                                    FBE_RAID_GROUP_TYPE_RAID3);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities+1, "Found unexpected number of RGs!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Created Raid Group Type 3 successfully with invalid size! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 3... ===\n");
    /* remove all RG objects   */
    rabo_karabekian_remove_raid_group(RABO_KARABEKIAN_RAID_ID); 

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0, &num_parities);

    mut_printf(MUT_LOG_TEST_STATUS, "=====  Raid Group Create Type 3 test =====\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=============== END ======================\n");

    MUT_ASSERT_INT_EQUAL_MSG(num_parities, rabo_karabekian_num_system_parities, "Found unexpected number of RGs!");
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Rabo Karabekin multi Test completed successfully ===",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * rabo_karabekian_create_raid_group()
 ****************************************************************
 * @brief
 *  This function builds and sends a Raid group creation request
 *  to the Job service
 *
 * @param raid_group_id - id of raid group
 * @param raid_group_capacity - capacity of raid group
 * @param drive_count - number of drives in raid group
 * @param raid_group_type - type of raid group
 *
 * @return void
 *
 * @author
 * 04/04/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void rabo_karabekian_create_raid_group(fbe_object_id_t raid_group_id,
                                       fbe_lba_t raid_group_capacity,
                                       fbe_u32_t drive_count, 
                                       fbe_raid_group_type_t raid_group_type)
{
    fbe_status_t status;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         index = 0;
    fbe_object_id_t                                   raid_group_id1 = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;
  
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, raid_group_type);

    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));
    fbe_raid_group_create_request.drive_count    = drive_count;
    fbe_raid_group_create_request.raid_group_id  = raid_group_id;
    
    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus       = rabo_karabekianDiskSet[index].bus;
        fbe_raid_group_create_request.disk_array[index].enclosure = rabo_karabekianDiskSet[index].enclosure;
        fbe_raid_group_create_request.disk_array[index].slot      = rabo_karabekianDiskSet[index].slot;
    }

    /* b_bandwidth = true indicates 
     * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024 for element_size 
     */
    fbe_raid_group_create_request.b_bandwidth           = FBE_TRUE; 
    fbe_raid_group_create_request.capacity              = raid_group_capacity; 
    fbe_raid_group_create_request.raid_type             = raid_group_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = RABO_KARABEKIAN_TIMEOUT;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fbe_api_job_service_raid_group_create failed");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

   /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         RABO_KARABEKIAN_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, &raid_group_id1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id1);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id1, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__,
            raid_group_type);
}

/*!**************************************************************
 * rabo_karabekian_remove_raid_group()
 ****************************************************************
 * @brief
 * This function removes a raid group
 *
 * @param - rg_id id of raid group to remove 
 *
 * @return - none
 *
 * @author
 * 04/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void rabo_karabekian_remove_raid_group(fbe_raid_group_number_t rg_id)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_destroy_t fbe_raid_group_destroy_request;
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - call Job Service API to destroy Raid Group object, START\n",
            __FUNCTION__);

    fbe_raid_group_destroy_request.raid_group_id = rg_id;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = RABO_KARABEKIAN_TIMEOUT;
    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to destroy raid group");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group destroy call sent successfully! ===\n");

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number,
                                         RABO_KARABEKIAN_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");

    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: call Job Service API to destroy Raid Group object, END\n",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * rabo_karabekian_find_object_in_job_service_creation_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to see if a
 *  given object exists in the Job service creation queue
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of search request
 *
 * @author
 * 01/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t rabo_karabekian_find_object_in_job_service_creation_queue(
        fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t                 job_queue_element;
    fbe_status_t                            status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE,
            in_object_id,
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
 * end rabo_karabekian_find_object_in_job_service_creation_queue()
 **************************************************************/

/*!**************************************************************
 * rabo_karabekian_disable_creation_queue_processing
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

fbe_status_t rabo_karabekian_disable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: call to disable queue \n",
            __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
            in_object_id,
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
 * end rabo_karabekian_disable_creation_queue_processing()
 ******************************************************/

/*!**************************************************************
 * rabo_karabekian_enable_creation_queue_processing
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

fbe_status_t rabo_karabekian_enable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
            in_object_id,
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
 * end rabo_karabekian_enable_creation_queue_processing()
 *****************************************************/

/*!**************************************************************
 * rabo_karabekian_add_element_to_queue
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

fbe_status_t rabo_karabekian_add_element_to_queue(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with test values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_ADD_ELEMENT_TO_QUEUE_TEST,
            in_object_id,
            (fbe_u8_t *)NULL,
            0);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_job_service_add_element_to_queue_test(&job_queue_element,
            FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/******************************************
 * end rabo_karabekian_add_element_to_queue()
 *****************************************/

/*!**************************************************************
 * rabo_karabekian_get_number_of_element_in_creation_queue
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

fbe_status_t rabo_karabekian_get_number_of_element_in_creation_queue(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       *num_of_objects)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE,
            in_object_id,
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
 * end rabo_karabekian_get_number_of_element_in_creation_queue()
 ************************************************************/

/*!**************************************************************
 * rabo_karabekian_remove_element_from_creation_queue
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

fbe_status_t rabo_karabekian_remove_element_from_creation_queue(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_rabo_karabekian_build_control_element (&job_queue_element,
            FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE,
            in_object_id,
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
 * end rabo_karabekian_remove_element_from_creation_queue()
 *******************************************************/

/*!**************************************************************
 * rabo_karabekian_request_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Rabo Karabekian request
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

static fbe_status_t rabo_karabekian_request_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end rabo_karabekian_request_completion_function()
 ********************************************************/

/*!**************************************************************
 * rabo_karabekian_test_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Rabo Karabekian request sent to the Job Service
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

static fbe_status_t rabo_karabekian_test_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end rabo_karabekian_test_completion_function()
 ********************************************************/

/*!**************************************************************
 * rabo_karabekian_test_set_cancel_function()
 ****************************************************************
 * @brief
 *  This function sets a cancel function for the 
 *  Rabo Karabekian request sent to the Job Service
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

fbe_status_t rabo_karabekian_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p)
{
    /* Set the packet cancellation and context. */
    return FBE_STATUS_OK;

} 
/*******************************************************
 * end rabo_karabekian_test_set_cancel_function()
 ********************************************************/

/*!**************************************************************
 * fbe_rabo_karabekian_build_control_element()
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

fbe_status_t fbe_rabo_karabekian_build_control_element(
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
 * end fbe_rabo_karabekian_build_control_element()
 ***********************************************/

/*********************************************************************
 * rabo_karabekian_wait_for_step_to_complete()
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
fbe_status_t rabo_karabekian_wait_for_step_to_complete(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms)
{
    fbe_u32_t    current_time = 0;
    fbe_status_t status;

    status = FBE_STATUS_OK;
    while (current_time < timeout_ms)
    {
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_LOW, "call to get number of objects in queue failed, status %d\n",
                    status);
            return status;
        }

        /* send request to job service to finish all elements before exiting test */
        current_time = current_time + 100;
        fbe_api_sleep (100);
    }

    return status;
}
/***********************************************
 * end rabo_karabekian_wait_for_step_to_complete()
 ***********************************************/

fbe_status_t rabo_karabekian_get_available_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                                   fbe_test_raid_group_disk_set_t *disk_set,fbe_u32_t num_of_drives_required)
{
    fbe_u32_t       drive_index = 0;
    fbe_u32_t       index = 0;
    fbe_u32_t       block_index =0; 
    fbe_u32_t       drive_cnt = 0;
    fbe_u32_t       drive_count = 0;

    for(block_index = 1; block_index < FBE_TEST_BLOCK_SIZE_LAST; block_index++)
    {
        for(drive_index = 1; drive_index < FBE_DRIVE_TYPE_LAST; drive_index++)
        {
            drive_count = drive_locations_p->drive_counts[block_index][drive_index];
            for(drive_cnt = 0;drive_cnt <drive_count;drive_cnt++)
            {
                disk_set[index].bus = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].bus;
                disk_set[index].enclosure = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].enclosure;
                disk_set[index].slot = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].slot;
                drive_locations_p->drive_counts[block_index][drive_index]--;

                
                index++;
                /*We need only 15 drives to run this test*/    
                if(index >= num_of_drives_required)
                    return FBE_STATUS_OK;
            }
            
        }
    }
    return FBE_STATUS_NO_OBJECT;
}


/*********************************************
 * end fbe_rabo_karabekian_test file
 **********************************************/



