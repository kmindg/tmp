
#include <ntddk.h>
#include "EmcPAL_DriverShell.h"

#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_event_log_api.h"
#include "klogservice.h"
#include "EmcPAL_Memory.h"
#include "fbe/fbe_sep_shim_kernel_interface.h"
#include "fbe/fbe_driver_notification_interface.h"
#include "fbe/fbe_package.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_physical_package.h"
#include "SEPMessages.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe_database.h"
#include "fbe/fbe_board.h"
#include "EmcUTIL.h"
#include "fbe/fbe_base_config.h"

typedef struct sep_device_extention_s {
    PEMCPAL_DEVICE_OBJECT device_object;
} sep_device_extention_t;

typedef struct irp_context_s {
	PEMCPAL_IRP 									PIrp;
	fbe_serialized_control_transaction_t * 	user_packet;
	ULONG 									outLength;
    void *									sg_list;
}irp_context_t;

typedef struct io_irp_context_s {
	PEMCPAL_IRP 						PIrp;
	ULONG 						outLength;
}io_irp_context_t;


/* Forward declarations */
EMCPAL_STATUS sep_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS sep_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS sep_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS sep_read( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS sep_write( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS sep_unload( IN PEMCPAL_DRIVER PEmcpalDriverObject);
static fbe_status_t sep_connect_to_physical_package_control(void);
static fbe_status_t sep_connect_to_physical_package_io(void);
static fbe_status_t sep_connect_to_esp_package_control(void);

static fbe_status_t sep_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static EMCPAL_STATUS sep_mgmt_packet_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS sep_get_control_entry_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS sep_get_io_entry_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS sep_is_service_mode_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS sep_get_semv_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS sep_set_semv_ioctl(PEMCPAL_IRP PIrp);
static fbe_status_t sep_set_contorl_entry_in_esp(void);
static fbe_status_t sep_set_boot_status(void);
static fbe_status_t sep_get_board_object_id(fbe_object_id_t *object_id);
static fbe_status_t sep_set_local_flt_exp_status(void);
static fbe_status_t sep_set_local_veeprom_cpu_status(void);

static sep_device_extention_t * device_extention = NULL;
static PEMCPAL_NATIVE_DRIVER_OBJECT driver_ptr = NULL;

extern void  sep_external_run_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id);
extern void  sep_external_fast_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id);

EMCPAL_STATUS sep_internal_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );

static EMCPAL_STATUS sep_enable_bgs_single_sp(PEMCPAL_IRP PIrp);
static void sep_control_background_services(fbe_bool_t enable_all);

EMCPAL_STATUS EmcpalDriverEntry( IN PEMCPAL_DRIVER  PEmcpalDriverObject )
{
	EMCPAL_STATUS status;
	PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject = EmcpalDriverGetNativeDriverObject(PEmcpalDriverObject);
    fbe_status_t fbe_status;


	fbe_base_package_startup_trace(	FBE_PACKAGE_ID_SEP_0,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "SEP driver, starting...\n");

	status = EmcpalExtDeviceCreate(   PDriverObject,
                               sizeof(sep_device_extention_t),
                               FBE_SEP_DEVICE_NAME,
                               FBE_SEP_DEVICE_TYPE,
                               0,
                               FALSE,
                               &device_object );

	if ( !EMCPAL_IS_SUCCESS(status) ){
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SEP driver, IoCreateDevice failed, status: %X\n", status);
		return status;
	}
	
	/* Init device_extension */
	device_extention = (sep_device_extention_t *) EmcpalDeviceGetExtension(device_object);
	device_extention->device_object = device_object;

	/* Create dispatch points for open, close, ioctl and unload. */
    driver_ptr = PDriverObject;/*keep it so bvd object can use it*/
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, sep_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, sep_close);
	EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_READ, sep_read);
	EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_WRITE, sep_write);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, sep_ioctl);
	EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL, sep_internal_ioctl);/*this is for the BVD interface DGL Read/Write*/
    EmcpalDriverSetUnloadCallback(PEmcpalDriverObject, sep_unload);


    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA( FBE_SEP_DEVICE_LINK, FBE_SEP_DEVICE_NAME );

    if ( !EMCPAL_IS_SUCCESS(status) ) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SEP driver, IoCreateSymbolicLink failed, status: %X\n", status);
		EmcpalExtDeviceDestroy(device_object);
		return status;
    }

	
	EmcutilEventLog(
				   SEP_INFO_LOAD_VERSION,
				   0, NULL, "%s %s",
				   __DATE__, __TIME__);
	

	/* setup connections to physical package */
	sep_connect_to_physical_package_control();
    sep_connect_to_physical_package_io();
   	sep_connect_to_esp_package_control();

	/*we let ESP know what is out control entry (used for drive FW download to talk to PVD) since we were not there when it started*/
	sep_set_contorl_entry_in_esp();

    sep_set_boot_status();

