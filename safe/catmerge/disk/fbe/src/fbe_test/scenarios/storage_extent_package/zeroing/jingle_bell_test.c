/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file jingle_bell_test.c
 ***************************************************************************
 *
 * @brief   : LUN Zeroing Test
 *
 * @version
 *  1/4/2012  Wei He  - Created.
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
#include "fbe_private_space_layout.h"
#include "fbe/fbe_random.h"
#include "sep_rebuild_utils.h"





/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * jingle_bell_short_desc = "LUN Zeroing Test";
char * jingle_bell_long_desc =
"\n"
"   The Jingle_bell_test tests two LUN zero api\n"
"       fbe_api_lun_initiate_zero() is used for zero both user LUN and system LUN\n"
"           The supported system LUNs include VAULT, PSM etc.\n"
"       fbe_api_lun_initiate_reboot_zero() is used for zero LUN after reboot\n"
"           It changes the generation number in non-paged metadata.\n"
"           After the system reboot, LUN will be automatically zeroed.\n"
"\n"
"Starting Config:\n"
"        [PP] armada board\n"
"        [PP] SAS PMC port\n"
"        [PP] viper enclosure\n"
"        [PP] 2 SAS drives\n"
"        [PP] 2 logical drive\n"
"        [SEP] 2 provision drive\n"
"        [SEP] 2 virtual drive\n"
"        [SEP] 1 parity raid group (raid 0 - 520)\n"
"        [SEP] 1 LUN\n"
"\n"
"   STEP 1: Bring up the initial topology.\n"
"        - Create and verify the initial physical package config.\n"
"        - Create provisioned drives and attach edges to logical drives.\n"
"        - Create virtual drives and attach edges to provisioned drives.\n"
"        - Create raid raid group objects and attach to virtual drives.\n\n"
"   STEP 2: Create Raid Group and the user LUN on it\n"
"   STEP 3: Test zero user LUN without reboot\n"
"        - Use rdgen to write unzero pattern to LUN.\n"
"        - Trigger LUN zero.\n"
"        - Use rdgen to read and check whether the data is zeroed.\n"
"   STEP 4: Test zero user LUN with reboot\n"
"        - Use rdgen to write unzero pattern to LUN.\n"
"        - Trigger LUN zero, actually modify the LUN generation number.\n"
"        - Reboot both SPs.\n"
"        - Use rdgen to read and check whether the data is zeroed.\n"
"   STEP 5: Test zero vault LUN without reboot\n"
"        - Use rdgen to write unzero pattern to vault LUN.\n"
"        - Trigger vault LUN zero.\n"
"        - Use rdgen to read and check whether the data is zeroed.\n"
"   STEP 5: Test zero PSM LUN without reboot\n"
"        - Use rdgen to write unzero pattern to PSM LUN.\n"
"        - Trigger PSM LUN zero.\n"
"        - Use rdgen to read and check whether the data is zeroed.\n"
"\n";


/*!*******************************************************************
 * @def JINGLE_BELL_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define JINGLE_BELL_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @def JINGLE_BELL_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing complete
 *
 *********************************************************************/
#define JINGLE_BELL_POLLING_INTERVAL 100 /*ms*/

/* Element size of our LUNs.
 */
#define JINGLE_BELL_TEST_ELEMENT_SIZE       128  /*FBE_LBA_INVALID */
#define JINGLE_BELL_TEST_WAIT_MSEC          30000
#define JINGLE_BELL_LUN_CAPACITY            0xE000
#define JINGLE_BELL_LUN_ZERO_TIMEOUT        40000


#define JINGLE_BELL_TEST_LUNS_PER_RAID_GROUP  2



