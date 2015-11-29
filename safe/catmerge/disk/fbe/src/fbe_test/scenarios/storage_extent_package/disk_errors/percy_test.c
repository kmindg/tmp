
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file percy_test.c
 ***************************************************************************
 *
 * @brief
 *  Percy is Thomas the Tank Engine's friend.   In this adventure he will
 *  be testing the Drive Improved Error Handling (DIEH) feature.
 *
 * @version
 *   05/14/2010 - Created.  Wayne Garrett
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_stat_api.h"
#include "esp_tests.h"  
#include "pp_utils.h"  
#include "physical_package_tests.h"
#include "fbe_test_esp_utils.h"

#include "../../environment_service_package/fp_test.h"  //TODO: use SEP Test APIs to init esp
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"



/*******************************************************************************************/
char * percy_short_desc = "DIEH test";
char * percy_long_desc ="Tests and verifies the loading DIEH xml files";
/*******************************************************************************************/

static  fbe_test_params_t * pTest = NULL;


/*************************
 *   MACRO DEFINITIONS
 *************************/
#define PERCY_DIEH_CONFIG_GOOD_FILENAME           "percy_test_dieh1.xml"
#define PERCY_DIEH_CONFIG_MALFORMED_FILENAME      "percy_test_dieh2.xml"
#define PERCY_DIEH_CONFIG_EXCEPTION_LIST_FILENAME "percy_test_dieh3.xml"


typedef enum percy_dieh_bucket_e
{
    PERCY_DIEH_MEDIA,
    PERCY_DIEH_RECOVERED,
    PERCY_DIEH_HARDWARE,
    PERCY_DIEH_LINK,
    PERCY_DIEH_CUMULATIVE,
} percy_dieh_bucket_t;


typedef struct percy_scsi_error_s {
    fbe_scsi_sense_key_t sk;
    fbe_scsi_additional_sense_code_t asc;
    fbe_scsi_additional_sense_code_qualifier_t ascq;
} percy_scsi_error_t;

percy_scsi_error_t PERCY_MEDIA_ERROR = {0x03,0x11,0x00};
percy_scsi_error_t PERCY_RECOVERABLE_ERROR = {0x01,0x17,0x00};

/************************* 
  *  EXTERNS
  ************************/
extern fbe_status_t sleeping_beauty_remove_drive(fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot);
extern void fbe_test_sep_util_set_object_trace_flags(fbe_trace_type_t trace_type, 
                                              fbe_object_id_t object_id, 
                                              fbe_package_id_t package_id,
                                              fbe_trace_level_t level);
struct media_error_test_case_s;
extern void denali_create_scsi_cc_error_record(struct media_error_test_case_s *const case_p,
                                        fbe_dest_error_record_t* record_p,
                                        fbe_object_id_t pdo_object_id,
                                        fbe_scsi_command_t opcode,
                                        fbe_scsi_sense_key_t sk,
                                        fbe_scsi_additional_sense_code_t asc,
                                        fbe_scsi_additional_sense_code_qualifier_t ascq,
                                        fbe_port_request_status_t port_status);
extern void denali_add_scsi_command_to_record(fbe_dest_error_record_t* record_p,
                                       fbe_scsi_command_t opcode);
extern void denali_add_record_to_dest(fbe_dest_error_record_t* record_p, fbe_dest_record_handle_t * record_handle_p);
extern fbe_status_t denali_inject_scsi_error_stop(void);


/************************* 
 *    PROTOTYPES
 *************************/
static void         percy_test_missing_config(void);
static void         percy_test_good_config(void);
static void         percy_verify_good_config(void);
static void         percy_test_malformed_config(void);
static void         percy_test_default_config(void);
static void         percy_verify_default_config(void);
static void         percy_verify_tunable_parameters(void);
static void         percy_test_default_config_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, percy_dieh_bucket_t dieh_bucket, fbe_u32_t num_to_eol, fbe_u32_t num_to_fail);
static void         percy_verify_errors_do_not_kill(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, percy_dieh_bucket_t dieh_bucket, fbe_u32_t num_ios_to_send, fbe_bool_t avoid_burst_reduction);
static fbe_bool_t   percy_is_default_record(const fbe_drive_configuration_record_t *record_p);
static fbe_bool_t   percy_is_rcx_100g_record(const fbe_drive_configuration_record_t *record_p);
static fbe_bool_t   percy_is_rcx_200g_record(const fbe_drive_configuration_record_t *record_p);
static fbe_bool_t   percy_is_viper_c_record(const fbe_drive_configuration_record_t *record_p);
static void         verify_default_record(const fbe_drive_configuration_record_t *record_p);
static void         verify_rcx_record(const fbe_drive_configuration_record_t *record_p);

static void         percy_set_logical_crc_actions_in_registry(fbe_pdo_logical_error_action_t  action_bitmap);
static void         percy_test_crc_error(void);
static void         percy_test_verify_crc_error(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t expected_action_bitmap);
static void         percy_test_verify_kill_then_restore(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, 
                                                fbe_object_id_t object_id, fbe_pdo_logical_error_action_t expected_action_bitmap);
void                percy_test_insert_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void                percy_test_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static void         percy_test_dieh_load_config_file(fbe_drive_mgmt_dieh_load_config_t *config_info, fbe_dieh_load_status_t expected_status);
static void         percy_test_get_dieh_info(fbe_object_id_t object_id, fbe_physical_drive_dieh_stats_t *dieh_stats);
static void         percy_test_set_pdo_trace_level(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_trace_level_t level);
static void         percy_test_set_all_pdo_trace_level(fbe_trace_level_t level);
static void         percy_wait_for_dieh_config_to_update(void);
static void         percy_test_dieh_emeh(void);
static void         percy_test_dieh_exception_list(void);
static void         percy_verify_dieh_exception(fbe_object_id_t pdo, fbe_object_id_t pvd, fbe_drive_config_scsi_sense_code_t *scsi_error, fbe_payload_cdb_fis_error_flags_t action);
static void         percy_restore_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_bool_t   percy_is_pdo_registered_to_valid_handle(fbe_object_id_t pdo);
static void         percy_test_verify_default_config_04xx_exception(void);
static void         percy_test_driveconfig_override(void);
static void         percy_registry_write_driveconfig_override_file(fbe_u8_t * filepathname);
static void         percy_verify_override_config(void);


/*******************************************************************************************/

/*!**************************************************************
 * percy_setup() 
 ****************************************************************
 * @brief
 *  Setup function called by fbe_test framework for tests are run
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
void percy_setup(void)
{
#if 1
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_LAPA_RIOS_CONIG, //FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
#else

    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {       
        fbe_test_pp_util_create_physical_config_for_disk_counts(1 /*520*/,1 /*4k*/, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
        elmo_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();
#endif
}

/*!**************************************************************
 * percy_destroy() 
 ****************************************************************
 * @brief
 *  Teardown function called by fbe_test framework after test
 *  completes
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
void percy_destroy(void)
{
#if 1
    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
#else

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    //fbe_test_sep_util_restore_sector_trace_level();
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
#endif
}

/*!**************************************************************
 * percy_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
void percy_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;   
        
   /* Wait util there is no firmware upgrade in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

     /* Wait util there is no resume prom read in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "%s Start\n",__FUNCTION__);
                        

    /****** Run test cases *****/

    /* If you need to change path to xml file for testing then use the following code..    
    fbe_test_esp_create_registry_file(FBE_FALSE);
    fbe_test_esp_registry_write(ESP_REG_PATH,
                                FBE_DIEH_PATH_REG_KEY, 
                                FBE_REGISTRY_VALUE_SZ, 
                                "./",   // path to xml file.
                               0);
    */
    /* Verify default config already loaded */
    percy_wait_for_dieh_config_to_update();

    percy_test_verify_default_config_04xx_exception();

    percy_test_dieh_emeh();  /*extended media error handling test*/

    percy_test_dieh_exception_list();

    /* load and verify missing config */
    percy_test_missing_config();

    /* load and verify malformed config */
    percy_test_malformed_config();

    /* load and verify a good config */
    percy_test_good_config();
 
    percy_test_default_config();        

    /* Test drive config override (aka paco-r phase 1).*/
    percy_test_driveconfig_override();

    /* Test logical crc error handling */
    percy_test_crc_error();

    mut_printf(MUT_LOG_LOW, "%s passed...\n", __FUNCTION__);

    return;
}
/******************************************
 * end percy_test()
 ******************************************/


/*!**************************************************************
 * percy_test_missing_config() 
 ****************************************************************
 * @brief
 *  Test case where DMO attempts to load a configuration file that
 *  doesn't exist
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_missing_config(void)
{
    fbe_drive_mgmt_dieh_load_config_t config_info;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    strncpy(config_info.file, "foo.xml", FBE_DIEH_PATH_LEN-1);   // foo.xml does not exist.
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_FILE_READ_ERROR);

    percy_wait_for_dieh_config_to_update();
}

/*!**************************************************************
 * percy_test_good_config() 
 ****************************************************************
 * @brief
 *  Test case where DMO attempts to load a valid configuration file 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_good_config(void)
{
    fbe_drive_mgmt_dieh_load_config_t config_info;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    strncpy(config_info.file, PERCY_DIEH_CONFIG_GOOD_FILENAME, FBE_DIEH_PATH_LEN-1);
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);

    /* verify configuration  */
    percy_verify_good_config();

    percy_wait_for_dieh_config_to_update();    
}

