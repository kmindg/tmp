/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file crayon_dualsp_test.c
 ***************************************************************************
 *
 * @brief
 *  This test is used to test the system memory configuration check with EMV in database service
 *
 * @version
 *   05/19/2012 - Created. Zhipeng Hu
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
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_trace_interface.h"

char * crayon_shin_chan_dualsp_test_short_desc = "database: system memory configuration check with EMV test.";
char * crayon_shin_chan_dualsp_test_long_desc ="\
The crayon_shin_chan_dualsp scenario tests system memory configuration check in database \n\
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
STEP 1: Write local EMV registry and set it to 4GB on both SPs\n\
            Then start system and check system boot normally\n\
STEP 2: Setup - Create initial topology.\n\
        - Create initial RGs and LUNs.\n\
STEP 3: Test dismatched SPs in ICA - Physical memory on two SPs dismatch \n\
        - Shutdown both SPs\n\
        - Generate ICA stamps on the db drives \n\
        - set local EMV registry on active side to 4GB while passive side to 8GB \n\
        - start both SPs\n\
        - verify that active SP normally boots while the passive SP enters service mode \n\
        - verify that the SharedExpectedMemroyValue registry on passive side is set to 4GB \n\
        - clear the SharedExpectedMemroyValue registry on passive SP \n\
        - shutdown passive SP , set its local EMV registry to 4GB\n\
        - reboot passive SP and verify the system boots normally \n\
STEP 4: Test Memory Upgrade - two SPs match each other and running on target SP\n\
        - Set SEMV info in db with SEMV=4GB CTM=8GB\n\
        - Shutdown both SPs\n\
        - Set local EMV registry on both sides to 8GB\n\
        - start both SPs\n\
        - verify that both SPs normally boots \n\
        - verify that the SEMV info read from db is set to 8GB and CTM is cleared \n\
STEP 5: Test Memory Upgrade - two SPs dismatch each other. Active SP running on source SP while passive side running on target SP\n\
        - Set SEMV info in db with SEMV=4GB CTM=8GB\n\
        - Shutdown both SPs\n\
        - Set local EMV registry on active side to 4GB while passive side to 8GB\n\
        - start both SPs\n\
        - verify that active SP normally boots, SEMV is still 4GB and CTM is cleared \n\
        - verify that passive SP enters service mode and SharedExpectedMemroyValue registry is set to 4GB\n\
        - clear SharedExpectedMemroyValue registry \n\
        - Shutdown SPB and set its local EMV registry to 4GB \n\
        - Boot SPB and verify that system boots normally \n\
STEP 6: Test Memory Upgrade - two SPs match each other. Both SPs running on src SP\n\
        - Set SEMV info in db with SEMV=4GB CTM=8GB\n\
        - Shutdown both SPs\n\
        - Set local EMV registry on both sides to 4GB\n\
        - start both SPs\n\
        - verify that both SP normally boots, SEMV is still 4GB and CTM is cleared \n\
\n";

/* The number of LUNs in our raid group.
 */
#define CRAYON_DUALSP_TEST_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def CRAYON_DUALSP_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define CRAYON_DUALSP_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def CRAYON_DUALSP_TEST_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define CRAYON_DUALSP_TEST_MAX_RAID_GROUP_COUNT 3

/*!*******************************************************************
 * @def CRAYON_DUALSP_TEST_DB_DRIVE_COUNT
 *********************************************************************
 * @brief system drive count - 3
 *
 *********************************************************************/
#define CRAYON_DUALSP_TEST_DB_DRIVE_COUNT 3


/*!*******************************************************************
 * @def CRAYON_DUALSP_TEST_SOURCE_SP_MEM
 *********************************************************************
 * @brief the size of the memory on source SP.
 *
 *********************************************************************/
#define CRAYON_DUALSP_TEST_SOURCE_SP_MEM 4

/*!*******************************************************************
 * @def CRAYON_DUALSP_TEST_TARGET_SP_MEM
 *********************************************************************
 * @brief the size of the memory on target SP.
 *
 *********************************************************************/
#define CRAYON_DUALSP_TEST_TARGET_SP_MEM 8


