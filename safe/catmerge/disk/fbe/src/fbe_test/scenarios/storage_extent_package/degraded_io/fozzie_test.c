/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fozzie_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains single loop failure test cases.
 *
 * @version
 *   05/09/2012 - Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "VolumeAttributes.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * fozzie_short_desc = "Single Loop Failure test.";
char * fozzie_long_desc ="\
Single Loop Failure test cases.\n\
Options:\n\
    -run_test_cases : bitmap of test cases specified by fozzie_test_case_t.\n\
                      Default is to run all\n";

/*!*******************************************************************
 * @def FOZZIE_SLF_MS_TIMEOUT
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *
 *********************************************************************/
#define FOZZIE_SLF_MS_TIMEOUT        60000

/*!*******************************************************************
 * @def FOZZIE_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FOZZIE_RAID_TYPE_COUNT 6
/*!*******************************************************************
 * @def FOZZIE_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define FOZZIE_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FOZZIE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FOZZIE_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @var fozzie_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fozzie_test_contexts[FOZZIE_LUNS_PER_RAID_GROUP * 2];


/*!*******************************************************************
 * @enum fozzie_test_case_e
 *********************************************************************
 * @brief Contains a bitmap of test cases which can be specified 
 *        by -run_test_cases option
 *
 *********************************************************************/
typedef enum fozzie_test_case_e {        
    FOZZIE_TEST_CASE_LINK_BOUNCE    = 0x0001,
    /* ... add other cases here. */
    FOZZIE_TEST_CASE_REMAINING      = 0x8000,
    FOZZIE_TEST_CASE_ALL            = 0xffff
} fozzie_test_case_t;


typedef enum fozzie_drive_removal_reason_e {
    FOZZIE_FAIL_DRIVE = 0,
    FOZZIE_PULL_DRIVE,
    FOZZIE_LOGOUT_DRIVE,
}fozzie_drive_removal_reason_t;


/*************************
 * Local Static Functions.
 *************************/
