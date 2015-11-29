/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_kernel_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the simulation implementation main entry points for the basic volume driver object.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_bvd_interface_private.h"
#include "simulation/cache_to_mcr_transport.h"

/*****************************************
 * LOCAL Members
 *****************************************/
typedef struct fbe_test_lun_identity_s {
    fbe_char_t  device_path[CACHE_TO_MCR_PATH_MAX * 2]; // path to device /dev/CLARiiONDiskN
    PVOID       block_edge;                             // package identifier used to forward IO
} fbe_test_lun_identity_t, *fbe_test_lun_identity_p;

static fbe_test_lun_identity_t * 	fbe_bvd_sim_simulated_lu[CACHE_TO_MCR_MAX_LU];/*should be enouhg for now*/
static fbe_object_id_t			 	fbe_bvd_sim_bvd_object_id;
static fbe_spinlock_t				table_lock;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_bvd_sim_export_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *	os_device_info, csx_cpuchar_t dev_name);
static fbe_status_t fbe_bvd_sim_unexport_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *	os_device_info);


/************************************************************************************************************************************/													

fbe_status_t fbe_bvd_interface_load(void)
{
	fbe_bvd_interface_monitor_load_verify();
    return FBE_STATUS_OK;
}

fbe_status_t fbe_bvd_interface_unload(void)
{
	
    return FBE_STATUS_OK;
}


fbe_status_t fbe_bvd_interface_init (fbe_bvd_interface_t *bvd_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /*clear the table*/
	fbe_zero_memory(fbe_bvd_sim_simulated_lu, sizeof(*fbe_bvd_sim_simulated_lu) * CACHE_TO_MCR_MAX_LU);
	fbe_base_object_get_object_id((fbe_base_object_t *)bvd_object_p, &fbe_bvd_sim_bvd_object_id);

    /* Set the default offset */
    status = fbe_base_config_set_default_offset((fbe_base_config_t *) bvd_object_p);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

	fbe_spinlock_init(&table_lock);

	return FBE_STATUS_OK;
}

fbe_status_t fbe_bvd_interface_export_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *	os_device_info)
{
	return fbe_bvd_sim_export_os_device(bvd_object_p, os_device_info, NULL);

}

fbe_status_t fbe_bvd_interface_unexport_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
	return fbe_bvd_sim_unexport_os_device(bvd_object_p, os_device_info);
}

fbe_status_t fbe_bvd_interface_export_os_special_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info,
                                                        csx_puchar_t dev_name, csx_puchar_t link_name)
{
	return fbe_bvd_sim_export_os_device(bvd_object_p, os_device_info, dev_name);
    
}

fbe_status_t fbe_bvd_interface_unexport_os_special_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
	return fbe_bvd_sim_unexport_os_device(bvd_object_p, os_device_info);
}


