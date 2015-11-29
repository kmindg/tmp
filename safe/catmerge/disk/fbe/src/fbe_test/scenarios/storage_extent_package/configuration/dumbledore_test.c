

/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "generics.h"
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
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "dba_export_types.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_random.h"





char* dumbledore_short_desc = "Verify job action state changed";
char* dumbledore_long_desc = "Verify job action state changed";

/* Count of rows in the table.
 */
#define DUMBLEDORE_TEST_ENCL_MAX 1

/*!*******************************************************************
 * @def DUMBLEDORE_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 1 encl + 15 pdo) 
 *
 *********************************************************************/
#define DUMBLEDORE_TEST_NUMBER_OF_PHYSICAL_OBJECTS 18 

/*!*******************************************************************
 * @def DUMBLEDORE_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define DUMBLEDORE_TEST_NS_TIMEOUT        120000 /*wait maximum of 120 seconds*/

#define DUMBLEDORE_TEST_RAID_GROUP_COUNT 1
#define DUMBLEDORE_TEST_RAID_GROUP_ID_1   0
#define DUMBLEDORE_TEST_RAID_GROUP_ID_2   1
#define DUMBLEDORE_TEST_COPY_FROM_SLOT 6
#define DUMBLEDORE_TEST_COPY_TO_SLOT 9



typedef  struct  dumbledore_test_notification_context_s
 {
     fbe_semaphore_t     sem;
     fbe_object_id_t     object_id;
     fbe_job_type_t     job_type;
     fbe_status_t       job_service_info_status;
     fbe_u64_t          job_number;
 }dumbledore_test_notification_context_t;

/* The different test numbers.
 */
typedef enum dumbledore_test_enclosure_num_e
{
    DUMBLEDORE_TEST_ENCL1_WITH_SAS_DRIVES = 0
    //DUMBLEDORE_TEST_ENCL2_WITH_SAS_DRIVES
} dumbledore_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum dumbledore_test_drive_type_e
{
    SAS_DRIVE,
} dumbledore_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    dumbledore_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    dumbledore_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;


/*!*******************************************************************
 * @def dumbledore_rg_config_1
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t dumbledore_rg_config_1[DUMBLEDORE_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          DUMBLEDORE_TEST_RAID_GROUP_ID_1,
    /* rg_disk_set */
    {{0,0,5}, {0,0,6}, {0,0,7}}
};

/*!*******************************************************************
 * @def dumbledore_rg_config_faulty
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t dumbledore_rg_config_faulty[DUMBLEDORE_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          DUMBLEDORE_TEST_RAID_GROUP_ID_1,
    /* rg_disk_set */
    {{0,0,16}, {0,0,17}, {0,0,18}}
};

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        DUMBLEDORE_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
};

static fbe_lun_number_t lun_number_1 = 0x100;

static void dumbledore_test_load_physical_config(void);
void  dumbledore_test_notification_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context);

/*!**************************************************************
 * dumbledore_test_createrg()
 ****************************************************************
 * @brief
 *  Creates a RG.
 *
 * @param rg_object_id .              
 *
 * @return None.
 *
 ****************************************************************/
void dumbledore_test_createrg(fbe_object_id_t *rg_object_id, fbe_test_rg_configuration_t* rg_config_type, fbe_bool_t faulty) {

   fbe_api_raid_group_create_t         create_rg;
   fbe_status_t status;
   fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

   fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
   create_rg.raid_group_id = rg_config_type[0].raid_group_id;
   create_rg.b_bandwidth = rg_config_type[0].b_bandwidth;
   create_rg.capacity = rg_config_type[0].capacity;
   create_rg.drive_count = rg_config_type[0].width;
   create_rg.raid_type = rg_config_type[0].raid_type;
   create_rg.is_private = FBE_FALSE;

   mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");

   fbe_copy_memory(create_rg.disk_array, rg_config_type[0].rg_disk_set, rg_config_type[0].width * sizeof(fbe_test_raid_group_disk_set_t));

   status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, rg_object_id, &job_error_code);

   if (!faulty) {
      MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                                  "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR"); 
   }
   else {
      MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION, 
                                  "job error code is not FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION"); 
   }
   
  return;
}

