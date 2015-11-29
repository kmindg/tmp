/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_terminator_kernel_main.c
 ***************************************************************************
 *
 *  Description
 *      Kernel space entry point to the terminator service
 *
 *
 *  History:
 *      06/11/08    sharel  Created
 *      11/16/10    miaot   Updated for terminator_init_services()
 ***************************************************************************/

#include "spid_kernel_interface.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_terminator_kernel_api.h"
#include "fbe_terminator_kernel.h"
#include "fbe/fbe_file.h"

#include "fbe_terminator.h"
#include "fbe_terminator_file_api.h"
#include "terminator_drive.h"
#include "terminator_port.h"
#include "terminator_simulated_disk.h"

#include "K10MiscUtil.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_Memory.h"

#ifdef C4_INTEGRATED
#include "ct_featurestate.h"
#endif /* C4_INTEGRATED */

// The names of all our registry entries
#define TERMINATOR_CONFIG_SIM_DRV_TYPE      "SimDriveType"
#define TERMINATOR_CONFIG_DIR               "ConfigDir"
#define TERMINATOR_CONFIG_FILE              "ConfigFile"
#define TERMINATOR_DISK_STORAGE_DIR         "DiskStorageDir"
#define TERMINATOR_CONFIG_REMOTE_SVR_NAME   "RemoteServerName"
#define TERMINATOR_CONFIG_REMOTE_SVR_PORT   "RemoteServerPort"
#define TERMINATOR_CONFIG_ARRAY_ID          "ArrayId"
#define TERMINATOR_ACCESS_MODE_NO_CACHE     "AccessModeNoCache"
#define TERMINATOR_TRACE_LEVEL              "TraceLevel"
#define TERMINATOR_PORT_IO_REG_THREAD_COUNT     "PortIOThreadCount"

#define MAX_REMOTE_SVR_NAME_LEN    32

typedef struct terminator_device_extension_s
{
    PEMCPAL_DEVICE_OBJECT                      device_object;
    terminator_simulated_drive_type_t   drive_type;
    fbe_u8_t                            cfg_dir[FBE_MAX_PATH];
    fbe_u8_t                            cfg_file[MAX_CONFIG_FILE_NAME];
    fbe_u8_t                            disk_storage_dir[FBE_MAX_PATH];
    char                                remote_svr_name[MAX_REMOTE_SVR_NAME_LEN];
    unsigned short                      remote_svr_port;
    unsigned short                      array_id;
    fbe_u32_t                           access_mode;
    fbe_u32_t                           trace_level;
} terminator_device_extension_t;

static terminator_device_extension_t *device_extension = NULL;

// Forward declarations
fbe_status_t terminator_init_services(void);
fbe_status_t terminator_destroy_services(void);
EMCPAL_STATUS terminator_set_config(void);
EMCPAL_STATUS terminator_open(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp);
EMCPAL_STATUS terminator_close(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp);
EMCPAL_STATUS terminator_ioctl(IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp);
EMCPAL_STATUS terminator_get_interface(PEMCPAL_IRP PIrp);
EMCPAL_STATUS terminator_unload(IN PEMCPAL_DRIVER PPalDriverObject);