/*!*******************************************************************
 * @var jingle_bell_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t jingle_bell_test_contexts[JINGLE_BELL_TEST_LUNS_PER_RAID_GROUP * 2];



/*!*******************************************************************
 * @var jingle_bell_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t jingle_bell_rg_configuration[] = 
{
     /* width,  capacity          raid type,                  class,        block size  RAID-id.  bandwidth.*/
    {3,         FBE_LBA_INVALID,  FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,            0,        0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static void jingle_bell_test_create_lun(fbe_lun_number_t   lun_number,fbe_test_rg_configuration_t *rg_config_ptr);
static void jingle_bell_test_destroy_lun(fbe_lun_number_t   lun_number);
static void jingle_bell_test_create_rg(void);
static fbe_bool_t jingle_bell_lun_zeroing_check_event_log_msg(fbe_object_id_t pvd_object_id,
                                                fbe_lun_number_t lun_number, fbe_bool_t  is_lun_zero_start );
static fbe_status_t jingle_bell_wait_for_lun_zeroing_complete(fbe_u32_t lun_number, 
                                                fbe_u32_t timeout_ms, fbe_package_id_t package_id);
static void jingle_bell_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
static void jingle_bell_test_check_lun_zeroing_started(fbe_lun_number_t   lun_number);
static fbe_status_t jingle_bell_get_nonpaged_metadata(fbe_object_id_t object_id,
                               fbe_base_config_control_metadata_get_info_t *metadata_get_info);
static fbe_status_t jingle_bell_get_gen_num_from_md(fbe_config_generation_t *gen_num,
                                                                          fbe_object_id_t object_id);
static void jingle_bell_write_nonzero_pattern(fbe_object_id_t lun_object_id);
static void jingle_bell_read_check_zero_pattern(fbe_object_id_t lun_object_id);
static void jingle_bell_test_system_reboot(void);
static void jingle_bell_test_generation_number_change(fbe_object_id_t lun_object_id);
static void jingle_bell_test_zero_user_lun(fbe_object_id_t lun_object_id);
static void jingle_bell_test_zero_user_lun_with_reboot(fbe_object_id_t lun_object_id);
static void jingle_bell_test_zero_vault_lun(void);
static void jingle_bell_test_zero_psm_lun(void);
static void jingle_bell_run_dualsp_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
void jingle_bell_dualsp_cleanup(void);
void jingle_bell_dualsp_setup(void);
static void jingle_bell_test_zero_user_lun_with_reboot_dualsp(fbe_object_id_t lun_object_id);

static void jingle_bell_test_zero_user_lun_dualsp(fbe_object_id_t lun_object_id);
static void jingle_bell_test_generation_number_change_dualsp(fbe_object_id_t lun_object_id);
static void jingle_bell_test_zero_system_lun_dualsp(void);
static void jingle_bell_test_zero_system_lun_dualsp_with_removing_drive(void);
static void jingle_bell_test_system_reboot_dualsp(void);
void jingle_remove_drive_and_verify(fbe_u32_t                           bus,
                                    fbe_u32_t                           enclosure,
                                    fbe_u32_t                           slot,
                                    fbe_api_terminator_device_handle_t* out_drive_info_p_spa,
                                    fbe_api_terminator_device_handle_t* out_drive_info_p_spb);

void jingle_reinsert_drive_and_verify(fbe_u32_t                           bus,
                                      fbe_u32_t                           enclosure,
                                      fbe_u32_t                           slot,
                                      fbe_api_terminator_device_handle_t* in_drive_info_p_spa,
                                      fbe_api_terminator_device_handle_t* in_drive_info_p_spb);


/*!**************************************************************
 * jingle_bell_write_nonzero_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param lun_object_id  - The target LUN object id.               
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 ****************************************************************/
static void jingle_bell_write_nonzero_pattern(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &jingle_bell_test_contexts[0];
    fbe_status_t status;
    
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             1024, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             1,    /* */
                                             JINGLE_BELL_TEST_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;
}

/*!**************************************************************
 * jingle_bell_start_userzero_io
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param lun_object_id  - The target LUN object id.               
 * @param context_p - The rdgen io context
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 ****************************************************************/
static void jingle_bell_start_userzero_io(fbe_object_id_t lun_object_id, fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t status;
    
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_ZERO_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             0x800,    /* min blocks*/
                                             0x800    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
}


/*!**************************************************************
 * jingle_bell_read_check_zero_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param lun_object_id  - The target LUN object id.               
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 ****************************************************************/
static void jingle_bell_read_check_zero_pattern(fbe_object_id_t lun_object_id)
{
    fbe_api_rdgen_context_t *context_p = &jingle_bell_test_contexts[0];
    fbe_status_t status;
    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             1024, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             1,    /* */
                                             JINGLE_BELL_TEST_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}

/*!**************************************************************
 * jingle_bell_get_gen_num_from_md()
 ****************************************************************
 * @brief
 *  Read the generation number from metadata.
 *
 * @param gen_num - Pointer to the generation number.               
 * @param object_id - The target LUN's object id.
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 ****************************************************************/

static fbe_status_t jingle_bell_get_gen_num_from_md(fbe_config_generation_t *gen_num, 
                                                                         fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_base_config_control_metadata_get_info_t metadata_get_info;
    fbe_base_config_nonpaged_metadata_t *base_config_nonpaged_metadata_p;

    status = jingle_bell_get_nonpaged_metadata(object_id, &metadata_get_info);

    if(FBE_STATUS_OK == status)
    {
        base_config_nonpaged_metadata_p = (fbe_base_config_nonpaged_metadata_t*)metadata_get_info.nonpaged_data.data;
        *gen_num = base_config_nonpaged_metadata_p->generation_number;
    }
    
    return status;
}



/*!**************************************************************
 * jingle_bell_get_nonpaged_metadata()
 ****************************************************************
 * @brief
 *  Read the nonpaged metadata.
 *
 * @param gen_num - Pointer to the nonpaged metadata.               
 * @param gen_num - The target LUN's object id.
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 ****************************************************************/

static fbe_status_t jingle_bell_get_nonpaged_metadata(fbe_object_id_t object_id,
                              fbe_base_config_control_metadata_get_info_t *metadata_get_info)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    status = fbe_api_common_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
                                                metadata_get_info,
                                                sizeof(fbe_base_config_control_metadata_get_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW, "Get metadata failed\n"); 
        if(status != FBE_STATUS_OK)
        {
            return status;
        }else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}


