/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.


 File Name:
      ppfdDriverEntry.c

 Contents:
      DriverEntry()

 Internal:

 Exported:
      DriverEntry() 

 Revision History:
    06-Feb-09 Eric Quintin  Add support for detecting backend type (FC or SAS/SATA)
    05-Mar-09 Eric Quintin  Added support to read ktrace/windbg/debugbreak from the registry. 
                            Fix all comment headers and complete "cleanup" of code.

***************************************************************************/

//
// Include Files
//
//#include "K10MiscUtil.h"
#include "ppfdDeviceExtension.h"
#include "ppfdMajorFunctions.h"
#include "EmcPrefast.h"
//#include <ntstrsafe.h>
#include "ppfdMisc.h"
#include "ppfdDriverEntryProto.h"       // Local function prototypes
#include "ppfdKtraceProto.h"
#include "spid_kernel_interface.h"      // to get the SP ID to support getting the SLIC type in slot 0
#include "specl_interface.h"
#include "generic_utils_lib.h"
#include "k10ntddk.h"
#include "cpd_interface.h"   /* for CPD_IOCTL_ADD_DEVICE */
#include "ppfd_panic.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcUTIL.h"



//
//Imported functions
//
void ppfdPhysicalPackageInitScsiPortNumberInMap( UCHAR scsiPortNumber );
EMCPAL_STATUS ppfdDispatchPhysicalPackage( IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp );
BOOL ppfdCheckBreakOnEntry( void );
VOID ppfdInitializeDebugLevel( void );
BOOL ppfdIsPnPDiskDeviceObject(PEMCPAL_DEVICE_OBJECT deviceObject );
BOOL ppfdIsDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject );
EMCPAL_STATUS ppfdCreateDiskDeviceObjects( PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject );
#ifndef C4_INTEGRATED
EMCPAL_STATUS ppfdAddDevice(PEMCPAL_NATIVE_DRIVER_OBJECT driverObject, PEMCPAL_DEVICE_OBJECT physicalDeviceObject );
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */


// Allocate the PPFD Global Data Structure that is useful for debugging with debug extension
PPFD_GLOBAL_DATA PPFD_GDS;



/*******************************************************************************************

 Function:
    NTSTATUS EmcpalDriverEntry(
           PEMCPAL_DRIVER pPalDriverObject
           )
    
 Description:
    Initializes and creates the PPFD Device

 Arguments:
    pPalDriverObject  

 Return Value:
    STATUS_SUCCESS or NTSTATUS ERROR code if failure
    
 Revision History:

/*******************************************************************************************/

EMCPAL_STATUS
EmcpalDriverEntry(PEMCPAL_DRIVER pPalDriverObject)
{
    PEMCPAL_NATIVE_DRIVER_OBJECT  PDriverObject;
    PEMCPAL_DEVICE_OBJECT  ppfdPortDeviceObject = NULL;
    EMCPAL_STATUS        ntStatus = EMCPAL_STATUS_SUCCESS;

    DWORD           i;
    BOOL            bStatus;
    CHAR            outBuffer[MAX_LOG_CHARACTER_BUFFER] = "";

    // Establish initial ktrace default level immediately.  It will be initialzed later by reading values from
    // the registry.  If they don't exist in the registry, then the default will be used.
    ppfdGlobalTraceFlags = PPFD_GLOBAL_TRACE_FLAGS_DEFAULT;

    PDriverObject = (PEMCPAL_NATIVE_DRIVER_OBJECT)EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    // Initialize our global data structure (pass the driver object as it is saved in the structure)
    ppfdInitGlobalData( PDriverObject );

    //
    // The Rtl String functions can only be accessed at PASSIVE level
    //
    DBG_PAGED_CODE();
 
    csx_p_snprintf(outBuffer,
                sizeof(outBuffer),
                "PPFD: Starting %s() - Compiled for %s\n",
                __FUNCTION__,
                EmcutilBuildType());

    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START,  outBuffer);

    // For debug purposes, break in if registry flag set 
    if (ppfdCheckBreakOnEntry())
        EmcpalDebugBreakPoint();

    // Initialize the ktrace and winbdg message levels based on registry settings
    ppfdInitializeDebugLevel();

    //
    // Create the ppfdPort0 device object
    //
    ntStatus = EmcpalExtDeviceCreate(
                               PDriverObject,
                               sizeof(PPFD_DEVICE_EXTENSION),
                               PPFDPORT_DEVICE_NAME,
                               EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               &ppfdPortDeviceObject
                               );

     if ((!EMCPAL_IS_SUCCESS(ntStatus)) || (ppfdPortDeviceObject == NULL))
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry IoCreateDevice()Failed status=0x%x\n", ntStatus );
        return( ntStatus );
    }
    
        
    //
    // Default to dispatch all through one central Dispatch function.
    // Filter drivers should have a dispatch entry for all IRP's.
    //
    for (i = 0; i < EMCPAL_NUM_MAJOR_FUNCTIONS; ++i)
    {
        EmcpalExtDriverIrpHandlerSet(PDriverObject, i, DispatchAny);
    }

    //
    // Create dispatch points for create/open, close, and unload.
    //

    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CREATE,
                                 ppfdDrvOpen);
    EmcpalExtDriverIrpHandlerSet(PDriverObject, EMCPAL_IRP_TYPE_CODE_CLOSE,
                                 ppfdDrvClose);
#if 0
    //Removing unload entry point since PPFD is not supposed to be unloaded
    EmcpalDriverSetUnloadCallback(pPalDriverObject, ppfdDrvUnload);
#endif
    EmcpalExtDeviceIoDirectSet(ppfdPortDeviceObject);            
        
    // Increment stack size by one so that we can send requests received at name device object to 
    // underlying miniport driver.
    EmcpalDeviceSetIrpStackSize(ppfdPortDeviceObject, EmcpalDeviceGetIrpStackSize(ppfdPortDeviceObject)+  1);
      
#ifndef C4_INTEGRATED
    // Setup our add device callback.
    EmcpalExtDriverAddDeviceHandlerSet(PDriverObject, ppfdAddDevice);
