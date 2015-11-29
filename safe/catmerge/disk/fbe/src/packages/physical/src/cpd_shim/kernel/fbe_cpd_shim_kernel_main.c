#include "generics.h"
#include "fbe/fbe_winddk.h"
#include "fbe_cpd_shim.h"
#include "fbe_cpd_shim_private.h"
#include "fbe/fbe_transport.h"
 
//#include "storport.h"
#include "scsiclass.h"
//#include "ntddscsi.h"
#include "cpd_interface.h" /* CPD_CONFIG definition, it also includes icdmini.h for NTBE_SRB and NTBE_SGL */
#include "speeds.h"

#include "cpd_shim_kernel_private.h"
#include "fbe/fbe_library_interface.h"

#include "spid_kernel_interface.h"
#include "specl_interface.h"

static fbe_thread_t								cpd_shim_ioctl_process_io_thread_handle;
static fbe_rendezvous_event_t					cpd_shim_ioctl_process_io_event;
static cpd_shim_ioctl_process_io_thread_flag_t	cpd_shim_ioctl_process_io_thread_flag;

static fbe_spinlock_t  cpd_port_array_lock;
static cpd_port_t cpd_port_array[FBE_CPD_SHIM_MAX_PORTS];

static fbe_status_t cpd_shim_send_process_io(cpd_port_t *cpd_port);
static void cpd_shim_ioctl_process_io_thread_func(void * context);
static BOOLEAN fbe_cpd_shim_callback (IN void * context, IN CPD_EVENT_INFO *pInfo);
static fbe_status_t fbe_cpd_shim_get_cpd_capabilities(IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject, fbe_u32_t   io_portal_number,
								  OUT CPD_CAPABILITIES *pCapabilitiesInfo);
static fbe_status_t fbe_cpd_shim_send_cpd_ext_ioctl(IN PEMCPAL_DEVICE_OBJECT  pPortDeviceObject,/*IN fbe_u32_t   io_portal_number,*/
								IN VOID           *pIoCtl,IN ULONG          Size,IN BOOLEAN        InternalDeviceIoctl);

static fbe_status_t fbe_cpd_shim_send_register_callback (fbe_u32_t  port_handle,fbe_u32_t cpd_shim_registration_flags);
static fbe_status_t fbe_cpd_shim_send_unregister_callback(fbe_u32_t  port_handle);
static fbe_status_t fbe_cpd_shim_get_cpd_config (IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject, IN fbe_u32_t   io_portal_number,
							 OUT CPD_CONFIG *pConfigInfo);
static fbe_status_t cpd_shim_get_hardware_info(IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject,
                               fbe_cpd_shim_hardware_info_t *hdw_info);

static fbe_status_t fbe_cpd_shim_get_flare_bus(PUCHAR BufferPtr, fbe_u32_t BufferSize, fbe_cpd_shim_port_role_t *port_role, fbe_u32_t *flare_bus);
static fbe_status_t fbe_cpd_shim_get_device_type(fbe_u32_t cpd_login_flags, fbe_cpd_shim_discovery_device_type_t *cpd_device_type);

static VOID  fbe_cpd_shim_device_event_dpc_function(PVOID Context);
static VOID  fbe_cpd_shim_port_event_dpc_function(PVOID Context);

EMCPAL_DECLARE_DPC_FUNC(fbe_cpd_shim_device_event_dpc_function_base)
EMCPAL_DECLARE_DPC_FUNC(fbe_cpd_shim_port_event_dpc_function_base)

static void  fbe_cpd_shim_cdb_io_miniport_callback(void *callback_context1, void *callback_context2);
#ifdef ALAMOSA_WINDOWS_ENV
static VOID  fbe_cpd_shim_cdb_io_completion_dpc_function(IN struct _KDPC  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 );
#else
static VOID  fbe_cpd_shim_cdb_io_completion_dpc_function(IN PVOID  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 );
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

static void  fbe_cpd_shim_fis_io_miniport_callback(void *callback_context1, void *callback_context2);
#ifdef ALAMOSA_WINDOWS_ENV
static VOID  fbe_cpd_shim_fis_io_completion_dpc_function(IN struct _KDPC  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 );
#else
static VOID  fbe_cpd_shim_fis_io_completion_dpc_function(IN PVOID Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 );
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

static VOID  fbe_cpd_shim_fis_io_completion_callback_function(IN PVOID  DeferredContext,IN PVOID  SystemArgument1);
static VOID  fbe_cpd_shim_cdb_io_completion_callback_function(IN PVOID  DeferredContext,IN PVOID  SystemArgument1);


static fbe_status_t fbe_cpd_shim_initialize_port_event_queue(cpd_port_t *cpd_port);
static fbe_status_t fbe_cpd_shim_add_to_port_event_queue(cpd_port_t *cpd_port,fbe_cpd_shim_callback_info_t *callback_info);
static fbe_status_t fbe_cpd_shim_get_next_port_event_from_queue(cpd_port_t *cpd_port,fbe_cpd_shim_callback_info_t *callback_info);
static fbe_status_t fbe_cpd_shim_schedule_port_event_dpc(cpd_port_t * cpd_port);
static fbe_status_t fbe_cpd_shim_schedule_device_event_dpc(cpd_port_t * cpd_port);
static fbe_status_t fbe_cpd_shim_get_login_reason(CPD_EVENT_INFO *pInfo, fbe_cpd_shim_device_table_entry_t	*current_device_entry);
static fbe_status_t cpd_shim_map_cpd_sfp_media_interface_information(CPD_MEDIA_INTERFACE_INFO *cpd_media_interface_info,
                                                 fbe_cpd_shim_sfp_media_interface_info_t *fbe_media_interface_info);

static SGL_PADDR cpd_shim_get_phys_addr(void * vaddr, void * xlat_context);

static fbe_bool_t cpd_shim_async_io = FBE_TRUE;
static fbe_bool_t cpd_shim_dmrb_zeroing = FBE_FALSE;

#ifndef ALAMOSA_WINDOWS_ENV
static VOID fbe_cpd_shim_cdb_io_completion_dpc_function_wrapper(csx_p_dpc_context_t context);
static VOID fbe_cpd_shim_fis_io_completion_dpc_function_wrapper(csx_p_dpc_context_t context);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

static fbe_status_t fbe_cpd_shim_register_encryption_keys(fbe_u32_t port_handle, 
                                                          fbe_u8_t target,
                                                          fbe_u8_t operation,
                                                          fbe_u8_t format,
                                                          fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                                          void *context);
static fbe_status_t fbe_cpd_shim_unregister_encryption_keys(fbe_u32_t port_handle, 
                                                            fbe_u8_t target,
                                                            fbe_u8_t format,
                                                            fbe_u8_t operation,
                                                            fbe_base_port_mgmt_unregister_keys_t * unregister_info);
