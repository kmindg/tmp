#include "generics.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_package.h" 
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "dba_export_types.h"



static fbe_u32_t get_elapsed_seconds(fbe_u64_t timestamp);
static fbe_u64_t get_time();
static void admin_verify_notification_callback_function (update_object_msg_t * update_object_msg, void *context);
static void admin_verify_play_with_physical(fbe_object_id_t *obj_array, fbe_u32_t total_obj, fbe_u32_t loops);
static void admin_verify_play_with_sep(fbe_object_id_t *obj_array, fbe_u32_t total_obj, fbe_u32_t loops);
static fbe_status_t admin_verify_add_phy_object (fbe_object_id_t object_id);
static fbe_status_t admin_verify_add_logical_object (fbe_object_id_t object_id);
static fbe_status_t admin_verify_add_board_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_port_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_encl_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_disk_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_lun(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_pvd(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_rg(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static fbe_status_t admin_verify_add_vd(fbe_object_id_t object_id, fbe_lifecycle_state_t state);
static void admin_get_all_raid_groups();
static void admin_get_all_luns();
static void admin_get_all_pvds();
static void simulate_full_poll();
static void simulate_full_poll_threshold_reached();

static fbe_u32_t api_count = 0;

int __cdecl main(int argc, char *argv[])
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t	*						sep_obj_array = NULL;
	fbe_object_id_t	*						pp_obj_array = NULL;
	fbe_u32_t								total_objects = 0;
	fbe_u32_t								actual = 0;
	fbe_notification_registration_id_t		reg_id;
	fbe_u32_t 							total_luns, returned_luns;
	fbe_database_lun_info_t *			lun_info;
    fbe_u32_t                           index;
    fbe_topology_object_type_t          object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_api_provision_drive_update_pool_id_list_t update_pvd_pool_id_list;
    fbe_object_id_t                     lun_obj_id_1 = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     lun_obj_id_2 = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           lun_number_1 = 10000; /* Just hardcode */
    fbe_u32_t                           lun_number_2 = 10001;
    fbe_cmi_service_get_info_t          cmi_info;
    fbe_job_service_error_type_t    job_error_type;
    fbe_assigned_wwid_t          wwn1;
    fbe_assigned_wwid_t          wwn2;

    


#include "fbe/fbe_emcutil_shell_maincode.h"

#if 0
    /*this app will work in user space and talk to the simulator. We can switch this easily since we also
	link with the user mode that talks to kernel, just needs a different init*/
	 status  = fbe_api_common_init_sim();
	  if (status != FBE_STATUS_OK) {
        printf("\nCan't connect to FBE API User/Kernel, make sure FBE is running !!!\n");
        return -1;
    }
  /*set function entries for commands that would go to the physical package*/
	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

	/*set function entries for commands that would go to the sep package*/
	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

	/*set function entries for commands that would go to the esp package*/
	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

	/*set function entries for commands that would go to the neit package*/
	fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
												   fbe_api_sim_transport_send_io_packet,
												   fbe_api_sim_transport_send_client_control_packet);

   /*need to initialize the client to connect to the server*/
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_sim_transport_init_client(FBE_SIM_SP_A, FBE_TRUE);
#else
	fbe_api_common_init_user(FBE_TRUE);
#endif
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_SWAP_INFO,
																  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
																  FBE_TOPOLOGY_OBJECT_TYPE_ALL,
																  admin_verify_notification_callback_function,
																  NULL,
																  &reg_id);


	if (status != FBE_STATUS_OK) {
		printf("\nCAn't register!!!\n");
		return -1;
	}

	// panic the array if we have too many full polls
	simulate_full_poll_threshold_reached();

	/*discover physical pacakge*/
	status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
		printf("\nfbe_api_get_total_objects from Physical FAILED !!!\n");
        return -1;
	}

	pp_obj_array = (fbe_object_id_t *)malloc (sizeof(fbe_object_id_t) * total_objects);

	status = fbe_api_enumerate_objects(pp_obj_array, total_objects, &actual, FBE_PACKAGE_ID_PHYSICAL);
	printf("Physical pacakge total_objects:%d status:%d\n", actual, status);

	/*discover SEP pacakge*/
	status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
		printf("\nfbe_api_get_total_objects from SEP FAILED !!!\n");
        return -1;
	}

	sep_obj_array = (fbe_object_id_t *)malloc (sizeof(fbe_object_id_t) * total_objects);

	status = fbe_api_enumerate_objects(sep_obj_array, total_objects, &actual, FBE_PACKAGE_ID_SEP_0);
	printf("SEP pacakge total_objects:%d status:%d\n", actual, status);

    /*Update all pvds pool id */
	printf("SEP pacakge: start to update PVD pool id with 5. time = %llu \n", get_time());
    fbe_zero_memory(&update_pvd_pool_id_list, sizeof(update_pvd_pool_id_list));
    for (index = 0; index < actual; index++)
    {
        status = fbe_api_get_object_type(sep_obj_array[index], &object_type, FBE_PACKAGE_ID_SEP_0);
        if (object_type != FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE)
            continue;
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_object_id = sep_obj_array[index];
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_pool_id = 5;
        update_pvd_pool_id_list.pvd_pool_data_list.total_elements++;

        if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements == MAX_UPDATE_PVD_POOL_ID)
        {
            status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
            update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
        }
    }
    if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements != 0)
    {
            status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
            update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
    }
	printf("SEP pacakge: done with updating PVD pool id with 5 . time = %llu\n", get_time());
	printf("SEP pacakge: reset the pool id with FBE_POOL_ID_INVALID. time = %llu \n", get_time());
    fbe_zero_memory(&update_pvd_pool_id_list, sizeof(update_pvd_pool_id_list));
    for (index = 0; index < actual; index++)
    {
        status = fbe_api_get_object_type(sep_obj_array[index], &object_type, FBE_PACKAGE_ID_SEP_0);
        if (object_type != FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE)
            continue;
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_object_id = sep_obj_array[index];
        update_pvd_pool_id_list.pvd_pool_data_list.pvd_pool_data[update_pvd_pool_id_list.pvd_pool_data_list.total_elements].pvd_pool_id = FBE_POOL_ID_INVALID;
        update_pvd_pool_id_list.pvd_pool_data_list.total_elements++;

        if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements == MAX_UPDATE_PVD_POOL_ID)
        {
            status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
            update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
        }
    }
    if (update_pvd_pool_id_list.pvd_pool_data_list.total_elements != 0)
    {
            status = fbe_api_provision_drive_update_pool_id_list(&update_pvd_pool_id_list);
            update_pvd_pool_id_list.pvd_pool_data_list.total_elements = 0;
    }
	printf("SEP pacakge: done with resetting the pool id with FBE_POOL_ID_INVALID. time = %llu \n", get_time());

	free (pp_obj_array);
	free (sep_obj_array);

	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
    lun_info = (fbe_database_lun_info_t *)malloc(total_luns * sizeof(fbe_database_lun_info_t));
    fbe_zero_memory(lun_info, sizeof(fbe_database_lun_info_t) * total_luns);
    status = fbe_api_database_get_all_luns(lun_info, total_luns, &returned_luns);
	printf("I got %d luns, status: %d\n", returned_luns, status);
    for (index = 0; index < returned_luns; index++) {
        printf(" index = %d. Lun obj_id: %d, lun_num: %d\n", index, lun_info[index].lun_object_id, lun_info[index].lun_number);
    }
	free(lun_info);


    /* Update LUN wwn */
    for (index = 0; index < 16; index++)
    {
        wwn1.bytes[index] = (fbe_u8_t)1;
        wwn2.bytes[index] = (fbe_u8_t)2;
    }

    /* We create two LUNs before running this tool */
    status = fbe_api_database_lookup_lun_by_number(lun_number_1, &lun_obj_id_1);
    if (status != FBE_STATUS_OK || lun_obj_id_1 == FBE_OBJECT_ID_INVALID)
        return 0;
    status = fbe_api_database_lookup_lun_by_number(lun_number_2, &lun_obj_id_2);
    if (status != FBE_STATUS_OK || lun_obj_id_2 == FBE_OBJECT_ID_INVALID)
        return 0;


    fbe_api_cmi_service_get_info(&cmi_info);
    /* active SP update LUN 1; passive sp update LUN 2*/
    if (cmi_info.sp_state == FBE_CMI_STATE_ACTIVE)
    {
        for (index = 0; index < 10000; index++)
        {
            status = fbe_api_update_lun_wwn(lun_obj_id_1, &wwn1, &job_error_type);
            printf(" update lun1 (obj_id %x) wwn. status = %d, time = %llu \n", lun_obj_id_1, status, get_time());
        }
            printf(" update lun1 (obj_id %x) wwn DONE . xxxxxxxxxxxxxxxxxx .time = %llu \n", lun_obj_id_1, get_time());
    } else {
        for (index = 0; index < 10000; index++)
        {
            status = fbe_api_update_lun_wwn(lun_obj_id_2, &wwn2, &job_error_type);
            printf(" update lun2 (obj_id %x) wwn. status = %d, time = %llu \n", lun_obj_id_2, status, get_time());
        }
            printf(" update lun2 (obj_id %x) wwn DONE . xxxxxxxxxxxxxxxxxx .time = %llu \n", lun_obj_id_2, get_time());
    }


    fbe_api_sleep(INFINITE);
	
	return 0;
}