/*!**************************************************************
 * percy_verify_good_config() 
 ****************************************************************
 * @brief
 *  Verifies a valid configuration file was loaded.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_verify_good_config(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t									i = 0;
    fbe_api_drive_config_get_handles_list_t     handles;
    fbe_drive_configuration_record_t            record;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get drive_config handles");
    MUT_ASSERT_INT_NOT_EQUAL_MSG(0,handles.total_count,"0 drive_configuration handles returned");

    for (i = 0; i<handles.total_count; i++ )
    {
        status = fbe_api_drive_configuration_interface_get_drive_table_entry(handles.handles_list[i], &record);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get_drive_table_entry");

        if (FBE_DRIVE_VENDOR_SAMSUNG == record.drive_info.drive_vendor)
        {
            /* For this test the loaded file will have 0x05 for Samsung cummulative control flag*/
            MUT_ASSERT_UINT64_EQUAL_MSG(0x05, record.threshold_info.io_stat.control_flag, "control_flag doesn't match");
            break;
        }
    }

}

/*!**************************************************************
 * percy_test_malformed_config() 
 ****************************************************************
 * @brief
 *  Test malformed configuration is detected and parser returns
 *  an error.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_malformed_config(void)
{
    fbe_drive_mgmt_dieh_load_config_t config_info;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    strncpy(config_info.file, PERCY_DIEH_CONFIG_MALFORMED_FILENAME, FBE_DIEH_PATH_LEN-1);
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_XML_ELEMENT_PARSE_ERROR);

    percy_wait_for_dieh_config_to_update();
}

/*!**************************************************************
 * percy_test_default_config() 
 ****************************************************************
 * @brief
 *  Test that the default configuration can be loaded if a file
 *  is not supplied.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_default_config(void)
{
    fbe_dieh_load_status_t status;
    fbe_drive_mgmt_dieh_load_config_t config_info;
    fbe_physical_drive_dieh_stats_t dieh_stats;
    fbe_object_id_t object_id;
    fbe_u32_t bus=0, encl=0, slot=4;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    config_info.file[0] = '\0';    /* empty filename string will load default */

    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);

    percy_verify_default_config();
    percy_verify_tunable_parameters();


    /* verify a drive has registered with a valid dieh handle */
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    percy_test_get_dieh_info(object_id, &dieh_stats);

    percy_wait_for_dieh_config_to_update();    

    /* test injecting errors */
    percy_test_default_config_actions(bus, encl, slot, PERCY_DIEH_MEDIA,     50,  100);
    percy_restore_drive(bus,encl,slot);

    percy_test_default_config_actions(bus, encl, slot, PERCY_DIEH_RECOVERED, 250, 300);
    percy_restore_drive(bus,encl,slot);
}


/*!**************************************************************
 * percy_verify_default_config() 
 ****************************************************************
 * @brief
 *  Verify the default configuration was loaded.  This should be used as a
 *  sanity test to make sure dieh ratios arn't accidently changed.
 *  When change are made to default DIEH behavior this test should
 *  be kept in synch.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/30/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_verify_default_config(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t									i = 0;
    fbe_api_drive_config_get_handles_list_t     handles;
    fbe_drive_configuration_record_t            record;
    fbe_bool_t is_default_found = FBE_FALSE;
    fbe_bool_t is_rcx_100g_found = FBE_FALSE;
    fbe_bool_t is_rcx_200g_found = FBE_FALSE;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get drive_config handles");
    MUT_ASSERT_INT_EQUAL_MSG(handles.total_count, 3, "default config has wrong number of records");


    for (i = 0; i<handles.total_count; i++ )
    {
        status = fbe_api_drive_configuration_interface_get_drive_table_entry(handles.handles_list[i], &record);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get_drive_table_entry");

        if (percy_is_default_record(&record)) 
        {
            is_default_found = FBE_TRUE;
            verify_default_record(&record);  
        }
        else if (percy_is_rcx_100g_record(&record))
        {
            is_rcx_100g_found = FBE_TRUE;
            verify_rcx_record(&record);
        }
        else if (percy_is_rcx_200g_record(&record))
        {
            is_rcx_200g_found = FBE_TRUE;
            verify_rcx_record(&record);
        }
    }

    MUT_ASSERT_INT_EQUAL_MSG(is_default_found, FBE_TRUE, "default rec not found");
    MUT_ASSERT_INT_EQUAL_MSG(is_rcx_100g_found, FBE_TRUE, "rcx 100g rec not found");
    MUT_ASSERT_INT_EQUAL_MSG(is_rcx_200g_found, FBE_TRUE, "rcx 200g rec not found");
}

/*!**************************************************************
 * percy_verify_tunable_parameters() 
 ****************************************************************
 * @brief
 *  Verify the default DriveConfiguration XML tunable parameters are 
 *  loaded.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/18/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void         percy_verify_tunable_parameters(void)
{
    fbe_dcs_tunable_params_t params={0};
    fbe_status_t status;
    status = fbe_api_drive_configuration_interface_get_params(&params);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL_MSG(params.service_time_limit_ms, 18000, "service time incorrect");
    MUT_ASSERT_UINT64_EQUAL_MSG(params.remap_service_time_limit_ms, 18000, "remap svc time incorrect");
    MUT_ASSERT_UINT64_EQUAL_MSG(params.emeh_service_time_limit_ms, 27000, "emeh svc time incorrect");
}


/*!**************************************************************
 * percy_verify_default_config_media_errors() 
 ****************************************************************
 * @brief
 *  Verify that the default configuration media bucket issues
 *  EOL and KILL actions at appropriate time.
 *
 * @param bus, enclosure, slot.
 *
 * @return None.
 *
 * @author
 *  5/14/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_default_config_actions(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, percy_dieh_bucket_t dieh_bucket, fbe_u32_t num_to_eol, fbe_u32_t num_to_fail)
{
    fbe_object_id_t pdo, pvd;
    fbe_dest_error_record_t record = {0};
    fbe_dest_error_record_t record_out = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t rdgen_context = {0};
    fbe_status_t status;
    fbe_u32_t times_to_insert = num_to_fail+10; 
    fbe_u32_t num_ios_to_send = num_to_fail+10;
    fbe_u32_t i;
    fbe_bool_t is_eol_found = FBE_FALSE;
    fbe_scsi_sense_key_t sk;
    fbe_scsi_additional_sense_code_t asc;
    fbe_scsi_additional_sense_code_qualifier_t ascq;
    fbe_u8_t *bucket_str = NULL;
    fbe_object_death_reason_t death_reason;
    fbe_object_death_reason_t expected_death_reason;    
    
    /* verify by injecting errors into a drive */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,encl,slot, &pdo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus,encl,slot, &pvd);        
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* Create DEST record */

    switch(dieh_bucket)
    {
        case PERCY_DIEH_MEDIA:
            sk = 0x03;
            asc = 0x11;
            ascq = 0x00;
            expected_death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED;
            bucket_str = "MEDIA";
            break;
        case PERCY_DIEH_RECOVERED:
            sk = 0x01;
            asc = 0x17;
            ascq = 0x00;
            expected_death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED;
            bucket_str = "RECOVERED";
            break;
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "%s bucket %d not implemented", __FUNCTION__, dieh_bucket);
            MUT_FAIL();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s %d_%d_%d %s eol:%d kill%d", __FUNCTION__, bus, encl, slot, bucket_str, num_to_eol, num_to_fail);

    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       sk, asc, ascq, 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    record.num_of_times_to_insert = times_to_insert;
    denali_add_record_to_dest(&record, &record_handle);


    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Test for EOL action */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Test EOL action", __FUNCTION__);
    for (i=0; i<num_ios_to_send; i++)
    {        
        fbe_api_get_block_edge_info_t edge_info;

        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pdo,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_PHYSICAL,          // package id
                                           FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x100,                            // start lba
                                           0x100,                            // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);               

        fbe_api_sleep(110);  /* avoid burst reduction */

        /* ride through reset */
        status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_READY, 10000,  /*msec*/ FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* check if we went into EOL */
        status = fbe_api_get_block_edge_info(pvd, 0, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE){
            is_eol_found = FBE_TRUE;
            break;
        }

    }

    MUT_ASSERT_INT_EQUAL_MSG(is_eol_found, FBE_TRUE, "Failure: EOL not found");

    /* verify EOL occurred at correct number of errors */
    fbe_api_dest_injection_get_record(&record_out, record_handle);
    if (record_out.times_inserted < num_to_eol-10 || record_out.times_inserted > num_to_eol+10)   /* check a range.  Avg is 28 with no fading and no successful IO*/
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s num errors injected to EOL (%d) not in range %d +-10", __FUNCTION__, record_out.times_inserted, num_to_eol);
        MUT_FAIL_MSG("Number of HW errors injected is not in expected range");
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s %s EOL initiated at error %d", __FUNCTION__, bucket_str, record_out.times_inserted);



    /* Test the fail action */

    while (record_out.times_inserted < times_to_insert)
    {
        fbe_lifecycle_state_t current_state;

        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pdo,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_PHYSICAL,          // package id
                                           FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x100,                            // start lba
                                           0x100,                            // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);               

        fbe_api_sleep(110);  /* avoid burst reduction */

        status = fbe_api_get_object_lifecycle_state(pdo, &current_state, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid status for drive.  status=%d", __FUNCTION__, status);
            break;
        }

        if (current_state == FBE_LIFECYCLE_STATE_ACTIVATE ||    /* ride through resets */
            current_state == FBE_LIFECYCLE_STATE_PENDING_ACTIVATE ||
            current_state == FBE_LIFECYCLE_STATE_PENDING_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Not Ready.  state=%d", __FUNCTION__, current_state);
            fbe_api_sleep(2000);
        }
        else if (current_state != FBE_LIFECYCLE_STATE_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Failed.  state=%d", __FUNCTION__, current_state);
            break;
        }

        fbe_api_dest_injection_get_record(&record_out, record_handle);
    }

    /* verify pdo goes to fail*/
    status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_FAIL, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify DEST injected an appropriate number of errors */
    fbe_api_dest_injection_get_record(&record_out, record_handle);
    if (record_out.times_inserted < num_to_fail-2 || record_out.times_inserted > num_to_fail+2)   /* check a range.*/
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s num errors injected to KILL (%d) not in range %d +-2", __FUNCTION__, record_out.times_inserted, num_to_fail);
        MUT_FAIL_MSG("Number of HW errors injected is not in expected range");
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s %s KILL initiated at error %d", __FUNCTION__, bucket_str, record_out.times_inserted);


    /* Verify death reason */
    status = fbe_api_get_object_death_reason(pdo, &death_reason, NULL, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(death_reason, expected_death_reason);


    /* cleanup
     */
    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
}