/*!****************************************************************************
 *  jingle_bell_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the jingle_bell test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *
 *****************************************************************************/
static void jingle_bell_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   lun_number = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_object_id_t             lun_object_id;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    mut_printf(MUT_LOG_LOW, "=== LUN BIND ===\n"); 

    rg_config_p = rg_config_ptr;
    if(!fbe_test_rg_config_is_enabled(rg_config_p))
        return;

    jingle_bell_test_create_rg();

    lun_number = 0;
    jingle_bell_test_create_lun(lun_number,rg_config_p);

    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = jingle_bell_wait_for_lun_zeroing_complete(lun_object_id, 
                                                       JINGLE_BELL_LUN_ZERO_TIMEOUT, 
                                                       FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "LUN background zero complete\n");
    /*

    jingle_bell_test_generation_number_change(lun_object_id);
    */

    jingle_bell_test_zero_user_lun(lun_object_id);
    jingle_bell_test_zero_vault_lun();
    jingle_bell_test_zero_psm_lun();
    jingle_bell_test_zero_user_lun_with_reboot(lun_object_id);

    mut_printf(MUT_LOG_LOW, "=== Jingle_bell test complete ===\n");

    return;
}
/******************************************
 * end jingle_bell_test()
 ******************************************/
/*!****************************************************************************
 *  jingle_bell_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the jingle_bell_dualsp_test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author 
 * 1/4/2012  Wei He  - Created.
 *
 *****************************************************************************/
static void jingle_bell_run_dualsp_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   lun_number = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_object_id_t             lun_object_id;
    fbe_sim_transport_connection_target_t active_target;

    mut_printf(MUT_LOG_LOW, "=== Start jingle_bell_dualsp test ===\n");
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = rg_config_ptr;
    if(!fbe_test_rg_config_is_enabled(rg_config_p))
        return;

    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);

    mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test :Active SP is %s ===\n", 
               active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    jingle_bell_test_create_rg();
    mut_printf(MUT_LOG_LOW, "=== Raid Group Created ===\n"); 

    lun_number = 0;
    jingle_bell_test_create_lun(lun_number,rg_config_p);
    mut_printf(MUT_LOG_LOW, "=== LUN bound ===\n"); 

    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "Waiting for LUN background zero\n");
    status = jingle_bell_wait_for_lun_zeroing_complete(lun_object_id, JINGLE_BELL_LUN_ZERO_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "LUN background zero complete\n");

    jingle_bell_test_zero_user_lun_dualsp(lun_object_id);
    jingle_bell_test_zero_system_lun_dualsp();
    jingle_bell_test_zero_system_lun_dualsp_with_removing_drive();
    jingle_bell_test_zero_user_lun_with_reboot_dualsp(lun_object_id);
    
    mut_printf(MUT_LOG_LOW, "=== Jingle_bell test complete ===\n");
    return;
}
/******************************************
 * end jingle_bell_dualsp_test()
 ******************************************/

/*!****************************************************************************
 * jingle_bell_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2/22/2011 - Created. He, Wei
 ******************************************************************************/
void jingle_bell_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * jingle_bell_dualsp_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2/22/2011 - Created. He, Wei
 ******************************************************************************/
void jingle_bell_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}


/*!****************************************************************************
 * jingle_bell_test()
 ******************************************************************************
 * @brief
 *  Run lun zeroing test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 ******************************************************************************/
void jingle_bell_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &jingle_bell_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
   
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, jingle_bell_run_tests);

     return;
}
/******************************************
 * end jingle_bell_test()
 ******************************************/

