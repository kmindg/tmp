#include "mut.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_spare.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_cmi_interface.h"


#define FBE_TEST_SEP_DRIVE_WAIT_MSEC    360000  /*! @note This MUST be greater than 60 seconds! */


fbe_status_t fbe_test_sep_drive_verify_pvd_state(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_lifecycle_state_t expected_state,
                                                 fbe_u32_t timeout_ms)
{
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_u32_t elapsed_time = 0;

    do
    {
        /* Get the provision drive object id by location of the drive.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(port_number,
                                                                enclosure_number, 
                                                                slot_number, 
                                                                &object_id);
    
        /* Object id must be valid with all other lifecycle state.
         */
        if (object_id == FBE_OBJECT_ID_INVALID) 
        {
            if (expected_state == FBE_LIFECYCLE_STATE_DESTROY)
            {
                /* It does not exist, this was expected.
                 */
                return FBE_STATUS_OK;
            }
            else
            {
                /* Wait for the object to come into existance.
                 */
                EmcutilSleep(500);
                elapsed_time += 500;
            }
        }
        else
        {
            /* If success break out of the loop
             */
            if (status != FBE_STATUS_OK)
            {
                return status;
            }
            break;
        }
    } while ((object_id == FBE_OBJECT_ID_INVALID) &&
             (elapsed_time < timeout_ms));

    /* Verify whether physical drive object is in expected State, it waits
     * for the few seconds if it PDO is not in expected state.
     */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     expected_state, 
                                                     timeout_ms,
                                                     FBE_PACKAGE_ID_SEP_0);

    return status;
}

void fbe_test_sep_drive_configure_extra_drives_as_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_test_rg_configuration_t     *current_rg_config_p = rg_config_p;
    fbe_test_raid_group_disk_set_t  *drive_to_spare_p = NULL;
    fbe_u32_t                       drive_number;
    fbe_u32_t                       num_removed_drives;

    /*  Disable the recovery queue to delay spare swap-in for any RG at this moment untill all are available
     */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* For every raid group, configure extra drives as hot-spares based on the number of drives removed. */
    for (index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the number of removed drives in the RG. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);

        /* This requires the test to privode enough extra drives for hotspare. */
        MUT_ASSERT_CONDITION(num_removed_drives, <=,  current_rg_config_p->num_of_extra_drives);
        /* Configure hot-spares from the RG extra disk set. */
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {    
            /* Get an extra drive to configure as hot spare. */
            drive_to_spare_p = &current_rg_config_p->extra_disk_set[drive_number];

            mut_printf(MUT_LOG_TEST_STATUS, "== %s sparing rg %d (%d_%d_%d). ==", 
                       __FUNCTION__, index,  
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Wait for the PVD to be ready.  We need to wait at a minimum for the object to be created,
             * but the PVD is not spareable until it is ready. 
             */
            status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                          drive_to_spare_p->enclosure, 
                                                          drive_to_spare_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          FBE_TEST_SEP_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Also wait for the drive to become ready on the peer, if dual-SP test.
             */
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_sim_transport_connection_target_t   local_sp;
                fbe_sim_transport_connection_target_t   peer_sp;
        
                /*  Get the ID of the local SP. */
                local_sp = fbe_api_sim_transport_get_target_server();
        
                /*  Get the ID of the peer SP. */
                if (FBE_SIM_SP_A == local_sp)
                {
                    peer_sp = FBE_SIM_SP_B;
                }
                else
                {
                    peer_sp = FBE_SIM_SP_A;
                }
                fbe_api_sim_transport_set_target_server(peer_sp);
                status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                              drive_to_spare_p->enclosure, 
                                                              drive_to_spare_p->slot,
                                                              FBE_LIFECYCLE_STATE_READY,
                                                              FBE_TEST_SEP_DRIVE_WAIT_MSEC);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                fbe_api_sim_transport_set_target_server(local_sp);
            }

            /* Configure extra disk as a hot spare so that it will become part of the raid group permanently.
             */
            status = fbe_test_sep_util_configure_drive_as_hot_spare(drive_to_spare_p->bus, 
                                                                    drive_to_spare_p->enclosure, 
                                                                    drive_to_spare_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        }
        current_rg_config_p++;
   }

    /*  enable the recovery queue so that hot spare swap in occur concurrently at best suitable place
     */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);


   return;
}

