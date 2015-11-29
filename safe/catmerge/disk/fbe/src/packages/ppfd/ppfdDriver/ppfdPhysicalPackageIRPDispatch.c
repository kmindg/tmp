/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2010
 All rights reserved.
 Licensed material - property of EMC Corporation.


 File Name:
      ppfdPhysicalPackageIRPDispatch.c

 Contents:
      The main IRP dispatch function when booting from a SAS or SATA backend (i.e. PhysicalPackage 
      is used for I/O).  The code in this file does NOT interface directly to the PhysicalPackage driver, but
      will call "ppfdPhysicalPackageXXX" functions (isolated in the file ppfdPhydicalPackageInterface.c) to
      interface with the PhysicalPackage driver. 


 Exported:
        ppfdDispatchPhysicalPackage( IN PDEVICE_OBJECT fido, IN PIRP Irp )

 Revision History:

***************************************************************************/

//
// Interface
//
#include "k10ntddk.h"
#include "storport.h" 
#include "scsiclass.h"          // for IOCTL_STORAGE_QUERY_PROPERTY define
//#include "ntddscsi.h"           // Scsi IOCTL defines
#include "cpd_interface.h"      // cpd IOCTL defines

#include "ppfdDeviceExtension.h"
#include "ppfdKtraceProto.h"
#include "ppfdMisc.h"
#include "ppfd_panic.h"



// ASIDC is using this define for pass thru buffer length..
// I'll just define a local value here to use for now..should be the same value as the ASIDC value
//#define ASIDC_CPD_CONFIG_PASSTHRU_LEN (32)
#define CPD_CONFIG_PASSTHRU_LEN (32)

// Prototypes..move later to a header file
EMCPAL_STATUS ppfdSendReadRequestToPP(  IN PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB, PEMCPAL_IRP Irp );
EMCPAL_STATUS ppfdSendWriteRequestToPP(  IN PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB, PEMCPAL_IRP Irp );
BOOL ppfdClaimDevice( IN PEMCPAL_DEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB );
BOOL ppfdReleaseDevice( IN PEMCPAL_DEVICE_OBJECT fido,  PCPD_SCSI_REQ_BLK pSRB );
BOOL ppfdCPDIoctlAddDevice(S_CPD_PORT_LOGIN *pCPDLoginIoctl );
BOOL ppfdCPDIoctlRegisterCallbacks(IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp, S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl );
EMCPAL_STATUS ppfdDispatchDiskDevice( IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp );
BOOL ppfdSRBExecuteScsi( IN PEMCPAL_DEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB );

void ppfdMapNtStatusToSrbStatus( EMCPAL_STATUS ntStatus, UCHAR *SrbStatus_p );

//
// Functions imported from the PPFD PhysicalPackage interface file
//
BOOL ppfdPhysicalPackageGetNumDisks( DWORD portNumber, DWORD *pNumDisks );
BOOL ppfdPhysicalPackageClaimDevice( UCHAR PathID, UCHAR TargetID, PEMCPAL_DEVICE_OBJECT *ppfdFiDO );
EMCPAL_STATUS ppfdPhysicalPackageSendRead( ULONG StartLBA, ULONG Length, PSGL pSGL, PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PEMCPAL_IRP Irp );
EMCPAL_STATUS ppfdPhysicalPackageSendWrite( ULONG StartLBA, ULONG Length, PSGL pSGL, PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject,  PEMCPAL_IRP Irp );
BOOL ppfdPhysicalPackageAddPPFDFidoToMap( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, UCHAR DiskNum, BOOL bPnPDisk );
BOOL ppfdPhysicalPackageGetPPFDFidoFromMap( PEMCPAL_DEVICE_OBJECT *ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB ); 
BOOL ppfdPhysicalPackageRegisterCallback(S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl);
BOOL ppfdPhysicalPackageDeRegisterCallback( S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl);
BOOL ppfdPhysicalPackageGetReadCapacity( PEMCPAL_DEVICE_OBJECT fido, READ_CAPACITY_DATA *pReadCapacity );
BOOL ppfdPhysicalPackageWaitForDriveReady( S_CPD_PORT_LOGIN *pCPDLoginIoctl );

//
// Other imported functions and global variables
//
EMCPAL_STATUS ppfdForwardIRP( PEMCPAL_DEVICE_OBJECT fido, PEMCPAL_IRP Irp );
BOOL ppfdIsDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject );
BOOL ppfdIsPnPDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject);
void CompleteRequest( PEMCPAL_IRP Irp, EMCPAL_STATUS status, ULONG info);
BOOL ppfdPhysicalPackageGetScsiAddress( PEMCPAL_DEVICE_OBJECT fido, SCSI_ADDRESS *pScsiAddress );
BOOL ppfdIsPnPDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject );
EMCPAL_STATUS ppfdForwardPowerIrp( PEMCPAL_DEVICE_OBJECT fido, PEMCPAL_IRP Irp );


/*******************************************************************************************

 Function:
    NTSTATUS ppfdDispatchPhysicalPackage( IN PDEVICE_OBJECT fido, IN PIRP Irp )

 Description:
    Main dispatch handler for all IRP's when PhysicalPackage is being used for I/O (i.e. when there is a
    SAS or SATA backend).  ASIDC and Ntmirror use various IRPs/IOCTL's to interact with the Scsiport device.
    This PPFD dispatch handler will process these IRP's/IOCTL's by getting the information 
    from the PhysicalPackage, or "hard code" the data in some cases if the PhysicalPackage does not 
    provide/export that information.  PPFD will fill in the required IRP/IOCTL data structures to satisfy 
    ASIDC and Ntmirror.  In some cases, the IRP may be passed on to the underlying Scisport/miniport which will
    handle the processing of the IRP.

 Arguments:
    IN PDEVICE_OBJECT fido
    IN PIRP Irp

 Return Value:
    STATUS_SUCCESS

 Others:   

 Revision History:

 *******************************************************************************************/

