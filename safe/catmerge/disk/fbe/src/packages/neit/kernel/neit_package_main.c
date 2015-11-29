
#include <ntddk.h>
#include "EmcPAL_DriverShell.h"

#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_neit_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_sep.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "EmcPAL_Memory.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_driver_notification_interface.h"
#include "fbe/fbe_rdgen.h"

typedef struct neit_package_device_extention_s {
    PEMCPAL_DEVICE_OBJECT device_object;
} neit_package_device_extention_t;
static neit_package_device_extention_t * neit_package_device_extention = NULL;

typedef struct irp_context_s {
	PEMCPAL_IRP 							PIrp;
	fbe_serialized_control_transaction_t * 	user_packet;
	ULONG 									outLength;
	fbe_payload_control_operation_opcode_t	user_control_code;
	void *									sg_list;
}irp_context_t;

typedef struct io_irp_context_s {
	PEMCPAL_IRP 						PIrp;
	ULONG 						outLength;
}io_irp_context_t;

/* Globals */
extern void rdgen_external_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id);


/* Forward declarations */
EMCPAL_STATUS neit_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_unload( IN PEMCPAL_DRIVER PPalDriverObject);

static EMCPAL_STATUS neit_package_mgmt_packet_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_get_control_entry_ioctl(PEMCPAL_IRP PIrp);

static fbe_status_t 
neit_package_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
neit_package_mgmt_packet_ioctl_rdgen_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static void neit_package_destroy_notification_callback(update_object_msg_t * update_object_msg, void * context);
static EMCPAL_STATUS neit_package_init_ioctl(PEMCPAL_IRP PIrp);

static fbe_status_t neit_connect_to_physical_package_control(void);
static fbe_status_t neit_connect_to_physical_package_io (void);

static fbe_status_t neit_connect_to_sep_control(void);
static fbe_status_t neit_connect_to_sep_io (void);
static fbe_status_t allocate_memory_for_rdgen_control(fbe_packet_t *packet_p);

EMCPAL_STATUS EmcpalDriverEntry( IN PEMCPAL_DRIVER  PEmcpalDriverObject )
{
	EMCPAL_STATUS status;
	PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject = EmcpalDriverGetNativeDriverObject(PEmcpalDriverObject);
    
	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"NEIT package DRIVER, starting...\n");

	status = EmcpalExtDeviceCreate(   PDriverObject,
                               sizeof(neit_package_device_extention_t),
                               FBE_NEIT_PACKAGE_DEVICE_NAME,
                               FBE_NEIT_PACKAGE_DEVICE_TYPE,
                               0,
                               FALSE,
                               &device_object );

	if ( !EMCPAL_IS_SUCCESS(status) ){
		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"NEIT package DRIVER, IoCreateDevice failed, status: %X\n", status);
		return status;
	}
	
	/* Init device_extension */
	neit_package_device_extention = (neit_package_device_extention_t *) EmcpalDeviceGetExtension(device_object);
	neit_package_device_extention->device_object = device_object;

	/* Create dispatch points for open, close, ioctl and unload. */
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, neit_package_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, neit_package_close);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, neit_package_ioctl);
    EmcpalDriverSetUnloadCallback(PEmcpalDriverObject, neit_package_unload);

    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA( FBE_NEIT_PACKAGE_DEVICE_LINK, FBE_NEIT_PACKAGE_DEVICE_NAME );

    if ( !EMCPAL_IS_SUCCESS(status) ) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"NEIT package DRIVER, IoCreateSymbolicLink failed, status: %X\n", status);
		EmcpalExtDeviceDestroy(device_object);
		return status;
    }

    neit_connect_to_physical_package_control();
	neit_connect_to_physical_package_io();

	neit_connect_to_sep_control();
	neit_connect_to_sep_io();

    fbe_rdgen_queue_set_external_handler(rdgen_external_queue_push_queue_element);

	neit_package_init(NULL);
    fbe_driver_notification_init(FBE_PACKAGE_ID_NEIT);

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"NEIT package DRIVER, Loaded !\n");

	return EMCPAL_STATUS_SUCCESS;
}

EMCPAL_STATUS neit_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
	return status;
}

EMCPAL_STATUS neit_package_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
	return status;
}

