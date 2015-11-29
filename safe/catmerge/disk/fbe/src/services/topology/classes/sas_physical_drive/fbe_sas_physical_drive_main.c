#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sas_physical_drive_private.h"
#include "fbe_scsi.h"

#include "fbe_perfstats.h"

/* Class methods forward declaration */
fbe_status_t sas_physical_drive_load(void);
fbe_status_t sas_physical_drive_unload(void);
fbe_status_t fbe_sas_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

/* Export class methods  */
fbe_class_methods_t fbe_sas_physical_drive_class_methods = {FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,
                                                    sas_physical_drive_load,
                                                    sas_physical_drive_unload,
                                                    fbe_sas_physical_drive_create_object,
                                                    fbe_sas_physical_drive_destroy_object,
                                                    fbe_sas_physical_drive_control_entry,
                                                    fbe_sas_physical_drive_event_entry,
                                                    fbe_sas_physical_drive_io_entry,
                                                    fbe_sas_physical_drive_monitor_entry};


fbe_block_transport_const_t fbe_sas_physical_drive_block_transport_const = {fbe_sas_physical_drive_block_transport_entry,
																			fbe_base_physical_drive_process_block_transport_event,
																			fbe_sas_physical_drive_io_entry,
                                                                            NULL, NULL};


/* Execution context operations. */
static fbe_status_t sas_physical_drive_send_read_capacity(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);

static fbe_status_t sas_physical_drive_update_sas_element(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);

static fbe_status_t sas_physical_drive_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t 
sas_physical_drive_load(void)
{
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_physical_drive_t) < FBE_MEMORY_CHUNK_SIZE);
    /* KvTrace("%s object size %d\n", __FUNCTION__, sizeof(fbe_sas_physical_drive_t)); */

    return fbe_sas_physical_drive_monitor_load_verify();
}

fbe_status_t 
sas_physical_drive_unload(void)
{
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_sas_physical_drive_t * sas_physical_drive;
    fbe_status_t status;

    /* Call parent constructor */
    status = fbe_base_physical_drive_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sas_physical_drive, FBE_CLASS_ID_SAS_PHYSICAL_DRIVE);  

    fbe_sas_physical_drive_init(sas_physical_drive);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_sas_physical_drive_init(fbe_sas_physical_drive_t * sas_physical_drive)
{
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_object_id_t my_object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t current_time;

    /* Initialize block transport server */
    
    fbe_base_physical_drive_set_block_transport_const((fbe_base_physical_drive_t *) sas_physical_drive,
                                                       &fbe_sas_physical_drive_block_transport_const);

    /* For non-vault drives, by default qdepth is 22  */
    fbe_base_physical_drive_set_outstanding_io_max((fbe_base_physical_drive_t *) sas_physical_drive,
        FBE_SAS_PHYSICAL_DRIVE_OUTSTANDING_IO_MAX);

	/* Set I/O throttling */
	/* This code is under construction and evaluation by Tom Dibb */
	/* Once tuning will be done appropriate default value will be  created */
	/* For now we will limit I/O throttling to 10 MB - half of what it was before */
	//fbe_base_physical_drive_set_io_throttle_max((fbe_base_physical_drive_t *) sas_physical_drive, 0x800 * 10); /* 10 MB */

	fbe_base_physical_drive_set_stack_limit(base_physical_drive);

    /*it should be really learned from the drive itself*/
    info_ptr->drive_qdepth = FBE_SAS_PHYSICAL_DRIVE_OUTSTANDING_IO_MAX;

    fbe_base_physical_drive_set_default_timeout((fbe_base_physical_drive_t *) sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT); 

    /* Initialize drive info block */
    fbe_zero_memory(&sas_physical_drive->sas_drive_info, sizeof(fbe_sas_physical_drive_info_block_t));
    sas_physical_drive->sas_drive_info.use_additional_override_tbl = FBE_FALSE;

    sas_physical_drive->sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_OK;
    sas_physical_drive->sas_drive_info.sanitize_percent = FBE_DRIVE_MAX_SANITIZATION_PERCENT;
    

    /* Initialize virtual functions.  Subclasses can override these. */
    sas_physical_drive->get_vendor_table = fbe_sas_physical_drive_get_vendor_table;   
    sas_physical_drive->get_port_error = sas_physical_drive_get_port_error;

    /* Initialize performance statistics */
    status = fbe_perfstats_is_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                             &sas_physical_drive->b_perf_stats_enabled);
    if (status != FBE_STATUS_OK) {
        sas_physical_drive->b_perf_stats_enabled = FBE_FALSE;
    }
 
    fbe_base_object_get_object_id((fbe_base_object_t *)sas_physical_drive, &my_object_id);
    status = fbe_perfstats_get_system_memory_for_object_id(my_object_id,
                                                           FBE_PACKAGE_ID_PHYSICAL,
                                                           (void**)&sas_physical_drive->performance_stats.counter_ptr.pdo_counters);    

    if (status == FBE_STATUS_OK && sas_physical_drive->performance_stats.counter_ptr.pdo_counters)
    {
        fbe_zero_memory(sas_physical_drive->performance_stats.counter_ptr.pdo_counters, 
            sizeof(*sas_physical_drive->performance_stats.counter_ptr.pdo_counters));
        
        fbe_base_physical_drive_reset_statistics(base_physical_drive);
        current_time =  fbe_get_time_in_us();
        sas_physical_drive->performance_stats.counter_ptr.pdo_counters->object_id = my_object_id;
        sas_physical_drive->performance_stats.counter_ptr.pdo_counters->timestamp = current_time;     
    }

    /* Initialize enhanced queuing timer */
    fbe_sas_physical_drive_init_enhanced_queuing_timer(sas_physical_drive);    


    return status;
}