void fozzie_slf_test_case_1(fbe_test_rg_configuration_t *rg_config_p);
void fozzie_slf_test_case_2(fbe_test_rg_configuration_t *rg_config_p);
void fozzie_slf_test_case_3(fbe_test_rg_configuration_t *rg_config_p);
void fozzie_slf_test_case_4(fbe_test_rg_configuration_t *rg_config_p);
void fozzie_slf_test_case_5(fbe_test_rg_configuration_t *rg_config_p);
void fozzie_slf_test_case_6(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void fozzie_slf_test_case_7(void);

extern fbe_test_rg_configuration_array_t biff_raid_group_config_qual[];
extern void big_bird_start_io(fbe_test_rg_configuration_t * const rg_config_p,
                       fbe_api_rdgen_context_t **context_p,
                       fbe_u32_t threads,
                       fbe_u32_t io_size_blocks,
                       fbe_rdgen_operation_t rdgen_operation,
                       fbe_test_random_abort_msec_t ran_abort_msecs,
                       fbe_api_rdgen_peer_options_t peer_options);
extern void fbe_test_io_wait_for_io_count(fbe_api_rdgen_context_t *context_p,
                                   fbe_u32_t num_tests,
                                   fbe_u32_t io_count,
                                   fbe_u32_t timeout_msecs);


/*!**************************************************************
 * fozzie_slf_test()
 ****************************************************************
 * @brief
 *  This function tests single loop failure functionality.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    if (rg_config_p->test_case_bitmask == 0 /*not specified. do default cases*/   ||
        rg_config_p->test_case_bitmask & FOZZIE_TEST_CASE_REMAINING)
    {   
        mut_printf(MUT_LOG_TEST_STATUS,"fozzie_slf_test: start single loop failure test\n");
    
        fozzie_slf_test_case_1(rg_config_p);
    
        fozzie_slf_test_case_2(rg_config_p);
        fozzie_slf_test_case_5(rg_config_p);
        fozzie_slf_test_case_3(rg_config_p);
        fozzie_slf_test_case_4(rg_config_p);
    }
    return;
}
/******************************************
 * end fozzie_slf_test()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_remove_drive()
 ****************************************************************
 * @brief
 *  This function removes a drive from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param position_to_remove - position to remove.
 * @param removal_reason - how to remove drive
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_remove_drive(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_remove, fozzie_drive_removal_reason_t removal_reason)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pdo_id;
    fbe_api_terminator_device_handle_t drive_handle;
    fbe_status_t status;

    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];

    switch(removal_reason)
    {
        case FOZZIE_FAIL_DRIVE:
            status = fbe_api_get_physical_drive_object_id_by_location(drive_to_remove_p->bus, 
                                                                      drive_to_remove_p->enclosure,
                                                                      drive_to_remove_p->slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pdo_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_wait_for_object_lifecycle_state(pdo_id, FBE_LIFECYCLE_STATE_FAIL, 20000, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== Failing drive %d_%d_%d on SP %d Completed. ==", 
                       drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        case FOZZIE_PULL_DRIVE:
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot,
                                                     &rg_config_p->drive_handle[position_to_remove]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            mut_printf(MUT_LOG_TEST_STATUS, "== Pulling drive %d_%d_%d on SP %d Completed. ==", 
                       drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        case FOZZIE_LOGOUT_DRIVE:
            status = fbe_api_terminator_get_drive_handle(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure,
                                                         drive_to_remove_p->slot,
                                                         &drive_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);        
        
            status = fbe_api_terminator_force_logout_device(drive_handle);            
            mut_printf(MUT_LOG_TEST_STATUS, "== Logout drive %d_%d_%d on SP %d Completed. ==", 
                       drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "Fozzie unexpected removal reason %d", removal_reason);
            MUT_FAIL();
    }
}
/******************************************
 * end fozzie_slf_test_remove_drive()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_insert_drive()
 ****************************************************************
 * @brief
 *  This function inserts a drive from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param position_to_remove - position to remove.
 * @param removal_reason - how drove was removed.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_insert_drive(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_insert, fozzie_drive_removal_reason_t removal_reason)
{
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_object_id_t pdo_id;
    fbe_api_terminator_device_handle_t drive_handle;
    fbe_status_t status;

    drive_to_insert_p = &rg_config_p->rg_disk_set[position_to_insert];

    switch(removal_reason)
    {
        case FOZZIE_FAIL_DRIVE:  /* recover it*/
            status = fbe_api_get_physical_drive_object_id_by_location(drive_to_insert_p->bus, 
                                                                     drive_to_insert_p->enclosure,
                                                                     drive_to_insert_p->slot,
                                                                     &pdo_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pdo_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_wait_for_object_lifecycle_state(pdo_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== recover drive %d_%d_%d on SP %d Completed. ==", 
                drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        case FOZZIE_PULL_DRIVE:  /* reinsert */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                         drive_to_insert_p->enclosure, 
                                                         drive_to_insert_p->slot,
                                                         rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            mut_printf(MUT_LOG_TEST_STATUS, "== reinsert drive %d_%d_%d on SP %d Completed. ==", 
                drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        case FOZZIE_LOGOUT_DRIVE: /* login */
            status = fbe_api_terminator_get_drive_handle(drive_to_insert_p->bus, 
                                                         drive_to_insert_p->enclosure,
                                                         drive_to_insert_p->slot,
                                                         &drive_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    
            status = fbe_api_terminator_force_login_device(drive_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            
            mut_printf(MUT_LOG_TEST_STATUS, "== login drive %d_%d_%d on SP %d Completed. ==", 
                drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot, fbe_api_sim_transport_get_target_server());
            break;
    
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "Fozzie unexpected removal reason %d", removal_reason);
            MUT_FAIL();
    }
}
/******************************************
 * end fozzie_slf_test_insert_drive()
 ******************************************/


/*!**************************************************************
 * fozzie_slf_test_logout_enclosure_for_drive_position()
 ****************************************************************
 * @brief
 *  This function logs out an enclosure, and child devices a drive from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param position_to_remove - position to remove.
 * 
 * @return None.   
 *
 * @author
 *  07/30/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void fozzie_slf_test_logout_enclosure_for_drive_position(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_remove)
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_status_t status;

    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];

    status = fbe_api_terminator_get_enclosure_handle(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure,
                                                     &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);        

    status = fbe_api_terminator_force_logout_device(encl_handle);   
             
    mut_printf(MUT_LOG_TEST_STATUS, "== Logout enclosure %d_%d on SP %d Initiated. ==", 
               drive_to_remove_p->bus, drive_to_remove_p->enclosure, fbe_api_sim_transport_get_target_server());
}

/*!**************************************************************
 * fozzie_slf_test_login_enclosure_for_drive_position()
 ****************************************************************
 * @brief
 *  This function logs in an enclosure, and child devices a drive from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param position_to_remove - position contianing encl to log in.
 * 
 * @return None.   
 *
 * @author
 *  07/30/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void fozzie_slf_test_login_enclosure_for_drive_position(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_insert)
{
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_status_t status;

    drive_to_insert_p = &rg_config_p->rg_disk_set[position_to_insert];

    status = fbe_api_terminator_get_enclosure_handle(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure,
                                                     &encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);        

    status = fbe_api_terminator_force_logout_device(encl_handle);            

    mut_printf(MUT_LOG_TEST_STATUS, "== Login enclosure %d_%d on SP %d Initiated. ==", 
               drive_to_insert_p->bus, drive_to_insert_p->enclosure, fbe_api_sim_transport_get_target_server());
}


/*!**************************************************************
 * fozzie_slf_test_logout_all_rg_positions()
 ****************************************************************
 * @brief
 *  This function logs out all RG positions.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  07/30/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void fozzie_slf_test_logout_all_rg_positions(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t pos;

    for (pos=0; pos < rg_config_p->width; pos++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== Logout enclosure for drive %d_%d_%d on SP %d ==", 
                   rg_config_p->rg_disk_set[pos].bus, 
                   rg_config_p->rg_disk_set[pos].enclosure, 
                   rg_config_p->rg_disk_set[pos].slot, 
                   fbe_api_sim_transport_get_target_server());

        fozzie_slf_test_remove_drive(rg_config_p, pos, FOZZIE_LOGOUT_DRIVE);         
    }
}

/*!**************************************************************
 * fozzie_slf_test_login_all_rg_positions()
 ****************************************************************
 * @brief
 *  This function logs in all RG positions.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  07/30/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void fozzie_slf_test_login_all_rg_positions(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t pos;

    for (pos=0; pos < rg_config_p->width; pos++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== Login enclosure for drive %d_%d_%d on SP %d ==", 
                   rg_config_p->rg_disk_set[pos].bus, 
                   rg_config_p->rg_disk_set[pos].enclosure, 
                   rg_config_p->rg_disk_set[pos].slot, 
                   fbe_api_sim_transport_get_target_server());

        fozzie_slf_test_insert_drive(rg_config_p, pos, FOZZIE_LOGOUT_DRIVE); 
    }
}



/*!**************************************************************
 * fozzie_slf_test_wait_bvd_attr()
 ****************************************************************
 * @brief
 *  This function checks BVD attributes for slf from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param is_set - whether the attribute is set.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_wait_bvd_attr(fbe_test_rg_configuration_t *rg_config_p, fbe_bool_t is_set)
{
    fbe_object_id_t                     lun_object_id;
    fbe_object_id_t                     bvd_object_id;
    fbe_lun_number_t                    lun_number;
    fbe_status_t                        status;
    fbe_u32_t                           current_time = 0;
    fbe_volume_attributes_flags         attribute;

    lun_number = rg_config_p->logical_unit_configuration_list[0].lun_number;

    /* Get the object ID for the LUN; will use it later */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Get the object ID for BVD; will use it later */
    status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);

    while (current_time < FOZZIE_SLF_MS_TIMEOUT)
    {
        /* Get attr from BVD object */
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (is_set)
        {
            /* The attr is set */
            if (attribute & VOL_ATTR_PATH_NOT_PREFERRED)
            {
                break;
            }
        }
        else
        {
            /* The attr is cleared */
            if ((attribute & VOL_ATTR_PATH_NOT_PREFERRED) == 0)
            {
                break;
            }
        }
        current_time += 1000;
        fbe_api_sleep(1000);
    }

    MUT_ASSERT_INT_NOT_EQUAL(current_time, FOZZIE_SLF_MS_TIMEOUT);
}
/******************************************
 * end fozzie_slf_test_wait_bvd_attr()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_wait_pvd_slf_state()
 ****************************************************************
 * @brief
 *  This function checks pvd slf state from one SP.
 *  
 * @param pvd_id - pvd object id.
 * @param is_slf - whether slf state is set.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_wait_pvd_slf_state(fbe_object_id_t pvd_id, fbe_bool_t is_slf)
{
    fbe_status_t                        status;
    fbe_u32_t                           current_time = 0;
    fbe_bool_t                          b_slf;

    while (current_time < FOZZIE_SLF_MS_TIMEOUT)
    {
        /* Get slf state */
        status = fbe_api_provision_drive_is_slf(pvd_id, &b_slf);

        if ((status == FBE_STATUS_OK) && (b_slf == is_slf))
        {
            break;
        }
        current_time += 1000;
        fbe_api_sleep(1000);
    }

    MUT_ASSERT_INT_NOT_EQUAL(current_time, FOZZIE_SLF_MS_TIMEOUT);
}
/******************************************
 * end fozzie_slf_test_wait_pvd_slf_state()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_wait_rg_degraded()
 ****************************************************************
 * @brief
 *  This function checks pvd slf state from one SP.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param is_degraded - whether the raid is degraded.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_wait_rg_degraded(fbe_test_rg_configuration_t * rg_config_p, fbe_bool_t is_degraded)
{
    fbe_u32_t                           current_time = 0;
    fbe_bool_t                          b_degraded;

    while (current_time < FOZZIE_SLF_MS_TIMEOUT)
    {
        /* Get raid degrade info */
        b_degraded = fbe_test_rg_is_degraded(rg_config_p);
        if (b_degraded == is_degraded)
        {
            break;
        }
        current_time += 1000;
        fbe_api_sleep(1000);
    }

    MUT_ASSERT_INT_NOT_EQUAL(current_time, FOZZIE_SLF_MS_TIMEOUT);

}
/******************************************
 * end fozzie_slf_test_wait_rg_degraded()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_check_drive_slf()
 ****************************************************************
 * @brief
 *  This function checks pvd, raid, bvd from both SPs, when a drive
 *  is in slf.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param pvd_id - provision drive object id.
 * @param fail_sp - which SP the drive is slf.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_check_drive_slf(fbe_test_rg_configuration_t *rg_config_p, 
                                       fbe_object_id_t pvd_id,
                                       fbe_sim_transport_connection_target_t fail_sp)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - fail_sp;
    fbe_object_id_t rg_object_id;

    /* Check the PVD is in SLF */
    fbe_api_sim_transport_set_target_server(fail_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_TRUE);
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_FALSE);
    /* Check PVDs on both SPs are in ready state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, pvd_id, 
		FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is in ready state */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, rg_object_id, 
		FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Check the RAID is not degraded */
    fozzie_slf_test_wait_rg_degraded(rg_config_p, FBE_FALSE);

    /* Check BVD attr on both sides */
    fbe_api_sim_transport_set_target_server(fail_sp);
    fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_TRUE);
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
}
/******************************************
 * end fozzie_slf_test_check_drive_slf()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_check_drive_fail_on_both_sides()
 ****************************************************************
 * @brief
 *  This function checks pvd, raid, bvd from both SPs, when a drive
 *  failed on both sides.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param pvd_id - provision drive object id.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_check_drive_fail_on_both_sides(fbe_test_rg_configuration_t *rg_config_p, 
                                                    fbe_object_id_t pvd_id,
                                                    fbe_sim_transport_connection_target_t this_sp)
{
    fbe_status_t status;
    //fbe_sim_transport_connection_target_t this_sp = fbe_api_sim_transport_get_target_server();
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - this_sp;
    fbe_object_id_t rg_object_id;

    /* Check the PVD failed */