//++
// Name:
//    EmcpalDriverEntry
//
// Description:
//    Initializes the Terminator kernel driver. Performs several functions:
//       1. Gets configuration from registry.
//       2. Creates Terminator device.
//       3. Sets dispatch functions for driver.
//       4. Initializes the FBE service manager and services.
//
// Arguments:
//    pPalDriverObject
//        This is the driver's PAL driver object.
//
// Returns:
//    STATUS_SUCCESS: Driver initialized successfully.
//    Other: Error status bubbled up from OS APIs.
//--
EMCPAL_STATUS EmcpalDriverEntry(IN PEMCPAL_DRIVER pPalDriverObject)
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;
    SPID        spInfo;

    PEMCPAL_NATIVE_DRIVER_OBJECT  PDriverObject = EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    KvTrace("terminator driver, starting...\n");

    status = EmcpalExtDeviceCreate(PDriverObject, sizeof(terminator_device_extension_t),
        FBE_TERMINATOR_DEVICE_NAME, FBE_TERMINATOR_DEVICE_TYPE, 0, FALSE, &device_object );

    if ( !EMCPAL_IS_SUCCESS(status) ){
        KvTrace("terminator driver, failed to create device, status: %X.\n", status);
        return status;
    }

    // Init device_extension
    device_extension = (terminator_device_extension_t *) EmcpalDeviceGetExtension(device_object);
    device_extension->device_object = device_object;

    // Read configuration into device extension
    status = terminator_set_config();
    
    if ( !EMCPAL_IS_SUCCESS(status) ) {
        KvTrace("terminator driver, failed to read registry settings, status:%X.\n", status);
        return status;
    }

    // Create a link from our device name to a name in the Win32 namespace.
    status = EmcpalExtDeviceAliasCreateA( FBE_TERMINATOR_DEVICE_LINK, FBE_TERMINATOR_DEVICE_NAME );

    if ( !EMCPAL_IS_SUCCESS(status) ) {
        KvTrace("terminator driver, failed to create symbolic link, status: %X.\n", status);
        EmcpalExtDeviceDestroy(device_object);
        return status;
    }

    // Create dispatch points for open, close, ioctl and unload.
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE, terminator_open);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE, terminator_close);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL, terminator_ioctl);
    EmcpalDriverSetUnloadCallback(pPalDriverObject, terminator_unload);

    // set SPID
    status = spidGetSpid(&spInfo);
    if (!EMCPAL_IS_SUCCESS(status)) {
        KvTrace("terminator driver, failed to get spid, status: %X\n", status);
        EmcpalExtDeviceAliasDestroyA(FBE_TERMINATOR_DEVICE_LINK);
        EmcpalExtDeviceDestroy(device_object);
        return status;
    }
    fbe_terminator_api_set_sp_id((spInfo.engine == 0) ? TERMINATOR_SP_A : TERMINATOR_SP_B);

    fbe_status = terminator_init_services();
    if(fbe_status != FBE_STATUS_OK) {
        KvTrace("terminator driver, failed to init services, status: %X.\n", fbe_status);
        EmcpalExtDeviceAliasDestroyA(FBE_TERMINATOR_DEVICE_LINK);
        EmcpalExtDeviceDestroy(device_object);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    return status;
}

