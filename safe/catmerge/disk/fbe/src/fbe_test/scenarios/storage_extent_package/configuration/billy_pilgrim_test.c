/*!*************************************************************************
 * @file billy_pilgrim_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for destroying Raid Groups via the
 *   Job service API
 *
 * @version
 *   02/12/2010 - Created. Jesus medina
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

char * billy_pilgrim_short_desc = "API Raid Group destroy with Job Service test";
char * billy_pilgrim_long_desc =
"\n"
"\n"
"The Billy Pilgrim Scenario tests the infrastructure needed for Raid Group\n"
"destruction. \n"
"\n"
"This scenario covers the following cases:\n"
"\t- Destruction of Raid Groups that are connected to unique and shared virtual drives\n"
"\t  Verifies that the destroy Raid Group API can successfully destroy Raid Groups\n"
"\t  virtual drives, and all edges via Job service\n"
"\t- Destruction of Raid Groups that are connected to shared virtual drives\n"
"\t  Verifies that the destroy Raid Group API can successfully destroy Raid Groups\n"
"\t  virtual drives, and edges when the virtual drive is solely connected to the\n"
"\t  raid group\n"
"\t  Verifies that the destroy Raid Group API does not virtual drives when the \n"
"\t  virtual drive is connected to more than one raid group\n"
"\t- It verifies that database service can offer transaction operations to update\n"
"\t  objects in memory only.\n"
"\n"
"Dependencies: \n"
"\t- The Job Service ability to queue destroy type requests and process these \n"
"\t  type of requests\n"
"\t- Database service ability to provide APIs to open and close\n"
"\t  update-in-memory transactions.\n"
"\t- Database service ability to persist database with the\n"
"\t  Persistence service. \n"
"\t  Persistence service API to read-in persisted objects\n"
"\t  Block transport code support to obtain list of upstream objects\n"
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
"\t- send the raid group create request to the job service via the new\n"
"\tcreate RG API.\n"
"STEP 3: call SEP API to destroy Raid Group object.\n"
"\tVerify that Job service performs parameter verification before \n"
"\tit queues requests.\n"
"\t- for each raid group request, verify that: \n"
"\t- proper raid group id is provided\n"
"Step 4: Verify that Job service can receive and is able to process destroy\n"
"\trequests.\n"
"\t-  Job service queues destroy requests as they arrive\n"
"\t-  Job service dequeues and process destroy requests\n"
"\t-  Job service properly validates packet's Raid Group Destroy parameters\n"
"\t-  Job service opens a transaction to remove objects from memory with the\n"
"\t   Database service\n"
"\t-  Job service destroys objects and edges in memory\n"
"\t-  Job service closes the transaction to remove objects in memory with the\n"
"\t   database service\n"
"Step 5: Verify that the Raid components do not exist anymore in memory and on disk\n"
"\t- verify that SEP Raid Group Destroy API results in: \n"
"\t-  for the case where raid group is connected to unique virtual drives:\n"
"\t    - Raid objects with I/O edges to virtual drives no longer exist.\n"
"\t    - VD objects with I/O edges to provision drives no longer exist.\n"
"\tDescription last updated: 10/14/2011\n"
;

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a function
 *         register
 */
#define BILLY_PILGRIM_TIMEOUT  60000 /*wait maximum of 1 minute*/

#define BILLY_PILGRIM_NUMBER_RAID_GROUPS   5

#define BILLY_PILGRIM_PVD_SIZE (TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY * 2)

#define  BILLY_PILGRIM_RAID_GROUP_WIDTH      4

/*!*******************************************************************
 * @enum billy_pilgrim_logical_drives_e
 *********************************************************************
 * @brief Enumeration of the logical objects used by Billy Pilgrim test
 * 
 *********************************************************************/
enum billy_pilgrim_sep_object_ids_e {
    BILLY_PILGRIM_RAID_ID = 21,
    BILLY_PILGRIM_MAX_ID = 22
};

fbe_test_raid_group_disk_set_t billy_pilgrimDiskSet[6] = { 
	{0, 0 , 2},
	{0, 0 , 3},
	{0, 0 , 0},
	{0, 0 , 0},
	{0, 0 , 0},
	{0, 0 , 0}
};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
fbe_status_t fbe_billy_pilgrim_build_control_element(
        fbe_job_queue_element_t        *job_queue_element,
        fbe_job_service_control_t      control_code,
        fbe_object_id_t                object_id,
        fbe_u8_t                       *command_data_p,
        fbe_u32_t                      command_size);

