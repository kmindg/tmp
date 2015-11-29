/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file pingu_slot_limit_test.c
 ***************************************************************************
 *
 * @brief
 *   This file tests physical fru limits in the database service
 *
 * @version
 *   03/10/2014 - Created.  Wayne Garrett
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_base_environment_debug.h"
#include "fbe_test_esp_utils.h"
#include "EspMessages.h"


/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char * pingu_slot_limit_short_desc = "Database service physical fru limit test";
char * pingu_slot_limit_long_desc = 
"Configures an SP of type HELLCAT_LITE, which has max slot limit of 75.\n\
Adds viper enclosures until max limit is hit \n";


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
#define SLOT_LIMIT_PHYSICAL_CONFIG_PORTS 2
#define SLOT_LIMIT_PHYSICAL_CONFIG_ENCLOSURES 5

/*!*******************************************************************
 * @def PINGU_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define PINGU_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @def PINGU_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define PINGU_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def PINGU_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define PINGU_CHUNKS_PER_LUN 3


/*-----------------------------------------------------------------------------
     GLOBALS 
*/
typedef enum pingu_drive_fault_type_e
{
    PINGU_DRIVE_FAULT_NONE,
    PINGU_DRIVE_FAULT_PERSITED,
    PINGU_DRIVE_FAULT_LINK
}pingu_drive_fault_type_t;

/*-----------------------------------------------------------------------------
    EXTERN:
*/
extern fbe_test_rg_configuration_array_t biff_raid_group_config_qual[];

extern void         biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void         biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_bool_t wait_for_destroy);
extern fbe_status_t biff_get_pvd_connected_to_pdo(fbe_object_id_t pdo, fbe_object_id_t *pvd, fbe_u32_t timeout_ms);
extern void         sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, fbe_u32_t death_reason);
extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/
void pingu_insert_max_slot_replace_many_drives(fbe_u32_t bus, fbe_u32_t encl, pingu_drive_fault_type_t drive_fault);
void pingu_remove_drives_and_insert_new_enclosure(void);
void pingu_insert_max_slot_limit_then_replace_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, pingu_drive_fault_type_t drive_fault);
void pingu_insert_max_slot_limit_then_replace_rg_drive(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void pingu_slot_limit_test_insert_too_many_drives(void);
void pingu_slot_limit_add_viper_enclosure(fbe_u32_t port_loc, fbe_u32_t encl_loc);
void pingu_slot_limit_remove_enclosure(fbe_u32_t port_loc, fbe_u32_t encl_loc);
void pingu_slot_verify_event_log_encl_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u8_t* from_state_str, fbe_u8_t* to_state_str);
void pingu_slot_limit_verify_pvd_not_connected_to_pdo_both_sp(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void pingu_slot_limit_create_physical_config(void);

/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/


/*!****************************************************************************
 * pingu_slot_limit_test()
 ******************************************************************************
 * @brief
 *  This is the main entry point for the pingu_slot_limit test  
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  03/10/2014 - Created. Wayne Garrett
 ******************************************************************************/

void pingu_slot_limit_dualsp_test(void)
{    
    //fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    fbe_test_rg_configuration_t *rg_config_p;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_u32_t                   num_drives = 0;
    fbe_u32_t                   max_slot_limit = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s START ===", __FUNCTION__); 

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* 0 - verify that setup fully populated max slot limit. */
    fbe_api_esp_drive_mgmt_get_total_drives_count(&num_drives);
    fbe_api_esp_drive_mgmt_get_max_platform_drive_count(&max_slot_limit);
    MUT_ASSERT_INT_EQUAL(num_drives, max_slot_limit);
    MUT_ASSERT_INT_NOT_EQUAL(num_drives, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "%s max_slot_limit=%d", __FUNCTION__, max_slot_limit);     

    /* 1 - test with spare */
    pingu_insert_max_slot_replace_many_drives(0, 1, PINGU_DRIVE_FAULT_NONE);
    pingu_insert_max_slot_replace_many_drives(0, 1, PINGU_DRIVE_FAULT_PERSITED);
    pingu_insert_max_slot_replace_many_drives(0, 1, PINGU_DRIVE_FAULT_LINK);

    /* 2 - test with various rg types */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);
    rg_config_p = &biff_raid_group_config_qual[0][0];
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                   raid_type_string_p, rg_config_p->raid_type);
        MUT_FAIL();
    }
    raid_group_count = fbe_test_get_rg_count(rg_config_p);

    if (raid_group_count == 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid_group_count==0, raid type %s.  Dont't forget to fbe_test_sep_util_init_rg_configuration_array()", 
                   raid_type_string_p);
        MUT_FAIL();
    }

   /* Now create the raid groups and run the tests
    */
   fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                  pingu_insert_max_slot_limit_then_replace_rg_drive,
                                  PINGU_LUNS_PER_RAID_GROUP,
                                  PINGU_CHUNKS_PER_LUN,FBE_FALSE);

   fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);


    /* 3 - test removing multiple drives then inserting new enclosure */
    pingu_remove_drives_and_insert_new_enclosure();


    mut_printf(MUT_LOG_TEST_STATUS, "=== %s END ===", __FUNCTION__); 
}


