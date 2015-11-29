#include <ntddk.h>

#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_neit_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_physical_package.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_neit_flare_package.h"
#include "fbe_neit_private.h"
#include "EmcPAL_Memory.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "EmcPAL_DriverShell.h"


typedef struct neit_package_device_extention_s {
    PEMCPAL_DEVICE_OBJECT device_object;
} neit_package_device_extention_t;
static neit_package_device_extention_t * neit_flare_package_device_extention = NULL;

typedef struct irp_context_s {
    PEMCPAL_IRP                                    PIrp;
    fbe_packet_t *                          user_packet;
    ULONG                                   outLength;
    fbe_payload_control_operation_opcode_t  user_control_code;
}irp_context_t;

typedef struct io_irp_context_s {
    PEMCPAL_IRP                        PIrp;
    ULONG                       outLength;
}io_irp_context_t;

/* Forward declarations */
EMCPAL_STATUS neit_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS neit_package_unload(IN PEMCPAL_DRIVER    pPalDriverObject)   ;

static EMCPAL_STATUS neit_package_add_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_remove_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_remove_object_ioctl(PEMCPAL_IRP PIrp);

static EMCPAL_STATUS neit_package_get_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_start_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_stop_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS neit_package_init_ioctl(PEMCPAL_IRP PIrp);

static EMCPAL_STATUS dest_package_add_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS dest_package_remove_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS dest_package_remove_object_ioctl(PEMCPAL_IRP PIrp);

static EMCPAL_STATUS dest_package_get_record_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS dest_package_start_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS dest_package_stop_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS dest_package_init_ioctl(PEMCPAL_IRP PIrp);


fbe_status_t fbe_get_package_id(fbe_package_id_t *package_id)
{
	*package_id = FBE_PACKAGE_ID_NEIT;
	return FBE_STATUS_OK;
}

EMCPAL_STATUS EmcpalDriverEntry(IN PEMCPAL_DRIVER pPalDriverObject)
{
    EMCPAL_STATUS status;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    neit_package_device_extention_t * device_extention = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;
    EMCPAL_PUNICODE_STRING PRegistryPath = {0};

    PDriverObject = EmcpalDriverGetNativeDriverObject(pPalDriverObject);
    PRegistryPath = EmcpalDriverGetNativeRegistryPath(pPalDriverObject);

    fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
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
        fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "NEIT package DRIVER, IoCreateDevice failed, status: %X\n", status);
        return status;
    }
    
    /* Init device_extension */
    neit_flare_package_device_extention = device_extention = (neit_package_device_extention_t *) EmcpalDeviceGetExtension(device_object);
    device_extention->device_object = device_object;

    /* Create dispatch points for open, close, ioctl and unload. */
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, neit_package_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, neit_package_close);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, neit_package_ioctl);
    EmcpalDriverSetUnloadCallback(pPalDriverObject, (EMCPAL_DRIVER_UNLOAD_CALLBACK)neit_package_unload);

    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA( FBE_NEIT_PACKAGE_DEVICE_LINK, FBE_NEIT_PACKAGE_DEVICE_NAME );

   if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "NEIT package DRIVER, IoCreateSymbolicLink failed, status: %X\n", status);
        EmcpalExtDeviceDestroy(device_object);
        return status;
    }

	fbe_api_common_init_kernel();
	fbe_api_object_map_interface_init();
    neit_flare_package_init();

    return EMCPAL_STATUS_SUCCESS;
}

EMCPAL_STATUS neit_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
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

    fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