/*!**************************************************************
 * percy_verify_errors_do_not_kill() 
 ****************************************************************
 * @brief
 *  Verify that the default configuration media bucket issues
 *  EOL and KILL actions at appropriate time.
 *
 * @param bus, enclosure, slot.
 *
 * @return None.
 *
 * @author
 *  5/14/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_verify_errors_do_not_kill(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, percy_dieh_bucket_t dieh_bucket, fbe_u32_t num_errors_to_inject, fbe_bool_t avoid_burst_reduction)
{
    fbe_object_id_t pdo, pvd;
    fbe_dest_error_record_t record = {0};
    fbe_dest_error_record_t record_out = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t rdgen_context = {0};
    fbe_status_t status;    
    fbe_scsi_sense_key_t sk;
    fbe_scsi_additional_sense_code_t asc;
    fbe_scsi_additional_sense_code_qualifier_t ascq;
    fbe_u8_t *bucket_str = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "%s ...", __FUNCTION__);
    
    /* verify by injecting errors into a drive */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,encl,slot, &pdo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus,encl,slot, &pvd);        
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* Create DEST record */

    switch(dieh_bucket)
    {
        case PERCY_DIEH_MEDIA:
            sk = 0x03;
            asc = 0x11;
            ascq = 0x00;
            bucket_str = "MEDIA";
            break;
        case PERCY_DIEH_RECOVERED:
            sk = 0x01;
            asc = 0x17;
            ascq = 0x00;
            bucket_str = "RECOVERED";
            break;
        case PERCY_DIEH_HARDWARE:
            sk = 0x04;
            asc = 0x44;
            ascq = 0x00;
            bucket_str = "HARDWARE";
            break;
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "%s bucket %d not implemented", __FUNCTION__, dieh_bucket);
            MUT_FAIL();
    }

    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       sk, asc, ascq, 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    record.num_of_times_to_insert = num_errors_to_inject;
    denali_add_record_to_dest(&record, &record_handle);


    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);



    /* Test the fail action */

    while (record_out.times_inserted < num_errors_to_inject)
    {
        fbe_lifecycle_state_t current_state;
        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pdo,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_PHYSICAL,          // package id
                                           FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x100,                            // start lba
                                           0x100,                            // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);               

        if (avoid_burst_reduction)
        {
            fbe_api_sleep(110);  /* avoid burst reduction */
        }

        status = fbe_api_get_object_lifecycle_state(pdo, &current_state, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid status for drive.  status=%d", __FUNCTION__, status);
            break;
        }

        if (current_state == FBE_LIFECYCLE_STATE_ACTIVATE ||    /* ride through resets */
            current_state == FBE_LIFECYCLE_STATE_PENDING_ACTIVATE ||
            current_state == FBE_LIFECYCLE_STATE_PENDING_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Not Ready.  state=%d", __FUNCTION__, current_state);
            fbe_api_sleep(2000);
        }
        else if (current_state != FBE_LIFECYCLE_STATE_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Failed.  state=%d", __FUNCTION__, current_state);
            break;
        }

        fbe_api_dest_injection_get_record(&record_out, record_handle);

    }

    /* verify pdo doesn't  fail*/
    status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_READY, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify DEST injected an appropriate number of errors */
    fbe_api_dest_injection_get_record(&record_out, record_handle);
    if (record_out.times_inserted != num_errors_to_inject)   /* check a range.*/
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s num errors injected %d does not match %d", __FUNCTION__, record_out.times_inserted, num_errors_to_inject);
        MUT_FAIL_MSG("Number of errors injected is not in expected range");
    }    

    /* cleanup
     */
    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
}


/*!**************************************************************
 * percy_verify_errors_do_kill() 
 ****************************************************************
 * @brief
 *  Verify that the default configuration media bucket issues
 *  EOL and KILL actions at appropriate time.
 *
 * @param bus, enclosure, slot.
 *
 * @return None.
 *
 * @author
 *  5/14/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_verify_errors_do_kill(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, percy_dieh_bucket_t dieh_bucket, fbe_u32_t expected_num_errors, fbe_bool_t avoid_burst_reduction)
{
    fbe_object_id_t pdo, pvd;
    fbe_dest_error_record_t record = {0};
    fbe_dest_error_record_t record_out = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t rdgen_context = {0};
    fbe_status_t status;
    fbe_scsi_sense_key_t sk;
    fbe_scsi_additional_sense_code_t asc;
    fbe_scsi_additional_sense_code_qualifier_t ascq;
    fbe_u8_t *bucket_str = NULL;
    fbe_u32_t num_errors_to_inject = expected_num_errors+10;  /* increase to give some margin*/

    mut_printf(MUT_LOG_TEST_STATUS, "%s ...", __FUNCTION__);
    
    /* verify by injecting errors into a drive */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,encl,slot, &pdo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus,encl,slot, &pvd);        
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* Create DEST record */

    switch(dieh_bucket)
    {
        case PERCY_DIEH_MEDIA:
            sk = 0x03;
            asc = 0x11;
            ascq = 0x00;
            bucket_str = "MEDIA";
            break;
        case PERCY_DIEH_RECOVERED:
            sk = 0x01;
            asc = 0x17;
            ascq = 0x00;
            bucket_str = "RECOVERED";
            break;
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "%s bucket %d not implemented", __FUNCTION__, dieh_bucket);
            MUT_FAIL();
    }

    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       sk, asc, ascq, 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    record.num_of_times_to_insert = num_errors_to_inject;
    denali_add_record_to_dest(&record, &record_handle);


    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);



    /* Test the fail action */

    while (record_out.times_inserted < num_errors_to_inject)
    {
        fbe_lifecycle_state_t current_state;
        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pdo,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_PHYSICAL,          // package id
                                           FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x100,                            // start lba
                                           0x100,                            // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);               

        if (avoid_burst_reduction)
        {
            fbe_api_sleep(110);  /* avoid burst reduction */
        }

        status = fbe_api_get_object_lifecycle_state(pdo, &current_state, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid status for drive.  status=%d", __FUNCTION__, status);
            break;
        }

        if (current_state == FBE_LIFECYCLE_STATE_ACTIVATE ||    /* ride through resets */
            current_state == FBE_LIFECYCLE_STATE_PENDING_ACTIVATE ||
            current_state == FBE_LIFECYCLE_STATE_PENDING_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Not Ready.  state=%d", __FUNCTION__, current_state);
            fbe_api_sleep(2000);
        }
        else if (current_state != FBE_LIFECYCLE_STATE_READY)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Failed.  state=%d", __FUNCTION__, current_state);
            break;
        }

        fbe_api_dest_injection_get_record(&record_out, record_handle);

    }

    /* verify pdo failed*/
    status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_FAIL, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify DEST injected an appropriate number of errors */
    fbe_api_dest_injection_get_record(&record_out, record_handle);
    if (record_out.times_inserted < expected_num_errors-1 || record_out.times_inserted > expected_num_errors+1)   /* check a range.*/
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s errors expected:%d injected:%d not within +-1", __FUNCTION__, expected_num_errors, record_out.times_inserted);
        MUT_FAIL_MSG("Number of errors injected is not in expected range");
    }    

    /* cleanup
     */
    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
}