fbe_status_t 
fbe_cpd_shim_init(void)
{
	fbe_u32_t i;

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_DEBUG_HIGH,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s entry\n", __FUNCTION__);


	FBE_ASSERT_AT_COMPILE_TIME(sizeof(cpd_dmrb_io_context_t) < FBE_PAYLOAD_DMRB_MEMORY_SIZE);

    fbe_spinlock_init(&cpd_port_array_lock);
	for(i = 0 ; i < FBE_CPD_SHIM_MAX_PORTS; i++) {
		fbe_zero_memory(&cpd_port_array[i],sizeof(cpd_port_t));
        cpd_port_array[i].is_valid = FBE_FALSE;
		cpd_port_array[i].io_port_number = FBE_CPD_SHIM_INVALID_PORT_NUMBER;
		cpd_port_array[i].io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
		cpd_port_array[i].state = CPD_SHIM_PORT_STATE_UNINITIALIZED;		
	}
    
    fbe_rendezvous_event_init(&cpd_shim_ioctl_process_io_event);

    /* Start thread */
	cpd_shim_ioctl_process_io_thread_flag = IOCTL_PROCESS_IO_THREAD_RUN;
    fbe_thread_init(&cpd_shim_ioctl_process_io_thread_handle, "fbe_cpd_io", cpd_shim_ioctl_process_io_thread_func, NULL);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_destroy(void)
{
    fbe_u32_t i;

    /* Stop run_queue thread */
    cpd_shim_ioctl_process_io_thread_flag = IOCTL_PROCESS_IO_THREAD_STOP;  
    fbe_rendezvous_event_set(&cpd_shim_ioctl_process_io_event);
    fbe_thread_wait(&cpd_shim_ioctl_process_io_thread_handle);
    fbe_thread_destroy(&cpd_shim_ioctl_process_io_thread_handle);

    fbe_rendezvous_event_destroy(&cpd_shim_ioctl_process_io_event);			
    for(i = 0 ; i < FBE_CPD_SHIM_MAX_PORTS; i++) {
        if ( (cpd_port_array[i].is_valid == FBE_TRUE) ||
             (cpd_port_array[i].miniport_file_object != NULL) ||
			 (cpd_port_array[i].miniport_device_object != NULL) ||
			 (cpd_port_array[i].miniport_callback_handle != NULL))
		{
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s port %X portal %X at index %X is still in use.\n",__FUNCTION__,
								cpd_port_array[i].io_port_number,cpd_port_array[i].io_portal_number,i);

		}
		else
		{
			fbe_zero_memory(&cpd_port_array[i],sizeof(cpd_port_t));
            cpd_port_array[i].is_valid = FBE_FALSE;
			cpd_port_array[i].io_port_number = FBE_CPD_SHIM_INVALID_PORT_NUMBER;
			cpd_port_array[i].io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
			cpd_port_array[i].state = CPD_SHIM_PORT_STATE_UNINITIALIZED;			
		}
	}

    fbe_spinlock_destroy(&cpd_port_array_lock);    
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_port_init(fbe_u32_t  io_port_number, fbe_u32_t   io_portal_number,fbe_u32_t *port_handle)
{
    csx_char_t      deviceNameBuffer[FBE_CPD_DEVICE_NAME_SIZE];
    PEMCPAL_FILE_OBJECT    fileObject;
    PEMCPAL_DEVICE_OBJECT  scsiDeviceObject,portDeviceObject;
    EMCPAL_STATUS        nt_status;
	cpd_port_t      *cpd_port = NULL;
	CPD_CONFIG      *cpd_config = NULL;
	fbe_u32_t		max_device_table_size = 0;
	PCHAR           PassThruBuff = NULL;
	fbe_status_t	status;
	fbe_u32_t i;

    fbe_spinlock_lock(&cpd_port_array_lock);
	for(i = 0; i < FBE_CPD_SHIM_MAX_PORTS; i++) {
	/* Locate a free entry in the table.*/
        if ((cpd_port_array[i].is_valid == FBE_FALSE) &&
            (cpd_port_array[i].io_port_number == FBE_CPD_SHIM_INVALID_PORT_NUMBER) &&
			(cpd_port_array[i].io_portal_number == FBE_CPD_SHIM_INVALID_PORTAL_NUMBER)){
                cpd_port_array[i].is_valid = FBE_TRUE;
			break;
		}
	}
    fbe_spinlock_unlock(&cpd_port_array_lock);

	if (i == FBE_CPD_SHIM_MAX_PORTS){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}
	else{

		cpd_port = &(cpd_port_array[i]);		
        csx_p_snprintf(deviceNameBuffer, FBE_CPD_DEVICE_NAME_SIZE, "\\Device\\ScsiPort%d", io_port_number);
		
        nt_status = EmcpalExtDeviceOpen(deviceNameBuffer, FILE_READ_ATTRIBUTES, &fileObject, &scsiDeviceObject);

        if (!EMCPAL_IS_SUCCESS(nt_status)) {
            cpd_port->is_valid = FBE_FALSE;            
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error call to IoGetDeviceObjectPointer failed. %X\n", __FUNCTION__, nt_status);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		/* Communicate directly with the miniport device object and skip PPFD device object.*/
        if ((fileObject != NULL) && (EmcpalDeviceGetRelatedDeviceObject(fileObject) != NULL)){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s  Using 0x%llX instead of 0x%llX for IOCTLS.\n", __FUNCTION__, (unsigned long long)EmcpalDeviceGetRelatedDeviceObject(fileObject), (unsigned long long)scsiDeviceObject);

            portDeviceObject = EmcpalDeviceGetRelatedDeviceObject(fileObject);			
		}else{
			portDeviceObject = scsiDeviceObject;
		}

        nt_status = EmcpalExtDeviceReference(portDeviceObject,EMCPAL_IOCTL_FILE_ANY_ACCESS,NULL,EmcpalKernelMode);
        if (nt_status != EMCPAL_STATUS_SUCCESS){
            EmcpalExtDeviceClose(fileObject);
            cpd_port->is_valid = FBE_FALSE;            
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s referencing scsiport object failed %X\n", __FUNCTION__, nt_status);

			return FBE_STATUS_GENERIC_FAILURE;
		}

        EmcpalExtDeviceClose(fileObject);
		/* Cannot allocate memory for device table at this point since we do not know the max device supported from miniport driver. */		
		cpd_port->port_info.port_update_generation_code = 0;
        cpd_config = fbe_allocate_nonpaged_pool_with_tag(sizeof (CPD_CONFIG), FBE_CPD_SHIM_MEMORY_TAG);
		if (cpd_config == NULL){
            EmcpalExtDeviceDereference(portDeviceObject);
            cpd_port->is_valid = FBE_FALSE;            
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error allocating memory for CPD_CONFIG \n", __FUNCTION__);
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}
		fbe_zero_memory(cpd_config,sizeof (CPD_CONFIG));

        PassThruBuff = fbe_allocate_nonpaged_pool_with_tag(FBE_CPD_PASS_THRU_BUFFER_SIZE, FBE_CPD_SHIM_MEMORY_TAG);
		if (PassThruBuff  == NULL){
            EmcpalExtDeviceDereference(portDeviceObject);
            fbe_release_nonpaged_pool(cpd_config);
            cpd_port->is_valid = FBE_FALSE;
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error allocating memory for pass thru buffer\n", __FUNCTION__);
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}
		fbe_zero_memory(PassThruBuff,FBE_CPD_PASS_THRU_BUFFER_SIZE);	

		cpd_config->pass_thru_buf = PassThruBuff;
		cpd_config->pass_thru_buf_len = FBE_CPD_PASS_THRU_BUFFER_SIZE;
		status  = fbe_cpd_shim_get_cpd_config(portDeviceObject,   io_portal_number,cpd_config);
		if(status != FBE_STATUS_OK){				
            EmcpalExtDeviceDereference(portDeviceObject);
            fbe_release_nonpaged_pool(cpd_config);
            fbe_release_nonpaged_pool(PassThruBuff);
            cpd_port->is_valid = FBE_FALSE;            
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s fbe_cpd_shim_get_cpd_config failed %X\n", __FUNCTION__, status);
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}
		max_device_table_size = cpd_config->k10_drives + cpd_config->max_infrastructure_devices * 2 +
				                    cpd_config->k10_tgts_more + cpd_config->k10_initiators + 1;			
		cpd_port->topology_device_information_table = 
            fbe_allocate_nonpaged_pool_with_tag((sizeof(fbe_cpd_shim_device_table_t) + max_device_table_size * sizeof(fbe_cpd_shim_device_table_entry_t)), FBE_CPD_SHIM_MEMORY_TAG);
		if (cpd_port->topology_device_information_table == NULL){
            EmcpalExtDeviceDereference(portDeviceObject);
            fbe_release_nonpaged_pool(cpd_config);
            fbe_release_nonpaged_pool(PassThruBuff);
            cpd_port->is_valid = FBE_FALSE;            
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error allocating memory for device table \n", __FUNCTION__);

			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}else {
			fbe_zero_memory(cpd_port->topology_device_information_table,
				(sizeof(fbe_cpd_shim_device_table_t) + max_device_table_size * sizeof(fbe_cpd_shim_device_table_entry_t)));
			cpd_port->topology_device_information_table->device_table_size = max_device_table_size;			
		}
        /* Check whether the miniport driver supports core affinity. */
        if (cpd_config->mp_enabled){
            cpd_port->mc_core_affinity_enabled = FBE_TRUE;
            cpd_port->core_affinity_proc_mask = cpd_config->active_proc_mask;
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s MC core affinity supported by miniport.\n", __FUNCTION__);
        }else{
            cpd_port->mc_core_affinity_enabled = FBE_FALSE;
        }

        cpd_port->port_event_callback_info_queue = fbe_allocate_nonpaged_pool_with_tag(FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY*sizeof(fbe_cpd_shim_callback_info_t), 
                                                                                       FBE_CPD_SHIM_MEMORY_TAG);
		if (cpd_port->port_event_callback_info_queue == NULL){
            EmcpalExtDeviceDereference(portDeviceObject);
            fbe_release_nonpaged_pool(cpd_config);
            fbe_release_nonpaged_pool(PassThruBuff);
            fbe_release_nonpaged_pool(cpd_port->topology_device_information_table);
			cpd_port->topology_device_information_table = NULL;
            cpd_port->is_valid = FBE_FALSE;            
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error allocating memory for event queue\n", __FUNCTION__);

			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}else {
			fbe_cpd_shim_initialize_port_event_queue(cpd_port);
		}
		
        fbe_atomic_exchange(&cpd_port->send_process_io_ioctl,FBE_FALSE);
		cpd_port->io_port_number =  io_port_number;
		cpd_port->io_portal_number =   io_portal_number;
		cpd_port->miniport_file_object = fileObject;
		cpd_port->miniport_device_object = portDeviceObject;
		cpd_port->state = CPD_SHIM_PORT_STATE_INITIALIZED;
        cpd_port->port_reset_in_progress = FBE_FALSE;
		*port_handle = i;
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s  io_port_number = %X   io_portal_number = %X\n", __FUNCTION__,  io_port_number,io_portal_number);
        EmcpalDpcCreate(EmcpalDriverGetCurrentClientObject(),
                        &(cpd_port->port_event_callback_dpc),
                        fbe_cpd_shim_port_event_dpc_function_base,
                        fbe_cpd_shim_port_event_dpc_function,
                        (PVOID)cpd_port, "cpdshimportevent");
        EmcpalDpcCreate(EmcpalDriverGetCurrentClientObject(),
                         &(cpd_port->device_event_callback_dpc),
                         fbe_cpd_shim_device_event_dpc_function_base,
                         fbe_cpd_shim_device_event_dpc_function,
                         (PVOID)cpd_port, "cpdshimdevevent");
#ifndef ALAMOSA_WINDOWS_ENV
        csx_p_dpc_create_always(CSX_MY_MODULE_CONTEXT(), &(cpd_port->cdb_io_completion_dpc), "fbe_cdb_dpc", fbe_cpd_shim_cdb_io_completion_dpc_function_wrapper,(PVOID)cpd_port);
      csx_dlist_head_init (&cpd_port->cdb_io_completion_list);
        csx_p_spl_create_id_always(CSX_MY_MODULE_CONTEXT(), &cpd_port->cdb_io_completion_list_lock, "fbe_cdb_io_list_lock"); 
      
        csx_p_dpc_create_always(CSX_MY_MODULE_CONTEXT(), &(cpd_port->fis_io_completion_dpc), "fdb_fis_dpc", fbe_cpd_shim_fis_io_completion_dpc_function_wrapper,(PVOID)cpd_port);
      csx_dlist_head_init (&cpd_port->fis_io_completion_list);
        csx_p_spl_create_id_always(CSX_MY_MODULE_CONTEXT(), &cpd_port->fis_io_completion_list_lock, "fbe_fis_io_list_lock"); 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
		/*KeSetTargetProcessorDpc(&(cpd_port->port_event_callback_dpc),0);*/
        fbe_release_nonpaged_pool(cpd_config);
        fbe_release_nonpaged_pool(PassThruBuff);
	}

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_port_destroy(fbe_u32_t port_handle)
{
	cpd_port_t      *cpd_port = NULL;

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s  io_port_number = %X \n", __FUNCTION__, port_handle);


    cpd_port = &(cpd_port_array[port_handle]);

    EmcpalDpcDestroy(&(cpd_port->port_event_callback_dpc));
    EmcpalDpcDestroy(&(cpd_port->device_event_callback_dpc));

    EmcpalExtDeviceDereference(cpd_port->miniport_device_object);

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_dpc_destroy(&cpd_port->cdb_io_completion_dpc);
    csx_p_spl_destroy_id(&cpd_port->cdb_io_completion_list_lock);
    CSX_ASSERT_H_RDC(csx_dlist_is_empty(&cpd_port->cdb_io_completion_list));

    csx_p_dpc_destroy(&cpd_port->fis_io_completion_dpc);
    csx_p_spl_destroy_id(&cpd_port->fis_io_completion_list_lock);
    CSX_ASSERT_H_RDC(csx_dlist_is_empty(&cpd_port->fis_io_completion_list));
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

	if (cpd_port->topology_device_information_table != NULL){
        fbe_release_nonpaged_pool(cpd_port->topology_device_information_table);
	}

    if (cpd_port->port_event_callback_info_queue != NULL){
        fbe_release_nonpaged_pool(cpd_port->port_event_callback_info_queue);
        fbe_spinlock_destroy(&(cpd_port->consumer_index_lock));
    }

    fbe_spinlock_lock(&cpd_port_array_lock);
	fbe_zero_memory(cpd_port,sizeof(cpd_port_t));
    cpd_port->is_valid = FBE_FALSE;
	cpd_port->io_port_number = FBE_CPD_SHIM_INVALID_PORT_NUMBER;
	cpd_port->io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
	cpd_port->state = CPD_SHIM_PORT_STATE_UNINITIALIZED;	
    fbe_spinlock_unlock(&cpd_port_array_lock);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_enumerate_backend_io_modules(fbe_cpd_shim_enumerate_io_modules_t * cpd_shim_enumerate_io_modules)
{
    fbe_u32_t                   ioIndex, ctrlIndex;
    fbe_u32_t                   io_module_array_index = 0;
    fbe_u32_t                   specl_enumerated_portals = 0;    
    EMCPAL_STATUS               nt_status;
    fbe_cpd_io_module_info_t    *current_io_module = NULL;
    SPID                        sp_info;
    SP_ID                       sp_id;
    SPECL_IO_SUMMARY            *ioData;
    SPECL_ERROR                 transactionStatus;
    fbe_cpd_io_module_type_t    io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_INVALID;


    if (cpd_shim_enumerate_io_modules == NULL){
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s  Invalid parameter.\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(cpd_shim_enumerate_io_modules,sizeof(fbe_cpd_shim_enumerate_io_modules_t));
    cpd_shim_enumerate_io_modules->rescan_required = FBE_TRUE;

    nt_status = spidGetSpid(&sp_info);

    if (!EMCPAL_IS_SUCCESS(nt_status))
    {
        /* SPID driver request failed
         */
        sp_id = SP_INVALID;
    }
    else
    {
        sp_id = (sp_info.engine == 0) ? SP_A : SP_B;
    }

    ioData = fbe_allocate_nonpaged_pool_with_tag(sizeof(SPECL_IO_SUMMARY), FBE_CPD_SHIM_MEMORY_TAG);
    if (ioData == NULL){
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s  Error allocating memory for SPECL_IO_SUMMARY.\n", __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(ioData,sizeof(SPECL_IO_SUMMARY));
    nt_status = speclGetIOModuleStatus(ioData);
    if (EMCPAL_IS_SUCCESS(nt_status))
    {
        for (ioIndex = 0; ioIndex < ioData->ioSummary[sp_id].ioModuleCount; ioIndex++)
        {
            transactionStatus = ioData->ioSummary[sp_id].ioStatus[ioIndex].transactionStatus;
            if (transactionStatus == EMCPAL_STATUS_SUCCESS)
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s  Successfully queried IOModule information.\n", __FUNCTION__);
            }
            else
            {
                  
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,            
                                       "%s  Error getting IOModule information. Status 0x%x\n", __FUNCTION__,transactionStatus);
                #ifndef C4_INTEGRATED  /* temp skip for beachcomber */
                fbe_release_nonpaged_pool_with_tag(ioData,FBE_CPD_SHIM_MEMORY_TAG);
                return FBE_STATUS_GENERIC_FAILURE;
                #endif /* C4_INTEGRATED - C4HW - C4BUG - seems suspicious */
            }
        }
    }

    /* Currently the plan is to return the information of only SAS IO modules.*/	
    specl_enumerated_portals = 0;

    /* For each I/O module...*/
    for (ioIndex = 0; ioIndex < ioData->ioSummary[sp_id].ioModuleCount; ioIndex++)
    {
        /* For each controller on each I/O module... */
        if (ioData->ioSummary[sp_id].ioStatus[ioIndex].transactionStatus == EMCPAL_STATUS_SUCCESS){
            for (ctrlIndex = 0; ctrlIndex < ioData->ioSummary[sp_id].ioStatus[ioIndex].ioControllerCount; ctrlIndex++)
            {
                switch(ioData->ioSummary[sp_id].ioStatus[ioIndex].ioControllerInfo[ctrlIndex].protocol){
                  case IO_CONTROLLER_PROTOCOL_UNKNOWN:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_UNKNOWN;
                      break;
                  case IO_CONTROLLER_PROTOCOL_FIBRE:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_FC;
                      break;
                  case IO_CONTROLLER_PROTOCOL_ISCSI:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_ISCSI;
                      break;
                  case IO_CONTROLLER_PROTOCOL_SAS:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_SAS;
                      break;
                  case IO_CONTROLLER_PROTOCOL_AGNOSTIC:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_PROTOCOL_AGNOSTIC;
                      break;
                  default:
                      io_module_type = FBE_CPD_SHIM_IO_MODULE_TYPE_INVALID;
                      break;
                }

                current_io_module = &(cpd_shim_enumerate_io_modules->io_module_array[io_module_array_index]);
                current_io_module->io_module_type = io_module_type;
                current_io_module->inserted = ioData->ioSummary[sp_id].ioStatus[ioIndex].inserted;
                if (ioData->ioSummary[sp_id].ioStatus[ioIndex].type == 1)
                {
                    current_io_module->power_good = ioData->ioSummary[sp_id].ioStatus[ioIndex].type1.powerGood;
                }
                else
                {
                    current_io_module->power_good = FBE_TRUE;
                }
                current_io_module->port_count = ioData->ioSummary[sp_id].ioStatus[ioIndex].ioControllerInfo[ctrlIndex].ioPortCount;
                specl_enumerated_portals += current_io_module->port_count;
                io_module_array_index++;

                switch (ioData->ioSummary[sp_id].ioStatus[ioIndex].moduleType)
                {
                case IO_MODULE_TYPE_SLIC_1_0:
			        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								        FBE_TRACE_LEVEL_INFO,
								        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s  IO Module %d. Power %s Port Count : %d.\n", __FUNCTION__, ioIndex,
                                        (current_io_module->power_good ? "GOOD" : "BAD"),current_io_module->port_count);
                    break;

                case IO_MODULE_TYPE_BEM:
			        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								        FBE_TRACE_LEVEL_INFO,
								        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s  Base Module SAS controller. Port Count : %d.\n", __FUNCTION__, current_io_module->port_count);
                    break;

                case IO_MODULE_TYPE_MEZZANINE:
			        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								        FBE_TRACE_LEVEL_INFO,
								        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s  Mezzanine SAS controller. Port Count : %d.\n", __FUNCTION__, current_io_module->port_count);
                    break;

                case IO_MODULE_TYPE_ONBOARD:
			        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								        FBE_TRACE_LEVEL_INFO,
								        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s  Onboard SAS controller. Port Count : %d.\n", __FUNCTION__, current_io_module->port_count);
                    break;
                }


            }
        }
    }
    cpd_shim_enumerate_io_modules->number_of_io_modules = io_module_array_index;
    cpd_shim_enumerate_io_modules->total_enumerated_io_ports = specl_enumerated_portals;
    cpd_shim_enumerate_io_modules->rescan_required = FBE_FALSE;
    fbe_release_nonpaged_pool_with_tag(ioData,FBE_CPD_SHIM_MEMORY_TAG);

    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_cpd_shim_enumerate_backend_ports(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports)
{
	CPD_CONFIG    * cpd_config = NULL;
	CPD_CAPABILITIES  * cpd_capabilities = NULL;
    PCHAR              PassThruBuff;
    fbe_u32_t        io_port_number;
	fbe_u32_t         io_portal_number;
	fbe_u32_t       max_portal_number,max_discovered_portals = 0;
    PEMCPAL_DEVICE_OBJECT  scsiDeviceObject,portDeviceObject;
    EMCPAL_STATUS        nt_status;
    PEMCPAL_FILE_OBJECT    fileObject;
    csx_char_t           deviceNameBuffer[FBE_CPD_DEVICE_NAME_SIZE];
	fbe_u32_t		flare_bus;
	fbe_status_t	status;
    fbe_cpd_shim_backend_port_info_t *be_port_info = NULL;
    fbe_cpd_shim_port_role_t port_role = FBE_CPD_SHIM_PORT_ROLE_INVALID;
    fbe_cpd_shim_connect_class_t   connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
    fbe_cpd_shim_hardware_info_t hdw_info;

    if (cpd_shim_enumerate_backend_ports == NULL){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s invalid ptr passed to function. %X\n", __FUNCTION__, (unsigned int)cpd_shim_enumerate_backend_ports);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(cpd_shim_enumerate_backend_ports,sizeof(fbe_cpd_shim_enumerate_backend_ports_t));
	cpd_shim_enumerate_backend_ports->number_of_io_ports = 0;
    cpd_shim_enumerate_backend_ports->total_discovered_io_ports = 0;
    cpd_shim_enumerate_backend_ports->rescan_required = FBE_TRUE;

	/* Allocate cpd_config structure */
    cpd_config = fbe_allocate_nonpaged_pool_with_tag(sizeof (CPD_CONFIG), FBE_CPD_SHIM_MEMORY_TAG);
    if (cpd_config == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory(cpd_config,sizeof (CPD_CONFIG));

    cpd_capabilities = fbe_allocate_nonpaged_pool_with_tag( sizeof (CPD_CAPABILITIES), FBE_CPD_SHIM_MEMORY_TAG);
	if (cpd_capabilities  == NULL){
        fbe_release_nonpaged_pool(cpd_config);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory(cpd_capabilities,sizeof (CPD_CAPABILITIES));

    PassThruBuff = fbe_allocate_nonpaged_pool_with_tag( FBE_CPD_PASS_THRU_BUFFER_SIZE, FBE_CPD_SHIM_MEMORY_TAG);
	if (PassThruBuff  == NULL){
        fbe_release_nonpaged_pool(cpd_config);
        fbe_release_nonpaged_pool(cpd_capabilities);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory(PassThruBuff,FBE_CPD_PASS_THRU_BUFFER_SIZE);	

	max_discovered_portals = 0;

    /* Open port driver controller device objects by name. */
	for(io_port_number = 0;  io_port_number < FBE_CPD_SHIM_MAX_PORTS;  io_port_number++) {

        csx_p_snprintf(deviceNameBuffer, FBE_CPD_DEVICE_NAME_SIZE, "\\Device\\ScsiPort%d",  io_port_number);

        nt_status = EmcpalExtDeviceOpen(deviceNameBuffer, FILE_READ_ATTRIBUTES, &fileObject, &scsiDeviceObject);

        if (!EMCPAL_IS_SUCCESS(nt_status)) {
            continue;
        }

        if ((fileObject != NULL) && (EmcpalDeviceGetRelatedDeviceObject(fileObject) != NULL)){
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s  Using 0x%p instead of 0x%p for IOCTLS.\n", __FUNCTION__, EmcpalDeviceGetRelatedDeviceObject(fileObject),scsiDeviceObject);

            portDeviceObject = EmcpalDeviceGetRelatedDeviceObject(fileObject);
		}else{
			portDeviceObject = scsiDeviceObject;
		}

        /* Query the hardware information.*/
        status  = cpd_shim_get_hardware_info(portDeviceObject,&hdw_info);
		if(status != FBE_STATUS_OK){				
            EmcpalExtDeviceClose(fileObject);
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s fbe_cpd_shim_get_hardware_info failed %X\n", __FUNCTION__, status);

			continue;
		}

		/* Query the port capabilities.*/
		status  = fbe_cpd_shim_get_cpd_capabilities(portDeviceObject, 0,cpd_capabilities);
		if(status != FBE_STATUS_OK){
            EmcpalExtDeviceClose(fileObject);
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s fbe_cpd_shim_get_cpd_capabilities fail %X\n", __FUNCTION__, status);

			continue;
		}
		max_portal_number = cpd_capabilities->nr_portals;

		for (io_portal_number = 0;   io_portal_number < max_portal_number;   io_portal_number++){

            /* Config ioctl only supported on portal 0 unless independent ports supported.*/  
            if((io_portal_number > 0) && !(cpd_capabilities->misc & CPD_CAP_INDEPENDENT_PORTS)){
                break;
            }

            fbe_zero_memory(PassThruBuff,FBE_CPD_PASS_THRU_BUFFER_SIZE);
			cpd_config->pass_thru_buf = PassThruBuff;
			cpd_config->pass_thru_buf_len = FBE_CPD_PASS_THRU_BUFFER_SIZE;

			status  = fbe_cpd_shim_get_cpd_config(portDeviceObject,   io_portal_number,cpd_config);
			if(status != FBE_STATUS_OK){				
				fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
									FBE_TRACE_LEVEL_WARNING,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s fbe_cpd_shim_get_cpd_config fail %X\n", __FUNCTION__, status);

				continue;
			}
			/** Check if the connect_class is SAS */
            switch(cpd_config->connect_class){
                case CPD_CC_FIBRE_CHANNEL:
                    connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
                    break;
                case CPD_CC_INTERNET_SCSI:
                    connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;                    
                    break;
                case CPD_CC_SAS:
                    connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
                    break;
                case CPD_CC_FCOE:
                    connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FCOE;
                    break;

                default:
                    connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
                    break;
            }

			max_discovered_portals++;
			/** We have no plans to support SAS front end ports */
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s. SAS Port Number:%d\n", __FUNCTION__,  io_port_number);


			/* Check if it is Back End port */
			status = fbe_cpd_shim_get_flare_bus(cpd_config->pass_thru_buf, FBE_CPD_PASS_THRU_BUFFER_SIZE, &port_role,&flare_bus);
			if(status == FBE_STATUS_OK){
                be_port_info = &(cpd_shim_enumerate_backend_ports->io_port_array[cpd_shim_enumerate_backend_ports->number_of_io_ports]);
                be_port_info->connect_class =  connect_class;
                be_port_info->port_role = port_role;
				be_port_info->port_number =  io_port_number;
				be_port_info->portal_number =   io_portal_number;
				be_port_info->assigned_bus_number = flare_bus;
                be_port_info->hdw_info = hdw_info;

				cpd_shim_enumerate_backend_ports->number_of_io_ports++;
			}else{
				fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
									FBE_TRACE_LEVEL_INFO,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s fbe_cpd_shim_get_flare_bus fail %X\n", __FUNCTION__, status);
			}			
		}

        EmcpalExtDeviceClose(fileObject);		
	} /* for(port_number = 0;  io_port_number < FBE_CPD_SHIM_MAX_PORTS;  io_port_number++) */

    cpd_shim_enumerate_backend_ports->total_discovered_io_ports = max_discovered_portals;
	cpd_shim_enumerate_backend_ports->rescan_required = FBE_FALSE;
	/* release cpd_config */
    fbe_release_nonpaged_pool(cpd_config);
    fbe_release_nonpaged_pool(cpd_capabilities);
    fbe_release_nonpaged_pool(PassThruBuff);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_port_register_callback(fbe_u32_t port_handle,
                                    fbe_u32_t cpd_shim_registration_flags,
									fbe_cpd_shim_callback_function_t callback_function,
									fbe_cpd_shim_callback_context_t callback_context)
{
	fbe_status_t status;

	cpd_port_array[port_handle].callback_function = callback_function;
	cpd_port_array[port_handle].callback_context = callback_context;

	status = fbe_cpd_shim_send_register_callback(port_handle,cpd_shim_registration_flags);
	if (status != FBE_STATUS_OK){
		cpd_port_array[port_handle].callback_function = NULL;
		cpd_port_array[port_handle].callback_context = NULL;
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Register callback failed port_handle = %X\n", __FUNCTION__, port_handle);

    }else{
        fbe_atomic_compare_exchange(&(cpd_port_array[port_handle].port_reset_in_progress),FBE_FALSE,FBE_TRUE);
    }
   
	return status;
}

fbe_status_t 
fbe_cpd_shim_port_unregister_callback(fbe_u32_t port_handle)
{

	fbe_status_t status = FBE_STATUS_OK;

	if ((cpd_port_array[port_handle].miniport_callback_handle != NULL) &&
		 (cpd_port_array[port_handle].callback_function != NULL))
	{
        /* Do not let any additional IOCTLs go to the miniport for this port instance.*/
        fbe_atomic_compare_exchange(&(cpd_port_array[port_handle].port_reset_in_progress),FBE_TRUE,FBE_FALSE);
		status = fbe_cpd_shim_send_unregister_callback(port_handle);
		if (status != FBE_STATUS_OK){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Unregister callback failed port_handle = %X\n", __FUNCTION__, port_handle);

		}
	}

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s  io_port_number = %X\n", __FUNCTION__, port_handle);


	cpd_port_array[port_handle].callback_function = NULL;
	cpd_port_array[port_handle].callback_context = NULL;

	return FBE_STATUS_OK;
}



static BOOLEAN 
fbe_cpd_shim_callback (IN void * context, IN CPD_EVENT_INFO *pInfo)
{	
	cpd_port_t * cpd_port = (cpd_port_t *)context;
	fbe_u32_t					current_device_index = 0;
	fbe_cpd_shim_device_table_entry_t	*current_device_entry = NULL;	
	fbe_cpd_shim_callback_info_t		callback_info;
	CPD_PORT_FAILURE_TYPE				port_failure;
	CPD_PORT_INFORMATION				*link_info;
	fbe_bool_t					device_event = FALSE,skip_notify_port_event = TRUE; 

    if ((context == NULL) || (pInfo == NULL))
    {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_CRITICAL_ERROR,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Critical error NULL input pointer \n",__FUNCTION__);

		return FALSE;
    }		

    fbe_zero_memory(&callback_info,sizeof(fbe_cpd_shim_callback_info_t));

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s event %d, io_port_number: %d, io_portal_number: %d \n",
                        __FUNCTION__, pInfo->type,
                        cpd_port->io_port_number, cpd_port->io_portal_number);

    switch(pInfo->type)
    {
	case CPD_EVENT_LINK_DOWN:

		callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN;
		link_info = &(pInfo->u.link_info);
		callback_info.info.port_lane_info.portal_number = link_info->portal_nr;
		callback_info.info.port_lane_info.nr_phys = link_info->port_info.sas.nr_phys;
		callback_info.info.port_lane_info.phy_map = link_info->port_info.sas.phy_map;
		callback_info.info.port_lane_info.nr_phys_enabled = link_info->port_info.sas.nr_phys_enabled;
		callback_info.info.port_lane_info.nr_phys_up = link_info->port_info.sas.nr_phys_up;
		
		cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_LINK_DOWN;
		cpd_port->port_info.port_lane_info = callback_info.info.port_lane_info;
		cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;
		
		skip_notify_port_event = FALSE;
		break;

	case CPD_EVENT_LINK_UP:
		callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP;
		link_info = &(pInfo->u.link_info);
		callback_info.info.port_lane_info.portal_number = link_info->portal_nr;
		callback_info.info.port_lane_info.nr_phys = link_info->port_info.sas.nr_phys;
		callback_info.info.port_lane_info.phy_map = link_info->port_info.sas.phy_map;
		callback_info.info.port_lane_info.nr_phys_enabled = link_info->port_info.sas.nr_phys_enabled;
		callback_info.info.port_lane_info.nr_phys_up = link_info->port_info.sas.nr_phys_up;
		
		cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_LINK_UP;
		cpd_port->port_info.port_lane_info = callback_info.info.port_lane_info;
		cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;

		skip_notify_port_event = FALSE;
		break;

	case CPD_EVENT_LINK_STATE_CHANGE:
		switch(pInfo->u.link_info.port_info.sas.change_type)
		{
		case CPD_LINK_DISCOVERY_STARTED:
			cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;
			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_BEGIN;
			/* These notifications are skipped for now since physical package is not using this. */
			skip_notify_port_event = TRUE;
			break;
		case CPD_LINK_DISCOVERY_CHANGES_COMPLETED:
			cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;
			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_COMPLETE;
			/* These notifications are skipped for now since physical package is not using this. */
			skip_notify_port_event = TRUE;
			break;
        case CPD_LINK_DEGRADED:
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS;
			link_info = &(pInfo->u.link_info);
			callback_info.info.port_lane_info.portal_number = link_info->portal_nr;
			callback_info.info.port_lane_info.nr_phys = link_info->port_info.sas.nr_phys;
			callback_info.info.port_lane_info.phy_map = link_info->port_info.sas.phy_map;
			callback_info.info.port_lane_info.nr_phys_enabled = link_info->port_info.sas.nr_phys_enabled;
			callback_info.info.port_lane_info.nr_phys_up = link_info->port_info.sas.nr_phys_up;

			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_LINK_DEGRADED;
			cpd_port->port_info.port_lane_info = callback_info.info.port_lane_info;
			cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;

			skip_notify_port_event = FALSE;
			break;

		case CPD_LINK_ALL_LANES_UP:
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS;
			link_info = &(pInfo->u.link_info);
			callback_info.info.port_lane_info.portal_number = link_info->portal_nr;
			callback_info.info.port_lane_info.nr_phys = link_info->port_info.sas.nr_phys;
			callback_info.info.port_lane_info.phy_map = link_info->port_info.sas.phy_map;
			callback_info.info.port_lane_info.nr_phys_enabled = link_info->port_info.sas.nr_phys_enabled;
			callback_info.info.port_lane_info.nr_phys_up = link_info->port_info.sas.nr_phys_up;

			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_LINK_UP;
			cpd_port->port_info.port_lane_info = callback_info.info.port_lane_info;
			cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;

			skip_notify_port_event = FALSE;
			break;

		default:
			break;

		}		
		
		break;

	case CPD_EVENT_SFP_CONDITION:
		cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;
		callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_SFP;

        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_DEBUG_LOW,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Rcvd SFP Condition for Port:%d Portal:%d. \n",
                               __FUNCTION__,
                               cpd_port->io_port_number,
                               cpd_port->io_portal_number);
		
        /* Map CPD information and store it in cpd_port.*/
        cpd_shim_map_cpd_sfp_media_interface_information(&(pInfo->u.mii),&(cpd_port->sfp_info));
        cpd_port->sfp_info_valid = FBE_TRUE;
        skip_notify_port_event = FALSE;        
        break;

    case CPD_EVENT_LOGIN:
		current_device_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(pInfo->u.port_login.miniport_login_context);
		if (current_device_index >= cpd_port->topology_device_information_table->device_table_size){
			/*panic()*/
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Critical error device_index larger than table size. \n",__FUNCTION__);

			return FALSE;
		}

		current_device_entry = &(cpd_port->topology_device_information_table->device_entry[current_device_index]);

        current_device_entry->log_out_received = FALSE;
		fbe_cpd_shim_get_device_type(pInfo->u.port_login.flags,&current_device_entry->device_type);
		current_device_entry->device_id =  (fbe_miniport_device_id_t)pInfo->u.port_login.miniport_login_context;
		current_device_entry->device_address = pInfo->u.port_login.name->name.sas.sas_address;
		current_device_entry->device_locator.enclosure_chain_depth = pInfo->u.port_login.address->address.sas.s.depth;
		current_device_entry->device_locator.enclosure_chain_width = pInfo->u.port_login.address->address.sas.s.exp_nr;
		current_device_entry->device_locator.phy_number = pInfo->u.port_login.address->address.sas.s.phy_nr;
		current_device_entry->device_locator.padding = 0;
		current_device_entry->parent_device_id = (fbe_miniport_device_id_t)pInfo->u.port_login.parent_context;
        /* Updates to handle modifications to Miniport spec
           Miniport will log in expanders in a failed state if the addition of the expander
           causes an invalid configuration.
        */
        fbe_cpd_shim_get_login_reason(pInfo,current_device_entry);
		fbe_atomic_increment(&(current_device_entry->current_gen_number));
		device_event = TRUE;
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s IOPort:%d Portal:%d LOGIN for device %d\n", __FUNCTION__, 
                               cpd_port->io_port_number, 
                               cpd_port->io_portal_number, 
                               current_device_index);
		break;


	/*case CPD_EVENT_DEVICE_FAILED:*/
	case CPD_EVENT_LOGOUT:		
		current_device_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(pInfo->u.port_login.miniport_login_context);
		if (current_device_index >= cpd_port->topology_device_information_table->device_table_size){
			/*panic()*/
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Critical error device_index larger than table size. \n",__FUNCTION__);

			return FALSE;
		}

		/*in case this LOGOUT is because the OS is going down we'll ignore that.
		The reason we do it is because if we send the logout to the port objects, all the RGs will think
		The drives just died under them and will start a series of CMI traffic to the peer. Technically 
		the single loop failure code should detect this is a false logout and ignore it but until it's 
		robust, we have to prevent this case*/
		if (pInfo->u.port_login.flags & CPD_LOGIN_SHUTDOWN_LOGOUT) {
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								   FBE_TRACE_LEVEL_INFO,
								   FBE_TRACE_MESSAGE_ID_INFO,
								   "%s IOPort:%d Portal:%d IGNORE LOGOUT,device %d, OS shutdown\n", __FUNCTION__, 
								   cpd_port->io_port_number, 
								   cpd_port->io_portal_number, 
								   current_device_index);

			return TRUE;
		}

		current_device_entry = &(cpd_port->topology_device_information_table->device_entry[current_device_index]);
		/*ASSERT(current_device_entry->device_id ==  (fbe_miniport_device_id_t)pInfo->u.port_login.miniport_login_context);*/
		current_device_entry->log_out_received = TRUE;
		fbe_atomic_increment(&(current_device_entry->current_gen_number));		
		device_event = TRUE;
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s IOPort:%d Portal:%d LOGOUT for device %d\n", __FUNCTION__, 
                               cpd_port->io_port_number, 
                               cpd_port->io_portal_number, 
                               current_device_index);
		break;

	case CPD_EVENT_PORT_FAILURE:
		cpd_port->port_info.port_update_generation_code = cpd_port->current_generation_code;
		port_failure = pInfo->u.port_fail.failure;

        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s IOPort:%d Portal:%d PORT_FAILURE received type 0x%x\n", __FUNCTION__, 
                               cpd_port->io_port_number, 
                               cpd_port->io_portal_number, 
                               port_failure);

		if (port_failure == CPD_PORT_FAILURE_DRIVER_FAILURE){

            /* Do not let any additional IOCTLs go to the miniport for this port instance.*/
            fbe_atomic_compare_exchange(&cpd_port->port_reset_in_progress,FBE_TRUE,FBE_FALSE);

			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_RESET_BEGIN;
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_DRIVER_RESET;
			callback_info.info.driver_reset.driver_reset_event_type = FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN;
		} else if (port_failure == CPD_PORT_FAILURE_DRIVER_READY){
			cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_RESET_COMPLETE;
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_DRIVER_RESET;
			callback_info.info.driver_reset.driver_reset_event_type = FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED;
		} else if (port_failure == CPD_PORT_FAILURE_RECOVERED_CTLR_ERROR ) {
            cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_CTRL_RESET_START;
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_CTRL_RESET;
			callback_info.info.driver_reset.driver_reset_event_type = FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_BEGIN;
        } else if (port_failure == CPD_PORT_FAILURE_CTLR_READY ) {
            cpd_port->port_info.port_state = CPD_SHIM_PHYSICAL_PORT_STATE_CTRL_RESET_COMPLETE;
			callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_CTRL_RESET;
			callback_info.info.driver_reset.driver_reset_event_type = FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_COMPLETED;
        }
		skip_notify_port_event = FALSE;
		break;
        case CPD_EVENT_ENC_ENABLED :
        case CPD_EVENT_ENC_KEY_SET :
        case CPD_EVENT_ENC_KEY_INVALIDATED:
        case CPD_EVENT_ENC_SELF_TEST :
        case CPD_EVENT_ENC_ALL_KEYS_FLUSHED :
        case CPD_EVENT_ENC_MODE_MODIFIED :
            callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_ENCRYPTION;
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s Encryption event: 0x%x status 0x%x\n", __FUNCTION__, 
                                       pInfo->type, pInfo->u.enc_key.status);
            if(pInfo->u.enc_key.caller_context != NULL)
            {
                callback_info.info.encrypt_context.context = pInfo->u.enc_key.caller_context;
                if (pInfo->u.enc_key.status == CPD_RC_NO_ERROR )
                {
                    callback_info.info.encrypt_context.status = FBE_STATUS_OK;
                }
                else
                {
                    callback_info.info.encrypt_context.status = FBE_STATUS_GENERIC_FAILURE;
                }
                skip_notify_port_event = FALSE;
            }
            break;
	default:		
		return FALSE;
		break;

	}

	if(device_event){	
		fbe_cpd_shim_schedule_device_event_dpc(cpd_port);
	}else{
		if (!skip_notify_port_event){
			fbe_atomic_increment(&(cpd_port->current_generation_code));
			fbe_cpd_shim_add_to_port_event_queue(cpd_port,&callback_info);
			fbe_cpd_shim_schedule_port_event_dpc(cpd_port);
		}
	}
	return TRUE;
}

fbe_status_t 
fbe_cpd_shim_port_register_payload_completion(fbe_u32_t port_number,
                                        fbe_payload_ex_completion_function_t completion_function,
                                        fbe_payload_ex_completion_context_t  completion_context)
{	

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s port_number = %X\n", __FUNCTION__, port_number);

	cpd_port_array[port_number].payload_completion_function = completion_function;
	cpd_port_array[port_number].payload_completion_context = completion_context;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_port_unregister_payload_completion(fbe_u32_t port_number)
{
	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s port_number = %X\n", __FUNCTION__, port_number);

	cpd_port_array[port_number].payload_completion_function = NULL;
	cpd_port_array[port_number].payload_completion_context = NULL;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_send_payload(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{

	fbe_status_t					status = FBE_STATUS_OK;
	fbe_payload_cdb_operation_t *	payload_cdb_operation = NULL;
	fbe_payload_dmrb_operation_t *  payload_dmrb_operation = NULL;
	cpd_dmrb_io_context_t        *  dmrb_io_context = NULL;
	PCPD_DMRB						dmrb = NULL;
	PHYSICAL_ADDRESS				physical_address;	
	fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * pre_sg_list = NULL;
	fbe_sg_element_t * post_sg_list = NULL;
	fbe_u32_t						i = 0,queue_pos = 0;
	cpd_port_t					   *cpd_port = NULL;
	CPD_DMRB_STATUS					dmrb_enqueue_status;
#if 0
    EMCPAL_LARGE_INTEGER					performance_frequency;
    EMCPAL_LARGE_INTEGER					performance_counter;
#endif
	fbe_payload_attr_t				payload_attr = 0;
	fbe_u64_t						physical_offset = 0;
    fbe_port_request_status_t       port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_port_recovery_status_t      port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
    fbe_u32_t                       data_transferred = 0;
    fbe_key_handle_t                key_handle;

    cpd_port = &(cpd_port_array[port_handle]);

	if (payload == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: payload == NULL\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
	if (payload_cdb_operation == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: payload_cdb_operation == NULL\n", __FUNCTION__);

		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload_dmrb_operation = fbe_payload_ex_allocate_dmrb_operation(payload);
	if (payload_dmrb_operation == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: payload_dmrb_operation == NULL\n", __FUNCTION__);

		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_payload_dmrb_get_memory(payload_dmrb_operation,(void **)&dmrb_io_context);
	if (FBE_STATUS_OK != status){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: error getting memory for dmrb.\n", __FUNCTION__);

        status = fbe_payload_ex_release_dmrb_operation(payload,payload_dmrb_operation);
		if (status != FBE_STATUS_OK){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: Error releasing DMRB operation. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);

		}
		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);
		return status;
	}

    fbe_payload_ex_increment_dmrb_operation_level(payload);
	dmrb = &(dmrb_io_context->dmrb);

	/* fbe_zero_memory(dmrb,sizeof(CPD_DMRB));*/ /* The memory is already zeroed by payload_init 
											   called from fbe_transport_reuse_packet or 
											   fbe_transport_initialize_packet*/

	dmrb->revision = CPD_DMRB_REVISION;	
	/* CPD does not define an equivalnet for FBE_PAYLOAD_CDB_FLAGS_UNSPECIFIED_DIRECTION.*/
	if(payload_cdb_operation->payload_cdb_flags == FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER){
		dmrb->function			= CPD_INITIATOR_NO_DATA;
	}else if(payload_cdb_operation->payload_cdb_flags & FBE_PAYLOAD_CDB_FLAGS_DATA_IN){
		dmrb->function			= CPD_INITIATOR_DATA_IN;
	}else if(payload_cdb_operation->payload_cdb_flags & FBE_PAYLOAD_CDB_FLAGS_DATA_OUT){
		dmrb->function			= CPD_INITIATOR_DATA_OUT;
	}
    dmrb->flags = 0;
    if(payload_cdb_operation->payload_cdb_flags & FBE_PAYLOAD_CDB_FLAGS_DRIVE_FW_UPGRADE)
    {
        dmrb->flags = CPD_DMRB_FLAGS_DISABLE_MAX_XFER_LEN_CHK;
    }
	dmrb->target.login_context = UIntToPtr(cpd_device_id);
	dmrb->target.portal_nr = cpd_port->io_portal_number;
	dmrb->target.login_qualifier = 0 ;
    dmrb->target.lun.lun = 0;
    dmrb->target.lun.access_mode = 0;
    dmrb->physical_offset = 0;
    dmrb->block_addr = 0;

    /* Let the miniport know this is a 4K IO or not and the sector overhead bytes. This is mostly for
     * encryption related purposes
     */
    if (payload_cdb_operation->payload_cdb_flags & FBE_PAYLOAD_CDB_FLAGS_BLOCK_SIZE_4160) {
        dmrb->total_sector_size = FBE_BLOCK_SIZES_4160;
        dmrb->sector_overhead = 64;
    } else {
        dmrb->total_sector_size = FBE_BLOCK_SIZES_520;
        dmrb->sector_overhead = 8;
    }
    


    switch (payload_cdb_operation->payload_cdb_task_attribute) {
		case FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE:
			dmrb->u.scsi.queue_action = CPD_SIMPLE_QUEUE_TAG;
			break;
		case FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_ORDERED:
			dmrb->u.scsi.queue_action = CPD_ORDERED_QUEUE_TAG;
			break;
		case FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_HEAD_OF_QUEUE:
			dmrb->u.scsi.queue_action = CPD_HEAD_OF_QUEUE_TAG;
			break;
		default:
			dmrb->u.scsi.queue_action = CPD_SIMPLE_QUEUE_TAG;
			break;
	};

	dmrb->timeout = (UINT_32E)payload_cdb_operation->timeout/1000;
    dmrb->data_transferred = 0;	
	dmrb->sgl_offset	= 0;
	dmrb->data_length = 0;   
	fbe_payload_cdb_get_transfer_count(payload_cdb_operation, &dmrb->data_length);
	if((dmrb->data_length == 0) &&  (dmrb->function	!= CPD_INITIATOR_NO_DATA)) {
        EmcpalDebugBreakPoint();
	}

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_ex_get_pre_sg_list(payload, &pre_sg_list);
    fbe_payload_ex_get_post_sg_list(payload, &post_sg_list);
    fbe_payload_ex_get_attr(payload, &payload_attr);
    fbe_payload_ex_get_key_handle(payload, &key_handle);

    if(key_handle != FBE_INVALID_KEY_HANDLE) {
        dmrb->enc_key = (CPD_KEY_HANDLE)key_handle;
        dmrb->block_addr = fbe_get_cdb_lba(payload_cdb_operation);
    }
    else {
        dmrb->enc_key = CPD_KEY_PLAINTEXT;
    }

	if(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET){
        fbe_payload_ex_get_physical_offset(payload, &physical_offset);
	}
	dmrb->flags |= CPD_DMRB_FLAGS_SGL_DESCRIPTOR;
	dmrb->sgl = (struct _SGL *)dmrb_io_context->sgl_descriptor;

	for(i = 0 ; i < 3; i++){
		if(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET){
			dmrb_io_context->sgl_descriptor[i].sgl_ctrl.xlat_type = SGL_CONSTANT_TRANSLATION;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.physical_offset = physical_offset;
		} else {
			dmrb_io_context->sgl_descriptor[i].sgl_ctrl.xlat_type = SGL_FUNCTION_TRANSLATION;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.sgl_xlat_func.sgl_get_paddr = cpd_shim_get_phys_addr;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.sgl_xlat_func.xlat_context = NULL;
		}

		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.repeat_this_sgl = FALSE; /* Valid for the first entry only */
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.repeat_all_sgls = FALSE; 
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.skip_metadata = FALSE; 
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.last_descriptor = FALSE; 
		dmrb_io_context->sgl_descriptor[i].repeat_count = 0;
	}

    dmrb_io_context->sgl_descriptor[0].sgl_ptr = (VSGL*)pre_sg_list;
    dmrb_io_context->sgl_descriptor[1].sgl_ptr = (VSGL*)sg_list;
	/* CBE WRITE SAME */
	if(sg_list == NULL){
		dmrb_io_context->sgl_descriptor[0].sgl_ctrl.last_descriptor = TRUE; 
	}

    dmrb_io_context->sgl_descriptor[2].sgl_ptr = (VSGL*)post_sg_list;

#ifndef ALAMOSA_WINDOWS_ENV
    if(!(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET)) {
        if ((dmrb_io_context->sgl_descriptor[0].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[0].sgl_ptr->virt_address != NULL))
        {
            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[0].sgl_ptr->virt_address, NULL) != 0);
        }
        if ((dmrb_io_context->sgl_descriptor[1].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[1].sgl_ptr->virt_address != NULL))
        {

            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[1].sgl_ptr->virt_address, NULL) != 0);
        }
        if ((dmrb_io_context->sgl_descriptor[2].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[2].sgl_ptr->virt_address != NULL))
        {
            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[2].sgl_ptr->virt_address, NULL) != 0);
        }
    }
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system - check for failure to convert VA to PA */
	dmrb_io_context->sgl_descriptor[2].sgl_ctrl.last_descriptor = TRUE; 


	dmrb->u.scsi.cdb_length		= payload_cdb_operation->cdb_length;
        //dmrb->timeout = (UINT_32E)payload_cdb_operation->timeout/1000;
	dmrb->timeout = (UINT_32E)(payload_cdb_operation->timeout >> 10);
        // payload clears max possible cdb and fills in what is needed
        EmcpalMoveMemory(&dmrb->u.scsi.cdb[0], &(payload_cdb_operation->cdb[0]), FBE_PAYLOAD_CDB_CDB_SIZE);
	dmrb->u.scsi.scsi_status      = 0;  
	dmrb->u.scsi.sense_length		= FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE;
	physical_address.QuadPart = fbe_get_contigmem_physical_address((void *)(&payload_cdb_operation->sense_info_buffer[0]));
	dmrb->u.scsi.sense_buffer		= &payload_cdb_operation->sense_info_buffer[0];
	dmrb->u.scsi.sense_phys_offset = (PUCHAR)physical_address.QuadPart - payload_cdb_operation->sense_info_buffer;/**/

	dmrb_io_context->device_id   = cpd_device_id;
	dmrb_io_context->port_handle = port_handle;
	dmrb_io_context->payload     = payload;

    dmrb->callback_arg1  	= (void *)dmrb; 
    dmrb->callback_arg2	    = (void *)dmrb_io_context;
    dmrb->callback_handle   = cpd_port->miniport_callback_handle;
    dmrb->status			= 0;	

    if (cpd_port->mc_core_affinity_enabled){
        dmrb->callback			= (void *)fbe_cpd_shim_cdb_io_completion_callback_function; /* void (* callback) (void *, void *); */
    }else{
	dmrb->callback			= (void *)fbe_cpd_shim_cdb_io_miniport_callback; /* void (* callback) (void *, void *); */
#ifdef ALAMOSA_WINDOWS_ENV
	KeInitializeDpc(&(dmrb_io_context->dpc), fbe_cpd_shim_cdb_io_completion_dpc_function,(PVOID)dmrb_io_context);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
    }
#if 0
	performance_counter = KeQueryPerformanceCounter(&performance_frequency);
	payload_cdb_operation->time_stamp = performance_counter.QuadPart / (performance_frequency.QuadPart / (fbe_u64_t)1000); /* Start time in ms */
#endif

	/* Submit the request.*/
	dmrb_enqueue_status = cpd_port->enqueue_pending_dmrb(cpd_port->enque_pending_dmrb_context,dmrb,&queue_pos);

    if ((!cpd_port->mc_core_affinity_enabled) && (dmrb_enqueue_status == CPD_STATUS_GOOD_NOTIFY)){
            /* Needs a PROCESS_IO to kick start things.*/        
            fbe_atomic_exchange(&cpd_port->send_process_io_ioctl,FBE_TRUE);
            fbe_rendezvous_event_set(&cpd_shim_ioctl_process_io_event);
            status = FBE_STATUS_OK;
	}else if(dmrb_enqueue_status != CPD_STATUS_GOOD){

		/* Error submitting IO. */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: Error submitting IO. Port 0x%llX Payload 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port,(unsigned long long)dmrb_io_context->payload);


        data_transferred = dmrb->data_transferred;
        if (fbe_payload_ex_release_dmrb_operation(dmrb_io_context->payload,payload_dmrb_operation) != FBE_STATUS_OK){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: Error releasing DMRB operation. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);

		}

        status = FBE_STATUS_GENERIC_FAILURE;
        port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
        switch(dmrb->status){
            case CPD_STATUS_INVALID_REQUEST:
                port_request_status = FBE_PORT_REQUEST_STATUS_INVALID_REQUEST;
                break;

            case CPD_STATUS_DEVICE_NOT_LOGGED_IN:
                port_request_status = FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN;
                break;
            case CPD_STATUS_DEVICE_BUSY:
                port_request_status = FBE_PORT_REQUEST_STATUS_BUSY;
                break;
            case CPD_STATUS_INCIDENTAL_ABORT:
                port_request_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
                //port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
                break;
            default:
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "send payload: Unknown status from miniport. Port 0x%p Status 0x%x\n",cpd_port,dmrb->status);

                port_request_status = FBE_PORT_REQUEST_STATUS_ERROR;
                break;
        }
        fbe_payload_cdb_set_transferred_count(payload_cdb_operation,data_transferred);
        fbe_payload_cdb_set_request_status(payload_cdb_operation,port_request_status);//FBE_STATUS_IO_FAILED_RETRYABLE ?	
        fbe_payload_cdb_set_recovery_status(payload_cdb_operation,port_recovery_status);      
        
        cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
	}

    return status;
}

