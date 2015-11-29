/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cassie_dualsp_test.c
 ***************************************************************************
 *
 * @brief
 *  This test is used to test the system db header operation
 *
 * @version
 *   04/19/2012 - Created. Zhipeng Hu
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_database.h"
#include "fbe/fbe_api_event_log_interface.h"


char * cassie_dualsp_test_short_desc = "database versioning dualsp test.";
char * cassie_dualsp_test_long_desc ="\
run the database versioning test cases \
1. The cassie_dualsp scenario tests system db header operation in SEP NDU stage \
\n\
Dependencies:\n\
        - database service.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] SAS drives and SATA drives\n\
        [PP] logical drive\n\
        [SEP] provision drive\n\
\n\
Case 1 test system db header operation\
STEP 1: Setup - Create initial topology.\n\
        - Create initial RGs and LUNs.\n\
STEP 2: Get system db headers from both SPs \n\
        - verify they are equal to each other.\n\
STEP 3: Change the system db header on active side to a newer version\n\
STEP 4: Reboot SPB\n\
        - get system db headers from both SPs\n\
        - check whether they are equal to each other\n\
        - check the version is the new one\n\
STEP 5: Reboot SPA\n\
        -get system db headers from both SPs\n\
        -check whether they are equal to each other\n\
        -check the version is the new one\n\
STEP 6: Commit NDU\n\
        -send NDU commit request to database service of SPA (it is passive side now)\n\
        -wait the notification of NDU commit job\n\
        -get system db headers from both SPs\n\
        -check they are equal to each other\n\
        -check the version is the original one\n\
\n\
case 2: test the unknown db CMI messaage handling\n\
STEP1: send a unknown type of cmi message from SPA to SPB \n\
STEP2: wait for the cmi message back\n\
";

/* The number of LUNs in our raid group.
 */
#define CASSIE_DUALSP_TEST_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def CASSIE_DUALSP_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define CASSIE_DUALSP_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def CASSIE_DUALSP_TEST_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define CASSIE_DUALSP_TEST_MAX_RAID_GROUP_COUNT 3

/*!*******************************************************************
 * @def CASSIE_DUALSP_TEST_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define CASSIE_DUALSP_TEST_WAIT_MSEC 60000

/*!*******************************************************************
 * @def CASSIE_DUALSP_TEST_NEW_SEP_VERSION
 *********************************************************************
 * @brief new SEP version we want to set.
 *
 *********************************************************************/
#define CASSIE_DUALSP_TEST_NEW_SEP_VERSION 19880724


/*!*******************************************************************
 * @var cassie_dualsp_raid_groups
 *********************************************************************
 * @brief Test config.
 *
 *********************************************************************/
fbe_test_rg_configuration_t cassie_dualsp_raid_groups[CASSIE_DUALSP_TEST_MAX_RAID_GROUP_COUNT + 1] = 
{
    /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,    520,            3,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,    520,            2,          0},
    {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,    520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

typedef  struct  cassie_dualsp_test_ndu_commit_context_s
{
    fbe_semaphore_t     sem;
    fbe_job_service_info_t    job_srv_info;
}cassie_dualsp_test_ndu_commit_context_t;


#define CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE 4

/*!*******************************************************************
 * function declarations
 *********************************************************************/
static void cassie_dualsp_load_physical_config(fbe_test_rg_configuration_t *rg_config_p);

static void  cassie_dualsp_test_ndu_commit_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context);

static fbe_status_t    cassie_dualsp_get_and_verify_system_db_header(database_system_db_header_t* out_sys_db_header);

static fbe_status_t  cassie_dualsp_check_entry_value(void);

static void    cassie_dualsp_print_system_db_header(database_system_db_header_t* sys_db_header);

static void cassie_dualsp_reboot_system(fbe_sim_transport_connection_target_t target);

static void cassie_dualsp_create_topology(void);
static void cassie_dualsp_destroy_topology(void);

static void cassie_dualsp_step2_verify_after_first_boot(void);

static void cassie_dualsp_step3_verify_after_modify(void);

static void cassie_dualsp_step4_verify_ndu_commit(void);


/*!**************************************************************
 * cassie_dualsp_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param config_p - Current config.
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void cassie_dualsp_load_physical_config(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                   raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    raid_group_count = fbe_test_get_rg_count(rg_config_p);
    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                       &num_520_drives,
                                                       &num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
               __FUNCTION__, raid_type_string_p, num_520_drives, num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                        num_520_drives, num_4160_drives);
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives + 3,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    return;
}
/******************************************
 * end cassie_dualsp_load_physical_config()
 ******************************************/