/*!****************************************************************************
 * jingle_bell_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run dual sp lun zeroing test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 ******************************************************************************/
void jingle_bell_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &jingle_bell_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
   
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, jingle_bell_run_dualsp_tests);

     return;
}
/******************************************
 * end jingle_bell_test()
 ******************************************/


/*!****************************************************************************
 *  jingle_bell_test_check_lun_zeroing_started
 ******************************************************************************
 *
 * @brief
 *    This function verify that lun zeroing is not started  yet. 
 *    
 *
 * @return  void
 *
 * @author
 *        5/17/2011 - Created. Vishnu Sharma
 *****************************************************************************/
static void jingle_bell_test_check_lun_zeroing_started(fbe_object_id_t lu_object_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_api_lun_get_zero_status_t       lu_zero_status;
    
   /*get lun zeroing status*/
    status = fbe_api_lun_get_zero_status(lu_object_id, &lu_zero_status);

    if (status == FBE_STATUS_GENERIC_FAILURE){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
        return ;
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                  "%s: lu_object_id 0x%x has not yet started zeroing %d zeroing\n", 
                  __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage);

    /*Make sure that disk zeroing is not started yet  */
    MUT_ASSERT_TRUE(lu_zero_status.zero_percentage == 0);
        
    return;
}

/******************************************
 * end jingle_bell_test_check_lun_zeroing_started()
 ******************************************/


/*!****************************************************************************
 *  jingle_bell_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the jingle_bell test.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 *****************************************************************************/

void jingle_bell_setup(void)
{

    mut_printf(MUT_LOG_LOW, "=== jingle_bell setup ===\n");    

    if (fbe_test_util_is_simulation())
    {

        zeroing_physical_config();
        sep_config_load_sep_and_neit();
    }
    
    return; 
}
/******************************************
 * end jingle_bell_setup()
 ******************************************/

/*!**************************************************************
 * jingle_bell_dualsp_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  
 *
 ****************************************************************/
void jingle_bell_dualsp_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Jingle Bell dual SP test ===\n");

    if (fbe_test_util_is_simulation())
    {
        /* Load the config on both SPs. */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        zeroing_physical_config();
        //sep_config_load_sep_and_neit();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        zeroing_physical_config();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        sep_config_load_sep_and_neit_both_sps();
        //sep_config_load_sep_and_neit();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    return;
}


/*!****************************************************************************
 *  jingle_bell_wait_for_lun_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks whether lun zeroing gets completed or not.
 *   and make sure that "LUN zeroing complete" event message has been logged.
 *
 * @return  void
 *
 * @author
 *    11/18/2010 - Created. Amit Dhaduk
 *
 *
 *****************************************************************************/