//++
// Name:
//    terminator_set_config
//
// Description:
//    Reads config from registry and sets global data.
//
// Arguments:
//   None
//
// Returns:
//    STATUS_SUCCESS: Configuration initialized successfully.
//    Other: Error status bubbled up from OS APIs.
//--
EMCPAL_STATUS terminator_set_config(void)
{
    EMCPAL_STATUS    status = EMCPAL_STATUS_SUCCESS;
    UINT_32E    sim_drive_type;
    CHAR       *svr_name;
    ULONG       name_len;
    UINT_32E    svr_port,
                array_id;
    CHAR       *config_dir;
    ULONG       config_dir_len;
    CHAR       *config_file;
    ULONG       config_file_len;
    UINT_32E    access_mode_no_cache;
    CHAR       *disk_storage_dir;
    ULONG       disk_storage_dir_len;
    UINT_32E    trace_level,
                port_io_thread_count;
    const char *registryPath = EmcpalDriverGetRegistryPath(EmcpalGetCurrentDriver());

    //
    // get simulation drive type
    //
    status = EmcpalGetRegistryValueAsUint32Default(registryPath,
                                                   TERMINATOR_CONFIG_SIM_DRV_TYPE,
                                                   &sim_drive_type,
                                                   TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE);
    device_extension->drive_type = (terminator_simulated_drive_type_t)sim_drive_type;
    if (device_extension->drive_type < 0 || device_extension->drive_type >= TERMINATOR_SIMULATED_DRIVE_TYPE_LAST)
    {
        KvTrace("terminator driver, incorrect drive type 0x%x.\n", (int)sim_drive_type);
        return status;
    }

    if (device_extension->drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE)
    {
        //
        // get the remote server name
        //
        status = EmcpalGetRegistryValueAsString(registryPath,
                                                TERMINATOR_CONFIG_REMOTE_SVR_NAME,
                                                &svr_name,
                                                &name_len);
        if (!EMCPAL_IS_SUCCESS(status))
        {
            KvTrace("terminator driver, no remote server name.\n");
            EmcpalFreeRegistryMem(svr_name);
            return status;
        }

        if (name_len >= MAX_REMOTE_SVR_NAME_LEN)
        {
            KvTrace("terminator driver, the server name is too long.\n");
            EmcpalFreeRegistryMem(svr_name);
            return status;
        }

        EmcpalCopyMemory(device_extension->remote_svr_name, svr_name, name_len);
        EmcpalFreeRegistryMem(svr_name);

        //
        // get the remote server port
        //
        status = EmcpalGetRegistryValueAsUint32(registryPath,
                                                TERMINATOR_CONFIG_REMOTE_SVR_PORT,
                                                &svr_port);
        if (!EMCPAL_IS_SUCCESS(status))
        {
            KvTrace("terminator driver, failed to get remote server port, status: %X.\n", status);
            return status;
        }
        device_extension->remote_svr_port = (unsigned short)svr_port;

        //
        // get the array ID
        //
        status = EmcpalGetRegistryValueAsUint32(registryPath,
                                                TERMINATOR_CONFIG_ARRAY_ID,
                                                &array_id);
        if (!EMCPAL_IS_SUCCESS(status))
        {
            KvTrace("terminator driver, failed to get array id, status: %X.\n", status);
            return status;
        }
        device_extension->array_id = (unsigned short)array_id;
    }
    
    if (device_extension->drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE)
    {
        //
        // get the access mode to open disks
        //
        status = EmcpalGetRegistryValueAsUint32(registryPath,
                                                TERMINATOR_ACCESS_MODE_NO_CACHE,
                                                &access_mode_no_cache);
        if (!EMCPAL_IS_SUCCESS(status) || access_mode_no_cache == 0)
        {
            KvTrace("terminator driver, opening disks without NO CACHE/O_DIRECT.\n");
            device_extension->access_mode = 0;
        }
        else
        {
            KvTrace("terminator driver, opening disks with NO CACHE/O_DIRECT.\n");
            device_extension->access_mode = FBE_FILE_NCACHE;
        }
    }

    //
    // get the config dir
    //
    status = EmcpalGetRegistryValueAsString(registryPath,
                                            TERMINATOR_CONFIG_DIR,
                                            &config_dir,
                                            &config_dir_len);
    if (!EMCPAL_IS_SUCCESS(status))
    {
        KvTrace("terminator driver, failed to get local config directory, status %X.\n", status);
        EmcpalFreeRegistryMem(config_dir);
        return status;
    }

    if (config_dir_len >= FBE_MAX_PATH)
    {
        KvTrace("terminator driver, the specified config directory is too long.\n");
        EmcpalFreeRegistryMem(config_dir);
        return status;
    }

    EmcpalCopyMemory(device_extension->cfg_dir, config_dir, config_dir_len);
    EmcpalFreeRegistryMem(config_dir);

    //
    // get the config file
    //
    status = EmcpalGetRegistryValueAsString(registryPath,
                                            TERMINATOR_CONFIG_FILE,
                                            &config_file,
                                            &config_file_len);
    if (!EMCPAL_IS_SUCCESS(status))
    {
        KvTrace("terminator driver, failed to get local config file, status: %X.\n", status);
        EmcpalFreeRegistryMem(config_file);
        return status;
    }

    if (config_file_len >= MAX_CONFIG_FILE_NAME)
    {
        KvTrace("terminator driver, the specified config file name is too long.\n");
        EmcpalFreeRegistryMem(config_file);
        return status;
    }

    EmcpalCopyMemory(device_extension->cfg_file, config_file, config_file_len);
    EmcpalFreeRegistryMem(config_file);
    
    //
    // get the disk storage dir
    //
    status = EmcpalGetRegistryValueAsString(registryPath,
                                            TERMINATOR_DISK_STORAGE_DIR,
                                            &disk_storage_dir,
                                            &disk_storage_dir_len);
    if (!EMCPAL_IS_SUCCESS(status))
    {
        KvTrace("terminator driver, failed to get disk storage directory, status: %X.\n", status);
        EmcpalFreeRegistryMem(disk_storage_dir);
        return status;
    }

    if (disk_storage_dir_len >= FBE_MAX_PATH)
    {
        KvTrace("terminator driver, the specified disk storage directory is too long.\n");
        EmcpalFreeRegistryMem(disk_storage_dir);
        return status;
    }

    EmcpalCopyMemory(device_extension->disk_storage_dir, disk_storage_dir, disk_storage_dir_len);
    EmcpalFreeRegistryMem(disk_storage_dir);

    //
    // get the trace level
    //
    status = EmcpalGetRegistryValueAsUint32Default(registryPath,
                                                   TERMINATOR_TRACE_LEVEL,
                                                   &trace_level,
                                                   FBE_TRACE_LEVEL_INFO);
    device_extension->trace_level = (fbe_u32_t)trace_level;

    //
    // get the terminator port io thread count
    //
    status = EmcpalGetRegistryValueAsUint32Default(registryPath,
                                                   TERMINATOR_PORT_IO_REG_THREAD_COUNT,
                                                   &port_io_thread_count,
                                                   TERMINATOR_PORT_IO_DEFAULT_THREAD_COUNT);
    if (port_io_thread_count <= TERMINATOR_PORT_IO_MAX_THREAD_COUNT)
    {
        terminator_port_io_thread_count = (fbe_u32_t)port_io_thread_count;
        KvTrace("terminator driver, the port io thread count is set to 0x%X.\n", port_io_thread_count);
    }
    else
    {
        terminator_port_io_thread_count = TERMINATOR_PORT_IO_MAX_THREAD_COUNT;
        KvTrace("terminator driver, the port io thread count 0x%X exceeds max, set to max.\n", port_io_thread_count);
    }

    return status;
}