#endif /* C4_INTEGRATED - C4ARCH - OSdisk - C4BUG - PnP mech usage */

    //
    // Init the device extension data
    //
    ntStatus = ppfdDrvInitializeDeviceExtension(ppfdPortDeviceObject );                                                   
    if ( !EMCPAL_IS_SUCCESS(ntStatus) )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry ppfdDrvInitializeDeviceExtension() Failed status=0x%x\n", ntStatus );

        // Cleanup before returning
        EmcpalExtDeviceDestroy( ppfdPortDeviceObject );
        return( ntStatus );
    }
  

    //
    // Detect the type of backend we are booting from so that we can initialize for either Fibre backend or 
    // SAS/SATA backend.  For SAS/SATA backend, PPFD will have to use the PhysicalPackage so we initialize
    // the interface to PP in that case
    //

    if( ppfdDetectBackendType() == FC_BACKEND )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: DriverEntry Boot Device Type is FC\n" );

        // The backend "boot" drives are Fibre channel. In that case we will attach to the Scsiport/FC miniport 
        // for backend 0.  We can then forward all IRP's (unmodified)to the Scsiport/miniport driver.  No need 
        // to use the Physicalpackage for Fibre backend.
        //
        bStatus = AttachToScsiPort( PDriverObject, ppfdPortDeviceObject );
        if( !bStatus )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry failed to attach to BE0 Scsiport for Fibre backend!\n" );
            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        // Not FC, so must be SAS or SATA..we'll use the Physicalpackage
        // If the BE Type detected is something other than SAS/SATA or FC, we'll ktrace
        // Prefer not to panic in that case..it may just be a new SLIC that we do not have supported in the detect code, but
        // we can still do I/O to it through PP
        if( ppfdDetectBackendType() == UNSUPPORTED_BACKEND )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdDetectBackendType Unsupported Boot Device Type, default to SAS/SATA!\n");
        }
        else
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: DriverEntry Boot Device Type is SAS or SATA\n" );
        }
        
        //
        // Init Physical Package interface and attach to backend 0 Scsiport.  
        //
        bStatus = ppfdPhysicalPackageInterfaceInit();
        if( bStatus )
        {
            //Starting this thread only for SAS backend.
            ntStatus = ppfdWriteStartIoRestartThread();
            if ( !EMCPAL_IS_SUCCESS(ntStatus) )
            {
                PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry ppfdWriteStartIoRestartThread() Failed status=0x%x\n", ntStatus );

                EmcpalExtDeviceDestroy( ppfdPortDeviceObject );
                return( ntStatus );
            }

            bStatus = AttachToScsiPort( PDriverObject, ppfdPortDeviceObject );
            if( !bStatus )
            {
                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry failed to attach to BE0 Scsiport for SAS/SATA backend!\n" );
                ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
            }
            else
            {
                //
                // Create PPFD disk device objects to be used by ASIDC.
#ifndef C4_INTEGRATED
                // PPFD "PnP" disk device objects used by NtMirror are creatd
                // in "ppfdAddDevice()" (called via the Windows plug and play
                // manager)
#else /* C4_INTEGRATED */
                // For C4, ntmirror also uses the "non-PnP" disk device objects.
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */
                //
                ntStatus = ppfdCreateDiskDeviceObjects( PDriverObject );


            }
        }
        else
        {
            //
            // Failed to init the PhysicalPackage interface.  This would likely only happen if the Physicalpackage driver
            // failed to start.  Without PhysicalPackage, PPFD cannot access the boot drives..so we'll panic.
            //
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry failed, PhysicalPackage Init failed!\n" );
            panic( PPFD_FAILED_PHYSICALPACKAGE_INIT, 0 );
        }
    }
  
    //
    // If we failed, cleanup before returning the ntStatus error
    //
    if ( !EMCPAL_IS_SUCCESS(ntStatus) )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: DriverEntry Failed status=0x%x\n", ntStatus );

        // Cleanup before returning
        if ( NULL != ppfdPortDeviceObject )
        {
            EmcpalExtDeviceDestroy( ppfdPortDeviceObject );
        }
    }
        
    // Log end of DriverEntry() to start buffer.
    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: DriverEntry complete, status 0x%x\n", ntStatus );

    return ( ntStatus );
}
// .End DriverEntry


/*******************************************************************************************

 Function:
    void ppfdInitGlobalData( PDRIVER_OBJECT PDriverObject )
    
 Description:
    Initializes data in the PPFD global data  structure "PPFD_GDS".
    Should be called very early in DriverEntry routine.

 Arguments:
    PDriverObject

 Return Value:
    none

 Revision History:
*******************************************************************************************/
void ppfdInitGlobalData( PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject )
{
    SPID spid;
    EMCPAL_STATUS ntStatus;
    PEMCPAL_CLIENT pPalClient;
    ULONG   Size = 0;

    PPFD_GDS.pDriverObject = PDriverObject;
    // This will be updated when PPFD detects the backend type during DriverEntry() processing
    PPFD_GDS.beType = UNSUPPORTED_BACKEND;
    // This will be updated when PPFD locates the backend 0 Scsiport device during DriverEntry() processing
    PPFD_GDS.Be0ScsiPortObject = 0;        

    ntStatus = spidGetSpid(&spid);
    if (((spid.engine == SP_A) || (spid.engine == SP_B)) && (EMCPAL_IS_SUCCESS( ntStatus )))
    {
        PPFD_GDS.SpId = spid.engine;
    }
    else
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_START,"PPFD: ppfdInitGlobalData Unable to get SP ID!\n");
        PPFD_GDS.SpId = SP_INVALID;
        // should we panic?
    }

    PPFD_GDS.pnpDiskNumber = 0;

    pPalClient = EmcpalDriverGetCurrentClientObject();

    Size = sizeof (PPFD_WRITE_CONTEXT);

    EmcpalLookasideListCreate(pPalClient,
                              &PPFD_GDS.IoContextLookasideListHead,
                              NULL, NULL, Size, 0 /* depth */, 'xfpp');

    Size = sizeof(PPFD_SGL_LIST_HEADER) +
           PPFD_SMALL_SGL_LIST_MAX_SGL_COUNT * sizeof(fbe_sg_element_t);

    EmcpalLookasideListCreate(pPalClient,
                              &PPFD_GDS.SmallSGLListLookasideListHead,
                              NULL, NULL, Size, 0 /* depth */, 'sfpp');

    Size = sizeof(PPFD_SGL_LIST_HEADER) +
           PPFD_MEDIUM_SGL_LIST_MAX_SGL_COUNT * sizeof(fbe_sg_element_t);

    EmcpalLookasideListCreate(pPalClient,
                              &PPFD_GDS.MediumSGLListLookasideListHead,
                              NULL, NULL, Size, 0 /* depth */, 'mfpp');


    fbe_spinlock_init(&PPFD_GDS.IoRetryQueueSpinLock);

    PPFD_GDS.AbortIoRetryThread = FALSE;
    fbe_rendezvous_event_init(&PPFD_GDS.IoRetryEvent);
    EmcpalInitializeListHead(&PPFD_GDS.IoRetryListHead);
}