#ifdef C4_INTEGRATED
    /* Log the fault status code to be read via the I2C fault expander.
     * This will provide additional boot-up/init status from the peer SP. 
     * Although, a successful driverEntry status from NTmir is not indicative
     * of 'OS_RUNNING', is it the last driver necessary to load (besides EMCRemote)
     * in order to come up degraded, and be able to log into the array.
     *
     * The NTmir is replaced by SEP Lun, move this from DriveEntry of the NTmir.
     * speclSetLocalFaultExpanderStatus(OS_RUNNING);
     * speclSetLocalVeepromCpuStatus(CSR_OS_RUNNING);
     */

    /* comment out since the speclSetLocalFaultExpanderStatus is not support in codes.
     * sep_set_local_flt_exp_status(); 
     */
    sep_set_local_veeprom_cpu_status();
#endif

    /* sep uses the system threads to queue items, rather than the transport threads */
    fbe_transport_fast_queue_set_external_handler(sep_external_fast_queue_push_queue_element);
    fbe_transport_run_queue_set_external_handler(sep_external_run_queue_push_queue_element);
    
    fbe_status = sep_init(NULL);
    if(fbe_status != FBE_STATUS_OK) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "SEP DRIVER, failed to initialized the SEP, status: %X\n", status);
        
        EmcpalExtDeviceDestroy(device_object);
        EmcpalExtDeviceAliasDestroyA(FBE_SEP_DEVICE_LINK);
        return EMCPAL_STATUS_FAILED_DRIVER_ENTRY;
    }

	/*just before all the objects go wild and use the vault drives, we'll stop here all background services.
	This is a single SP operation here and not the job version of it.
	When all layered drivers are up and running, and when K10Governor finished all the user space processes,
	it will start Navicmom which will send us the IOCTL and will re-enable all the background operations.
	if the re-enable failed, we don't care,  the scheduller will restart them within 15 minutes if they did not get started.
	At this point, all the services were started by the DB and other entieis so when we get here, no one else should re-enable them
	after we'll disable them.*/

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "SEP Driver disabling background services during boot\n");

	/*TEMP HACK !!!!!
	Just until the KH folks fix their startup script to do the same trick, we will NOT stop bgs on KH*/
	#ifdef ALAMOSA_WINDOWS_ENV
	sep_control_background_services(FBE_FALSE);
	#endif
    
    fbe_driver_notification_init(FBE_PACKAGE_ID_SEP_0);
    
	fbe_base_package_startup_trace(	FBE_PACKAGE_ID_SEP_0,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "SEP driver DriverEntry Done.\n");

    return EMCPAL_STATUS_SUCCESS;
}