/*!*******************************************************************
 * @var crayon_dualsp_raid_groups
 *********************************************************************
 * @brief Test config.
 *
 *********************************************************************/
fbe_test_rg_configuration_t crayon_dualsp_raid_groups[CRAYON_DUALSP_TEST_MAX_RAID_GROUP_COUNT + 1] = 
{
    /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,    520,            3,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,    520,            2,          0},
    {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,    520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


/*!*******************************************************************
 * function declarations
 *********************************************************************/
extern void sep_config_load_sep_and_neit_setup_package_entries(void);

static void crayon_dualsp_load_physical_config(fbe_test_rg_configuration_t *rg_config_p);

static void crayon_dualsp_shutdown_system(fbe_sim_transport_connection_target_t target);

static void crayon_dualsp_poweron_system(fbe_sim_transport_connection_target_t target);

static void crayon_dualsp_reboot_system(fbe_sim_transport_connection_target_t target);


/*!**************************************************************
 * crayon_dualsp_load_physical_config()
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
static void crayon_dualsp_load_physical_config(fbe_test_rg_configuration_t *rg_config_p)
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
 * end crayon_dualsp_load_physical_config()
 ******************************************/


/*!**************************************************************
 * crayon_dualsp_set_local_emv_registy
 ****************************************************************
 * @brief
 *  set local emv registry value for both SPs.
 *
 * @param if the input param is 0, do not set local emv registry for this SP.               
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_set_local_emv_registy(fbe_u32_t spa_lemv, fbe_u32_t spb_lemv)
{
    fbe_sim_transport_connection_target_t   org_sp;
    fbe_status_t    status;

    /*reserve the original target server*/
    org_sp = fbe_api_sim_transport_get_target_server();

    if(spa_lemv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "set_local_emv_registy: set target server to SPA fail");
        
        status = fbe_test_esp_registry_write(K10DriverConfigRegPath,
                                    K10EXPECTEDMEMORYVALUEKEY, 
                                    FBE_REGISTRY_VALUE_DWORD,
                                    &spa_lemv, 
                                    sizeof(fbe_u32_t));
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "set_local_emv_registy: set lemv on SPA fail");
    }
    
    if(spb_lemv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "set_local_emv_registy: set target server to SPB fail");
        
        status = fbe_test_esp_registry_write(K10DriverConfigRegPath,
                                    K10EXPECTEDMEMORYVALUEKEY, 
                                    FBE_REGISTRY_VALUE_DWORD,
                                    &spb_lemv, 
                                    sizeof(fbe_u32_t));
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "set_local_emv_registy: set lemv on SPB fail");
    }
    
    fbe_api_sim_transport_set_target_server(org_sp);
}

/*!**************************************************************
 * crayon_dualsp_get_local_emv_registy
 ****************************************************************
 * @brief
 *  get local emv registry value for both SPs.
 *
 * @param if the input param is NULL, do not get local emv registry for this SP.     
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_get_local_emv_registy(fbe_u32_t *spa_lemv, fbe_u32_t *spb_lemv)
{
    fbe_sim_transport_connection_target_t   org_sp;
    fbe_u32_t DefaultInputValue = 0;
    fbe_status_t    status;

    /*reserve the original target server*/
    org_sp = fbe_api_sim_transport_get_target_server();

    if(NULL != spa_lemv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_local_emv_registy: set target server to SPA fail");
        
        status = fbe_test_esp_registry_read(K10DriverConfigRegPath,
                                                        K10EXPECTEDMEMORYVALUEKEY, 
                                                        spb_lemv, 
                                                        sizeof(fbe_u32_t), 
                                                        FBE_REGISTRY_VALUE_DWORD, 
                                                        &DefaultInputValue, 
                                                        sizeof(fbe_u32_t), 
                                                        FALSE);

        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_local_emv_registy: get lemv on SPA fail");
    }
    
    if(NULL != spb_lemv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_local_emv_registy: set target server to SPB fail");
        
        status = fbe_test_esp_registry_read(K10DriverConfigRegPath,
                                                        K10EXPECTEDMEMORYVALUEKEY, 
                                                        spb_lemv, 
                                                        sizeof(fbe_u32_t), 
                                                        FBE_REGISTRY_VALUE_DWORD, 
                                                        &DefaultInputValue, 
                                                        sizeof(fbe_u32_t), 
                                                        FALSE);

        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_local_emv_registy: get lemv on SPB fail");
    }
    
    fbe_api_sim_transport_set_target_server(org_sp);
}