/*******************************************************************************************
 Function:
    NTSTATUS ppfdDrvInitializeDeviceExtension(IN PDEVICE_OBJECT PDeviceObject)
    
 Description:
    Initialize the device extension data for the device object passed in.  

 Arguments:
    PDeviceObject

 Return Value:
    STATUS_SUCCESS:  the Device Extension was initialized
    STATUS_INVALID_PARAMETER - Returned if the PDeviceObject passed in is null

 Revision History:

*******************************************************************************************/
EMCPAL_STATUS
ppfdDrvInitializeDeviceExtension( PEMCPAL_DEVICE_OBJECT PDeviceObject )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    PPPFD_DEVICE_EXTENSION pDeviceExtension = NULL;
        
    if (PDeviceObject==NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_STD,"PPFD: ppfdDrvInitializeDeviceExtension: NULL PDeviceObject!\n");
        return ( EMCPAL_STATUS_INVALID_PARAMETER );
    }

    pDeviceExtension = (PPPFD_DEVICE_EXTENSION) EmcpalDeviceGetExtension(PDeviceObject);
    pDeviceExtension->DeviceObject = PDeviceObject;
    

    pDeviceExtension->BE0ScsiPortDeviceObject = NULL;
    pDeviceExtension->CommonDevExt.DeviceFlags = PPFD_PORT_DEVICE_OBJECT;
    pDeviceExtension->CommonDevExt.Size = sizeof( PPPFD_DEVICE_EXTENSION );
    pDeviceExtension->CommonDevExt.Version = 1;
    
    return ( ntStatus );
}
// .End ppfdDrvInitializeDeviceExtension




/*******************************************************************************************
 Function:
    void AsyncFillIoControlHdr( CPD_IOCTL_HEADER *pIoControl )

 Description: 
    Fills in a SRB_IO_CONTROL structure with some default values.  Helper function.
    If pSrbIoControl is NULL, it will return without doing anything.

 Arguments: 
    pSrbIoControl - Pointer to a SRB_IO_CONTROL structure to fill in.

 Return Value: 
    None

 /*******************************************************************************************/
 
EMCPAL_LOCAL void AsyncFillIoControlHdr( CPD_IOCTL_HEADER *pIoControl )
{
    if (pIoControl==NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_STD,"PPFD: AsyncFillIoControlHdr: NULL input pointer!\n");
        return;
    }

    CPD_IOCTL_HEADER_INIT(pIoControl, 0, 10, 0);
    
} // end of function AsyncFillIoControlHdr()



/******************************************************************************
 Function:
    NTSTATUS ppfdSendIoctl (IN DEVICE_OBJECT  *pPortDeviceObject,
                            IN VOID           *pIoCtl,
                            IN ULONG          Size)

 Description: 
    Sends a MiniPort-Specific IOCTL to the MiniPort.
    
Arguments: 
    pPortDeviceObject - Miniport device object for adapter, not individual disk
    pIoCtl - Details of the message.
    Size   - Size in bytes of the message.
 
 Return Value: 
    NTSTATUS =

 History:

******************************************************************************/
EMCPAL_LOCAL EMCPAL_STATUS
ppfdSendIoctl(IN PEMCPAL_DEVICE_OBJECT  pPortDeviceObject,
              IN VOID                  *pIoCtl,
              IN ULONG                  Size)
{
    EMCPAL_STATUS        status;
    EMCPAL_LARGE_INTEGER TimeOut;

    // make sure we're running at IRQL_PASSIVE_LEVEL. This will panic if we are
    // not.
    IRQL_PASSIVE();
    
    if ((pPortDeviceObject==NULL) || (pIoCtl==NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_STD,
                   "PPFD: PpfdSendIoctl: NULL input pointer!\n");

        return (EMCPAL_STATUS_INVALID_PARAMETER);
    }
    //
    // The idea here is that the SRB should timeout within
    // PPFD_SRB_TIMEOUT seconds.  By setting our timeout period to be
    // twice that long (negative for relative time) we'll catch cases
    // where the IRP gets stuck somewhere.
    //
    TimeOut.QuadPart = -2 * PPFD_SRB_TIMEOUT * TicksPerSecond;

    status = EmcpalExtIrpDoIoctlWithTimeout(EmcpalDriverGetCurrentClientObject(),
                                            IOCTL_SCSI_MINIPORT,
                                            pPortDeviceObject,
                                            pIoCtl, Size,
                                            pIoCtl, Size,
                                            EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&TimeOut));

    if (status == EMCPAL_STATUS_TIMEOUT)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_STD,
                   "PPFD: PpfdSendIoctl: CpdSrb Timed out.\n");
    }
    status = CPD_IOCTL_STATUS(status, (CPD_IOCTL_HEADER *)pIoCtl);

    return (status);
}   /* end of ppfdSendIoctl() */