EMCPAL_STATUS neit_package_unload(  IN PEMCPAL_DRIVER PPalDriverObject )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
	PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject = EmcpalDriverGetNativeDriverObject(PPalDriverObject);
	EMCPAL_KIRQL					starting_irql = EmcpalGetCurrentLevel();

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"NEIT package DRIVER, unloading...\n");
	
    fbe_driver_notification_destroy();
    
	neit_package_destroy();

	/*some sanity check*/
    if(EmcpalGetCurrentLevel() > starting_irql) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_SEP_0,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: neit_package_destroy() IRQL was left elevated, Starting IRQL: %X, Current IRQL %X\n",
								__FUNCTION__, starting_irql, EmcpalGetCurrentLevel());

        EmcpalLevelLower(starting_irql);
    }

    /* Delete the link from our device name to a name in the Win32 namespace. */
    status =  EmcpalExtDeviceAliasDestroyA( FBE_NEIT_PACKAGE_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"neit pkg DRIVER, IoDeleteSymbolicLink failed, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);

	/* Finally delete our device object */
    EmcpalExtDeviceDestroy(device_object);

    /* Remove reference to the deleted device object */
    neit_package_device_extention =  NULL;

    return(EMCPAL_STATUS_SUCCESS);
}

EMCPAL_STATUS neit_package_ioctl( PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP PIrp)
{
	EMCPAL_STATUS status;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;
    ULONG IoControlCode = 0;	

	pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s, entry. IoControlCode: %X \n", __FUNCTION__, IoControlCode);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_NEIT_PACKAGE_DEVICE_TYPE) {
		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid device, type: %X\n", __FUNCTION__ , EmcpalExtDeviceTypeGet(PDeviceObject));

		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_BAD_DEVICE_TYPE;
	}
 
	switch ( IoControlCode ) {
		case FBE_NEIT_MGMT_PACKET_IOCTL:
			status = neit_package_mgmt_packet_ioctl(PIrp);
			break;
		case FBE_NEIT_GET_MGMT_ENTRY_IOCTL:
			status = neit_package_get_control_entry_ioctl(PIrp);
			break;
        case FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL:
            status = fbe_driver_notification_register_application (PIrp);
            break;
        case FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL:
                status = fbe_driver_notification_get_next_event (PIrp);
            break;
        case FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL:
            status = fbe_driver_notification_unregister_application (PIrp);
            break;
		default: 
			fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Invalid IoControlCode: %X\n", __FUNCTION__ , IoControlCode);

			EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
			EmcpalExtIrpInformationFieldSet(PIrp, 0);
			status = EmcpalExtIrpStatusFieldGet(PIrp);
			EmcpalIrpCompleteRequest(PIrp);
			break;
    }

	return status;
}

static EMCPAL_STATUS 
neit_package_mgmt_packet_ioctl(PEMCPAL_IRP PIrp)
{
	PEMCPAL_IO_STACK_LOCATION  				irpStack;
	fbe_u8_t * 								ioBuffer = NULL;
	ULONG 									outLength = 0;
	fbe_serialized_control_transaction_t *	user_packet = NULL;
	fbe_packet_t * 							packet = NULL;
	irp_context_t *							irp_context = NULL;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    
	/*allocate conext for completing the call later, we check later if it's null*/
	irp_context = (irp_context_t *) fbe_allocate_nonpaged_pool_with_tag(sizeof (irp_context_t),'tien');

	if (irp_context == NULL) {
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
	user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get the transaction from the user*/

    /*alocate a new packet we will serialize into*/
	packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(packet);

	status = fbe_serialize_lib_unpack_transaction(user_packet, packet, &irp_context->sg_list);
	if (status != FBE_STATUS_OK) {

		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"NEIT pkg DRIVER, Unable to de-serialize user space packet..\n");

		ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
		user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
		user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		fbe_release_nonpaged_pool_with_tag (irp_context, 'tien');
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;
	}
   
	/*keep the context for later*/
	irp_context->outLength = outLength;
	irp_context->PIrp = PIrp;
	irp_context->user_packet = user_packet;
	
    /* If RDGEN attrib is set, we need to immediately complete the packet and not hold on to it. */
    if(user_packet->packet_attr & FBE_PACKET_FLAG_RDGEN)
    {
        fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s irp_context:0x%p\n", __FUNCTION__, irp_context);

		fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "irp_context_content:ol:%d, pirp:0x%p, packet:0x%p\n", irp_context->outLength, irp_context->PIrp, irp_context->user_packet);

        user_packet->packet_status.code = FBE_STATUS_OK;
		user_packet->packet_status.qualifier = 0;
		user_packet->payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        
        /* Complete the IRP in this case as the client or the parent process will be waiting for it. */
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, outLength);
        
        fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Sending the 0x%p packet packet for RDGEN attrib..\n", packet);

		fbe_release_nonpaged_pool_with_tag (irp_context, 'tien');

		/*before we send the packet let's set it's attribute to make sure rdgen will not try to 
		copy into it's buffer anything because this buffer is gone*/
		fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_RDGEN);

		/* we alos need to replace the buffer associated with this packet with a buffer that will stay here even as the IRP is completed.
		The reason here that the RDGEN function that is being called by this packet will have some asynch elements here which will try to access
		this buffer so we want to leave it here until completion
		*/
		status = allocate_memory_for_rdgen_control(packet);
		if (status != FBE_STATUS_OK) {
			fbe_transport_release_packet (packet);
			ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
			user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
			user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
			EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
			EmcpalExtIrpInformationFieldSet(PIrp, 0);
			EmcpalIrpCompleteRequest(PIrp);
			return EMCPAL_STATUS_UNSUCCESSFUL;
		}

		fbe_transport_set_completion_function(packet, neit_package_mgmt_packet_ioctl_rdgen_completion, irp_context);

		fbe_service_manager_send_control_packet(packet);

		EmcpalIrpCompleteRequest(PIrp);
		
		return EMCPAL_STATUS_SUCCESS;
    }
   
	fbe_transport_set_completion_function(packet, neit_package_mgmt_packet_ioctl_completion, irp_context);

	/* the order is important here, do not change it
	 * 1. Mark pending
	 * 2. forward the ioctl
	 * 3. return pending
	 */

	EmcpalIrpMarkPending (PIrp);
	fbe_service_manager_send_control_packet(packet);
	return EMCPAL_STATUS_PENDING;
}