static fbe_status_t jingle_bell_wait_for_lun_zeroing_complete(fbe_object_id_t lu_object_id, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_lun_get_zero_status_t       lu_zero_status;
    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u32_t                           lun_number;

    status = fbe_api_database_lookup_lun_by_object_id(lu_object_id, &lun_number);
    if(status != FBE_STATUS_OK)
        return status;
    
    while (current_time < timeout_ms){

        status = fbe_api_lun_get_zero_status(lu_object_id, &lu_zero_status);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            mut_printf(MUT_LOG_LOW, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);
            return status;
        }

        mut_printf(MUT_LOG_LOW, "%s: lu_object_id 0x%x has completed %d zeroing\n", 
                   __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage);
        

        /* check if disk zeroing completed */
        if (lu_zero_status.zero_percentage == 100)
        {
            /* check an event message for lun zeroing completed
             * Event log message may take little bit more time to log in
             */
            is_msg_present = jingle_bell_lun_zeroing_check_event_log_msg(lu_object_id, lun_number, FBE_FALSE);
            if(is_msg_present)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "LUN Zeroing completed for object 0x%x\n", lu_object_id);
                return status;
            }
        }

        current_time = current_time + JINGLE_BELL_POLLING_INTERVAL;
        fbe_api_sleep (JINGLE_BELL_POLLING_INTERVAL);
    }
    
    mut_printf(MUT_LOG_LOW, 
               "%s: lu_object_id %d percentage-0x%x, event msg present %d, %d ms!, \n", 
               __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage, is_msg_present, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/******************************************
 * end jingle_bell_wait_for_lun_zeroing_complete()
 ******************************************/


/*!****************************************************************************
 *  jingle_bell_test_create_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for creating a LUN.
 *
 * @param   lun_number - name of LUN to create
 *
 * @return  None 
 * 5/17/2011 - Modified. Vishnu Sharma
 * 1/4/2012 - Modified. Wei He.
 *****************************************************************************/
static void jingle_bell_test_create_lun(fbe_lun_number_t   lun_number,
                                     fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                       status;
    fbe_api_lun_create_t               fbe_lun_create_req;
    fbe_object_id_t                    object_id;
    fbe_bool_t                         is_msg_present = FBE_FALSE;
    fbe_job_service_error_type_t       job_error_type;        
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type        = rg_config_p->raid_type;
    fbe_lun_create_req.raid_group_id    = rg_config_p->raid_group_id;
    fbe_lun_create_req.lun_number       = lun_number;
    fbe_lun_create_req.capacity         = JINGLE_BELL_LUN_CAPACITY;
    fbe_lun_create_req.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b            = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset       = FBE_LBA_INVALID;
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, JINGLE_BELL_TEST_NS_TIMEOUT, &object_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* check an event message for lun zeroing started */
    is_msg_present = jingle_bell_lun_zeroing_check_event_log_msg(object_id, lun_number, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "LUN Zeroing start - Event log msg is not present!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__, object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "LUN Zeroing started for LU object 0x%x\n", object_id);

    return;

}  
/**************************************************
 * end jingle_bell_test_create_lun()
 **************************************************/

/*!****************************************************************************
 *  jingle_bell_test_destroy_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for destroying a LUN.  
 *
 * @param   lun_number - name of LUN to destroy
 *
 * @return  None 
 *
 *****************************************************************************/
static void jingle_bell_test_destroy_lun(fbe_lun_number_t   lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       fbe_lun_destroy_req;
    fbe_job_service_error_type_t job_error_type;
    
    fbe_lun_destroy_req.lun_number = lun_number;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

    /* Destroy a LUN */
    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  JINGLE_BELL_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

    return;

}  
/**************************************************
 * end jingle_bell_test_destroy_lun()
 **************************************************/


/*!**************************************************************
 * jingle_bell_test_create_rg()
 ****************************************************************
 * @brief
 *  This is the test function for creating a Raid Group.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void jingle_bell_test_create_rg(void)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_object_id_t								rg_object_id_from_job;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    /* Create the SEP objects for the configuration */

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(jingle_bell_rg_configuration);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(jingle_bell_rg_configuration[0].job_number,
                                         JINGLE_BELL_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             jingle_bell_rg_configuration[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rg_object_id_from_job, rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} 
/**************************************************
 * end jingle_bell_test_create_rg()
 **************************************************/


/*!****************************************************************************
 *  jingle_bell_lun_zeroing_check_event_log_msg
 ******************************************************************************
 *
 * @brief
 *   This is the test function to check event messages for LUN zeroing operation.
 *
 * @param   pvd_object_id       - provision drive object id
 * @param   lun_number          - LUN number
 * @param   is_lun_zero_start  - 
 *                  FBE_TRUE  - check message for lun zeroing start
 *                  FBE_FALSE - check message for lun zeroing complete
 *
 * @return  is_msg_present 
 *                  FBE_TRUE  - if expected event message found
 *                  FBE_FALSE - if expected event message not found
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk 
 *
 *****************************************************************************/
static fbe_bool_t jingle_bell_lun_zeroing_check_event_log_msg(fbe_object_id_t lun_object_id,
                                    fbe_lun_number_t lun_number, fbe_bool_t  is_lun_zero_start )
{

    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;    
    fbe_u32_t                           message_code;               


    if(is_lun_zero_start == FBE_TRUE)
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED;
    }
    else
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED;
    }

    /* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                    &is_msg_present, 
                    message_code, lun_number, lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return is_msg_present; 
}
/*******************************************************
 * end jingle_bell_lun_zeroing_check_event_log_msg()
 *******************************************************/

/*!**************************************************************
 * jingle_bell_test_system_reboot()
 ****************************************************************
 * @brief
 *  Reboot sep and neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_system_reboot(void)
{
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(JINGLE_BELL_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * jingle_bell_test_system_reboot_dualsp()
 ****************************************************************
 * @brief
 *  Reboot sep and neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/4/2012  Wei He  - Created.
 ****************************************************************/
static void jingle_bell_test_system_reboot_dualsp(void)
{
    //fbe_status_t status;
    
    fbe_sim_transport_connection_target_t ori_target;
    fbe_test_sep_get_active_target_id(&ori_target);
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the the Dual SP ......\n");
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep();


    sep_config_load_sep_and_neit_both_sps();

    //sep_config_load_sep_and_neit();
    //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    //sep_config_load_sep_and_neit();

    //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    //status = fbe_test_sep_util_wait_for_database_service(JINGLE_BELL_TEST_WAIT_MSEC);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    //status = fbe_test_sep_util_wait_for_database_service(JINGLE_BELL_TEST_WAIT_MSEC);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_api_sim_transport_set_target_server(ori_target);
}


/*!**************************************************************
 * jingle_bell_test_generation_number_change_test()
 ****************************************************************
 * @brief
 *  test update generation number only.
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_generation_number_change(fbe_object_id_t lun_object_id)
{

    fbe_config_generation_t     generation_number;
    fbe_config_generation_t     original_generation_number;
    fbe_status_t status = FBE_STATUS_INVALID;

    mut_printf(MUT_LOG_LOW, "=== Read Original Nonpaged Metadata Generation Number ===\n");
    status = jingle_bell_get_gen_num_from_md(&original_generation_number, lun_object_id);
    mut_printf(MUT_LOG_LOW, "=== Original Generation Number: %llu ===\n",
	       (unsigned long long)original_generation_number);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== Test Update Generation Number ===\n");
    status = fbe_api_lun_initiate_reboot_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== Read New Nonpaged Metadata Generation Number ===\n");
    status = jingle_bell_get_gen_num_from_md(&generation_number, lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "=== New Generation Number: %llu ===\n",
	       (unsigned long long)generation_number);
}

/*!**************************************************************
 * jingle_bell_test_generation_number_change_test()
 ****************************************************************
 * @brief
 *  test update generation number only.
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_generation_number_change_dualsp(fbe_object_id_t lun_object_id)
{
    fbe_config_generation_t     generation_number;
    fbe_config_generation_t     original_generation_number;
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_sim_transport_connection_target_t active_target;
    fbe_sim_transport_connection_target_t target;
    mut_printf(MUT_LOG_LOW, "=== Start reboot zero lun ===\n");
    fbe_test_sep_get_active_target_id(&active_target);

    target = active_target;
    fbe_api_sim_transport_set_target_server(target);

    mut_printf(MUT_LOG_LOW, "=== Read Original Nonpaged Metadata Generation Number ===\n");
    status = jingle_bell_get_gen_num_from_md(&original_generation_number, lun_object_id);
    mut_printf(MUT_LOG_LOW, "=== Original Generation Number: %llu ===\n",
	       (unsigned long long)original_generation_number);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== Test Update Generation Number ===\n");
    status = fbe_api_lun_initiate_reboot_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== Read New Nonpaged Metadata Generation Number ===\n");
    status = jingle_bell_get_gen_num_from_md(&generation_number, lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "=== New Generation Number: %llu ===\n",
	       (unsigned long long)generation_number);
    
    if(target = FBE_SIM_SP_B)
        target = FBE_SIM_SP_A;
    else
        target = FBE_SIM_SP_B;

    mut_printf(MUT_LOG_LOW, "=== Read New Nonpaged Metadata Generation Number from the other SP===\n");
    status = jingle_bell_get_gen_num_from_md(&generation_number, lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "=== New Generation Number: %llu ===\n",
	       (unsigned long long)generation_number);

}


/*!**************************************************************
 * jingle_bell_test_zero_user_lun()
 ****************************************************************
 * @brief
 *  test LUN zero by send zero request directly.
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_user_lun(fbe_object_id_t lun_object_id)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    
    mut_printf(MUT_LOG_LOW, "===Start Testing Zero User LUN===\n");
    mut_printf(MUT_LOG_LOW, "Write non-zero pattern to LUN\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "Trigger zero LUN operation\n");        
    status = fbe_api_lun_initiate_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_LOW, "Waiting for zero complete\n");        
    status = jingle_bell_wait_for_lun_zeroing_complete(lun_object_id, JINGLE_BELL_LUN_ZERO_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    jingle_bell_read_check_zero_pattern(lun_object_id);
    mut_printf(MUT_LOG_LOW, "===Test Zero User LUN passed===\n");
}


/*!**************************************************************
 * jingle_bell_test_zero_user_lun_dualsp()
 ****************************************************************
 * @brief
 *  test LUN zero by send zero request directly to vault.
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_user_lun_dualsp(fbe_object_id_t lun_object_id)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_sim_transport_connection_target_t ori_target;
    fbe_test_sep_get_active_target_id(&ori_target);
   
    mut_printf(MUT_LOG_LOW, "===Start Testing Zero Vault LUN===\n");
    
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_LOW, "Write non-zero pattern to LUN\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "Trigger zero LUN operation\n");        
    status = fbe_api_lun_initiate_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    jingle_bell_read_check_zero_pattern(lun_object_id);
    fbe_api_sim_transport_set_target_server(ori_target);
    mut_printf(MUT_LOG_LOW, "===Test Zero User LUN passed===\n");
}

/*!**************************************************************
 * jingle_bell_test_zero_user_lun_with_reboot()
 ****************************************************************
 * @brief
 *  test LUN zero by changing the generation number,
 *  zero will start automatically on reboot
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_user_lun_with_reboot(fbe_object_id_t lun_object_id)
{
    fbe_status_t status = FBE_STATUS_INVALID;

    mut_printf(MUT_LOG_LOW, "=== Start reboot zero lun test===\n");
    mut_printf(MUT_LOG_LOW, "Write nonzero pattern\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);
    mut_printf(MUT_LOG_LOW, "fbe_api_lun_initiate_reboot_zero\n");
    status = fbe_api_lun_initiate_reboot_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    jingle_bell_test_system_reboot();

    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /*
    mut_printf(MUT_LOG_LOW, "Waiting for zero complete\n");
    status = jingle_bell_wait_for_lun_zeroing_complete(lun_object_id, 
                                                       JINGLE_BELL_LUN_ZERO_TIMEOUT, 
                                                       FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);*/
    
    mut_printf(MUT_LOG_LOW, "jingle_bell_read_check_zero_pattern : checking zero state\n");
    jingle_bell_read_check_zero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "=== Successfully reboot zero lun ===\n");
}