/*!**************************************************************
 * crayon_dualsp_get_shared_emv_registy
 ****************************************************************
 * @brief
 *  get shared emv registry value for both SPs.
 *
 * @param if the input param is NULL, do not get shared emv registry for this SP.     
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_get_shared_emv_registy(fbe_u32_t *spa_semv, fbe_u32_t *spb_semv)
{
    fbe_sim_transport_connection_target_t   org_sp;
    fbe_u32_t DefaultInputValue = 0;
    fbe_status_t    status;

    /*reserve the original target server*/
    org_sp = fbe_api_sim_transport_get_target_server();

    if(NULL != spa_semv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_shared_emv_registy: set target server to SPA fail");
        
        status = fbe_test_esp_registry_read(K10DriverConfigRegPath,
                                                        K10SHAREDEXPECTEDMEMORYVALUEKEY, 
                                                        spa_semv, 
                                                        sizeof(fbe_u32_t), 
                                                        FBE_REGISTRY_VALUE_DWORD, 
                                                        &DefaultInputValue, 
                                                        sizeof(fbe_u32_t), 
                                                        FALSE);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_shared_emv_registy: get semv on SPA fail");
    }
    
    if(NULL != spb_semv)
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_shared_emv_registy: set target server to SPB fail");
        
        status = fbe_test_esp_registry_read(K10DriverConfigRegPath,
                                                        K10SHAREDEXPECTEDMEMORYVALUEKEY, 
                                                        spb_semv, 
                                                        sizeof(fbe_u32_t), 
                                                        FBE_REGISTRY_VALUE_DWORD, 
                                                        &DefaultInputValue, 
                                                        sizeof(fbe_u32_t), 
                                                        FALSE);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "get_shared_emv_registy: get semv on SPB fail");
    }
    
    fbe_api_sim_transport_set_target_server(org_sp);
}