void dumbledore_test_createLun(fbe_object_id_t *lun_object_id, fbe_bool_t faulty) {

   fbe_status_t status;
   fbe_api_lun_create_t            fbe_lun_create_req;
   fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

   /* Bind a Lun on RG 1 */
   fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
   mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on a Raid Group start===\n");
   fbe_lun_create_req.raid_type = dumbledore_rg_config_1[0].raid_type;
   fbe_lun_create_req.raid_group_id = dumbledore_rg_config_1[0].raid_group_id; 
   fbe_lun_create_req.lun_number = lun_number_1;
   if (!faulty) {
      fbe_lun_create_req.capacity = 0x1000;
   }
   else {
      fbe_lun_create_req.capacity = 0xfffffffff;
   }
   fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
   fbe_lun_create_req.ndb_b = FBE_FALSE;
   fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
   fbe_lun_create_req.addroffset = FBE_LBA_INVALID; 
   fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
   fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
   

   status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 30000, lun_object_id, &job_error_code);
   

   if (!faulty) {
      MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR , "create lun job fail");

      mut_printf(MUT_LOG_TEST_STATUS, "=== Bind lun on a Raid Group end ===\n");
   }
   else {
      MUT_ASSERT_TRUE(status != FBE_STATUS_OK);   
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_INVALID_ID , "create lun job fail");
   }

   return;
}

void dumbledore_test_updateLun(fbe_object_id_t lun_object_id, fbe_status_t faulty) {

   fbe_status_t status;
   fbe_u32_t attributes = LU_PRIVATE;
   fbe_api_job_service_lun_update_t    lu_update_job;
   fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

   mut_printf(MUT_LOG_TEST_STATUS, "---START to update lun ---\n");
   lu_update_job.object_id = lun_object_id;
   lu_update_job.update_type = FBE_LUN_UPDATE_ATTRIBUTES;
   lu_update_job.attributes = attributes;
   status = fbe_api_update_lun_attributes(lun_object_id, attributes, &job_error_code);
   if (!faulty) {
      MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);
   }
   else {
      MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
      MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_UNKNOWN_ID, job_error_code);
   }

   return;
}

void dumbledore_test_verifyValues(fbe_object_id_t object_id, fbe_job_type_t job_type, dumbledore_test_notification_context_t *context, fbe_status_t jobStatus, fbe_bool_t faulty) {
   
   if (!faulty && object_id != FBE_OBJECT_ID_INVALID) {
      MUT_ASSERT_TRUE(context->object_id == object_id);
      mut_printf(MUT_LOG_LOW, " Job Object ID Matched! \n");
   }
   MUT_ASSERT_TRUE(context->job_type == job_type);
   mut_printf(MUT_LOG_LOW, " Job Type Matched!\n");
   MUT_ASSERT_TRUE(context->job_service_info_status == jobStatus);
   mut_printf(MUT_LOG_LOW, " Job Status Matched! \n");


   return;
}

void dumbledore_test_destroyLun(fbe_object_id_t lun_object_id, fbe_bool_t faulty) {
   fbe_status_t                    status;
   fbe_api_lun_destroy_t           fbe_lun_destroy_req;
   fbe_job_service_error_type_t    job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;
   fbe_lun_number_t lun_number;

   status = fbe_api_database_lookup_lun_by_object_id(lun_object_id,&lun_number);
   if (!faulty) {
      MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

      mut_printf(MUT_LOG_LOW, " lun number = 0x%x", lun_number);
   }
   else {
      MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
      lun_number = 0xFFFFFFFF;
   }

   fbe_lun_destroy_req.lun_number = lun_number;

   status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 30000, &job_error_code);

   if (!faulty) {
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, 
                                  "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");
   }
   else {
      MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
      MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_INVALID_ID, 
                                  "job error code is not FBE_JOB_SERVICE_ERROR_INVALID_ID");
   }
   return;
}

