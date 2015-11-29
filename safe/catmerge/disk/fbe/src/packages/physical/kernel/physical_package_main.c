
#include <ntddk.h>

#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "physical_package_kernel_interface.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe_api_common_transport.h"
#include "fbe_api_enclosure_interface.h"
#include "fbe_api_object_map_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "klogservice.h"
#include "EmcPAL_Memory.h"
#include "fbe/fbe_driver_notification_interface.h"
#include "fbe/fbe_package.h"
#include "fbe_packet_serialize_lib.h"
#include "EmcPAL_DriverShell.h"
#include "EmcUTIL.h"

#ifdef C4_INTEGRATED
#include "fbe/fbe_api_common.h"
#include "EmcPAL_Irp_Extensions.h"
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */


typedef struct physical_package_device_extention_s {
    PEMCPAL_DEVICE_OBJECT device_object;
} physical_package_device_extention_t;

typedef struct irp_context_s {
    PEMCPAL_IRP 							PIrp;
    fbe_serialized_control_transaction_t * 	user_packet;
    ULONG 							outLength;
    void *									sg_list;
}irp_context_t;

typedef struct io_irp_context_s {
    PEMCPAL_IRP 						PIrp;
    ULONG 						outLength;
}io_irp_context_t;

/* Forward declarations */
EMCPAL_STATUS physical_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS physical_package_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS physical_package_ioctl( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp );
EMCPAL_STATUS physical_package_unload(IN PEMCPAL_DRIVER       pPalDriverObject);

static fbe_status_t physical_package_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static EMCPAL_STATUS physical_package_mgmt_packet_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS physical_package_get_control_entry_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS physical_package_get_io_entry_ioctl(PEMCPAL_IRP PIrp);

static physical_package_device_extention_t * device_extention = NULL;

EMCPAL_STATUS EmcpalDriverEntry(IN PEMCPAL_DRIVER pPalDriverObject)
{
    EMCPAL_STATUS status;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    fbe_status_t fbe_status;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;
    EMCPAL_PUNICODE_STRING PRegistryPath = {0};

    PDriverObject = EmcpalDriverGetNativeDriverObject(pPalDriverObject);
    PRegistryPath = EmcpalDriverGetNativeRegistryPath(pPalDriverObject);
    
    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "physical pkg DRIVER, starting...\n");

    status = EmcpalExtDeviceCreate(   PDriverObject,
                               sizeof(physical_package_device_extention_t),
                               FBE_PHYSICAL_PACKAGE_DEVICE_NAME,
                               FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,
                               0,
                               FALSE,
                               &device_object );

    if ( !EMCPAL_IS_SUCCESS(status) ){
        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "physical pkg DRIVER, IoCreateDevice failed, status: %X\n", status);
        return status;
    }
    
    /* Init device_extension */
    device_extention = (physical_package_device_extention_t *) EmcpalDeviceGetExtension(device_object);
    device_extention->device_object = device_object;

    /* Create dispatch points for open, close, ioctl and unload. */
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, physical_package_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, physical_package_close);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, physical_package_ioctl);
    EmcpalDriverSetUnloadCallback(pPalDriverObject, (EMCPAL_DRIVER_UNLOAD_CALLBACK)physical_package_unload);

    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA( FBE_PHYSICAL_PACKAGE_DEVICE_LINK, FBE_PHYSICAL_PACKAGE_DEVICE_NAME );

    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "physical pkg DRIVER, IoCreateSymbolicLink failed, status: %X\n", status);
        EmcpalExtDeviceDestroy(device_object);
        return status;
    }

    EmcutilEventLog(
                   PHYSICAL_PACKAGE_INFO_LOAD_VERSION,
                   0, NULL, "%s %s",
                   __DATE__, __TIME__);

    fbe_status = physical_package_init();
    if(fbe_status != FBE_STATUS_OK) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "physical pkg DRIVER, failed to initialized the physical pkg, status: %X\n", status);

        EmcpalExtDeviceDestroy(device_object);
        EmcpalExtDeviceAliasDestroyA(FBE_PHYSICAL_PACKAGE_DEVICE_LINK);
        return EMCPAL_STATUS_FAILED_DRIVER_ENTRY;
    }

    fbe_driver_notification_init(FBE_PACKAGE_ID_PHYSICAL);

    return EMCPAL_STATUS_SUCCESS;
}