static void fbe_cpd_shim_cdb_io_miniport_callback(void *callback_context1, void *callback_context2)
{
	cpd_dmrb_io_context_t        *  dmrb_io_context = (cpd_dmrb_io_context_t *)callback_context2;

#ifdef ALAMOSA_WINDOWS_ENV
	KeInsertQueueDpc(&(dmrb_io_context->dpc),callback_context1,callback_context2);
#else
    cpd_port_t                     *cpd_port = &cpd_port_array[dmrb_io_context->port_handle];
    csx_p_spl_lock_id(&cpd_port->cdb_io_completion_list_lock);
    csx_dlist_add_entry_tail(&cpd_port->cdb_io_completion_list, &dmrb_io_context->io_completion_list_entry); 
    csx_p_spl_unlock_id(&cpd_port->cdb_io_completion_list_lock);
    csx_p_dpc_fire(&cpd_port->cdb_io_completion_dpc);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

	return;
}


fbe_status_t 
fbe_cpd_shim_send_fis(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{
	fbe_status_t					status = FBE_STATUS_OK;	
	fbe_payload_fis_operation_t *	payload_fis_operation = NULL;
	fbe_payload_dmrb_operation_t *  payload_dmrb_operation = NULL;
	cpd_dmrb_io_context_t        *  dmrb_io_context = NULL;
	PCPD_DMRB						dmrb = NULL;
	PHYSICAL_ADDRESS				physical_address;
	fbe_u32_t						i = 0,queue_pos = 0;
	cpd_port_t					   *cpd_port = NULL;
	CPD_DMRB_STATUS					dmrb_enqueue_status = CPD_STATUS_GOOD;

	fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * pre_sg_list = NULL;
	fbe_sg_element_t * post_sg_list = NULL;
	fbe_payload_sg_descriptor_t * fis_sg_descriptor = NULL;
	fbe_u32_t						repeat_count = 0;
	fbe_payload_attr_t				payload_attr = 0;
	fbe_u64_t						physical_offset = 0;
    fbe_port_request_status_t       port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_port_recovery_status_t      port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
    fbe_u32_t                       data_transferred = 0;

	cpd_port = &(cpd_port_array[port_handle]);

	if (payload == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: payload == NULL\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
	if (payload_fis_operation == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: payload_fis_operation == NULL\n", __FUNCTION__);

		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload_dmrb_operation = fbe_payload_ex_allocate_dmrb_operation(payload);
	if (payload_dmrb_operation == NULL) {
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: payload_dmrb_operation == NULL\n", __FUNCTION__);

		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_payload_dmrb_get_memory(payload_dmrb_operation,(void **)&dmrb_io_context);
	if (FBE_STATUS_OK != status){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: error getting memory for dmrb.\n", __FUNCTION__);

        status = fbe_payload_ex_release_dmrb_operation(payload,payload_dmrb_operation);
		if (status != FBE_STATUS_OK){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: Error releasing DMRB operation. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);

		}
		cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);
		return status;

	}
    fbe_payload_ex_increment_dmrb_operation_level(payload);

	dmrb = &(dmrb_io_context->dmrb);
	fbe_zero_memory(dmrb,sizeof(CPD_DMRB));
	dmrb->revision = CPD_DMRB_REVISION;
	
	/* CPD does not define an equivalnet for FBE_PAYLOAD_CDB_FLAGS_UNSPECIFIED_DIRECTION.*/
	if(payload_fis_operation->payload_fis_flags == FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER){
		dmrb->function			= CPD_INITIATOR_NO_DATA;
	}else if(payload_fis_operation->payload_fis_flags & FBE_PAYLOAD_FIS_FLAGS_DATA_IN){
		dmrb->function			= CPD_INITIATOR_DATA_IN;
	}else if(payload_fis_operation->payload_fis_flags & FBE_PAYLOAD_FIS_FLAGS_DATA_OUT){
		dmrb->function			= CPD_INITIATOR_DATA_OUT;
	}

	dmrb->target.portal_nr = cpd_port->io_portal_number;
	dmrb->target.login_context = UIntToPtr(cpd_device_id);
	/*dmrb->target.login_qualifier = login_qualifier ;  */

	switch(payload_fis_operation->payload_fis_task_attribute){
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_SIMPLE:
			dmrb->u.sata.queue_action = CPD_SIMPLE_QUEUE_TAG;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_ORDERED:
			dmrb->u.sata.queue_action = CPD_ORDERED_QUEUE_TAG;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_HEAD_OF_QUEUE:
			dmrb->u.sata.queue_action = CPD_HEAD_OF_QUEUE_TAG;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NCQ:
			dmrb->u.sata.queue_action = CPD_SATA_NCQ_TAG;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_DMA:
			dmrb->u.sata.queue_action = CPD_SATA_DMA_UNTAGGED;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO:
			dmrb->u.sata.queue_action = CPD_SATA_PIO_UNTAGGED;
			break;
		case FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA:
			dmrb->u.sata.queue_action = CPD_SATA_NON_DATA;
			break;

		default:
			dmrb->u.sata.queue_action = CPD_SIMPLE_QUEUE_TAG;
			break;
	};

	dmrb->timeout		= (UINT_32E)payload_fis_operation->timeout/1000; /*miniport works in second world, and we get it in ms.*/

	dmrb->data_transferred = 0;	
	dmrb->sgl_offset	= 0;
	dmrb->data_length = 0;


	fbe_payload_fis_get_transfer_count(payload_fis_operation, &dmrb->data_length);

	if((dmrb->data_length == 0) &&  (dmrb->function	!= CPD_INITIATOR_NO_DATA)) {
        EmcpalDebugBreakPoint();
	}

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_ex_get_pre_sg_list(payload, &pre_sg_list);
    fbe_payload_ex_get_post_sg_list(payload, &post_sg_list);

    fbe_payload_ex_get_attr(payload, &payload_attr);
    if(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET){
        fbe_payload_ex_get_physical_offset(payload, &physical_offset);
	}

	dmrb->flags |= CPD_DMRB_FLAGS_SGL_DESCRIPTOR;
	dmrb->sgl = (struct _SGL *)dmrb_io_context->sgl_descriptor;

	fbe_payload_fis_get_sg_descriptor(payload_fis_operation, &fis_sg_descriptor);
	fbe_payload_sg_descriptor_get_repeat_count(fis_sg_descriptor, &repeat_count);

	for(i = 0 ; i < 3; i++){
		if(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET){
			dmrb_io_context->sgl_descriptor[i].sgl_ctrl.xlat_type = SGL_CONSTANT_TRANSLATION;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.physical_offset = physical_offset;
		} else {
			dmrb_io_context->sgl_descriptor[i].sgl_ctrl.xlat_type = SGL_FUNCTION_TRANSLATION;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.sgl_xlat_func.sgl_get_paddr = cpd_shim_get_phys_addr;
			dmrb_io_context->sgl_descriptor[i].sgl_xlat.sgl_xlat_func.xlat_context = NULL;
		}

		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.repeat_this_sgl = FALSE; 
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.repeat_all_sgls = FALSE; /* Valid for the first entry only */
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.skip_metadata = FALSE; 
		dmrb_io_context->sgl_descriptor[i].sgl_ctrl.last_descriptor = FALSE; 
		dmrb_io_context->sgl_descriptor[i].repeat_count = 0;
	}

	if(repeat_count > 0){
		dmrb_io_context->sgl_descriptor[0].sgl_ctrl.repeat_all_sgls = TRUE;
		dmrb_io_context->sgl_descriptor[0].repeat_count = repeat_count;
	}

    dmrb_io_context->sgl_descriptor[0].sgl_ptr = (VSGL*)pre_sg_list;
    dmrb_io_context->sgl_descriptor[1].sgl_ptr = (VSGL*)sg_list;
	/* CBE WRITE SAME */
	if(sg_list == NULL){
		dmrb_io_context->sgl_descriptor[0].sgl_ctrl.last_descriptor = TRUE; 
	}

    dmrb_io_context->sgl_descriptor[2].sgl_ptr = (VSGL*)post_sg_list;

#ifndef ALAMOSA_WINDOWS_ENV
    if(!(payload_attr & FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET)) {
        if ((dmrb_io_context->sgl_descriptor[0].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[0].sgl_ptr->virt_address != NULL))
        {
            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[0].sgl_ptr->virt_address, NULL) != 0);
        }
        if ((dmrb_io_context->sgl_descriptor[1].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[1].sgl_ptr->virt_address != NULL))
        {

            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[1].sgl_ptr->virt_address, NULL) != 0);
        }
        if ((dmrb_io_context->sgl_descriptor[2].sgl_ptr != NULL) && (dmrb_io_context->sgl_descriptor[2].sgl_ptr->virt_address != NULL))
        {
            CSX_ASSERT_H_RDC(cpd_shim_get_phys_addr((void *)dmrb_io_context->sgl_descriptor[2].sgl_ptr->virt_address, NULL) != 0);
        }
    }
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system - check for failure to convert VA to PA */
	dmrb_io_context->sgl_descriptor[2].sgl_ctrl.last_descriptor = TRUE; 


	dmrb->u.sata.fis_length = payload_fis_operation->fis_length;
	/* ASSERT(dmrb->u.sata.fis_length <= 20);*/
    EmcpalMoveMemory(&dmrb->u.sata.fis[0], &(payload_fis_operation->fis[0]), dmrb->u.sata.fis_length);	
	dmrb->u.sata.rsp_length		= FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE;
	physical_address.QuadPart = fbe_get_contigmem_physical_address((void *)(&payload_fis_operation->response_buffer[0]));
	dmrb->u.sata.rsp_buffer		= &payload_fis_operation->response_buffer[0];
	dmrb->u.sata.rsp_phys_offset = (PUCHAR)physical_address.QuadPart - payload_fis_operation->response_buffer;/**/

	dmrb_io_context->device_id   = cpd_device_id;
	dmrb_io_context->port_handle = port_handle;
	dmrb_io_context->payload     = payload;

	dmrb->callback_arg1  	= (void *)dmrb; 
	dmrb->callback_arg2	    = (void *)dmrb_io_context;
	dmrb->callback_handle   = cpd_port->miniport_callback_handle;
	dmrb->status			= 0;	

	dmrb->flags |= CPD_DMRB_FLAGS_SATA_CMD;

    if (cpd_port->mc_core_affinity_enabled){
        dmrb->callback			= (void *)fbe_cpd_shim_fis_io_completion_callback_function; /* void (* callback) (void *, void *); */
    }else{
        dmrb->callback			= (void *)fbe_cpd_shim_fis_io_miniport_callback; /* void (* callback) (void *, void *); */
#ifdef ALAMOSA_WINDOWS_ENV
	KeInitializeDpc(&(dmrb_io_context->dpc), fbe_cpd_shim_fis_io_completion_dpc_function,(PVOID)dmrb_io_context);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
    }
	/* Submit the request.*/
	dmrb_enqueue_status = cpd_port->enqueue_pending_dmrb(cpd_port->enque_pending_dmrb_context,dmrb,&queue_pos);
    if ((!cpd_port->mc_core_affinity_enabled) && (dmrb_enqueue_status == CPD_STATUS_GOOD_NOTIFY)){
        /* Needs a PROCESS_IO to kick start things.*/
        fbe_atomic_exchange(&cpd_port->send_process_io_ioctl,FBE_TRUE);
        fbe_rendezvous_event_set(&cpd_shim_ioctl_process_io_event);		
            status = FBE_STATUS_OK;
	}else if(dmrb_enqueue_status != CPD_STATUS_GOOD){
		/* Error submitting IO. */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: Error submitting IO. Port 0x%llX Payload 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port,(unsigned long long)dmrb_io_context->payload);

        data_transferred = dmrb->data_transferred;
        if (fbe_payload_ex_release_dmrb_operation(dmrb_io_context->payload,payload_dmrb_operation) != FBE_STATUS_OK){
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: Error scheduling port event DPC. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);

		}
		status = FBE_STATUS_GENERIC_FAILURE;
        port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
        switch(dmrb->status){
            case CPD_STATUS_INVALID_REQUEST:
                port_request_status = FBE_PORT_REQUEST_STATUS_INVALID_REQUEST;
                break;
            case CPD_STATUS_DEVICE_NOT_LOGGED_IN:
                port_request_status = FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN;
                break;
            case CPD_STATUS_DEVICE_BUSY:
                port_request_status = FBE_PORT_REQUEST_STATUS_BUSY;
                break;
            case CPD_STATUS_INCIDENTAL_ABORT:
                port_request_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
                //port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
                break;
            default:
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Unknown status received from miniport driver. Port 0x%p Status 0x%x\n",__FUNCTION__,cpd_port,dmrb->status);

                port_request_status = FBE_PORT_REQUEST_STATUS_ERROR;
                break;
        }

        fbe_payload_fis_set_transferred_count(payload_fis_operation,data_transferred);
        fbe_payload_fis_set_request_status(payload_fis_operation,port_request_status);//FBE_STATUS_IO_FAILED_RETRYABLE ?	
        fbe_payload_fis_set_recovery_status(payload_fis_operation,port_recovery_status);      
        
        cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
	}
    return status;
}