#if 0
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_TRUE);
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_TRUE);
#endif
    /* Check PVDs on both SPs are in fail state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, pvd_id, 
		FBE_LIFECYCLE_STATE_FAIL, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is in ready state */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, rg_object_id, 
		    FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        /* Check RG is degraded */
        fozzie_slf_test_wait_rg_degraded(rg_config_p, FBE_TRUE);

        /* Check BVD attr on both sides */
        fbe_api_sim_transport_set_target_server(this_sp);
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);
        fbe_api_sim_transport_set_target_server(peer_sp);
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);
    }
    else
    {
        /* For RAID0, it should be broken now */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, rg_object_id, 
		    FBE_LIFECYCLE_STATE_FAIL, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
}
/******************************************
 * end fozzie_slf_test_check_drive_fail_on_both_sides()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_check_drive_no_slf()
 ****************************************************************
 * @brief
 *  This function checks pvd, raid, bvd from both SPs, when a drive
 *  is good on both sides.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param pvd_id - provision drive object id.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
 void fozzie_slf_test_check_drive_no_slf(fbe_test_rg_configuration_t *rg_config_p, 
                                         fbe_object_id_t pvd_id)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = fbe_api_sim_transport_get_target_server();
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - this_sp;
    fbe_object_id_t rg_object_id;

    /* Check the PVD is not in SLF */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_FALSE);
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_FALSE);
    /* Check PVDs on both SPs are in ready state */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, pvd_id, 
		FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is in ready state */
    //fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, rg_object_id, 
		FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is not degraded */
    fozzie_slf_test_wait_rg_degraded(rg_config_p, FBE_FALSE);

    /* Check BVD attr on both sides */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
}
/******************************************
 * end fozzie_slf_test_check_drive_no_slf()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_1()
 ****************************************************************
 * @brief
 *  This case tests one drive slf case:
 *  STEP 1: a drive in a raid group dies on one SP.
 *  STEP 2: check SLF state is set on this SP, but cleared on the other SP.
 *          check the raid group is not degraded.
 *  STEP 3: the same drive comes back on SPA.
 *  STEP 4: check SLF state is not set on both SPs.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_1(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;
    fbe_api_rdgen_context_t *context_p = &fozzie_test_contexts[0];
    fbe_u32_t num_contexts = 0;

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove the drive in one SP */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);

    fbe_api_sim_transport_set_target_server(this_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O. ==", __FUNCTION__);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, 4096, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(5000);
    //fbe_test_io_wait_for_io_count(context_p, num_contexts, 1, 1000);
    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);
    /* Make sure there is no error.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reinsert the drive */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_1()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_2()
 ****************************************************************
 * @brief
 *  This case tests one drive slf->failure->slf->good:
 *  STEP 1: a drive in a raid group dies on one SP.
 *  STEP 2: check SLF state is set on this SP, but cleared on the other SP.
 *          check the raid group is not degraded.
 *  STEP 3: the same drive dies on the other SP.
 *  STEP 4: check the PVDs go to fail state.
 *          check the raid group starts rebuild logging.
 *  STEP 5: the same drive comes back on one SP.
 *  STEP 6: check PVDs on both SPs are in ready state.
 *          check the raid group is not degraded.
 *  STEP 7: the same drive comes back on peer SP.
 *  STEP 8: check both side has bvd attr cleared.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_2(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove the drive from this SP */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);

    /* Remove the drive from peer SP */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_fail_on_both_sides(rg_config_p, pvd_id, this_sp);

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - this_sp;
        peer_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - peer_sp;
    }

    /* Insert the drive on one side */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, peer_sp);

    /* Insert the drive on peer side */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_2()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_3()
 ****************************************************************
 * @brief
 *  This case tests slf evaluation during PVD join:
 *  STEP 1: set a hook on one SP to stop in Join request.
 *  STEP 2: reboot peer SP.
 *  STEP 3: remove a drive in the peer SP.
 *  STEP 4: check the PVD is in activate state.
 *  STEP 5: delete the hook.
 *  STEP 6: check PVDs on both SPs are in ready state.
 *          check the raid group is not degraded.
 *  STEP 7: the same drive comes back on peer SP.
 *  STEP 8: check both side has bvd attr cleared.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/08/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_3(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_lifecycle_state_t object_state;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);

    /* Add hook */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Adding Debug Hook...\n");
    fbe_api_sim_transport_set_target_server(this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_scheduler_add_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                                              FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);


    /* Reboot peer SP */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fbe_test_sep_util_destroy_esp_neit_sep();
    sep_config_load_esp_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(120000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_database_set_load_balance(FBE_TRUE);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Remove drive in peer SP */
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    /* Check the PVD is in activate state (hook) */
    status = fbe_api_get_object_lifecycle_state(pvd_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(object_state, FBE_LIFECYCLE_STATE_ACTIVATE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Delete Debug Hook...\n");
    fbe_api_sim_transport_set_target_server(this_sp);
    status = fbe_api_scheduler_del_debug_hook(pvd_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                                              FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, peer_sp);

    /* Insert the drive on peer side */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_3()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_4()
 ****************************************************************
 * @brief
 *  This case tests slf->failure when the other SP fails:
 *  STEP 1: a drive in a raid group dies on one SP.
 *  STEP 2: check SLF state is set on this SP, but cleared on the other SP.
 *          check the raid group is not degraded.
 *  STEP 3: The other SP dies.
 *  STEP 4: check the PVD in this SP is in fail state.
 *          check the raid group is degraded.
 *  STEP 5: the same drive comes back on this SP.
 *  STEP 6: check PVD is back to ready state.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_4(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_lifecycle_state_t object_state;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;
    fbe_bool_t b_degraded;

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove the drive in one SP */
    fbe_api_sim_transport_set_target_server(this_sp);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);

    /* Shutdown peer sep and neit */
    fbe_api_sim_transport_set_target_server(peer_sp);
    fbe_test_sep_util_destroy_esp_neit_sep();

    /* Check the PVD is in fail state */
    fbe_api_sim_transport_set_target_server(this_sp);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_FAIL, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is in ready state */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)
    {
        MUT_ASSERT_INT_EQUAL(object_state, FBE_LIFECYCLE_STATE_READY);
        /* Check RG is degraded */
        b_degraded = fbe_test_rg_is_degraded(rg_config_p);
        MUT_ASSERT_INT_EQUAL(b_degraded, FBE_TRUE);
    }
    else
    {
        /* For RAID0, it should be broken now */
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, 60000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Check BVD attr not set */
    //fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);

    /* Reinsert the drive */
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);

    /* Check the PVD is in ready state */
    fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_FALSE);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check the RAID is in ready state */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Check BVD attr not set */
    fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);

    /* Now load the system*/
    fbe_api_sim_transport_set_target_server(peer_sp);
    sep_config_load_esp_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(120000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_database_set_load_balance(FBE_TRUE);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Check everything good */
    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_4()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_5()
 ****************************************************************
 * @brief
 *  This case tests multiple SLF drives in the same raid group:
 *  STEP 1: multiple drives in a raid group dies in different SPs.
 *  STEP 2: check SLF state is set on PVDs in different SPs.
 *          check the raid group is not degraded.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  05/03/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_5(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t num_drives_to_remove = (fbe_random()%(rg_config_p->width)+1);
    fbe_u32_t position_to_remove;
    fbe_u32_t num_removed_per_sp[2] = {0, 0};
    fbe_u32_t this_sp;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id[16];
    fbe_object_id_t rg_object_id;
    fbe_u32_t drive_removed_in_sp[16];

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start\n", __FUNCTION__);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        num_drives_to_remove = rg_config_p->width;
    }
    /* Remove drives */
    for (position_to_remove = 0; position_to_remove < num_drives_to_remove; position_to_remove++)
    {
        this_sp = fbe_random()%2;
        num_removed_per_sp[this_sp]++;
        drive_removed_in_sp[position_to_remove] = this_sp;
        drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                &pvd_id[position_to_remove]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_api_sim_transport_set_target_server(this_sp+1);
        fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Check the PVDs are in ready state */
    for (position_to_remove = 0; position_to_remove < num_drives_to_remove; position_to_remove++)
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, pvd_id[position_to_remove], 
			FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Drives removed: total %d, SPA %d SPB %d\n", 
               num_drives_to_remove, num_removed_per_sp[0], num_removed_per_sp[1]);

    /* Check the RAID is in ready state */
    //fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, rg_object_id, 
		FBE_LIFECYCLE_STATE_READY, FOZZIE_SLF_MS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 0
    if (num_removed_per_sp[0] > num_removed_per_sp[1])
    {
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_TRUE);
    }
    else
    {
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    if (num_removed_per_sp[1] > num_removed_per_sp[0])
    {
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_TRUE);
    }
    else
    {
        fozzie_slf_test_wait_bvd_attr(rg_config_p, FBE_FALSE);
    }