EMCPAL_STATUS ppfdDispatchPhysicalPackage( IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp )
{
    PPPFD_DEVICE_EXTENSION pdx = NULL;
    PPPFD_DISK_DEVICE_EXTENSION pddx = NULL;
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    ULONG IoctlCode;

    PEMCPAL_IO_STACK_LOCATION stack = NULL;
    PCPD_SCSI_REQ_BLK pSRB = NULL;
    UCHAR type;
    UCHAR SrbFunctionCode;

    S_CPD_CONFIG *pCPDIoctl;
    S_CPD_PORT_LOGIN *pCPDLoginIoctl;

    if ((fido == NULL) || (Irp == NULL)){
        panic(PPFD_IO_INVALID_PARAMETER,(UINT_PTR)fido);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    // If this IRP is directed towards a PPFD disk device object, then dispatch to the "disk device" handler.
    // Otherwise, it is directed towards the port device object (PPFDPort0)..fall through and handle it
    //
    if( ppfdIsDiskDeviceObject( fido ) || ppfdIsPnPDiskDeviceObject( fido ) )
    {
        ntStatus = ppfdDispatchDiskDevice( fido, Irp );
        return ntStatus;
    }

    pdx = (PPPFD_DEVICE_EXTENSION) EmcpalDeviceGetExtension(fido);
    pddx = (PPPFD_DISK_DEVICE_EXTENSION) EmcpalDeviceGetExtension(fido);

    stack = EmcpalIrpGetCurrentStackLocation(Irp);
    pSRB = EmcpalExtIrpStackParamScsiSrbGet(stack);
    type = EmcpalExtIrpStackMajorFunctionGet(stack);

    //
    // The IRP is directed towards the PPFD port device object
    // 
    switch ( type )
    {
        case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
            //
            // ASIDC sends a number of IOCTLS to get various pieces of information
            // We need to get that information from the PhysicalPackage driver, or in some cases just "hard code"
            // the information, and return it via the IOCTL buffer
            //
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IRP_MJ_DEVICE_CONTROL\n" );
            IoctlCode =EmcpalExtIrpStackParamIoctlCodeGet(stack);
            pCPDIoctl = (S_CPD_CONFIG*)EmcpalExtIrpSystemBufferGet(Irp);

            if ((IoctlCode == IOCTL_SCSI_MINIPORT) && (pCPDIoctl != NULL))
            {
                switch(CPD_IOCTL_HEADER_GET_OPCODE(&pCPDIoctl->ioctl_hdr))
                {
                     case CPD_IOCTL_REGISTER_CALLBACKS:

                          PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage CPD_IOCTL_REGISTER_CALLBACKS\n" );

                          //Completion of IO or forwarding of the IO happens in ppfdCPDIoctlRegisterCallbacks.

                          ppfdCPDIoctlRegisterCallbacks(fido, Irp, (S_CPD_REGISTER_CALLBACKS*)pCPDIoctl); 

                        break;

                      case CPD_IOCTL_ADD_DEVICE:

                        // This is sent when ASIDC or NtMirror "logs in" the drive

                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage CPD_IOCTL_ADD_DEVICE\n" );

                        // The PP handles "login" for the drives.  We'll just report back
                        // that the drive is already logged in.
                        // Wait for the disk to be ready before reporting that the drive is logged in. 
                        // PPFD will get a callback (because we registerd for callbacks on disk events)
                        // that will result in the disk object ID being updated in the map when it is ready
                        // So we will just wait for the disk object ID to be valid and fail the login if it never becomes
                        // valid (after a timeout period). 

                        pCPDLoginIoctl = (S_CPD_PORT_LOGIN*)EmcpalExtIrpSystemBufferGet(Irp);
                        if( ppfdPhysicalPackageWaitForDriveReady( pCPDLoginIoctl ) )
                        {
                            CPD_IOCTL_HEADER_SET_RETURN_CODE((&pCPDIoctl->ioctl_hdr), CPD_RC_DEVICE_ALREADY_LOGGED_IN);
                        }
                        else
                        {
                            CPD_IOCTL_HEADER_SET_RETURN_CODE(&(pCPDIoctl->ioctl_hdr), CPD_RC_DEVICE_NOT_READY);
                        }

                        // Have to return a success status because the Return Code is not returned on failures. 
                        // The client will recieve STATUS_SUCCESS and will have to check the CPD Return code
                        // to determine if the IOCTL succeeded or failed.

                        ntStatus = EMCPAL_STATUS_SUCCESS;
                        CompleteRequest( Irp, ntStatus, sizeof(S_CPD_PORT_LOGIN) );

                        break;

                     default:

                         PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IRP_MJ_DEVICE_CONTROL IOCTL= 0x%x!\n",IoctlCode  );
                         ntStatus = ppfdForwardIRP( fido, Irp );
                         break;

                  }//End switch
            }            
            else
            {
                PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IRP_MJ_DEVICE_CONTROL IOCTL= 0x%x!\n",IoctlCode  );
                ntStatus = ppfdForwardIRP( fido, Irp );
            }

            break;
        //end  IRP_MJ_DEVICE_CONTROL

        case EMCPAL_IRP_TYPE_CODE_SCSI:
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IRP_MJ_SCSI\n" );
            IoctlCode =EmcpalExtIrpStackParamIoctlCodeGet(stack);
            // Handle the specific IOCTL
            switch ( IoctlCode )
            {
                case IOCTL_SCSI_MINIPORT:
                    SrbFunctionCode =  EmcpalExtIrpStackParamScsiSrbGetFunction(stack);
                    pCPDIoctl = (S_CPD_CONFIG*)EmcpalExtIrpStackParamScsiSrbGetDataBuffer(stack);

                    if ((SrbFunctionCode == SRB_FUNCTION_IO_CONTROL) &&
                        (pCPDIoctl != NULL) &&
                        (CPD_IOCTL_HEADER_GET_OPCODE(&pCPDIoctl->ioctl_hdr) == CPD_IOCTL_ADD_DEVICE)){
                        // This is sent when ASIDC or NtMirror "logs in" the drive
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage CPD_IOCTL_ADD_DEVICE\n" );
                        
                        // The PP handles "login" for the drives.  We'll just report back
                        // that the drive is already logged in.
                        
                        // Wait for the disk to be ready before reporting that the drive is logged in. 
                        // PPFD will get a callback (because we registerd for callbacks on disk events)
                        // that will result in the disk object ID being updated in the map when it is ready
                        // So we will just wait for the disk object ID to be valid and fail the login if it never becomes
                        // valid (after a timeout period). 
                        pCPDLoginIoctl = (S_CPD_PORT_LOGIN*)EmcpalExtIrpStackParamScsiSrbGetDataBuffer(stack);
                        if( ppfdPhysicalPackageWaitForDriveReady( pCPDLoginIoctl ) )
                        {
                            CPD_IOCTL_HEADER_SET_RETURN_CODE(&(pCPDIoctl->ioctl_hdr), CPD_RC_DEVICE_ALREADY_LOGGED_IN);
                        }
                        else
                        {
                            CPD_IOCTL_HEADER_SET_RETURN_CODE(&(pCPDIoctl->ioctl_hdr), CPD_RC_DEVICE_NOT_READY);
                        }
                        
                        // Have to return a success status because the Return Code is not returned on failures. 
                        // The client will recieve STATUS_SUCCESS and will have to check the CPD Return code
                        // to determine if the IOCTL succeeded or failed.
                        ntStatus = EMCPAL_STATUS_SUCCESS;
                        CompleteRequest( Irp, ntStatus, sizeof(S_CPD_PORT_LOGIN) );
                     }else{
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage Unsupported IOCTL_SCSI_MINIPORT SrbFn= 0x%x!\n",SrbFunctionCode  );
                        ntStatus = ppfdForwardIRP( fido, Irp );
                     }
                     break; //case IOCTL_SCSI_MINIPORT:

                case IOCTL_SCSI_EXECUTE_NONE:
                    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IOCTL_SCSI_EXECUTE_NONE\n" );
                    SrbFunctionCode =  EmcpalExtIrpStackParamScsiSrbGetFunction(stack);

                    switch ( SrbFunctionCode )
                    {
                        // Physicalpackage does not have an interface to "claim" devices
                        // We will support it within PPFD
                        //
                        case SRB_FUNCTION_CLAIM_DEVICE:
                            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage SRB_FUNCTION_CLAIM_DEVICE\n" );
                            if (ppfdClaimDevice( fido, pSRB )){
                                ntStatus = EMCPAL_STATUS_SUCCESS;
                            }else{
                                ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
                                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s ppfdClaimDevice returned error.\n",
                                                __FUNCTION__);
                            }
                            CompleteRequest( Irp, ntStatus, 0 );
                            break;

                        case SRB_FUNCTION_RELEASE_DEVICE:
                            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage SRB_FUNCTION_RELEASE_DEVICE\n" );
                            if (ppfdReleaseDevice( fido, pSRB )){
                                ntStatus = EMCPAL_STATUS_SUCCESS;
                            }else{
                                ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
                                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s ppfdReleaseDevice returned error.\n",
                                                __FUNCTION__);
                            }
                            CompleteRequest( Irp, ntStatus, 0 );
                            break;
                        default:
                            PPFD_TRACE(PPFD_TRACE_FLAG_WARN, TRC_K_STD, "PPFD: DispatchPhysicalPackage Unsupported IOCTL_SCSI_EXECUTE_NONE FnCode: 0x%x!\n",SrbFunctionCode );
                            // Just forward the request
                            ntStatus = ppfdForwardIRP(fido, Irp); 
                            break;
                    }
                    break;//case IOCTL_SCSI_EXECUTE_NONE

                default:
                    PPFD_TRACE(PPFD_TRACE_FLAG_WARN, TRC_K_STD, "PPFD: DispatchPhysicalPackage Unsupported IOCTL 0x%x!\n",IoctlCode );
                    // Just forward the request
                    ntStatus = ppfdForwardIRP(fido, Irp); 
                    break;
            }
            break; //case IRP_MJ_SCSI

        case EMCPAL_IRP_TYPE_CODE_POWER:
            // Need to handle this IRP differently than others when forwarding it
            ntStatus = ppfdForwardPowerIrp( fido, Irp );
            break;
        
        case EMCPAL_IRP_TYPE_CODE_PNP:
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchPhysicalPackage IRP_MJ_PNP\n" );
            // Just complete the PNP IRP here
            CompleteRequest( Irp, EMCPAL_STATUS_SUCCESS, 0 );
            break;

        case EMCPAL_IRP_TYPE_CODE_CLEANUP:
            // Just complete the cleanup IRP
            CompleteRequest( Irp, EMCPAL_STATUS_SUCCESS, 0 );
            break;

        default:
            PPFD_TRACE(PPFD_TRACE_FLAG_WARN, TRC_K_STD, "PPFD: DispatchPhysicalPackage Unsupported IRP= 0x%x!\n", type );
            // Just forward the request
            ntStatus = ppfdForwardIRP(fido, Irp); 
            break;
    } // end switch( type )
    return ntStatus;
} //.end DispatchPhysicalPackage