/*******************************************************************************************

 Function:
    NTSTATUS GetScsiConfig (DEVICE_OBJECT *pPortDeviceObject, CPD_CONFIG **ppConfigInfo)

 Description:
    Most of this function was copied from the Asidc driver code (asidcGetScsiConfig())
    This routine sends a Config IOCTL request to a port driver to return configuration information. 
    Space for the information is allocated by this routine. The caller is responsible for freeing both the 
    pass_thru_buf within the buffer containing the configuration information and the CPD_CONFIG buffer 
    itself. This routine is synchronous.
 
 Arguments:
    pPortDeviceObject - Port driver device object representing the HBA.
    ppConfigInfo - Location to store pointer to Config structure.
 
 Return Value:
    NT Status indicating the results of the operation.  If successful, caller must free buffers as noted above.  
    If not successful, caller should NOT use the *ppConfigInfo pointer since it will not be valid.
 
  History:
    ECQ     9/5/08 Copied (and slightly modified) from ASIDC function AsidcScsiGetConfg()

/*******************************************************************************************/

EMCPAL_STATUS GetScsiConfig (PEMCPAL_DEVICE_OBJECT pPortDeviceObject, CPD_CONFIG **ppConfigInfo)
{
    EMCPAL_STATUS        ntStatus = EMCPAL_STATUS_SUCCESS;
    CPD_CONFIG      *pBuffer = NULL;
    S_CPD_CONFIG    IoCtl; // this includes a CPD_CONFIG of its own

    if (pPortDeviceObject == NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_START, "PPFD: GetScsiConfig: NULL input pointer!\n");
        return( EMCPAL_STATUS_INVALID_PARAMETER );
    }

    //
    // Allocate a buffer for the SCSI Configuration data.
    // Caller is responsible for freeing this buffer.
    //
    pBuffer = fbe_allocate_nonpaged_pool_with_tag(sizeof(CPD_CONFIG), 'dfpp');
    if (pBuffer == NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD:GetScsiConfig: Unable to alloc CPD_CONFIG\n");
        return(EMCPAL_STATUS_INSUFFICIENT_RESOURCES);
    }
    fbe_zero_memory(pBuffer, sizeof(CPD_CONFIG));

    *ppConfigInfo = pBuffer;

    //
    //  Set up most of the CPD_IOCTL_HEADER structure by calling this function
    //  instead of using the following:
    //  IoCtl.ioctl_hdr = SrbIoControlTemplate;
    // 
    AsyncFillIoControlHdr(&IoCtl.ioctl_hdr );

    CPD_IOCTL_HEADER_SET_OPCODE(&IoCtl.ioctl_hdr, CPD_IOCTL_GET_CONFIG);
    CPD_IOCTL_HEADER_SET_LEN(&IoCtl.ioctl_hdr, sizeof(CPD_CONFIG));
    
    IoCtl.param.portal_nr        = 0; // only portal 0 supported at this time.

    IoCtl.param.fmt_ver_major = CPD_CONFIG_FORMAT_VERSION;
    IoCtl.param.pass_thru_buf = fbe_allocate_nonpaged_pool_with_tag( PPFD_CPD_CONFIG_PASSTHRU_LEN, 'dfpp');
    if (IoCtl.param.pass_thru_buf == NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD:GetScsiConfig: Unable to alloc CPD_CONFIG passthru\n");
        fbe_release_nonpaged_pool(pBuffer);
        return(EMCPAL_STATUS_INSUFFICIENT_RESOURCES);
    }
    
    IoCtl.param.pass_thru_buf_len = PPFD_CPD_CONFIG_PASSTHRU_LEN;
    fbe_zero_memory(IoCtl.param.pass_thru_buf, IoCtl.param.pass_thru_buf_len);

    ntStatus = ppfdSendIoctl(pPortDeviceObject, 
                            &IoCtl, 
                            (ULONG)sizeof(IoCtl));


    //
    // If IOCTL called successfully, wait on completion event 
    //
    if (EMCPAL_IS_SUCCESS(ntStatus))
    {
        // This line copies the contents of the CPD_CONFIG structure,
        // including the pass_thru_buf pointer into caller's buffer.
        // Note that *ppConfigInfo is the same as pBuffer.
        //
        memcpy(*ppConfigInfo, &(IoCtl.param), sizeof(CPD_CONFIG));
    }
    else
    {
        // The IOCTL could not be sent to the disk for some reason 
        // This will happen if the ScsiportN we are sending the IOCTL to is not our miniport because
        // only our miniport supports the custom "CPD_XXX" IOCTLS.  

        if (pBuffer != NULL)
        {
            fbe_release_nonpaged_pool(pBuffer);
        }
        if (IoCtl.param.pass_thru_buf != NULL)
        {
            fbe_release_nonpaged_pool(IoCtl.param.pass_thru_buf);
        }
    }

    return(ntStatus);

} /* end of function GetScsiConfig() */


/*******************************************************************************************

 Function:
    NTSTATUS ppfdSendCPDIoctlSetSystemDevices( DEVICE_OBJECT *pPortDeviceObject, 
                                          UINT_8E primary_phy_id, fbe_u64_t primary_exp_sas_addr,
                                          UINT_8E secondary_phy_id, fbe_u64_t secondary_exp_sas_addr )

 Description:
 
 Arguments:
    pPortDeviceObject - ScsiPort driver device object representing Backend 0 Scsiport
 
 Return Value:
    
  History:
    

/*******************************************************************************************/

EMCPAL_STATUS ppfdSendCPDIoctlSetSystemDevices( PEMCPAL_DEVICE_OBJECT pPortDeviceObject, 
                                          UINT_8E primary_phy_id, fbe_u64_t primary_exp_sas_addr,
                                          UINT_8E secondary_phy_id, fbe_u64_t secondary_exp_sas_addr )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    S_CPD_SET_SYSTEM_DEVICES IoCtl;

    if (pPortDeviceObject == NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR,TRC_K_START, "PPFD: ppfdSendCPDIoctlSetSystemDevices NULL input pointer!\n");
        return( EMCPAL_STATUS_INVALID_PARAMETER );
    }

    //
    //  Set up most of the CPD_IOCTL_HEADER structure by calling this function
    //  instead of using the following:
    //  IoCtl.ioctl_hdr = SrbIoControlTemplate;
    // 
    AsyncFillIoControlHdr(&IoCtl.ioctl_hdr );

    CPD_IOCTL_HEADER_SET_OPCODE(&IoCtl.ioctl_hdr, CPD_IOCTL_SET_SYSTEM_DEVICES);
    CPD_IOCTL_HEADER_SET_LEN(&IoCtl.ioctl_hdr, sizeof(CPD_SET_SYSTEM_DEVICES));

    IoCtl.param.u.sas.phy_of_primary = primary_phy_id;
    IoCtl.param.u.sas.sas_address_of_primary_exp = primary_exp_sas_addr;
    IoCtl.param.u.sas.phy_of_secondary = secondary_phy_id;
    IoCtl.param.u.sas.sas_address_of_secondary_exp = secondary_exp_sas_addr;
    