#endif

    /* Insert drives */
    for (position_to_remove = 0; position_to_remove < num_drives_to_remove; position_to_remove++)
    {
        fbe_api_sim_transport_set_target_server(drive_removed_in_sp[position_to_remove] + 1);
        fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
    }

    /* Check drives not in slf */
    for (position_to_remove = 0; position_to_remove < num_drives_to_remove; position_to_remove++)
    {
        fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id[position_to_remove]);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_5()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_6()
 ****************************************************************
 * @brief
 *  This case tests the case when the system is booting up, a drive
 *  is dead on one SP and good at the other SP.
 *  STEP 1: SEP shuts down on both SPs.
 *  STEP 2: Remove a drive on one SP.
 *  STEP 3: SEP boots up on both SPs.
 *  STEP 4: check the drive is in SLF.
 *          check the raid group is not degraded.
 *  
 * @param rg_config_p - pointer to raid group configuration.
 * @param context_p - pointer to context.
 * 
 * @return None.   
 *
 * @author
 *  08/06/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_6(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;
    fbe_sim_transport_connection_target_t boot_sp;

    if (rg_config_p->test_case_bitmask == 0 /*not specified. do default cases*/   ||
        rg_config_p->test_case_bitmask & FOZZIE_TEST_CASE_REMAINING)
    {  
        mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
        drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                &pvd_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        /* Shutdown peer sep and neit */
        fbe_api_sim_transport_set_target_server(peer_sp);
        fbe_test_sep_util_destroy_esp_neit_sep();
        /* Shutdown sep and neit in this SP */
        fbe_api_sim_transport_set_target_server(this_sp);
        fbe_test_sep_util_destroy_esp_neit_sep();
        
        /* Remove the drive in one SP */
        fbe_api_sim_transport_set_target_server(this_sp);
        fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
        
        /* Now load the system*/
        boot_sp = fbe_random()%2 + FBE_SIM_SP_A;
        mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Booting SP %d first\n", __FUNCTION__, boot_sp);
        fbe_api_sim_transport_set_target_server(boot_sp);
        sep_config_load_esp_sep_and_neit();
        status = fbe_test_sep_util_wait_for_database_service(120000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_database_set_load_balance(FBE_TRUE);
        fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
        fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);
        
        /* Load the peer system*/
        boot_sp = FBE_SIM_SP_A + FBE_SIM_SP_B - boot_sp;
        mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Booting SP %d\n", __FUNCTION__, boot_sp);
        fbe_api_sim_transport_set_target_server(boot_sp);
        sep_config_load_esp_sep_and_neit();
        status = fbe_test_sep_util_wait_for_database_service(120000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_database_set_load_balance(FBE_TRUE);
        fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
        fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);
        
        /* Check slf */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps (FBE_TRUE, pvd_id, 
            FBE_LIFECYCLE_STATE_READY, 360000, FBE_PACKAGE_ID_SEP_0);
        fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);
        
        /* Reinsert the drive */
        fbe_api_sim_transport_set_target_server(this_sp);
        fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_PULL_DRIVE);
        /* Check everything good */
        fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);
        
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        
        mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
    }
}
/******************************************
 * end fozzie_slf_test_case_6()
 ******************************************/

