#include "fbe/fbe_terminator_api.h"
#include "simulation/cache_to_mcr_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_database_interface.h"

/*3 drives + 1 terminating entry*/
#define CACHE_TO_MCR_DRIVES_IN_SET (3+1)
#define CACHE_TO_MCR_MAX_RG 100

typedef struct cache_to_mcr_disk_set_s {
    fbe_u32_t bus;
    fbe_u32_t enclosure;
    fbe_u32_t slot;
}cache_to_mcr_disk_set_t;

typedef struct chache_to_mcr_lu_info_s{
	fbe_object_id_t	object_id;
	fbe_lba_t		previous_offset;
	fbe_lba_t		previous_capacity;
	fbe_u32_t		previous_rg_id;
}chache_to_mcr_lu_info_t;

static cache_to_mcr_disk_set_t *	cache_to_mcr_disk_set;
static fbe_u32_t	total_sets = 0; /*the maximum sets we have*/
static fbe_u32_t	current_set = 0;/*we will rotate through these*/
static fbe_object_id_t	rg_id_array[CACHE_TO_MCR_MAX_RG];
static chache_to_mcr_lu_info_t	lu_array[CACHE_TO_MCR_MAX_LU];

void cache_to_mcr_transport_1enclosure_15drives(void)
{
	fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_u64_t                           sas_addr = (fbe_u64_t)0x987654321;
    fbe_u32_t                           slot = 0;
    
	fbe_u32_t							slot_count = 0;
	cache_to_mcr_disk_set_t *			disk_g_ptr = NULL;
    
    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    
    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x0000000000000001;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port0_handle);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    
    for (slot = 0; slot < 15; slot++) {
        /*insert a sas_drive to port 0 encl 0 slot 0*/
        sas_drive.backend_number = 0;
        sas_drive.encl_number = 0;
        sas_drive.sas_address = ++sas_addr;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x1f601000/*0x10B5C730*/;
        sas_drive.block_size = 520;
        fbe_sprintf(sas_drive.drive_serial_number, 17, "%llX",
		    (unsigned long long)sas_drive.sas_address);
        printf("inserting drive with SN:%s to terminator location 0_0_%d\n", sas_drive.drive_serial_number, slot);
        status  = fbe_terminator_api_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
    }

	/*create the disk set table for a single enclosure where we can't use the first 4 drives*/
	/*we always use CACHE_TO_MCR_DRIVES_IN_SET drives and in this case we have 3 sets since we have 11 drives left to use*/
	disk_g_ptr = cache_to_mcr_disk_set = malloc( 3 * CACHE_TO_MCR_DRIVES_IN_SET * sizeof(cache_to_mcr_disk_set_t));
	memset(cache_to_mcr_disk_set, 0 , 3 * CACHE_TO_MCR_DRIVES_IN_SET * sizeof(cache_to_mcr_disk_set_t));
	slot = 4;
	do {
		disk_g_ptr->bus = 0;
		disk_g_ptr->enclosure = 0;
		disk_g_ptr->slot = slot;

		slot++;
		disk_g_ptr++;
		slot_count++;

		/*is it time for the terminating one*/
		if (slot_count == (CACHE_TO_MCR_DRIVES_IN_SET - 1)) {
			disk_g_ptr->bus = FBE_U32_MAX;
			disk_g_ptr->enclosure = FBE_U32_MAX;
			disk_g_ptr->slot = FBE_U32_MAX;

			disk_g_ptr++;
			slot_count = 0;
			total_sets ++;
		}

	} while (total_sets < 3);

	memset(lu_array, 0xFF, sizeof(chache_to_mcr_lu_info_t) * CACHE_TO_MCR_MAX_LU);/*clear lun table*/
    
}

void cache_to_mcr_transport_2enclosure_30drives(void)
{
	
	

}

void cache_to_mcr_transport_4enclosure_60drives(void)
{
	
	

}