/*!****************************************************************************
 *  pingu_slot_limit_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the pingu_slot_limit test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 03/10/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pingu_slot_limit_dualsp_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for pingu_slot_limit test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;

        array_p = &biff_raid_group_config_qual[0];
        for (test_index = 0; test_index < PINGU_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        pingu_slot_limit_create_physical_config();
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        pingu_slot_limit_create_physical_config();
       
        sep_config_load_esp_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    /* Reduce debouncer time to speed up test. */
    fbe_api_database_set_garbage_collection_debouncer(5000);
    
    return;

} /* End pingu_slot_limit_test_setup() */


/*!****************************************************************************
 *  pingu_slot_limit_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Bubble Bass test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 03/10/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pingu_slot_limit_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for pingu_slot_limit test ===\n");

    if(fbe_test_util_is_simulation())
    {
        /* Turn off delay for I/O completion in the terminator */
        fbe_api_terminator_set_io_global_completion_delay(0);

        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }

    return;
} 

void pingu_insert_max_slot_replace_many_drives(fbe_u32_t bus, fbe_u32_t encl, pingu_drive_fault_type_t drive_fault)
{
    fbe_object_id_t pdo, pvd;
    fbe_status_t status;    
    fbe_u32_t s, i;
    fbe_u32_t num_drives=10;
    fbe_sim_transport_connection_target_t this_sp, other_sp, sp; 

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 
    

    mut_printf(MUT_LOG_TEST_STATUS, "%s START", __FUNCTION__);

    if (drive_fault == PINGU_DRIVE_FAULT_PERSITED)
    {       
        /* fail all drives in enclosure */
        for (s=0; s<num_drives; s++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Failing %d_%d_%d persisted fault", __FUNCTION__, bus, encl, s);

            fbe_api_get_physical_drive_object_id_by_location(bus, encl, s, &pdo); 
            fbe_api_physical_drive_fail_drive(pdo, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED);
        
            status = fbe_test_pp_util_verify_pdo_state(bus, encl, s, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
            status = fbe_test_sep_drive_verify_pvd_state(bus, encl, s, FBE_LIFECYCLE_STATE_FAIL, 10000);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        //fbe_api_sleep(3000);
    }
    else if (drive_fault == PINGU_DRIVE_FAULT_LINK)
    {
        /* fail all drives in enclosure */
        for (s=0; s<num_drives; s++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Failing %d_%d_%d link fault", __FUNCTION__, bus, encl, s);
    
            /* fail drive then replace */
            fbe_api_get_physical_drive_object_id_by_location(bus, encl, s, &pdo); 
            fbe_api_physical_drive_fail_drive(pdo, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED);
    
            status = fbe_test_pp_util_verify_pdo_state(bus, encl, s, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d connected to pdo:0x%x", __FUNCTION__, bus, encl, s, pdo);
            status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 70000 /*ms*/);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d 0x%x is ready", __FUNCTION__, bus, encl, s, pvd);
            status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);                   
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        //fbe_api_sleep(3000);
    }


    /* remove and insert a new drive without trigger PVD garbage collection.  This
       will cause us to exceeded database fru limit. It's expected that db service
       will not connect PDOs edge. */    
    mut_printf(MUT_LOG_TEST_STATUS, "%s removing drives from %d_%d", __FUNCTION__, bus, encl);
    for (s=0; s<num_drives; s++)
    {
        biff_remove_drive(bus, encl, s, FBE_FALSE);
    }
    for (s=0; s<num_drives; s++)
    {
        /* verify objects are destroyed */
        status = sleeping_beauty_wait_for_state(bus, encl, s, FBE_LIFECYCLE_STATE_DESTROY, 10000 /*ms*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    fbe_api_sleep(1000);


    for (s=0; s<num_drives; s++)
    {
        /* insert drive on both sides */
        biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520, NULL, bus, encl, s);
    }
    

    //fbe_api_sleep(3000);   

    for (i = 0; i<2; i++)
    {
        sp = (i==0)?this_sp:other_sp;
        fbe_api_sim_transport_set_target_server(sp);
     
        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying inserted drives on SP%s", __FUNCTION__, (sp==FBE_SIM_SP_A)?"A":"B");
           
        for (s=0; s<num_drives; s++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PDO %d_%d_%d ready", __FUNCTION__, bus, encl, s);
            status = fbe_test_pp_util_verify_pdo_state(bus, encl, s, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
                
        for (s=0; s<num_drives; s++)
        {            
            fbe_api_get_physical_drive_object_id_by_location(bus, encl, s, &pdo); 

            mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d connected to pdo:0x%x", __FUNCTION__, bus, encl, s, pdo);
            status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 70000 /*ms*/);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
            mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d 0x%x is ready", __FUNCTION__, bus, encl, s, pvd);
            status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);        
            //status = fbe_test_sep_drive_verify_pvd_state(bus, encl, s, FBE_LIFECYCLE_STATE_READY, 10000);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    fbe_api_sim_transport_set_target_server(this_sp); /* restore */

    //fbe_api_sleep(3000);

    mut_printf(MUT_LOG_TEST_STATUS, "%s END\n", __FUNCTION__);
}

/*!****************************************************************************
 *  pingu_remove_drives_and_insert_new_enclosure
 ******************************************************************************
 *
 * @brief
 *  This will remove multiple drives then insert a new enclosure.   Verify
 *  we don't exceed max fru limit.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 04/29/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pingu_remove_drives_and_insert_new_enclosure(void)
{    
    fbe_u32_t bus_to_remove_from = 1;
    fbe_u32_t encl_to_remove_from = 2;
    fbe_u32_t num_drives_to_remove = 5;
    fbe_u32_t new_encl_bus = SLOT_LIMIT_PHYSICAL_CONFIG_PORTS-1;
    fbe_u32_t new_encl_loc = SLOT_LIMIT_PHYSICAL_CONFIG_ENCLOSURES;
    fbe_u32_t s;
    fbe_u32_t pvds_connected = 0;
    fbe_object_id_t pdo, pvd;
    fbe_status_t status;    
    fbe_sim_transport_connection_target_t this_sp, other_sp; 

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 
   

    /* remove drives 
    */
    mut_printf(MUT_LOG_TEST_STATUS, "%s removing drives from %d_%d", __FUNCTION__, bus_to_remove_from, encl_to_remove_from);
    for (s=0; s<num_drives_to_remove; s++)
    {
        biff_remove_drive(bus_to_remove_from, encl_to_remove_from, s, FBE_FALSE);
    }
    for (s=0; s<num_drives_to_remove; s++)
    {
        /* verify objects are destroyed */
        status = sleeping_beauty_wait_for_state(bus_to_remove_from, encl_to_remove_from, s, FBE_LIFECYCLE_STATE_DESTROY, 10000 /*ms*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    fbe_api_sleep(1000);

    /* insert new encl
    */
    pingu_slot_limit_add_viper_enclosure(new_encl_bus, new_encl_loc);

    /* verify all PDOs are ready. 
    */
    mut_printf(MUT_LOG_TEST_STATUS, "%s verifying inserted drives", __FUNCTION__);

    for (s=0; s<15; s++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PDO %d_%d_%d ready", __FUNCTION__, new_encl_bus, new_encl_loc, s);
        status = fbe_test_pp_util_verify_pdo_state(new_encl_bus, new_encl_loc, s, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    fbe_api_sleep(10000);

    /* verify same number of drives removed are connect from new enclosure */
    pvds_connected = 0;
    for (s=0; s<15; s++)
    {            
        fbe_api_get_physical_drive_object_id_by_location(new_encl_bus, new_encl_loc, s, &pdo); 
       
        status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 0/*ms*/);
        if (status == FBE_STATUS_OK)
        {
            pvds_connected++;
            mut_printf(MUT_LOG_TEST_STATUS, "%s PVD %d_%d_%d 0x%x connected to pdo:0x%x. Verifying ready", __FUNCTION__, new_encl_bus, new_encl_loc, s, pvd, pdo);
            status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);        
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s PVD %d_%d_%d NOT connected to pdo:0x%x", __FUNCTION__, new_encl_bus, new_encl_loc, s, pdo);
        }
    }
    MUT_ASSERT_INT_EQUAL(pvds_connected, num_drives_to_remove);


    /* cleanup */
    pingu_slot_limit_remove_enclosure(new_encl_bus, new_encl_loc);
}


/*!****************************************************************************
 *  pingu_insert_max_slot_limit_then_replace_drive
 ******************************************************************************
 *
 * @brief
 *  This will insert too many drives and verify that database will not
 *  connect if it has already reached its capacity.
 *
 * @param   bus, encl, slot : drive location
 *          drive_fault : indicates if drive should be failed before removing and fault type.
 *
 * @return  None 
 *
 * @author
 * 03/10/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pingu_insert_max_slot_limit_then_replace_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, pingu_drive_fault_type_t drive_fault)
{
    fbe_object_id_t pdo, pvd;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s START", __FUNCTION__);


    if (drive_fault == PINGU_DRIVE_FAULT_PERSITED)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Failing %d_%d_%d persisted fault", __FUNCTION__, bus, encl, slot);

        /* fail drive then replace */
        fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo); 
        fbe_api_physical_drive_fail_drive(pdo, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED);
    
        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    
        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d connected to pdo:0x%x", __FUNCTION__, bus, encl, slot, pdo);
        status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 70000 /*ms*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d 0x%x is failed", __FUNCTION__, bus, encl, slot, pvd);
        status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_FAIL, 10000, FBE_PACKAGE_ID_SEP_0);                   
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    
        //fbe_api_sleep(3000);
    }
    else if (drive_fault == PINGU_DRIVE_FAULT_LINK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Failing %d_%d_%d link fault", __FUNCTION__, bus, encl, slot);

        /* fail drive then replace */
        fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo); 
        fbe_api_physical_drive_fail_drive(pdo, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED);

        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d connected to pdo:0x%x", __FUNCTION__, bus, encl, slot, pdo);
        status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 70000 /*ms*/);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d 0x%x is ready", __FUNCTION__, bus, encl, slot, pvd);
        status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);                   
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        //fbe_api_sleep(3000);
    }


    /* remove and insert a new drive without trigger PVD garbage collection.  This
       will cause us to exceeded database fru limit. It's expected that db service
       will not connect PDOs edge. */    
    biff_remove_drive(bus, encl, slot, FBE_TRUE);
    fbe_api_sleep(3000);
    biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520, NULL, bus, encl, slot);
    
    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo); 
    status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 130000 /*ms*/);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "pvd not found");

    mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d connected to pdo:0x%x", __FUNCTION__, bus, encl, slot, pdo);
    status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 70000 /*ms*/);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s verifying PVD %d_%d_%d 0x%x is ready", __FUNCTION__, bus, encl, slot, pvd);
    status = fbe_api_wait_for_object_lifecycle_state(pvd, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);                   
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //fbe_api_sleep(3000);

    mut_printf(MUT_LOG_TEST_STATUS, "%s END\n", __FUNCTION__);
}