//    IoCtl.param.fmt_ver_major = CPD_CONFIG_FORMAT_VERSION;
    ntStatus = ppfdSendIoctl(pPortDeviceObject, 
                            &IoCtl, 
                            (ULONG)sizeof(IoCtl));

    if (!EMCPAL_IS_SUCCESS(ntStatus))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdSendCPDIoctlSetSystemDevices() failed IOCTL ntStatus=0x%x!\n", ntStatus );
    }

    return(ntStatus);

} /* end of function ppfdSendCPDIoctlSetSystemDevices() */



/***************************************************************************
 
 BOOLEAN GetMiniportBus(UCHAR *pBuffer, 
                            int BufferSize,
                            USHORT *pValue)

 Routine Description:
    Parses the supplied buffer for the characters B(x), and returns
    the (base 10) value of 'x' in *pValue

 Arguments:
    pBuffer - points to buffer to be parsed.
    BufferSize - specifies the size of the buffer.
    pValue - pointer to storage for Base 10 value of x, parsed from B(x)

 Return Value:

    TRUE if a value found, else FALSE.

 Revision History:

 ***************************************************************************/
#define ISADIGIT(c) (((c) >= '0') && ((c) <= '9')) 
#define DIGITVALUE(c) ((int)(c) - 0x30)
EMCPAL_LOCAL BOOLEAN GetMiniportBus(UCHAR *pBuffer, 
                            int BufferSize,
                            USHORT *pValue)
{
    int     Index;
    USHORT  Bus = 0;
    BOOLEAN LookingForB = TRUE;
    BOOLEAN ValueDetected = FALSE;

    if ((pBuffer == NULL) || (pValue == NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: GetMiniportBus() NULL ptr 0x%p 0x%p!\n", pBuffer, pValue);
        return(FALSE);
    }

    // Process the supplied buffer.

    for (Index = 0; Index < BufferSize; Index++) 
    {
        if (LookingForB && (pBuffer[Index] == 'B')) 
        {
            // We have encountered the 'B'.
            LookingForB = FALSE;
        }
        else if (!LookingForB) 
        {
            if (ISADIGIT(pBuffer[Index])) 
            {
                // Process this digit.
                ValueDetected = TRUE;
                Bus = (Bus * 10) + DIGITVALUE(pBuffer[Index]);
            }
            else if (pBuffer[Index] == ')') 
            {
                // We found the closing parenthesis.  Bail out.
                break;
            }
        }
    }

    // Fill in value for caller, only valid if ValueDetected is TRUE.
    // 
    *pValue = Bus;

    return (ValueDetected);
} /* end of function GetMiniportBus() */


/*******************************************************************************************

 Function:
    BOOL IsScsiPortBE0 ( PDEVICE_OBJECT ScsiPortDeviceObject, PBOOL pbIsBEZero )

 Description:
    Determine if the Scsiport device that is passed to this function is the Backend 0 Scsiport
    device object.

 Arguments:
    ScsiPortDeviceObject - Pointer to a Scsiport device Object
    pbIsBEZero - Points to BOOL value that is filled with TRUE or FALSE if success

 Return Value:
    TRUE -  Successfully retrieved the config info string from the miniport for the given 
            Scsiport object. In this case, *pbIsBEZero will be TRUE on return if it is 
            backend 0, or FALSE if not

    FALSE - Failed to retrieve the string 
            If failure, the value returned for pbIsBEZero should not be used

 Others:   

 Revision History:

 *******************************************************************************************/


BOOL IsScsiPortBE0 ( PEMCPAL_DEVICE_OBJECT ScsiPortDeviceObject, BOOL *pbIsBEZero )
{
    EMCPAL_STATUS    ntstatus;
    CPD_CONFIG *pConfigInfo = NULL;
    BOOL retVal = TRUE;
    USHORT      bus=0;

    
    if ((pbIsBEZero == NULL) || (ScsiPortDeviceObject == NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: IsScsiPortBE0 NULL ptr 0x%p 0x%p!\n",ScsiPortDeviceObject,pbIsBEZero );
        return(FALSE);
    }

    *pbIsBEZero = FALSE;

    // Note that we need to free the pass_thru_buf and pConfigInfo data which is allocated by GetScsiConfig() (per the GetScsiConfig() 
    // function header comments).  Free only if successful..and after we use the data obviously
    ntstatus = GetScsiConfig(ScsiPortDeviceObject, &pConfigInfo);

    if(EMCPAL_IS_SUCCESS(ntstatus) && (pConfigInfo != NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_START, "PPFD: IsScsiPortBE0 pConfigInfo->display_name %s\n",pConfigInfo->display_name );

        //
        // We got the config information successfully, now parse it to find the backend bus number
        // 
        if (GetMiniportBus((UCHAR *)pConfigInfo->pass_thru_buf,
                           pConfigInfo->pass_thru_buf_len, &bus))
        {
            // Found a backend bus, now check to see if it is bus 0
            if( bus == 0 )
            {
                PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: IsScsiPortBE0 found BE0!\n");
                *pbIsBEZero= TRUE;
            }
        }
        // Free the ConfigInfo and pass)thru_buff  now that we are done with it
        if (pConfigInfo != NULL)
        {
            if (pConfigInfo->pass_thru_buf != NULL)
            {
                fbe_release_nonpaged_pool(pConfigInfo->pass_thru_buf);
            }
            fbe_release_nonpaged_pool(pConfigInfo);
        }
    }
    else 
    {
        // This is expected if we send the CPD IOCTL to a Scsiport-miniport that is not our driver
        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_START, "PPFD: %s FAILED ntstatus = 0x%x\n  Info Ptr 0x%p", 
                        __FUNCTION__,ntstatus,pConfigInfo );
        retVal = FALSE;
    }

    return( retVal );
}



/*******************************************************************************************

 Function:
    NTSTATUS AttachToScsiPort( PDRIVER_OBJECT  PDriverObject,PDEVICE_OBJECT ppfdPortDeviceObject )

 Description:
    Scans for ScsiportX (where x is 0,1,2,3..) that represents Backend 0 (boot drives are on BE0) and attaches 
    via IoAttachDeviceToDeviceStack()

 Arguments:
    PDriverObject - PPFD Driver Object Pointer
    ppfdPortDeviceObject - The PPFD Port device Object created during driver entry

 Return Value:
    TRUE - Attached Successfully
    FALSE - Failed to attach

 Others:   

 Revision History:

*******************************************************************************************/

BOOL AttachToScsiPort( PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject,PEMCPAL_DEVICE_OBJECT ppfdPortDeviceObject )
{
    fbe_char_t                  deviceName[256];
    EMCPAL_STATUS               ntStatus = EMCPAL_STATUS_SUCCESS;
    BOOL                        bSearchingForBE0 = TRUE;
    ULONG                       scsiPortNumber = 0;
    ULONG                       currentFullScanCount = 0;
    BOOL                        bIsPortBE0= FALSE;
    BOOL                        bStatus = FALSE;
    PEMCPAL_DEVICE_OBJECT       ScsiPortDeviceObject=NULL;
    PEMCPAL_FILE_OBJECT         ScsiPortFileObject=NULL;
    PPFD_DEVICE_EXTENSION       *ppfdDevExt;
    

    if ((PDriverObject == NULL) || (ppfdPortDeviceObject == NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: AttachToScsiPort NULL ptr 0x%p 0x%p!\n",PDriverObject,ppfdPortDeviceObject );
        return(FALSE);
    }

    ppfdDevExt =(PPFD_DEVICE_EXTENSION *)EmcpalDeviceGetExtension(ppfdPortDeviceObject);

    // Scan for SCSIport devices
    //
    // Find the SCSI port that represents backend 0, then save the Scsi port device object pointer 
    // into the device extension so it can be used in dispatch function later
    //
    
    do
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_OPT, TRC_K_START, "PPFD: Search for ScsiPorts\n" );

        ntStatus = csx_p_snprintf ( deviceName, sizeof(deviceName), "\\Device\\ScsiPort%d", scsiPortNumber );
        if (!EMCPAL_IS_SUCCESS(ntStatus))
        {
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: Error creating ScsiPort device string - exiting!\n" );
            break;
        }

        // We could keep track of both the SCSI port file and device objects in order to 
        // appropriately detach the PPFD from the device objects and dereference the
        // file objects when PPFD is unloaded.  However, the PPFD is only
        // ever going to be unloaded when the system is rebooted so it's
        // not really an issue.

        ntStatus = EmcpalExtDeviceOpen(deviceName,
                                        FILE_READ_ATTRIBUTES,
                                        &ScsiPortFileObject,
                                        &ScsiPortDeviceObject);

        //
        // If we fail to get the device object pointer (status!= SUCCESS), then there are no more
        // SCSIPort devices and we will break out of the loop since status indicates failure
        //
        if (EMCPAL_IS_SUCCESS(ntStatus))
        {
            PPFD_TRACE( PPFD_TRACE_FLAG_OPT, TRC_K_START, "PPFD: Found SCSI port: 0x%p, %s\n", ScsiPortDeviceObject, deviceName );

            //
            // We want to attach to BE0 only, where the vault drives are connected.
            // Determine if this port represents BE0 and attach if it does.
            //
            bStatus = IsScsiPortBE0( ScsiPortDeviceObject, &bIsPortBE0 );

            if (bStatus)
            {
                if( bIsPortBE0 )
                {
                    bSearchingForBE0 = FALSE;       // break out of SCSIPort search loop

                    // Update the scsi port number in our PPFD disk device objects map
                    ppfdPhysicalPackageInitScsiPortNumberInMap( (UCHAR)scsiPortNumber );
#ifndef C4_INTEGRATED
            
                    // Attach the PPFD port device object that was passed in to the BE0 SCSIPort
                    // device object

                    ScsiPortDeviceObject = EmcpalExtDeviceAttachCreate(ppfdPortDeviceObject,
                                                ScsiPortDeviceObject);
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */

                    // Save the BE0 ScsiPort Device Object in the device extension data..it will be used in dispatch
                    // function to pass on the IRP's to the ScsiPort device.
                    //
                    ppfdDevExt->BE0ScsiPortDeviceObject = ScsiPortDeviceObject;
                    // Save it in the PPFD global data structure as well
                    PPFD_GDS.Be0ScsiPortObject = ScsiPortDeviceObject;

                    if (ScsiPortDeviceObject == NULL)
                    {
                        // Failed to attach to the device.
                        //

                        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: Attach PPFD to SCSI port %s FAILED\n", deviceName );
                    
                        ntStatus = EMCPAL_STATUS_NO_SUCH_DEVICE;
                        break;
                    }

                    PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: Attached PPFD to SCSI port:%s\n", deviceName );
                    bStatus = TRUE;
                
                }// end if( bIsPortBE0 )
            }// end if( bStatus )
            // Try the next SCSI port.
            scsiPortNumber ++;
        } //end if(NT_SUCCESS(ntStatus))
        else
        {
            // We got to the last ScsiPort (or did not find any) without finding the port 
            // that represents BE0.  So, return an error status.
            //
            if (bSearchingForBE0){
                currentFullScanCount++;
                PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: BE0 not found. Rescan Count %d. Max port scanned %d\n", currentFullScanCount,scsiPortNumber); 
                //Retry a few times to allow miniport to initialize.
                if (currentFullScanCount < PPFD_MAX_FULL_PORT_SCAN_ON_INIT){
                    scsiPortNumber = 0;
                    ntStatus = EMCPAL_STATUS_SUCCESS;
                }else{
                    //No point in proceeding further if we cannot find BE0.
                    panic( PPFD_FAILED_PHYSICALPACKAGE_INIT, scsiPortNumber );
                }
            }
        }
    }  while ( EMCPAL_IS_SUCCESS(ntStatus) && (bSearchingForBE0) );

    return ( bStatus );
}
// .End AttachToScsiPorts