static void admin_verify_notification_callback_function (update_object_msg_t * update_object_msg, void *context)
{
	printf("Notification:Pkg:0x%X, objid:0x%X, obj_type:0x%llX, notify_type:0x%llX\n", 
		   update_object_msg->notification_info.source_package,
		   update_object_msg->object_id,
		   update_object_msg->notification_info.object_type,
		   update_object_msg->notification_info.notification_type );

	if (update_object_msg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION) {
		if (update_object_msg->notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_START) {
			printf("Reconstruct:start\n");
		}else if (update_object_msg->notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_END){
			printf("Reconstruct:end\n");
		}else{
			printf("Reconstruct:percent:%d\n",update_object_msg->notification_info.notification_data.data_reconstruction.percent_complete );
		}
	}

	if (update_object_msg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_ZEROING) {
		if (update_object_msg->notification_info.notification_data.object_zero_notification.zero_operation_status == FBE_OBJECT_ZERO_STARTED) {
			printf("Zero:start\n");
		}else if (update_object_msg->notification_info.notification_data.object_zero_notification.zero_operation_status == FBE_OBJECT_ZERO_ENDED){
			printf("Zero:end\n");
		}else{
			printf("Zero:percent:%d\n", update_object_msg->notification_info.notification_data.object_zero_notification.zero_operation_progress_percent );
		}
	}

	if (update_object_msg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_SWAP_INFO) {

		if (update_object_msg->notification_info.notification_data.swap_info.command_type == FBE_SPARE_COMPLETE_COPY_COMMAND) {
	
			
			fbe_object_id_t rgid;
			fbe_database_raid_group_info_t rginfo;
			fbe_u32_t x;

			fbe_api_sleep(5);
	
			fbe_api_database_lookup_raid_group_by_number(0, &rgid);
			fbe_api_database_get_raid_group(rgid, &rginfo);
			for (x = 0; x < rginfo.pvd_count; x++) {
				printf("pvdid %d\n", rginfo.pvd_list[x]);
	
			}
		}
	}

}