fbe_status_t billy_pilgrim_disable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t billy_pilgrim_enable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t billy_pilgrim_add_element_to_creation_queue(fbe_object_id_t in_object_id);
fbe_status_t billy_pilgrim_find_object_in_job_service_creation_queue(fbe_object_id_t in_object_id);
fbe_status_t billy_pilgrim_remove_element_from_creation_queue(fbe_object_id_t in_object_id);

static void billy_pilgrim_set_trace_level(fbe_trace_type_t trace_type,
        fbe_u32_t id,
        fbe_trace_level_t level);

fbe_status_t billy_pilgrim_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p);

fbe_status_t billy_pilgrim_get_number_of_element_in_creation_queue(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       *num_of_objects);

fbe_status_t billy_pilgrim_wait_for_step_to_complete(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       timeout_ms);

static void billy_pilgrim_remove_raid_group(fbe_raid_group_number_t rg_id);

static void billy_pilgrim_create_raid_group(fbe_object_id_t raid_group_id,
        fbe_lba_t raid_group_capacity,
        fbe_u32_t drive_count, 
        fbe_raid_group_type_t  raid_group_type);

extern fbe_status_t rabo_karabekian_get_available_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                                   fbe_test_raid_group_disk_set_t *disk_set,fbe_u32_t num_of_drives_required);


/*!**************************************************************
 * billy_pilgrim_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the Billy Pilgrim
 * test: API Raid Group creation with Job Service test
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 02/15/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void billy_pilgrim_test(void)
{
    fbe_status_t     status;
    fbe_test_discovered_drive_locations_t drive_locations;
    
    fbe_u32_t mirror_count_before_create = 0;
    fbe_u32_t mirror_count_after_create = 0;
    fbe_u32_t mirror_count_after_delete = 0;

    fbe_u32_t striper_count_before_create = 0;
    fbe_u32_t striper_count_after_create = 0;
    fbe_u32_t striper_count_after_delete = 0;


    mut_printf(MUT_LOG_LOW, "=== Billy Pilgrim Test starts ===\n");
    billy_pilgrim_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
            FBE_OBJECT_ID_INVALID,
            FBE_TRACE_LEVEL_DEBUG_LOW);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /*get the all available drive information*/
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /*Fill the global disk set array with the available drives information*/
    status = rabo_karabekian_get_available_drive_location(&drive_locations,billy_pilgrimDiskSet+2,4);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_before_create);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &striper_count_before_create);
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 10... ===\n");
 
    /* Call Job Service API to create Raid Group type 10 object. */
    billy_pilgrim_create_raid_group(BILLY_PILGRIM_RAID_ID,
                                    FBE_LBA_INVALID, //BILLY_PILGRIM_PVD_SIZE, //0x216a7000,
                                    BILLY_PILGRIM_RAID_GROUP_WIDTH, 
                                    FBE_RAID_GROUP_TYPE_RAID10);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_after_create);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0, &striper_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(mirror_count_after_create, mirror_count_before_create+2, "Found unexpected number of mirrors!");
    MUT_ASSERT_INT_EQUAL_MSG(striper_count_after_create, striper_count_before_create+1, "Found unexpected number of stripers!");
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 10 created successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 10 ... ===\n");
 
    /* Call Job Service API to destroy Raid Group object. */
    billy_pilgrim_remove_raid_group(BILLY_PILGRIM_RAID_ID); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &striper_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    MUT_ASSERT_INT_EQUAL(mirror_count_before_create, mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL(striper_count_before_create, striper_count_after_delete);

    mut_printf(MUT_LOG_TEST_STATUS, "=== First Raid Group Type 5 deleted successfully! ===\n");

#if 0
    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_before_create);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 5... ===\n");
    /* Call Job Service API to create Raid Group type 5 object. */
    billy_pilgrim_create_raid_group(BILLY_PILGRIM_RAID_ID+1,
                                    BILLY_PILGRIM_PVD_SIZE,
                                    BILLY_PILGRIM_RAID_GROUP_WIDTH+1, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

     /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+1, "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully! ===\n");
 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Second Raid Group Type 5... ===\n");

    /* Call Job Service API to create Raid Group type 5 object. */
    billy_pilgrim_create_raid_group(BILLY_PILGRIM_RAID_ID+2,
                                    FBE_LBA_INVALID,
                                    BILLY_PILGRIM_RAID_GROUP_WIDTH+2, 
                                    FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+2, "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 created successfully! ===\n");