//++
// Name:
//    terminator_init_services
//
// Description:
//    Operate on Terminator related initializations.
//
// Arguments:
//    N/A
//
// Returns:
//    FBE_STATUS_OK: All the initializations completed successfully.
//    Other: Error status bubbled up from Terminator APIs.
//--
fbe_status_t terminator_init_services(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
#ifdef C4_INTEGRATED
    fbe_u64_t virtual_deploymnet_feature_state = FEATURESTATE_EXCLUDED;
    fbe_status_t ff_status = FBE_STATUS_GENERIC_FAILURE;
#endif

    KvTrace("%s starting...\n", __FUNCTION__);

    fbe_trace_set_default_trace_level(device_extension->trace_level);

    // get driver simulated type
    status = fbe_terminator_api_set_simulated_drive_type(device_extension->drive_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to set the simulated drive type, status: %X!\n",
                         __FUNCTION__, status);
        return status;
    }

    if (device_extension->drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE) 
        terminator_simulated_disk_remote_file_simple_init(EmcpalDriverGetCurrentClientObject()->csxModuleContext, 
                device_extension->remote_svr_name, device_extension->remote_svr_port, device_extension->array_id);

    status = fbe_terminator_api_init();
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to initialize the Terminator, status: %X!\n",
                         __FUNCTION__, status);
        return status;
    }

    //
    // set the terminator io mode to be enabled
    //
    status = fbe_terminator_api_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to set the terminator io mode, status: %X!\n", 
                         __FUNCTION__, status);
        return status;  
    }

    //
    // load the terminator disk storage dir from the device extension, this should be prior to
    //  fbe_terminator_api_load_config_file() since it will call terminator_file_api_init_new()
    //  to activate all the devices, while activating the drives, it needs to use
    //  terminator_file_api_get_disk_storage_dir() to get the disk storage dir.
    //
    status = fbe_terminator_api_load_disk_storage_dir(device_extension->disk_storage_dir);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to load terminator disk storage directory, status: %X!\n",
                         __FUNCTION__,
                         status);
        return status;
    }
    
    //
    // Enable access modes pulled from registry.  This must happen before fbe_terminator_api_load_config_file()
    // because it changes how the disk files are opened.
    // Currently only supports O_DIRECT
    //
    status = fbe_terminator_api_load_access_mode(device_extension->access_mode);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to load terminator access mode, status: %X!\n",
                         __FUNCTION__,
                         status);
        return status;
    }