static void fbe_cpd_shim_fis_io_miniport_callback(void *callback_context1, void *callback_context2)
{
	cpd_dmrb_io_context_t        *  dmrb_io_context = (cpd_dmrb_io_context_t *)callback_context2;

#ifdef ALAMOSA_WINDOWS_ENV
	KeInsertQueueDpc(&(dmrb_io_context->dpc),callback_context1,callback_context2);
#else
    cpd_port_t                     *cpd_port = &cpd_port_array[dmrb_io_context->port_handle];
    csx_p_spl_lock_id(&cpd_port->fis_io_completion_list_lock);
    csx_dlist_add_entry_tail(&cpd_port->fis_io_completion_list, &dmrb_io_context->io_completion_list_entry); 
    csx_p_spl_unlock_id(&cpd_port->fis_io_completion_list_lock);
    csx_p_dpc_fire(&cpd_port->fis_io_completion_dpc);
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

	return;
}

#ifndef ALAMOSA_WINDOWS_ENV
static VOID
fbe_cpd_shim_fis_io_completion_dpc_function_wrapper(csx_p_dpc_context_t context)
{
    cpd_port_t *cpd_port = (cpd_port_t *)context;
    cpd_dmrb_io_context_t *dmrb_io_context;

    csx_p_spl_lock_id(&cpd_port->fis_io_completion_list_lock);
    dmrb_io_context = CSX_DLIST_REMOVE_STRUCTURE_AT_HEAD(&cpd_port->fis_io_completion_list, cpd_dmrb_io_context_t, io_completion_list_entry);   
    while (CSX_NOT_NULL(dmrb_io_context)) {
        csx_p_spl_unlock_id(&cpd_port->fis_io_completion_list_lock);
	fbe_cpd_shim_fis_io_completion_dpc_function(NULL, (PVOID) dmrb_io_context, NULL, NULL);
        csx_p_spl_lock_id(&cpd_port->fis_io_completion_list_lock);
        dmrb_io_context = CSX_DLIST_REMOVE_STRUCTURE_AT_HEAD(&cpd_port->fis_io_completion_list, cpd_dmrb_io_context_t, io_completion_list_entry);   
    }
    csx_p_spl_unlock_id(&cpd_port->fis_io_completion_list_lock);
}
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */

static VOID  
fbe_cpd_shim_fis_io_completion_callback_function(IN PVOID  DeferredContext,IN PVOID  SystemArgument1)
{	
    fbe_status_t	status = FBE_STATUS_OK;
    cpd_dmrb_io_context_t			*dmrb_io_context = (cpd_dmrb_io_context_t*)DeferredContext;
    fbe_payload_fis_operation_t		*payload_fis_operation = NULL;
    fbe_payload_dmrb_operation_t	*payload_dmrb_operation = NULL;
    PCPD_DMRB						dmrb = NULL;
    cpd_port_t						*cpd_port = NULL;
    fbe_port_request_status_t port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_port_recovery_status_t port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_cpd_device_id_t             device_id;
    fbe_u32_t                       data_transferred = 0;

    cpd_port = &(cpd_port_array[dmrb_io_context->port_handle]);
    dmrb = &(dmrb_io_context->dmrb);
    payload = dmrb_io_context->payload;
    device_id = dmrb_io_context->device_id;
    data_transferred = dmrb->data_transferred;

	switch(dmrb->status){
		case CPD_STATUS_GOOD:
			port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
			break;
		case CPD_STATUS_INVALID_REQUEST:
			port_request_status = FBE_PORT_REQUEST_STATUS_INVALID_REQUEST;
			break;
		case CPD_STATUS_INSUFFICIENT_RESOURCES:
			port_request_status = FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES;
			break;
		case CPD_STATUS_DEVICE_NOT_LOGGED_IN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN;
			break;
		case CPD_STATUS_DEVICE_BUSY:
			port_request_status = FBE_PORT_REQUEST_STATUS_BUSY;
			break;
		case CPD_STATUS_BAD_REPLY:
			port_request_status = FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR;
			break;
		case CPD_STATUS_TIMEOUT:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
			break;
		case CPD_STATUS_DEVICE_NOT_RESPONDING:
			port_request_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
			break;
		case CPD_STATUS_LINK_FAIL:
			port_request_status = FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR;
			break;
		case CPD_STATUS_DATA_OVERRUN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DATA_OVERRUN;
			break;
		case CPD_STATUS_DATA_UNDERRUN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN;
			break;
		case CPD_STATUS_ABORTED_BY_UPPER_LEVEL:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE;
			break;
		case CPD_STATUS_INCIDENTAL_ABORT:
			port_request_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
			break;
		case CPD_STATUS_ABORTED_BY_DEVICE:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE;
			break;
		case CPD_STATUS_SATA_REJECTED_NCQ_MODE:
			port_request_status = FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE;
			break;
		case CPD_STATUS_SATA_NCQ_ERROR:
			port_request_status = FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR;
			break;
		default:
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Unknown status received from miniport driver. Port 0x%llX Status 0x%x\n",__FUNCTION__,(unsigned long long)cpd_port,dmrb->status);

			port_request_status = FBE_PORT_REQUEST_STATUS_ERROR;
			break;
	}

    switch(dmrb->recovery_status)
    {
		case CPD_STATUS_NO_RECOVERY_PERFORMED:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
            break;
        case CPD_STATUS_GOOD:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_SUCCESS;
            break;
        case CPD_STATUS_TIMEOUT:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_TIMED_OUT;
            break;
        case CPD_STATUS_BAD_REPLY:
        case CPD_STATUS_DEVICE_NOT_RESPONDING:
        case CPD_STATUS_LINK_FAIL:
        case CPD_STATUS_ABORTED_BY_DEVICE:
        case CPD_STATUS_INVALID_REQUEST:
        case CPD_STATUS_SATA_REJECTED_NCQ_MODE:
        case CPD_STATUS_INCIDENTAL_ABORT:
        case CPD_STATUS_SATA_NCQ_ERROR:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
            break;
        default:
		/*KvTrace("%s Unknown status received from miniport driver. Port 0x%llX Status 0x%x\n",__FUNCTION__,cpd_port,dmrb->status,0);*/
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Unknown status received from miniport driver. Port 0x%llX Recovery Status 0x%x\n",__FUNCTION__,(unsigned long long)cpd_port,dmrb->recovery_status);

		port_recovery_status = FBE_PORT_RECOVERY_STATUS_ERROR;
		break;

    }
    payload_dmrb_operation = fbe_payload_ex_get_dmrb_operation(dmrb_io_context->payload);
    status = fbe_payload_ex_release_dmrb_operation(dmrb_io_context->payload, payload_dmrb_operation);
	if (status != FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Error releasing DMRB operation. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);

	}
    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
    fbe_payload_fis_set_transferred_count(payload_fis_operation,data_transferred);
	fbe_payload_fis_set_request_status(payload_fis_operation,port_request_status);	
    fbe_payload_fis_set_recovery_status(payload_fis_operation,port_recovery_status);

    cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
    return;
}

#ifdef ALAMOSA_WINDOWS_ENV
static VOID  fbe_cpd_shim_fis_io_completion_dpc_function(IN struct _KDPC  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 )
#else
static VOID  fbe_cpd_shim_fis_io_completion_dpc_function(IN PVOID Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 )
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
{	
    fbe_cpd_shim_fis_io_completion_callback_function(DeferredContext,SystemArgument1);
	return;
}


#ifndef ALAMOSA_WINDOWS_ENV
static VOID
fbe_cpd_shim_cdb_io_completion_dpc_function_wrapper(csx_p_dpc_context_t context)
{
    cpd_port_t *cpd_port = (cpd_port_t *)context;
    cpd_dmrb_io_context_t *dmrb_io_context;

    csx_p_spl_lock_id(&cpd_port->cdb_io_completion_list_lock);
    dmrb_io_context = CSX_DLIST_REMOVE_STRUCTURE_AT_HEAD(&cpd_port->cdb_io_completion_list, cpd_dmrb_io_context_t, io_completion_list_entry);   
    while (CSX_NOT_NULL(dmrb_io_context)) {
        csx_p_spl_unlock_id(&cpd_port->cdb_io_completion_list_lock);
        fbe_cpd_shim_cdb_io_completion_dpc_function(NULL, (PVOID) dmrb_io_context, NULL, NULL);
       csx_p_spl_lock_id(&cpd_port->cdb_io_completion_list_lock);
	dmrb_io_context = CSX_DLIST_REMOVE_STRUCTURE_AT_HEAD(&cpd_port->cdb_io_completion_list, cpd_dmrb_io_context_t, io_completion_list_entry);   
    }
    csx_p_spl_unlock_id(&cpd_port->cdb_io_completion_list_lock);
}
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */


static VOID  
fbe_cpd_shim_cdb_io_completion_callback_function(IN PVOID  DeferredContext,IN PVOID  SystemArgument1)
{

	fbe_status_t	status;
	cpd_dmrb_io_context_t			*dmrb_io_context = (cpd_dmrb_io_context_t*)DeferredContext;
	fbe_payload_cdb_operation_t		*payload_cdb_operation = NULL;
	PCPD_DMRB						dmrb = NULL;
	cpd_port_t						*cpd_port = NULL;
	fbe_payload_cdb_scsi_status_t	 payload_scsi_status;
	fbe_port_request_status_t port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_port_recovery_status_t port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
    fbe_payload_dmrb_operation_t	*payload_dmrb_operation = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_cpd_device_id_t             device_id;
    fbe_u32_t                       data_transferred = 0;

#if 0
    EMCPAL_LARGE_INTEGER					 performance_frequency;
    EMCPAL_LARGE_INTEGER					 performance_counter;
#endif

    

	cpd_port = &(cpd_port_array[dmrb_io_context->port_handle]);
	dmrb = &(dmrb_io_context->dmrb);
    payload = dmrb_io_context->payload;
    device_id = dmrb_io_context->device_id;

    data_transferred = dmrb->data_transferred;
	
	switch(dmrb->u.scsi.scsi_status){
		case SCSISTAT_GOOD:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;			
			break;
		case SCSISTAT_CHECK_CONDITION:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
			break;
		case SCSISTAT_CONDITION_MET:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CONDITION_MET;
			break;
		case SCSISTAT_BUSY:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY;
			break;
		case SCSISTAT_INTERMEDIATE:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE;
			break;
		case SCSISTAT_INTERMEDIATE_COND_MET:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE_CONDITION_MET;
			break;
		case SCSISTAT_RESERVATION_CONFLICT:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT;
			break;
		case SCSISTAT_COMMAND_TERMINATED:
		case SCSISTAT_TASK_ABORTED:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_ABORTED;
			break;
		case SCSISTAT_QUEUE_FULL:
                        payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_SET_FULL;
                        break;
		case SCSISTAT_ACA_ACTIVE:
                        payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_ACA_ACTIVE;
                        break;
		default:
			payload_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;/* ??*/
			break;
	}	

	switch(dmrb->status){
		case CPD_STATUS_GOOD:
			port_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
			break;
		case CPD_STATUS_INVALID_REQUEST:
			port_request_status = FBE_PORT_REQUEST_STATUS_INVALID_REQUEST;
			break;
		case CPD_STATUS_INSUFFICIENT_RESOURCES:
			port_request_status = FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES;
			break;
		case CPD_STATUS_DEVICE_NOT_LOGGED_IN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN;
			break;
		case CPD_STATUS_DEVICE_BUSY:
			port_request_status = FBE_PORT_REQUEST_STATUS_BUSY;
			break;
		case CPD_STATUS_BAD_REPLY:
			port_request_status = FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR;
			break;
		case CPD_STATUS_TIMEOUT:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
			break;
		case CPD_STATUS_DEVICE_NOT_RESPONDING:
			port_request_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
			break;
		case CPD_STATUS_LINK_FAIL:
			port_request_status = FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR;
			break;
		case CPD_STATUS_DATA_OVERRUN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DATA_OVERRUN;
			break;
		case CPD_STATUS_DATA_UNDERRUN:
			port_request_status = FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN;
			break;
		case CPD_STATUS_ABORTED_BY_UPPER_LEVEL:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE;
			break;
		case CPD_STATUS_INCIDENTAL_ABORT:
			port_request_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
			break;
		case CPD_STATUS_ABORTED_BY_DEVICE:
			port_request_status = FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE;
			break;
        case CPD_STATUS_ENCRYPTION_NOT_ENABLED:
			port_request_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED;
			break;
		case CPD_STATUS_ENCRYPTION_KEY_NOT_FOUND:
			port_request_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE;
			break;
		case CPD_STATUS_ENCRYPTION_KEY_ERROR:
			port_request_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR;
			break;
		default:
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"IO Comp:Unknown status from miniport.Port:0x%llX Status:0x%x\n",(unsigned long long)cpd_port,dmrb->status);

			port_request_status = FBE_PORT_REQUEST_STATUS_ERROR;
			break;
	}

    switch(dmrb->recovery_status)
    {
        case CPD_STATUS_NO_RECOVERY_PERFORMED:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED;
            break;
        case CPD_STATUS_GOOD:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_SUCCESS;
            break;
        case CPD_STATUS_TIMEOUT:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_TIMED_OUT;
            break;
        case CPD_STATUS_INVALID_REQUEST: 
        case CPD_STATUS_UNSUPPORTED:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_COMPLETED_WITH_ERROR;
            break;
        case CPD_STATUS_BAD_REPLY:
        case CPD_STATUS_DEVICE_NOT_RESPONDING:
        case CPD_STATUS_LINK_FAIL:
        case CPD_STATUS_ABORTED_BY_DEVICE:
        case CPD_STATUS_INCIDENTAL_ABORT:
            port_recovery_status = FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR;
            break;
        default:
            /*KvTrace("%s Unknown status received from miniport driver. Port 0x%llx Status 0x%x\n",__FUNCTION__,(unsigned long long)cpd_port,dmrb->status,0);*/
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "IO Comp. Unknown recovery from miniport. Port:0x%llX Recovery:0x%x\n",(unsigned long long)cpd_port,dmrb->recovery_status);

            port_recovery_status = FBE_PORT_RECOVERY_STATUS_ERROR;
            break;
    }
    payload_dmrb_operation = fbe_payload_ex_get_dmrb_operation(payload);    
    status = fbe_payload_ex_release_dmrb_operation(payload, payload_dmrb_operation );
    if (status != FBE_STATUS_OK){
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Error releasing DMRB operation. Port 0x%llX\n",__FUNCTION__,(unsigned long long)cpd_port);
    }

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if (payload_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD){
	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s  IO Status = %X Payload: %X CPD ID: %X\n", __FUNCTION__,  
                               payload_scsi_status, (unsigned int)payload, device_id );

    }

    fbe_payload_cdb_set_transferred_count(payload_cdb_operation,data_transferred);
    fbe_payload_cdb_set_scsi_status(payload_cdb_operation,payload_scsi_status);
    fbe_payload_cdb_set_request_status(payload_cdb_operation,port_request_status);    
    fbe_payload_cdb_set_recovery_status(payload_cdb_operation,port_recovery_status);
    if ((port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS) &&
         port_request_status != FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN){ /* It's too noisy */
         fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s  CDB Status = %X Payload: %X CPD ID: %X\n", __FUNCTION__,  
                                port_request_status, (unsigned int)payload, device_id );

    }

#if 0
	performance_counter = KeQueryPerformanceCounter(&performance_frequency);
	payload_cdb_operation->time_stamp = (performance_counter.QuadPart / (performance_frequency.QuadPart / (fbe_u64_t)1000)) - payload_cdb_operation->time_stamp; /* Elapsed time in ms */
#endif

    cpd_port->payload_completion_function(payload, cpd_port->payload_completion_context);	
    return;
}

#ifdef ALAMOSA_WINDOWS_ENV
static VOID  fbe_cpd_shim_cdb_io_completion_dpc_function(IN struct _KDPC  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 )
#else
static VOID  fbe_cpd_shim_cdb_io_completion_dpc_function(IN PVOID  *Dpc,IN PVOID  DeferredContext,IN PVOID  SystemArgument1,IN PVOID  SystemArgument2 )
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
{	
    fbe_cpd_shim_cdb_io_completion_callback_function(DeferredContext,SystemArgument1);
	return;
}

/***************************************************************************
 * 
 * Routine Description: 
 * 
 *   Fills in a CPD_IOCTL_HEADER structure with some default values.
 * 
 * Arguments: 
 * 
 *   io_control - Pointer to a SRB_IO_CONTROL structure to fill in.
 *
 *
 ***************************************************************************/
static fbe_status_t
cpd_shim_build_default_io_control_header(CPD_IOCTL_HEADER * io_control)
{
    if (io_control == NULL) {
		/* Put some trace here */
		return FBE_STATUS_GENERIC_FAILURE;
	}

    CPD_IOCTL_HEADER_INIT(io_control, 0, 10, 0);
    return FBE_STATUS_OK;
} 

/***************************************************************************
 * 
 * Routine Description: 
 * 
 *   Sends a MiniPort-Specific IOCTL to the MiniPort.
 * 
 * Arguments: 
 * 
 *   pPortDeviceObject - Miniport device object
 *
 *   pIoCtl - Details of the message.
 *
 *   Size   - Size in bytes of the message.
 *
 *   InternalDeviceIoctl - Set to TRUE for most CPD ioctls (see note in code)
 *
 * Return Value: 
 * 
 *   NT Status.
 *
 * Note: Must be running at IRQL_PASSIVE_LEVEL.
 *
 * Revision History:
 *
 *   09/24/03  MWH  Now panic in timeout cases - most callers already
 *                  panicked if this function returned an unsuccessful status.
 *   02/04/04  MWH  Added InternalDeviceIoctl parameter.
 *   06/29/04  MWH  Now OR SRB_FLAGS_NO_QUEUE_FREEZE to SrbFlags.
 *	10-Oct-07 . Peter Puhov - Copied from MirrorSendIoctl function
 *
 ***************************************************************************/

static fbe_status_t 
fbe_cpd_shim_send_cpd_ext_ioctl(IN PEMCPAL_DEVICE_OBJECT  pPortDeviceObject,
								/*IN fbe_u32_t   io_portal_number,*/
								IN VOID           *pIoCtl,
								IN ULONG          Size,
								IN BOOLEAN        InternalDeviceIoctl)
{
    PEMCPAL_IRP                     pIrp = NULL;
    CPD_EXT_SRB             *pCpdSrb = NULL;
    EMCPAL_IRP_STATUS_BLOCK         IoStatus;
    fbe_rendezvous_event_t             Event;
    EMCPAL_STATUS           nt_status;

    VOID* pOutputBuffer;
    ULONG OutputBufferLength;

    if ((pPortDeviceObject==NULL) || (pIoCtl==NULL)){
		/* Add some trace here */
		return FBE_STATUS_GENERIC_FAILURE;
	}

    pCpdSrb = fbe_allocate_nonpaged_pool_with_tag(sizeof (CPD_EXT_SRB), FBE_CPD_SHIM_MEMORY_TAG);
    if (pCpdSrb == NULL) {
		/* Add some trace here */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Clear SRB and copy Ioctl */
    fbe_zero_memory (pCpdSrb, sizeof(CPD_EXT_SRB));

    cpd_os_io_set_length(&pCpdSrb->srb, sizeof(CPD_SCSI_REQ_BLK));
	cpd_os_io_set_command(&pCpdSrb->srb, SRB_FUNCTION_IO_CONTROL);
	cpd_os_io_set_flags(&pCpdSrb->srb, SRB_FLAGS_FLARE_CMD | SRB_FLAGS_NO_QUEUE_FREEZE);
	cpd_os_io_set_xfer_len(&pCpdSrb->srb, Size);
	cpd_os_io_set_native_time_out(&pCpdSrb->srb, 20);
	cpd_os_io_set_data_buf(&pCpdSrb->srb, pIoCtl);

    /* Miniport knows about these, but SCSI Port does not. */
    pCpdSrb->ext_flags               = CPD_EXT_SRB_UNSPECIFIED_DIRECTION | CPD_EXT_SRB_IS_EXT_SRB;
    pCpdSrb->time_out_value          = 10; // special case

    fbe_rendezvous_event_init(&Event);

    /* Testing has shown that InternalDeviceIoctl determines how our input
     * buffer is updated by SCSIPORT/miniport.
     *
     * If TRUE, our input buffer will be modified regardless of how the
     * miniport completes the IOCTL (success or failure).  We must set 
     * pOutputBuffer to NULL and OutputBufferLength to 0.
     * Required for CPD_IOCTL_ADD_DEVICE only, since we need to check for 
     * "drive already logged in" if IOCTL is failed by miniport.
     *
     * If FALSE, our input buffer will be modified ONLY if the IOCTL is
     * successfully completed by the miniport.  We must set pOutputBuffer
     * and OutputBufferLength to the same values as our input parameters.
     * This is our traditional approach, but it is not valid for 
     * CPD_IOCTL_ADD_DEVICE.
     */
    if (InternalDeviceIoctl) {
        pOutputBuffer = NULL;
        OutputBufferLength = 0;
    }
    else {
        pOutputBuffer = pIoCtl;
        OutputBufferLength = Size;
    }

    /* must be running at IRQL_PASSIVE_LEVEL */
    pIrp = EmcpalExtIrpBuildIoctl(
        IOCTL_SCSI_MINIPORT,    // IN ULONG             IoControlCode
        pPortDeviceObject,      // IN PDEVICE_OBJECT    DeviceObject
        pIoCtl,                 // IN PVOID             InputBuffer
        Size,                   // IN ULONG             InputBufferLength
        pOutputBuffer,          // IN PVOID             OutputBuffer
        OutputBufferLength,     // IN ULONG             OutputBufferLength
        InternalDeviceIoctl,    // IN BOOLEAN           InternalDeviceIoCtl
        fbe_rendezvous_event_get_internal_event(&Event), // IN PKEVENT           Event
        &IoStatus);             // OUT PIO_STATUS_BLOCK IoStatusBlock

    if (pIrp == NULL) {
        /* Add trace here */
        fbe_release_nonpaged_pool(pCpdSrb);
		fbe_rendezvous_event_destroy(&Event);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    cpd_os_io_set_original_request(&pCpdSrb->srb, pIrp);

    /* send this request to the miniport */
    nt_status = EmcpalExtIrpSendAsync(pPortDeviceObject, pIrp);

    /* Wait for synchronous requests. */
    if (nt_status == EMCPAL_STATUS_PENDING) {

        /* The idea here is that the SRB should timeout within
         * NTMIRROR_SRB_TIMEOUT (20)seconds.  By setting out timeout period to be
         * twice that long (negative for relative time) we'll catch cases
         * where the IRP gets stuck somewhere.
         */

		/* #define TicksPerSecond  (10000000L)   1 tick = 100 ns defined in asidc_misc.h, mirror_misc.h etc. */
        nt_status = fbe_rendezvous_event_wait(&Event, 40000);

        if (nt_status == EMCPAL_STATUS_TIMEOUT) {
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Critical error CPD Srb Timed out \n", __FUNCTION__);


            fbe_release_nonpaged_pool(pCpdSrb);
			fbe_rendezvous_event_destroy(&Event);
			return FBE_STATUS_GENERIC_FAILURE;
        }
        else {
            nt_status = EmcpalExtIrpStatusBlockStatusFieldGet(&IoStatus);
        }
    }
    nt_status = CPD_IOCTL_STATUS(nt_status, (CPD_IOCTL_HEADER *)pIoCtl);

    fbe_release_nonpaged_pool(pCpdSrb);

    if(EMCPAL_IS_SUCCESS(nt_status)){
		fbe_rendezvous_event_destroy(&Event);
		return FBE_STATUS_OK;
	} else {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Code:0x%08x failed, Status:0x%08x \n", 
                               __FUNCTION__,
                               ((SRB_IO_CONTROL *)pIoCtl)->ControlCode,
                               ((SRB_IO_CONTROL *)pIoCtl)->ReturnCode);
		fbe_rendezvous_event_destroy(&Event);
		return FBE_STATUS_GENERIC_FAILURE;
	}
}

/***************************************************************************
 * 
 * Routine Description: 
 * 
 *      This function will register DMD callback.
 * 
 * Arguments: 
 * 
 *      pDriverObject - pointer to the Device Object
 *      pPortDeviceObject - pointer to the first PortDeviceObject claimed
 *
 * Return Value: 
 *
 *      TRUE - if Async Data updated & callback registered
 *      FALSE - if Async Data not update or callback not registered
 *
 * Revision History:
 *   03/03/03  JAP      Created.
 *	24-Oct-07  Peter Puhov - Copied from MirrorDiscoveryInitCallback function
 ***************************************************************************/
static fbe_status_t
fbe_cpd_shim_send_register_callback (fbe_u32_t  port_handle, fbe_u32_t cpd_shim_registration_flags)
{
    fbe_status_t status;
    S_CPD_REGISTER_CALLBACKS * io_ctl;
    fbe_u32_t   cpd_registration_flags = 0;

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* TEMP 2* */
    io_ctl = fbe_allocate_nonpaged_pool_with_tag(2*sizeof(S_CPD_REGISTER_CALLBACKS), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (io_ctl, sizeof(S_CPD_REGISTER_CALLBACKS));

    cpd_registration_flags = 0;
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR){
        cpd_registration_flags |= CPD_REGISTER_INITIATOR;
    }
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_TARGET){
        cpd_registration_flags |= CPD_REGISTER_TARGET_USING_DMRBS;/* TODO: Update this when we implement Trgt mode.*/
    }
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_NOTIFY_EXISTING_LOGINS){
        cpd_registration_flags |= CPD_REGISTER_NOTIFY_EXISTING_LOGINS;
    }
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS){
        cpd_registration_flags |= CPD_REGISTER_SFP_EVENTS;
    }
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_FILTER_INTERCEPT){
        cpd_registration_flags |= CPD_REGISTER_FILTER_INTERCEPT;
    } 
    if (cpd_shim_registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_ENCRYPTION) {
        cpd_registration_flags |= CPD_REGISTER_ENCRYPTION;
    }
    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);
    
    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_REGISTER_CALLBACKS);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_REGISTER_CALLBACKS));
    
	io_ctl->param.portal_nr		   =  cpd_port_array[port_handle].io_portal_number;
    io_ctl->param.context          = &cpd_port_array[port_handle];
    io_ctl->param.async_callback   = fbe_cpd_shim_callback;
    io_ctl->param.callback_handle  = NULL;
    io_ctl->param.flags            = cpd_registration_flags;/*CPD_REGISTER_INITIATOR | CPD_REGISTER_NOTIFY_EXISTING_LOGINS;*/
    io_ctl->param.target_callbacks.dmrb.get_free_dmrb         = NULL;
    io_ctl->param.target_callbacks.dmrb.process_io            = NULL;
    io_ctl->param.target_callbacks.dmrb.enqueue_pending_dmrb  = NULL;

    /* Send IOCTL to miniport via SCSIPORT. */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
													io_ctl, 
													sizeof (S_CPD_REGISTER_CALLBACKS), 
													FALSE);
    if (status == FBE_STATUS_OK) {
		
        if (io_ctl->param.target_callbacks.dmrb.enqueue_pending_dmrb == NULL) {
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Register Callback failed \n", __FUNCTION__);

			status = FBE_STATUS_GENERIC_FAILURE;
		}else{
            cpd_port_array[port_handle].process_io = io_ctl->param.target_callbacks.dmrb.process_io;
			cpd_port_array[port_handle].enqueue_pending_dmrb = io_ctl->param.target_callbacks.dmrb.enqueue_pending_dmrb;
			cpd_port_array[port_handle].enque_pending_dmrb_context = io_ctl->param.target_callbacks.dmrb.miniport_context;
			cpd_port_array[port_handle].miniport_callback_handle = io_ctl->param.callback_handle;
			fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Registered CallbackHandle %X\n", __FUNCTION__, (unsigned int)cpd_port_array[port_handle].miniport_callback_handle);

		}
	}

    fbe_release_nonpaged_pool(io_ctl);
	return status;
}



/***************************************************************************
 * 
 * Routine Description: 
 * 
 *   Disable Asynchronous Event reporting.
 ***************************************************************************/
static fbe_status_t
fbe_cpd_shim_send_unregister_callback(fbe_u32_t  port_handle)
{
    fbe_status_t                status;
    S_CPD_REGISTER_CALLBACKS    * io_ctl = NULL;

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* TEMP 2* */
    io_ctl = fbe_allocate_nonpaged_pool_with_tag(2*sizeof(S_CPD_REGISTER_CALLBACKS), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_REGISTER_CALLBACKS);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_REGISTER_CALLBACKS));

    io_ctl->param.portal_nr		   = cpd_port_array[port_handle].io_portal_number;
    io_ctl->param.callback_handle  = cpd_port_array[port_handle].miniport_callback_handle;
    io_ctl->param.flags            = CPD_REGISTER_DEREGISTER;

    /* Send IOCTL to miniport via SCSIPORT. */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
													io_ctl, 
													sizeof (S_CPD_REGISTER_CALLBACKS), 
													FALSE);

	if(status == FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Unregistered CallbackHandle %X\n", __FUNCTION__, (unsigned int)cpd_port_array[port_handle].miniport_callback_handle);

		cpd_port_array[port_handle].miniport_callback_handle = NULL;
	}

    fbe_release_nonpaged_pool(io_ctl);
	return status;
}

fbe_status_t 
fbe_cpd_shim_get_hardware_info(fbe_u32_t port_handle,
                               fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t status;

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    if (hdw_info == NULL){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  hdw_info ptr %X\n", __FUNCTION__,  (unsigned int)hdw_info);
		return FBE_STATUS_GENERIC_FAILURE;
    }

    status = cpd_shim_get_hardware_info(cpd_port_array[port_handle].miniport_device_object,hdw_info);

    return status;
}

fbe_status_t fbe_cpd_shim_set_encryption_mode(fbe_u32_t port_handle, 
                                              fbe_port_encryption_mode_t mode)
{
    fbe_status_t   status;
    S_CPD_ENC_KEY_MGMT * s_cpd_key_mgmt;     
    CPD_ENC_MODE enc_mode;

    if(mode == FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS) {
        enc_mode = CPD_ENC_WRAPPED_DEKS;
    }
    else if (mode == FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS){
        enc_mode = CPD_ENC_WRAPPED_KEKS_DEKS;
    }
    else {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Incorrect mode :%d\n", __FUNCTION__, mode );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    s_cpd_key_mgmt = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_ENC_KEY_MGMT), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_key_mgmt == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_key_mgmt, sizeof(S_CPD_ENC_KEY_MGMT));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_key_mgmt->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_key_mgmt->ioctl_hdr, CPD_IOCTL_ENC_KEY_MGMT);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_key_mgmt->ioctl_hdr, sizeof(CPD_ENC_KEY_MGMT));
  
    s_cpd_key_mgmt->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    s_cpd_key_mgmt->param.operation = CPD_ENC_SET_MODE;
    
    s_cpd_key_mgmt->param.enc_mode = enc_mode;
    
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_key_mgmt, 
                                             sizeof(S_CPD_ENC_KEY_MGMT), FALSE);
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Setting to Wrapped DEKs and KEKs mode failed %X\n", __FUNCTION__, status);
    }
    fbe_release_nonpaged_pool(s_cpd_key_mgmt);
    return status;
}
fbe_status_t fbe_cpd_shim_register_data_encryption_keys(fbe_u32_t port_handle, 
                                                        fbe_base_port_mgmt_register_keys_t * port_register_keys_p)
{

    return fbe_cpd_shim_register_encryption_keys(port_handle,
                                                 CPD_ENC_DEK,
                                                 CPD_ENC_SET_KEY,
                                                 CPD_ENC_WRAPPED,
                                                 port_register_keys_p,
                                                 NULL);
}