void fbe_test_sep_drive_get_permanent_spare_edge_index(fbe_object_id_t   vd_object_id,
                                                       fbe_edge_index_t  *edge_index_p)
{
    fbe_api_virtual_drive_get_configuration_t   vd_configuration;


    /* Initialize edge index as invalid. */
    *edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* Get the virtual drive configuration mode. */
    fbe_api_virtual_drive_get_configuration(vd_object_id, &vd_configuration);

    /* Derive the edge index from the configuration mode. */
    if(vd_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        *edge_index_p = 0;
    }
    else if(vd_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
         *edge_index_p = 1;
    }

    MUT_ASSERT_INT_NOT_EQUAL(*edge_index_p, FBE_EDGE_INDEX_INVALID);
}

void fbe_test_sep_drive_wait_permanent_spare_swap_in(fbe_test_rg_configuration_t * rg_config_p,
                                                     fbe_u32_t position)
{
    fbe_edge_index_t                edge_index;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_status_t                    status;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_u32_t                       bus, encl, slot;


    /* Get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    /* Get the edge index where the permanent spare swaps in. */
    fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);

    /* Wait for spare to swap-in. */
    status = fbe_api_wait_for_block_edge_path_state(vd_object_id,
                                                    edge_index,
                                                    FBE_PATH_STATE_ENABLED,
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_TEST_SEP_DRIVE_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    
    /* Get the spare edge information for reporting purposes. */
    status = fbe_api_get_block_edge_info(vd_object_id,
                                         edge_index,
                                         &edge_info,
                                         FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_location(edge_info.server_id, &bus, &encl, &slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:permanent spare %d-%d-%d gets swapped-in, vd_obj_id:0x%x, hs_obj_id:0x%x",
               __FUNCTION__, bus, encl, slot, vd_object_id, edge_info.server_id);

    /* Also wait for the drive to become ready on the peer, if dual-SP test.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;
        fbe_sim_transport_connection_target_t   peer_sp;

        /*  Get the ID of the local SP. */
        local_sp = fbe_api_sim_transport_get_target_server();

        /*  Get the ID of the peer SP. */
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }
        fbe_api_sim_transport_set_target_server(peer_sp);
        /* Wait for spare to swap-in. */
        status = fbe_api_wait_for_block_edge_path_state(vd_object_id,
                                                        edge_index,
                                                        FBE_PATH_STATE_ENABLED,
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_TEST_SEP_DRIVE_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

        /* Get the spare edge information for reporting purposes. */
        status = fbe_api_get_block_edge_info(vd_object_id,
                                             edge_index,
                                             &edge_info,
                                             FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        status = fbe_api_provision_drive_get_location(edge_info.server_id, &bus, &encl, &slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_TEST_STATUS, "%s:permanent spare %d-%d-%d gets swapped-in, vd_obj_id:0x%x, hs_obj_id:0x%x",
                   __FUNCTION__, bus, encl, slot, vd_object_id, edge_info.server_id);

        /* Set the target server back to the local SP. */
        fbe_api_sim_transport_set_target_server(local_sp);
    }
}

void fbe_test_sep_drive_wait_extra_drives_swap_in(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_u32_t                       num_removed_drives;
    fbe_test_raid_group_disk_set_t *unused_drive_to_insert_p;
    fbe_test_rg_configuration_t     *current_rg_config_p = NULL;
    fbe_u32_t                       position_to_insert;
    fbe_u32_t                       drive_number;
    #define MAX_RAID_GROUPS 20
    #define MAX_POSITIONS_TO_REINSERT 2
    fbe_u32_t saved_position_to_reinsert[MAX_RAID_GROUPS][MAX_POSITIONS_TO_REINSERT];
    fbe_u32_t saved_num_removed_drives[MAX_RAID_GROUPS];

    if (raid_group_count > MAX_RAID_GROUPS)
    {
        MUT_FAIL_MSG("unexpected number of raid groups");
    }
    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the number of removed drives. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);
        if (num_removed_drives > MAX_POSITIONS_TO_REINSERT)
        {
            MUT_FAIL_MSG("unexpected number of positions to reinsert");
        }
        saved_num_removed_drives[index] = num_removed_drives;
        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {    
            /* Get the next position to insert a new drive. */
            position_to_insert = fbe_test_sep_util_get_next_position_to_insert(current_rg_config_p);
            saved_position_to_reinsert[index][drive_number] = position_to_insert;

            /* Update the drives removed array. */
            fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert); 

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];
        
            if (drive_number < current_rg_config_p->num_of_extra_drives)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== %s wait permanent swap-in for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            
                /* Wait for an extra drive to be permanent swap-in for the failed position. */
                fbe_test_sep_drive_wait_permanent_spare_swap_in(current_rg_config_p, position_to_insert);
            
                mut_printf(MUT_LOG_TEST_STATUS, "== %s swap in complete for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            }
        }

        current_rg_config_p++;
    }
    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        for (drive_number = 0; drive_number < saved_num_removed_drives[index]; drive_number++)
        {
            position_to_insert = saved_position_to_reinsert[index][drive_number];

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting unused drive rg index %d (%d_%d_%d). ==", 
                       __FUNCTION__, index, 
                       unused_drive_to_insert_p->bus, unused_drive_to_insert_p->enclosure, unused_drive_to_insert_p->slot);

            /* Insert removed drive back. */
            if(fbe_test_util_is_simulation())
            {
                if (1)
                {
                    status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                             unused_drive_to_insert_p->enclosure, 
                                                             unused_drive_to_insert_p->slot,
                                                             current_rg_config_p->drive_handle[position_to_insert]);
                }
                else
                {
                    status = fbe_test_pp_util_insert_sas_drive(unused_drive_to_insert_p->bus, 
                                                               unused_drive_to_insert_p->enclosure, 
                                                               unused_drive_to_insert_p->slot,
                                                               520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                               &current_rg_config_p->peer_drive_handle[position_to_insert]);
                }
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_sim_transport_connection_target_t   local_sp;
                    fbe_sim_transport_connection_target_t   peer_sp;

                    /*  Get the ID of the local SP. */
                    local_sp = fbe_api_sim_transport_get_target_server();

                    /*  Get the ID of the peer SP. */
                    if (FBE_SIM_SP_A == local_sp)
                    {
                        peer_sp = FBE_SIM_SP_B;
                    }
                    else
                    {
                        peer_sp = FBE_SIM_SP_A;
                    }

                    /*  Set the target server to the peer and reinsert the drive there. */
                    fbe_api_sim_transport_set_target_server(peer_sp);
                    if (1)
                    {
                        status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                                 unused_drive_to_insert_p->enclosure, 
                                                                 unused_drive_to_insert_p->slot,
                                                                 current_rg_config_p->drive_handle[position_to_insert]);
                    }
                    else
                    {
                        status = fbe_test_pp_util_insert_sas_drive(unused_drive_to_insert_p->bus, 
                                                                   unused_drive_to_insert_p->enclosure, 
                                                                   unused_drive_to_insert_p->slot,
                                                                   520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                                   &current_rg_config_p->peer_drive_handle[position_to_insert]);
                    }
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                            unused_drive_to_insert_p->enclosure, 
                                                            unused_drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_sim_transport_connection_target_t   local_sp;
                    fbe_sim_transport_connection_target_t   peer_sp;

                    /*  Get the ID of the local SP. */
                    local_sp = fbe_api_sim_transport_get_target_server();

                    /*  Get the ID of the peer SP. */
                    if (FBE_SIM_SP_A == local_sp)
                    {
                        peer_sp = FBE_SIM_SP_B;
                    }
                    else
                    {
                        peer_sp = FBE_SIM_SP_A;
                    }

                    /*  Set the target server to the peer and reinsert the drive there. */
                    fbe_api_sim_transport_set_target_server(peer_sp);
                    status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                                unused_drive_to_insert_p->enclosure, 
                                                                unused_drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
        
            /* Wait for the unused pvd object to be in ready state. */
            status = fbe_test_sep_util_wait_for_pvd_state(unused_drive_to_insert_p->bus, 
                                                          unused_drive_to_insert_p->enclosure, 
                                                          unused_drive_to_insert_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          FBE_TEST_SEP_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    return;
}