/*!**************************************************************
 * jingle_bell_test_zero_user_lun_with_reboot_dualsp()
 ****************************************************************
 * @brief
 *  test LUN zero by changing the generation number,
 *  zero will start automatically on reboot
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_user_lun_with_reboot_dualsp(fbe_object_id_t lun_object_id)
{
    
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_sim_transport_connection_target_t ori_target;
    mut_printf(MUT_LOG_LOW, "=== Start reboot zero lun ===\n");
    fbe_test_sep_get_active_target_id(&ori_target);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_LOW, "Write nonzero pattern on SP A\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);
    
    mut_printf(MUT_LOG_LOW, "fbe_api_lun_initiate_reboot_zero, SPA\n");
    status = fbe_api_lun_initiate_reboot_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    jingle_bell_test_system_reboot_dualsp();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    mut_printf(MUT_LOG_LOW, "jingle_bell_read_check_zero_pattern : checking zero state, SPA\n");
    jingle_bell_read_check_zero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "=== Successfully reboot zero lun ===\n");
    fbe_api_sim_transport_set_target_server(ori_target);
}


/*!**************************************************************
 * jingle_bell_test_zero_vault_lun()
 ****************************************************************
 * @brief
 *  test vault LUN zero by send zero request directly to vault.
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_vault_lun(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    
    mut_printf(MUT_LOG_LOW, "===Start Testing Zero Vault LUN===\n");

    
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_LOW, "Write non-zero pattern to LUN\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "Trigger zero LUN operation\n");        
    status = fbe_api_lun_initiate_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    jingle_bell_read_check_zero_pattern(lun_object_id);
    mut_printf(MUT_LOG_LOW, "===Test Zero Vault LUN passed===\n");
}


/*!**************************************************************
 * jingle_bell_test_zero_psm_lun()
 ****************************************************************
 * @brief
 *  test PSM LUN zero by send zero request directly to psm. 
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_psm_lun(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM;
    
    mut_printf(MUT_LOG_LOW, "===Start Testing Zero PSM LUN===\n");

    
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_LOW, "Write non-zero pattern to LUN\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "Trigger zero LUN operation\n");        
    status = fbe_api_lun_initiate_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    jingle_bell_read_check_zero_pattern(lun_object_id);
    mut_printf(MUT_LOG_LOW, "===Test Zero PSM LUN passed===\n");
}

/*!**************************************************************
 * jingle_bell_test_zero_system_lun_dualsp()
 ****************************************************************
 * @brief
 *  test system LUN zero by send zero request directly to psm. 
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_system_lun_dualsp(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM;
    fbe_sim_transport_connection_target_t ori_target;
    fbe_test_sep_get_active_target_id(&ori_target);
    
    mut_printf(MUT_LOG_LOW, "===Start Testing PSM LUN DualSP===\n");
    
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready\n");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_LOW, "Write non-zero pattern to LUN\n");
    jingle_bell_write_nonzero_pattern(lun_object_id);

    mut_printf(MUT_LOG_LOW, "Trigger zero LUN operation\n");        
    status = fbe_api_lun_initiate_zero(lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    jingle_bell_read_check_zero_pattern(lun_object_id);
    
    mut_printf(MUT_LOG_LOW, "===Test Zero PSM LUN passed===\n");
    fbe_api_sim_transport_set_target_server(ori_target);
}

/*!**************************************************************
 * jingle_bell_test_zero_system_lun_dualsp_with_removing_drive()
 ****************************************************************
 * @brief
 *  test system LUN zero by send zero request directly to psm, 
 *  at the same time glitching one system drive
 *
 * @param lun object id.               
 *
 * @return None.
 *
 * @author
 *  2/13/2012
 ****************************************************************/
