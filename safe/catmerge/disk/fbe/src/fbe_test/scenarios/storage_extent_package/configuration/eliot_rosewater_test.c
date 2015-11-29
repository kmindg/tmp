/*!*************************************************************************
 * @file eliot_rosewater_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for Creating Raid Groups on dual SPs via the
 *   Job service API and Config service API
 *
 * @version
 *   07/13/2010 - Created. Jesus medina
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
#include "pp_utils.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_job_service.h"
#include "fbe_test_configurations.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_job_service_interface.h" 
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_test_common_utils.h"

char * eliot_rosewater_short_desc = "API Raid Group creation/persistent on both SPs with Job Service test";
char * eliot_rosewater_long_desc =
"\n"
"\n"
"The Eliot Rosewater Scenario tests the infrastructure needed for Raid Group\n"
"creation on dual SPs environment. \n"
"\n"
"This scenario tests the creation of Raid Group for the following cases:\n"
"\n"
"  - A RG creation consuming the entire virtual drive exported capacity\n"
"\n"
"It covers the following cases:\n"
"\t- It verifies that the Create Raid Group API can successfully create Raid Groups\n"
"\t  via Job service\n"
"\t- It verifies that configuration service can offer transaction operations to update\n"
"\t  objects in memory and on disk on both SPs\n"
"\t- It verifies that configuration service can persists objects in memory and on disk.\n"
"\t- on both SPs.\n"
"\n"
"Dependencies: \n"
"\t- The Job Service ability to queue create type requests and process these \n"
"\t  type of requests\n"
"\t- Configuration service ability to provide APIs to open and close\n"
"\t  update-in-memory transactions.\n"
"\t- Configuration service ability to persist database with the\n"
"\t  Persistence service. \n"
"\t  Persistence service API to read-in persisted objectson both SPs\n"
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
"\t- Create a Raid Group type 5.\n"
"\t- Verify RG is persisted on Active side.\n"
"\t- Verify RG is persisted on Passive side.\n"
"STEP 2: call SEP API to Remove RG object.\n"
"\t- Verify RG no longer exists on Active side.\n"
"\t- Verify RG no longer exists on Passive side.\n"
"STEP 3: call SEP API to create RG object with error.\n"
"\t- Verify Config service aborts transaction on Active side.\n"
"\t- Verify Config service aborts transaction on passive side.\n"
"\t- Verify RG was not created on any SP due to error.\n"
"Last update 12/15/11\n"

;

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a function
 *         register
 */
#define ELIOT_ROSEWATER_TIMEOUT  60 /* no. of seconds to wait */