static fbe_status_t
sep_connect_to_physical_package_control(void)
{
    fbe_package_get_control_entry_t     get_control_entry;
    EMCPAL_STATUS                       status;
    PEMCPAL_FILE_OBJECT                 file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
			      FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR,
			      &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
	fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: physical package DeviceOpen failed, "
			       "status 0x%x\n", __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
			       file_object,
			       FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY_IOCTL,
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
			       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the ioctl entry*/
    fbe_service_manager_set_physical_package_control_entry(get_control_entry.control_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s physical package control_entry:%p\n", 
                           __FUNCTION__,
			   (void *)get_control_entry.control_entry); 
    return FBE_STATUS_OK;
}

static fbe_status_t
sep_connect_to_physical_package_io (void)
{
    fbe_package_get_io_entry_t	get_io_entry;
    EMCPAL_STATUS               status;
    PEMCPAL_FILE_OBJECT         file_object = NULL;

    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
			      FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR,
			      &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
	fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: physical package DeviceOpen failed, "
			       "status 0x%x\n", __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
			       file_object,
			       FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY_IOCTL,
			       &get_io_entry,
			       sizeof (fbe_package_get_io_entry_t),
			       &get_io_entry,
			       sizeof (fbe_package_get_io_entry_t),
			       NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
			       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the io entry*/
    fbe_topology_set_physical_package_io_entry(get_io_entry.io_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s physical package io_entry:%p\n", 
                           __FUNCTION__, (void*)get_io_entry.io_entry); 

    return FBE_STATUS_OK;
}

EMCPAL_STATUS sep_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

	fbe_sep_shim_set_async_io(FBE_TRUE); /* By default we will not break the context */
	fbe_sep_shim_set_async_io_compl(FBE_FALSE); /* By default we will not break the context */

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
	return status;
}

EMCPAL_STATUS sep_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
	return status;
}

EMCPAL_STATUS sep_unload(  IN PEMCPAL_DRIVER PEmcpalDriverObject )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
	PEMCPAL_DEVICE_OBJECT device_object = NULL;
	EMCPAL_KIRQL					starting_irql = EmcpalGetCurrentLevel();
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject = EmcpalDriverGetNativeDriverObject(PEmcpalDriverObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"SEP driver, unloading...\n");

	fbe_driver_notification_destroy();
	
	sep_destroy();

	/*some sanity check*/
    if(EmcpalGetCurrentLevel() > starting_irql) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: sep_destroy() IRQL was left elevated, Starting IRQL: %X, Current IRQL %X\n",
								__FUNCTION__, starting_irql, EmcpalGetCurrentLevel());

        EmcpalLevelLower(starting_irql);
    }

    /* Delete the link from our device name to a name in the Win32 namespace. */
    status =  EmcpalExtDeviceAliasDestroyA( FBE_SEP_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SEP driver, IoDeleteSymbolicLink failed, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);
	
	/* Finally delete our device object */
    EmcpalExtDeviceDestroy(device_object);

	device_extention = NULL;

	return(EMCPAL_STATUS_SUCCESS);
}

EMCPAL_STATUS sep_ioctl( PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP PIrp)
{
	EMCPAL_STATUS status;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;
    ULONG IoControlCode = 0;	

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, entry.\n", __FUNCTION__);

	pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_SEP_DEVICE_TYPE) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"SEP driver,  invalid device, type: %X\n", EmcpalExtDeviceTypeGet(PDeviceObject));
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_BAD_DEVICE_TYPE;
	}
 
	switch ( IoControlCode )
    {
    case FBE_SEP_IS_SERVICE_MODE_IOCTL:
        status = sep_is_service_mode_ioctl(PIrp);
        break;
	case FBE_SEP_MGMT_PACKET_IOCTL:
		status = sep_mgmt_packet_ioctl(PIrp);
		break;
	case FBE_SEP_GET_MGMT_ENTRY_IOCTL:
		status = sep_get_control_entry_ioctl(PIrp);
		break;
	case FBE_SEP_GET_IO_ENTRY_IOCTL:
		status = sep_get_io_entry_ioctl(PIrp);
		break;
	case FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL:
		status = fbe_driver_notification_register_application (PIrp);
		break;
	case FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL:
		status = fbe_driver_notification_get_next_event (PIrp);
		break;
	case FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL:
		status = fbe_driver_notification_unregister_application (PIrp);
		break;
       case FBE_SEP_GET_SEMV_IOCTL:
               status = sep_get_semv_ioctl(PIrp);
               break;
       case FBE_SEP_SET_SEMV_IOCTL:
               status = sep_set_semv_ioctl(PIrp);
               break;
	case FBE_SEP_ENABLE_BACKGROUND_SVC_SINGLE_SP:
		status = sep_enable_bgs_single_sp (PIrp); 
		break;

    default: 
		/*anything else we don't understant, we forward to the sep shim for processing*/
		status = fbe_sep_shim_kernel_process_ioctl_irp(PDeviceObject, PIrp);
		break;
    }

	return status;
}