fbe_status_t fbe_cpd_shim_register_kek(fbe_u32_t port_handle, 
                                       fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                       void *port_kek_context)
{
    return fbe_cpd_shim_register_encryption_keys(port_handle,
                                                 CPD_ENC_KEK,
                                                 CPD_ENC_SET_KEY,
                                                 CPD_ENC_WRAPPED,
                                                 port_register_keys_p,
                                                 port_kek_context);
}

fbe_status_t fbe_cpd_shim_register_kek_kek(fbe_u32_t port_handle, 
                                           fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                           void *port_kek_kek_context)
{
    return fbe_cpd_shim_register_encryption_keys(port_handle,
                                                 CPD_ENC_KEK,
                                                 CPD_ENC_SET_AND_PERSIST_KEY,
                                                 CPD_ENC_PLAINTEXT,
                                                 port_register_keys_p,
                                                 port_kek_kek_context);
}

static fbe_status_t fbe_cpd_shim_register_encryption_keys(fbe_u32_t port_handle, 
                                                          fbe_u8_t target,
                                                          fbe_u8_t operation,
                                                          fbe_u8_t format,
                                                          fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                                          void *context)
{
    fbe_status_t   status;
    S_CPD_ENC_KEY_MGMT * s_cpd_key_mgmt;     

    s_cpd_key_mgmt = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_ENC_KEY_MGMT), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_key_mgmt == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_key_mgmt, sizeof(S_CPD_ENC_KEY_MGMT));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_key_mgmt->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_key_mgmt->ioctl_hdr, CPD_IOCTL_ENC_KEY_MGMT);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_key_mgmt->ioctl_hdr, sizeof(CPD_ENC_KEY_MGMT));

    s_cpd_key_mgmt->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    s_cpd_key_mgmt->param.target = target;
    s_cpd_key_mgmt->param.operation = operation;
    s_cpd_key_mgmt->param.format = format; 
    s_cpd_key_mgmt->param.nr_keys = port_register_keys_p->num_of_keys;
    s_cpd_key_mgmt->param.key_size = port_register_keys_p->key_size; 
    s_cpd_key_mgmt->param.key_handle_ptr = (CPD_KEY_HANDLE *)&port_register_keys_p->mp_key_handle[0];
    s_cpd_key_mgmt->param.key_ptr = (UINT_8E *)port_register_keys_p->key_ptr;
    s_cpd_key_mgmt->param.caller_context = context;
    if(format == CPD_ENC_PLAINTEXT) {
        s_cpd_key_mgmt->param.unwrap_key_handle = CPD_KEY_PLAINTEXT;
    }
    else{ 
        s_cpd_key_mgmt->param.unwrap_key_handle = (CPD_KEY_HANDLE) port_register_keys_p->kek_handle;
    }

	status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_key_mgmt, 
                                             sizeof(S_CPD_ENC_KEY_MGMT), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s failed %X\n", __FUNCTION__, status);

    }

    fbe_release_nonpaged_pool(s_cpd_key_mgmt);
    return status;
}

fbe_status_t fbe_cpd_shim_rebase_all_keys(fbe_u32_t port_handle, 
                                          fbe_key_handle_t key_handle,
                                          void *port_kek_context)
{
    fbe_status_t   status;
    S_CPD_ENC_KEY_MGMT * s_cpd_key_mgmt;     

    s_cpd_key_mgmt = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_ENC_KEY_MGMT), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_key_mgmt == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_key_mgmt, sizeof(S_CPD_ENC_KEY_MGMT));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_key_mgmt->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_key_mgmt->ioctl_hdr, CPD_IOCTL_ENC_KEY_MGMT);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_key_mgmt->ioctl_hdr, sizeof(CPD_ENC_KEY_MGMT));

    s_cpd_key_mgmt->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    s_cpd_key_mgmt->param.target = CPD_ENC_DEK;
    s_cpd_key_mgmt->param.operation = CPD_ENC_REBASE_ALL_KEYS;
    s_cpd_key_mgmt->param.format = CPD_ENC_WRAPPED; 
    s_cpd_key_mgmt->param.nr_keys = 0;
    s_cpd_key_mgmt->param.caller_context = port_kek_context;
    s_cpd_key_mgmt->param.unwrap_key_handle = (CPD_KEY_HANDLE) key_handle;
    
	status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_key_mgmt, 
                                             sizeof(S_CPD_ENC_KEY_MGMT), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s failed %X\n", __FUNCTION__, status);

    }

    fbe_release_nonpaged_pool(s_cpd_key_mgmt);
    return status;
}
fbe_status_t fbe_cpd_shim_unregister_data_encryption_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_encryption_keys(port_handle,
                                                   CPD_ENC_DEK,
                                                   CPD_ENC_WRAPPED,
                                                   CPD_ENC_INVALIDATE_KEY,
                                                   unregister_info);

}

fbe_status_t fbe_cpd_shim_unregister_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_encryption_keys(port_handle,
                                                   CPD_ENC_KEK,
                                                   CPD_ENC_WRAPPED,
                                                   CPD_ENC_INVALIDATE_KEY,
                                                   unregister_info);
}

fbe_status_t fbe_cpd_shim_unregister_kek_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_encryption_keys(port_handle,
                                                   CPD_ENC_KEK,
                                                   CPD_ENC_PLAINTEXT,
                                                   CPD_ENC_CLEAR_PERSISTED_KEYS_CTLR_WIDE,
                                                   unregister_info);
}

fbe_status_t fbe_cpd_shim_reestablish_key_handle(fbe_u32_t port_handle, fbe_base_port_mgmt_reestablish_key_handle_t * key_handle_info)
{
    fbe_status_t   status;
    S_CPD_ENC_KEY_MGMT * s_cpd_key_mgmt;     

    s_cpd_key_mgmt = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_ENC_KEY_MGMT), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_key_mgmt == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_key_mgmt, sizeof(S_CPD_ENC_KEY_MGMT));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_key_mgmt->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_key_mgmt->ioctl_hdr, CPD_IOCTL_ENC_KEY_MGMT);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_key_mgmt->ioctl_hdr, sizeof(CPD_ENC_KEY_MGMT));	

    s_cpd_key_mgmt->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    s_cpd_key_mgmt->param.target = CPD_ENC_KEK;
    s_cpd_key_mgmt->param.operation = CPD_ENC_REESTABLISH_KEY;
    s_cpd_key_mgmt->param.format = CPD_ENC_PLAINTEXT; 
    s_cpd_key_mgmt->param.nr_keys = 1;
    s_cpd_key_mgmt->param.key_handle_ptr = (CPD_KEY_HANDLE *)&key_handle_info->mp_key_handle;

	status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_key_mgmt, 
                                             sizeof(S_CPD_ENC_KEY_MGMT), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s failed %X\n", __FUNCTION__, status);

    }

    fbe_release_nonpaged_pool(s_cpd_key_mgmt);
    return status;
}
static fbe_status_t fbe_cpd_shim_unregister_encryption_keys(fbe_u32_t port_handle, 
                                                            fbe_u8_t target,
                                                            fbe_u8_t format,
                                                            fbe_u8_t operation,
                                                            fbe_base_port_mgmt_unregister_keys_t * unregister_info)
{
    fbe_status_t   status;
    S_CPD_ENC_KEY_MGMT * s_cpd_key_mgmt;     

    s_cpd_key_mgmt = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_ENC_KEY_MGMT), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_key_mgmt == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_key_mgmt, sizeof(S_CPD_ENC_KEY_MGMT));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_key_mgmt->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_key_mgmt->ioctl_hdr, CPD_IOCTL_ENC_KEY_MGMT);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_key_mgmt->ioctl_hdr, sizeof(CPD_ENC_KEY_MGMT));	

    s_cpd_key_mgmt->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    s_cpd_key_mgmt->param.target = target;
    s_cpd_key_mgmt->param.operation = operation;
    s_cpd_key_mgmt->param.format = format; 
    s_cpd_key_mgmt->param.nr_keys = unregister_info->num_of_keys;
    s_cpd_key_mgmt->param.key_handle_ptr = (CPD_KEY_HANDLE *)&unregister_info->mp_key_handle[0];

	status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_key_mgmt, 
                                             sizeof(S_CPD_ENC_KEY_MGMT), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s failed %X\n", __FUNCTION__, status);

    }

    fbe_release_nonpaged_pool(s_cpd_key_mgmt);
    return status;
}

fbe_status_t fbe_cpd_shim_get_port_name_info(fbe_u32_t port_handle, fbe_cpd_shim_port_name_info_t *port_name_info)
{
    fbe_status_t   status;
    S_CPD_NAME * s_cpd_name;     

    s_cpd_name = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_NAME), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_name == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_name, sizeof(S_CPD_NAME));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_name->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_name->ioctl_hdr, CPD_IOCTL_GET_NAME);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_name->ioctl_hdr, sizeof(CPD_NAME));

    s_cpd_name->param.portal_nr = cpd_port_array[port_handle].io_portal_number;
   
	status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object, 
                                             s_cpd_name, 
                                             sizeof(S_CPD_NAME), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s failed %X\n", __FUNCTION__, status);

    }
    else
    {
        port_name_info->sas_address = s_cpd_name->param.name.sas.sas_address;
    }

    fbe_release_nonpaged_pool(s_cpd_name);
    return status;
}

static fbe_status_t 
cpd_shim_get_hardware_info(IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject,
                               fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t   status;
    S_CPD_HARDWARE_INFO * s_cpd_hardware_info;     

    if (pPortDeviceObject == NULL){
		return FBE_STATUS_GENERIC_FAILURE;
    }

    s_cpd_hardware_info = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_HARDWARE_INFO), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_hardware_info == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_hardware_info, sizeof(S_CPD_HARDWARE_INFO));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_hardware_info->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_hardware_info->ioctl_hdr, CPD_IOCTL_GET_HARDWARE_INFO);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_hardware_info->ioctl_hdr, sizeof(CPD_HARDWARE_INFO));

    status = fbe_cpd_shim_send_cpd_ext_ioctl(pPortDeviceObject, s_cpd_hardware_info, sizeof(S_CPD_HARDWARE_INFO), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_send_cpd_ext_ioctl fail %X\n", __FUNCTION__, status);

    }

    hdw_info->vendor   = s_cpd_hardware_info->param.vendor;
    hdw_info->device   = s_cpd_hardware_info->param.device;
    hdw_info->bus      = s_cpd_hardware_info->param.bus;
    hdw_info->slot     = s_cpd_hardware_info->param.slot;
    hdw_info->function = s_cpd_hardware_info->param.function;

    hdw_info->hw_major_rev = s_cpd_hardware_info->param.hw_major_rev;
    hdw_info->hw_minor_rev = s_cpd_hardware_info->param.hw_minor_rev;
    hdw_info->firmware_rev_1 = s_cpd_hardware_info->param.firmware_rev_1;
    hdw_info->firmware_rev_2 = s_cpd_hardware_info->param.firmware_rev_2;
    hdw_info->firmware_rev_3 = s_cpd_hardware_info->param.firmware_rev_3;
    hdw_info->firmware_rev_4 = s_cpd_hardware_info->param.firmware_rev_4;

    fbe_release_nonpaged_pool(s_cpd_hardware_info);
    return status;

} /* end of function fbe_cpd_shim_get_hardware_info() */

static fbe_status_t 
fbe_cpd_shim_get_cpd_capabilities(IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject,
								  fbe_u32_t   io_portal_number,
								  OUT CPD_CAPABILITIES *pCapabilitiesInfo)
{
    fbe_status_t   status;
    S_CPD_CAPABILITIES * s_cpd_capabilities; 

    if (pPortDeviceObject == NULL){
		return FBE_STATUS_GENERIC_FAILURE;
    }

    s_cpd_capabilities = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_CAPABILITIES), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_capabilities == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_capabilities, sizeof(S_CPD_CAPABILITIES));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_capabilities->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_capabilities->ioctl_hdr, CPD_IOCTL_GET_CAPABILITIES);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_capabilities->ioctl_hdr, sizeof(CPD_CAPABILITIES));

    s_cpd_capabilities->param.portal_nr     =   io_portal_number;    
	status = fbe_cpd_shim_send_cpd_ext_ioctl(pPortDeviceObject, s_cpd_capabilities, sizeof(S_CPD_CAPABILITIES), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_send_cpd_ext_ioctl fail %X\n", __FUNCTION__, status);

    }

	fbe_copy_memory(pCapabilitiesInfo, &s_cpd_capabilities->param, sizeof(CPD_CAPABILITIES));
    fbe_release_nonpaged_pool(s_cpd_capabilities);
    return status;

} /* end of function fbe_cpd_shim_get_cpd_capabilities() */


/***************************************************************************
 * fbe_cpd_shim_get_cpd_config()
 ***************************************************************************
 *
 * Routine Description:
 *
 *   This routine sends a Config IOCTL request to a port driver to return
 *   configuration information. Space for the information is allocated
 *   by this routine. The caller is responsible for freeing the buffer 
 *   containing the configuration information. This routine is synchronous.
 *
 * Arguments:
 *
 *   pPortDeviceObject - Port driver device object representing the HBA.
 *   pConfigInfo - Location to store pointer to Config structure.
 *
 * Return Value:
 *
 *   NT Status indicating the results of the operation.
 *
 * History:
 *
 *   1-30-03    JAP     Created (taken from ASIDC)
 *   8-14-07    puhovp  Created (taken from NTBEGetScsiConfig)
 ***************************************************************************/

static fbe_status_t 
fbe_cpd_shim_get_cpd_config (IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject, 
							 IN fbe_u32_t   io_portal_number,
							 OUT CPD_CONFIG *pConfigInfo)
{
    fbe_status_t   status;
    S_CPD_CONFIG * s_cpd_config;

    if (pPortDeviceObject == NULL){
		return FBE_STATUS_GENERIC_FAILURE;
    }

    s_cpd_config = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_CONFIG), FBE_CPD_SHIM_MEMORY_TAG);
    if (s_cpd_config == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (s_cpd_config, sizeof(S_CPD_CONFIG));

    /* Set up most of the SRB_IO_CONTROL structure */
    cpd_shim_build_default_io_control_header(&s_cpd_config->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&s_cpd_config->ioctl_hdr, CPD_IOCTL_GET_CONFIG);
    CPD_IOCTL_HEADER_SET_LEN(&s_cpd_config->ioctl_hdr, sizeof(CPD_CONFIG));

    s_cpd_config->param.fmt_ver_major = CPD_CONFIG_FORMAT_VERSION;
    s_cpd_config->param.pass_thru_buf = pConfigInfo->pass_thru_buf;
    s_cpd_config->param.pass_thru_buf_len = pConfigInfo->pass_thru_buf_len;	    
    s_cpd_config->param.portal_nr = io_portal_number;

	status = fbe_cpd_shim_send_cpd_ext_ioctl(pPortDeviceObject, s_cpd_config, sizeof(S_CPD_CONFIG), FALSE);

    /* If IOCTL called successfully, wait on completion event */
    if (status != FBE_STATUS_OK) {
        /* the Login IOCTL could not be sent to the disk for some reason */
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_send_cpd_ext_ioctl fail %X\n", __FUNCTION__, status);

    }

	fbe_copy_memory(pConfigInfo, &s_cpd_config->param, sizeof(CPD_CONFIG));
    fbe_release_nonpaged_pool(s_cpd_config);
    return status;

} /* end of function fbe_cpd_shim_get_cpd_config() */

fbe_status_t 
fbe_cpd_shim_get_port_config_info(fbe_u32_t port_handle,
                                  fbe_cpd_shim_port_configuration_t *fbe_config_info)
{
    fbe_status_t status;
    PEMCPAL_DEVICE_OBJECT portDeviceObject = NULL;
    fbe_u32_t   io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
    CPD_CONFIG    * cpd_config = NULL;
    PCHAR           PassThruBuff = NULL;
    fbe_cpd_shim_port_role_t cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_INVALID;
    fbe_cpd_shim_connect_class_t   connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
    fbe_u32_t flare_bus = 0xFFFF;
    cpd_port_t      *cpd_port = NULL;

	if (port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid port handle 0x%X \n", __FUNCTION__,port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
    }

    cpd_port = &(cpd_port_array[port_handle]);
    portDeviceObject = cpd_port->miniport_device_object;
    io_portal_number  = cpd_port->io_portal_number;

    if ((io_portal_number == FBE_CPD_SHIM_INVALID_PORTAL_NUMBER) ||
        (portDeviceObject == NULL)){
		    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							    FBE_TRACE_LEVEL_WARNING,
							    FBE_TRACE_MESSAGE_ID_INFO,
							    "%s Invalid port handle 0x%X \n", __FUNCTION__,port_handle);
		    return FBE_STATUS_GENERIC_FAILURE;
     }

    cpd_config = fbe_allocate_nonpaged_pool_with_tag(sizeof (CPD_CONFIG), FBE_CPD_SHIM_MEMORY_TAG);
	if (cpd_config == NULL){		
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Allocation of memory for cpd_config failed \n", __FUNCTION__);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	fbe_zero_memory(cpd_config,sizeof (CPD_CONFIG));

    PassThruBuff = fbe_allocate_nonpaged_pool_with_tag(FBE_CPD_PASS_THRU_BUFFER_SIZE, FBE_CPD_SHIM_MEMORY_TAG);
	if (PassThruBuff  == NULL){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Allocation of memory for PassThruBuff failed \n", __FUNCTION__);
        fbe_release_nonpaged_pool(cpd_config);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}
	fbe_zero_memory(PassThruBuff,FBE_CPD_PASS_THRU_BUFFER_SIZE);	

	cpd_config->pass_thru_buf = PassThruBuff;
	cpd_config->pass_thru_buf_len = FBE_CPD_PASS_THRU_BUFFER_SIZE;
	status  = fbe_cpd_shim_get_cpd_config(portDeviceObject,   io_portal_number,cpd_config);
	if(status != FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_get_cpd_config failed %X\n", __FUNCTION__, status);
        fbe_release_nonpaged_pool(PassThruBuff);
        fbe_release_nonpaged_pool(cpd_config);
        return status;
    }

    switch(cpd_config->connect_class){
        case CPD_CC_FIBRE_CHANNEL:
            connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
            break;
        case CPD_CC_INTERNET_SCSI:
            connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
            break;
        case CPD_CC_SAS:
            connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
            break;
        case CPD_CC_FCOE:
            connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FCOE;
            break;
        default:
            connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
            break;
    }

    fbe_config_info->connect_class = connect_class;

	status = fbe_cpd_shim_get_flare_bus(cpd_config->pass_thru_buf, FBE_CPD_PASS_THRU_BUFFER_SIZE, &cpd_port_role,&flare_bus);
	if(status == FBE_STATUS_OK){        
        fbe_config_info->port_role = cpd_port_role;
        fbe_config_info->flare_bus_num = flare_bus;
    }else{
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_get_flare_bus failed %X\n", __FUNCTION__, status);

    }

    fbe_config_info->multi_core_affinity_enabled = cpd_port->mc_core_affinity_enabled = (cpd_config->mp_enabled ? FBE_TRUE : FBE_FALSE);
    fbe_config_info->core_affinity_proc_mask     = cpd_port->core_affinity_proc_mask  = cpd_config->active_proc_mask;
    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s MC core affinity %s supported by miniport. Port 0x%x Portal 0x%x\n",
                        __FUNCTION__,(cpd_config->mp_enabled ? "is": "NOT"),cpd_port->io_port_number,cpd_port->io_portal_number);

    fbe_release_nonpaged_pool(PassThruBuff);
    fbe_release_nonpaged_pool(cpd_config);
    return status;
}

fbe_status_t 
fbe_cpd_shim_get_port_capabilities(fbe_u32_t port_handle,
                                  fbe_cpd_shim_port_capabilities_t *port_capabilities)
{
    PEMCPAL_DEVICE_OBJECT portDeviceObject = NULL;
    fbe_u32_t   io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
    CPD_CAPABILITIES * cpd_capabilities;
    fbe_status_t       status;

    if ((port_handle >= FBE_CPD_SHIM_MAX_PORTS) || (port_capabilities == NULL)){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid port handle 0x%X \n", __FUNCTION__,port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
    }

    portDeviceObject = cpd_port_array[port_handle].miniport_device_object;
    io_portal_number  = cpd_port_array[port_handle].io_portal_number;

    if ((io_portal_number == FBE_CPD_SHIM_INVALID_PORTAL_NUMBER) ||
        (portDeviceObject == NULL)){
		    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							    FBE_TRACE_LEVEL_WARNING,
							    FBE_TRACE_MESSAGE_ID_INFO,
							    "%s Invalid port handle 0x%X \n", __FUNCTION__,port_handle);
		    return FBE_STATUS_GENERIC_FAILURE;
        }

    cpd_capabilities = fbe_allocate_nonpaged_pool_with_tag(sizeof (CPD_CAPABILITIES), FBE_CPD_SHIM_MEMORY_TAG);
	if (cpd_capabilities == NULL){		
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Allocation of memory for cpd_capabilities failed \n", __FUNCTION__);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	fbe_zero_memory(cpd_capabilities,sizeof (CPD_CAPABILITIES));

	status  = fbe_cpd_shim_get_cpd_capabilities(portDeviceObject, io_portal_number,cpd_capabilities);
	if(status == FBE_STATUS_OK){
        /* Map CPD information to SHIM info.*/
        port_capabilities->maximum_transfer_length = cpd_capabilities->max_transfer_length;
        port_capabilities->maximum_sg_entries = cpd_capabilities->max_sg_entries;
        port_capabilities->sg_length = cpd_capabilities->sg_length;
        port_capabilities->maximum_portals = cpd_capabilities->nr_portals;
        port_capabilities->portal_number = cpd_capabilities->portal_nr;
        port_capabilities->link_speed = cpd_capabilities->link_speed;
    }else{        
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s fbe_cpd_shim_get_cpd_capabilities failed %X\n", __FUNCTION__, status);        
    }

    fbe_release_nonpaged_pool(cpd_capabilities);
    return status;
}