/*******************************************************************************************

 Function:
    BACKEND_TYPE ppfdDetectBackendType( void )

 Description:
    Determine what type of SLIC is inserted in slot 0.  Uses specl function to determine this.
    This function will cache the type in the PPFD global data structure so that it will only have to call
    specl one time to get the data.  Subsequent calls to this function will return the cached value.

 Arguments: 
    None

 Return Value:
    FC_BACKEND
    SAS_BACKEND
    UNSUPPORTED_BACKEND

 Revision History:

*******************************************************************************************/
BACKEND_TYPE ppfdDetectBackendType( void ) 
{
    SPECL_PCI_DATA          pciData;
    IO_CONTROLLER_PROTOCOL  bootProtocol;
    BACKEND_TYPE            beType;

    // If we already detected it, just use the type we already have rather than calling Specl
    if( PPFD_GDS.beType != UNSUPPORTED_BACKEND )
    {
        return( PPFD_GDS.beType );
    }

    //
    // Get the boot protocol from SPECL
    //
    speclGetBootDeviceInfo( &pciData,
                            &bootProtocol);

    switch( bootProtocol )
    {
        case IO_CONTROLLER_PROTOCOL_SAS:
            beType = SAS_BACKEND;
            break;

        case IO_CONTROLLER_PROTOCOL_FIBRE:
            beType = FC_BACKEND;
            break;

        default:
            // Don't ktrace here, let the caller ktrace
            beType = UNSUPPORTED_BACKEND;
            break;
    }

    // Cache the detected type in our global data stucture
    PPFD_GDS.beType = beType;
    return( beType );
}