/*!**************************************************************
 * crayon_dualsp_step2_create_topology()
 ****************************************************************
 * @brief
 *  create initial user raid groups and user luns.
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_step2_create_topology(void)
{
    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(&crayon_dualsp_raid_groups[0],
                                                      CRAYON_DUALSP_TEST_MAX_RAID_GROUP_COUNT,
                                                      CRAYON_DUALSP_TEST_CHUNKS_PER_LUN, 
                                                      CRAYON_DUALSP_TEST_LUNS_PER_RAID_GROUP);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(&crayon_dualsp_raid_groups[0], CRAYON_DUALSP_TEST_MAX_RAID_GROUP_COUNT);
}

/*!**************************************************************
 * crayon_dualsp_step3_dismatch_phy_mem_in_ica()
 ****************************************************************
 * @brief
 *  crayon_dualsp_step3_dismatch_phy_mem_in_ica
 * - Shutdown both SPs
 * - Generate ICA stamps on the db drives 
 * - set local EMV registry on active side to 4GB while passive side to 8GB 
 * - start both SPs
 * - verify that active SP normally boots while the passive SP enters service mode 
 * - verify that the SharedExpectedMemroyValue registry on passive side is set to 4GB 
 * - clear the SharedExpectedMemroyValue registry on passive SP 
 * - shutdown passive SP , set its local EMV registry to 4GB
 * - reboot passive SP and verify the system boots normally
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_step3_dismatch_phy_mem_in_ica(void)
{
    fbe_status_t    status;
    fbe_u32_t   slot;
    fbe_u32_t   emv;
    fbe_object_id_t pdo_id;
    fbe_database_state_t   db_state;

    /*shutdown both SPs*/
    crayon_dualsp_shutdown_system(FBE_SIM_SP_A);
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);

    /*generate ica stamps so do ICA*/
    for (slot = 0; slot < CRAYON_DUALSP_TEST_DB_DRIVE_COUNT; slot++) 
    {
        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &pdo_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: fail get pdo obj id");
        if(FBE_OBJECT_ID_INVALID == pdo_id)
            continue;
        
        status = fbe_api_physical_drive_interface_generate_ica_stamp(pdo_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: generate ica stamp for db failed");
    }

    /*set local EMV registry on active side to 4GB while passive side to 8GB, simulates SPID behavior*/
    crayon_dualsp_set_local_emv_registy(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, CRAYON_DUALSP_TEST_TARGET_SP_MEM);

    /*start both SPs*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_A);
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);

    /*verify now*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: get db state from SPA failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "step3_dismatch_phy_mem_in_ica: expected DB state != actual DB state on active side");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_SERVICE_MODE, db_state, "step3_dismatch_phy_mem_in_ica: expected DB state != actual DB state on passive side");

    crayon_dualsp_get_shared_emv_registy(NULL, &emv); /*check whether MCR sets the SharedExpectedMemoryValue registry*/
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, emv, "step3_dismatch_phy_mem_in_ica: expected semv registry != actual semv registry");

    /*shutdown SPB, set its local EMV registry to 4GB (simulate k10_software_start.pl behavior)*/
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0); /*reset error reported by database_system_memory_configuration_check before entering service mode*/
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: fail reset error counter for sep");
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    crayon_dualsp_set_local_emv_registy(0, CRAYON_DUALSP_TEST_SOURCE_SP_MEM);

    /*power on passive SP*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);
    
    /*verify now*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "step3_dismatch_phy_mem_in_ica: expected DB state  != actual DB state on passive side");
}

/*!**************************************************************
 * crayon_dualsp_step4_normal_conversion_on_target_sp()
 ****************************************************************
 * @brief
 *  crayon_dualsp_step4_normal_conversion_on_target_sp
 * - Set SEMV info in db with SEMV=4GB CTM=8GB
 * - Shutdown both SPs
 * - Set local EMV registry on both sides to 8GB
 * - start both SPs
 * - verify that both SPs normally boots 
 * - verify that the SEMV info read from db is set to 8GB and CTM is cleared
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  05/21/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_step4_normal_conversion_on_target_sp(void)
{
    fbe_status_t    status;
    fbe_database_state_t   db_state;

    union
    {
        fbe_database_emv_t  shared_emv_info;
        struct
        {
            fbe_u32_t   opaque_emv;
            fbe_u32_t   ctm;
        }emv_info_seg;
    }emv_info;

    /*set shared expected memory information in db with SEMV = 4GB and CTM = 8GB*/
    /*simulates IFC package behavior*/
    emv_info.emv_info_seg.opaque_emv = 4;
    emv_info.emv_info_seg.ctm = 8;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on active side");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on passive side");

    /*shutdown both SPs*/
    crayon_dualsp_shutdown_system(FBE_SIM_SP_A);
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);

    /*set local EMV registry on both sides to 8GB, simulates SPID behavior*/
    crayon_dualsp_set_local_emv_registy(CRAYON_DUALSP_TEST_TARGET_SP_MEM, CRAYON_DUALSP_TEST_TARGET_SP_MEM);

    /*start both SPs*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_A);
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);

    /*verify now*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: get db state from SPA failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "step4_normal_conversion_on_target_sp: expected DB state != actual DB state on active side");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "step4_normal_conversion_on_target_sp: expected DB state != actual DB state on passive side");


    /*verify that the SEMV info read from db is set to 8GB and CTM is cleared*/
    fbe_zero_memory(&emv_info, sizeof(emv_info));
    status = fbe_api_database_get_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: get shared emv info from db failed");
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_TARGET_SP_MEM, emv_info.emv_info_seg.opaque_emv, 
                                                        "step4_normal_conversion_on_target_sp: expected semv != actual semv");
    MUT_ASSERT_INT_EQUAL_MSG(0, emv_info.emv_info_seg.ctm, 
                                                        "step4_normal_conversion_on_target_sp: expected ctm != actual ctm");
    
}