#endif

 
#if 0
    /* Call Job Service API to destroy Raid Group object. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting first Raid Group Type 5... ===\n");
    billy_pilgrim_remove_raid_group(BILLY_PILGRIM_RAID_ID+1); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_delete);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");
    MUT_ASSERT_INT_EQUAL(rg_count_before_create+1, rg_count_after_delete);
    mut_printf(MUT_LOG_TEST_STATUS, "=== First Raid Group Type 5 deleted successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting second Raid Group Type 5... ===\n");
    billy_pilgrim_remove_raid_group(BILLY_PILGRIM_RAID_ID+2); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, FBE_PACKAGE_ID_SEP_0,  &rg_count_after_delete);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");
    MUT_ASSERT_INT_EQUAL(rg_count_before_create, rg_count_after_delete);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 deleted successfully! ===\n");
#endif

#if 0
    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_before_create);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &striper_count_before_create);
   
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: mirror_count_before_create %d, striper_count_before_create %d\n",
            __FUNCTION__, mirror_count_before_create, striper_count_before_create);

    /* Call Job Service API to create Raid Group type 10 object. */
    billy_pilgrim_create_raid_group(BILLY_PILGRIM_RAID_ID+3,
                                    BILLY_PILGRIM_PVD_SIZE, //0x216a7000, FBE_LBA_INVALID,
                                    BILLY_PILGRIM_RAID_GROUP_WIDTH+2, 
                                    FBE_RAID_GROUP_TYPE_RAID10);
    /* get the number of RGs in the system and compare after create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_after_create);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0, &striper_count_after_create);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: mirror_count_after_create %d, striper_count_after_create %d\n",
            __FUNCTION__, mirror_count_after_create, striper_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(mirror_count_after_create, mirror_count_before_create+3, "Found unexpected number of mirrors!");
    MUT_ASSERT_INT_EQUAL_MSG(striper_count_after_create, striper_count_before_create+1, "Found unexpected number of stripers!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 10 created successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting First Raid Group Type 10 ... ===\n");

    /* Call Job Service API to destroy Raid Group object. */
    billy_pilgrim_remove_raid_group(BILLY_PILGRIM_RAID_ID+3); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &striper_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: mirror_count_after_delete %d, striper_count_after_delete %d\n",
            __FUNCTION__, mirror_count_after_delete, striper_count_after_delete);

    MUT_ASSERT_INT_EQUAL(mirror_count_after_create-3, mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL(striper_count_after_create-1, striper_count_after_delete);

    mut_printf(MUT_LOG_TEST_STATUS, "=== First Raid Group Type 5 deleted successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Second Raid Group Type 10 ... ===\n");
 
    /* Call Job Service API to destroy Raid Group object. */
    billy_pilgrim_remove_raid_group(BILLY_PILGRIM_RAID_ID); 

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0,  &mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_STRIPER, FBE_PACKAGE_ID_SEP_0,  &striper_count_after_delete);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");

    MUT_ASSERT_INT_EQUAL(mirror_count_before_create-2, mirror_count_after_delete);
    MUT_ASSERT_INT_EQUAL(striper_count_before_create-1, striper_count_after_delete);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Second Raid Group Type 5 deleted successfully! ===\n");
#endif
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Billy Pilgrim Test completed successfully ===",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * billy_pilgrim_create_raid_group()
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

void billy_pilgrim_create_raid_group(fbe_object_id_t raid_group_id,
                                     fbe_lba_t raid_group_capacity,
                                     fbe_u32_t drive_count, 
                                     fbe_raid_group_type_t raid_group_type)
{
    fbe_status_t status;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request = {0};
    fbe_u32_t                                         index = 0;
    fbe_object_id_t 					              raid_group_id1 = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t 					              raid_group_id1_from_job = FBE_OBJECT_ID_INVALID;
	fbe_job_service_error_type_t 					  job_error_code;
    fbe_status_t 									  job_status;
  
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, raid_group_type);

    fbe_raid_group_create_request.drive_count = drive_count;
    fbe_raid_group_create_request.raid_group_id = raid_group_id;

    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus       = billy_pilgrimDiskSet[index].bus;
        fbe_raid_group_create_request.disk_array[index].enclosure = billy_pilgrimDiskSet[index].enclosure;
        fbe_raid_group_create_request.disk_array[index].slot      = billy_pilgrimDiskSet[index].slot;
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
    fbe_raid_group_create_request.job_number            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = BILLY_PILGRIM_TIMEOUT;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_raid_group_create_request.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
										 BILLY_PILGRIM_TIMEOUT,
										 &job_error_code,
										 &job_status,
										 &raid_group_id1_from_job);

	MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
	MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, 
                                                               &raid_group_id1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id1);
    MUT_ASSERT_INT_EQUAL(raid_group_id1_from_job, raid_group_id1);

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
 * billy_pilgrim_remove_raid_group()
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