void dumbledore_test_destroyRg(fbe_object_id_t rg_object_id, fbe_bool_t faulty) {
   fbe_status_t                     status;
   fbe_api_destroy_rg_t             destroy_rg;
   fbe_raid_group_number_t        raid_group_id;
   fbe_job_service_error_type_t     job_error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

   status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_id);
   if (!faulty) {
      MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   }
   else{
      MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
   }

   /* Destroy a RAID group */
      fbe_zero_memory(&destroy_rg, sizeof(fbe_api_destroy_rg_t));
    destroy_rg.raid_group_id = raid_group_id;

    status = fbe_api_destroy_rg(&destroy_rg, FBE_TRUE, 100000, &job_error_code);
    if (!faulty) {
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_NO_ERROR, "job error code is not FBE_JOB_SERVICE_ERROR_NO_ERROR");
    }
    else {
       MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
       MUT_ASSERT_INT_EQUAL_MSG(job_error_code, FBE_JOB_SERVICE_ERROR_UNKNOWN_ID, "job error code is not FBE_JOB_SERVICE_ERROR_UNKNOWN_ID");
    }

    return;
}


void dumbledore_test_enable_rgPowerSaving(fbe_object_id_t *rg_object_id) {
   fbe_status_t status;
   fbe_base_config_object_power_save_info_t    object_ps_info;

   status = fbe_api_enable_raid_group_power_save(*rg_object_id);
   MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   /*let's wait for a while since RG propagate it to the LU*/
   fbe_api_sleep(14000);

   status = fbe_api_get_object_power_save_info(*rg_object_id, &object_ps_info);
   MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);
  
   return;
}

void dumbledore_test_disable_rgPowerSaving(fbe_object_id_t *rg_object_id) {
   fbe_status_t status;
   fbe_base_config_object_power_save_info_t    object_ps_info;

   status = fbe_api_disable_raid_group_power_save(*rg_object_id);
   MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /*let's wait for a while since RG propagate it to the LU*/
   fbe_api_sleep(14000);

   status = fbe_api_get_object_power_save_info(*rg_object_id, &object_ps_info);

   MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_FALSE);

   return;
}

fbe_status_t dumbledore_test_copy_to_replacement_disk(fbe_object_id_t rg_object_id)
{
   fbe_u32_t                               source_slot = DUMBLEDORE_TEST_COPY_FROM_SLOT;
   fbe_u32_t                               dest_slot = DUMBLEDORE_TEST_COPY_TO_SLOT;
   //fbe_database_raid_group_info_t          rg_info;
   fbe_status_t                            status;
   fbe_object_id_t                         dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
   fbe_object_id_t                         src_pvd_object_id = FBE_OBJECT_ID_INVALID; 
   fbe_provision_drive_copy_to_status_t    copy_status;

   // Get source pvd id for user copy.
   status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, source_slot, &src_pvd_object_id);
   MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   // Get destination PVD id.
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, dest_slot, &dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_copy_to_replacement_disk(src_pvd_object_id, dest_pvd_object_id, &copy_status);

    return status;
}


