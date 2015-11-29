/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file esp_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the ESP Driver Entry and related functions.
 * 
 *  
 * @ingroup.
 * 
 * @version
 *   08-March-2010: Created.
 *
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include <ntddk.h>
#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "klogservice.h"
#include "fbe/fbe_driver_notification_interface.h"
#include "fbe/fbe_package.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_api_common.h" 
#include "fbe/fbe_registry.h"
#include "EmcPAL_DriverShell.h"
#include "generics.h"	/*we hate it but need it to receive the IOCTLs here from host side*/
#include "adm_api.h"
#include "flare_adm_ioctls.h"
#include "flare_export_ioctls.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "spid_enum_types.h"
#include "global_cab_ids.h"
#include "cc.h"
#include "sunburst_errors.h"
#include "specl_types.h"
#include "specl_interface.h"
#include "fbe_api_database_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "EmcUTIL.h"

typedef struct esp_device_extension_s{
    PEMCPAL_DEVICE_OBJECT device_object;
}esp_device_extension_t;

typedef struct irp_context_s {
    PEMCPAL_IRP                                    PIrp;
    fbe_serialized_control_transaction_t *  user_packet;
    ULONG                                   outLength;
    void *                                  sg_list;
}irp_context_t;

typedef struct io_irp_context_s {
    PEMCPAL_IRP                        PIrp;
    ULONG                       outLength;
}io_irp_context_t;

static fbe_bool_t	cache_registered = FBE_FALSE;
typedef struct
{
    PEMCPAL_IRP		notification_irp;
    fbe_spinlock_t	cache_notification_lock;
    fbe_bool_t      b_sps_state_change_pending;
} cache_sps_data;
cache_sps_data cache_sps_register;

/*******************************
 *   Local Function Prototypes
 ******************************/
EMCPAL_STATUS esp_open(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp);
EMCPAL_STATUS esp_close(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp);
EMCPAL_STATUS esp_ioctl( PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP PIrp);
EMCPAL_STATUS esp_unload(IN PEMCPAL_DRIVER pPalDriverObject);
static EMCPAL_STATUS esp_mgmt_packet_ioctl(PEMCPAL_IRP PIrp);
static EMCPAL_STATUS esp_get_control_entry_ioctl(PEMCPAL_IRP PIrp);
static fbe_status_t esp_mgmt_packet_ioctl_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static EMCPAL_STATUS esp_get_io_entry_ioctl(PEMCPAL_IRP PIrp);
fbe_status_t getEspBPT(fbe_u32_t *IsBPT_p);
fbe_status_t getEspTraceLevel(fbe_u32_t *TraceLevel_p);

static esp_device_extension_t * device_extension = NULL;
static fbe_notification_element_t cache_notification_element;

static EMCPAL_STATUS esp_set_sep_ctrl_entry(PEMCPAL_IRP PIrp);

/*get env status*/
static EMCPAL_STATUS cache_get_env_status(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr);
static EMCPAL_STATUS cache_register_for_sp_state_change(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr);
static fbe_status_t cache_register_notification_element(void);
static fbe_status_t cache_unregister_notification_element(void);
// Cancel the IOCTL sent by Cache driver
static void esp_cache_io_cancel(IN PEMCPAL_DEVICE_OBJECT DeviceObject,IN PEMCPAL_IRP irp_p);
// IRP completion callback routine for cache driver IOCTL.
static fbe_status_t esp_cache_complete_irp(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context);
static EMCPAL_STATUS esp_ok_to_power_down(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr);
static EMCPAL_STATUS esp_get_system_info(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr);
static EMCPAL_STATUS esp_quiesce_bg_io(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr);
static EMCPAL_STATUS hostside_get_general_io_port_status(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr);