void billy_pilgrim_remove_raid_group(fbe_raid_group_number_t rg_id)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_destroy_t fbe_raid_group_destroy_request;
	fbe_job_service_error_type_t 			 job_error_code;
    fbe_status_t 							 job_status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - call Job Service API to destroy Raid Group object, START\n",
            __FUNCTION__);
    
    fbe_raid_group_destroy_request.raid_group_id = rg_id;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = BILLY_PILGRIM_TIMEOUT;
    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to destroy raid group");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group destroy call sent successfully! ===\n");

     /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number,
										 BILLY_PILGRIM_TIMEOUT,
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
 * billy_pilgrim_find_object_in_job_service_creation_queue
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

fbe_status_t billy_pilgrim_find_object_in_job_service_creation_queue(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t                 job_queue_element;
    fbe_status_t                            status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_find_object_in_job_service_creation_queue()
 **************************************************************/

/*!**************************************************************
 * billy_pilgrim_disable_creation_queue_processing
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

fbe_status_t billy_pilgrim_disable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: call to disable queue \n",
            __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_disable_creation_queue_processing()
 ******************************************************/

/*!**************************************************************
 * billy_pilgrim_enable_creation_queue_processing
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

fbe_status_t billy_pilgrim_enable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_enable_creation_queue_processing()
 *****************************************************/

/*!**************************************************************
 * billy_pilgrim_add_element_to_queue
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

fbe_status_t billy_pilgrim_add_element_to_queue(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with test values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_add_element_to_queue()
 *****************************************/

/*!**************************************************************
 * billy_pilgrim_get_number_of_element_in_creation_queue
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

fbe_status_t billy_pilgrim_get_number_of_element_in_creation_queue(
        fbe_object_id_t in_object_id, 
        fbe_u32_t       *num_of_objects)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_get_number_of_element_in_creation_queue()
 ************************************************************/

/*!**************************************************************
 * billy_pilgrim_remove_element_from_creation_queue
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

fbe_status_t billy_pilgrim_remove_element_from_creation_queue(fbe_object_id_t in_object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    job_queue_element.object_id = in_object_id;

    /* Setup the job service control element with appropriate values */
    status = fbe_billy_pilgrim_build_control_element (&job_queue_element,
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
 * end billy_pilgrim_remove_element_from_creation_queue()
 *******************************************************/

/*!**************************************************************
 * billy_pilgrim_request_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Billy Pilgrim request
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

static fbe_status_t billy_pilgrim_request_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end billy_pilgrim_request_completion_function()
 ********************************************************/

/*!**************************************************************
 * billy_pilgrim_test_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  Billy Pilgrim request sent to the Job Service
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

static fbe_status_t billy_pilgrim_test_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end billy_pilgrim_test_completion_function()
 ********************************************************/

/*!**************************************************************
 * billy_pilgrim_test_set_cancel_function()
 ****************************************************************
 * @brief
 *  This function sets a cancel function for the 
 *  Billy Pilgrim request sent to the Job Service
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

fbe_status_t billy_pilgrim_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p)
{
    /* Set the packet cancellation and context. */
    return FBE_STATUS_OK;

} 
/*******************************************************
 * end billy_pilgrim_test_set_cancel_function()
 ********************************************************/

/*!**************************************************************
 * fbe_billy_pilgrim_build_control_element()
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

fbe_status_t fbe_billy_pilgrim_build_control_element(
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
 * end fbe_billy_pilgrim_build_control_element()
 ***********************************************/

/*********************************************************************
 * billy_pilgrim_wait_for_step_to_complete()
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
fbe_status_t billy_pilgrim_wait_for_step_to_complete(
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
 * end billy_pilgrim_wait_for_step_to_complete()
 ***********************************************/

/*********************************************************************
 * billy_pilgrim_set_trace_level()
 *********************************************************************
 * @brief
 * sets the trace level for this test
 * 
 * @param - id object to set trace level for
 * @param - trace level 
 *
 * @return fbe_status_t
 *
 * @author
 * 01/13/2010 - Created. Jesus Medina
 *    
 *********************************************************************/
static void billy_pilgrim_set_trace_level(fbe_trace_type_t trace_type,
        fbe_u32_t id,
        fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/***************************************
 * end billy_pilgrim_set_trace_level()
 **************************************/

/*********************************************
 * end fbe_billy_pilgrim_test file
 **********************************************/