#if defined(__cplusplus)
extern "C" {
fbe_status_t __stdcall fbe_get_package_id(fbe_package_id_t * package_id)
#else
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
#endif												  
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}

#if defined(__cplusplus)
}
#else

#endif

static void admin_verify_play_with_physical(fbe_object_id_t *obj_array, fbe_u32_t total_obj, fbe_u32_t loops)
{
	fbe_status_t 	status;
	fbe_u32_t		current_obj;
	fbe_u32_t		loop_count;

	for (loop_count = 0; loop_count < loops; loop_count++) {
		for (current_obj = 0; current_obj < total_obj ; current_obj++) {
			status = admin_verify_add_phy_object(obj_array[current_obj]);
		}
	}

}


static void admin_verify_play_with_sep(fbe_object_id_t *obj_array, fbe_u32_t total_obj, fbe_u32_t loops)
{
	fbe_status_t 	status;
	fbe_u32_t		current_obj;
	fbe_u32_t		loop_count;

	for (loop_count = 0; loop_count < loops; loop_count++) {
		for (current_obj = 0; current_obj < total_obj ; current_obj++) {
			status = admin_verify_add_logical_object(obj_array[current_obj]);
		}
	}


}


static fbe_status_t admin_verify_add_phy_object (fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_topology_object_type_t                  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_lifecycle_state_t                       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*first get the object type*/
    status = fbe_api_get_object_type (object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
	api_count++;

    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*let's get the state of teh object ot see if we can add it to the map*/
    status = fbe_api_get_object_lifecycle_state (object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
	api_count++;
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*we insert the object to the map only when it's ready, if it's not, when it would become ready we would add it via notification*/
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_FAIL)) {
        return FBE_STATUS_OK;
    }

    /*based on the object type we found we send a request to the map to change the state*/
    switch (object_type){
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:     
        status = admin_verify_add_board_object (object_id, lifecycle_state);            
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PORT:     
        status = admin_verify_add_port_object (object_id, lifecycle_state);         
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = admin_verify_add_encl_object (object_id, lifecycle_state);
        break;          
    case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
        status = admin_verify_add_disk_object (object_id, lifecycle_state);
        break;
    default:
        status =  FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
    
}

static fbe_status_t admin_verify_add_board_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_board_mgmt_io_comp_info_t comp;

	status = fbe_api_board_get_iom_info(object_id, &comp);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	printf("%s: id:%d\n", __FUNCTION__,  object_id);

	return FBE_STATUS_OK;

	
}