/*!***************************************************************
 * EmcpalDriverEntry()
 ****************************************************************
 * @brief
 *  Driver Entry function ESP Driver
 *
 * @param pPalDriverObject - a pointer to the EMCPal driver object
 * 
 * @return NTSTATUS - Status of ESP driver loading
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS EmcpalDriverEntry(IN PEMCPAL_DRIVER pPalDriverObject)
{
    EMCPAL_STATUS status;
    fbe_status_t  fbe_status;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    fbe_u32_t IsBPT = 0;
    fbe_u32_t EspTraceLevel = FBE_TRACE_LEVEL_INFO;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;
    EMCPAL_PUNICODE_STRING PRegistryPath = {0};

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "ESP DriverEntry: starting.\n");
    
    // update the boot status info
    speclSetLocalSoftwareBootStatus(SW_GENERAL_STS_NONE,
                                    SW_COMPONENT_ESP,           // sw component
                                    SW_DRIVER_ENTRY);           // sw code 
                                    
    getEspBPT(&IsBPT);
    if(IsBPT)
    {
         fbe_debug_break();
    }
    
    PDriverObject = EmcpalDriverGetNativeDriverObject(pPalDriverObject);
    PRegistryPath = EmcpalDriverGetNativeRegistryPath(pPalDriverObject);

    /* Read the Registy value for ESP Trace level */
    fbe_status = getEspTraceLevel(&EspTraceLevel);
    if(fbe_status == FBE_STATUS_OK)
    {
        if(EspTraceLevel  >FBE_TRACE_LEVEL_INVALID &&
           EspTraceLevel < FBE_TRACE_LEVEL_LAST)
        {
            fbe_trace_set_default_trace_level(EspTraceLevel);
        }
    }

    status = EmcpalExtDeviceCreate(PDriverObject,
                            sizeof(esp_device_extension_t),
                            FBE_ESP_DEVICE_NAME,
                            FBE_ESP_DEVICE_TYPE,
                            0,
                            FALSE,
                            &device_object );

    if ( !EMCPAL_IS_SUCCESS(status) ){
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "ESP DriverEntry: IoCreateDevice failed, status: %X\n", status);
        return status;
    }

    /* Init device_extension */
    device_extension  = (esp_device_extension_t *) EmcpalDeviceGetExtension(device_object);
    device_extension->device_object = device_object;

    /* Create dispatch points for open, close, ioctl and unload. */
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, esp_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, esp_close);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, esp_ioctl);
    EmcpalDriverSetUnloadCallback(pPalDriverObject, (EMCPAL_DRIVER_UNLOAD_CALLBACK)esp_unload);

    /* Create a link from our device name to a name in the Win32 namespace. */
    status = EmcpalExtDeviceAliasCreateA(FBE_ESP_DEVICE_LINK, FBE_ESP_DEVICE_NAME);

    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "ESP DriverEntry: IoCreateSymbolicLink failed, status: %X\n", status);
                               EmcpalExtDeviceDestroy(device_object);
        return status;
    }

    EmcutilEventLog(
                   ESP_INFO_LOAD_VERSION,
                   0, NULL, "%s %s",
                   __DATE__, __TIME__);

    /* Initializing the fbe_api and serives.
     */
    fbe_status = esp_init();
    if(fbe_status != FBE_STATUS_OK) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "ESP DriverEntry: failed to initialized the ESP, status: %X\n", status);  

        EmcpalExtDeviceDestroy(device_object);
        EmcpalExtDeviceAliasDestroyA(FBE_ESP_DEVICE_LINK);
        return EMCPAL_STATUS_FAILED_DRIVER_ENTRY;
    }

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "ESP DriverEntry: Init Complete starting Driver Notification Init\n");


    /* register to get events from the notification service,
     * these events will be sent later via IOCTL to registered user applications
     */
    fbe_driver_notification_init(FBE_PACKAGE_ID_ESP);

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "ESP DriverEntry: Driver Notification Init Completed\n");

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                       FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "ESP DriverEntry: End.\n");

    return EMCPAL_STATUS_SUCCESS;
}
/***********************
    end DriverEntry()   
************************/

/*!***************************************************************
 * esp_open()
 ****************************************************************
 * @brief
 *  This is function for IRP_MJ_CREATE of device.
 *
 * @param PDeviceObject - Pointer to the device object.
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_open.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS esp_open(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "ESP driver, %s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}
/*******************
    end esp_open()  
********************/

/*!***************************************************************
 * esp_close()
 ****************************************************************
 * @brief
 *  This is function for IRP_MJ_CLOSE of device.
 *
 * @param PDeviceObject - Pointer to the device object.
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_open.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS esp_close(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "ESP DRIVER, %s, entry.\n", __FUNCTION__);

    /* Just complete the IRP */
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}
/*********************
    end esp_close()     
**********************/

/*!***************************************************************
 * esp_unload()
 ****************************************************************
 * @brief
 *  This is function for driver unload.
 *
 * @param PDriverObject - Pointer to the driver object.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS esp_unload(IN PEMCPAL_DRIVER pPalDriverObject)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "ESP DRIVER, unloading...\n");

    PDriverObject = (PEMCPAL_NATIVE_DRIVER_OBJECT)EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    fbe_driver_notification_destroy();
    cache_unregister_notification_element();
    esp_destroy();

    /* Delete the link from our device name to a name in the Win32 namespace. */
    status =  EmcpalExtDeviceAliasDestroyA( FBE_ESP_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "ESP DRIVER, IoDeleteSymbolicLink failed, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);
    
    /* Finally delete our device object */
    EmcpalExtDeviceDestroy(device_object);

    device_extension = NULL;

    return EMCPAL_STATUS_SUCCESS;
}
/**********************
    end esp_unload      
***********************/