/*!**************************************************************
 * crayon_dualsp_step5_conversion_active_source_passive_target()
 ****************************************************************
 * @brief
 *  crayon_dualsp_step5_conversion_active_source_passive_target
 *  Active SP running on source SP while passive side running on target SP
 * - Set SEMV info in db with SEMV=4GB CTM=8GB
 * - Shutdown both SPs
 * - Set local EMV registry on active side to 4GB while passive side to 8GB
 * - start both SPs
 * - verify that active SP normally boots, SEMV is still 4GB and CTM is cleared 
 * - verify that passive SP enters service mode and SharedExpectedMemroyValue registry is set to 4GB
 * - clear SharedExpectedMemroyValue registry.
 * - Shutdown SPB and set its local EMV registry to 4GB 
 * - Boot SPB and verify that system boots normally
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  05/21/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_step5_conversion_active_source_passive_target(void)
{
    fbe_status_t    status;
    fbe_u32_t   emv;
    fbe_database_state_t   db_state;

    union
    {
        fbe_database_emv_t  shared_emv_info;
        struct
        {
            fbe_u32_t   opaque_emv;
            fbe_u32_t   ctm;
        }emv_info_seg;
    }emv_info;

    /*set shared expected memory information in db with SEMV = 4GB and CTM = 8GB*/
    /*simulates IFC package behavior*/
    emv_info.emv_info_seg.opaque_emv = 4;
    emv_info.emv_info_seg.ctm = 8;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on active side");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on passive side");

    /*shutdown both SPs*/
    crayon_dualsp_shutdown_system(FBE_SIM_SP_A);
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);

    /*set local EMV registry on active side to 4GB while passive side to 8GB, simulates SPID behavior*/
    crayon_dualsp_set_local_emv_registy(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, CRAYON_DUALSP_TEST_TARGET_SP_MEM);

    /*start both SPs*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_A);
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);

    /*verify now*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step5_conversion_active_source_passive_target: get db state from SPA failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "crayon_dualsp_step5_conversion_active_source_passive_target: expected DB state != actual DB state on active side");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step5_conversion_active_source_passive_target: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_SERVICE_MODE, db_state, "crayon_dualsp_step5_conversion_active_source_passive_target: expected DB state != actual DB state on passive side");

    crayon_dualsp_get_shared_emv_registy(NULL, &emv); /*check whether MCR sets the SharedExpectedMemoryValue registry*/
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, emv, "step3_dismatch_phy_mem_in_ica: expected semv registry != actual semv registry");

    /*verify that the SEMV info read from db is set to 8GB and CTM is cleared*/
    fbe_zero_memory(&emv_info, sizeof(emv_info));
    status = fbe_api_database_get_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step5_conversion_active_source_passive_target: get shared emv info from db failed");
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, emv_info.emv_info_seg.opaque_emv, 
                                                        "crayon_dualsp_step5_conversion_active_source_passive_target: expected semv != actual semv");
    MUT_ASSERT_INT_EQUAL_MSG(0, emv_info.emv_info_seg.ctm, 
                                                        "crayon_dualsp_step5_conversion_active_source_passive_target: expected ctm != actual ctm");

    /*Shutdown SPB*/
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0); /*reset error reported by database_system_memory_configuration_check before entering service mode*/
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step3_dismatch_phy_mem_in_ica: fail reset error counter for sep");
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);

    /*set its local EMV to 4GB, simulates the k10_software_start.pl behavior*/
    crayon_dualsp_set_local_emv_registy(0, CRAYON_DUALSP_TEST_SOURCE_SP_MEM);

    /*Boot SPB and verify that it is in READY state now*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step5_conversion_active_source_passive_target: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "crayon_dualsp_step5_conversion_active_source_passive_target: expected DB state != actual DB state on passive side");
    
}


/*!**************************************************************
 * crayon_dualsp_step6_normal_conversion_on_source_sp()
 ****************************************************************
 * @brief
 *  crayon_dualsp_step6_normal_conversion_on_source_sp
 *  two SPs match each other. Both SPs running on src SP
 * - Set SEMV info in db with SEMV=4GB CTM=8GB
 * - Shutdown both SPs
 * - Set local EMV registry on both sides to 4GB
 * - start both SPs
 * - verify that both SP normally boots, SEMV is still 4GB and CTM is cleared 
 *
 * @param none.               
 *
 * @return None.
 *
 * @author
 *  05/21/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_step6_normal_conversion_on_source_sp(void)
{
    fbe_status_t    status;
    fbe_database_state_t   db_state;

    union
    {
        fbe_database_emv_t  shared_emv_info;
        struct
        {
            fbe_u32_t   opaque_emv;
            fbe_u32_t   ctm;
        }emv_info_seg;
    }emv_info;

    /*set shared expected memory information in db with SEMV = 4GB and CTM = 8GB*/
    /*simulates IFC package behavior*/
    emv_info.emv_info_seg.opaque_emv = 4;
    emv_info.emv_info_seg.ctm = 8;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on active side");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "step4_normal_conversion_on_target_sp: fail set semv info in db on passive side");

    /*shutdown both SPs*/
    crayon_dualsp_shutdown_system(FBE_SIM_SP_A);
    crayon_dualsp_shutdown_system(FBE_SIM_SP_B);

    /*set local EMV registry on both sides to 4GB, simulates SPID behavior*/
    crayon_dualsp_set_local_emv_registy(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, CRAYON_DUALSP_TEST_SOURCE_SP_MEM);

    /*start both SPs*/
    crayon_dualsp_poweron_system(FBE_SIM_SP_A);
    crayon_dualsp_poweron_system(FBE_SIM_SP_B);

    /*verify now*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step6_normal_conversion_on_source_sp: get db state from SPA failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "crayon_dualsp_step6_normal_conversion_on_source_sp: expected DB state != actual DB state on active side");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&db_state);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step6_normal_conversion_on_source_sp: get db state from SPB failed");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_READY, db_state, "crayon_dualsp_step6_normal_conversion_on_source_sp: expected DB state != actual DB state on passive side");

    /*verify that the SEMV info read from db is set to 4GB and CTM is cleared*/
    fbe_zero_memory(&emv_info, sizeof(emv_info));
    status = fbe_api_database_get_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "crayon_dualsp_step6_normal_conversion_on_source_sp: get shared emv info from db failed");
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, emv_info.emv_info_seg.opaque_emv, 
                                                        "crayon_dualsp_step6_normal_conversion_on_source_sp: expected semv != actual semv");
    MUT_ASSERT_INT_EQUAL_MSG(0, emv_info.emv_info_seg.ctm, 
                                                        "crayon_dualsp_step6_normal_conversion_on_source_sp: expected ctm != actual ctm");    
}