/*******************************************************************************************

 Function:
    void CompleteRequest(IN PIRP Irp, IN NTSTATUS status, ULONG  info)

 Description:
    This just a "wrapper" for calling IoCompleteRequest(.  Fill in the IoStatus data with the 
    status and info that is passed in, then call IoCompleteRequest() to complete the IRP.
    
 Arguments:
    IRP - Pointer to the IRP to complete
    status - The NTSTATUS to return in IoStatus.Status
    info - Info to return in IoStatus.Information

 Return Value:
    None      

 Revision History:

*******************************************************************************************/
void CompleteRequest(IN PEMCPAL_IRP Irp, IN EMCPAL_STATUS status, ULONG info)
{

    if ( Irp == NULL )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: CompleteRequest NULL ptr 0x%p!\n",Irp );
        return;
    }
    EmcpalExtIrpStatusFieldSet(Irp, status);
    EmcpalExtIrpInformationFieldSet(Irp, info);
    EmcpalIrpCompleteRequest(Irp);
}                       

/*******************************************************************************************
    NTSTATUS ppfdForwardPowerIRP( PDEVICE_OBJECT fido, PIRP Irp )
   
 Description:
    IRP_MJ_POWER requires special handling to forward. This function handles the special requirements
    for forwarding the power IRP. Forwards the Irp (passed in) to the device object that the fido (passed in) 
    is attached to.
    
 Arguments:
    fido - The PPFD Filter Device Object
    Irp - Irp to Forward
 
 Return Value:
    NTSTATUS

 Revision History:

*******************************************************************************************/

EMCPAL_STATUS ppfdForwardPowerIrp( PEMCPAL_DEVICE_OBJECT fido, PEMCPAL_IRP Irp )
{
    PPPFD_DEVICE_EXTENSION pdx = NULL;
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;

    if ((fido == NULL) || (Irp == NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdForwardPowerIrp NULL ptr 0x%p 0x%p!\n",fido,Irp );
        return ( EMCPAL_STATUS_INVALID_PARAMETER );
    }
    pdx = (PPPFD_DEVICE_EXTENSION) EmcpalDeviceGetExtension(fido);

    //
    // Pass on the IRP to the BE0 Scsiport Device Object
    //
     
    if( pdx->BE0ScsiPortDeviceObject )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdForwardPowerIRP Calling BE0ScsiPortDeviceObject\n" );
        EmcpalExtIrpStartPoNext(Irp);
        EmcpalIrpCopyCurrentIrpStackLocationToNext(Irp);
        ntStatus = EmcpalExtIrpSendPoAsync(pdx->BE0ScsiPortDeviceObject, Irp);
    }
    else
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdForwardPowerIRP [ERROR]BE0ScsiPortDeviceObject is not initialized!\n" );
    }
    return( ntStatus );
}

