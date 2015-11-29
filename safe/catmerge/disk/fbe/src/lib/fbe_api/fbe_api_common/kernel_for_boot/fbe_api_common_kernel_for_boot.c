/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_common_kernel_for_boot.c
 ***************************************************************************
 *
 * @brief
 *   Kernel implementation for common API interface which is linked with boot time drivers
 *      
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_common_kernel_interface
 *
 * @version
 *      3/3/11    sharel - Created
 *    
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_neit_package.h"
#include "fbe_api_packet_queue.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_kms.h"

/**************************************
				Local variables
**************************************/
/*!********************************************************************* 
 * @struct fbe_api_kerenl_package_connection_info_t 
 *  
 * @brief 
 *   This contains the data info for Kernel package connection structure.
 *
 * @ingroup fbe_api_common_kernel_interface
 * @ingroup fbe_api_kerenl_package_connection_info
 **********************************************************************/
typedef struct fbe_api_kerenl_package_connection_info_s{
    fbe_char_t *                    driver_link_name;       /*!< Driver link name */
    fbe_u32_t                       get_control_entry_iocl; /*!< get control entry IOCTL */
    fbe_u32_t                       get_io_entry_iocl;      /*!< get IO entry IOCTL */
    fbe_service_control_entry_t     control_entry;          /*!< control entry */
    fbe_service_io_entry_t          io_entry;               /*!< IO entry */
    PEMCPAL_FILE_OBJECT             devfile_object;         /*!< Device File Object */
}fbe_api_kerenl_package_connection_info_t;

/*this is where we set up the package names to start with, and as we go we would also add the handles*/
/* NOTE: This table should be synced with fbe_package_id_t definition. 
         Please add a new entry for each new item added to fbe_package_id_t.
 */
static fbe_api_kerenl_package_connection_info_t packages_handles[FBE_PACKAGE_ID_LAST + 1] = 
{
    /* FBE_PACKAGE_ID_INVALID */
    {NULL, 0, 0, NULL, NULL, NULL},

    /* FBE_PACKAGE_ID_PHYSICAL */
    {FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR,
     FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY_IOCTL,
     FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY_IOCTL,
     fbe_service_manager_send_control_packet, NULL, NULL},

     /* FBE_PACKAGE_ID_NEIT */
    {FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR,
     FBE_NEIT_GET_MGMT_ENTRY_IOCTL,
     0,
     fbe_service_manager_send_control_packet, NULL, NULL},

    /* FBE_PACKAGE_ID_SEP_0 */
    {FBE_SEP_DEVICE_NAME_CHAR,
     FBE_SEP_GET_MGMT_ENTRY_IOCTL,
     FBE_SEP_GET_IO_ENTRY_IOCTL,
     fbe_service_manager_send_control_packet, NULL, NULL},

	/* FBE_PACKAGE_ID_ESP */
    {FBE_ESP_DEVICE_NAME_CHAR,
     FBE_ESP_GET_MGMT_ENTRY_IOCTL,
     0,
     fbe_service_manager_send_control_packet, NULL, NULL},

	/* FBE_PACKAGE_ID_KMS */
    {FBE_KMS_DEVICE_NAME_CHAR,
     FBE_KMS_GET_MGMT_ENTRY_IOCTL,
     0,
     fbe_service_manager_send_control_packet, NULL, NULL},

    /* FBE_PACKAGE_ID_LAST */
    {NULL, 0, 0, NULL, NULL, NULL},

};

static fbe_u32_t                        fbe_api_common_kernel_init_reference_count = 0;

