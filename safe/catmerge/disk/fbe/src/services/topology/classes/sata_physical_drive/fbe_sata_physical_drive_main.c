/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_sata_interface.h"


#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sata_physical_drive_private.h"
#include "fbe_scsi.h"

/* Class methods forward declaration */
fbe_status_t sata_physical_drive_load(void);
fbe_status_t sata_physical_drive_unload(void);
fbe_status_t fbe_sata_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

/* Export class methods  */
fbe_class_methods_t fbe_sata_physical_drive_class_methods = {FBE_CLASS_ID_SATA_PHYSICAL_DRIVE,
													sata_physical_drive_load,
													sata_physical_drive_unload,
													fbe_sata_physical_drive_create_object,
													fbe_sata_physical_drive_destroy_object,
													fbe_sata_physical_drive_control_entry,
													fbe_sata_physical_drive_event_entry,
													fbe_sata_physical_drive_io_entry,
													fbe_sata_physical_drive_monitor_entry};


fbe_block_transport_const_t fbe_sata_physical_drive_block_transport_const = {fbe_sata_physical_drive_block_transport_entry,
																			 fbe_base_physical_drive_process_block_transport_event,
																			 fbe_sata_physical_drive_io_entry,
                                                                             NULL, NULL};


/* Execution context operations. */
static fbe_status_t sata_physical_drive_send_read_capacity(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

static fbe_status_t sata_physical_drive_update_sata_element(fbe_sata_physical_drive_t * sata_physical_drive, fbe_packet_t * packet);

static fbe_status_t sata_physical_drive_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t 
sata_physical_drive_load(void)
{
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sata_physical_drive_t) < FBE_MEMORY_CHUNK_SIZE);
	return fbe_sata_physical_drive_monitor_load_verify();
}

fbe_status_t 
sata_physical_drive_unload(void)
{
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_sata_physical_drive_t * sata_physical_drive;
    fbe_status_t status;

    /* Call parent constructor */
    status = fbe_base_physical_drive_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sata_physical_drive, FBE_CLASS_ID_SATA_PHYSICAL_DRIVE);  

	fbe_sata_physical_drive_init(sata_physical_drive);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_sata_physical_drive_init(fbe_sata_physical_drive_t * sata_physical_drive)
{
	/* Initialize block transport server */
	
	fbe_base_physical_drive_set_block_transport_const((fbe_base_physical_drive_t *) sata_physical_drive,
													   &fbe_sata_physical_drive_block_transport_const);

	fbe_base_physical_drive_block_transport_enable_tags((fbe_base_physical_drive_t *) sata_physical_drive);

	/* This should be set in activete rotary after identify command
	fbe_base_physical_drive_set_outstanding_io_max((fbe_base_physical_drive_t *) sata_physical_drive,
													   FBE_SATA_PHYSICAL_DRIVE_OUTSTANDING_IO_MAX);
	*/

	sata_physical_drive->sata_physical_drive_attributes = 0;

	/*
	sata_physical_drive->tag_bitfield = 0;
	sata_physical_drive->max_queue_depth = 0;
	fbe_queue_init(&sata_physical_drive->tag_queue_head);
	fbe_spinlock_init(&sata_physical_drive->tag_lock);
	*/

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sata_physical_drive_t * sata_physical_drive;
	
	sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sata_physical_drive,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.\n", __FUNCTION__);

	/* Check parent edges */
	/* Cleanup */
	/*
	fbe_queue_destroy(&sata_physical_drive->tag_queue_head);
	fbe_spinlock_destroy(&sata_physical_drive->tag_lock);
	*/
	/* Call parent destructor */
	status = fbe_base_physical_drive_destroy_object(object_handle);
	return status;
}
#if 0
fbe_status_t 
fbe_sata_physical_drive_allocate_tag(fbe_sata_physical_drive_t * sata_physical_drive, fbe_u8_t * tag)
{
	fbe_u32_t i;

	*tag = 0;
	/* Check if the max_queue_depth is set */
	if(sata_physical_drive->max_queue_depth == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                "%s max_queue_depth is zero\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_lock(&sata_physical_drive->tag_lock);
	/* Look for the free tag */
	for(i = 0; i < sata_physical_drive->max_queue_depth; i++){
		if(!(sata_physical_drive->tag_bitfield & (0x1 << i))){ /* The bit is not set */
			sata_physical_drive->tag_bitfield |= (0x1 << i);     /* Set the bit */
			*tag = i;
			break;
		}
	}
	fbe_spinlock_unlock(&sata_physical_drive->tag_lock);

	/*Check if we were successful */
	if(i == sata_physical_drive->max_queue_depth){
        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Out of tags\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* I we are on A side we should use upper half of the tags */
	if(sata_physical_drive->sata_physical_drive_attributes & FBE_SATA_PHYSICAL_DRIVE_FLAG_ENCL_A){
		*tag += sata_physical_drive->max_queue_depth;
	}

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sata_physical_drive_release_tag(fbe_sata_physical_drive_t * sata_physical_drive, fbe_u8_t tag)
{
	fbe_spinlock_lock(&sata_physical_drive->tag_lock);
	if(sata_physical_drive->sata_physical_drive_attributes & FBE_SATA_PHYSICAL_DRIVE_FLAG_ENCL_A){
		tag -= sata_physical_drive->max_queue_depth;
	}
	/* Check if this tag was allocated */
	if(!(sata_physical_drive->tag_bitfield & (0x1 << tag))){ /* The bit is not set */
		fbe_spinlock_unlock(&sata_physical_drive->tag_lock);

        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                "%s tag %X was not allocated\n", __FUNCTION__, tag);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Clear the bit */
	sata_physical_drive->tag_bitfield &= ~(0x1 << tag);
	fbe_spinlock_unlock(&sata_physical_drive->tag_lock);

	return FBE_STATUS_OK;
}
#endif

#if 0
fbe_status_t 
fbe_sata_physical_drive_set_max_queue_depth(fbe_sata_physical_drive_t * sata_physical_drive, fbe_u32_t max_queue_depth)
{
	/* SATA TAG has only 5 bits in it and we can use only half of it */
	if(max_queue_depth > 16){ /* 0x1F / 2 */
        fbe_base_object_trace(  (fbe_base_object_t *)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                "%s max_queue_depth %X too big\n", __FUNCTION__, max_queue_depth);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sata_physical_drive->max_queue_depth = max_queue_depth;
	return FBE_STATUS_OK;
}
#endif