/*!**************************************************************
 * fozzie_slf_test_case_7()
 ****************************************************************
 * @brief
 *  This case tests the case when enclousure 0_0 is in firmware
 *  download.
 *  STEP 1: Starts LCC firmware download on one SP.
 *  STEP 2: Check system drives are in SLF.
 *  STEP 3: LCC firmware download finishes.
 *  STEP 4: check system drives are not in SLF.
 *  
 * @param None.
 * 
 * @return None.   
 *
 * @author
 *  09/18/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_slf_test_case_7(void)
{
    fbe_object_id_t pvd_id;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;

    deviceType = FBE_DEVICE_TYPE_LCC;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    if (fbe_random()%2)
    {
        this_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
        location.slot = 1;
    }
    fbe_api_sim_transport_set_target_server(this_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);

    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 10000, 10000, TRUE);
    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
    mut_printf(MUT_LOG_LOW, "=== LCC upgrade Initiated ===");
   
    /* Verify the upgrade is in progress.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  

    /* Check slf */
    for (pvd_id = 1; pvd_id <= 4; pvd_id++)
    {
        fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_TRUE);
    }

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  
    mut_printf(MUT_LOG_LOW, "=== LCC upgrade finished ===");
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    /* Check no slf */
    for (pvd_id = 1; pvd_id <= 4; pvd_id++)
    {
        fozzie_slf_test_wait_pvd_slf_state(pvd_id, FBE_FALSE);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}