/*!**************************************************************
 * cassie_dualsp_test_ndu_commit_callback_function()
 ****************************************************************
 * @brief
 *  callback function when ndu commit is finished.
 *
 * @param update_obj_msg - job commit/rollback sends this message.               
 * @param context - cassie_dualsp_test_ndu_commit_context we registered.        
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void  cassie_dualsp_test_ndu_commit_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context)
{
    cassie_dualsp_test_ndu_commit_context_t*  commit_context = (cassie_dualsp_test_ndu_commit_context_t*)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    if(FBE_JOB_TYPE_SEP_NDU_COMMIT != update_obj_msg->notification_info.notification_data.job_service_error_info.job_type)
        return;
    commit_context->job_srv_info = update_obj_msg->notification_info.notification_data.job_service_error_info;
    fbe_semaphore_release(&commit_context->sem, 0, 1, FALSE);
}

/*!**************************************************************
 * cassie_dualsp_get_and_verify_system_db_header()
 ****************************************************************
 * @brief
 *  get system db headers from both SPs
 *  verify they are equal to each other
 *
 * @param none.               
 *
 * @return out_sys_db_header - the read system db header.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static fbe_status_t    cassie_dualsp_get_and_verify_system_db_header(database_system_db_header_t* out_sys_db_header)
{
    fbe_status_t    status;
    database_system_db_header_t spa_db_header;
    database_system_db_header_t spb_db_header;
    fbe_sim_transport_connection_target_t original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    /************read system db header from SPA************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    status = fbe_api_database_get_system_db_header(&spa_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPA's system db header failed~~~");

    /************read system db header from SPB************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
    status = fbe_api_database_get_system_db_header(&spb_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPB's system db header failed~~~");

    fbe_api_sim_transport_set_target_server(original_target);

    /************compare system db headers read from two sides************/    
    if(spa_db_header.magic_number != spb_db_header.magic_number
       || spa_db_header.persisted_sep_version != spb_db_header.persisted_sep_version
       || spa_db_header.version_header.size != spb_db_header.version_header.size)
       return FBE_STATUS_GENERIC_FAILURE;

    fbe_copy_memory(out_sys_db_header, &spa_db_header, sizeof(database_system_db_header_t));

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * cassie_dualsp_print_system_db_header()
 ****************************************************************
 * @brief
 *  print a system db header
 *
 * @param sys_db_header - the system db header we want to print.          
 *
 * @return none.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void    cassie_dualsp_print_system_db_header(database_system_db_header_t* sys_db_header)
{
    mut_printf(MUT_LOG_TEST_STATUS, "===print system db header:===");
    mut_printf(MUT_LOG_TEST_STATUS, 
                    "mg_num: 0x%llx version_header_size:%d persisted_sep_version:0x%llx",
                    sys_db_header->magic_number, sys_db_header->version_header.size,
                    sys_db_header->persisted_sep_version);
}


/*!**************************************************************
 * cassie_dualsp_entry_value()
 ****************************************************************
 * @brief
 *     Check if the version_header is initialized with correct value 
 *     in object entry/ user entry /edge entry
 *     This function must be invoked after after the first reboot in simulation test.
 *     
 *
 * @param none.               
 *
 * @author
 *  05/07/2012 - Created. Gaohp
 *
 ****************************************************************/