/*!***************************************************************************
 *          fbe_test_sep_drive_check_for_disk_zeroing_event()
 *****************************************************************************
 * 
 * @brief   Wait for the disk zero event to be logged. If it does not find the event in first attempt then
 *             it wait for some time and check that again in loop. It will report an error if get timeout.
 *
 * @param   pvd_object_id -   object id to wait for
 *               message_code - event message code
 *               bus          -         bus number 
 *               enclosure  -         enclosure number
 *               slot -       -         slot number

 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_drive_check_for_disk_zeroing_event(fbe_object_id_t pvd_object_id, 
                                        fbe_u32_t message_code,  fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_status_t                        status = FBE_STATUS_OK; 
    fbe_u32_t                           current_time = 0;
    fbe_bool_t                          is_msg_present = FBE_FALSE; 

    /* There might be small delay to logged an event so lets wait for some time if we do not find it in first attempt */
    while (current_time < FBE_TEST_SEP_DRIVE_WAIT_MSEC)
    {

        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                                        &is_msg_present, 
                                                        message_code,
                                                        bus,
                                                        enclosure,
                                                        slot,
                                                        pvd_object_id);

        /* Now process the status
        */
        if ((status== FBE_STATUS_OK)&&(is_msg_present == FBE_TRUE)){
            mut_printf(MUT_LOG_TEST_STATUS,  
                        "Disk zeroing %s event exists for PVD obj: 0x%x ", 
                        (message_code == SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED) ? "started" : "completed", 
                        pvd_object_id );
            return status;
        }

        current_time = current_time + 500;
        fbe_api_sleep(500);
    }

    /* We timed out
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS,  
            "Disk zeroing %s event does not exists for PVD obj: 0x%x ", 
            (message_code == SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED) ? "started" : "completed", 
            pvd_object_id );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*****************************************************************
 * end fbe_test_sep_drive_check_for_disk_zeroing_event()
 *****************************************************************/