/*!***************************************************************
 * esp_ioctl()
 ****************************************************************
 * @brief
 *  This is function for IRP_MJ_DEVICE_CONTROL of device.
 *
 * @param PDeviceObject - Pointer to the device object.
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_ioctl.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
EMCPAL_STATUS esp_ioctl( PEMCPAL_DEVICE_OBJECT  PDeviceObject, PEMCPAL_IRP PIrp)
{
    EMCPAL_STATUS status;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;
    ULONG IoControlCode = 0;    

    fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "ESP DRIVER, %s, entry.\n", __FUNCTION__);

    pIoStackLocation   = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_ESP_DEVICE_TYPE) {
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "ESP DRIVER,  invalid device, type: %X\n", EmcpalExtDeviceTypeGet(PDeviceObject));
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_BAD_DEVICE_TYPE;
    }
 
    switch (IoControlCode)
    {
    case FBE_ESP_MGMT_PACKET_IOCTL:
        status = esp_mgmt_packet_ioctl(PIrp);
        break;
    case FBE_ESP_GET_MGMT_ENTRY_IOCTL:
        status = esp_get_control_entry_ioctl(PIrp);
        break;
    case FBE_ESP_GET_IO_ENTRY_IOCTL:
        status = esp_get_io_entry_ioctl(PIrp);
        break;
    case FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_driver_notification_register_application (PIrp);
        break;
    case FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL:
        status = fbe_driver_notification_get_next_event (PIrp);
        break;
    case FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_driver_notification_unregister_application (PIrp);
        break;
    case IOCTL_FLARE_GET_SP_ENVIRONMENT_STATE:
        status = cache_get_env_status(PIrp, pIoStackLocation);
        break;
    case IOCTL_FLARE_REGISTER_FOR_SP_CHANGE_NOTIFICATION:
        status = cache_register_for_sp_state_change(PIrp, pIoStackLocation);
        break;
    case IOCTL_FLARE_OK_TO_POWER_DOWN:
        status = esp_ok_to_power_down(PIrp, pIoStackLocation);
        break;
    case IOCTL_FLARE_ADM_GET_SYSTEM_INFO:
        status = esp_get_system_info(PIrp, pIoStackLocation);
        break;
    case IOCTL_FLARE_QUIESCE_BACKGROUND_IO:
        status = esp_quiesce_bg_io(PIrp, pIoStackLocation);
        break;
    case FBE_ESP_SET_SEP_CTRL_ENTRY_IOCTL:
        status = esp_set_sep_ctrl_entry(PIrp);
                break;
    case IOCTL_ADM_GET_GENERAL_IO_PORT_STATUS:
        status = hostside_get_general_io_port_status(PIrp, pIoStackLocation);
        break;
    default: 
        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
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
/*******************
    end esp_ioctl   
********************/

/*!***************************************************************
 * esp_mgmt_packet_ioctl()
 ****************************************************************
 * @brief
 *  This is function for Management IOCTL.
 *
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_mgmt_packet_ioctl.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
 static EMCPAL_STATUS 
esp_mgmt_packet_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION               irpStack;
    fbe_u8_t *                              ioBuffer = NULL;
    ULONG                                   outLength = 0;
    fbe_serialized_control_transaction_t *  user_packet = NULL;
    fbe_packet_t *                          packet = NULL;
    irp_context_t *                         irp_context = NULL;
    fbe_cpu_id_t                            cpu_id;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    
    /*allocate conext for completing the call later, we check later if it's null*/
    irp_context = (irp_context_t *)fbe_allocate_nonpaged_pool_with_tag(sizeof (irp_context_t),'kpse');
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

        fbe_base_package_trace(FBE_PACKAGE_ID_ESP,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "ESP DRIVER, Unable to de-serialize user space packet..\n");

        ioBuffer = EmcpalExtIrpSystemBufferGet(PIrp);
        user_packet = (fbe_serialized_control_transaction_t *)ioBuffer;/*get he transaction from the user*/
        user_packet->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
        fbe_release_nonpaged_pool_with_tag (irp_context, 'kpse');
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
   
    /*keep the context for later*/
    irp_context->outLength = outLength;
    irp_context->PIrp = PIrp;
    irp_context->user_packet = user_packet;
    
    fbe_transport_set_completion_function (packet, esp_mgmt_packet_ioctl_completion, irp_context);

    /*the order is important here, no not change it
    1. Mark pending
    2. forward the ioctl
    3. return pending
    */

    EmcpalIrpMarkPending (PIrp);
    fbe_service_manager_send_control_packet(packet);
    return EMCPAL_STATUS_PENDING;
}
/*********************************
    end esp_mgmt_packet_ioctl()
**********************************/

/*!***************************************************************
 * esp_mgmt_packet_ioctl_completion()
 ****************************************************************
 * @brief
 *  This is function for Management IOCTL completion fu.
 *
 * @param PIrp - Pointer to the I/O request packet
 * @context - 
 * 
 * @return NTSTATUS - Status for esp_mgmt_packet_ioctl_completion.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
static fbe_status_t 
esp_mgmt_packet_ioctl_completion(fbe_packet_t * packet,
                                 fbe_packet_completion_context_t context)
{
    irp_context_t *                         irp_context = (irp_context_t *)context;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;

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
    
    fbe_release_nonpaged_pool_with_tag(irp_context, 'kpse');

    return EMCPAL_STATUS_SUCCESS;
}
/*********************************************
    end esp_mgmt_packet_ioctl_completion()  
**********************************************/