static fbe_status_t admin_verify_add_port_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_port_link_information_t 	link_info;

	status = fbe_api_port_get_link_information(object_id, &link_info);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;
}

static fbe_status_t admin_verify_add_encl_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
   fbe_enclosure_fault_t fault;

	status = fbe_api_enclosure_get_fault_info(object_id, &fault);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;
}

static fbe_status_t admin_verify_add_disk_object(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_physical_drive_information_t drive_info;
	fbe_bool_t					enabled;
	fbe_physical_drive_mgmt_get_error_counts_t  error_counts;

	status = fbe_api_physical_drive_get_cached_drive_info(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	status = fbe_api_physical_drive_is_wr_cache_enabled(object_id, &enabled, FBE_PACKET_FLAG_NO_ATTRIB);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	status = fbe_api_physical_drive_get_error_counts(object_id, &error_counts, FBE_PACKET_FLAG_NO_ATTRIB);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	
	printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;
}


static csx_timestamp_frequency_t performance_frequency = 0;

fbe_u64_t static get_time()
{
    fbe_time_t            t = 0;
    csx_timestamp_value_t performance_count = 0;
    
    /* The first time we need to retreive the performance_frequency */
    if(performance_frequency == 0){
        csx_p_get_timestamp_frequency(&performance_frequency);
    }
    
    csx_p_get_timestamp_value(&performance_count);
    if (performance_frequency > 1000L) {
        t = performance_count / (performance_frequency / 1000L);
    } else {
        t = (performance_count * 1000L) / performance_frequency;
    }

    return t;
}



static fbe_u32_t get_elapsed_seconds(fbe_u64_t timestamp)
{
    fbe_time_t  delta_time = (get_time() - timestamp) / (fbe_time_t)1000;

    if (delta_time > (fbe_time_t)0xffffffff)
    {
        return(0xffffffff);
    }
    else
    {
        return((fbe_u32_t)delta_time);
    }
}

static fbe_status_t admin_verify_add_logical_object (fbe_object_id_t object_id)
{
	fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_topology_object_type_t                  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_lifecycle_state_t                       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*first get the object type, BVD will return error, this is fine*/
    status = fbe_api_get_object_type (object_id, &object_type, FBE_PACKAGE_ID_SEP_0);
	api_count++;
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*let's get the state of teh object ot see if we can add it to the map*/
    status = fbe_api_get_object_lifecycle_state (object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
	api_count++;
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*we insert the object to the map only when it's ready, if it's not, when it would become ready we would add it via notification*/
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_FAIL)) {
        return FBE_STATUS_OK;
    }

    /*based on the object type we found we send a request to the map to change the state*/
    switch (object_type){
    case FBE_TOPOLOGY_OBJECT_TYPE_LUN:     
        status = admin_verify_add_lun (object_id, lifecycle_state);            
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP:     
        status = admin_verify_add_rg (object_id, lifecycle_state);         
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE:
        status = admin_verify_add_pvd (object_id, lifecycle_state);
        break;          
    case FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE:
        status = admin_verify_add_vd (object_id, lifecycle_state);
        break;
    default:
        status =  FBE_STATUS_OK;/*don'e care in this app*/
		break;
    }

    return status;

}