/*******************************************
				Local functions
********************************************/
static fbe_status_t fbe_api_common_send_control_packet_between_drivers (fbe_packet_t *packet);
static fbe_status_t fbe_api_common_send_io_packet_between_drivers (fbe_packet_t *packet);
static fbe_status_t connect_to_control_and_io_entries(fbe_package_id_t package_id);
static void disconnect_control_and_io_entries(fbe_package_id_t package_id);
static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_kernel_for_boot(void);
static fbe_status_t fbe_api_common_init_contiguous_packet_queue_kernel_for_boot(void);
/*!***************************************************************
 * @fn fbe_api_common_init_kernel()
 *****************************************************************
 * @brief
 *   simulation version of init. 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_init_kernel ()
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_libs_function_entries_t     entries;
    fbe_package_id_t                    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_kernel_init_reference_count > 0 ) {
        fbe_api_common_kernel_init_reference_count++;
        return FBE_STATUS_OK; /* We already did init */
    }

    fbe_api_common_kernel_init_reference_count++;

    /*connect to all possible packages*/
    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id ++) {
        if (packages_handles[package_id].driver_link_name != NULL) {
            connect_to_control_and_io_entries(package_id);
        }
    }
    

    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
        if (packages_handles[package_id].driver_link_name != NULL) {
            entries.lib_control_entry = fbe_api_common_send_control_packet_between_drivers;
            entries.lib_io_entry = fbe_api_common_send_io_packet_between_drivers;
            
            status = fbe_api_common_set_function_entries(&entries, package_id);
            if (status !=FBE_STATUS_OK) {
                return status;
            }
        }
    }

    status  = fbe_api_notification_interface_init();
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
	}

    status  = fbe_api_common_init_contiguous_packet_queue_kernel_for_boot();
	if (status != FBE_STATUS_OK) {
		return status;
	}
	
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s initialized\n", __FUNCTION__); 

	return status; 
}

/*!***************************************************************
 * @fn fbe_api_common_destroy_kernel()
 *****************************************************************
 * @brief
 *   simulation version of destroy. 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_destroy_kernel (void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_package_id_t   	package_id = FBE_PACKAGE_ID_INVALID;

    if(fbe_api_common_kernel_init_reference_count > 1) {
        fbe_api_common_kernel_init_reference_count --;
        return FBE_STATUS_OK;
    }
    

	fbe_api_common_destroy_job_notification();
    fbe_api_notification_interface_destroy();

	for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id ++) {
        if (packages_handles[package_id].driver_link_name != NULL) {
            disconnect_control_and_io_entries(package_id);
        }
    }

    fbe_api_common_kernel_init_reference_count --;
    fbe_api_common_destroy_contiguous_packet_queue_kernel_for_boot();


	fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s destroy done\n", __FUNCTION__); 

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_between_drivers(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending packet. 
 *
 * @param packet - pointer to the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_control_packet_between_drivers (fbe_packet_t *packet)
{  
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_kernel_init_reference_count == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__); 
		fbe_transport_set_status(packet,FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    fbe_transport_get_package_id(packet, &package_id);
    if (packages_handles[package_id].control_entry != NULL) {
        return packages_handles[package_id].control_entry(packet);
    }else{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s control entry uninitialized for package:%d\n", __FUNCTION__, package_id); 
		fbe_transport_set_status(packet,FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

}

/*!***************************************************************
 * @fn fbe_api_common_send_io_packet_between_drivers(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending io packet. 
 *
 * @param packet - pointer to the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_io_packet_between_drivers (fbe_packet_t *packet)
{  
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_kernel_init_reference_count == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    fbe_transport_get_package_id(packet, &package_id);
    if (packages_handles[package_id].io_entry != NULL) {
        return packages_handles[package_id].io_entry(packet);
    }else{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s IO entry uninitialized for package:%d\n", __FUNCTION__, package_id); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

}

/*!***************************************************************
 * @fn connect_to_control_and_io_entries(
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   call package to initialize the io and control entry. 
 *
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  7/20/09: sharel created
 *
 ****************************************************************/
static fbe_status_t
connect_to_control_and_io_entries(fbe_package_id_t package_id)
{
    fbe_package_get_control_entry_t	get_control_entry;
    fbe_package_get_io_entry_t		get_io_entry;
    EMCPAL_STATUS			status;
    PEMCPAL_FILE_OBJECT			file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
			      packages_handles[package_id].driver_link_name,
			      &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_FLARE_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: EmcpalDeviceOpen failed for %s\n",
			       __FUNCTION__,
			       packages_handles[package_id].driver_link_name); 
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    packages_handles[package_id].devfile_object = file_object;

    if (0 != packages_handles[package_id].get_io_entry_iocl) {
        /* We may want to specify timeout to make sure SEP do not hung on us */
        status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
				   file_object,
    				   packages_handles[package_id].get_io_entry_iocl,
				   &get_io_entry,
				   sizeof (fbe_package_get_io_entry_t),
				   &get_io_entry,
				   sizeof (fbe_package_get_io_entry_t),
				   NULL);

        if (!EMCPAL_IS_SUCCESS(status)) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
			   "%s io request fail with status %x\n", __FUNCTION__,
			   status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /*now that we got back, we save for future use the ioctl entry*/
        packages_handles[package_id].io_entry = get_io_entry.io_entry;

        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
		      "%s for package:%d, package_io_entry:%p\n", __FUNCTION__,
		      package_id, (void*)packages_handles[package_id].io_entry);
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
			       file_object,
    			       packages_handles[package_id].get_control_entry_iocl,
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       NULL);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s io request fail with status %x\n", __FUNCTION__,
		      status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the ioctl entry*/
    packages_handles[package_id].control_entry = get_control_entry.control_entry;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
		  "%s for package:%d, package_control_entry:%p\n", __FUNCTION__,
		  package_id, (void*)packages_handles[package_id].control_entry); 
    return FBE_STATUS_OK;
}