static fbe_status_t 
neit_package_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    irp_context_t *irp_context = (irp_context_t *)context;
    fbe_status_t   status = FBE_STATUS_GENERIC_FAILURE;

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

    fbe_release_nonpaged_pool_with_tag (irp_context, 'tien');

    return EMCPAL_STATUS_SUCCESS;
}

static fbe_status_t 
neit_package_mgmt_packet_ioctl_rdgen_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{

	fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_ex_t  *payload_p = NULL;
    void * buffer_ptr = NULL;

	payload_p = fbe_transport_get_payload_ex(packet);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_ptr);
    if (buffer_ptr == NULL){
         fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: can't get buffer from packet 0x%p, this is a Leak\n", __FUNCTION__, packet); 
        
    }else{
        
		/*don't forget to release the buffer we allocated for the control operation anymore*/
		fbe_release_nonpaged_pool_with_tag(buffer_ptr, 'dgdr');

	}

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"Releasing the packet 0x%p for RDGEN attrib..\n", packet);

    fbe_transport_release_packet (packet);

    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_get_control_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
	fbe_package_get_control_entry_t   * neit_package_get_control_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	neit_package_get_control_entry = (fbe_package_get_control_entry_t *)ioBuffer;
	neit_package_get_control_entry->control_entry = fbe_service_manager_send_control_packet;

	fbe_base_package_trace(	FBE_PACKAGE_ID_NEIT,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"NEIT_PACKAGE driver, %s, control_entry: %p.\n",
							__FUNCTION__, neit_package_get_control_entry->control_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
	EmcpalIrpCompleteRequest(PIrp);
	return EMCPAL_STATUS_SUCCESS;
}

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_NEIT;
	return FBE_STATUS_OK;
}

static fbe_status_t
neit_connect_to_sep_control(void)
{
    fbe_package_get_control_entry_t     get_control_entry;
    EMCPAL_STATUS                       status;
    PEMCPAL_FILE_OBJECT                 file_object = NULL;

    /*get a pointer to the physical package*/
    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_SEP_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: SEP DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status); 

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_SEP_GET_MGMT_ENTRY_IOCTL,
                               &get_control_entry,
                               sizeof (fbe_package_get_control_entry_t),
                               &get_control_entry,
                               sizeof (fbe_package_get_control_entry_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now that we got back, we save for future use the ioctl entry*/
    fbe_service_manager_set_sep_control_entry(get_control_entry.control_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s SEP control_entry:%p\n", 
                           __FUNCTION__,
                          (void *)get_control_entry.control_entry); 
    return FBE_STATUS_OK;
}

static fbe_status_t
neit_connect_to_physical_package_control(void)
{
    fbe_package_get_control_entry_t     get_control_entry;
    EMCPAL_STATUS                       status;
    PEMCPAL_FILE_OBJECT                 file_object = NULL;

    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR,
                              &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Physical Package DeviceOpen failed, "
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
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now that we got back, we save for future use the ioctl entry*/
    fbe_service_manager_set_physical_package_control_entry(get_control_entry.control_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Physical Package control_entry:%p\n", 
                           __FUNCTION__,
                          (void *)get_control_entry.control_entry); 
    return FBE_STATUS_OK;
}