/*!**************************************************************
 * fbe_test_sep_drive_disable_background_zeroing_for_all_pvds()
 ****************************************************************
 * @brief
 *  This util function will disable background zeroing operation on all provision 
 *  drive objects.
 * 
 * 
 * @return fbe_status_t 
 *
 * @author
 *  12/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_drive_disable_background_zeroing_for_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /* First get all list of provision drive objects
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== == disable BGZ for all PVDs, total pvd objects %d. ==", num_objects);

    /* Iterate over each of the PVD objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* disable background zeroing
         */
        status = fbe_api_base_config_disable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== disable BGZ,  pvd id 0x%x ==", obj_list_p[index]);
        
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/*****************************************************************
 * end fbe_test_sep_drive_disable_background_zeroing_for_all_pvds()
 *****************************************************************/

/*!**************************************************************
 * fbe_test_sep_drive_disable_sniff_verify_for_all_pvds()
 ****************************************************************
 * @brief
 *  This util function will disable sniff verify operation on all provision 
 *  drive objects.
 * 
 * 
 * @return fbe_status_t 
 *
 * @author
 *  3/26/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t  fbe_test_sep_drive_disable_sniff_verify_for_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /* First get all list of provision drive objects
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== == disable sniff verify for all PVDs, total pvd objects %d. ==", num_objects);

    /* Iterate over each of the PVD objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* disable sniff
         */
        status = fbe_api_base_config_disable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== disable SNIFF,  pvd id 0x%x ==", obj_list_p[index]);
        
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/*****************************************************************
 * end fbe_test_sep_drive_disable_background_zeroing_for_all_pvds()
 *****************************************************************/

/*!**************************************************************
 * fbe_test_sep_drive_enable_background_zeroing_for_all_pvds()
 ****************************************************************
 * @brief
 *  This util function will enable background zeroing operation on all provision 
 *  drive objects.
 * 
 * 
 * @return fbe_status_t 
 *
 * @author
 *  12/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_drive_enable_background_zeroing_for_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /* First get all list of provision drive objects
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== == disable BGZ for all PVDs, total pvd objects %d. ==", num_objects);

    /* Iterate over each of the PVD objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* disable background zeroing
         */
        status = fbe_api_base_config_enable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== enable BGZ,  pvd id 0x%x ==", obj_list_p[index]);
        
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/*****************************************************************
 * end fbe_test_sep_drive_enable_background_zeroing_for_all_pvds()
 *****************************************************************/