#define ELIOT_ROSEWATER_WAIT_MSEC  (1000 * ELIOT_ROSEWATER_TIMEOUT) /* no. of milli-seconds to wait */

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*!*******************************************************************
 * @var eliot_rosewater_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t eliot_rosewater_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                      block size      RAID-id.    bandwidth.*/
    {4,       FBE_LBA_INVALID,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
fbe_sim_transport_connection_target_t eliot_rosewater_get_active_side(void);

static void eliot_rosewater_set_trace_level(fbe_trace_type_t trace_type,
        fbe_u32_t id,
        fbe_trace_level_t level);


/* RG delete function */
static void eliot_rosewater_remove_raid_group(fbe_raid_group_number_t rg_id); 

/* RG create function */
static void eliot_rosewater_create_raid_group(fbe_test_rg_configuration_t *rg_config_p);

/* Eliot Rosewater step functions */

/* Call Job Service API to create Raid Group objects. Verify it has been persisted on both SPs */
static void step1(fbe_test_rg_configuration_t *rg_config_ptr);

/* delete RG, verify it has been deleted on both SPs */
static void step2(fbe_test_rg_configuration_t *rg_config_p);

/* Create RG with error, verify Config service aborts transaction on both SPs  */
//static void step3();

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/


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
 * @param rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 10/03/2011 - Modified Sandeep Chaudhari
 ****************************************************************/

void step1(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request = {0};
    fbe_u32_t                                         provision_index = 0;
//    fbe_u32_t                                         provision_index2 = 0;
    fbe_object_id_t                                   raid_group_id;   
    fbe_job_service_error_type_t                      job_error_code;
    fbe_status_t                                      job_status;
    fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t previous_active_connection_target = FBE_SIM_INVALID_SERVER;

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - Step 2,  call Job Service API to create Raid Group objects\n",
            __FUNCTION__);

    fbe_raid_group_create_request.drive_count = rg_config_p->width;
    fbe_raid_group_create_request.raid_group_id = rg_config_p->raid_group_id;
    
    for (provision_index = 0;  provision_index < fbe_raid_group_create_request.drive_count;  
            provision_index++)
    {
        fbe_raid_group_create_request.disk_array[provision_index].bus = rg_config_p->rg_disk_set[provision_index].bus;
        fbe_raid_group_create_request.disk_array[provision_index].enclosure = rg_config_p->rg_disk_set[provision_index].enclosure;
        fbe_raid_group_create_request.disk_array[provision_index].slot= rg_config_p->rg_disk_set[provision_index].slot;
    }

    /* The test assumes the Raid Group creation takes the whole size exported
     * by the provision drive. Part 2 will address raid group specific sizes
     */ 
    fbe_raid_group_create_request.b_bandwidth           = rg_config_p->b_bandwidth; 
    fbe_raid_group_create_request.capacity              = rg_config_p->capacity; 
    fbe_raid_group_create_request.raid_type             = rg_config_p->raid_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = ELIOT_ROSEWATER_WAIT_MSEC;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating RG on Active side ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Waiting on Job %llu ===\n",
	       (unsigned long long)fbe_raid_group_create_request.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         ELIOT_ROSEWATER_WAIT_MSEC,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id,
            &raid_group_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id, FBE_LIFECYCLE_STATE_READY, ELIOT_ROSEWATER_WAIT_MSEC,
            FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_api_sleep(5000);

    /* get active side target */
    active_connection_target = eliot_rosewater_get_active_side();

    if (active_connection_target == FBE_SIM_SP_A)
    {
        passive_connection_target = FBE_SIM_SP_B;
    }
    /* created RG on ACTIVE side*/
    mut_printf(MUT_LOG_LOW, "=== Created Raid Group object %d on Active side (%s) ===", 
            raid_group_id, active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    mut_printf(MUT_LOG_LOW, "=== Verifying Raid Group object %d on Passive side (%s) ===", 
            raid_group_id, active_connection_target != FBE_SIM_SP_A ? "SP A" : "SP B");

    if (active_connection_target == FBE_SIM_SP_A)
    {
        /* make sure SPB(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        previous_active_connection_target = FBE_SIM_SP_A;
        passive_connection_target = FBE_SIM_SP_B;
    }
    else
    {
        /* make sure SPA(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        previous_active_connection_target = FBE_SIM_SP_B;
        passive_connection_target = FBE_SIM_SP_A;
    }

    /* find the object id of the raid group on SPB */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id,
            &raid_group_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    mut_printf(MUT_LOG_LOW, "=== Verified Raid Group object %d on Passive side(%s) ===", 
            raid_group_id, active_connection_target != FBE_SIM_SP_A ? "SP A" : "SP B");

    /* reset active size */
    //fbe_api_sim_transport_set_target_server(active_connection_target);

#if 0       
    mut_printf(MUT_LOG_TEST_STATUS, "\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating RG on Passive side ===\n");

    /* ========================================== */
    /*      create a new RG on passive side       */
    /* ========================================== */

    fbe_raid_group_create_request.drive_count = ELIOT_ROSEWATER_RAID_GROUP_WIDTH-1;
    fbe_raid_group_create_request.raid_group_id = ELIOT_ROSEWATER_RAID_ID+1;

    provision_index2 = ELIOT_ROSEWATER_RAID_GROUP_WIDTH;
    for (provision_index = 0;  provision_index < fbe_raid_group_create_request.drive_count;  
            provision_index++)
    {
        fbe_raid_group_create_request.disk_array[provision_index].bus = 
            eliot_rosewaterDiskSet[provision_index2].bus;
        fbe_raid_group_create_request.disk_array[provision_index].enclosure = 
            eliot_rosewaterDiskSet[provision_index2].enclosure;
        fbe_raid_group_create_request.disk_array[provision_index].slot = 
            eliot_rosewaterDiskSet[provision_index2].slot;      
        provision_index2++;
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

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Waiting on Job %d ===\n",fbe_raid_group_create_request.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
            ELIOT_ROSEWATER_TIMEOUT,
            &job_error_code,
            &job_status);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id,
            &raid_group_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id, FBE_LIFECYCLE_STATE_READY, ELIOT_ROSEWATER_WAIT_MSEC,
            FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* created RG on PASSIVE side*/
    mut_printf(MUT_LOG_LOW, "=== Created Raid Group object %d on PASSIVE side (%s) ===", 
            raid_group_id, active_connection_target != FBE_SIM_SP_A ? "SP A" : "SP B");

    mut_printf(MUT_LOG_LOW, "=== Verifying Raid Group object %d on ACTIVE side (%s) ===", 
            raid_group_id, active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    if (active_connection_target == FBE_SIM_SP_A)
    {
        /* make sure SPB(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        previous_active_connection_target = FBE_SIM_SP_A;
        passive_connection_target = FBE_SIM_SP_B;
    }
    else
    {
        /* make sure SPA(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        previous_active_connection_target = FBE_SIM_SP_B;
        passive_connection_target = FBE_SIM_SP_A;
    }

    /* reset active size */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /* find the object id of the raid group on SPB */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id,
            &raid_group_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    mut_printf(MUT_LOG_LOW, "=== Verified Raid Group object %d on ACTIVE side (%s) ===", 
            raid_group_id, active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

#endif
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end - Step 2,  call Job Service API to create Raid Group object, END\n",
            __FUNCTION__);
}

/*!**************************************************************
 * step2()
 ****************************************************************
 * @brief
 *
 * Remove RG object and verify RG no longer exists on Active side
 * as well as on Passive side
 *
 * a) Remove RG.
 * b) Verify RG no longer exists on Active side.
 * c) Verify RG no longer exists on Passive side.
 *
 * @param - rg_config_p - Pointer to the raid group config info 
 *
 * @return - none
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 10/03/2011 - Modified Sandeep Chaudhari
 ****************************************************************/
void step2(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status;
    fbe_object_id_t                         raid_group_id;
    fbe_sim_transport_connection_target_t   active_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   passive_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   previous_active_connection_target = FBE_SIM_INVALID_SERVER;

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - Step 2, Delete RG, Verify Raid Group object has been removed on both SPs\n",
            __FUNCTION__);

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    mut_printf(MUT_LOG_TEST_STATUS, "Raid Group ID : %d \t Raid Group Object ID : 0x%x",
                                        rg_config_p->raid_group_id,
                                        raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 5... ===\n");

    /* Call Job Service API to destroy Raid Group object. */
    eliot_rosewater_remove_raid_group(rg_config_p->raid_group_id);

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);
    
    /* get active side target */
    active_connection_target = eliot_rosewater_get_active_side();

    if (active_connection_target == FBE_SIM_SP_A)
    {
        passive_connection_target = FBE_SIM_SP_B;
    }

    /* created RG on ACTIVE side*/
    mut_printf(MUT_LOG_LOW, "=== Raid Group %d Deleted successfully on Active side (%s) ===", 
            raid_group_id, active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    mut_printf(MUT_LOG_LOW, "=== Verifying Raid Group %d does not exists on Passive side(%s) ===", 
            raid_group_id, active_connection_target != FBE_SIM_SP_A ? "SP A" : "SP B");

    if (active_connection_target == FBE_SIM_SP_A)
    {
        /* make sure SPB(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        previous_active_connection_target = FBE_SIM_SP_A;
        passive_connection_target = FBE_SIM_SP_B;
    }
    else
    {
        /* make sure SPA(passive side) agrees with the configuration */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        previous_active_connection_target = FBE_SIM_SP_B;
        passive_connection_target = FBE_SIM_SP_A;
    }

    /* Verify that the Raid components have been destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    mut_printf(MUT_LOG_LOW, "=== Verified Raid Group %d does not exists on Passive side (%s) ===", 
                                    rg_config_p->raid_group_id,
                                    active_connection_target != FBE_SIM_SP_A ? "SP A" : "SP B");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: end - Step 2,  Verify Raid Group object has been removed, END\n",
            __FUNCTION__);

    return;
}

/*!**************************************************************
 * eliot_rosewater_create_raid_group()
 ****************************************************************
 * @brief
 *  This function builds and sends a Raid group creation request
 *  to the Job service
 *
 * @param rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 10/03/2011 - Modified Sandeep Chaudhari
 ****************************************************************/
void eliot_rosewater_create_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_api_job_service_raid_group_create_t fbe_raid_group_create_request = {0};
    fbe_u32_t                               index = 0;
    fbe_object_id_t                         raid_group_id1 = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t            job_error_code;
    fbe_status_t                            job_status;
  
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, rg_config_p->raid_type);

    
    fbe_raid_group_create_request.drive_count    = rg_config_p->width;
    fbe_raid_group_create_request.raid_group_id  = rg_config_p->raid_group_id;
    
    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus = rg_config_p->rg_disk_set[index].bus;
        fbe_raid_group_create_request.disk_array[index].enclosure = rg_config_p->rg_disk_set[index].enclosure;
        fbe_raid_group_create_request.disk_array[index].slot = rg_config_p->rg_disk_set[index].slot;
    }

    /* b_bandwidth = true indicates 
     * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024 for element_size 
     */
    fbe_raid_group_create_request.b_bandwidth           = FBE_TRUE; 
    fbe_raid_group_create_request.capacity              = rg_config_p->capacity; 
    fbe_raid_group_create_request.raid_type             = rg_config_p->raid_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = ELIOT_ROSEWATER_WAIT_MSEC;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fbe_api_job_service_raid_group_create failed");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

   /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         ELIOT_ROSEWATER_TIMEOUT,
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
                                                     FBE_LIFECYCLE_STATE_READY, ELIOT_ROSEWATER_WAIT_MSEC,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__,
            rg_config_p->raid_type);
}

/*!**************************************************************
 * eliot_rosewater_remove_raid_group()
 ****************************************************************
 * @brief
 * This function removes a raid group
 *
 * @param - rg_id id of raid group to remove 
 *
 * @return - none
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void eliot_rosewater_remove_raid_group(fbe_raid_group_number_t rg_id)
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
    fbe_raid_group_destroy_request.destroy_timeout_msec = ELIOT_ROSEWATER_WAIT_MSEC;
    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to destroy raid group");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group destroy call sent successfully! ===\n");

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number,
                                         ELIOT_ROSEWATER_WAIT_MSEC,
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
 * eliot_rosewater_get_active_side
 ****************************************************************
 * @brief
 *  This function returns the SP that is currently defined
 *  as Active
 *
 * @param - void
 * @return fbe_status - status of search request
 *
 * @author
 * 07/26/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/
fbe_sim_transport_connection_target_t eliot_rosewater_get_active_side(void)
{
    fbe_cmi_service_get_info_t spa_cmi_info;
    fbe_cmi_service_get_info_t spb_cmi_info;
    fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;

    do {
        fbe_api_sleep(100);
        /* We have to figure out which SP is ACTIVE */
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
    mut_printf(MUT_LOG_LOW, "=== ACTIVE side (%s) ===", 
            active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    return active_connection_target;
}



/*!**************************************************************
 * fbe_eliot_rosewater_build_control_element()
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

fbe_status_t fbe_eliot_rosewater_build_control_element(
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
 * end fbe_eliot_rosewater_build_control_element()
 ***********************************************/

/*********************************************************************
 * eliot_rosewater_set_trace_level()
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
static void eliot_rosewater_set_trace_level(fbe_trace_type_t trace_type,
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
 * end eliot_rosewater_set_trace_level()
 **************************************/

/*!**************************************************************
 * eliot_rosewater_dualsp_run_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the Eliot Rosewater
 * test: API Raid Group creation with Job Service test
 * 
 * @param  - rg_config_p        - pointer to raid group
 * @param  - context_pp         - pointer to context
 * @return - none
 *
 * @author
 * 09/30/2011 - Created. Sandeep Chaudhari.
 * 
 ****************************************************************/

void eliot_rosewater_dualsp_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void * context_p)
{
    fbe_u32_t                       raid_group_count = 0;
    fbe_u32_t                       rg_index = 0;
    fbe_test_rg_configuration_t     *rg_config_p;

    raid_group_count =  fbe_test_get_rg_array_length(rg_config_ptr);
    
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Call Job Service API to create a Raid Group type 5 object--no errors. */
            step1(rg_config_ptr);
            
            /* delete RG and verify it does not exist on any SP */
            step2(rg_config_ptr);
        }
    }
    
    return;
}