/******************************************
 * end fozzie_slf_test_case_7()
 ******************************************/


void fozzie_link_bouncing_one_drive_no_io(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Remove the drive in one SP */
    fbe_api_sim_transport_set_target_server(this_sp);

    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);
    fbe_api_sleep(20);   /*TODO: make this random*/
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);
    fbe_api_sleep(20);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);
    fbe_api_sleep(20);
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);
    fbe_api_sleep(20);
    fozzie_slf_test_remove_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);

    /* Wait for the old drive to be removed.*/
    status = fbe_test_pp_util_verify_pdo_state(drive_to_remove_p->bus, 
                                               drive_to_remove_p->enclosure,
                                               drive_to_remove_p->slot,
                                               FBE_LIFECYCLE_STATE_ACTIVATE, 20000);

    /* Check slf */
    fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);

    /* Reinsert the drive */    
    fozzie_slf_test_insert_drive(rg_config_p, position_to_remove, FOZZIE_LOGOUT_DRIVE);
    
    status = fbe_test_pp_util_verify_pdo_state(drive_to_remove_p->bus, 
                                               drive_to_remove_p->enclosure,
                                               drive_to_remove_p->slot,
                                               FBE_LIFECYCLE_STATE_READY, 20000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);


    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}

static fbe_status_t fozzie_test_clear_drive_error_counts(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t pos;
    fbe_object_id_t pdo_obj;

    for (pos = 0; pos < rg_config_p->width; pos++)
    {
        /* Get the physical drive object id.
         */
        status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[pos].bus, 
                                                                   rg_config_p->rg_disk_set[pos].enclosure, 
                                                                   rg_config_p->rg_disk_set[pos].slot, 
                                                                   &pdo_obj);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(pdo_obj != FBE_OBJECT_ID_INVALID);

        /* Clear error counts. 
        */
        status = fbe_api_physical_drive_clear_error_counts(pdo_obj, FBE_PACKET_FLAG_NO_ATTRIB);        
    }
    return status;
}