/*!**************************************************************
 * fbe_test_sep_drive_enable_sniff_verify_for_all_pvds()
 ****************************************************************
 * @brief
 *  This util function will enable sniff verify operation on all provision 
 *  drive objects.
 * 
 * 
 * @return fbe_status_t 
 *
 * @author
 *  3/26/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t  fbe_test_sep_drive_enable_sniff_verify_for_all_pvds(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /* First get all list of provision drive objects
     */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable sniff verify for all PVDs, total pvd objects %d. ==", num_objects);

    /* Iterate over each of the PVD objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* disable sniff
         */
        status = fbe_api_base_config_enable_background_operation(obj_list_p[index], FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== enable SNIFF,  pvd id 0x%x ==", obj_list_p[index]);
        
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/*****************************************************************
 * end fbe_test_sep_drive_enable_background_sniff_for_all_pvds()
 *****************************************************************/

/*!****************************************************************************
 * fbe_test_sep_drive_pull_drive()
 ******************************************************************************
 * @brief
 *  This funtion is used to remove the drive.
 *
 * @param   drive_to_remove_p - Pointer to drive location for drive to remove
 * @param   out_drive_info_p - Pointer to drive handle to save for re-insertion
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t fbe_test_sep_drive_pull_drive(fbe_test_raid_group_disk_set_t *drive_to_remove_p,
                                           fbe_api_terminator_device_handle_t *out_drive_info_p)

                            
{
    fbe_status_t                        status;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                          b_pull_drive = FBE_TRUE;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_remove_from_both = FBE_FALSE;
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
        b_remove_from_both = FBE_TRUE;
    }

    /* get the pvd object-id by location. */
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus,
                                                            drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(pvd_object_id, FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "%s pvd obj: 0x%x (%d_%d_%d) on SP: %d(%d)",
                __FUNCTION__, pvd_object_id,
               drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot,
               this_sp, (b_remove_from_both) ? other_sp : this_sp);

    /* Verify state before being removed from both SPs*/
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id,
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     10000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the drive from both SPs.*/
    if(fbe_test_util_is_simulation())
    {
        /* We only ever pull drives since on hardware and simulation we 
         * test the same way.  Pull the drive out and then re-insert it. 
         * Or pull the drive out, let a spare swap in and then re-insert the pulled drive. 
         * This is because we cannot insert "new" drives on hardware and we would like to 
         * test the same way on hardware and in simulation. 
         */
        if (1 || b_pull_drive)
        {
            status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure, 
                                                 drive_to_remove_p->slot,
                                                 out_drive_info_p);
        }
        else
        {
            status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                       drive_to_remove_p->enclosure, 
                                                       drive_to_remove_p->slot);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (b_remove_from_both)
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            if (1 || b_pull_drive)
            {
                status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot,
                                                     out_drive_info_p);
            }
            else
            {
                status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                           drive_to_remove_p->enclosure, 
                                                           drive_to_remove_p->slot);
            }
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                             drive_to_remove_p->enclosure, 
                                             drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (b_remove_from_both)
        {
            /* Set the target server to SPB and remove the drive there. */
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                    drive_to_remove_p->enclosure, 
                                                    drive_to_remove_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the target server back to SPA. */
            fbe_api_sim_transport_set_target_server(this_sp);
        }

    }

    /* Verify state after being removed from both SPs*/
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id,
                                                     FBE_LIFECYCLE_STATE_FAIL,
                                                     10000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}
/*****************************************
 * end fbe_test_sep_drive_pull_drive()
 *****************************************/

/*!***************************************************************************
 *          fbe_test_sep_drive_reinsert_drive()
 ***************************************************************************** 
 * 
 * @brief   Re-insert a previously removed drive.
 *
 * @param   drive_to_insert_p - Pointer to drive location for drive to re-insert.
 * @param   drive_handle_to_insert - Drive handle for drive to re-insert.
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 *****************************************************************************/
fbe_status_t fbe_test_sep_drive_reinsert_drive(fbe_test_raid_group_disk_set_t *drive_to_insert_p,
                                               fbe_api_terminator_device_handle_t drive_handle_to_insert)
                            
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_insert_on_both = FBE_FALSE;
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
        b_insert_on_both = FBE_TRUE;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s handle: 0x%llx(%d_%d_%d) on SP: %d(%d)",
                __FUNCTION__, drive_handle_to_insert,
               drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot,
               this_sp, (b_insert_on_both) ? other_sp : this_sp);

    /* inserting the same drive back */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                 drive_to_insert_p->enclosure, 
                                                 drive_to_insert_p->slot,
                                                 drive_handle_to_insert);
       MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
       if (b_insert_on_both)
       {
           fbe_api_sim_transport_set_target_server(other_sp);
           status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot,
                                                    drive_handle_to_insert);
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
           fbe_api_sim_transport_set_target_server(this_sp);
       }
    }
    else
    {
       status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                   drive_to_insert_p->enclosure, 
                                                   drive_to_insert_p->slot);
       MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

       if (b_insert_on_both)
       {
           /* Set the target server to SPB and insert the drive there. */
           fbe_api_sim_transport_set_target_server(other_sp);
           status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                       drive_to_insert_p->enclosure, 
                                                       drive_to_insert_p->slot);
           MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

           /* Set the target server back to SPA. */
           fbe_api_sim_transport_set_target_server(this_sp);
       }
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Verify the PDO and LDO are in the READY state
     */
    status = fbe_test_pp_util_verify_pdo_state(drive_to_insert_p->bus,      
                                               drive_to_insert_p->enclosure,
                                               drive_to_insert_p->slot,  
                                               FBE_LIFECYCLE_STATE_READY,
                                               10000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*****************************************
 * end fbe_test_sep_drive_reinsert_drive()
 *****************************************/