static fbe_status_t fbe_bvd_sim_export_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *	os_device_info, csx_cpuchar_t dev_name)
{
	fbe_u32_t					entry = 0;
    fbe_u32_t                   lun_num = (fbe_u32_t)os_device_info->lun_number;

	fbe_spinlock_lock(&table_lock);
	/*search for a free entry*/
	for (entry = 0; entry < CACHE_TO_MCR_MAX_LU; entry ++) {
		if (fbe_bvd_sim_simulated_lu[entry] == NULL) {
			break;
		}
	}
	
	if (entry == CACHE_TO_MCR_MAX_LU) {
		fbe_spinlock_unlock(&table_lock);

		fbe_base_object_trace((fbe_base_object_t *)bvd_object_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s no more entries in LU table, limited to %d\n", __FUNCTION__, CACHE_TO_MCR_MAX_LU);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*found one let's give it memory and fill the data*/
	fbe_bvd_sim_simulated_lu[entry] = (fbe_test_lun_identity_t *)fbe_memory_native_allocate(sizeof(fbe_test_lun_identity_t));

	fbe_spinlock_unlock(&table_lock);

	/*we are in simulation, let's skip the NULL check, we should not fail here :),
	let's copy all the data we know. Once the LU will be created in cache, it will send us a usurper command
	via the FBE API transport to get this information and we will use the pointer at IO time.
	swprintf is not as safe, but still, we are in simulation only...*/
	if (dev_name == NULL) {
        csx_p_sprintf(fbe_bvd_sim_simulated_lu[entry]->device_path,"\\Device\\CLARiiONdisk%d", lun_num);
    }else{
        csx_p_sprintf(fbe_bvd_sim_simulated_lu[entry]->device_path, "%s", dev_name);
    }

	fbe_bvd_sim_simulated_lu[entry]->block_edge = (void *)&os_device_info->block_edge;

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_bvd_sim_unexport_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *	os_device_info)
{
	fbe_u32_t					entry = 0;
	fbe_u32_t                   lun_num = (fbe_u32_t)os_device_info->lun_number;

	fbe_spinlock_lock(&table_lock);
	/*look for the entry in the table and remove it*/
	for (entry = 0; entry < CACHE_TO_MCR_MAX_LU; entry ++) {
		if ((fbe_bvd_sim_simulated_lu[entry] != NULL) && 
			((fbe_block_edge_t *)fbe_bvd_sim_simulated_lu[entry]->block_edge == &os_device_info->block_edge)) {

			fbe_memory_native_release(fbe_bvd_sim_simulated_lu[entry]);
			fbe_bvd_sim_simulated_lu[entry] = NULL;
			break;
		}
	}
	fbe_spinlock_unlock(&table_lock);

	if (entry == CACHE_TO_MCR_MAX_LU) {
		fbe_base_object_trace((fbe_base_object_t *)bvd_object_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s did not find lun #%d to remove\n", __FUNCTION__, lun_num);

		return FBE_STATUS_GENERIC_FAILURE;
	}else{
		return FBE_STATUS_OK;
	}

}

fbe_status_t fbe_bvd_interface_map_os_device_name_to_block_edge(const fbe_u8_t *name, void **block_edge_ptr, fbe_object_id_t *bvd_object_id)
{
    fbe_u32_t 			entry;
	fbe_u8_t 			exported_lu[64];
	const fbe_u8_t *	user_lu_name = name;
	fbe_bool_t			match = FBE_TRUE;
	fbe_u32_t			c = 0;

    /*we have to go throught he whole array since me might have holes in the middle when devices are unbound
	the exported devices by MCR are Wchar and the input from the user is not so we have to do some tricks*/
	fbe_spinlock_lock(&table_lock);
	for (entry = 0; entry < CACHE_TO_MCR_MAX_LU; entry ++) {
		if (fbe_bvd_sim_simulated_lu[entry] == NULL) {
			continue;
		}

		match = FBE_TRUE;
		fbe_copy_memory(exported_lu, fbe_bvd_sim_simulated_lu[entry]->device_path, 64);
        user_lu_name = name;
		c = 0;
		while ((user_lu_name != NULL) && (*user_lu_name != 0)) {
			/*we check both upper and lowercase versions*/
			if ((exported_lu[c] != *user_lu_name) && ((exported_lu[c] - 32)!= *user_lu_name)) {
				match = FBE_FALSE;
				break;
			}
			c += 2;/*exported LU is wchar so we need to skip one more byte*/
			user_lu_name++;
		}

		if (FBE_IS_TRUE(match)) {
			/*found it*/
			*block_edge_ptr = (void *)fbe_bvd_sim_simulated_lu[entry]->block_edge;
			*bvd_object_id = fbe_bvd_sim_bvd_object_id;
			fbe_spinlock_unlock(&table_lock);
			return FBE_STATUS_OK;
		}
	}

	fbe_spinlock_unlock(&table_lock);
	return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_bvd_interface_destroy (fbe_bvd_interface_t *bvd_object_p)
{
	fbe_u32_t entry;

	fbe_spinlock_lock(&table_lock);
    /* Destroy any global data.*/
	for (entry = 0; entry < CACHE_TO_MCR_MAX_LU; entry ++) {
		if (fbe_bvd_sim_simulated_lu[entry] != NULL) {
			fbe_base_object_trace((fbe_base_object_t *)bvd_object_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "someone forgot to unbind LU %s\n", fbe_bvd_sim_simulated_lu[entry]->device_path);

			fbe_memory_native_release(fbe_bvd_sim_simulated_lu[entry]);
		}
	}

	fbe_spinlock_unlock(&table_lock);

	fbe_spinlock_destroy(&table_lock);

	return FBE_STATUS_OK;
}

fbe_status_t fbe_bvd_interface_export_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    return FBE_STATUS_OK;
}
fbe_status_t fbe_bvd_interface_unexport_lun(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info_p)
{
    return FBE_STATUS_OK;
}