#ifdef C4_INTEGRATED
    ff_status = get_feature_state ("VIRTUAL_DEPLOYMENT", &virtual_deploymnet_feature_state);

    if (!EMCPAL_IS_SUCCESS(ff_status))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "Failed to get 'VIRTUAL_DEPLOYMENT' feature flag value (status=0x%x)", ff_status);
    }
    else
    {
        if (virtual_deploymnet_feature_state == FEATURESTATE_ENABLED)
        {
            terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "'VIRTUAL_DEPLOYMENT' feature flag detected: ignore terminator config file.");

            return FBE_STATUS_OK;
        }
    }
#endif

    //
    // load the terminator config file from the config dir
    //
    status = fbe_terminator_api_load_config_file(device_extension->cfg_dir, device_extension->cfg_file);
    if (status != FBE_STATUS_OK)
    {

        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to load the terminator config file, status: %X!\n",
                         __FUNCTION__, status);

        return status;
    }

    return status;
}

//++
// Name:
//    terminator_open
//
// Description:
//    Handles IRP_MJ_CREATE. Just complete the IRP.
//
// Arguments:
//    PDeviceObject
//        Caller-supplied pointer to a DEVICE_OBJECT structure. This is the
//        device object for the target device, previously created by the driver's
//        AddDevice routine.
//
//    PIrp
//        Caller-supplied pointer to an IRP structure that describes the requested
//        I/O operation.
//
// Returns:
//    STATUS_SUCCESS
//--
EMCPAL_STATUS terminator_open( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s, entry.\n", __FUNCTION__);

    // Just complete the IRP
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

//++
// Name:
//    terminator_close
//
// Description:
//    Handles IRP_MJ_CLOSE. Just complete the IRP.
//
// Arguments:
//    PDeviceObject
//        Caller-supplied pointer to a DEVICE_OBJECT structure. This is the
//        device object for the target device, previously created by the driver's
//        AddDevice routine.
//
//    PIrp
//        Caller-supplied pointer to an IRP structure that describes the requested
//        I/O operation.
//
// Returns:
//    STATUS_SUCCESS
//--
EMCPAL_STATUS terminator_close( IN PEMCPAL_DEVICE_OBJECT PDeviceObject, IN PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;

    FBE_UNREFERENCED_PARAMETER(PDeviceObject);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s, entry.\n", __FUNCTION__);

    // Just complete the IRP
    EmcpalExtIrpStatusFieldSet(PIrp, status);
    EmcpalExtIrpInformationFieldSet(PIrp, 0);

    EmcpalIrpCompleteRequest(PIrp);
    return status;
}

//++
// Name:
//    terminator_destroy_services
//
// Description:
//    Shuts down the services in the opposite order in which they were started.
//
// Arguments:
//    N/A
//
// Returns:
//    FBE_STATUS_OK: Services shutdown successfully.
//    Other: Error status bubbled up from OS APIs.
//--
fbe_status_t
terminator_destroy_services(void)
{
    return fbe_terminator_api_destroy();
}

//++
// Name:
//    terminator_unload
//
// Description:
//    Unloads the driver. Basically undoes all the stuff we did in DriverEntry.
//
// Arguments:
//    PDriverObject
//        Caller-supplied pointer to a DRIVER_OBJECT structure. This is the
//        driver's driver object.
//
// Returns:
//    N/A
//--
EMCPAL_STATUS terminator_unload( IN PEMCPAL_DRIVER PPalDriverObject )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_DEVICE_OBJECT device_object = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject = EmcpalDriverGetNativeDriverObject(PPalDriverObject);

    KvTrace("terminator driver, unloading...\n");

    terminator_destroy_services();

    // Delete the link from our device name to a name in the Win32 namespace.
    status = EmcpalExtDeviceAliasDestroyA( FBE_TERMINATOR_DEVICE_LINK );
    if ( !EMCPAL_IS_SUCCESS(status) ) {
        KvTrace("terminator driver, failed to delete symbolic link, status: %X \n", status);
    }

    /* Find our Device Object and Device Extension*/
    device_object = EmcpalExtDeviceListFirstGet(PDriverObject);

    // Finally delete our device object
    EmcpalExtDeviceDestroy(device_object);
    device_extension = NULL;

    return(EMCPAL_STATUS_SUCCESS);
}