EMCPAL_STATUS neit_package_unload(IN PEMCPAL_DRIVER    pPalDriverObject)    
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    neit_package_device_extention_t * device_extention = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;

    PDriverObject = (PEMCPAL_NATIVE_DRIVER_OBJECT)EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "NEIT package DRIVER, unloading...\n");

    neit_flare_package_destroy();
	fbe_api_object_map_interface_destroy();
	fbe_api_common_destroy_kernel();


    /* Delete the link from our device name to a name in the Win32 namespace. */
    status =  EmcpalExtDeviceAliasDestroyA( FBE_NEIT_PACKAGE_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "neit pkg DRIVER, IoDeleteSymbolicLink failed, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);
    device_extention = (neit_package_device_extention_t *) EmcpalDeviceGetExtension(device_object);

    /* Finally delete our device object */
    EmcpalExtDeviceDestroy(device_object);

    return EMCPAL_STATUS_SUCCESS;
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
                            "%s, entry. IoControlCode: %d \n", __FUNCTION__, IoControlCode);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_NEIT_PACKAGE_DEVICE_TYPE) {
        fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid device, type: %X\n", __FUNCTION__ , EmcpalExtDeviceTypeGet(PDeviceObject));

        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_BAD_DEVICE_TYPE;
    }
 
    switch ( IoControlCode ) {
        case FBE_NEIT_PACKAGE_ADD_RECORD_IOCTL:
            status = neit_package_add_record_ioctl(PIrp);
            break;
            
        case FBE_NEIT_PACKAGE_REMOVE_RECORD_IOCTL:
            status = neit_package_remove_record_ioctl(PIrp);
            break;

        case FBE_NEIT_PACKAGE_REMOVE_OBJECT_IOCTL:
            status = neit_package_remove_object_ioctl(PIrp);
            break;

        case FBE_NEIT_PACKAGE_GET_RECORD_IOCTL:
            status = neit_package_get_record_ioctl(PIrp);
            break;

        case FBE_NEIT_PACKAGE_START_IOCTL:
            status = neit_package_start_ioctl(PIrp);
            break;

        case FBE_NEIT_PACKAGE_STOP_IOCTL:
            status = neit_package_stop_ioctl(PIrp);
            break;

        case FBE_NEIT_PACKAGE_INIT_IOCTL:
            status = neit_package_init_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_ADD_RECORD_IOCTL:
            status = dest_package_add_record_ioctl(PIrp);
            break;
            

        case FBE_DEST_PACKAGE_REMOVE_RECORD_IOCTL:
            status = dest_package_remove_record_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_REMOVE_OBJECT_IOCTL:
            status = dest_package_remove_object_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_GET_RECORD_IOCTL:
            status = dest_package_get_record_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_START_IOCTL:
            status = dest_package_start_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_STOP_IOCTL:
            status = dest_package_stop_ioctl(PIrp);
            break;

        case FBE_DEST_PACKAGE_INIT_IOCTL:
            status = dest_package_init_ioctl(PIrp);
            break;


        default: 
            fbe_base_package_trace( FBE_PACKAGE_ID_NEIT,
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
neit_package_add_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_neit_package_add_record_t *         record = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_neit_package_add_record_t *)ioBuffer;

    neit_package_add_record(&record->neit_error_record, &record->neit_record_handle);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_remove_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_neit_package_remove_record_t *      record = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_neit_package_remove_record_t *)ioBuffer;

    neit_package_remove_record(record->neit_record_handle);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_remove_object_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_neit_package_remove_object_t *      remove_object = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    remove_object = (fbe_neit_package_remove_object_t *)ioBuffer;

    neit_package_remove_object(remove_object->object_id);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_get_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_neit_package_get_record_t *         record = NULL;
    fbe_status_t status;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_neit_package_get_record_t *)ioBuffer;

    status = neit_package_get_record(record->neit_record_handle, &record->neit_error_record);

    if (status == FBE_STATUS_OK) {
        status = EMCPAL_STATUS_SUCCESS;
    }   
    else
    {
        status = EMCPAL_STATUS_INVALID_HANDLE;
    }
    
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

static EMCPAL_STATUS 
neit_package_start_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    neit_package_start();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_stop_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    neit_package_stop();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
neit_package_init_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    neit_package_cleanup_queue();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}


/* Drive Error Simulation Tool ioctl functions */

static EMCPAL_STATUS 
dest_package_add_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_dest_add_record_t *                 record = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_dest_add_record_t *)ioBuffer;

    fbe_dest_add_record(&record->dest_error_record, &record->dest_record_handle);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
dest_package_remove_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_dest_remove_record_t *              record = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_dest_remove_record_t *)ioBuffer;

    fbe_dest_remove_record(record->dest_record_handle);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
dest_package_remove_object_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_dest_remove_object_t *      remove_object = NULL;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    remove_object = (fbe_dest_remove_object_t *)ioBuffer;

    fbe_dest_remove_object(remove_object->object_id);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
dest_package_get_record_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_dest_get_record_t *         record = NULL;
    fbe_status_t status;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    record = (fbe_dest_get_record_t *)ioBuffer;

    status = fbe_dest_get_record(record->dest_record_handle, &record->dest_error_record);

    if (status == FBE_STATUS_OK) {
        status = EMCPAL_STATUS_SUCCESS;
    }   
    else
    {
        status = EMCPAL_STATUS_INVALID_HANDLE;
    }
    
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

static EMCPAL_STATUS 
dest_package_start_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    fbe_dest_start();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
dest_package_stop_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    fbe_dest_stop();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
dest_package_init_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION                      irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    fbe_dest_cleanup_queue();

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}


EMCPAL_STATUS get_package_device_object(PEMCPAL_DEVICE_OBJECT *device_object)
{
    if(neit_flare_package_device_extention != NULL) 
    {
        *device_object = neit_flare_package_device_extention->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}
