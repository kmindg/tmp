#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_port.h"

#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "sas_pmc_port_private.h"
#include "fbe_cpd_shim.h"
#include "fbe_base_board.h"
#include "fbe_sas_port.h"
#include "fbe/fbe_stat_api.h"

/* Class methods forward declaration */
fbe_status_t fbe_sas_pmc_port_load(void);
fbe_status_t fbe_sas_pmc_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_pmc_port_class_methods = {FBE_CLASS_ID_SAS_PMC_PORT,
							        			      fbe_sas_pmc_port_load,
												      fbe_sas_pmc_port_unload,
												      fbe_sas_pmc_port_create_object,
												      fbe_sas_pmc_port_destroy_object,
												      fbe_sas_pmc_port_control_entry,
												      fbe_sas_pmc_port_event_entry,
                                                      fbe_sas_pmc_port_io_entry,
													  fbe_sas_pmc_port_monitor_entry};


/* Forward declaration */
static fbe_status_t sas_pmc_port_get_parent_sas_element(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);



fbe_status_t fbe_sas_pmc_port_load(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_PMC_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_pmc_port_t) < FBE_MEMORY_CHUNK_SIZE);

    status = fbe_cpd_shim_init();
    if ( FBE_STATUS_OK != status )
    {
        fbe_topology_class_trace(FBE_CLASS_ID_SAS_PMC_PORT,
                         FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "fbe_cpd_shim_init() failed with status 0x%x\n", status);
        return status;
    }

	return fbe_sas_pmc_port_monitor_load_verify();
}

fbe_status_t fbe_sas_pmc_port_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_PMC_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	fbe_cpd_shim_destroy();
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_pmc_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_pmc_port_t * sas_pmc_port;
	fbe_status_t status;	

	/* Call parent constructor */
	status = fbe_sas_port_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_pmc_port, FBE_CLASS_ID_SAS_PMC_PORT);	

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	fbe_sas_pmc_port_init(sas_pmc_port);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_pmc_port_init(fbe_sas_pmc_port_t * sas_pmc_port)
{	
	fbe_status_t status = FBE_STATUS_OK;	

	fbe_base_object_lock((fbe_base_object_t *)sas_pmc_port);
	sas_pmc_port->reset_complete_received = FALSE;
    sas_pmc_port->current_generation_code = 1;
	sas_pmc_port->device_table_ptr  = NULL;
	sas_pmc_port->da_child_index = FBE_SAS_PMC_INVALID_TABLE_INDEX;
    sas_pmc_port->da_enclosure_activating_fw = FBE_FALSE;
	sas_pmc_port->callback_registered = FBE_FALSE;
	fbe_spinlock_init(&sas_pmc_port->list_lock);

	/* Overwrite transport I/O entry */
	sas_pmc_port->sas_port.ssp_transport_server.base_transport_server.io_entry = fbe_sas_pmc_port_io_entry;
	sas_pmc_port->sas_port.stp_transport_server.base_transport_server.io_entry = fbe_sas_pmc_port_io_entry;
	fbe_base_object_unlock((fbe_base_object_t *)sas_pmc_port);    

	/* Don't check this in, for temp debugging only!
    fbe_lifecycle_set_trace_flags(&FBE_LIFECYCLE_CONST_DATA(sas_pmc_port),
                                           (fbe_base_object_t *)sas_pmc_port,										   
                                           (FBE_LIFECYCLE_TRACE_FLAGS_STATE_CHANGE |
										    FBE_LIFECYCLE_TRACE_FLAGS_CONDITIONS));
    */


	sas_pmc_port->io_counter = 2 * FBE_STAT_MAX_INTERVAL;
	sas_pmc_port->io_error_tag = 0;
	fbe_stat_init(&sas_pmc_port->port_stat, 10000, 100, 50);
	
	return status;
}


fbe_status_t 
fbe_sas_pmc_port_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sas_pmc_port_t *sas_pmc_port;

    sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);


	fbe_stat_destroy(&sas_pmc_port->port_stat);

    /* Release memory that was allocated for the table.*/
    if (sas_pmc_port->device_table_ptr  != NULL){
        fbe_memory_native_release(sas_pmc_port->device_table_ptr);
        sas_pmc_port->device_table_ptr = NULL;        
    }

    fbe_spinlock_destroy(&sas_pmc_port->list_lock);
	/* Call parent destructor */
	status = fbe_sas_port_destroy_object(object_handle);
	return status;
}