/*******************************************************************************************

 Function:
    NTSTATUS ppfdDispatchDiskDevice( IN PDEVICE_OBJECT fido, IN PIRP Irp ) )

 Description:
   
   All IRP's being sent to one of the "PPFD Disk Device Objects" (aka PPFD fido = Filter Device Object)
   will be handled through this dispatch function.  In some cases, the IRP will require PhysicalPackage to
   complete processing (for I/O).  Both ASIDC and NtMirror send IRP's that are directed to a PPFD disk device.
   The NtMirror IRP's are directed to one of the "pnp" PPFD disk devices.

 Arguments:
    fido - Pointer to the PPFD disk device object (FiDO) that the IRP is being sent to.
           The disk device object can be either the "non pnp" PPFD disk device object created by PPFD during
           DriverEntry (ASIDC uses these by "claiming" them), or the PnP PPFD disk device object that gets created 
           in the "ppfdAddDevice" function (for Ntmirror).
 
    Irp - Pointer to the IRP that is being sent to the disk device

 Return Value:
    NTSTATUS

 Others:   

 Revision History:

 *******************************************************************************************/
EMCPAL_STATUS ppfdDispatchDiskDevice( IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;
    PEMCPAL_IO_STACK_LOCATION stack = EmcpalIrpGetCurrentStackLocation(Irp);
    UCHAR type = EmcpalExtIrpStackMajorFunctionGet(stack);
    ULONG IoctlCode;
    UCHAR SrbFunctionCode;
    PCPD_SCSI_REQ_BLK pSRB = EmcpalExtIrpStackParamScsiSrbGet(stack);    
    SCSI_ADDRESS ScsiAddress;   
    ULONG   Information = 0;



    switch ( type )
    {
        case EMCPAL_IRP_TYPE_CODE_SCSI:
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IRP_MJ_SCSI\n" );
            IoctlCode =EmcpalExtIrpStackParamIoctlCodeGet(stack);
            SrbFunctionCode =  EmcpalExtIrpStackParamScsiSrbGetFunction(stack);

            // Handle the specific IOCTL
            switch ( IoctlCode )
            {
                case IOCTL_SCSI_EXECUTE_NONE:
                    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IOCTL_SCSI_EXECUTE_NONE\n" );
                   
                    if (SrbFunctionCode == SRB_FUNCTION_CLAIM_DEVICE){
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice SRB_FUNCTION_CLAIM_DEVICE\n" );
                        if (ppfdClaimDevice( fido, pSRB )){
                            ntStatus = EMCPAL_STATUS_SUCCESS;
                        }else{
                            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
                            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s ppfdClaimDevice returned error.\n",
                                            __FUNCTION__);
                        }
                        CompleteRequest( Irp, ntStatus, 0 );
                    }else if (cpd_os_io_get_cdb(pSRB)[0] == SCSIOP_TEST_UNIT_READY){
                        // Could get this for TUR command..6 byte CDB from ASIDc
                        // Check the command in the CDB for SCSIOP_TEST_UNIT_READY
                        // This gets sent by NtMirror when about to do a rebuild of the drive (after drive
                        // removal/insertion for example)

                        // Just return success
                        cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
                        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice SCSIOP_TEST_UNIT_READY\n" );
                        CompleteRequest( Irp, EMCPAL_STATUS_SUCCESS, 0 );

                    }else{
                        // Fail the request and trace this.
                        cpd_os_io_set_status(pSRB, SRB_STATUS_INVALID_REQUEST);
                        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice unsupported Cdb: 0x%x\n",cpd_os_io_get_cdb(pSRB)[0] );
                        CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                    }
                    break;

                case IOCTL_SCSI_EXECUTE_IN:
                    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IOCTL_SCSI_EXECUTE_IN\n" );

                    if (SrbFunctionCode == SRB_FUNCTION_EXECUTE_SCSI){
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice SRB_FUNCTION_EXECUTE_SCSI\n" );
                        ppfdSRBExecuteScsi( fido, pSRB );
                        CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                    }else{
                        // Fail the request and trace this.
                        cpd_os_io_set_status(pSRB, SRB_STATUS_INVALID_REQUEST);
                        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice unsupported Srb fn code: 0x%x\n",SrbFunctionCode);
                        CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                    }
                    break;

                case IOCTL_SCSI_EXECUTE_OUT:
                    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IOCTL_SCSI_EXECUTE_OUT\n" );

                    if (cpd_os_io_get_cdb(pSRB)[0] == SCSIOP_REASSIGN_BLOCKS)
                    {
                        /* We can get this request incase of sync remap request,
                           So return success on the same. Then Nt Mirror will start invalidation of sector */
                        cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
                        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice Reassign request.\n");
                        CompleteRequest( Irp, EMCPAL_STATUS_SUCCESS, 0 );
                    }
                    else
                    {
                        // Fail the request and trace this.
                        cpd_os_io_set_status(pSRB, SRB_STATUS_INVALID_REQUEST);
                        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice unsupported Cdb0: 0x%x\n",(cpd_os_io_get_cdb(pSRB)[0]));
                        CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                    }

                    break;

                default:
                    //
                    // Check to see if it is a read or write command
                    // First check cdb length..if it's 10, then it's a valid CDB10 format command..get the cdb opcode
                    //
                    if( cpd_os_io_get_cdb_length(pSRB) == 10 ){
                        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice Default 10 Byte CDB\n");
                        switch (cpd_os_io_get_cdb(pSRB)[0]){
                            case SCSIOP_READ:                        
                                EmcpalIrpMarkPending(Irp);
                                ntStatus = ppfdSendReadRequestToPP( fido, pSRB, Irp );
                                if (ntStatus != EMCPAL_STATUS_PENDING){
                                    CompleteRequest( Irp, ntStatus, 0 );
                                }
                                break;
                          
                            case SCSIOP_WRITE:
                                EmcpalIrpMarkPending(Irp);
                                ntStatus = ppfdSendWriteRequestToPP( fido, pSRB, Irp );
                                if (ntStatus != EMCPAL_STATUS_PENDING){
                                    CompleteRequest( Irp, ntStatus, 0 );
                                }
                                break;
                            default:
                                // Fail the request and trace this.
                                cpd_os_io_set_status(pSRB, SRB_STATUS_INVALID_REQUEST);
                                cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                                PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice unsupported Cdb0: 0x%x\n",cpd_os_io_get_cdb(pSRB)[0]);
                                CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                                break;
                        }
                    }
                    else{
                        
                        if (cpd_os_io_get_cdb(pSRB)[0] == SCSIOP_REASSIGN_BLOCKS){
                            //Reassign is taken care of in PhyscialPackage/Drive
                            //So no need for NtMirror to issue this command.
                            //A success will result in the continuation of the State Machine in NtMirror
                            cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
                            cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice Reassign request.\n");
                            CompleteRequest( Irp, EMCPAL_STATUS_SUCCESS, 0 );
                        }else{
                            // Log unsupported CDB
                            PPFD_TRACE(PPFD_TRACE_FLAG_WARN, TRC_K_STD, "PPFD: DispatchDiskDevice Default Unsupported CDB Size!\n");
                            // Fail the request and trace this.
                            cpd_os_io_set_status(pSRB, SRB_STATUS_INVALID_REQUEST);
                            cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
                            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice unsupported Cdb0: 0x%x\n",(cpd_os_io_get_cdb(pSRB)[0]));
                            CompleteRequest( Irp, EMCPAL_STATUS_NOT_IMPLEMENTED, 0 );
                        }
                    }
                    break;
            }//end switch ( IoctlCode ) for IRP_MJ_SCSI
            break;


        case EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL:
            //Keeping this case : seperate to trace the Ioctl code
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IRP_MJ_DEVICE_CONTROL\n" );
            IoctlCode =EmcpalExtIrpStackParamIoctlCodeGet(stack);            
            if (IoctlCode == IOCTL_SCSI_GET_ADDRESS){
                if( EmcpalExtIrpStackParamIoctlOutputSizeGet(stack) != sizeof(SCSI_ADDRESS) )
                {
                    ntStatus = EMCPAL_STATUS_INVALID_PARAMETER;
                    Information = 0;
                }
                else
                {
                    if( ppfdPhysicalPackageGetScsiAddress( fido, &ScsiAddress ))
                    {
                        memcpy( EmcpalExtIrpSystemBufferGet(Irp), &ScsiAddress, sizeof(SCSI_ADDRESS));
                        ntStatus = EMCPAL_STATUS_SUCCESS;
                        Information = sizeof(SCSI_ADDRESS);
                    }
                    else
                    {                        
                        ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
                        Information = 0;
                    }
                }
                CompleteRequest( Irp, ntStatus, Information );
            }else{
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice Forwarding Ioctl 0x%x \n",IoctlCode );            
            ntStatus = ppfdForwardIRP( fido, Irp );
            }
            break;  //IRP_MJ_DEVICE_CONTROL

        case EMCPAL_IRP_TYPE_CODE_PNP:
            //Keeping this case : seperate to trace PNP
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: DispatchDiskDevice IRP_MJ_PNP\n" );
            ntStatus = ppfdForwardIRP( fido, Irp );
            break;

        default:
            PPFD_TRACE(PPFD_TRACE_FLAG_WARN, TRC_K_STD, "PPFD: DispatchDiskDevice forwarding unhandled IRP!\n" );
            ntStatus = ppfdForwardIRP(fido, Irp); 
            break;
    }// end switch ( type )

    return( ntStatus );
}