void fozzie_link_bouncing_all_rg_drives_no_io(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t position_to_remove = (fbe_random()%(rg_config_p->width));
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_u32_t pos;
    fbe_u32_t wait_time = fbe_random()%20;
    fbe_u32_t i;
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_sim_transport_connection_target_t this_sp = FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s === Start on SP %d\n", __FUNCTION__, this_sp);
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];


    /* 1. Log out all drives in RG on one SP 
    */
    fbe_api_sim_transport_set_target_server(this_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== Bounce wait time %d ==\n", wait_time);
    for (i=0; i<100; i++)
    {
        fozzie_slf_test_logout_all_rg_positions(rg_config_p);
        fbe_api_sleep(wait_time);
        fozzie_slf_test_login_all_rg_positions(rg_config_p);
        fbe_api_sleep(wait_time);

        /*Logouts can cause DIEH link bucket to increase and eventually shoot the drive.*/
        status = fozzie_test_clear_drive_error_counts(rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fozzie_slf_test_logout_all_rg_positions(rg_config_p);

    /* 2. Verify all drives are in SLF 
    */
    for (pos = 0; pos < rg_config_p->width; pos++)
    {
        drive_to_remove_p = &rg_config_p->rg_disk_set[pos];
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                &pvd_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== Drive %d_%d_%d PVD ID 0x%x ==", drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                pvd_id);
    
        /* Wait for drive to logout.*/
        status = fbe_test_pp_util_verify_pdo_state(drive_to_remove_p->bus, 
                                                   drive_to_remove_p->enclosure,
                                                   drive_to_remove_p->slot,
                                                   FBE_LIFECYCLE_STATE_ACTIVATE, 20000);

        /* Check slf */
        fozzie_slf_test_check_drive_slf(rg_config_p, pvd_id, this_sp);
    }

    /* 3. Log in all drives 
    */    
    fozzie_slf_test_login_all_rg_positions(rg_config_p);


    /* 4. Verify no drives in SLF 
    */
    for (pos = 0; pos < rg_config_p->width; pos++)
    {
        drive_to_remove_p = &rg_config_p->rg_disk_set[pos];
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                &pvd_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "== Drive %d_%d_%d on SP 0x%x ==", drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure,
                                                                drive_to_remove_p->slot,
                                                                pvd_id);

        status = fbe_test_pp_util_verify_pdo_state(drive_to_remove_p->bus, 
                                                   drive_to_remove_p->enclosure,
                                                   drive_to_remove_p->slot,
                                                   FBE_LIFECYCLE_STATE_READY, 20000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        fozzie_slf_test_check_drive_no_slf(rg_config_p, pvd_id);
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "\n=== %s Complete ===\n", __FUNCTION__);
}


void fozzie_link_bouncing_test(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_scheduler_debug_hook_t pvd_activate_hook;
    fbe_u32_t pos;
    fbe_object_id_t pvd;
    fbe_status_t status;

    if (rg_config_p->test_case_bitmask == 0 /*not specified. do default cases*/   ||
        rg_config_p->test_case_bitmask & FOZZIE_TEST_CASE_LINK_BOUNCE)
    {        
        /* initialize hook */
        pvd_activate_hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE;
        pvd_activate_hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_DISABLED;
        pvd_activate_hook.check_type = SCHEDULER_CHECK_STATES;
        pvd_activate_hook.action = SCHEDULER_DEBUG_ACTION_CONTINUE;
        pvd_activate_hook.val1 = 0;
        pvd_activate_hook.val2 = 0;
    
            
        /* setup debug hook */
        for (pos=0; pos < rg_config_p->width; pos++)
        {
            fbe_api_provision_drive_get_obj_id_by_location_from_topology(rg_config_p->rg_disk_set[pos].bus,
                                                                         rg_config_p->rg_disk_set[pos].enclosure,
                                                                         rg_config_p->rg_disk_set[pos].slot,
                                                                         &pvd);                                                                         
            pvd_activate_hook.object_id = pvd;  
            status = fbe_api_scheduler_add_debug_hook(pvd_activate_hook.object_id,
                                                      pvd_activate_hook.monitor_state,
                                                      pvd_activate_hook.monitor_substate,
                                                      pvd_activate_hook.val1, 
                                                      pvd_activate_hook.val2, 
                                                      pvd_activate_hook.check_type, 
                                                      pvd_activate_hook.action);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
                        
        /* -----------------------------
           run tests 
        */
        fozzie_link_bouncing_one_drive_no_io(rg_config_p, context_p);
        fozzie_link_bouncing_all_rg_drives_no_io(rg_config_p, context_p);


        /* verify pvd never transitioned to activate state */      
        for (pos=0; pos < rg_config_p->width; pos++)
        {            

            fbe_api_provision_drive_get_obj_id_by_location_from_topology(rg_config_p->rg_disk_set[pos].bus,
                                                                         rg_config_p->rg_disk_set[pos].enclosure,
                                                                         rg_config_p->rg_disk_set[pos].slot,
                                                                         &pvd);                                                                         

            pvd_activate_hook.object_id = pvd;  
        
            status = fbe_api_scheduler_get_debug_hook(&pvd_activate_hook);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
            MUT_ASSERT_INTEGER_EQUAL_MSG(pvd_activate_hook.counter, 0, "FAILURE: PVD entered Activate state");    

            /* cleanup */
            status = fbe_api_scheduler_del_debug_hook(pvd_activate_hook.object_id,
                                                      pvd_activate_hook.monitor_state,
                                                      pvd_activate_hook.monitor_substate,
                                                      pvd_activate_hook.val1, 
                                                      pvd_activate_hook.val2, 
                                                      pvd_activate_hook.check_type, 
                                                      pvd_activate_hook.action);                                                      
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
    }
}


/*!**************************************************************
 * fozzie_dualsp_test()
 ****************************************************************
 * @brief
 *  Run dual SP SLF test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/11/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_dualsp_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Enable SLF */
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);


    /* Test link bouncing*/
    fbe_test_run_test_on_rg_config_with_time_save(&biff_raid_group_config_qual[1][0], 
                                                  NULL,
                                                  fozzie_link_bouncing_test,
                                                  FOZZIE_LUNS_PER_RAID_GROUP,
                                                  FOZZIE_CHUNKS_PER_LUN,
                                                  FBE_FALSE);

    /* Run the test cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < FOZZIE_RAID_TYPE_COUNT; raid_type_index++)
    {
        rg_config_p = &biff_raid_group_config_qual[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_count(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }


        /* Now create the raid groups and run the tests
         */
        fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                       fozzie_slf_test,
                                       FOZZIE_LUNS_PER_RAID_GROUP,
                                       FOZZIE_CHUNKS_PER_LUN,FBE_FALSE);
        
    }    /* for all raid types. */

    /* Run test case 6.
     */
    fbe_test_run_test_on_rg_config(&biff_raid_group_config_qual[1][0], 
                                   NULL,
                                   fozzie_slf_test_case_6,
                                   FOZZIE_LUNS_PER_RAID_GROUP,
                                   FOZZIE_CHUNKS_PER_LUN);

    /* Run test case 7.
     */
    fozzie_slf_test_case_7();


    /* Disable SLF */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_provision_drive_set_slf_enabled(FBE_FALSE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_provision_drive_set_slf_enabled(FBE_FALSE);

    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end fozzie_dualsp_test()
 ******************************************/

/*!**************************************************************
 * fozzie_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a dual SP single loop failure test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  05/11/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;

        array_p = &biff_raid_group_config_qual[0];
        for (test_index = 0; test_index < FOZZIE_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        sep_config_load_esp_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * end fozzie_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * fozzie_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fozzie dual SP test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  05/11/2012 - Created. chenl6
 *
 ****************************************************************/
void fozzie_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
}
/******************************************
 * end fozzie_dualsp_cleanup()
 ******************************************/

/*******************************
 * end file fozzie_test.c
 *******************************/