/*!**************************************************************
 * eliot_rosewater_dualsp_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the Eliot Rosewater
 * test: API Raid Group creation with Job Service test
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/
void eliot_rosewater_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    mut_printf(MUT_LOG_LOW, "=== Eliot Rosewater Test starts ===\n");

    eliot_rosewater_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
                                    FBE_OBJECT_ID_INVALID,
                                    FBE_TRACE_LEVEL_DEBUG_LOW);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &eliot_rosewater_raid_group_config[0];
    
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, eliot_rosewater_dualsp_run_test);
    
    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Eliot Rosewater Test completed successfully ===",
            __FUNCTION__);
    return;
}


/*!**************************************************************
 * eliot_rosewater_dualsp_setup
 ****************************************************************
 * @brief
 *  This function inits the Eliot Rosewater test
 *
 * @return - none
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/
void eliot_rosewater_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;

    if (fbe_test_util_is_simulation())
    { 
        /* we do the setup on SPB side*/
        mut_printf(MUT_LOG_LOW, "=== eliot_rosewater_setup_test Loading Physical Config on SPB ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Load the physical package and create the physical configuration. */
        job_control_physical_config();

        /* we do the setup on SPA side*/
        mut_printf(MUT_LOG_LOW, "=== eliot_rosewater_setup test Loading Physical Config on SPA ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        
        /* Load the physical package and create the physical configuration. */
        job_control_physical_config();

		sep_config_load_sep_with_time_save(FBE_TRUE);
    
        /* get active side target */
        active_connection_target = eliot_rosewater_get_active_side();

        /* we do the setup on ACTIVE side*/
        mut_printf(MUT_LOG_LOW, "=== eliot_rosewater_setup Loading Config on ACTIVE side (%s) ===", 
                active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        fbe_api_sim_transport_set_target_server(active_connection_target);/*just to be on the safe side, even though it's the default*/
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}

/*!**************************************************************
 * eliot_rosewater_dualsp_cleanup
 ****************************************************************
 * @brief
 *  This function destroys the Eliot Rosewater configuration
 *
 * @return none 
 *
 * @author
 * 07/13/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void eliot_rosewater_dualsp_cleanup()
{
    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    { 
        /*start with SPB so we leave the switch at SPA when we exit*/
        mut_printf(MUT_LOG_LOW, "=== eliot_rosewatertest unloading Config from SPB ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_sep_physical();
    
        /*back to A*/
        mut_printf(MUT_LOG_LOW, "=== eliot_rosewater_test Unloading Config from SPA ===");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_sep_physical();   
    }

    return;
}


/*********************************************
 * end fbe_eliot_rosewater_test file
 **********************************************/