static EMCPAL_STATUS 
sep_is_service_mode_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_bool_t          *is_service_mode;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	is_service_mode = (fbe_bool_t *)ioBuffer;
    *is_service_mode = sep_is_service_mode();


    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
	EmcpalIrpCompleteRequest(PIrp);
	return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
sep_mgmt_packet_ioctl(PEMCPAL_IRP PIrp)
{
	PEMCPAL_IO_STACK_LOCATION  				irpStack;
	fbe_u8_t * 								ioBuffer = NULL;
	ULONG 									outLength = 0;
	fbe_serialized_control_transaction_t *	user_packet = NULL;
	fbe_packet_t * 							packet = NULL;
	irp_context_t *							irp_context = NULL;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpu_id_t                            cpu_id = FBE_CPU_ID_INVALID;
    
	/*allocate conext for completing the call later, we check later if it's null*/
	irp_context = (irp_context_t *) fbe_allocate_nonpaged_pool_with_tag ( sizeof (irp_context_t),'kpes');

	if (irp_context == NULL) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"SEP, fail to allocate irp_context..\n");
		ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
		user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
		user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;

	}
    
	irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

	ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
	outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);
	user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/

	/*alocate a new packet we will serialize into*/
	packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(packet);

	status = fbe_serialize_lib_unpack_transaction(user_packet, packet, &irp_context->sg_list);
	if (status !=FBE_STATUS_OK) {

		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"SEP, Unable to de-serialize user space packet..\n");

		ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
		user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
		user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		fbe_release_nonpaged_pool_with_tag (irp_context, 'kpes');
        fbe_transport_release_packet (packet);
		return EMCPAL_STATUS_UNSUCCESSFUL;
	}
   
    /*keep the context for later*/
	irp_context->outLength = outLength;
	irp_context->PIrp = PIrp;
	irp_context->user_packet = user_packet;

    /* Set the CPU ID if it is invalid */
    fbe_transport_get_cpu_id(packet, &cpu_id);
    if (cpu_id == FBE_CPU_ID_INVALID) 
    {
        cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(packet, cpu_id);
    }
    
    fbe_transport_set_completion_function (packet, sep_mgmt_packet_ioctl_completion, irp_context);

    /*the order is important here, no not change it
	1. Mark pending
	2. forward the ioctl
	3. return pending
	*/

    EmcpalIrpMarkPending (PIrp);
    fbe_service_manager_send_control_packet(packet);
    return EMCPAL_STATUS_PENDING;
}

static fbe_status_t 
sep_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    irp_context_t *							irp_context = (irp_context_t *)context;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

	/*let copy all the statuses fronm the packet to the transaction we are about to return to user pace*/
	status = fbe_serialize_lib_repack_transaction(irp_context->user_packet, packet);
	if (status == FBE_STATUS_OK) {
		EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_SUCCESS);
	} else {
        EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
	}

    EmcpalExtIrpInformationFieldSet(irp_context->PIrp, irp_context->outLength);

	EmcpalIrpCompleteRequest(irp_context->PIrp);

	/*don't forget to release the context and the memory of the packet, we don't need it anymore*/
    fbe_transport_release_packet (packet);

	
	fbe_release_nonpaged_pool_with_tag (irp_context, 'kpes');

	return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