/*!**************************************************************
 * dumbledore_dualsp_test()
 ****************************************************************
 * @brief
 *  dumbledore_dualsp_test creates correct and faulty configurations.
 *  It registers for notifications for job action state change(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED) and verifies if the job attributes such as error code, type, status etc., contain expected values.
 *  
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

void dumbledore_dualsp_test(void) {

   fbe_status_t status;
   fbe_status_t sem_status;
   fbe_sim_transport_connection_target_t   active_connection_target    = FBE_SIM_INVALID_SERVER;
   fbe_sim_transport_connection_target_t   passive_connection_target   = FBE_SIM_INVALID_SERVER;
   fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
   fbe_object_id_t                     lun_object_id = FBE_OBJECT_ID_INVALID;

   dumbledore_test_notification_context_t dumbledore_context;
   fbe_notification_registration_id_t          reg_id;

   /* Get the active SP */
   fbe_test_sep_get_active_target_id(&active_connection_target);
   MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);

   mut_printf(MUT_LOG_LOW, "=== ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

   /* set the passive SP */
   passive_connection_target = (active_connection_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);
   fbe_api_sim_transport_set_target_server(passive_connection_target);

   //dumbledore_test_initialize(&dumbledore_context, &reg_id);

   fbe_semaphore_init(&dumbledore_context.sem, 0, 1);

   status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED ,
                                                                      FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                      FBE_TOPOLOGY_OBJECT_TYPE_ALL,
                                                                      dumbledore_test_notification_callback_function,
                                                                      &dumbledore_context,
                                                                      &reg_id);
   MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*------------- Faulty Configuration -----------------------*/

   //Create a Faulty RG
   dumbledore_test_createrg(&rg_object_id, dumbledore_rg_config_faulty, true);
   // wait for the notification
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   //Verify if the values are right.
   dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_RAID_GROUP_CREATE, &dumbledore_context, FBE_STATUS_GENERIC_FAILURE, true);

   // Create faulty LUN
   dumbledore_test_createLun(&lun_object_id, true);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_CREATE, &dumbledore_context, FBE_STATUS_NO_OBJECT, true);

   // Update faulty LUN
   lun_object_id = FBE_OBJECT_ID_INVALID;
   dumbledore_test_updateLun(lun_object_id, true);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_UPDATE, &dumbledore_context, FBE_STATUS_OK, true);

   //Destroy faulty LUN
   lun_object_id = FBE_OBJECT_ID_INVALID;
   dumbledore_test_destroyLun(lun_object_id, true);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_DESTROY, &dumbledore_context, FBE_STATUS_NO_OBJECT, true);

   //Destroy faulty RG
   rg_object_id = FBE_OBJECT_ID_INVALID;
   dumbledore_test_destroyRg(rg_object_id, true);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_RAID_GROUP_DESTROY, &dumbledore_context, FBE_STATUS_NO_OBJECT, true);



   /*----------------------- Correct Configuration ------------------*/

   rg_object_id = FBE_OBJECT_ID_INVALID;
   lun_object_id = FBE_OBJECT_ID_INVALID;
   //Create a RG
   dumbledore_test_createrg(&rg_object_id, dumbledore_rg_config_1, false);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_RAID_GROUP_CREATE, &dumbledore_context, FBE_STATUS_OK, false);

   //Enable power saving on RG
   dumbledore_test_enable_rgPowerSaving(&rg_object_id);
   sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
   MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
   dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_UPDATE_RAID_GROUP, &dumbledore_context, FBE_STATUS_OK, false);

    // Create LUN
    dumbledore_test_createLun(&lun_object_id, false);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_CREATE, &dumbledore_context, FBE_STATUS_OK, false);

     // Update LUN
    dumbledore_test_updateLun(lun_object_id, false);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_UPDATE, &dumbledore_context, FBE_STATUS_OK, false);

    //Copy to replacement disk
    //mut_printf(MUT_LOG_LOW, "======== CopyTo Before LUN Destroyed ========\n");
    //status = dumbledore_test_copy_to_replacement_disk(rg_object_id);
    // The failure is expected for the copy to operation on an unconsumed RG.
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    //MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    //dumbledore_test_verifyValues(FBE_OBJECT_ID_INVALID, FBE_JOB_TYPE_DRIVE_SWAP, &dumbledore_context, FBE_STATUS_OK, false);

    //Destroy LUN
    dumbledore_test_destroyLun(lun_object_id, false);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(lun_object_id, FBE_JOB_TYPE_LUN_DESTROY, &dumbledore_context, FBE_STATUS_OK, false);

    //Copy to replacement disk
    mut_printf(MUT_LOG_LOW, "======== CopyTo After LUN Destroyed ========\n");
    status = dumbledore_test_copy_to_replacement_disk(rg_object_id);
    // The failure is expected for the copy to operation on an unconsumed RG.
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(FBE_OBJECT_ID_INVALID, FBE_JOB_TYPE_DRIVE_SWAP, &dumbledore_context, FBE_STATUS_GENERIC_FAILURE, false);
    MUT_ASSERT_TRUE_MSG((dumbledore_context.job_number != 0), "ERROR: Job sends a notification with a job_number 0!\n");

     //Disable power saving on RG
    dumbledore_test_disable_rgPowerSaving(&rg_object_id);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_UPDATE_RAID_GROUP, &dumbledore_context, FBE_STATUS_OK, false);

    //Destroy RG
    dumbledore_test_destroyRg(rg_object_id, false);
    sem_status = fbe_semaphore_wait_ms(&dumbledore_context.sem, 20000);
    MUT_ASSERT_TRUE(sem_status != FBE_STATUS_TIMEOUT);
    dumbledore_test_verifyValues(rg_object_id, FBE_JOB_TYPE_RAID_GROUP_DESTROY, &dumbledore_context, FBE_STATUS_OK, false);

    // Unregister the notification
    status = fbe_api_notification_interface_unregister_notification(dumbledore_test_notification_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&dumbledore_context.sem);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   return;
}