/**************************************************************************
 *
 * DESCRIPTION:
 *     This function issues a synchronous IOCTL to DMD driver to handle the speed change
 *                                                          
 * PARAMETERS:
 *
 * RETURN VALUES/ERRORS:
 *
 * NOTES:
 *
 * HISTORY:
 *   Jul-22-05 BP - Initial revision
 *   Nov-16-07 Peter Puhov copied from cm_be_issue_speed_change_ioctl
 **************************************************************************/
fbe_status_t 
fbe_cpd_shim_change_speed(fbe_u32_t port_handle, fbe_port_speed_t speed)
{
    fbe_status_t status;
    S_CPD_CAPABILITIES * io_ctl; 

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s  io_port_number = %X, speed = %X\n", __FUNCTION__, port_handle, speed);


	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  io_port_number %X\n", __FUNCTION__, port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_CAPABILITIES), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory (io_ctl, sizeof(S_CPD_CAPABILITIES));

    /* Initialize common SRB IOCTL control fields */     
	cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_SET_CAPABILITIES);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_CAPABILITIES));

    /* Initialize SET_CAPABILITIES specific fields */
    io_ctl->param.misc = CPD_CAP_RESET_CONTROLLER;
    if(speed == FBE_PORT_SPEED_SAS_3_GBPS) {
       io_ctl->param.link_speed = SPEED_THREE_GIGABIT;
    }
    else if(speed == FBE_PORT_SPEED_SAS_6_GBPS) {
        io_ctl->param.link_speed = SPEED_SIX_GIGABIT;
    } else if(speed == FBE_PORT_SPEED_SAS_12_GBPS) {
        io_ctl->param.link_speed = SPEED_TWELVE_GIGABIT;
    } else {
	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
				FBE_TRACE_LEVEL_WARNING,
				FBE_TRACE_MESSAGE_ID_INFO,
				"%s Invalid speed %X\n", __FUNCTION__, speed);

        fbe_release_nonpaged_pool(io_ctl);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	io_ctl->param.portal_nr =   cpd_port_array[port_handle].io_portal_number;

    /*  Issue IOCTL to change speed */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
														io_ctl, 
														sizeof(S_CPD_CAPABILITIES), 
														FALSE);

    fbe_release_nonpaged_pool(io_ctl);

	return status;
} 

fbe_status_t
fbe_cpd_shim_get_port_info(fbe_u32_t port_handle,fbe_port_info_t * port_info)
{
	fbe_status_t status;
	S_CPD_PORT_INFORMATION	* io_ctl;

	fbe_zero_memory(port_info, sizeof(fbe_port_info_t));

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  io_port_number %X\n", __FUNCTION__, port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_PORT_INFORMATION), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

	fbe_zero_memory(io_ctl, sizeof(S_CPD_PORT_INFORMATION));

    /* Set up most of the SRB_IO_CONTROL structure by calling this function
     *  instead of using the following:
     *  IoCtl.SrbIoControl = SrbIoControlTemplate;
     */
    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_GET_PORT_INFO);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_PORT_INFORMATION));

	fbe_zero_memory(&io_ctl->param, sizeof(CPD_PORT_INFORMATION));

	io_ctl->param.portal_nr =   cpd_port_array[port_handle].io_portal_number;

    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
														io_ctl, 
														sizeof(S_CPD_PORT_INFORMATION), 
														FALSE);

	if(status != FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s port %X fbe_cpd_shim_send_cpd_ext_ioctl failed\n",__FUNCTION__, port_handle);

        fbe_release_nonpaged_pool(io_ctl);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    switch (io_ctl->param.link_speed) {
    case SPEED_ONE_FIVE_GIGABIT:
		port_info->link_speed = FBE_PORT_SPEED_SAS_1_5_GBPS;
		break;
    case SPEED_THREE_GIGABIT:
		port_info->link_speed = FBE_PORT_SPEED_SAS_3_GBPS;
		break;
    case SPEED_SIX_GIGABIT:
		port_info->link_speed = FBE_PORT_SPEED_SAS_6_GBPS;
		break;
    case SPEED_TWELVE_GIGABIT:
		port_info->link_speed = FBE_PORT_SPEED_SAS_12_GBPS;
		break;
    default:
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
                            "%s port %X  Uknown speed %X\n",__FUNCTION__, port_handle, io_ctl->param.link_speed);

		break;
	}

    switch(io_ctl->param.encryption_mode) {
        case CPD_ENC_DISABLED :
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_DISABLED;
            break;
        case CPD_ENC_PLAINTEXT_KEYS:
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_PLAINTEXT_KEYS;
            break;
        case CPD_ENC_WRAPPED_DEKS :
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS;
            break;
        case CPD_ENC_WRAPPED_KEKS_DEKS :
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS;
            break;
        case CPD_ENC_MODE_UNDEFINED :
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_INVALID;
            break;
        case CPD_ENC_NON_FUNCTIONAL :
            port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_NOT_SUPPORTED;
            break;
        default:
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s port %X  unknown mode %X\n",__FUNCTION__, port_handle, io_ctl->param.encryption_mode);

            break;

    }
	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s port_number=%X,speed=%X, mode:%d\n", 
                        __FUNCTION__, port_handle, port_info->link_speed,
                        port_info->enc_mode);

    fbe_release_nonpaged_pool(io_ctl);
	return status;
}

fbe_status_t
fbe_cpd_shim_get_port_name(fbe_u32_t port_handle,fbe_port_name_t * port_name)
{
	fbe_status_t status;
	S_CPD_NAME	* io_ctl;

	fbe_zero_memory(port_name, sizeof(fbe_port_name_t));

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  io_port_number %X\n", __FUNCTION__, port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof (S_CPD_NAME), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

	fbe_zero_memory(io_ctl, sizeof(S_CPD_NAME));

    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_GET_NAME);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_NAME));

	fbe_zero_memory(&io_ctl->param, sizeof(CPD_NAME));

	io_ctl->param.portal_nr =   cpd_port_array[port_handle].io_portal_number;
	io_ctl->param.flags = CPD_NAME_INITIATOR;

    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
														io_ctl, 
														sizeof(S_CPD_NAME),
														FALSE);

	if(status != FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s port %X fbe_cpd_shim_send_cpd_ext_ioctl failed\n",__FUNCTION__, port_handle);

        fbe_release_nonpaged_pool(io_ctl);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	port_name->port_name.sas_address = io_ctl->param.name.sas.sas_address;

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
						FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s  port_handle = %X, name = %llX\n", __FUNCTION__, port_handle, (unsigned long long)port_name->port_name.sas_address);


    fbe_release_nonpaged_pool(io_ctl);
	return status;
}

/***************************************************************************
 * 
 * Routine Description: 
 *     
 * 
 * Arguments: 
 * 
 *
 * Return Value: 
 *
 *
 * Revision History:
 ***************************************************************************/
fbe_status_t
fbe_cpd_shim_get_media_inteface_info(fbe_u32_t  port_handle, fbe_cpd_shim_media_interface_information_type_t mii_type,
                                 fbe_cpd_shim_sfp_media_interface_info_t *media_interface_info)
{
    fbe_status_t status;
    S_CPD_MEDIA_INTERFACE_INFO  * io_ctl;
    CPD_MEDIA_INTERFACE_INFO_TYPE   cpd_mii_type = CPD_MII_TYPE_LAST;
    cpd_port_t * cpd_port = NULL;    

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    cpd_port = &cpd_port_array[port_handle];

    switch (mii_type){
        case FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_ALL:
            cpd_mii_type = CPD_MII_TYPE_ALL;
            break;
        case FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_HIGHEST:
            cpd_mii_type = CPD_MII_TYPE_HIGHEST;
            break;
        case FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_LAST:
            cpd_mii_type = CPD_MII_TYPE_LAST;
            break;
        case FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_CACHED:
            if (cpd_port->sfp_info_valid){
                fbe_copy_memory(media_interface_info,&(cpd_port->sfp_info),sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
                return FBE_STATUS_OK;
            }else{
                cpd_mii_type = CPD_MII_TYPE_LAST;
            }
            break;
    }
	
    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof(S_CPD_MEDIA_INTERFACE_INFO), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (io_ctl, sizeof(S_CPD_MEDIA_INTERFACE_INFO));

    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_GET_MEDIA_INTERFACE_INFO);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_MEDIA_INTERFACE_INFO));

    io_ctl->param.portal_nr		   =  cpd_port_array[port_handle].io_portal_number;    
    io_ctl->param.mii_type         =  cpd_mii_type;

    /* Send IOCTL to miniport via SCSIPORT. */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
													io_ctl, 
													sizeof (S_CPD_MEDIA_INTERFACE_INFO), 
													FALSE);
    if (status == FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
								FBE_TRACE_LEVEL_DEBUG_LOW,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s MII:%d Rcvd SFP Condition for Port:%d Portal:%d. \n",
                               __FUNCTION__,
                               cpd_mii_type,
                               cpd_port->io_port_number,
                               cpd_port->io_portal_number);
        cpd_shim_map_cpd_sfp_media_interface_information(&(io_ctl->param),
                                                        media_interface_info);
        cpd_port->sfp_info_valid = FBE_TRUE;
    }

    fbe_release_nonpaged_pool(io_ctl);
	return status;
}


static fbe_status_t
cpd_shim_map_cpd_sfp_media_interface_information(CPD_MEDIA_INTERFACE_INFO *cpd_media_interface_info,
                                                 fbe_cpd_shim_sfp_media_interface_info_t *fbe_media_interface_info)
{
    fbe_cpd_shim_sfp_condition_type_t condition_type = FBE_CPD_SHIM_SFP_CONDITION_INVALID;    
    fbe_cpd_shim_sfp_subcondition_type_t subcondition_type = FBE_CPD_SHIM_SFP_NONE;
    fbe_cpd_shim_sfp_media_type_t            media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_INVALID;
    fbe_u32_t                           string_size = 0;

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "CPDSHIM: MII Rcvd: Cond:%d SubCond:%d MediaType:%d\n", 
                           cpd_media_interface_info->condition,
                           cpd_media_interface_info->sub_condition,
                           cpd_media_interface_info->mii_media_type);
    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "PartNum:%s SerNum:%s\n", 
                           cpd_media_interface_info->vendor_part_number,
                           cpd_media_interface_info->vendor_serial_number);


    switch(cpd_media_interface_info->condition){
        case CPD_SFP_CONDITION_GOOD:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
            break;
        case CPD_SFP_CONDITION_INSERTED:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_INSERTED;
            break;
        case CPD_SFP_CONDITION_REMOVED:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_REMOVED;
            break;
        case CPD_SFP_CONDITION_FAULT:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_FAULT;
            break;
        case CPD_SFP_CONDITION_WARNING:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_WARNING;
            break;
        case CPD_SFP_CONDITION_INFO:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_INFO;
            break;

        default:
            condition_type = FBE_CPD_SHIM_SFP_CONDITION_INVALID;
            break;
    }

    fbe_media_interface_info->condition_type = condition_type;

    switch(cpd_media_interface_info->sub_condition)
    {
    case CPD_SFP_NONE:
        subcondition_type = FBE_CPD_SHIM_SFP_NONE;
        break;
    case CPD_SFP_INFO_SPD_LEN_AVAIL:
        subcondition_type = FBE_CPD_SHIM_SFP_INFO_SPD_LEN_AVAIL;
        break;
    case CPD_SFP_INFO_RECHECK_MII:
        subcondition_type = FBE_CPD_SHIM_SFP_INFO_RECHECK_MII;
        break;
    case CPD_SFP_INFO_BAD_EMC_CHKSUM:
        subcondition_type = FBE_CPD_SHIM_SFP_INFO_BAD_EMC_CHKSUM;
        break;
    case CPD_SFP_BAD_CHKSUM:
        subcondition_type = FBE_CPD_SHIM_SFP_BAD_CHKSUM;
        break;
    case CPD_SFP_BAD_I2C:
        subcondition_type = FBE_CPD_SHIM_SFP_BAD_I2C;
        break;
    case CPD_SFP_DEV_ERR:
        subcondition_type = FBE_CPD_SHIM_SFP_DEV_ERR;
        break;
    case CPD_SFP_DIAG_TXFAULT:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TXFAULT;
        break;
    case CPD_SFP_DIAG_TEMP_HI_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_ALARM;
        break;
    case CPD_SFP_DIAG_TEMP_LO_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_ALARM;
        break;
    case CPD_SFP_DIAG_VCC_HI_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_VCC_HI_ALARM;
        break;
    case CPD_SFP_DIAG_VCC_LO_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_VCC_LO_ALARM;
        break;
    case CPD_SFP_DIAG_TX_BIAS_HI_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_ALARM;
        break;
    case CPD_SFP_DIAG_TX_BIAS_LO_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_ALARM;
        break;
    case CPD_SFP_DIAG_TX_POWER_HI_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_ALARM;
        break;
    case CPD_SFP_DIAG_TX_POWER_LO_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_ALARM;
        break;
    case CPD_SFP_DIAG_RX_POWER_HI_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_ALARM;
        break;
    case CPD_SFP_DIAG_RX_POWER_LO_ALARM:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_ALARM;
        break;
    case CPD_SFP_DIAG_TEMP_HI_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_WARNING;
        break;
    case CPD_SFP_DIAG_TEMP_LO_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_WARNING;
        break;
    case CPD_SFP_DIAG_VCC_HI_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_VCC_HI_WARNING;
        break;
    case CPD_SFP_DIAG_VCC_LO_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_VCC_LO_WARNING;
        break;
    case CPD_SFP_DIAG_TX_BIAS_HI_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_WARNING;
        break;
    case CPD_SFP_DIAG_TX_BIAS_LO_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_WARNING;
        break;
    case CPD_SFP_DIAG_TX_POWER_HI_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_WARNING;
        break;
    case CPD_SFP_DIAG_TX_POWER_LO_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_WARNING;
        break;
    case CPD_SFP_DIAG_RX_POWER_HI_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_WARNING;
        break;
    case CPD_SFP_DIAG_RX_POWER_LO_WARNING:
        subcondition_type = FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_WARNING;
        break;
    case CPD_SFP_UNQUAL_OPT_NOT_4G:
        subcondition_type = FBE_CPD_SHIM_SFP_UNQUAL_OPT_NOT_4G;
        break;
    case CPD_SFP_UNQUAL_COP_AUTO:
        subcondition_type = FBE_CPD_SHIM_SFP_UNQUAL_COP_AUTO;
        break;
    case CPD_SFP_UNQUAL_COP_SFP_SPEED:
        subcondition_type = FBE_CPD_SHIM_SFP_UNQUAL_COP_SFP_SPEED;
        break;
    case CPD_SFP_UNQUAL_SPEED_EXCEED_SFP:
        subcondition_type = FBE_CPD_SHIM_SFP_UNQUAL_SPEED_EXCEED_SFP;
        break;
    case CPD_SFP_UNQUAL_PART:
        subcondition_type = FBE_CPD_SHIM_SFP_UNQUAL_PART;
        break;
    case CPD_SFP_UNKNOWN_TYPE:
        subcondition_type = FBE_CPD_SHIM_SFP_UNKNOWN_TYPE;
        break;
    case CPD_SFP_SPEED_MISMATCH:
        subcondition_type = FBE_CPD_SHIM_SFP_SPEED_MISMATCH;
        break;
    case CPD_SFP_EDC_MODE_MISMATCH:
        subcondition_type = FBE_CPD_SHIM_SFP_EDC_MODE_MISMATCH;
        break;
    case CPD_SFP_SAS_SPECL_ERROR:
        subcondition_type = FBE_CPD_SHIM_SFP_SAS_SPECL_ERROR;
        break;

    }
    fbe_media_interface_info->condition_additional_info = subcondition_type;

    if (cpd_media_interface_info->valid.speeds){
        fbe_media_interface_info->speeds = cpd_media_interface_info->speeds;
    }else{
        fbe_media_interface_info->speeds = SPEED_UNKNOWN;
    }

    fbe_media_interface_info->hw_type = cpd_media_interface_info->hw_type;

    if(cpd_media_interface_info->checksum_status == CPD_SFP_CHECK_SUM_GOOD)
    {
    
        if (cpd_media_interface_info->valid.mii_media_type){
            switch(cpd_media_interface_info->mii_media_type){
                case CPD_SFP_COPPER:
                    media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_COPPER;
                    break;
                case CPD_SFP_OPTICAL:
                    media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_OPTICAL;
                    break;
                case CPD_SFP_NAS_COPPER:
                    media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_NAS_COPPER;
                    break;
                case CPD_SFP_UNKNOWN:
                    media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_UNKNOWN;
                    break;
                case CPD_SFP_MINI_SAS_HD:
                    media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_MINI_SAS_HD;
                    break;
            }
            fbe_media_interface_info->media_type = media_type;
        }else{
            fbe_media_interface_info->media_type = FBE_CPD_SHIM_SFP_MEDIA_TYPE_INVALID;
        }
    }
    
    string_size = CSX_MIN(FBE_CPD_SHIM_SFP_EMC_DATA_LENGTH,FBE_PORT_SFP_EMC_DATA_LENGTH);
    if (cpd_media_interface_info->valid.emc_info)
    {
        if (cpd_media_interface_info->emc_part_number)
            fbe_copy_memory(fbe_media_interface_info->emc_part_number,cpd_media_interface_info->emc_part_number,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->emc_part_number,string_size);
        if (cpd_media_interface_info->emc_part_revision)
            fbe_copy_memory(fbe_media_interface_info->emc_part_revision,cpd_media_interface_info->emc_part_revision,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->emc_part_revision,string_size);
        if (cpd_media_interface_info->emc_serial_number)
            fbe_copy_memory(fbe_media_interface_info->emc_serial_number,cpd_media_interface_info->emc_serial_number,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->emc_serial_number,string_size);
    }
    else
    {
        fbe_zero_memory(fbe_media_interface_info->emc_part_number,string_size);
        fbe_zero_memory(fbe_media_interface_info->emc_part_revision,string_size);
        fbe_zero_memory(fbe_media_interface_info->emc_serial_number,string_size);
    }

    string_size = CSX_MIN(FBE_CPD_SHIM_SFP_VENDOR_DATA_LENGTH,FBE_PORT_SFP_VENDOR_DATA_LENGTH);
    if (cpd_media_interface_info->valid.vendor_info)
    {
        if (cpd_media_interface_info->vendor_part_number)
            fbe_copy_memory(fbe_media_interface_info->vendor_part_number,cpd_media_interface_info->vendor_part_number,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->vendor_part_number,string_size);
        if (cpd_media_interface_info->vendor_part_revision)
            fbe_copy_memory(fbe_media_interface_info->vendor_part_revision,cpd_media_interface_info->vendor_part_revision,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->vendor_part_revision,string_size);
        if (cpd_media_interface_info->vendor_serial_number)
            fbe_copy_memory(fbe_media_interface_info->vendor_serial_number,cpd_media_interface_info->vendor_serial_number,string_size);
        else
            fbe_zero_memory(fbe_media_interface_info->vendor_serial_number,string_size);
    }
    else
    {
        fbe_zero_memory(fbe_media_interface_info->vendor_part_number,string_size);
        fbe_zero_memory(fbe_media_interface_info->vendor_part_revision,string_size);
        fbe_zero_memory(fbe_media_interface_info->vendor_serial_number,string_size);
    }

    return FBE_STATUS_OK;
}

/******************************************************************************

NAME		fbe_cpd_shim_get_flare_bus

DESCRIPTION	Parses the supplied buffer for the characters B(x), and returns
			the (base 10) value of 'x'.

ARGUMENTS	BufferPtr points to buffer to be parsed.
			BufferSize specifies the size of the buffer.

RETURNS		Base 10 value of x, parsed from B(x).

AUTHOR      08-Apr-03 . Ashok Tamilarasan - Copied from ExtractKI function written by Thomas B. Westbom                                            
            14-Aug-07 . Peter Puhov - Copied from NTBEGetFlareBus function written by Ashok Tamilarasan

******************************************************************************/
#define ISADIGIT(c)	(((c) >= '0') && ((c) <= '9')) 
#define DIGITVALUE(c) ((int)(c) - 0x30)