static fbe_bool_t percy_is_default_record(const fbe_drive_configuration_record_t *record_p)
{
    if (FBE_DRIVE_TYPE_SAS       == record_p->drive_info.drive_type   &&
        FBE_DRIVE_VENDOR_INVALID == record_p->drive_info.drive_vendor &&        
        strncmp(record_p->drive_info.part_number, "", 1) == 0         &&
        strncmp(record_p->drive_info.fw_revision, "", 1) == 0         &&
        strncmp(record_p->drive_info.serial_number_start, "", 1) == 0 &&
        strncmp(record_p->drive_info.serial_number_end, "", 1) == 0 )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

static fbe_bool_t percy_is_rcx_100g_record(const fbe_drive_configuration_record_t *record_p)
{
    if (FBE_DRIVE_TYPE_SAS       == record_p->drive_info.drive_type   &&
        FBE_DRIVE_VENDOR_SAMSUNG == record_p->drive_info.drive_vendor &&        
        strncmp(record_p->drive_info.part_number, "DG118032713", FBE_SCSI_INQUIRY_PART_NUMBER_SIZE) == 0   &&
        strncmp(record_p->drive_info.fw_revision, "", 1) == 0         &&
        strncmp(record_p->drive_info.serial_number_start, "", 1) == 0 &&
        strncmp(record_p->drive_info.serial_number_end, "", 1) == 0 )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

static fbe_bool_t percy_is_rcx_200g_record(const fbe_drive_configuration_record_t *record_p)
{
    if (FBE_DRIVE_TYPE_SAS       == record_p->drive_info.drive_type   &&
        FBE_DRIVE_VENDOR_SAMSUNG == record_p->drive_info.drive_vendor &&        
        strncmp(record_p->drive_info.part_number, "DG118032714", FBE_SCSI_INQUIRY_PART_NUMBER_SIZE) == 0   &&
        strncmp(record_p->drive_info.fw_revision, "", 1) == 0         &&
        strncmp(record_p->drive_info.serial_number_start, "", 1) == 0 &&
        strncmp(record_p->drive_info.serial_number_end, "", 1) == 0 )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

static fbe_bool_t percy_is_viper_c_record(const fbe_drive_configuration_record_t *record_p)
{
    if (FBE_DRIVE_TYPE_SAS       == record_p->drive_info.drive_type   &&
        FBE_DRIVE_VENDOR_HITACHI == record_p->drive_info.drive_vendor &&        
        strncmp(record_p->drive_info.part_number, "DG118032693", FBE_SCSI_INQUIRY_PART_NUMBER_SIZE) == 0   &&
        strncmp(record_p->drive_info.fw_revision, "", 1) == 0         &&
        strncmp(record_p->drive_info.serial_number_start, "", 1) == 0 &&
        strncmp(record_p->drive_info.serial_number_end, "", 1) == 0 )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}


static void verify_default_record(const fbe_drive_configuration_record_t *record_p)
{
    /* media bucket*/
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight, 18000, "media wait doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.num, 3, "media actions num doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].flag, FBE_STAT_ACTION_FLAG_FLAG_RESET, "media reset doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].ratio, 30, "media reset ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].reactivate_ratio, 5, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].flag, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, "media eol doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].ratio, 50, "media eol ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].reactivate_ratio, FBE_U32_MAX, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].flag, FBE_STAT_ACTION_FLAG_FLAG_FAIL, "media fail doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].ratio, 100, "media fail ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].reactivate_ratio, FBE_U32_MAX, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].type, FBE_STAT_WEIGHT_EXCEPTION_OPCODE, "media wt excp 0 type doesn't match"); 
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].u.opcode, 0x2e, "media wt excp 0 op doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].change, 20, "media wt excp 1 change doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].type, FBE_STAT_WEIGHT_EXCEPTION_OPCODE, "media wt excp 1 type doesn't match"); 
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].u.opcode, 0x8e, "media wt excp 1 op doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].change, 20, "media wt excp 1 change doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[2].type, FBE_STAT_WEIGHT_EXCEPTION_INVALID, "media wt excp 2 type doesn't match"); 
    /* Flags */
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.record_flags, FBE_STATS_FLAG_RELIABLY_CHALLENGED, "record flags doesn't match");
    /* exception list*/
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].type_and_action, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA, "cat excp 0 type doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.sense_key, 0x01, "cat excp 0 sk doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.asc_range_start, 0x0c, "cat excp 0 asc_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.asc_range_end, 0x0c, "cat excp 0 asc_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.ascq_range_start, 0x00, "cat excp 0 ascq_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.ascq_range_end, 0xff, "cat excp 0 ascq_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].type_and_action, (FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE|FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE), "cat excp 1 type doesn't match");
}

static void verify_rcx_record(const fbe_drive_configuration_record_t *record_p)
{
    /* media bucket */
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight, 36000, "media wait doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.num, 3, "media actions num doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].flag, FBE_STAT_ACTION_FLAG_FLAG_RESET, "media reset doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].ratio, 50, "media reset ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[0].reactivate_ratio, 10, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].flag, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, "media eol doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].ratio, 89, "media eol ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[1].reactivate_ratio, FBE_U32_MAX, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].flag, FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME, "media fail doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].ratio, 100, "media fail ratio doesn't match");
    MUT_ASSERT_INT_EQUAL_MSG(record_p->threshold_info.media_stat.actions.entry[2].reactivate_ratio, FBE_U32_MAX, "reactivation doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].type, FBE_STAT_WEIGHT_EXCEPTION_OPCODE, "media wt excp 0 type doesn't match"); 
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].u.opcode, 0x2e, "media wt excp 0 op doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[0].change, 20, "media wt excp 1 change doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].type, FBE_STAT_WEIGHT_EXCEPTION_OPCODE, "media wt excp 1 type doesn't match"); 
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].u.opcode, 0x8e, "media wt excp 1 op doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[1].change, 20, "media wt excp 1 change doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->threshold_info.media_stat.weight_exceptions[2].type, FBE_STAT_WEIGHT_EXCEPTION_INVALID, "media wt excp 2 type doesn't match"); 
    /*exception list*/
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].type_and_action, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA, "cat excp 0 type doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.sense_key, 0x01, "cat excp 0 sk doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.asc_range_start, 0x0c, "cat excp 0 asc_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.asc_range_end, 0x0c, "cat excp 0 asc_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.ascq_range_start, 0x00, "cat excp 0 ascq_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[0].scsi_fis_union.scsi_code.ascq_range_end, 0xff, "cat excp 0 ascq_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].type_and_action, FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL, "cat excp 1 type doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].scsi_fis_union.scsi_code.sense_key, 0x04, "cat excp 1 sk doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].scsi_fis_union.scsi_code.asc_range_start, 0x44, "cat excp 1 asc_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].scsi_fis_union.scsi_code.asc_range_end, 0x44, "cat excp 1 asc_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].scsi_fis_union.scsi_code.ascq_range_start, 0x00, "cat excp 1 ascq_range_start doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[1].scsi_fis_union.scsi_code.ascq_range_end, 0xff, "cat excp 1 ascq_range_end doesn't match");
    MUT_ASSERT_UINT64_EQUAL_MSG(record_p->category_exception_codes[2].type_and_action, (FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE|FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE), "cat excp 2 type doesn't match");
}

/*!**************************************************************
 * percy_set_logical_crc_actions_in_registry() 
 ****************************************************************
 * @brief
 *  Change the registry setting for the default CRC error action.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/15/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_set_logical_crc_actions_in_registry(fbe_pdo_logical_error_action_t  action_bitmap)
{   
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "%s action = 0x%x", __FUNCTION__, action_bitmap);

    status = fbe_test_esp_create_registry_file(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_esp_registry_write(ESP_REG_PATH,
                                FBE_LOGICAL_ERROR_ACTION_REG_KEY, 
                                FBE_REGISTRY_VALUE_DWORD, 
                                &action_bitmap,   
                                0);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}


/*!**************************************************************
 * percy_test_crc_error() 
 ****************************************************************
 * @brief
 *  Test setting various CRC actions and verifying the behaviour.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/15/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_crc_error(void)
{
    fbe_pdo_logical_error_action_t action_bitmap;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    /* test DMO's default CRC actions*/
    action_bitmap = FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC;
    percy_test_verify_crc_error(0, 0, 4, action_bitmap);  
        
    /* change the action bitmap and test various combinations*/
    action_bitmap = FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC;
    fbe_api_esp_drive_mgmt_set_crc_actions(&action_bitmap);
    percy_test_verify_crc_error(0, 0, 4, action_bitmap);  

    action_bitmap = FBE_PDO_ACTION_KILL_ON_OTHER_CRC;
    fbe_api_esp_drive_mgmt_set_crc_actions(&action_bitmap);
    percy_test_verify_crc_error(0, 0, 4, action_bitmap);

    action_bitmap = FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC | FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC | FBE_PDO_ACTION_KILL_ON_OTHER_CRC;
    fbe_api_esp_drive_mgmt_set_crc_actions(&action_bitmap);
    percy_test_verify_crc_error(0, 0, 4, action_bitmap);
}