/*******************************************************************************************

 Function:
    NTSTATUS ppfdForwardIRP( PDEVICE_OBJECT fido, PIRP Irp )
   

 Description:
    Forwards the Irp (passed in) to the device object that the fido (passed in) is attached to.
    The fido that is passed to us will be one of 3 possible objects:
        
        - PPFDPort0 device object (used by ASIDC)
        - Non PnP PPFD Disk Device Object (created during DriverEntry() for ASIDC)
        - PnP PPFD Disk Device Object (created in ppfdAddDevice(), used by NtMirror)

 Arguments:
    fido - The PPFD Filter Device Object
    Irp - Irp to Forward
 
 Return Value:
    NTSTATUS STATUS_SUCCESS or error status

 Revision History:

*******************************************************************************************/
EMCPAL_STATUS ppfdForwardIRP( PEMCPAL_DEVICE_OBJECT fido, PEMCPAL_IRP Irp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    PPPFD_DEVICE_EXTENSION pdx = NULL;
    PPPFD_DISK_DEVICE_EXTENSION pddx = NULL;


    if ((fido == NULL) || (Irp == NULL))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdForwardIrp NULL ptr 0x%p 0x%p!\n",fido,Irp );
        return ( EMCPAL_STATUS_INVALID_PARAMETER );
    }
    pdx = (PPPFD_DEVICE_EXTENSION) EmcpalDeviceGetExtension(fido);
    pddx = (PPPFD_DISK_DEVICE_EXTENSION) EmcpalDeviceGetExtension(fido);

    //
    // If the fido is a disk device object (either pnp or non pnp), then forward to the targetDiskDeviceObject
    // that this fido is attached to
    //
    if( (ppfdIsPnPDiskDeviceObject(  fido  )) || (ppfdIsDiskDeviceObject( fido )))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdForwardIRP IRP Target is a PnP Disk Device Object\n" );

        // This is a disk device object, forward the Irp to the targetDiskDeviceObject 
        //
        EmcpalIrpSkipCurrentIrpStackLocation(Irp);
        if( pddx->targetDiskDeviceObject )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdForwardIRP Calling PNP Disk Device Object\n" );
            ntStatus =(EmcpalExtIrpSendAsync( pddx->targetDiskDeviceObject,Irp));
        }
    }
  
    else
    {
        //
        // Pass on the IRP to the BE0 Scsiport Device Object
        //
 
        EmcpalIrpSkipCurrentIrpStackLocation(Irp);
        
        if( pdx->BE0ScsiPortDeviceObject )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdForwardIRP Calling BE0ScsiPortDeviceObject\n" );
            ntStatus = EmcpalExtIrpSendAsync(pdx->BE0ScsiPortDeviceObject, Irp);
        }
        else
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: [ERROR]BE0ScsiPortDeviceObject is not initialized!\n" );
        }
    }
    return( ntStatus );       
}

/*******************************************************************************************

 Function:
    NTSTATUS ppfdDispatchFCBackend( IN PDEVICE_OBJECT fido, IN PIRP Irp )   

 Description:
    This function just forwards the IRP to the underlying backend 0 Scisport. It will be
    called only when there is a Fibre Channel backend 0.

 Arguments:
    PDEVICE_OBJECT fido - The device object pointer that the IRP is being sent to
    PIRP Irp - Pointer to the IRP being dispatched

 Return Value:
    NTSTATUS

 Revision History:

*******************************************************************************************/
EMCPAL_STATUS ppfdDispatchFCBackend( IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp )
{
    PEMCPAL_IO_STACK_LOCATION stack = EmcpalIrpGetCurrentStackLocation(Irp);
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    UCHAR type = EmcpalExtIrpStackMajorFunctionGet(stack);

    //
    // The power Irp is sent during shutdown..needs to be handled differently when forwarding 
    //
    if( type == EMCPAL_IRP_TYPE_CODE_POWER )
    {
        ntStatus = ppfdForwardPowerIrp( fido, Irp );
    }
    else
    {
        ntStatus = ppfdForwardIRP( fido, Irp );    
    }
  
    return ( ntStatus );
}

/*******************************************************************************************
 Function:
    NTSTATUS DispatchAny( IN PDEVICE_OBJECT fido, IN PIRP Irp )   

 Description:
     Main dispatch entry for ALL Irp's.  The Irp will be further dispatched to either the Fibre Channel
     Irp dispatch handler or the PhysicalPackage Irp dispatch handler(for SAS and SATA backend).

 Arguments:
    PDEVICE_OBJECT fido - The device object pointer that the IRP is being sent to
    PIRP Irp - Pointer to the IRP being dispatched

 Return Value:
    NTSTATUS

 Revision History:

*******************************************************************************************/

EMCPAL_STATUS DispatchAny(IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp)
{                           // DispatchAny
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION stack = EmcpalIrpGetCurrentStackLocation(Irp);
    
    // This array of strings is just used so we can dump the name of the IRP to ktrace/debugger
    static char* irpname[] = {
        "IRP_MJ_CREATE",
        "IRP_MJ_CREATE_NAMED_PIPE",
        "IRP_MJ_CLOSE",
        "IRP_MJ_READ",
        "IRP_MJ_WRITE",
        "IRP_MJ_QUERY_INFORMATION",
        "IRP_MJ_SET_INFORMATION",
        "IRP_MJ_QUERY_EA",
        "IRP_MJ_SET_EA",
        "IRP_MJ_FLUSH_BUFFERS",
        "IRP_MJ_QUERY_VOLUME_INFORMATION",
        "IRP_MJ_SET_VOLUME_INFORMATION",
        "IRP_MJ_DIRECTORY_CONTROL",
        "IRP_MJ_FILE_SYSTEM_CONTROL",
        "IRP_MJ_DEVICE_CONTROL",
        "IRP_MJ_SCSI",       // aka IRP_MJ_INTERNAL_DEVICE_CONTROL
        "IRP_MJ_SHUTDOWN",
        "IRP_MJ_LOCK_CONTROL",
        "IRP_MJ_CLEANUP",
        "IRP_MJ_CREATE_MAILSLOT",
        "IRP_MJ_QUERY_SECURITY",
        "IRP_MJ_SET_SECURITY",
        "IRP_MJ_POWER",
        "IRP_MJ_SYSTEM_CONTROL",
        "IRP_MJ_DEVICE_CHANGE",
        "IRP_MJ_QUERY_QUOTA",
        "IRP_MJ_SET_QUOTA",
        "IRP_MJ_PNP",
        };

    UCHAR type = EmcpalExtIrpStackMajorFunctionGet(stack);

    if (type >= arraysize(irpname))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: Unknown IRP, major type0x%x\n", type);
    }
    else
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: IRP = %s, 0x%x\n", irpname[type], type);
    }
    
    if( ppfdDetectBackendType() == FC_BACKEND )
    {
        ntStatus = ppfdDispatchFCBackend( fido, Irp );
    }
    else 
    {
        //
        // For SAS and Sata backend..go through the PhysicalPackage
        //
        ntStatus = ppfdDispatchPhysicalPackage( fido, Irp);
    }
  
    return ntStatus;
}               


// End ppfdDriverEntry.c