static fbe_status_t  cassie_dualsp_check_entry_value(void)
{
    fbe_status_t    status;
    fbe_database_control_versioning_op_code_t  versioning_code = FBE_DATABASE_CONTROL_CHECK_VERSION_HEADER_IN_ENTRY;

    /************check entry value from SPA************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    /*Database service will check the first 50 entries in every entry type by this op_code */
    status = fbe_api_database_versioning_operation(&versioning_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~check entry value in SPA failed~~~");

    /************check entry value from SPB************/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
    /*Database service will check the first 50 entries in every entry type by this op_code */
    status = fbe_api_database_versioning_operation(&versioning_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPB's system db header failed~~~");

    return status;

}

/*!**************************************************************
 * cassie_dualsp_step1_create_topology()
 ****************************************************************
 * @brief
 *  create initial user raid groups and user luns.
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void cassie_dualsp_create_topology(void)
{
    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(&cassie_dualsp_raid_groups[0],
                                                      CASSIE_DUALSP_TEST_MAX_RAID_GROUP_COUNT,
                                                      CASSIE_DUALSP_TEST_CHUNKS_PER_LUN, 
                                                      CASSIE_DUALSP_TEST_LUNS_PER_RAID_GROUP);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(&cassie_dualsp_raid_groups[0], CASSIE_DUALSP_TEST_MAX_RAID_GROUP_COUNT);
}

static void cassie_dualsp_destroy_topology(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * cassie_dualsp_step2_verify_after_first_boot()
 ****************************************************************
 * @brief
 *  get system db headers from both SPs.
 *  verify they are equal to each other
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *  05/07/2012 - modified. Hongpo Gao
 *
 ****************************************************************/
static void cassie_dualsp_step2_verify_after_first_boot(void)
{
    fbe_status_t    status;
    database_system_db_header_t  system_db_header;
    
    status = cassie_dualsp_get_and_verify_system_db_header(&system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~system db header on the two SPs are not equal.~~~\n");

    cassie_dualsp_print_system_db_header(&system_db_header);

    status = cassie_dualsp_check_entry_value();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~check entry value failed.~~~\n");

}

/*!**************************************************************
 * cassie_dualsp_step3_verify_after_modify()
 ****************************************************************
 * @brief
 * - change the system db header on active side to a newer version
 * - reboot SPB
 * - get system db headers from both SPs
 * - check whether they are equal to each other
 * - check the version is the new one
 * - reboot SPA
 * - get system db headers from both SPs
 * - check whether they are equal to each other
 * - check the version is the new one
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void cassie_dualsp_step3_verify_after_modify(void)
{
    fbe_status_t    status;
    database_system_db_header_t  system_db_header;
    database_system_db_header_t  updated_system_db_header;

    mut_printf(MUT_LOG_TEST_STATUS, "===first get system db headers from both SPs and verify===");
    status = cassie_dualsp_get_and_verify_system_db_header(&system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~system db header on the two SPs are not equal.~~~\n");

    /*set system db header to new version in SPA*/
    mut_printf(MUT_LOG_TEST_STATUS, "===second set system db headers in SPA to a new version===");
    updated_system_db_header = system_db_header;
    updated_system_db_header.persisted_sep_version = CASSIE_DUALSP_TEST_NEW_SEP_VERSION;
    updated_system_db_header.version_header.size = system_db_header.version_header.size;
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_system_db_header(&updated_system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~set system db header to new version failed~~~\n");

    /*reboot SPB*/
    mut_printf(MUT_LOG_TEST_STATUS, "===then reboot SPB===");
    cassie_dualsp_reboot_system(FBE_SIM_SP_B);

    /*read and verify system db headers on both SPs*/
    mut_printf(MUT_LOG_TEST_STATUS, "===third get system db headers and verify after reboot SPB===");
    fbe_zero_memory(&system_db_header, sizeof(database_system_db_header_t));
    status = cassie_dualsp_get_and_verify_system_db_header(&system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~system db header on the two SPs are not equal.~~~\n");
    cassie_dualsp_print_system_db_header(&system_db_header);

    /*verify the system db header version is the new one*/
    MUT_ASSERT_UINT64_EQUAL_MSG(CASSIE_DUALSP_TEST_NEW_SEP_VERSION, 
                                                    system_db_header.persisted_sep_version, 
                                                    "~~~system db header version is not the new one. So strange!~~~\n");

    /*reboot SPA*/
    mut_printf(MUT_LOG_TEST_STATUS, "===now we reboot SPA===");
    cassie_dualsp_reboot_system(FBE_SIM_SP_A);

    /*read and verify system db headers on both SPs*/
    mut_printf(MUT_LOG_TEST_STATUS, "===At last get system db headers and verify after reboot SPA===");
    fbe_zero_memory(&system_db_header, sizeof(database_system_db_header_t));
    status = cassie_dualsp_get_and_verify_system_db_header(&system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~system db header on the two SPs are not equal.~~~\n");
    cassie_dualsp_print_system_db_header(&system_db_header);

    /*verify the system db header version is the new one*/
    MUT_ASSERT_UINT64_EQUAL_MSG(CASSIE_DUALSP_TEST_NEW_SEP_VERSION, 
                                                    system_db_header.persisted_sep_version, 
                                                    "~~~system db header version is not the new one. So strange!~~~\n");
    
}

/*!**************************************************************
 * cassie_dualsp_step4_verify_ndu_commit()
 ****************************************************************
 * @brief
 * - send NDU commit request to database service of SPA (it is passive side now)
 * - wait the notification of NDU commit job
 * - get system db headers from both SPs
 * - check they are equal to each other
 * - check the version is the original one
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void cassie_dualsp_step4_verify_ndu_commit(void)
{
    fbe_status_t    status;
    database_system_db_header_t  system_db_header;
    cassie_dualsp_test_ndu_commit_context_t commit_context;
    fbe_notification_registration_id_t  reg_id;

    fbe_semaphore_init(&commit_context.sem, 0, 1);

    /*register notification about ndu commit job*/
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
        FBE_PACKAGE_NOTIFICATION_ID_SEP_0, FBE_TOPOLOGY_OBJECT_TYPE_ALL, 
        cassie_dualsp_test_ndu_commit_callback_function, &commit_context, &reg_id);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*send ndu commit to database service*/
    status = fbe_api_database_commit();
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "~~~commit system db header failed~~~");

    /*wait notification from ndu commit job*/
    status = fbe_semaphore_wait_ms(&commit_context.sem, 30000);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,  "~~~wait ndu commit job failed~~~");
    status = fbe_api_notification_interface_unregister_notification(cassie_dualsp_test_ndu_commit_callback_function ,
             reg_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~unregister notification failed~~~");
    fbe_semaphore_destroy(&commit_context.sem);

    status = cassie_dualsp_get_and_verify_system_db_header(&system_db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~system db header on the two SPs are not equal.~~~\n");

    /*verify the system db header version is not the new one*/
    MUT_ASSERT_UINT64_NOT_EQUAL(CASSIE_DUALSP_TEST_NEW_SEP_VERSION, 
                                                    system_db_header.persisted_sep_version);    
}


/*!**************************************************************
 * cassie_dualsp_reboot_system()
 ****************************************************************
 * @brief
 *  reboot the target SP.
 *
 * @param target - target SP that we want to reboot.               
 *
 * @return None.
 *
 * @author
 *  04/19/2012 - Created. zphu
 *
 ****************************************************************/
static void cassie_dualsp_reboot_system(fbe_sim_transport_connection_target_t target)
{
    fbe_sim_transport_connection_target_t   original_target;
    original_target = fbe_api_sim_transport_get_target_server();
    
    /*reboot sep*/
    fbe_api_sim_transport_set_target_server(target);
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();

    fbe_api_sim_transport_set_target_server(original_target);
}

static void cassie_dualsp_decrease_system_db_header(fbe_sim_transport_connection_target_t target,
                                                    database_system_db_header_t *p_db_header)
{
    fbe_status_t status;
    database_system_db_header_t db_header;

    fbe_api_sim_transport_set_target_server(target);
    mut_printf(MUT_LOG_TEST_STATUS, "---read system db header\n");
    status = fbe_api_database_get_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPA's system db header failed~~~");

    /* Before change the db_header, copy it */
    *p_db_header = db_header;
    
    db_header.persisted_sep_version--;
    db_header.rg_user_entry_size -= CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE;

    mut_printf(MUT_LOG_TEST_STATUS, "---modify and write system db header\n");
    status = fbe_api_database_set_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~set SPA's system db header failed~~~");

}

static void cassie_dualsp_increase_system_db_header(fbe_sim_transport_connection_target_t target,
                                                    database_system_db_header_t *p_db_header)
{
    fbe_status_t status;
    database_system_db_header_t db_header;

    fbe_api_sim_transport_set_target_server(target);
    mut_printf(MUT_LOG_TEST_STATUS, "---read system db header\n");
    status = fbe_api_database_get_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~get SPA's system db header failed~~~");

    /* Before change the db_header, copy it */
    *p_db_header = db_header;
    
    /* increate the version number twice*/
    db_header.persisted_sep_version++;
    db_header.persisted_sep_version++;
    db_header.rg_user_entry_size += CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE;
    db_header.rg_user_entry_size += CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE;

    mut_printf(MUT_LOG_TEST_STATUS, "---modify and write system db header\n");
    status = fbe_api_database_set_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~set SPA's system db header failed~~~");

}

/*!**************************************************************
 * cassie_dualsp_simulation_setup()
 ****************************************************************
 * @brief
 *  Setup the test configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  04/19/2012 - Created
 *
 ****************************************************************/
void cassie_dualsp_setup()
{
    fbe_sim_transport_connection_target_t   sp;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for cassie dualsp test ===\n");

    fbe_test_sep_util_init_rg_configuration_array(&cassie_dualsp_raid_groups[0]);
    
    /* set the target server */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
    /* make sure target set correctly */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
    /* Load the physical configuration */
    mut_printf(MUT_LOG_TEST_STATUS, "=== load pp for SPB ===\n");
    cassie_dualsp_load_physical_config(&cassie_dualsp_raid_groups[0]);
    
    /* set the target server */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    /* make sure target set correctly */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== load pp for SPA ===\n");
    cassie_dualsp_load_physical_config(&cassie_dualsp_raid_groups[0]);

    /* Load SEP and NEIT*/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load sep and neit packages */
    mut_printf(MUT_LOG_TEST_STATUS, "=== load SEP and NEIT in SPA ===\n");
    sep_config_load_sep_and_neit_both_sps();

    return;
}
/**************************************
 * end cassie_dualsp_setup()
 **************************************/
 
/*!**************************************************************
 * cassie_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  cleanup the test configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  04/19/2012 - Created
 *
 ****************************************************************/
void cassie_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== clean up dualsp test ===\n");

    /* Clear the dual sp mode*/
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    return;
}


/*!************************************************************** 
 * cassie_dualsp_test_case1()
 ****************************************************************
 * @brief
 *    test the system db header
 ****************************************************************/
void cassie_dualsp_test_case1(void)
{
    /* step 1: Setup - Bring up initial topology.
     * - Create user rgs and luns*/
    mut_printf(MUT_LOG_TEST_STATUS, "------Test Case 1 \n");
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~ step1: bring up initial topology ~~~~\n");
    cassie_dualsp_create_topology();

    /* step 2: Get system db headers from both SPs.
     * - verify they are equal to each other
     * - check the object/user/edge entry version value after the first reboot
     */
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~ step2: verify system db headers after first boot~~~~\n");
    cassie_dualsp_step2_verify_after_first_boot();

    /* step 3: Change system db header on active side and verify
     * - change the system db header on active side to a newer version
     * - reboot SPB
     * - get system db headers from both SPs
     * - check whether they are equal to each other
     * - check the version is the new one
     * - reboot SPA
     * - get system db headers from both SPs
     * - check whether they are equal to each other
     * - check the version is the new one*/
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~ step3: verify system db headers after modifying it~~~~\n");
    cassie_dualsp_step3_verify_after_modify();
    
    /* step 4: Commit NDU
     * - send NDU commit request to database service of SPA (it is passive side now)
     * - wait the notification of NDU commit job
     * - get system db headers from both SPs
     * - check they are equal to each other
     * - check the version is the original one*/    
    mut_printf(MUT_LOG_TEST_STATUS, "~~~~ step4: verify ndu commit job~~~~\n");
    cassie_dualsp_step4_verify_ndu_commit();
    cassie_dualsp_destroy_topology();
    return;
}

/*!************************************************************** 
 * cassie_dualsp_test_case2()
 ****************************************************************
 * @brief
 *    test the unknown db cmi message type handling
 ****************************************************************/
void cassie_dualsp_test_case2(void)
{
    fbe_database_control_versioning_op_code_t  versioning_code = FBE_DATABASE_CONTROL_CHECK_UNKNOWN_CMI_MSG_HANDLING;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "------Test Case 2 \n");
    /* Send command to SPA, let SPA send an unknown cmi message to SPB */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    status = fbe_api_database_versioning_operation(&versioning_code);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "~~~test unknown db cmi msg type handling in SPA failed~~~");
}

