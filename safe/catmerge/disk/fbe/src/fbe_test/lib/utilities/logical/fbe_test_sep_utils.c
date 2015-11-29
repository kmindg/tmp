/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for use with the Storage Extent 
 *  Package (SEP).
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "sep_tests.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe_spare.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_terminator_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_random.h"
#include "pp_utils.h"
#include "neit_utils.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_database.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe_raid_library.h"
#include "fbe_test.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_extent_pool.h"

/*****************************************
 * DEFINITIONS
 *****************************************/
#define FBE_TEST_UTILS_POLLING_INTERVAL 100 /*ms*/

#define FBE_TEST_UTILS_CREATE_TIMEOUT_MS        120000 /*ms*/
#define FBE_TEST_UTILS_DESTROY_TIMEOUT_MS       120000 /*ms*/
#define FBE_TEST_UTILS_DRIVE_SWAP_TIMEOUT_MS    120000 /*ms*/ 
#define FBE_TEST_UTILS_WAIT_TIMEOUT_MS            8000  /*ms*/
#define FBE_TEST_UTILS_SET_PS_TRIGGER_TIMEOUT_MS 120000 /*ms*/
#define FBE_TEST_CHUNK_SIZE 0x800

/*!*******************************************************************
 * @def FBE_TEST_SEP_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/

#define FBE_TEST_SEP_WAIT_MSEC 30000

/*!*******************************************************************
 * @def FBE_TEST_WAIT_OBJECT_TIMEOUT_MS
 *********************************************************************
 * @brief Number of milliseconds we wait for an object.
 *        We make this relatively large since if it takes this long
 *        then we know something is very wrong.
 *
 *********************************************************************/

#define FBE_TEST_WAIT_OBJECT_TIMEOUT_MS 60000


/*!*******************************************************************
 * @def FBE_TEST_DEFAULT_IO_SECONDS_QUAL
 *********************************************************************
 * @brief Run I/O for a while before checking the error counts.
 *
 *********************************************************************/
#define FBE_TEST_DEFAULT_IO_SECONDS_QUAL 3

/*!*******************************************************************
 * @def FBE_TEST_DEFAULT_IO_SECONDS_EXTENDED
 *********************************************************************
 * @brief In extended mode we use a different time to wait before
 *        checking the error counts.
 *
 *********************************************************************/
#define FBE_TEST_DEFAULT_IO_SECONDS_EXTENDED 5

/*************************
 *   GLOBALS
 *************************/
static fbe_api_terminator_device_handle_t drive_handle[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
static fbe_bool_t fbe_test_sep_util_dualsp_test_mode = FBE_FALSE;
static fbe_bool_t fbe_test_sep_util_encryption_test_mode = FBE_FALSE;
/* By default we will remove on both SPs.
 */
static fbe_bool_t fbe_test_sep_util_should_remove_on_both_sps_mode = FBE_TRUE;
static fbe_bool_t fbe_test_sep_util_error_injection_test_mode = FBE_FALSE;  // tests with error-injection need to clear this.
static fbe_u32_t  sep_rebuild_utils_chunks_per_rebuild_g  = 1;

fbe_bool_t is_esp_loaded = FBE_FALSE;

static void sep_util_print_dps_statistics(fbe_memory_dps_statistics_t * memory_dps_statistics);
static void sep_util_print_metadata_statistics(fbe_api_base_config_get_metadata_statistics_t * metadata_statistics);
void fbe_test_sep_util_wait_for_initial_lun_verify_completion(fbe_object_id_t lun_object_id);
static fbe_status_t fbe_test_sep_util_setup_rg_keys(fbe_test_rg_configuration_t * raid_group_configuration_p);
static fbe_u32_t fbe_test_get_rg_tested_count(fbe_test_rg_configuration_t *rg_config_p);
 /*************************
 *   FUNCTIONS
 *************************/

fbe_status_t fbe_test_sep_util_wait_for_database_service(fbe_u32_t timeout_ms)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                   current_time = 0;
    fbe_database_state_t   state = FBE_DATABASE_STATE_READY;
    
    while (current_time < timeout_ms)
    {
        status = fbe_api_database_get_state(&state);

        if ( status == FBE_STATUS_OK
             && (state == FBE_DATABASE_STATE_READY
                 || state == FBE_DATABASE_STATE_SERVICE_MODE
                 || state == FBE_DATABASE_STATE_CORRUPT
				 || state == FBE_DATABASE_STATE_BOOTFLASH_SERVICE_MODE)) {
            
            return status;
        }


        if(current_time &&  (current_time / FBE_TEST_UTILS_POLLING_INTERVAL)%10 == 0){
            fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "%s: database service not ready after %d ms!\n", 
                      __FUNCTION__, current_time);
        }
        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: database service not ready after %d ms!\n", 
                  __FUNCTION__, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_test_sep_util_wait_for_database_service_ready(fbe_u32_t timeout_ms)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                   current_time = 0;
    fbe_database_state_t   state = FBE_DATABASE_STATE_READY;
    
    while (current_time < timeout_ms)
    {
        status = fbe_api_database_get_state(&state);

        if ( status == FBE_STATUS_OK
             && (state == FBE_DATABASE_STATE_READY)) {
            return status;
        }


        if(current_time &&  (current_time / FBE_TEST_UTILS_POLLING_INTERVAL)%10 == 0){
            fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "%s: database service not ready after %d ms!\n", 
                      __FUNCTION__, current_time);
        }
        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: database service not ready after %d ms!\n", 
                  __FUNCTION__, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t fbe_test_sep_util_wait_for_ica_stamp_generation(fbe_u32_t timeout_ms)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                   current_time = 0;
    fbe_bool_t                  is_ica_complete = FBE_FALSE;
    
    while (current_time < timeout_ms)
    {
        status = fbe_api_sim_server_get_ica_status(&is_ica_complete);

        if ( status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Error in getting ICA stamp status:0x%x\n", __FUNCTION__, status);
            return status;
        }

        if ( is_ica_complete == FBE_TRUE ) 
        {
            return status;
        }

        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: ICA stamp generation not done after %d ms!\n", __FUNCTION__, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
fbe_test_sep_util_get_virtual_drive_spare_info(fbe_object_id_t vd_object_id, 
                                               fbe_spare_drive_info_t * spare_drive_info_p)
{
    return  fbe_api_virtual_drive_get_spare_info(vd_object_id, 
                                                 spare_drive_info_p);
}

fbe_status_t fbe_test_sep_util_quiesce_all_raid_groups(fbe_class_id_t class_id)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /*! First get all members of this class.
     * 
     *  @todo Remove system objects from list
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(obj_list_p);
        return status;
    }

    for ( index = 0; index < num_objects; index++)
    {
        status = fbe_api_raid_group_quiesce(obj_list_p[index], FBE_PACKET_FLAG_NO_ATTRIB);

        if (status != FBE_STATUS_OK)
        {
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}

fbe_status_t fbe_test_sep_util_unquiesce_all_raid_groups(fbe_class_id_t class_id)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /*! First get all members of this class.
     * 
     *  @todo Remove system objects from list
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(obj_list_p);
        return status;
    }

    for ( index = 0; index < num_objects; index++)
    {
        status = fbe_api_raid_group_unquiesce(obj_list_p[index], FBE_PACKET_FLAG_NO_ATTRIB);

        if (status != FBE_STATUS_OK)
        {
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/*!**************************************************************
 * fbe_test_sep_util_get_raid_group_disk_info()
 ****************************************************************
 * @brief
 *  Refresh all the drive slot information for a raid group configuration.
 *  This info can change over time as permanent spares get swapped in.
 *
 * @param rg_config_p - Our configuration.
 *
 * @return None.
 *
 * @author
 *  12/13/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_get_raid_group_disk_info(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t index;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t port, encl, slot;
    
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t mirror_index;
        fbe_api_base_config_downstream_object_list_t mirror_downstream_object_list;

        /* For each mirrored pair get the drive locations.
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++)
        {
            /* Get the downstream object list for this mirrored pair.  
             */ 
            fbe_test_sep_util_get_ds_object_list(downstream_object_list.downstream_object_list[index], 
                                                 &mirror_downstream_object_list);

            for (mirror_index = 0; 
                  mirror_index < mirror_downstream_object_list.number_of_downstream_objects; 
                  mirror_index++)
            {
                /* We arrange each pair as consecutive drives in the raid group. 
                 * The first index gets us to a pair, * 2 gets us to a drive number. 
                 * Then we add on the index within the mirror to get us to the specific drive in the mirror. 
                 */
                fbe_u32_t disk_set_index = (index * 2) + mirror_index;
                status = fbe_api_provision_drive_get_location(mirror_downstream_object_list.downstream_object_list[mirror_index],
                                                               &port, &encl, &slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                rg_config_p->rg_disk_set[disk_set_index].bus = port;
                rg_config_p->rg_disk_set[disk_set_index].enclosure = encl;
                rg_config_p->rg_disk_set[disk_set_index].slot = slot;
            } /* end for each disk in the mirror. */
        } /* end for each mirrored pair. */
    }
    else
    {
        /* Loop over all the edges. 
         * Since this type of rg only has one level, just get the drive info for each of these edges. 
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++)
        {
            /* We can issue this command to the VD level since the traverse bit is set in the packet 
             * and thus, the VD will simply forward it down to the next level. 
             */
            status = fbe_api_provision_drive_get_location(downstream_object_list.downstream_object_list[index],
                                                           &port, &encl, &slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p->rg_disk_set[index].bus = port;
            rg_config_p->rg_disk_set[index].enclosure = encl;
            rg_config_p->rg_disk_set[index].slot = slot;

            //mut_printf(MUT_LOG_TEST_STATUS, "RG: %d [%d]: %d_%d_%d", rg_config_p->raid_group_id, index, port, encl, slot);
        }
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_get_raid_group_disk_info()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_raid_group_refresh_disk_info()
 ****************************************************************
 * @brief
 *  Refresh all the drive slot information for a raid group
 *  configuration array.  This info can change over time
 *  as permanent spares get swapped in.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None.
 *
 * @author
 *  12/13/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_raid_group_refresh_disk_info(fbe_test_rg_configuration_t * rg_config_p, 
                                                    fbe_u32_t raid_group_count)
{
    fbe_u32_t rg_index;

    /* Just loop over all our raid groups, refreshing each one.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }
        fbe_test_sep_util_get_raid_group_disk_info(&rg_config_p[rg_index]);
    }
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_get_drive_capacity()
 ****************************************************************
 * @brief
 *  This routine returns drive capacity from pvd object for the given disk set.
 *  
 *
 * @param disk_set_p - disk set information.
 * @param drive_capacity_p - drive capacity to return.
 *
 * @return None.
 *
 * @author
 *  06/18/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_get_drive_capacity(fbe_test_raid_group_disk_set_t *disk_set_p, 
                                          fbe_lba_t *drive_capacity_p)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_object_id_t                 pvd_id;

    /* get the pvd object id. */
    status = fbe_api_provision_drive_get_obj_id_by_location(disk_set_p->bus,
                                                   disk_set_p->enclosure,
                                                   disk_set_p->slot,
                                                   &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the drive capacity from pvd object */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    

    *drive_capacity_p = pvd_info.capacity;

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_get_drive_offset()
 ****************************************************************
 * @brief
 *  This routine returns drive default offset from pvd object for 
 *  the given disk set.
 *  
 *
 * @param disk_set_p - disk set information.
 * @param drive_offset_p - drive offset to return.
 *
 * @return None.
 *
 * @author
 *  06/18/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_get_drive_offset(fbe_test_raid_group_disk_set_t *disk_set_p, 
                                        fbe_lba_t *drive_offset_p)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_object_id_t                 pvd_id;

    /* get the pvd object id. */
    status = fbe_api_provision_drive_get_obj_id_by_location(disk_set_p->bus,
                                                   disk_set_p->enclosure,
                                                   disk_set_p->slot,
                                                   &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the drive capacity from pvd object */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    

    *drive_offset_p = pvd_info.default_offset;

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_raid_group_refresh_extra_disk_info()
 ****************************************************************
 * @brief
 *  Refresh all the extra drive slot information for a raid group
 *  configuration array.  This info can change over time
 *  as permanent spares get swapped in. This routine takes the drive 
 *  capacity into account while select extra drive for rg from unused
 *  drive pool.
 *
 * @note
 *  Right now this routine select extra drive from unused drive pool only
 *  if it's capacity match with rg drive capacity. The reason is spare drive
 *  selection select drive based on removed drive capacity and we don't know 
 *  yet which drive is going to remove from the raid group next. Hence 
 *  while creating the rg, test does select drive for same capacity and while
 *  sparing, it will use extra drive for same capacity to make sure that when
 *  we run the test on hardware, there should not be any drive selection problem.
 *
 * @todo
 *  When there will be change in spare selection to select drive based on minimum rg
 *  drive capacity, we need to refer minimum drive capacity from rg while select extra drive.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None.
 *
 * @author
 *  05/13/2011 - Created. Amit Dhaduk
 *  06/18/2011 - Modified. Amit Dhaduk
 *  
 *
 ****************************************************************/
void fbe_test_sep_util_raid_group_refresh_extra_disk_info(fbe_test_rg_configuration_t * rg_config_p, 
                                                    fbe_u32_t raid_group_count)
{

    fbe_u32_t                               rg_index;
    fbe_u32_t                               index;
    fbe_u32_t                               removed_drive_index;
    fbe_u32_t                               drive_type;
    fbe_u32_t                               extra_drive_index;
    fbe_u32_t                               unused_drive_index;
    fbe_lba_t                               rg_drive_capacity;
    fbe_lba_t                               max_rg_drive_capacity;
    fbe_lba_t                               unused_drive_capacity;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_block_size_t                   block_size;
    fbe_bool_t                              skip_removed_drive;
    fbe_u32_t                               alternate_block_size;
    fbe_u32_t                               drive_type_index;   
    fbe_u32_t                               block_size_index;
    fbe_u32_t                               available_drive_count[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];

    /* find the all available drive info */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* find the all available unused pvd info */
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);
    for (drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++) {
        for(block_size_index = 0; block_size_index < FBE_TEST_BLOCK_SIZE_LAST; block_size_index++){
            available_drive_count[block_size_index][drive_type_index] = unused_pvd_locations.drive_counts[block_size_index][drive_type_index];
        }
    }

    /* Just loop over all our raid groups, refreshing extra disk info with available unused pvd .
    */    
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        drive_type = rg_config_p[rg_index].drive_type;

        /* find out the block size */
        switch (rg_config_p[rg_index].block_size) {
            case 520:
                block_size = FBE_TEST_BLOCK_SIZE_520;
                break;
            case 4160:
                block_size = FBE_TEST_BLOCK_SIZE_4160;
                break;
            case FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE:
                block_size = FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE;
                break;
            default:
                 block_size = FBE_TEST_BLOCK_SIZE_INVALID;
                 MUT_ASSERT_TRUE_MSG(FBE_FALSE, " refresh extra disk info - Unsupported block size\n");
            return;
                break;
        };
        
        max_rg_drive_capacity = 0;

        /* get maximum drive capacity from raid group
         */
        for(index = 0 ; index < rg_config_p[rg_index].width; index++)
        {
            skip_removed_drive = FBE_FALSE;

            /* skip the drive if it is removed
             */
            for(removed_drive_index =0; removed_drive_index <  rg_config_p[rg_index].num_removed; removed_drive_index++)
            {
                if(index == rg_config_p[rg_index].drives_removed_array[removed_drive_index])
                {
                    skip_removed_drive = FBE_TRUE;
                    break;
                }
            }

            if (skip_removed_drive == FBE_TRUE)
            {
                continue;
            }
        
            /* find out the drive capacity */
            fbe_test_sep_util_get_drive_capacity(&rg_config_p[rg_index].rg_disk_set[index], &rg_drive_capacity);

            /* mut_printf(MUT_LOG_TEST_STATUS, "== refresh extra disk  -rg disk (%d_%d_%d), cap 0x%x . ==", 
                    rg_config_p[rg_index].rg_disk_set[index].bus, rg_config_p[rg_index].rg_disk_set[index].enclosure, rg_config_p[rg_index].rg_disk_set[index].slot, rg_drive_capacity );
            */

            if (max_rg_drive_capacity < rg_drive_capacity)
            {
                max_rg_drive_capacity = rg_drive_capacity;
            }
        }

        extra_drive_index = 0;
        /* If required, scan all the unused drive and pick up the drive which 
         * is not yet selected and match the capacity. 
         */
        if (rg_config_p[rg_index].num_of_extra_drives < 1)
        {
            /* Just print an informational message.
             */
            mut_printf(MUT_LOG_TEST_STATUS,  "refresh extra: num of extra drives: %d rg_index: %d raid id: %d type: %d width: %d",
                       rg_config_p[rg_index].num_of_extra_drives, rg_index, rg_config_p[rg_index].raid_group_id,
                       rg_config_p[rg_index].raid_type, rg_config_p[rg_index].width);
            continue;
        }
        /* For mixed we will randomize which one we pick.
         * But if there are no more of that type we choose the other.
         */
        if (fbe_random() % 2) {
            block_size = FBE_TEST_BLOCK_SIZE_4160;
            alternate_block_size = FBE_TEST_BLOCK_SIZE_520;
        } else {
            block_size = FBE_TEST_BLOCK_SIZE_520;
            alternate_block_size = FBE_TEST_BLOCK_SIZE_4160;
        }

        /* Until all the extra drives are found.
         */
        while (extra_drive_index != rg_config_p[rg_index].num_of_extra_drives) {
            /* If no more of that size, choose the other size. */
            if ((available_drive_count[block_size][drive_type] == 0) &&
                (available_drive_count[alternate_block_size][drive_type] != 0)) {
                block_size = alternate_block_size;
            } else if ((available_drive_count[block_size][drive_type] == 0) &&
                       (available_drive_count[alternate_block_size][drive_type] == 0)) {
                MUT_FAIL_MSG("Not enough drives remaining");
            }

            /* Validate the number of extra drives.
             */
            for(unused_drive_index = 0; 
                 unused_drive_index < unused_pvd_locations.drive_counts[block_size][drive_type]; 
                 unused_drive_index++)
            {
                /* check if drive is not yet selected 
                 */
                if (unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot != FBE_INVALID_SLOT_NUM) {
        
                    fbe_test_sep_util_get_drive_capacity(&unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index], 
                                                        &unused_drive_capacity);
        
                    /* check if the drive capacity match with rg max capacity 
                     */
                    if(max_rg_drive_capacity == unused_drive_capacity) {
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].bus = 
                                unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].bus;
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].enclosure = 
                                unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].enclosure;
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].slot = 
                                unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot;
        
                        /*
                        mut_printf(MUT_LOG_TEST_STATUS,  "filling extra disk pvd(%d_%d_%d) block size %d, drive type %d, rg drive capacity 0x%x, unused drive cnt %d, un dri cap 0x%x, rg index %d\n",
                            rg_config_p[rg_index].extra_disk_set[extra_drive_index].bus,
                            rg_config_p[rg_index].extra_disk_set[extra_drive_index].enclosure,
                            rg_config_p[rg_index].extra_disk_set[extra_drive_index].slot,
                            block_size, drive_type, rg_drive_capacity, unused_pvd_locations.drive_counts[block_size][drive_type] ,
                            unused_drive_capacity, rg_index); 
                        */
        
                        extra_drive_index++;
        
                        /* invalid the drive slot information for selected drive so that next time it will skip from selection
                        */
                        unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot = FBE_INVALID_SLOT_NUM;
        
                        /* No longer available 
                         */
                        available_drive_count[block_size][drive_type]--;

                        /* check if all extra drive selected for this rg 
                        */
                        if(extra_drive_index == rg_config_p[rg_index].num_of_extra_drives){
                            break;
                        }
                    }
                }
            } 
        }

        /* Validate that sufficient `extra' drives were located.
         */
        if (extra_drive_index != rg_config_p[rg_index].num_of_extra_drives)
        {

            /* We had make sure that have enough drives during setup. Let's fail the test and see
             * why could not get the suitable extra drive
             */
            mut_printf(MUT_LOG_TEST_STATUS,  "Do not find unused pvd, block size %d, drive type %d, max rg drive capacity 0x%llx, unused drive cnt %d, rg index %d\n",
                block_size, drive_type,
	        (unsigned long long)max_rg_drive_capacity,
	        unused_pvd_locations.drive_counts[block_size][drive_type], rg_index );

            /* fail the test to figure out that why test did not able to find out the unused pvd */
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Do not find unused pvd while refresh extra disk info table\n");
        }

    } /* end for ( rg_index = 0; rg_index < raid_group_count; rg_index++) */

    return;
}
/**************************************
 * end fbe_test_sep_util_raid_group_refresh_disk_info()
 **************************************/

fbe_status_t fbe_test_sep_util_lun_initiate_verify(fbe_object_id_t in_lun_object_id, 
                                                   fbe_lun_verify_type_t in_verify_type,
                                                   fbe_bool_t in_b_verify_entire_lun,
                                                   fbe_lba_t in_verify_start_lba,
                                                   fbe_block_count_t in_verify_blocks)
{
    return fbe_api_lun_initiate_verify(in_lun_object_id, 
                                       FBE_PACKET_FLAG_NO_ATTRIB, 
                                       in_verify_type,
                                       in_b_verify_entire_lun,
                                       in_verify_start_lba,
                                       in_verify_blocks);    
}

#if 0
fbe_status_t fbe_test_util_initiate_verify_on_specific_area(fbe_object_id_t in_raid_object_id, 
                                                            fbe_raid_verify_type_t in_verify_type,
                                                            fbe_lba_t  verify_lba,
                                                            fbe_lba_t  block_count)
{
    return fbe_test_only_api_initiate_verify_on_specific_area(in_raid_object_id, FBE_PACKET_FLAG_NO_ATTRIB, 
                                                              in_verify_type, verify_lba,
                                                              block_count);    
}
#endif

fbe_status_t fbe_test_verify_start_on_all_rg_luns(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_lun_verify_type_t verify_type)
{
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return fbe_api_raid_group_initiate_verify(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB, verify_type);    
}
fbe_status_t fbe_test_sep_util_raid_group_initiate_verify(fbe_object_id_t in_raid_object_id, 
                                                         fbe_lun_verify_type_t in_verify_type)
{
    return fbe_api_raid_group_initiate_verify(in_raid_object_id, FBE_PACKET_FLAG_NO_ATTRIB, in_verify_type);    
}

fbe_status_t fbe_test_sep_util_lun_get_verify_report(fbe_object_id_t in_lun_object_id, 
                                    fbe_lun_verify_report_t* out_verify_report_p)
{
    return fbe_api_lun_get_verify_report(in_lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB, out_verify_report_p);    
}

fbe_status_t fbe_test_sep_util_lun_get_verify_status(fbe_object_id_t in_lun_object_id, 
                                    fbe_lun_get_verify_status_t* out_verify_status_p)
{
    return fbe_api_lun_get_verify_status(in_lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB, out_verify_status_p);    
}

fbe_status_t fbe_test_sep_util_lun_clear_verify_report(fbe_object_id_t in_lun_object_id)
{
    return fbe_api_lun_clear_verify_report(in_lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);    
}

fbe_status_t fbe_test_sep_util_lun_config_clear_total_error_count(fbe_test_logical_unit_configuration_t *logical_unit_configuration_p)
{
    MUT_ASSERT_NOT_NULL_MSG(logical_unit_configuration_p, "clear total: lu config is NULL \n");
    logical_unit_configuration_p->total_error_count = 0;
    return FBE_STATUS_OK;    
}

fbe_status_t fbe_test_sep_util_lun_config_increment_total_error_count(fbe_test_logical_unit_configuration_t *logical_unit_configuration_p,
                                                                      fbe_u32_t pass_count,
                                                                      fbe_u32_t new_errors_to_add)
{
    MUT_ASSERT_NOT_NULL_MSG(logical_unit_configuration_p, "increment total: lu config is NULL \n");
    mut_printf(MUT_LOG_TEST_STATUS, 
               "increment total: lun number: %d pass_count: %d current total: %d increment: %d \n",
               logical_unit_configuration_p->lun_number, pass_count,
               logical_unit_configuration_p->total_error_count, new_errors_to_add);
    logical_unit_configuration_p->total_error_count += new_errors_to_add;
    return FBE_STATUS_OK;    
}


void fbe_test_sep_util_wait_for_rg_metada_verify_completion(fbe_object_id_t            rg_object_id,
                                                            fbe_u32_t                  pass_count)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           max_wait_in_seconds = 600;
    fbe_u32_t                           sleep_time;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   current_target = FBE_TRANSPORT_SP_A;
    fbe_u32_t                           metadata_verify_pass_count = 0;

    /* First get the sp id this was invoked on and out peer.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    current_target = target_invoked_on;

    for (sleep_time = 0; sleep_time < (max_wait_in_seconds*10); sleep_time++)
    {
        /*! Always put the target server back to the one this method was 
         *  invoked on.
         *
         *  @todo Find out who is switching the target server
         */
        current_target = fbe_api_transport_get_target_server();
        if (current_target != target_invoked_on)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "wait for rg metadata verify complete: current target: %d different than target invoked on: %d",
                       current_target, target_invoked_on);
            MUT_ASSERT_INT_EQUAL(target_invoked_on, current_target);

            /*! @todo For now simply set the target back to the original
             */
            status = fbe_api_transport_set_target_server(target_invoked_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }


        // Get metadata verify pass count for the RG.
        fbe_api_raid_group_get_paged_metadata_verify_pass_count(rg_object_id, &metadata_verify_pass_count);

        if(metadata_verify_pass_count == pass_count)
        {
            return;
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    //  The verify operation took too long and timed out.
    MUT_ASSERT_TRUE(0)
    return;

}


/*!**************************************************************
 * fbe_test_sep_util_wait_for_lun_initial_verify_completion()
 ****************************************************************
 * @brief
 *  Wait until the initial verify finishes.
 *
 * @param lun_object_id - Lun to wait for the initial verify to complete on.
 *
 * @return None.
 *
 * @author
 *  12/14/2012 - Created - Kevin Schleicher
 *
 ****************************************************************/
void fbe_test_sep_util_wait_for_initial_lun_verify_completion(fbe_object_id_t lun_object_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           max_wait_in_seconds = 600;
    fbe_u32_t                           sleep_time;
    fbe_api_lun_get_lun_info_t          lun_info;

    for (sleep_time = 0; sleep_time < (max_wait_in_seconds*10); sleep_time++)
    {
        // Get the lun info for this lun
        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (lun_info.b_initial_verify_already_run == FBE_TRUE)
        {
           return;
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    //  The verify operation took too long and timed out.
    MUT_ASSERT_TRUE(0)
    return;

}

void fbe_test_sep_util_wait_for_lun_verify_completion(fbe_object_id_t            object_id,
                                                      fbe_lun_verify_report_t*   verify_report_p,
                                                      fbe_u32_t                  pass_count)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           max_wait_in_seconds = 600;
    fbe_u32_t                           sleep_time;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   current_target = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   peer;
    fbe_lun_verify_report_t verify_report_peer;

    /* First get the sp id this was invoked on and out peer.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    current_target = target_invoked_on;

    for (sleep_time = 0; sleep_time < (max_wait_in_seconds*10); sleep_time++)
    {
        /*! Always put the target server back to the one this method was 
         *  invoked on.
         *
         *  @todo Find out who is switching the target server
         */
        current_target = fbe_api_transport_get_target_server();
        if (current_target != target_invoked_on)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "wait for lun verify complete: current target: %d different than target invoked on: %d",
                       current_target, target_invoked_on);
            MUT_ASSERT_INT_EQUAL(target_invoked_on, current_target);

            /*! @todo For now simply set the target back to the original
             */
            status = fbe_api_transport_set_target_server(target_invoked_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        // Get the verify report for this LUN.
        fbe_test_sep_util_lun_get_verify_report(object_id, verify_report_p);

        if(verify_report_p->pass_count == pass_count)
        {
           return;
        }

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            peer = (current_target == FBE_SIM_SP_B) ? FBE_SIM_SP_A : FBE_SIM_SP_B;
            fbe_api_sim_transport_set_target_server(peer);
            fbe_test_sep_util_lun_get_verify_report(object_id, &verify_report_peer);

            fbe_api_sim_transport_set_target_server(current_target);
            if(verify_report_peer.pass_count == pass_count)
            {
                return;
            }
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    //  The verify operation took too long and timed out.
    MUT_ASSERT_TRUE(0)
    return;

}


fbe_lba_t fbe_test_sep_util_get_blocks_per_lun(fbe_lba_t rg_capacity,
                                               fbe_u32_t luns_per_rg,
                                               fbe_u32_t data_drives,
                                               fbe_chunk_size_t chunk_size)
{
    fbe_lba_t blocks_per_lun;
    fbe_lba_t per_drive_lun_blocks;

    /* First determine roughly how many blocks each LUN can have.
     */
    blocks_per_lun = (rg_capacity / luns_per_rg);

    /* Next determine how much this is per drive for each LUN.
     */
    per_drive_lun_blocks = blocks_per_lun / data_drives;

    /* Round the per drive lun blocks down if we are not a multiple 
     * of the chunk size. 
     * All LUNs must be a multiple of the chunk size. 
     */
    if (per_drive_lun_blocks % chunk_size)
    {
        per_drive_lun_blocks -= (per_drive_lun_blocks % chunk_size);
    }

    /* To get the full lun capacity, just multiply the per drive blocks
     * by the number of data drives. 
     */
    blocks_per_lun = data_drives * per_drive_lun_blocks;

    return blocks_per_lun;
}

fbe_status_t fbe_test_sep_util_get_raid_type_string(fbe_raid_group_type_t raid_group_type,
                                                    const fbe_char_t **string_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (raid_group_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            *string_p = "raid-0";
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            *string_p = "raid-5";
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            *string_p = "raid-3";
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            *string_p = "raid-6";
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            *string_p = "raid-10";
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            *string_p = "raid-1";
            break;
        default:
            *string_p = "unknown";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return status;
}
fbe_status_t fbe_test_sep_util_get_block_size_string(fbe_block_size_t block_size_enum,
                                                     const fbe_char_t **string_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (block_size_enum)
    {
        case 4160:
            *string_p = "4160";
            break;
        case 520:
            *string_p = "520";
            break;
        case FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE:
            *string_p = "mixed 4k+520";
            break;
        default:
            *string_p = "unknown-block-size";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return status;
}

void fbe_test_sep_util_get_raid_group_object_id(fbe_test_rg_configuration_t * rg_config_p,
                                                fbe_object_id_t * rg_object_id_p)

{
    /* initialize the raid group object id as invalid. */
    *rg_object_id_p = FBE_OBJECT_ID_INVALID;

    /* get the RAID group object-id. */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, rg_object_id_p);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *rg_object_id_p);
}

void fbe_test_sep_util_get_lun_object_id(fbe_object_id_t first_object_id,
                                         fbe_u32_t width,
                                         fbe_object_id_t *object_id_p)
{
    fbe_object_id_t vd_first_object_id = first_object_id;
    fbe_object_id_t vd_last_object_id = vd_first_object_id + width - 1;
    fbe_object_id_t rg_object_id = vd_last_object_id + 1;
    fbe_object_id_t lun_object_id = rg_object_id + 1;

    /* The luns start after the raid group.
     */
    *object_id_p = lun_object_id;
    return;
}

fbe_status_t fbe_test_sep_util_get_raid_class_info(fbe_raid_group_type_t raid_type,
                                                   fbe_class_id_t class_id,
                                                   fbe_u32_t width,
                                                   fbe_lba_t capacity,
                                                   fbe_u16_t *data_disks_p,
                                                   fbe_lba_t *capacity_p)
{
    fbe_api_raid_group_class_get_info_t get_info;
    fbe_status_t status;
    get_info.raid_type = raid_type;
    get_info.width = width;
    get_info.b_bandwidth = FBE_FALSE;
    get_info.exported_capacity = capacity;

    status = fbe_api_raid_group_class_get_info(&get_info, class_id);
    if (data_disks_p != NULL)
    {
    *data_disks_p = get_info.data_disks;
    }
    if (capacity_p != NULL)
    {
    *capacity_p = get_info.imported_capacity;
    }
    return status;
}

fbe_status_t fbe_test_sep_util_fill_raid_group_disk_set(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_raid_group_disk_set_t *disk_set_520_p,
                                                        fbe_test_raid_group_disk_set_t *disk_set_4160_p)
{
   /* For a given configuration fill in the actual drives we will
    * be using.  This function makes it easy to create flexible
    * configurations without having to know exactly which disk goes
    * in which raid group.  We first create a set of disk sets for
    * the 4160 and 520 disks.  Then we call this function and
    * pick drives from those disk sets to use in our raid groups.
     */
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t rg_disk_index;
    fbe_u32_t extra_disk_index;
    fbe_u32_t disk_set_index_520 = 0;
    fbe_u32_t disk_set_index_4160 = 0;

    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 4160 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (raid_configuration_list_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(&raid_configuration_list_p[rg_index]);
        }
        if (!fbe_test_rg_config_is_enabled(&raid_configuration_list_p[rg_index]))
        {
            continue;
        }
        for(rg_disk_index = 0; rg_disk_index < raid_configuration_list_p[rg_index].width; rg_disk_index++)
        {
            fbe_test_raid_group_disk_set_t * disk_set_p = NULL;

            /* Fill in the next 4160 drive.
             */
            if(raid_configuration_list_p[rg_index].block_size == 520)
            {
                /* Fill in the next 520 drive.
                 */
                status = fbe_test_sep_util_get_next_disk_in_set(disk_set_index_520, 
                                                                disk_set_520_p,
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK) { return status; }
                raid_configuration_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                disk_set_index_520++;
            }
            else if (raid_configuration_list_p[rg_index].block_size == 4160)
            {
                status = fbe_test_sep_util_get_next_disk_in_set(disk_set_index_4160, 
                                                                disk_set_4160_p,
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK) { return status; }
                raid_configuration_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                disk_set_index_4160++;
            }
            else
            {
                /* Else invalid block size
                 */
                MUT_ASSERT_INT_EQUAL(520, raid_configuration_list_p[rg_index].block_size);
            }
        }

        /* fill out the extra disk informaiton if extra drive count is set by consumer test
          */
        for(extra_disk_index = 0; extra_disk_index < raid_configuration_list_p[rg_index].num_of_extra_drives; extra_disk_index++)
        {
            fbe_test_raid_group_disk_set_t * disk_set_p = NULL;

            /* Fill in the next 520 extra drive.
             */
            if(raid_configuration_list_p[rg_index].block_size == 520)
            {
                /* Fill in the next 520 extra drive.
                 */
                status = fbe_test_sep_util_get_next_disk_in_set(disk_set_index_520, 
                                                                disk_set_520_p,
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK) { return status; }
                raid_configuration_list_p[rg_index].extra_disk_set[extra_disk_index] = *disk_set_p;
                disk_set_index_520++;
            }
            else if(raid_configuration_list_p[rg_index].block_size == 4160)
            {
                status = fbe_test_sep_util_get_next_disk_in_set(disk_set_index_4160, 
                                                                disk_set_4160_p,
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK) { return status; }
                raid_configuration_list_p[rg_index].extra_disk_set[extra_disk_index] = *disk_set_p;
                disk_set_index_4160++;
            }
            else
            {
                /* Block size not supported*/
                MUT_ASSERT_INT_EQUAL(520, raid_configuration_list_p[rg_index].block_size);
            }

        }

        
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_test_sep_util_discover_raid_group_disk_set(fbe_test_rg_configuration_t * rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_discovered_drive_locations_t *drive_locations_p)
{
   /* For a given configuration fill in the actual drives we will
    * be using.  This function makes it easy to create flexible
    * configurations without having to know exactly which disk goes
    * in which raid group.  We first create a set of disk sets for
    * the 4160 and 520 disks.  Then we call this function and
    * pick drives from those disk sets to use in our raid groups.
     */
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t rg_disk_index;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_drive_type_t        drive_type;
    fbe_test_block_size_t   block_size;
    fbe_test_block_size_t   second_block_size;
    fbe_bool_t              b_disks_found;
    fbe_test_raid_group_disk_set_t * disk_set_p = NULL;
    fbe_block_size_t current_block_size;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));


    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 4160 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p[rg_index].block_size, &block_size);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        if (rg_config_p[rg_index].block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE) {
            second_block_size = FBE_TEST_BLOCK_SIZE_4160;
        }
        if (rg_config_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(&rg_config_p[rg_index]);
        }
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }
        if (rg_config_p[rg_index].b_use_fixed_disks) {
            continue;
        }

        /* Discover the Drive type we will use for this RG
         */
        b_disks_found = false;
        for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
        {
            if (rg_config_p[rg_index].block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE) {
                fbe_u32_t first_count = rg_config_p[rg_index].width / 2 + ((rg_config_p[rg_index].width % 2) ? 1 : 0);
                fbe_u32_t second_count = rg_config_p[rg_index].width / 2;
                fbe_u32_t first_extra_count = rg_config_p[rg_index].num_of_extra_drives / 2 + ((rg_config_p[rg_index].num_of_extra_drives % 2) ? 1 : 0);
                fbe_u32_t second_extra_count = rg_config_p[rg_index].num_of_extra_drives / 2;
                /* If we are mixed, make sure we have enough of each type.
                 * Half the drives are of one type, and the other half of another type. 
                 */
                if ( ((first_count + first_extra_count) <= local_drive_counts[block_size][drive_type]) &&
                     ((second_count + second_extra_count) <= local_drive_counts[second_block_size][drive_type])) {
                    rg_config_p[rg_index].drive_type = drive_type;
                    for (rg_disk_index = 0; rg_disk_index < rg_config_p[rg_index].width; rg_disk_index++) {
                        disk_set_p = NULL;
                        if ((rg_disk_index % 2) == 0) {
                            current_block_size = FBE_TEST_BLOCK_SIZE_520;
                        } else {
                            current_block_size = FBE_TEST_BLOCK_SIZE_4160;
                        }
                        local_drive_counts[current_block_size][drive_type]--;

                        /* Fill in the next drive.
                         */
                        status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[current_block_size][drive_type], 
                                                                        drive_locations_p->disk_set[current_block_size][drive_type],
                                                                        &disk_set_p);
                        if (status != FBE_STATUS_OK) {
                            return status;
                        }
                        rg_config_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                    }

                    /* populate required extra disk set if extra drive count is set by consumer test
                     */
                    for (rg_disk_index = 0; rg_disk_index < rg_config_p[rg_index].num_of_extra_drives; rg_disk_index++) {
                        disk_set_p = NULL;

                        if ((rg_disk_index % 2) == 0) {
                            current_block_size = FBE_TEST_BLOCK_SIZE_520;
                        } else {
                            current_block_size = FBE_TEST_BLOCK_SIZE_4160;
                        }

                        local_drive_counts[current_block_size][drive_type]--;
                        
                        /* Fill in the next drive.
                         */
                        status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[current_block_size][drive_type], 
                                                                        drive_locations_p->disk_set[current_block_size][drive_type],
                                                                        &disk_set_p);
                        if (status != FBE_STATUS_OK) {
                            return status;
                        }
                        rg_config_p[rg_index].extra_disk_set[rg_disk_index] = *disk_set_p;
                    }
                    b_disks_found = true;
                    break;
                }
            } else if ((rg_config_p[rg_index].width + rg_config_p[rg_index].num_of_extra_drives) 
                       <= local_drive_counts[block_size][drive_type])
            {
                rg_config_p[rg_index].drive_type = drive_type;
                for (rg_disk_index = 0; rg_disk_index < rg_config_p[rg_index].width; rg_disk_index++)
                {
                    disk_set_p = NULL;
                    local_drive_counts[block_size][drive_type]--;

                    /* Fill in the next drive.
                     */
                    status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                    drive_locations_p->disk_set[block_size][drive_type],
                                                                    &disk_set_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                    rg_config_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                }

                /* populate required extra disk set if extra drive count is set by consumer test
                 */
                for (rg_disk_index = 0; rg_disk_index < rg_config_p[rg_index].num_of_extra_drives; rg_disk_index++)
                {
                    disk_set_p = NULL;
                    local_drive_counts[block_size][drive_type]--;

                    /* Fill in the next drive.
                     */
                    status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                    drive_locations_p->disk_set[block_size][drive_type],
                                                                    &disk_set_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                    rg_config_p[rg_index].extra_disk_set[rg_disk_index] = *disk_set_p;
                }
                
                b_disks_found = true;
                break;
            }
        }
        if(b_disks_found == false)
        {
            /* We did make sure we had enough drives before entering this function
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}


fbe_status_t fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    /* This function is derived from fbe_test_sep_util_discover_raid_group_disk_set() to consider
     * drive capacity into account while running the test on hardware. It only selects  
     * drives having same capacity for each drive type. Right now Job service select spare drive 
     * based on removed drive capacity and if array has drives with two different capacity for same type
     * then at some point of time there is possibility that test does not able to find suitable
     * spare drive. Hence we have added temporary code to select drive for same capacity to run 
     * some fbe_tests on hardware. Once we will change the spare drive selection algorithm in job service
     * to select drive based on minimum rg drive capacity, we do not need this special handling to 
     * consider drive capacity.
    
    
    /* For a given configuration fill in the actual drives we will
     * be using.  This function makes it easy to create flexible
     * configurations without having to know exactly which disk goes
     * in which raid group.  We first create a set of disk sets for
     * the 4160 and 520 disks.  Then we call this function and
     * pick drives from those disk sets to use in our raid groups.
     */

    fbe_status_t            status;
    fbe_u32_t               rg_index;
    fbe_u32_t               rg_disk_index;
    fbe_u32_t               drive_index;
    fbe_u32_t               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_lba_t               drive_reference_capacity[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_drive_type_t        drive_type;
    fbe_test_block_size_t   block_size;
    fbe_bool_t              b_rg_disks_found;
    fbe_bool_t              b_extra_disks_found;
    fbe_lba_t               drive_capacity;
    fbe_test_raid_group_disk_set_t * disk_set_p = NULL;
  

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));


    /* initialize drive reference capacity for each drive type 
     */
    for(block_size =0; block_size < FBE_TEST_BLOCK_SIZE_LAST; block_size++)
    {
        for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
        {
            if(local_drive_counts[block_size][drive_type] > 0)
            {
                /* get the first drive index for each drive type
                 */
                drive_index = local_drive_counts[block_size][drive_type] - 1;

                /* Pick up the first disk set for each drive type 
                 */
                status = fbe_test_sep_util_get_next_disk_in_set(drive_index, 
                                                                drive_locations_p->disk_set[block_size][drive_type],
                                                                &disk_set_p);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
            
                /* get drive capacity to consider it as reference capacity 
                 */
                fbe_test_sep_util_get_drive_capacity(disk_set_p,  &drive_capacity);
                drive_reference_capacity[block_size][drive_type] = drive_capacity;
                mut_printf(MUT_LOG_TEST_STATUS, "=== reference drive capacity block size %d, drive type %d, cap 0x%llx ==", 
                    block_size, drive_type, (unsigned long long)drive_capacity);
            }
        }
    }

    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 4160 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(raid_configuration_list_p[rg_index].block_size, &block_size);
        if (raid_configuration_list_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(&raid_configuration_list_p[rg_index]);
        }
        if (!fbe_test_rg_config_is_enabled(&raid_configuration_list_p[rg_index]))
        {
            continue;
        }

        /* Discover the Drive type we will use for this RG
         */
        b_rg_disks_found= FBE_FALSE;
        b_extra_disks_found = FBE_FALSE;
        for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
        {
            if(raid_configuration_list_p[rg_index].num_of_extra_drives == 0)
            {
                /* fill out rg disk set which does not require extra disk while running the test */
                if(raid_configuration_list_p[rg_index].width <= local_drive_counts[block_size][drive_type])
                {
                    raid_configuration_list_p[rg_index].drive_type = drive_type;
                    for (rg_disk_index = 0; rg_disk_index < raid_configuration_list_p[rg_index].width; rg_disk_index++)
                    {
                        disk_set_p = NULL;
                        local_drive_counts[block_size][drive_type]--;

                        /* Fill in the next drive.
                         */
                        status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                        drive_locations_p->disk_set[block_size][drive_type],
                                                                        &disk_set_p);
                        if (status != FBE_STATUS_OK)
                        {
                            return status;
                        }
                        raid_configuration_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                    }
                    b_rg_disks_found = FBE_TRUE;
                    break;
                } /* end  if(raid_configuration_list_p[rg_index].width <= local_drive_counts[block_size][drive_type]) */
            } /* end if(raid_configuration_list_p[rg_index].num_of_extra_drives == 0) */
            else
            {
                /* fill out rg disk set which require extra disks while running the test */
                raid_configuration_list_p[rg_index].drive_type = drive_type;
                rg_disk_index = 0;

                /* loop through available drives and select drive if it's capacity match with reference drive capacity
                 */
                while(local_drive_counts[block_size][drive_type] != 0)
                {
                    local_drive_counts[block_size][drive_type]--;
                    disk_set_p = NULL;

                    /* get the next available drive.
                    */
                    status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                        drive_locations_p->disk_set[block_size][drive_type],
                                                        &disk_set_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }

                    /* get the drive capacity */
                    fbe_test_sep_util_get_drive_capacity(disk_set_p,  &drive_capacity);

                    /* select the drive only if it's capacity match with reference drive capacity
                    */
                    if(drive_reference_capacity[block_size][drive_type] == drive_capacity)
                    {
                        raid_configuration_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                        rg_disk_index ++;
/*                        mut_printf(MUT_LOG_TEST_STATUS, "=== rg drive %d_%d_%d,  block size %d, drive type %d, cap 0x%x ==\n",disk_set_p->bus, 
                            disk_set_p->enclosure,
                            disk_set_p->slot,
                            block_size, drive_type, drive_capacity); */

                        /* check if all raid group drives are selected
                         */
                        if(rg_disk_index == raid_configuration_list_p[rg_index].width)
                        {
                            b_rg_disks_found = FBE_TRUE;
                            break;
                        }
                    } /*  end if(drive_reference_capacity[block_size][drive_type] == drive_capacity) */
                } /* end while(local_drive_counts[block_size][drive_type] != 0) */

                /* fill out extra disk set information */

                drive_index = 0;
                /* loop through available drives and pickup only those drive which match capacity with reference drive capacity
                */
                while(local_drive_counts[block_size][drive_type] != 0)
                {
                    local_drive_counts[block_size][drive_type]--;
                    disk_set_p = NULL;

                    /* Get the next available drive.
                    */
                    status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                        drive_locations_p->disk_set[block_size][drive_type],
                                                        &disk_set_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }

                    fbe_test_sep_util_get_drive_capacity(disk_set_p, &drive_capacity);

                    /* select the drive only if it's capacity match with reference drive capacity
                    */
                    if(drive_reference_capacity[block_size][drive_type] == drive_capacity)
                    {
                        raid_configuration_list_p[rg_index].extra_disk_set[drive_index] = *disk_set_p;
                        drive_index ++;

                        /* mut_printf(MUT_LOG_TEST_STATUS, "=== extra drive %d_%d_%d,  rg index %d,  block size %d, drive type %d, cap 0x%x ==\n",
                            disk_set_p->bus, 
                            disk_set_p->enclosure,
                            disk_set_p->slot,
                            rg_index,
                            block_size, drive_type, drive_capacity); */

                        /* check if all extra drives are selected
                         */
                        if(drive_index == raid_configuration_list_p[rg_index].num_of_extra_drives)
                        {
                            b_extra_disks_found = FBE_TRUE;
                            break;
                        }
                    }

                } /* end while(local_drive_counts[block_size][drive_type] != 0) */

                if((b_rg_disks_found == FBE_TRUE) && (b_extra_disks_found == FBE_TRUE))
                {
                    break;
                }
            }/* end else of if(raid_configuration_list_p[rg_index].num_of_extra_drives == 0)*/
        } /* end for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++) */

        if(raid_configuration_list_p[rg_index].num_of_extra_drives == 0)
        {
            if(b_rg_disks_found == FBE_FALSE)
            {
                /* We did make sure we had enough drives before entering this function
                */
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            if((b_rg_disks_found == FBE_FALSE) || (b_extra_disks_found == FBE_FALSE))
            {

               /* disable rg for this pass since we do not have sufficient disk for same type and capacity
                * and will try to run it in next pass. It may possible that some test will not run on hardware
                * if do not have enough drives with same type and capacity
                */
                raid_configuration_list_p[rg_index].b_create_this_pass = FBE_FALSE;
                raid_configuration_list_p[rg_index].b_already_tested = FBE_FALSE;
                raid_configuration_list_p[rg_index].b_valid_for_physical_config = FBE_FALSE;
                mut_printf(MUT_LOG_TEST_STATUS, "=== skip rg index %d because of insufficient disk with same capacity, width %d ==\n", 
                    rg_index,raid_configuration_list_p[rg_index].width );
            }
        }
    } /* end for(rg_index = 0; rg_index < raid_group_count; rg_index++) */
    return FBE_STATUS_OK;
}


//@todo: herry_monster: enable for herry_monster case
//fbe_status_t fbe_test_sep_util_fill_rg_disks( fbe_test_rg_configuration_t *raid_confg_list_p,
//                                                                               fbe_u32_t rg_index,
//                                                                               fbe_u32_t * disk_set_index,
//                                                                               fbe_block_count_t rg_drive_capacity, 
//                                                                               fbe_test_raid_group_disk_set_t* in_disk_set_p,
//                                                                               fbe_block_count_t *in_drive_cap)
//{
//   /* This function checks the disks capacity for the given raid group configuration
//    * and then fill it accordingly.
//     */
//
//    fbe_u32_t rg_disk_index;
//    fbe_status_t status = FBE_STATUS_OK;
//    
//    for(rg_disk_index = 0; rg_disk_index < raid_confg_list_p[rg_index].width; rg_disk_index++)
//    {
//        fbe_test_raid_group_disk_set_t * disk_set_p = NULL;
//
//        while( (*disk_set_index) <= FBE_TEST_MAX_DRIVE_LOCATIONS)
//        {
//            /* We will brows through all the disks for desired capacity
//             */
//            if(in_drive_cap[*disk_set_index] >= rg_drive_capacity)
//            {
//                /* the capacity is matching, so copy the disk in raid groups disk set
//                 */
//                status = fbe_test_sep_util_get_next_disk_in_set(*disk_set_index, 
//                                                                                                 in_disk_set_p,
//                                                                                                 &disk_set_p);
//                if (status != FBE_STATUS_OK) { return status; }      
//                raid_confg_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
//                (*disk_set_index)++;
//                in_drive_cap++;
//                break;
//            }
//            else
//            {
//                /* this drive is of lesser size, skip this
//                 */
//                mut_printf(MUT_LOG_TEST_STATUS, "%s drive not used! size not matching.\n", __FUNCTION__);
//                mut_printf(MUT_LOG_TEST_STATUS, "%s drive cap %d, rg_cap %d .\n", 
//                                   in_drive_cap[*disk_set_index],rg_drive_capacity);
//               (*disk_set_index)++;
//                in_drive_cap++;
//            }
//        }/* end while( (*disk_set_index) <= FBE_TEST_MAX_DRIVE_LOCATIONS) */
//
//    }
//    return status;
//}


//@todo: herry_monster: enable for herry_monster case
//fbe_status_t fbe_test_sep_util_fill_raid_group_disk_set_with_cap(fbe_test_rg_configuration_t * raid_configuration_list_p,
//                                                        fbe_u32_t raid_group_count,
//                                                        fbe_test_drive_locations_t *drive_locations_p)
//{
//   /* For a given configuration fill in the actual drives we will be using.
//    * This function checks the disks capacity for the given raid group configuration
//    * and then fill it accordingly. It is possible that we have inserted special size disks for the raid group. 
//    * We first create a set of disk sets for the 4160 and 520 disks.  Then we call this function and pick 
//    * drives from those disk sets to use in our raid groups.
//     */
//    fbe_status_t status = FBE_STATUS_OK;
//    fbe_u32_t rg_index;
//
//    fbe_u32_t disk_set_index_520 = 0;
//    fbe_u32_t disk_set_index_4160 = 0;
//    fbe_test_raid_group_disk_set_t * disk_set_4160 = &drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_4160][0];
//    fbe_test_raid_group_disk_set_t * disk_set_520 = &drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_520][0];
//    fbe_block_count_t *drive_4160_cap = &drive_locations_p->drive_capacity[FBE_TEST_BLOCK_SIZE_4160][0];
//    fbe_block_count_t *drive_520_cap = &drive_locations_p->drive_capacity[FBE_TEST_BLOCK_SIZE_520][0];
//
//    fbe_block_count_t max_drive_cap = fbe_test_sep_util_get_maximum_drive_capacity(raid_configuration_list_p,
//                                                                                                                                                      raid_group_count);
//
//    /* For every raid group, fill in the entire disk set by using our static 
//     * set or disk sets for 4160 and 520. 
//     */
//    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
//    {
//        if (raid_configuration_list_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
//        {
//            /* Must initialize the rg configuration structure.
//             */
//            fbe_test_sep_util_init_rg_configuration(&raid_configuration_list_p[rg_index]);
//        }
//        if (!fbe_test_rg_config_is_enabled(&raid_configuration_list_p[rg_index]))
//        {
//            continue;
//        }
//        if(raid_configuration_list_p[rg_index].block_size == 4160)
//        {
//            status = fbe_test_sep_util_fill_rg_disks(raid_configuration_list_p,
//                                                                rg_index,
//                                                                &disk_set_index_4160,
//                                                                max_drive_cap,
//                                                                disk_set_4160,
//                                                                drive_4160_cap);
//        }
//        else
//        {
//            status = fbe_test_sep_util_fill_rg_disks(raid_configuration_list_p,
//                                                                rg_index,
//                                                                &disk_set_index_520,
//                                                                max_drive_cap,
//                                                                disk_set_520,
//                                                                drive_520_cap);
//        }
//    }
//    return status;
//}


fbe_status_t 
fbe_test_sep_util_fill_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_p,
                                                  fbe_raid_group_number_t raid_group_id,
                                                  fbe_raid_group_type_t raid_type,
                                                  fbe_lun_number_t lun_number,
                                                  fbe_lba_t lun_capacity,
                                                  fbe_lba_t imported_capacity)
{
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
    logical_unit_configuration_p->imported_capacity = imported_capacity;
    logical_unit_configuration_p->capacity = lun_capacity;
    logical_unit_configuration_p->lun_number = lun_number;
    logical_unit_configuration_p->raid_group_id = raid_group_id;
    logical_unit_configuration_p->raid_type = raid_type;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_sep_util_fill_raid_group_lun_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                    fbe_u32_t raid_group_count,
                                                    fbe_lun_number_t lun_number,
                                                    fbe_u32_t logical_unit_count,
                                                    fbe_lba_t lun_capacity)
{
    fbe_u32_t           rg_index = 0;
    fbe_u32_t           lu_index = 0;
    fbe_raid_group_number_t  raid_group_id = FBE_RAID_ID_INVALID;
    fbe_u16_t           data_disks = 0;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /*!@todo Add maximum number of RAID groups and Logical units per RAID group limit. */

    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 4160 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_lba_t                                       capacity = FBE_LBA_INVALID;
        fbe_api_lun_calculate_capacity_info_t           lun_calculate_imported_capacity;

        if(raid_group_configuration_p == NULL) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User has given invalid RAID group configuration.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }                    

        /* Get the RAID group id. */
        raid_group_id = raid_group_configuration_p->raid_group_id;
        if(raid_group_id == FBE_RAID_ID_INVALID)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User(* thas given invalid RAID group id.", __FUNCTION__);
            continue;
        }

        /* Get the number of data disks for this RAID group. */
        status = fbe_test_sep_util_get_raid_class_info(raid_group_configuration_p->raid_type,
                                                       raid_group_configuration_p->class_id,
                                                       raid_group_configuration_p->width,
                                                       1,    /* not used yet. */
                                                       &data_disks,
                                                       &capacity);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Calcuate the imported capacity of LUN using LUN class. */         
        lun_calculate_imported_capacity.imported_capacity = FBE_LBA_INVALID;
        lun_calculate_imported_capacity.exported_capacity = lun_capacity;
        lun_calculate_imported_capacity.lun_align_size = FBE_TEST_CHUNK_SIZE * data_disks;
        status = fbe_api_lun_calculate_imported_capacity(&lun_calculate_imported_capacity);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INTEGER_NOT_EQUAL(lun_calculate_imported_capacity.imported_capacity, FBE_LBA_INVALID);

        /* Make the raid group capacity exactly that consumed by all the LUNs, but not 0. */
        if (logical_unit_count != 0) {
            raid_group_configuration_p->capacity = 
                logical_unit_count * lun_calculate_imported_capacity.imported_capacity;
        }

        /* Add the extra chunk */        
        raid_group_configuration_p->capacity += (FBE_TEST_CHUNK_SIZE * data_disks) * raid_group_configuration_p->extra_chunk;

        /* Fill out the lun information for this RAID group. */
        for(lu_index = 0; lu_index < logical_unit_count; lu_index++)
        {
            raid_group_id = raid_group_configuration_p->raid_group_id;
            fbe_test_sep_util_fill_logical_unit_configuration(&raid_group_configuration_p->logical_unit_configuration_list[lu_index],
                                                              raid_group_id,
                                                              raid_group_configuration_p->raid_type,
                                                              lun_number++,
                                                              lun_capacity,
                                                              lun_calculate_imported_capacity.imported_capacity);
        }

        /* Update the number of luns per RAID group details. */
        raid_group_configuration_p->number_of_luns_per_rg = logical_unit_count;
        raid_group_configuration_p++;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_sep_util_calculate_raid_group_capacity_for_lun_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                                      fbe_lba_t * raid_group_capacity_for_lun_p)
{
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_list_p;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       lu_index = 0;
    fbe_api_lun_calculate_capacity_info_t           lun_calculate_imported_capacity;
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       capacity;

    /* Calcuate the RAID group capacity for the Logical unit configuration. */
    *raid_group_capacity_for_lun_p = 0;

    logical_unit_configuration_list_p = raid_group_configuration_p->logical_unit_configuration_list;


    /* Get data disks number to calculate the lun_align_size */
    status = fbe_test_sep_util_get_raid_class_info(raid_group_configuration_p->raid_type,
                                                   raid_group_configuration_p->class_id,
                                                   raid_group_configuration_p->width,
                                                   1,    /* not used yet. */
                                                   &data_disks,
                                                   &capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Index through user provided LU configuration to calculate the total
     * capacity we need for the RAID group.
     */
    for(lu_index = 0; lu_index < raid_group_configuration_p->number_of_luns_per_rg; lu_index++)
    {
        if((logical_unit_configuration_list_p == NULL) ||
           (logical_unit_configuration_list_p->capacity == FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User has provided wrong configuration for the LUN.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Calculate the imported capacity using LUN classs.
         */
        lun_calculate_imported_capacity.exported_capacity = logical_unit_configuration_list_p->capacity;
        lun_calculate_imported_capacity.lun_align_size = FBE_TEST_CHUNK_SIZE * data_disks;
        status = fbe_api_lun_calculate_imported_capacity(&lun_calculate_imported_capacity);
        if((status != FBE_STATUS_OK) || (lun_calculate_imported_capacity.imported_capacity == FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s LUN imported capacity calculation failed, status: %d", __FUNCTION__, status);
            return status;
        }
        
        /* Calculate the RAID group capacity we need for the asked user configuration. */
        (*raid_group_capacity_for_lun_p) += lun_calculate_imported_capacity.imported_capacity;
        logical_unit_configuration_list_p++;
    }

    return status;
}

fbe_u32_t fbe_test_sep_util_increase_lun_chunks_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t in_chunks_per_lun,
                                                                  fbe_u32_t data_disks)
                                                       
{
    fbe_u32_t   updated_chunks_per_lun = in_chunks_per_lun;

    /* Switch on the raid type.
     */
    switch(rg_config_p->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            /* For RAID-0 bandwidth groups we must increase the chunks per lun
             * to guarantee sufficient errors.
             */
            if ((rg_config_p->b_bandwidth == FBE_TRUE) &&
                (rg_config_p->width == 1)                 )
            {
                updated_chunks_per_lun = (in_chunks_per_lun * 3);
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID10:
            /* For raid 10 the number of chunks in the lun should be a multiple 
             * of the number of mirrored pairs (data disks). This ensures that 
             * when we write our background pattern we consume the same amount 
             * on each pair and thus initialize the entire extent of the raid 
             * group.  Otherwise pre-reads for unaligned requests can find 
             * checksum errors. 
             */
            updated_chunks_per_lun += data_disks - (in_chunks_per_lun % data_disks);
            break;

        default:
            /* Leave the chunks per lun unmodified.
             */
            break;
    }

    /* Return the updated chunks per lun
     */
    return updated_chunks_per_lun;
}

void fbe_test_sep_util_fill_lun_configurations_rounded(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_u32_t raid_group_count, 
                                                       fbe_u32_t chunks_per_lun,
                                                       fbe_u32_t luns_per_rg)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_lun_number_t   lun_number = 0;
    fbe_u32_t current_chunks_per_lun;

    /* Fill up the logical unit configuration for each RAID group of this test index.
     * We round each capacity up to a default number of chunks. 
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_api_raid_group_class_get_info_t rg_info;
        fbe_lba_t lun_capacity;

        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        current_chunks_per_lun = chunks_per_lun;

        /* For the given config create a LUN with the appropriate number of chunks.
         */
        rg_info.width = rg_config_p[rg_index].width;
        rg_info.raid_type = rg_config_p[rg_index].raid_type;
        rg_info.single_drive_capacity = FBE_U32_MAX;
        rg_info.exported_capacity = FBE_LBA_INVALID; /* set to invalid to indicate not used. */
        status = fbe_api_raid_group_class_get_info(&rg_info, rg_config_p[rg_index].class_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Update the chunks per lun based of raid group configuration
         */
        current_chunks_per_lun = fbe_test_sep_util_increase_lun_chunks_for_config(rg_config_p,
                                                                                  current_chunks_per_lun,
                                                                                  rg_info.data_disks);

        /* Now round to the nearest chunks size
         */
        lun_capacity = (FBE_RAID_DEFAULT_CHUNK_SIZE *((fbe_block_count_t) current_chunks_per_lun)) * rg_info.data_disks;

        mut_printf(MUT_LOG_TEST_STATUS, "LUN: 0x%x, Capacity 0x%llx ",
		   lun_number, (unsigned long long)lun_capacity);

        status = fbe_test_sep_util_fill_raid_group_lun_configuration(&rg_config_p[rg_index],
                                                                     1, /* Number to fill. */
                                                                     lun_number,
                                                                     luns_per_rg,
                                                                     lun_capacity);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        lun_number += luns_per_rg;
    }
    return;
}

static fbe_status_t fbe_test_sep_util_setup_rg_keys(fbe_test_rg_configuration_t * raid_group_configuration_p)
{
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;              
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_status_t        status;
    fbe_sim_transport_connection_target_t this_sp, other_sp;


    status = fbe_test_util_wait_for_rg_object_id_by_number(raid_group_configuration_p->raid_group_id, &rg_object_id, 18000);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_util_wait_for_encryption_state(rg_object_id, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED, 
                                                     18000);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Encryption state unexpected\n");

    fbe_api_wait_for_object_lifecycle_state(rg_object_id,FBE_LIFECYCLE_STATE_SPECIALIZE, 1800, FBE_PACKAGE_ID_SEP_0);

    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));

    /* Get the keys */
    status = fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


   this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Push it down */
    status = fbe_api_database_setup_encryption_key(&encryption_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* now that the key was pushed, the raid object should get out of that state */
    //status = fbe_test_util_wait_for_encryption_state(rg_object_id, 
    //                                        FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
    //                                        18000);
    //MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Encryption state unexpected\n");

    // fbe_api_sim_transport_set_target_server(other_sp);

    /* Push it down */
    //status = fbe_api_database_setup_encryption_key(&encryption_info);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //fbe_api_sim_transport_set_target_server(this_sp);
    return status;
}
fbe_status_t fbe_test_sep_util_generate_invalid_encryption_key(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_object_id_t rg_object_id,
                                                               fbe_database_control_setup_encryption_key_t *key_info)
{
    fbe_config_generation_t generation_number = 0;
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t           index;

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;
    key_info->key_setup.key_table_entry[0].object_id = rg_object_id;
    key_info->key_setup.key_table_entry[0].num_of_keys = rg_config_p->width;
    key_info->key_setup.key_table_entry[0].generation_number = generation_number;

    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBE%d%d%d\n",index, rg_object_id, 0);
        fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask = FBE_ENCRYPTION_KEY_MASK_VALID;
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_test_sep_util_generate_encryption_key(fbe_object_id_t object_id,
                                                       fbe_database_control_setup_encryption_key_t *key_info)
{
    fbe_status_t status;
    fbe_config_generation_t generation_number;
    fbe_api_base_config_get_width_t get_width;
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t           index;

    status = fbe_api_base_config_get_generation_number(object_id, &generation_number);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_base_config_get_width(object_id, &get_width);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;
    key_info->key_setup.key_table_entry[0].object_id = object_id;
    key_info->key_setup.key_table_entry[0].num_of_keys = get_width.width;
    key_info->key_setup.key_table_entry[0].generation_number = generation_number;
    
    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBE%d%d%d\n",index, object_id, 0);
        fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        /* This will make position 0 marked as VALID */
        fbe_encryption_key_mask_set(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask, 
                                    0, FBE_ENCRYPTION_KEY_MASK_VALID);
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_test_sep_util_invalidate_key(fbe_object_id_t object_id,
                                              fbe_database_control_setup_encryption_key_t *key_info,
                                              fbe_u32_t key_index,
                                              fbe_encryption_key_mask_t mask)
{
    fbe_status_t status;
    fbe_api_base_config_get_width_t get_width;
    fbe_u8_t keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t index;

    status = fbe_api_base_config_get_width(object_id, &get_width);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;

    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        if (key_index == 0) {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        } else {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key2, keys, FBE_ENCRYPTION_KEY_SIZE);
        } 
        fbe_encryption_key_mask_set(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask,
                                    key_index,
                                    mask);
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_test_sep_util_generate_next_key(fbe_object_id_t object_id,
                                                 fbe_database_control_setup_encryption_key_t *key_info,
                                                 fbe_u32_t key_index,
                                                 fbe_u32_t generation,
                                                 fbe_encryption_key_mask_t mask)
{
    fbe_u8_t  keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t index;

    key_info->key_setup.num_of_entries = 1;
    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++) {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBE%d%d%d",index, object_id, generation);
        if (key_index == 0) {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        } else {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key2, keys, FBE_ENCRYPTION_KEY_SIZE);
        }

        fbe_encryption_key_mask_set(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask,
                                    key_index,
                                    mask);
    }

	 mut_printf(MUT_LOG_TEST_STATUS," Next key %s mask 0x%llx",keys, key_info->key_setup.key_table_entry[0].encryption_keys[0].key_mask);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_test_sep_util_create_raid_group(fbe_test_rg_configuration_t * raid_group_configuration_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_create_t             fbe_raid_group_create_req;
    fbe_u32_t                                           drive_slot = 0;
    const fbe_char_t *                                        raid_type_string_p = NULL;
    const fbe_char_t *                                        block_size_string_p = NULL;

    fbe_zero_memory(&fbe_raid_group_create_req,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_test_sep_util_get_raid_type_string(raid_group_configuration_p->raid_type, &raid_type_string_p);
    fbe_test_sep_util_get_block_size_string(raid_group_configuration_p->block_size, &block_size_string_p);
    /* Verify the user provided configuration. */
    MUT_ASSERT_NOT_NULL_MSG(raid_group_configuration_p, "User has provided RAID group configuration as NULL.");
    MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_RAID_GROUP_TYPE_UNKNOWN, raid_group_configuration_p->raid_type, "User has provided invalid RAID type.");

    mut_printf(MUT_LOG_TEST_STATUS, "Create a RAID Group with RAID group ID: %d type: %s width: %d block_size: %s", 
               raid_group_configuration_p->raid_group_id, raid_type_string_p, raid_group_configuration_p->width, block_size_string_p);

    /* Create a RAID group object with RAID group create API. */
    fbe_raid_group_create_req.b_bandwidth = raid_group_configuration_p->b_bandwidth;
    fbe_raid_group_create_req.capacity = raid_group_configuration_p->capacity;
    //fbe_raid_group_create_req.explicit_removal = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = raid_group_configuration_p->raid_group_id;
    fbe_raid_group_create_req.raid_type = raid_group_configuration_p->raid_type;
    fbe_raid_group_create_req.drive_count = raid_group_configuration_p->width;
    fbe_raid_group_create_req.job_number = 0;
    fbe_raid_group_create_req.user_private = FBE_FALSE;
    fbe_raid_group_create_req.wait_ready = FBE_FALSE; 
    fbe_raid_group_create_req.ready_timeout_msec = FBE_TEST_WAIT_OBJECT_TIMEOUT_MS;
    fbe_raid_group_create_req.is_system_rg = FBE_FALSE;

    /* Validate drive count */
    if (raid_group_configuration_p->width > FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "Create a RAID Group: width: %d is greater than max raid group width: %d", 
                   raid_group_configuration_p->width, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Fill up the drive information for the RAID group. */
    for (drive_slot = 0; drive_slot < fbe_raid_group_create_req.drive_count; ++drive_slot)
    {
        fbe_raid_group_create_req.disk_array[drive_slot].bus = raid_group_configuration_p->rg_disk_set[drive_slot].bus;
        fbe_raid_group_create_req.disk_array[drive_slot].enclosure = raid_group_configuration_p->rg_disk_set[drive_slot].enclosure;
        fbe_raid_group_create_req.disk_array[drive_slot].slot = raid_group_configuration_p->rg_disk_set[drive_slot].slot;
    }

    /* Create a RAID group using Job service API to create a RAID group. */
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group create job sent successfully! ==");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)fbe_raid_group_create_req.job_number);

    /* update the job number for the */
    raid_group_configuration_p->job_number = fbe_raid_group_create_req.job_number;


    return status;
}

fbe_status_t
fbe_test_sep_util_destroy_raid_group(fbe_raid_group_number_t raid_group_id)
{
    fbe_status_t                                    status;
    fbe_api_job_service_raid_group_destroy_t        fbe_raid_group_destroy_request;
    fbe_object_id_t                                 rg_object_id;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;

    /* Validate that the raid group exist */
    status = fbe_api_database_lookup_raid_group_by_number(raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);


    fbe_test_sep_util_print_metadata_statistics(rg_object_id);

    /* Destroy a RAID group */
    fbe_raid_group_destroy_request.raid_group_id = raid_group_id;
    fbe_raid_group_destroy_request.job_number = 0;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = FBE_TEST_WAIT_OBJECT_TIMEOUT_MS;

    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroy job sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, 
               "Raid group id: %d Object Id: 0x%x Job number 0x%llX", 
               raid_group_id, rg_object_id,
	       (unsigned long long)fbe_raid_group_destroy_request.job_number);

    status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number ,
                                         FBE_TEST_UTILS_DESTROY_TIMEOUT_MS,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");

    /* RG probably has LUNs in it.. */
    if(job_error_code == FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "RG cannot be deleted.. ");
        mut_printf(MUT_LOG_TEST_STATUS, "It has upstream objects in it (probably LUNs).. Delete those first..\n");

        return FBE_STATUS_OK;
    }

    /* Make sure the RAID object successfully destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully! ===");

    return FBE_STATUS_OK;
}
static fbe_bool_t fbe_test_sep_util_force_no_initial_verify = FBE_FALSE;
void fbe_test_sep_util_set_no_initial_verify(fbe_bool_t b_value)
{
    fbe_test_sep_util_force_no_initial_verify = b_value;
}

fbe_bool_t fbe_test_sep_util_get_no_initial_verify(void)
{
    return fbe_test_sep_util_force_no_initial_verify;
}

fbe_status_t
fbe_test_sep_util_create_logical_unit(fbe_test_logical_unit_configuration_t * lun_configuration_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_api_job_service_lun_create_t                    fbe_lun_create_req;

    /* Verify the user provided configuration. */
    MUT_ASSERT_NOT_NULL_MSG(lun_configuration_p, "User has provided LUN configuration as NULL.");

    mut_printf(MUT_LOG_TEST_STATUS, "Create a LUN with logical unit number:%d", lun_configuration_p->lun_number);

    /* Fill up the logical unit information. */
    /* @!TODO: need to enhance this function so all data comes from caller and not hardcoded as shown
     * below.  This function is currently only used by one test script while other tests use their own 
     * similar utility function.
     * Need to clean this up.
     */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_job_service_lun_create_t));
    fbe_lun_create_req.raid_type = lun_configuration_p->raid_type;
    fbe_lun_create_req.raid_group_id = lun_configuration_p->raid_group_id; 
    fbe_lun_create_req.lun_number = lun_configuration_p->lun_number;
    fbe_lun_create_req.capacity = lun_configuration_p->capacity;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = fbe_test_sep_util_force_no_initial_verify;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.job_number = 0;
    fbe_lun_create_req.user_private = FBE_FALSE;
    fbe_lun_create_req.wait_ready = FBE_FALSE;
    fbe_lun_create_req.ready_timeout_msec = FBE_TEST_WAIT_OBJECT_TIMEOUT_MS;
    fbe_lun_create_req.is_system_lun = FBE_FALSE;

    /* create a Logical Unit using job service fbe api. */
    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);

    lun_configuration_p->job_number = fbe_lun_create_req.job_number;

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)fbe_lun_create_req.job_number);
    return status;
}

fbe_status_t
fbe_test_sep_util_destroy_logical_unit(fbe_lun_number_t   lun_number)
{
    fbe_status_t                            status;
    fbe_api_job_service_lun_destroy_t       fbe_lun_destroy_req;
    fbe_object_id_t                         lun_object_id;
    fbe_object_id_t                         object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Get the object id of the LUN so we can use it below to wait for it
     * to get destroyed. 
     */ 
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;
    fbe_lun_destroy_req.job_number = 0;
    fbe_lun_destroy_req.wait_destroy = FBE_FALSE;
    fbe_lun_destroy_req.destroy_timeout_msec = FBE_TEST_WAIT_OBJECT_TIMEOUT_MS;

    status = fbe_api_job_service_lun_destroy(&fbe_lun_destroy_req);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroy job sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)fbe_lun_destroy_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_destroy_req.job_number ,
                                         FBE_TEST_UTILS_DESTROY_TIMEOUT_MS,
                                         &job_error_code,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN destruction failed");


    /* Make sure the LUN Object successfully destroyed */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(lun_number, &object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===");

    /* Make sure the LUN has destroyed itself.
     * We wait for an invalid state and expect to get back an error.
     */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, 
                                                     FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                     30000, 
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN %d destroyed successfully! ===", fbe_lun_destroy_req.lun_number);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_test_sep_util_create_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_p,
                                                    fbe_u32_t luns_per_raid_group)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       lu_index = 0;
    fbe_test_logical_unit_configuration_t * const   orig_logical_unit_configuration_p = logical_unit_configuration_p;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;

    /* Verify the user provided configuration. */
    MUT_ASSERT_NOT_NULL_MSG(logical_unit_configuration_p, "User has provided LUN configuration list as NULL.");
    mut_printf(MUT_LOG_TEST_STATUS, "%d LUN to be created.", luns_per_raid_group);

    /* Go through all the RG for which user has provided configuration data. */
    for (lu_index = 0; lu_index < luns_per_raid_group; lu_index++)
    {
        /* Create a RAID group with user provided configuration. */
        status = fbe_test_sep_util_create_logical_unit(logical_unit_configuration_p);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical Unit failed.");
        logical_unit_configuration_p++;
    }

    logical_unit_configuration_p = orig_logical_unit_configuration_p;
    for (lu_index = 0; lu_index < luns_per_raid_group; lu_index++)
    {    
       
        status = fbe_api_common_wait_for_job(logical_unit_configuration_p->job_number,
                                             FBE_TEST_UTILS_CREATE_TIMEOUT_MS,
                                             &job_error_code,
                                             &job_status,
                                             NULL);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created LUN config");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN config creation failed");

        logical_unit_configuration_p++;
    }

    return status;
}

fbe_status_t 
fbe_test_sep_util_destroy_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_list_p,
                                                     fbe_u32_t luns_per_raid_group)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       lu_index = 0;

    /* Verify the user provided configuration. */
    MUT_ASSERT_NOT_NULL_MSG(logical_unit_configuration_list_p, "User has provided LUN configuration list as NULL.");
    MUT_ASSERT_TRUE_MSG((luns_per_raid_group != 0), "Number of LUN provided by user is zero.");

    /* Go through all the LUNs for this RAID group. */
    for (lu_index = 0; lu_index < luns_per_raid_group; lu_index++)
    {
        if(logical_unit_configuration_list_p == NULL) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User has given invalid Logical unit configuration.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Destroy a logical unit. */
        status = fbe_test_sep_util_destroy_logical_unit(logical_unit_configuration_list_p->lun_number);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical Unit failed.");
        logical_unit_configuration_list_p++;
    }
    return status;
}
/*!**************************************************************
 * fbe_test_sep_util_init_degraded_info()
 ****************************************************************
 * @brief
 *  Initialize the raid group degraged information.  This method
 *  should be invoked when testing degraded raid groups.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_sep_util_init_degraded_info(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t remove_index;
    rg_config_p->num_removed = 0;
    rg_config_p->num_needing_spare = 0;    
    for (remove_index = 0; remove_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; remove_index++)
    {
        rg_config_p->drives_removed_array[remove_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
        rg_config_p->drives_needing_spare_array[remove_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
        rg_config_p->specific_drives_to_remove[remove_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }
    for (remove_index = 0; remove_index < FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE; remove_index++)
    {
        rg_config_p->drives_removed_history[remove_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }
    return;
}
/* end of fbe_test_sep_util_init_degraded_info() */

/*!**************************************************************
 * fbe_test_sep_util_init_rg_configuration()
 ****************************************************************
 * @brief
 *  Initialize the raid group config struct.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_sep_util_init_rg_configuration(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_sep_util_init_degraded_info(rg_config_p);
    rg_config_p->b_already_tested = FBE_FALSE;
    rg_config_p->b_create_this_pass = FBE_TRUE;
    rg_config_p->b_use_fixed_disks = FBE_FALSE;
    rg_config_p->b_valid_for_physical_config = FBE_TRUE;
    rg_config_p->magic = FBE_TEST_RG_CONFIGURATION_MAGIC;
    rg_config_p->extra_chunk = FBE_TEST_INIT_INVALID_EXTRA_CHUNK;
    rg_config_p->num_of_extra_drives = 0;
    rg_config_p->source_disk.bus = 0;
    rg_config_p->source_disk.enclosure = 0;
    rg_config_p->source_disk.slot = 0;
    return;
}
/* end of fbe_test_sep_util_init_rg_configuration() */

/*!**************************************************************
 * fbe_test_sep_util_init_rg_configuration_array()
 ****************************************************************
 * @brief
 *  Initialize the raid group config struct.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_sep_util_init_rg_configuration_array(fbe_test_rg_configuration_t *rg_config_p)
{
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        fbe_test_sep_util_init_rg_configuration(rg_config_p);
        rg_config_p++;
    }
    return;
}
/* end of fbe_test_sep_util_init_rg_configuration_array() */



/*!**************************************************************
 * fbe_sep_test_util_get_default_block_size()
 ****************************************************************
 * @brief
 *  Return if we should only execute certain configurations.
 *
 * @param none       
 *
 * @return - fbe_u32_t - 520, 4160 or FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE
 *
 ****************************************************************/

fbe_u32_t fbe_sep_test_util_get_default_block_size(void)
{
    char *value = mut_get_user_option_value("-block_size");

    if (value == NULL) {
        /* Randomly select the block size for all raid groups.
         */
        return FBE_TEST_RG_CONFIG_RANDOM_TAG;
    }
    else if (strcmp(value, "520") == 0) {
        return 520;
    }
    else if ((strcmp(value, "4160") == 0) ||
             (strcmp(value, "4k") == 0)) {
        return 4160;
    }
    else if (strcmp(value, "mixed") == 0) {
        return FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE;
    }
    else {
        mut_printf(MUT_LOG_TEST_STATUS, "unexpected value %s expected 520, 4k or mixed", value);
        MUT_FAIL_MSG("expected 520, 4k or mixed");
    }
    return 0;
}
/*!**************************************************************
 * fbe_test_rg_setup_random_block_sizes()
 ****************************************************************
 * @brief
 *  Initialize the raid group config struct.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_rg_setup_random_block_sizes(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t block_size = fbe_sep_test_util_get_default_block_size();
    fbe_test_rg_setup_block_sizes(rg_config_p, block_size);
    return;
}
/* end of fbe_test_rg_setup_random_block_sizes() */

/*!**************************************************************
 * fbe_test_rg_setup_block_sizes()
 ****************************************************************
 * @brief
 *  Initialize the raid group config struct with the specified block size
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_rg_setup_block_sizes(fbe_test_rg_configuration_t *rg_config_p, 
                                   fbe_block_size_t block_size)
{
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX){
        rg_config_p->block_size = block_size;
        rg_config_p++;
    }
    return;
}
/* end of fbe_test_rg_setup_block_sizes() */

/*!**************************************************************
 * fbe_test_sep_util_randomize_rg_configuration_array()
 ****************************************************************
 * @brief
 *  Check structure for random tags and then randomize that value.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_sep_util_randomize_rg_configuration_array(fbe_test_rg_configuration_t *rg_config_p)
{
    const fbe_char_t       *raid_type_string_p = NULL;
    const fbe_char_t       *block_size_string_p = NULL;
    fbe_u32_t               random_num;
    fbe_raid_group_type_t   original_raid_type = rg_config_p->raid_type;
    fbe_u32_t               next_block_size;
    fbe_u32_t               random_number;
    /* This will automatically setup the seed if it has not already been set.
     */
    fbe_test_init_random_seed();
    random_number = fbe_random();
    switch (random_number % 3) {
        case 2:
            next_block_size = FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE;
            break;
        case 1:
            next_block_size = 4160;
            break;
        case 0:
        default:
            next_block_size = 520;
            break;
    };
    fbe_test_sep_util_get_block_size_string(next_block_size, &block_size_string_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== randomize block size start with %s", block_size_string_p);

    if (fbe_test_sep_util_get_cmd_option("-unmap_default_drive_type"))
    {
        fbe_test_pp_utils_set_default_520_sas_drive(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP);
        fbe_test_pp_utils_set_default_4160_sas_drive(FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP);
    }

    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (rg_config_p->raid_type == FBE_TEST_RG_CONFIG_RANDOM_TAG)
        {
            random_num = fbe_random() % 6;
            switch (random_num)
            {
                case 0:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID0;
                    rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                    break;
                case 1:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID1;
                    rg_config_p->class_id = FBE_CLASS_ID_MIRROR;
                    break;
                case 2:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID10;
                    rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                    break;
                case 3:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID3;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
                case 4:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID6;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
                case 5:
                default:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID5;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
            };

            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize raid type: %d (%s) for rg id: %d", 
                       rg_config_p->raid_type, raid_type_string_p, rg_config_p->raid_group_id);
        }
        else if (rg_config_p->raid_type == FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT)
        {
            random_num = fbe_random() % 5;
            switch (random_num)
            {
                case 0:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID3;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
                case 1:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID6;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
                case 2:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID1;
                    rg_config_p->class_id = FBE_CLASS_ID_MIRROR;
                    break;
                case 3:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID10;
                    rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                    break;
                case 4:
                default:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID5;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
            };
            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize redundant raid type: %d (%s) for rg id: %d", 
                       rg_config_p->raid_type, raid_type_string_p, rg_config_p->raid_group_id);
        }
        else if (rg_config_p->raid_type == FBE_TEST_RG_CONFIG_RANDOM_TYPE_SINGLE_REDUNDANT)
        {
            random_num = fbe_random() % 4;
            switch (random_num)
            {
                case 0:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID3;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
                case 1:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID1;
                    rg_config_p->class_id = FBE_CLASS_ID_MIRROR;
                    break;
                case 2:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID10;
                    rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                    break;
                case 3:
                default:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID5;
                    rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                    break;
            };
            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize single redundant raid type: %d (%s) for rg id: %d", 
                       rg_config_p->raid_type, raid_type_string_p, rg_config_p->raid_group_id);
        }
        else if (rg_config_p->raid_type == FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY)
        {
            random_num = fbe_random() % 3;
            switch (random_num)
            {
                case 0:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID3;
                    break;
                case 1:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID6;
                    break;
                case 2:
                default:
                    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID5;
                    break;
            };

            rg_config_p->class_id = FBE_CLASS_ID_PARITY;

            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize parity raid type: %d (%s) for rg id: %d", 
                       rg_config_p->raid_type, raid_type_string_p, rg_config_p->raid_group_id);
        }


        if (rg_config_p->width == FBE_TEST_RG_CONFIG_RANDOM_TAG)
        {
            fbe_u32_t range;
            switch (rg_config_p->raid_type)
            {
                case FBE_RAID_GROUP_TYPE_RAID0:
                    range = FBE_RAID_LIBRARY_RAID0_MAX_WIDTH - FBE_RAID_LIBRARY_RAID0_MIN_WIDTH + 1;
                    random_num = fbe_random() % range;
                    rg_config_p->width = FBE_RAID_LIBRARY_RAID0_MIN_WIDTH + random_num;
                    break;
                case FBE_RAID_GROUP_TYPE_RAID5:
                    range = FBE_RAID_LIBRARY_RAID5_MAX_WIDTH - FBE_RAID_LIBRARY_RAID5_MIN_WIDTH + 1;
                    random_num = fbe_random() % range;
                    rg_config_p->width = FBE_RAID_LIBRARY_RAID5_MIN_WIDTH + random_num;
                    break;
                case FBE_RAID_GROUP_TYPE_RAID6:
                    range = FBE_RAID_LIBRARY_RAID6_MAX_WIDTH - FBE_RAID_LIBRARY_RAID6_MIN_WIDTH + 1;
                    random_num = fbe_random() % range;
                    if (random_num % 2)
                    {
                        random_num++;
                    }
                    rg_config_p->width = FBE_RAID_LIBRARY_RAID6_MIN_WIDTH + random_num;
                    break;
                case FBE_RAID_GROUP_TYPE_RAID3:
                    /* Randomize between the 3 different widths.
                     */
                    random_num = fbe_random() % 3;
                    switch (random_num)
                    {
                        case 0:
                            rg_config_p->width = FBE_RAID_LIBRARY_RAID3_MIN_WIDTH;
                            break;
                        case 1:
                            rg_config_p->width = FBE_RAID_LIBRARY_RAID3_VAULT_WIDTH;
                            break;
                        case 2:
                        default:
                            rg_config_p->width = FBE_RAID_LIBRARY_RAID3_MAX_WIDTH;
                            break;
                    };
                    break;
                case FBE_RAID_GROUP_TYPE_RAID1:
                    if (original_raid_type != FBE_TEST_RG_CONFIG_RANDOM_TYPE_SINGLE_REDUNDANT)
                    {
                        range = FBE_RAID_LIBRARY_RAID1_MAX_WIDTH - FBE_RAID_LIBRARY_RAID1_MIN_WIDTH + 1;
                        random_num = fbe_random() % range;
                        rg_config_p->width = FBE_RAID_LIBRARY_RAID1_MIN_WIDTH + random_num;
                    }
                    else
                    {
                        rg_config_p->width = FBE_RAID_LIBRARY_RAID1_MIN_WIDTH;
                    }
                    break;
                case FBE_RAID_GROUP_TYPE_RAID10:
                    range = FBE_RAID_LIBRARY_RAID10_MAX_WIDTH - FBE_RAID_LIBRARY_RAID10_MIN_WIDTH + 1;
                    random_num = fbe_random() % range;
                    /* Only even widths are supported.
                     */
                    if (random_num % 2)
                    {
                        random_num++;
                    }
                    rg_config_p->width = FBE_RAID_LIBRARY_RAID10_MIN_WIDTH + random_num;
                    break;
                default:
                    mut_printf(MUT_LOG_TEST_STATUS, "%d raid type unknown", rg_config_p->raid_type);
                    MUT_FAIL_MSG("Unknown raid type");
                    break;
            };
            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize width: %d for rg id: %d type %d (%s)", 
                       rg_config_p->width, rg_config_p->raid_group_id, rg_config_p->raid_type, raid_type_string_p);
        }
        if (rg_config_p->b_bandwidth == FBE_TEST_RG_CONFIG_RANDOM_TAG)
        {
            random_num = fbe_random() % 2;
            rg_config_p->b_bandwidth = (random_num) ? FBE_TRUE : FBE_FALSE;
            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize bandwidth: %d for rg id: %d type %d (%s)", 
                       rg_config_p->b_bandwidth, rg_config_p->raid_group_id, rg_config_p->raid_type, raid_type_string_p);
        }
        if (rg_config_p->block_size == FBE_TEST_RG_CONFIG_RANDOM_TAG)
        {
            if ((rg_config_p->width == 1)                                && 
                (next_block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE)    ) {
                next_block_size = 4160;
            }
            rg_config_p->block_size = next_block_size;
            fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
            fbe_test_sep_util_get_block_size_string(next_block_size, &block_size_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "randomize block size: %d for rg id: %d type %d (%s) block_size: %s", 
                       rg_config_p->block_size, rg_config_p->raid_group_id, rg_config_p->raid_type, raid_type_string_p, block_size_string_p);

            switch (next_block_size) {
                case 520:
                    next_block_size = 4160;
                    break;
                case 4160:
                    next_block_size = FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE;
                    break;
                case FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE:
                    next_block_size = 520;
                    break;
                default:
                    mut_printf(MUT_LOG_TEST_STATUS, "next block size is: %d", next_block_size);
                    MUT_FAIL_MSG("unknown next block size");
            };
        }
        MUT_ASSERT_INT_NOT_EQUAL(rg_config_p->width, 0);
        rg_config_p++;
    }
    return;
}
/* end of fbe_test_sep_util_randomize_rg_configuration_array() */

fbe_status_t
fbe_test_sep_util_create_raid_group_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                  fbe_u32_t raid_group_count)
                                                  
{
    return fbe_test_sep_util_create_configuration(raid_group_configuration_p,
                                                                       raid_group_count,
                                                                       FBE_FALSE);
}

fbe_status_t
fbe_test_sep_util_create_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                       fbe_u32_t raid_group_count,
                                       fbe_bool_t encryption_on)
{ 
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rg_index = 0;
    fbe_u32_t                                   lu_index = 0;
    fbe_lba_t                                   raid_group_capacity_for_lun = 0;
    fbe_test_rg_configuration_t * const         orig_raid_group_configuration_p = raid_group_configuration_p;
    fbe_object_id_t                             rg_object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (raid_group_configuration_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(raid_group_configuration_p);
        }
        if (!fbe_test_rg_config_is_enabled(raid_group_configuration_p))
        {
            raid_group_configuration_p++;
            continue;
        }
        /* Wait for pvds creation*/
        fbe_test_sep_util_wait_for_pvd_creation(raid_group_configuration_p->rg_disk_set,raid_group_configuration_p->width,10000);
        /* Create a RAID group with user provided configuration. */
        status = fbe_test_sep_util_create_raid_group(raid_group_configuration_p);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of RAID group failed.");

        if(encryption_on)
        {
            status = fbe_test_sep_util_setup_rg_keys(raid_group_configuration_p);
        }
        raid_group_configuration_p++;
    }

    raid_group_configuration_p = orig_raid_group_configuration_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(raid_group_configuration_p))
        {
            raid_group_configuration_p++;
            continue;
        }
        /* wait for notification from job service. */
        status = fbe_api_common_wait_for_job(raid_group_configuration_p->job_number,
                                             FBE_TEST_UTILS_CREATE_TIMEOUT_MS,
                                             &job_error_code,
                                             &job_status,
                                             NULL);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Create RG job failed");
        MUT_ASSERT_TRUE_MSG((job_error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR),  "RG creation failed"); 

        raid_group_configuration_p++;
    }

    /*hack just to make sure we wait for them to be ready*/
    fbe_api_sleep(1000);
    raid_group_configuration_p = orig_raid_group_configuration_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(raid_group_configuration_p))
        {
            raid_group_configuration_p++;
            continue;
        }
        /* Find the object id of the raid group */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group_configuration_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* verify the raid group get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, FBE_TEST_UTILS_CREATE_TIMEOUT_MS,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Calcuate the RAID group capacity required for the Logical unit configuration. */
        status = fbe_test_sep_util_calculate_raid_group_capacity_for_lun_configuration(raid_group_configuration_p,
                                                                                       &raid_group_capacity_for_lun);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "LUN imported capacity calculation failed.");

        /* RAID group capacity for LUN is higher than then user provided 
         * configured capacity and so return error.
         */
        MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(FBE_LBA_INVALID, raid_group_configuration_p->capacity, "RAID group exported capacity is invalid.");
        MUT_ASSERT_TRUE_MSG((raid_group_capacity_for_lun <= raid_group_configuration_p->capacity), 
                            "User asked RAID group capacity is not enough to create user LUNs with user given capacity for LUN.");

        /* Create a set of LUNs for this RAID group. */
        status = fbe_test_sep_util_create_logical_unit_configuration(raid_group_configuration_p->logical_unit_configuration_list,
                                                                     raid_group_configuration_p->number_of_luns_per_rg);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical units failed.");

        raid_group_configuration_p++; 
    }

    raid_group_configuration_p = orig_raid_group_configuration_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_object_id_t lun_object_id;
        if (!fbe_test_rg_config_is_enabled(raid_group_configuration_p))
        {
            raid_group_configuration_p++;
            continue;
        }
        for (lu_index = 0; lu_index < raid_group_configuration_p->number_of_luns_per_rg; lu_index++)
        {
            /* find the object id of the lun - unless the lun id was defaulted .... we have no way of knowing the lun
             id or the object id in that case */
            if ( raid_group_configuration_p->logical_unit_configuration_list[lu_index].lun_number != FBE_LUN_ID_INVALID )
            {
                status = fbe_api_database_lookup_lun_by_number(raid_group_configuration_p->logical_unit_configuration_list[lu_index].lun_number, 
                                                                    &lun_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
                
                /* verify the lun get to ready state in reasonable time. */
                status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                      lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                                      FBE_TEST_WAIT_OBJECT_TIMEOUT_MS,
                                                                                      FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                                  __FUNCTION__, lun_object_id,  raid_group_configuration_p->logical_unit_configuration_list[lu_index].raid_type);
                /* verify the lun connect to BVD in reasonable time. */
                status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                           lun_object_id,
                                                                           FBE_TEST_WAIT_OBJECT_TIMEOUT_MS);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x connects to BVD. encrypt: %d",
                                  __FUNCTION__, lun_object_id, encryption_on);
            }
        }
        raid_group_configuration_p++;
    }
    
    return status;
}

fbe_status_t
fbe_test_sep_util_destroy_raid_group_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p, fbe_u32_t raid_group_count)
{

    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       rg_index = 0;

    /* Go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if(raid_group_configuration_p == NULL) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User has given invalid RAID group configuration.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (!fbe_test_rg_config_is_enabled(raid_group_configuration_p))
        {
            raid_group_configuration_p++;
            continue;
        }

        /* Destroy a set of LUNs for this RAID group. */
        status = fbe_test_sep_util_destroy_logical_unit_configuration(raid_group_configuration_p->logical_unit_configuration_list,
                                                                      raid_group_configuration_p->number_of_luns_per_rg);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical units failed.");

        /* DEstroy a RAID group with user provided configuration. */
        status = fbe_test_sep_util_destroy_raid_group(raid_group_configuration_p->raid_group_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");
        raid_group_configuration_p++;
    }
    
    return status;
}
/*!**************************************************************
 * fbe_test_sep_util_destroy_all_user_luns()
 ****************************************************************
 * @brief
 *  Destroy all the user LUNs in the system.
 *
 * @param None.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_destroy_all_user_luns(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t lun_index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_u32_t lun_number;
    fbe_bool_t b_is_system_object;

    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

    if (status != FBE_STATUS_OK)
    {
        if ( obj_list_p)
        {
            fbe_api_free_memory(obj_list_p);
        }
        return status;
    }

       /* Go through all the LUNs for this RAID group. */
    for (lun_index = 0; lun_index < num_objects; lun_index++)
    {
        status = fbe_api_database_lookup_lun_by_object_id(obj_list_p[lun_index], &lun_number);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot map to unit number status 0x%x\n",
                          __FUNCTION__, obj_list_p[lun_index], status);
            continue;
        }

        b_is_system_object = FBE_FALSE;
        status = fbe_api_database_is_system_object(obj_list_p[lun_index], &b_is_system_object);

        if (status != FBE_STATUS_OK) 
        { 
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot determine if system obj 0x%x\n",
                          __FUNCTION__, obj_list_p[lun_index], status);
            continue;
        }
        /* We only destroy user objects.
         */
        if (b_is_system_object)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s skipping system lun %d\n", __FUNCTION__, lun_number);
            continue;
        }
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s destroy lun %d\n", __FUNCTION__, lun_number);

        /* Destroy a logical unit.
         */
        status = fbe_test_sep_util_destroy_logical_unit(lun_number);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical Unit failed.");
    }
    if (obj_list_p)
    {
        fbe_api_free_memory(obj_list_p);
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_destroy_all_user_luns()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_destroy_all_raid_group_of_class()
 ****************************************************************
 * @brief
 *  Destroy every raid group that has the input class id.
 *
 * @param class_id - The class id to destroy.              
 *
 * @return fbe_status_t
 *
 * @author
 *  3/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_destroy_all_raid_group_of_class(fbe_class_id_t class_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t rg_index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_u32_t raid_group_number;
    fbe_bool_t b_is_system_object;

    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

    if (status != FBE_STATUS_OK)
    {
        if(obj_list_p)
        {
            fbe_api_free_memory(obj_list_p);
        }
        return status;
    }

    /* Go through all the LUNs for this RAID group. */
    for (rg_index = 0; rg_index < num_objects; rg_index++)
    {
        status = fbe_api_database_lookup_raid_group_by_object_id(obj_list_p[rg_index], &raid_group_number);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot map to unit number status 0x%x\n",
                          __FUNCTION__, obj_list_p[rg_index], status);
            continue;
        }
        b_is_system_object = FBE_FALSE;
        status = fbe_api_database_is_system_object(obj_list_p[rg_index], &b_is_system_object);

        if (status != FBE_STATUS_OK) 
        { 
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot determine if system obj 0x%x\n",
                          __FUNCTION__, obj_list_p[rg_index], status);
            continue;
        }
        /* We only destroy user objects.
         */
        if (b_is_system_object)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s skipping system rg %d\n", __FUNCTION__, raid_group_number);
            continue;
        }
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s destroy raid group %d\n", __FUNCTION__, raid_group_number);
        /* Destroy a logical unit.
         */
        status = fbe_test_sep_util_destroy_raid_group(raid_group_number);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of raid group failed.");
    }
    if(obj_list_p)
    {
        fbe_api_free_memory(obj_list_p);
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_destroy_all_raid_group_of_class()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_destroy_all_user_raid_groups()
 ****************************************************************
 * @brief
 *   Destroy every raid group the user has bound.
 *
 * @param None.             
 *
 * @return fbe_status_t 
 *
 * @author
 *  3/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_destroy_all_user_raid_groups(void)
{
    fbe_status_t status;
    /* First destroy the upper level raid groups. 
     * Then destroy the lower level (mirror raid groups). 
     */
    status = fbe_test_sep_util_destroy_all_raid_group_of_class(FBE_CLASS_ID_PARITY);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_test_sep_util_destroy_all_raid_group_of_class(FBE_CLASS_ID_STRIPER);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_test_sep_util_destroy_all_raid_group_of_class(FBE_CLASS_ID_MIRROR);
    return status;
}
/******************************************
 * end fbe_test_sep_util_destroy_all_user_raid_groups()
 ******************************************/

fbe_status_t fbe_test_sep_util_get_next_disk_in_set(fbe_u32_t disk_set_index,
                                                    fbe_test_raid_group_disk_set_t *base_disk_set_p,
                                                    fbe_test_raid_group_disk_set_t **disk_set_p)
{
    if (base_disk_set_p[disk_set_index].bus == FBE_U32_MAX)
    {
        /* We hit the end of the array. FBE_U32_MAX is used as a terminator.
         */
        *disk_set_p = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *disk_set_p = &base_disk_set_p[disk_set_index];
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_test_sep_util_get_block_size_enum()
 ****************************************************************
 * @brief
 *  Classify the input block size in terms of an enum.
 *
 * @param block_size - Size of block to classify.
 * @param block_size_enum_p - Enumerated value to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_get_block_size_enum(fbe_block_size_t block_size,
                                                   fbe_test_block_size_t *block_size_enum_p)
{ 
    switch (block_size)
    {
        case FBE_BLOCK_SIZES_520:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_520;
            break;
        case FBE_BLOCK_SIZES_512:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_512;
            break;
        case FBE_BLOCK_SIZES_4096:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_4096;
            break;
        case FBE_BLOCK_SIZES_4160:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_4160;
            break;
        case FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_520;
            break;
        default:
            *block_size_enum_p = FBE_TEST_BLOCK_SIZE_INVALID;
            mut_printf(MUT_LOG_LOW, "%s block size: %d unexpected\n", __FUNCTION__, block_size);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_util_get_block_size_enum()
 ******************************************/
/*!**************************************************************
 * fbe_test_rg_config_is_enabled()
 ****************************************************************
 * @brief
 *  Determine if this configuration is currently enabled.
 *  This value gets set elsewhere either by the init routine
 *  fbe_test_sep_util_init_rg_configuration()
 *  Or via a call to fbe_test_sep_util_setup_rg_config()
 *
 * @param rg_config_p - current raid group config to evaluate.               
 *
 * @return FBE_TRUE - this is enabled.
 *         FBE_FALSE - not enabled.
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_test_rg_config_is_enabled(fbe_test_rg_configuration_t * rg_config_p)
{
    return ((rg_config_p->width != FBE_U32_MAX) && (rg_config_p->b_create_this_pass));
}
/******************************************
 * end fbe_test_rg_config_is_enabled()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_setup_rg_config()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param drive_locations_p - The set of drives we discovered.
 *
 * @return -   
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_setup_rg_config(fbe_test_rg_configuration_t * rg_config_p,
                                               fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rg_index = 0;
    fbe_test_rg_configuration_t * const         orig_rg_config_p = rg_config_p;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_u32_t raid_group_count = 0;
    fbe_test_block_size_t   block_size;
    fbe_test_block_size_t   second_block_size;
    fbe_drive_type_t        drive_type;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));
    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* For the give drive counts, disable any configs not valid for this 
     * physical config. 
     * In other words if we do not have enough drives total to create the 
     * given raid group, then disable it. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        if (rg_config_p->block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE) {
            second_block_size = FBE_TEST_BLOCK_SIZE_4160;
        }

        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type)){
            rg_config_p->b_already_tested = FBE_TRUE;
            rg_config_p->b_create_this_pass = FBE_FALSE;
        }
        if (!rg_config_p->b_already_tested) {
    
            if (rg_config_p->b_use_fixed_disks) {
                rg_config_p->b_create_this_pass = FBE_TRUE;
                rg_config_p->b_already_tested = FBE_TRUE;
                rg_config_p->b_valid_for_physical_config = FBE_TRUE;
            }
            else {
                /* Check if we can find enough drives of same drive type
                 */
                rg_config_p->b_create_this_pass = FBE_FALSE;
                rg_config_p->b_valid_for_physical_config = FBE_FALSE;
    
                for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++) {
                     /* consider extra drives into account if count is set by consumer test
                      */
                     if (rg_config_p->block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE) {
                         fbe_u32_t first_count = rg_config_p->width / 2 + ((rg_config_p->width %2) ? 1 : 0);
                         fbe_u32_t second_count = rg_config_p->width / 2;
                         fbe_u32_t first_extra_count = rg_config_p->num_of_extra_drives / 2 + ((rg_config_p->num_of_extra_drives %2) ? 1 : 0);
                         fbe_u32_t second_extra_count = rg_config_p->num_of_extra_drives / 2;
                         /* If we are mixed, make sure we have enough of each type.
                          */
                        if ( ((first_count + first_extra_count) <= local_drive_counts[block_size][drive_type]) &&
                             ((second_count + second_extra_count) <= local_drive_counts[second_block_size][drive_type])) {
                            /* We will use this in the current pass. 
                             * Mark it as used so that the next time through we will not create it. 
                             */
                            rg_config_p->b_create_this_pass = FBE_TRUE;
                            rg_config_p->b_already_tested = FBE_TRUE;
                            rg_config_p->b_valid_for_physical_config = FBE_TRUE;
                            local_drive_counts[block_size][drive_type] -= first_count;
                            local_drive_counts[block_size][drive_type] -= first_extra_count;
                            local_drive_counts[second_block_size][drive_type] -= second_count;
                            local_drive_counts[second_block_size][drive_type] -= second_extra_count;
                            break;
                        }
                     } else if ((rg_config_p->width + rg_config_p->num_of_extra_drives) <= local_drive_counts[block_size][drive_type]) {
                        /* We will use this in the current pass. 
                         * Mark it as used so that the next time through we will not create it. 
                         */
                        rg_config_p->b_create_this_pass = FBE_TRUE;
                        rg_config_p->b_already_tested = FBE_TRUE;
                        rg_config_p->b_valid_for_physical_config = FBE_TRUE;
                        local_drive_counts[block_size][drive_type] -= rg_config_p->width;
                        local_drive_counts[block_size][drive_type] -= rg_config_p->num_of_extra_drives;
                        break;
                    }
                }
            }
        }
        else {
            rg_config_p->b_create_this_pass = FBE_FALSE;
        }

        rg_config_p++;
    }

    //@todo:herry_monster:  This was written to take in account the capacity of drive while 
    // creating the raid group., disable the 'fbe_test_sep_util_fill_raid_group_disk_set()'
    //fbe_test_sep_util_fill_raid_group_disk_set_with_cap(orig_rg_config_p,
//                                                        raid_group_count,
//                                                        drive_locations_p);


    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_sep_util_discover_raid_group_disk_set(orig_rg_config_p,
                                                                raid_group_count,
                                                                drive_locations_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Consider drive capacity into account while running the test on hardware.
         */

        /* ! @todo: When we will change the spare selection algorithm to select spare drive based
         *   on minimum raid group drive capacity then we do not need this special handling.
         */
        status = fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(orig_rg_config_p,
                                                    raid_group_count,
                                                    drive_locations_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /*! @todo really we should change the below function to take a fbe_test_drive_locations_t structure.
     */

    /* This function will update unused drive info for those raid groups
       which are going to be used in this pass
    */
    fbe_test_sep_util_set_unused_drive_info(orig_rg_config_p,
                                            raid_group_count,
                                            local_drive_counts,
                                            drive_locations_p);

    return status;
}
/******************************************
 * end fbe_test_sep_util_setup_rg_config()
 ******************************************/

fbe_status_t fbe_test_sep_util_report_created_rgs(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rg_index = 0;
    fbe_u32_t raid_group_count = 0;
    fbe_u32_t tested_count = 0;
    fbe_u32_t invalid_count = 0;
    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* For the give drive counts, disable any configs not valid for this 
     * physical config. 
     * In other words if we do not have enough drives total to create the 
     * given raid group, then disable it. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (rg_config_p->b_already_tested)
        {
            tested_count++;
        }
        else if (rg_config_p->b_valid_for_physical_config == FBE_FALSE)
        {
            invalid_count++;
        }
        rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Tested %d out of %d raid groups", tested_count, raid_group_count);
    return status;
}
/*!**************************************************************
 * fbe_test_launch_cli()
 ****************************************************************
 * @brief
 *  Launch cli for one SP.
 *
 * @param b_both - TRUE to launch both SPs, FALSE to launch SPA.               
 *
 * @return None   
 *
 * @author
 *  2/15/2013 - Created. Rob Foley
 *
 ****************************************************************/
static const fbe_char_t *fbe_test_cli_extension = "fbecli.exe";
static void fbe_test_launch_cli(fbe_bool_t b_spa, fbe_bool_t b_spb)
{
#ifdef ALAMOSA_WINDOWS_ENV /* The command line is different on Linux */
    char startup_stringa[120] = "start ";
    char startup_stringb[120] = "start ";
#else /* We are running on Linux */
    char startup_stringa[200] = "xterm -iconic -sb -sl 50000 -e ";
    char startup_stringb[200] = "xterm -iconic -sb -sl 50000 -e ";
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
    const fbe_u8_t *port_str = NULL;
    fbe_status_t status;

    status = fbe_api_transport_map_server_mode_to_port(FBE_SIM_SP_A, &port_str);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to map server target to port\n", __FUNCTION__);
        return;
    }
    if (b_spa)
    {
        sprintf(startup_stringa, "%s %s s a p%s", startup_stringa, fbe_test_cli_extension, port_str);
    #ifndef ALAMOSA_WINDOWS_ENV
        strcat(startup_stringa, " 2>&1 > /dev/null &");
    #endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
        mut_printf(MUT_LOG_HIGH, "Launching program %s\n", startup_stringa);
        system(startup_stringa);
    }

    if (b_spb)
    {
        sprintf(startup_stringb, "%s %s s b p%s", startup_stringb, fbe_test_cli_extension, port_str);
    #ifndef ALAMOSA_WINDOWS_ENV
        strcat(startup_stringb, " 2>&1 > /dev/null &");
    #endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
        mut_printf(MUT_LOG_HIGH, "Launching program %s\n", startup_stringb);
        system(startup_stringb);
    }

    return;
}
/******************************************
 * end fbe_test_launch_cli()
 ******************************************/

/*!**************************************************************
 * fbe_test_check_launch_cli()
 ****************************************************************
 * @brief
 *  Check to see if we need to launch the CLI.
 * 
 *  Then pause after launching the cli.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/22/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_check_launch_cli(void)
{
    char *value = NULL;
    if (fbe_test_sep_util_get_cmd_option("-cli"))
    {
        value = mut_get_user_option_value("-cli");
        
        if ((value == NULL) || !strcmp(value, "spa"))
        {
            fbe_test_launch_cli(FBE_TRUE, FBE_FALSE);
            mut_printf(MUT_LOG_LOW, "Test paused, press any key to continue...");
            getchar();
        }
        else if (!strcmp(value, "spb"))
        {
            fbe_test_launch_cli(FBE_FALSE, FBE_TRUE);
            mut_printf(MUT_LOG_LOW, "Test paused, press any key to continue...");
            getchar();
        }
        else if (!strcmp(value, "both"))
        {
            fbe_test_launch_cli(FBE_TRUE, FBE_TRUE);
            mut_printf(MUT_LOG_LOW, "Test paused, press any key to continue...");
            getchar();
        }
    }
}
/******************************************
 * end fbe_test_check_launch_cli()
 ******************************************/
/*!**************************************************************
 * fbe_test_run_test_on_rg_config()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  5/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_run_test_on_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, context_p, test_fn, luns_per_rg, chunks_per_lun,FBE_TRUE);
    return;
}
/******************************************
 * end fbe_test_run_test_on_rg_config()
 ******************************************/

/*!**************************************************************
 * fbe_test_run_test_on_rg_config_encrypted()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations with encryption.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  5/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_run_test_on_rg_config_encrypted(fbe_test_rg_configuration_t *rg_config_p,
                                              void * context_p,
                                              fbe_test_rg_config_test test_fn,
                                              fbe_u32_t luns_per_rg,
                                              fbe_u32_t chunks_per_lun)
{
    fbe_test_run_test_on_rg_config_encrypted_with_time_save(rg_config_p, context_p, test_fn, luns_per_rg, chunks_per_lun,FBE_TRUE);
    return;
}
/******************************************
 * end fbe_test_run_test_on_rg_config_encrypted()
 ******************************************/

void fbe_test_create_raid_group_params_init(fbe_test_create_raid_group_params_t *params_p)
{
    fbe_zero_memory(params_p, sizeof(fbe_test_create_raid_group_params_t));

    /* Default is false.
     */
    params_p->b_encrypted = FBE_FALSE;
    params_p->b_skip_create = FBE_FALSE;
}

void fbe_test_create_raid_group_params_init_for_time_save(fbe_test_create_raid_group_params_t *params_p,
                                                          void * context_p,
                                                          fbe_test_rg_config_test test_fn,
                                                          fbe_u32_t luns_per_rg,
                                                          fbe_u32_t chunks_per_lun,
                                                          fbe_bool_t b_no_destroy)
{
    fbe_test_create_raid_group_params_init(params_p);

    params_p->context_p = context_p;
    params_p->test_fn = test_fn;
    params_p->luns_per_rg = luns_per_rg;
    if (chunks_per_lun > 10)
    {
        params_p->chunks_per_lun = chunks_per_lun;
    } 
    else
    {
        params_p->chunks_per_lun = chunks_per_lun * sep_rebuild_utils_chunks_per_rebuild_g; 
    }
    params_p->destroy_rg_config = !b_no_destroy;
}
static fbe_u32_t fbe_test_case_bitmask = 0; 
void fbe_test_set_case_bitmask(fbe_u32_t bitmask)
{
    fbe_test_case_bitmask = bitmask;
}
fbe_bool_t fbe_test_get_test_case_bitmask(fbe_u32_t *bitmask_p)
{
    fbe_bool_t b_return = FBE_FALSE;
    fbe_u32_t debug_flags = 0;

    b_return = fbe_test_sep_util_get_cmd_option_int("-run_test_cases", &debug_flags);

    if (fbe_test_case_bitmask != 0) {
        debug_flags |= fbe_test_case_bitmask;
        b_return = FBE_TRUE;
    }
    if (b_return) {
        mut_printf(MUT_LOG_TEST_STATUS, "-run_test_cases 0x%x\n", debug_flags);
    }
    *bitmask_p = debug_flags;
    return b_return;
}
/*!**************************************************************
 * fbe_test_run_test_on_rg_config_with_time_save()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - Destory the rg config.
 *
 * @return  None.
 *
 * @author
 *  1/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_run_test_on_rg_config_params(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_test_create_raid_group_params_t *params_p)
{
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_u32_t raid_group_count_this_pass = 0;
    fbe_u32_t raid_group_count = 0;
    void * context_p = params_p->context_p;
    fbe_test_rg_config_test test_fn = params_p->test_fn;
    fbe_u32_t luns_per_rg = params_p->luns_per_rg;
    fbe_u32_t chunks_per_lun = params_p->chunks_per_lun;
    fbe_bool_t destroy_rg_config = params_p->destroy_rg_config;
    fbe_bool_t b_encrypted = params_p->b_encrypted;

    if (fbe_test_sep_util_get_cmd_option("-skip_test")) {
        mut_printf(MUT_LOG_TEST_STATUS, "== -skip_test is set, do not run test.");
        return;
    }

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);

    /* If we have another iteration, then make sure to destroy the config. 
     * If we are done then we do not need to destroy the config. 
     */
    if ((raid_group_count != raid_group_count_this_pass) &&
        (fbe_test_get_rg_tested_count(rg_config_p) != raid_group_count)){
       destroy_rg_config = FBE_TRUE;
    }
    /* Continue to loop until we have no more raid groups.
     */
    while (raid_group_count_this_pass > 0)
    {
        fbe_u32_t debug_flags;
        fbe_u32_t trace_level = FBE_TRACE_LEVEL_INFO;
        mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);
        /* Create the raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
        /* Setup the lun capacity with the given number of chunks for each lun.
         */
        fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, raid_group_count,
                                                          chunks_per_lun, 
                                                          luns_per_rg);

        /* Kick of the creation of all the raid group with Logical unit configuration.
         */
        if (!params_p->b_skip_create) {
            fbe_test_sep_util_create_configuration(rg_config_p, raid_group_count, b_encrypted);
        }
        
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_trace_level", &trace_level))
        {
            fbe_test_sep_util_set_rg_trace_level_both_sps(rg_config_p, (fbe_trace_level_t) trace_level);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_library_debug_flags_both_sps(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
        }
        if (fbe_test_get_test_case_bitmask(&debug_flags))
        {
            rg_config_p->test_case_bitmask = debug_flags;
        }
        else
        {
            rg_config_p->test_case_bitmask = 0;
        }
        /* With this option we will skip the test, we just create/destroy the configs.
         */
        mut_pause();
        fbe_test_check_launch_cli();

        if (fbe_test_sep_util_get_cmd_option("-panic_sp"))
        {
            fbe_api_panic_sp();
        }
        if (fbe_test_sep_util_get_cmd_option("-fail_test"))
        {
            MUT_FAIL_MSG("Failing test on purpose because of -fail_test argument");
        }
        if (fbe_test_sep_util_get_cmd_option("-just_create_config"))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "-just_create_config selected, skipping test\n");
        }
        else
        {
            fbe_u32_t repeat_count = 1;
            fbe_u32_t loop_count = 0;
            /* The default number of times to run is 1, -repeat_count can alter this.
             */
            fbe_test_sep_util_get_cmd_option_int("-repeat_count", &repeat_count);
            /* Run the tests some number of times.
             * Note it is expected that a 0 repeat count will cause us to not run the test.
             */
            while (loop_count < repeat_count)
            {
                test_fn(rg_config_p, context_p);
                loop_count++;
                /* Only trace if the user supplied a repeat count.
                 */
                if (repeat_count > 0)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "test iteration [%d of %d] completed", loop_count, repeat_count);
                }
            }
        }

        if (destroy_rg_config && !params_p->b_skip_create) {
           /* Check for errors and destroy the rg config.
            */
           fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count_this_pass);
        }

        /* Setup the rgs for the next pass.
         */
        fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);
        raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    }

    fbe_test_sep_util_report_created_rgs(rg_config_p);

    /* Done testing rgconfigs. Mark them invalid.
     */
    fbe_test_rg_config_mark_invalid(rg_config_p);
    return;
}
/**************************************
 * end fbe_test_run_test_on_rg_config_params()
 **************************************/
/*!**************************************************************
 * fbe_test_run_test_on_rg_config_with_time_save()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - Destory the rg config.
 *
 * @return  None.
 *
 * @author
 *  1/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_run_test_on_rg_config_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                                   void * context_p,
                                                   fbe_test_rg_config_test test_fn,
                                                   fbe_u32_t luns_per_rg,
                                                   fbe_u32_t chunks_per_lun,
                                                   fbe_bool_t destroy_rg_config)
{
    fbe_test_create_raid_group_params_t params;

    fbe_test_create_raid_group_params_init_for_time_save(&params, context_p, test_fn,luns_per_rg,chunks_per_lun,destroy_rg_config);
    /* Take defaults for other params. */

    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
}

/******************************************
 * end fbe_test_run_test_on_rg_config_with_time_save()
 ******************************************/

/*!**************************************************************
 * fbe_test_run_test_on_rg_config_with_time_save()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - Destory the rg config.
 *
 * @return  None.
 *
 * @author
 *  1/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_run_test_on_rg_config_encrypted_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                                             void * context_p,
                                                             fbe_test_rg_config_test test_fn,
                                                             fbe_u32_t luns_per_rg,
                                                             fbe_u32_t chunks_per_lun,
                                                             fbe_bool_t destroy_rg_config)
{
    fbe_test_create_raid_group_params_t params;

    fbe_test_create_raid_group_params_init_for_time_save(&params, context_p, test_fn,luns_per_rg,chunks_per_lun,destroy_rg_config);
    /* Take defaults for other params. */

    params.b_encrypted = FBE_TRUE;

    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
}

/******************************************
 * end fbe_test_run_test_on_rg_config_encrypted_with_time_save()
 ******************************************/

/*!**************************************************************
 * fbe_test_run_test_on_rg_config_with_extra_disk()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  06/01/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{
    /* run the test */
    fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, context_p, test_fn,
                                   luns_per_rg,
                                   chunks_per_lun,
                                   FBE_TRUE);

    return;
}
/******************************************
 * end fbe_test_run_test_on_rg_config_with_extra_disk()
 ******************************************/


/*!**************************************************************
 * fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - FBE_TRUE does NOT destroy the RG - saves time. FBE_FALSE destroys the RG
 *
 * @return  None.
 *
 * @author
 *  06/01/2011 - Created. Preethi Poddatur
 *
 ****************************************************************/


void fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun,
                                    fbe_bool_t destroy_rg_config)
{

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    /* populate the required extra disk count in rg config */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
     
    /* Generally, the extra disks on rg will be configured as hot spares. In order to make configuring 
    particular PVDs(extra disks) as hot spares work, we need to set set automatic hot spare not working (set to false). */    
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    if (destroy_rg_config == FBE_TRUE) {
       /* run the test */
       fbe_test_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun);
    }
    else {
       /* run the test */
       fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun,
                                      FBE_FALSE);
    }

    return;
}


/******************************************
 * end fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save()
 ******************************************/




/*!**************************************************************
 * fbe_test_run_test_on_rg_config_no_create()
 ****************************************************************
 * @brief
 *  Given a raid group config, determine which disks to use 
 *  and run the test on the applicable configurations.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param test_fn - The test function to run
 *
 * @return -   
 *
 * @author
 *  5/5/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_test_run_test_on_rg_config_no_create(fbe_test_rg_configuration_t *rg_config_p,
                                              void * context_p,
                                              fbe_test_rg_config_test test_fn)
{
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_u32_t raid_group_count_this_pass = 0;
    fbe_u32_t raid_group_count = 0;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);

    /* Continue to loop until we have no more raid groups.
     */
    while (raid_group_count_this_pass > 0)
    {
        mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);
        /* Create the raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Run the tests.
         */
        test_fn(rg_config_p, context_p);

        /* Setup the rgs for the next pass.
         */
        fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);
        raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    }
    fbe_test_sep_util_report_created_rgs(rg_config_p);
    return;
}
/**************************************************
 * end fbe_test_run_test_on_rg_config_no_create()
 **************************************************/

fbe_status_t fbe_test_sep_util_get_pdo_location_by_object_id(fbe_object_id_t pdo_object_id,
                                                             fbe_port_number_t *port_p,
                                                             fbe_enclosure_number_t *enc_p,
                                                             fbe_enclosure_slot_number_t *slot_p)
{
    fbe_status_t status;
    *port_p = FBE_PORT_NUMBER_INVALID;
    *enc_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    status  = fbe_api_get_object_port_number(pdo_object_id, port_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* We have an enclosure.
     */
    status = fbe_api_get_object_enclosure_number(pdo_object_id, enc_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    /* We have a slot.
     */
    status = fbe_api_get_object_drive_number(pdo_object_id, slot_p);
    return status;
}


/*!**************************************************************
 * fbe_test_sep_util_get_upstream_object_count()
 ****************************************************************
 * @brief
 *  This function returns the number of upstream objects connected with pvd object.
 * 
 *
 * @param object_id - pvd object id
 * @param number_of_upstream_edges - number of upstream edges to return
 *
 * @return  fbe_status.
 *
 * @author
 *  06/08/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_get_upstream_object_count(fbe_object_id_t object_id, 
                                                                        fbe_u32_t *number_of_upstream_edges)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;

    upstream_object_list.number_of_upstream_objects = 0;

    status = fbe_api_base_config_get_upstream_object_list(object_id, &upstream_object_list);

    if(status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW, "=== %s fail to get upstream object count  ===\n", __FUNCTION__);
        return status;
    }

    *number_of_upstream_edges = upstream_object_list.number_of_upstream_objects ;

    return status;

}
/**************************************************
 * end fbe_test_sep_util_get_upstream_object_count()
 **************************************************/
/*!**************************************************************
 * fbe_test_sep_util_get_any_unused_pvd_object_id()
 ****************************************************************
 * @brief
 *  This function returns object ID of ANY PVD which is not part of any RG
 * 
 * @param unused_pvd_object_it - Buffer to store the value
 *
 * @return  fbe_status_t - FBE_STATUS_OK if unused PVD found. 
 *                         FBE_STATUS_GENERIC_FAILURE otherwise.
 *
 * @author
 *  06/01/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_get_any_unused_pvd_object_id(fbe_object_id_t *unused_pvd_object_id)
{
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_block_size_t               block_size; 
    fbe_drive_type_t                    drive_type_index;
    fbe_status_t status;

    /* find the all available drive info */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* find the all available unused pvd info */
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);

     /* loop over all available drives and find out unused pvd
     */
    for(block_size = 0; block_size <= FBE_TEST_BLOCK_SIZE_LAST; block_size++)
    {
        for(drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++)
        {
            if(unused_pvd_locations.drive_counts[block_size][drive_type_index] != 0)
            {
                // get id of a PVD object
                status = fbe_api_provision_drive_get_obj_id_by_location( unused_pvd_locations.disk_set[block_size][drive_type_index][0].bus,
                                                                         unused_pvd_locations.disk_set[block_size][drive_type_index][0].enclosure,
                                                                         unused_pvd_locations.disk_set[block_size][drive_type_index][0].slot,
                                                                         unused_pvd_object_id);
                return status;
            }
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/*!**************************************************************
 * fbe_test_sep_util_get_all_unused_pvd_location()
 ****************************************************************
 * @brief
 *  This function find out unused pvd locations from given disk set and returns
 *  that info to the caller.
 * 
 *
 * @param drive_locations_p - pointer to all available drive locations
 * @param unused_pvd_locations_p - pointer to fill out unused pvd location
 *
 * @return  None.
 *
 * @author
 *  06/01/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_get_all_unused_pvd_location(fbe_test_discovered_drive_locations_t *drive_locations_p,
                                        fbe_test_discovered_drive_locations_t *unused_pvd_locations_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     pvd_object_id;
    fbe_u32_t                           index;
    fbe_u32_t                           unused_pvd_index;
    fbe_test_block_size_t               block_size; 
    fbe_test_raid_group_disk_set_t      *drive_info_p = NULL;
    fbe_drive_type_t                    drive_type_index;
    fbe_u32_t                           upstream_object_count;
    fbe_lifecycle_state_t               lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lba_t                           drive_capacity = FBE_LBA_INVALID;
    fbe_api_provision_drive_info_t      provision_drive_info;

    /* Zero the output array
     */
    fbe_zero_memory(unused_pvd_locations_p, sizeof(*unused_pvd_locations_p));

    /* loop over all available drives and find out unused pvd
     */
    for(block_size = 0; block_size <= FBE_TEST_BLOCK_SIZE_LAST; block_size++)
    {
        for(drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++)
        {
            unused_pvd_locations_p->drive_counts[block_size][drive_type_index] = 0;   
            unused_pvd_index = 0;
            for(index =0 ; index < drive_locations_p->drive_counts[block_size][drive_type_index]; index ++)
            {
                drive_info_p = &drive_locations_p->disk_set[block_size][drive_type_index][index];

                /* get the object id.of provision drive */
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_info_p->bus,
                                                drive_info_p->enclosure,
                                                drive_info_p->slot,
                                                &pvd_object_id);
                if (status != FBE_STATUS_OK)
                {
                    /* Invaid pvd object continue*/
                    continue;
                }

                status = fbe_api_get_object_lifecycle_state(pvd_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
                if ((status != FBE_STATUS_OK)                                ||
                    ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)     &&
                     (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)    )    )
                {
                    /* Not ready, continue */
                    continue;
                }

                /* Check the EOL */
                status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (provision_drive_info.end_of_life_state)
                {
                    /* EOL set, skip*/
                    continue;
                }

                /* Validate the type is either `unconsumed' or `test spare'.
                 */
                if ((provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)          &&
                    (provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)    )
                {
                    /* This provision drive is not usable as a spare.*/
                    continue;
                }

                /* get the number of upstream objects for pvd
                 */
                fbe_test_sep_util_get_upstream_object_count(pvd_object_id, &upstream_object_count);

                /* if upstream object count is zero then consider pvd as unused
                */
                if (upstream_object_count == 0)
                {
                    unused_pvd_locations_p->disk_set[block_size][drive_type_index][unused_pvd_index].bus = drive_info_p->bus;
                    unused_pvd_locations_p->disk_set[block_size][drive_type_index][unused_pvd_index].enclosure= drive_info_p->enclosure;
                    unused_pvd_locations_p->disk_set[block_size][drive_type_index][unused_pvd_index].slot = drive_info_p->slot;
                    unused_pvd_locations_p->drive_counts[block_size][drive_type_index]++;

                    /* get the drive capacity to trace it
                     */
                    fbe_test_sep_util_get_drive_capacity(& unused_pvd_locations_p->disk_set[block_size][drive_type_index][unused_pvd_index], 
                                                                            &drive_capacity);
                    /* mut_printf(MUT_LOG_TEST_STATUS, "== found unused pvd (%d_%d_%d), block size %d, drive type %d, cap 0x%x . ==", 
                            drive_info_p->bus, drive_info_p->enclosure, drive_info_p->slot, block_size, drive_type_index,drive_capacity );
                     */
                

                    unused_pvd_index++;
                }
            }
        }
    }
    return;
}
/**************************************************
 * end fbe_test_sep_util_get_all_unused_pvd_location()
 **************************************************/

void fbe_test_sep_util_discover_all_drive_information(fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_handle = 0;
    fbe_u32_t                   port_idx = 0, drive_idx = 0;
    fbe_port_number_t           port_number;
    fbe_u32_t                   total_objects = 0;
    fbe_class_id_t              class_id;
    fbe_object_id_t            *obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   board_count = 0, port_count = 0, encl_count = 0, physical_drive_count = 0;
    fbe_drive_type_t            drive_type_enum;
    const fbe_u32_t             READY_STATE_WAIT_TIME_LOCAL = 3000;
    fbe_u32_t                   drive_type = 0;
    fbe_u32_t                   system_drive_missing_count = 0;
    fbe_u32_t                   drive_count_520 = 0, drive_count_512 = 0, drive_count_4096 = 0, drive_count_4160 = 0;

    
    #define MAX_DRIVES_TO_IGNORE 4
    #define MAX_MISSING_SYSTEM_DRIVE    (MAX_DRIVES_TO_IGNORE - (MAX_DRIVES_TO_IGNORE - 1))
    /*! @note First (4) drives are system drives
     */
    /* keep track of first N drives since we do not want to allow creation of configuration on these drives yet.
     */
    fbe_object_id_t pdo_object_id[MAX_DRIVES_TO_IGNORE]; 
    fbe_object_id_t pvd_object_id[MAX_DRIVES_TO_IGNORE]; 

    fbe_zero_memory(drive_locations_p, sizeof(fbe_test_discovered_drive_locations_t));
    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* Today the board is always object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(class_id, FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port.
     */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        /* For the first port wait for the port and enclosure to come up.
         */
        if (port_idx == 0)
        {
            fbe_u32_t           object_handle;
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state(object_handle,
                                                              FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number(object_handle, &port_number);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Verify if the enclosure exists */
            status = fbe_api_get_enclosure_object_id_by_location(port_idx, 0, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state of the enclosure */
            status = fbe_api_wait_for_object_lifecycle_state(object_handle, FBE_LIFECYCLE_STATE_READY, 
                                                             READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < MAX_DRIVES_TO_IGNORE; drive_idx++)
            {
                /* Get the pvd object id
                 */
                status = fbe_api_provision_drive_get_obj_id_by_location(port_idx, 0, drive_idx, &pvd_object_id[drive_idx]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Validate that at least (MAX_DRIVES_TO_IGNORE - MAX_MISSING_SYSTEM_DRIVE)
                 * are in the Ready state.
                 */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
                                                           FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL);
                if (status != FBE_STATUS_OK)
                {
                    /* We allow (1) degraded system drive.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS, "%s system drive: %d_%d_%d (pvd obj: 0x%x) not in Ready state.", 
                               __FUNCTION__, port_idx, 0, drive_idx, pvd_object_id[drive_idx]);
                    pdo_object_id[drive_idx] = FBE_OBJECT_ID_INVALID;
                    system_drive_missing_count++;
                    MUT_ASSERT_TRUE_MSG((system_drive_missing_count <= MAX_MISSING_SYSTEM_DRIVE), "TOO MANY SYSTEM DISKS MISSING");
                }
                else
                {
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, 0, drive_idx, &pdo_object_id[drive_idx]);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
        else
        {
            /* Get the handle of the port and validate port exists or not
             */
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    /* We do not know exactly how many drives we have.  We assume more than 0 objects total.
     */
    MUT_ASSERT_INT_NOT_EQUAL(total_objects, 0);
    mut_printf(MUT_LOG_LOW, "Total number of object is %d", total_objects);

    obj_list = (fbe_object_id_t *)malloc(sizeof(fbe_object_id_t) * total_objects);
    MUT_ASSERT_TRUE(obj_list != NULL);

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Loop over all the objects in the physical package and
     * determine how many drives there are. 
     */
    for (obj_count = 0; obj_count < object_num; obj_count++)
    {
        fbe_physical_drive_information_t drive_info;
        fbe_test_block_size_t block_size_enum;
        fbe_object_id_t object_id = obj_list[obj_count];
        status = fbe_api_get_object_type(obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            /* If the object is in a bad state just skip it.
             */
            continue;
        }

        switch (obj_type)
        {
            case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
                ++board_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
                ++port_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
                ++encl_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
                ++physical_drive_count;

                 /* Attempt to bring the drive up
                  */
                status = fbe_test_sep_util_bring_drive_up(object_id, READY_STATE_WAIT_TIME_LOCAL);
                if (status == FBE_STATUS_OK) {


                    /* Fetch the bus, enclosure, slot drive information.
                     */
                    status = fbe_api_physical_drive_get_drive_information(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        mut_printf(MUT_LOG_LOW, "%s unable to get pdo drive info for object id: 0x%x status: 0x%x", 
                                   __FUNCTION__, object_id, status);
                    }
                    /* Determine block size since we are counting how many of each block size we have, 
                     * and we are saving the b_e_s based on the drive block size. 
                     */
                    status = fbe_test_sep_util_get_block_size_enum(drive_info.block_size, &block_size_enum);
                    if (status != FBE_STATUS_OK) 
                    { 
                        mut_printf(MUT_LOG_LOW, "%s object id: 0x%x block size: %d unexpected", 
                                   __FUNCTION__, object_id, drive_info.block_size);
                    }
    
                    /* Determine drive type.  
                     */
                    drive_type_enum = drive_info.drive_type;
    
                    /* We will skip the first 4 drives for now.
                     */
                    if ((object_id != pdo_object_id[0]) && 
                        (object_id != pdo_object_id[1]) && 
                        (object_id != pdo_object_id[2]) && 
                        (object_id != pdo_object_id[3])    ) 
                    {
                        fbe_u32_t drive_index = drive_locations_p->drive_counts[block_size_enum][drive_type_enum];
    
                        status = fbe_test_sep_util_get_pdo_location_by_object_id(object_id,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].bus,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].enclosure,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].slot);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        drive_locations_p->drive_counts[block_size_enum][drive_type_enum]++;
                        //@todo : enable for herry_monster as the drive capacity is the key for the test
                        //drive_locations_p->drive_capacity[block_size_enum][drive_index] = drive_info.gross_capacity;
                    }
                }
                break;
            default:
                break;
        }
    }

    mut_printf(MUT_LOG_LOW, "Total number of board is %d", board_count);
    mut_printf(MUT_LOG_LOW, "Total number of port(s) is %d", port_count);
    mut_printf(MUT_LOG_LOW, "Total number of enclosure(s) is %d", encl_count);

    /* Get the total number of 520 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_520 = drive_count_520 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_520][drive_type];
    }

    /* Get the total number of 512 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_512 = drive_count_512 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_512][drive_type];
    } 
    
    /* Get the total number of 4096 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4096 = drive_count_4096 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4096][drive_type];
    }

    /* Get the total number of 4160 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4160 = drive_count_4160 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4160][drive_type];
    }       
     
    mut_printf(MUT_LOG_LOW, "Total number of physical drives is %d", physical_drive_count);
    mut_printf(MUT_LOG_LOW, "Total number of non-db physical drives by type:  520: %d 512: %d 4096: %d 4160: %d", 
               drive_count_520, 
               drive_count_512,
               drive_count_4096,
               drive_count_4160);

    free(obj_list);

    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful ===");
    return;
}


void fbe_test_sep_util_discover_all_power_save_capable_drive_information(fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_handle = 0;
    fbe_u32_t                   port_idx = 0, drive_idx = 0;
    fbe_port_number_t           port_number;
    fbe_u32_t                   total_objects = 0;
    fbe_class_id_t              class_id;
    fbe_object_id_t            *obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   board_count = 0, port_count = 0, encl_count = 0, physical_drive_count = 0;
    fbe_drive_type_t            drive_type_enum;
    const fbe_u32_t             READY_STATE_WAIT_TIME_LOCAL = 3000;
    fbe_u32_t                   drive_type = 0;
    fbe_u32_t                   system_drive_missing_count = 0;
    fbe_u32_t                   drive_count_520 = 0, drive_count_512 = 0, drive_count_4096 = 0, drive_count_4160 = 0;

    
    #define MAX_DRIVES_TO_IGNORE 4
    #define MAX_MISSING_SYSTEM_DRIVE    (MAX_DRIVES_TO_IGNORE - (MAX_DRIVES_TO_IGNORE - 1))
    /*! @note First (4) drives are system drives
     */
    /* keep track of first N drives since we do not want to allow creation of configuration on these drives yet.
     */
    fbe_object_id_t pdo_object_id[MAX_DRIVES_TO_IGNORE]; 
    fbe_object_id_t pvd_object_id[MAX_DRIVES_TO_IGNORE]; 

    fbe_zero_memory(drive_locations_p, sizeof(fbe_test_discovered_drive_locations_t));
    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* Today the board is always object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(class_id, FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port.
     */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        /* For the first port wait for the port and enclosure to come up.
         */
        if (port_idx == 0)
        {
            fbe_u32_t           object_handle;
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state(object_handle,
                                                              FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number(object_handle, &port_number);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Verify if the enclosure exists */
            status = fbe_api_get_enclosure_object_id_by_location(port_idx, 0, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state of the enclosure */
            status = fbe_api_wait_for_object_lifecycle_state(object_handle, FBE_LIFECYCLE_STATE_READY, 
                                                             READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < MAX_DRIVES_TO_IGNORE; drive_idx++)
            {
                /* Get the pvd object id
                 */
                status = fbe_api_provision_drive_get_obj_id_by_location(port_idx, 0, drive_idx, &pvd_object_id[drive_idx]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Validate that at least (MAX_DRIVES_TO_IGNORE - MAX_MISSING_SYSTEM_DRIVE)
                 * are in the Ready state.
                 */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
                                                           FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL);
                if (status != FBE_STATUS_OK)
                {
                    /* We allow (1) degraded system drive.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS, "%s system drive: %d_%d_%d (pvd obj: 0x%x) not in Ready state.", 
                               __FUNCTION__, port_idx, 0, drive_idx, pvd_object_id[drive_idx]);
                    pdo_object_id[drive_idx] = FBE_OBJECT_ID_INVALID;
                    system_drive_missing_count++;
                    MUT_ASSERT_TRUE_MSG((system_drive_missing_count <= MAX_MISSING_SYSTEM_DRIVE), "TOO MANY SYSTEM DISKS MISSING");
                }
                else
                {
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, 0, drive_idx, &pdo_object_id[drive_idx]);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
        else
        {
            /* Get the handle of the port and validate port exists or not
             */
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    /* We do not know exactly how many drives we have.  We assume more than 0 objects total.
     */
    MUT_ASSERT_INT_NOT_EQUAL(total_objects, 0);
    mut_printf(MUT_LOG_LOW, "Total number of object is %d", total_objects);

    obj_list = (fbe_object_id_t *)malloc(sizeof(fbe_object_id_t) * total_objects);
    MUT_ASSERT_TRUE(obj_list != NULL);

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Loop over all the objects in the physical package and
     * determine how many drives there are. 
     */
    for (obj_count = 0; obj_count < object_num; obj_count++)
    {
        fbe_physical_drive_information_t drive_info;
        fbe_test_block_size_t block_size_enum;
        fbe_object_id_t object_id = obj_list[obj_count];
        status = fbe_api_get_object_type(obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            /* If the object is in a bad state just skip it.
             */
            continue;
        }

        switch (obj_type)
        {
            case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
                ++board_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
                ++port_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
                ++encl_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
                ++physical_drive_count;

                 /* Attempt to bring the drive up
                  */
                status = fbe_test_sep_util_bring_drive_up(object_id, READY_STATE_WAIT_TIME_LOCAL);
                if (status == FBE_STATUS_OK) {


                    /* Fetch the bus, enclosure, slot drive information.
                     */
                    status = fbe_api_physical_drive_get_drive_information(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
					mut_printf(MUT_LOG_LOW, "%s object id: 0x%x spin_down_qualified: %d", 
                                   __FUNCTION__, object_id, drive_info.spin_down_qualified);
                    if (status != FBE_STATUS_OK)
                    {
                        mut_printf(MUT_LOG_LOW, "%s unable to get pdo drive info for object id: 0x%x status: 0x%x", 
                                   __FUNCTION__, object_id, status);
                    }
                    /* Determine block size since we are counting how many of each block size we have, 
                     * and we are saving the b_e_s based on the drive block size. 
                     */
                    status = fbe_test_sep_util_get_block_size_enum(drive_info.block_size, &block_size_enum);
                    if (status != FBE_STATUS_OK) 
                    { 
                        mut_printf(MUT_LOG_LOW, "%s object id: 0x%x block size: %d unexpected", 
                                   __FUNCTION__, object_id, drive_info.block_size);
                    }
    
                    /* Determine drive type.  
                     */
                    drive_type_enum = drive_info.drive_type;
    
                    /* We will skip the first 4 drives and we will skip the drives cannot power save
                     */
                    if ((object_id != pdo_object_id[0]) && 
                        (object_id != pdo_object_id[1]) && 
                        (object_id != pdo_object_id[2]) && 
                        (object_id != pdo_object_id[3]) &&
                         (drive_info.spin_down_qualified)) 
                    {
                        fbe_u32_t drive_index = drive_locations_p->drive_counts[block_size_enum][drive_type_enum];
    
                        status = fbe_test_sep_util_get_pdo_location_by_object_id(object_id,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].bus,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].enclosure,
                                                                                 &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].slot);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        drive_locations_p->drive_counts[block_size_enum][drive_type_enum]++;
                        //@todo : enable for herry_monster as the drive capacity is the key for the test
                        //drive_locations_p->drive_capacity[block_size_enum][drive_index] = drive_info.gross_capacity;
                    }
                }
                break;
            default:
                break;
        }
    }

    mut_printf(MUT_LOG_LOW, "Total number of board is %d", board_count);
    mut_printf(MUT_LOG_LOW, "Total number of port(s) is %d", port_count);
    mut_printf(MUT_LOG_LOW, "Total number of enclosure(s) is %d", encl_count);

    /* Get the total number of 520 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_520 = drive_count_520 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_520][drive_type];
    }

    /* Get the total number of 512 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_512 = drive_count_512 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_512][drive_type];
    } 
    
    /* Get the total number of 4096 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4096 = drive_count_4096 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4096][drive_type];
    }

    /* Get the total number of 4160 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4160 = drive_count_4160 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4160][drive_type];
    }       
     
    mut_printf(MUT_LOG_LOW, "Total number of physical drives is %d", physical_drive_count);
    mut_printf(MUT_LOG_LOW, "Total number of non-db physical drives by type:  520: %d 512: %d 4096: %d 4160: %d", 
               drive_count_520, 
               drive_count_512,
               drive_count_4096,
               drive_count_4160);

    free(obj_list);

    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful ===");
    return;
}


fbe_status_t fbe_test_get_520_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                             fbe_test_raid_group_disk_set_t * disk_set_p)
{
    fbe_u32_t   drive_index = 0;
    fbe_u32_t   drive_cnt = 0;

    for(drive_index = 0; drive_index < FBE_DRIVE_TYPE_LAST; drive_index++)
    {
        drive_cnt = drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_520][drive_index];
        if(drive_cnt != 0)
        {
            drive_cnt--;
            disk_set_p->bus = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_520][drive_index][drive_cnt].bus;
            disk_set_p->enclosure = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_520][drive_index][drive_cnt].enclosure;
            disk_set_p->slot = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_520][drive_index][drive_cnt].slot;
            drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_520][drive_index]--;
            return FBE_STATUS_OK; 
        }
    }
    return FBE_STATUS_NO_OBJECT;
}

fbe_status_t fbe_test_get_4160_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                             fbe_test_raid_group_disk_set_t * disk_set_p)
{
    fbe_u32_t   drive_index = 0;
    fbe_u32_t   drive_cnt = 0;

    for(drive_index = 0; drive_index < FBE_DRIVE_TYPE_LAST; drive_index++)
    {
        drive_cnt = drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4160][drive_index];
        if(drive_cnt != 0)
        {
            drive_cnt--;
            disk_set_p->bus = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_4160][drive_index][drive_cnt].bus;
            disk_set_p->enclosure = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_4160][drive_index][drive_cnt].enclosure;
            disk_set_p->slot = drive_locations_p->disk_set[FBE_TEST_BLOCK_SIZE_4160][drive_index][drive_cnt].slot;
            drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4160][drive_index]--;
            return FBE_STATUS_OK; 
        }
    }
    return FBE_STATUS_NO_OBJECT;
}
/*!**************************************************************
 * fbe_test_sep_util_wait_for_all_objects_ready()
 ****************************************************************
 * @brief
 *  Wait for all objects that are part of this rg to be ready.
 *
 * @param rg_config_p - config to wait for.               
 *
 * @return None   
 *
 * @author
 *  10/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_wait_for_all_objects_ready(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status;
    fbe_u32_t       width;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t object_id;
    fbe_u32_t       lun_index;
    fbe_u32_t       position;
    fbe_object_id_t vd_object_ids[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* There is no need to wait if the rg_config is not enabled
     */
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {            
        return;
    }

    /* Get the raid group object id and then the first VD object id
     */
    width = rg_config_p->width;
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Wait for the RG.
     * Wait up to 70 seconds for this object to become ready.
     * 20 Mar 12 Dave Agans -- changed 20 to 120 since we are now flushing write-log to disk before ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 120000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_util_get_all_vd_object_ids(rg_object_id, vd_object_ids);  
    /* Wait for all VD objects to be ready before continuing.
     */
    for (position = 0; position < width; position++)
    {
        /* Wait up to 70 seconds for this object to become ready.
         */ 
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_ids[position]);
        status = fbe_api_wait_for_object_lifecycle_state(vd_object_ids[position], FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Wait for all lun objects to be ready before continuing.
     */
    for (lun_index = 0; lun_index < rg_config_p->number_of_luns_per_rg; lun_index++)
    {
        fbe_lun_number_t lun_number =  rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

        /* Wait up to 20 seconds for the lun object to become ready.
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);
        status = fbe_api_database_lookup_lun_by_number(lun_number, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/******************************************
 * end fbe_test_sep_util_wait_for_all_objects_ready()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_wait_for_rg_objects_ready()
 ****************************************************************
 * @brief
 *  Wait for all raid group objects to get ready for a given
 *  configuration array.
 *
 * @param rg_config_p - config array.               
 *
 * @return None.
 *
 * @author
 *  10/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_wait_for_rg_objects_ready(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index;
    fbe_test_rg_configuration_t * current_rg_config_p = NULL;

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_sep_wait_for_rg_objects_ready()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_wait_for_rg_objects_ready_both_sps()
 ****************************************************************
 * @brief
 *  Wait for all rg objects to be ready on both SPs.
 *
 * @param rg_config_p - config array.               
 *
 * @return None.
 *
 * @author
 *  10/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_wait_for_rg_objects_ready_both_sps(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_status_t status;
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_wait_for_rg_objects_ready(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_wait_for_rg_objects_ready(rg_config_p);
    
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/******************************************
 * end fbe_test_sep_wait_for_rg_objects_ready_both_sps()
 ******************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_object_trace_flags
 ***************************************************************************
 * @brief
 *  This function sets the FBE trace level.
 * 
 * @param trace_type - The type of tracing 
 * @param object_id  - object to set trace level of
 * @param package_id - package of object
 * @param level      - Trace level
 *
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_util_set_object_trace_flags(fbe_trace_type_t trace_type, 
                                              fbe_object_id_t object_id, 
                                              fbe_package_id_t package_id,
                                              fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = object_id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, package_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_object_trace_flags()
 **************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_service_trace_flags
 ***************************************************************************
 * @brief
 *  This function sets the FBE trace level.
 * 
 * @param trace_type - The type of tracing 
 * @param service_id  - service to set trace level of
 * @param package_id - package of object
 * @param level      - Trace level
 *
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_util_set_service_trace_flags(fbe_trace_type_t trace_type, 
                                              fbe_service_id_t service_id, 
                                              fbe_package_id_t package_id,
                                              fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = service_id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, package_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_service_trace_flags()
 **************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_rg_trace_level
 ***************************************************************************
 * @brief
 *  This function sets the FBE trace level for all configured rgs.
 * 
 * @param rg_config_p - config array.
 * @param level       - Trace level
 *
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_trace_level(fbe_test_rg_configuration_t *rg_config_p, fbe_trace_level_t level)
{
    fbe_status_t status;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_object_id_t rg_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_test_sep_util_set_object_trace_flags(FBE_TRACE_TYPE_OBJECT, rg_object_id, FBE_PACKAGE_ID_SEP_0, level);
            //fbe_test_sep_util_set_service_trace_flags(FBE_TRACE_TYPE_SERVICE, FBE_SERVICE_ID_METADATA, FBE_PACKAGE_ID_SEP_0, level);
        }
        rg_config_p++;
    }
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_rg_trace_level()
 **************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_rg_trace_level_both_sps
 ***************************************************************************
 * @brief
 *  This function sets the raid group trace level on both sps..
 * 
 * @param rg_config_p - config array.
 * @param trace_level       - Trace level
 *
 * @return void
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_trace_level_both_sps(fbe_test_rg_configuration_t *rg_config_p, fbe_trace_level_t trace_level)
{
    fbe_test_sep_util_set_rg_trace_level(rg_config_p, (fbe_trace_level_t)trace_level);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_set_rg_trace_level(rg_config_p, (fbe_trace_level_t)trace_level);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_rg_trace_level_both_sps()
 **************************************/

/*!**************************************************************************
 * ffbe_test_sep_util_set_raid10_downstream_group_debug_flags()
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the mirrors under a RAID-10
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 ***************************************************************************/
static void fbe_test_sep_util_set_raid10_downstream_group_debug_flags(fbe_test_rg_configuration_t *rg_config_p, 
                                                                      fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_status_t    status;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t       mirror_index;
    fbe_object_id_t rg_object_id;
    fbe_raid_group_debug_flags_t current_debug_flags;

    /* If we are a raid 10 set the debug flags for the mirror raid groups also.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* For a raid 10 the downstream object list has the IDs of the mirrors. 
         * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
         */
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        for (mirror_index = 0; 
             mirror_index < downstream_object_list.number_of_downstream_objects; 
             mirror_index++)
        {
            rg_object_id = downstream_object_list.downstream_object_list[mirror_index];
            status = fbe_api_raid_group_get_group_debug_flags(rg_object_id, &current_debug_flags);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, current_debug_flags|debug_flags);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return;
}
/**********************************************************************
 * end fbe_test_sep_util_set_raid10_downstream_group_debug_flags()
 **********************************************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_rg_debug_flags
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the raid group object.
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 * @author
 *  5/11/2011 - Created. Rob Foley
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_debug_flags(fbe_test_rg_configuration_t *rg_config_p, fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_status_t status;
    fbe_raid_group_debug_flags_t current_debug_flags; 
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_object_id_t rg_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_raid_group_get_group_debug_flags(rg_object_id, &current_debug_flags);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, current_debug_flags|debug_flags);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                fbe_test_sep_util_set_raid10_downstream_group_debug_flags(rg_config_p, debug_flags);
            }
        }
        rg_config_p++;
    }
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_rg_debug_flags()
 **************************************/
/*!**************************************************************************
 * fbe_test_sep_util_set_rg_debug_flags_both_sps
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the raid group object.
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 * @author
 *  6/3/2011 - Created. Rob Foley
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_debug_flags_both_sps(fbe_test_rg_configuration_t *rg_config_p, fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_test_sep_util_set_rg_debug_flags(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_set_rg_debug_flags(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_rg_debug_flags_both_sps()
 **************************************/

/*!**************************************************************************
 * ffbe_test_sep_util_set_raid10_downstream_library_debug_flags()
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the mirrors under a RAID-10
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 ***************************************************************************/
static void fbe_test_sep_util_set_raid10_downstream_library_debug_flags(fbe_test_rg_configuration_t *rg_config_p, 
                                                                        fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_status_t    status;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t       mirror_index;
    fbe_object_id_t rg_object_id;
    fbe_raid_library_debug_flags_t current_debug_flags;

    /* If we are a raid 10 set the debug flags for the mirror raid groups also.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* For a raid 10 the downstream object list has the IDs of the mirrors. 
         * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
         */
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        for (mirror_index = 0; 
             mirror_index < downstream_object_list.number_of_downstream_objects; 
             mirror_index++)
        {
            rg_object_id = downstream_object_list.downstream_object_list[mirror_index];
            status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, &current_debug_flags);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, current_debug_flags|debug_flags);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return;
}
/**********************************************************************
 * end fbe_test_sep_util_set_raid10_downstream_library_debug_flags()
 **********************************************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_rg_library_debug_flags
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the raid group object.
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 * @author
 *  6/29/2011 - Created. Rob Foley
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_library_debug_flags(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_status_t status;
    fbe_raid_library_debug_flags_t current_debug_flags;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_object_id_t rg_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, &current_debug_flags);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, current_debug_flags|debug_flags);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                fbe_test_sep_util_set_raid10_downstream_library_debug_flags(rg_config_p, debug_flags);
            }
        }
        rg_config_p++;
    }
    return;
}
/*****************************************************
 * end  fbe_test_sep_util_set_rg_library_debug_flags()
 *****************************************************/

/*!**************************************************************************
 * fbe_test_sep_util_set_rg_library_debug_flags_both_sps
 ***************************************************************************
 * @brief
 *  This function sets the debug flags for the raid group object.
 * 
 * @param rg_config_p - config array.
 * @param debug_flags - Debug flags to set on all configured rgs.
 *
 * @return void
 *
 * @author
 *  6/29/2011 - Created. Rob Foley
 *
 ***************************************************************************/
void fbe_test_sep_util_set_rg_library_debug_flags_both_sps(fbe_test_rg_configuration_t *rg_config_p, 
                                                           fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_test_sep_util_set_rg_library_debug_flags(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_set_rg_library_debug_flags(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return;
}
/**************************************
 * end  fbe_test_sep_util_set_rg_library_debug_flags_both_sps()
 **************************************/

fbe_status_t fbe_test_sep_util_send_job_service_command(fbe_object_id_t    object_id,
        fbe_job_service_control_t in_job_control)
{
    fbe_job_queue_element_t                  job_queue_element;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t  status_info;

    /* Set the job service request with passed-in values.
    */
    job_queue_element.num_objects = 0;


    /* Disable the queue access.
    */
    if(in_job_control == FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE)
    {
        job_queue_element.queue_access = FALSE;
    }
    else
    {
        job_queue_element.queue_access = TRUE;
    }

    job_queue_element.job_type = FBE_JOB_TYPE_INVALID;
    job_queue_element.object_id = object_id;


    /* Send the job service external command request to job service.
    */
    status = fbe_api_common_send_control_packet_to_service (in_job_control,
            &job_queue_element,
            sizeof(fbe_job_queue_element_t),
            FBE_SERVICE_ID_JOB_SERVICE,
            FBE_PACKET_FLAG_NO_ATTRIB,
            &status_info,
            FBE_PACKAGE_ID_SEP_0);
    return status;
}
/**************************************************
 * end fbe_test_sep_util_send_job_service_command()
 **************************************************/   

/*!****************************************************************************
 * fbe_test_sep_util_lookup_bvd
 ******************************************************************************
 *
 * @brief
 *    This function returns the BVD object ID.
 *
 * @return  bvd_object_id   - BVD object ID
 *
 * @version
 *    03/2010 - Created. Susan Rundbaken (rundbs)
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_lookup_bvd(fbe_object_id_t* bvd_object_id_p)
{
    return fbe_api_bvd_interface_get_bvd_object_id(bvd_object_id_p);
}

/*!****************************************************************************
 * fbe_test_sep_util_wait_for_pvd_np_flags
 ******************************************************************************
 *
 * @brief
 *    This function wait for specific np flag is set in PVD.
 *
 * @param   object_id  -  provision drive object id
 * @param   flag       -  flag bits
 * @param   timeout_ms -  timeout value
 *
 * @return  fbe_status_t   -  status of this operation
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_pvd_np_flags(fbe_object_id_t object_id, fbe_provision_drive_np_flags_t flag, fbe_u32_t timeout_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_metadata_nonpaged_data_t nonpaged_data;
    fbe_provision_drive_nonpaged_metadata_t *pvd_np;
    fbe_u32_t current_time = 0;

    while (current_time < timeout_ms)
    {
        status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        pvd_np = (fbe_provision_drive_nonpaged_metadata_t *) nonpaged_data.data;

        if ((pvd_np->flags & flag) == flag)
        {
            return status;
        }
        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep (FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    return FBE_STATUS_GENERIC_FAILURE;

}

/*!****************************************************************************
 * fbe_test_sep_util_wait_for_scrub_complete
 ******************************************************************************
 *
 * @brief
 *    This function wait for scrub to complete.
 *
 * @param   timeout_ms -  timeout value
 *
 * @return  fbe_status_t   -  status of this operation
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_scrub_complete(fbe_u32_t timeout_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_system_scrub_progress_t  scrub_progress;
    fbe_u32_t current_time = 0;

    while (current_time < timeout_ms)
    {
        status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (scrub_progress.blocks_already_scrubbed == scrub_progress.total_capacity_in_blocks)
        {
            return status;
        }
		mut_printf(MUT_LOG_TEST_STATUS, "%s: Already 0x%llx Total 0x%llx", 
                   __FUNCTION__, scrub_progress.blocks_already_scrubbed, scrub_progress.total_capacity_in_blocks);
        current_time = current_time + 1000;
        fbe_api_sleep (1000); /* We should pool with 1 sec intervals */
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_clear_verify_report
 ******************************************************************************
 *
 * @brief
 *    This function clears the sniff verify report on the specified provision
 *    drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_clear_verify_report( fbe_object_id_t in_provision_drive_object_id )
{
    return fbe_api_provision_drive_clear_verify_report( in_provision_drive_object_id, FBE_PACKET_FLAG_NO_ATTRIB );
}


/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_disable_verify
 ******************************************************************************
 *
 * @brief
 *    This function disables sniff verifies on the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *    11/11/2010 - Modified. Manjit Singh - Redefined the function to use fbe_api 
 *                                          to disable sniff verify.
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_disable_verify( fbe_object_id_t in_provision_drive_object_id )
{
    fbe_api_job_service_update_pvd_sniff_verify_status_t        update_pvd_sniff_verify_state;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_job_service_error_type_t                                js_error_type;

    /* initialize error type as no error. */
    js_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    update_pvd_sniff_verify_state.pvd_object_id = in_provision_drive_object_id;
    update_pvd_sniff_verify_state.sniff_verify_state = FBE_FALSE;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_pvd_sniff_verify(&update_pvd_sniff_verify_state);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_sniff_verify_state.job_number,
                                         FBE_TEST_UTILS_WAIT_TIMEOUT_MS,
                                         &js_error_type,
                                         &job_status,
                                         NULL);
    /* verify the job status. */
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out enabling sniff verify");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Configuring PVD to enable sniff verify failed");
    return status;
}


/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_enable_verify
 ******************************************************************************
 *
 * @brief
 *    This function enables sniff verifies on the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *    11/11/2010 - Modified. Manjit Singh - Redefined the function to use fbe_api 
 *                                          to enable sniff verify.
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_enable_verify( fbe_object_id_t in_provision_drive_object_id )
{
    fbe_api_job_service_update_pvd_sniff_verify_status_t        update_pvd_sniff_verify_state;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_job_service_error_type_t                                js_error_type;

    /* initialize error type as no error. */
    js_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    update_pvd_sniff_verify_state.pvd_object_id = in_provision_drive_object_id;
    update_pvd_sniff_verify_state.sniff_verify_state = FBE_TRUE;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_pvd_sniff_verify(&update_pvd_sniff_verify_state);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_sniff_verify_state.job_number,
                                         FBE_TEST_UTILS_WAIT_TIMEOUT_MS,
                                         &js_error_type,
                                         &job_status,
                                         NULL);
    /* verify the job status. */
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out enabling sniff verify");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Configuring PVD to enable sniff verify failed");
    return status;
}


/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_get_verify_report
 ******************************************************************************
 *
 * @brief
 *    This function gets the sniff verify report for the specified provision
 *    drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   out_verify_report_p           -  verify report output parameter
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_get_verify_report(
                                                     fbe_object_id_t in_provision_drive_object_id,
                                                     fbe_provision_drive_verify_report_t* out_verify_report_p
                                                   )
{
    return fbe_api_provision_drive_get_verify_report( in_provision_drive_object_id,
                                                      FBE_PACKET_FLAG_NO_ATTRIB,
                                                      out_verify_report_p
                                                    );
}

/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_get_verify_status
 ******************************************************************************
 *
 * @brief
 *    This function gets the sniff verify status for the specified provision
 *    drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   out_verify_status_p           -  verify status output parameter
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_get_verify_status(
                                                     fbe_object_id_t in_provision_drive_object_id,
                                                     fbe_provision_drive_get_verify_status_t* out_verify_status_p
                                                   )
{
    return fbe_api_provision_drive_get_verify_status( in_provision_drive_object_id,
                                                      FBE_PACKET_FLAG_NO_ATTRIB,
                                                      out_verify_status_p
                                                    );
}


/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_set_verify_checkpoint
 ******************************************************************************
 *
 * @brief
 *    This function sets a new sniff verify checkpoint for the specified
 *    provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 * @param   in_checkpoint                 -  new sniff verify checkpoint
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_set_verify_checkpoint(
                                                         fbe_object_id_t in_provision_drive_object_id,
                                                         fbe_lba_t       in_checkpoint
                                                       )
{
    return fbe_api_provision_drive_set_verify_checkpoint( in_provision_drive_object_id,
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          in_checkpoint
                                                        );
}

/*!**************************************************************
 * fbe_test_set_default_trace_level()
 ****************************************************************
 * @brief
 *  Set the default trace level
 *
 * @param level - new default trace level to set.
 *
 * @return - None.
 *
 ****************************************************************/

void fbe_test_sep_util_set_default_trace_level(fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = FBE_TRACE_TYPE_DEFAULT;
    level_control.fbe_id = FBE_OBJECT_ID_INVALID;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_test_sep_util_set_default_trace_level()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_set_trace_level()
 ****************************************************************
 * @brief
 *  Set the default trace level
 *
 * @param level - new default trace level to set.
 * @param package_id - The package to set for.
 *
 * @return - None.
 *
 ****************************************************************/
void fbe_test_sep_util_set_trace_level(fbe_trace_level_t level,
                                       fbe_package_id_t package_id)
{
    fbe_u32_t value;
    fbe_trace_level_t new_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    /* We will take the higher of either the passed in value or the 
     * value set in the environment. 
     */
    if (fbe_test_sep_util_get_cmd_option_int("-fbe_default_trace_level", &value))
    {
        new_level = value;
        if (new_level > level)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "using trace level of: %d for package_id: %d", new_level, package_id);
            level = new_level;
        }
    }
    level_control.trace_type = FBE_TRACE_TYPE_DEFAULT;
    level_control.fbe_id = FBE_OBJECT_ID_INVALID;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, package_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_test_sep_util_set_trace_level()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_set_rba_flags()
 ****************************************************************
 * @brief
 *  Set the default rba trace level
 *
 * @param level - new default trace level to set.
 * @param package_id - The package to set for.
 *
 * @return - None.
 *
 ****************************************************************/
void fbe_test_sep_util_set_rba_flags(void)
{
    fbe_status_t status;
    char *value = mut_get_user_option_value("-traffic_trace_flags");

    if (value != NULL){
        fbe_bool_t b_all = FBE_FALSE;
        fbe_bool_t b_all_rg = FBE_FALSE;

        if (!strcmp(value, "all")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable all rba flags\n");
            b_all = FBE_TRUE;
        }
        if (!strcmp(value, "all_rg")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable all RAID group rba flags\n");
            b_all_rg = FBE_TRUE;
        }
        if (b_all || !strcmp(value, "fbe_lun")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable lun rba flags\n");
            status = fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_LUN, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        if (b_all || b_all_rg || !strcmp(value, "fbe_rg")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable rg rba flags\n");
            status = fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_RG, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        if (b_all || b_all_rg || !strcmp(value, "fbe_rg_fru")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable rg_fru rba flags\n");
            status = fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_RG_FRU, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        if (b_all || !strcmp(value, "vd")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable vd rba flags\n");
            status = fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_VD, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        if (b_all || !strcmp(value, "pvd")){
            mut_printf(MUT_LOG_TEST_STATUS, "enable pvd rba flags\n");
            status = fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_PVD, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    return;
}
/******************************************
 * end fbe_test_sep_util_set_rba_flags()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_reduce_sector_trace_level()
 ****************************************************************
 * @brief
 *  Set the default sector trace level as to only report critical
 *  errors
 *
 * @param   None
 *
 * @return  None.
 *
 ****************************************************************/
void fbe_test_sep_util_reduce_sector_trace_level(void)
{
    fbe_status_t   status;
     
    status = fbe_api_sector_trace_set_trace_level(FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL,FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_restore_sector_trace_level()
 ****************************************************************
 * @brief
 *  Restore the default sector trace level to the default value
 *
 * @param   None
 *
 * @return  None.
 *
 ****************************************************************/
void fbe_test_sep_util_restore_sector_trace_level(void)
{
    fbe_status_t   status;
     
    status = fbe_api_sector_trace_restore_default_level();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_wait_rg_pos_rebuild()
 ****************************************************************
 * @brief
 *  Wait for a raid group rebuild to complete.
 *
 * @param rg_object_id - Object id to wait on
 * @param rebuild_position - position in the raid group to check.
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  3/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_rg_pos_rebuild(fbe_object_id_t rg_object_id,
                                                   fbe_u32_t rebuild_position,
                                                   fbe_u32_t wait_seconds)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;

    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define BIG_BIRD_DISPLAY_REBUILD_MSEC 2000

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (rg_info.rebuild_checkpoint[rebuild_position] == FBE_LBA_INVALID)
        {
            /* Only display complete if we waited below.
             */
            if (elapsed_msec > 0)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "=== %s rebuild complete rg: 0x%x pos: %d", 
                           __FUNCTION__, rg_object_id, rebuild_position);
            }
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % BIG_BIRD_DISPLAY_REBUILD_MSEC) == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for rebuild rg: 0x%x pos: %d chkpt: 0x%llx", 
                       __FUNCTION__, rg_object_id, rebuild_position,
		       (unsigned long long)rg_info.rebuild_checkpoint[rebuild_position]);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000))
    {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_rg_pos_rebuild()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_raid_group_to_rebuild
 *****************************************************************************
 *
 * @brief   This function waits (blocking) for a raid group to
 *          completely rebuild.
 * 
 * @param   raid_group_object_id   - object id of raid group object
 *
 * @return status - Raid group must be ready state
 *
 ***************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_raid_group_to_rebuild(fbe_object_id_t raid_group_object_id)
{
    fbe_status_t    status;
    fbe_u32_t       position;
    fbe_api_raid_group_get_info_t       raid_group_info;    //  raid group information from "get info" command
    fbe_lifecycle_state_t   current_state;
    
    /* Validate raid group is ready and that position is valid.
     */
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_lifecycle_state(raid_group_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, current_state);

    /* For all raid group positions
     */
    for (position = 0; position < raid_group_info.width; position++)
    {
        status = fbe_test_sep_util_wait_rg_pos_rebuild(raid_group_object_id, position, 
                                                       FBE_TEST_SEP_UTIL_RB_WAIT_SEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Always return the execution status
     */   
    return(status);

}   // End fbe_test_sep_util_wait_for_raid_group_to_rebuild()

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_raid_group_to_start_rebuild
 *****************************************************************************
 *
 * @brief   This function waits (blocking) for a raid group to start to rebuild.
 * 
 * @param   raid_group_object_id   - object id of raid group object
 *
 * @return status - Raid group must be ready state
 *
 ***************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(fbe_object_id_t raid_group_object_id)
{
    fbe_status_t    status;
    fbe_u32_t       total_milliseconds_to_wait = 10000;
    fbe_u32_t       sleep_period_milliseconds = 100;
    fbe_u32_t       sleep_index = 0;
    fbe_u32_t       sleep_count = (total_milliseconds_to_wait / sleep_period_milliseconds);
    fbe_api_raid_group_get_info_t       raid_group_info;    //  raid group information from "get info" command
 
    /* Wait for the RG.
     * Wait up to 20 seconds for this object to become ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
  
    /* Fetch the raid group information
     */
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     

    /* For all raid group positions check
     */
    status = FBE_STATUS_TIMEOUT;
    while ((sleep_index < sleep_count) &&
           (status != FBE_STATUS_OK)      )
    {
        fbe_u32_t       position;
        
        /* For all position check if rebuild started.
         */
        for (position = 0; position < raid_group_info.width; position++)
        {
            if (raid_group_info.rebuild_checkpoint[position] != FBE_LBA_INVALID)
            {
                status = FBE_STATUS_OK;
                break;
            }
        }

        //  Wait before trying again 
        if (status != FBE_STATUS_OK)
        {
            fbe_status_t    get_info_status;
            
            EmcutilSleep(sleep_period_milliseconds);
        
            //  Get the raid group information
            get_info_status = fbe_api_raid_group_get_info(raid_group_object_id, 
                                                          &raid_group_info, 
                                                          FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, get_info_status);
            mut_printf(MUT_LOG_LOW, "RG %d Pos %d: Rebuild checkpoint is 0x%llx!\n",
                       raid_group_object_id,
                       position,
                       (unsigned long long)raid_group_info.rebuild_checkpoint[position]);
        }

        sleep_index++;
    }
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Rebuild is not started after 10 seconds!!!\n");
    /* Always return the execution status
     */   
    return(status);

}   // End fbe_test_sep_util_wait_for_raid_group_to_start_rebuild()

/*!***************************************************************************
 *          fbe_test_util_wait_for_encryption_state
 *****************************************************************************
 *
 * @brief   This function waits (blocking) for a raid group the encryption
 *  state to be the expected state .
 * 
 * @param   raid_group_object_id   - object id of raid group object
 * @param   expected_state         - Expected encryption state
 * @param   total_timeout          - Max timeout in ms
 *
 * @return status
 *
 ***************************************************************************/
fbe_status_t fbe_test_util_wait_for_encryption_state(fbe_object_id_t object_id, 
                                                     fbe_base_config_encryption_state_t expected_state,
                                                     fbe_u32_t total_timeout_in_ms)
{
    fbe_u32_t timeout = 0;
    fbe_base_config_encryption_state_t current_state;
    fbe_status_t    status;

    while(timeout < total_timeout_in_ms)
    {
        /* Wait for the encryption state of the object */
        status = fbe_api_base_config_get_encryption_state(object_id, &current_state);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(current_state == expected_state)
        {
            break;
        }
        fbe_api_sleep(100);
        timeout += 100;
    }

    if(current_state != expected_state)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Current State %d but Expected State is %d\n", 
                   current_state, expected_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        return FBE_STATUS_OK;
    }
}

/*!***************************************************************************
 *          fbe_test_util_wait_for_rg_object_id_by_number
 *****************************************************************************
 *
 * @brief   This function get the rg object ID by number 
 *  
 * 
 * @param   rg_number        - RG number we want the RG object ID
 * @param   object_id        - Object ID
 * @param   total_timeout     Max timeout in ms
 *
 * @return status
 *
 ***************************************************************************/
fbe_status_t fbe_test_util_wait_for_rg_object_id_by_number(fbe_raid_group_number_t rg_number, 
                                                           fbe_object_id_t *object_id, 
                                                           fbe_u32_t total_timeout_in_ms)
{
    fbe_u32_t timeout = 0;
    fbe_status_t status;

    while(timeout < total_timeout_in_ms)
    {
        /* First get the Object ID for the Raid Group */
        status = fbe_api_database_lookup_raid_group_by_number(rg_number, object_id);
        if(*object_id != FBE_OBJECT_ID_INVALID)
        {
            break;
        }
        fbe_api_sleep(100);
        timeout += 100;
    }

    return status;
}

/*!**************************************************************
 *          fbe_test_sep_util_get_cmd_option_int()
 ****************************************************************
 * @brief
 *  Return an option value that is an integer.
 *
 * @param option_name_p - name of option to get
 * @param value_p - pointer to value to return.
 *
 * @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p)
{
    char *value = mut_get_user_option_value(option_name_p);

    if (value != NULL)
    {
        *value_p = strtoul(value, 0, 0);
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

/*!**************************************************************
 *          fbe_test_sep_util_get_cmd_option()
 ****************************************************************
 * @brief
 *  Determine if an option is set.
 *
 * @param option_name_p - name of option to get
 *
 * @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_get_cmd_option(char *option_name_p)
{
    if (mut_option_selected(option_name_p))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_sep_test_util_get_extended_testing_level()
 ****************************************************************
 * @brief
 *  Return if we should execute additional extended testing.
 *
 * @param none       
 *
 * @return - fbe_u32_t - The level of extended testing we should
 *                       perform.  Typically 0 means a qual test,
 *                       and 1 and above is extended testing
 *                       as defined by the test.
 *
 ****************************************************************/

fbe_u32_t fbe_sep_test_util_get_extended_testing_level(void)
{
    fbe_u32_t value = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-extended_test_level", &value))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== Extended Testing level is %d ==", value);
        return value;
    }
    /* No extended testing is defined.  Just use the default test level.
     */
    return fbe_test_get_default_extended_test_level();
}

/*!**************************************************************
 * fbe_sep_test_util_get_raid_testing_extended_level()
 ****************************************************************
 * @brief
 *  Return if we should execute additional extended testing.
 *
 * @param none       
 *
 * @return - fbe_u32_t - The level of extended testing we should
 *                       perform.  Typically 0 means a qual test,
 *                       and 1 and above is extended testing
 *                       as defined by the test.
 *
 ****************************************************************/

fbe_u32_t fbe_sep_test_util_get_raid_testing_extended_level(void)
{
    fbe_u32_t value = 0;
    static fbe_bool_t b_traced = FBE_FALSE;

    if (fbe_test_sep_util_get_cmd_option_int("-raid_test_level", &value) ||
        fbe_test_sep_util_get_cmd_option_int("-rtl", &value))
    {
        /* Nothing to do.  Just display the value received below.
         */
    }
    else
    {
        value = fbe_test_get_default_extended_test_level();
    }
    if ((value != 0) && !b_traced)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== Raid Testing Extended level is %d ==", value);
       b_traced = FBE_TRUE;
    }
    /* No extended testing is defined, so return the default.
     */
    return value;
}
/*!**************************************************************
 * fbe_sep_test_util_raid_type_enabled()
 ****************************************************************
 * @brief
 *  Determine if we should test this raid type.
 * 
 *  The user will use these switches to choose:
 *    -raid0_only - Only test raid 0.
 *    -raid5_only - Only test raid 5.
 *    -raid3_only - Only test raid 3.
 *    -raid6_only - Only test raid 6.
 *    -raid1_only - Only test raid 1.
 *    -raid10_only - Only test raid 10.
 *    -parity_only - Only test raid 5, 3, and 6.
 *    -mirror_only - Only test raid 1 and 10.
 *
 * @param raid_type - The raid type to validate.       
 *
 * @return - fbe_bool_t - FBE_TRUE to test this raid type
 *                       FBE_FALSE otherwise.
 *
 ****************************************************************/

fbe_bool_t fbe_sep_test_util_raid_type_enabled(fbe_raid_group_type_t raid_type)
{
    fbe_bool_t b_option_found = FBE_FALSE;

    if (mut_option_selected("-raid0_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-parity_only"))
    {
        b_option_found = FBE_TRUE;
        if ((raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID6))
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-mirror_only"))
    {
        b_option_found = FBE_TRUE;
        if ((raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID10))
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-raid5_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID5)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-raid3_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID3)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-raid6_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-raid10_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("-raid1_only"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid0"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            return FBE_TRUE;
        }   
    }
        
    if (mut_option_selected("parity"))
    {
        b_option_found = FBE_TRUE;
        if ((raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID6))
        {
            return FBE_TRUE;
        }   
    }
    if (mut_option_selected("mirror"))
    {
        b_option_found = FBE_TRUE;
        if ((raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
            (raid_type == FBE_RAID_GROUP_TYPE_RAID10))
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid1"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid10"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid5"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID5)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid3"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID3)
        {
            return FBE_TRUE;
        }
    }
    if (mut_option_selected("raid6"))
    {
        b_option_found = FBE_TRUE;
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            return FBE_TRUE;
        }
    }
    /* If an option was found and we are here, then we did not 
     * get a match on this. 
     */
    if (b_option_found)
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_sep_test_util_raid_type_enabled()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_set_default_io_flags()
 ****************************************************************
 * @brief
 *  Setup a set of default flags that we want set for all
 *  I/O type tests.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void fbe_test_sep_util_set_default_io_flags(void)
{
    fbe_test_sep_util_set_raid_library_default_debug_flags(0    /* no default */);

    /* Set the default trace level to info.  This also checks for the 
     * default trace level on the command line. 
     */
    fbe_test_sep_util_set_trace_level(FBE_TRACE_LEVEL_INFO, FBE_PACKAGE_ID_SEP_0);

    fbe_test_sep_util_set_virtual_drive_class_debug_flags(0 /* no default */);

    fbe_test_sep_util_set_pvd_class_debug_flags(0 /* no default */);

    fbe_test_sep_util_set_terminator_drive_debug_flags(0 /* no default */);
    fbe_test_sep_util_set_lifecycle_debug_trace_flag(0 /* no default */);
    fbe_test_sep_util_set_rba_flags();
    return;
}
/******************************************
 * end fbe_test_sep_util_set_default_io_flags()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_set_default_terminator_and_rba_flags()
 ****************************************************************
 * @brief
 *  Setup a set of default flags that we want set for all
 *  I/O type tests, which are not covered by load_param.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void fbe_test_sep_util_set_default_terminator_and_rba_flags(void)
{
    fbe_test_sep_util_set_terminator_drive_debug_flags(0 /* no default */);
    fbe_test_sep_util_set_rba_flags();
    return;
}
/******************************************
 * end fbe_test_sep_util_set_default_terminator_and_rba_flags()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_get_drive_remove_delay()
 *****************************************************************************
 *
 * @brief   This method will return either the passed (i.e. in_drive_removal_delay)
 *          or the system value of `fbe_drive_remove_delay' (in milliseconds).
 * 
 * @param   in_drive_remove_delay - If not FBE_U32_MAX, then this is the starting
 *                                  value.
 *                                  Else default_remove_delay is the starting value.
 *                      
 * @return  drive remove delay - Delay in milliseconds between drive removals
 *
 ***************************************************************************/
fbe_u32_t fbe_test_sep_util_get_drive_remove_delay(fbe_u32_t in_drive_remove_delay)
{
    fbe_u32_t   default_remove_delay = 100;

    /* Determine starting value (i.e. if not overridden by system value)
     */

    if (in_drive_remove_delay != FBE_U32_MAX)
    {
        default_remove_delay = in_drive_remove_delay;
    }
    
    /* If the system override is value is present, that is the value used.
     */
    if (fbe_test_sep_util_get_cmd_option_int("drive_remove_delay", &default_remove_delay))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using drive remove delay of: %d milliseconds \n", default_remove_delay);
    }
    return default_remove_delay;
} /* end fbe_test_sep_util_get_drive_remove_delay */

/*!***************************************************************************
 *          fbe_test_sep_util_get_drive_insert_delay()
 *****************************************************************************
 *
 * @brief   This method will return either the passed (i.e. in_drive_insert_delay)
 *          or the system value of `fbe_drive_insert_delay' (in milliseconds).
 * 
 * @param   in_drive_insert_delay - If not FBE_U32_MAX, then this is the starting
 *                                  value.
 *                                  Else default_remove_delay is the starting value.
 *                      
 * @return  drive insert delay - Delay in milliseconds between drive insertions
 *
 ***************************************************************************/
fbe_u32_t fbe_test_sep_util_get_drive_insert_delay(fbe_u32_t in_drive_insert_delay)
{
    fbe_u32_t   default_insert_delay = 100;

    /* Determine starting value (i.e. if not overridden by system value)
     */

    if (in_drive_insert_delay != FBE_U32_MAX)
    {
        default_insert_delay = in_drive_insert_delay;
    }
    
    /* If the system override is value is present, that is the value used.
     */
    if (fbe_test_sep_util_get_cmd_option_int("drive_insert_delay", &default_insert_delay))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using drive insert delay of: %d milliseconds \n", default_insert_delay);
    }
    return default_insert_delay;
} /* end fbe_test_sep_util_get_drive_insert_delay */

/*!***************************************************************************
 *          fbe_test_sep_util_get_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   This method will return either the passed (i.e. in_debug_flags)
 *          or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -raid_library_default_debug_flags is set, we use that value
 *                otherwise we use the passed in value.
 *                      
 * @return  debug flags - Debug flags that should be enabled
 *
 ***************************************************************************/
fbe_u32_t fbe_test_sep_util_get_raid_library_debug_flags(fbe_u32_t in_debug_flags)
{
    fbe_u32_t   default_debug_flags = 0;

    /* Determine starting value (i.e. if not overridden by system value)
     */

    if (in_debug_flags != 0)
    {
        default_debug_flags = in_debug_flags;
    }
    
    /* If the system override is value is present, that is the value used.
     */
    if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &default_debug_flags))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using raid library debug flags of: 0x%x", default_debug_flags);
    }
    return default_debug_flags;
} /* end fbe_test_sep_util_get_raid_library_debug_flags */


/*!***************************************************************************
 * fbe_test_sep_util_set_raid_library_default_debug_flags
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -raid_library_default_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_raid_library_default_debug_flags(fbe_raid_library_debug_flags_t flags)
{
    fbe_raid_library_debug_flags_t raid_library_debug_flags;
    fbe_u32_t cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-raid_library_default_debug_flags", &cmd_line_flags))
    {
        raid_library_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using raid library default debug flags of: 0x%x", raid_library_debug_flags);
    }
    else
    {
        raid_library_debug_flags = flags;
    }

    /* Set the default raid library debug flag.
     */
    fbe_api_raid_group_set_library_debug_flags(FBE_OBJECT_ID_INVALID, raid_library_debug_flags);
    return;
}
/**************************************************************
 * end fbe_test_sep_util_set_raid_library_default_debug_flags()
 **************************************************************/

/*!***************************************************************************
 * fbe_test_sep_util_set_raid_library_class_debug_flags
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param raid_group_class_id - The raid group class id to set the raid group
 *          debug flags for.
 * @param flags - if -raid_group_library_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_raid_library_class_debug_flags(fbe_class_id_t raid_group_class_id,
                                                          fbe_raid_library_debug_flags_t flags)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_library_debug_flags_t raid_library_debug_flags;
    fbe_u32_t       cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &cmd_line_flags))
    {
        raid_library_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS,
		   "class id: %d raid library debug flags of: 0x%x",
		   raid_group_class_id, raid_library_debug_flags);
    }
    else
    {
        raid_library_debug_flags = flags;
    }

    /* Set the default raid library debug flag.
     */
    fbe_api_raid_group_set_class_library_debug_flags(raid_group_class_id, raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/**********************************************************
 * end fbe_test_sep_util_set_raid_library_class_debug_flags()
 **********************************************************/

/*!***************************************************************************
 * fbe_test_sep_util_set_raid_group_class_debug_flags
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param raid_group_class_id - The raid group class id to set the raid group
 *          debug flags for.
 * @param flags - if -raid_group_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_raid_group_class_debug_flags(fbe_class_id_t raid_group_class_id,
                                                        fbe_raid_group_debug_flags_t flags)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_debug_flags_t raid_group_debug_flags;
    fbe_u32_t       cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &cmd_line_flags))
    {
        raid_group_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS,
		   "class id: %d raid group debug flags of: 0x%x",
		   raid_group_class_id, raid_group_debug_flags);
    }
    else
    {
        raid_group_debug_flags = flags;
    }

    /* Set the default raid group debug flag.
     */
    fbe_api_raid_group_set_class_group_debug_flags(raid_group_class_id, raid_group_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/**********************************************************
 * end fbe_test_sep_util_set_raid_group_class_debug_flags()
 **********************************************************/

/*!***************************************************************************
 * fbe_test_sep_util_set_virtual_drive_class_debug_flags
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -virtual_drive_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_virtual_drive_class_debug_flags(fbe_virtual_drive_debug_flags_t flags)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_virtual_drive_debug_flags_t virtual_drive_debug_flags;
    fbe_u32_t       cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-virtual_drive_debug_flags", &cmd_line_flags))
    {
        virtual_drive_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using virtual drive debug flags of: 0x%x", virtual_drive_debug_flags);
    }
    else
    {
        virtual_drive_debug_flags = flags;
    }

    /* Set the default raid library debug flag.
     */
    status = fbe_api_virtual_drive_set_debug_flags(FBE_OBJECT_ID_INVALID, virtual_drive_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note If the virtual drive debug flags are non-zero then we want to
     *        set any raid group or raid library flags also.
     */
    if (virtual_drive_debug_flags != 0)
    {
        fbe_test_sep_util_set_raid_library_class_debug_flags(FBE_CLASS_ID_VIRTUAL_DRIVE, 0 /* No default*/);
        fbe_test_sep_util_set_raid_group_class_debug_flags(FBE_CLASS_ID_VIRTUAL_DRIVE, 0 /* No default*/);
    }
    return;
}
/*************************************************************
 * end fbe_test_sep_util_set_virtual_drive_class_debug_flags()
 *************************************************************/

/*!***************************************************************************
 * fbe_test_sep_util_set_pvd_class_debug_flags
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -pvd_class_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_pvd_class_debug_flags(fbe_provision_drive_debug_flags_t flags)
{
    fbe_provision_drive_debug_flags_t pvd_debug_flags;
    fbe_u32_t cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-pvd_class_debug_flags", &cmd_line_flags))
    {
        pvd_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using pvd class debug flags of: 0x%x", pvd_debug_flags);
    }
    else
    {
        pvd_debug_flags = flags;
    }

    /* Set the default PVD debug flag. */
    fbe_test_sep_util_provision_drive_set_class_debug_flag(pvd_debug_flags);
    return;
}
/***************************************************
 * end fbe_test_sep_util_set_pvd_class_debug_flags()
 ***************************************************/

/*!***************************************************************************
 * fbe_test_sep_util_set_terminator_drive_debug_flags()
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -term_drive_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_terminator_drive_debug_flags(fbe_terminator_drive_debug_flags_t in_terminator_drive_debug_flags)
{
    char * CSX_MAYBE_UNUSED drive_range_string_p = mut_get_user_option_value("-term_drive_debug_range");
    fbe_terminator_drive_select_type_t drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_INVALID;
    fbe_u32_t first_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t last_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t backend_bus_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t encl_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t slot_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags = 0;
    fbe_u32_t cmd_line_flags = 0;

    if (fbe_test_sep_util_get_cmd_option_int("-term_drive_debug_flags", &cmd_line_flags))
    {
        terminator_drive_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using term drive debug flags of: 0x%x", terminator_drive_debug_flags);

        /*! @todo Need to handle the following strings:
         *          o "<first>-<last>"
         *          o "b/e/s"
         *
         *  @todo Currently if either `all drives' or `single index' is not
         *        specified, we will default to `all drives'.
         *       
         */
        if (fbe_test_sep_util_get_cmd_option_int("-term_drive_debug_range", &cmd_line_flags))
        {
            first_term_drive_index = cmd_line_flags;
            mut_printf(MUT_LOG_TEST_STATUS, "using term drive index range of: 0x%x", first_term_drive_index);
        }
        else
        {
            /*! @todo Currently we default to `all drives'
             */
            first_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES;
        }

        /*! @todo More work to do to support range.  For now either single terminator
         *        array index or `all drives'.
         */
        if (first_term_drive_index != FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)
        {
            drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_INDEX;
        }
        else
        {
            drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_ALL_DRIVES;
        }

        /* Now invoke the api to set the flags.  Currently we don't check status.
         */
        fbe_api_terminator_set_simulated_drive_debug_flags(drive_select_type,
                                                           first_term_drive_index,
                                                           last_term_drive_index,
                                                           backend_bus_number,
                                                           encl_number,
                                                           slot_number,
                                                           terminator_drive_debug_flags);

    } /* end if the `set terminator drive debug flags' option is set*/

    return;
}
/*************************************************************
 * end of fbe_test_sep_util_set_terminator_drive_debug_flags()
 *************************************************************/

/*!**************************************************************
 * fbe_test_sep_util_fail_drive()
 ****************************************************************
 * @brief
 *  This function will fail the drive specified from fbe_hw_test
 * 
 * @param None
 *
 * @return None
 *
 * @author
 *  8/1/2011    - Created. Deanna Heng
 * 
 ****************************************************************/
void fbe_test_sep_util_fail_drive()
{
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   bed[3];
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
  
    fbe_protocol_error_injection_error_record_t record_p;
    fbe_protocol_error_injection_record_handle_t handle_p;
    fbe_protocol_error_injection_scsi_error_t protocol_error_p;
    char                        *value = mut_get_user_option_value("-fail_drive");

    if (value !=NULL) {
        /*temp_p = strok(value, "_");
        while (temp_p != NULL) {
            bed[index] = atoi(temp_p);
            temp_p = strok(NULL, "_");
            index++;
        }
        MUT_ASSERT_INT_EQUAL_MSG(3, index, "Invalid drive location"); */
            
        bed[0] = atoi(&value[0]);
        bed[1] = atoi(&value[2]);
        bed[2] = atoi(&value[4]);

        /* get the object id to inject errors on*/
        status = fbe_api_get_physical_drive_object_id_by_location(bed[0], 
                                                                      bed[1], 
                                                                      bed[2], 
                                                                      &object_id);

        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to get drive object id!");
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Fail drive: %d_%d_%d. Object id: %d", __FUNCTION__, bed[0], bed[1], bed[2], object_id);

        status = fbe_api_set_object_to_state(object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable fail drive!");

            
        /* Initialize the error injection record.
         */
        fbe_zero_memory(&record_p, sizeof(fbe_protocol_error_injection_error_record_t));
        record_p.object_id = object_id;
        record_p.lba_start  = 0x0;
        record_p.lba_end    = FBE_RAID_DEFAULT_CHUNK_SIZE - 1;

        /* set the scsi error information */
        protocol_error_p.scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
        protocol_error_p.scsi_additional_sense_code = FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
        protocol_error_p.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
        protocol_error_p.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
        protocol_error_p.scsi_command[0] = FBE_SCSI_READ_16;

        record_p.protocol_error_injection_error.protocol_error_injection_scsi_error = protocol_error_p;
        /* make sure the number of times to insert is large so 
           that the drive stays failed */
        record_p.num_of_times_to_insert = FBE_U32_MAX;
        record_p.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;

        /*set the error using NEIT package. */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: starting error injection SK:%d, ASC:%d.", 
               __FUNCTION__, 
               record_p.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key, 
               record_p.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code);

        /* add the error record and start injection*/
        status = fbe_api_protocol_error_injection_add_record(&record_p, &handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        status = fbe_api_protocol_error_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}
/**************************************
 * end fbe_test_sep_util_fail_drive()
 **************************************/

/*!**************************************************************
 * fbe_test_get_rg_count()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups we will be working with.
 *  Only count entries that are enabled for this pass.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
fbe_u32_t fbe_test_get_rg_count(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t num_rgs = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (rg_config_p->b_create_this_pass)
        {
            num_rgs++;
        }
        rg_config_p++;
    }
    return num_rgs;
}
/**************************************
 * end fbe_test_get_rg_count()
 **************************************/

/*!**************************************************************
 * fbe_test_get_rg_tested_count()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups we already tested.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
static fbe_u32_t fbe_test_get_rg_tested_count(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t num_rgs = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (rg_config_p->b_already_tested)
        {
            num_rgs++;
        }
        rg_config_p++;
    }
    return num_rgs;
}
/**************************************
 * end fbe_test_get_rg_tested_count()
 **************************************/
/*!**************************************************************
 * fbe_test_get_rg_array_length()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups in the array.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
fbe_u32_t fbe_test_get_rg_array_length(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t num_rgs = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        num_rgs++;
        rg_config_p++;
    }
    return num_rgs;
}
/**************************************
 * end fbe_test_get_rg_array_length()
 **************************************/

/*!**************************************************************
 * fbe_test_rg_config_mark_invalid()
 ****************************************************************
 * @brief
 *  Initialize the fields not populated by the Test.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return none.  
 *
 ****************************************************************/
void fbe_test_rg_config_mark_invalid(fbe_test_rg_configuration_t *rg_config_p)
{
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        rg_config_p->magic = 0;
        rg_config_p++;
    }
}
/**************************************
 * end fbe_test_init_rg_config()
 **************************************/


/*!**************************************************************
 * fbe_test_set_specific_drives_to_remove()
 ****************************************************************
 * @brief
 *  This function will populate the rg config table with the
 *  specific set of drives to be removed for the test.
 *  Currently used only by the stretch test
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
void fbe_test_set_specific_drives_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t *drives_to_remove_p)
{
    fbe_u32_t index = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (*drives_to_remove_p != FBE_U32_MAX)
    {        
        rg_config_p->specific_drives_to_remove[index] = *drives_to_remove_p;
        drives_to_remove_p++;
        index++;
    }
    return;
}
/**************************************
 * end fbe_test_set_specific_drives_to_remove()
 **************************************/


void fbe_test_sep_util_enable_pkg_trace_limits(fbe_package_id_t package_id)
{
    fbe_status_t status;

    /* Setup a limit for critical errors.  We stop on the first one.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                           1,    /* Num errors. */
                                           package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Setup a limit for errors.  We stop on the first one.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                           1,    /* Num errors. */
                                           package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}


void fbe_test_sep_util_disable_pkg_trace_limits(fbe_package_id_t package_id)
{
    fbe_status_t status;

    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                           0,    /* Num errors. */
                                           package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable a limit for errors.  Set action to invalid.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                           0,    /* Num errors. */
                                           package_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
void fbe_test_sep_util_enable_trace_limits(fbe_bool_t sep_only)
{
    fbe_u32_t debug_flags;
    if (fbe_test_sep_util_get_cmd_option("-stop_sep_neit_on_error"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-stop_sep_neit_on_error set.  Enable error limits. ");

        fbe_test_sep_util_enable_pkg_trace_limits(FBE_PACKAGE_ID_SEP_0);
        if (!sep_only) 
        {
            fbe_test_sep_util_enable_pkg_trace_limits(FBE_PACKAGE_ID_NEIT);
        }
    }
    if (fbe_test_sep_util_get_cmd_option("-stop_sector_trace_on_error"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-stop_sector_trace_on_error set. ");

        fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);
    }
    if (fbe_test_sep_util_get_error_injection_test_mode()==FBE_FALSE)
    {
        if (fbe_test_sep_util_get_cmd_option_int("-sector_trace_debug_flags", &debug_flags))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "-sector_trace_debug_flags set 0x%x ", debug_flags);
            fbe_test_sep_util_set_sector_trace_debug_flags((fbe_sector_trace_error_flags_t)debug_flags);
        }
        else
        {
            fbe_test_sep_util_set_sector_trace_debug_flags((fbe_sector_trace_error_flags_t)FBE_SECTOR_TRACE_DEFAULT_ERROR_FLAGS);
        }
    }
    return;
}


void fbe_test_sep_util_disable_trace_limits(void)
{
    if (fbe_test_sep_util_get_cmd_option("-stop_sep_neit_on_error"))
    {
        fbe_test_sep_util_disable_pkg_trace_limits(FBE_PACKAGE_ID_SEP_0);
        fbe_test_sep_util_disable_pkg_trace_limits(FBE_PACKAGE_ID_NEIT);
    }
    if (fbe_test_sep_util_get_cmd_option("-stop_sector_trace_on_error"))
    {
        fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);
    }
    return;
}
/*!**************************************************************
 * fbe_test_sep_util_rg_config_array_count()
 ****************************************************************
 * @brief
 *  Get the count of items in the array, which is null terminated.
 *
 * @param array_p - ptr to array to traverse.               
 *
 * @return count of items in the array.   
 *
 * @author
 *  9/1/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_util_rg_config_array_count(fbe_test_rg_configuration_array_t *array_p)
{
    fbe_u32_t index = 0;

    while (array_p[index][0].width != FBE_U32_MAX)
    {
        index++;
    }
    return index;
}
/******************************************
 * end fbe_test_sep_util_rg_config_array_count()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_rg_config_array_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *  
 *  Note that this also will fill out the disk set for each of the
 *  raid groups passed in.
 *
 * @param array_p - The array of raid group configs we will create.
 *
 * @notes It is assumed that the raid group configs are not created
 *        all at the same time, but each array index is created and
 *        then destroyed before creating the next array index.
 *        Thus, the disk sets between different array indexes can overlap.
 *
 * @return None.
 *
 * @author
 *  8/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_rg_config_array_load_physical_config(fbe_test_rg_configuration_array_t *array_p)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_block_count_t drive_capacity = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;

    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        /* First get the count of drives.
         */
        fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                           &current_num_520_drives,
                                                           &current_num_4160_drives);
        num_520_drives = FBE_MAX(num_520_drives, current_num_520_drives);
        num_4160_drives = FBE_MAX(num_4160_drives, current_num_4160_drives);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives );

    /* Fill in our raid group disk sets with this physical config.
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, num_520_drives, num_4160_drives);
    }

    /*caculate the drive capacity now
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        fbe_block_count_t current_drive_capacity;
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        current_drive_capacity = fbe_test_sep_util_get_maximum_drive_capacity(rg_config_p, 
                                                                                                                  raid_group_count);
        if(current_drive_capacity > drive_capacity)
        {
            drive_capacity = current_drive_capacity;
        }
    }

    if(drive_capacity < TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)
    {
        drive_capacity = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
    }

    /* need to add some more blocks for metadata of the objects which are not calculated as
     * we only considered the user capacity during calculations
     */
    drive_capacity += FBE_TEST_EXTRA_CAPACITY_FOR_METADATA;

    mut_printf(MUT_LOG_TEST_STATUS, "Drive capacity 0x%llx blocks (520 bps)", 
               (unsigned long long)drive_capacity);
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            drive_capacity);
    return;
}
/******************************************
 * end fbe_test_sep_util_rg_config_array_load_physical_config()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_rg_config_array_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *  
 *  Note that this also will fill out the disk set for each of the
 *  raid groups passed in.
 *
 * @param array_p - The array of raid group configs we will create.
 *
 * @notes It is assumed that the raid group configs are not created
 *        all at the same time, but each array index is created and
 *        then destroyed before creating the next array index.
 *        Thus, the disk sets between different array indexes can overlap.
 *
 * @return None.
 *
 * @author
 *  8/23/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(fbe_test_rg_configuration_array_t *array_p)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;
    fbe_u32_t current_520_extra_drives = 0;
    fbe_u32_t current_4160_extra_drives = 0;
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_block_count_t drive_capacity = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;

    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }

        /* First get the count of drives.
         */
        fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                           &current_num_520_drives,
                                                           &current_num_4160_drives);
        num_520_drives = FBE_MAX(num_520_drives, current_num_520_drives);
        num_4160_drives = FBE_MAX(num_4160_drives, current_num_4160_drives);

        /* consider extra disk count into account while create physical config
         */
        fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, raid_group_count,
                                                  &current_520_extra_drives,
                                                  &current_4160_extra_drives);
        num_520_extra_drives = FBE_MAX(num_520_extra_drives, current_520_extra_drives);
        num_4160_extra_drives = FBE_MAX(num_4160_extra_drives, current_4160_extra_drives);
    }

    num_520_drives += num_520_extra_drives;
    num_4160_drives += num_4160_extra_drives;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives );

    /* Fill in our raid group disk sets with this physical config.
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, num_520_drives, num_4160_drives);
    }

    /*caculate the drive capacity now
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        fbe_block_count_t current_drive_capacity;
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        current_drive_capacity = fbe_test_sep_util_get_maximum_drive_capacity(rg_config_p, 
                                                                                                                  raid_group_count);
        if(current_drive_capacity > drive_capacity)
        {
            drive_capacity = current_drive_capacity;
        }
    }

    if(drive_capacity < TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)
    {
        drive_capacity = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
    }

    /* need to add some more blocks for metadata of the objects which are not calculated as
     * we only considered the user capacity during calculations
     */
    drive_capacity += FBE_TEST_EXTRA_CAPACITY_FOR_METADATA;

    mut_printf(MUT_LOG_TEST_STATUS, "Drive capacity 0x%llx ", 
                          (unsigned long long)drive_capacity);
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            drive_capacity);
    return;
}
/********************************************************************************
 * end fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config()
 ********************************************************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_rg_num_enclosures_and_drives()
 ****************************************************************
 * @brief
 *  Determine the number of enclosures and drives for this raid group.
 *  Fill out the rg_disk_set of the raid group config.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups
 * @param num_520_drives_p - Total number of this block size drive we need
 *                           to create this raid group config.
 * @param num_4160_drives_p - Total number of this block size drive.
 *
 * @return None.
 *
 * @author
 *  4/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_get_rg_num_enclosures_and_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_raid_groups,
                                                        fbe_u32_t *num_520_drives_p,
                                                        fbe_u32_t *num_4160_drives_p)
{
    fbe_u32_t rg_index;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            if (rg_config_p[rg_index].block_size == 520) {
                num_520_drives += rg_config_p[rg_index].width;
            }
            else if (rg_config_p[rg_index].block_size == 4160){
                num_4160_drives += rg_config_p[rg_index].width;
            }
            else if (rg_config_p[rg_index].block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE){
                num_4160_drives += rg_config_p[rg_index].width/2;

                if (rg_config_p[rg_index].width >= 1) {
                    num_520_drives += (rg_config_p[rg_index].width % 2) ? 1 : 0;
                    num_520_drives += rg_config_p[rg_index].width/2;
                }
            }
        }
    }

    if (num_520_drives_p != NULL)
    {
        *num_520_drives_p = num_520_drives;
    }
    if (num_4160_drives_p != NULL)
    {
        *num_4160_drives_p = num_4160_drives;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_get_rg_num_enclosures_and_drives()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_rg_num_extra_drives()
 ****************************************************************
 * @brief
 *  Determine the number of total extra drives for this raid group required.
 *
 * @param rg_config_p - We determine how many of each extra drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups
 * @param num_520_drives_p - Total number of this block size extra drive we need
 *                           for this raid group config.
 * @param num_4160_drives_p - Total number of this block size drive we need
 *                           for this raid group config.
 *
 * @return None.
 *
 * @author
 *  5/27/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_get_rg_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_raid_groups,
                                                        fbe_u32_t *num_520_drives_p,
                                                        fbe_u32_t *num_4160_drives_p)
{
    fbe_u32_t rg_index;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            /* calculate the total required number of extra drives 
             */
            if (rg_config_p[rg_index].block_size == 520) {
                num_520_drives += rg_config_p[rg_index].num_of_extra_drives;
            } else if (rg_config_p[rg_index].block_size == 4160) {
                num_4160_drives += rg_config_p[rg_index].num_of_extra_drives;
            } else /* Mixed */ {
                num_520_drives += rg_config_p[rg_index].num_of_extra_drives / 2;
                num_4160_drives += rg_config_p[rg_index].num_of_extra_drives / 2;
                /* Use 520 for remainder. */
                if ((rg_config_p[rg_index].num_of_extra_drives % 2) != 0) {
                    num_520_drives++;
                }
            }
        }
    }

    if (num_520_drives_p != NULL)
    {
        *num_520_drives_p = num_520_drives;
    }
    if (num_4160_drives_p != NULL)
    {
        *num_4160_drives_p = num_4160_drives;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_get_rg_num_extra_drives()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_rg_num_extra_drives()
 ****************************************************************
 * @brief
 *  Initialize the rg config with number of extra drive required for each rg. This 
 *  information will use by a test while removing a drive and insert a new drive.
 *
 * @note   
 *  this function only needs to call from a fbe_test which needs to support
 *  use case of remove a drive and insert a new drive in simulation or on hardware. 
 *  By default extra drive count remains set to zero for a test which does not need 
 *  functionality to insert a new drive.
 *
 * @param rg_config_p - Array of raidgroup to determine how many extra drive needed 
 *                                for each rg and to populate extra drive count.
 *
 * 
 *
 * @return None.
 *
 * @author
 *  5/27/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_populate_rg_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   rg_index;
    fbe_u32_t   num_req_extra_drives;
    fbe_u32_t   num_raid_groups;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            fbe_test_sep_util_get_rg_type_num_extra_drives(rg_config_p[rg_index].raid_type,
                                                        rg_config_p[rg_index].width,
                                                        &num_req_extra_drives);

            /* set the required number of extra drives for each rg in rg config 
            */         
            rg_config_p[rg_index].num_of_extra_drives = num_req_extra_drives;
        }
    }

}
/**************************************
 * end fbe_test_sep_util_init_rg_num_extra_drives()
 **************************************/
 

/*!**************************************************************
 * fbe_test_sep_util_get_rg_type_num_extra_drives()
 ****************************************************************
 * @brief
 *  Determine number of extra drives for given raid group type.
 *  Extra disk count is calculated based on maximum number of disk 
 *  can be removed for degraded raid group.
 *  
 *
 * @param raid_type - raid group type
 * @param rg_width -  number of drive in rg
 * @param num_extra_drives_p - pointer to return required number of 
 *                             extra drives 
 *
 * @return None.
 *
 * @author
 *  5/25/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
void fbe_test_sep_util_get_rg_type_num_extra_drives(fbe_raid_group_type_t    raid_type,
                                                        fbe_u32_t   rg_width,
                                                        fbe_u32_t   *num_extra_drives_p)
{

    fbe_u32_t extra_required_drives = 0;

    switch(raid_type)
    {

        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
            extra_required_drives = 1;
            break;

        case FBE_RAID_GROUP_TYPE_RAID6:
            extra_required_drives = 2;
            break;

        case FBE_RAID_GROUP_TYPE_RAID1:
            extra_required_drives = rg_width - 1;
            break;

        case FBE_RAID_GROUP_TYPE_RAID10:
            extra_required_drives = rg_width/2;
            break;

        default:
            /* this is not expected */
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Unexpected raid group type to find out extra disk count\n");
        break;

    }

    if (num_extra_drives_p != NULL)
    {
        *num_extra_drives_p = extra_required_drives;
    }

  
}
/**************************************
 * end fbe_test_sep_util_get_rg_type_num_extra_drives()
 **************************************/


/*!**************************************************************
 * fbe_test_sep_util_rg_fill_disk_sets()
 ****************************************************************
 * @brief
 *  Fill out the disk sets for a given raid group when we have
 *  a given number of max disks.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups -
 * @param max_520_drives - Total number of 520 drives we are creating.
 * @param max_4160_drives - Total number of 4160 drives we are creating.
 *
 * @return None.
 *
 * @author
 *  9/2/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_rg_fill_disk_sets(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t num_raid_groups,
                                         fbe_u32_t max_520_drives,
                                         fbe_u32_t max_4160_drives)
{
    fbe_u32_t rg_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;

    num_520_enclosures = (max_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((max_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (max_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((max_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Fill in the raid group disk sets.
     */
    {
        fbe_u32_t drive_index;
        fbe_u32_t current_520_slot = 0;
        fbe_u32_t current_4160_slot = 0;
        fbe_u32_t current_520_encl = 1;
        fbe_u32_t current_4160_encl = 1 + num_520_enclosures;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]) &&
                !rg_config_p[rg_index].b_use_fixed_disks)
            {
                for ( drive_index = 0; drive_index < rg_config_p[rg_index].width; drive_index++)
                {
                    if ((rg_config_p[rg_index].block_size == 520) ||
                        (rg_config_p[rg_index].block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE &&
                         (drive_index % 2) == 0))
                    {
                        rg_config_p[rg_index].rg_disk_set[drive_index].bus = 0;
                        rg_config_p[rg_index].rg_disk_set[drive_index].enclosure = current_520_encl;
                        rg_config_p[rg_index].rg_disk_set[drive_index].slot = current_520_slot;
                        current_520_slot++;
                        current_520_encl += (current_520_slot == FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0;
                        if (current_520_slot == FBE_TEST_DRIVES_PER_ENCL)
                        {
                            current_520_slot = 0;
                        }
                    }
                    else if ((rg_config_p[rg_index].block_size == 4160) ||
                             (rg_config_p[rg_index].block_size == FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE &&
                              (drive_index % 2) != 0))
                    {
                        rg_config_p[rg_index].rg_disk_set[drive_index].bus = 0;
                        rg_config_p[rg_index].rg_disk_set[drive_index].enclosure = current_4160_encl;
                        rg_config_p[rg_index].rg_disk_set[drive_index].slot = current_4160_slot;
                        current_4160_slot++;
                        current_4160_encl += (current_4160_slot == FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0;
                        if (current_4160_slot == FBE_TEST_DRIVES_PER_ENCL)
                        {
                            current_4160_slot = 0;
                        }
                    }
                    else
                    {
                        MUT_ASSERT_INT_EQUAL(520, rg_config_p[rg_index].block_size);
                    }
                }
            }  // if enabled
        }
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_rg_fill_disk_sets()
 **************************************/

void fbe_test_sep_util_get_ds_object_list(fbe_object_id_t object_id,
                                          fbe_api_base_config_downstream_object_list_t *ds_list_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    /* initialize the downstream object list.
     */
    ds_list_p->number_of_downstream_objects = 0;
    for (index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        ds_list_p->downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
    /* For Raid 10, we need to get the object id of the mirror.
     * Issue the control code to RAID group to get its downstream edge list. 
     */  
    status = fbe_api_base_config_get_ds_object_list(object_id, ds_list_p,
                 /* We want the packet to get processed even in specialize.  */
                                                    FBE_PACKET_FLAG_INTERNAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
fbe_status_t
fbe_test_sep_util_get_virtual_drive_object_id_by_position(fbe_object_id_t rg_object_id,
                                                          fbe_u32_t position,
                                                          fbe_object_id_t * vd_object_id_p)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t get_info;
    fbe_u32_t  current_time = 0;

    MUT_ASSERT_TRUE(position < FBE_MAX_DOWNSTREAM_OBJECTS);

    while (current_time < FBE_TEST_WAIT_OBJECT_TIMEOUT_MS)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &get_info, FBE_PACKET_FLAG_EXTERNAL);

        if (status == FBE_STATUS_OK)
        {
            break;
        }
        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_UTILS_POLLING_INTERVAL);
    } 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* initialize the vd object id as invalid. */
    *vd_object_id_p = FBE_OBJECT_ID_INVALID;

    if (get_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t mirror_index;
        fbe_u32_t position_in_mirror;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        /* For a raid 10 the downstream object list has the IDs of the mirrors. 
         * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
         */
        mirror_index = position / 2;
        position_in_mirror = position % 2;
        fbe_test_sep_util_get_ds_object_list(downstream_object_list.downstream_object_list[mirror_index], 
                                             &downstream_object_list);

        *vd_object_id_p = downstream_object_list.downstream_object_list[position_in_mirror];
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *vd_object_id_p);
    }
    else
    {
        /* Get the virtual drive object-id for the specified position. */
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        *vd_object_id_p = downstream_object_list.downstream_object_list[position];
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *vd_object_id_p);
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_test_sep_util_get_all_vd_object_ids()
 ****************************************************************
 * @brief
 *  Fetch the array of vd object ids for a given rg.
 *
 * @param rg_object_id - object id to get vd array for.
 * @param vd_object_ids_p - Ptr to array of vd object ids.
 *                          assumed to be FBE_RAID_MAX_DISK_ARRAY_WIDTH
 *
 * @return None.   
 *
 * @author
 *  10/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_get_all_vd_object_ids(fbe_object_id_t rg_object_id,
                                             fbe_object_id_t * vd_object_ids_p)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t get_info;
    fbe_u32_t position;

    status = fbe_api_raid_group_get_info(rg_object_id, &get_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_CONDITION(get_info.width, <=, FBE_MAX_DOWNSTREAM_OBJECTS);

    /* initialize the vd object ids to invalid.
     */
    for (position = 0; position < FBE_RAID_MAX_DISK_ARRAY_WIDTH; position++)
    {
        vd_object_ids_p[position] = FBE_OBJECT_ID_INVALID;
    }

    if (get_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t striper_position;
        fbe_u32_t array_position = 0;
        fbe_api_base_config_downstream_object_list_t mirror_downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each position in the striper get both mirror positions.
         */
        for (striper_position = 0; striper_position < get_info.width; striper_position++)
        {
            fbe_test_sep_util_get_ds_object_list(downstream_object_list.downstream_object_list[striper_position], 
                                                 &mirror_downstream_object_list);

            vd_object_ids_p[array_position] = mirror_downstream_object_list.downstream_object_list[0];
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_ids_p[array_position]);
            vd_object_ids_p[array_position + 1] = mirror_downstream_object_list.downstream_object_list[1];
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_ids_p[array_position + 1]);

            array_position += 2;
        }
    }
    else
    {
        /* Get the virtual drive object-id for the specified position. */
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        for (position = 0; position < get_info.width; position++)
        {
            vd_object_ids_p[position] = downstream_object_list.downstream_object_list[position];
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_ids_p[position]);
        }
    }
}
/******************************************
 * end fbe_test_sep_util_get_all_vd_object_ids()
 ******************************************/

/*****************************************************************************
 *          fbe_test_sep_util_banner()
 *****************************************************************************
 *
 * @brief   This method will display a banner message
 * 
 ***************************************************************************/
void fbe_test_sep_util_banner(const char *func, const char *message)
{
    fbe_u32_t length;
    char      *header;
    char      *formatted_message;

    /* Length is message length + spaces + border + terminator */
    length = (fbe_u32_t)(strlen(func) + strlen(message) + 5);

    header = fbe_api_allocate_memory(length);
    formatted_message = fbe_api_allocate_memory(length);

    memset(header, '=', length - 1);
    header[length - 1] = 0;

    _snprintf(formatted_message, length, "= %s%s =", func, message);

    mut_printf(MUT_LOG_LOW, "%s", header);
    mut_printf(MUT_LOG_LOW, "%s", formatted_message);
    mut_printf(MUT_LOG_LOW, "%s", header);

    fbe_api_free_memory(header);
    fbe_api_free_memory(formatted_message);
}

/*!**************************************************************
 * fbe_test_sep_util_get_virtual_drive_configuration_mode
 ****************************************************************
 * @brief
 *  This function is used to get the configuration mode of the
 *  virtual drive object.
 * 
 * @param vd_object_id  - virtual drive object id.  
 *
 * @return configuration_mode_p - configuration mode of virtual 
 *                                drive object.
 *
 * @author
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
void fbe_test_sep_util_get_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                            fbe_virtual_drive_configuration_mode_t * configuration_mode_p)
{
    fbe_api_virtual_drive_get_configuration_t virtual_drive_configuration;
    fbe_status_t status;

    /* get the configuration of the virtual drive object. */
    status = fbe_api_virtual_drive_get_configuration(vd_object_id, &virtual_drive_configuration);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* return configuration mode details. */
    *configuration_mode_p = virtual_drive_configuration.configuration_mode;
}


/*!**************************************************************
 * fbe_test_util_build_job_service_control_element()
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
fbe_status_t
fbe_test_util_build_job_service_control_element(fbe_job_queue_element_t *   job_queue_element,
                                                fbe_job_service_control_t   control_code,
                                                fbe_object_id_t             object_id,
                                                fbe_u8_t *                  command_data_p,
                                                fbe_u32_t                   command_size)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_job_type_t  job_type = FBE_JOB_TYPE_INVALID;

    /* Context size cannot be greater than job element context size. */
    if (command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    job_queue_element->object_id = object_id;

    /* only set the job type element for requests that are queued */
    switch(control_code)
    {
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS,
                        "%s unsupported job control code: %d ",
                        __FUNCTION__, control_code);
             MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
             return status;
    }

    job_queue_element->job_type = job_type;
    job_queue_element->timestamp = 0;
    job_queue_element->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->current_state  = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->queue_access = TRUE;
    job_queue_element->num_objects = 0;
    job_queue_element->command_size = command_size;

    if (command_data_p != NULL)
    {
        fbe_copy_memory(job_queue_element->command_data, command_data_p, command_size);
    }
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_test_sep_util_disable_recovery_queue
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
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_disable_recovery_queue(fbe_object_id_t object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: disable recovery queue.", __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_test_util_build_job_service_control_element(&job_queue_element,
                                                             FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                             object_id,
                                                             (fbe_u8_t *)NULL,
                                                             0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Job service control element creating failed");

    job_queue_element.queue_access = FALSE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_enable_recovery_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to enable
 *  recovery queue processing.
 * 
 * @param object_id  - the ID of the object.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_enable_recovery_queue(fbe_object_id_t object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: enable recovery queue.", __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_test_util_build_job_service_control_element(&job_queue_element,
                                                             FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                             object_id,
                                                             (fbe_u8_t *)NULL,
                                                             0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Job service control element creating failed");

    job_queue_element.queue_access = TRUE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_disable_creation_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to disable
 *  creation queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_disable_creation_queue(fbe_object_id_t object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: disable creation queue.", __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_test_util_build_job_service_control_element(&job_queue_element,
                                                             FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE,
                                                             object_id,
                                                             (fbe_u8_t *)NULL,
                                                             0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Job service control element creating failed");

    job_queue_element.queue_access = FALSE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_enable_creation_queue
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to enable
 *  creation queue processing.
 * 
 * @param object_id  - the ID of the object.
 *
 * @return fbe_status - status of enable request
 *
 * @author
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_enable_creation_queue(fbe_object_id_t object_id)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: enable creation queue.", __FUNCTION__);

    /* Setup the job service control element with appropriate values */
    status = fbe_test_util_build_job_service_control_element(&job_queue_element,
                                                             FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE,
                                                             object_id,
                                                             (fbe_u8_t *)NULL,
                                                             0);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Job service control element creating failed");

    job_queue_element.queue_access = TRUE;
    status = fbe_api_job_service_process_command(&job_queue_element,
                                                 FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_physical
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_physical(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;
    unload_packages.physical = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_packages(&unload_packages); 
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_physical
 ****************************************************************
 * @brief
 *  Destorys all 4 packages: neit, sep, esp, physical
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. Saranyadevi Ganesan
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_esp_neit_sep_physical(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.esp = FBE_TRUE;
    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;
    unload_packages.physical = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_packages(&unload_packages); 
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_physical_no_esp
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_physical_no_esp(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;
    unload_packages.physical = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_packages(&unload_packages); 
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}
/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_physical_both_sps
 ****************************************************************
 * @brief
 *  This function destroys both SPs.
 * 
 * @param None
 *
 * @return none
 *
 * @author
 *  5/4/2011 - Created. Rob Foley
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_physical_both_sps(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/**************************************
 * end fbe_test_sep_util_destroy_neit_sep_physical_both_sps()
 **************************************/

void fbe_test_sep_util_destroy_esp_neit_sep_physical_both_sps(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_esp_neit_sep_physical();
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_sep_neit_physical_verify_no_trace_error
 ****************************************************************
 * @brief
 *  This function validates that no errors occurred in
 *  these packages.
 * 
 * @param None.
 *
 * @return none
 *
 * @author
 *  11/10/2010 - Created. Rob Foley
 * 
 ****************************************************************/
void fbe_test_sep_util_sep_neit_physical_verify_no_trace_error(void)
{
    fbe_status_t status;
    fbe_api_trace_error_counters_t error_counters;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_EQUAL(error_counters.trace_error_counter, 0);

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_EQUAL(error_counters.trace_error_counter, 0);

    if (fbe_test_sep_util_get_error_injection_test_mode()==FBE_FALSE)
    {
        status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get sector trace error counters from sep Package!");
        if (sector_trace_counters.total_errors_traced != 0)
        {
            fbe_api_sector_trace_display_counters(FBE_TRUE /* verbose */);
        }
        MUT_ASSERT_INT_EQUAL(sector_trace_counters.total_errors_traced, 0);
    }

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_EQUAL(error_counters.trace_error_counter, 0);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_sep_neit_physical_reset_error_counters
 ****************************************************************
 * @brief
 *  This function resets trace error counters
 * 
 * @param None.
 *
 * @return none
 *
 * @author
 *  06/09/2011 - Created. Vamsi V
 * 
 ****************************************************************/
void fbe_test_sep_util_sep_neit_physical_reset_error_counters(void)
{
    fbe_status_t status;

    /* reset the error counters and remove all error records */
    status = fbe_api_system_reset_failure_info(FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;   
}

void fbe_test_sep_util_destroy_packages(const fbe_test_packages_t *unload_packages)
{
    fbe_u32_t count = 0;
    fbe_test_package_list_t package_list;
    fbe_status_t status = FBE_STATUS_INVALID;

    fbe_zero_memory(&package_list, sizeof(package_list));

    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    
    if ( unload_packages->neit == FBE_TRUE )
    {
        package_list.package_list[count] = FBE_PACKAGE_ID_NEIT;
        count++;
    }
    if ( unload_packages->sep == FBE_TRUE )
    {
        package_list.package_list[count] = FBE_PACKAGE_ID_SEP_0;
        count++;
    }
    if ( is_esp_loaded == FBE_TRUE || unload_packages->esp == FBE_TRUE ) 
    {
        package_list.package_list[count] = FBE_PACKAGE_ID_ESP;
        count++;
    }
    if ( unload_packages->physical == FBE_TRUE )
    {
        package_list.package_list[count] = FBE_PACKAGE_ID_PHYSICAL;
        count++;
    }

    package_list.number_of_packages = count;

    fbe_test_common_util_package_destroy(&package_list);
    if ( unload_packages->physical == FBE_TRUE )
    {
        status = fbe_api_terminator_destroy();
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");
    }
    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);
    return;
}

/*!**************************************************************
 * Fbe_test_sep_util_destroy_neit_sep
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_destroy_packages(&unload_packages);   
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

void fbe_test_sep_util_destroy_esp_neit_sep(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.esp = FBE_TRUE;
    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_destroy_packages(&unload_packages); 
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_both_sps
 ****************************************************************
 * @brief
 *  This function
 *
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 *
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_both_sps(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    /* Start destroying SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_destroy_neit_sep();

    /* Start destroying SP B.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_destroy_neit_sep();

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_sep_physical
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_sep_physical(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.sep = FBE_TRUE;
    unload_packages.physical = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_destroy_packages(&unload_packages);
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_destroy_sep_physical_both_sps
 ****************************************************************
 * @brief
 *  This function destroys SEP and Physical Package in both SPs.
 * 
 * @param None
 *
 * @return none
 *
 * @author
 *  7/15/2011 - Created. Arun S
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_sep_physical_both_sps(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    fbe_test_sep_util_destroy_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    fbe_test_sep_util_destroy_sep_physical();
    return;
}
/**************************************
 * end fbe_test_sep_util_destroy_sep_physical_both_sps()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_destroy_sep
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 * 06/11/2010 - Created. guov
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_sep(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_destroy_packages(&unload_packages);
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

/*!**************************************************************
 * fbe_test_sep_util_configure_pvd_as_hot_spare()
 ****************************************************************
 * @brief
 *  This function sends a request to job service to configure
 *  the provision drive as hot spare. 
 * 
 * @param pvd_object_id  - pvd object-id.
 * @param b_allow_is_use_drives - FBE_TRUE - Allow one or more drives
 *          to be in-use.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
void fbe_test_sep_util_configure_pvd_as_hot_spare(fbe_object_id_t  pvd_object_id, fbe_u8_t *opaque_data, fbe_u32_t opaque_data_size,
                                                  fbe_bool_t b_allow_is_use_drives)
{
    fbe_api_job_service_update_provision_drive_config_type_t    update_pvd_config_type;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_job_service_error_type_t                                job_error_type;
    fbe_u32_t                                                   max_opaque_data_size;
    fbe_u8_t                                                    *opaque_data_buffer_p;

    /* initialize error type as no error. */ 
    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* configure the pvd as hot spare. */
    update_pvd_config_type.pvd_object_id = pvd_object_id;
    update_pvd_config_type.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE;
    fbe_zero_memory(update_pvd_config_type.opaque_data, sizeof(update_pvd_config_type.opaque_data));
    fbe_copy_memory(update_pvd_config_type.opaque_data, opaque_data, opaque_data_size);

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_config_type.job_number,
                                         FBE_TEST_UTILS_CREATE_TIMEOUT_MS,
                                         &job_error_type,
                                         &job_status,
                                         NULL);

    /* verify the job status. */
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(FBE_STATUS_TIMEOUT, status, "Timed out configuring hot spare");
    MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_STATUS_OK, job_status, "Configuring PVD as hot spare failed");

    /* it maybe that the pvd is already in-use for a raid group*/
    switch(job_error_type)
    {
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
            break;

        case FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE:
            /* It is not an error if the drive is already configured as an 
             * `hot-spare'.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s job error: %d pvd obj: 0x%x already hot-spare.",
                       __FUNCTION__, job_error_type, pvd_object_id);
            return;
            break;

        case FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP:
            if (b_allow_is_use_drives == FBE_TRUE)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s job error: %d pvd obj: 0x%x already swapped.",
                           __FUNCTION__, job_error_type, pvd_object_id); 
            }
            else
            {
                MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type, "Configuring PVD as hot-spare failed");
            }
            break;

        default:
            MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type, "Configuring PVD as hot-spare failed");
            break;
    }

    /* verify the opaque_data is correctly stored */
    fbe_api_provision_drive_get_max_opaque_data_size(&max_opaque_data_size);
    opaque_data_buffer_p = (fbe_u8_t*) fbe_api_allocate_contiguous_memory(max_opaque_data_size);
    MUT_ASSERT_NOT_NULL(opaque_data_buffer_p);
    fbe_zero_memory(opaque_data_buffer_p, max_opaque_data_size);
    /* read the hot spare information from provision drive config*/
    status = fbe_api_provision_drive_get_opaque_data(pvd_object_id, max_opaque_data_size, opaque_data_buffer_p);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_BUFFER_EQUAL(update_pvd_config_type.opaque_data, opaque_data_buffer_p, max_opaque_data_size);
    fbe_api_free_contiguous_memory(opaque_data_buffer_p);
}

/*!**************************************************************
 * fbe_test_sep_util_configure_hot_spares()
 ****************************************************************
 * @brief
 *  This function is used to configure the drive as hot spare.
 * 
 * @param hot_spare_configuration_p - hot spare configuration.
 * @param hot_spare_count           - hot spare count.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_configure_hot_spares(fbe_test_hs_configuration_t * hot_spare_configuration_p,
                                       fbe_u32_t hot_spare_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       hs_index = 0;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_sep_util_hot_spare_dummy_info_t dummy_info;


    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* go through all the drives which needs to configure as hot spare.*/
    for (hs_index = 0; hs_index < hot_spare_count; hs_index++)
    {
        if(hot_spare_configuration_p == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Configure the port %d, enclosure %d, slot %d as Hot Spare.", 
                   hot_spare_configuration_p->disk_set.bus,
                   hot_spare_configuration_p->disk_set.enclosure,
                   hot_spare_configuration_p->disk_set.slot);

        status = fbe_api_provision_drive_get_obj_id_by_location(hot_spare_configuration_p->disk_set.bus,
                                                                hot_spare_configuration_p->disk_set.enclosure,
                                                                hot_spare_configuration_p->disk_set.slot,
                                                                &hot_spare_configuration_p->hs_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, hot_spare_configuration_p->hs_pvd_object_id);

        /* Check if the provision drive has already been configured as a hot-spare.
         */
        status = fbe_api_provision_drive_get_info(hot_spare_configuration_p->hs_pvd_object_id, 
                                                  &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s pvd obj: 0x%x (%d_%d_%d) already configured as spare.",
                       __FUNCTION__,  hot_spare_configuration_p->hs_pvd_object_id,
                       hot_spare_configuration_p->disk_set.bus,
                       hot_spare_configuration_p->disk_set.enclosure,
                       hot_spare_configuration_p->disk_set.slot);
        }
        else
        {
            /* Else we need to configure the spare.
             */
            dummy_info.magic_num = 0xFBE8FBE8;
            dummy_info.object_id = hot_spare_configuration_p->hs_pvd_object_id;
            dummy_info.port = hot_spare_configuration_p->disk_set.bus;
            dummy_info.encl = hot_spare_configuration_p->disk_set.enclosure;
            dummy_info.slot = hot_spare_configuration_p->disk_set.slot;
    
            fbe_test_sep_util_configure_pvd_as_hot_spare(hot_spare_configuration_p->hs_pvd_object_id, 
                                                         (fbe_u8_t *)&dummy_info, 
                                                         sizeof(fbe_test_sep_util_hot_spare_dummy_info_t),
                                                         FBE_FALSE /* Don't expect any in-use drives*/);
        }

        /* Goto next*/
        hot_spare_configuration_p++;
    }    
    return status;
}
/*!**************************************************************
 * fbe_test_sep_util_reconfigure_hot_spares()
 ****************************************************************
 * @brief
 *  This function is used to configure the drive as hot spare.
 * 
 * @param hot_spare_configuration_p - hot spare configuration.
 * @param hot_spare_count           - hot spare count.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_reconfigure_hot_spares(fbe_test_hs_configuration_t * hot_spare_configuration_p,
                                         fbe_u32_t hot_spare_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       hs_index = 0;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_sep_util_hot_spare_dummy_info_t dummy_info;

    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* go through all the drives which needs to configure as hot spare.*/
    for (hs_index = 0; hs_index < hot_spare_count; hs_index++)
    {
        if(hot_spare_configuration_p == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Configure the port %d, enclosure %d, slot %d as Hot Spare.", 
                   hot_spare_configuration_p->disk_set.bus,
                   hot_spare_configuration_p->disk_set.enclosure,
                   hot_spare_configuration_p->disk_set.slot);

        status = fbe_api_provision_drive_get_obj_id_by_location(hot_spare_configuration_p->disk_set.bus,
                                                                hot_spare_configuration_p->disk_set.enclosure,
                                                                hot_spare_configuration_p->disk_set.slot,
                                                                &hot_spare_configuration_p->hs_pvd_object_id);
        /* Test might have already consumed one or more hot-spares*/
        if (status != FBE_STATUS_OK)
        {
            hot_spare_configuration_p++;
            continue;
        }
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, hot_spare_configuration_p->hs_pvd_object_id);

        /* Check if the provision drive has already been configured as a hot-spare.
         */
        status = fbe_api_provision_drive_get_info(hot_spare_configuration_p->hs_pvd_object_id, 
                                                  &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s pvd obj: 0x%x (%d_%d_%d) already configured as spare.",
                       __FUNCTION__,  hot_spare_configuration_p->hs_pvd_object_id,
                       hot_spare_configuration_p->disk_set.bus,
                       hot_spare_configuration_p->disk_set.enclosure,
                       hot_spare_configuration_p->disk_set.slot);
        }
        else
        {
            /* Else we need to configure the spare.
             */
            dummy_info.magic_num = 0xFBE8FBE8;
            dummy_info.object_id = hot_spare_configuration_p->hs_pvd_object_id;
            dummy_info.port = hot_spare_configuration_p->disk_set.bus;
            dummy_info.encl = hot_spare_configuration_p->disk_set.enclosure;
            dummy_info.slot = hot_spare_configuration_p->disk_set.slot;

            fbe_test_sep_util_configure_pvd_as_hot_spare(hot_spare_configuration_p->hs_pvd_object_id, 
                                                     (fbe_u8_t *)&dummy_info, 
                                                     sizeof(fbe_test_sep_util_hot_spare_dummy_info_t),
                                                     FBE_TRUE /* Allow some hot-spares to be in-use */);
        }
        hot_spare_configuration_p++;
    }    
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_configure_pvd_as_automatic_spare()
 ****************************************************************
 * @brief
 *  This function sends a request to job service to configure
 *  the provision drive as `unknown' (i.e. available for automatic sparing).
 * 
 * @param pvd_object_id  - pvd object-id.
 * @param b_allow_is_use_drives - FBE_TRUE - Allow one or more drives
 *          to be in-use.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
void fbe_test_sep_util_configure_pvd_as_automatic_spare(fbe_object_id_t  pvd_object_id, fbe_u8_t *opaque_data, fbe_u32_t opaque_data_size,
                                                        fbe_bool_t b_allow_is_use_drives)
{
    fbe_api_job_service_update_provision_drive_config_type_t    update_pvd_config_type;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_job_service_error_type_t                                job_error_type;
    fbe_u32_t                                                   max_opaque_data_size;
    fbe_u8_t                                                    *opaque_data_buffer_p;

    /* initialize error type as no error. */ 
    job_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* configure the pvd as hot spare. */
    update_pvd_config_type.pvd_object_id = pvd_object_id;
    update_pvd_config_type.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
    fbe_zero_memory(update_pvd_config_type.opaque_data, sizeof(update_pvd_config_type.opaque_data));
    fbe_copy_memory(update_pvd_config_type.opaque_data, opaque_data, opaque_data_size);

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_config_type.job_number,
                                         FBE_TEST_UTILS_CREATE_TIMEOUT_MS,
                                         &job_error_type,
                                         &job_status,
                                         NULL);

    /* verify the job status. */
    MUT_ASSERT_INTEGER_NOT_EQUAL_MSG(FBE_STATUS_TIMEOUT, status, "Timed out configuring automatic spare");
    MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_STATUS_OK, job_status, "Configuring PVD as automatic spare failed");

    /* it maybe that the pvd is already in-use for a raid group*/
    switch(job_error_type)
    {
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
            break;

        case FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED:
            /* It is not an error if the drive is already configured as an 
             * `automatic' spare.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s job error: %d pvd obj: 0x%x already automatic.",
                       __FUNCTION__, job_error_type, pvd_object_id);
            return;
            break; 

        case FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP:
            if (b_allow_is_use_drives == FBE_TRUE)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s job error: %d pvd obj: 0x%x already automatic.",
                           __FUNCTION__, job_error_type, pvd_object_id); 
            }
            else
            {
                MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type, "Configuring PVD as automatic spare failed");
            }
            break;

        default:
            MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_type, "Configuring PVD as automatic spare failed");
            break;
    }

    /* verify the opaque_data is correctly stored */
    fbe_api_provision_drive_get_max_opaque_data_size(&max_opaque_data_size);
    opaque_data_buffer_p = (fbe_u8_t*) fbe_api_allocate_contiguous_memory(max_opaque_data_size);
    MUT_ASSERT_NOT_NULL(opaque_data_buffer_p);
    fbe_zero_memory(opaque_data_buffer_p, max_opaque_data_size);
    /* read the hot spare information from provision drive config*/
    status = fbe_api_provision_drive_get_opaque_data(pvd_object_id, max_opaque_data_size, opaque_data_buffer_p);
    MUT_ASSERT_INTEGER_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_BUFFER_EQUAL(update_pvd_config_type.opaque_data, opaque_data_buffer_p, max_opaque_data_size);
    fbe_api_free_contiguous_memory(opaque_data_buffer_p);
}

/*!**************************************************************
 * fbe_test_sep_util_configure_automatic_spares()
 ****************************************************************
 * @brief
 *  This function is used to configure the drive as automatic spare.
 * 
 * @param spare_configuration_p - hot spare configuration.
 * @param spare_count           - hot spare count.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_configure_automatic_spares(fbe_test_hs_configuration_t *spare_configuration_p,
                                             fbe_u32_t spare_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       spare_index = 0;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_sep_util_hot_spare_dummy_info_t dummy_info;

    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* go through all the drives which needs to configure as hot spare.*/
    for (spare_index = 0; spare_index < spare_count; spare_index++)
    {
        if(spare_configuration_p == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Configure the port %d, enclosure %d, slot %d as `Auto-Spare'", 
                   spare_configuration_p->disk_set.bus,
                   spare_configuration_p->disk_set.enclosure,
                   spare_configuration_p->disk_set.slot);

        status = fbe_api_provision_drive_get_obj_id_by_location(spare_configuration_p->disk_set.bus,
                                                                spare_configuration_p->disk_set.enclosure,
                                                                spare_configuration_p->disk_set.slot,
                                                                &spare_configuration_p->hs_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, spare_configuration_p->hs_pvd_object_id);

        /* Check if the provision drive has already been configured as an automatic
         * spare.
         */
        status = fbe_api_provision_drive_get_info(spare_configuration_p->hs_pvd_object_id, 
                                                  &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                  "%s pvd obj: 0x%x (%d_%d_%d) already configured as automatic.",
                  __FUNCTION__,  spare_configuration_p->hs_pvd_object_id,
                  spare_configuration_p->disk_set.bus,
                  spare_configuration_p->disk_set.enclosure,
                  spare_configuration_p->disk_set.slot);
        }
        else
        {
            dummy_info.magic_num = 0xFBE8FBE8;
            dummy_info.object_id = spare_configuration_p->hs_pvd_object_id;
            dummy_info.port = spare_configuration_p->disk_set.bus;
            dummy_info.encl = spare_configuration_p->disk_set.enclosure;
            dummy_info.slot = spare_configuration_p->disk_set.slot;

            fbe_test_sep_util_configure_pvd_as_automatic_spare(spare_configuration_p->hs_pvd_object_id, 
                                                           (fbe_u8_t *)&dummy_info, 
                                                           sizeof(fbe_test_sep_util_hot_spare_dummy_info_t),
                                                           FBE_TRUE /* Allow some hot-spares to be in-use */);
        }

        /* Goto next
         */
        spare_configuration_p++;
    }    
    return status;
}
/*!****************************************************************************
 * fbe_test_sep_util_remove_hotspares_from_hotspare_pool()
 ******************************************************************************
 * @brief
 *  This method is used to temporarily remove the configured hot-spares from
 *  the hot-spare pool.
 *  
 *
 * @param   rg_config_p - Pointer to array or raid groups to move hot-spares.
 * @param   raid_group_count - Number of raid groups to perform action on
 *
 * @return  status
 *
 * @note    The hot-spares moved come from the extra_drives pool.
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t fbe_test_sep_util_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p, 
                                                                   fbe_u32_t raid_group_count)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index;
    fbe_test_hs_configuration_t hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t       hs_index = 0;
    fbe_test_raid_group_disk_set_t *disk_set_p = NULL;
    
    /* For all the raid groups requested.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Skip the raid group if it is not enabled this pass
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /*! @note There are some test that will no longer have any `extra'
         *        disk.  Print a message if that is the case and continue.
         */
        if (current_rg_config_p->num_of_extra_drives < 1)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "remove hot-spares: rg_index: %d rg id: %d type: %d no extra drives remaining\n", 
                       rg_index, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
            current_rg_config_p++;
            continue;
        }

        /* Locate all the hot-spares.
         */
        disk_set_p = &current_rg_config_p->extra_disk_set[0];
        for (hs_index = 0; hs_index < current_rg_config_p->num_of_extra_drives; hs_index ++)
        {
            /* 0_0_0 Is a special drive location that designates the end of the
             * drive list.
             */
            if (((disk_set_p + hs_index)->bus == 0)      &&
                ((disk_set_p + hs_index)->enclosure == 0) &&
                ((disk_set_p + hs_index)->slot == 0)         )
            {
                /* End of list reached*/
                break;
            }

            /* Else populate the `hot-spare' configuration information.
             */
            hs_config[hs_index].block_size = 520;
            hs_config[hs_index].disk_set = *(disk_set_p + hs_index) ;
            hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
        }
        if(hs_index == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s rg_index: %d rg id: %d type: %d insufficient spares\n", 
                       __FUNCTION__, rg_index, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
            return status;
        }
        else
        {
            status = fbe_test_sep_util_configure_automatic_spares(hs_config, hs_index);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }
    return status;
}

/*!****************************************************************************
 * fbe_test_sep_util_add_hotspares_to_hotspare_pool()
 ******************************************************************************
 * @brief
 *  This method is used to place the hot-spares back into the hot-spare pool.
 *  
 *
 * @param   disk_set_p - Pointer to array of drives to move to hot-spare pool
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t fbe_test_sep_util_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p, 
                                                              fbe_u32_t raid_group_count)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index;
    fbe_test_hs_configuration_t hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t       hs_index = 0;
    fbe_test_raid_group_disk_set_t *disk_set_p = NULL;
    
    /* For all the raid groups requested.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Skip the raid group if it is not enabled this pass
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* If this method was invoked we assume there are drives to move
         */
        MUT_ASSERT_TRUE(current_rg_config_p->num_of_extra_drives > 0);
        disk_set_p = &current_rg_config_p->extra_disk_set[0];

        /* Locate all the hot-spares.
         */
        for (hs_index = 0; hs_index < current_rg_config_p->num_of_extra_drives; hs_index ++)
        {
            /* 0_0_0 Is a special drive location that designates the end of the
             * drive list.
             */
            if (((disk_set_p + hs_index)->bus == 0)      &&
                ((disk_set_p + hs_index)->enclosure == 0) &&
                ((disk_set_p + hs_index)->slot == 0)         )
            {
                /* End of list reached*/
                break;
            }

            /* Else populate the `hot-spare' configuration information.
             */
            hs_config[hs_index].block_size = 520;
            hs_config[hs_index].disk_set = *(disk_set_p + hs_index) ;
            hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
        }
        if (hs_index == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s rg_index: %d rg id: %d type: %d insufficient spares\n", 
                       __FUNCTION__, rg_index, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
            return status;
        }
        else
        {
            status = fbe_test_sep_util_reconfigure_hot_spares(hs_config, hs_index);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_swap_in_proactive_spare
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to swap-in
 *  proactive spare.
 * 
 * @param vd_object_id  - virtual drive object-id.
 * @param b_wait_for_job - FBE_TRUE - wait for job completion
 * @param job_error_type_p  - pointer to job error code
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 05/12/2010 - Created. Dhaval Patel
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_swap_in_proactive_spare(fbe_object_id_t vd_object_id,
                                                       fbe_bool_t b_wait_for_job,
                                                       fbe_job_service_error_type_t * job_error_type_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_virtual_drive_get_info_t                virtual_drive_info;
    fbe_api_job_service_drive_swap_in_request_t     drive_swap_in_request;
    fbe_status_t                                    job_status;


    mut_printf(MUT_LOG_TEST_STATUS, "%s:Initiate Proactive Spare swap-in request for vd_object_id:%d.\n", __FUNCTION__, vd_object_id);

    /* Get the virtual drive information.*/
    status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Build the swap-in request.*/
    drive_swap_in_request.vd_object_id = vd_object_id;
    drive_swap_in_request.original_object_id = virtual_drive_info.original_pvd_object_id;
    drive_swap_in_request.spare_object_id = FBE_OBJECT_ID_INVALID; /* spare algorithm will select the spare. */
    drive_swap_in_request.command_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    *job_error_type_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* issue the job service drive swap in request. */
    status = fbe_api_job_service_drive_swap_in_request(&drive_swap_in_request);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Proactive spare swap-in job sent successfully! ==\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)drive_swap_in_request.job_number);
 
    /* If requested, wait for the copy job to start.
     */
    if (b_wait_for_job == FBE_TRUE)
    {
        status = fbe_api_common_wait_for_job(drive_swap_in_request.job_number,
                                         FBE_TEST_UTILS_DRIVE_SWAP_TIMEOUT_MS,
                                         job_error_type_p,
                                         &job_status,
                                         NULL);

        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to swap in Proactive Spare");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Proactive Spare swap in failed");
    }

    return status;
}


/*!**************************************************************
 * fbe_test_sep_util_user_copy
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to start
 *  user copy.
 * 
 * @param vd_object_id  - virtual drive object-id.
 * @param pvd_object_id - pvd object id of destenation drive
 * @param b_wait_for_job - FBE_TRUE - wait for job completion
 * @param job_error_type_p  - pointer to job error code
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 03/21/2011 - Created. Maya Dagon
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_user_copy(fbe_object_id_t vd_object_id,
                                         fbe_object_id_t pvd_object_id,
                                         fbe_bool_t b_wait_for_job,
                                         fbe_job_service_error_type_t * job_error_type_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_job_service_drive_swap_in_request_t drive_swap_in_request;
    fbe_api_virtual_drive_get_info_t            virtual_drive_info;
    fbe_status_t                                job_status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s:Initiate user copy request for vd_object_id:%d.\n", __FUNCTION__, vd_object_id);

    /* Get the virtual drive information.*/
    status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    drive_swap_in_request.vd_object_id = vd_object_id;
    drive_swap_in_request.original_object_id = virtual_drive_info.original_pvd_object_id;
    drive_swap_in_request.spare_object_id = pvd_object_id; 
    drive_swap_in_request.command_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    *job_error_type_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* issue the job service  request. */
    status = fbe_api_job_service_drive_swap_in_request(&drive_swap_in_request);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== User copy job sent successfully! ==\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)drive_swap_in_request.job_number);
 
    /* If requested, wait for the copy job to start.
     */
    if (b_wait_for_job == FBE_TRUE)
    {
        status = fbe_api_common_wait_for_job(drive_swap_in_request.job_number,
                                             FBE_TEST_UTILS_DRIVE_SWAP_TIMEOUT_MS,
                                             job_error_type_p,
                                             &job_status,
                                             NULL);
    
        MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to swap in Proactive Spare");
        MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Proactive Spare swap in failed");
    }
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_update_permanent_spare_trigger_timer()
 ****************************************************************
 * @brief
 *  This function sends a request to job service to change the
 *  permanent spare trigger timer in configuration. 
 * 
 * @param permanent_spare_trigger_timer  - permanent spare trigger
 *                                         timer in seconds.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 09/16/2010 - Created. Dhaval Patel
 * 05/27/2011 - Modified Harshal Wanjari
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_update_permanent_spare_trigger_timer(fbe_u64_t permanent_spare_trigger_timer)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    ) 
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Send to the active sp.
     */
    status = fbe_api_update_permanent_spare_swap_in_time(permanent_spare_trigger_timer);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_check_permanent_spare_trigger_timer(permanent_spare_trigger_timer);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If we are in dualsp just validate it on the peer
     */
    if (b_send_to_both)
    {
        /* Should have automatically updated the timer on peer
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_test_sep_util_check_permanent_spare_trigger_timer(permanent_spare_trigger_timer);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    return status;
}
/**************************************************************
 * end fbe_test_sep_util_update_permanent_spare_trigger_timer()
 **************************************************************/

/*!**************************************************************
 * fbe_test_sep_util_configure_drive_as_hot_spare()
 ****************************************************************
 * @brief
 *  This function is used to configure the drive as hot spare.
 * 
 * @param port
 * @param encl
 * @param slot
 *
 * @return fbe_status - status of disable request
 *
 * @author
 *  11/11/2010 - Created. Rob Foley
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_configure_drive_as_hot_spare(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 pvd_object_id;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_sep_util_hot_spare_dummy_info_t dummy_info;

    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    status = fbe_api_provision_drive_get_obj_id_by_location(port, encl, slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Configure the %d_%d_%d object: 0x%x as Hot Spare.", 
               __FUNCTION__, port, encl, slot, pvd_object_id);

    /* Check if the provision drive has already been configured as a hot-spare.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, 
                                             &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s pvd obj: 0x%x (%d_%d_%d) already configured as spare.",
                   __FUNCTION__,  pvd_object_id,
                   port,
                   encl,
                   slot);
   }
   else
   {
       dummy_info.magic_num = 0xFBE8FBE8;
       dummy_info.object_id = pvd_object_id;
       dummy_info.port = port;
       dummy_info.encl = encl;
       dummy_info.slot = slot;

       fbe_test_sep_util_configure_pvd_as_hot_spare(pvd_object_id, 
                                             (fbe_u8_t *)&dummy_info, 
                                                 sizeof(fbe_test_sep_util_hot_spare_dummy_info_t),
                                                 FBE_FALSE /* Do drives should be in-use */);
   }

   return status;
}

/*!**************************************************************
 * fbe_test_sep_util_wait_for_pvd_state()
 ****************************************************************
 * @brief
 *  This function is used to wait for a pvd to reach a given lifecycle state.
 * 
 * @param port
 * @param encl
 * @param slot
 * @param lifecycle state - State we want to wait for.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 *  11/12/2010 - Created. Rob Foley
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_wait_for_pvd_state(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot,
                                     fbe_lifecycle_state_t state,
                                     fbe_u32_t max_wait_msecs)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t pvd_object_id;
    fbe_u32_t remaining_msecs = max_wait_msecs;

    /* We continue looping until we get the pvd object id. 
     */
    while (remaining_msecs > 0)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(port, encl, slot, &pvd_object_id);

        /* The edge connection may be delayed... so it is recommended to wait for it.. */

        if (status == FBE_STATUS_OK) 
        {
            /* Now wait for the lifecycle state of the pvd object.
             */
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                  pvd_object_id, state,
                                                                                  remaining_msecs,
                                                                                  FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        fbe_api_sleep(500);
        remaining_msecs -= 500;
    }
    return status;
}
/**************************************
 * end fbe_test_sep_util_wait_for_pvd_state()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_sum_verify_results_data()
 ****************************************************************
 * @brief
 *  This function adds up all the error counts in the specified 
 *  verify results data.
 * 
 * @param in_verify_results_p   - pointer to verify results data
 * @param out_count_p           - pointer to count of reported errors
 *
 * @return void
 *
 * @author
 *  06/21/2010 - Created. Ashwin
 *
 ****************************************************************/
void fbe_test_sep_util_sum_verify_results_data(fbe_lun_verify_counts_t* in_verify_results_p,
                                             fbe_u32_t* out_count_p)
{
    fbe_u32_t   count;

    count = in_verify_results_p->correctable_coherency;
    count += in_verify_results_p->correctable_lba_stamp;
    count += in_verify_results_p->correctable_multi_bit_crc;
    count += in_verify_results_p->correctable_shed_stamp;
    count += in_verify_results_p->correctable_single_bit_crc;
    count += in_verify_results_p->correctable_time_stamp;
    count += in_verify_results_p->correctable_write_stamp;
    count += in_verify_results_p->uncorrectable_coherency;
    count += in_verify_results_p->uncorrectable_lba_stamp;
    count += in_verify_results_p->uncorrectable_multi_bit_crc;
    count += in_verify_results_p->uncorrectable_shed_stamp;
    count += in_verify_results_p->uncorrectable_single_bit_crc;
    count += in_verify_results_p->uncorrectable_time_stamp;
    count += in_verify_results_p->uncorrectable_write_stamp;

    *out_count_p = count;
    return;

}   // End fbe_test_sep_util_sum_verify_results_data()


/*!**************************************************************
 * fbe_test_sep_util_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param position_to_remove - The position in each rg to remove.               
 *
 * @return None.
 *
 *
 ****************************************************************/

void fbe_test_sep_util_remove_drives(fbe_u32_t position_to_remove,
                                    fbe_test_rg_configuration_t* in_current_rg_config_p )
{
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;

        /* Remove a drive from the 520 rg.
         */
    drive_to_remove_p = &(rg_config_p->rg_disk_set[position_to_remove]);
        mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: Removing drive in position: 0x%x", 
               __FUNCTION__, position_to_remove);
        

    if(fbe_test_util_is_simulation())
    {
        if (position_to_remove > (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "position to remove %d > (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1 %d",
                       position_to_remove, (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1));
            MUT_FAIL();
        }
        else {
           status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus,
                                                drive_to_remove_p->enclosure,
                                                drive_to_remove_p->slot,
                                                &drive_handle[position_to_remove]);
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus,
                                             drive_to_remove_p->enclosure,
                                             drive_to_remove_p->slot
                                             );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    }

    return;
}
/******************************************
 * end fbe_test_sep_util_remove_drives()
 ******************************************/


/*!**************************************************************
 * fbe_test_sep_util_insert_drives()
 ****************************************************************
 * @brief
 *  insert drives, one per raid group.
 *
 * @param position_to_insert - The array index to insert.               
 *
 * @return None.
 *
 *
 ****************************************************************/

void fbe_test_sep_util_insert_drives(fbe_u32_t position_to_insert,
                                    fbe_test_rg_configuration_t* in_current_rg_config_p)
{
   fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
   fbe_status_t status;
    
   fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;
    

    /* VDs are the first objects to be created.
    */
    //fbe_object_id_t vd_object_id = rg_config_p->first_object_id + position_to_insert;

    /* Insert a drive from the 520 rg.
     */
    drive_to_insert_p = &(rg_config_p->rg_disk_set[position_to_insert]);

    //mut_printf(MUT_LOG_TEST_STATUS, 
    //           "%s: inserting drive vd: %d\n", __FUNCTION__, vd_object_id);
    
    if(fbe_test_util_is_simulation())
    {
        if (position_to_insert > (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "position to remove %d > (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1 %d",
                       position_to_insert, (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1));
            MUT_FAIL();
        }
        else {
           status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus,
                                                    drive_to_insert_p->enclosure,
                                                    drive_to_insert_p->slot,
                                                    drive_handle[position_to_insert]);                                                
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus,
                                                    drive_to_insert_p->enclosure,
                                                    drive_to_insert_p->slot
                                                    );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    }

    return;
}
/******************************************
 * end fbe_test_sep_util_insert_drives()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_for_pdo_ready()
 ****************************************************************
 * @brief
 *
 * @param position_to_insert - The array index to insert.               
 *
 * @return None.
 *
 *
 ****************************************************************/

void fbe_test_sep_util_wait_for_pdo_ready(fbe_u32_t position_to_insert,
                                             fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;
    

    /* Insert a drive from the 520 rg.
    */
    drive_to_insert_p = &(rg_config_p->rg_disk_set[position_to_insert]);
    
    
        /* Verify the PDO and LDO are in the READY state
         */
        status = fbe_test_pp_util_verify_pdo_state(drive_to_insert_p->bus, 
                                                   drive_to_insert_p->enclosure,
                                                   drive_to_insert_p->slot,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   FBE_TEST_SEP_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
       
    return;

}// End fbe_test_sep_util_wait_for_pdo_ready


/*!**************************************************************
 * fbe_test_sep_get_active_target_id
 ****************************************************************
 * @brief
 *  This function identifies the SP that is currently defined
 *  as Active. If neither SP is currently defined as active,
 *  the output parameter is set to "invalid server".
 * 
 * @param out_active_sp_p - pointer to the active SP identifier               
 *
 * @return  void
 * 
 * @author
 *  07/2010 - Created. Susan Rundbaken (rundbs)
 * 
 ****************************************************************/

void fbe_test_sep_get_active_target_id(fbe_sim_transport_connection_target_t* out_active_sp_p)
{
    fbe_status_t                            spa_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_status_t                            spb_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t   target_invoked_on = FBE_SIM_SP_A;
    fbe_cmi_service_get_info_t              spa_cmi_info;
    fbe_cmi_service_get_info_t              spb_cmi_info;
    fbe_u32_t                               total_ms_waited = 0;

    /* Wait for either the verify to start or timeout.
     */
    *out_active_sp_p = FBE_SIM_INVALID_SERVER;
    while ((*out_active_sp_p == FBE_SIM_INVALID_SERVER) &&
           (total_ms_waited < FBE_TEST_SEP_WAIT_MSEC)      ) {
        /* Get CMI info for each target */
        target_invoked_on = fbe_api_sim_transport_get_target_server();
        if (fbe_api_transport_is_target_initted(FBE_SIM_SP_A)) {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            spa_status = fbe_api_cmi_service_get_info(&spa_cmi_info);
        }

        if (fbe_api_transport_is_target_initted(FBE_SIM_SP_B))  {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            spb_status = fbe_api_cmi_service_get_info(&spb_cmi_info);
        }
        fbe_api_sim_transport_set_target_server(target_invoked_on);

        /* Check for the active state and set the output parameter accordingly */
        if ((spa_status == FBE_STATUS_OK)                   &&
            (spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE)    ) {
            *out_active_sp_p = FBE_SIM_SP_A;
            return;
        }

        if ((spb_status == FBE_STATUS_OK)                   &&
            (spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE)    ) {
            *out_active_sp_p = FBE_SIM_SP_B;
            return;
        }

        /* If got this far, the active SP is not set */
        *out_active_sp_p = FBE_SIM_INVALID_SERVER;
        fbe_api_sleep(100);
        total_ms_waited += 100;
    }

    /* Failed to get a valid target.
     */
    MUT_ASSERT_TRUE_MSG(*out_active_sp_p != FBE_SIM_INVALID_SERVER, "Failed to get a valid target SP");
    return;

} /* End fbe_test_sep_get_active_target_id() */

/*!**************************************************************
 * fbe_test_sep_util_raid_group_get_flush_error_counters()
 ****************************************************************
 * @brief
 *  Function to get the raid group write log flush error counters.
 * 
 * @param in_raid_group_object_id - id of the raid group
 * @param out_flush_error_counters_p - pointer to the buffer to place the counters in
 *
 * @return fbe_status - status of request
 *
 * @author
 *  5/11/2012 - Created. Dave Agans
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_raid_group_get_flush_error_counters(fbe_object_id_t in_raid_group_object_id, 
                                                                   fbe_raid_flush_error_counts_t* out_flush_error_counters_p)
{
    return fbe_api_raid_group_get_flush_error_counters(in_raid_group_object_id,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       out_flush_error_counters_p);    
}
/* End fbe_test_sep_util_raid_group_get_flush_error_counters() */

/*!**************************************************************
 * fbe_test_sep_util_raid_group_clear_flush_error_counters()
 ****************************************************************
 * @brief
 *  Function to clear the raid group write log flush error counters.
 * 
 * @param in_raid_group_object_id - id of the raid group
 *
 * @return fbe_status - status of request
 *
 * @author
 *  5/11/2012 - Created. Dave Agans
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_raid_group_clear_flush_error_counters(fbe_object_id_t in_raid_group_object_id)
{
    return fbe_api_raid_group_clear_flush_error_counters(in_raid_group_object_id, FBE_PACKET_FLAG_NO_ATTRIB);    
}
/* End fbe_test_sep_util_raid_group_clear_flush_error_counters() */

/*!**************************************************************
 * fbe_test_sep_number_of_ios_outstanding
 ****************************************************************
 * @brief
 *  This function identifies the SP that is currently defined
 *  as Active. If neither SP is currently defined as active,
 *  the output parameter is set to "invalid server".
 * 
 * @param out_active_sp_p - pointer to the active SP identifier               
 *
 * @return  void
 * 
 * @author
 *  08/19/2010 - Created. Ashwin Tamilarasan
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_number_of_ios_outstanding(fbe_object_id_t object_id,
                                                    UINT_32 *ios_outstanding_p)
{
    fbe_status_t                        status;
    fbe_api_raid_group_get_io_info_t    raid_group_io_info;

    fbe_zero_memory(&raid_group_io_info, sizeof(fbe_api_raid_group_get_io_info_t));
    *ios_outstanding_p = 0;

    status = fbe_api_raid_group_get_io_info(object_id, &raid_group_io_info);

    if (status == FBE_STATUS_OK)
    {
        *ios_outstanding_p = raid_group_io_info.outstanding_io_count;
    }

    return status;

} // End fbe_test_sep_number_of_ios_outstanding

/*!***************************************************************************
 *          fbe_test_sep_util_get_raid_group_io_info()
 *****************************************************************************
 *
 * @brief   This function gets the I/O information of the raid group specified.
 *          Currently this includes:
 *              o Outstanding I/O count
 *              o Quiesced I/O count
 *              o Is quiesced
 * 
 * @param   rg_object_id - Raid group object id
 * @param   raid_group_io_info_p - Pointer to riad group I/O information to update        
 *
 * @return  status
 * 
 * @author
 *  05/09/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_get_raid_group_io_info(fbe_object_id_t rg_object_id,
                                                      fbe_api_raid_group_get_io_info_t *raid_group_io_info_p)
{
    /* Zero the input
     */
    fbe_zero_memory(raid_group_io_info_p, sizeof(*raid_group_io_info_p));

    return (fbe_api_raid_group_get_io_info(rg_object_id, raid_group_io_info_p));

}
/************************************************* 
 * end fbe_test_sep_util_get_raid_group_io_info()
 *************************************************/

/*!*******************************************************************
 *          fbe_test_sep_utils_wait_for_base_config_clustered_flags()
 *********************************************************************
 * @brief
 *  Wait for this raid group's flags to reach the input value.
 * 
 *  We will check the flags first.
 *  If the flags match the expected value, we return immediately.
 *  If the flags are not what we expect, we will wait for
 *  half a second, before checking again.
 * 
 *  When the total wait time matches the input wait seconds
 *  we will return with failure.
 *
 * @param rg_object_id - Object id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  10/7/2010 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_utils_wait_for_base_config_clustered_flags(fbe_object_id_t rg_object_id,
                                                fbe_u32_t wait_seconds,
                                                fbe_base_config_clustered_flags_t base_config_clustered_flags,
                                                fbe_bool_t b_cleared)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * 1000;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (b_cleared)
        {
            /* Check the flag is cleared.
             */
            if ((rg_info.base_config_clustered_flags & base_config_clustered_flags) == 0)
            {
                /* Flags are cleared, just break out and return.
                 */
                break;
            }
        }
        else
        {
            /* Just check that the flag is set.
             */
            if ((rg_info.base_config_clustered_flags & base_config_clustered_flags) == base_config_clustered_flags)
            {
                /* Flags we expected are set, just break out and return.
                 */
                break;
            }
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: rg obj: 0x%x config flags: 0x%llx cleared: %d wait secs: %d timed-out\n", 
                   __FUNCTION__, rg_object_id,
		   (unsigned long long)base_config_clustered_flags,
		   b_cleared, wait_seconds);

        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/***************************************************************
 * end fbe_test_sep_utils_wait_for_base_config_clustered_flags()
 ***************************************************************/

/*!**************************************************************
 * fbe_test_sep_utils_wait_for_all_base_config_flags()
 ****************************************************************
 * @brief
 *  Wait for all raid group flags to reach the input value.
 * 
 *  We will check the flags first.
 *  If the flags match the expected value, we return immediately.
 *  If the flags are not what we expect, we will wait for
 *  half a second, before checking again.
 * 
 *  When the total wait time matches the input wait seconds
 *  we will return with failure.
 *
 * @param class_id - Class id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  08/13/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_utils_wait_for_all_base_config_flags(fbe_class_id_t class_id,    
                                                     fbe_u32_t wait_seconds,
                                                     fbe_base_config_clustered_flags_t flags,
                                                     fbe_bool_t b_cleared)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /*! First get all members of this class.
     * 
     *  @todo Remove system objects from list
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Iterate over each of the objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* Wait for the flags to match on this object.
         */
        status = fbe_test_sep_utils_wait_for_base_config_clustered_flags(obj_list_p[index],
                                                     wait_seconds,
                                                     flags, b_cleared);

        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}

/*!****************************************************************************
 *      fbe_test_sep_util_provision_drive_set_debug_flag
 ******************************************************************************
 *
 * @brief
 *    This function sets a debug trace flag for the specified provision drive. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @param   in_flag                              -  debug trace flag to set at object level
 *
 * @return  void
 *
 * @version
 *    09/07/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/

void fbe_test_sep_util_provision_drive_set_debug_flag(
                                    fbe_object_id_t in_provision_drive_object_id,
                                    fbe_provision_drive_debug_flags_t in_flag)
{

    fbe_status_t status;

    status = fbe_api_provision_drive_set_debug_flag(in_provision_drive_object_id, in_flag);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
} 
/**************************************************************
 * end fbe_test_sep_util_provision_drive_set_debug_flag()
 **************************************************************/


/*!****************************************************************************
 *      fbe_test_sep_util_provision_drive_set_class_debug_flag
 ******************************************************************************
 *
 * @brief
 *    This function sets a debug trace flag for the specified provision drive. 
 *
 *
 * @param   in_flag                 -  debug trace flag to set at class level
 *
 * @return  void
 *
 * @version
 *    09/07/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/

void fbe_test_sep_util_provision_drive_set_class_debug_flag(fbe_provision_drive_debug_flags_t in_flag)
{

    fbe_status_t status;

    status = fbe_api_provision_drive_set_class_debug_flag(in_flag);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
}


/****************************************************************
 * end fbe_test_sep_util_provision_drive_set_class_debug_flag()
 ****************************************************************/

/*!****************************************************************************
 *      fbe_test_sep_util_provision_drive_set_class_debug_flag
 ******************************************************************************
 *
 * @brief
 *    This function enables and verifies system power saving. 
 *
 *
 * @param   none
 *
 * @return  void
 *
 * @version
 *    11/16/2010 - Created. 
 *
 ******************************************************************************/
void fbe_test_sep_util_enable_system_power_saving(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    
    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);
}

/*!**************************************************************
 * fbe_test_sep_util_get_next_position_to_remove()
 ****************************************************************
 * @brief
 *  Remove the next position from the raid group.  Uses the removed
 *  information.
 *
 * @param rg_config_p - Pointer to raid group config          
 * @param removal_mode - Algorithm of drive remove to use.
 *
 * @return fbe_u32_t - Position to remove. If not positions are 
 *                     available, FBE_TEST_SEP_UTIL_INVALID_POSITION is
 *                     returned.
 * 
 * @note    Doesn't check for shutting down the raid group or not.
 *
 ****************************************************************/
fbe_u32_t fbe_test_sep_util_get_next_position_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t   remove_index = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   position_to_remove = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   can_be_removed[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t   random_position = FBE_TEST_SEP_UTIL_INVALID_POSITION;

    if ((rg_config_p->width - rg_config_p->num_removed) == 0)
    {

        /* There are no drives available to be removed.
        */
        return FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }

    /* Generate the can be removed array
     * First initialize all the positions to being present.
     */
    for (remove_index = 0; remove_index < rg_config_p->width; remove_index++)
    {
        can_be_removed[remove_index] = remove_index;
    }

    /* For each position found on the drives removed array, set that position to
     * removed in our local array.
     */
    for (remove_index = 0; remove_index < rg_config_p->num_removed; remove_index++)
    {
        fbe_u32_t   removed_position;

        /* Sanity check that the removed position is present in the array.
         */
        MUT_ASSERT_TRUE(remove_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION);

        removed_position = rg_config_p->drives_removed_array[remove_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)

        can_be_removed[removed_position] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }

    /* Check which removal mode is requested.
     */
    switch (removal_mode)
    {
        case FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL:
            /* Loop through the 'can be removed' array to find the first position that can be removed.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    /* Found a position to be removed.
                     * Set position_to_remove to the position to be removed.
                     */
                    position_to_remove = can_be_removed[remove_index];
                    break;
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_RANDOM:
            /* Pick a random number from the number of drives available to remove.
             * The number of drives available should be nonzero.
             */
            MUT_ASSERT_TRUE((rg_config_p->width - rg_config_p->num_removed) != 0);
            random_position = fbe_random() % (rg_config_p->width - rg_config_p->num_removed);

            /* Loop through until and skip 'random_position' number of available drives to
             * return a random drive.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    if (random_position == 0)
                    {
                        /* Found a position to be removed.
                         * Set position_to_remove to the position to be removed.
                         */
                        position_to_remove = can_be_removed[remove_index];
                        break;
                    }
                    else
                    {
                        random_position--;
                    }
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_SPECIFIC:
            /* Loop through the 'specific_drives_to_remove' array to find the first position that is not already removed.
             */
            remove_index = 0;
            for ( remove_index = 0; remove_index < rg_config_p->width; remove_index++)
            {                
                /* Found a position to be removed.
                 * Set position_to_remove to the position to be removed.
                 */
                position_to_remove = rg_config_p->specific_drives_to_remove[remove_index];                    
                if(position_to_remove == rg_config_p->drives_removed_array[remove_index])
                {                    
                    continue;
                }

               break;                               
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        default:
            /* Not a valid algorithm.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Invalid removal algorithm %d\n", 
                       __FUNCTION__, removal_mode);
            MUT_ASSERT_TRUE(FBE_FALSE);
            break;
    }

    MUT_ASSERT_TRUE(position_to_remove < rg_config_p->width);
    return position_to_remove;
}
/* end of fbe_test_sep_util_get_next_position_to_remove() */

/*!**************************************************************
 * fbe_test_sep_util_add_removed_position()
 ****************************************************************
 * @brief
 *  Add the removed position to the removed array
 *
 * @param rg_config_p - Pointer to raid group config          
 * @param position_to_remove - Position to add to the removed array
 * 
 * @return None
 * 
 * @note    Doesn't check for shutting down the raid group or not.
 *
 ****************************************************************/
void fbe_test_sep_util_add_removed_position(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t position_to_remove)
{
    fbe_u32_t drive_index;

    MUT_ASSERT_TRUE(position_to_remove < rg_config_p->width); 
    MUT_ASSERT_TRUE(rg_config_p->num_removed < rg_config_p->width); 
    for ( drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        if ( position_to_remove == rg_config_p->drives_removed_array[drive_index])
        {
            MUT_FAIL_MSG("this position is already in the drives removed array.\n");
        }
    }

    /* Add drive to removed array
     */
    rg_config_p->drives_removed_array[rg_config_p->num_removed] = position_to_remove;

    /* This position now needs a rebuild.
     * Save the history
     */
    drive_index = FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE;

    while (--drive_index != 0)
    {
        rg_config_p->drives_removed_history[drive_index] = rg_config_p->drives_removed_history[drive_index-1];
    }
    rg_config_p->drives_removed_history[drive_index] = position_to_remove;

    rg_config_p->num_removed++;

    return;
}
/* end of fbe_test_sep_util_add_removed_position() */

/*!**************************************************************
 * fbe_test_sep_get_drive_remove_history()
 ****************************************************************
 * @brief
 *  Retrieve the drive removal history for the provided test config
 *  and index. (0 is the last removal, 1 is the second to last, etc...)
 *
 * @param rg_config_p - Pointer to raid group config          
 * @param history_index - Index to the history
 *
 * @return fbe_u32_t - Position that was removed. If value is beyond
 *                     FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE, then
 *                     FBE_TEST_SEP_UTIL_INVALID_POSITION is
 *                     returned.
 * 
 ****************************************************************/
fbe_u32_t fbe_test_sep_get_drive_remove_history(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t history_index)
{
    if (history_index >= FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE)
    {
        return FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }

    return rg_config_p->drives_removed_history[history_index];
}
/* end of fbe_test_sep_get_drive_remove_history() */

/*!**************************************************************
 * fbe_test_sep_util_add_needing_spare_position()
 ****************************************************************
 * @brief
 *  Add the removed position to the needing spare array
 *
 * @param rg_config_p - Pointer to raid group config          
 * @param position_needing_spare - Position that needs a spare
 * 
 * @return None
 * 
 * @note    Doesn't check for shutting down the raid group or not.
 *
 ****************************************************************/
void fbe_test_sep_util_add_needing_spare_position(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_u32_t position_needing_spare)
{
    fbe_u32_t drive_index;

    MUT_ASSERT_TRUE(position_needing_spare < rg_config_p->width); 
    MUT_ASSERT_TRUE(rg_config_p->num_needing_spare < rg_config_p->width); 
    for ( drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        if ( position_needing_spare == rg_config_p->drives_needing_spare_array[drive_index])
        {
            MUT_FAIL_MSG("this position is already in the drives needing spare array.\n");
        }
    }
    /* Add drive to needing spare array
     */
    rg_config_p->drives_needing_spare_array[rg_config_p->num_needing_spare] = position_needing_spare;
    rg_config_p->num_needing_spare++;

    return;
}
/* end of fbe_test_sep_util_add_needing_spare_position() */

/*!***************************************************************************
 *          fbe_test_sep_util_get_next_position_to_insert()
 *****************************************************************************
 * 
 * @brief   Find the next position to insert using the removed array.
 *
 * @param   rg_config_p - Pointer to raid group config          
 *
 * @return  fbe_u32_t - Position to insert
 *
 *****************************************************************************/
fbe_u32_t fbe_test_sep_util_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   position_to_insert = FBE_TEST_SEP_UTIL_INVALID_POSITION;

    /*! @note This assumes that drives have been removed
     */
    MUT_ASSERT_TRUE((fbe_s32_t)rg_config_p->num_removed > 0);
    MUT_ASSERT_TRUE(rg_config_p->num_removed <= rg_config_p->width); 
     
    /* Just increment the number of the last drive we removed.
     */
    position_to_insert = rg_config_p->drives_removed_array[rg_config_p->num_removed - 1]; 
    MUT_ASSERT_TRUE(position_to_insert < rg_config_p->width); 
    return position_to_insert;
}
/* end of fbe_test_sep_util_get_next_position_to_insert() */

/*!***************************************************************************
 *          fbe_test_sep_util_get_random_position_to_insert()
 *****************************************************************************
 * 
 * @brief   Find the next position to insert using the removed array.
 *
 * @param   rg_config_p - Pointer to raid group config          
 *
 * @return  fbe_u32_t - Position to insert
 *
 *****************************************************************************/
fbe_u32_t fbe_test_sep_util_get_random_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   position_to_insert = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   removed_index;
    fbe_u32_t   random_position1 = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   random_position2 = FBE_TEST_SEP_UTIL_INVALID_POSITION;

    /*! @note This assumes that drives have been removed
     */
    MUT_ASSERT_TRUE((fbe_s32_t)rg_config_p->num_removed > 0);
    MUT_ASSERT_TRUE(rg_config_p->num_removed <= rg_config_p->width); 

    /* Generate (2) (current the maximum number of drives that can be removed
     * before going shutdown is 2) random numbers.
     */
    random_position1 = fbe_random() % rg_config_p->width;
    random_position2 = fbe_random() % rg_config_p->width;
    for (removed_index = 0; removed_index < rg_config_p->num_removed; removed_index++)
    {
        fbe_u32_t   removed_position;

        /* Attempt to match (1) of the (2) random positions with a removed
         * position
         */
        MUT_ASSERT_TRUE(removed_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[removed_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION);
        removed_position = rg_config_p->drives_removed_array[removed_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)
        if (random_position1 == removed_position) 
        {
            position_to_insert = random_position1;
            break; 
        }
        else if (random_position2 == removed_position) 
        {
            position_to_insert = random_position2;
            break; 
        }
    }

    /* If the random method didn't work just insert the first removed position
     */
    if (position_to_insert == FBE_TEST_SEP_UTIL_INVALID_POSITION)
    {
        position_to_insert = rg_config_p->drives_removed_array[rg_config_p->num_removed - 1]; 
    }
    MUT_ASSERT_TRUE(position_to_insert < rg_config_p->width); 
    return position_to_insert;
}
/* end of fbe_test_sep_util_get_random_position_to_insert() */

/*!***************************************************************************
 *          fbe_test_sep_util_removed_position_inserted()
 *****************************************************************************
 * 
 * @brief   Updated the drive removed array with the inserted position
 *
 * @param   rg_config_p - Pointer to raid group config          
 * @param   inserted_position - Position that was inserted
 * 
 * @return  None
 *
 *****************************************************************************/
void fbe_test_sep_util_removed_position_inserted(fbe_test_rg_configuration_t *rg_config_p,
                                                 fbe_u32_t inserted_position)
{
    fbe_u32_t drive_index;
    fbe_bool_t b_found = FBE_FALSE;
    for ( drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        if ( inserted_position == rg_config_p->drives_removed_array[drive_index])
        {
            b_found = FBE_TRUE;
            rg_config_p->drives_removed_array[drive_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
        }
    }
    if (b_found == FBE_FALSE)
    {
        MUT_FAIL_MSG("position to remove is not found.\n");
    }
    rg_config_p->num_removed--;
    return;
}
/* end of fbe_test_sep_util_removed_position_inserted() */

/*!***************************************************************************
 *          fbe_test_sep_util_delete_needing_spare_position()
 *****************************************************************************
 * 
 * @brief   Updated the drive needing spare array with the spared position
 *
 * @param   rg_config_p - Pointer to raid group config          
 * @param   inserted_position - Position that was inserted
 * 
 * @return  None
 *
 *****************************************************************************/
void fbe_test_sep_util_delete_needing_spare_position(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t spared_position)
{
    fbe_u32_t drive_index;
    fbe_bool_t b_found = FBE_FALSE;
    for ( drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        if ( spared_position == rg_config_p->drives_needing_spare_array[drive_index])
        {
            b_found = FBE_TRUE;
            rg_config_p->drives_needing_spare_array[drive_index] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
        }
    }
    if (b_found == FBE_FALSE)
    {
        MUT_FAIL_MSG("position to delete is not found.\n");
    }
    rg_config_p->num_needing_spare--;
    return;
}
/* end of fbe_test_sep_util_delete_needing_spare_position() */

fbe_u32_t fbe_test_sep_util_get_num_needing_spare(fbe_test_rg_configuration_t *rg_config_p)
{
    MUT_ASSERT_TRUE(rg_config_p->num_needing_spare <= rg_config_p->width); 
    return rg_config_p->num_needing_spare;
}

fbe_u32_t fbe_test_sep_util_get_needing_spare_position_from_index(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t needing_spare_index)
{
    MUT_ASSERT_TRUE(rg_config_p->num_needing_spare <= rg_config_p->width); 
    MUT_ASSERT_TRUE(needing_spare_index < rg_config_p->width); 
    MUT_ASSERT_TRUE(rg_config_p->drives_needing_spare_array[needing_spare_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION); 
    return rg_config_p->drives_needing_spare_array[needing_spare_index];
}

fbe_u32_t fbe_test_sep_util_get_num_removed_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    MUT_ASSERT_TRUE(rg_config_p->num_removed <= rg_config_p->width); 
    return rg_config_p->num_removed;
}

/*!**************************************************************
 * fbe_test_sep_util_get_io_seconds()
 ****************************************************************
 * @brief
 *  Return the number of seconds to perform I/Os.
 *
 * @param  - None.               
 *
 * @return - Number of I/Os that we should perform in this test.
 *
 ****************************************************************/
fbe_u32_t fbe_test_sep_util_get_io_seconds(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t io_seconds;

    if (test_level > 0)
    {
        io_seconds = FBE_TEST_DEFAULT_IO_SECONDS_EXTENDED;
    }
    else
    {
        io_seconds = FBE_TEST_DEFAULT_IO_SECONDS_QUAL;
    }

    if (fbe_test_sep_util_get_cmd_option_int("-fbe_io_seconds", &io_seconds))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using io time of: %d seconds", io_seconds);
    }
    return io_seconds;
}
/******************************************
 * end fbe_test_sep_util_get_io_seconds() 
 ******************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_all_lun_objects_ready()
 *****************************************************************************
 * 
 * @brief   Wait for the lun objects for a particular raid group to become
 *          ready.
 * 
 * @param   rg_config_p - Pointer to (1) raid group configuration with luns
 * @param   target_sp_of_luns - Target sp that the lun objects reside on
 *
 * @return  none
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_all_lun_objects_ready(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_transport_connection_target_t target_sp_of_luns)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           width;
    fbe_object_id_t                     rg_object_id;
    fbe_u32_t                           lun_index;
    fbe_object_id_t                     lun_object_id;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   original_target = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   peer_target = FBE_TRANSPORT_SP_B;

    /* First get the sp id this was invoked on.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    original_target = target_invoked_on;

    /* Determine the peer sp (for debug)
     */
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A; 

    /*! @note Both the object_id and the identifier are guaranteed to be the 
     *        same on both sps!
     */
    width = rg_config_p->width;
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Wait for all lun objects to be ready before continuing.
     */
    for (lun_index = 0; lun_index < rg_config_p->number_of_luns_per_rg; lun_index++)
    {
        fbe_lun_number_t lun_number = rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

        /* Wait up to FBE_TEST_WAIT_OBJECT_TIMEOUT_MS seconds for the lun object to become ready.
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, rg_config_p->raid_group_id);
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(lun_object_id, 
                                                                            target_sp_of_luns,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            FBE_TEST_WAIT_OBJECT_TIMEOUT_MS, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Success, validate that we haven't changed targets
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(original_target, target_invoked_on);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 *          fbe_test_sep_util_validate_no_raid_errors()
 ****************************************************************
 * @brief
 *  Validate that no raid errors were encountered.
 *
 * @param  None
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_validate_no_raid_errors(void)
{
    fbe_status_t status;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;

    /* validate no errors encountered.
     */
    status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (sector_trace_counters.total_invocations)
    {
        fbe_api_sector_trace_display_counters(FBE_TRUE);
        MUT_FAIL_MSG("Unexpected raid errors found, see above counters");
    }
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_validate_no_raid_errors_both_sps()
 ****************************************************************
 * @brief
 *  Make sure we saw no raid errors on both SPs.
 *
 * @param  None
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_validate_no_raid_errors_both_sps(void)
{
    fbe_test_sep_util_validate_no_raid_errors();
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_validate_no_raid_errors();
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return;
}

/*!**************************************************************
 *          fbe_test_sep_util_set_error_injection_test_mode()
 ****************************************************************
 * @brief
 *  mark the test as error injection type
 *
 * @param   b_set_err_inj_mode - FBE_TRUE - error injection mode
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_set_error_injection_test_mode(fbe_bool_t b_set_err_inj_mode)
{
    /* If enabling/disabling error injection mode log a message
     */
    if (b_set_err_inj_mode != fbe_test_sep_util_error_injection_test_mode)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Changing error injection mode from: %d to %d", 
                   __FUNCTION__, fbe_test_sep_util_error_injection_test_mode, b_set_err_inj_mode); 
        fbe_test_sep_util_error_injection_test_mode = b_set_err_inj_mode;
    }

    return;
}
/*!**************************************************************
 *          fbe_test_sep_util_get_error_injection_test_mode()
 ****************************************************************
 * @brief
 *  Simply determine if a test is doing error injection or not.
 *
 * @param  None
 *
 * @return fbe_bool_t - FBE_TRUE if test does error injection
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_get_error_injection_test_mode(void)
{
    return (fbe_test_sep_util_error_injection_test_mode);
}

/*!**************************************************************
 *          fbe_test_sep_util_set_dualsp_test_mode()
 ****************************************************************
 * @brief
 *  Simply set the dualsp mode to the value passed
 *
 * @param   b_set_dualsp_mode - FBE_TRUE - Enabled dualsp mode
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_set_dualsp_test_mode(fbe_bool_t b_set_dualsp_mode)
{
    /* If enabling/disabling dualsp mode log a message
     */
    if (b_set_dualsp_mode != fbe_test_sep_util_dualsp_test_mode)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Changing dualsp mode from: %d to %d", 
                   __FUNCTION__, fbe_test_sep_util_dualsp_test_mode, b_set_dualsp_mode); 
        fbe_test_sep_util_dualsp_test_mode = b_set_dualsp_mode;
    }

    return;
}
/*!**************************************************************
 *          fbe_test_sep_util_get_encryption_test_mode()
 ****************************************************************
 * @brief
 *  Simply determine if dualsp mode is enabled or not.
 *
 * @param  None
 *
 * @return fbe_bool_t - FBE_TRUE if dualsp mode is enabled
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_get_encryption_test_mode(void)
{
    return (fbe_test_sep_util_encryption_test_mode);
}
/*!**************************************************************
 *          fbe_test_sep_util_set_encryption_test_mode()
 ****************************************************************
 * @brief
 *  Simply set the dualsp mode to the value passed
 *
 * @param   b_set_dualsp_mode - FBE_TRUE - Enabled dualsp mode
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_set_encryption_test_mode(fbe_bool_t b_set_encrypted_mode)
{
    /* If enabling/disabling dualsp mode log a message
     */
    if (b_set_encrypted_mode != fbe_test_sep_util_encryption_test_mode)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Changing dualsp mode from: %d to %d", 
                   __FUNCTION__, fbe_test_sep_util_encryption_test_mode, b_set_encrypted_mode); 
        fbe_test_sep_util_encryption_test_mode = b_set_encrypted_mode;
    }

    return;
}
/*!**************************************************************
 *          fbe_test_sep_util_get_dualsp_test_mode()
 ****************************************************************
 * @brief
 *  Simply determine if dualsp mode is enabled or not.
 *
 * @param  None
 *
 * @return fbe_bool_t - FBE_TRUE if dualsp mode is enabled
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_get_dualsp_test_mode(void)
{
    return (fbe_test_sep_util_dualsp_test_mode);
}


/*!**************************************************************
 *          fbe_test_sep_util_set_remove_on_both_sps_mode()
 ****************************************************************
 * @brief
 *  Simply set if we should remove drives on both SPs or not.
 *
 * @param   b_set_remove_both_sps_mode - FBE_TRUE - Enable remove on both SPs
 *                             FBE_FALSE - Remove on just one SP.
 *
 * @return  None
 *
 ****************************************************************/
void fbe_test_sep_util_set_remove_on_both_sps_mode(fbe_bool_t b_set_remove_both_sps_mode)
{
    /* If enabling/disabling dualsp mode log a message
     */
    if (b_set_remove_both_sps_mode != fbe_test_sep_util_should_remove_on_both_sps_mode)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Changing dualsp mode from: %d to %d", 
                   __FUNCTION__, fbe_test_sep_util_dualsp_test_mode, b_set_remove_both_sps_mode); 
        fbe_test_sep_util_should_remove_on_both_sps_mode = b_set_remove_both_sps_mode;
    }

    return;
}
/*!**************************************************************
 *          fbe_test_sep_util_should_remove_on_both_sps()
 ****************************************************************
 * @brief
 *  Simply determine if we should pull drives on one or both SPs.
 *
 * @param  None
 *
 * @return fbe_bool_t - FBE_TRUE if we should pull on both SPs.
 *
 ****************************************************************/
fbe_bool_t fbe_test_sep_util_should_remove_on_both_sps(void)
{
    return (fbe_test_sep_util_should_remove_on_both_sps_mode);
}

void fbe_test_sep_util_print_dps_statistics(void)
{
    fbe_transport_connection_target_t   original_target;
    fbe_memory_dps_statistics_t memory_dps_statistics;
    fbe_status_t status;

    /* First get the sp id this was invoked on. */
    original_target = fbe_api_transport_get_target_server();

    if(fbe_test_sep_util_dualsp_test_mode){
        /* Set target to SPA */
        status = fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "SPA DPS statistics");
    status = fbe_api_dps_memory_get_dps_statistics(&memory_dps_statistics, FBE_PACKAGE_ID_SEP_0);
    sep_util_print_dps_statistics(&memory_dps_statistics);

    if(fbe_test_sep_util_dualsp_test_mode){
        /* Set target to SPB */
        status = fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "SPB DPS statistics");
        status = fbe_api_dps_memory_get_dps_statistics(&memory_dps_statistics, FBE_PACKAGE_ID_SEP_0);
        sep_util_print_dps_statistics(&memory_dps_statistics);

        /* Set target back to original */
        status = fbe_api_transport_set_target_server(original_target);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}

static void sep_util_print_dps_statistics(fbe_memory_dps_statistics_t * memory_dps_statistics)
{
    fbe_u64_t total_count;
    fbe_u32_t pool_index;
    fbe_cpu_id_t cpu_id;

    total_count = 0;
    for(pool_index = FBE_MEMORY_DPS_QUEUE_ID_MAIN; pool_index < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_index++) {
        for (cpu_id = 0; cpu_id < FBE_CPU_ID_MAX; cpu_id++) {
            total_count += memory_dps_statistics->request_count[pool_index][cpu_id];
            total_count += memory_dps_statistics->request_data_count[pool_index][cpu_id];
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Total request count %lld ", (long long)total_count);

    total_count = 0;
    for(pool_index = FBE_MEMORY_DPS_QUEUE_ID_MAIN; pool_index < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_index++) {
        for (cpu_id = 0; cpu_id < FBE_CPU_ID_MAX; cpu_id++) {
            total_count += memory_dps_statistics->deferred_count[pool_index][cpu_id];
            total_count += memory_dps_statistics->deferred_data_count[pool_index][cpu_id];
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Total deferred count %lld ", (long long)total_count);

}

void fbe_test_sep_util_print_metadata_statistics(fbe_object_id_t object_id)
{
    fbe_transport_connection_target_t   original_target;
    fbe_api_base_config_get_metadata_statistics_t  metadata_statistics;
    fbe_status_t status;

    /* First get the sp id this was invoked on. */
    original_target = fbe_api_transport_get_target_server();

    if(fbe_test_sep_util_dualsp_test_mode){
        /* Set target to SPA */
        status = fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "SPA metadata statistics ID: %d", object_id);
    status = fbe_api_base_config_get_metadata_statistics(object_id, &metadata_statistics);
    sep_util_print_metadata_statistics(&metadata_statistics);

    if(fbe_test_sep_util_dualsp_test_mode){
        /* Set target to SPB */
        status = fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "SPB metadata statistics ID: %d", object_id);
        status = fbe_api_base_config_get_metadata_statistics(object_id, &metadata_statistics);
        sep_util_print_metadata_statistics(&metadata_statistics);

        /* Set target back to original */
        status = fbe_api_transport_set_target_server(original_target);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}

static void sep_util_print_metadata_statistics(fbe_api_base_config_get_metadata_statistics_t * metadata_statistics)
{
    mut_printf(MUT_LOG_TEST_STATUS, "stripe_lock_count %lld ",
	       (long long)metadata_statistics->stripe_lock_count);
    mut_printf(MUT_LOG_TEST_STATUS, "local_collision_count %lld ",
	       (long long)metadata_statistics->local_collision_count);
    mut_printf(MUT_LOG_TEST_STATUS, "peer_collision_count %lld ",
	       (long long)metadata_statistics->peer_collision_count);
    mut_printf(MUT_LOG_TEST_STATUS, "cmi_message_count %lld ",
	       (long long)metadata_statistics->cmi_message_count);

}

/*!**************************************************************
 * fbe_test_sep_util_set_sector_trace_flags()
 ****************************************************************
 * @brief
 *  Initialize the error flags inside section trace.
 *
 * @param new_err_flags - new error flags.               
 *
 * @return status
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_sector_trace_flags(fbe_sector_trace_error_flags_t new_error_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Loop until we find the table number or we hit the terminator.
     */
    status = fbe_api_sector_trace_set_trace_flags(new_error_flags,FBE_FALSE);
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_set_sector_trace_debug_flags()
 ****************************************************************
 * @brief
 *  Initialize sector trace debug flag.
 *
 * @param new_dbg_flag - new debug flags.               
 *
 * @return status
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_sector_trace_debug_flags(fbe_sector_trace_error_flags_t new_dbg_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Loop until we find the table number or we hit the terminator.
     */
    status = fbe_api_sector_trace_set_stop_on_error_flags(new_dbg_flags,0,FBE_FALSE);
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_set_sector_trace_stop_on_error()
 ****************************************************************
 * @brief
 *  Initialize sector trace debug flag stop on error.
 *
 * @param stop_on_err - FBE_TRUE, stop on error.               
 *
 * @return status
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_sector_trace_stop_on_error(fbe_bool_t stop_on_err)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Loop until we find the table number or we hit the terminator.
     */
    status = fbe_api_sector_trace_set_stop_on_error(stop_on_err);
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_set_sector_trace_level()
 ****************************************************************
 * @brief
 *  Initialize the raid group config struct.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_sector_trace_level(fbe_sector_trace_error_level_t new_error_level)
{
    fbe_status_t status = FBE_STATUS_OK;    
    status = fbe_api_sector_trace_set_trace_level(new_error_level,FBE_FALSE);
    return status;
}
/*!****************************************************************************
 *  fbe_test_sep_get_data_drives_for_raid_group
 ******************************************************************************
 *
 * @brief
 *   This function returns number of data disks for given raid group configuration.
 *
 * @return  data_drives
 *
 * @author
 *    2/14/2011 - Created. Mahesh Agarkar
 *
 *****************************************************************************/
fbe_u32_t fbe_test_sep_get_data_drives_for_raid_group_config(fbe_test_rg_configuration_t *rg_config)
{
    fbe_u32_t data_drives = 0;

    switch(rg_config->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID1:
        data_drives = 1;
        break;

        case FBE_RAID_GROUP_TYPE_RAID10:
        data_drives = 1;
        break;

        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID3:
        data_drives = rg_config->width - 1;
        break;

        case FBE_RAID_GROUP_TYPE_RAID0:
        data_drives = rg_config->width; 
        break;

        case FBE_RAID_GROUP_TYPE_RAID6:
        data_drives = rg_config->width - 2;
        break;

        case FBE_RAID_GROUP_TYPE_SPARE:
        data_drives = 1;
        break;

        case FBE_RAID_GROUP_TYPE_UNKNOWN:
        default:
        data_drives = 0;
    }

    return data_drives;

}

/*!****************************************************************************
 *  fbe_test_sep_get_data_drives_for_raid_group
 ******************************************************************************
 *
 * @brief
 *   This function returns maximum drive capacity for given raid group configuration used
 *   for generating the physical configuration in the test
 *
 * @param   rg_config_p - Pointer to array of test raid group configurations
 * @param   num_raid_groups - Number of raid groups under test
 *
 * @return  drive_capacity
 *
 * @author
 *    2/14/2011 - Created. Mahesh Agarkar
 *
 *****************************************************************************/
fbe_block_count_t fbe_test_sep_util_get_maximum_drive_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t num_raid_groups)
{
    fbe_block_count_t max_drive_capacity = 0;
    fbe_u32_t rg_index, data_drives;

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        fbe_block_count_t drive_capacity;
        data_drives = fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]);
        if (data_drives != 0) {
            drive_capacity = ((rg_config_p[rg_index].capacity) / data_drives);
            if(drive_capacity > max_drive_capacity)
            {
                max_drive_capacity = drive_capacity;
            }
        }
    }
    return max_drive_capacity;
}

/*!****************************************************************************
 *  fbe_test_sep_util_set_unused_drive_info
 ******************************************************************************
 *
 * @brief
 *   This function is used to set the unused drive info in the raid group
 *   structure.
 *
 * @return  None
 *
 * @author
 *    05/11/2011 - Created. Vishnu Sharma
 *
 *****************************************************************************/
void fbe_test_sep_util_set_unused_drive_info(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t num_raid_groups,
                                             fbe_u32_t unused_drive_count[][FBE_DRIVE_TYPE_LAST],
                                             fbe_test_discovered_drive_locations_t *drive_locations_p)  
{
    fbe_u32_t             rg_index;
    fbe_status_t          status = FBE_STATUS_OK;
    fbe_u32_t             no_of_entries = FBE_RAID_MAX_DISK_ARRAY_WIDTH;
    
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        /* We will try to find out unused drives for the raid groups 
           which are going to be used in this pass. 
        */  
        if(fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            fbe_test_block_size_t block_size;
            fbe_test_raid_group_disk_set_t *disk_set_p = NULL;
            fbe_drive_type_t drive_type = rg_config_p[rg_index].drive_type;
            fbe_u32_t unused_drive_cnt = 0;

            status = fbe_test_sep_util_get_block_size_enum(rg_config_p[rg_index].block_size, &block_size);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Calculate index of unused drive.
             */
            unused_drive_cnt = unused_drive_count[block_size][drive_type];

            /*Initialise the array with zero values*/
            fbe_zero_memory(rg_config_p[rg_index].unused_disk_set,
                            no_of_entries * sizeof(fbe_test_raid_group_disk_set_t));

            
            /* Take the pointer of the disk set the block size (4160,520).
               We start allocating drives from highest array index. So
               any unused drives will start from index 0.
             */   
            disk_set_p = &drive_locations_p->disk_set[block_size][drive_type][0];

            /*If the no of unused drives are greater than array size only copy
              array size entries from the drive location array. We can increase the array size
              if required.
            */
            if(unused_drive_cnt > no_of_entries )
            {
                fbe_copy_memory(rg_config_p[rg_index].unused_disk_set,
                                disk_set_p,
                                no_of_entries * sizeof(fbe_test_raid_group_disk_set_t));             
            }
            else if(unused_drive_cnt > 0)
            {
                fbe_copy_memory(rg_config_p[rg_index].unused_disk_set,
                            disk_set_p,
                            unused_drive_cnt * sizeof(fbe_test_raid_group_disk_set_t));             
            }

        }
    }
}

/*!****************************************************************************
 *  fbe_test_sep_util_unconfigure_all_spares_in_the_system
 ******************************************************************************
 *
 * @brief
 *   This function is used to unconfigure all the available
 *   hot spares in the system.
 *
 * @return  None
 *
 * @author
 *    05/11/2011 - Created. Vishnu Sharma
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_unconfigure_all_spares_in_the_system(void)  
{
    fbe_u32_t             index;
    fbe_api_provisional_drive_get_spare_drive_pool_t  spare_drive_pool;
    fbe_status_t          status = FBE_STATUS_OK;

    status = fbe_api_provision_drive_get_spare_drive_pool(&spare_drive_pool);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     for(index = 0;index <spare_drive_pool.number_of_spare;index++)
     {
        fbe_provision_drive_config_type_t config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
        fbe_api_provision_drive_update_config_type_t  update_pvd_config;

        // set the object ID and configuration type to the update PVD data structure
        update_pvd_config.pvd_object_id = spare_drive_pool.spare_object_list[index];
        update_pvd_config.config_type = config_type;

        // Update the configuation 
        status = fbe_api_provision_drive_update_config_type(&update_pvd_config);

        // return if update configuration fails
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "**** Update index: %d total: %d pvd: 0%x to type: %d failed with status: 0x%x  ****", 
                       index, spare_drive_pool.number_of_spare,
                       update_pvd_config.pvd_object_id, update_pvd_config.config_type, status);
            return status;    
        }

     }
     return status;
}


/*!****************************************************************************
 *  fbe_test_sep_util_verify_pdo_objects_state_in_the_system
 ******************************************************************************
 *
 * @brief
 *   This function is used to verify all system pdo object state and if it is in fail state then try to bring
 *   it online before test start.
 *   
 *
 * @return  fbe_status
 *
 * @author
 *    06/21/2011 - Created. Amit Dhaduk
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_verify_pdo_objects_state_in_the_system(void)  
{

    fbe_object_id_t *           obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_u32_t                           total_objects = 0;
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    //fbe_lifecycle_state_t   current_state = NULL;

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    /* We do not know exactly how many drives we have.  We assume more than 0 objects total.
     */
    MUT_ASSERT_INT_NOT_EQUAL(total_objects, 0);
    mut_printf(MUT_LOG_LOW, "Total number of object is %d", total_objects);

    obj_list = (fbe_object_id_t *)malloc(sizeof(fbe_object_id_t) * total_objects);
    MUT_ASSERT_TRUE(obj_list != NULL);

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   /* Loop over all the objects in the physical package and verify pdo object state
     */
    for (obj_count = 0; obj_count < object_num; obj_count++)
    {
        fbe_object_id_t object_id = obj_list[obj_count];

        status = fbe_api_get_object_type(object_id, &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            free(obj_list);
        }

        /* check that all physical drive object be in READY state before test start */
        if( obj_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE)
        {
            fbe_test_sep_util_bring_drive_up(object_id, 5000);
        }
    }

    free(obj_list);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_test_sep_util_bring_drive_up()
 ****************************************************************
 * @brief
 *  This function will attempt to bring a drive up, print
 *  a log message if the drive cannot be brought back up, 
 *  and continue. 
 * 
 * @param fbe_object_id_t - the drive object id
 *        fbe_u32_t - wait time (ms) for drive to be ready.
 *
 * @return fbe_status_t
 *
 * @author
 *  8/1/2011    - Created. Amit Dhaduk/Deanna Heng
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_bring_drive_up(fbe_object_id_t object_id, fbe_u32_t wait_time) {

    fbe_lifecycle_state_t   current_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_get_object_lifecycle_state(object_id, &current_state, FBE_PACKAGE_ID_PHYSICAL);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Fail to get lifecycle state for object 0x%x", object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Topology will return `Busy' if object is in specialize */
    if (current_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        fbe_api_sleep(100);
    }

    /* If the drive is in hibernate, leave it there.
     */
    if (current_state == FBE_LIFECYCLE_STATE_HIBERNATE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s pdo obj: 0x%x is hibernating", 
                   __FUNCTION__, object_id);
        status = FBE_STATUS_OK;
    }
    else if (current_state != FBE_LIFECYCLE_STATE_READY)
    {
        /* clear drive error counter */
        status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Bring the drive online before test start */
        fbe_api_set_object_to_state(object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);

        /* wait for x seconds to bring the drive online back
         */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, wait_time, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "WARNING !!! PDO object 0x%x is discovered in fail state before test start !!!", object_id);
        }
    }

    return status;
}
/**************************************************
 * end fbe_test_sep_util_bring_drive_up()
 **************************************************/

fbe_status_t fbe_hw_clear_error_records()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_protocol_error_injection_error_record_t record_p;
    fbe_protocol_error_injection_record_handle_t handle_p = NULL;
 
    /*Initialize the Error record*/
    fbe_zero_memory(&record_p, sizeof(fbe_protocol_error_injection_error_record_t));
    //fbe_test_neit_utils_init_error_injection_record(&record_p);
    do{
        /* For getting first record, we send handle_p as NULL. */
        status = fbe_api_protocol_error_injection_get_record_next(handle_p, &record_p);
 
        if(handle_p == NULL || status != FBE_STATUS_OK)
        {
            return status;
        }
 
        status = fbe_api_protocol_error_injection_remove_record(&record_p);
 
    }while(handle_p != NULL);
 
    return FBE_STATUS_OK;
}
 
void fbe_test_sep_util_disable_all_error_injection() {
    fbe_status_t status;
 
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    status = fbe_api_logical_error_injection_destroy_objects();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    fbe_test_sep_util_clear_drive_errors();
 
    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    status = fbe_hw_clear_error_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
}
 
void fbe_test_sep_util_clear_drive_errors()
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    //fbe_u32_t                           port_idx = 0, enclosure_idx = 0, drive_idx = 0;
    fbe_u32_t                           total_objects = 0;
    fbe_object_id_t *           obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
 
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    //fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);
    obj_list = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
 
    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    /* Loop over all the objects in the physical package and
     * determine how many drives there are. 
     */
    for (obj_count = 0; obj_count < object_num; obj_count++)
    {
        status = fbe_api_get_object_type(obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);

        if (status != FBE_STATUS_OK)
        {
            break;
        }
 
        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
           status = fbe_api_physical_drive_clear_error_counts(obj_list[obj_count], FBE_PACKET_FLAG_NO_ATTRIB);
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    if(obj_list != NULL)
    {
        /*Free the memory*/ 
        free(obj_list);
    }
}

/*!**************************************************************
 * fbe_test_sep_util_check_permanent_spare_trigger_timer()
 ****************************************************************
 * @brief
 *  This function get permanent spare trigger time for system
 * 
 * @param permanent_spare_trigger_timer  - permanent spare trigger
 *                                         timer in seconds.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 27/05/2011    - Created. Harshal Wanjari
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_check_permanent_spare_trigger_timer(fbe_u64_t permanent_spare_trigger_timer)
{
    fbe_status_t status;
    fbe_api_virtual_drive_permanent_spare_trigger_time_t spare_config_info;

    /*check the updated permenent spare trigger time. */
    status = fbe_api_virtual_drive_get_permanent_spare_trigger_time(&spare_config_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    MUT_ASSERT_INTEGER_EQUAL(permanent_spare_trigger_timer, spare_config_info.permanent_spare_trigger_time);

    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_update_extra_chunk_size()
 ****************************************************************
 * @brief
 *  Update extra chunk size for each of the array of the RG that
 *  test is using.
 *
 * @param rg_config_p - array of raid_groups.               
 *
 * @return None
 *
 ****************************************************************/
void fbe_test_sep_util_update_extra_chunk_size(fbe_test_rg_configuration_t *rg_config_p, fbe_chunk_size_t in_extra_chunk)
{
    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        rg_config_p->extra_chunk = in_extra_chunk;           

        rg_config_p++;
    }
    return;
} /* end of fbe_test_sep_util_update_extra_chunk_size() */

/******************************************
 * end fbe_test_sep_util_get_permanent_spare_trigger_timer()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_user_lun_count()
 ****************************************************************
 * @brief
 *  count all the user LUNs in the system.
 *
 * @param lun_count.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/18/2011 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_get_user_lun_count(fbe_u32_t * lun_count)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_control_get_stats_t    get_db_stats;
    
    status = fbe_api_database_get_stats(&get_db_stats);
    *lun_count = get_db_stats.num_user_luns; 
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_get_all_user_rg_count()
 ****************************************************************
 * @brief
 *   count every raid group the user has bound.
 *
 * @param rg_count - (OUT) count of the user RGs.             
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/18/2011 - Created.  
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_get_all_user_rg_count(fbe_u32_t * rg_count)
{
    fbe_status_t                        status;
    fbe_database_control_get_stats_t    get_db_stats;
    #if 0
    fbe_u32_t    class_count;

    *rg_count = 0;

    status = fbe_test_sep_util_count_all_raid_group_of_class(FBE_CLASS_ID_PARITY, &class_count);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    *rg_count = *rg_count + class_count;

    status = fbe_test_sep_util_count_all_raid_group_of_class(FBE_CLASS_ID_STRIPER, &class_count);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    *rg_count = *rg_count + class_count;

    status = fbe_test_sep_util_count_all_raid_group_of_class(FBE_CLASS_ID_MIRROR, &class_count);
    *rg_count = *rg_count + class_count;
    #endif

    status = fbe_api_database_get_stats(&get_db_stats);
    *rg_count = get_db_stats.num_user_raids;
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_count_all_raid_group_of_class()
 ****************************************************************
 * @brief
 *  count every raid group that has the input class id.
 *
 * @param class_id - The class id to destroy.              
 *
 * @return fbe_status_t
 *
 * @author
 *  5/18/2011 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_count_all_raid_group_of_class(fbe_class_id_t class_id, fbe_u32_t * rg_count)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t rg_index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_u32_t raid_group_number;
    fbe_bool_t b_is_system_object;
    fbe_u32_t num_user_objects = 0;

    *rg_count = 0;
    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );

    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(obj_list_p);
        return status;
    }

    for (rg_index = 0; rg_index < num_objects; rg_index++)
    {
        status = fbe_api_database_lookup_raid_group_by_object_id(obj_list_p[rg_index], &raid_group_number);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot map to unit number status 0x%x\n",
                          __FUNCTION__, obj_list_p[rg_index], status);
            continue;
        }
        b_is_system_object = FBE_FALSE;
        status = fbe_api_database_is_system_object(obj_list_p[rg_index], &b_is_system_object);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s object id 0x%x cannot determine if system obj 0x%x\n",
                          __FUNCTION__, obj_list_p[rg_index], status);
            continue;
        }
        /* We only count user objects.
         */
        if (b_is_system_object)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s skipping system rg %d\n", __FUNCTION__, raid_group_number);
            continue;
        }

        if (raid_group_number == -1)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s skipping R10 mirror under striper %d\n", __FUNCTION__, raid_group_number);
            continue;
        }
        num_user_objects++;
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s Found raid group %d\n", __FUNCTION__, raid_group_number);
    }
    fbe_api_free_memory(obj_list_p);
    *rg_count = num_user_objects;
    return status;
}

/******************************************
 * end fbe_test_sep_util_get_user_lun_count()
 ******************************************/

/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_pool_id
 ******************************************************************************
 *
 * @brief
 *    This function updates the Pool-id on the specified provision drive. 
 *
 * @param   in_pvd_object_id  -  provision drive object id
 * @param   pool_id  -  provision drive pool-id
 *
 * @return  fbe_status_t   -  status of this operation
 *
 * @version
 *    06/16/2011 - Created. Arun S
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_provision_drive_pool_id(fbe_object_id_t in_pvd_object_id, fbe_u32_t pool_id)
{
    fbe_api_provision_drive_update_pool_id_t        update_pvd_pool_id;
    fbe_status_t                                    status;
   
    update_pvd_pool_id.object_id = in_pvd_object_id;
    update_pvd_pool_id.pool_id = pool_id;

    /* update the provision drive pool-id. */
    status = fbe_api_provision_drive_update_pool_id(&update_pvd_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return status;
}

/*!****************************************************************************
 * fbe_test_sep_util_update_multi_provision_drive_pool_id
 ******************************************************************************
 *
 * @brief
 *    This function updates multi pvds Pool-id. 
 *
 * @param   in_pvd_object_id_p -  provision drive object id array
 * @param   pool_id_p  -  provision drive pool-id array
 * @param   pvd_count - the element number of the pvd_object_id array 
 *
 * @return  fbe_status_t   -  status of this operation
 *
 * @version
 *    04/10/2013 - Created. Hongpo Gao 
 *
 ******************************************************************************/

fbe_status_t
fbe_test_sep_util_update_multi_provision_drive_pool_id(fbe_object_id_t *in_pvd_object_id_p, 
                                                       fbe_u32_t *pool_id_p, fbe_u32_t pvd_count)
{
    fbe_api_provision_drive_update_pool_id_list_t        update_pvd_pool_id_list;
    fbe_status_t                                    status;
    fbe_u32_t                                       index;
    fbe_u32_t                                       *p1, *p2;
   
    MUT_ASSERT_NOT_NULL_MSG(in_pvd_object_id_p, "update multi pvd pool id: pvd_obj_id_p is NULL \n");
    MUT_ASSERT_NOT_NULL_MSG(pool_id_p, "update multi pvd pool id: pool_id_p is NULL \n");


    fbe_zero_memory(&update_pvd_pool_id_list, sizeof(fbe_api_provision_drive_update_pool_id_list_t));

    p1 = in_pvd_object_id_p;
    p2 = pool_id_p;
    for (index = 0; index < pvd_count; index++)
    {
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_object_id = p1[index];
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_pool_id = p2[index];
        update_pvd_pool_id_list.pvd_pool_data_list.total_elements++;
        if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements == MAX_UPDATE_PVD_POOL_ID)
        {
            status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
        }
    }

    if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements != 0)
    {
        status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
    }

    return status;
}

/*!****************************************************************************
 *  fbe_test_sep_util_provision_drive_wait_disk_zeroing
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing status with reading checkpoint 
 *   in loop. It exits when disk zeroing reach at given checkpoint or time
 *   out occurred.
 *
 * @param object_id              - provision drive object id
 * @param max_wait_msecs         - max timeout value to check for disk zeroing completion
 * @param expect_zero_checkpoint - wait till disk zeroing reach at this checkpoint
 * 
 * @return fbe_status_t              fbe status 
 *
 *****************************************************************************/

fbe_status_t fbe_test_sep_util_provision_drive_wait_disk_zeroing(fbe_object_id_t object_id, 
                                            fbe_lba_t   expect_zero_checkpoint,
                                            fbe_u32_t   max_wait_msecs)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lba_t               curr_checkpoint;

    while (current_time < max_wait_msecs){

        /* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);
            return status;
        }

        /* check if disk zeroing reach at given checkpoint */
        if (curr_checkpoint >= expect_zero_checkpoint){
            mut_printf(MUT_LOG_TEST_STATUS, "=== disk zeroing wait complete, chkpt 0x%llx\n", curr_checkpoint);           
            return status;
        }
        current_time = current_time + 100;
        fbe_api_sleep (100);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%llx, %d ms!\n", 
                  __FUNCTION__, object_id, (unsigned long long)curr_checkpoint,
		  max_wait_msecs);
    MUT_ASSERT_TRUE( FBE_FALSE );

    return FBE_STATUS_GENERIC_FAILURE;
}
/***************************************************************
 * end fbe_test_sep_util_provision_drive_wait_disk_zeroing()
 ***************************************************************/

/*!**************************************************************
 * fbe_test_wait_for_rg_zero_complete()
 ****************************************************************
 * @brief
 *  Wait until the zeroing is complete for all drives in the rg.
 *
 * @param rg_config_p - config ptr.
 *
 * @return None.   
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_wait_for_rg_zero_complete(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status; 
    fbe_object_id_t                 pvd_object_id;
    fbe_u32_t index;

    mut_printf(MUT_LOG_LOW, "=== Wait for rg_id: %d to complete zeroing. ===\n", rg_config_p->raid_group_id);   

    for (index = 0 ; index < rg_config_p->width; index++)
    {
        /* get provision drive object id for first drive */
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[index].bus,
                                                                 rg_config_p->rg_disk_set[index].enclosure,
                                                                 rg_config_p->rg_disk_set[index].slot,
                                                                 &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* wait for disk zeroing operation to complete */
        status = fbe_test_sep_util_provision_drive_wait_disk_zeroing(pvd_object_id, FBE_LBA_INVALID, 50 * 1000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_LOW, "=== zeroing completed. ===\n");    

}
/******************************************
 * end fbe_test_wait_for_rg_zero_complete()
 ******************************************/
/*!***************************************************************************
 *          fbe_test_sep_util_get_expected_verify_error_counts()
 *****************************************************************************
 *
 * @brief   Based on raid type and the number of errors being injected, this
 *          method determines how many correctable and uncorrectable errors
 *          to expect when a verify is executed.
 * 
 * @param   raid_group_object_id   - object id of raid group object
 * @param   start_lba - First lba on first and second position to inject error
 * @param   errors_to_inject_on_first_position - number of errors injected on
 *              first (i.e. position 0) position.
 * @param   errors_to_inject_on_second_position - number of errors injected
 *              on second position (i.e. position 1)
 * @param   expected_num_of_correctable_errors_p - number of correctable errors
 *              expected
 * @param   expected_num_of_uncorrectable_errors_p - number of uncorrectable
 *              errors expected
 * 
 * @note    Assumes non-degraded raid group
 *
 * @return  status - Raid group must be ready state
 *
 ***************************************************************************/
fbe_status_t fbe_test_sep_util_get_expected_verify_error_counts(fbe_object_id_t raid_group_object_id,
                                                                fbe_lba_t  start_lba,
                                                                fbe_u32_t  errors_injected_on_first_position,
                                                                fbe_u32_t  errors_injected_on_second_position,
                                                                fbe_u32_t *expected_num_of_correctable_errors_p,
                                                                fbe_u32_t *expected_num_of_uncorrectable_errors_p)
{
    fbe_status_t                    status;
    fbe_u32_t                       width;
    fbe_u32_t                       max_injected_errors;
    fbe_u32_t                       min_injected_errors;
    fbe_api_raid_group_get_info_t   raid_group_info;    //  raid group information from "get info" command
    fbe_lifecycle_state_t           current_state;
    
    /* Validate raid group is ready and that position is valid.
     */
    status = fbe_api_raid_group_get_info(raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    width = raid_group_info.width;
    status = fbe_api_get_object_lifecycle_state(raid_group_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, current_state);

    /* The number of expected errors is based on raid group type
     */
    max_injected_errors = FBE_MAX(errors_injected_on_first_position, errors_injected_on_second_position);
    min_injected_errors = FBE_MIN(errors_injected_on_first_position, errors_injected_on_second_position);
    switch(raid_group_info.raid_type)
    {
        /* Mirror raid groups.
         */
        case FBE_RAID_GROUP_TYPE_RAID10:
            /* Since the errors are injected on adjacent positions, treat it 
             * like a 2-way mirror
             */
            width = 2;
            // Fall-thru
        case FBE_RAID_GROUP_TYPE_RAID1:
            MUT_ASSERT_TRUE_MSG(((width == 3) || (width == 2)), "sep util: Unsupported mirror width \n");
            if (width == 3)
            {
                *expected_num_of_correctable_errors_p = max_injected_errors;
                *expected_num_of_uncorrectable_errors_p = 0;
            }
            else
            {
                *expected_num_of_correctable_errors_p = max_injected_errors - min_injected_errors;
                *expected_num_of_uncorrectable_errors_p = min_injected_errors;
            }
            break;

        /* Single Parity raid groups 
         */
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
            {
                fbe_u32_t   strips_per_parity_stripe;
                fbe_u32_t   parity_position;
                fbe_bool_t  b_injecting_to_parity_position;

                /* Parity position is always position 0 for RAID-3
                 */
                if (raid_group_info.raid_type == FBE_RAID_GROUP_TYPE_RAID3)
                {
                    parity_position = 0;
                }
                else
                {
                    strips_per_parity_stripe = raid_group_info.element_size * raid_group_info.elements_per_parity_stripe;
                    parity_position = (start_lba / strips_per_parity_stripe) % raid_group_info.width;
                }

                /* Now determine if we are injecting to the parity positin or not.
                 */
                if ((parity_position == 0) ||
                    (parity_position == 1)    )
                {
                    b_injecting_to_parity_position = FBE_TRUE;
                }
                else
                {
                    b_injecting_to_parity_position = FBE_FALSE;
                }

                /*! @note Inject the following errors on a X + 1 RAID-5 raid group:
                 *          o Inject (40) single block errors into the first data position
                 *          o Inject ( 5) single block errors into the second data position
                 *        The result varies based on whether any errors are injected to the
                 *        parity position or not.  This is due to the fact that parity is never
                 *        invalidated and therefore always correctable.
                 *          o Parity is the first position:
                 *              + (40) Correctable errors on the first data position
                 *              + ( 5) Uncorrectable errors on the second data position
                 *          o No errors injected to parity ((5) blocks invalidated on both data positions)
                 *              + ( 5) Uncorrectable errors on both data positions
                 *              + (35) Correctable errors on first data position
                 */
                if (b_injecting_to_parity_position == FBE_TRUE)
                {
                    *expected_num_of_correctable_errors_p = max_injected_errors;
                    *expected_num_of_uncorrectable_errors_p = min_injected_errors;
                }
                else
                {
                    *expected_num_of_correctable_errors_p = max_injected_errors - min_injected_errors;
                    *expected_num_of_uncorrectable_errors_p = min_injected_errors;
                }

            } /* end if single parity raid group type*/
            break;

        /* Striped raid groups
         */
        case FBE_RAID_GROUP_TYPE_RAID0:
            *expected_num_of_correctable_errors_p = 0;
            *expected_num_of_uncorrectable_errors_p = max_injected_errors + min_injected_errors;
            break;

        /*! @todo Striped mirrors, RAID-6  and spare are not fully supported 
         */
        default:
        case FBE_RAID_GROUP_TYPE_RAID6:
        case FBE_RAID_GROUP_TYPE_SPARE:
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "sep util: Unsupported raid group type \n");
            break;

    } /* end switch on raid group type */

    /* Success
     */
    return FBE_STATUS_OK;

} /* End of fbe_test_sep_util_get_expected_verify_error_counts() */
/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_LUN_edge_on_sp()
 *****************************************************************************
 * 
 * @brief   Wait for the LUN object on the sp specified.  Use this method when the
 *          object may not reside on the calling sp.
 *
 * @param   b_wait_for_both_sps - FBE_TRUE Need to wait for both sps
 * @param   object_id - object id to wait for
 * @param   timeout_ms - Timeout in ms
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_LUN_edge_on_sp(fbe_object_id_t lun_object_id, 
                                                          fbe_transport_connection_target_t target_sp_of_object,
                                                          fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_OK; 
    fbe_status_t                        target_status;
    fbe_u32_t                           current_time = 0;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   peer_target = FBE_TRANSPORT_SP_B;
    fbe_object_id_t                     bvd_object_id;
    fbe_volume_attributes_flags         attribute;

    /* Get the object ID for BVD; will use it later, it should be the same on both SPs */
    status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);

    /* First get the sp id this was invoked on and out peer.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    
    while (current_time < timeout_ms)
    {
        fbe_transport_connection_target_t   current_target = target_invoked_on;

        /* Configure the target before sending the control command
         */
        if (target_sp_of_object != target_invoked_on)
        {
             status = fbe_api_transport_set_target_server(target_sp_of_object);
             MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
             current_target = target_sp_of_object;
        }

        /* Issue the request
         */
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);

        /* Set the target back to the sp it was invoked on (if required)
         */
        if (target_sp_of_object != target_invoked_on)
        {
             target_status = fbe_api_transport_set_target_server(target_invoked_on);
             MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, target_status);
             current_target = target_invoked_on;
        }

        /* Now process the status
         */
        if (status == FBE_STATUS_OK){
            mut_printf(MUT_LOG_TEST_STATUS,  
                          "wait_for_LUN_on_sp: LUN obj: 0x%x sp of object: %d this sp: %d connected to BVD",
                          lun_object_id, target_sp_of_object, target_invoked_on);
            return status;
        }

        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    /* We timed out
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS,
               "wait_for_LUN_on_sp: LUN obj: 0x%x this sp: %d object sp: %d didn't connect to BVD %d",
               lun_object_id, target_invoked_on, target_sp_of_object, bvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*****************************************************************
 * end fbe_test_sep_util_wait_for_LUN_edge_on_sp()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_LUN_edge_on_all_sps()
 *****************************************************************************
 * 
 * @brief   Wait for the LUN object on (1) or both sps to connect to BVD
 *
 * @param   b_wait_for_both_sps - FBE_TRUE Need to wait for both sps
 * @param   object_id - object id to wait for
 * @param   timeout_ms - Timeout in ms
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_bool_t b_wait_for_both_sps,
                                                               fbe_object_id_t lun_object_id, 
                                                               fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_target;

    if (b_wait_for_both_sps)
    {
    
        /* For all targets wait for lifecycle state for the object specified
         */
        for (current_target = FBE_TRANSPORT_SP_A; current_target <= FBE_TRANSPORT_SP_B; current_target++)
        {
            /* Invoke the `wait for object' specifying the target server
             */
            status = fbe_test_sep_util_wait_for_LUN_edge_on_sp(lun_object_id, 
                                                                current_target,
                                                                timeout_ms);
            if (status != FBE_STATUS_OK)
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for LUN edge on both sps: obj: 0x%x timeout(ms): %d target: %d status: 0x%x", 
                           lun_object_id, timeout_ms, current_target, status);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
    } /* end b_wait_for_both_sps */
    else
    {
        current_target = fbe_api_transport_get_target_server();
        status = fbe_test_sep_util_wait_for_LUN_edge_on_sp(lun_object_id, current_target, timeout_ms);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Success
     */
    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_test_sep_util_wait_for_LUN_edge_on_all_sps()
 *******************************************************************/


/*used by SEP tests to be able to load ESP package, different than the ESP interface which needs a bit more control*/
fbe_status_t fbe_test_util_create_registry_file(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_sim_server_pid sp_pid = 0;
    fbe_u8_t file_path[64] = {'\0'};
    fbe_file_handle_t fp = NULL;
    csx_status_e dir_status;  
    csx_p_dir_t dir_object;
    csx_char_t csx_access_mode[256];
    csx_module_context_t    csx_module_context;

    csx_p_memset(csx_access_mode, '\0', sizeof(csx_access_mode));
    csx_p_strcat(csx_access_mode, "b");
    csx_module_context = EmcpalClientGetCsxModuleContext(EmcpalDriverGetCurrentClientObject());

    status = fbe_api_sim_server_get_pid(&sp_pid);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to get SP PID. status: 0x%x\n",
                      __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Need to create sp_sim_pid directory if it doesn't exist.
     */
#ifdef ALAMOSA_WINDOWS_ENV
    fbe_sprintf(file_path, sizeof(file_path), ".\\sp_sim_pid%d", (int)sp_pid);
#else
    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d", (int)sp_pid);
#endif
    dir_status = csx_p_dir_open(csx_module_context, &dir_object, file_path);
    mut_printf(MUT_LOG_TEST_STATUS, "status of directory open for %s was 0x%x", file_path, dir_status);
    if (CSX_SUCCESS(dir_status)) {
        dir_status = csx_p_dir_close(&dir_object);
        mut_printf(MUT_LOG_TEST_STATUS, "status of directory close for %s was 0x%x", file_path, dir_status); 
    }
    dir_status = csx_p_dir_create(file_path);
    if(!(CSX_SUCCESS(dir_status) || (dir_status == CSX_STATUS_OBJECT_ALREADY_EXISTS)))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: failed to create sp_sim_pid status: 0x%x.\n",
                      __FUNCTION__, dir_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Then create the file
     */
#ifdef ALAMOSA_WINDOWS_ENV
    fbe_sprintf(file_path, sizeof(file_path), ".\\sp_sim_pid%d\\esp_reg_pid%d.txt", sp_pid, sp_pid);
#else
    fbe_sprintf(file_path, sizeof(file_path), "./sp_sim_pid%d/esp_reg_pid%d.txt", (int)sp_pid, (int)sp_pid);
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */

    fp = fbe_file_open(file_path, FBE_FILE_RDWR, 0, NULL);
    if(fp == FBE_FILE_INVALID_HANDLE)
    {
        fp = fbe_file_creat(file_path, FBE_FILE_RDWR);
        fbe_file_close(fp);
    }
    else
    {
        fbe_file_close(fp);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_test_sep_util_raid_group_generate_error_trace()
 ***************************************************************************** 
 * 
 * @brief   This function sets the `generate error trace' flag for the raid
 *          group specified by the raid group object id.
 * 
 * @param   rg_object_id - The raid group object id ot generate the error trace
 *                         for.
 *
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @author
 *  10/03/2011  Ron Proulx  - Created
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_raid_group_generate_error_trace(fbe_object_id_t rg_object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t           raid_group_info;
    fbe_sim_transport_connection_target_t   local_sp = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   peer_sp = FBE_SIM_INVALID_SERVER;
    fbe_cmi_service_get_info_t              cmi_info;
        
    /* Validate the object id passed
     */
    if (rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        MUT_ASSERT_TRUE_MSG(rg_object_id != FBE_OBJECT_ID_INVALID,
                            "test_sep_util: Invalid rg_object_id");
    }   

    /* Validate that this is a valid raid group
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We ignore the status returned
     */
    fbe_api_raid_group_set_group_debug_flags(rg_object_id,
                                             FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE);

    /* If we are in dualsp mode repeat the sequence for the peer.
     */
    local_sp = fbe_api_sim_transport_get_target_server();
    peer_sp = (FBE_SIM_SP_A == local_sp) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        /* Only send it if the peer is alive.
         */
        if (cmi_info.peer_alive) 
        {
            fbe_api_sim_transport_set_target_server(peer_sp);
            /* Validate that this is a valid raid group
             */
            status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* We ignore the status returned
             */
            fbe_api_raid_group_set_group_debug_flags(rg_object_id,
                                                 FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE);

            /* Set the SP back to the original.
             */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }

    /* Return execution status
     */
    return status;
}
/*******************************************************************
 * end of fbe_test_sep_util_raid_group_generate_error_trace()
 *******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_get_minimum_lun_capacity_for_config()
 ***************************************************************************** 
 * 
 * @brief   This function sets the `generate error trace' flag for the raid
 *          group specified by the raid group object id.
 * 
 * @param   rg_config_p - Array of raid groups to check lun capacity for
 * @param   min_lun_capacity_p - Pointer to capacity to update
 *
 * @return  status - Typically FBE_STATUS_OK
 * 
 * @author
 *  10/05/2011  Ron Proulx  - Created
 * 
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_get_minimum_lun_capacity_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_lba_t *min_lun_capacity_p)
{
    fbe_u32_t       raid_group_count;
    fbe_u32_t       rg_index;
    fbe_u32_t       lun_index;
    fbe_lba_t       min_lun_capacity = FBE_LBA_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_test_logical_unit_configuration_t *lun_config_p = NULL;

    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* For every raid group that is enabled get the 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        current_rg_config_p = &rg_config_p[rg_index];
        if (current_rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Just continue
             */
            continue;
        }
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        /* Walk thru the lun list
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            lun_config_p = &current_rg_config_p->logical_unit_configuration_list[lun_index];
            if (lun_config_p->capacity < min_lun_capacity)
            {
                min_lun_capacity = lun_config_p->capacity;
            }
        }

    } /* end for all raid groups */

    /* Update the passed lun capacity */
    if (min_lun_capacity == FBE_LBA_INVALID)
    {
        min_lun_capacity = 0;
    }
    *min_lun_capacity_p = min_lun_capacity;

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end of fbe_test_sep_util_get_minimum_lun_capacity_for_config()
 *******************************************************************/


/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_LUN_create_event()
 *****************************************************************************
 * 
 * @brief   Wait for the LUN create event
 *         This function used a fix timeout
 *
 * @param   object_id - object id to wait for
 * @param   lun_number - lun number
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_LUN_create_event(fbe_object_id_t lun_object_id, 
                                                         fbe_lun_number_t lun_number)
{
    fbe_status_t                        status = FBE_STATUS_OK; 
    fbe_u32_t                           current_time = 0;
    fbe_bool_t                          is_msg_present = FBE_FALSE; 

    while (current_time < FBE_TEST_WAIT_OBJECT_TIMEOUT_MS)
    {
        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                             &is_msg_present, 
                                             SEP_INFO_LUN_CREATED,
                                             lun_object_id, 
                                             lun_number); 

        /* Now process the status
         */
        if ((status== FBE_STATUS_OK)&&(is_msg_present == FBE_TRUE)){
            mut_printf(MUT_LOG_TEST_STATUS,  
                          "LUN_create_event exists for LUN obj: 0x%x lun_number %d",
                          lun_object_id, lun_number);
            return status;
        }

        current_time = current_time + FBE_TEST_UTILS_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_UTILS_POLLING_INTERVAL);
    }

    /* We timed out
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS,  
                  "LUN_create_event DOESNOT exist for LUN obj: 0x%x lun_number %d (is_present %d)",
                  lun_object_id, lun_number, is_msg_present);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*****************************************************************
 * end fbe_test_sep_util_wait_for_LUN_create_event()
 *****************************************************************/

/*!**************************************************************
 * fbe_test_wait_for_objects_of_class_state()
 ****************************************************************
 * @brief
 *  Wait for all objects of a class to get to a given state.
 *
 * @param class_id - class to wait for.
 * @param package_id - Package in which we should find these objects.
 * @param expected_lifecycle_state - Lifecycle state to wait for.
 * @param timeout_ms - The number of milliseconds before we timeout.
 *
 * @return None.   
 *
 * @author
 *  11/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_wait_for_objects_of_class_state(fbe_class_id_t class_id, 
                                              fbe_package_id_t package_id,
                                              fbe_lifecycle_state_t expected_lifecycle_state,
                                              fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for all objects of class to get to this state.
     */
    for ( index = 0; index < num_objects; index++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(obj_list_p[index], 
                                                         expected_lifecycle_state,
                                                         timeout_ms,
                                                         package_id);

        if (status != FBE_STATUS_OK)
        {
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
}
/******************************************
 * end fbe_test_wait_for_objects_of_class_state()
 ******************************************/

/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_disable_zeroing
 ******************************************************************************
 *
 * @brief
 *    This function disables background zeroing operation for the 
 *    specified provision drive object. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    11/17/2011 - Created. Omer Miranda
 *
 ******************************************************************************/

fbe_status_t fbe_test_sep_util_provision_drive_disable_zeroing( fbe_object_id_t in_provision_drive_object_id )
{
    return fbe_api_base_config_disable_background_operation(in_provision_drive_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
}
/*****************************************************************
 * end fbe_test_sep_util_provision_drive_disable_zeroing()
 *****************************************************************/

/*!****************************************************************************
 * fbe_test_sep_util_provision_drive_enable_zeroing
 ******************************************************************************
 *
 * @brief
 *    This function enables background zeroing operation for the 
 *    specified provision drive object. 
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    11/17/2011 - Created. Omer Miranda
 *
 ******************************************************************************/

fbe_status_t fbe_test_sep_util_provision_drive_enable_zeroing( fbe_object_id_t in_provision_drive_object_id )
{
    return fbe_api_base_config_enable_background_operation(in_provision_drive_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
}
/*****************************************************************
 * end fbe_test_sep_util_provision_drive_disable_zeroing()
 *****************************************************************/

/*!****************************************************************************
 * fbe_test_sep_util_rg_config_disable_zeroing
 ******************************************************************************
 *
 * @brief
 *    This function disables background zeroing operation for the 
 *    entire Raid group configuration's disk set. 
 *
 * @param   rg_config_p - Array of raid groups
 *
 * @return  fbe_status_t -  status of this operation
 *
 * @version
 *    11/19/2011 - Created. Omer Miranda
 *
 ******************************************************************************/

fbe_status_t fbe_test_sep_util_rg_config_disable_zeroing( fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t raid_group_count;
    fbe_u32_t rg_index;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t disk_index;
    fbe_object_id_t pvd_object_id;

    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        current_rg_config_p = &rg_config_p[rg_index];
        if(fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            for(disk_index = 0 ; disk_index < current_rg_config_p->width; disk_index++)
            {
                status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                        current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                        current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                        &pvd_object_id);
                MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

                /* Before starting the sniff related operation disable the disk zeroing.
                 */
                status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
    }
    return status;
}
/*****************************************************************
 * end fbe_test_sep_util_rg_config_disable_zeroing()
 *****************************************************************/

/*!****************************************************************************
 * fbe_test_sep_util_rg_config_enable_zeroing
 ******************************************************************************
 *
 * @brief
 *    This function enables background zeroing operation for the 
 *    entire Raid group configuration's disk set. 
 *
 * @param   rg_config_p - Array of raid groups
 *
 * @return  fbe_status_t -  status of this operation
 *
 * @version
 *
 ******************************************************************************/

fbe_status_t fbe_test_sep_util_rg_config_enable_zeroing( fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t raid_group_count;
    fbe_u32_t rg_index;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t disk_index;
    fbe_object_id_t pvd_object_id;

    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        current_rg_config_p = &rg_config_p[rg_index];
        if(fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            for(disk_index = 0 ; disk_index < current_rg_config_p->width; disk_index++)
            {
                status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                        current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                        current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                        &pvd_object_id);
                MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

                /* Before starting the sniff related operation disable the disk zeroing.
                 */
                status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd_object_id);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
    }
    return status;
}
/*****************************************************************
 * end fbe_test_sep_util_rg_config_enable_zeroing()
 *****************************************************************/

/*!****************************************************************************
 * fbe_test_sep_util_wait_for_pvd_creation
 ******************************************************************************
 *
 * @brief
 *    This function wait for pvds to be created, for a given b-e-d list    
 *
 * @param   disk_set       - Array of disk location
 * @param   disk_count     - number of disks
 * @param   wait_time_ms   - wait time for the creation of each pvd  
 *
 ******************************************************************************/
void fbe_test_sep_util_wait_for_pvd_creation(fbe_test_raid_group_disk_set_t * disk_set,fbe_u32_t disk_count,fbe_u32_t wait_time_ms)
{
    fbe_u32_t    disk_index; 
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    timeout_ms = wait_time_ms;
    fbe_object_id_t  pvd_id;
 
    /* get PVD ids */
    for (disk_index = 0;  disk_index < disk_count; disk_index++)
    {
        pvd_id = FBE_OBJECT_ID_INVALID;
        timeout_ms = wait_time_ms;

        while ((pvd_id == FBE_OBJECT_ID_INVALID) &&(timeout_ms > 0))
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(disk_set[disk_index].bus,
                                                                    disk_set[disk_index].enclosure,
                                                                    disk_set[disk_index].slot,
                                                                    &pvd_id);
            
            fbe_api_sleep (FBE_TEST_UTILS_POLLING_INTERVAL);
            timeout_ms =- FBE_TEST_UTILS_POLLING_INTERVAL ;
        }
        MUT_ASSERT_INT_NOT_EQUAL(pvd_id, FBE_OBJECT_ID_INVALID);
    }
    return;
}
/*****************************************************************
 * end fbe_test_sep_util_wait_for_pvd_creation()
 *****************************************************************/
/*!**************************************************************
 * fbe_test_sep_util_zero_object_capacity()
 ****************************************************************
 * @brief
 *  Seed rg with zero pattern.
 *
 * @param rg_object_id  
 * @param start_offset - Offset to start zeroing at.
 *
 * @return None.
 *
 * @author
 *  12/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_zero_object_capacity(fbe_object_id_t rg_object_id,
                                            fbe_lba_t start_offset)
{
    fbe_api_rdgen_context_t context;
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_init(&context, 
                                             rg_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_ZERO_ONLY,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             start_offset,    /* Start lba*/
                                             start_offset,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba=capacity */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             FBE_LBA_INVALID,    /* min blocks=capacity. */
                                             FBE_LBA_INVALID    /* Max blocks=capacity */ );

    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(&context, 
                                             rg_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             start_offset,    /* Start lba*/
                                             start_offset,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba=capacity */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             2048,    /* min blocks */
                                             2048    /* max blocks */ );
    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end fbe_test_sep_util_zero_object_capacity()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_disable_system_drive_zeroing()
 ****************************************************************
 * @brief
 *  Disable system drive background zeroing.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  1/17/2012 - Created. Rob Foley
 *  
 ****************************************************************/

void fbe_test_sep_util_disable_system_drive_zeroing(void)
{
    fbe_status_t status;
    fbe_u32_t slot;
    fbe_object_id_t pvd_object_id;

    for (slot = 0; slot < 4; slot++)
    {
        /* We will disable background zeroing for the first 4 drives.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}
/**************************************
 * end fbe_test_sep_util_disable_system_drive_zeroing()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_enable_system_drive_zeroing()
 ****************************************************************
 * @brief
 *  Disable system drive background zeroing.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2/13/2014 - Created. Rob Foley
 *  
 ****************************************************************/

void fbe_test_sep_util_enable_system_drive_zeroing(void)
{
    fbe_status_t status;
    fbe_u32_t slot;
    fbe_object_id_t pvd_object_id;

    for (slot = 0; slot < 4; slot++)
    {
        /* We will disable background zeroing for the first 4 drives.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}
/**************************************
 * end fbe_test_sep_util_enable_system_drive_zeroing()
 **************************************/

/*!**************************************************************
 * fbe_test_disable_rg_background_ops_system_drives()
 ****************************************************************
 * @brief
 *  Disable all background ops on the system drives like
 *  background verify and background zeroing.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/25/2012 - Created. Rob Foley
 *  
 ****************************************************************/

void fbe_test_disable_rg_background_ops_system_drives(void)
{
    fbe_u32_t rg_object_id;
    fbe_status_t status;
    for (rg_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST + 1;
          rg_object_id < FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST;
          rg_object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id,FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_base_config_disable_background_operation(rg_object_id, 
                                                                  FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/**************************************
 * end fbe_test_disable_rg_background_ops_system_drives()
 **************************************/

/*!**************************************************************
 * fbe_test_enable_rg_background_ops_system_drives()
 ****************************************************************
 * @brief
 *  Disable all background ops on the system drives like
 *  background verify and background zeroing.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2/13/2014 - Created. Rob Foley
 *  
 ****************************************************************/

void fbe_test_enable_rg_background_ops_system_drives(void)
{
    fbe_u32_t rg_object_id;
    fbe_status_t status;
    for (rg_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST + 1;
          rg_object_id < FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST;
          rg_object_id++)
    {
        status = fbe_api_base_config_enable_background_operation(rg_object_id, 
                                                                  FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/**************************************
 * end fbe_test_enable_rg_background_ops_system_drives()
 **************************************/

/*!**************************************************************
 * fbe_test_disable_background_ops_system_drives()
 ****************************************************************
 * @brief
 *  Disable all background ops on the system drives like
 *  background verify and background zeroing.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/25/2012 - Created. Rob Foley
 *  
 ****************************************************************/

void fbe_test_disable_background_ops_system_drives(void)
{
    fbe_test_sep_util_disable_system_drive_zeroing();
    mut_printf(MUT_LOG_TEST_STATUS, "System drive background zeroing is: DISABLED\n");

    fbe_test_disable_rg_background_ops_system_drives();
    mut_printf(MUT_LOG_TEST_STATUS, "RG background verify is: DISABLED\n");

}
/**************************************
 * end fbe_test_disable_background_ops_system_drives()
 **************************************/

/*!*******************************************************************
 * @def FBE_TEST_PCT_TO_ENABLE_SYSTEM_BG_ZEROING
 *********************************************************************
 * @brief Percentage of time we will enable background zeroing 
 *        for the system drives.
 *
 *********************************************************************/
#define FBE_TEST_PCT_TO_ENABLE_SYSTEM_BG_ZEROING 50

/*!**************************************************************
 * fbe_test_sep_util_randmly_disable_system_zeroing()
 ****************************************************************
 * @brief
 *  Decide randomly if we should disable system drive background
 *  zeroing, and then disable it if needed.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_randmly_disable_system_zeroing(void)
{
    fbe_u32_t random_seed = (fbe_u32_t)fbe_get_time();
    /* We would like system drive background zeroing to only be enabled a 
     * certain percentage of the time, since this will slow down the tests. 
     *  
     * System drives are big and will be constantly zeroing in the background.
     */
    if (!fbe_test_sep_util_get_cmd_option("-sep_enable_system_drive_zeroing") &&
        ( fbe_test_sep_util_get_cmd_option("-sep_disable_system_drive_zeroing") ||
          (random_seed % 100) > FBE_TEST_PCT_TO_ENABLE_SYSTEM_BG_ZEROING))
    {
        fbe_test_sep_util_disable_system_drive_zeroing();
        mut_printf(MUT_LOG_TEST_STATUS, "System drive background zeroing is: DISABLED\n");
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "System drive background zeroing is: ENABLED\n");
    }
}
/******************************************
 * end fbe_test_sep_util_randmly_disable_system_zeroing()
 ******************************************/

/*!**************************************************************
 * fbe_test_set_object_id_for_r10()
 ****************************************************************
 * @brief
 *  Helper function to get mirror object id from striper object id.
 *
 * @param rg_object_id  - Striper raid group object id.
 *
 * @return None.
 *
 * @author
 *  10/14/2011 - Created. Vishnu Sharma
 ****************************************************************/
void fbe_test_set_object_id_for_r10(fbe_object_id_t *rg_object_id)
{
    fbe_api_base_config_downstream_object_list_t downstream_raid_object_list;
    fbe_u32_t index =0;
            
    /* initialize the downstream object list. */
    downstream_raid_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        downstream_raid_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    fbe_api_base_config_get_downstream_object_list(*rg_object_id, &downstream_raid_object_list);
    if (downstream_raid_object_list.number_of_downstream_objects == 0)
    {
         MUT_ASSERT_TRUE(0);
    }

    *rg_object_id = downstream_raid_object_list.downstream_object_list[0];
}
/********************************************************
 * end fbe_test_set_object_id_for_r10()
 ********************************************************/

fbe_status_t fbe_test_pp_util_pull_drive_hw(fbe_u32_t port_number, 
                                            fbe_u32_t enclosure_number, 
                                            fbe_u32_t slot_number)
{
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           lastSlot = 0;
    fbe_status_t                        status;
    fbe_object_id_t                     pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     enclosure_object_id = FBE_OBJECT_ID_INVALID;
    fbe_base_object_mgmt_drv_dbg_ctrl_t debugControl;

    // update the encl number 
    location.bus = port_number;
    location.enclosure = enclosure_number;
    location.slot = slot_number;

    // get enclosure object id
    status = fbe_api_get_enclosure_object_id_by_location(location.bus, location.enclosure, &enclosure_object_id);
    if ((enclosure_object_id == FBE_OBJECT_ID_INVALID) || (status != FBE_STATUS_OK))
    {	
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for bus and enclosure");
    }

    // need the number of slots in this enclosure
    status = fbe_api_enclosure_get_slot_count(enclosure_object_id, &lastSlot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Get pdo object ID
    status = fbe_api_get_physical_drive_object_id_by_location(location.bus, 
                                                              location.enclosure, 
                                                              location.slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_object_id != FBE_OBJECT_ID_INVALID);

    // send command to remove the drive
    fbe_zero_memory(&debugControl, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
    debugControl.driveDebugAction[location.slot] = FBE_DRIVE_ACTION_REMOVE;
    debugControl.driveCount = lastSlot;
    status = fbe_api_enclosure_send_drive_debug_control(enclosure_object_id, &debugControl);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for the pdo to transition from ready state
    status = fbe_wait_for_object_to_transiton_from_lifecycle_state(pdo_object_id,
                                                                   FBE_LIFECYCLE_STATE_READY,
                                                                   20000,
                                                                   FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_test_pp_util_reinsert_drive_hw(fbe_u32_t port_number,
                                                fbe_u32_t enclosure_number,
                                                fbe_u32_t slot_number)
{
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           lastSlot = 0;
    fbe_status_t                        status;
    fbe_object_id_t                     enclosure_object_id = FBE_OBJECT_ID_INVALID;
    fbe_base_object_mgmt_drv_dbg_ctrl_t debugControl;

    // update the encl number 
    location.bus = port_number;
    location.enclosure = enclosure_number;
    location.slot = slot_number;

    // get enclosure object id
    status = fbe_api_get_enclosure_object_id_by_location(location.bus, location.enclosure, &enclosure_object_id);
    if ((enclosure_object_id == FBE_OBJECT_ID_INVALID) || (status != FBE_STATUS_OK))
    {	
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't get object ID for bus and enclosure");
    }

    // need the number of slots in this enclosure
    status = fbe_api_enclosure_get_slot_count(enclosure_object_id, &lastSlot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // send command to remove the drive
    fbe_zero_memory(&debugControl, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
    debugControl.driveDebugAction[location.slot] = FBE_DRIVE_ACTION_INSERT;
    debugControl.driveCount = lastSlot;
    status = fbe_api_enclosure_send_drive_debug_control(enclosure_object_id, &debugControl);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for the pdo to transition to ready state
    status = fbe_test_pp_util_verify_pdo_state(location.bus, location.enclosure, location.slot, 
                                               FBE_LIFECYCLE_STATE_READY, 20000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 * fbe_test_sep_util_set_lifecycle_debug_flags
 *****************************************************************************
 * @brief  This function handles the object tracing flag.
 * 
 * @param flags - Lifecycle debug flag
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_sep_util_set_lifecycle_debug_trace_flag(fbe_u32_t flags)
{
    fbe_object_id_t trace_object;
    fbe_u32_t cmd_line_flags = 0;
    fbe_status_t status;

    if(fbe_test_sep_util_get_cmd_option_int("-lifecycle_state_debug_flags", &cmd_line_flags))
    {
        /* We only handle the object level lifecycle tracing over here.
         * The SEP and CLASS level lifecycle tracing has already been handled
         * before objects are created.
         */
        if(cmd_line_flags == FBE_LIFECYCLE_DEBUG_FLAG_OBJECT_TRACING)
        {
            fbe_test_sep_util_get_cmd_option_int("-object_id", &trace_object);
            if(trace_object >= FBE_OBJECT_ID_INVALID)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid object: 0x%x\n", 
                        __FUNCTION__, trace_object);
                return;
            }
            status = fbe_test_sep_util_enable_object_lifecycle_trace(trace_object);
            if(status != FBE_STATUS_OK)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s Can't enable lifecycle trace flags for object: 0x%x\n", 
                    __FUNCTION__, trace_object);
            }
            else
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s Enabled lifecycle trace flags for object: 0x%x\n", 
                    __FUNCTION__, trace_object);
            }
        }
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_set_lifecycle_debug_flags()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_set_lifecycle_trace_control_flag()
 ****************************************************************
 * @brief
 *  Set the lifecycle debug trace flag.
 *
 * @param new_err_flags - new error flags.               
 *
 * @return status
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_lifecycle_trace_control_flag(fbe_u32_t flag)
{
    fbe_api_lifecycle_debug_trace_control_t trace_control;
    fbe_status_t status = FBE_STATUS_OK;
    
    trace_control.trace_flag = flag;

    status = fbe_api_lifecycle_set_debug_trace_flag(&trace_control,FBE_PACKAGE_ID_SEP_0);

    return status;
}
/**************************************
 * end fbe_test_sep_util_set_lifecycle_trace_control_flag()
 **************************************/
/*!**************************************************************
 * fbe_test_sep_util_enable_object_lifecycle_trace()
 ****************************************************************
 * @brief
 *  Set the lifecycle debug trace flag for the specified object.
 *
 * @param fbe_object_id_t - Object ID
 *
* @return fbe_status_t.
 *
 * @author
 *  02/13/2012 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_enable_object_lifecycle_trace(fbe_object_id_t object_id)
{
    fbe_api_trace_level_control_t   control;
    fbe_status_t status = FBE_STATUS_OK;

    control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
    control.fbe_id = object_id;
    control.trace_level = FBE_TRACE_LEVEL_INFO;
    control.trace_flag = FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE;
    status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);

    return status;
}
/****************************************************
 * end fbe_test_sep_util_enable_object_lifecycle_trace()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_disable_object_lifecycle_trace()
 ****************************************************************
 * @brief
 *  Disable the lifecycle debug trace flag for the specified object.
 *
 * @param fbe_object_id_t - Object ID
 *
* @return fbe_status_t.
 *
 * @author
 *  02/13/2012 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_disable_object_lifecycle_trace(fbe_object_id_t object_id)
{
    fbe_api_trace_level_control_t   control;
    fbe_status_t status = FBE_STATUS_OK;

    control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
    control.fbe_id = object_id;
    control.trace_level = FBE_TRACE_LEVEL_INFO;
    control.trace_flag = 0;
    status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);

    return status;
}
/****************************************************
 * end fbe_test_sep_util_disable_object_lifecycle_trace()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_update_position_info()
 *****************************************************************************
 * 
 * @brief   Update the information (i.e. drive location) for a position in the
 *          raid group.  This can be require after a copy operation completes.
 *
 * @param   rg_config_p - Pointer to raid group config          
 * @param   inserted_position - Position that was inserted
 * 
 * @return  None
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_update_position_info(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t position_to_update,
                                                    fbe_test_raid_group_disk_set_t *original_disk_p,
                                                    fbe_test_raid_group_disk_set_t *new_disk_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t pvd_object_id;
    fbe_api_provision_drive_info_t provision_drive_info;

    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_TRUE(position_to_update < rg_config_p->width)

    /* Validate the new information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(new_disk_p->bus,
                                                            new_disk_p->enclosure,
                                                            new_disk_p->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display some information and update it.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Updating rg obj: 0x%x pos: %d from: (%d_%d_%d) to: (%d_%d_%d) pvd obj: 0x%x", 
               __FUNCTION__, rg_object_id, position_to_update,
               original_disk_p->bus, original_disk_p->enclosure, original_disk_p->slot,
               new_disk_p->bus, new_disk_p->enclosure, new_disk_p->slot, pvd_object_id); 
    *(&rg_config_p->rg_disk_set[position_to_update]) = *new_disk_p;

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_test_rg_is_degraded()
 ****************************************************************
 * @brief
 *  Check if the input raid group is degraded.
 *
 * @param rg_config_p - RG to check.               
 *
 * @return FBE_TRUE if degraded, FBE_FALSE otherwise.   
 *
 * @author
 *  3/27/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_test_rg_is_degraded(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_object_id_t rg_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t ds_index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Check for degraded on each mirrored pair.
         */
        for (ds_index = 0; 
              ds_index < downstream_object_list.number_of_downstream_objects; 
              ds_index++)
        {
            fbe_bool_t b_degraded;
            status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[ds_index], 
                                                 &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            b_degraded = fbe_api_raid_group_info_is_degraded(&raid_group_info);
            if (b_degraded)
            {
                return FBE_TRUE;
            }
        }
    }
    else
    {
        return fbe_api_raid_group_info_is_degraded(&raid_group_info);
    }
    return FBE_FALSE;
}
/******************************************
 * end fbe_test_rg_is_degraded()
 ******************************************/
/*!**************************************************************
 * fbe_test_rg_get_degraded_bitmask()
 ****************************************************************
 * @brief
 *  Get the position bitmask of degraded.
 *
 * @param rg_object_id - rg's object id.
 * @param degraded_bitmask_p - bitmask of degraded positions.
 *
 * @return None.
 *
 * @author
 *  6/8/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_rg_get_degraded_bitmask(fbe_object_id_t rg_object_id,
                                      fbe_raid_position_bitmask_t *degraded_bitmask_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_api_raid_group_get_info_t raid_group_info;

    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    *degraded_bitmask_p = 0;
    for (index = 0; index < raid_group_info.width; index++)
    {
        if ((raid_group_info.b_rb_logging[index]) || 
            (raid_group_info.rebuild_checkpoint[index] != FBE_LBA_INVALID))
        {
            *degraded_bitmask_p |= (1 << index);
        }
    }
    return;
}
/******************************************
 * end fbe_test_rg_get_degraded_bitmask()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_wait_for_degraded_bitmask()
 ****************************************************************
 * @brief
 *  This function is used to wait for a pvd to reach a given lifecycle state.
 * 
 *
 * @return fbe_status - status of disable request
 *
 * @author
 *  11/12/2010 - Created. Rob Foley
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_wait_for_degraded_bitmask(fbe_object_id_t rg_object_id,
                                            fbe_raid_position_bitmask_t expected_degraded_bitmask,
                                            fbe_u32_t max_wait_msecs)
{
    fbe_u32_t remaining_msecs = max_wait_msecs;
    fbe_raid_position_bitmask_t degraded_bitmask;

    /* We continue looping until we get the pvd object id. 
     */
    while (remaining_msecs > 0)
    {
        fbe_test_rg_get_degraded_bitmask(rg_object_id, &degraded_bitmask);

        if (degraded_bitmask == expected_degraded_bitmask)
        {
            return FBE_STATUS_OK;
        }
        fbe_api_sleep(500);
        remaining_msecs -= 500;
    }
    return FBE_STATUS_TIMEOUT;
}
/**************************************
 * end fbe_test_sep_util_wait_for_degraded_bitmask()
 **************************************/
/*!**************************************************************
 * fbe_test_sep_util_validate_no_rg_verifies()
 ****************************************************************
 * @brief
 *  First mark the raid group for verify.
 *  A debug hook prevents verify from running.
 *  Validate that no verifies are marked following a rebuild.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None 
 *
 * @author
 *  3/23/2012 - Created. Rob Foley
 ****************************************************************/

void fbe_test_sep_util_validate_no_rg_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_api_raid_group_get_info_t rg_info;

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* This is here for debugging.  If the below check fails we will want to have this info handy.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If the rebuild is done, no chunks should be needing a verify.
         */
        if ((paged_info.num_error_vr_chunks > 0) || 
            (paged_info.num_incomplete_write_vr_chunks > 0) || 
            (paged_info.num_system_vr_chunks > 0) ||             
            (paged_info.num_read_only_vr_chunks > 0) || 
            (paged_info.num_user_vr_chunks > 0))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks err_vr:%d iw_vr: %d sys_vr:%d ro_vr: %d us_vr: %d", 
                       __FUNCTION__, rg_object_id, (int)paged_info.num_error_vr_chunks, 
                       (int)paged_info.num_incomplete_write_vr_chunks, (int)paged_info.num_system_vr_chunks, (int)paged_info.num_read_only_vr_chunks, (int)paged_info.num_user_vr_chunks );
            MUT_FAIL_MSG("There are chunks needing verify on SP A");
        }

        /* In the case of a dual sp test we will also check the peer since 
         * some of this information can be cached on the peer.  We want to make sure it is correct there also. 
         */
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if ((paged_info.num_error_vr_chunks > 0) || 
                (paged_info.num_incomplete_write_vr_chunks > 0) || 
                (paged_info.num_system_vr_chunks > 0) ||
                (paged_info.num_read_only_vr_chunks > 0) || 
                (paged_info.num_user_vr_chunks > 0))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks err_vr:%d iw_vr: %d sys_vr: %d ro_vr: %d us_vr: %d", 
                           __FUNCTION__, rg_object_id, (int)paged_info.num_error_vr_chunks, 
                           (int)paged_info.num_incomplete_write_vr_chunks, (int)paged_info.num_system_vr_chunks, (int)paged_info.num_read_only_vr_chunks, (int)paged_info.num_user_vr_chunks );
                MUT_FAIL_MSG("There are chunks needing verify on SP B");                
            }
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end fbe_test_sep_util_validate_no_rg_verifies()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_for_lun_zeroing_complete()
 ****************************************************************
 * @brief
 *  Wait for lun zeroing to be completed.
 *
 * @param lun_number
 * @param timeout_ms - Timeout in milliseconds.
 * @package_id
 * 
 * @return fbe_status_t 
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_wait_for_lun_zeroing_complete(fbe_u32_t lun_number, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_lun_get_zero_status_t       lu_zero_status;
    fbe_object_id_t                     lu_object_id;
    fbe_bool_t                          is_msg_present = FBE_FALSE;
    #define FBE_TEST_ZERO_POLLING_INTERVAL 200
    /* get the lun zero percentange after we start disk zeroing. */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    while (current_time < timeout_ms){

        status = fbe_api_lun_get_zero_status(lu_object_id, &lu_zero_status);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "%s: lu_object_id 0x%x has completed %d zeroing\n", 
                      __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage);

        /* check if disk zeroing completed */
        if (lu_zero_status.zero_percentage == 100)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "LUN Zeroing completed for object 0x%x\n", lu_object_id);
            return status;
        }

        current_time = current_time + FBE_TEST_ZERO_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_ZERO_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: lu_object_id %d percentage-0x%x, event msg present %d, %d ms!, \n", 
                  __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage, is_msg_present, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************************
 * end fbe_test_sep_util_wait_for_lun_zeroing_complete()
 *******************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_clear_disk_eol()
 *****************************************************************************
 * 
 * @brief   Clear the EOL (End Of Life) for the specified disk.  A
 *          usurper is sent to the correct physical drive to clear the EOL
 *          attribute.
 *
 * @param   bus - The bus number of the disk to clear EOL for       
 * @param   enclosure - The enclosure number of the disk to clear EOL for       
 * @param   slot - The slot number of the disk to clear EOL for       
 * 
 * @return  status - FBE_STATUS_OK unless the drive isn't located.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_clear_disk_eol(fbe_u32_t bus,
                                              fbe_u32_t enclosure,
                                              fbe_u32_t slot)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_object_id_t                                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                                 pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t                  provision_drive_info = {0};
    fbe_u32_t                                       wait_attempts = 0;
    fbe_physical_drive_set_dieh_media_threshold_t   threshold;
    fbe_bool_t                                      b_is_dualsp_mode;
    fbe_bool_t                                      b_send_to_both = FBE_FALSE;
    fbe_transport_connection_target_t               this_sp;
    fbe_transport_connection_target_t               other_sp;
    fbe_cmi_service_get_info_t                      cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    ) 
    {
        b_send_to_both = FBE_TRUE;
    }

    /* Get the physical drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,
                                                              enclosure,
                                                              slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((pdo_object_id == FBE_OBJECT_ID_INVALID) && (wait_attempts < 20))
    {
        /* Wait until we can get pvd object id.
         */
        status = fbe_api_get_physical_drive_object_id_by_location(bus, 
                                                                 enclosure, 
                                                                 slot, 
                                                                 &pdo_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_sleep(500);
        }
        wait_attempts++;
    }

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    wait_attempts = 0;
    while ((pvd_object_id == FBE_OBJECT_ID_INVALID) && (wait_attempts < 20))
    {
        /* Wait until we can get pvd object id.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                enclosure,
                                                                slot,
                                                                &pvd_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_sleep(500);
        }
        wait_attempts++;
    }

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Always clear marked offline first.
     */
    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Check and clear EOL*/
    if(provision_drive_info.end_of_life_state == FBE_TRUE)
    {
        /* Finally clear the EOL in the provision drive.
         */
        status = fbe_api_provision_drive_clear_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Display some information.                 
         */                                          
        mut_printf(MUT_LOG_TEST_STATUS, "%s EOL cleared for pvd: 0x%x disk: 0x%x(%d_%d_%d) ", 
                   __FUNCTION__, pvd_object_id, pdo_object_id,
                   bus, enclosure, slot);
    }
    else
    {
        /* We always expect the EOL is set.. But there are cases where it wont be set.. 
         * in that case juz return a status.. 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s EOL is NOT set.. so no need to clear. pvd: 0x%x disk: 0x%x(%d_%d_%d) ", 
                   __FUNCTION__, pvd_object_id, pdo_object_id,
                   bus, enclosure, slot);
    }

    /* Always set the drive thresholds back to defaults.
     */
    threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS;
    threshold.option = 0; 
    status = fbe_api_physical_drive_set_dieh_media_threshold(pdo_object_id, &threshold);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (b_send_to_both)
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_get_physical_drive_object_id_by_location(bus, 
                                                                  enclosure, 
                                                                  slot, 
                                                                  &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_physical_drive_set_dieh_media_threshold(pdo_object_id, &threshold);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_test_sep_util_validate_drive_fault_state()
 *****************************************************************************
 * 
 * @brief   Validate that within the timeout period the `Drive Fault' for the 
 *          specified disk is the state expected.
 *              o FBE_TRUE
 *                  OR
 *              o FBE_FALSE
 *
 * @param   bus - The bus number of the disk to validate the Drive Fault for       
 * @param   enclosure - The enclosure number of the disk to validate Drive Fault for       
 * @param   slot - The slot number of the disk to validate Drive Fault for  
 * @param   b_expected_state_is_set - FBE_TRUE - We expect the state to be FBE_TRUE
 *                                  - FBE_FALSE - We expect the state to be FBE_FALSE     
 * 
 * @return  status - FBE_STATUS_OK unless the drive isn't located.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_validate_drive_fault_state(fbe_u32_t bus,
                                                          fbe_u32_t enclosure,
                                                          fbe_u32_t slot,
                                                          fbe_bool_t b_expected_state_is_set)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     pvd_object_id;
    fbe_api_provision_drive_info_t      provision_drive_info;
    fbe_u32_t                           max_wait_in_seconds = 10;
    fbe_u32_t                           sleep_time;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   other_sp = FBE_TRANSPORT_SP_B;
    fbe_bool_t                          b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();                  

    /* Get and validate the pvd object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

    /* First get the sp id this was invoked on and out peer.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    other_sp = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    for (sleep_time = 0; sleep_time < (max_wait_in_seconds*10); sleep_time++)
    {
        /* First wait for SPA then wait for SPB.
         */

        /* Get the info for this target.
         */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If the state matches the requested check if we need to wait for SPB.
         */
        if (provision_drive_info.drive_fault_state == b_expected_state_is_set)
        {
            /* If dualsp and not SPB set to SPB.
             */
            if ((b_is_dualsp_mode == FBE_TRUE)                      &&
                (fbe_api_transport_get_target_server() != other_sp)    )
            {
                fbe_api_transport_set_target_server(other_sp);
            }
            else
            {
                /* Else we are done.  Set the target back to the original and
                 * return success.
                 */
                fbe_api_transport_set_target_server(target_invoked_on);
                return FBE_STATUS_OK;
            }
        }

        // Sleep 100 miliseconds
        fbe_api_sleep(100);
    }

    /* Took to long.
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS, "%s Validate drive fault of: %d for pvd: 0x%x(%d_%d_%d) took more than: %d seconds", 
               __FUNCTION__, b_expected_state_is_set, pvd_object_id, bus, enclosure, slot, max_wait_in_seconds);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}

/*!***************************************************************************
 *          fbe_test_sep_util_set_drive_fault()
 *****************************************************************************
 * 
 * @brief   Set the `Drive Fault' for the specified disk.  A
 *          usurper is sent to the correct physical drive to set the `Drive
 *          Fault' condition.
 *
 * @param   bus - The bus number of the disk to set the Drive Fault for       
 * @param   enclosure - The enclosure number of the disk to set Drive Fault for       
 * @param   slot - The slot number of the disk to set Drive Fault for       
 * 
 * @return  status - FBE_STATUS_OK unless the drive isn't located.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_set_drive_fault(fbe_u32_t bus,
                                               fbe_u32_t enclosure,
                                               fbe_u32_t slot)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t pvd_object_id;
    fbe_object_id_t drive_object_id;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get and validate the pvd object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_FALSE(provision_drive_info.drive_fault_state);

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,
                                                              enclosure,
                                                              slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set the `drive fault' in the pdo.
     */
    status = fbe_api_physical_drive_update_drive_fault(drive_object_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the `drive fault' state to be set in the associated pvd.
     */
    status = fbe_test_sep_drive_verify_pvd_state(bus, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_FAIL,
                                                 30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the drive fault state is True.
     */
    status = fbe_test_sep_util_validate_drive_fault_state(bus, enclosure, slot,
                                                          FBE_TRUE /* Validate it is set*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display some information.                 
     */                                          
    mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Fault for pvd: 0x%x disk: 0x%x(%d_%d_%d) ", 
               __FUNCTION__, pvd_object_id, drive_object_id,
               bus, enclosure, slot);

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_test_sep_util_clear_drive_fault()
 *****************************************************************************
 * 
 * @brief   Clear the `Drive Fault' for the specified disk.  A
 *          usurper is sent to the correct physical drive to clear the `Drive
 *          Fault' condition.
 *
 * @param   bus - The bus number of the disk to clear the Drive Fault for       
 * @param   enclosure - The enclosure number of the disk to clear Drive Fault for       
 * @param   slot - The slot number of the disk to clear Drive Fault for       
 * 
 * @return  status - FBE_STATUS_OK unless the drive isn't located.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_clear_drive_fault(fbe_u32_t bus,
                                               fbe_u32_t enclosure,
                                               fbe_u32_t slot)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t pvd_object_id;
    fbe_object_id_t drive_object_id;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get and validate the pvd object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(provision_drive_info.drive_fault_state);

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus,
                                                              enclosure,
                                                              slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear the `drive fault' in the pdo.
     */
    status = fbe_api_physical_drive_update_drive_fault(drive_object_id, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear the marked offline in the pvd */
    status = fbe_api_provision_drive_clear_swap_pending(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear drive fault */
    status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the drive fault state is False.
     */
    status = fbe_test_sep_util_validate_drive_fault_state(bus, enclosure, slot,
                                                          FBE_FALSE /* Validate it is clear*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display some information.                 
     */                                          
    mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Fault for pvd: 0x%x disk: 0x%x(%d_%d_%d) ", 
               __FUNCTION__, pvd_object_id, drive_object_id,
               bus, enclosure, slot);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_test_sniff_disable_all_pvds()
 ****************************************************************
 * @brief
 *  Disable sniffing on all pvd objects.
 *
 * @param 
 *
 * @return None.   
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sniff_disable_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for all objects of class to get to this state.
     */
    for ( index = 0; index < num_objects; index++)
    {
        status = fbe_api_base_config_disable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    fbe_api_free_memory(obj_list_p);
}
/******************************************
 * end fbe_test_sniff_disable_all_pvds()
 ******************************************/
/*!**************************************************************
 * fbe_test_sniff_enable_all_pvds()
 ****************************************************************
 * @brief
 *  Enable sniffing on all pvd objects.
 *
 * @param 
 *
 * @return None.   
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sniff_enable_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    /*! First get all members of this class.
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for all objects of class to get to this state.
     */
    for ( index = 0; index < num_objects; index++)
    {
        status = fbe_api_base_config_disable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);

        if (status != FBE_STATUS_OK)
        {
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
}
/******************************************
 * end fbe_test_sniff_enable_all_pvds()
 ******************************************/

/*!**************************************************************
 * fbe_test_copy_wait_for_completion()
 ****************************************************************
 * @brief
 *  Wait for our copy to complete.
 *  There is an assumption that we have previously kicked off the copy.
 *
 * @param vd_object_id - Object id to wait for.
 * @param dest_edge_index - Edge index that is the destination of the copy.
 *
 * @return None.
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_copy_wait_for_completion(fbe_object_id_t vd_object_id,
                                               fbe_u32_t dest_edge_index)
{
    fbe_status_t                    status;         
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the copy. vd_object_id: 0x%x dest_index: 0x%x", 
               vd_object_id, dest_edge_index);

    /* Loop until copy finishes.
     */
    while (current_time < timeout_ms)
    {
        /*  get the raid group information */
        status = fbe_api_raid_group_get_info(vd_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*  if the rebuild checkpoint is set to the logical marker for the given disk, then the rebuild has 
         *  finished - return here.
         */
        if (raid_group_info.rebuild_checkpoint[dest_edge_index] == FBE_LBA_INVALID)
        {
            fbe_api_virtual_drive_get_configuration_t   get_configuration;

            /* We also wait for the swap out.
             */
            status = fbe_api_virtual_drive_get_configuration(vd_object_id, &get_configuration);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if ((get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
                (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
            {
                return FBE_STATUS_OK;
            }
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }
    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: object %d did not complete copy in %d ms!\n", 
                  __FUNCTION__, vd_object_id, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_test_copy_wait_for_completion()
 ******************************************/
/*!**************************************************************
 * fbe_test_verify_get_verify_start_substate()
 ****************************************************************
 * @brief
 *  Get the correct verify substate where we are about to
 *  send a verify request for a particular verify type.
 *
 * @param verify_type - Type of verify to return substate for.
 * @param substate_p - Ptr to return substate
 *
 * @return None
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_verify_get_verify_start_substate(fbe_lun_verify_type_t verify_type,
                                               fbe_u32_t *substate_p)
{
    *substate_p = FBE_SCHEDULER_MONITOR_SUB_STATE_INVALID;;
    switch (verify_type)
    {
        case FBE_LUN_VERIFY_TYPE_USER_RO:
            *substate_p = FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START;
            break;
        case FBE_LUN_VERIFY_TYPE_USER_RW:
            *substate_p = FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START;
            break;
        case FBE_LUN_VERIFY_TYPE_ERROR:
            *substate_p = FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT;
            break;
        case FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE:
            *substate_p = FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START;
            break;
        case FBE_LUN_VERIFY_TYPE_SYSTEM:
            *substate_p = FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START;
            break;
        default:
            mut_printf(MUT_LOG_TEST_STATUS, "verify type %d unknown\n", verify_type);
            MUT_FAIL_MSG("unknown verify type");
    };
}
/******************************************
 * end fbe_test_verify_get_verify_start_substate()
 ******************************************/

/*!**************************************************************
 * fbe_test_verify_wait_for_verify_completion()
 ****************************************************************
 * @brief
 *  Wait for the raid group verify chunk count to goto zero.
 *
 * @param rg_object_id - Object id to wait on
 * @param wait_seconds - max seconds to wait
 *
 * @param verify type
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  3/30/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_verify_wait_for_verify_completion(fbe_object_id_t rg_object_id,
                                                        fbe_u32_t wait_ms,
                                                        fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_lba_t chkpt;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < wait_ms)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_verify_checkpoint(&rg_info, verify_type, &chkpt);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (chkpt == FBE_LBA_INVALID)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       (unsigned long long)rg_info.error_verify_checkpoint);
            break;
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_ms * 1000))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x timed out waiting for verify %d msecs",
                   rg_object_id, elapsed_msec);
        MUT_FAIL_MSG("timed out in wait for verify");
    }
    return status;
}
/******************************************
 * end fbe_test_verify_wait_for_verify_completion()
 ******************************************/
/*!**************************************************************
 * fbe_test_rg_info_verify_is_finished()
 ****************************************************************
 * @brief
 *  Determine if verify is done.
 *
 * @param rg_info_p 
 *
 * @return fbe_bool_t 
 *
 * @author
 *  10/27/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_test_rg_info_verify_is_finished(fbe_api_raid_group_get_info_t *rg_info_p)
{
    if ((rg_info_p->rw_verify_checkpoint == FBE_LBA_INVALID) &&
        (rg_info_p->ro_verify_checkpoint == FBE_LBA_INVALID) &&
        (rg_info_p->error_verify_checkpoint == FBE_LBA_INVALID) &&
        (rg_info_p->system_verify_checkpoint == FBE_LBA_INVALID) &&
        (rg_info_p->incomplete_write_verify_checkpoint == FBE_LBA_INVALID) &&
        (rg_info_p->b_is_event_q_empty == FBE_TRUE)) {
        return FBE_TRUE; 
    }
    return FBE_FALSE; 
}
/******************************************
 * end fbe_test_rg_info_verify_is_finished()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_wait_for_raid_group_verify()
 ****************************************************************
 * @brief
 *  Wait for the raid group verify chunk count to goto zero.
 *  Also, make sure the event q is empty to insure no verifies
 *  could be waiting on the event q.
 *
 * @param rg_object_id - Object id to wait on
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  11/02/2012, copied from abby_cadabby_wait_for_raid_group_verify()
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_raid_group_verify(fbe_object_id_t rg_object_id,
                                                          fbe_u32_t wait_seconds)
{
     fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_api_raid_group_get_info_t mirror_rg_info;
            fbe_bool_t b_finished = FBE_TRUE;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            for (index = 0;
                 index < downstream_object_list.number_of_downstream_objects;
                 index++) {
                status = fbe_api_raid_group_get_info(downstream_object_list.downstream_object_list[index], 
                                                     &mirror_rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                mut_printf(MUT_LOG_TEST_STATUS, "mirror obj: 0x%x rw: %llx ro: %llx err: %llx sys: %llx iw: %llx eq: %u",
                           downstream_object_list.downstream_object_list[index],
                           mirror_rg_info.rw_verify_checkpoint,
                           mirror_rg_info.ro_verify_checkpoint,
                           mirror_rg_info.error_verify_checkpoint,
                           mirror_rg_info.system_verify_checkpoint,
                           mirror_rg_info.incomplete_write_verify_checkpoint, mirror_rg_info.b_is_event_q_empty);
                if (!fbe_test_rg_info_verify_is_finished(&mirror_rg_info)) {
                    b_finished = FBE_FALSE;
                    break;
                }
            }
            if (b_finished) {
                break;
            }
        } else if (fbe_test_rg_info_verify_is_finished(&rg_info)) {
            mut_printf(MUT_LOG_TEST_STATUS, "obj: 0x%x rw: %llx ro: %llx err: %llx sys: %llx iw: %llx eq: %u",
                       rg_object_id,
                       rg_info.rw_verify_checkpoint,
                       rg_info.ro_verify_checkpoint,
                       rg_info.error_verify_checkpoint,
                       rg_info.system_verify_checkpoint,
                       rg_info.incomplete_write_verify_checkpoint, rg_info.b_is_event_q_empty);
            break;
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }
    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000))
    {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_for_raid_group_verify()
 ******************************************/
/*!****************************************************************************
 *  fbe_test_zero_wait_for_zero_on_demand_state
 ******************************************************************************
 *
 * @brief
 *   This function checks zero_on_demand state
 *
 * @param object_id    - provision drive object id
 * @param timeout_ms   - max timeout value to check for ZOD state
 * @param zod_state    - expected ZOD state
 * 
 * @return fbe_status_t              fbe status 
 *
 * @author
 *  9/17/2012 - Created. NChiu
 *
 *****************************************************************************/
fbe_status_t fbe_test_zero_wait_for_zero_on_demand_state(fbe_object_id_t object_id, fbe_u32_t timeout_ms, fbe_bool_t zod_state)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_api_provision_drive_info_t pvd_info;
    
    while (current_time < timeout_ms){

        /* get the zero on demand state */
        status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
       
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get PVD info failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        /* check if disk zeroing completed */
        if (pvd_info.zero_on_demand == zod_state) {
            return FBE_STATUS_OK;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d zero_on_demand-0x%x, %d ms!\n", 
                  __FUNCTION__, object_id, pvd_info.zero_on_demand, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/**************************************
 * end fbe_test_zero_wait_for_zero_on_demand_state()
 **************************************/

/*!****************************************************************************
 *  fbe_test_zero_wait_for_disk_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing complete status with reading checkpoint 
 *    in loop. It exits only when disk zeroing completed or time our occurred.
 *
 * @param object_id    - provision drive object id
 * @param timeout_ms   - max timeout value to check for disk zeroing  complete
 * 
 * @return fbe_status_t              fbe status 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/

fbe_status_t fbe_test_zero_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, fbe_u32_t timeout_ms)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lba_t               zero_checkpoint;
    fbe_u16_t               zero_percentage;
    
    while (current_time < timeout_ms){

        /* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &zero_checkpoint); 
        
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }
        /* test the API for disk zeroing percentage */
        status = fbe_api_provision_drive_get_zero_percentage(object_id, &zero_percentage);       

        if (status != FBE_STATUS_OK)
        {                    
            return status;
        }
        /* check if disk zeroing completed */
        if (zero_checkpoint == FBE_LBA_INVALID)
        {
            
            /* test the API for disk zeroing percentage completed */
            MUT_ASSERT_INT_EQUAL(zero_percentage, 100); 

            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%x, %d ms!\n", 
                  __FUNCTION__, object_id, (unsigned int)zero_checkpoint, timeout_ms);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return FBE_STATUS_GENERIC_FAILURE;
}
/**************************************
 * end fbe_test_zero_wait_for_disk_zeroing_complete()
 **************************************/
/*!****************************************************************************
 *  fbe_test_zero_wait_for_rg_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing complete status with reading checkpoint 
 *    in loop. It exits only when disk zeroing completed or time our occurred.
 *
 * @param rg_config_p - Configuration
 * 
 * @return fbe_status_t              fbe status 
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_test_zero_wait_for_rg_zeroing_complete(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t pvd_object_id;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_position;

    for (rg_position = 0; rg_position < rg_config_p->width; rg_position++)
    {
        fbe_u32_t current_removed;
        fbe_bool_t b_removed = FBE_FALSE;
        /* If the drive is removed, skip it.
         */
        for (current_removed = 0; current_removed < rg_config_p->num_removed; current_removed++)
        {
            if (current_rg_config_p->drives_removed_array[current_removed] == rg_position)
            {
                b_removed = FBE_TRUE;
            }
        }
        if (b_removed)
        {
            continue;
        }
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[rg_position].bus,
                                                                current_rg_config_p->rg_disk_set[rg_position].enclosure,
                                                                current_rg_config_p->rg_disk_set[rg_position].slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/**************************************
 * end fbe_test_zero_wait_for_rg_zeroing_complete()
 **************************************/

/*!**************************************************************
 * fbe_test_copy_determine_dest_index()
 ****************************************************************
 * @brief
 *  Determine which will be the destination index for 
 *  a vd that will be starting a copy.
 *
 * @param vd_object_ids_p - Array of object ids.
 * @param vd_count - number of object ids in array.
 * @param dest_index_p - Array of destination indexes,
 *                       should fill in vd_count worth of indexes.
 *
 * @return None.
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_copy_determine_dest_index(fbe_object_id_t *vd_object_ids_p,
                                        fbe_u32_t vd_count,
                                        fbe_u32_t *dest_index_p)
{
    fbe_status_t status;
    fbe_api_virtual_drive_get_configuration_t   get_configuration;
    fbe_u32_t vd_index;

    for ( vd_index = 0; vd_index < vd_count; vd_index++)
    {
        status = fbe_api_virtual_drive_get_configuration(vd_object_ids_p[vd_index],
                                                         &get_configuration);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
        {
            dest_index_p[vd_index] = 1;
        }
        else if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
        {
            dest_index_p[vd_index] = 0;
        }
        else
        {
            MUT_FAIL_MSG("Unknown configuration mode");
        }
    }
}
/******************************************
 * end fbe_test_copy_determine_dest_index()
 ******************************************/
/*!**************************************************************
 * fbe_test_rg_populate_extra_drives()
 ****************************************************************
 * @brief
 *  Initialize the rg config with number of extra drive required for each rg. This 
 *  information will use by a test while removing a drive and insert a new drive.
 *
 * @note   
 *  this function only needs to call from a fbe_test which needs to support
 *  use case of remove a drive and insert a new drive in simulation or on hardware. 
 *  By default extra drive count remains set to zero for a test which does not need 
 *  functionality to insert a new drive.
 *
 * @param rg_config_p - Array of raidgroup to determine how many extra drive needed 
 *                                for each rg and to populate extra drive count.
 *
 * 
 *
 * @return None.
 *
 * @author
 *  4/7/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_rg_populate_extra_drives(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_u32_t extra_drives)
{
    fbe_u32_t   rg_index;
    fbe_u32_t   num_req_extra_drives;
    fbe_u32_t   num_raid_groups;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            fbe_test_sep_util_get_rg_type_num_extra_drives(rg_config_p[rg_index].raid_type,
                                                        rg_config_p[rg_index].width,
                                                        &num_req_extra_drives);

            /* set the required number of extra drives for each rg in rg config 
            */      
            num_req_extra_drives = num_req_extra_drives + extra_drives;  
            rg_config_p[rg_index].num_of_extra_drives = num_req_extra_drives;
        }
    }

    return;
}
/***********************************************
 * end fbe_test_rg_populate_extra_drives()
 ***********************************************/

/*!**************************************************************
 * fbe_test_get_rg_object_ids()
 ****************************************************************
 * @brief
 *  Fetch the array of rg object ids for this test.
 *
 * @param rg_config_p - Configuration under test.
 * @param rg_object_ids_p - Ptr to array of object ids to return.
 *
 * @return None   
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_get_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_object_id_t *rg_object_ids_p)
{   
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_object_id_t striper_object_id;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &striper_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            
            fbe_test_sep_util_get_ds_object_list(striper_object_id, &downstream_object_list);
            MUT_ASSERT_INT_NOT_EQUAL(downstream_object_list.number_of_downstream_objects, 0);

            /* For now we always choose the first mirror.
             */
            rg_object_ids_p[rg_index] = downstream_object_list.downstream_object_list[0];
        }
        else
        {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_ids_p[rg_index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_ids_p[rg_index], FBE_OBJECT_ID_INVALID);
        current_rg_config_p++;
    }
}
/**************************************
 * end fbe_test_get_rg_object_ids
 **************************************/
/*!**************************************************************
 * fbe_test_get_top_rg_object_ids()
 ****************************************************************
 * @brief
 *  Fetch the array of rg object ids for this test.
 *
 * @param rg_config_p - Configuration under test.
 * @param rg_object_ids_p - Ptr to array of object ids to return.
 *
 * @return None   
 *
 * @author
 *  2/28/2014 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_get_top_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_object_id_t *rg_object_ids_p)
{   
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_ids_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_ids_p[rg_index], FBE_OBJECT_ID_INVALID);
        current_rg_config_p++;
    }
}
/**************************************
 * end fbe_test_get_top_rg_object_ids
 **************************************/
/*!**************************************************************
 * fbe_test_get_rg_ds_object_id()
 ****************************************************************
 * @brief
 *  Fetch the downstream objects for a given raid group
 *
 * @param rg_config_p - Configuration under test.
 * @param rg_object_id_p - Ptr to object id to return.
 * @@param mirror_index - If this is a raid 10 the mirror to fetch
 *                        the downstream objects for.
 *
 * @return None   
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_get_rg_ds_object_id(fbe_test_rg_configuration_t *rg_config_p,
                                  fbe_object_id_t *rg_object_id_p,
                                  fbe_u32_t mirror_index)
{   
    fbe_status_t status;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_object_id_t striper_object_id;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &striper_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_test_sep_util_get_ds_object_list(striper_object_id, &downstream_object_list);
        MUT_ASSERT_INT_NOT_EQUAL(downstream_object_list.number_of_downstream_objects, 0);

        /* For now we always choose the first mirror.
         */
        *rg_object_id_p = downstream_object_list.downstream_object_list[mirror_index];
    }
    else
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, rg_object_id_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/**************************************
 * end fbe_test_get_rg_ds_object_id
 **************************************/
/*!**************************************************************
 * fbe_test_get_pvd_object_ids()
 ****************************************************************
 * @brief
 *  Fetch the array of pvd object ids for this test.
 *
 * @param rg_config_p - Configuration under test.
 * @param pvd_object_ids_pp - Ptr to 2d array of pvd object ids to return.
 * @param rg_position - Set to rg position to fetch for PVD.
 *                      If this is negative, it is relative to end of rg.
 *
 * @return None   
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_get_pvd_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_object_id_t *pvd_object_ids_p,
                                      fbe_u32_t rg_position)
{   

    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t current_rg_position;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        if (rg_position == FBE_U32_MAX)
        {
            current_rg_position = current_rg_config_p->width - 1;
        }
        else
        {
            current_rg_position = rg_position;
        }
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[current_rg_position].bus,
                                                                current_rg_config_p->rg_disk_set[current_rg_position].enclosure,
                                                                current_rg_config_p->rg_disk_set[current_rg_position].slot,
                                                                &pvd_object_ids_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_get_pvd_object_ids()
 ******************************************/
/*!**************************************************************
 * fbe_test_get_vd_object_ids()
 ****************************************************************
 * @brief
 *  Fetch all the vd ids for us in our test.
 *
 * @param  rg_config_p - current test config.
 * @param vd_object_ids_p - array of vd object ids one per rg.
 *
 * @return None.
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_get_vd_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_object_id_t *vd_object_ids_p,
                                fbe_u32_t rg_position)
{   

    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t current_rg_position;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        if (rg_position == FBE_U32_MAX)
        {
            current_rg_position = current_rg_config_p->width - 1;
        }
        else
        {
            current_rg_position = rg_position;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 
                                                                           current_rg_position,
                                                                           &vd_object_ids_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_get_vd_object_ids()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_element_size()
 ****************************************************************
 * @brief
 *  Figure out element size for this RG.
 *
 * @param  rg_config_p - current test config.
 *
 * @return RG element size.
 *
 * @author
 *  4/24/2012 - Ported from Kermit. Vamsi V
 *
 ****************************************************************/
fbe_u32_t fbe_test_sep_util_get_element_size(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t elsz;
    if (rg_config_p->b_bandwidth)
    {
        elsz = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        elsz = FBE_RAID_SECTORS_PER_ELEMENT;
    }
    return elsz;
}
/******************************************
 * end fbe_test_sep_util_get_element_size()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_set_scsi_error_from_error_type()
 ****************************************************************
 * @brief
 *  This function fills the scsi error structure with the SK,ASC/ASCQ
 *  from the error type
 *
 * @param  protocol_error_p - Protocol error buffer to set the SK,ASC/ASCQ
 * @param  error_type - Type of error
 *
 * @return None
 *
 * @author
 *  06/07/2012 - Ported from Kermit. Ashok Tamilarasan
 *
 ****************************************************************/
void fbe_test_sep_util_set_scsi_error_from_error_type(fbe_protocol_error_injection_scsi_error_t *protocol_error_p,
                                                      fbe_test_protocol_error_type_t error_type)
{
    fbe_scsi_sense_key_t                        scsi_sense_key;
    fbe_scsi_additional_sense_code_t            scsi_additional_sense_code;
    fbe_scsi_additional_sense_code_qualifier_t  scsi_additional_sense_code_qualifier;
    fbe_port_request_status_t                   port_status;
    /* Map an error enum into a specific error.
     */
    switch (error_type)
    {
        case FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_ABORTED_CMD;
            scsi_additional_sense_code = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_ABORTED_CMD;
            scsi_additional_sense_code = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_WRITE_PROTECT;
            scsi_additional_sense_code = FBE_SCSI_ASC_COMMAND_NOT_ALLOWED;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_DROP:
            /* Treat it like a non-retryable error, but we also set the drop bit.
             */
            scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
            scsi_additional_sense_code = FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_MEDIUM_ERROR;
            scsi_additional_sense_code = FBE_SCSI_ASC_READ_DATA_ERROR;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_SOFT_MEDIA_ERROR:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_RECOVERED_ERROR;
            scsi_additional_sense_code = FBE_SCSI_ASC_RECOVERED_WITH_RETRIES;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        default:
            scsi_sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE;
            scsi_additional_sense_code = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
            scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO;
            port_status = FBE_PORT_REQUEST_STATUS_INVALID_REQUEST;
            MUT_FAIL_MSG("unknown error type");
            break;
    };
    protocol_error_p->scsi_sense_key = scsi_sense_key;
    protocol_error_p->scsi_additional_sense_code = scsi_additional_sense_code;
    protocol_error_p->scsi_additional_sense_code_qualifier = scsi_additional_sense_code_qualifier;
    protocol_error_p->port_status = port_status;
    return;
}

/*!**************************************************************
 * fbe_test_sep_util_get_scsi_command()
 ****************************************************************
 * @brief
 *  Returns the SCSI command for the corresponding block operation
 *
 * @param  opcode - block Opcode
 *
 * @return scsi_command
 *
 * @author
 *  06/07/2012 - Ported from Kermit. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_u8_t fbe_test_sep_util_get_scsi_command(fbe_payload_block_operation_opcode_t opcode)
{
    fbe_u8_t scsi_command;
    /* Map a block operation to a scsi command that we should inject on.
     */
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            scsi_command = FBE_SCSI_READ_16;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            scsi_command = FBE_SCSI_WRITE_16;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
            scsi_command = FBE_SCSI_WRITE_SAME_16;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
            scsi_command = FBE_SCSI_WRITE_VERIFY_16;
            break;
        default:
            MUT_FAIL_MSG("unknown opcode");
            scsi_command = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
            break;
    };
    return scsi_command;
}


/*!****************************************************************************
 *  fbe_test_sep_util_get_active_passive_sp
 ******************************************************************************
 *
 * @brief
 *    This function returns the active or passive SP from the object's perspective
 *
 * @param object_id - Object ID 
 * @param active_sp - Buffer to store which is the active SP
 * @param passive_sp - Buffer to store which is the passive SP
 *
 * @return  fbe_status_t
 *
 * @author
 *    04/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_get_active_passive_sp(fbe_object_id_t object_id,
                                                     fbe_sim_transport_connection_target_t *active_sp,
                                                     fbe_sim_transport_connection_target_t *passive_sp)
{
    fbe_sim_transport_connection_target_t target_sp;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_metadata_info_t metadata_info;
    fbe_status_t status;

    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    target_sp = fbe_api_sim_transport_get_target_server();
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* Get the active SP which will be our target SP for the commands */
    if(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        /* since the metadata element is active for the SP the API was issued to the 
         * API target SP is the active SP and the peer is the passive 
         */
        *active_sp = target_sp;
        *passive_sp = peer_sp;
    }
    else
    {
        /* since the metadata element is passive for the SP the API was issued to the 
         * API target SP is the passive SP and the peer is the active SP
         */
        *passive_sp = target_sp;
        *active_sp = peer_sp;
    }
    return status;
}

/*!**************************************************************
 * fbe_test_sep_util_set_active_target_for_bes()
 ****************************************************************
 * @brief
 *  Sets the active target for specified provision drive (b/e/s).
 *
 * @param  bus, encl, slot
 *
 * @return None
 *
 * @author
 *  07/09/2012 - Prahlad Purohit
 *
 ****************************************************************/
void
fbe_test_sep_util_set_active_target_for_bes(fbe_u32_t bus, 
                                            fbe_u32_t encl, 
                                            fbe_u32_t slot)
{
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    fbe_status_t                          status;
    fbe_object_id_t pvd_object_id;

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, encl, slot, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

    fbe_test_sep_util_get_active_passive_sp(pvd_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

/*!**************************************************************
 * fbe_test_sep_util_set_active_target_for_pvd()
 ****************************************************************
 * @brief
 *  Sets the active target for specified provision drive.
 *
 * @param  pvd_object_id - pvd object id.
 *
 * @return None
 *
 * @author
 *  07/09/2012 - Prahlad Purohit
 *
 ****************************************************************/
void
fbe_test_sep_util_set_active_target_for_pvd(fbe_object_id_t pvd_object_id)
{
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    fbe_status_t                          status;
    
    fbe_test_sep_util_get_active_passive_sp(pvd_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

/*!**************************************************************
 * fbe_test_sep_util_wait_for_initial_verify()
 ****************************************************************
 * @brief
 *  Wait until the initial verify finishes.
 *
 * @param rg_config_p - config pointer to wait for lun completions on
 *
 * @return None.
 *
 * @author
 *  7/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_wait_for_initial_verify(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_api_lun_get_lun_info_t lun_info;
	fbe_metadata_info_t		metadata_info;
    fbe_transport_connection_target_t target, peer;
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id,
                                                              &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        target = fbe_api_sim_transport_get_target_server();
        peer = (target == FBE_SIM_SP_B) ? FBE_SIM_SP_A : FBE_SIM_SP_B;
        status  = fbe_api_base_config_metadata_get_info(rg_object_id, &metadata_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "rg_num: %d rg_id: 0x%x SP %c is %s", 
                   current_rg_config_p->raid_group_id, 
                   rg_object_id,
                   (target == FBE_SIM_SP_A) ? 'A' : 'B',
                   (metadata_info.metadata_element_state != FBE_METADATA_ELEMENT_STATE_ACTIVE) ?
                   "passive" : "active");
        /* Make sure we get the verify report from the active SP.
         */
        if ((metadata_info.metadata_element_state != FBE_METADATA_ELEMENT_STATE_ACTIVE) &&
            fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(peer);
        }
        for ( lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* First make sure we finished the zeroing.
             */
            fbe_test_sep_util_wait_for_lun_zeroing_complete(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                            FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);

            /* Then make sure we finished the verify.  
             */
            fbe_test_sep_util_wait_for_initial_lun_verify_completion(lun_object_id);

            /* Next make sure that the LUN was requested to do initial verify and that it performed it already.
             */
            status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);
        
            /* We should not check to make sure no errors have been found in the current or past verifies
             * since more verifies may very well have run since the initial verify was run.
             */
        }

        /* We now waited for the raid group to complete its own verify 
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* Wait for the verify on each mirrored pair.
             */
            for (index = 0; 
                  index < downstream_object_list.number_of_downstream_objects; 
                  index++)
            {
                status = fbe_test_sep_util_wait_for_raid_group_verify(downstream_object_list.downstream_object_list[index], 
                                                                      (FBE_TEST_WAIT_TIMEOUT_MS/1000));
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        else
        {
            fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, (FBE_TEST_WAIT_TIMEOUT_MS/1000));
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        fbe_api_sim_transport_set_target_server(target);
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_wait_for_initial_verify()
 **************************************/

/*!**************************************************************
 * fbe_test_utils_get_event_log_error_count()
 ****************************************************************
 * @brief
 *  Return the number of errors of a given type.
 *
 * @param msg_id - message id to get count for.
 * @param count_p - Ptr to the count structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  8/11/2011 - Created. Jason White
 *
 ****************************************************************/
fbe_status_t fbe_test_utils_get_event_log_error_count(fbe_u32_t msg_id,
                                                      fbe_u32_t *count_p)
{
    fbe_event_log_msg_count_t message_count;
    fbe_status_t status;

    message_count.msg_id = msg_id;
    message_count.msg_count = 0;
    status = fbe_api_get_event_log_msg_count(&message_count, FBE_PACKAGE_ID_SEP_0);

    *count_p = message_count.msg_count;
    return status;
}
/******************************************
 * end fbe_test_utils_get_event_log_error_count()
 ******************************************/

/*!**************************************************************
 * fbe_test_rg_is_broken()
 ****************************************************************
 * @brief
 *  Check if the input raid group is broken
 *
 * @param rg_config_p - RG to check.               
 *
 * @return FBE_TRUE if degraded, FBE_FALSE otherwise.   
 *
 * @author
 *  09/11/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_test_rg_is_broken(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         rg_object_id;
    fbe_bool_t              b_is_broken = FBE_FALSE;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait up to `wait time' for life cycle `Not Ready' or `Hibernation'
     */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_FAIL, 
                                                     FBE_TEST_WAIT_OBJECT_TIMEOUT_MS,
                                                     FBE_PACKAGE_ID_SEP_0);

    /* If success then it is broken.
     */
    if (status == FBE_STATUS_OK)
    {
        b_is_broken = FBE_TRUE;
    }

    return b_is_broken;
}
/******************************************
 * end fbe_test_rg_is_broken()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_wait_for_rg_objects_fail()
 ****************************************************************
 * @brief
 *  Wait for all raid group objects to get fail for a given
 *  configuration array.
 *
 * @param   rg_config_p - config array.
 * @param   raid_group_count - Count of raid groups under test
 * @param   b_is_shutdown - FBE_TRUE - Sufficient drives have been
 *              removed to force a shutdown
 *
 * @return  status
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_wait_for_rg_objects_fail(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t raid_group_count,
                                                   fbe_bool_t b_is_shutdown)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       index;
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    fbe_object_id_t                 rg_object_id;
    fbe_bool_t                      b_validate_rg_failed;

    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        b_validate_rg_failed = FBE_TRUE;
        switch(current_rg_config_p->raid_type)
        {
            case FBE_RAID_GROUP_TYPE_RAID1:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                if ((b_is_shutdown == FBE_FALSE)     &&
                    (current_rg_config_p->width > 2)    )
                {
                    b_validate_rg_failed = FBE_FALSE;
                }
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* 1 parity drive - always check */
                break;
   
            case FBE_RAID_GROUP_TYPE_RAID6:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                if (b_is_shutdown == FBE_FALSE)
                {
                    b_validate_rg_failed = FBE_FALSE;
                }
                break;

            default:
                /* Type not supported.*/
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: rg id: %d type: %d not supported", 
                   __FUNCTION__, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
        }

        /* Wait up to `wait time' for life cycle `Not Ready' or `Hibernation'
         */
        if (b_validate_rg_failed == FBE_TRUE)
        {
    
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                             FBE_LIFECYCLE_STATE_FAIL, 
                                                             FBE_TEST_SEP_WAIT_MSEC,
                                                             FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        current_rg_config_p++;
    }

    return FBE_STATUS_OK;

}
/*********************************************
 * end fbe_test_sep_wait_for_rg_objects_fail()
 *********************************************/
/*!**************************************************************
 * fbe_test_sep_duplicate_config()
 ****************************************************************
 * @brief
 *  Make a copy of a config to recreate 
 *
 * @param from_rg_config_p - raid group config to recreate    
 *        to_rg_config_p - duplicate raid group config    
 *
 * @return
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_test_sep_duplicate_config(fbe_test_rg_configuration_t * from_rg_config_p,
                                    fbe_test_rg_configuration_t * to_rg_config_p) {
    fbe_u32_t i = 0;
    fbe_u32_t raid_group_count = 0;
    raid_group_count = fbe_test_get_rg_array_length(&from_rg_config_p[0]);

    for (i=0;i < raid_group_count; i++) 
    {
        to_rg_config_p[i] = from_rg_config_p[i];
    }
    if (i < FBE_TEST_MAX_RAID_GROUP_COUNT)
    {
        to_rg_config_p[i].width = FBE_U32_MAX;
    }
}
/******************************************
 * end fbe_test_sep_duplicate_config()
 ******************************************/

/*!**************************************************************
 * fbe_test_create_physical_config_for_rg()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups
 *
 * @return None.
 *
 * @author
 *  4/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_create_physical_config_for_rg(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t num_raid_groups)
{
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;


    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, num_raid_groups,
                                          &num_520_drives,
                                          &num_4160_drives);

    /* consider extra disk set into account if count is set by consumer test of this functionality
     */
    fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, num_raid_groups,
                                          &num_520_extra_drives,
                                          &num_4160_extra_drives);
    num_520_drives+=num_520_extra_drives;
    num_4160_drives+=num_4160_extra_drives;
    
    
    /* Fill in our raid group disk sets with this physical config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, num_raid_groups, num_520_drives, num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    return;
}
/**********************************************
 * end fbe_test_create_physical_config_for_rg()
 **********************************************/


/*!**************************************************************
 * fbe_test_create_physical_config_with_capacity()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 4160) we need from the rg configs.
 * @param num_raid_groups
 * @param extra_blocks
 * @return None.
 *
 * @author
 *  7/3/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_create_physical_config_with_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t num_raid_groups,
                                                   fbe_block_count_t extra_blocks)
{
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;


    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, num_raid_groups,
                                                       &num_520_drives,
                                                       &num_4160_drives);

    /* consider extra disk set into account if count is set by consumer test of this functionality
     */
    fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, num_raid_groups,
                                              &num_520_extra_drives,
                                              &num_4160_extra_drives);
    num_520_drives+=num_520_extra_drives;
    num_4160_drives+=num_4160_extra_drives;
    
    
    /* Fill in our raid group disk sets with this physical config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, num_raid_groups, num_520_drives, num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    fbe_test_pp_util_create_config_vary_capacity(num_520_drives,
                                                 num_4160_drives,
                                                 TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                 extra_blocks);
    return;
}
/**********************************************
 * end fbe_test_create_physical_config_with_capacity()
 **********************************************/

/*!***************************************************************************
 *      fbe_test_sep_validate_raid_groups_are_not_degraded()
 ***************************************************************************** 
 * 
 * @brief   Check if ALL the raid groups are degrade or not.  If any are not
 *
 * @param   rg_config_p - Array of raid groups to check for degraded
 * @param   raid_group_count - Count of raid groups to check
 *
 * @return  status - FBE_STATUS_OK - All raid groups are optimal
 *                   FBE_STATUS_GENERIC_FAILURE - One or more raid groups are
 *                      degraded.
 *
 * @author
 *  09/24/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_validate_raid_groups_are_not_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_is_any_rg_degraded = FBE_FALSE;
    fbe_u32_t       rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        b_is_any_rg_degraded = fbe_test_rg_is_degraded(current_rg_config_p);
        if (b_is_any_rg_degraded == FBE_TRUE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s rg_index: %d id: %d is degraded", 
                       __FUNCTION__, rg_index, current_rg_config_p->raid_group_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
        
        current_rg_config_p++;
    }

    return status;

}
/***********************************************************
 * end fbe_test_sep_validate_raid_groups_are_not_degraded()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_is_spare_drive_healthy()
 ***************************************************************************** 
 * 
 * @brief   Simply determine if the drive (specified by location) is:
 *              o Configured as `Unconsumed' or `test spare'
 *              o Is in the Ready state
 *              o Does not have EOL, Drive Fault etc, marked
 *
 * @param   disk_to_check_p - Pointer to disk location to check
 *
 * @return  bool - FBE_TRUE - The drive specified is a spare and is healthy.
 *
 * @author
 *  10/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_test_sep_is_spare_drive_healthy(fbe_test_raid_group_disk_set_t *disk_to_check_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_state_t   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;

    /* First validate that we can get the pvd object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(disk_to_check_p->bus,
                                                            disk_to_check_p->enclosure,
                                                            disk_to_check_p->slot,
                                                            &pvd_object_id);
    if (status != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Failed to get pvd object for disk: %d_%d_%d status: 0x%x",
                   __FUNCTION__, 
                   disk_to_check_p->bus, disk_to_check_p->enclosure, disk_to_check_p->slot,
                   status);
        return FBE_FALSE;
    }

    /* Now validate that the drive is ready.
     */
    status = fbe_api_get_object_lifecycle_state(pvd_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if ((status != FBE_STATUS_OK)                      ||
        (lifecycle_state != FBE_LIFECYCLE_STATE_READY)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s lifecycle state: %d for pvd obj: 0x%x disk: %d_%d_%d status: 0x%x not Ready",
                   __FUNCTION__, lifecycle_state, pvd_object_id,
                   disk_to_check_p->bus, disk_to_check_p->enclosure, disk_to_check_p->slot,
                   status);
        return FBE_FALSE;
    }

    /* Now validate that the drive is configured either as `unconsumed' or
     * `test spare.
     */
    provision_drive_info.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    if ((status != FBE_STATUS_OK)                                                                       ||
        ((provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)          &&
         (provision_drive_info.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)    )    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s config type: %d for pvd obj: 0x%x disk: %d_%d_%d status: 0x%x not Unconsumed/Test Spare",
                   __FUNCTION__, provision_drive_info.config_type, pvd_object_id,
                   disk_to_check_p->bus, disk_to_check_p->enclosure, disk_to_check_p->slot,
                   status);
        return FBE_FALSE;
    }

    /* Now validate the `health':
     *  o EOL not set
     *  o Drive Fault not set
     */
    if ((provision_drive_info.end_of_life_state != FBE_FALSE)            ||
        (provision_drive_info.drive_fault_state != FBE_FALSE)            ||
        (provision_drive_info.slf_state != FBE_PROVISION_DRIVE_SLF_NONE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s EOL: %d fault: %d slf: %d for pvd obj: 0x%x disk: %d_%d_%d status: 0x%x not healthy",
                   __FUNCTION__, provision_drive_info.end_of_life_state, provision_drive_info.drive_fault_state,
                   provision_drive_info.slf_state, pvd_object_id,
                   disk_to_check_p->bus, disk_to_check_p->enclosure, disk_to_check_p->slot,
                   status);
        return FBE_FALSE;
    }

    /* Otherwise the drive is a valid and healthy spare.
     */
    return FBE_TRUE;
}
/********************************************
 * end fbe_test_sep_is_spare_drive_healthy()
 ********************************************/

void fbe_test_sep_util_check_stripe_lock(fbe_object_id_t object_id)
{
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
	fbe_status_t status;
	fbe_base_config_control_get_stripe_blob_t * blob_A;
	fbe_base_config_control_get_stripe_blob_t * blob_B;
	fbe_u64_t i;

    target_invoked_on = fbe_api_transport_get_target_server();

	blob_A = fbe_api_allocate_memory(sizeof(fbe_base_config_control_get_stripe_blob_t));
	blob_B = fbe_api_allocate_memory(sizeof(fbe_base_config_control_get_stripe_blob_t));

    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
	status = fbe_api_base_config_get_stripe_blob(object_id, blob_A);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
	status = fbe_api_base_config_get_stripe_blob(object_id, blob_B);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


	for(i = 0; (i < blob_A->size) && (i < FBE_MEMORY_CHUNK_SIZE); i++){
		if((blob_A->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER){
			MUT_ASSERT_TRUE((blob_B->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL);
		} else if((blob_A->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL){
			MUT_ASSERT_TRUE((blob_B->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER);
		} else if((blob_A->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_SHARED){
			MUT_ASSERT_TRUE((blob_B->slot[i] & METADATA_STRIPE_LOCK_SLOT_STATE_MASK) == METADATA_STRIPE_LOCK_SLOT_STATE_SHARED);
		} else {
			MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Uknown slot value\n");
		}
	}

	mut_printf(MUT_LOG_TEST_STATUS, "SL: object_id: 0x%x checked %d slots", object_id, blob_A->size);

    fbe_api_transport_set_target_server(target_invoked_on);

	fbe_api_free_memory(blob_A);
	fbe_api_free_memory(blob_B);
}

/*!***************************************************************************
 *          fbe_test_sep_util_set_update_peer_checkpoint_interval()
 ***************************************************************************** 
 * 
 * @brief   Sets the period between updates to the peer when the active
 *          changes the checkpoint(s).  Value is in seconds.
 *
 * @param   update_peer_period - Period between updates (in seconds)
 *
 * @return  status
 *
 * @author
 *  03/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_set_update_peer_checkpoint_interval(fbe_u32_t update_peer_period)
{
    fbe_status_t                            status;
    fbe_bool_t                              b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t   other_sp = FBE_SIM_SP_B;

    /* Get this and other SP
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Update the local SP.
     */
    status = fbe_api_raid_group_set_update_peer_checkpoint_interval(update_peer_period);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we are in dualsp update the peer
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_set_update_peer_checkpoint_interval(update_peer_period);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*************************************************************
 * end fbe_test_sep_util_set_update_peer_checkpoint_interval()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_verify_start()
 ***************************************************************************** 
 * 
 * @brief   Waits for the verify type specified to start and returns the verify
 *          info.
 *
 * @param   lun_object_id - The LUN object id to chekc verify status for
 * @param   verify_type - The type of verify to wait for
 * @param   lun_verify_status_p - Pointer to verify status
 *
 * @return  status
 *
 * @author
 *  04/05/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_verify_start(fbe_object_id_t lun_object_id,                 
                                                     fbe_lun_verify_type_t verify_type,           
                                                     fbe_api_lun_get_status_t *lun_verify_status_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       total_ms_waited = 0;

    /* Wait for either the verify to start or timeout.
     */
    while (total_ms_waited < FBE_TEST_SEP_WAIT_MSEC)
    {
        /* Get the verify status.
         */
        status = fbe_api_lun_get_bgverify_status(lun_object_id, lun_verify_status_p, verify_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Simply check that the checkpoint is not at the end marker (i.e. -1)
         */
        if (lun_verify_status_p->checkpoint != FBE_LBA_INVALID)
        {
            /* The veify has started.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s verify type: %d chkpt: 0x%llx percent: %d has started",
                       __FUNCTION__, verify_type,
                       (unsigned long long)lun_verify_status_p->checkpoint,
                       lun_verify_status_p->percentage_completed);
            return FBE_STATUS_OK;
        }

        /* Wait the normal interval
         */
        fbe_api_sleep(FBE_TEST_UTILS_POLLING_INTERVAL);
        total_ms_waited += FBE_TEST_UTILS_POLLING_INTERVAL;
    }

    /* The verify did not start.
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s verify type: %d chkpt: 0x%llx percent: %d did not start in: %d secs",
               __FUNCTION__, verify_type,
               (unsigned long long)lun_verify_status_p->checkpoint,
               lun_verify_status_p->percentage_completed,
               (FBE_TEST_SEP_WAIT_MSEC / 1000));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*************************************************************
 * end fbe_test_sep_util_wait_for_verify_start()
 *************************************************************/

/*!**************************************************************
 * fbe_test_sep_util_disable_spare_library_confirmation()
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to disable
 *  the spare library (job service) to virtual drive object
 *  confirmation process.
 * 
 * @param   None
 *
 * @return fbe_status - status of disable request
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_disable_spare_library_confirmation(void)
{
    fbe_status_t                            status;
    fbe_bool_t                              b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;
    fbe_u32_t                               operation_timeout_secs = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS;


    mut_printf(MUT_LOG_TEST_STATUS, "== %s: disable job to object confirmation ==", 
               __FUNCTION__);

    /* Find this and other SP
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s: this: %d peer: %d disable job to object confirmation ==", 
               __FUNCTION__, this_sp, other_sp);

    /* If we are in dualsp set the timer on both SPs.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        /* First change the spare config on this SP (assumed to be alive)
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_TRUE,             /* Disable operation confirmation*/
                                                              FBE_FALSE,            /* Do not set default values */
                                                              operation_timeout_secs /* Ignored */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Timer is not automatically updated on SPB.  If the peer is alive
         * update the spare config on the peer.
         */
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_TRUE)
        {
            /* Update the peer then set the target to the original.
             */
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_job_service_set_spare_library_config(FBE_TRUE,             /* Disable operation confirmation*/
                                                                  FBE_FALSE,            /* Do not set default values */
                                                                  operation_timeout_secs /* Ignored */ );
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    else
    {
        /* Else single SP mode.
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_TRUE,             /* Disable operation confirmation*/
                                                              FBE_FALSE,            /* Do not set default values */
                                                              operation_timeout_secs /* Ignored */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*************************************************************
 * end fbe_test_sep_util_disable_spare_library_confirmation()
 *************************************************************/

/*!**************************************************************
 * fbe_test_sep_util_enable_spare_library_confirmation()
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to restore the
 *  spare operation confirmation (set values to default).
 * 
 * @param   None
 *
 * @return fbe_status - status of disable request
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_enable_spare_library_confirmation(void)
{
    fbe_status_t                            status;
    fbe_bool_t                              b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;
    fbe_u32_t                               operation_timeout_secs = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s: enable job to object confirmation ==", 
               __FUNCTION__);

    /* Find this and other SP
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* If we are in dualsp set the timer on both SPs.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        /* First change the spare config on this SP (assumed to be alive)
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,            /* Do not disable operation confirmation*/
                                                              FBE_TRUE,             /* Restore default values */
                                                              operation_timeout_secs /* Ignored */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Timer is not automatically updated on SPB.  If the peer is alive
         * update the spare config on the peer.
         */
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_TRUE)
        {
            /* Update the peer then set the target to the original.
             */
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,        /* Do not disable operation confirmation*/
                                                                  FBE_TRUE,         /* Restore default values */
                                                                  operation_timeout_secs /* Ignored */ );
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    else
    {
        /* Else single SP mode.
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,            /* Do not disable operation confirmation*/
                                                              FBE_TRUE,             /* Restore default values */
                                                              operation_timeout_secs /* Ignored */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/************************************************************
 * end fbe_test_sep_util_enable_spare_library_confirmation()
 ***********************************************************/

/*!**************************************************************
 * fbe_test_sep_util_set_spare_library_confirmation_timeout()
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to change the
 *  the spare library (job service) to virtual drive object
 *  confirmation timeout (the current default is 110 seconds).
 * 
 * @param   None
 *
 * @return fbe_status - status of change timeout request.
 *
 * @author
 *  06/21/2013  Ron Proulx  - Created.
 * 
 ****************************************************************/
fbe_status_t fbe_test_sep_util_set_spare_library_confirmation_timeout(fbe_u32_t new_spare_library_confirmation_timeout_secs)
{
    fbe_status_t                            status;
    fbe_bool_t                              b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;
    fbe_u32_t                               operation_timeout_secs = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS;

    /*! @note Currently there is not get so we assume the default.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: change job to object confirmation timeout from: %d to %d ==", 
               __FUNCTION__, operation_timeout_secs, new_spare_library_confirmation_timeout_secs);

    /* Find this and other SP
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* If we are in dualsp set the timer on both SPs.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        /* First change the spare config on this SP (assumed to be alive)
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,            /* Enable operation confirmation*/
                                                              FBE_FALSE,            /* Do not set default values */
                                                              new_spare_library_confirmation_timeout_secs /* Update timeout */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Timer is not automatically updated on SPB.  If the peer is alive
         * update the spare config on the peer.
         */
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_TRUE)
        {
            /* Update the peer then set the target to the original.
             */
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,            /* Enable operation confirmation*/
                                                                  FBE_FALSE,            /* Do not set default values */
                                                                  new_spare_library_confirmation_timeout_secs /* Update timeout */ );
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    else
    {
        /* Else single SP mode.
         */
        status = fbe_api_job_service_set_spare_library_config(FBE_FALSE,            /* Enable operation confirmation*/
                                                              FBE_FALSE,            /* Do not set default values */
                                                              new_spare_library_confirmation_timeout_secs /* Update timeout */ );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/***************************************************************
 * end fbe_test_sep_util_set_spare_library_confirmation_timeout()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_wait_for_swap_request_to_complete()
 *****************************************************************************
 *
 * @brief   Wait for the swap request to complete.
 *
 * @param   vd_object_id - Virtual drive object id to wait for
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  04/18/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_wait_for_swap_request_to_complete(fbe_object_id_t vd_object_id)
{
    fbe_status_t                    status;
    fbe_u32_t                       total_ms_waited = 0;
    fbe_u32_t                       wait_interval_ms = 100;
     fbe_u32_t                      timeout_ms = FBE_TEST_UTILS_DRIVE_SWAP_TIMEOUT_MS;
    fbe_api_virtual_drive_get_info_t vd_info;


    /* Wait until the request is complete or timeout
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((vd_info.b_request_in_progress == FBE_TRUE) &&
           (total_ms_waited < timeout_ms)                 )
    {
        /* Wait until complete or timeout.
         */
        status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait 100ms
         */
        total_ms_waited += wait_interval_ms;
        fbe_api_sleep(wait_interval_ms);
    }

    /* Check if complete
     */
    if (vd_info.b_request_in_progress == FBE_TRUE)
    {
        /* Timedout
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s vd obj: 0x%x mode: %d in-progress: %d did not complete in: %d seconds", 
                   __FUNCTION__, vd_object_id, vd_info.configuration_mode, vd_info.b_request_in_progress,
                   (total_ms_waited / 1000));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Success
     */
    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_test_sep_util_wait_for_swap_request_to_complete()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_glitch_drive()
 *****************************************************************************
 *
 * @brief   For the position specified, `glitch' the drive.  `Glitch' means
 *          quickly transition the drive out of Ready thru Activate and back
 *          to Ready.
 *
 * @param   rg_config_p - Pointer to raid group to glitch (1) drive
 * @param   position_to_glitch - Drive position to glitch
 *
 * @return  fbe_status_t
 * 
 * @note    This method only `glitches' the drive on the raid group Active SP.
 *
 * @author
 *  05/09/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_glitch_drive(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t position_to_glitch)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         pdo_object_id;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the raid group object id.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(rg_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        /* Else the active is the `other' SP.  Validate that the other SP
         * is alive.
         */
        active_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for rg obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, rg_object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Glitch the PDO object on the raid group Active SP
     */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[position_to_glitch].bus,      
                                                              rg_config_p->rg_disk_set[position_to_glitch].enclosure,
                                                              rg_config_p->rg_disk_set[position_to_glitch].slot,     
                                                              &pdo_object_id);                                       
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! Now force the PDO thru Activate back to Ready.
     * 
     *  @note Do NOT wait for the object to become Ready.  There may be other
     *        raid groups under test.
     */
    status = fbe_api_set_object_to_state(pdo_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Display some information.                 
     */                                          
    mut_printf(MUT_LOG_TEST_STATUS, "%s pdo obj: 0x%x glitched drive", 
               __FUNCTION__, pdo_object_id);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_test_sep_util_glitch_drive()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_utils_get_bitcount()
 *****************************************************************************
 *
 * @brief   Just get the number of bits set in the mask.
 *
 * @param   bitmask - 32-bit mask
 * 
 * @return  count - Count of bits set
 *****************************************************************************/
fbe_u32_t fbe_test_sep_utils_get_bitcount(fbe_u32_t bitmask)
{
    fbe_u32_t bitcount = 0;

    /* Count total bits set in a bitmask.
     */
    while (bitmask > 0)
    {
        /* Clear off the highest bit.
         */
        bitmask &= bitmask - 1;
        bitcount++;
    }

    return bitcount;
}
/****************************************************
 * end fbe_test_sep_utils_get_bitcount()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_destroy_kms()
 *****************************************************************************
 *
 * @brief   This function destroys KMS package.
 *
 * @param   NULL
 *
 * @return  NULL
 *
 * @author
 *  12/06/2013  Lili Chen  - Created.
 *
 *****************************************************************************/
void fbe_test_sep_util_destroy_kms(void)
{
    //fbe_test_package_list_t package_list;
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_KMS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unload kms failed");

#if 0
    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_KMS;

    fbe_test_common_util_package_destroy(&package_list);

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);
#endif

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}
/****************************************************
 * end fbe_test_sep_util_destroy_kms()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_destroy_kms_both_sps()
 *****************************************************************************
 *
 * @brief   This function destroys KMS package on both SPs.
 *
 * @param   NULL
 *
 * @return  NULL
 *
 * @author
 *  12/06/2013  Lili Chen  - Created.
 *
 *****************************************************************************/
void fbe_test_sep_util_destroy_kms_both_sps(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_kms();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_kms();
    return;
}
/****************************************************
 * end fbe_test_sep_util_destroy_kms_both_sps()
 ****************************************************/
/*!**************************************************************
 * fbe_test_is_test_case_enabled()
 ****************************************************************
 * @brief
 *  Decide if we can execute this test case.
 *
 * @param test_case_index
 * @param test_cases_options_p
 * 
 * @return fbe_bool_t
 *
 * @author
 *  12/12/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_test_is_test_case_enabled(fbe_u32_t test_case_index,
                                         fbe_char_t *test_cases_options_p)
{
    const fbe_char_t * prompt_delimit = " ,\t";
    fbe_u32_t current_test_case_index;
    fbe_char_t *current_arg_p = NULL;
    fbe_u32_t arg_count = 0;

    if (test_cases_options_p == NULL) {
        /* No options specified.
         */
        return FBE_TRUE;
    }

    current_arg_p = strtok(test_cases_options_p, prompt_delimit);

    while (current_arg_p != NULL) {
        arg_count++;
        
        current_test_case_index = strtoul(current_arg_p, 0, 0);
        if (current_test_case_index == test_case_index)
        {
            return FBE_TRUE;
        }
        current_arg_p = strtok(NULL, prompt_delimit);
    }

    if (arg_count == 0){
        /* No options specified.
         */
        return FBE_TRUE;
    }
    /* Options were specified and did not enable this test case.
     */
    return FBE_FALSE;
}
/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_kms
 ****************************************************************
 * @brief
 *  This function 
 * 
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 *  12/11/2013  Lili Chen  - Created.
 * 
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_kms(void)
{
    fbe_test_packages_t unload_packages;
    fbe_zero_memory(&unload_packages, sizeof(fbe_test_packages_t));

    unload_packages.neit = FBE_TRUE;
    unload_packages.sep = FBE_TRUE;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);
    fbe_test_sep_util_destroy_kms();
    fbe_test_sep_util_destroy_packages(&unload_packages); 
    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}
/****************************************************
 * end fbe_test_sep_util_destroy_neit_sep_kms()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_destroy_neit_sep_kms_both_sps
 ****************************************************************
 * @brief
 *  This function
 *
 * @param object_id  - the ID of the object.
 *
 * @return none
 *
 * @author
 *  12/11/2013  Lili Chen  - Created.
 *
 ****************************************************************/
void fbe_test_sep_util_destroy_neit_sep_kms_both_sps(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    /* Start destroying SP A.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_destroy_neit_sep_kms();

    /* Start destroying SP B.
     */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_destroy_neit_sep_kms();

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);
    return;
}
/****************************************************
 * end fbe_test_sep_util_destroy_neit_sep_kms_both_sps()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_enable_kms_encryption
 ****************************************************************
 * @brief
 *  This function enables encryption in kms package.
 *
 * @param None
 *
 * @return status
 *
 * @author
 *  12/20/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_enable_kms_encryption(void)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;
    fbe_status_t get_status;

	/* Need to find Active SP */
	/* Check current target first */

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_ENABLE_ENCRYPTION,
                                                 NULL,
                                                 0,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_test_sep_util_get_enable_kms_status(&get_status);
    while (status == FBE_STATUS_OK && get_status == FBE_STATUS_PENDING) {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: waiting enable kms to finish\n", __FUNCTION__);
        fbe_api_sleep(1000);
        status = fbe_test_sep_util_get_enable_kms_status(&get_status);
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_enable_kms_encryption()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_enable_kms_status
 ****************************************************************
 * @brief
 *  This function gets enable kms status.
 *
 * @param get_status
 *
 * @return status
 *
 * @author
 *  03/10/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_get_enable_kms_status(fbe_status_t *get_status)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_GET_ENABLE_STATUS,
                                                 get_status,
                                                 sizeof(fbe_status_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_get_enable_kms_status()
 ****************************************************/

/*!**************************************************************
 * fbe_test_util_get_kms_hook
 ****************************************************************
 * @brief
 *  This function adds a hook in kms package,
 *  which will pause in between the load and the push.
 *
 * @param hook_p - Hook to fetch.  The state in the hook
 *                 determines the specific hook to fetch.
 *
 * @return status
 *
 * @author
 *  2/27/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_util_get_kms_hook(fbe_kms_hook_t *hook_p)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_kms_control_get_hook_t get_hook;

	/* Need to find Active SP */
	/* Check current target first */
    get_hook.hook.state = hook_p->state;
    status = fbe_api_common_send_control_packet(FBE_KMS_CONTROL_CODE_GET_HOOK,
                                                 &get_hook,
                                                 sizeof(fbe_kms_control_get_hook_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *hook_p = get_hook.hook;
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_test_util_get_kms_hook()
 ****************************************************/
/*!**************************************************************
 * fbe_test_util_wait_for_kms_hook()
 ****************************************************************
 * @brief
 *  Wait until kms hits our hook.
 *
 * @param state - The hook state for the hook we previously set.  
 * @param timeout_ms - Time to wait max for hook.             
 *
 * @return fbe_status_t  
 *
 * @author
 *  2/27/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_util_wait_for_kms_hook(fbe_kms_hook_state_t state, fbe_u32_t timeout_ms)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_kms_hook_t kms_hook;
    fbe_u32_t poll_time = 200;

    kms_hook.state = state;
    while (current_time < timeout_ms){
        status = fbe_test_util_get_kms_hook(&kms_hook);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }
            
        if (kms_hook.hit_count > 0){
            return FBE_STATUS_OK;
        }
        current_time = current_time + poll_time;
        fbe_api_sleep(poll_time);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: hook not hit for state %u in %u ms!\n", 
                  __FUNCTION__, (unsigned int)kms_hook.state, (unsigned int)timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_test_util_wait_for_kms_hook()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_clear_kms_hook
 ****************************************************************
 * @brief
 *  This function clears a hook in kms package.
 *
 * @param state - the hook state to clear.
 *
 * @return status
 *
 * @author
 *  2/27/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_clear_kms_hook(fbe_kms_hook_state_t state)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;
    fbe_kms_control_clear_hook_t clear_hook;

    clear_hook.hook.state = state;
    status = fbe_api_common_send_control_packet(FBE_KMS_CONTROL_CODE_CLEAR_HOOK,
                                                 &clear_hook,
                                                 sizeof(fbe_kms_control_clear_hook_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/****************************************************
 * end fbe_test_sep_util_clear_kms_hook()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_fill_system_raid_group_lun_config()
 *****************************************************************************
 *
 * @brief   Given an input raid group an an input set of disks,
 *          determine which disks to use for this raid group configuration.
 *          This allows us to use a physical configuration, which is not
 *          configurable.
 *
 * @param   rg_config_p - current raid group config to evaluate.
 * @param   num_luns_to_test - The number of LUNs in each raid group to test.
 * @param   physical_chunks_per_lun_to_test - The number of physical chunks to test per lun
 *              FBE_U32_MAX - Use the entire capacity
 *
 * @return -   
 *
 * @author
 *  01/15/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_fill_system_raid_group_lun_config(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_luns_to_test,
                                                        fbe_u32_t physical_chunks_per_lun_to_test)
{
#define FBE_TEST_FILL_LUNS_MAX_LUNS         3
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               i = 0;
    fbe_u32_t                               index = 0;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               rg_index;
    fbe_u32_t                               raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t                         rg_object_id;
    fbe_api_raid_group_get_info_t           rg_info;
    fbe_lba_t                               lun_capacity_to_test = 0;
    fbe_u32_t                               lu_index = 0;
    fbe_lun_number_t                        lun_number[FBE_TEST_FILL_LUNS_MAX_LUNS];
    fbe_object_id_t                         lun_object_id[FBE_TEST_FILL_LUNS_MAX_LUNS];
    //fbe_api_lun_get_lun_info_t              lun_info;
    fbe_database_lun_info_t lun_info;
    fbe_private_space_layout_lun_info_t     psl_lun_info;
    fbe_test_logical_unit_configuration_t  *lun_config_p = NULL;
    fbe_api_lun_calculate_capacity_info_t   calculate_capacity;

    /* This method only supports so many luns
     */
    if (num_luns_to_test > FBE_TEST_FILL_LUNS_MAX_LUNS) {
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                 "requested number of luns exceeds supported");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i = 0; i < FBE_TEST_FILL_LUNS_MAX_LUNS; i++) {
        lun_number[i] = FBE_LUN_ID_INVALID;
        lun_object_id[i] = FBE_OBJECT_ID_INVALID;
    }

    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 512 and 520. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* Validate the raid group is a private raid group.
         */
        if ((current_rg_config_p->raid_group_id < FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR) ||
            (current_rg_config_p->raid_group_id > FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT)     ) {
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group is not private");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get the raid group information
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Based on the raid group id determine the number and lun info
         */
        switch(current_rg_config_p->raid_group_id) {
            case FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR:
                if (num_luns_to_test > 3) {
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group does not contain requested number of luns");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                current_rg_config_p->number_of_luns_per_rg = FBE_MIN(3, num_luns_to_test);
                break;
            case FBE_PRIVATE_SPACE_LAYOUT_RG_ID_4_DRIVE_R5:
                if (num_luns_to_test > 3) {
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group does not contain requested number of luns");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                current_rg_config_p->number_of_luns_per_rg = FBE_MIN(3, num_luns_to_test);
                break;
            case FBE_PRIVATE_SPACE_LAYOUT_RG_ID_RAW_MIRROR:
                if (num_luns_to_test > 2) {
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group does not contain requested number of luns");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                current_rg_config_p->number_of_luns_per_rg = FBE_MIN(2, num_luns_to_test);
                break;
            case FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT:
                if (num_luns_to_test > 1) {
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group does not contain requested number of luns");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                current_rg_config_p->number_of_luns_per_rg = 1;
                break;
            default:
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group is not private");
                return FBE_STATUS_GENERIC_FAILURE;
        }

        index = 0;
        for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST; i <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST ; i++)
        {
            status = fbe_private_space_layout_get_lun_by_object_id(i, &psl_lun_info);
            if(FBE_STATUS_OK != status)
            {
                continue;
            }
            if((current_rg_config_p->raid_group_id == psl_lun_info.raid_group_id) && (index < current_rg_config_p->number_of_luns_per_rg))
            {
                lun_number[index] = psl_lun_info.lun_number;
                lun_object_id[index] = psl_lun_info.object_id;
                index++;
            }
            if (index >= current_rg_config_p->number_of_luns_per_rg)
            {
                break;
            }
        }

        /* Fill out the lun information for this RAID group. 
         */
        for (lu_index = 0; lu_index < current_rg_config_p->number_of_luns_per_rg; lu_index++) 
        {
            /* Wait for system LUNs to be ready.
             */
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[lu_index], 
                                                             FBE_LIFECYCLE_STATE_READY, 
                                                             30000, 
                                                             FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            //status = fbe_api_lun_get_lun_info(lun_object_id[lu_index], &lun_info);
            lun_info.lun_object_id = lun_object_id[lu_index];
            status = fbe_api_database_get_lun_info(&lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            lun_config_p = &current_rg_config_p->logical_unit_configuration_list[lu_index];
            mut_printf(MUT_LOG_TEST_STATUS, "== XX Lun capacity from info 0x%llx %d 0x%llx==", lun_info.capacity, physical_chunks_per_lun_to_test, rg_info.lun_align_size);

            /* The lun info capacity is the exported capacity (i.e. it doesn't
             * include the lun metadata).
             */
            if (physical_chunks_per_lun_to_test == FBE_U32_MAX) {
                MUT_ASSERT_TRUE(lun_info.capacity >= ((rg_info.lun_align_size + FBE_RAID_DEFAULT_CHUNK_SIZE) * 2));
                lun_capacity_to_test = lun_info.capacity;
            } else if (lun_info.capacity < (rg_info.lun_align_size * physical_chunks_per_lun_to_test)) {
                lun_capacity_to_test = lun_info.capacity;
            } else {
                MUT_ASSERT_TRUE(lun_info.capacity >= (rg_info.lun_align_size * physical_chunks_per_lun_to_test));
                lun_capacity_to_test = rg_info.lun_align_size * physical_chunks_per_lun_to_test;
            }
            lun_config_p->capacity = lun_capacity_to_test;
            mut_printf(MUT_LOG_TEST_STATUS, "== XX Lun capacity to test 0x%llx ==", lun_capacity_to_test);

            /* The imported capacity is the actual capacity (including metadata)
             * imported to the lun.  The capacity set in the lun config is the
             * capacity that was requested to test.
             */
            calculate_capacity.imported_capacity = FBE_LBA_INVALID;
            calculate_capacity.exported_capacity = lun_info.capacity;
            calculate_capacity.lun_align_size = rg_info.lun_align_size;
            status = fbe_api_lun_calculate_imported_capacity(&calculate_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            lun_config_p->imported_capacity = calculate_capacity.imported_capacity;
            lun_config_p->lun_number = lun_number[lu_index];
            lun_config_p->raid_type = current_rg_config_p->raid_type;
            lun_config_p->raid_group_id = current_rg_config_p->raid_group_id;
        }

        /* Goto next raid group */
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_test_fill_system_raid_group_lun_config()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_populate_system_rg_config()
 *****************************************************************************
 *
 * @brief   Given an input raid group an input set of disks,
 *          determine which disks to use for this raid group configuration.
 *          This allows us to use a physical configuration, which is not
 *          configurable.
 *
 * @param   rg_config_p - Array of raid groups configs for.  The system id
 *                        determines which private raid group and LUN to use.
 * @param   num_luns_to_test - The number of LUNs in each raid group to test.
 * @param   chunks_per_lun_to_test - The number of physical chunks to test per lun
 *
 * @return  status
 *
 * @author
 *  01/15/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_populate_system_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t num_luns_to_test,
                                                fbe_u32_t chunks_per_lun_to_test)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           drive_index;
    fbe_object_id_t                     rg_object_id;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_u32_t                           raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Fill up the logical unit configuration for each RAID group of this test index.
     * We round each capacity up to a default number of chunks. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* Validate the raid group is a private raid group.
         */
        if ((current_rg_config_p->raid_group_id < FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR) ||
            (current_rg_config_p->raid_group_id > FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT)     ) {
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                     "raid group is not private");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Populate the system raid group information.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        // wait until the system raid group is ready
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p->width         = rg_info.width;
        current_rg_config_p->capacity      = rg_info.capacity;
        current_rg_config_p->raid_type     = rg_info.raid_type;
        switch(rg_info.raid_type) {
            case FBE_RAID_GROUP_TYPE_RAID1:
                current_rg_config_p->class_id = FBE_CLASS_ID_MIRROR;
                break;
            case FBE_RAID_GROUP_TYPE_RAID10:
                current_rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                break;
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
            case FBE_RAID_GROUP_TYPE_RAID6:
                current_rg_config_p->class_id = FBE_CLASS_ID_PARITY;
                break;   
            case FBE_RAID_GROUP_TYPE_RAID0:
                current_rg_config_p->class_id = FBE_CLASS_ID_STRIPER;
                break;
            default:
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE,
                                         "raid type not supported");
                return FBE_STATUS_GENERIC_FAILURE;
        }
        current_rg_config_p->block_size    = rg_info.imported_block_size;
        current_rg_config_p->b_bandwidth   = (rg_info.element_size == 128) ? 0 : 1;
        current_rg_config_p->b_create_this_pass = FBE_TRUE;
        current_rg_config_p->number_of_luns_per_rg = num_luns_to_test;
        current_rg_config_p->magic = FBE_TEST_RG_CONFIGURATION_MAGIC;
        current_rg_config_p->b_valid_for_physical_config = FBE_TRUE;
        for (drive_index = 0; drive_index < rg_info.width; drive_index++) {
            current_rg_config_p->rg_disk_set[0].bus = 0;
            current_rg_config_p->rg_disk_set[0].enclosure = 0;
            current_rg_config_p->rg_disk_set[0].slot = drive_index;
        }

        /* Now populate the raid group config lun information from the
         * private lun information.
         */
        status = fbe_test_fill_system_raid_group_lun_config(current_rg_config_p,
                                                            num_luns_to_test,
                                                            chunks_per_lun_to_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next*/
        current_rg_config_p++;

    } /* for all raid groups */

    return status;
}
/***************************************************
 * end fbe_test_populate_system_rg_config()
 ***************************************************/
/*!**************************************************************
 * fbe_test_rg_wait_for_zeroing()
 ****************************************************************
 * @brief
 *  Wait until the raid group drives finish zeroing.
 *
 * @param rg_config_p - Current RAID Group Configuration.               
 *
 * @return -   
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_rg_wait_for_zeroing(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_rg_wait_for_zeroing()
 ******************************************/
static fbe_bool_t fbe_test_sep_util_encryption_enabled = FBE_FALSE;
fbe_status_t fbe_test_sep_start_encryption_or_rekey(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;

    if (!fbe_test_sep_util_encryption_enabled) {

        mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption and enable KMS started ==");
        status = fbe_test_sep_encryption_add_hook_and_enable_kms(rg_config_p, fbe_test_sep_util_get_dualsp_test_mode());
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks to wait for encryption and enable KMS status: 0x%x. ==", status);
        fbe_test_sep_util_encryption_enabled = FBE_TRUE;
    } else {
        fbe_test_sep_util_kms_start_rekey();
    }
    return FBE_STATUS_OK;
}
void fbe_test_sep_reset_encryption_enabled(void)
{
    fbe_test_sep_util_encryption_enabled = FBE_FALSE;
}
/*!**************************************************************
 * fbe_test_sep_util_kms_start_rekey
 ****************************************************************
 * @brief
 *  This function starts rekey in kms package.
 *
 * @param None
 *
 * @return status
 *
 * @author
 *  01/20/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_start_rekey(void)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_START_REKEY,
                                                 NULL,
                                                 0,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_start_rekey()
 ****************************************************/
/*!**************************************************************
 * fbe_test_sep_util_kms_get_deks
 ****************************************************************
 * @brief
 *  This function gets DEKs from kms package.
 *
 * @param get_keys
 *
 * @return status
 *
 * @author
 *  01/20/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_get_rg_keys(fbe_kms_control_get_keys_t *get_keys)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_GET_RG_DEKS,
                                                 get_keys,
                                                 sizeof(fbe_kms_control_get_keys_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_get_deks()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_kms_start_backup
 ****************************************************************
 * @brief
 *  This function sends a command to KMS to start backup.
 *
 * @param fname
 *
 * @return status
 *
 * @author
 *  02/25/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_start_backup(fbe_u8_t *fname)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_START_BACKUP,
                                                 fname,
                                                 KMS_FILE_NAME_SIZE,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_start_backup()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_kms_complete_backup
 ****************************************************************
 * @brief
 *  This function sends a command to KMS to start backup.
 *
 * @param fname
 *
 * @return status
 *
 * @author
 *  02/25/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_complete_backup(fbe_u8_t *fname)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_COMPLETE_BACKUP,
                                                 fname,
                                                 KMS_FILE_NAME_SIZE,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_complete_backup()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_kms_restore
 ****************************************************************
 * @brief
 *  This function sends a command to KMS to start backup.
 *
 * @param fname
 *
 * @return status
 *
 * @author
 *  02/25/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_restore(fbe_u8_t *fname)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet (FBE_KMS_CONTROL_CODE_RESTORE,
                                                 fname,
                                                 KMS_FILE_NAME_SIZE,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_restore()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_kms_set_rg_keys()
 ****************************************************************
 * @brief
 *  This function sets new DEKs into the KMS package.
 *
 * @param set_keys
 *
 * @return status
 *
 * @author
 *  2/27/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_set_rg_keys(fbe_kms_control_set_keys_t *set_keys)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet(FBE_KMS_CONTROL_CODE_SET_RG_DEKS,
                                                set_keys,
                                                sizeof(fbe_kms_control_set_keys_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_set_rg_keys()
 ****************************************************/

/*!**************************************************************
 * fbe_test_sep_util_kms_push_rg_keys()
 ****************************************************************
 * @brief
 *  This function tells KMS to push keys for a given RG.
 *
 * @param set_keys
 *
 * @return status
 *
 * @author
 *  3/5/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_kms_push_rg_keys(fbe_kms_control_set_keys_t *set_keys)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t                     status_info;

    status = fbe_api_common_send_control_packet(FBE_KMS_CONTROL_CODE_PUSH_RG_DEKS,
                                                set_keys,
                                                sizeof(fbe_kms_control_set_keys_t),
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_KMS);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
 * end fbe_test_sep_util_kms_push_rg_keys()
 ****************************************************/


/*!**************************************************************
 * fbe_test_sep_util_quiesce_raid_groups()
 ****************************************************************
 * @brief
 *  Quiesce all rgs in this configuration.
 *
 * @param rg_config_p - Config to quiesce.
 * @param raid_group_count - Total number of raid groups.
 *
 * @return None.
 *
 * @author
 *  8/19/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_quiesce_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_u32_t raid_group_count)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_status_t status;

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        /* If we are a raid 10, we quiesce the mirrors since this is the redundant level, which is 
         * the most interesting to test. 
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_u32_t mirror_index;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            /* For a raid 10 the downstream object list has the IDs of the mirrors. 
             * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
             */
            for (mirror_index = 0; 
                mirror_index < downstream_object_list.number_of_downstream_objects; 
                mirror_index++)
            {
                status = fbe_api_raid_group_quiesce(downstream_object_list.downstream_object_list[mirror_index], 
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        else
        {
            status = fbe_api_raid_group_quiesce(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_quiesce_raid_groups
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_unquiesce_raid_groups()
 ****************************************************************
 * @brief
 *  Unquiesce all rgs in this configuration.
 *
 * @param rg_config_p - Config to quiesce.
 * @param raid_group_count - Total number of raid groups.
 *
 * @return None.
 *
 * @author
 *  8/19/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_unquiesce_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t raid_group_count)
{   
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_status_t status;

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        /* If we are a raid 10, we unquiesce the mirrors since this is the redundant level, which is 
         * the most interesting to test. 
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_u32_t mirror_index;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            /* For a raid 10 the downstream object list has the IDs of the mirrors. 
             * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
             */
            for (mirror_index = 0; 
                mirror_index < downstream_object_list.number_of_downstream_objects; 
                mirror_index++)
            {
                status = fbe_api_raid_group_unquiesce(downstream_object_list.downstream_object_list[mirror_index], 
                                                      FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        else
        {
            status = fbe_api_raid_group_unquiesce(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_unquiesce_raid_groups
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_for_rg_base_config_flags()
 ****************************************************************
 * @brief
 *  Unquiesce all rgs in this configuration.
 *
 * @param rg_config_p - Config to quiesce.
 * @param raid_group_count - Total number of raid groups.
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 *
 * @return None.
 *
 * @author
 *  8/19/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_wait_for_rg_base_config_flags(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t raid_group_count,
                                                     fbe_u32_t wait_seconds,
                                                     fbe_base_config_clustered_flags_t base_config_clustered_flags,
                                                     fbe_bool_t b_cleared)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_status_t status;

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        /* If we are a raid 10, we test the mirrors since this is the redundant level, which is 
         * the most interesting to test. 
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_u32_t mirror_index;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            /* For a raid 10 the downstream object list has the IDs of the mirrors. 
             * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
             */
            for (mirror_index = 0; 
                  mirror_index < downstream_object_list.number_of_downstream_objects; 
                  mirror_index++)
            {
                status = fbe_test_sep_utils_wait_for_base_config_clustered_flags(
                    downstream_object_list.downstream_object_list[mirror_index],
                    wait_seconds, 
                    base_config_clustered_flags,
                    b_cleared);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        else
        {
            status = fbe_test_sep_utils_wait_for_base_config_clustered_flags(rg_object_id,
                                                                             wait_seconds, 
                                                                             base_config_clustered_flags,
                                                                             b_cleared);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end fbe_test_sep_util_wait_for_rg_base_config_flags
 **************************************/

/*!***************************************************************************
 *          fbe_test_sep_reset_raid_memory_statistics()
 *****************************************************************************
 *
 * @brief   For performance reasons there is no lock for getting or resetting
 *          the raid library memory statistics.  Although we cannot prevent
 *          the private raid groups from starting I/O we confirm that all the
 *          stats are zero.
 * 
 * @param   rg_config_p - Raid group config to test.
 *
 * @return  status
 *
 * @author
 *  02/14/2014  Ron Proulx
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_reset_raid_memory_statistics(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;  
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_api_raid_memory_stats_t     raid_memory_stats;
    fbe_u32_t                       time_out_secs = (FBE_TEST_WAIT_OBJECT_TIMEOUT_MS / 1000);
    fbe_u32_t                       time_waited_secs = 0;

    /* Quiesce the raid groups under test.
     */
    fbe_test_sep_util_quiesce_raid_groups(rg_config_p, raid_group_count);

    /* If there is a heavy load on the system it can take a while for the monitor to 
     * run to process the quiesce condition.  Thus we allow 60 seconds here for 
     * getting all the quiesce flags set. 
     */
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    (FBE_TEST_WAIT_OBJECT_TIMEOUT_MS / 1000), 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED,
                                                    FBE_FALSE    /* Don't wait for clear, wait for set */);

    /* Reset the memory stats and wait for them to be zero.
     */
    status = fbe_api_raid_group_reset_raid_memory_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_raid_memory_stats(&raid_memory_stats);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while (((raid_memory_stats.allocations != 0) ||
            (raid_memory_stats.frees       != 0)    ) &&
           (time_waited_secs < time_out_secs)            ) {
        /* Sleep for a short period
         */
        fbe_api_sleep(1000);
        time_waited_secs++;

        /* Reset and fetch
         */
        status = fbe_api_raid_group_reset_raid_memory_stats();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_get_raid_memory_stats(&raid_memory_stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log the wait time
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s waited %d secs current allocations: 0x%llx frees: 0x%llx ==", 
               __FUNCTION__, time_waited_secs,
               raid_memory_stats.allocations, raid_memory_stats.frees);

    /* Unquiesce the raid groups
     */
    fbe_test_sep_util_unquiesce_raid_groups(rg_config_p, raid_group_count);
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    (FBE_TEST_WAIT_OBJECT_TIMEOUT_MS / 1000), 
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                    FBE_TRUE    /* Yes, wait for cleared */);

    /* Now check for success
     */
    if ((raid_memory_stats.allocations != 0) ||
        (raid_memory_stats.frees       != 0)    ) {
        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK,"failed to reset raid memory stats");
        return status;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_test_sep_reset_raid_memory_statistics()
 *************************************************/


/********************************************************************************
 * The below controls if we should use a simulation PSL, which has much smaller
 * private space LUNs.  
 * This allows us to have a test where background operations complete quickly
 * Zeroing, verify, rebuild all complete quick since the PSL LUNs are small. 
 *********************************************************************************/
static fbe_bool_t fbe_test_use_fbe_sim_psl = FBE_FALSE;
void fbe_test_set_use_fbe_sim_psl(fbe_bool_t b_use_sim_psl)
{
    fbe_test_use_fbe_sim_psl = b_use_sim_psl;
}
fbe_bool_t fbe_test_should_use_sim_psl(void)
{
    return fbe_test_use_fbe_sim_psl;
}


void fbe_test_wait_for_critical_error(void)
{
    fbe_status_t status;
    fbe_api_trace_error_counters_t error_counters;
    fbe_u32_t total_ms;
    /* Wait for the critical error to occur.
     * The I/Os will complete but continue retrying.
     */
    total_ms = 0;
    while (total_ms < FBE_TEST_WAIT_TIMEOUT_MS)
    {
        status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (error_counters.trace_critical_error_counter != 0)
        {
            return;
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== Waiting for critical error ==");
            fbe_api_sleep(500);
            total_ms += 500;
        }
    }
    MUT_FAIL_MSG("Timeout:  Did not find critical error message.");
}

/*!*******************************************************************
 *          fbe_test_sep_utils_wait_pvd_for_base_config_clustered_flags()
 *********************************************************************
 * @brief
 *  Wait for this raid group's flags to reach the input value.
 * 
 *  We will check the flags first.
 *  If the flags match the expected value, we return immediately.
 *  If the flags are not what we expect, we will wait for
 *  half a second, before checking again.
 * 
 *  When the total wait time matches the input wait seconds
 *  we will return with failure.
 *
 * @param pvd_object_id - Object id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  03/17/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_utils_wait_pvd_for_base_config_clustered_flags(fbe_object_id_t pvd_object_id,
                                                fbe_u32_t wait_seconds,
                                                fbe_base_config_clustered_flags_t base_config_clustered_flags,
                                                fbe_bool_t b_cleared)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_clustered_flags_t flags;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * 1000;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_provision_drive_get_clustered_flag(pvd_object_id, &flags);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (b_cleared)
        {
            /* Check the flag is cleared.
             */
            if ((flags & base_config_clustered_flags) == 0)
            {
                /* Flags are cleared, just break out and return.
                 */
                break;
            }
        }
        else
        {
            /* Just check that the flag is set.
             */
            if ((flags & base_config_clustered_flags) == base_config_clustered_flags)
            {
                /* Flags we expected are set, just break out and return.
                 */
                break;
            }
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: pvd obj: 0x%x config flags: 0x%llx cleared: %d wait secs: %d timed-out\n", 
                   __FUNCTION__, pvd_object_id,
		   (unsigned long long)base_config_clustered_flags,
		   b_cleared, wait_seconds);

        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/***************************************************************
 * end fbe_test_sep_utils_wait_for_base_config_clustered_flags()
 ***************************************************************/
void fbe_test_set_rg_vault_encrypt_wait_time(fbe_u32_t wait_time_ms)
{
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = wait_time_ms;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
void fbe_test_reset_rg_vault_encrypt_wait_time(void)
{
    /*! @todo better would be to get the options initially and save them.  
     * Then restore them here. 
     */
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = 60000;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}

static fbe_status_t
fbe_test_sep_util_verify_stripe_lock_blob_slot(fbe_u8_t slot_a, fbe_u8_t slot_b)
{
    if ((slot_a == 0) && (slot_b == 3) ||
        (slot_a == 3) && (slot_b == 0) ||
        (slot_a == 1) && (slot_b == 1))
    {
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * fbe_test_sep_util_verify_stripe_lock_state()
 ****************************************************************
 * @brief
 *  This function is used to verify stripe locks after IO is stopped.
 * 
 * @param object_id
 *
 * @return fbe_status - status of request
 *
 * @author
 *  04/17/2014 - Created. Lili Chen
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_verify_stripe_lock_state(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_api_metadata_get_sl_info_t *spa_info = NULL, *spb_info = NULL;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_u32_t size;
    fbe_u32_t i, j;
    fbe_u8_t slot_a, slot_b;

    this_sp = fbe_api_sim_transport_get_target_server();

    spa_info = (fbe_api_metadata_get_sl_info_t *)malloc(sizeof(fbe_api_metadata_get_sl_info_t));
    spb_info = (fbe_api_metadata_get_sl_info_t *)malloc(sizeof(fbe_api_metadata_get_sl_info_t));
    if (spa_info == NULL || spb_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    spa_info->object_id = object_id;
    status = fbe_api_metadata_get_sl_info(spa_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    spb_info->object_id = object_id;
    status = fbe_api_metadata_get_sl_info(spb_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Verify blobs */
    MUT_ASSERT_INT_EQUAL(spa_info->large_io_count, 0);
    MUT_ASSERT_INT_EQUAL(spb_info->large_io_count, 0);
    MUT_ASSERT_INT_EQUAL(spa_info->blob_size, spb_info->blob_size);
    size = spa_info->blob_size;
    status = fbe_test_sep_util_verify_stripe_lock_blob_slot(spa_info->slots[size-1] & 3, spb_info->slots[size-1] & 3);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_verify_stripe_lock_blob_slot(spa_info->slots[size-2] & 3, spb_info->slots[size-2] & 3);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (i = 0; i < (size - 2); i++)
    {
        slot_a = spa_info->slots[i];
        slot_b = spb_info->slots[i];
		for (j = 0; j < 4; j++)
        {
            status = fbe_test_sep_util_verify_stripe_lock_blob_slot(slot_a & 0x3, slot_b & 0x3);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            slot_a = (slot_a >> 2);
            slot_b = (slot_b >> 2);
        }
    }

    fbe_api_sim_transport_set_target_server(this_sp);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_test_sep_util_verify_stripe_lock_state()
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_verify_rg_config_stripe_lock_state()
 ****************************************************************
 * @brief
 *  This function is used to verify stripe locks after IO is stopped.
 * 
 * @param rg_config_p
 *
 * @return fbe_status - status of request
 *
 * @author
 *  04/17/2014 - Created. Lili Chen
 * 
 ****************************************************************/
fbe_status_t
fbe_test_sep_util_verify_rg_config_stripe_lock_state(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_object_id_t             rg_object_id;

    if (!fbe_test_sep_util_get_dualsp_test_mode())
    {
        return FBE_STATUS_OK;
    }

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
			status = fbe_test_sep_util_verify_stripe_lock_state(rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    return status;
}
/**************************************
 * end fbe_test_sep_util_verify_rg_config_stripe_lock_state()
 **************************************/
/*!**************************************************************
 * fbe_test_set_vault_wait_time()
 ****************************************************************
 * @brief
 *  This function sets up some raid group params such as the
 *  vault wait time so that our vault encrypts quickly.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_set_vault_wait_time(void)
{
    #define VAULT_ENCRYPT_WAIT_TIME_MS 1000
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = VAULT_ENCRYPT_WAIT_TIME_MS;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    mut_printf(MUT_LOG_TEST_STATUS, "== Set vault wait time to %d msec", VAULT_ENCRYPT_WAIT_TIME_MS);
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
/******************************************
 * end fbe_test_set_vault_wait_time()
 ******************************************/

/* TRUE means the SP is OK.  FALSE means we took it down.
 */
static fbe_bool_t fbe_test_sp_state[2] = {FBE_TRUE, FBE_TRUE};
void fbe_test_mark_sp_down(fbe_test_process_t which_sp)
{
    fbe_test_sp_state[which_sp] = FBE_FALSE;
}
void fbe_test_mark_sp_up(fbe_test_process_t which_sp)
{
    fbe_test_sp_state[which_sp] = FBE_TRUE;
}
fbe_bool_t fbe_test_is_sp_up(fbe_test_process_t which_sp)
{
    return fbe_test_sp_state[which_sp];
}
fbe_test_process_t fbe_test_conn_target_to_sp(fbe_transport_connection_target_t conn_target)
{
    if (conn_target == FBE_TRANSPORT_SP_A) {
        return FBE_TEST_SPA;
    }
    else {
        return FBE_TEST_SPB;
    }
}
static fbe_u32_t fbe_test_max_allowed_commits = 2;
static fbe_u32_t fbe_test_num_commits = 0;
void fbe_test_set_commit_level(fbe_u32_t commit_level)
{
    fbe_status_t status;
    database_system_db_header_t db_header;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    
    fbe_test_num_commits++;
    /* At most we need to do this for the first 2 SPs to boot.
     */
    if (fbe_test_num_commits > fbe_test_max_allowed_commits) {
        mut_printf(MUT_LOG_TEST_STATUS, "Not committing. Already committed %u times.", fbe_test_max_allowed_commits); 
        return;
    }
    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_api_database_get_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Get system db header failed\n");

    db_header.persisted_sep_version = commit_level;

    mut_printf(MUT_LOG_TEST_STATUS, "---modify and write system db header with new version %u\n", commit_level);
    status = fbe_api_database_set_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Update system db header failed \n");

    mut_printf(MUT_LOG_TEST_STATUS, "--persist system db header\n");
    status = fbe_api_database_persist_system_db_header(&db_header);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Persist system db header failed \n");
}


/*!***************************************************************************
 *          fbe_test_sep_util_set_chunks_per_rebuild()
 ***************************************************************************** 
 * 
 * @brief   Sets the number of chunks to rebuild per cycle
 *
 * @param   num_chunks_per_rebuild - Number of chunks
 *
 * @return  status
 *
 * @author
 *  03/13/2015  Deanna Heng  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_util_set_chunks_per_rebuild(fbe_u32_t num_chunks_per_rebuild)
{
    fbe_status_t                            status;
    fbe_bool_t                              b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t   other_sp = FBE_SIM_SP_B;

    /* Get this and other SP
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "Setting number of chunks to %d\n", num_chunks_per_rebuild);
    sep_rebuild_utils_chunks_per_rebuild_g = num_chunks_per_rebuild;

    /* Update the local SP.
     */
    status = fbe_api_raid_group_set_chunks_per_rebuild(num_chunks_per_rebuild);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we are in dualsp update the peer
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_set_chunks_per_rebuild(num_chunks_per_rebuild);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*************************************************************
 * end fbe_test_sep_util_set_chunks_per_rebuild()
 *************************************************************/
/*!***************************************************************************
 *          fbe_test_sep_util_set_random_chunks_per_rebuild()
 ***************************************************************************** 
 * 
 * @brief   Sets the number of chunks to rebuild per cycle between 1 and 10 inclusive
 *
 * @author
 *  03/13/2015  Deanna Heng  - Created.
 *
 *****************************************************************************/
void fbe_test_sep_util_set_random_chunks_per_rebuild(void)
{
    //fbe_u32_t num_chunks = (fbe_random() % FBE_RAID_IOTS_MAX_CHUNKS) + 1;
    fbe_u32_t num_chunks = (fbe_random() % 4) + 1;

    fbe_test_sep_util_set_chunks_per_rebuild(num_chunks);
    return; 
}
/*************************************************************
 * end fbe_test_sep_util_set_random_chunks_per_rebuild()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_util_get_chunks_per_rebuild()
 ***************************************************************************** 
 * 
 * @brief   Return the number of chunks per rebuild
 *
 * @author
 *  03/13/2015  Deanna Heng  - Created.
 *
 *****************************************************************************/
fbe_u32_t fbe_test_sep_util_get_chunks_per_rebuild(void)
{
    return sep_rebuild_utils_chunks_per_rebuild_g;
}
/*************************************************************
 * end fbe_test_sep_util_get_chunks_per_rebuild()
 *************************************************************/
/********************************************************************************
 * The below controls slice size. This allows us to use smaller slice sizes in sim.
 *********************************************************************************/
static fbe_block_count_t fbe_test_ext_pool_slice_blocks = FBE_LBA_INVALID;
void fbe_test_set_ext_pool_slice_blocks(fbe_block_count_t slice_blocks)
{
    fbe_test_ext_pool_slice_blocks = slice_blocks;
}
fbe_block_count_t fbe_test_get_ext_pool_slice_blocks(void)
{
    return fbe_test_ext_pool_slice_blocks;
}

void fbe_test_set_ext_pool_slice_blocks_sim(void)
{
    /* For now we use 1 meg slice for simulation since the default 1 GB is too large to reasonably test.
     */
    fbe_test_set_ext_pool_slice_blocks(0x800);
}
void fbe_test_configure_ext_pool_slice_blocks(void)
{
#if 0
    fbe_status_t status;
    /* Set the blocks per slice so that we can test with more than one slice in sim.
     */
    fbe_extent_pool_class_set_options_t options;
    if (fbe_test_get_ext_pool_slice_blocks() != FBE_LBA_INVALID) {
        options.blocks_per_slice = fbe_test_ext_pool_slice_blocks;
        status = fbe_api_extent_pool_class_set_options(&options, FBE_CLASS_ID_EXTENT_POOL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
#endif
}

/*******************************
 * end file fbe_test_sep_utils.c
 *******************************/