/*!***************************************************************
 * esp_get_control_entry_ioctl()
 ****************************************************************
 * @brief
 *  This is function for control entry IOCTL.
 *
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_get_control_entry_ioctl.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
static EMCPAL_STATUS 
esp_get_control_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_package_get_control_entry_t   * esp_get_control_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    esp_get_control_entry = (fbe_package_get_control_entry_t *)ioBuffer;
    esp_get_control_entry->control_entry = fbe_service_manager_send_control_packet;

    fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "ESP driver, %s, control_entry: %p.\n",
                            __FUNCTION__, esp_get_control_entry->control_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}
/**************************************
    end esp_get_control_entry_ioctl()
***************************************/

/*!***************************************************************
 * esp_get_io_entry_ioctl()
 ****************************************************************
 * @brief
 *  This is function for IO entry IOCTL.
 *
 * @param PIrp - Pointer to the I/O request packet
 * 
 * @return NTSTATUS - Status for esp_get_io_entry_ioctl.
 *
 * @author
 *  08-March-2010: Created.
 *
 ****************************************************************/
static EMCPAL_STATUS 
esp_get_io_entry_ioctl(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_package_get_io_entry_t   * esp_get_io_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    esp_get_io_entry = (fbe_package_get_io_entry_t *)ioBuffer;
    esp_get_io_entry->io_entry = fbe_topology_send_io_packet;

    fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "ESP driver, %s, io_entry: %p.\n",
                            __FUNCTION__, esp_get_io_entry->io_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}
/**************************************
    end esp_get_io_entry_ioctl()
***************************************/

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
    if(device_extension != NULL) 
    {
        *device_object = device_extension->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}
/**************************************
    end get_package_device_object()
***************************************/

/*!***************************************************************
 *      fbe_api_esp_common_init()
 ****************************************************************
 * @brief
 *  This function initialized the fbe_api for kernel.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18-March-2010: Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_esp_common_init(void)
{
    return(fbe_api_common_init_kernel());
}
/***********************************
    end fbe_api_esp_common_init()    
************************************/

/*!***************************************************************
 *      fbe_api_esp_common_destroy()
 ****************************************************************
 * @brief
 *  This function destroys the fbe_api for kernel.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * @author
 *  06-January-2012: Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_esp_common_destroy(void)
{
    return(fbe_api_common_destroy_kernel());
}
/***********************************
    end fbe_api_esp_common_init()    
************************************/
/*!***************************************************************
 *      fbe_get_registry_path()
 ****************************************************************
 * @brief
 *  This function return the registry path for ESP.
 *
 * @param 
 * 
 * @return fbe_u8_t *  Path for ESP registry
 *
 * @author
 *  22 -April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_u8_t* fbe_get_registry_path()
{
    return EspRegPath;
}

/***********************************
    end fbe_get_registry_path()    
************************************/

/*!***************************************************************
 *      fbe_get_fup_registry_path()
 ****************************************************************
 * @brief
 *  This function returns the registry path for firmware upgrade.
 *
 * @param 
 * 
 * @return fbe_u8_t *  Path for K10DriverConfig registry
 *
 * @author
 *  19-Aug-2010: PHE - Created.
 *
 ****************************************************************/
fbe_u8_t* fbe_get_fup_registry_path()
{
    return K10DriverConfigRegPath;
}

/**********************************
    end fbe_get_fup_registry_path()
***********************************/

/*!***************************************************************
 *      getEspBPT()
 ****************************************************************
 * @brief
 *  This function is to check ESP registry for breakon entry for ESP driver
 *
 * @param IsBPT_p : Buffer to store registry value.
 * 
 * @return fbe_status_t  
 *
 * @author
 *  22 -April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t
getEspBPT(fbe_u32_t *IsBPT_p)
{
    fbe_u32_t EspBpt;   
    fbe_u8_t defaultBPT =0;
    fbe_status_t status;

    status = fbe_registry_read(NULL,
                      fbe_get_registry_path(),
                      EspBPTKey, 
                      &EspBpt,
                      sizeof(EspBpt),
                      FBE_REGISTRY_VALUE_DWORD,
                      &defaultBPT,
                      0,
                      FALSE);
    if(FBE_STATUS_OK == status)
    {
        *IsBPT_p = EspBpt;
    }
    return status;
}
/***********************************
    end getEspBPT()    
************************************/

/*!***************************************************************
 * @fn      getEspTraceLevel()
 ****************************************************************
 * @brief
 *  This function is to check ESP registry for Trace Level of ESP driver
 *
 * @param TraceLevel_p : Buffer to store registry value.
 * 
 * @return fbe_status_t  
 *
 * @author
 *  1-July-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t
getEspTraceLevel(fbe_u32_t *TraceLevel_p)
{
    fbe_u32_t EspTraceLevel;   
    fbe_u8_t defaultTraceLevel;
    fbe_status_t status;

    defaultTraceLevel = fbe_trace_get_default_trace_level();

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               EspTraceLevelKey, 
                               &EspTraceLevel,
                               sizeof(EspTraceLevel),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultTraceLevel,
                               0,
                               FALSE);
    if(status == FBE_STATUS_OK)
    {
        *TraceLevel_p = EspTraceLevel;
    }
    
    return status;
}
/***********************************
    end getEspTraceLevel()    
************************************/

