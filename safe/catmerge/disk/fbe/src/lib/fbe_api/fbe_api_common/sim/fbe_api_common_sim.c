/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_common_sim.c
 ***************************************************************************
 *
 * @brief
 *   Simulation implementation for common API interface.
 *      
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_common_sim_interface
 *
 * @version
 *      10/10/08    sharel - Created
 *    
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_api_common.h"
#include "fbe_api_packet_queue.h"

/**************************************
				Local variables
**************************************/
static fbe_u32_t                        fbe_api_common_sim_init_reference_count = 0;

/*!********************************************************************* 
 * @struct io_and_control_entries_t 
 *  
 * @brief 
 *   This contains the data info for Sim io and control entries.
 *
 * @ingroup fbe_api_common_sim_interface
 * @ingroup io_and_control_entries
 **********************************************************************/
typedef struct io_and_control_entries_s{
    fbe_io_entry_function_t     io_entry;       /*!< IO Entry */
    fbe_service_control_entry_t control_entry;  /*!< Control Entry */
    fbe_u32_t                   magic_number;   /*!< Magic Number */
}io_and_control_entries_t;

/*!********************************************************************* 
 * @var lib_entries
 *  
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
fbe_api_libs_function_entries_t     lib_entries;

/*we have a table in the size of all possible packages and we keep an entry per packae*/
/*!********************************************************************* 
 * @var io_and_control_entries_table [FBE_PACKAGE_ID_LAST]
 *  
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
io_and_control_entries_t    io_and_control_entries_table [FBE_PACKAGE_ID_LAST];
static void *								cmm_chunk_addr[FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS];

/*******************************************
				Local functions
********************************************/
static fbe_status_t fbe_api_common_send_io_packet_to_simulation_pipe (fbe_packet_t *packet);
static fbe_status_t fbe_api_common_send_control_packet_to_simulation_pipe (fbe_packet_t *packet);
static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_sim(void);
static fbe_status_t fbe_api_common_init_contiguous_packet_queue_sim(void);

/*!***************************************************************
 * @fn fbe_api_common_init_sim()
 *****************************************************************
 * @brief
 *   simulation version of init
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_init_sim (void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    
    if (fbe_api_common_sim_init_reference_count> 0 ) {
        fbe_api_common_sim_init_reference_count++;
		return FBE_STATUS_OK; /* We already did init */
	}

    lib_entries.lib_control_entry = fbe_api_common_send_control_packet_to_simulation_pipe;
    lib_entries.lib_io_entry = fbe_api_common_send_io_packet_to_simulation_pipe;

	status  = fbe_api_common_init_contiguous_packet_queue_sim();
	if (status != FBE_STATUS_OK) {
		return status;
	}
    /* This is needed since we are using the transport run queues as part of the recieve thread.
     */
    status = fbe_transport_init();
    if (status != FBE_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init transport\n", __FUNCTION__); 
		return status;
	}
    fbe_api_common_sim_init_reference_count++;

    status  = fbe_api_notification_interface_init();
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
	}

    return status; 
}

/*!***************************************************************
 * @fn fbe_api_common_destroy_sim()
 *****************************************************************
 * @brief
 *   simulation version of destroy
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_destroy_sim ()
{
    fbe_status_t status = FBE_STATUS_OK;

    if(fbe_api_common_sim_init_reference_count> 1) {
        fbe_api_common_sim_init_reference_count --;
        return FBE_STATUS_OK;
    }

	fbe_api_common_destroy_job_notification();
    fbe_api_notification_interface_destroy();
           
    fbe_api_common_destroy_contiguous_packet_queue_sim();

    status = fbe_transport_destroy();
    if (status != FBE_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to destroy transport\n", __FUNCTION__); 
		return status;
	}

	fbe_api_common_sim_init_reference_count--;

	fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s destroy done\n", __FUNCTION__); 

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_simulation_pipe(
 *      fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending packet
 *
 * @param packet - pointer to the packet info
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_control_packet_to_simulation_pipe (fbe_packet_t *packet)
{  
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_sim_init_reference_count== 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    fbe_transport_get_package_id(packet, &package_id);

    if (io_and_control_entries_table[package_id].magic_number != IO_AND_CONTROL_ENTRY_MAGIC_NUMBER) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but control entry not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    /*we send the packet to the service manager of the simulator*/
    return io_and_control_entries_table[package_id].control_entry(packet);

}