/*!**************************************************************
 * percy_test_verify_crc_error() 
 ****************************************************************
 * @brief
 *  Verify the behaviour for the given action bitmap.
 *
 * @param bus, encl, slot   - drive location.
 * @param expected_action_bitmap  - actions to verify
 *
 * @return None.
 *
 * @author
 *  5/15/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_verify_crc_error(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_pdo_logical_error_action_t expected_action_bitmap)
{
    fbe_status_t status;
    fbe_physical_drive_dieh_stats_t dieh_stats;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t state;
    fbe_u32_t i;
    const fbe_u32_t one_sec = 1000;  //msec

    mut_printf(MUT_LOG_LOW, "%s verify action bitmap 0x%x", __FUNCTION__, expected_action_bitmap);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* 1.  verify drive has expected action set.  Note that if drive was just inserted it may take
       a bit before DMO pushes down the default actions.  So we'll periodically check the action until
       it gets set */
    for (i=0; i<15*one_sec; i+= one_sec)    // pool every second for 15 secs
    {    
       percy_test_get_dieh_info(object_id, &dieh_stats);

       if (expected_action_bitmap == dieh_stats.crc_action)
       {
           break;
       }
       mut_printf(MUT_LOG_LOW, "%s expected action 0x%x != action 0x%x. retrying", __FUNCTION__, expected_action_bitmap, dieh_stats.crc_action);
       fbe_api_sleep(one_sec);
    }
    
    MUT_ASSERT_INT_EQUAL(expected_action_bitmap, dieh_stats.crc_action);  // does drive have expected bitmap

    /* 2.  test that for all actions not set that they don't shoot the drive
    */
    if (! expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC)
    {        
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS);
    }
    if (! expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC)
    {        
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT);
    }
    if (! expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_OTHER_CRC)
    {        
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC);
    }
    /* The previous calls are synchronous, so PDO should have transitioned
       out of Ready State if action is taken.  Verify no action taken, that is state is still in Ready */
    status = fbe_api_get_object_lifecycle_state(object_id, &state, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, state); 


    /* 3.  Now verify for each action set that drive is shot
       */
    if (expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC)
    {
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS);
        percy_test_verify_kill_then_restore(bus, encl, slot, object_id, expected_action_bitmap);        
    }
    if (expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC)
    {
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT);
        percy_test_verify_kill_then_restore(bus, encl, slot, object_id, expected_action_bitmap);        
    }
    if (expected_action_bitmap & FBE_PDO_ACTION_KILL_ON_OTHER_CRC)
    {
        fbe_api_physical_drive_send_logical_error(object_id, FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC);
        percy_test_verify_kill_then_restore(bus, encl, slot, object_id, expected_action_bitmap);        
    }

}

/*!**************************************************************
 * percy_test_verify_kill_then_restore() 
 ****************************************************************
 * @brief
 *  Verify the drive is failed,  then restore it be removing and
 *  reinserting it.
 *
 * @param bus, encl, slot   - drive location.
 * @param expected_action_bitmap  - actions to verify
 *
 * @return None.
 *
 * @author
 *  5/15/2012 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_verify_kill_then_restore(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, 
                                                fbe_object_id_t object_id, fbe_pdo_logical_error_action_t expected_action_bitmap)
{
    fbe_status_t status;
    fbe_lifecycle_state_t state;

    /* verify not in ready*/
    status = fbe_api_get_object_lifecycle_state(object_id, &state, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_LIFECYCLE_STATE_READY, state);  

    /* verify goes to fail*/
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_FAIL, 30000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*restore drive and verify correct action is loaded*/
    percy_test_remove_drive(bus, encl, slot);    
    percy_test_insert_drive(bus, encl, slot);    

    /* wait for pdo & pvd to come up */
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /*msec*/);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
}

/*!**************************************************************
 * percy_test_insert_drive() 
 ****************************************************************
 * @brief
 *  Insert a drive.
 *
 * @param bus, encl, slot   - drive location.
 *
 * @return None.
 *
 * @author
 *  5/15/2012 - Created.  Wayne Garrett
 ****************************************************************/
void percy_test_insert_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_status_t                        status;


    drive_info.location.bus =       bus;
    drive_info.location.enclosure = encl;
    drive_info.location.slot =      slot;    
    
    status = fbe_api_terminator_get_enclosure_handle(bus,encl,&encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    sas_drive.backend_number = bus;
    sas_drive.encl_number = encl;
    sas_drive.capacity = 0x1ED065;
    sas_drive.block_size = 520;
    sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number + sas_drive.backend_number) << 16) + slot;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;

    mut_printf(MUT_LOG_LOW, "Inserting drive %d_%d_%d", sas_drive.backend_number, sas_drive.encl_number, slot);
    //status = fbe_api_terminator_insert_sas_drive (encl_handle, slot, &sas_drive, &drive_handle);
    status  = fbe_test_pp_util_insert_sas_drive(bus,
                                                encl,
                                                slot,
                                                520,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
}


/*!**************************************************************
 * percy_test_insert_drive() 
 ****************************************************************
 * @brief
 *  Remove a drive.
 *
 * @param bus, encl, slot   - drive location.
 *
 * @return None.
 *
 * @author
 *  3/26/2013 - Created.  Wayne Garrett
 ****************************************************************/
void percy_test_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "Removing drive %d_%d_%d", bus, encl, slot);

    status = sleeping_beauty_remove_drive(bus, encl, slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);   
}

/*!**************************************************************
 * percy_test_dieh_load_config_file() 
 ****************************************************************
 * @brief
 *  Load DIEH config.  Retry a few times if it fails since
 *  PDO may not have unregistered from previous test config.
 *
 * @param config_info.  DIEH config file info.
 *
 * @return None.
 *
 * @author
 *  10/09/2012 Wayne Garrett - Created
 ****************************************************************/
static void percy_test_dieh_load_config_file(fbe_drive_mgmt_dieh_load_config_t *config_info, fbe_dieh_load_status_t expected_status)
{
    const fbe_u32_t SLEEP_INTERVAL_MSEC = 500;
    const fbe_u32_t MAX_SLEEP_MSEC = 10000;
    fbe_u32_t sleep_total = 0;
    fbe_dieh_load_status_t dieh_status;
        
    dieh_status = fbe_api_esp_drive_mgmt_dieh_load_config_file(config_info);

    /* we may still be processing the previous test's load.  Try a few times.*/
    sleep_total = 0;
    while (FBE_DMO_THRSH_UPDATE_ALREADY_IN_PROGRESS == dieh_status &&
           sleep_total < MAX_SLEEP_MSEC)
    {
        mut_printf(MUT_LOG_LOW, "%s update in progress. waiting...", __FUNCTION__); 
        fbe_api_sleep(SLEEP_INTERVAL_MSEC);
        sleep_total += SLEEP_INTERVAL_MSEC;

        dieh_status = fbe_api_esp_drive_mgmt_dieh_load_config_file(config_info);
    }

    MUT_ASSERT_INT_EQUAL_MSG(dieh_status, expected_status, "dieh load config failed");
}

/*!**************************************************************
 * percy_test_get_dieh_info() 
 ****************************************************************
 * @brief
 *  Gets DIEH stats.  Retry a few times if it fails since
 *  PDO may be in the middle of re-registering for new config.
 *
 * @param object_id.  PDO object ID
 * @param dieh_stats. The DIEH stats to be returned.
 *
 * @return None.
 *
 * @author
 *  10/09/2012 Wayne Garrett - Created
 ****************************************************************/
static void percy_test_get_dieh_info(fbe_object_id_t object_id, fbe_physical_drive_dieh_stats_t *dieh_stats)
{
    const fbe_u32_t SLEEP_INTERVAL_MSEC = 500;
    const fbe_u32_t MAX_SLEEP_MSEC = 10000;
    fbe_u32_t sleep_total = 0;
    fbe_status_t status;

    status = fbe_api_physical_drive_get_dieh_info(object_id, dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* PDO might be in middle of re-registering.   Try a few times. */
    sleep_total = 0;
    while (FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE == dieh_stats->drive_configuration_handle &&
           sleep_total < MAX_SLEEP_MSEC)
    {
        mut_printf(MUT_LOG_LOW, "%s drive in middle of re-register. waiting...", __FUNCTION__); 
        fbe_api_sleep(SLEEP_INTERVAL_MSEC);
        sleep_total += SLEEP_INTERVAL_MSEC;

        status = fbe_api_physical_drive_get_dieh_info(object_id, dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE, dieh_stats->drive_configuration_handle, "get_dieh_info failed. invalid handle");
}

/*!**************************************************************
 * percy_test_set_pdo_trace_level() 
 ****************************************************************
 * @brief
 *  Change tracing level for a specific PDO instance.
 *
 * @param bus, encl, slot.  physical location of PDO.
 * @param level. Tracing level
 *
 * @return None.
 *
 * @author
 *  10/09/2012 Wayne Garrett - Created
 ****************************************************************/
static void percy_test_set_pdo_trace_level(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_trace_level_t level)
{
    fbe_object_id_t object_id;
    fbe_status_t status;

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_id);
    if (object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_OBJECT, object_id, FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL, level);
    }
        
}