/*!**************************************************************
 * dumbledore_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the dumbledore test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void dumbledore_test_load_physical_config(void)
{

    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               total_objects = 0;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[DUMBLEDORE_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < DUMBLEDORE_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < DUMBLEDORE_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
        {
            fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number, 
                                              encl_table[enclosure].encl_number,   
                                              slot,                                
                                              encl_table[enclosure].block_size,    
                                              TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY * 4,      
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
    status = fbe_api_wait_for_number_of_objects(DUMBLEDORE_TEST_NUMBER_OF_PHYSICAL_OBJECTS, DUMBLEDORE_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == DUMBLEDORE_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End dumbledore_test_load_physical_config() */


void dumbledore_test_notification_callback_function(update_object_msg_t * update_obj_msg,void * context)
{
    dumbledore_test_notification_context_t*  notification_context = (dumbledore_test_notification_context_t*)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    notification_context->object_id = update_obj_msg->object_id;
    notification_context->job_type = update_obj_msg->notification_info.notification_data.job_service_error_info.job_type;
    notification_context->job_service_info_status = update_obj_msg->notification_info.notification_data.job_service_error_info.status;
    notification_context->job_number = update_obj_msg->notification_info.notification_data.job_service_error_info.job_number;
    fbe_semaphore_release(&notification_context->sem, 0, 1, FALSE);
}

/*!****************************************************************************
 *  dumbledore_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the dumbledore_test. It is responsible
 *  for loading the physical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumbledore_dualsp_setup(void) {

   fbe_sim_transport_connection_target_t   sp;

   if (fbe_test_util_is_simulation())
   {
       mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for dumbledore dualsp test ===\n");
       /* Set the target server */
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

       /* Make sure target set correctly */
       sp = fbe_api_sim_transport_get_target_server();
       MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

       mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                  sp == FBE_SIM_SP_A ? "SP A" : "SP B");

       /* Load the physical config on the target server */
       dumbledore_test_load_physical_config();

       /* Set the target server */
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

       /* Make sure target set correctly */
       sp = fbe_api_sim_transport_get_target_server();
       MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

       mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                  sp == FBE_SIM_SP_A ? "SP A" : "SP B");

       /* Load the physical config on the target server */
       dumbledore_test_load_physical_config();

       /* We will work primarily with SPA.  The configuration is automatically
        * instantiated on both SPs.  We no longer create the raid groups during
        * the setup.
        */
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
       sep_config_load_sep_and_neit_both_sps();
   }


  return;
}

/*!****************************************************************************
 *  dumbledore_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the dumbledore_dualsp_test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void dumbledore_dualsp_cleanup(void) {
   mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for dumbledore_dualsp_test ===\n");

    // Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* clear the error counter */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    //if (fbe_test_util_is_simulation())
    //{
        // First execute teardown on SP B then on SP A
      //  fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
       // fbe_test_sep_util_destroy_neit_sep_physical();

        // First execute teardown on SP A
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        //fbe_test_sep_util_destroy_neit_sep_physical();
    //}
   return;
}