static fbe_status_t
neit_connect_to_physical_package_io (void)
{
    fbe_package_get_io_entry_t  get_io_entry;
    EMCPAL_STATUS               status;
    PEMCPAL_FILE_OBJECT         file_object = NULL;

    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR,
                              &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Physical Package DeviceOpen failed, "
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
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the io entry*/
    fbe_topology_set_physical_package_io_entry(get_io_entry.io_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s physical package io_entry:%p\n", 
                           __FUNCTION__, (void *)get_io_entry.io_entry); 
    return FBE_STATUS_OK;
}

static fbe_status_t
neit_connect_to_sep_io (void)
{
    fbe_package_get_io_entry_t  get_io_entry;
    EMCPAL_STATUS               status;
    PEMCPAL_FILE_OBJECT         file_object = NULL;

    status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
                              FBE_SEP_DEVICE_NAME_CHAR, &file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: SEP DeviceOpen failed, status 0x%x\n",
                               __FUNCTION__, status); 

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We may want to specify timeout to make sure SEP do not hung on us */
    status = EmcpalDeviceIoctl(EmcpalDriverGetCurrentClientObject(),
                               file_object,
                               FBE_SEP_GET_IO_ENTRY_IOCTL,
                               &get_io_entry,
                               sizeof (fbe_package_get_io_entry_t),
                               &get_io_entry,
                               sizeof (fbe_package_get_io_entry_t),
                               NULL);

    (void) EmcpalDeviceClose(file_object);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s io request fail with status %x\n",
                               __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now that we got back, we save for future use the io entry*/
    fbe_topology_set_sep_io_entry(get_io_entry.io_entry);

    fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s SEP io_entry:%p\n", 
                           __FUNCTION__, (void *)get_io_entry.io_entry); 
    return FBE_STATUS_OK;
}

EMCPAL_STATUS get_package_device_object(PEMCPAL_DEVICE_OBJECT *device_object)
{
    if(neit_package_device_extention != NULL) 
    {
        *device_object = neit_package_device_extention->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}

/*!***************************************************************
 *      fbe_api_neit_package_common_init()
 ****************************************************************
 * @brief
 *  This function initialized the fbe_api for kernel.
 *
 * @param void
 * 
 * @return fbe_status_t
 *
 * History:
 *  05/01/2012  Created. kothal
 *
 ****************************************************************/
fbe_status_t fbe_api_neit_package_common_init(void)
{
    return(fbe_api_common_init_kernel());
}
/***********************************
    end fbe_api_neit_package_common_init()    
************************************/

/*!***************************************************************
 *      fbe_api_neit_package_common_destroy()
 ****************************************************************
 * @brief
 *  This function destroys the fbe_api for kernel.
 *
 * @param void
 * 
 * @return fbe_status_t
 *
 * History:
 *  05/01/2012  Created. kothal
 *
 ****************************************************************/
fbe_status_t fbe_api_neit_package_common_destroy(void)
{
    return(fbe_api_common_destroy_kernel());
}
/***********************************
    end fbe_api_neit_package_common_destroy()    
************************************/


static fbe_status_t allocate_memory_for_rdgen_control(fbe_packet_t *packet_p)
{
	fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_u32_t len;
    fbe_payload_ex_t  *payload_p = NULL;
	void * driver_level_buffer = NULL;
	void * buffer_ptr = NULL;

	payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = (fbe_payload_control_operation_t *)payload_p->next_operation;/*a bad hack but this whoe rdgen spli start is a hack....*/

    fbe_payload_control_get_buffer(control_operation_p, &buffer_ptr);
    if (buffer_ptr == NULL)
    {
         fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: can't get buffer from packet 0x%p\n", __FUNCTION__, packet_p); 
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);

	/*allocate this memory*/
	driver_level_buffer = fbe_allocate_nonpaged_pool_with_tag ( len, 'dgdr');
	if (driver_level_buffer == NULL) {

         fbe_base_package_trace(FBE_PACKAGE_ID_NEIT,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: can't allocate memory for packet 0x%p\n", __FUNCTION__, packet_p); 
        
        return FBE_STATUS_GENERIC_FAILURE;
	}

	/*and copy it to it the current buffer*/
	fbe_copy_memory(driver_level_buffer, buffer_ptr, len);

	/*and give the packet the new buffer*/
	control_operation_p->buffer = driver_level_buffer;/*a bit ugly but nothing we can do here*/

	/*Coverity may cry here that we don't free the memory, oh wel....
	we all know we do that at neit_package_mgmt_packet_ioctl_rdgen_completion*/
	return FBE_STATUS_OK;
    
}