/*!***************************************************************
 * @fn cache_get_env_status(PIRP irpPtr, PIO_STACK_LOCATION ioStackPtr)
 ****************************************************************
 * @brief
 *  This function gets the environment status for cache.
 *
 * @param irpPtr        - pointer to the IRP data
 * @param ioStackPtr    - pointer to the I/O Stack data
 *
 * @return
 *  NTSTATUS
 *
 * @version
 *  03/29/2012  Vera Wang Created. 
 *
 ****************************************************************/
static EMCPAL_STATUS cache_get_env_status(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{
    SP_ENVIRONMENT_STATE* envInfo = (SP_ENVIRONMENT_STATE *)EmcpalExtIrpStackParamType3InputBufferGet(ioStackPtr);
    EMCPAL_STATUS ioctl_status = EMCPAL_STATUS_SUCCESS;
    fbe_common_cache_status_info_t cacheStatusInfo = {0};
    fbe_status_t              status;


    if(envInfo == NULL){
        envInfo = (SP_ENVIRONMENT_STATE *)EmcpalExtIrpSystemBufferGet(irpPtr);
    }

    status = fbe_api_esp_common_getCacheStatus(&cacheStatusInfo);
    
    switch (cacheStatusInfo.cacheStatus) 
    {
        case FBE_CACHE_STATUS_FAILED_SHUTDOWN:
            envInfo->HardwareStatus = CACHE_HDW_SHUTDOWN_IMMINENT;
            break;
        case FBE_CACHE_STATUS_APPROACHING_OVER_TEMP:
            envInfo->HardwareStatus = CACHE_HDW_APPROACHING_OVER_TEMP;
            break;
        case FBE_CACHE_STATUS_FAILED:
            envInfo->HardwareStatus = CACHE_HDW_FAULT;
            break;
        default:
            envInfo->HardwareStatus = CACHE_HDW_OK;
            break;
    }

    if (status == FBE_STATUS_BUSY) 
    {
        ioctl_status = EMCPAL_STATUS_DEVICE_BUSY;
    }
    else
    {
        ioctl_status = EMCPAL_STATUS_SUCCESS;
    }

    envInfo->SpsPercentCharged = (ULONG) cacheStatusInfo.batteryTime;
    envInfo->SsdFault = (BOOL) cacheStatusInfo.ssdFaulted;

    /* Complete the IRP */
    EmcpalExtIrpStatusFieldSet(irpPtr, ioctl_status);
    EmcpalExtIrpInformationFieldSet(irpPtr, 0);
    EmcpalIrpCompleteRequest(irpPtr);
    return ioctl_status;
}    /* end of cache_get_env_status() */

/*!***************************************************************
 * @fn cache_register_for_sp_state_change(PIRP irpPtr, PIO_STACK_LOCATION ioStackPtr)
 ****************************************************************
 * @brief
 *  This function registers for the SP State changes.
 *
 * @param irpPtr        - pointer to the IRP data
 * @param ioStackPtr    - pointer to the I/O Stack data
 *
 * @return
 *  NTSTATUS
 *
 * @version 
 *   03/29/2012   Vera Wang Created.
 *
 ****************************************************************/
static EMCPAL_STATUS cache_register_for_sp_state_change(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{

    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    fbe_status_t  fbe_status;

    /*first time must return*/
    if (FBE_IS_FALSE(cache_registered)) 
    {
        cache_registered = FBE_TRUE;
        fbe_spinlock_init(&cache_sps_register.cache_notification_lock);
        /* Register for state changes from ESP to be propogated to cache 
         * We register for object data changes for all objects in ESP package.
         */
        fbe_status = cache_register_notification_element();
        if (fbe_status != FBE_STATUS_OK) 
        {
            EmcpalExtIrpStatusFieldSet(irpPtr, EMCPAL_STATUS_UNSUCCESSFUL);
            EmcpalExtIrpInformationFieldSet(irpPtr, 0);
            EmcpalIrpCompleteRequest(irpPtr);
            return EMCPAL_STATUS_UNSUCCESSFUL;
        }

        fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "ESP driver, %s: first time. \n",
                                __FUNCTION__);
        status = EMCPAL_STATUS_SUCCESS;
        /* Complete the IRP with STATUS_SUCCESS */
        EmcpalExtIrpStatusFieldSet(irpPtr, EMCPAL_STATUS_SUCCESS);
        EmcpalExtIrpInformationFieldSet(irpPtr, 0);
        EmcpalIrpCompleteRequest(irpPtr);
    }
    else
    {
        fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "ESP driver, %s: not first time. \n",
                                __FUNCTION__);   
        /*keep it here for when we have state changes on the SP level that require cache notifications*/
        fbe_spinlock_lock(&cache_sps_register.cache_notification_lock);
        if (cache_sps_register.b_sps_state_change_pending == FBE_TRUE)
        {
            cache_sps_register.b_sps_state_change_pending = FBE_FALSE;
            status = EMCPAL_STATUS_SUCCESS;
        }
        else
        {
            EmcpalIrpCancelRoutineSet(irpPtr, esp_cache_io_cancel);
            if (EmcpalIrpIsCancelInProgress(irpPtr) && EmcpalIrpCancelRoutineSet(irpPtr, NULL))
            {
                /* This means that IRP is cancelled by IO manager but CANCEL routine
                 * is not called yet. Return cancel status
                 */
                status = EMCPAL_STATUS_CANCELLED;
            }
            else
            {
                EmcpalIrpMarkPending(irpPtr);
                /* store the irpPtr otherwise cancel routine will never find it */
                cache_sps_register.notification_irp = irpPtr;
                /* Send this status so that calling function will know
                 * that we have stored the IRP pointer and it should 
                 * not attempt to access it.
                 */
                status = EMCPAL_STATUS_PENDING;
            }
        }
        fbe_spinlock_unlock(&cache_sps_register.cache_notification_lock);
    }
    return status;
}