/*******************************************************************************************

 Function: 
    BOOL ppfdSRBExecuteScsi( IN PDEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB )

 Description:
   Handle the SRB_FUNCTION_EXECUTE_SCSI command.  The only command this function supports is the
   SCSIOP_READ_CAPACITY command.  ASIDC sends this command when it starts up. 

 Arguments:
    fido - Pointer to the PPFD disk device object (FiDO) that the SRB is being sent to.
           
    pSRB - SRB pointer

 Return Value:
    TRUE - Successfully got capacity
    FALSE - Failed to get the capacity

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdSRBExecuteScsi( IN PEMCPAL_DEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB )
{
    BOOL bStatus=TRUE;
    UCHAR CDBOpcode;
    READ_CAPACITY_DATA ReadCapacity;
    READ_CAPACITY_DATA *pReadCapacity;     

    //
    // Get the SCSI command/operation code from the CDB (Command Descriptor Block)
    //
    CDBOpcode= cpd_os_io_get_cdb(pSRB)[0];
   
    switch ( CDBOpcode )
    {
        case SCSIOP_READ_CAPACITY:
            PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdSRBExecuteScsi SCSIOP_READ_CAPACITY\n" );
            pReadCapacity = (READ_CAPACITY_DATA*)cpd_os_io_get_data_buf(pSRB);

            // Get the capacity of this drive from PhysicalPackage
            bStatus = ppfdPhysicalPackageGetReadCapacity( fido, &ReadCapacity );
           
            // The Read capacity data is returned in Big Endian format, so we need
            // to swap the bytes before returning the data.
            //
            ((PFOUR_BYTE)&pReadCapacity->LogicalBlockAddress)->Byte0 = ((PFOUR_BYTE)&ReadCapacity.LogicalBlockAddress)->Byte3;
            ((PFOUR_BYTE)&pReadCapacity->LogicalBlockAddress)->Byte1 = ((PFOUR_BYTE)&ReadCapacity.LogicalBlockAddress)->Byte2;
            ((PFOUR_BYTE)&pReadCapacity->LogicalBlockAddress)->Byte2 = ((PFOUR_BYTE)&ReadCapacity.LogicalBlockAddress)->Byte1;
            ((PFOUR_BYTE)&pReadCapacity->LogicalBlockAddress)->Byte3 = ((PFOUR_BYTE)&ReadCapacity.LogicalBlockAddress)->Byte0;
            cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
            cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
            break;

        default:
            break;
    }
    
    return( bStatus );
}


/*******************************************************************************************

 Function:
    BOOL ppfdReleaseDevice( IN PDEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB  )

 Description:
    We are not forwarding the "SRB_FUNCTION_RELEASE_DEVICE" request to the Scsiport/miniport.  
    Process it within PPFD.  This function just complete it successfully.

 Arguments:
    fido
    pSRB  - Pointer to a Scsi Request Block   

 Return Value:
    TRUE
    FALSE

 Others:   

 Revision History:
*******************************************************************************************/
BOOL ppfdReleaseDevice( PEMCPAL_DEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB )
{
    BOOL bStatus = TRUE;
  
    if( (pSRB == NULL) || (fido == NULL)  )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdReleaseDevice invalid parameter fido= 0x%p,pSRB= 0x%p\n", fido, pSRB );
        return( FALSE );
    }

    cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
    cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);

    return( bStatus );
}

 
 /*******************************************************************************************

 Function:
    BOOL ppfdClaimDevice( PDEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB  )

 Description:
    We are not forwarding the "claim" request to the Scsiport/miniport.  Instead, return
    a PPFD disk device object.  ASIDC/NtMirror will send all IRP's intended for the disk device 
    to the "PPFD Disk Device Object" which will allow PPFD to translate that IRP (IRP_MJ_SCSI Read/Write 
    for example) into the appropriate PhysicalPackage API calls.  See ppfdDispatchDiskDevice() for handling of IRP's
    directed to a PPFD disk device.

 Arguments:
    fido - Pointer to the disk device object to "claim"
    pSRB - Pointer to the SCSI_REQUEST_BLOCK info

 Return Value:
    TRUE - Claimed device
    FALSE - Failed to claim device

    The data in cpd_os_io_get_data_buf(pSRB) will contain the "claimed" ppfdDiskDeviceObject on return if
    the return value is TRUE.  

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdClaimDevice( PEMCPAL_DEVICE_OBJECT fido, PCPD_SCSI_REQ_BLK pSRB )
{
    BOOL bStatus=TRUE;
    PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject;
    
   
    if ( ppfdIsPnPDiskDeviceObject( fido )) 
    {
        // In this case, the fido passed to us IS the ppfd Disk device object to return
        // (NtMirror is claiming the device in this case)
        ppfdDiskDeviceObject = fido;
    }
    else
    {
        // Get the PPFD disk device object (aka PPFD Fido) corresponding to the device specified in
        // the SRB from our map. 
        bStatus = ppfdPhysicalPackageGetPPFDFidoFromMap( &ppfdDiskDeviceObject, pSRB ); 
    }

    if( bStatus )
    {
        //
        // Return our PPFD disk device object so that all IRP's from ASIDC that are directed
        // to the disk (IRP_MJ_SCSI Reads/Writes for example) will come through this object which we 
        // can then direct to the physical package instead of the scsiport/miniport.
        //
        cpd_os_io_set_data_buf(pSRB, ppfdDiskDeviceObject);
        cpd_os_io_set_status(pSRB, SRB_STATUS_SUCCESS);
        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);
    }
    else
    {
        //Log error
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdClaimDevice failed!!\n");
        // Setting this to zero is sufficient to indicate an error.  Don't need to set SrbStatus to failure.
        cpd_os_io_set_data_buf(pSRB, 0);
        cpd_os_io_set_status(pSRB, SRB_STATUS_NO_DEVICE);
        cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);

    }

    return( bStatus );
}

/*******************************************************************************************

 Function:
    NTSTATUS ppfdSendReadRequestToPP( PCPD_SCSI_REQ_BLK pSRB, PDEVICE_OBJECT ppfdDiskDeviceObject )

 Description:
    This function is a small "wrapper" function for calling ppfdPhysicalPackageSendRead() (which will
    make the FBE API calls to the PhysicalPackage).  
 
 Arguments:
    pSRB - Pointer to the SCSI Request Block structure
    ppfdDiskDeviceObject - The disk to read

 Return Value:
    TRUE - Success
    FALSE - Failure
    
    Also,  cpd_os_io_get_status(pSRB) and cpd_os_io_get_scsi_status(pSRB) are updated on return.
    cpd_os_io_get_scsi_status(pSRB) is always = SCSISTAT_GOOD, whereas SrbStatus will reflect the
    results of the write operation.  The SRB status is "mapped" to an FBE status by the
    code in the ppfdPhysicalPackageInterface.c

 Others:   

 Revision History:

 *******************************************************************************************/