fbe_status_t cache_to_mcr_transport_create_fbe_lun(fbe_lba_t capacity, fbe_u32_t number)
{
	fbe_status_t			status;
	fbe_api_lun_create_t	create_lu;
    fbe_database_lun_info_t	lun_info;
    fbe_job_service_error_type_t job_error_type;

    fbe_zero_memory(&create_lu, sizeof(fbe_api_lun_create_t));

	#if 0/*enable once non destructive bind works in FBE*/
	if ((lu_array[number].object_id != 0xFFFFFFFF) && (capacity == lu_array[number].previous_capacity)) {
		/*we already had a lun here, let's do a non destructive bind*/
		create_lu.ndb_b = FBE_TRUE;
		create_lu.addroffset = lu_array[number].previous_offset;
		create_lu.raid_group_id = lu_array[number].previous_rg_id;/*make sure we bind in the same location*/
	}else{
		create_lu.ndb_b = FBE_FALSE;/*brand new lun*/
		create_lu.addroffset = 0;
		create_lu.raid_group_id = current_set;
	}
	#else
	if ((lu_array[number].object_id != 0xFFFFFFFF)) {
		/*we already had a lun here*/
		return FBE_STATUS_OK;
	}
	create_lu.ndb_b = FBE_FALSE;/*brand new lun*/
    create_lu.noinitialverify_b = FBE_FALSE;
	create_lu.addroffset = 0;
	create_lu.raid_group_id = current_set;
	#endif

    create_lu.capacity = capacity;
	create_lu.lun_number = number;
	create_lu.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
	create_lu.raid_type = FBE_RAID_GROUP_TYPE_RAID5;/*really should be in the rg_id_array table*/
	sprintf((char *)&create_lu.user_defined_name, "%d", number);
	sprintf((char *)&create_lu.world_wide_name, "%d", number);

    status = fbe_api_create_lun(&create_lu, FBE_TRUE, 30000, &lu_array[number].object_id, &job_error_type);

	if (status != FBE_STATUS_OK) {
		printf("\n\nCache To Mcr Simulation Failed to create LUN %d\n\n", number);
	}else{
		/*once the lun is created, we need to know it's offset for next time if we decide to do a non destructive bind*/
		lun_info.lun_object_id = lu_array[number].object_id;
		status  = fbe_api_database_get_lun_info(&lun_info);
		if (status != FBE_STATUS_OK) {
			printf("\n\nCache To Mcr Simulation Failed to get lun %d info\n\n", number);
		}else if (create_lu.ndb_b == FBE_FALSE){
			/*remember for next time. We record the info only after the first bind and don't touch it later*/
			lu_array[number].previous_offset = lun_info.offset;
			lu_array[number].previous_capacity = capacity;
			lu_array[number].previous_rg_id = current_set;
		}

		printf("\nCache-Mcr Sim - Created LUN ID:%d, on RG ID:%d, offset:0x%llX \n", number, current_set, (unsigned long long)lun_info.offset);
	}

	current_set++;/*for next time*/
	if (current_set == total_sets) {
		current_set = 0;
	}

	return status;

}

fbe_status_t cache_to_mcr_transport_destroy_fbe_lun(fbe_u32_t number)
{

	fbe_status_t	status;
    fbe_job_service_error_type_t job_error_type;

	fbe_api_lun_destroy_t	destroy_lun;
	destroy_lun.lun_number = number;
	status = fbe_api_destroy_lun(&destroy_lun, FBE_TRUE, 30000, &job_error_type);

	return status;
    
}

fbe_status_t cache_to_mcr_transport_create_rgs(void)
{
	fbe_u32_t					set = 0;
	fbe_api_raid_group_create_t	creae_rg;
    fbe_status_t 				status;
	fbe_u32_t					disks = 0;
	cache_to_mcr_disk_set_t	*	set_ptr = cache_to_mcr_disk_set;
    fbe_job_service_error_type_t job_error_type;

    fbe_zero_memory(&creae_rg, sizeof(fbe_api_raid_group_create_t));
	/*let's create RG on each of the disk sets we have*/
	for (set = 0; set < total_sets; set++) {
		creae_rg.b_bandwidth = 0;
		creae_rg.capacity = 0x1f501000/*0xF000000*/;

		for (disks = 0; disks < CACHE_TO_MCR_DRIVES_IN_SET - 1; disks++, set_ptr++) {
			memcpy(&creae_rg.disk_array[disks], set_ptr, sizeof(cache_to_mcr_disk_set_t));
		}
		set_ptr++ ;/* skip over the terminating one (we might not need it in the future*/
        
		creae_rg.drive_count = CACHE_TO_MCR_DRIVES_IN_SET - 1;
		//creae_rg.explicit_removal = 0;
		creae_rg.is_private = FBE_TRI_FALSE;
		creae_rg.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
		creae_rg.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
		creae_rg.raid_group_id = set;
		creae_rg.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
		creae_rg.user_private = FBE_FALSE;
        
        status = fbe_api_create_rg(&creae_rg, FBE_TRUE, 30000, &rg_id_array[set], &job_error_type);
		if (status != FBE_STATUS_OK) {
			printf("\n\nCache To Mcr Simulation FAiled to create RG %d\n\n", set);
			return FBE_STATUS_GENERIC_FAILURE;
		}else{
			printf("\nCache To Mcr Simulation - Created RG ID:%d\n", set);
		}
	}

	return FBE_STATUS_OK;
    
}

fbe_status_t cache_to_mcr_transport_wait_for_configuration_service(void)
{
	fbe_u32_t                 timeout = 600; /* 1 min */
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_status_t              status;
    
	do
    {
		status = fbe_api_database_get_state(&state);

        if ( status != FBE_STATUS_OK )
        {
            return status;
        }

        if (state == FBE_DATABASE_STATE_READY )
        {
            return FBE_STATUS_OK;
        }

        if (timeout == 0 )
        {
            break;
        }
        
		fbe_api_sleep(100);
		timeout --;
	}
    while(1);
    
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s Configuration service is not READY\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
}