EMCPAL_STATUS physical_package_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "physical pkg DRIVER, %s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

EMCPAL_STATUS physical_package_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "physical pkg DRIVER, %s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

EMCPAL_STATUS physical_package_unload(IN PEMCPAL_DRIVER       pPalDriverObject)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;

    PDriverObject = (PEMCPAL_NATIVE_DRIVER_OBJECT)EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "physical pkg DRIVER, unloading...\n");

    fbe_driver_notification_destroy();
    
    physical_package_destroy();

    /* Delete the link from our device name to a name in the Win32 namespace. */
    status =  EmcpalExtDeviceAliasDestroyA( FBE_PHYSICAL_PACKAGE_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "physical pkg DRIVER, IoDeleteSymbolicLink failed, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);
    
    /* Finally delete our device object */
    EmcpalExtDeviceDestroy(device_object);

    device_extention = NULL;

    return EMCPAL_STATUS_SUCCESS;
}

#ifdef C4_INTEGRATED

/* called from rdevice ioct by fbe_cli */
csx_status_e fbe_api_common_packet_from_fbe_cli (fbe_u32_t ioctl_code, csx_pvoid_t inBuffPtr, csx_pvoid_t outBuffPtr)
{
    fbe_status_t                      fbe_status;
    fbe_payload_control_buffer_t      buffer;
    unsigned char                     *rdevice_in_buff_ptr;
    unsigned char                     *rdevice_out_buff_ptr;
    fbe_api_fbe_cli_rdevice_inbuff_t  *fbe_cli_rdevice_in_data_ptr;
    fbe_api_fbe_cli_rdevice_outbuff_t *fbe_cli_rdevice_out_data_ptr;
    fbe_api_fbe_cli_rdevice_read_buffer_t *fbe_cli_rdevice_in_read_buffer_ptr;
    fbe_api_fbe_cli_rdevice_read_buffer_t *fbe_cli_rdevice_out_read_buffer_ptr;
    fbe_eses_read_buf_desc_t               *read_buf_desc_p;


    rdevice_out_buff_ptr = (unsigned char *) outBuffPtr;
    rdevice_in_buff_ptr = (unsigned char *) inBuffPtr;

    if (ioctl_code == FBE_CLI_SEND_PKT_IOCTL)
    {  
        fbe_payload_control_buffer_t  in_buffer;
        fbe_cli_rdevice_in_data_ptr = (fbe_api_fbe_cli_rdevice_inbuff_t  *) inBuffPtr;
        fbe_cli_rdevice_out_data_ptr = (fbe_api_fbe_cli_rdevice_outbuff_t *) outBuffPtr;

        if(0) csx_p_display_std_note ("%s, calling fbe_api_common_send_control_packet...\n", __FUNCTION__);

        in_buffer = (fbe_payload_control_buffer_t) (rdevice_in_buff_ptr + sizeof(fbe_api_fbe_cli_rdevice_inbuff_t));
        buffer = (fbe_payload_control_buffer_t) (rdevice_out_buff_ptr + sizeof(fbe_api_fbe_cli_rdevice_outbuff_t));
         /* copy in_buffer content to buffer which will also be the output buffer */
        fbe_copy_memory (buffer, in_buffer, fbe_cli_rdevice_in_data_ptr->buffer_length);

        fbe_status =  fbe_api_common_send_control_packet(fbe_cli_rdevice_in_data_ptr->control_code,
                                                buffer,
                                                (fbe_payload_control_buffer_length_t) fbe_cli_rdevice_in_data_ptr->buffer_length,
                                                fbe_cli_rdevice_in_data_ptr->object_id,
                                                fbe_cli_rdevice_in_data_ptr->attr,
                                                &(fbe_cli_rdevice_out_data_ptr->control_status_info),
                                                fbe_cli_rdevice_in_data_ptr->package_id);
    }
    else
    {
        fbe_cli_rdevice_in_read_buffer_ptr = (fbe_api_fbe_cli_rdevice_read_buffer_t *) inBuffPtr;
        fbe_cli_rdevice_out_read_buffer_ptr = (fbe_api_fbe_cli_rdevice_read_buffer_t *) outBuffPtr;
        read_buf_desc_p = (fbe_eses_read_buf_desc_t *) 
                          fbe_api_allocate_memory (fbe_cli_rdevice_in_read_buffer_ptr->operation.response_buf_size);
        if (read_buf_desc_p == CSX_NULL)
        {
            csx_p_display_std_note ("%s, failled allocating %d bytes from io memory...\n", 
                                    __FUNCTION__, fbe_cli_rdevice_in_read_buffer_ptr->operation.response_buf_size);

            return CSX_STATUS_GENERAL_FAILURE;
        }
        fbe_cli_rdevice_in_read_buffer_ptr->operation.response_buf_p = (fbe_u8_t *)read_buf_desc_p; 
        fbe_status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data (fbe_cli_rdevice_in_read_buffer_ptr->object_id,
                                                    &(fbe_cli_rdevice_in_read_buffer_ptr->operation),
                                                    fbe_cli_rdevice_in_read_buffer_ptr->control_code,
                                                    &(fbe_cli_rdevice_in_read_buffer_ptr->enclosure_status));
        fbe_copy_memory (fbe_cli_rdevice_out_read_buffer_ptr, fbe_cli_rdevice_in_read_buffer_ptr,
                         sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t));
        fbe_copy_memory (outBuffPtr+sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t),
                         read_buf_desc_p, fbe_cli_rdevice_in_read_buffer_ptr->operation.response_buf_size);
        fbe_api_free_memory(read_buf_desc_p);
    }
    
    if (fbe_status == FBE_STATUS_OK)
    {
        if(0) csx_p_display_std_note ("%s, fbe_api_common_send_control_packet succeeded...\n", __FUNCTION__);
        return CSX_STATUS_SUCCESS;
    }
    csx_p_display_std_note ("%s, calling fbe_api_common_send_control_packet failed...\n", __FUNCTION__);

    return CSX_STATUS_GENERAL_FAILURE;
}