/*!**************************************************************
 * percy_test_set_all_pdo_trace_level() 
 ****************************************************************
 * @brief
 *  Change all PDO's tracing levels
 *
 * @param level. Tracing level
 *
 * @return None.
 *
 * @author
 *  10/09/2012 Wayne Garrett - Created
 ****************************************************************/
static void percy_test_set_all_pdo_trace_level(fbe_trace_level_t level)
{
    fbe_u32_t p,e,s;

    for(p=0; p < pTest->max_ports; p++)
    {
        for(e=0; e < pTest->max_enclosures; e++)
        {
            for(s=0; s < pTest->max_drives; s++)
            {
                percy_test_set_pdo_trace_level(p, e, s, level);
            }
        }
    }
}

/*!**************************************************************
 * percy_wait_for_dieh_config_to_update() 
 ****************************************************************
 * @brief
 *  Verify dieh table update is not in progress.  If it's currently
 *  in progress we will wait a bit before failing.
 *
 * @param level. Tracing level
 *
 * @return None.
 *
 * @author
 *  10/09/2012 Wayne Garrett - Created
 ****************************************************************/
static void percy_wait_for_dieh_config_to_update(void)
{
    fbe_bool_t is_updating = FBE_FALSE;
    fbe_status_t status;
    const fbe_u32_t SLEEP_INTERVAL_MSEC = 500;
    const fbe_u32_t MAX_SLEEP_MSEC = 10000;
    fbe_u32_t sleep_total = 0;

    do
    {
        status = fbe_api_drive_configuration_interface_dieh_get_status(&is_updating);    
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (!is_updating)
        {
            break;
        }

        mut_printf(MUT_LOG_LOW, "%s dcs is updating dieh table. waiting...", __FUNCTION__); 
        fbe_api_sleep(SLEEP_INTERVAL_MSEC);
        sleep_total += SLEEP_INTERVAL_MSEC;

        MUT_ASSERT_INT_EQUAL_MSG( (sleep_total < MAX_SLEEP_MSEC), FBE_TRUE, "Timeout wating for table update to complete");
        
    } while (1);
}

void percy_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_verify(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

void percy_enable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_enable_verify(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

void percy_inject_one_error(fbe_object_id_t pdo, percy_scsi_error_t *error)
{
    fbe_dest_error_record_t record = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_status_t status;
    fbe_physical_drive_dieh_stats_t dieh_stats = {0};
    fbe_physical_drive_dieh_stats_t prev_dieh_stats = {0};
    fbe_api_rdgen_context_t rdgen_context = {0};

    /*TODO: test each kind of media error. i.e 01/17/00 */

    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       error->sk, error->asc, error->ascq, // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    denali_add_record_to_dest(&record, &record_handle);

    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    fbe_api_physical_drive_get_dieh_info(pdo, &prev_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);

    /* Send I/O.  We expect this to get an  error because of the error injected
     * above. 
     */  
    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x100,                            // start lba
                                       0x100,                            // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);     

    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  
    fbe_api_dest_injection_remove_all_records();

    fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);

    mut_printf(MUT_LOG_LOW, "io_count:0x%llx->0x%llx, media_et:0x%llx->0x%llx", 
               prev_dieh_stats.error_counts.drive_error.io_counter, dieh_stats.error_counts.drive_error.io_counter,
               prev_dieh_stats.error_counts.drive_error.media_error_tag, dieh_stats.error_counts.drive_error.media_error_tag);

}

static void percy_test_dieh_emeh()
{
    const fbe_u32_t bus  = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 4;
    fbe_object_id_t pdo, pvd;
    fbe_physical_drive_set_dieh_media_threshold_t threshold = {0};
    fbe_physical_drive_dieh_stats_t dieh_stats = {0};
    fbe_physical_drive_dieh_stats_t prev_dieh_stats = {0};
    fbe_status_t status;
    fbe_api_get_block_edge_info_t edge_info = {0};


    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* msec*/);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo != FBE_OBJECT_ID_INVALID);

    /* disable background services, so we can control DIEH fading*/
    percy_disable_background_services(bus,encl,slot);




    fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(dieh_stats.dieh_state.media_weight_adjust, FBE_STAT_WEIGHT_CHANGE_NONE);

    prev_dieh_stats = dieh_stats;

    /* prime the pump to get passed intial ET calculation */
    percy_inject_one_error(pdo, &PERCY_MEDIA_ERROR);
    fbe_api_sleep(110);
    percy_inject_one_error(pdo, &PERCY_MEDIA_ERROR);
    fbe_api_sleep(110);
    percy_inject_one_error(pdo, &PERCY_RECOVERABLE_ERROR);
    fbe_api_sleep(110);
    percy_inject_one_error(pdo, &PERCY_RECOVERABLE_ERROR);
    fbe_api_sleep(110);

    fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);

    mut_printf(MUT_LOG_LOW, "io_count:0x%llx->0x%llx, media_et:0x%llx->0x%llx", 
               prev_dieh_stats.error_counts.drive_error.io_counter, dieh_stats.error_counts.drive_error.io_counter,
               prev_dieh_stats.error_counts.drive_error.media_error_tag, dieh_stats.error_counts.drive_error.media_error_tag);

    /* make sure no other IO could be fading stats.*/
    MUT_ASSERT_UINT64_EQUAL(dieh_stats.error_counts.drive_error.io_counter, prev_dieh_stats.error_counts.drive_error.io_counter);


    /* Steps
        1. Verify current
        2. Verify disable
        3. Verify reseting to default
    */

    
    {
        /* 1. Verify current media threshold */
        fbe_u64_t delta;
        fbe_u64_t media_change;
        //fbe_u32_t prev_media_weight_adjust;
        fbe_u32_t new_weight;


        fbe_api_physical_drive_get_dieh_info(pdo, &prev_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.dieh_state.media_weight_adjust, FBE_STAT_WEIGHT_CHANGE_NONE);
    
        percy_inject_one_error(pdo, &PERCY_MEDIA_ERROR);
        fbe_api_sleep(110);
        percy_inject_one_error(pdo, &PERCY_RECOVERABLE_ERROR);
        fbe_api_sleep(110);

        fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
            
        delta = dieh_stats.error_counts.drive_error.media_error_tag - prev_dieh_stats.error_counts.drive_error.media_error_tag;
        media_change = (fbe_u64_t)(dieh_stats.drive_stat.media_stat.weight/(double)(dieh_stats.drive_stat.media_stat.weight - delta)*100);
    
        new_weight = (fbe_u32_t)(dieh_stats.dieh_state.media_weight_adjust/100.0 * dieh_stats.drive_stat.media_stat.weight);

        mut_printf(MUT_LOG_LOW, "verify current - io_count:0x%llx->0x%llx, media_et:0x%llx->0x%llx, percent:%d->0x%llx", 
                   prev_dieh_stats.error_counts.drive_error.io_counter, dieh_stats.error_counts.drive_error.io_counter,
                   prev_dieh_stats.error_counts.drive_error.media_error_tag, dieh_stats.error_counts.drive_error.media_error_tag,
                   dieh_stats.dieh_state.media_weight_adjust, media_change);

        mut_printf(MUT_LOG_LOW, "verify current - media_weight:%lld, delta:%lld",
                   dieh_stats.drive_stat.media_stat.weight, delta);

        MUT_ASSERT_UINT64_EQUAL_MSG(new_weight, delta, "new_weight != delta");


        /**********************************************************************************************************/
        
        /* 2.  Disable EMEH.  */

        mut_printf(MUT_LOG_LOW, "%s Test Disabling DIEH Media Error Threshold", __FUNCTION__);

        threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_DISABLE;
        threshold.option = 0; 
        fbe_api_physical_drive_set_dieh_media_threshold(pdo, &threshold);

        fbe_api_physical_drive_get_dieh_info(pdo, &prev_dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.dieh_mode, FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.dieh_state.media_weight_adjust, FBE_STAT_WEIGHT_CHANGE_NONE);

        percy_verify_errors_do_not_kill(0, 0, 4, PERCY_DIEH_MEDIA,     300, FBE_FALSE);
        percy_verify_errors_do_not_kill(0, 0, 4, PERCY_DIEH_RECOVERED, 900, FBE_FALSE);
        percy_verify_errors_do_not_kill(0, 0, 4, PERCY_DIEH_HARDWARE,  300, FBE_FALSE);

        fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);

        /* verify ratios did not change while EMEH disabled */
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.media_error_ratio, dieh_stats.error_counts.media_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.recovered_error_ratio, dieh_stats.error_counts.recovered_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.io_error_ratio, dieh_stats.error_counts.io_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.hardware_error_ratio, dieh_stats.error_counts.hardware_error_ratio); 

        /**********************************************************************************************************/

        /* 3. Reset to default */
        mut_printf(MUT_LOG_LOW, "%s Test Restore DIEH Media Error Threshold", __FUNCTION__);
        threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS;
        threshold.option = 0; 
        fbe_api_physical_drive_set_dieh_media_threshold(pdo, &threshold);

        /* after EMEH restore, verify ratios have not changed.  */
        fbe_api_physical_drive_get_dieh_info(pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(dieh_stats.dieh_mode, FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT);
        MUT_ASSERT_INT_EQUAL(dieh_stats.dieh_state.media_weight_adjust, FBE_STAT_WEIGHT_CHANGE_NONE);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.media_error_ratio, dieh_stats.error_counts.media_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.recovered_error_ratio, dieh_stats.error_counts.recovered_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.io_error_ratio, dieh_stats.error_counts.io_error_ratio);
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.hardware_error_ratio, dieh_stats.error_counts.hardware_error_ratio); 
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.healthCheck_error_ratio, dieh_stats.error_counts.healthCheck_error_ratio); 
        MUT_ASSERT_INT_EQUAL(prev_dieh_stats.error_counts.link_error_ratio, dieh_stats.error_counts.link_error_ratio);

        /* Verify EOL still set */
        status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus,encl,slot, &pvd);        
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        status = fbe_api_get_block_edge_info(pvd, 0, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

        percy_verify_errors_do_kill(0, 0, 4, PERCY_DIEH_MEDIA, 98, FBE_TRUE);
        percy_restore_drive(bus,encl,slot);                  
    }


    /* cleanup */
    percy_enable_background_services(bus,encl,slot);
    fbe_api_physical_drive_clear_error_counts(pdo, FBE_PACKET_FLAG_NO_ATTRIB);
}