static fbe_status_t
fbe_cpd_shim_get_flare_bus(PUCHAR BufferPtr, fbe_u32_t BufferSize, 
                           fbe_cpd_shim_port_role_t *port_role,
                           fbe_u32_t *flare_bus)
{
	fbe_u32_t	Index = 0, SavedIndex = 0;
	fbe_u32_t	Bus = 0;
	fbe_bool_t	LookingForPortType = FBE_TRUE;
    fbe_bool_t	LookingForSpecialPortType = FBE_TRUE;
    fbe_bool_t  LookingForPortNumber = FBE_TRUE;
	fbe_bool_t  ValueDetected = FBE_FALSE;    
    fbe_cpd_shim_port_role_t cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_INVALID;

    /* Format ofthe passup string is as follows:
     * Back End Port  - "B(X)" where X is the BE port #
     * Front End Port - "I(X)" where X is the FE port #
     * Uncommitted Port-"U(?)" No port number is assigned.
     */

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "CPDSHIM: get_flare_bus recvd: %s\n", 
                           BufferPtr);

	// Process the supplied buffer.
	for (Index = 0; Index < BufferSize; Index++) {
        if (LookingForPortType){
            switch (BufferPtr[Index]){
                case 'B':
                    LookingForPortType = FBE_FALSE;
                    cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_BE;
                    break;
                case 'I':
                    LookingForPortType = FBE_FALSE;
                    cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
                    break;
                case 'U':
                    LookingForPortType = FBE_FALSE;
                    cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_UNC;
                    break;
                default:
                    break;
            }
		}else{			
			if (LookingForPortNumber && ISADIGIT(BufferPtr[Index])) {
				// Process this digit.
				ValueDetected = FBE_TRUE;
				Bus = (Bus * 10) + DIGITVALUE(BufferPtr[Index]);
			}
			else if (BufferPtr[Index] == ')') {
				// We found the closing parenthesis.  Bail out.
                SavedIndex = Index;
				break;
			}
		}
	}

    if(!ValueDetected && (cpd_port_role != FBE_CPD_SHIM_PORT_ROLE_UNC)) {
        /* There is not bus value, set a very high value */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* for uncommitted port, we need to set Bus number to default/invalid */
    if(!ValueDetected && (cpd_port_role == FBE_CPD_SHIM_PORT_ROLE_UNC)) {
        Bus = FBE_INVALID_PORT_NUM;
    }

    /* TODO: Check for SPECIAL FE ports*/
    /* Format "I(2)D(250)AF(225)" or
     *        "I(2)D(250)" or        
     *        "I(2)AF(225)"         
     */

    if (cpd_port_role == FBE_CPD_SHIM_PORT_ROLE_FE){
        if (SavedIndex < (BufferSize - 1)){
            //More information is present in the passup buffer.
            //Check whether this is a special port.
            for (Index = SavedIndex; Index < BufferSize; Index++){
                if (LookingForSpecialPortType){
                    if (BufferPtr[Index] == 'D'){
                        cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_SPECIAL;
                        break;
                    }else if (BufferPtr[Index] == 'A'){
                        Index++;
                        if (Index < BufferSize){
                            if (BufferPtr[Index] == 'F'){
                                cpd_port_role = FBE_CPD_SHIM_PORT_ROLE_SPECIAL;
                                break;
                            }
                        }
                    }
                }
            }// end of for
        }
    }

	*flare_bus = Bus;
    *port_role = cpd_port_role;
	return FBE_STATUS_OK;
} /* end of fbe_cpd_shim_get_flare_bus() */


static fbe_status_t
fbe_cpd_shim_get_device_type(fbe_u32_t cpd_login_flags, fbe_cpd_shim_discovery_device_type_t *cpd_device_type)
{
	if (cpd_login_flags & CPD_LOGIN_EXPANDER_DEVICE){
		*cpd_device_type = FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE;
	}else if (cpd_login_flags & CPD_LOGIN_VIRTUAL_DEVICE){
		*cpd_device_type = FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL;
	}else if (cpd_login_flags & CPD_LOGIN_SATA_DEVICE){
		*cpd_device_type = FBE_CPD_SHIM_DEVICE_TYPE_STP;
	}else{
		*cpd_device_type = FBE_CPD_SHIM_DEVICE_TYPE_SSP;
	}
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_cpd_shim_get_login_reason(CPD_EVENT_INFO *pInfo, fbe_cpd_shim_device_table_entry_t	*current_device_entry)
{
    CPD_LOGIN_REASON    reason;
	if (pInfo->u.port_login.flags & CPD_LOGIN_PLACEHOLDER){
        reason = pInfo->u.port_login.more_info.reason;

        current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_INVALID;
        switch (reason){
            case CPD_LOGIN_EXPANDER_UNSUPPORTED:
                current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_EXPANDER;
                break;
            case CPD_LOGIN_UNSUPPORTED_TOPOLOGY:
                current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_TOPOLOGY;
                break;
            case CPD_LOGIN_EXCEEDS_LIMITS:
                current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_TOO_MANY_END_DEVICES;
                break;
            case CPD_LOGIN_MIXED_COMPLIANCE:
                current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_EXPANDER_MIXED_COMPLIANCE;
                break;
        };		
        /*
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Special login reason 0x%x for device ctxt 0x%x\n",__FUNCTION__,
                            current_device_entry->device_login_reason,
                            pInfo->u.port_login.miniport_login_context);
        */
    }else{
        current_device_entry->device_login_reason = CPD_SHIM_DEVICE_LOGIN_REASON_NORMAL;
	}
	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_cpd_shim_initialize_port_event_queue(cpd_port_t *cpd_port)
{
	if (cpd_port->port_event_callback_info_queue != NULL){
		fbe_zero_memory(cpd_port->port_event_callback_info_queue,FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY*sizeof(fbe_cpd_shim_callback_info_t));
		fbe_spinlock_init(&(cpd_port->consumer_index_lock));
		cpd_port->consumer_index = 0;
		cpd_port->producer_index = 0;
	}
	return FBE_STATUS_OK;
}

static fbe_atomic_t
cpd_shim_increment_with_mod(fbe_atomic_t *index,fbe_atomic_t mod)
{
    fbe_atomic_t old_value = *index;
    fbe_atomic_t current_value = old_value;
    fbe_atomic_t new_value;

    for(;;){
        new_value = (old_value + 1) % mod;
        current_value = fbe_atomic_compare_exchange(index, new_value, old_value);
        if (current_value == old_value){
            return(new_value);
        }
        old_value = current_value;
    }
    return (new_value);
}

/* Called at DEVICE IRQL level from miniport driver callback.*/
static fbe_status_t
fbe_cpd_shim_add_to_port_event_queue(cpd_port_t *cpd_port,fbe_cpd_shim_callback_info_t *callback_info)
{
    fbe_atomic_t local_producer_index;
    fbe_atomic_t local_consumer_index;

    fbe_atomic_exchange(&local_producer_index,cpd_port->producer_index);
    fbe_atomic_exchange(&local_consumer_index,cpd_port->consumer_index);
	local_producer_index = cpd_port->producer_index;
    if ( ((local_producer_index  + 1)%FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY) == local_consumer_index){
		/*Queue full.*/
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

    EmcpalCopyMemory(&(cpd_port->port_event_callback_info_queue[local_producer_index]),callback_info,sizeof(fbe_cpd_shim_callback_info_t));
    cpd_shim_increment_with_mod(&cpd_port->producer_index,FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY);    

	return FBE_STATUS_OK;
}

/* Called at DISPATCH_LEVEL from DPC routine.*/
static fbe_status_t
fbe_cpd_shim_get_next_port_event_from_queue(cpd_port_t *cpd_port,fbe_cpd_shim_callback_info_t *callback_info)
{	
    fbe_atomic_t local_producer_index;
    fbe_atomic_t local_consumer_index;

    fbe_spinlock_lock(&(cpd_port->consumer_index_lock));
    fbe_atomic_exchange(&local_producer_index,cpd_port->producer_index);
    fbe_atomic_exchange(&local_consumer_index,cpd_port->consumer_index);
    
    if ( local_consumer_index  == local_producer_index) {        
        fbe_spinlock_unlock(&(cpd_port->consumer_index_lock));
		return FBE_STATUS_BUSY;
	}

    EmcpalCopyMemory(callback_info,&(cpd_port->port_event_callback_info_queue[local_consumer_index]),sizeof(fbe_cpd_shim_callback_info_t));
    cpd_shim_increment_with_mod(&cpd_port->consumer_index,FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY);    
    fbe_spinlock_unlock(&(cpd_port->consumer_index_lock));

	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_cpd_shim_schedule_port_event_dpc(cpd_port_t * cpd_port)
{
        EmcpalDpcExecute(&(cpd_port->port_event_callback_dpc));

	return FBE_STATUS_OK;
}
static fbe_status_t
fbe_cpd_shim_schedule_device_event_dpc(cpd_port_t * cpd_port)
{
        EmcpalDpcExecute(&(cpd_port->device_event_callback_dpc));

	return FBE_STATUS_OK;
}


static VOID  
fbe_cpd_shim_port_event_dpc_function(PVOID Context)
{
	fbe_cpd_shim_callback_info_t  callback_info;
	fbe_status_t	status;
	cpd_port_t * cpd_port = (cpd_port_t *)Context;

	fbe_zero_memory(&callback_info,sizeof(fbe_cpd_shim_callback_info_t));

	while ((status = fbe_cpd_shim_get_next_port_event_from_queue(cpd_port,&callback_info)) == FBE_STATUS_OK){
		if(cpd_port->callback_function != NULL){
			cpd_port->callback_function(&callback_info, cpd_port->callback_context);
		}
		fbe_zero_memory(&callback_info,sizeof(fbe_cpd_shim_callback_info_t));
	}

	return;
}

// Define base DPC function that will set any required context (e.g. CSX) prior to calling our DPC
EMCPAL_DEFINE_DPC_FUNC(fbe_cpd_shim_port_event_dpc_function_base, fbe_cpd_shim_port_event_dpc_function)

static VOID 
fbe_cpd_shim_device_event_dpc_function(PVOID Context)
{
	fbe_cpd_shim_callback_info_t  callback_info;	
	cpd_port_t * cpd_port = (cpd_port_t *)Context;

	fbe_zero_memory(&callback_info,sizeof(fbe_cpd_shim_callback_info_t));
	callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE;
	callback_info.info.topology_device_information_table = cpd_port->topology_device_information_table;
	if(cpd_port->callback_function != NULL){
		cpd_port->callback_function(&callback_info, cpd_port->callback_context);
	}
	return;
}

// Define base DPC function that will set any required context (e.g. CSX) prior to calling our DPC
EMCPAL_DEFINE_DPC_FUNC(fbe_cpd_shim_device_event_dpc_function_base, fbe_cpd_shim_device_event_dpc_function)

static fbe_bool_t cpd_shim_is_port_valid(cpd_port_t *cpd_port)
{
    fbe_bool_t port_valid = FBE_FALSE;

    if ((cpd_port != NULL) &&
         (cpd_port->io_port_number != FBE_CPD_SHIM_INVALID_PORT_NUMBER) &&
         (cpd_port->io_portal_number != FBE_CPD_SHIM_INVALID_PORTAL_NUMBER) &&
         (cpd_port->miniport_device_object != NULL) &&
         (cpd_port->state == CPD_SHIM_PORT_STATE_INITIALIZED)){

             port_valid = FBE_TRUE;
         }

    return (port_valid);
}

static fbe_status_t
cpd_shim_send_process_io(cpd_port_t *cpd_port)
{
    fbe_status_t            status;
	S_CPD_CALLBACK_HANDLE	*io_ctl = NULL;

    if (cpd_port->process_io)
    {
        cpd_port->process_io(cpd_port->enque_pending_dmrb_context);
        return FBE_STATUS_OK;
    }

    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof(S_CPD_CALLBACK_HANDLE), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (io_ctl, sizeof(S_CPD_CALLBACK_HANDLE));

    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_PROCESS_IO);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_CALLBACK_HANDLE));

    io_ctl->param.callback_handle = cpd_port->miniport_callback_handle;

    /* Send IOCTL to miniport */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port->miniport_device_object,
                                             io_ctl, 
                                             sizeof (S_CPD_CALLBACK_HANDLE), 
                                             FALSE);

	if(status != FBE_STATUS_OK){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Process IO ioctl failed %X\n", __FUNCTION__, status);
	}

    fbe_release_nonpaged_pool(io_ctl);

    return status;
}

static void 
cpd_shim_ioctl_process_io_thread_func(void * context)
{
    cpd_port_t	   *cpd_port = NULL;
    fbe_u32_t       ii = 0;
    EMCPAL_STATUS        nt_status;
    fbe_atomic_t    send_process_io_ioctl = FBE_FALSE;
    
	FBE_UNREFERENCED_PARAMETER(context);

    while(cpd_shim_ioctl_process_io_thread_flag == IOCTL_PROCESS_IO_THREAD_RUN)    
	{
        nt_status = fbe_rendezvous_event_wait(&cpd_shim_ioctl_process_io_event, 40000);
        fbe_rendezvous_event_clear(&cpd_shim_ioctl_process_io_event);
        for (ii = 0; ii < FBE_CPD_SHIM_MAX_PORTS; ii++){
            cpd_port = &(cpd_port_array[ii]);

		if(cpd_shim_ioctl_process_io_thread_flag == IOCTL_PROCESS_IO_THREAD_RUN) {
                if (cpd_shim_is_port_valid(cpd_port)){
                    send_process_io_ioctl = fbe_atomic_exchange(&cpd_port->send_process_io_ioctl,FBE_FALSE);
                    if ((send_process_io_ioctl == FBE_TRUE) &&
                         (cpd_port->enque_pending_dmrb_context != NULL)){
                             if (FBE_FALSE == fbe_atomic_exchange(&cpd_port->port_reset_in_progress,cpd_port->port_reset_in_progress)){
                                cpd_shim_send_process_io(cpd_port);
                             }
                    }
                }
            } 
        } /* End for()*/
    }

    cpd_shim_ioctl_process_io_thread_flag = IOCTL_PROCESS_IO_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static SGL_PADDR 
cpd_shim_get_phys_addr(void * vaddr, void * xlat_context)
{
	PHYSICAL_ADDRESS physical_address;

	physical_address.QuadPart = fbe_get_contigmem_physical_address(vaddr);

#ifndef ALAMOSA_WINDOWS_ENV
    CSX_ASSERT_H_RDC(CSX_NOT_ZERO(physical_address.QuadPart));
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system - check for failure to convert VA to PA */
	return physical_address.QuadPart;
}

fbe_status_t 
fbe_cpd_shim_reset_device(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id)
{
    fbe_status_t                status;
    S_CPD_RESET_DEVICE		    io_ctl;

	if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);


		return FBE_STATUS_GENERIC_FAILURE;
	}

    cpd_shim_build_default_io_control_header(&io_ctl.ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl.ioctl_hdr, CPD_IOCTL_RESET_DEVICE);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl.ioctl_hdr, sizeof(io_ctl.param));

    io_ctl.param.portal_nr = cpd_port_array[port_handle].io_portal_number;
	io_ctl.param.phy_nr = CPD_RESET_SPECIFIED_DEVICE;
    io_ctl.param.miniport_login_context  = (void *)(UINT_PTR)cpd_device_id;

    /* Send IOCTL to miniport via SCSIPORT. */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
													&io_ctl, 
													sizeof (S_CPD_RESET_DEVICE), 
													FALSE);

	if(status == FBE_STATUS_OK){
#if 0
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Sent reset to device: %X portal: %X\n", __FUNCTION__,
				cpd_device_id,cpd_port_array[port_handle].io_portal_number);
#endif
	}
	return status;
}


fbe_status_t 
fbe_cpd_shim_port_get_device_table_ptr(fbe_u32_t port_handle,fbe_cpd_shim_device_table_t **device_table_ptr)
{
	cpd_port_t * cpd_port = NULL;
	fbe_status_t  status = FBE_STATUS_OK;

	if (port_handle >= FBE_CPD_SHIM_MAX_PORTS){
		status = FBE_STATUS_GENERIC_FAILURE;
	}else {
		cpd_port = &cpd_port_array[port_handle];
		if (cpd_port->topology_device_information_table != NULL){
			*device_table_ptr = cpd_port->topology_device_information_table;
		}else{
			status = FBE_STATUS_GENERIC_FAILURE;
		}
	}
	return status;
}

fbe_status_t 
fbe_cpd_shim_port_get_device_table_max_index(fbe_u32_t port_handle,fbe_u32_t *device_table_max_index)
{
	cpd_port_t * cpd_port = NULL;
	fbe_status_t  status = FBE_STATUS_OK;

	if ((port_handle >= FBE_CPD_SHIM_MAX_PORTS) || (device_table_max_index == NULL)){
		status = FBE_STATUS_GENERIC_FAILURE;
	}else {
		cpd_port = &cpd_port_array[port_handle];
		if (cpd_port->topology_device_information_table != NULL){
			*device_table_max_index = cpd_port->topology_device_information_table->device_table_size;
		}else{
			status = FBE_STATUS_GENERIC_FAILURE;
		}
	}
	return status;
}
fbe_status_t 
fbe_cpd_shim_reset_expander_phy(fbe_u32_t port_handle, fbe_cpd_device_id_t smp_port_device_id,fbe_u8_t phy_id)
{
    fbe_status_t                status;
    S_CPD_RESET_DEVICE		    io_ctl;

    if(port_handle >= FBE_CPD_SHIM_MAX_PORTS){
        fbe_base_library_trace(FBE_LIBRARY_ID_PMC_SHIM,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Invalid  port_handle %X\n", __FUNCTION__,  port_handle);


        return FBE_STATUS_GENERIC_FAILURE;
    }

    cpd_shim_build_default_io_control_header(&io_ctl.ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl.ioctl_hdr, CPD_IOCTL_RESET_DEVICE);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl.ioctl_hdr, sizeof(io_ctl.param));

    io_ctl.param.portal_nr = cpd_port_array[port_handle].io_portal_number;
    io_ctl.param.phy_nr = phy_id;
    io_ctl.param.miniport_login_context  = (void *)(UINT_PTR)smp_port_device_id;

    /* Send IOCTL to miniport via SCSIPORT. */
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
                                                    &io_ctl, 
                                                    sizeof (S_CPD_RESET_DEVICE), 
                                                    FALSE);

    if(status != FBE_STATUS_OK){

        fbe_base_library_trace(FBE_LIBRARY_ID_PMC_SHIM,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Error sending reset to port id: %X, SMP dev 0x%X phy: 0x%X \n", __FUNCTION__,
                            port_handle,smp_port_device_id,phy_id);

    }
    return status;
}

/* Supports only lane information for SAS ports for now.
   Information about link speed and link status will be available for all port types. */
fbe_status_t
fbe_cpd_shim_get_port_lane_info(fbe_u32_t  port_handle, fbe_cpd_shim_connect_class_t connect_class,
                                /*fbe_port_info_t * port_info*/fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    fbe_status_t status;
    S_CPD_PORT_INFORMATION  * io_ctl = NULL;
	CPD_PORT_INFORMATION	* link_info = NULL;
    cpd_port_t * cpd_port = NULL;    

	if((port_handle >= FBE_CPD_SHIM_MAX_PORTS) || (port_lane_info == NULL)){
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Invalid  parameter %X\n", __FUNCTION__,  port_handle);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	

    cpd_port = &cpd_port_array[port_handle];

    io_ctl = fbe_allocate_nonpaged_pool_with_tag(sizeof(S_CPD_PORT_INFORMATION), FBE_CPD_SHIM_MEMORY_TAG);
    if (io_ctl == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
	fbe_zero_memory (io_ctl, sizeof(S_CPD_PORT_INFORMATION));

    cpd_shim_build_default_io_control_header(&io_ctl->ioctl_hdr);

    CPD_IOCTL_HEADER_SET_OPCODE(&io_ctl->ioctl_hdr, CPD_IOCTL_GET_PORT_INFO);
    CPD_IOCTL_HEADER_SET_LEN(&io_ctl->ioctl_hdr, sizeof(CPD_PORT_INFORMATION));

	io_ctl->param.portal_nr		   =  cpd_port_array[port_handle].io_portal_number;    

    // Send IOCTL to miniport via SCSIPORT. 
    status = fbe_cpd_shim_send_cpd_ext_ioctl(cpd_port_array[port_handle].miniport_device_object,
													io_ctl, 
													sizeof (S_CPD_PORT_INFORMATION), 
													FALSE);
    if (status == FBE_STATUS_OK) {
		link_info = &(io_ctl->param);

        port_lane_info->link_speed = link_info->link_speed;
		port_lane_info->portal_number = link_info->portal_nr;
        port_lane_info->link_state = CPD_SHIM_PORT_LINK_STATE_INVALID;
        if (link_info->link_status == CPD_LS_LINK_DOWN){
            port_lane_info->link_state = CPD_SHIM_PORT_LINK_STATE_DOWN;
        }else if (link_info->link_status == CPD_LS_LINK_UP){
            port_lane_info->link_state = CPD_SHIM_PORT_LINK_STATE_UP;
        }

        /* SAS specific information. */
        /* TODO: FC, iScsi and FCOE related info.*/
        if (connect_class == FBE_CPD_SHIM_CONNECT_CLASS_SAS){
		    port_lane_info->nr_phys = link_info->port_info.sas.nr_phys;
		    port_lane_info->phy_map = link_info->port_info.sas.phy_map;
		    port_lane_info->nr_phys_enabled = link_info->port_info.sas.nr_phys_enabled;
		    port_lane_info->nr_phys_up = link_info->port_info.sas.nr_phys_up;

            if((link_info->link_status == CPD_LS_LINK_UP) && 
               (port_lane_info->nr_phys_up < port_lane_info->nr_phys))
            {
                port_lane_info->link_state = CPD_SHIM_PORT_LINK_STATE_DEGRADED;
            }
        }
    }else{
		fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
							FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s Error querying PORT Information %X. %X\n", __FUNCTION__,  port_handle, status);
    }

    fbe_release_nonpaged_pool(io_ctl);
	/* OR return cached information in Port structure.*/
    return status;
}

fbe_status_t 
fbe_cpd_shim_set_async_io(fbe_bool_t async_io)
{
	cpd_shim_async_io = async_io;

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO, "cpd_shim_async_io: 0x%X\n", cpd_shim_async_io);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_get_async_io(fbe_bool_t * async_io)
{
	*async_io = cpd_shim_async_io;	

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cpd_shim_set_dmrb_zeroing(fbe_bool_t dmrb_zeroing)
{
	cpd_shim_dmrb_zeroing = dmrb_zeroing;

	fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO, "cpd_shim_dmrb_zeroing: 0x%X\n", cpd_shim_dmrb_zeroing);

	return FBE_STATUS_OK;
}