/*!**************************************************************
 * crayon_dualsp_shutdown_system()
 ****************************************************************
 * @brief
 *  shutdown the target SP.
 *
 * @param target - target SP that we want to shutdown.               
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_shutdown_system(fbe_sim_transport_connection_target_t target)
{
    fbe_sim_transport_connection_target_t   original_target;
    original_target = fbe_api_sim_transport_get_target_server();
    
    /*shutdown sep*/
    fbe_api_sim_transport_set_target_server(target);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * crayon_dualsp_poweron_system()
 ****************************************************************
 * @brief
 *  power-on the target SP.
 *
 * @param target - target SP that we want to power-on.               
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void crayon_dualsp_poweron_system(fbe_sim_transport_connection_target_t target)
{
    fbe_sim_transport_connection_target_t   original_target;
    fbe_status_t    status;
    
    original_target = fbe_api_sim_transport_get_target_server();
    
    /*reboot sep*/
    fbe_api_sim_transport_set_target_server(target);

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_sep_and_neit_setup_package_entries();
    fbe_test_sep_util_set_default_io_flags();  // some default flags was removed from previous function
    status = fbe_test_sep_util_wait_for_database_service(20000);
    
    fbe_api_sim_transport_set_target_server(original_target);
}

/*!**************************************************************
 * crayon_load_esp_sep_and_neit_packages()
 ****************************************************************
 * @brief
 *  load esp, sep and neit packages
 *  right after loading esp, we will create and write local EMV registry for both SPs
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  05/19/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
static void  crayon_load_esp_sep_and_neit_packages(void)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t target;

    target = fbe_api_sim_transport_get_target_server();

    fbe_test_util_create_registry_file();

    /*set local emv registries, simulates SPID behavior*/
    if(FBE_SIM_SP_A == target)
        crayon_dualsp_set_local_emv_registy(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, 0);
    else
        crayon_dualsp_set_local_emv_registy(0, CRAYON_DUALSP_TEST_SOURCE_SP_MEM);

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}