/*!***************************************************************
 * @fn fbe_api_set_simulation_io_and_control_entries(
 *       fbe_package_id_t package_id,
 *       fbe_io_entry_function_t io_entry_function,
 *       fbe_service_control_entry_t control_entry)
 *****************************************************************
 * @brief
 *   set the control entry fro sending control packaset
 *
 * @param package_id - packet ID
 * @param io_entry_function - io entry function
 * @param control_entry - control entry 
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_simulation_io_and_control_entries (fbe_package_id_t package_id,
                                                                         fbe_io_entry_function_t io_entry_function,
                                                                         fbe_service_control_entry_t control_entry)
{
    if ((package_id <= FBE_PACKAGE_ID_INVALID) || (package_id >= FBE_PACKAGE_ID_LAST)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_and_control_entries_table[package_id].control_entry = control_entry;
    io_and_control_entries_table[package_id].io_entry = io_entry_function;
    io_and_control_entries_table[package_id].magic_number = IO_AND_CONTROL_ENTRY_MAGIC_NUMBER;

    /*for each entry we init here, we also set the one in the common*/
    fbe_api_common_set_function_entries(&lib_entries, package_id);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_common_send_io_packet_to_simulation_pipe(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending io packet
 *
 * @param packet - pointer to the packet info
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_io_packet_to_simulation_pipe (fbe_packet_t *packet)
{  
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_sim_init_reference_count== 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    fbe_transport_get_package_id(packet, &package_id);

    if (io_and_control_entries_table[package_id].magic_number != IO_AND_CONTROL_ENTRY_MAGIC_NUMBER) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but IO entry not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    /*we send the packet to the service manager of the simulator*/
    return io_and_control_entries_table[package_id].io_entry(packet);

}


static fbe_status_t fbe_api_common_init_contiguous_packet_queue_sim(void)
{
    fbe_u32_t 						count = 0;
	fbe_api_packet_q_element_t *	packet_q_element = NULL;
	fbe_status_t					status;
	fbe_u8_t * 						ptr = NULL;
    fbe_u32_t						chunk = 0;
	fbe_queue_head_t *				p_queue_head = fbe_api_common_get_contiguous_packet_q_head();
	fbe_spinlock_t *				spinlock = fbe_api_common_get_contiguous_packet_q_lock();
	fbe_queue_head_t *				o_queue_head = fbe_api_common_get_outstanding_contiguous_packet_q_head();

	fbe_queue_init(p_queue_head);
	fbe_spinlock_init(spinlock);
	fbe_queue_init(o_queue_head);

	/*we are going to allocate a 1MB chunk from CMM so let's make sure it will fit*/
	FBE_ASSERT_AT_COMPILE_TIME((sizeof(fbe_api_packet_q_element_t) * FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK) <= 1048576);

    for (;chunk < FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS; chunk++) {
	
		/* Allocate the memory */
		ptr = malloc(1048576);
		if (ptr == NULL) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s ecan't get memory\n", __FUNCTION__);
			return FBE_STATUS_GENERIC_FAILURE;
		}

		cmm_chunk_addr[chunk] = (void *)ptr;/*keep it so we can free it*/
	
		for (count = 0; count < FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK; count ++) {
			packet_q_element = (fbe_api_packet_q_element_t *)ptr;
			fbe_zero_memory(packet_q_element, sizeof(fbe_api_packet_q_element_t));
			fbe_queue_element_init(&packet_q_element->q_element);
			status = fbe_transport_initialize_packet(&packet_q_element->packet);
			if (status != FBE_STATUS_OK) {
				fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't inti packet\n", __FUNCTION__); 
				return FBE_STATUS_GENERIC_FAILURE;/*potenital leak but this is very bad anyway*/
			}
	
			fbe_queue_push(p_queue_head, &packet_q_element->q_element);
	
			ptr += sizeof(fbe_api_packet_q_element_t);
		}
	
	}
	
	return FBE_STATUS_OK;
	
}

static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_sim(void)
{
	
    fbe_u32_t  					chunk = 0;
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
	}
	
	fbe_spinlock_unlock(spinlock);
	fbe_spinlock_destroy(spinlock);

	/*free CMM memory*/
	for (;chunk < FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS; chunk++) {
		free(cmm_chunk_addr[chunk]);
	}
	
    return FBE_STATUS_OK;

}