csx_status_e fbe_api_common_map_interface_get_from_fbe_cli (csx_pvoid_t inBuffPtr, csx_pvoid_t outBuffPtr)
{
   fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;
   fbe_api_fbe_cli_rdevice_inbuff_t              *fbe_cli_rdevice_in_data_ptr = 
                                                 (fbe_api_fbe_cli_rdevice_inbuff_t *) inBuffPtr;
   fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t  *fbe_cli_rdevice_out_data_ptr = 
                                                  (fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t  *) outBuffPtr;

   switch (fbe_cli_rdevice_in_data_ptr->fbe_cli_map_interface_code)
   {
       case FBE_CLI_MAP_INTERFACE_GET_PORT:
           fbe_status = fbe_api_object_map_interface_get_port_obj_id(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                     &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_ENCL:
           fbe_status = fbe_api_object_map_interface_get_enclosure_obj_id(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                          fbe_cli_rdevice_in_data_ptr->encl_num,
                                                                          &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_LDRV:
           fbe_status = fbe_api_object_map_interface_get_logical_drive_obj_id(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                              fbe_cli_rdevice_in_data_ptr->encl_num,
                                                                              fbe_cli_rdevice_in_data_ptr->disk_num,
                                                                              &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_PDRV:
           fbe_status = fbe_api_object_map_interface_get_physical_drive_obj_id(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                               fbe_cli_rdevice_in_data_ptr->encl_num,
                                                                               fbe_cli_rdevice_in_data_ptr->disk_num,
                                                                               &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_OBJL:
           fbe_status = fbe_api_object_map_interface_get_object_lifecycle_state(fbe_cli_rdevice_in_data_ptr->object_id,
                                                                                &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_BORD:
           fbe_status = fbe_api_object_map_interface_get_board_obj_id(&(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_TYPE: 
           fbe_status = fbe_api_object_map_interface_get_object_type(fbe_cli_rdevice_in_data_ptr->object_id,
                                                                     &fbe_cli_rdevice_out_data_ptr->requested_value.val64);
           break;
       case FBE_CLI_MAP_INTERFACE_GET_NUML:
           fbe_status = fbe_api_object_map_interface_get_total_logical_drives(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                              &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));
           break;
       case FBE_CLI_MAP_INTERFACE_GET_EADD:
           fbe_status = fbe_api_object_map_interface_get_encl_addr(fbe_cli_rdevice_in_data_ptr->port_num,
                                                                   fbe_cli_rdevice_in_data_ptr->encl_num,
                                                                   &(fbe_cli_rdevice_out_data_ptr->requested_value.val32_1));

           break;
       case FBE_CLI_MAP_INTERFACE_GET_EPOS:
           fbe_status = fbe_api_object_map_interface_get_encl_physical_pos(&fbe_cli_rdevice_out_data_ptr->requested_value.val32_1,
                                                                   &fbe_cli_rdevice_out_data_ptr->requested_value.val32_2,
                                                                   fbe_cli_rdevice_in_data_ptr->encl_addr);
           break;
       default:
           csx_p_display_std_note ("%s,invalid fbe_api_object_map interface code 0x%x\n", __FUNCTION__,
                                   fbe_cli_rdevice_in_data_ptr->fbe_cli_map_interface_code);
           break;
    }
    fbe_cli_rdevice_out_data_ptr->fbe_status_rc = fbe_status;

   if (fbe_status == FBE_STATUS_OK)
   {
       return CSX_STATUS_SUCCESS;
   }
   csx_p_display_std_note ("%s, calling fbe_api_object_map_interface_get_* failed...\n", __FUNCTION__);
   return CSX_STATUS_GENERAL_FAILURE;
}

#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */

EMCPAL_STATUS physical_package_ioctl( PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP PIrp)
{
    EMCPAL_STATUS status;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;
    ULONG IoControlCode = 0;	

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "physical pkg DRIVER, %s, entry.\n", __FUNCTION__);

    pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_PHYSICAL_PACKAGE_DEVICE_TYPE) {
        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "physical pkg DRIVER,  invalid device, type: %X\n", EmcpalExtDeviceTypeGet(PDeviceObject));
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_BAD_DEVICE_TYPE;
    }
 
    switch ( IoControlCode )
    {
    case FBE_PHYSICAL_PACKAGE_MGMT_PACKET_IOCTL:
        status = physical_package_mgmt_packet_ioctl(PIrp);
        break;
    case FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY_IOCTL:
        status = physical_package_get_control_entry_ioctl(PIrp);
        break;
    case FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY_IOCTL:
        status = physical_package_get_io_entry_ioctl(PIrp);
        break;
    case FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_driver_notification_register_application (PIrp);
        break;
    case FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL:
        status = fbe_driver_notification_get_next_event (PIrp);
        break;
    case FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_driver_notification_unregister_application (PIrp);
        break;
#ifdef C4_INTEGRATED            
            //  code to route rdevice ioctls sent from fbecli to fbe_api.  
     case FBE_CLI_SEND_PKT_IOCTL:
     case FBE_CLI_SEND_PKT_SGL_IOCTL:
     case FBE_CLI_GET_MAP_IOCTL:
        status = (IoControlCode == FBE_CLI_GET_MAP_IOCTL) ? fbe_api_common_map_interface_get_from_fbe_cli (EmcpalExtIrpStackParamIoctlInputBufferGet(pIoStackLocation), 
                                                                                                           EmcpalExtIrpStackParamIoctlOutputBufferGet(pIoStackLocation)) :
                                                            fbe_api_common_packet_from_fbe_cli (IoControlCode, EmcpalExtIrpStackParamIoctlInputBufferGet(pIoStackLocation),
                                                                                                               EmcpalExtIrpStackParamIoctlOutputBufferGet(pIoStackLocation));

        EmcpalExtIrpStatusFieldSet(PIrp, status);
        EmcpalExtIrpInformationFieldSet(PIrp, EmcpalExtIrpStackParamIoctlOutputSizeGet(pIoStackLocation));
        EmcpalIrpCompleteRequest(PIrp);
        break;
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */

    default: 
        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "physical pkg DRIVER, invalid IoControlCode: %X\n", IoControlCode);
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        status = EmcpalExtIrpStatusFieldGet(PIrp);
        EmcpalIrpCompleteRequest(PIrp);
        break;
    }

    return status;
}

static EMCPAL_STATUS 
physical_package_mgmt_packet_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  				irpStack;
    fbe_u8_t * 								ioBuffer = NULL;
    ULONG 									outLength = 0;
    fbe_serialized_control_transaction_t *	user_packet = NULL;
    fbe_packet_t * 							packet = NULL;
    irp_context_t *							irp_context = NULL;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpu_id_t                            cpu_id;
    
    /*allocate conext for completing the call later, we check later if it's null*/
    irp_context = (irp_context_t *)fbe_allocate_nonpaged_pool_with_tag(sizeof (irp_context_t),'pyhp');
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
    user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/

    /*alocate a new packet we will serialize into*/
    packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet);

    /* Get CPU ID and set it in the packet */
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);

    status = fbe_serialize_lib_unpack_transaction(user_packet, packet, &irp_context->sg_list);
    if (status !=FBE_STATUS_OK) {

        fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "physical pkg DRIVER, Unable to de-serialize user space packet..\n");

        ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
        user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
        user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        fbe_release_nonpaged_pool_with_tag(irp_context, 'pyhp');
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
   
    /*keep the context for later*/
    irp_context->outLength = outLength;
    irp_context->PIrp = PIrp;
    irp_context->user_packet = user_packet;

    fbe_transport_set_completion_function (packet, physical_package_mgmt_packet_ioctl_completion, irp_context);

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
physical_package_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    irp_context_t *							irp_context = (irp_context_t *)context;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

    /*let copy all the statuses fronm the packet to the transaction we are about to return to user pace*/
    status = fbe_serialize_lib_repack_transaction(irp_context->user_packet, packet);
    /*set the flags in the IRP*/
    if (status == FBE_STATUS_OK) {
        EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_SUCCESS);
    } else {
        EmcpalExtIrpStatusFieldSet(irp_context->PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
    }

    EmcpalExtIrpInformationFieldSet(irp_context->PIrp, irp_context->outLength);

    EmcpalIrpCompleteRequest(irp_context->PIrp);

    /*don't forget to release the context and the memory of the packet, we don't need it anymore*/
    fbe_transport_release_packet (packet);

    
    fbe_release_nonpaged_pool_with_tag(irp_context, 'pyhp');

    return EMCPAL_STATUS_SUCCESS;

}

static EMCPAL_STATUS 
physical_package_get_control_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_package_get_control_entry_t   * physical_package_get_control_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    physical_package_get_control_entry = (fbe_package_get_control_entry_t *)ioBuffer;
    physical_package_get_control_entry->control_entry = fbe_service_manager_send_control_packet;

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "physical pkg DRIVER, %s, control_entry: %p.\n",
                            __FUNCTION__, physical_package_get_control_entry->control_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

static EMCPAL_STATUS 
physical_package_get_io_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_package_get_io_entry_t   * physical_package_get_io_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    physical_package_get_io_entry = (fbe_package_get_io_entry_t *)ioBuffer;
    physical_package_get_io_entry->io_entry = fbe_topology_send_io_packet;

    fbe_base_package_trace(	FBE_PACKAGE_ID_PHYSICAL,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "physical pkg DRIVER, %s, io_entry: %p.\n",
                            __FUNCTION__, physical_package_get_io_entry->io_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

EMCPAL_STATUS get_package_device_object(PEMCPAL_DEVICE_OBJECT *device_object)
{
    if(device_extention != NULL) 
    {
        *device_object = device_extention->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}
    
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}