static fbe_status_t admin_verify_add_lun(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_api_lun_get_lun_info_t  		lun_info;
	fbe_api_base_config_downstream_object_list_t list;
    fbe_api_base_config_get_width_t width;
    

	status = fbe_api_lun_get_lun_info(object_id, &lun_info);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	status = fbe_api_base_config_get_downstream_object_list(object_id, &list);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	/*send to the rg*/
	status = fbe_api_base_config_get_width(list.downstream_object_list[0], &width);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}else{
		printf("%s:rg width:%d\n", __FUNCTION__,  width.width);
	}


    printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;

}

static fbe_status_t admin_verify_add_pvd(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_provision_drive_control_get_metadata_memory_t memory;
	

	status = fbe_api_provision_drive_get_metadata_memory(object_id, &memory);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;

}

static fbe_status_t admin_verify_add_rg(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
	fbe_api_raid_group_get_info_t info;
	fbe_api_base_config_upstream_object_list_t list;
	fbe_api_base_config_get_width_t width;
	fbe_u32_t 	i;
	

	status = fbe_api_raid_group_get_info(object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	status = fbe_api_base_config_get_upstream_object_list(object_id, &list);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	for (i = 0; i < list.number_of_upstream_objects; i++) {
		/*send to the rg*/
		status = fbe_api_base_config_get_width(list.upstream_object_list[0], &width);
		api_count++;
		if (status != FBE_STATUS_OK) {
			printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
		}else{
			printf("%s:lun width:%d\n", __FUNCTION__,  width.width);
		}
	}


    printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;

}

static fbe_status_t admin_verify_add_vd(fbe_object_id_t object_id, fbe_lifecycle_state_t state)
{
	fbe_status_t					status;
    fbe_api_virtual_drive_get_configuration_t get;
	

	status = fbe_api_virtual_drive_get_configuration(object_id, &get);
	api_count++;
	if (status != FBE_STATUS_OK) {
		printf("%s:ERROR: id:%d\n", __FUNCTION__,  object_id);
	}

	printf("%s: id:%d\n", __FUNCTION__,  object_id);
	return FBE_STATUS_OK;

}

static void simulate_full_poll_threshold_reached() {
	fbe_u32_t                           i = 0;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_database_control_get_stats_t *  get_stats = NULL;
    fbe_u32_t                           threshold_count = 0;

    get_stats = (fbe_database_control_get_stats_t *)malloc(sizeof(fbe_database_control_get_stats_t));
    fbe_zero_memory(get_stats,  sizeof(fbe_database_control_get_stats_t));
    status = fbe_api_database_get_stats(get_stats);
    if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
    threshold_count = get_stats->threshold_count;

    // do N full polls to increase the threshold count
    for (i=0; i< FBE_DATABASE_MAX_POLL_RECORDS ; i++) {
		printf("Iteration %d: issuing full poll\n", i);
		if (get_stats->threshold_count != threshold_count) {
			free(get_stats);
			printf("Unxexpected threshold count\n");
			return;
		}
        simulate_full_poll();
    }

    printf("Verify threshold count increases after %d consecutive full polls\n",
               FBE_DATABASE_MAX_POLL_RECORDS); 
    status = fbe_api_database_get_stats(get_stats);
	if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
	if (get_stats->threshold_count != threshold_count+1) {
		printf("Unxexpected threshold count\n");
	}

    free(get_stats);
}

// Simulate a full poll by admin with consecutive calls for get_all_luns, get_all_rgs, and get_all_pvds
static void simulate_full_poll(void)
{   
    fbe_database_control_get_stats_t *  get_stats;
    fbe_status_t                        status;
    fbe_u32_t                           current_threshold_count = 0;

    printf("Issue a full poll\n"); 
    get_stats = (fbe_database_control_get_stats_t *)malloc(sizeof(fbe_database_control_get_stats_t));
    fbe_zero_memory(get_stats,  sizeof(fbe_database_control_get_stats_t));


	admin_get_all_raid_groups();
    status = fbe_api_database_get_stats(get_stats);
	if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
	if (POLL_REQUEST_GET_ALL_RAID_GROUPS != get_stats->last_poll_bitmap) {
        printf("Unmatching bitmap %d\n", get_stats->last_poll_bitmap);
		free(get_stats);
		return;
	}
	if (get_stats->threshold_count != current_threshold_count) { 
		free(get_stats);
        printf("Unxexpected threshold count\n");
		return;
	}

    admin_get_all_luns();
    status = fbe_api_database_get_stats(get_stats);
	if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
	if ((POLL_REQUEST_GET_ALL_LUNS | POLL_REQUEST_GET_ALL_RAID_GROUPS) != get_stats->last_poll_bitmap) {
		free(get_stats);
		printf("Unmatching bitmap\n");
		return;
	}
    current_threshold_count = get_stats->threshold_count;

	admin_get_all_pvds();
    status = fbe_api_database_get_stats(get_stats);
	if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
	if (0 != get_stats->last_poll_bitmap) {
		free(get_stats);
		printf("Unmatching bitmap\n");
		return;
	}

	admin_get_all_raid_groups();
    status = fbe_api_database_get_stats(get_stats);
	if (status != FBE_STATUS_OK) {
		free(get_stats);
		printf("failed to get stats\n"); 
		return;
	}
	if (POLL_REQUEST_GET_ALL_RAID_GROUPS != get_stats->last_poll_bitmap) {
		free(get_stats);
        printf("Unmatching bitmap\n");
		return;
	}

    free(get_stats);
}

static void admin_get_all_raid_groups() {
	fbe_u32_t 						    total_rgs, returned_rgs;
	fbe_database_raid_group_info_t *	rg_info = NULL;
	fbe_status_t                        status;

	printf("Verifying poll stats for getting all raid groups\n"); 

	/*also, test the interface that is getting all the raid groups in one shot*/
	status = fbe_api_get_total_objects_of_all_raid_groups(FBE_PACKAGE_ID_SEP_0, &total_rgs);
	if (status != FBE_STATUS_OK) {
		printf("failed to enumerate all raid group class!!!\n"); 
		return;
	}
	rg_info = (fbe_database_raid_group_info_t *)malloc(total_rgs * sizeof(fbe_database_raid_group_info_t));
	fbe_zero_memory(rg_info, total_rgs * sizeof(fbe_database_raid_group_info_t));
	status = fbe_api_database_get_all_raid_groups(rg_info, total_rgs, &returned_rgs);
	if (status != FBE_STATUS_OK) {
		free(rg_info);
		printf("failed to get all raid groups\n"); 
		return;
	}
	free(rg_info);
}

static void admin_get_all_luns() {
	fbe_u32_t 							total_luns, returned_luns;
	fbe_database_lun_info_t *			lun_info = NULL;
	fbe_status_t                        status;

	printf("Verifying poll stats for getting all luns\n"); 
	/*also, test the interface that is getting all the luns in one shot*/
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
	if (status != FBE_STATUS_OK) {
		printf("failed to enumerate lun class!!!\n"); 
		free(lun_info);
		return;
	}
	lun_info = (fbe_database_lun_info_t *)malloc(total_luns * sizeof(fbe_database_lun_info_t));
	fbe_zero_memory(lun_info, total_luns * sizeof(fbe_database_lun_info_t));
	status = fbe_api_database_get_all_luns(lun_info, total_luns, &returned_luns);
	if (status != FBE_STATUS_OK) {
		printf("failed to get all luns\n"); 
		free(lun_info);
		return;
	}

	free(lun_info);

}

static void admin_get_all_pvds() {
	fbe_u32_t 					        total_pvds, returned_pvds;
	fbe_database_pvd_info_t *	        pvd_list = NULL;
	fbe_status_t                        status;

	printf("Verifying poll statistics after getting all pvds\n"); 
	/*also, test the interface that is getting all the raid groups in one shot*/
	status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &total_pvds);
	if (status != FBE_STATUS_OK) {
		free(pvd_list);
		printf("failed to enumerate all PVD class!!!\n"); 
		return;
	}
	pvd_list = (fbe_database_pvd_info_t *)malloc(total_pvds * sizeof(fbe_database_pvd_info_t));
	fbe_zero_memory(pvd_list, total_pvds * sizeof(fbe_database_pvd_info_t));
	status = fbe_api_database_get_all_pvds(pvd_list, total_pvds, &returned_pvds);
	if (status != FBE_STATUS_OK) {
		free(pvd_list);
		printf("failed to get all pvds\n"); 
		return;
	}
	free(pvd_list);
}