static void jingle_bell_test_zero_system_lun_dualsp_with_removing_drive(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_object_id_t lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM;
    fbe_sim_transport_connection_target_t ori_target;
    fbe_api_terminator_device_handle_t drive_handle_p_spa;
    fbe_api_terminator_device_handle_t drive_handle_p_spb;
    fbe_api_rdgen_context_t *rdgen_userzero_context_p = &jingle_bell_test_contexts[0];

    fbe_test_sep_get_active_target_id(&ori_target);
    
    mut_printf(MUT_LOG_LOW, "===Start Testing PSM LUN DualSP with glitching one system drive===");
    
    mut_printf(MUT_LOG_LOW, "Waiting for LUN object ready");
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    // open some debug flags for this RG
    status = fbe_api_raid_group_set_group_debug_flags(0xF, FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING 
                                                      | FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING
                                                      | FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    

    mut_printf(MUT_LOG_LOW, "start userzero IO to LUN");
    jingle_bell_start_userzero_io(lun_object_id, rdgen_userzero_context_p);
    fbe_api_sleep (1000);

    mut_printf(MUT_LOG_LOW, "remove one system drive 0_0_1");        
    jingle_remove_drive_and_verify(0, 0, 1, &drive_handle_p_spa, &drive_handle_p_spb);

    fbe_api_sleep (4000);

    mut_printf(MUT_LOG_LOW, "reinsert the system drive 0_0_1 back");        

    jingle_reinsert_drive_and_verify(0, 0, 1, &drive_handle_p_spa, &drive_handle_p_spb);

    mut_printf(MUT_LOG_LOW, "stop rdgen zero io");        
    status = fbe_api_rdgen_stop_tests(rdgen_userzero_context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_userzero_context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_LOW, "===Test Zero PSM LUN with glitching system drive passed===\n");
}


void jingle_remove_drive_and_verify(fbe_u32_t                           bus,
                                    fbe_u32_t                           enclosure,
                                    fbe_u32_t                           slot,
                                    fbe_api_terminator_device_handle_t* out_drive_info_p_spa,
                                    fbe_api_terminator_device_handle_t* out_drive_info_p_spb)
{
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             pvd_object_id;                      // pvd's object id
    fbe_sim_transport_connection_target_t   orig_sp;               // local SP id


    //  Get the ID of the local SP. 
    orig_sp = fbe_api_sim_transport_get_target_server();

    //  Get the PVD object id before we remove the drive
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the drive
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, out_drive_info_p_spa); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, out_drive_info_p_spb); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", bus, enclosure, slot);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    //  Verify that the PVD is in the FAIL state
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    //  Verify that the PVD is in the FAIL state
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    //  Set the target server back to the local SP. 
    fbe_api_sim_transport_set_target_server(orig_sp);

    //  Return
    return;

} 

void jingle_reinsert_drive_and_verify(fbe_u32_t                           bus,
                                      fbe_u32_t                           enclosure,
                                      fbe_u32_t                           slot,
                                      fbe_api_terminator_device_handle_t* in_drive_info_p_spa,
                                      fbe_api_terminator_device_handle_t* in_drive_info_p_spb)
{

    fbe_sim_transport_connection_target_t   orig_sp;               // local SP id
    fbe_object_id_t             pvd_object_id;                      // pvd's object id
    fbe_status_t                status;                             // fbe status
    

    //  Get the ID of the local SP.
    orig_sp = fbe_api_sim_transport_get_target_server();


    //  Get the PVD object id before we remove the drive
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, *in_drive_info_p_spa);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, *in_drive_info_p_spb);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    //  Set the target server back to the local SP. 
    fbe_api_sim_transport_set_target_server(orig_sp);

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted Drive: %d_%d_%d\n", bus, enclosure, slot);

    //  Verify that the PVD is in the READY state
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
    
    //  Return
    return;
} 

/*************************
 * end file jingle_bell_test.c
 *************************/