EMCPAL_STATUS ppfdSendReadRequestToPP( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB, PEMCPAL_IRP Irp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_PENDING;
    CPD_EXT_SRB *pCpdSrb = NULL;
    ULONG Length=0;

#if DBG
    ULONG Offset=0;
    int index=0;
    PHYSICAL_ADDRESS Address;
#endif

    ULONG StartLBA =0;
    
    pCpdSrb = (CPD_EXT_SRB *)pSRB;

#if DBG
    // Only include this for debug build. Provides detailed info on the SGL (Scatter Gather List) that is being
    // passed to us for read operation
    if( pCpdSrb->ext_flags && CPD_EXT_SRB_IS_EXT_SRB )
    {
        // This is an "extended" CPD SRB (specific to our miniport)
        while( pCpdSrb->sgl[index].PhysAddress )
        {
            // Dump SGL info until we reach the end of the list..
            Length = pCpdSrb->sgl[index].PhysLength;
            Address.QuadPart = pCpdSrb->sgl[index].PhysAddress;
            Offset = pCpdSrb->sgl_offset;
            PPFD_TRACE(PPFD_TRACE_FLAG_DBG, TRC_K_STD, "PPFD: ppfdSendSRBIoRequestToPP pCpdSrb SGL[%d] Length= 0x%x, Address= 0x%llx, SGL Offset= 0x%x\n",
		       index, Length, (unsigned long long)Address.QuadPart,
		       Offset );

            index++;
        }
    }
#endif

    // Do any pre-processing here

    // Send the PP packet
    StartLBA = GET_LBA_FROM_CDB10( pSRB );
    Length = GET_LENGTH_FROM_CDB10( pSRB );
    PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdSendReadRequestToPP Length= 0x%x, Start LBA= 0x%x,ppfdDiskDeviceObject= 0x%p\n", Length, StartLBA,ppfdDiskDeviceObject );

    cpd_os_io_set_status(pSRB, SRB_STATUS_PENDING);
    ntStatus = ppfdPhysicalPackageSendRead( StartLBA, Length, pCpdSrb->sgl, ppfdDiskDeviceObject, Irp );      
    if (ntStatus != EMCPAL_STATUS_PENDING){
        // Do not set the status here if the packet was submitted. We might end up overwriting the actual status.        
        ppfdMapNtStatusToSrbStatus(ntStatus,&(cpd_os_io_get_status(pSRB)));
        PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdSendReadRequestToPP Error NTSTATUS 0x%x mapped SRB Status 0x%x",ntStatus,cpd_os_io_get_status(pSRB) );        
    }
    return( ntStatus );
}