static void crayon_first_start_system(void)
{

    fbe_status_t status;

    /* Start loading SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    crayon_load_esp_sep_and_neit_packages();

    /* Start loading SP B in parallel.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    crayon_load_esp_sep_and_neit_packages();

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_setup_package_entries();
    fbe_test_sep_util_set_default_io_flags();  // some default flags was removed from previous function
    status = fbe_test_sep_util_wait_for_database_service(20000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_setup_package_entries();
    fbe_test_sep_util_set_default_io_flags();  // some default flags was removed from previous function
    status = fbe_test_sep_util_wait_for_database_service(20000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}

/*!**************************************************************
 * crayon_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup the test configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/19/2012 - Created
 *
 ****************************************************************/
void crayon_shin_chan_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for crayon dualsp test ===\n");

    fbe_test_sep_util_init_rg_configuration_array(&crayon_dualsp_raid_groups[0]);
    
    /* set the target server */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
    /* make sure target set correctly */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
    /* Load the physical configuration */
    mut_printf(MUT_LOG_TEST_STATUS, "=== load pp for SPB ===\n");
    crayon_dualsp_load_physical_config(&crayon_dualsp_raid_groups[0]);
    
    /* set the target server */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    /* make sure target set correctly */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== load pp for SPA ===\n");
    crayon_dualsp_load_physical_config(&crayon_dualsp_raid_groups[0]);    

    /* Load SEP and NEIT*/    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load sep and neit packages */
    mut_printf(MUT_LOG_TEST_STATUS, "=== load SEP and NEIT in SPA ===\n");
    //crayon_first_start_system();

    sep_config_load_sep_and_neit_both_sps();

    return;
}
/**************************************
 * end crayon_dualsp_setup()
 **************************************/
 
/*!**************************************************************
 * crayon_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  cleanup the test configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/19/2012 - Created
 *
 ****************************************************************/
void crayon_shin_chan_dualsp_cleanup(void)
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
 * crayon_dualsp_test()
 ****************************************************************
 * @brief
 *  dualsp test entry.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/19/2012 - Created
 *
 ****************************************************************/
void  crayon_shin_chan_dualsp_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    union
    {
        fbe_database_emv_t  shared_emv_info;
        struct
        {
            fbe_u32_t   opaque_emv;
            fbe_u32_t   ctm;
        }emv_info_seg;
    }emv_info;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /*Verify the shared emv is set correctly*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "main test: get emv failed.");
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, 
                                                        emv_info.emv_info_seg.opaque_emv, 
                                                        "main test: get emv != expected emv");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_emv_params(&emv_info.shared_emv_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "main test: get emv failed.");
    MUT_ASSERT_INT_EQUAL_MSG(CRAYON_DUALSP_TEST_SOURCE_SP_MEM, 
                                                        emv_info.emv_info_seg.opaque_emv, 
                                                        "main test: get emv != expected emv");

    /* step 2: Setup - Bring up initial topology.
     * - Create user rgs and luns*/
     mut_printf(MUT_LOG_TEST_STATUS, "main test 2: now bring up initial topology");
     crayon_dualsp_step2_create_topology();

    /* step 3: crayon_dualsp_step4_normal_conversion_on_target_sp.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "main test 3: Test dismatched SPs in ICA - Physical memory on two SPs dismatch");
    crayon_dualsp_step3_dismatch_phy_mem_in_ica();

    /* step 4: Test Memory Upgrade - two SPs match each other and running on target SP.
     */
    crayon_dualsp_step4_normal_conversion_on_target_sp();

    /* step 5: Test Memory Upgrade - two SPs dismatch each other. Active SP running on source SP while passive side running on target SP.
     */
    crayon_dualsp_step5_conversion_active_source_passive_target();

    /* step 6: Test Memory Upgrade - two SPs match each other. Both SPs running on src SP.
     */
    crayon_dualsp_step6_normal_conversion_on_source_sp();
    

    return;
}
/*************************
 * end file crayon_dualsp_test.c
 *************************/