/*!************************************************************** 
 * cassie_dualsp_test_case3()
 ****************************************************************
 * @brief
 *   modify the persist db version by FBE_API. let the persist_db_version
 *   in system db header is smaller than the SEP_PACKAGE_VERSION.
 *   As a result, the SP is in upgrade and not commit.
 *   Operations:
 *   create a raid group.
 *   1. check the raid group's object entry, user entry's size on both SP.
 *   2. reboot the SP
 *      check the raid group's object entry, user entry's size on both SP.
 * 
 * 
 * @version 
 *  created by gaohp (5/17/2012)
 ****************************************************************/
void cassie_dualsp_test_case3(void)
{
    database_system_db_header_t spa_db_header;
    database_system_db_header_t spb_db_header;
    fbe_object_id_t object_id;
    fbe_database_get_tables_t   get_tables;
    fbe_sim_transport_connection_target_t active_target;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "------Test Case 3 \n");
    mut_printf(MUT_LOG_TEST_STATUS, "------step1: decrease the persist db version on both SP \n");
    cassie_dualsp_decrease_system_db_header(FBE_SIM_SP_A, &spa_db_header);
    cassie_dualsp_decrease_system_db_header(FBE_SIM_SP_B, &spb_db_header);
    
    mut_printf(MUT_LOG_TEST_STATUS, "------step2: create a raid group\n");

    fbe_test_sep_get_active_target_id(&active_target);
    fbe_api_sim_transport_set_target_server(active_target);
    cassie_dualsp_create_topology();

    mut_printf(MUT_LOG_TEST_STATUS, "------step3: check the user entry size in the database table in active SP\n");
    status = fbe_api_database_lookup_raid_group_by_number(cassie_dualsp_raid_groups[0].raid_group_id, &object_id);

    status = fbe_api_database_get_tables(object_id, &get_tables);

    mut_printf(MUT_LOG_TEST_STATUS, "the user_entry's size is 0x%x, the original entry's size is 0x%x\n", get_tables.user_entry.header.version_header.size, spa_db_header.rg_user_entry_size );
    MUT_ASSERT_INTEGER_EQUAL(get_tables.user_entry.header.version_header.size + CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE, spa_db_header.rg_user_entry_size);

    /* Set the passive target*/
    if(active_target == FBE_SIM_SP_A) {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    } else {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "------step4: check the user entry size in the database table in passive SP\n");
    status = fbe_api_database_lookup_raid_group_by_number(cassie_dualsp_raid_groups[0].raid_group_id, &object_id);

    status = fbe_api_database_get_tables(object_id, &get_tables);

    mut_printf(MUT_LOG_TEST_STATUS, "the user_entry's size is 0x%x, the original entry's size is 0x%x\n", get_tables.user_entry.header.version_header.size, spa_db_header.rg_user_entry_size );
    MUT_ASSERT_INTEGER_EQUAL(get_tables.user_entry.header.version_header.size + CASSIE_DECREASE_BYTES_OF_ENTRY_SIZE, spa_db_header.rg_user_entry_size);

	mut_printf(MUT_LOG_TEST_STATUS, "------step5: destroy the topology \n");
    cassie_dualsp_destroy_topology();

}