/*******************************************************************************************

 Function:
    NTSTATUS ppfdSendWriteRequestToPP( PCPD_SCSI_REQ_BLK pSRB, PDEVICE_OBJECT ppfdDiskDeviceObject )

 Description:
    This function is a small "wrapper" function for calling ppfdPhysicalPackageSendWrite() (which will
    make the FBE API calls to the PhysivalPackage).  


 Arguments:
    pSRB - Pointer to the SCSI Request Block structure
    ppfdDiskDeviceObject - The disk to read

 Return Value:
    TRUE - Success
    FALSE - Failure
    
    Also,  cpd_os_io_get_status(pSRB) and cpd_os_io_get_scsi_status(pSRB) are updated on return.
    cpd_os_io_get_scsi_status(pSRB) is always = SCSISTAT_GOOD, whereas SrbStatus will reflect the
    results of the write operation.  The SRB status is "mapped" to an FBE status by the
    code in the ppfdPhysicalPackageInterface.c
    

 Others:   

 Revision History:

 *******************************************************************************************/
EMCPAL_STATUS ppfdSendWriteRequestToPP( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB, PEMCPAL_IRP Irp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_PENDING;
    CPD_EXT_SRB *pCpdSrb = NULL;
    ULONG Length=0;
    ULONG StartLBA =0;
#if DBG
    ULONG Offset=0;
    PHYSICAL_ADDRESS Address;
    int index = 0;
#endif

    pCpdSrb = (CPD_EXT_SRB *)pSRB;


#if DBG
    // Only include this for debug build. Provides detailed info on the SGL (Scatter Gather List) that is being
    // passed to us for write operation
    if( pCpdSrb->ext_flags && CPD_EXT_SRB_IS_EXT_SRB )
    {
        // This is an "extended" CPD SRB (specific to our miniport)
        while( pCpdSrb->sgl[index].PhysAddress )
        {
            // Dump SGL info until we reach the end of the list..
            Length = pCpdSrb->sgl[index].PhysLength;
            Address.QuadPart = pCpdSrb->sgl[index].PhysAddress;
            Offset = pCpdSrb->sgl_offset;
            PPFD_TRACE(PPFD_TRACE_FLAG_DBG, TRC_K_STD, "PPFD: ppfdSendSRBIoRequestToPP pCpdSrb SGL[%d] Length= 0x%x, Address= 0x%llx, SGL Offset= 0x%x\n",
		       index, Length, (unsigned long long)Address.QuadPart,
		       Offset );
            index++;
        }
    }
#endif
    // Do any pre-processing here

    // Send the PP packet
    StartLBA = GET_LBA_FROM_CDB10( pSRB );
    Length = GET_LENGTH_FROM_CDB10( pSRB );

    PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdSendWriteRequestToPP Length= 0x%x, Start LBA= 0x%x,ppfdDiskDeviceObject= 0x%p\n", Length, StartLBA,ppfdDiskDeviceObject );
    ntStatus = ppfdPhysicalPackageSendWrite( StartLBA, Length, pCpdSrb->sgl, ppfdDiskDeviceObject, Irp );  
    if (ntStatus != EMCPAL_STATUS_PENDING){
        // Do not set the status here if the packet was submitted. We might end up overwriting the actual status.
        ppfdMapNtStatusToSrbStatus(ntStatus,&(cpd_os_io_get_status(pSRB)));
        PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdSendWriteRequestToPP Error NTSTATUS 0x%x mapped SRB Status 0x%x",ntStatus,cpd_os_io_get_status(pSRB) );        
    }

    return( ntStatus );
}