sep_get_control_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
	fbe_package_get_control_entry_t   * sep_get_control_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	sep_get_control_entry = (fbe_package_get_control_entry_t *)ioBuffer;
	sep_get_control_entry->control_entry = fbe_service_manager_send_control_packet;

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, control_entry: %p.\n",
							__FUNCTION__, sep_get_control_entry->control_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
	EmcpalIrpCompleteRequest(PIrp);
	return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
sep_get_io_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
	fbe_package_get_io_entry_t   * sep_get_io_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	sep_get_io_entry = (fbe_package_get_io_entry_t *)ioBuffer;
	sep_get_io_entry->io_entry = fbe_topology_send_io_packet;

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, io_entry: %p.\n",
							__FUNCTION__, sep_get_io_entry->io_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
	EmcpalIrpCompleteRequest(PIrp);
	return EMCPAL_STATUS_SUCCESS;
}


EMCPAL_STATUS sep_internal_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
	return fbe_sep_shim_kernel_process_internal_ioctl_irp(PDeviceObject, PIrp);
}

fbe_status_t sep_get_driver_object_ptr(PEMCPAL_NATIVE_DRIVER_OBJECT *driver_ptr_in)
{
	if (driver_ptr_in == NULL) {
		return FBE_STATUS_OK;
	}

	*driver_ptr_in = driver_ptr;
	return FBE_STATUS_OK;

}
/*!***************************************************************
 *      get_package_device_object()
 ****************************************************************
 * @brief
 *  This return device object.
 *
 * @param device_object
 * 
 * @return NTSTATUS - Status for package_get_device_object.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS get_package_device_object(PEMCPAL_DEVICE_OBJECT *device_object)
{
    if(device_extention != NULL) 
    {
        *device_object = device_extention->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}
/*************************************************
    end get_package_device_object()
*************************************************/

EMCPAL_STATUS sep_read( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, entry.\n", __FUNCTION__);

	status = fbe_sep_shim_kernel_process_read_irp(PDeviceObject, PIrp);
	return status;
}

EMCPAL_STATUS sep_write( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"SEP driver, %s, entry.\n", __FUNCTION__);

	status = fbe_sep_shim_kernel_process_write_irp(PDeviceObject, PIrp);
	return status;
}

static fbe_status_t
sep_connect_to_esp_package_control (void)
{
    fbe_package_get_control_entry_t	get_control_entry;
    EMCPAL_STATUS                       status;
    PEMCPAL_FILE_OBJECT                 file_object = NULL;

    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
			      FBE_ESP_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
	fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: ESP DeviceOpen failed, status 0x%x\n",
			        __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
			       file_object,
			       FBE_ESP_GET_MGMT_ENTRY_IOCTL,
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       &get_control_entry,
			       sizeof (fbe_package_get_control_entry_t),
			       NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
			       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the io entry*/
    fbe_service_manager_set_esp_package_control_entry(get_control_entry.control_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_SEP_0,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s ESP package control_entry:%p\n", 
                           __FUNCTION__,
			  (void *)get_control_entry.control_entry); 
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * sep_get_semv_ioctl()
 ****************************************************************
 * @brief
 *  This function is called when other modules send us FBE_SEP_GET_SEMV_IOCTL.
 *
 * @param PIrp - pointer of the received IRP
 * 
 * @return EMCPAL_STATUS - Status for sep_get_semv_ioctl.
 *
 * @author
 *  17-May-2012: Created. Zhipeng Hu
 *
 ****************************************************************/
static EMCPAL_STATUS sep_get_semv_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_u64_t          *get_semv;
    fbe_status_t      status;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    get_semv = (fbe_u64_t *)ioBuffer;
    status = fbe_database_get_shared_emv_info(get_semv);
    if (FBE_STATUS_OK == status) 
    {
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);              
    } else {
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
    }  

    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;

}