#if 0
/*!************************************************************** 
 * cassie_dualsp_test_case4()
 ****************************************************************
 * @brief
 *   modify the persist db version by FBE_API. let the persist_db_version
 *   in system db header is larger than the SEP_PACKAGE_VERSION.
 *   then,
 *   1. create raid group on SPA
 *   2. check the raid group's object_entry/user_entry size
 *   3. destroy raid group on SPB
 * 
 * 
 * @version 
 *  created by gaohp (5/22/2012)
 ****************************************************************/
void cassie_dualsp_test_case4(void)
{
    fbe_database_control_system_db_header_t spa_db_header;
    fbe_object_id_t object_id;
    fbe_database_get_tables_t   get_tables;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "------Test Case 4 \n");
    mut_printf(MUT_LOG_TEST_STATUS, "------step1: increase the persist db version on SPA \n");
    cassie_dualsp_reboot_system(FBE_SIM_SP_B);
    cassie_dualsp_increase_system_db_header(FBE_SIM_SP_A, &spa_db_header);
    
    mut_printf(MUT_LOG_TEST_STATUS, "------step2: create a raid group\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    cassie_dualsp_create_topology();

    mut_printf(MUT_LOG_TEST_STATUS, "------step3: check the user entry size in the database table on SPB\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_lookup_raid_group_by_number(cassie_dualsp_raid_groups[0].raid_group_id, &object_id);

    status = fbe_api_database_get_tables(object_id, &get_tables);

    mut_printf(MUT_LOG_TEST_STATUS, "the user_entry's size is 0x%x, the original entry's size is 0x%x\n", get_tables.user_entry.header.version_header.size, spa_db_header.rg_user_entry_size );
    MUT_ASSERT_INTEGER_EQUAL(get_tables.user_entry.header.version_header.size, spa_db_header.rg_user_entry_size);

    mut_printf(MUT_LOG_TEST_STATUS, "------step4: destroy the topology \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    cassie_dualsp_destroy_topology();

}
#endif

/*!**************************************************************
 * cassie_dualsp_test()
 ****************************************************************
 * @brief
 *  dualsp test entry.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  04/19/2012 - Created
 *
 ****************************************************************/
void  cassie_dualsp_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    cassie_dualsp_test_case1();
    cassie_dualsp_test_case2();
    cassie_dualsp_test_case3();

   // cassie_dualsp_test_case4();
    return;
}
/*************************
 * end file cassie_dualsp_test.c
 *************************/