static void disconnect_control_and_io_entries(fbe_package_id_t package_id)
{
	PEMCPAL_FILE_OBJECT *ppFileObject;

	ppFileObject = &packages_handles[package_id].devfile_object;

	if (NULL != *ppFileObject) {
		if (EMCPAL_IS_SUCCESS(EmcpalDeviceClose(*ppFileObject))) {
			*ppFileObject = NULL;
		}
	}
	return ;
}

static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_kernel_for_boot(void)
{
	fbe_u32_t 					count = 0;
	fbe_api_packet_q_element_t *packet_q_element = NULL;
	fbe_queue_head_t *			p_queue_head = fbe_api_common_get_contiguous_packet_q_head();
	fbe_spinlock_t *			spinlock = fbe_api_common_get_contiguous_packet_q_lock();
	fbe_queue_head_t *			o_queue_head = fbe_api_common_get_outstanding_contiguous_packet_q_head();

	fbe_spinlock_lock(spinlock);

	/*do we have any outstanding ones ?*/
	while (!fbe_queue_is_empty(o_queue_head) && count < 10) {
		/*let's wait for completion of a while*/
		fbe_spinlock_unlock(spinlock);
		fbe_api_sleep(500);
		count++;
		fbe_spinlock_lock(spinlock);
	}

    if (!fbe_queue_is_empty(o_queue_head)) {
		/*there will be a memory leak eventually but we can't stop here forever*/
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:outstanding packets did not complete in 5 sec\n", __FUNCTION__); 
	}

	/*remove pre-allocated queued packets*/
	while(!fbe_queue_is_empty(p_queue_head)) {
        packet_q_element = (fbe_api_packet_q_element_t *)fbe_queue_pop(p_queue_head);
        packet_q_element->packet.magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;
		fbe_transport_destroy_packet(&packet_q_element->packet);
		fbe_api_free_contiguous_memory(packet_q_element);
	}
	
	fbe_spinlock_unlock(spinlock);
	fbe_spinlock_destroy(spinlock);

	return FBE_STATUS_OK;

}

static fbe_status_t fbe_api_common_init_contiguous_packet_queue_kernel_for_boot()
{
	fbe_u32_t 						count = 0;
	fbe_api_packet_q_element_t *	packet_q_element = NULL;
	fbe_status_t					status;
	fbe_queue_head_t *				p_queue_head = fbe_api_common_get_contiguous_packet_q_head();
	fbe_spinlock_t *				spinlock = fbe_api_common_get_contiguous_packet_q_lock();
	fbe_queue_head_t *				o_queue_head = fbe_api_common_get_outstanding_contiguous_packet_q_head();

	fbe_queue_init(p_queue_head);
	fbe_spinlock_init(spinlock);
	fbe_queue_init(o_queue_head);
    
	for (count = 0; count < FBE_API_PRE_ALLOCATED_OS_PACKETS; count ++) {
		packet_q_element = (fbe_api_packet_q_element_t *)fbe_api_allocate_contiguous_memory(sizeof(fbe_api_packet_q_element_t));
		if (packet_q_element == NULL) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't allocate packet\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;/*potenital leak but this is very bad anyway*/

		}

		fbe_zero_memory(packet_q_element, sizeof(fbe_api_packet_q_element_t));
		fbe_queue_element_init(&packet_q_element->q_element);
		status = fbe_transport_initialize_packet(&packet_q_element->packet);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't init packet\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;/*potenital leak but this is very bad anyway*/
		}

		fbe_queue_push(p_queue_head, &packet_q_element->q_element);
	}
    
	return FBE_STATUS_OK;

}