//++
// Name:
//    terminator_ioctl
//
// Description:
//    Handles IRP_MJ_DEVICE_CONTROL.
//
// Arguments:
//    PDeviceObject
//        Caller-supplied pointer to a DEVICE_OBJECT structure. This is the
//        device object for the target device, previously created by the driver's
//        AddDevice routine.
//
//    PIrp
//        Caller-supplied pointer to an IRP structure that describes the requested
//        I/O operation.
//
// Returns:
//    STATUS_SUCCESS: IRP processed successfully.
//    STATUS_BAD_DEVICE_TYPE: Bad device object passed in.
//    STATUS_INVALID_DEVICE_REQUEST: Unrecognized IOCTL.
//--
EMCPAL_STATUS terminator_ioctl( PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP PIrp )
{
    EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION pIoStackLocation = NULL;
    ULONG IoControlCode = 0;

    if (EmcpalExtDeviceTypeGet(PDeviceObject) != FBE_TERMINATOR_DEVICE_TYPE) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s, invalid device type: %X\n", __FUNCTION__, EmcpalExtDeviceTypeGet(PDeviceObject));

        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_BAD_DEVICE_TYPE);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        EmcpalIrpCompleteRequest(PIrp);
        return EMCPAL_STATUS_BAD_DEVICE_TYPE;
    }

    pIoStackLocation = EmcpalIrpGetCurrentStackLocation(PIrp);

    /* Figure out the IOCTL */
    IoControlCode = EmcpalExtIrpStackParamIoctlCodeGet(pIoStackLocation);

    switch ( IoControlCode )
    {
    case FBE_TERMINATOR_GET_INTERFACE_IOCTL:
        status = terminator_get_interface(PIrp);
        break;

    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s, invalid IoControlCode: %X\n", __FUNCTION__, IoControlCode);

        EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_INVALID_DEVICE_REQUEST);
        EmcpalExtIrpInformationFieldSet(PIrp, 0);
        status = EmcpalExtIrpStatusFieldGet(PIrp);
        EmcpalIrpCompleteRequest(PIrp);
        break;
    }

    return status;
}