/*******************************************************************************************

 Function:
    BOOL ppfdCPDIoctlRegisterCallbacks(IN PDEVICE_OBJECT fido, IN PIRP Irp,S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl )
 
 Description:
    Save away the callback funtion address provided in the passed data so that PPFD can call this function
    for asynchronous disk events (drive removed/inserted, etc.) that ASIDc and NtMirror are concerned with.  
    PPFD registers with the PhysicalPackage driver for event notification, and will "translate" the PP events into 
    the events that NtMirror and ASIDC expect to see.  In the implementation prior to the introduction of the PP driver,
    the callback is registered with the miniport driver (i.e. FCDMTL for FC) which would do the callback for disk events.
    PPFD is now intercepting that callback registration and handling the callback to the ASIDC and NtMirror clients.

 Arguments:
 
 Return Value:
    TRUE - Success
    FALSE - Failure
    

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdCPDIoctlRegisterCallbacks(IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp, S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl )
{
    EMCPAL_STATUS status;

    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdCPDIoctlRegisterCallbacks flags= 0x%x\n", pCPDRegisterCallbacksIoctl->param.flags );

    // Check the flags
    // PPFD will only handle CPD_REGISTER_INITIATOR or CPD_REGISTER_DEREGISTER that are sent from ASIDC or NtMirror
    //
    if (pCPDRegisterCallbacksIoctl->param.flags & CPD_REGISTER_FILTER_INTERCEPT)    
    {

        status = EMCPAL_STATUS_SUCCESS;
        //Flag indicates that we need to handle this internally
        if( pCPDRegisterCallbacksIoctl->param.flags & CPD_REGISTER_INITIATOR )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdCPDIoctlRegisterCallbacks flags=CPD_REGISTER_INITIATOR\n" );
            if (!ppfdPhysicalPackageRegisterCallback( pCPDRegisterCallbacksIoctl)){
                PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: %s call to ppfdPhysicalPackageRegisterCallback returned error.\n",
                                    __FUNCTION__);
                status = EMCPAL_STATUS_UNSUCCESSFUL;
            }

        }
        else if( pCPDRegisterCallbacksIoctl->param.flags & CPD_REGISTER_DEREGISTER )
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdCPDIoctlRegisterCallbacks flags=CPD_REGISTER_DEREGISTER\n" );
            if (!ppfdPhysicalPackageDeRegisterCallback(pCPDRegisterCallbacksIoctl)){
                PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: %s call to ppfdPhysicalPackageDeRegisterCallback returned error.\n",
                                    __FUNCTION__);
                status = EMCPAL_STATUS_UNSUCCESSFUL;
            }
        }
        CompleteRequest(Irp,status,sizeof(*pCPDRegisterCallbacksIoctl));
    }else{
        ppfdForwardIRP( fido, Irp );
    }

    return( TRUE );
}




void ppfdMapNtStatusToSrbStatus( EMCPAL_STATUS ntStatus, UCHAR *SrbStatus_p )
{
    UCHAR SrbStatus;

    switch ( ntStatus )
    {
        case EMCPAL_STATUS_INVALID_PARAMETER:
            SrbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

        case EMCPAL_STATUS_SUCCESS:
            SrbStatus = SRB_STATUS_SUCCESS;
            break;


        case EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST:
            SrbStatus = SRB_STATUS_NO_DEVICE;
            break;

         case EMCPAL_STATUS_INSUFFICIENT_RESOURCES:
            SrbStatus = SRB_STATUS_ERROR;
            break;

         case EMCPAL_STATUS_UNSUCCESSFUL:
            SrbStatus = SRB_STATUS_ABORTED;
            break;

         case EMCPAL_STATUS_PENDING:
            SrbStatus = SRB_STATUS_PENDING;
            break;

         default:
            SrbStatus = SRB_STATUS_ERROR;
            break;
    }

    // Return it
    *SrbStatus_p = SrbStatus;
}