/*!**************************************************************
 * percy_test_dieh_exception_list() 
 ****************************************************************
 * @brief
 *  Test the DIEH exception list by verifying drive takes
 *  appropriate action.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  11/18/2013 Wayne Garrett - Created
 ****************************************************************/
static void percy_test_dieh_exception_list(void)
{
    const fbe_u32_t bus  = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 4;
    fbe_object_id_t pdo = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd = FBE_OBJECT_ID_INVALID;
    fbe_drive_mgmt_dieh_load_config_t config_info;    
    fbe_status_t status;    
    const fbe_u32_t sleep_interval = 1000; /* msec */
    fbe_u32_t wait_time;
    fbe_u32_t i,j;
    fbe_api_drive_config_get_handles_list_t handles = {0};
    fbe_drive_configuration_record_t dcs_record = {0};
    fbe_payload_cdb_fis_error_flags_t verified_actions = 0;


    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    strncpy(config_info.file, PERCY_DIEH_CONFIG_EXCEPTION_LIST_FILENAME, FBE_DIEH_PATH_LEN-1);
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);
    
    percy_wait_for_dieh_config_to_update();
    
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* msec*/);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo != FBE_OBJECT_ID_INVALID);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);        
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* verify drive has registered with new handle
     */
    for (wait_time = 0; wait_time < 30000; wait_time+=sleep_interval)  /*msec*/
    {
        if (percy_is_pdo_registered_to_valid_handle(pdo))
        {
            break;
        }
        fbe_api_sleep(sleep_interval);
    }
    MUT_ASSERT_TRUE_MSG(wait_time < 30000, "PDO failed to register with DIEH in time");



    /* Iterate through each exception and verify drive takes appropriate action.
    */

    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get drive_config handles");


    for (i = 0; i<handles.total_count; i++ )
    {
        status = fbe_api_drive_configuration_interface_get_drive_table_entry(handles.handles_list[i], &dcs_record);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get_drive_table_entry");

        j = 0;
        while (dcs_record.category_exception_codes[j].type_and_action != FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR)
        {
            percy_verify_dieh_exception(pdo, pvd, &dcs_record.category_exception_codes[j].scsi_fis_union.scsi_code,
                                        dcs_record.category_exception_codes[j].type_and_action);

            verified_actions |= dcs_record.category_exception_codes[j].type_and_action;
            j++;
        }
    }

    MUT_ASSERT_INT_EQUAL_MSG(verified_actions, 
                             (FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE |
                              FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE_CALLHOME |
                              FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL |
                              FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL_CALLHOME), 
                             "Not all exception actions verified");

}

/*!**************************************************************
 * percy_verify_dieh_exception() 
 ****************************************************************
 * @brief
 *  Test and verify a DIEH exception 
 * 
 * @param pdo        - pdo object ID
 * @param pvd        - pvd object ID
 * @param scsi_error - error code to inject.
 * @param action     - expected action after scsi_error is injected.
 *
 * @return None.
 *
 * @author
 *  11/18/2013 Wayne Garrett - Created
 ****************************************************************/