fbe_status_t 
fbe_sas_physical_drive_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sas_physical_drive_t * sas_physical_drive;
    
    sas_physical_drive = (fbe_sas_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry.\n", __FUNCTION__);
    /* Cleanup */ 
    /* fbe_transport_release_buffer(sas_physical_drive->sas_drive_info); */
    sas_physical_drive->b_perf_stats_enabled = FBE_FALSE;
    if (sas_physical_drive->performance_stats.counter_ptr.pdo_counters) 
    {
        fbe_perfstats_delete_system_memory_for_object_id(sas_physical_drive->performance_stats.counter_ptr.pdo_counters->object_id,
                                                         FBE_PACKAGE_ID_PHYSICAL);
    }

    /* Call parent destructor */
    status = fbe_base_physical_drive_destroy_object(object_handle);
    return status;
}

fbe_status_t 
fbe_sas_physical_drive_detach_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_ssp_transport_control_detach_edge_t ssp_detach_edge; /* sent to the server object */

    fbe_payload_ex_t * payload = NULL;
    fbe_path_state_t path_state;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_status_t status;
    fbe_semaphore_t sem;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);


    /* Remove the edge hook if any.*/
    fbe_ssp_transport_edge_remove_hook(&sas_physical_drive->ssp_edge, packet);

    status = fbe_ssp_transport_get_path_state(&sas_physical_drive->ssp_edge, &path_state);
    /* If we don't have an edge to detach, complete the packet and return Good status. */
    if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s SSP edge is Invalid. No need to detach it.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    ssp_detach_edge.ssp_edge = &sas_physical_drive->ssp_edge;

    /* Insert the edge. */
    fbe_semaphore_init(&sem, 0, 1);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if(control_operation == NULL) {    
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Failed to allocate control operation\n",__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;    
    }


    fbe_payload_control_build_operation(control_operation,  // control operation
                                    FBE_SSP_TRANSPORT_CONTROL_CODE_DETACH_EDGE,  // opcode
                                    &ssp_detach_edge, // buffer
                                    sizeof(fbe_ssp_transport_control_detach_edge_t));   // buffer_length 

    status = fbe_transport_set_completion_function(packet, sas_physical_drive_detach_edge_completion, &sem);

    fbe_ssp_transport_send_control_packet(&sas_physical_drive->ssp_edge, packet);

    fbe_semaphore_wait(&sem, NULL); /* We have to wait because ssp_detach_edge is on the stack */
    fbe_semaphore_destroy(&sem);


    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_physical_drive_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload); 
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_physical_drive_set_dc_rcv_diag_enabled(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t flag)
{
    sas_physical_drive->sas_drive_info.dc_rcv_diag_enabled = flag;
    return FBE_STATUS_OK;
}