/*!***************************************************************
 * sep_set_semv_ioctl()
 ****************************************************************
 * @brief
 *  This function is called when other modules send us FBE_SEP_SET_SEMV_IOCTL.
 *
 * @param PIrp - pointer of the received IRP
 * 
 * @return EMCPAL_STATUS - Status for sep_set_semv_ioctl.
 *
 * @author
 *  17-May-2012: Created. Zhipeng Hu
 *
 ****************************************************************/
static EMCPAL_STATUS sep_set_semv_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_u64_t          *set_semv;
    fbe_status_t      status;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    set_semv = (fbe_u64_t *)ioBuffer;
    status = fbe_database_set_shared_emv_info(*set_semv);
    if (FBE_STATUS_OK == status) 
    {
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);              
    } else {
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
    }  

    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;

}


static fbe_status_t sep_set_contorl_entry_in_esp(void)
{
    fbe_package_set_control_entry_t         set_control_entry;
    fbe_rendezvous_event_t                  rendezvous_event;
    EMCPAL_IRP_STATUS_BLOCK                 io_status_block;
    PEMCPAL_IRP                             irp_to_package = NULL;
    EMCPAL_STATUS                           nt_status;
    PEMCPAL_FILE_OBJECT                     file_object = NULL;
    PEMCPAL_DEVICE_OBJECT                   device_object = NULL;

    /*initialize the rendezvous_event*/
    fbe_rendezvous_event_init(&rendezvous_event);

    /*get a pointer to the ESP package*/
    nt_status = EmcpalExtDeviceOpen( FBE_ESP_DEVICE_NAME,
                                        FILE_ALL_ACCESS,
                                        &file_object,
                                        &device_object);

    if (!EMCPAL_IS_SUCCESS(nt_status)) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: ESP IoGetDeviceObjectPointer failed\n", __FUNCTION__); 

        fbe_rendezvous_event_destroy(&rendezvous_event);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	set_control_entry.control_entry = fbe_service_manager_send_control_packet;/*this is out control entry*/

    /*build the ioctl to send*/
    irp_to_package =  EmcpalExtIrpBuildIoctl (FBE_ESP_SET_SEP_CTRL_ENTRY_IOCTL,
                                                  device_object,
                                                  &set_control_entry,
                                                  sizeof (fbe_package_set_control_entry_t),
                                                  &set_control_entry,
                                                  sizeof (fbe_package_set_control_entry_t),
                                                  FALSE,
                                                  fbe_rendezvous_event_get_internal_event(&rendezvous_event),
                                                  &io_status_block);

   if (irp_to_package == NULL) {
       fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: irp_to_package is NULL !!!\n", __FUNCTION__); 
       fbe_rendezvous_event_destroy(&rendezvous_event);
       return FBE_STATUS_GENERIC_FAILURE;
   }

   /*send the ioctl*/
   nt_status = EmcpalExtIrpSendAsync(device_object, irp_to_package);
    if(!EMCPAL_IS_SUCCESS(nt_status) && (nt_status != EMCPAL_STATUS_PENDING)){
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s:IoCallDriver to ESP package failed status 0x%x\n", __FUNCTION__, nt_status); 
        fbe_rendezvous_event_destroy(&rendezvous_event);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We may want to specify timeout to make sure SEP do not hung on us */
    nt_status =  fbe_rendezvous_event_wait(&rendezvous_event, 0);
    if(!EMCPAL_IS_SUCCESS(EmcpalExtIrpStatusBlockStatusFieldGet(&io_status_block))){
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s io request fail with status %x\n", __FUNCTION__, EmcpalExtIrpStatusBlockStatusFieldGet(&io_status_block)); 
        fbe_rendezvous_event_destroy(&rendezvous_event);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Sent control entry:%p to ESP\n", 
                            __FUNCTION__, (void*)set_control_entry.control_entry); 

    fbe_rendezvous_event_destroy(&rendezvous_event);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_set_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
        fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
        fbe_semaphore_release(sem, 0, 1, FALSE);
        return(FBE_STATUS_OK);
}
/*!***************************************************************
 * sep_set_boot_status()
 ****************************************************************
 * @brief
 *  This function is called send set the boot status for SEP via
 *  the physical package
 *
 * 
 * @return fbe_status_t - Status.
 *
 * @author
 *  09-Oct-2012: Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t sep_set_boot_status(void)
{
    fbe_packet_t     *packet = NULL;
    fbe_object_id_t  board_object_id;
    fbe_status_t     status;
    fbe_base_board_set_local_software_boot_status_t boot_status;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    status = sep_get_board_object_id(&board_object_id);
    if((status != FBE_STATUS_OK) || 
       (board_object_id == FBE_OBJECT_ID_INVALID)) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Get board object ID failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Allocate packet */
    packet = fbe_allocate_contiguous_memory(sizeof(fbe_packet_t));
    if(packet == NULL) 
    {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    
    boot_status.componentExtStatus = SW_DRIVER_ENTRY;
    boot_status.componentStatus = SW_COMPONENT_SEP;
    boot_status.generalStatus = SW_GENERAL_STS_NONE;

    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_SOFTWARE_BOOT_STATUS,
                                       &boot_status,
                                       sizeof(fbe_base_board_set_local_software_boot_status_t),
                                       sizeof(fbe_base_board_set_local_software_boot_status_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                board_object_id);

    fbe_transport_set_completion_function(packet, 
                                          fbe_sep_set_completion, 
                                          &sem);
            

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_release_contiguous_memory(packet);

    return FBE_STATUS_OK;

}   // end of sep_set_boot_status

/*!***************************************************************
 * sep_get_board_object_id()
 ****************************************************************
 * @brief
 *  This function is called to get the board object ID
 *
 * 
 * @return fbe_status_t - Status
 *
 * @author
 *  09-Oct-2012: Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t sep_get_board_object_id(fbe_object_id_t *object_id)
{
    fbe_packet_t     *packet = NULL;
    fbe_topology_control_get_board_t	        topology_control_get_board;

    fbe_semaphore_t sem; 
    fbe_semaphore_init(&sem, 0, 1);

    topology_control_get_board.board_object_id = FBE_OBJECT_ID_INVALID;
    /* Allocate packet */
    packet = fbe_allocate_contiguous_memory(sizeof(fbe_packet_t));
    if(packet == NULL) 
    {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    
    fbe_transport_build_control_packet(packet, 
                                       FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                       &topology_control_get_board,
                                       sizeof(fbe_topology_control_get_board_t),
                                       sizeof(fbe_topology_control_get_board_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

    fbe_transport_set_completion_function (packet,
                                           fbe_sep_set_completion,
                                           &sem);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_release_contiguous_memory(packet);

    *object_id = topology_control_get_board.board_object_id ;
    return FBE_STATUS_OK;

}   // end of sep_get_board_object_id


/*this function is used by Navicmom to re-enable the background service that were disabled to start with
if it fails, we don't care, just go on and the scheduller will restart them within 15 minutes if they did not get started here*/
static EMCPAL_STATUS sep_enable_bgs_single_sp(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  						irpStack;
    ULONG              								outLength;

	irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	sep_control_background_services(FBE_TRUE);
    
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
	EmcpalIrpCompleteRequest(PIrp);
    
    return EMCPAL_STATUS_SUCCESS;

}

static void sep_control_background_services(fbe_bool_t enable_all)
{
	fbe_packet_t     *								packet = NULL;
	fbe_base_config_control_system_bg_service_t		control_bgs;

	 /* Allocate packet */
    packet = fbe_transport_allocate_packet ();
    if(packet == NULL) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: failed to allocate packet\n", __FUNCTION__);
        return;
    }

    fbe_transport_initialize_sep_packet(packet);

    control_bgs.enable = enable_all;
	control_bgs.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
	control_bgs.issued_by_ndu = FBE_TRUE;
    control_bgs.issued_by_scheduler = FBE_FALSE;

	fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "SEP Driver asked to %s background services\n", enable_all ? "Enable" : "Disable");
    
    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
                                       &control_bgs,
                                       sizeof(fbe_base_config_control_system_bg_service_t),
                                       sizeof(fbe_base_config_control_system_bg_service_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_BASE_CONFIG,
                                FBE_OBJECT_ID_INVALID);


    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);

    fbe_transport_wait_completion(packet);

    fbe_transport_release_packet(packet);


}