/*********************************************************************
 *            cache_register_notification_element()
 *********************************************************************
 *
 *  Description: cache register for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  Version:
 *    3/29/12: Vera Wang created
 *    
 *********************************************************************/
static fbe_status_t cache_register_notification_element(void)
{
    fbe_packet_t * 				    packet = NULL;	
    fbe_status_t				    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *			    payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL; 

    cache_notification_element.notification_function = esp_cache_complete_irp;
    cache_notification_element.notification_context = NULL;
    cache_notification_element.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    cache_notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
    
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    
    if(packet == NULL) {
        KvTrace("%s: failed to allocate memory for packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
                                         &cache_notification_element,
                                         sizeof(fbe_notification_element_t));


    /* Set packet address */
    fbe_transport_set_address(	packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);
    
    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        KvTrace ( "%s: unable to register for events\n", __FUNCTION__);	
        fbe_transport_release_packet(packet);
        return status;
    }
    
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn esp_cache_complete_irp
 * (fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
 ****************************************************************
 * @brief
 *  This function completes the ioctl for SP registration.
 *
 * @param fbe_object_id_t object_id 
 * @param fbe_notification_info_t notification_info
 * @param fbe_notification_context_t context
 *
 * @return fbe_status_t   
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t esp_cache_complete_irp(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
{

    PEMCPAL_IRP         irp_to_complete;
    
    if (notification_info.notification_data.data_change_info.device_mask != FBE_DEVICE_TYPE_SP_ENVIRON_STATE)
    {
        /* We don't care about other state changes. */
       return FBE_STATUS_OK;
    }
    
    /* Grab the lock protecting shared resource which stores the 
     * IRP received from cache 
     */
    fbe_spinlock_lock(&cache_sps_register.cache_notification_lock);

    irp_to_complete = cache_sps_register.notification_irp;
    if (irp_to_complete != NULL)
    {
        /* We're going to complete this IRP */
        cache_sps_register.notification_irp = NULL;
    }
    else
    {
        /* an indication to the code that state change has already occured */
        cache_sps_register.b_sps_state_change_pending = FBE_TRUE;
        fbe_spinlock_unlock(&cache_sps_register.cache_notification_lock);
        return FBE_STATUS_OK;
    }

    /* Release the lock protecting the shared resource which 
     * stores the IRP received from cache 
     */
    fbe_spinlock_unlock(&cache_sps_register.cache_notification_lock);

    /* Complete the IRP with STATUS_SUCCESS */
#ifndef ALAMOSA_WINDOWS_ENV
    CSX_ASSERT_H_RDC(CSX_NOT_NULL(EmcpalIrpCancelRoutineSet(irp_to_complete, NULL)));
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - CGCG - bug - need to clear the cancel routine before completing - this isn't a proper solution but it'll work for now */
    EmcpalExtIrpStatusFieldSet(irp_to_complete, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(irp_to_complete, 0);
    EmcpalIrpCompleteRequest(irp_to_complete);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn esp_cache_io_cancel(PDEVICE_OBJECT DeviceObject, PIRP irp_p)
 ****************************************************************
 * @brief
 *  This function cancels the ioctl for SP registration.
 *
 * @param DeviceObject  - pointer to ESP device object
 * @param irp_p         - pointer to IRP to be cancelled
 *
 * @return
 *  void
 *
 * @version
 *
 ****************************************************************/
static void esp_cache_io_cancel(IN PEMCPAL_DEVICE_OBJECT DeviceObject,
                                         IN PEMCPAL_IRP irp_p)
{
    /* Release the cancel spinlock held by IO manager 
     * while calling cancel routine 
     */
    EmcpalIrpCancelLockRelease(EmcpalExtIrpCancelIrqlGet(irp_p));

    /* Grab the lock protecting shared resource which stores the 
     * IRP received from cache 
     */
    fbe_spinlock_lock(&cache_sps_register.cache_notification_lock);

    /* reset the shared resource to point to NULL to indicate 
     * that no IRP from cache is pending. 
     */
    cache_sps_register.notification_irp = NULL;    

    /* Release the lock protecting the shared resource which 
     * stores the IRP received from cache 
     */
    fbe_spinlock_unlock(&cache_sps_register.cache_notification_lock);

    /* Complete the IRP with STATUS_CANCEL */
    EmcpalExtIrpStatusFieldSet(irp_p, EMCPAL_STATUS_CANCELLED);
    EmcpalIrpCancelRoutineSet(irp_p, NULL);
    EmcpalExtIrpInformationFieldSet(irp_p, 0);
    EmcpalIrpCompleteRequest(irp_p);
    
}

/*********************************************************************
 *            cache_register_notification_element()
 *********************************************************************
 *
 *  Description: cache unregister for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  Version:
 *    4/4/12: Vera Wang created
 *    
 *********************************************************************/
static fbe_status_t cache_unregister_notification_element(void)
{
    fbe_packet_t * 				    packet = NULL;	
    fbe_status_t				    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *			    payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL; 
   
    if (FBE_IS_FALSE(cache_registered)) {
        KvTrace("%s: notification is not registered\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }
 
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    
    if(packet == NULL) {
        KvTrace("%s: failed to allocate memory for packet\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
                                         &cache_notification_element,
                                         sizeof(fbe_notification_element_t));


    /* Set packet address */
    fbe_transport_set_address(	packet,
                                FBE_PACKAGE_ID_ESP,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);
    
    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        KvTrace ( "%s: unable to unregister for events\n", __FUNCTION__);	
        fbe_transport_release_packet(packet);
        return status;
    }
    
    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

static EMCPAL_STATUS esp_ok_to_power_down(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    fbe_status_t shutdown_status = FBE_STATUS_OK;

    /* Let ESP handle the power down. */
    shutdown_status = fbe_api_esp_common_powerdown();
    /* Trace the failure */
    if (shutdown_status != FBE_STATUS_OK)
    {
        status = EMCPAL_STATUS_UNSUCCESSFUL;
        KvTrace("%s: power down request returned failure\n", __FUNCTION__);
    }
    /* responsibility of AdminShim is just to send the 
     * request to power down SPS to ESP, we'll always return success back
     */

    EmcpalExtIrpStatusFieldSet(PIrp, status );
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

/*this is received from Host side and all they care about is the cabinet ID*/
static EMCPAL_STATUS esp_get_system_info(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{
    fbe_status_t						status;
    fbe_esp_board_mgmt_board_info_t 	platform_info;
    ADM_GET_SYSTEM_INFO_DATA *			data = (void*)EmcpalExtIrpSystemBufferGet(PIrp);
    fbe_u8_t *							cabinet_id = &data->cabinet_id;
    ADM_STATUS *						adm_status = &data->status;
    fbe_u8_t *							serial_num_p = &data->system_serial_number[0];
    SPECL_RESUME_DATA             		resume_data = {0};
    fbe_u32_t 							i;

    if (EmcpalExtIrpStackParamIoctlOutputSizeGet(ioStackPtr) < sizeof (ADM_GET_SYSTEM_INFO_DATA))
    {
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BUFFER_TOO_SMALL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return  EMCPAL_STATUS_BUFFER_TOO_SMALL;
    }
   

    for(i = 0; i < 20; i++){
        status =  fbe_api_esp_board_mgmt_getBoardInfo(&platform_info);
        if(status != FBE_STATUS_OK){
            KvTrace ("%s: admin_shim_get_system_info FAILED, retrying\n", __FUNCTION__);
            fbe_thread_delay(5000);
        } else {
            break;   
        }    
    }

    if (status != FBE_STATUS_OK){
         KvTrace ( "%s: unable to get board info\n", __FUNCTION__);	
         adm_status->condition_code = CC_NOT_SUPPORTED;
         adm_status->sunburst_error = HOST_ADMIN_LIB_FLARE_INTERNAL_ERROR;
         EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL );
         EmcpalExtIrpInformationFieldSet(PIrp, 0);
         EmcpalIrpCompleteRequest(PIrp);
         return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    switch(platform_info.platform_info.midplaneType)
    {
        case FOGBOW_MIDPLANE:
            *cabinet_id =  AEP_CAB_FOGBOW;
            break;
        case BROADSIDE_MIDPLANE:
             *cabinet_id = AEP_CAB_BROADSIDE;
             break;
        case HOLSTER_MIDPLANE:
             *cabinet_id = AEP_CAB_HOLSTER;
             break;
        case BUNKER_MIDPLANE:
             *cabinet_id = AEP_CAB_BUNKER;
             break;
        case CITADEL_MIDPLANE:
             *cabinet_id = AEP_CAB_CITADEL;
             break;
        case BOXWOOD_MIDPLANE:
            *cabinet_id = AEP_CAB_BOXWOOD;
            break;
        case KNOT_MIDPLANE:
            *cabinet_id = AEP_CAB_KNOT;
            break;
        default:
             *cabinet_id = AEP_CAB_DPE_ONBOARD_AGENT;
             break;
    }

    for(i = 0; i < 20; i++){
        speclGetResume(CHASSIS_SERIAL_EEPROM, &resume_data);
        if (resume_data.transactionStatus != 0x0){
            KvTrace ("%s: speclGetResume FAILED, retrying...", __FUNCTION__);    
            fbe_thread_delay(5000);
        } else {
            break;
        }
    }

    if (resume_data.transactionStatus == 0x0){
        fbe_copy_memory(serial_num_p, resume_data.resumePromData.product_serial_number, sizeof(resume_data.resumePromData.product_serial_number));
    }

    adm_status->condition_code = CC_NO_SENSE;
    adm_status->sunburst_error = HOST_REQUEST_COMPLETE;

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, sizeof (ADM_GET_SYSTEM_INFO_DATA));
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*ESP is not in the IO business, it just happens that SP cache sents it this IO since it sends it to anything
it also sends the FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL. We will route this to SEP*/
static EMCPAL_STATUS esp_quiesce_bg_io(PEMCPAL_IRP PIrp, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{
    PSP_QUIESCE_IO 	quiesce_struct = (PSP_QUIESCE_IO)EmcpalExtIrpSystemBufferGet(PIrp);
    fbe_status_t 	bg_io_status;
    fbe_bool_t      update_bgs_on_both_sp = FBE_TRUE;

    if (EmcpalExtIrpStackParamIoctlInputSizeGet(ioStackPtr) < sizeof(SP_QUIESCE_IO)){
        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BUFFER_TOO_SMALL);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return  EMCPAL_STATUS_BUFFER_TOO_SMALL;
    }

    if (quiesce_struct->flag)
    {
        bg_io_status = fbe_api_database_stop_all_background_service(update_bgs_on_both_sp);
    }
    else
    {
        bg_io_status = fbe_api_database_restart_all_background_service();
    }

    if (bg_io_status != FBE_STATUS_OK) {
        KvTrace ("%s: Failed to control MCR background IO...", __FUNCTION__);    
    }

    /*always return sucess*/
    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);
    EmcpalIrpCompleteRequest(PIrp);

    return EMCPAL_STATUS_SUCCESS;
}

/*!***************************************************************
 * esp_set_sep_ctrl_entry()
 ****************************************************************
 * @brief
 *  This is function is setting the SEP control entry so ESP can call it.
 *
 * @param PEMCPAL_IRP - Pointer to the I/O request packet
 * 
 * @return EMCPAL_STATUS - Status for esp_set_sep_ctrl_entry.
 *
 *
 ****************************************************************/
static EMCPAL_STATUS esp_set_sep_ctrl_entry(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  			irpStack;
    fbe_u8_t          * 				ioBuffer = NULL;
    ULONG               				inLength;
    ULONG               				outLength;
    fbe_package_set_control_entry_t   * esp_set_control_entry = NULL;

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    esp_set_control_entry = (fbe_package_set_control_entry_t *)ioBuffer;
    fbe_service_manager_set_sep_control_entry(esp_set_control_entry->control_entry);

    fbe_base_package_trace( FBE_PACKAGE_ID_ESP,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "ESP driver, %s, SEP control_entry: %p was set\n",
                            __FUNCTION__, esp_set_control_entry->control_entry);

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

/***********************************
    end esp_set_sep_ctrl_entry()    
************************************/




/*!***************************************************************
 * @fn hostside_get_general_io_port_status(PIRP irpPtr, PIO_STACK_LOCATION ioStackPtr)
 ****************************************************************
 * @brief
 *  This function checks whether or not the port configuration has been
 *  read from persistent storage and can be relied upon.
 *
 * @param irpPtr        - pointer to the IRP data
 * @param ioStackPtr    - pointer to the I/O Stack data
 *
 * @return
 *  NTSTATUS
 *
 * @version
 *  03/07/2013  Brion Philbin Created. 
 *
 ****************************************************************/
static EMCPAL_STATUS hostside_get_general_io_port_status(PEMCPAL_IRP irpPtr, PEMCPAL_IO_STACK_LOCATION ioStackPtr)
{
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength;
    ULONG               outLength;
    fbe_bool_t          *port_config_valid;
    fbe_esp_module_mgmt_general_status_t general_port_status = {0};
    fbe_status_t        status;
    

    ioBuffer  = EmcpalExtIrpSystemBufferGet(irpPtr);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(ioStackPtr);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(ioStackPtr);

    status = fbe_api_esp_module_mgmt_general_status(&general_port_status);
    port_config_valid = (fbe_bool_t *)ioBuffer;
    *port_config_valid = general_port_status.port_configuration_loaded;


    EmcpalExtIrpStatusFieldSet(irpPtr, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(irpPtr, outLength);
	EmcpalIrpCompleteRequest(irpPtr);
	return EMCPAL_STATUS_SUCCESS;
}    /* end of cache_get_env_status() */