void pingu_insert_max_slot_limit_then_replace_rg_drive(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t bus, encl, slot;    

    mut_printf(MUT_LOG_TEST_STATUS, "%s START\n", __FUNCTION__);


    /* test with first drive in RG. */
    bus = rg_config_p->rg_disk_set[0].bus;
    encl = rg_config_p->rg_disk_set[0].enclosure;
    slot = rg_config_p->rg_disk_set[0].slot;

    pingu_insert_max_slot_limit_then_replace_drive(bus, encl, slot, PINGU_DRIVE_FAULT_NONE);
    pingu_insert_max_slot_limit_then_replace_drive(bus, encl, slot, PINGU_DRIVE_FAULT_PERSITED);
    pingu_insert_max_slot_limit_then_replace_drive(bus, encl, slot, PINGU_DRIVE_FAULT_LINK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s END\n", __FUNCTION__);
}

/*!****************************************************************************
 *  pingu_slot_limit_test_insert_too_many_drives
 ******************************************************************************
 *
 * @brief
 *  This will insert too many drives and verify that database will not
 *  connect if it has already reached its capacity.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 * 03/10/2014 - Created. Wayne Garrett
 *****************************************************************************/
void pingu_slot_limit_test_insert_too_many_drives(void)
{
    const fbe_u32_t new_encl_port = SLOT_LIMIT_PHYSICAL_CONFIG_PORTS-1;
    const fbe_u32_t new_encl_loc  = SLOT_LIMIT_PHYSICAL_CONFIG_ENCLOSURES;

    mut_printf(MUT_LOG_TEST_STATUS, "%s START\n", __FUNCTION__);

    /* Add a new enclosure and attempt to insert drives.  It's expected
       that ESP will dectect exceeded slot count and fail the enclosure
       and any PDO's that were created.  */
    pingu_slot_limit_add_viper_enclosure(new_encl_port, new_encl_loc);

    pingu_slot_verify_event_log_encl_state(new_encl_port, new_encl_loc, "OK", "FAILED"); 

    /* Note: cannot verify drive event log since enclosure may be faulted before drives are created. */
    //sleeping_beauty_verify_event_log_drive_offline(new_encl_port, new_encl_loc, 0, NULL, FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS);

    mut_printf(MUT_LOG_TEST_STATUS, "%s END\n", __FUNCTION__);
}


void pingu_slot_limit_add_viper_enclosure(fbe_u32_t port_loc, fbe_u32_t encl_loc)
{
    fbe_u32_t slot,i;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    fbe_sim_transport_connection_target_t this_sp, other_sp, sp;

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (i = 0; i<2; i++)
    {
        sp = (i==0)?this_sp:other_sp;
        
        fbe_api_sim_transport_set_target_server(sp);

        fbe_api_terminator_get_port_handle(port_loc, &port_handle);

        mut_printf(MUT_LOG_TEST_STATUS, "%s %d_%d SP%s\n", __FUNCTION__, port_loc, encl_loc, (sp==FBE_SIM_SP_A)?"A":"B");

        fbe_test_pp_util_insert_viper_enclosure(port_loc, encl_loc, port_handle, &encl_handle);
        for ( slot = 0; slot < 15; slot++)
        {
            fbe_test_pp_util_insert_sas_drive(port_loc, encl_loc, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
        }
    }

    // restore target
    fbe_api_sim_transport_set_target_server(this_sp);
}

void pingu_slot_limit_remove_enclosure(fbe_u32_t port_loc, fbe_u32_t encl_loc)
{
    fbe_sim_transport_connection_target_t this_sp, other_sp, sp;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_status_t status;
    fbe_u32_t i;

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 
    

    for (i = 0; i<2; i++)
    {
        sp = (i==0)?this_sp:other_sp;

        fbe_api_sim_transport_set_target_server(sp);

        mut_printf(MUT_LOG_TEST_STATUS, "%s %d_%d SP%s\n", __FUNCTION__, port_loc, encl_loc, (sp==FBE_SIM_SP_A)?"A":"B");

        status = fbe_api_terminator_get_enclosure_handle(port_loc, encl_loc, &encl_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        status  = fbe_api_terminator_remove_sas_enclosure (encl_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    // restore target
    fbe_api_sim_transport_set_target_server(this_sp);
}


void pingu_slot_verify_event_log_encl_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u8_t* from_state_str, fbe_u8_t* to_state_str)
{
    fbe_status_t status;
    fbe_bool_t is_msg_present;
    fbe_device_physical_location_t  location;
    fbe_u8_t   deviceStr[FBE_DEVICE_STRING_LENGTH];

    location.bus = bus;
    location.enclosure = encl;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to create device string.");

    status = fbe_api_wait_for_event_log_msg(10000,
                                            FBE_PACKAGE_ID_ESP, 
                                            &is_msg_present, 
                                            ESP_ERROR_ENCL_STATE_CHANGED,
                                            &deviceStr[0],
                                            from_state_str,
                                            to_state_str);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(is_msg_present, FBE_TRUE);

}

void pingu_slot_limit_verify_pvd_not_connected_to_pdo_both_sp(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_object_id_t pdo, pvd;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp, other_sp, sp;

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (i = 0; i<2; i++)
    {
        sp = (i==0)?this_sp:other_sp;

        fbe_api_sim_transport_set_target_server(sp);

        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);                                                
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* verify no PVD is connected to new PDO */
        status = biff_get_pvd_connected_to_pdo(pdo, &pvd, 5000);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_NO_OBJECT, "failure: pvd connect to pdo");
    }

    // restore target
    fbe_api_sim_transport_set_target_server(this_sp);

}

/*!**************************************************************
 * pingu_slot_limit_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration. 
 *
 * @param  Non.e
 *
 * @return None.
 *
 * @author
 *  03/11/2014 - Created.  Wayne Garrett
 *
 ****************************************************************/

void pingu_slot_limit_create_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
   
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;

    fbe_u32_t p,e,slot;

    fbe_u32_t num_ports = SLOT_LIMIT_PHYSICAL_CONFIG_PORTS;
    fbe_u32_t num_encl_per_port = SLOT_LIMIT_PHYSICAL_CONFIG_ENCLOSURES;
    /* total objects =        board + ports     + enclosures                  + drives */
    fbe_u32_t total_expected_objects = 1 + num_ports + num_encl_per_port*num_ports + num_encl_per_port*num_ports*15;



    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    for (p = 0; p < num_ports; p++ )
    {         
        /* insert a port
         */
        fbe_test_pp_util_insert_sas_pmc_port(1+p, /* io port */
                                             2+p, /* portal */
                                             p, /* backend number */ 
                                             &port_handle);
    
        for (e = 0; e < num_encl_per_port; e++)
        {
            fbe_test_pp_util_insert_viper_enclosure(p, e, port_handle, &encl_handle);
    
            /* Insert drives for enclosures.
             */
            for ( slot = 0; slot < 15; slot++)
            {
                if ( p==0 && e==0 && slot < 4 )
                {
                    fbe_test_pp_util_insert_sas_drive(p, e, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
                }
                else{
                    fbe_test_pp_util_insert_sas_drive(p, e, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
                }
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(total_expected_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == total_expected_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}