/*!***************************************************************
 * sep_set_local_flt_exp_status()
 ****************************************************************
 * @brief
 *  This function is called send set the local fault expander status for SEP via
 *  the physical package
 *
 * 
 * @return fbe_status_t - Status.
 *
 * @author
 *  04-July-2015: Created. Jibing Dong 
 *
 ****************************************************************/
static fbe_status_t sep_set_local_flt_exp_status(void)
{
    fbe_packet_t     *packet = NULL;
    fbe_object_id_t  board_object_id;
    fbe_status_t     status;
    fbe_u8_t         fault_status;
    fbe_semaphore_t  sem;

    fbe_semaphore_init(&sem, 0, 1);

    status = sep_get_board_object_id(&board_object_id);
    if((status != FBE_STATUS_OK) || 
       (board_object_id == FBE_OBJECT_ID_INVALID)) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Get board object ID failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Allocate packet */
    packet = fbe_allocate_contiguous_memory(sizeof(fbe_packet_t));
    if(packet == NULL) 
    {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    
    fault_status = OS_RUNNING; 

    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_FLT_EXP_STATUS,
                                       &fault_status,
                                       sizeof(fbe_u8_t),
                                       sizeof(fbe_u8_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                board_object_id);

    fbe_transport_set_completion_function(packet, 
                                          fbe_sep_set_completion, 
                                          &sem);
            

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_release_contiguous_memory(packet);

    return FBE_STATUS_OK;

}   // end of sep_set_local_flt_exp_status

/*!***************************************************************
 * sep_set_local_veeprom_cpu_status()
 ****************************************************************
 * @brief
 *  This function is called send set the local veeprom cpu status for SEP via
 *  the physical package
 *
 * 
 * @return fbe_status_t - Status.
 *
 * @author
 *  04-July-2015: Created. Jibing Dong 
 *
 ****************************************************************/
static fbe_status_t sep_set_local_veeprom_cpu_status(void)
{
    fbe_packet_t     *packet = NULL;
    fbe_object_id_t  board_object_id;
    fbe_status_t     status;
    fbe_u32_t        cpu_status;
    fbe_semaphore_t  sem;

    fbe_semaphore_init(&sem, 0, 1);

    status = sep_get_board_object_id(&board_object_id);
    if((status != FBE_STATUS_OK) || 
       (board_object_id == FBE_OBJECT_ID_INVALID)) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Get board object ID failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Allocate packet */
    packet = fbe_allocate_contiguous_memory(sizeof(fbe_packet_t));
    if(packet == NULL) 
    {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    
    cpu_status = CSR_OS_RUNNING;

    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_VEEPROM_CPU_STATUS,
                                       &cpu_status,
                                       sizeof(fbe_u32_t),
                                       sizeof(fbe_u32_t),
                                       NULL,
                                       NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                board_object_id);

    fbe_transport_set_completion_function(packet, 
                                          fbe_sep_set_completion, 
                                          &sem);
            

    fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_release_contiguous_memory(packet);

    return FBE_STATUS_OK;

}   // end of sep_set_boot_status