//++
// Name:
//    terminator_get_interface
//
// Description:
//    Handles FBE_TERMINATOR_GET_INTERFACE_IOCTL requests.  Copies the Terminator
//    interface functions into the IRP's system buffer.
//
// Arguments:
//    PIrp
//        Caller-supplied pointer to an IRP structure that describes the requested
//        I/O operation.
//
// Returns:
//    STATUS_SUCCESS: Interface copied successfully.
//--
EMCPAL_STATUS terminator_get_interface(PEMCPAL_IRP PIrp)
{
    PEMCPAL_IO_STACK_LOCATION  irpStack = NULL;
    fbe_u8_t          * ioBuffer = NULL;
    ULONG               inLength = 0;
    ULONG               outLength = 0;
    fbe_terminator_get_interface_t   * terminator_interface = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s, entry\n", __FUNCTION__);

    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);

    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
    inLength  = EmcpalExtIrpStackParamIoctlInputSizeGet(irpStack);
    outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

    terminator_interface = (fbe_terminator_get_interface_t *)ioBuffer;

    // Initialize terminator interfaces
    terminator_interface->terminator_api.init  = fbe_terminator_api_init;

    // Initialize terminator miniport interfaces
    terminator_interface->miniport_api.terminator_miniport_api_port_init_function                     = fbe_terminator_miniport_api_port_init;
    terminator_interface->miniport_api.terminator_miniport_api_port_destroy_function                  = fbe_terminator_miniport_api_port_destroy;
    terminator_interface->miniport_api.terminator_miniport_api_register_callback_function             = fbe_terminator_miniport_api_register_callback;
    terminator_interface->miniport_api.terminator_miniport_api_unregister_callback_function           = fbe_terminator_miniport_api_unregister_callback;
    terminator_interface->miniport_api.terminator_miniport_api_register_payload_completion_function   = fbe_terminator_miniport_api_register_payload_completion;
    terminator_interface->miniport_api.terminator_miniport_api_unregister_payload_completion_function = fbe_terminator_miniport_api_unregister_payload_completion;
    terminator_interface->miniport_api.terminator_miniport_api_enumerate_cpd_ports_function           = fbe_terminator_miniport_api_enumerate_cpd_ports;
    terminator_interface->miniport_api.terminator_miniport_api_register_sfp_event_callback_function   = fbe_terminator_miniport_api_register_sfp_event_callback;
    terminator_interface->miniport_api.terminator_miniport_api_unregister_sfp_event_callback_function = fbe_terminator_miniport_api_unregister_sfp_event_callback;

    terminator_interface->miniport_api.terminator_miniport_api_get_port_type_function                = fbe_terminator_miniport_api_get_port_type;
    terminator_interface->miniport_api.terminator_miniport_api_remove_port_function                  = fbe_terminator_miniport_api_remove_port;
    terminator_interface->miniport_api.terminator_miniport_api_port_inserted_function                = fbe_terminator_miniport_api_port_inserted;
    terminator_interface->miniport_api.terminator_miniport_api_set_speed_function                    = fbe_terminator_miniport_api_set_speed;
    terminator_interface->miniport_api.terminator_miniport_api_get_port_info_function                = fbe_terminator_miniport_api_get_port_info;
    terminator_interface->miniport_api.terminator_miniport_api_send_payload_function                 = fbe_terminator_miniport_api_send_payload;
    terminator_interface->miniport_api.terminator_miniport_api_send_fis_function                     = fbe_terminator_miniport_api_send_fis;
    terminator_interface->miniport_api.terminator_miniport_api_reset_device_function                 = fbe_terminator_miniport_api_reset_device;
    terminator_interface->miniport_api.terminator_miniport_api_get_port_address_function             = fbe_terminator_miniport_api_get_port_address;
    terminator_interface->miniport_api.terminator_miniport_api_get_hardware_info_function            = fbe_terminator_miniport_api_get_hardware_info;
    terminator_interface->miniport_api.terminator_miniport_api_get_sfp_media_interface_info_function = fbe_terminator_miniport_api_get_sfp_media_interface_info;
    terminator_interface->miniport_api.terminator_miniport_api_port_configuration_function           = fbe_terminator_miniport_api_port_configuration;
    terminator_interface->miniport_api.terminator_miniport_api_get_port_link_info_function           = fbe_terminator_miniport_api_get_port_link_info;
    terminator_interface->miniport_api.terminator_miniport_api_register_keys_function                = fbe_terminator_miniport_api_register_keys;
    terminator_interface->miniport_api.terminator_miniport_api_unregister_keys_function              = fbe_terminator_miniport_api_unregister_keys;
    terminator_interface->miniport_api.terminator_miniport_api_reestablish_key_handle_function       = fbe_terminator_miniport_api_reestablish_key_handle;

    EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(PIrp, outLength);
    EmcpalIrpCompleteRequest(PIrp);
    return EMCPAL_STATUS_SUCCESS;
}

fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
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
    if(device_extension != NULL) 
    {
        *device_object = device_extension->device_object;
        return EMCPAL_STATUS_SUCCESS;
    }
    return EMCPAL_STATUS_NO_SUCH_DEVICE;
}
/*************************************************
    end get_package_device_object()
*************************************************/