static void percy_verify_dieh_exception(fbe_object_id_t pdo, fbe_object_id_t pvd, fbe_drive_config_scsi_sense_code_t *scsi_error, fbe_payload_cdb_fis_error_flags_t action)
{
    fbe_dest_error_record_t record = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t rdgen_context = {0};
    fbe_status_t status;
    fbe_physical_drive_information_t drive_info = {0};
    fbe_device_physical_location_t drive_location = {0};
    fbe_u8_t deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_bool_t is_msg_present = FBE_FALSE;

    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    drive_location.bus = drive_info.port_number;
    drive_location.enclosure = drive_info.enclosure_number;
    drive_location.slot = drive_info.slot_number;    



    fbe_api_clear_event_log(FBE_PACKAGE_ID_PHYSICAL);

    /* Create DEST record */
    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       scsi_error->sense_key, scsi_error->asc_range_start, scsi_error->ascq_range_start, // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    denali_add_record_to_dest(&record, &record_handle);

    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Send I/O.  We expect this to get an  error because of the error injected
     * above. 
     */  
    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x100,                            // start lba
                                       0x100,                            // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);       

    /* verify action */
    switch(action)
    {
    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE:
        /* verify EOL set on edge */
        status = fbe_api_wait_for_block_edge_path_attribute(pvd, 0, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE, FBE_PACKAGE_ID_SEP_0, 30000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        break;

    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL:
        /* verify drive_fault set on edge */
        status = fbe_api_wait_for_block_edge_path_attribute(pvd, 0, FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT, FBE_PACKAGE_ID_SEP_0, 30000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        /* verify pdo goes to fail*/
        status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_FAIL, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        break;

    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE_CALLHOME:
        /* verify call home event log */
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &drive_location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to creat device string.");

        status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_PHYSICAL, 
                                         &is_msg_present, 
                                         PHYSICAL_PACKAGE_ERROR_PDO_SET_EOL,
                                         &deviceStr[0],
                                         drive_info.product_serial_num);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");
        break;

    case FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL_CALLHOME:
        /* verify call home event log */
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &drive_location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to creat device string.");

        status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_PHYSICAL, 
                                         &is_msg_present, 
                                         PHYSICAL_PACKAGE_ERROR_PDO_SET_DRIVE_KILL,
                                         &deviceStr[0],
                                         drive_info.product_serial_num);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");
        break;

    default:
        mut_printf(MUT_LOG_LOW, "%s unhandled action 0x%x", __FUNCTION__, action); 
        MUT_FAIL_MSG("Unhandled action");
    }

    /* cleanup
     */
    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
      
    status = fbe_api_physical_drive_get_drive_information(pdo, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    percy_restore_drive(drive_info.port_number, drive_info.enclosure_number, drive_info.slot_number);
}

static void percy_restore_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_api_get_block_edge_info_t edge_info;
    fbe_object_id_t pvd;
    //fbe_api_terminator_device_handle_t drive_handle;
    fbe_u32_t wait_time;
    const fbe_u32_t sleep_interval = 1000; /*msec*/


    /* this is needed if PVD is broken because pvdsrvc clear doesn't work*/
    percy_test_remove_drive(bus,encl,slot);    
    percy_test_insert_drive(bus,encl,slot);    
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /*msec*/);   
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_provision_drive_clear_all_pvds_drive_states();

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (wait_time = 0; wait_time < 30000; wait_time+=sleep_interval)
    {
        status = fbe_api_get_block_edge_info(pvd, 0, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* clearing pvd will clear drive_fault and eol. verify it.  */
        if ((edge_info.path_attr & (FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE |
                                    FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT) ) == 0)
        {
            /* states have been cleared. */
            break;
        }
        fbe_api_sleep(sleep_interval);
    }
    MUT_ASSERT_INT_EQUAL_MSG((wait_time<30000), FBE_TRUE, "wait time expired. PVD edges not restored");
    //MUT_ASSERT_CONDITION_MSG(wait_time, <, 30000, "wait time expired. PVD edges not restored");

    status = fbe_test_sep_drive_verify_pvd_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /*restore drive and verify correct action is loaded*/
    percy_test_remove_drive(bus, encl, slot);    
    percy_test_insert_drive(bus, encl, slot);    
#if 0    
    /* reseat the drive */
    status = fbe_test_pp_util_pull_drive(bus, encl, slot, &drive_handle); /* this will clear any dieh stats. */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_DESTROY, 30000 /* wait 30sec */);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_pp_util_reinsert_drive(bus, encl, slot, drive_handle);  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif

    /* wait for pdo to come up */
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /* wait 30sec */);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_test_sep_drive_verify_pvd_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static fbe_bool_t percy_is_pdo_registered_to_valid_handle(fbe_object_id_t pdo)
{
    fbe_status_t status;
    fbe_physical_drive_dieh_stats_t pdo_dieh_stats = {0};
    fbe_api_drive_config_get_handles_list_t handles = {0};
    fbe_u32_t i;

    percy_test_get_dieh_info(pdo, &pdo_dieh_stats);

    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get drive_config handles");

    for (i = 0; i<handles.total_count; i++ )
    { 
        if (pdo_dieh_stats.drive_configuration_handle == handles.handles_list[i])
        {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}

/*!**************************************************************
 * percy_test_verify_default_config_04xx_exception() 
 ****************************************************************
 * @brief
 *  Verify that the default configuration exception for 04/xx will
 *  EOL on first occurance and still shoot the drive after an
 *  appropriate amount of 04/xx errors. 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/1/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_test_verify_default_config_04xx_exception(void)
{
    fbe_drive_mgmt_dieh_load_config_t config_info;
    fbe_u32_t bus=0, encl=0, slot=4;
    fbe_object_id_t pdo, pvd;
    fbe_dest_error_record_t record = {0};
    fbe_dest_error_record_t record_out = {0};
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t rdgen_context = {0};
    fbe_status_t status;
    fbe_u32_t times_to_insert = 40;
    fbe_u32_t num_ios_to_send = 40;
    fbe_u32_t i;


    mut_printf(MUT_LOG_TEST_STATUS, "%s ...", __FUNCTION__);

    config_info.file[0] = '\0';    /* empty filename string will load default */

    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);

    percy_wait_for_dieh_config_to_update();    

    /* verify by injecting errors into a drive */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,encl,slot, &pdo);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus,encl,slot, &pvd);        
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* Create DEST record */
    denali_create_scsi_cc_error_record(NULL, &record, pdo, FBE_SCSI_READ_10,  
                                       0x04, 0x44, 0x00, // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    record.lba_start = 0x100;
    record.lba_end   = 0x100; 
    record.num_of_times_to_insert = times_to_insert;
    denali_add_record_to_dest(&record, &record_handle);


    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Send I/O.  We expect this to get an  error because of the error injected
     * above. 
     */  
    status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                       pdo,                              // object id
                                       FBE_CLASS_ID_INVALID,             // class id
                                       FBE_PACKAGE_ID_PHYSICAL,          // package id
                                       FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                       0x100,                            // start lba
                                       0x100,                            // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);       

    /* verify EOL set on edge */
    status = fbe_api_wait_for_block_edge_path_attribute(pvd, 0, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE, FBE_PACKAGE_ID_SEP_0, 30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (i=0; i<num_ios_to_send; i++)
    {

        /* Note, eventually PDO goes into activate state due to a reset, which takes a couple seconds.  During this
           time the test is still sending IO.  The test could probably be improved some day. For now bumping up the
           number of IOs sent to ride through PDO's activate state.  */

        fbe_api_sleep(200);  /* avoid burst reduction */
        status = fbe_api_rdgen_send_one_io(&rdgen_context,
                                           pdo,                              // object id
                                           FBE_CLASS_ID_INVALID,             // class id
                                           FBE_PACKAGE_ID_PHYSICAL,          // package id
                                           FBE_RDGEN_OPERATION_READ_ONLY,    // rdgen opcode
                                           FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                           0x100,                            // start lba
                                           0x100,                            // min blocks
                                           FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                           0,0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);               
    }

    /* verify pdo goes to fail*/
    status = fbe_api_wait_for_object_lifecycle_state(pdo, FBE_LIFECYCLE_STATE_FAIL, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify DEST injected an appropriate number of errors */
    fbe_api_dest_injection_get_record(&record_out, record_handle);
    if (record_out.times_inserted < 27 || record_out.times_inserted > 32)   /* check a range.  Avg is 28 with no fading and no successful IO*/
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s num errors injected (%d) not in range of 27..32", __FUNCTION__, record_out.times_inserted);
        MUT_FAIL_MSG("Number of HW errors injected is not in expected range");
    }

    /* cleanup
     */
    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    

    /* this is needed by PVD is broken so pvdsrvc clear doesn't work*/
    percy_test_remove_drive(bus,encl,slot);    
    percy_test_insert_drive(bus,encl,slot);    
    status = fbe_test_pp_util_verify_pdo_state(bus,encl,slot, FBE_LIFECYCLE_STATE_READY, 30000 /*msec*/);                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  

    percy_restore_drive(bus,encl,slot);
}

static void percy_test_driveconfig_override(void)
{    
    fbe_u32_t bus=0, encl=0, slot=4;
    fbe_dcs_tunable_params_t params = {0};
    fbe_status_t status;
    fbe_drive_mgmt_dieh_load_config_t config_info;    
    config_info.file[0] = '\0';    /* empty filename string will load default */

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    percy_registry_write_driveconfig_override_file("DriveConfiguration_non-paco-r.xml");
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);
    percy_wait_for_dieh_config_to_update();    

    /* verify configuration  */
    percy_verify_override_config();

    /* verify DCS tunables*/
    status = fbe_api_drive_configuration_interface_get_params(&params);    
    MUT_ASSERT_UINT64_EQUAL_MSG(params.service_time_limit_ms, 27000, "service time incorrect");
    MUT_ASSERT_UINT64_EQUAL_MSG(params.remap_service_time_limit_ms, 27000, "remap svc time incorrect");
    MUT_ASSERT_UINT64_EQUAL_MSG(params.emeh_service_time_limit_ms, 27000, "emeh svc time incorrect");


    /* test injecting errors */
    percy_test_default_config_actions(bus, encl, slot, PERCY_DIEH_MEDIA,     50,  100);
    percy_restore_drive(bus,encl,slot);

    percy_test_default_config_actions(bus, encl, slot, PERCY_DIEH_RECOVERED, 250, 300);
    percy_restore_drive(bus,encl,slot);

    /* restore to default */
    percy_registry_write_driveconfig_override_file("DriveConfiguration.xml");
    percy_test_dieh_load_config_file(&config_info, FBE_DMO_THRSH_NO_ERROR);
    percy_wait_for_dieh_config_to_update();
}

/*!**************************************************************
 * percy_registry_write_driveconfig_override_file() 
 ****************************************************************
 * @brief
 *  Change the registry setting for the DriveConfiguration
 *  override XML file.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/15/2015 - Created.  Wayne Garrett
 ****************************************************************/
static void percy_registry_write_driveconfig_override_file(fbe_u8_t * filepathname)
{   
    fbe_status_t status;    

    status = fbe_test_esp_create_registry_file(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_esp_registry_write(/*ESP_REG_PATH*/EspRegPath,
                                FBE_DIEH_PATH_REG_KEY, 
                                FBE_REGISTRY_VALUE_SZ, 
                                filepathname,   
                                0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void percy_verify_override_config(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t									i = 0;
    fbe_api_drive_config_get_handles_list_t     handles;
    fbe_drive_configuration_record_t            record;
    fbe_bool_t is_default_found = FBE_FALSE;
    fbe_bool_t is_rcx_100g_found = FBE_FALSE;
    fbe_bool_t is_rcx_200g_found = FBE_FALSE;

    mut_printf(MUT_LOG_LOW, "%s ...", __FUNCTION__);

    status = fbe_api_drive_configuration_interface_get_handles(&handles, FBE_API_HANDLE_TYPE_DRIVE);

    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get drive_config handles");
    MUT_ASSERT_INT_EQUAL_MSG(handles.total_count, 3, "default config has wrong number of records");


    for (i = 0; i<handles.total_count; i++ )
    {
        status = fbe_api_drive_configuration_interface_get_drive_table_entry(handles.handles_list[i], &record);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get_drive_table_entry");

        if (percy_is_default_record(&record)) 
        {
            is_default_found = FBE_TRUE;
        }
        else if (percy_is_rcx_100g_record(&record))
        {
            is_rcx_100g_found = FBE_TRUE;
        }
        else if (percy_is_rcx_200g_record(&record))
        {
            is_rcx_200g_found = FBE_TRUE;
        }
    }

    MUT_ASSERT_INT_EQUAL_MSG(is_default_found, FBE_TRUE, "default rec not found");
    MUT_ASSERT_INT_EQUAL_MSG(is_rcx_100g_found, FBE_TRUE, "rcx 100g rec not found");
    MUT_ASSERT_INT_EQUAL_MSG(is_rcx_200g_found, FBE_TRUE, "rcx 200g rec not found");
}

/*************************
 * end file percy_test.c
 *************************/
