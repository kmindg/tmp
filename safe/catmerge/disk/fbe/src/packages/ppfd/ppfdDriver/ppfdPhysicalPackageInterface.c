/***********************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.

 File Name:
    ppfdPhysicalPackageInterface.c
    
 Contents:
    Contains functions related to interacting with the physical package driver. The functions 
    in this file utilize the "FBE API" library functions (named fbe_api_xxx) which can be used by 
    any driver or user space code that needs to access the physical package interface.
    
    All "exported" function names in this file are prefixed with "ppfdPhysicalPackage". For the PPFD driver, 
    this  should be the ONLY file with functions that call Physical Package FBE API's directly. Also,
    only functions in this file should access (read/write) the global PPFD_GDS.PPFDDiskObjectsMap[].  Any code outside
    of this file that requires access to the map MUST call a "ppfdPhysicalPackagexxx" function to read
    or write the map.  


 Exported:
    BOOL ppfdPhysicalPackageInterfaceInit( void )
    BOOL ppfdPhysicalPackageGetNumDisks(DWORD portNumber, DWORD *pNumDisks )
    BOOL ppfdPhysicalPackageClaimDevice( UCHAR PathID, UCHAR TargetID, PDEVICE_OBJECT *ppfdFiDO )
    p_PPFDDISKOBJ ppfdGetDiskObjIDFromPPFDFido( PDEVICE_OBJECT ppfdDiskDeviceObject, fbe_object_id_t *DiskOBJID )
    BOOL ppfdPhysicalPackageAddPPFDFidoToMap( PDEVICE_OBJECT ppfdDiskDeviceObject, UCHAR DiskNum, BOOL bPnPDisk )
    void ppfdPhysicalPackageSetScsiPortNumberInMap( UCHAR scsiPortNumber )
    BOOL ppfdPhysicalPackageGetScsiAddress( PDEVICE_OBJECT fido, SCSI_ADDRESS *pScsiAddress )    
    BOOL ppfdPhysicalPackageInitCrashDump( void )

 Revision History:

***********************************************************************************/

#include "cpd_interface.h"      
#include "K10MiscUtil.h"
#include "ppfdMisc.h"
#include "ppfdKtraceProto.h"
#include "ppfdDeviceExtension.h"
#include "ppfd_panic.h"
#include "spid_kernel_interface.h"      // to get the SP ID to support getting the SLIC type in slot 0
#include "fbe/fbe_notification_lib.h"

//
// FBE API function header files
//
#include "fbe_api_common.h"        
#include "fbe_api_common_transport.h"
#include "fbe_api_discovery_interface.h"
#include "fbe_api_object_map_interface.h"
#include "fbe_winddk.h"
#include "fbe_api_enclosure_interface.h"
#include "fbe_object.h"
#include "fbe_api_physical_drive_interface.h"
#include "fbe/fbe_logical_drive.h"  // fbe_logical_drive_get_pre_read_lba_blocks_for_write

#define PPFD_BOOT_PORT_NUMBER               0
#define PPFD_BOOT_ENCLOSURE_NUMBER          0

#define PPFD_SPA_PRIMARY_SLOT_NUMBER        0
#define PPFD_SPA_SECONDARY_SLOT_NUMBER      2
#define PPFD_SPB_PRIMARY_SLOT_NUMBER        1
#define PPFD_SPB_SECONDARY_SLOT_NUMBER      3

#define PPFD_INVALID_SLOT_NUMBER            0xFFFF

#define PPFD_OBJECT_MAP_MAX_SEMAPHORE_COUNT 0x7FFFFFFF 

extern PPFD_GLOBAL_DATA PPFD_GDS;

// Function prototypes
BOOL ppfdPhysicalPackageGetNumDisks( DWORD portNumber, DWORD *pNumDisks );
EMCPAL_LOCAL void ppfdInitDiskObjectMap( void );
char *ppfdGetFbeStatusString( fbe_status_t fbe_status_code );
char *ppfdGetFbeBlockOperationStatusString( fbe_payload_block_operation_status_t block_status );
char *ppfdGetFbeBlockOperationQualifierString( fbe_payload_block_operation_qualifier_t block_status );
p_PPFDDISKOBJ ppfdGetDiskObjIDFromPPFDFido( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, fbe_object_id_t *DiskOBJID );
EMCPAL_LOCAL fbe_status_t ppfdPhysicalPackageIoCompletionCallback (fbe_packet_t * io_packet, PEMCPAL_IRP Irp  );
void ppfdConvertTransportStatusToSRBStatus( fbe_status_t transport_status, 
                                         fbe_u32_t    transport_qualifier, UCHAR *SrbStatus_p,EMCPAL_STATUS *ntStatus_p );

void ppfdConvertBlockOperationStatusToSRBStatus( fbe_payload_ex_t *io_payload_p,
                                                    PCPD_SCSI_REQ_BLK pSrb,EMCPAL_STATUS *ntStatus_p);
void ppfdStateChangeCallback (void *context);
BOOL ppfdIsStateChangeMsgForBootDrive( update_object_msg_t *pStateChangeMsg );
BOOL ppfdIsStateChangeMsgForCurrentSPBootDrive( update_object_msg_t *pStateChangeMsg );

//Imported functions and globals
BOOLEAN ppfdIsPnPDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject );
BOOLEAN ppfdIsDiskDeviceObject( PEMCPAL_DEVICE_OBJECT  deviceObject );
EMCPAL_STATUS ppfdSendCPDIoctlSetSystemDevices( PEMCPAL_DEVICE_OBJECT pPortDeviceObject, UINT_8E primary_phy_id, fbe_u64_t primary_encl_sas_addr,
                                          UINT_8E secondary_phy_id, fbe_u64_t secondary_encl_sas_addr );
void CompleteRequest( PEMCPAL_IRP Irp, EMCPAL_STATUS status, ULONG info);


static PPFD_WRITE_STATUS ppfdWriteAdvanceState(PPFD_WRITE_CONTEXT * ioContext,BOOLEAN Success);
static PPFD_WRITE_STATUS ppfdWriteStartIO(PPFD_WRITE_CONTEXT * ioContext );
static PPFD_WRITE_STATUS ppfdWriteCheckEdge(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteAllocatePreReadSGL(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteAllocatePreReadBlocks(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteInitializePreReadSGL(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteLockPreReadBlocks(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteIssuePreReadRequest(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteCleanupPreReadIO(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteIssueWriteRequest(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteCompleteIO(PPFD_WRITE_CONTEXT * ioContext);
static fbe_status_t ppfdPhysicalPackageWriteCompletionCallback(fbe_packet_t * io_packet, fbe_packet_completion_context_t context);
static void ppfdGetPPDiskObjIdInMap( UCHAR DiskNumber, fbe_object_id_t *pObjID );
static PPFD_WRITE_STATUS ppfdWriteSubmitIORequest(PPFD_WRITE_CONTEXT * ioContext,BOOLEAN PreRead);
static void ppfdWriteSetNtStatus(PPFD_WRITE_CONTEXT * ioContext,EMCPAL_STATUS ntStatus);


static PPFD_WRITE_CONTEXT * ppfdAllocateIOContext(void);
static void ppfdReleaseIOContext(PPFD_WRITE_CONTEXT *ppfdWriteContext);
static fbe_sg_element_t * ppfdAllocateSGLMemory(SIZE_T  NumberOfSGLElements);
static void ppfdFreeSGLMemory(fbe_sg_element_t *sgl_element_p);
static PPFD_WRITE_STATUS ppfdAllocateBlocks(ULONG  block_size,
                                     void ** pre_read_begin_edge_block_p,
                                     void ** dummy_read_block_p,
                                     void ** pre_read_end_edge_block_p);
static void ppfdReleaseBlocks(void * pre_read_begin_edge_block_p,
                 void * dummy_read_block_p,
                 void * pre_read_end_edge_block_p);
static PPFD_WRITE_STATUS ppfdWriteLockRange(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteUnLockRange(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteQueueIoForRetry(PPFD_WRITE_CONTEXT * ioContext);
static PPFD_WRITE_STATUS ppfdWriteDeQueueIoForRetry(PPFD_WRITE_CONTEXT ** ioContext);
static EMCPAL_STATUS ppfdGetPPDiskObjIdInMapEntry( p_PPFDDISKOBJ pDiskInfoEntry, fbe_object_id_t *pObjID);
static void ppfdSetPPDiskObjIdInMap( UCHAR DiskNumber, fbe_object_id_t ObjID );
static fbe_status_t ppfdSetPPDiskNegotiatedBlockSizeInMap( p_PPFDDISKOBJ pDiskInfoEntry, fbe_block_transport_negotiate_t *negotiate_block_size );
static fbe_status_t ppfdGetPPDiskNegotiatedBlockSizeFromMap( p_PPFDDISKOBJ pDiskInfoEntry, fbe_block_transport_negotiate_t *negotiate_block_size );

typedef struct ppfd_packet_q_element_s{
	fbe_queue_element_t q_element;/*MUST be first*/
	fbe_packet_t packet;
}ppfd_packet_q_element_t;

typedef struct ppfd_block_q_element_s{
	fbe_queue_element_t q_element;/*MUST be first*/
	fbe_u8_t            block_buffer[PPFD_CLIENT_BLOCK_SIZE];
}ppfd_block_q_element_t;

static	fbe_queue_head_t ppfd_contiguous_packet_q_head;
static	fbe_queue_head_t ppfd_contiguous_block_q_head;
static 	fbe_spinlock_t	 ppfd_contiguous_packet_q_lock;
static 	fbe_spinlock_t	 ppfd_contiguous_block_q_lock;

static fbe_status_t ppfdInitContiguousPacketQueue(void);
static fbe_status_t ppfdDestoryContiguousPacketQueue(void);
static void ppfdReturnContiguousPacket(fbe_packet_t *packet);
static fbe_packet_t * ppfdGetContiguousPacket (void);

static fbe_status_t ppfdInitContiguousBlockQueue(void);
static fbe_status_t ppfdDestoryContiguousBlockQueue(void);
static void ppfdReturnContiguousBlock(fbe_u8_t *block);
static fbe_u8_t * ppfdGetContiguousBlock (void);
static void ppfd_object_map_dispatch_queue(void);
static void ppfd_object_map_thread_func(void * context);
static fbe_status_t ppfd_ppfd_object_map_init (void);
fbe_status_t ppfd_ppfd_object_map_destroy (void);
static fbe_status_t ppfd_object_map_register_notification_element(void);
static fbe_status_t ppfd_object_map_unregister_notification_element(void);
static fbe_status_t ppfd_object_map_update_all(void);
static fbe_status_t ppfd_object_map_notification_function(update_object_msg_t * update_object_msg, void * context);
static fbe_status_t ppfd_get_object_location(update_object_msg_t * update_msg);
static fbe_status_t ppfd_locate_object_location_in_table(update_object_msg_t * update_msg,fbe_u32_t  *drive_slot);
static void ppfd_carsh_dump_info_update_thread(void * context);

static update_object_msg_t * ppfdAllocateObjectNotificationStruct(void);
static void ppfdFreeObjectNotificationStruct(update_object_msg_t *ppfdObjMsg);

static fbe_spinlock_t       ppfd_object_map_update_queue_lock;
static fbe_queue_head_t     ppfd_object_map_update_queue_head;
static fbe_bool_t           ppfd_map_interface_initialized = FBE_FALSE;
static fbe_semaphore_t      ppfd_update_semaphore;
static fbe_thread_t         ppfd_object_map_thread_handle;
static fbe_notification_registration_id_t ppfd_reg_id;

fbe_semaphore_t      crash_dump_info_update_semaphore;
static fbe_thread_t         crash_dump_info_thread_handle;
typedef enum object_map_thread_flag_e{
    OBJECT_MAP_THREAD_RUN,
    OBJECT_MAP_THREAD_STOP,
    OBJECT_MAP_THREAD_DONE,
    OBJECT_MAP_THREAD_START_FAILED
}object_map_thread_flag_t;

static object_map_thread_flag_t     object_map_thread_flag;
static object_map_thread_flag_t     crash_dump_info_thread_flag;

//
// State table for Write.  
// WARNING:  If you change this table, make sure that it stays in sync
// with the PPFD_WRITE_STATE enum which are used as index values here.
// The enum is defined in ppfdMisc.h.
//
PPFD_WRITE_STATE_ENTRY  PpfdWriteStateTable[]={
    //PPFD_WRITE_INITIAL_STATE
    {
        PPFD_CHECK_WRITE_EDGE_PRESENT_STATE,
        PPFD_WRITE_COMPLETE_STATE,
    },

    //PPFD_CHECK_WRITE_EDGE_PRESENT_STATE
    {
        PPFD_ALLOCATE_PRE_READ_SGL_STATE, //TRUE //TODO: PEND possible
        PPFD_WRITE_IO_STATE         //FALSE
    },

    
    //PPFD_ALLOCATE_PRE_READ_SGL_STATE
    {
        PPFD_ALLOCATE_PRE_READ_BLOCKS_STATE, //TODO: PEND possible
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_ALLOCATE_PRE_READ_BLOCKS_STATE
    {
        PPFD_INITIALIZE_PRE_READ_SGL_STATE,
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_INITIALIZE_PRE_READ_SGL_STATE
    {
        PPFD_LOCK_PRE_READ_BLOCKS_STATE, //PEND possible
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_LOCK_PRE_READ_BLOCKS_STATE
    {
        PPFD_PRE_READ_IO_STATE,      //PEND possible
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_PRE_READ_IO_STATE
    {
        PPFD_PRE_READ_CLEAN_UP_STATE,
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_PRE_READ_CLEAN_UP_STATE
    {
        PPFD_WRITE_IO_STATE,        //PEND possible
        PPFD_WRITE_COMPLETE_STATE
    },

    //PPFD_WRITE_IO_STATE
    {
        PPFD_WRITE_COMPLETE_STATE,
        PPFD_WRITE_COMPLETE_STATE
    },


    //PPFD_WRITE_COMPLETE_STATE
    {
        PPFD_WRITE_COMPLETE_STATE,
        PPFD_WRITE_COMPLETE_STATE
    }
};


#define PPFD_PRE_ALLOCATED_CONTIG_PACKETS	1000
// This global keeps track of callbacks that have been registered with PPFD via the CPD_IOCTL_REGISTER_CALLBACK
// For SAS/SATA backend. PPFD "intercepts" this IOCTL (does not forward it to the miniport) and maintains the
// callback function addresses here.  We only allow up to 2 callbacks to be registered (one from ASIDC and one from NtMirror)
//
PPFD_CALLBACK_REGISTRATION_DATA ppfdCallbackRegistrationInfo;


/*******************************************************************************************
 Function:
    BOOL ppfdPhysicalPackageInterfaceInit( void )

 Description:
    Call the FBE API function fbe_api_common_init() to initialize the entry points for all function 
    calls that we will need to make to the FBE API library.  Also, initialize the PPFD Disk Device
    map PPFD_GDS.PPFDDiskObjectsMap[].

    If the FBE API init call fails, this function will return an error.

 Arguments:

 Return Value:
    TRUE - Successful
    FALSE - Error (caller should assume that the Physical Package driver/interface is not available and take
            appropriate action)
  
 Revision History:

*******************************************************************************************/

BOOL ppfdPhysicalPackageInterfaceInit( void )
{   
    fbe_status_t    fbeStatus;
    BOOL            bStatus = TRUE;
    DWORD           NumCallback;
    EMCPAL_STATUS   nt_status;

    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdPhysicalPackageInterfaceInit\n");

    // Have to initialize the FBE API library first before calling other fbe_api_xxx functions. 
    // This function will get the entry points into the PhysicalPackage driver and save them locally (in the FBE API
    // library code) for use by the other FBE API functions.

    fbeStatus = fbe_api_common_init_kernel();
    if( fbeStatus == FBE_STATUS_OK  )
    {
		ppfdInitContiguousPacketQueue();
        ppfdInitContiguousBlockQueue();
        //
        // Initialize our PPFD Disk objects map.
        // Note that when testing PPFD in the boot path (NtMirror), the Disk Object ID's have not been enumerated by the
        // PhysicalPackage yet (during PPFD driver entry), so when we init our local map here..the Disk OBJ ID's will just be
        // set to 0. When the callback that we register with PhysicalPackage is sent a notification for each of the boot disks,
        // the callback funcion (ppfdStateChangeCallback)will initialize the Disk Object ID in the map.  Also note that when
        // the first IRP to do a read (sent by NtMirror) of the boot drives comes in, the disk ready notification has not yet 
        // been received by PPFD, so ppfdPhysicalPackageSendRead() not find a valid Disk OBJ ID and will return an error.  
        // ntMirror will retry and after a few seconds, the Obj ID is updated by the callback and the read will succeed
        // We could "wait" for the drive object ID to be initialized when the first read is executed rather than return an 
        // error.
        // 
        ppfdInitDiskObjectMap();

        // Initialize the registered callbacks data
        ppfdCallbackRegistrationInfo.NumRegisteredCallbacks=0;
        for( NumCallback=0; NumCallback < PPFD_MAX_CLIENTS; NumCallback++ )
        {
            ppfdCallbackRegistrationInfo.RegisteredCallbacks[ NumCallback ].async_callback=0;
            ppfdCallbackRegistrationInfo.RegisteredCallbacks[ NumCallback ].context=0;
        }

        fbe_semaphore_init(&crash_dump_info_update_semaphore, 0, 0x7FFFFFFF);  
        crash_dump_info_thread_flag = OBJECT_MAP_THREAD_RUN;

        nt_status = fbe_thread_init(&crash_dump_info_thread_handle, "ppfd_crash_info", ppfd_carsh_dump_info_update_thread, NULL);
        if (nt_status != EMCPAL_STATUS_SUCCESS) {
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: can't start crash dump info update thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
            crash_dump_info_thread_flag = OBJECT_MAP_THREAD_START_FAILED;
            return FBE_STATUS_GENERIC_FAILURE;
        }    

	/*explicity call object map initializing since the new FBE API will not use it by default*/
	ppfd_ppfd_object_map_init();

    /* Trigger first IOCTL related to crash dump support to miniport driver.*/
    fbe_semaphore_release(&crash_dump_info_update_semaphore, 0, 1, FBE_FALSE);

    }else{
        // Return an error
        bStatus = FALSE;

        PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdPhysicalPackageInterfaceInit fbe_api_common_init_kernel = 0x%x (%s)\n", fbeStatus, ppfdGetFbeStatusString (fbeStatus));

    }

    return ( bStatus );
}


/*******************************************************************************************
 Function:
    p_PPFDDISKOBJ ppfdGetDiskObjIDFromPPFDFido( PDEVICE_OBJECT ppfdDiskDeviceObject, fbe_object_id_t *pDiskOBJID )
  
 Description:
    This function looks in the PPFD_GDS.PPFDDiskObjectsMap[] to see if there is a match for the the ppfdDiskDeviceObject
    passed in.  If it finds a match, it will then retrieve the "Disk OBJ ID" (the Physical Package representation
    of the disk) from the map and return it.  If the disk OBJ ID is 0, or it cannot find a match in the map for
    the ppfdDiskDeviceObject, then return FALSE.

 Arguments:
    ppfdDiskDeviceObject - Pointer to the PPFD Disk Device Object to "lookup" in the map
    pDiskOBJID - Pointer to return the Disk OBJ ID

 Return Value:
    A valid ptr - Successfully found the Disk OBJ ID for the ppfdDiskdeviceObject
    NULL - Failed to find a valid Disk OBJ ID for the ppfdDiskdeviceObject

 Revision History:

*******************************************************************************************/
 p_PPFDDISKOBJ ppfdGetDiskObjIDFromPPFDFido( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, fbe_object_id_t *pDiskOBJID )
 {
    BOOL bStatus = FALSE;
    UCHAR DiskNum=0;
    p_PPFDDISKOBJ pDiskObjectData = NULL;
    fbe_object_id_t DiskObjID = FBE_OBJECT_ID_INVALID;

    for ( DiskNum=0; DiskNum < TOTALDRIVES; DiskNum++ )
    {

        // If it's a PnP PPFD Disk Device Object (i.e. created in ppfdAddDevice() for NtMirror)
        // then lookup the matching PnP Fido in our map to find the PhysicalPackage Disk Obj ID
        //
        if( ppfdIsPnPDiskDeviceObject( ppfdDiskDeviceObject ) )
        {
            if( PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdPnPFiDO == ppfdDiskDeviceObject )
            {
                // Get the PP Disk OBJID for this PPFD Fido from the map
                ppfdGetPPDiskObjIdInMap(DiskNum,&DiskObjID);
                if( DiskObjID != FBE_OBJECT_ID_INVALID)
                {
                    bStatus = TRUE;
                    pDiskObjectData = &PPFD_GDS.PPFDDiskObjectsMap[DiskNum];
                    *pDiskOBJID = DiskObjID;
                }
                break;
            }
        }
        //
        // It's not a PnP PPFD Disk Device Object (i.e. it was created during ppfdDriverEntry()..used by ASIDC)
        // Lookup the matching non PnP Fido in our map to find the PhysicalPackage Disk Obj ID
        //
        else
        {
            if( PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdFiDO == ppfdDiskDeviceObject )
            {
                // Get the PP Disk OBJID for this PPFD Fido from the map
                ppfdGetPPDiskObjIdInMap(DiskNum,&DiskObjID);
                if( DiskObjID != FBE_OBJECT_ID_INVALID)
                {
                    bStatus = TRUE;
                    pDiskObjectData = &PPFD_GDS.PPFDDiskObjectsMap[DiskNum];
                    *pDiskOBJID = DiskObjID;
                }
                break;
            }
        }
    }
    return( pDiskObjectData );
 }

 
/*******************************************************************************************
 Function:
    BOOL ppfdPhysicalPackageAddPPFDFidoToMap( PDEVICE_OBJECT ppfdDiskDeviceObject, UCHAR DiskNum, BOOL bPnPDisk )
 
 Description:
    This function adds the ppfdDiskDeviceObject passed in to the PPFD_GDS.PPFDDiskObjectsMap[] map. Any code that needs
    to add a ppfdDiskDeviceObject to the map MUST go through this function rather than access the map directly.
    
 Arguments:
    ppfdDiskDeviceObject - The PPPFD disk device object to add to the map
    DiskNum - The DiskNum is used to index into the map to add the ppfdDiskDeviceObject.  Value should be 0-4.
    bPnPDisk - If TRUE, the ppfdDiskDeviceObject passed in is a PnP disk (created in ppfdAddDevice for NtMirror).  
    

 Return Value:
    TRUE - Successfully added to the map
    FALSE - If one if the parameters passed in is invalid, then FALSE will be returned and the map is not updated

   Revision History:

*******************************************************************************************/
 BOOL ppfdPhysicalPackageAddPPFDFidoToMap( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, UCHAR DiskNum, BOOL bPnPDisk )
 {
    BOOL bStatus = TRUE;
    
    if( (ppfdDiskDeviceObject == NULL) || (DiskNum > (TOTALDRIVES-1) ))
    {
       PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageAddPPFDFidoToMap Invalid Parameter [ERROR]!\n" );
       bStatus = FALSE;
    }
    else
    {
        if( bPnPDisk )
        {
            PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdPnPFiDO = ppfdDiskDeviceObject;
        }
        else
        {
            PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdFiDO = ppfdDiskDeviceObject;
        }
    }

    return( bStatus );
 }

/*******************************************************************************************
 Function:    
    BOOL ppfdPhysicalPackageGetPPFDFidoFromMap( PDEVICE_OBJECT *p_ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB ) 

 Description:
    This function retrieves the ppfdDiskDeviceObject from the PPFD_GDS.PPFDDiskObjectsMap[] for the Disk device specified 
    by the pSRB parameter. The data is returned via p_ppfdDiskDeviceObject. Any code that needs to retrieve a 
    ppfdDiskDeviceObject from the map MUST go through this function rather than access the map directly.
    
 Arguments:
    p_ppfdDiskDeviceObject - Pointer to the PPFD disk device object to fill in 
    pSRB - Points to a SCSI_REQUEST_BLOCK structure that specifies the disk's path ID and target ID
   
 Return Value:
    TRUE - Successfully retrieved the added to the map
    FALSE - If one of the parameters passed in is invalid or the ppfdDeviceObject in the map is found to be NULL,
        then FALSE will be returned
  
 Revision History:

*******************************************************************************************/

#define DRIVE0_MIRROR_ALIAS 128

BOOL ppfdPhysicalPackageGetPPFDFidoFromMap( PEMCPAL_DEVICE_OBJECT *p_ppfdDiskDeviceObject, PCPD_SCSI_REQ_BLK pSRB )
{
    BOOL bStatus = TRUE;
    PEMCPAL_DEVICE_OBJECT ppfdDiskDiskObject;

    // Check for valid parameters

    if( cpd_os_io_get_target_id(pSRB) > (TOTALDRIVES-1) || p_ppfdDiskDeviceObject == NULL )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageGetPPFDFidoFromMap Invalid Parameter [ERROR]!\n" );
        bStatus = FALSE;

    }
    else
    {
        // The target ID is always 0,1,2,3,4. The disks are in order in the map, so just use the target ID
        // to index into the array.

        //Non-PNP device created by PPFD will be used by ASIDC and NTBE.
        //ASIDC currently uses PathID 5 and NTBE tries to use the database drives and uses pathID 0 for the database drives.
        if((cpd_os_io_get_path_id(pSRB) == 5 ) || (cpd_os_io_get_path_id(pSRB) == 0 ))
        {
            if( ppfdDiskDiskObject = PPFD_GDS.PPFDDiskObjectsMap[cpd_os_io_get_target_id(pSRB)].ppfdFiDO )
            {
                *p_ppfdDiskDeviceObject = ppfdDiskDiskObject;
            }
            else
            {
                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageGetPPFDFidoFromMap Disk Device Object for %d is NULL [ERROR]!\n", cpd_os_io_get_target_id(pSRB) );
                bStatus=FALSE;
            }
        }
        else
        {
#ifndef C4_INTEGRATED
            if( ppfdDiskDiskObject = PPFD_GDS.PPFDDiskObjectsMap[cpd_os_io_get_target_id(pSRB)].ppfdPnPFiDO )
#else /* C4_INTEGRATED */
            /* For C4, this should be an ntmirror device request. Since we do
             * not have PnP devies, use the non-PnP map. Alternatively, could
             * have added (PathId == 4) to the if-stmt above.
             */
            if( ppfdDiskDiskObject = PPFD_GDS.PPFDDiskObjectsMap[cpd_os_io_get_target_id(pSRB)].ppfdFiDO )
#endif /* C4_INTEGRATED - C4ARCH - OSdisk */
            {
                // Return the PnP Disk device Object that was created in "ppfdAddDevice()"
                *p_ppfdDiskDeviceObject =  ppfdDiskDiskObject;
            }
            else
            {
                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageGetPPFDFidoFromMap PnP Disk Device Object for %d is NULL [ERROR]!\n", cpd_os_io_get_target_id(pSRB) );
                bStatus=FALSE;
            }
        }
    }

    return( bStatus );
}



/*******************************************************************************************
 Function:
    void ppfdPhysicalPackageInitScsiPortNumberInMap( UCHAR scsiPortNumber )

 Description:
    Initializes the Scsi "PortNumber" for all of the disks in our map with the scsiPortNumber passed in.
    The port number is needed so that when NtMirror sends the IOCTL to get the SCSI_ADDRESS in it's AddDevice() function
    (called via plug and play manager), we will return the correct port number. NtMirror uses this port number
    to create the ScsiPortN name (where N=port number).
 
 Arguments:
    scsiPortNumber

 Return Value:
    none

*******************************************************************************************/

void ppfdPhysicalPackageInitScsiPortNumberInMap( UCHAR scsiPortNumber )
{
    UCHAR               BootDiskSlotNum = 0;

    for(  BootDiskSlotNum=0; BootDiskSlotNum < TOTALDRIVES; BootDiskSlotNum++  )
    {
        // Set the port for all the disks in the map
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.PortNumber = scsiPortNumber;
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.PortNumber = scsiPortNumber;
    }
}


/*******************************************************************************************
 Function:
    void ppfdSetPPDiskObjIdInMap( UCHAR DiskNumber,  fbe_object_id_t ObjID )
 
 Description:
    Wrapper function to set the physical package disk object ID in the  PPFD_GDS.PPFDDiskObjectsMap[] map
    The DiskNumber should be between 0-4

 Arguments:
    DiskNumber
    ObjID 

 Return Value:
    none

*******************************************************************************************/

EMCPAL_LOCAL void
ppfdSetPPDiskObjIdInMap( UCHAR DiskNumber, fbe_object_id_t ObjID )
{
    if( DiskNumber < TOTALDRIVES )
    {
        fbe_spinlock_lock(&(PPFD_GDS.PPFDDiskObjectsMap[DiskNumber].ObjectInfoSpinLock));
        PPFD_GDS.PPFDDiskObjectsMap[DiskNumber].DiskObjID = ObjID;
        if (ObjID == FBE_OBJECT_ID_INVALID){
            PPFD_GDS.PPFDDiskObjectsMap[DiskNumber].negotiated_block_size_valid = FALSE;
        }
        fbe_spinlock_unlock(&(PPFD_GDS.PPFDDiskObjectsMap[DiskNumber].ObjectInfoSpinLock));
    }
    else
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter DiskNumber=%d\n", __FUNCTION__,DiskNumber );
    }
}

EMCPAL_LOCAL EMCPAL_STATUS
ppfdGetPPDiskObjIdInMapEntry( p_PPFDDISKOBJ pDiskInfoEntry,
                              fbe_object_id_t *pObjID )
{    
    if ((pDiskInfoEntry == NULL) || (pObjID == NULL)){
        return (EMCPAL_STATUS_INVALID_PARAMETER);
    }
    fbe_spinlock_lock(&(pDiskInfoEntry->ObjectInfoSpinLock));
    *pObjID = pDiskInfoEntry->DiskObjID;
    fbe_spinlock_unlock(&(pDiskInfoEntry->ObjectInfoSpinLock));

    return (EMCPAL_STATUS_SUCCESS);
}
/*******************************************************************************************
 Function:
    void ppfdGetPPDiskObjIdInMap( UCHAR DiskNumber,  fbe_object_id_t *pObjID )
 
 Description:
    Wrapper function to get the physical package disk object ID in the  PPFD_GDS.PPFDDiskObjectsMap[] map
    from the disk number.  The DiskNumber should be between 0-4

 Arguments:
    DiskNumber
    ObjID 

 Return Value:
    none

*******************************************************************************************/

EMCPAL_LOCAL void
ppfdGetPPDiskObjIdInMap( UCHAR DiskNumber, fbe_object_id_t *pObjID )
{
    fbe_spinlock_lock(&PPFD_GDS.ObjectMapInfoLock);
    if( DiskNumber < TOTALDRIVES )
    {
        *pObjID = PPFD_GDS.PPFDDiskObjectsMap[DiskNumber].DiskObjID;
    }
    else
    {
        *pObjID = FBE_OBJECT_ID_INVALID;
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdGetPPDiskObjIdInMap Invalid Parameter DiskNumber=%d\n", DiskNumber );
    }
    fbe_spinlock_unlock(&PPFD_GDS.ObjectMapInfoLock);
}

EMCPAL_LOCAL fbe_status_t
ppfdGetPPDiskNegotiatedBlockSizeFromMap( p_PPFDDISKOBJ pDiskInfoEntry,
                                         fbe_block_transport_negotiate_t *negotiate_block_size )
{
    fbe_status_t fbeStatus;

    if ((pDiskInfoEntry == NULL) || (negotiate_block_size == NULL)){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter DiskInfoPtr=0x%p negotiate=0x%p\n",
                                    __FUNCTION__,pDiskInfoEntry,negotiate_block_size );
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_spinlock_lock(&(pDiskInfoEntry->ObjectInfoSpinLock));
    if (pDiskInfoEntry->DiskObjID == FBE_OBJECT_ID_INVALID){
        fbe_spinlock_unlock(&(pDiskInfoEntry->ObjectInfoSpinLock));
        return (FBE_STATUS_NO_DEVICE);
    }
    if (pDiskInfoEntry->negotiated_block_size_valid == TRUE){        
        /*Copy already negotiated data.*/
        *negotiate_block_size = pDiskInfoEntry->negotiated_block_size;
        fbeStatus = FBE_STATUS_OK;
    }else{
        fbeStatus = FBE_STATUS_NOT_INITIALIZED;
    }
    fbe_spinlock_unlock(&(pDiskInfoEntry->ObjectInfoSpinLock));
    return (fbeStatus);
}

EMCPAL_LOCAL fbe_status_t
ppfdSetPPDiskNegotiatedBlockSizeInMap( p_PPFDDISKOBJ pDiskInfoEntry,
                                       fbe_block_transport_negotiate_t *negotiate_block_size )
{    
    if ((pDiskInfoEntry == NULL) || (negotiate_block_size == NULL)){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter DiskInfoPtr=0x%p negotiate=0x%p\n",
                                    __FUNCTION__,pDiskInfoEntry,negotiate_block_size );
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_spinlock_lock(&(pDiskInfoEntry->ObjectInfoSpinLock));
    if (pDiskInfoEntry->DiskObjID == FBE_OBJECT_ID_INVALID){
        fbe_spinlock_unlock(&(pDiskInfoEntry->ObjectInfoSpinLock));
        return (FBE_STATUS_NO_DEVICE);
    }
    pDiskInfoEntry->negotiated_block_size_valid = TRUE;
    /*Copy already negotiated data.*/
    pDiskInfoEntry->negotiated_block_size = *negotiate_block_size;    
    fbe_spinlock_unlock(&(pDiskInfoEntry->ObjectInfoSpinLock));
    return (FBE_STATUS_OK);
}
/*******************************************************************************************
 Function:
    void ppfdSetEnclObjIdInMap(fbe_object_id_t ObjID )
   
 Description:
    Wrapper function to set the physical package encl object ID.

 Arguments:    
    ObjID 

 Return Value:
    none

*******************************************************************************************/

EMCPAL_LOCAL void ppfdSetEnclObjIdInMap( fbe_object_id_t ObjID )
{
    fbe_spinlock_lock(&PPFD_GDS.ObjectMapInfoLock);
    PPFD_GDS.PPFDEnclObject.encl_object_id = ObjID;
    fbe_spinlock_unlock(&PPFD_GDS.ObjectMapInfoLock);
    return;
}

/*******************************************************************************************
 Function:
    void ppfdGetEnclObjIdInMap( fbe_object_id_t *pObjID )
 
 Description:
    Wrapper function to get the physical package encl object ID.

 Arguments:    
    ObjID 

 Return Value:
    none

*******************************************************************************************/

EMCPAL_LOCAL void ppfdGetEnclObjIdInMap( fbe_object_id_t *pObjID )
{
    fbe_spinlock_lock(&PPFD_GDS.ObjectMapInfoLock);
    *pObjID = PPFD_GDS.PPFDEnclObject.encl_object_id;
    fbe_spinlock_unlock(&PPFD_GDS.ObjectMapInfoLock);
    return;
}

/*******************************************************************************************
 Function:
    void ppfdInitDiskObjectMap( void )
   
 Description:
    Initializes the PPFD_GDS.PPFDDiskObjectsMap[].  Leaves the PhysicalPackage disk OBJ Id's and the ppfd FiDO disk objects 
    (both the pnp FiDO's and the non pnpo FiDO's)  uninitialized.  They will get filled in later via the callback
    fucntion that PPFD registers with PP for disk events.  The pnp disk FiDO's get filled in when ppfdAddDevice() 
    is called (ntMirror disk objects) and the non pnp disk FiDO's get filled in during DriverEntry() processin 
    when the FiDO disk objects are created.
    
  Arguments:
    none

 Return Value:
    none
  
 Revision History:

*******************************************************************************************/
EMCPAL_LOCAL void ppfdInitDiskObjectMap( void )
{
    UCHAR BootDiskSlotNum = 0;

    fbe_spinlock_init(&PPFD_GDS.ObjectMapInfoLock);
    PPFD_GDS.PPFDEnclObject.encl_object_id = FBE_OBJECT_ID_INVALID;
    PPFD_GDS.PPFDEnclObject.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    //
    // Init map for all drives
    //
    for(  BootDiskSlotNum=0; BootDiskSlotNum < TOTALDRIVES; BootDiskSlotNum++  )
    {
        fbe_spinlock_init(&(PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ObjectInfoSpinLock));
        fbe_spinlock_init(&(PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].IORegionSpinLock));
        EmcpalInitializeListHead(&(PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].WriteLockListHead));
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
        ppfdSetPPDiskObjIdInMap(BootDiskSlotNum,FBE_OBJECT_ID_INVALID);
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.PathId = 0;     
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.TargetId = BootDiskSlotNum;   
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.Lun =0;
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.Length = sizeof( SCSI_ADDRESS );
        // The port number will be updated by the code executed at DriverEntry that detects which ScsiPort is the backend 0
        // port.  For now, just initialize this to 0
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDDisk.PortNumber = 0;
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ppfdFiDO=0;
       
        
        // Init the pnp related entries
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.PathId = ALIAS_MIRROR_PATH_ID;     
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.TargetId = BootDiskSlotNum;  
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.Lun =0;
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.Length = sizeof( SCSI_ADDRESS );
        
        // The port number will be updated by the code executed at DriverEntry that detects which ScsiPort is the backend 0
        // port.  For now, just initialize this to 0.
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ScsiAddrPPFDpnpDisk.PortNumber = 0;
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].ppfdPnPFiDO=0;

        fbe_zero_memory(&(PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].negotiated_block_size),sizeof(fbe_block_transport_negotiate_t));
        PPFD_GDS.PPFDDiskObjectsMap[BootDiskSlotNum].negotiated_block_size_valid = FALSE;
    }
   
} //end ppfdInitDiskObjectMap()


/*******************************************************************************************
 Function:
    BOOL ppfdPhysicalPackageClaimDevice( UCHAR PathID, UCHAR TargetID, PDEVICE_OBJECT *ppfdFiDO )
    
 Description:
    Look for a matching disk in our map (matches the PathID and TargetID passed in) and return 
    the ppfdFiDO (PPFD Disk Device Object) for that disk.  ASIDC sends the "SRB_FUNCTION_CLAIM_DEVICE" 
    after it gets the Inquiry Data. It uses the TargetID/PathID from the inquiry data when it sends the 
    "SRB_FUNCTION_CLAIM_DEVICE".

 Arguments:
    PathID
    TargetID
    *ppfdFiDO - Pointer for returning the ppfd FiDO

 Return Value:
    TRUE    
    FALSE   
  
 Revision History:

*******************************************************************************************/

BOOL ppfdPhysicalPackageClaimDevice( UCHAR PathID, UCHAR TargetID, PEMCPAL_DEVICE_OBJECT *ppfdFiDO )
{
    BOOL bStatus = FALSE;
    int i;

    //
    // Find the associated Path/Target ID for this FiDO in our map and "claim" the device
    //
    // We only expect the claim request for non Pnp Disk devices (ASIDC)..so look for it in the ScsiAddrPPFDDisk 
    // member only, not the ScsiAddrPPFDpnpDisk member.
    //
    for( i=0; i< TOTALDRIVES; i++ )
    {
        if( (PPFD_GDS.PPFDDiskObjectsMap[i].ScsiAddrPPFDDisk.TargetId == TargetID) && (PPFD_GDS.PPFDDiskObjectsMap[i].ScsiAddrPPFDDisk.PathId == PathID) )
        {
            // Return the PPFD FiDO pointer
            *ppfdFiDO = PPFD_GDS.PPFDDiskObjectsMap[i].ppfdFiDO;
            bStatus=TRUE;
            break;
        }
    }

    return( bStatus );
}


EMCPAL_LOCAL void *ppfdAllocateMemory(SIZE_T  NumberOfBytes )
{
    // Allocate memory with tag ppfd
	return fbe_allocate_io_nonpaged_pool_with_tag ( (fbe_u32_t)NumberOfBytes, 'dfpp');
}

EMCPAL_LOCAL void ppfdFreeMemory( PVOID pMem )
{
    // Free memory with tag ppfd
  	fbe_release_io_nonpaged_pool_with_tag( pMem, 'dfpp' );
}


/*******************************************************************************************

 Function:
        
    EMCPAL_STATUS ppfdPhysicalPackageSendRead( ULONG StartLBA, ULONG Length, PSGL pSGL, PDEVICE_OBJECT ppfdDiskDeviceObject, UCHAR* SrbStatus )

 Description:
    Build a "Read" block operation payload packet and send it to Physicalpackage.
    This function determines which drive to send the "read" to by using the ppfdDiskDeviceObject (passed in) to look 
    for a match in our local PPFD disk device map. It retrieves the PhysicalPackage "Disk Object ID" for this disk from
    the map...to be used in the "FBE packet".  Several FBE API functions are used to create a "block operation payload" 
    which is sent to the PhysicalPackage.  This function will WAIT until the I/O completion callback routine is called 
    before returning.  It will timeout if the operation does not complete.
    
    The simplified sequence is as follows:
            ppfdGetDiskObjIDFromPPFDFido()
            Allocate memory for the packet_p
            fbe_transport_initialize_packet( packet_p )
            fbe_transport_get_payload_ex( packet_p )
            fbe_payload_ex_allocate_block_operation()
            fbe_payload_ex_set_sg_list()
            fbe_payload_block_build_operation()
            fbe_payload_ex_set_attr()
            fbe_payload_ex_set_physical_offset()
            fbe_api_common_send_io_packet()
            fbe_semaphore_wait() - wait for I/O to complete (callback will signal)
            fbe_semaphore_destroy()
            fbe_payload_block_get_status()
            fbe_payload_block_get_qualifier() 
            ppfdConvertBlockOperationStatusToSRBStatus()

 Arguments:
    StartLBA - Start the read at this LBA
    Length - Number of blocks/sectors to read
    pSGL - Pointer to the Scatter-Gather List. Note that the SGL passed to us contains physical addresses
    ppfdDiskDeviceObject - The PPFD disk device object (which represents the disk) to read

  
 Return Value:
    TRUE - Successfully completed the read operation
    FALSE - Failed the read (error message is written to ktrace STD)
    
    In both cases, the contents of *SrbStatus is updated with an SRB_STATUS_XXX value to reflect the status

 Others:   

 Revision History:

 *******************************************************************************************/

EMCPAL_STATUS ppfdPhysicalPackageSendRead( ULONG StartLBA, ULONG Length, PSGL pSGL, PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PEMCPAL_IRP Irp )

{
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t * io_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_object_id_t DiskObjID = FBE_OBJECT_ID_INVALID;
    fbe_status_t    fbeStatus;    
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
    fbe_block_transport_negotiate_t negotiate_block_size;
    p_PPFDDISKOBJ   pDiskObjectData = NULL;
                                            
    //
    // Check parameters for errors
    //

    if( (pSGL == NULL) || (Length ==0) || (ppfdDiskDeviceObject == NULL ) ){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter pSGL= 0x%p,Length= 0x%x, ppfdDiskDeviceObject= 0x%p\n",
                                __FUNCTION__, pSGL, Length, ppfdDiskDeviceObject);
        ntStatus = EMCPAL_STATUS_INVALID_PARAMETER;
        return ntStatus;        
    }

    //
    // Get the PP logical disk device object from the map based on the ppfdDiskDeviceObject
    // 
    pDiskObjectData = ppfdGetDiskObjIDFromPPFDFido( ppfdDiskDeviceObject, &DiskObjID );
    if((pDiskObjectData == NULL) ||(DiskObjID == FBE_OBJECT_ID_INVALID)){
        // Failed to get a valid object ID Return error
        // srbStatus is SRB_STATUS_ERROR by default
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Drive not in object table! Entry 0x%p ID 0x%x\n",
                            __FUNCTION__,pDiskObjectData,DiskObjID);        
        ntStatus = EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST;
        return ntStatus;
    }
   
	packet_p = ppfdGetContiguousPacket();
    if ( packet_p == NULL ){
        // Failed to allocate packet_p memory
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Allocate packet Failed!\n",__FUNCTION__);
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbeStatus = fbe_transport_set_packet_priority(packet_p,FBE_PACKET_PRIORITY_URGENT);
    if( fbeStatus != FBE_STATUS_OK ){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Error setting packet pririty! Error: %d\n",
                                                  __FUNCTION__,fbeStatus);
    }
    // Get the payload address from the packet
    io_payload_p = fbe_transport_get_payload_ex ( packet_p );
    if( io_payload_p == NULL ) {
        // Failed to get i/o payload, return error
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s get Payload Failed!\n",__FUNCTION__);
        // Free allocated packet_p before exiting
		ppfdReturnContiguousPacket(packet_p);/*no need to destroy or free memory*/
        packet_p = NULL;
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation( io_payload_p);
    if( block_operation_p == NULL )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block_operation_p [ERROR]\n",__FUNCTION__ );
        // Free allocated packet_p before exiting
		ppfdReturnContiguousPacket(packet_p);/*no need to destroy or free memory*/
        packet_p = NULL;
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }

    //
    // Set the SGL (scatter gather list)..which was passed to us..
    // 
    fbeStatus = fbe_payload_ex_set_sg_list( io_payload_p, (fbe_sg_element_t*)pSGL, 0);	
    if( fbeStatus == FBE_STATUS_OK )
    {
        fbe_zero_memory(&negotiate_block_size,sizeof(negotiate_block_size));
        fbeStatus = ppfdGetPPDiskNegotiatedBlockSizeFromMap(pDiskObjectData,&negotiate_block_size);
        if ( fbeStatus != FBE_STATUS_OK ){
            negotiate_block_size.requested_block_size = PPFD_CLIENT_BLOCK_SIZE;
            negotiate_block_size.requested_optimum_block_size = PPFD_CLIENT_OPTIMIUM_BLOCK_SIZE;
            negotiate_block_size.optimum_block_alignment = PPFD_CLIENT_OPTIMIUM_BLOCK_ALIGNMENT;    
            if ((fbeStatus = fbe_api_physical_drive_get_drive_block_size(DiskObjID,
                                                                            &negotiate_block_size,
                                                                            NULL,
                                                                            NULL)) == FBE_STATUS_OK)
            {
                 /* Save information for subsequent IOs.*/
                ppfdSetPPDiskNegotiatedBlockSizeInMap(pDiskObjectData,&negotiate_block_size);
            }else{        
                PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block size query failed 0x%x!\n",
                            __FUNCTION__,fbeStatus);                
            }
        }
        if( fbeStatus == FBE_STATUS_OK ){
             // fbe_io_memory_set_age_threshold_and_priority()
            fbeStatus = fbe_payload_block_build_operation( block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                        StartLBA,
                                                        Length,
                                                        negotiate_block_size.block_size,
                                                        negotiate_block_size.optimum_block_size,
                                                        NULL);
        }

        if( fbeStatus == FBE_STATUS_OK )
        {
            fbe_cpu_id_t cpu_id;

            // Tell PP that the SGL contains physical addresses so it does not try to translate them
            // from virtual to physical.
            fbe_payload_ex_set_attr( io_payload_p, FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET );
            fbe_payload_ex_set_physical_offset( io_payload_p, 0 );

            fbeStatus = fbe_payload_ex_increment_block_operation_level( io_payload_p );

            fbe_transport_get_cpu_id(packet_p, &cpu_id);
            if(cpu_id == FBE_CPU_ID_INVALID) {
                cpu_id = fbe_get_cpu_id();
                fbe_transport_set_cpu_id(packet_p, cpu_id);
            }
            
            fbeStatus = fbe_api_common_send_io_packet ( packet_p, DiskObjID,
                                        (fbe_packet_completion_function_t)ppfdPhysicalPackageIoCompletionCallback,
					                    Irp,
					                    NULL,
					                    NULL,
										FBE_PACKAGE_ID_PHYSICAL); 

            
            if((fbeStatus != FBE_STATUS_OK) && (fbeStatus != FBE_STATUS_PENDING)) {
                PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s fbe_api_common_send_io_packet failed fbeStatus= %s\n",
                    __FUNCTION__,ppfdGetFbeStatusString( fbeStatus ) );
            }
            //Since the packet is submitted, we should return STATUS_PENDING
            ntStatus = EMCPAL_STATUS_PENDING;
        }else{ // fbe_payload_block_build_operation failed        
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block build operation failed fbeStatus= %s\n",__FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
        }
    }else{  //fbe_payload_ex_set_sg_list() failed    
        ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s fbe_payload_ex_set_sg_list failed fbeStatus= %s\n",__FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
        // Free any memory we allocated    
        if(  block_operation_p ){
            // Need to release now that we are done with it
            fbe_payload_ex_release_block_operation( io_payload_p, block_operation_p );
        }
    
        if( packet_p != NULL ){
		   ppfdReturnContiguousPacket(packet_p);/*no need to destroy or free memory*/
           packet_p = NULL;
        }
        //
        // Failed to set the SGL, return error
        //
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s set SGL failed fbeStatus= %s\n",__FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );        
    }     
    
    if (ntStatus != EMCPAL_STATUS_PENDING){
        if (packet_p != NULL){
		   ppfdReturnContiguousPacket(packet_p);/*no need to destroy or free memory*/
           packet_p = NULL;
        }
    }

    return( ntStatus );
}

/*******************************************************************************************

 Function:
    fbe_status_t ppfdPhysicalPackageIoCompletionCallback (fbe_packet_t * io_packet,  PIRP Irp  )    

 Description:
    This callback function is called by the PP when an I/O operation has completed.
    The IRP that is been passed in will be signaled as completed to upper layer driver

 Arguments:
    io_packet - fbe_packet for this io transaction.
    Irp - Irp related to the passed in bfe_packet. This IRP will be signaled as completed to upper layer driver

 Return Value:
    FBE_STATUS_OK

 Others:   

 Revision History:

 *******************************************************************************************/
EMCPAL_LOCAL fbe_status_t
ppfdPhysicalPackageIoCompletionCallback( fbe_packet_t * io_packet,
                                         PEMCPAL_IRP Irp  )
{
    fbe_payload_ex_t * io_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_scsi_error_code_t error_code; 
    fbe_u32_t reported_sc_data = 0;
    PEMCPAL_IO_STACK_LOCATION stack;
    PCPD_SCSI_REQ_BLK pSRB;
    EMCPAL_STATUS    ntStatus;
    fbe_status_t     transport_status;
    fbe_u32_t        transport_qualifier;


    PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback\n" );        
    if((io_packet == NULL) || (Irp == NULL)){
        panic(PPFD_IO_INVALID_PARAMETER_IN_COMPLETION,(UINT_PTR)io_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    stack = EmcpalIrpGetCurrentStackLocation(Irp);
    pSRB = EmcpalExtIrpStackParamScsiSrbGet(stack);

    transport_status = fbe_transport_get_status_code(io_packet);
    transport_qualifier = fbe_transport_get_status_qualifier(io_packet);

    io_payload_p = fbe_transport_get_payload_ex ( io_packet );

    if( io_payload_p == NULL ) 
    {
        //This is a erroneos  condition ..panic here
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback get Payload Failed!\n");       
        panic(PPFD_IO_INVALID_PAYLOAD_IN_COMPLETION,(UINT_PTR)io_packet);                
    }

    block_operation_p = fbe_payload_ex_get_block_operation( io_payload_p);
    if( block_operation_p == NULL )
    {
        //This is a erroneos  condition ..panic here
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback block_operation_p [ERROR]\n" );        
        panic(PPFD_IO_INVALID_BLOCK_OP_IN_COMPLETION,(UINT_PTR)io_packet);
    }

    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &qualifier ); 

    PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback block_status=%d - %s\n", block_status, ppfdGetFbeBlockOperationStatusString( block_status ));
    if( block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS )
    {
         // Get more details on the specific error
         fbe_payload_ex_get_scsi_error_code(io_payload_p, &error_code);
         fbe_payload_ex_get_sense_data (io_payload_p, &reported_sc_data);
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback qualifier=%d - %s\n",qualifier, ppfdGetFbeBlockOperationQualifierString( qualifier ) );
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: ppfdPhysicalPackageIoCompletionCallback Scsi error code=0x%x, scdata=0x%x\n",error_code, reported_sc_data );
    }

    //Convert FBE transport status to SRB status
    ppfdConvertTransportStatusToSRBStatus(transport_status,transport_qualifier, &(cpd_os_io_get_status(pSRB)), &ntStatus);

    // Convert the FBE block operation status to an SRB Status    
    if (ntStatus == EMCPAL_STATUS_SUCCESS){
        ppfdConvertBlockOperationStatusToSRBStatus( io_payload_p, pSRB, &ntStatus);
    }

    //set the Scsistatus    
    cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);

	 ppfdReturnContiguousPacket(io_packet);/*no need to destroy or free memory*/

     //complete the irp
     CompleteRequest( Irp, ntStatus, 0 );	
   
     return FBE_STATUS_OK;

}


/*******************************************************************************************

 Function:
    BOOL ppfdPhysicalPackageGetReadCapacity( PDEVICE_OBJECT fido, READ_CAPACITY_DATA *pReadCapacity )

 Description:
    This function gets the data required for the SCSIOP_READ_CAPACITY command from the Physical Package.

 Arguments:
    fido - Pointer to the PPFD disk device object (FiDO) that we need to get the capaity for
    pReadCapacity - Points to the READ_CAPACITY_DATA structure that this function will fill in with the capacity info

 Return Value:
    TRUE - Successfully got capacity
    FALSE - Failed to get the capacity

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdPhysicalPackageGetReadCapacity( PEMCPAL_DEVICE_OBJECT fido, READ_CAPACITY_DATA *pReadCapacity )
{
    BOOL bStatus = TRUE;
    fbe_block_transport_negotiate_t negotiate_block_size;
    fbe_u32_t physical_block_size_p;
    fbe_u32_t optimal_block_size_p;
    fbe_lba_t capacity_p;
    fbe_object_id_t disk_object_id;
    fbe_status_t fbe_status;

    fbe_zero_memory(&negotiate_block_size,sizeof(negotiate_block_size));
    negotiate_block_size.requested_block_size = PPFD_CLIENT_BLOCK_SIZE;
	negotiate_block_size.requested_optimum_block_size = PPFD_CLIENT_OPTIMIUM_BLOCK_SIZE;
	negotiate_block_size.optimum_block_alignment = PPFD_CLIENT_OPTIMIUM_BLOCK_ALIGNMENT;
	
    if( (fido == NULL) || (pReadCapacity == NULL ))
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageGetReadCapacity() ERROR Invalid Parameter!\n" );
        return( FALSE );
    }

    //
    // Get the Disk OBJ ID corresponding to this fido from our map
    //

    if( ppfdGetDiskObjIDFromPPFDFido( fido, &disk_object_id ) == NULL)
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageGetReadCapacity() ERROR - cannot find disk Obj ID\n" );
        return( FALSE );
    }

    PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdPhysicalPackageGetReadCapacity() Get capacity for DiskObj=%d\n",disk_object_id );

	fbe_status = fbe_api_physical_drive_get_drive_block_size(disk_object_id, &negotiate_block_size, NULL, NULL);
	if ( fbe_status != FBE_STATUS_OK)
    {

        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageGetReadCapacity() ERROR - cannot get block size fbe_status= 0x%x\n", fbe_status );
		bStatus = FALSE;
	} 
    else 
    {
        /* Validate returned values and report an error if they don't make sense.*/
        /*if ((negotiate_block_size.physical_block_size < 512)
            ||(negotiate_block_size.optimum_block_size  < 1)
            ||(negotiate_block_size.optimum_block_size > 512)) 
        {
            /* Report error and continue processing*/
            //return FBE_STATUS_GENERIC_FAILURE;
        //}

        /* Return sane values processing */
		physical_block_size_p = negotiate_block_size.physical_block_size;
		optimal_block_size_p = negotiate_block_size.optimum_block_size;
		capacity_p = negotiate_block_size.block_count;
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdPhysicalPackageGetReadCapacity() Block Count= 0x%llx, Bytes per block= 0x%x \n",
		    (unsigned long long)capacity_p, physical_block_size_p );
        
        pReadCapacity->LogicalBlockAddress = (ULONG)capacity_p;
        pReadCapacity->BytesPerBlock = physical_block_size_p;
       
	}

    return( bStatus );
}


/*******************************************************************************************

 Function:
    BOOL ppfdPhysicalPackageGetScsiAddress( PDEVICE_OBJECT fido, SCSI_ADDRESS *pScsiAddress )
    
 Description:
    This gets called to handle the IOCTL_SCSI_GET_ADDRESS IRP.  NtMirror sends this IOCTL during
    it's "AddDevice" handling and Scsitarg als sends it.  
    
 Arguments:
    PDEVICE_OBJECT fido - Pointer to the PPFD Disk Device object that we need to retrieve the SCSI_ADDRESS
                    information for.

    SCSI_ADDRESS *pScsiAddress - Pointer to a SCSI_ADDRESS variable that will be filled in by this function
   
 Return Value:
    TRUE - Success
    FALSE - Failed

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdPhysicalPackageGetScsiAddress( PEMCPAL_DEVICE_OBJECT fido, SCSI_ADDRESS *pScsiAddress )
{
    BOOL bStatus = FALSE;
    UCHAR DiskNum = 0;

    // See if we find a match for the fido in the map
    for( DiskNum=0; DiskNum < TOTALDRIVES; DiskNum++ )
    {
        // Check to see if it's a pnp device Object
        if( ppfdIsPnPDiskDeviceObject( fido ) )
        {
            if( fido == PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdPnPFiDO )
            {
                pScsiAddress->PortNumber =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDpnpDisk.PortNumber;
                pScsiAddress->PathId =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDpnpDisk.PathId;
                pScsiAddress->TargetId =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDpnpDisk.TargetId;
                pScsiAddress->Length =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDpnpDisk.Length;
                pScsiAddress->Lun =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDpnpDisk.Lun;
                bStatus = TRUE;
                break;
            }
        }
        else 
        {
            // Look for a match with the non PnP Disk device objects and return that Scsiaddres data if
            // we find a match.  Scsitarg is one driver that will send the get scsi address IOCTL 
            // after claiming on of these disk devices.
            if( fido == PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ppfdFiDO )
            {
                pScsiAddress->PortNumber =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDDisk.PortNumber;
                pScsiAddress->PathId =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDDisk.PathId;
                pScsiAddress->TargetId =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDDisk.TargetId;
                pScsiAddress->Length =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDDisk.Length;
                pScsiAddress->Lun =  PPFD_GDS.PPFDDiskObjectsMap[DiskNum].ScsiAddrPPFDDisk.Lun;
                bStatus = TRUE;
                break;
            }
        }
    }
    return( bStatus );
}


/*******************************************************************************************

 Function:
    SP_ID ppfdGetSpId( void )
    
 Description:
    Reads the SP ID information from teh PPFD globadl data structure.  This serve as a wrapper
    to abstract access to the global data.  By saving the SP ID in a global, we don't have to 
    keep calling SPID driver to get the information, we just have to do it once during early 
    driver entry processing and then get the info from the global after that,

 Arguments:
    none

 Return Value:
    SP_A (0)
    SP_B (1)
    SP_INVALID (0xFF)

 Others:   

 Revision History:

 *******************************************************************************************/
EMCPAL_LOCAL SP_ID ppfdGetSpId( void )
{
    return( PPFD_GDS.SpId );
}

/*******************************************************************************************

 Function:
    BOOL ppfdIsStateChangeMsgForBootDrive( update_object_msg_t *pStateChangeMsg )
   
 Description:
    Checks to see if the state change message is for enclosure 0, port 0, drive 0-4
    Checks the object type too just as an extra test.  An object type of
    FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE is what we are interested in.

    returns TRUE if it is, and FALSE otherwise.

 Arguments:
    pStateChangeMsg - Pointer to the "State Change Message" to check

 Return Value:
    TRUE - The msg is for a state change on one of the "boot" drives
    FALSE - The msg is not for a state change on one of the "boot" drives

 Others:   

 Revision History:

 *******************************************************************************************/
 BOOL ppfdIsStateChangeMsgForBootDrive(  update_object_msg_t *pStateChangeMsg )
 {
     //Note: (pStateChangeMsg->port == 0) check has built in assumption that BE 0 
     // will be the boot Port. This should be modified when we plan to boot from 
     // any other BE port.
    if((pStateChangeMsg->notification_info.object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) && //Object type check
         ((pStateChangeMsg->port == PPFD_BOOT_PORT_NUMBER) && (pStateChangeMsg->encl == PPFD_BOOT_ENCLOSURE_NUMBER )
           && (pStateChangeMsg->drive >= 0) && ( pStateChangeMsg->drive < TOTALDRIVES)) //Drive location check.
        )
    {
        return( TRUE );
    }

    return( FALSE );
 }

 
/*******************************************************************************************

 Function:
    BOOL ppfdIsStateChangeMsgForCurrentSPBootDrive( update_object_msg_t *pStateChangeMsg )
   
 Description:
    Checks to see if the state change message is for enclosure 0, port 0, drive 0-3
    Checks the object type too just as an extra test.  An object type of
    FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE is what we are interested in.
    Additionally filter for the mirror drives for the current SP.

    returns TRUE if it is, and FALSE otherwise.

 Arguments:
    pStateChangeMsg - Pointer to the "State Change Message" to check

 Return Value:
    TRUE - The msg is for a state change on one of the "boot" drives for current SP
    FALSE - The msg is not for a state change on one of the "boot" drives

 Others:   

 Revision History:

 *******************************************************************************************/
 BOOL ppfdIsStateChangeMsgForCurrentSPBootDrive(  update_object_msg_t *pStateChangeMsg )
 {
     BOOL CurrentSpBootDrive = FALSE;

     //Note: (pStateChangeMsg->port == 0) check has built in assumption that BE 0 
     // will be the boot Port. This should be modified when we plan to boot from 
     // any other BE port.
    if((pStateChangeMsg->notification_info.object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) && //Object type check
         ((pStateChangeMsg->port == PPFD_BOOT_PORT_NUMBER) && (pStateChangeMsg->encl == PPFD_BOOT_ENCLOSURE_NUMBER )
           && (pStateChangeMsg->drive >= 0) && ( pStateChangeMsg->drive < TOTALDRIVES)) //Drive location check.
        )
    {
        if( ppfdGetSpId() == SP_A ){
            if ((pStateChangeMsg->drive == PPFD_SPA_PRIMARY_SLOT_NUMBER) ||
                (pStateChangeMsg->drive == PPFD_SPA_SECONDARY_SLOT_NUMBER))
            {
                CurrentSpBootDrive = TRUE;
            }
        }else{
            if ((pStateChangeMsg->drive == PPFD_SPB_PRIMARY_SLOT_NUMBER) ||
                (pStateChangeMsg->drive == PPFD_SPB_SECONDARY_SLOT_NUMBER))
            {
                CurrentSpBootDrive = TRUE;
            }
        }        
    }

    return( CurrentSpBootDrive);
 }

 /*******************************************************************************************

 Function:
    BOOL ppfdIsStateChangeMsgForBootEnclosure( update_object_msg_t *pStateChangeMsg )
   
 Description:
    Checks to see if the state change message is for enclosure 0, port 0.  An object type of
    FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE is what we are interested in.

    returns TRUE if it is, and FALSE otherwise.

 Arguments:
    pStateChangeMsg - Pointer to the "State Change Message" to check

 Return Value:
    TRUE - The msg is for a state change of the "boot" encl
    FALSE - The msg is not for a state change of the "boot" encl

 Others:   

 Revision History:

 *******************************************************************************************/
 BOOL ppfdIsStateChangeMsgForBootEnclosure(  update_object_msg_t *pStateChangeMsg )
 {
    if( (pStateChangeMsg->port == PPFD_BOOT_PORT_NUMBER) && (pStateChangeMsg->encl ==  PPFD_BOOT_ENCLOSURE_NUMBER)&& 
        (pStateChangeMsg->notification_info.object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
 }

/*******************************************************************************************

 Function:
    BOOL  ppfdPhysicalPackageInitCrashDump( void );

 Description:
    Initialize the crash dump support for SAS/Sata drives.  The SAS miniport does not know what
    drives are the boot drives (which is where the crash dump file must be written by the SAS
    miniport).  So, PPFD will get the information (slot to phy mapping) that the miniport needs from 
    the PhysicalPackage and send that info (via an IOCTL) to the miniport.

    This should not be called until the PP has completed discovery/enumeration of the Enclosure and
    drives.  Without a valid ObjId for the enclosure, then it cannot get the slot to phy mapping from 
    PP...so it cannot succeed unless the enlcosure has been discovered.

 Arguments:
    none

 Return Value:

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdPhysicalPackageInitCrashDump( void )
{
    fbe_status_t status = FBE_STATUS_OK;
    UINT_8E primary_phy_id = 0xFF;
    UINT_8E secondary_phy_id = 0xFF;
    EMCPAL_STATUS ntStatus;
    BOOL bStatus = TRUE;
    fbe_object_id_t         pdo_object_id;
    fbe_u64_t               primary_encl_sas_address = 0xFFFFFFFF;
    fbe_u64_t               secondary_encl_sas_address = 0xFFFFFFFF;
    fbe_drive_phy_info_t    drive_phy_info;
    

    // We want the phy_id for drives 0-3
    // We'll send the phy_id to the SAS miniport based on which SP we are. That is, phy_id for drives 0 and 2 sent
    // to miniport for SPA, and phy_id for drives 1 and 3 for SPB.
    //

    if( ppfdGetSpId() == SP_A ){
        //Get the mapping for drives 0 and 2 for SPA
        ppfdGetPPDiskObjIdInMap(PPFD_SPA_PRIMARY_SLOT_NUMBER,&pdo_object_id);
        if (FBE_OBJECT_ID_INVALID != pdo_object_id){
            fbe_zero_memory(&drive_phy_info, sizeof(drive_phy_info));
            status = fbe_api_physical_drive_get_phy_info(pdo_object_id, &drive_phy_info);
            if ((FBE_STATUS_OK == status) &&
                (drive_phy_info.enclosure_number == PPFD_BOOT_ENCLOSURE_NUMBER) &&
                (drive_phy_info.slot_number      == PPFD_SPA_PRIMARY_SLOT_NUMBER)){
                   primary_encl_sas_address = drive_phy_info.expander_sas_address;
                   primary_phy_id           = drive_phy_info.phy_id;
            }
        }

        ppfdGetPPDiskObjIdInMap(PPFD_SPA_SECONDARY_SLOT_NUMBER,&pdo_object_id);
        if (FBE_OBJECT_ID_INVALID != pdo_object_id){
            fbe_zero_memory(&drive_phy_info, sizeof(drive_phy_info));
            status = fbe_api_physical_drive_get_phy_info(pdo_object_id, &drive_phy_info);
            if ((FBE_STATUS_OK == status) &&
                (drive_phy_info.enclosure_number == PPFD_BOOT_ENCLOSURE_NUMBER) &&
                (drive_phy_info.slot_number      == PPFD_SPA_SECONDARY_SLOT_NUMBER)){
                secondary_encl_sas_address = drive_phy_info.expander_sas_address;
                secondary_phy_id           = drive_phy_info.phy_id;
            }
        }

    }else if( ppfdGetSpId() == SP_B ){
        //Get the mapping for drives 1 and 3 for SPB

        ppfdGetPPDiskObjIdInMap(PPFD_SPB_PRIMARY_SLOT_NUMBER,&pdo_object_id);
        if (FBE_OBJECT_ID_INVALID != pdo_object_id){
            fbe_zero_memory(&drive_phy_info, sizeof(drive_phy_info));
            status = fbe_api_physical_drive_get_phy_info(pdo_object_id, &drive_phy_info);
            if ((FBE_STATUS_OK == status) &&
                (drive_phy_info.enclosure_number == PPFD_BOOT_ENCLOSURE_NUMBER) &&
                (drive_phy_info.slot_number      == PPFD_SPB_PRIMARY_SLOT_NUMBER)){
                   primary_encl_sas_address = drive_phy_info.expander_sas_address;
                   primary_phy_id           = drive_phy_info.phy_id;
            }
        }

        ppfdGetPPDiskObjIdInMap(PPFD_SPB_SECONDARY_SLOT_NUMBER,&pdo_object_id);
        if (FBE_OBJECT_ID_INVALID != pdo_object_id){
            fbe_zero_memory(&drive_phy_info, sizeof(drive_phy_info));
            status = fbe_api_physical_drive_get_phy_info(pdo_object_id, &drive_phy_info);
            if ((FBE_STATUS_OK == status) &&
                (drive_phy_info.enclosure_number == PPFD_BOOT_ENCLOSURE_NUMBER) &&
                (drive_phy_info.slot_number      == PPFD_SPB_SECONDARY_SLOT_NUMBER)){
                secondary_encl_sas_address = drive_phy_info.expander_sas_address;
                secondary_phy_id           = drive_phy_info.phy_id;
            }
        }

    }else{
        // Invalid!
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s Invalid SP ID.\n",__FUNCTION__);
    }

    if( PPFD_GDS.Be0ScsiPortObject )
    {        
        if ((secondary_phy_id != 0xFF) || (primary_phy_id != 0xFF)){//Do not send IOCTL if both phy_ids are invalid.            
            ntStatus = ppfdSendCPDIoctlSetSystemDevices( PPFD_GDS.Be0ScsiPortObject,
                                                           primary_phy_id, primary_encl_sas_address,
                                                           secondary_phy_id, secondary_encl_sas_address);

            if (EMCPAL_IS_SUCCESS(ntStatus)){
                PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: %s Sent system drive info IOCTL to miniport 0x%x 0x%x .\n", __FUNCTION__,
                                primary_phy_id, secondary_phy_id);
            }else{
                PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s Failed to send IOCTL to miniport ntStatus= 0x%x \n", __FUNCTION__,ntStatus );
                bStatus = FALSE;
            }
        }
    }
    return ( bStatus );
}


/*******************************************************************************************

 Function:
    BOOL ppfdPhysicalPackageDeRegisterCallback( BOOL(*callback_function)(void *, CPD_EVENT_INFO*),
                                                void * registrant_context )
  
 Description:
    This function handles the de-registration of Client callbacks with PPFD.  The CPD_IOCTL_REGISTER_CALLBACK
    sent by NtMirror/ASIDC is "intercepted" by PPFD (if SAS/SATA backend, for FC the IOCTL is forwarded
    to the Scisport/miniport).  

 Arguments:
 
 Return Value:
    TRUE - Deregistration completed successfully
    FALSE - Deregistration failed

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdPhysicalPackageDeRegisterCallback(S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl)
{
    BOOL bStatus=TRUE;
    UCHAR Entry;

    if (pCPDRegisterCallbacksIoctl == NULL )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageDeRegisterCallback Failed NULL callback_function!\n" );            
        return( FALSE );
    }

    // If no callbacks have been registered then return failure
    if( !ppfdCallbackRegistrationInfo.NumRegisteredCallbacks )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageDeRegisterCallback Failed - No callbacks are registered!\n" );            
        return( FALSE );
    }


    for( Entry = 0; Entry < PPFD_MAX_CLIENTS; Entry++ )
    {
        // If there is a matching callback function registered..remove it
        if( ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].callback_handle == 
                                                pCPDRegisterCallbacksIoctl->param.callback_handle)
        {

            ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].async_callback = NULL;
            ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].context = NULL;
            ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].callback_handle = NULL;
            ppfdCallbackRegistrationInfo.NumRegisteredCallbacks--;
            break;
        }
    }

    if (Entry == PPFD_MAX_CLIENTS){
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s no match found NumRegistered=%d\n", 
                        __FUNCTION__,ppfdCallbackRegistrationInfo.NumRegisteredCallbacks);
        bStatus=FALSE;
    }else{            
        PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: %s success NumRegistered=%d\n", 
                        __FUNCTION__,ppfdCallbackRegistrationInfo.NumRegisteredCallbacks);        
        bStatus=TRUE;
    }
    return( bStatus );
}


/*******************************************************************************************

 Function:
    BOOL ppfdPhysicalPackageRegisterCallback( BOOL(*callback_function)(void *, CPD_EVENT_INFO*),
                                                void * registrant_context )
  
 Description:
    This function handles the registration of Client callbacks with PPFD.  The CPD_IOCTL_REGISTER_CALLBACK
    sent by NtMirror/ASIDC is "intercepted" by PPFD (if SAS/SATA backend, for FC the IOCTL is forwarded
    to the Scisport/miniport).  This function handles the registration by saving the callback function
    address into the global  structure "ppfdCallbackRegistrationInfo".

    It will support a maximum of 2 callbacks registered because that is all that should be required. It
    can easily be expanded by changing the value of PPFD_MAX_CLIENTS.  If the maximum number have already
    been registered, then an error is returned (FALSE)

 Arguments:
 
 Return Value:
    TRUE - Registration completed successfully
    FALSE - Regsitration failed..either because of invlaid parameters passed, or the maximum number
        of callbacks have already been registered

 Others:   

 Revision History:

 *******************************************************************************************/
BOOL ppfdPhysicalPackageRegisterCallback(S_CPD_REGISTER_CALLBACKS *pCPDRegisterCallbacksIoctl)    
{
    //pCPDRegisterCallbacksIoctl->param.async_callback, pCPDRegisterCallbacksIoctl->param.context 
    BOOL bStatus=TRUE;
    ULONG  Entry;

    if (pCPDRegisterCallbacksIoctl->param.async_callback == NULL)
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageRegisterCallback Failed NULL callback_function!\n" );            
        return( FALSE );
    }

    // Need to verify we can support another entry
    if( ppfdCallbackRegistrationInfo.NumRegisteredCallbacks < PPFD_MAX_CLIENTS )
    {
        // Insert the callback in an empty entry
        for( Entry = 0; Entry < PPFD_MAX_CLIENTS; Entry++ )
        {
            if( ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].async_callback == NULL )
            {
                // This entry is avaliable, save out callback data here and get out
                ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].async_callback = 
                                                pCPDRegisterCallbacksIoctl->param.async_callback;
                ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].context = 
                                                pCPDRegisterCallbacksIoctl->param.context;
                pCPDRegisterCallbacksIoctl->param.callback_handle = (void *)((UINT_PTR)(PPFD_REGISTER_CALLBACK_HANDLE_PREFIX | Entry));
                ppfdCallbackRegistrationInfo.RegisteredCallbacks[ Entry ].callback_handle =
                                                pCPDRegisterCallbacksIoctl->param.callback_handle;
                ppfdCallbackRegistrationInfo.NumRegisteredCallbacks++;
                break;
            }
        }

        if (Entry == PPFD_MAX_CLIENTS){
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageRegisterCallback no empty slot NumRegistered=%d\n", ppfdCallbackRegistrationInfo.NumRegisteredCallbacks);
            bStatus=FALSE;
        }else{            
            PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdPhysicalPackageRegisterCallback success NumRegistered=%d\n", ppfdCallbackRegistrationInfo.NumRegisteredCallbacks);        
            bStatus=TRUE;
        }
    }
    else
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageRegisterCallback Failed NumRegistered=%d\n", ppfdCallbackRegistrationInfo.NumRegisteredCallbacks);        
        bStatus=FALSE;
    }

    return( bStatus );
}

/*******************************************************************************************
 
 BOOL ppfdPhysicalPackageGetDiskObjIDFromLoopID( ULONG LoopID )

*******************************************************************************************/
EMCPAL_LOCAL BOOL
ppfdPhysicalPackageGetDiskObjIDFromLoopID( ULONG LoopId,
                                           fbe_object_id_t *pDiskObjID )
{
    UCHAR DiskNum;
    BOOL bStatus = FALSE;
    fbe_object_id_t DiskObjID = FBE_OBJECT_ID_INVALID;


    // The LoopId will either be from 0-4 (Non Aliased ASIDC) or from 128-131 (NtMirror)
    // All we care about is which disk it is.  The target ID will tell us which disk, and we can use that to lookup
    // the disk object ID from out local map.  We can extract the target ID using the CPD macro.
    DiskNum = CPD_GET_TARGET_FROM_CONTEXT( LoopId );
    if( DiskNum >= TOTALDRIVES )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdPhysicalPackageGetDiskObjIDFromLoopID error getting Target ID\n" );        
        bStatus = FALSE;
    }
    else
    {
        ppfdGetPPDiskObjIdInMap( DiskNum, &DiskObjID );
        *pDiskObjID = DiskObjID;
        bStatus = TRUE;
    }
    return( bStatus );
}

/*******************************************************************************************
 Function: 
    ULONG ppfdPhysicalPackageGetLoopIDFromMap( ULONG Drive, BOOL IsPnPDisk )
 
 Description:
    Reads the Path ID and Target ID from ppfdDiskObjectsMap for the specified Drive, and 
    translates into a LoopID value using the Path/Target ID (uses the macro 
    CPD_GET_LOGIN_CONTEXT_FROM_PATH_TARGET_ULONG to translate.

 Arguments:
    Drive - The boot drive to get the LoopID for (0-4)
    IsPnPDisk - If True, get the Path ID/Target Id from "ScsiAddrPPFDpnpDisk" member in the map. If false
            get the Path ID/Target Id from "ScsiAddrPPFDDisk" member in the map.

 Return Value:
    LoopID 

*******************************************************************************************/
ULONG ppfdPhysicalPackageGetLoopIDFromMap( ULONG Drive, BOOL IsPnPDisk )
{
    ULONG LoopId;
    UCHAR PId;
    UCHAR TId;

    // Derive the LoopID value from the path ID/target ID for this drive that is in our map
    // The drive is the

    if( Drive  >= TOTALDRIVES )
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdGetLoopID Invalid paramater Drive=%d\n", Drive);        
        return( 0 );
    }

    // For NtMirror pnp Drive
    if( IsPnPDisk )
    {
        PId = PPFD_GDS.PPFDDiskObjectsMap[ Drive ].ScsiAddrPPFDpnpDisk.PathId;
        TId = PPFD_GDS.PPFDDiskObjectsMap[ Drive ].ScsiAddrPPFDpnpDisk.TargetId;
    }
    else
    {
        PId = PPFD_GDS.PPFDDiskObjectsMap[ Drive ].ScsiAddrPPFDDisk.PathId;
        TId = PPFD_GDS.PPFDDiskObjectsMap[ Drive ].ScsiAddrPPFDDisk.TargetId;
    }

    LoopId = CPD_GET_LOGIN_CONTEXT_FROM_PATH_TARGET_ULONG(PId, TId );
    return( LoopId );
}

/*******************************************************************************************

 Function:
    BOOL ppfdPhysicalPackageWaitForDriveReady( S_CPD_CONFIG *pCPDIoctl )
 
 Description:
    Checks the ppfdDiskObjectsMap to see if the Disk OBject ID is non zero.  Keeps checking 
    every 500ms until either the Disk Object ID chanegs to a non zero value (this will happen
    when the callback we have registered with PhysicalPackage to get asycnhronous disk events
    is called with a notification that this disk is ready).  Continues to wait until
    RETRY_GET_PP_DISK_OBJ_ID is reached (this is a multiple of 500ms).

 Arguments:
 
 Return Value:
    TRUE - Drive is ready
    FALSE - Drive is not ready
    

 Others:   

 Revision History:

 *******************************************************************************************/

BOOL ppfdPhysicalPackageWaitForDriveReady( S_CPD_PORT_LOGIN *pCPDLoginIoctl )
{
    ULONG LoopID;
    BOOL bStatus;
    fbe_object_id_t DiskObjID = FBE_OBJECT_ID_INVALID;
    BOOL bDriveReady = FALSE;
    EMCPAL_LARGE_INTEGER sleepVal;
    UCHAR Retries, MaxRetries;
 
    if( pCPDLoginIoctl == NULL )
    {
        return FALSE;
    }

    MaxRetries = ((pCPDLoginIoctl->param.flags & CPD_LOGIN_BOOT_DEVICE) == CPD_LOGIN_BOOT_DEVICE) ?
        RETRY_GET_PP_DISK_OBJ_ID : 1;

    // Wait for PP to set the lifecycle state of this drive to ready
    sleepVal.QuadPart =(__int64)(-1 * (10000 * WAIT_FOR_PP_MS ));	

    LoopID = (ULONG)(ULONG_PTR) pCPDLoginIoctl->param.miniport_login_context;
    for( Retries = MaxRetries; Retries > 0; Retries-- )
    {
        bStatus = ppfdPhysicalPackageGetDiskObjIDFromLoopID( LoopID, &DiskObjID );
        if( bStatus )
        {
            // If it's 0, then the disk object ID has not been set in the map yet in the PPFD callback that is called
            // when PhysicalPackage has set the drive state to ready.  Waity and try again.
            if( DiskObjID == FBE_OBJECT_ID_INVALID)
            {
                 // Delay for a bit before trying again
                EmcpalThreadSleep(EMCPAL_NT_RELATIVE_TIME_TO_MSECS(&sleepVal));
            }
            else
            {
                bDriveReady = TRUE;
                break;
            }
        }
        else
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdPhysicalPackageWaitForDriveReadyppfdGetLoopID Error Getting Disk Obj ID from Map\n");        
        }
    }

    // If we failed to find the drive..bDriveReady will be FALSE here
    return( bDriveReady );
}

/*******************************************************************************************

 Function:
    char *ppfdGetFbeStatusString( fbe_status_t fbe_status_code )        
    

 Description:
    Return a pointer to a text string that represents the FBE_status code that is passed.  This
    function is used for ktrace/debug messages so that the error code can be displayed as a string.

 Arguments:
    fbe_status_code - The FBE_STATUS_XXX code (example FBE_STATUS_INVALID, etc)

 Return Value:
   Pointer to the error code string

 Others:   

 Revision History:

 *******************************************************************************************/
char *ppfdGetFbeStatusString( fbe_status_t fbe_status_code )
{
    char *strStatus = NULL;

    switch (fbe_status_code){
        case FBE_STATUS_INVALID:
            strStatus = "FBE_STATUS_INVALID";
            break;
        case FBE_STATUS_OK:
            strStatus = "FBE_STATUS_OK";
            break;
        case FBE_STATUS_MORE_PROCESSING_REQUIRED:
            strStatus = "FBE_STATUS_MORE_PROCESSING_REQUIRED";
            break;
        case FBE_STATUS_PENDING:
            strStatus = "FBE_STATUS_PENDING";
            break;
        case FBE_STATUS_CANCEL_PENDING:
            strStatus = "FBE_STATUS_CANCEL_PENDING";
            break;
        case FBE_STATUS_CANCELED:
            strStatus = "FBE_STATUS_CANCELED";
            break;
        case FBE_STATUS_TIMEOUT:
            strStatus = "FBE_STATUS_TIMEOUT";
            break;
        case FBE_STATUS_INSUFFICIENT_RESOURCES:
            strStatus = "FBE_STATUS_INSUFFICIENT_RESOURCES";
            break;
        case FBE_STATUS_NO_DEVICE:
            strStatus = "FBE_STATUS_NO_DEVICE";
            break;
        case FBE_STATUS_NO_OBJECT:
            strStatus = "FBE_STATUS_NO_OBJECT";
            break;
        case FBE_STATUS_BUSY:
            strStatus = "FBE_STATUS_BUSY";
            break;
        case FBE_STATUS_FAILED:
            strStatus = "FBE_STATUS_FAILED";
            break;
        case FBE_STATUS_DEAD:
            strStatus = "FBE_STATUS_DEAD";
            break;
        case FBE_STATUS_NOT_INITIALIZED:
            strStatus = "FBE_STATUS_NOT_INITIALIZED";
            break;
        case FBE_STATUS_TRAVERSE:
            strStatus = "FBE_STATUS_TRAVERSE";
            break;
        case FBE_STATUS_EDGE_NOT_ENABLED:
            strStatus = "FBE_STATUS_EDGE_NOT_ENABLED";
            break;
        case FBE_STATUS_COMPONENT_NOT_FOUND:
            strStatus = "FBE_STATUS_COMPONENT_NOT_FOUND";
            break;
        case FBE_STATUS_ATTRIBUTE_NOT_FOUND:
            strStatus = "FBE_STATUS_ATTRIBUTE_NOT_FOUND";
            break;
        case FBE_STATUS_GENERIC_FAILURE:
            strStatus = "FBE_STATUS_GENERIC_FAILURE";
            break;
        default:
            strStatus = "Unknown Error!!";
            break;
    }

    return strStatus;
}

/*******************************************************************************************

 Function:
    char *ppfdGetFbeBlockOperationStatusString( fbe_payload_block_operation_status_t block_status )
    

 Description:
    Return a pointer to a text string that represents the FBE status_code that is passed.  This
    function is used for ktrace/debug messages so that the error code can be displayed as a string.

 Arguments:
    block_status - The FBE block operation status code (example FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID, etc.)

 Return Value:
   Pointer to the error code string

 Others:   

 Revision History:

 *******************************************************************************************/
char *ppfdGetFbeBlockOperationStatusString( fbe_payload_block_operation_status_t block_status )
{

    char *ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID";

    // The FBE block status codes include negative values.  Easier to use a switch than an array of names that 
    // we index into.

    switch ( block_status )
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR";
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED";
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED";
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY";
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST";
            break;

         default:
            break;
    }

    return( ErrCodeString );

}

/*******************************************************************************************

 Function:
    char *ppfdGetFbeBlockOperationQualifierString( fbe_payload_block_operation_qualifier_t qualifier )
    
 Description:
    Return a pointer to a text string that represents the FBE status code (qualifier) that is passed.  This
    function is used for ktrace/debug messages so that the error code can be displayed as a string.

 Arguments:
    qualifier - The detailed "qualifier" code (example  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID, etc.)

 Return Value:
    Pointer to the error code string

 Others:   

 Revision History:

 *******************************************************************************************/
char *ppfdGetFbeBlockOperationQualifierString( fbe_payload_block_operation_qualifier_t qualifier )
{
    char *ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID";

    // The FBE block status codes include negative values.  Easier to use a switch than an array of names that 
    // we index into.

    switch ( qualifier )
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID";
            break;
        
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED";
            break;
        
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG";
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MINIPORT_ABORTED_IO:
            ErrCodeString = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MINIPORT_ABORTED_IO";
            break;
   
        default:
            break;
    }
    return( ErrCodeString );
}

/*******************************************************************************************

 Function:
    void 
    ppfdConvertTransportStatusToSRBStatus( fbe_status_t transport_status, 
                                         fbe_u32_t    transport_qualifier, UCHAR *SrbStatus_p,EMCPAL_STATUS *ntStatus_p )
    
 Description:
    This function converts this FBE packet status into a "compatible" SRB status (SRB_STATUS_XXXX).  
    
 Arguments:
    transport_status     - FBE packet transport status.
    transport_qualifier  - FBE packet transport status qualifier.
    SrbStatus_p          - Pointer to the SRB Staus.
    ntStatus_p           - EMCPAL_STATUS mapping of error.
 Return Value:
    

 Others:   

 Revision History:

 *******************************************************************************************/
void ppfdConvertTransportStatusToSRBStatus( fbe_status_t transport_status, 
                                         fbe_u32_t    transport_qualifier, UCHAR *SrbStatus_p,EMCPAL_STATUS *ntStatus_p )

{
    UCHAR                                   SrbStatus;
    EMCPAL_STATUS                           ntStatus;

    switch ( transport_status )
    {
        case FBE_STATUS_OK:
            SrbStatus = SRB_STATUS_SUCCESS;
            ntStatus = EMCPAL_STATUS_SUCCESS;
            break;

        case FBE_STATUS_CANCELED:
            SrbStatus = SRB_STATUS_ABORTED;
            ntStatus = EMCPAL_STATUS_IO_TIMEOUT; /* NtMirror/ASIDC does not initiate abort.
                                                    Abort status is mapped to timeout in NtMirror. */
            break;

        case FBE_STATUS_TIMEOUT:
            SrbStatus = SRB_STATUS_TIMEOUT;
            ntStatus = EMCPAL_STATUS_IO_TIMEOUT;
            break;

        case FBE_STATUS_BUSY:
        case FBE_STATUS_SLUMBER:
        case FBE_STATUS_IO_FAILED_RETRYABLE:
            SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            ntStatus = EMCPAL_STATUS_DEVICE_NOT_READY;
            break;

        case FBE_STATUS_EDGE_NOT_ENABLED:
        case FBE_STATUS_COMPONENT_NOT_FOUND:        
            SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            ntStatus = EMCPAL_STATUS_DEVICE_NOT_CONNECTED;
            break;

        case FBE_STATUS_DEAD:
        case FBE_STATUS_NO_DEVICE:
        case FBE_STATUS_NO_OBJECT:
            SrbStatus = SRB_STATUS_NO_DEVICE;
            ntStatus = EMCPAL_STATUS_NO_SUCH_DEVICE;            
            break;

        default:
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s unknown transport_status 0x%x!\n",
                        __FUNCTION__,transport_status);
            SrbStatus = SRB_STATUS_ERROR;
            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
            break;
    }

    // Return it
    *SrbStatus_p = SrbStatus;
    *ntStatus_p = ntStatus;

    return;
}

/*******************************************************************************************

 Function:
    void 
   ppfdConvertBlockOperationStatusToSRBStatus( fbe_payload_ex_t *io_payload_p,
                                                    PSCSI_REQUEST_BLOCK pSrb,EMCPAL_STATUS *ntStatus_p);
 
 Description:
    The "block_status" passed in is of the type "fbe_payload_block_operation_status_t". This function 
    converts this FBE status into a "compatible" SRB status (SRB_STATUS_XXXX).  
    
 Arguments:
    block_operation_p - FBE block operation payload.
    Srb               - Pointer to the SRB
    ntStatus_p        - EMCPAL_STATUS mapping of error.
 Return Value:
    

 Others:   

 Revision History:

 *******************************************************************************************/
void ppfdConvertBlockOperationStatusToSRBStatus( fbe_payload_ex_t *io_payload_p,
                                                    PCPD_SCSI_REQ_BLK pSrb,EMCPAL_STATUS *ntStatus_p)
{
    UCHAR                                   SrbStatus;
    EMCPAL_STATUS                           ntStatus;

    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_scsi_error_code_t                   error_code; 
    fbe_u32_t                               reported_sc_data = 0;
    fbe_lba_t                               lba = 0;
    fbe_u8_t                                AdditionalSenseCode = 0;
    fbe_u8_t                                AdditionalSenseCodeQualifier = 0;
    fbe_u8_t                                SenseKey = 0;
    PSENSE_DATA                             pSenseData = NULL;
    fbe_bool_t                              AutoSenseValid = FBE_FALSE;

    if((pSrb == NULL) || (io_payload_p == NULL)){
        panic(PPFD_IO_INVALID_PARAMETER_IN_COMPLETION,(UINT_PTR)io_payload_p);        
    }

    block_operation_p = fbe_payload_ex_get_block_operation( io_payload_p);
    if( block_operation_p == NULL )
    {
        //This is a bad ..panic here
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdConvertBlockOperationStatusToSRBStatus block_operation_p [ERROR]\n" );        
        panic(PPFD_IO_INVALID_BLOCK_OP_IN_COMPLETION,(UINT_PTR)io_payload_p);
    }

    pSenseData = cpd_os_io_get_sense_buf(pSrb);

    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &qualifier ); 


     if ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) ||
         ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
          (qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED)))
     {
         // Get more details on the specific error
         fbe_payload_ex_get_scsi_error_code(io_payload_p, &error_code);
         fbe_payload_ex_get_sense_data (io_payload_p, &reported_sc_data);

         /* sense_code = (((sense_key << 24) & 0xFF000000) | ((asc << 16) & 0x00FF0000) | ((ascq << 8) & 0x0000FF00));*/

         SenseKey                     = ((reported_sc_data & 0xFF000000) >> 24);
         AdditionalSenseCode          = ((reported_sc_data & 0x00FF0000) >> 16);
         AdditionalSenseCodeQualifier = ((reported_sc_data & 0x0000FF00) >> 8);
         if ((cpd_os_io_get_sense_len(pSrb)>= sizeof(SENSE_DATA)) &&
              (pSenseData != NULL)){
              AutoSenseValid = FBE_TRUE;

              //pSenseData->ErrorCode                    = ;
              pSenseData->Valid                        = 1;
              pSenseData->SenseKey                     = SenseKey;
              pSenseData->AdditionalSenseCode          = AdditionalSenseCode;
              pSenseData->AdditionalSenseCodeQualifier = AdditionalSenseCodeQualifier;
              pSenseData->AdditionalSenseLength        = 0;

              if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) ||
                  ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
                   (qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED)))
              {
                   fbe_payload_ex_get_media_error_lba(io_payload_p,&lba);
                   pSenseData->Information[3] = (fbe_u8_t) (lba & 0x000000FF);
                   pSenseData->Information[2] = (fbe_u8_t)((lba & 0x0000FF00) >> 8);
                   pSenseData->Information[1] = (fbe_u8_t)((lba & 0x00FF0000) >> 16);
                   pSenseData->Information[0] = (fbe_u8_t)((lba & 0xFF000000) >> 24);
                   PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: %s Media Error Remap. LBA: 0x%llx SRB 0x%p.\n", __FUNCTION__, lba, pSrb);
                   
               }
         }
    }

    switch ( block_status )
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            SrbStatus = SRB_STATUS_INVALID_REQUEST;
            ntStatus = EMCPAL_STATUS_NOT_IMPLEMENTED;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            if (qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED){
                SrbStatus = SRB_STATUS_ERROR_RECOVERY;
                ntStatus = EMCPAL_STATUS_SUCCESS;
            }else{
                SrbStatus = SRB_STATUS_SUCCESS;
                ntStatus = EMCPAL_STATUS_SUCCESS;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            SrbStatus = SRB_STATUS_TIMEOUT;
            ntStatus = EMCPAL_STATUS_IO_TIMEOUT;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            SrbStatus = SRB_STATUS_ERROR;
            ntStatus = EMCPAL_STATUS_DATA_ERROR;
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            SrbStatus = SRB_STATUS_ERROR;
            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            SrbStatus = SRB_STATUS_ABORTED;
            ntStatus = EMCPAL_STATUS_REQUEST_ABORTED;
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            SrbStatus = SRB_STATUS_BUSY;
            ntStatus = EMCPAL_STATUS_DEVICE_NOT_READY;
            break;

         case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            SrbStatus = SRB_STATUS_INVALID_REQUEST;
            ntStatus = EMCPAL_STATUS_NOT_IMPLEMENTED;
            break;

         default:
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s unknown block_status 0x%x!\n",
                        __FUNCTION__,block_status);
            SrbStatus = SRB_STATUS_ERROR;
            ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
            break;
    }

    // Return it
    if (AutoSenseValid){
        SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
    }    

    // Return it
    cpd_os_io_set_status(pSrb, SrbStatus);
    if (ntStatus_p != NULL)
    {
        *ntStatus_p = ntStatus;
    }

    return;
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteAdvanceState(PPFD_WRITE_CONTEXT * ioContext,BOOLEAN Success)

 Description:
   IOContext and the previous oprtation status (i.e whether success or failure) is passed in.
   This function advances the state m/c depending on the status of the previous operation.
    
 Arguments:
    ioContext - Context containing the information about the IO.
    Success   - Staus of the previous operation (whether success or failure) to decide on
                the next state.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteAdvanceState(PPFD_WRITE_CONTEXT * ioContext,BOOLEAN Success)
{
    PPFD_WRITE_STATE ppfdNextWriteState;
    PPFD_WRITE_STATUS ppfdWriteStatus = PPFD_WRITE_STATUS_ERROR;
    
    if (ioContext == NULL){
        panic(PPFD_IO_INVALID_PARAMETER,(UINT_PTR)ioContext);        
    }
    
    ASSERT(ioContext->CurrentState < PPFD_WRITE_COMPLETE_STATE);
    if (ioContext->CurrentState >= PPFD_WRITE_INVALID_STATE){
        //This should not happen!
        //Possible corrupt packet!
        ppfdNextWriteState = PPFD_WRITE_INVALID_STATE;
    }else{
        if (Success){
            ppfdNextWriteState = PpfdWriteStateTable[ioContext->CurrentState].NextStateOnSuccess;
        }else{
            ppfdNextWriteState = PpfdWriteStateTable[ioContext->CurrentState].NextStateOnFailure;
        }
    }
    ioContext->CurrentState = ppfdNextWriteState;
    switch (ppfdNextWriteState){
        case PPFD_CHECK_WRITE_EDGE_PRESENT_STATE:
            ppfdWriteStatus = ppfdWriteCheckEdge(ioContext);
            break;
        case PPFD_ALLOCATE_PRE_READ_SGL_STATE:
            ppfdWriteStatus = ppfdWriteAllocatePreReadSGL(ioContext);
            break;
        case PPFD_ALLOCATE_PRE_READ_BLOCKS_STATE:
            ppfdWriteStatus = ppfdWriteAllocatePreReadBlocks(ioContext);
            break;
        case PPFD_INITIALIZE_PRE_READ_SGL_STATE:
            ppfdWriteStatus = ppfdWriteInitializePreReadSGL(ioContext);
            break;
        case PPFD_LOCK_PRE_READ_BLOCKS_STATE:
            ppfdWriteStatus = ppfdWriteLockPreReadBlocks(ioContext);
            break;
        case PPFD_PRE_READ_IO_STATE:
            ppfdWriteStatus = ppfdWriteIssuePreReadRequest(ioContext);
            break;
        case PPFD_PRE_READ_CLEAN_UP_STATE:
            ppfdWriteStatus = ppfdWriteCleanupPreReadIO(ioContext);
            break;
        case PPFD_WRITE_IO_STATE:
            ppfdWriteStatus = ppfdWriteIssueWriteRequest(ioContext);
            break;
        case PPFD_WRITE_COMPLETE_STATE:
            ppfdWriteStatus = ppfdWriteCompleteIO(ioContext);
            break;
        default:
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid state ( %d)in IoContext 0x%p.\n",
                        __FUNCTION__,ioContext->CurrentState,ioContext);
            panic(PPFD_IO_INVALID_STATE,(UINT_PTR)ppfdNextWriteState);
            break;
    };

    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteStartIO(PPFD_WRITE_CONTEXT * ioContext )

 Description:
  Starting point for the write IO.
  An initialized IO context with all required information is passed in as the parameter.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteStartIO(PPFD_WRITE_CONTEXT * ioContext )
{
    fbe_block_transport_negotiate_t negotiate_block_size;
    fbe_status_t fbeStatus;

    //TODO:    
    //Optimize by storing info in per device structure and
    //updating on device change notification.
    fbe_zero_memory(&negotiate_block_size,sizeof(negotiate_block_size));
    fbeStatus = ppfdGetPPDiskNegotiatedBlockSizeFromMap(ioContext->pDiskObjectData,&negotiate_block_size);
    if ( fbeStatus != FBE_STATUS_OK ){
        negotiate_block_size.requested_block_size = PPFD_CLIENT_BLOCK_SIZE;
        negotiate_block_size.requested_optimum_block_size = PPFD_CLIENT_OPTIMIUM_BLOCK_SIZE;
        negotiate_block_size.optimum_block_alignment = PPFD_CLIENT_OPTIMIUM_BLOCK_ALIGNMENT;
        if ((fbeStatus = fbe_api_physical_drive_get_drive_block_size(ioContext->disk_object_id,
                                                                        &negotiate_block_size,
                                                                        NULL,
                                                                        NULL)) == FBE_STATUS_OK)
            

        {
            /* Save information for subsequent IOs.*/
            ppfdSetPPDiskNegotiatedBlockSizeInMap(ioContext->pDiskObjectData,&negotiate_block_size);
        }else{
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block size query failed 0x%x!\n",
                        __FUNCTION__,fbeStatus);
            return (ppfdWriteAdvanceState(ioContext,FALSE));
        }
    }

    if ( fbeStatus == FBE_STATUS_OK ){
        ioContext->negotiate_block_size = negotiate_block_size;
    }

    return (ppfdWriteAdvanceState(ioContext,TRUE));
}


/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteCheckEdge(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Negotiates the optimum block size, calculates mapped lba and edges.
    Transitons to next state depending on edge read requirement.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteCheckEdge(PPFD_WRITE_CONTEXT * ioContext)
{
    BOOLEAN preReadRequired = FALSE;
    fbe_lba_t start_lba;
    fbe_block_count_t block_count;    
    PPFD_WRITE_STATUS ppfdWriteStatus;
    fbe_block_transport_negotiate_t negotiate_block_size;

    start_lba = ioContext->OriginalStartLBA;
    block_count = ioContext->OriginalLength;
    negotiate_block_size = ioContext->negotiate_block_size;

    fbe_logical_drive_get_pre_read_lba_blocks_for_write(negotiate_block_size.block_size,
                                                negotiate_block_size.optimum_block_size,
                                                negotiate_block_size.physical_block_size,
                                                &start_lba,
                                                &block_count);

    if ((start_lba != ioContext->OriginalStartLBA)
        || (block_count != ioContext->OriginalLength)){
            //Store these values in context.            
            ioContext->start_lba = start_lba;
            ioContext->block_count = block_count;
            ioContext->start_aligned = ((ioContext->OriginalStartLBA % negotiate_block_size.optimum_block_size) == 0);
            ioContext->end_aligned = (((ioContext->OriginalStartLBA + ioContext->OriginalLength) % negotiate_block_size.optimum_block_size) == 0);

            preReadRequired = TRUE;
        }

    ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,preReadRequired);
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteAllocatePreReadSGL(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Allocates memory required for issuing pre-read request.
    If the allocation fails, the IO will be failed back to the originator.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   
   TODO: Update this similar to sector locking to PEND IO for memory if
     memory allocation fails.

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteAllocatePreReadSGL(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;    

    ioContext->pre_read_sgl =  ppfdAllocateSGLMemory((SIZE_T)(ioContext->block_count + 1));
    if (ioContext->pre_read_sgl == NULL){
        //TODO: Add to a list for SGL available notification
        //pend for SGL memory.
        //return (PPFD_WRITE_STATUS_PENDING);
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s IoContext 0x%p .Allocation of SGL memory for pre-read failed.\n",
                    __FUNCTION__,ioContext);

        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,FALSE);
    }else{
        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);
    }
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteAllocatePreReadBlocks(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Allocates memory required for issuing pre-read request.
    If the allocation fails, the IO will be failed back to the originator.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   
   TODO: Update this similar to sector locking to PEND IO for memory if
     memory allocation fails.

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteAllocatePreReadBlocks(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;
    ULONG BlocksNeeded = 1;
    void *      dummy_read_block_p = NULL;
    void *      pre_read_begin_edge_block_p = NULL;
    void *      pre_read_end_edge_block_p = NULL;

    dummy_read_block_p = &(ioContext->dummy_read_block);

    if (!ioContext->start_aligned){
        pre_read_begin_edge_block_p = &(ioContext->pre_read_begin_edge_block);
        BlocksNeeded++;
    }
    if (!ioContext->end_aligned){
        pre_read_end_edge_block_p = &(ioContext->pre_read_end_edge_block);
        BlocksNeeded++;
    }

    ppfdWriteStatus = ppfdAllocateBlocks(PPFD_CLIENT_BLOCK_SIZE,pre_read_begin_edge_block_p,dummy_read_block_p,pre_read_end_edge_block_p);
    if (ppfdWriteStatus == PPFD_WRITE_STATUS_SUCCESS){
        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);        
    }else{
        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,FALSE);
    }
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteAllocatePreReadBlocks(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    If SGL memory and block memory allocations are successfull,
    the sgl will be initialized for pre-read operation.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteInitializePreReadSGL(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;    
    fbe_sg_element_t * sgl_ptr;
    ULONG              index,dummy_sgl_count;
    PHYSICAL_ADDRESS   pre_read_begin,pre_read_end,dummy_block;

    sgl_ptr = ioContext->pre_read_sgl;
    if (sgl_ptr == NULL){
        //This should not happen
        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,FALSE);
        return (PPFD_WRITE_STATUS_ERROR);
    }

    dummy_sgl_count = (ULONG)ioContext->block_count;

    if (!ioContext->start_aligned && (ioContext->pre_read_begin_edge_block != NULL)){
        pre_read_begin.QuadPart = EmcpalFindPhysicalAddress(ioContext->pre_read_begin_edge_block);
    }
    
    if (!ioContext->end_aligned && (ioContext->pre_read_end_edge_block != NULL)){
        pre_read_end.QuadPart = EmcpalFindPhysicalAddress(ioContext->pre_read_end_edge_block);
    }

    if (ioContext->dummy_read_block != NULL){
        dummy_block.QuadPart = EmcpalFindPhysicalAddress(ioContext->dummy_read_block);
    }

    if (!ioContext->start_aligned){
        fbe_sg_element_init(sgl_ptr,ioContext->negotiate_block_size.block_size,(void *)pre_read_begin.QuadPart);
        fbe_sg_element_increment(&sgl_ptr);
        dummy_sgl_count--;
    }
    if (!ioContext->end_aligned){
        dummy_sgl_count--;
    }
    if (dummy_sgl_count != 0){
        for (index = 0; index < dummy_sgl_count;index++){
            fbe_sg_element_init(sgl_ptr,ioContext->negotiate_block_size.block_size,(void *)dummy_block.QuadPart);
            fbe_sg_element_increment(&sgl_ptr);
        }
    }
    if (!ioContext->end_aligned){
        fbe_sg_element_init(sgl_ptr,ioContext->negotiate_block_size.block_size,(void *)pre_read_end.QuadPart);
        fbe_sg_element_increment(&sgl_ptr);
    }
    fbe_sg_element_terminate(sgl_ptr);

    ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);
    return (ppfdWriteStatus);
}

static void ppfdWriteSetNtStatus(PPFD_WRITE_CONTEXT * ioContext,EMCPAL_STATUS ntStatus)
{
    ioContext->Status = ntStatus;
    return;
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteIsRangeOverlapped(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Checks whether the mapped region for IO is overlapping with alredy outstanding requests.
    Call should be made with the corresponding lock list spinlock held.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static BOOLEAN ppfdWriteIsRangeOverlapped(PPFD_WRITE_CONTEXT * ioContext)
{
    PEMCPAL_LIST_ENTRY       currentListElement = NULL;
    BOOLEAN           overlap = FALSE;
    PPFD_WRITE_CONTEXT * currentContext = NULL;

    currentListElement = ioContext->ListEntry.Flink;
    while(currentListElement != &(ioContext->ListEntry)){
        currentContext = CONTAINING_RECORD(currentListElement,PPFD_WRITE_CONTEXT,ListEntry);
        if (currentContext->RegionLocked){
            if ((currentContext->start_lba > (ioContext->start_lba + ioContext->block_count - 1)) 
                || (ioContext->start_lba  > (currentContext->start_lba + currentContext->block_count - 1))){
                    overlap = FALSE;
                }else{
                    overlap = TRUE;
                    break;
                }
        }
        currentListElement = currentContext->ListEntry.Flink;
    };
    return (overlap);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteLockRange(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Acquires lock for the region for which the IO is being issued.
    If the lock cannot be acquired, the IO will be pended till the the previous owner
    releases the lock. IO will be retried in another thread when the lock becomes available.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended.

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteLockRange(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;
    BOOLEAN           overlap = FALSE;    
    fbe_spinlock_t    *pSpinLock = NULL;
    PEMCPAL_LIST_ENTRY       pLockListHead = NULL;
    
    if ((ioContext == NULL) || (ioContext->pDiskObjectData == NULL)){
        panic(PPFD_IO_INVALID_PARAMETER,(UINT_PTR)ioContext);
    }
    pSpinLock = &(ioContext->pDiskObjectData->IORegionSpinLock);
    pLockListHead = &(ioContext->pDiskObjectData->WriteLockListHead);

    fbe_spinlock_lock(pSpinLock);
    if (EmcpalIsListEmpty(pLockListHead)){
        EmcpalInsertListTail(pLockListHead,&(ioContext->ListEntry));
        overlap = FALSE;
    }else{
        EmcpalInsertListTail(pLockListHead,&(ioContext->ListEntry));
        overlap = ppfdWriteIsRangeOverlapped(ioContext);
    }
    if (overlap){
        //Pend IO
        ppfdWriteStatus = PPFD_WRITE_STATUS_PENDING;
    }else{
        //Lock held
        ioContext->RegionLocked = TRUE;        
        ppfdWriteStatus = PPFD_WRITE_STATUS_SUCCESS;
    }
    fbe_spinlock_unlock(pSpinLock);
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteUnLockRange(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Releases lock for the region for which the IO is being issued.
    If  another IO was waiting on the lock and if this was the only unsatsfied locking operation 
    for the pended IO, that IO will be given the lock and restarted. IO will be retried in 
    another thread.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success

 Others:   

 Revision History:
 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteUnLockRange(PPFD_WRITE_CONTEXT * ioContext)
{    
    BOOLEAN           overlap = FALSE;
    PEMCPAL_LIST_ENTRY       currentListElement = NULL;
    PPFD_WRITE_CONTEXT * currentContext = NULL;
    fbe_spinlock_t       *pSpinLock = NULL;
    PEMCPAL_LIST_ENTRY        pLockListHead = NULL;
    
    if ((ioContext == NULL) || (ioContext->pDiskObjectData == NULL)){
        panic(PPFD_IO_INVALID_PARAMETER,(UINT_PTR)ioContext);
    }
    pSpinLock = &(ioContext->pDiskObjectData->IORegionSpinLock);
    pLockListHead = &(ioContext->pDiskObjectData->WriteLockListHead);


    if (!ioContext->RegionLocked){
        //Lock was never requested for this IO.
        return (PPFD_WRITE_STATUS_SUCCESS);
    }

    fbe_spinlock_lock(pSpinLock);
    EmcpalRemoveListEntry(&(ioContext->ListEntry));
    ioContext->RegionLocked = FALSE;

    //Try to start any other requests that were waiting for a lock that falls in this range.
    overlap = FALSE;
    currentListElement = pLockListHead->Flink;
    while(currentListElement != pLockListHead){
        currentContext = CONTAINING_RECORD(currentListElement,PPFD_WRITE_CONTEXT,ListEntry);
        if (!currentContext->RegionLocked){
            if ((currentContext->start_lba > (ioContext->start_lba + ioContext->block_count - 1)) 
                || (ioContext->start_lba  > (currentContext->start_lba + currentContext->block_count - 1))){
                    overlap = FALSE;
                }else{
                    overlap = TRUE;
                }
        }
        if (overlap){
            //Compare this renage to other locked ranges
            overlap = ppfdWriteIsRangeOverlapped(currentContext);
            if (!overlap){
                //Acquire the region lock
                currentContext->RegionLocked = TRUE;
                //Queue IO in restart queue.
                ppfdWriteQueueIoForRetry(currentContext);                
                //ppfdWriteStatus = ppfdWriteAdvanceState(currentContext,TRUE);
            }

            //Reset overlap value to FALSE for next iteration
            overlap = FALSE;
        }
        currentListElement = currentContext->ListEntry.Flink;
    };
    fbe_spinlock_unlock(pSpinLock);

    return (PPFD_WRITE_STATUS_SUCCESS);
}


/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteLockPreReadBlocks(PPFD_WRITE_CONTEXT * ioContext)

 Description:
   State function for sector region locking.
    
 Arguments:
    ioContext - Context containing the information about the IO.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteLockPreReadBlocks(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;
    
    ppfdWriteStatus = ppfdWriteLockRange(ioContext);
    if (ppfdWriteStatus == PPFD_WRITE_STATUS_PENDING){
        //IO will be restarted when lock is available.
        return (PPFD_WRITE_STATUS_PENDING);
    }
    
    ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);
    return (ppfdWriteStatus);
}

static PPFD_WRITE_STATUS ppfdWriteQueueIoForRetry(PPFD_WRITE_CONTEXT * ioContext)
{    
    fbe_spinlock_lock(&PPFD_GDS.IoRetryQueueSpinLock);
    EmcpalInsertListHead(&PPFD_GDS.IoRetryListHead,&(ioContext->IoQueueListEntry));
    fbe_spinlock_unlock(&PPFD_GDS.IoRetryQueueSpinLock);
    fbe_rendezvous_event_set(&PPFD_GDS.IoRetryEvent);
    return (PPFD_WRITE_STATUS_SUCCESS);
}

static PPFD_WRITE_STATUS ppfdWriteDeQueueIoForRetry(PPFD_WRITE_CONTEXT ** ioContext)
{    
    PEMCPAL_LIST_ENTRY       currentListElement;
    PPFD_WRITE_CONTEXT * currentContext;

    if (ioContext == NULL){
        return (PPFD_WRITE_STATUS_ERROR);
    }

    fbe_spinlock_lock(&PPFD_GDS.IoRetryQueueSpinLock);
    if (EmcpalIsListEmpty(&PPFD_GDS.IoRetryListHead)){
        currentContext = NULL;
    }else{
        currentListElement = EmcpalRemoveListHead(&PPFD_GDS.IoRetryListHead);
        currentContext = CONTAINING_RECORD(currentListElement,PPFD_WRITE_CONTEXT,IoQueueListEntry);
    }    
    fbe_spinlock_unlock(&PPFD_GDS.IoRetryQueueSpinLock);
    *ioContext = currentContext;

    return (PPFD_WRITE_STATUS_SUCCESS);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteSubmitIORequest(PPFD_WRITE_CONTEXT * ioContext)

 Description:
   Issues IO request to physical package. This is used for both pre-read and write IO.
    
 Arguments:
    ioContext - Context containing the information about the IO.
    PreRead   - TRUE if this is a prepread operation. FALSE for write operation.

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteSubmitIORequest(PPFD_WRITE_CONTEXT * ioContext,BOOLEAN PreRead)
{
    
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t * io_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_sg_element_t * sg_element_p = NULL;
    fbe_payload_pre_read_descriptor_t * pre_read_descriptor_p = NULL;    
    fbe_lba_t start_lba;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_status_t    fbeStatus;
    PPFD_WRITE_STATUS ppfdWriteStatus;
    if (ioContext == NULL){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid ioContext !\n",__FUNCTION__);
        return PPFD_WRITE_STATUS_ERROR;
    }

    packet_p = ioContext->packet;
    if( packet_p == NULL){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Packet !\n",__FUNCTION__);
        ppfdWriteSetNtStatus(ioContext,EMCPAL_STATUS_UNSUCCESSFUL);
        ppfdWriteAdvanceState(ioContext,FALSE);
        return PPFD_WRITE_STATUS_ERROR;
    }
    fbeStatus = fbe_transport_set_packet_priority(packet_p,FBE_PACKET_PRIORITY_URGENT);
    if( fbeStatus != FBE_STATUS_OK ){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Error setting packet pririty! Error: %d\n",
                                                  __FUNCTION__,fbeStatus);
    }
    // Get the payload address from the packet
    io_payload_p = fbe_transport_get_payload_ex ( packet_p );
    if( io_payload_p == NULL ){
        // Failed to get i/o payload, return error
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s get Payload Failed!\n",__FUNCTION__);
        ppfdWriteSetNtStatus(ioContext,EMCPAL_STATUS_UNSUCCESSFUL);
        ppfdWriteAdvanceState(ioContext,FALSE);
        return PPFD_WRITE_STATUS_ERROR;
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation( io_payload_p);
    if( block_operation_p == NULL ){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block_operation_p [ERROR]\n",__FUNCTION__);
        ppfdWriteSetNtStatus(ioContext,EMCPAL_STATUS_UNSUCCESSFUL);
        ppfdWriteAdvanceState(ioContext,FALSE);
        return PPFD_WRITE_STATUS_ERROR;
    }

    if (PreRead){
        sg_element_p = ioContext->pre_read_sgl;
        start_lba = ioContext->start_lba;
        block_count = ioContext->block_count;

        fbe_payload_pre_read_descriptor_set_lba(&ioContext->pre_read_descriptor,start_lba);
        fbe_payload_pre_read_descriptor_set_block_count(&ioContext->pre_read_descriptor,block_count);
        fbe_payload_pre_read_descriptor_set_sg_list(&ioContext->pre_read_descriptor,sg_element_p);

        pre_read_descriptor_p = NULL;
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    }else{
        sg_element_p = (fbe_sg_element_t*)ioContext->OriginalSGL;

        start_lba = ioContext->OriginalStartLBA;
        block_count = ioContext->OriginalLength;
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;

        if (ioContext->pre_read_sgl == NULL){
            pre_read_descriptor_p = NULL;            
        }else{
            pre_read_descriptor_p = &ioContext->pre_read_descriptor;
        }
    }
     // Set the SGL (scatter gather list)..which was passed to us..
    fbeStatus = fbe_payload_ex_set_sg_list( io_payload_p, sg_element_p, 0);
    if( fbeStatus == FBE_STATUS_OK ){
         // fbe_io_memory_set_age_threshold_and_priority()
        fbeStatus = fbe_payload_block_build_operation( block_operation_p, block_opcode,
                                                    start_lba,
                                                    block_count,
                                                    ioContext->negotiate_block_size.block_size,
                                                    ioContext->negotiate_block_size.optimum_block_size,
                                                    pre_read_descriptor_p);

        if( fbeStatus == FBE_STATUS_OK )
        {
            fbe_cpu_id_t cpu_id;

            // Tell PP that the SGL contains physical addresses so it does not try to translate them
            // from virtual to physical.
            fbe_payload_ex_set_attr( io_payload_p, FBE_PAYLOAD_FLAG_USE_PHYS_OFFSET );
            fbe_payload_ex_set_physical_offset( io_payload_p, 0 );

            fbeStatus = fbe_payload_ex_increment_block_operation_level( io_payload_p );

            fbe_transport_get_cpu_id(packet_p, &cpu_id);
            if(cpu_id == FBE_CPU_ID_INVALID) {
                cpu_id = fbe_get_cpu_id();
                fbe_transport_set_cpu_id(packet_p, cpu_id);
            }

            fbeStatus = fbe_api_common_send_io_packet ( packet_p, ioContext->disk_object_id,
                                        ppfdPhysicalPackageWriteCompletionCallback,
					                    ioContext,
					                    NULL,
					                    NULL,
										FBE_PACKAGE_ID_PHYSICAL); 

            if((fbeStatus != FBE_STATUS_OK) && (fbeStatus != FBE_STATUS_PENDING)) {
                PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s fbe_api_common_send_io_packet failed fbeStatus= %s\n",
                                        __FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
            }
            //Since the packet is submitted, we should return STATUS_PENDING
            ppfdWriteStatus = PPFD_WRITE_STATUS_PENDING;
        }else{ // fbe_payload_block_build_operation failed        
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block build operation failed fbeStatus= %s\n",
                                       __FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
            ppfdWriteSetNtStatus(ioContext,EMCPAL_STATUS_UNSUCCESSFUL);
            ppfdWriteStatus = PPFD_WRITE_STATUS_ERROR;
        }
    }else{  //fbe_payload_ex_set_sg_list() failed    
        ppfdWriteSetNtStatus(ioContext,EMCPAL_STATUS_UNSUCCESSFUL);
        ppfdWriteStatus = PPFD_WRITE_STATUS_ERROR;
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s fbe_payload_ex_set_sg_list failed fbeStatus= %s\n",
                                        __FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
        // Free any memory we allocated    
        if(  block_operation_p ){
            // Need to release now that we are done with it
            fbe_payload_ex_release_block_operation( io_payload_p, block_operation_p );
        }    
        // Failed to set the SGL, return error
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s set SGL failed fbeStatus= %s\n",
                                        __FUNCTION__, ppfdGetFbeStatusString( fbeStatus ) );
    }
    
    if (ppfdWriteStatus != PPFD_WRITE_STATUS_PENDING){
        ppfdWriteAdvanceState(ioContext,FALSE);
    }
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteIssuePreReadRequest(PPFD_WRITE_CONTEXT * ioContext)

 Description:
   Issues pre-read IO request to physical package. 
    
 Arguments:
    ioContext - Context containing the information about the IO.    

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteIssuePreReadRequest(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;    

    ppfdWriteStatus = ppfdWriteSubmitIORequest(ioContext,TRUE);    
    if (ppfdWriteStatus == PPFD_WRITE_STATUS_ERROR){
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s IoContext 0x%p .Error submitting pre-read request.\n",
                    __FUNCTION__,ioContext);
        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,FALSE);
    }else{
        ppfdWriteStatus = PPFD_WRITE_STATUS_PENDING;
    }
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteCleanupPreReadIO(PPFD_WRITE_CONTEXT * ioContext)

 Description:
   State function that is called on pre-read completion.
   Re-initializes packet for write operation.
    
 Arguments:
    ioContext - Context containing the information about the IO.    

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteCleanupPreReadIO(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;

    fbe_transport_reuse_packet(ioContext->packet);  /* Initialize for reuse for possible subsequent write.*/
    ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);
    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteIssueWriteRequest(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Initiates write IO to physical package.   
    
 Arguments:
    ioContext - Context containing the information about the IO.    

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteIssueWriteRequest(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;

    ppfdWriteStatus = ppfdWriteSubmitIORequest(ioContext,FALSE);
    if (ppfdWriteStatus == PPFD_WRITE_STATUS_ERROR){
        //Completion function will not be called
        //So we are responsible for responsible for the state transition
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s IoContext 0x%p .Error submitting write request.\n",
                    __FUNCTION__,ioContext);

        ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,FALSE);
    }else{
        //Completion function will be called
        //So completion function is responsible for the state transition
        ppfdWriteStatus = PPFD_WRITE_STATUS_PENDING;
    }

    return (ppfdWriteStatus);
}

/*******************************************************************************************

 Function:
    PPFD_WRITE_STATUS 
    ppfdWriteIssueWriteRequest(PPFD_WRITE_CONTEXT * ioContext)

 Description:
    Completion function for all IOs. This function will be invoked on success and failure.
    All clean up and mapping of IO errors happen in this function.
    IO context is released and the original Irp is completed.
    
 Arguments:
    ioContext - Context containing the information about the IO.    

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static PPFD_WRITE_STATUS ppfdWriteCompleteIO(PPFD_WRITE_CONTEXT * ioContext)
{
    PPFD_WRITE_STATUS ppfdWriteStatus;
    fbe_packet_t * io_packet;
    fbe_payload_ex_t * io_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_scsi_error_code_t error_code; 
    fbe_u32_t reported_sc_data = 0;
    PEMCPAL_IRP    Irp = NULL;
    PEMCPAL_IO_STACK_LOCATION stack = NULL;
    PCPD_SCSI_REQ_BLK pSRB = NULL;
    EMCPAL_STATUS    ntStatus;
    fbe_status_t     transport_status;
    fbe_u32_t        transport_qualifier;


    ppfdWriteStatus = ppfdWriteUnLockRange(ioContext);
    if (ppfdWriteStatus != PPFD_WRITE_STATUS_SUCCESS){
        //This should not happen
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s unlock failed! 0x%p\n",__FUNCTION__,ioContext);
    }

    Irp = ioContext->OriginalIrp;
    stack = EmcpalIrpGetCurrentStackLocation(Irp);
    pSRB = EmcpalExtIrpStackParamScsiSrbGet(stack);
    //1. Map errors and set Srb and Irp info

    io_packet = ioContext->packet;

    transport_status = fbe_transport_get_status_code(io_packet);
    transport_qualifier = fbe_transport_get_status_qualifier(io_packet);

    io_payload_p = fbe_transport_get_payload_ex(io_packet);
    if(io_payload_p == NULL){
        //This is a erroneos  condition ..panic here        
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s IoContext 0x%p .get Payload Failed!\n",
                    __FUNCTION__,ioContext);
        panic(PPFD_IO_INVALID_PAYLOAD_IN_COMPLETION,(UINT_PTR)io_packet);                
    }

    block_operation_p = fbe_payload_ex_get_block_operation( io_payload_p);
    if(block_operation_p == NULL){
        //This is a erroneos  condition ..panic here
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s IoContext 0x%p .block_operation_p [ERROR]\n",
                    __FUNCTION__,ioContext);
        panic(PPFD_IO_INVALID_BLOCK_OP_IN_COMPLETION,(UINT_PTR)io_packet);
    }

    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &qualifier ); 

    PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s block_status=%d - %s\n", 
                                    __FUNCTION__,block_status, ppfdGetFbeBlockOperationStatusString( block_status ));
    if( block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS )
    {
         // Get more details on the specific error
         fbe_payload_ex_get_scsi_error_code(io_payload_p, &error_code);
         fbe_payload_ex_get_sense_data (io_payload_p, &reported_sc_data);
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s qualifier=%d - %s\n",
                        __FUNCTION__,qualifier, ppfdGetFbeBlockOperationQualifierString( qualifier ) );
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s Scsi error code=0x%x, scdata=0x%x\n",
                                        __FUNCTION__,error_code, reported_sc_data );
    }

    //Convert FBE transport status to SRB status
    ppfdConvertTransportStatusToSRBStatus(transport_status,transport_qualifier, &(cpd_os_io_get_status(pSRB)), &ntStatus);

    // Convert the FBE block operation status to an SRB Status    
    if (ntStatus == EMCPAL_STATUS_SUCCESS){
        ppfdConvertBlockOperationStatusToSRBStatus( io_payload_p, pSRB, &ntStatus);
    }

    //set the Scsistatus    
    cpd_os_io_set_scsi_status(pSRB, SCSISTAT_GOOD);

    //2. Release pre-read blocks if any
    ppfdReleaseBlocks(ioContext->pre_read_begin_edge_block,
                       ioContext->dummy_read_block,
                       ioContext->pre_read_end_edge_block);

    //3. Release pre-read SGL if any
    if (ioContext->pre_read_sgl != NULL){
        ppfdFreeSGLMemory(ioContext->pre_read_sgl);
    }

    //4. Release context
    ppfdReleaseIOContext(ioContext);

     //5. Complete Irp
    CompleteRequest( Irp, ntStatus, 0 );

    if(EMCPAL_IS_SUCCESS(ntStatus)){
        ppfdWriteStatus = PPFD_WRITE_STATUS_SUCCESS;
    }else{
        ppfdWriteStatus = PPFD_WRITE_STATUS_ERROR;
    }

    return (ppfdWriteStatus);
}

static EMCPAL_STATUS ppfdPhysicalPackageInitializeIoContext(PPFD_WRITE_CONTEXT * ioContext, ULONG StartLBA, 
                                                ULONG Length, PSGL pSGL, p_PPFDDISKOBJ   pDiskObjectData, PEMCPAL_IRP Irp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;

    if ((ioContext == NULL) || (pSGL == NULL) 
        || (pDiskObjectData == NULL) || (Irp == NULL)){

        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter ioContext=0x%p Irp=0x%p\n", 
                    __FUNCTION__ ,ioContext,Irp);
        return (EMCPAL_STATUS_INVALID_PARAMETER);
    }
    ioContext->CurrentState = PPFD_WRITE_INITIAL_STATE;
    ioContext->OriginalIrp = Irp;
    ioContext->OriginalStartLBA = StartLBA;
    ioContext->OriginalLength = Length;
    ioContext->OriginalSGL = pSGL;
    ioContext->pDiskObjectData = pDiskObjectData;
    ntStatus = ppfdGetPPDiskObjIdInMapEntry(pDiskObjectData,&ioContext->disk_object_id);
    if (!EMCPAL_IS_SUCCESS(ntStatus )){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Error retrieving objID DiskObjPtr=0x%p Irp=0x%p\n", 
                                                    __FUNCTION__ ,pDiskObjectData,Irp);
    }   

    return (ntStatus);
}
EMCPAL_STATUS ppfdPhysicalPackageSendWrite( ULONG StartLBA, ULONG Length, PSGL pSGL, PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, PEMCPAL_IRP Irp )
{
    PPFD_WRITE_CONTEXT * ioContext = NULL;
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_UNSUCCESSFUL;
    PPFD_WRITE_STATUS ppfdWriteStatus;
    fbe_object_id_t DiskObjID = FBE_OBJECT_ID_INVALID;     
    p_PPFDDISKOBJ   pDiskObjectData = NULL;
 
    // Check parameters for errors
    if( (pSGL == NULL) || (Length ==0) || (ppfdDiskDeviceObject == NULL ) ){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Invalid Parameter pSGL= 0x%p,Length= 0x%x, ppfdDiskDeviceObject= 0x%p\n",
                        __FUNCTION__, pSGL, Length, ppfdDiskDeviceObject);
        ntStatus = EMCPAL_STATUS_INVALID_PARAMETER;
        return ntStatus;        
    }

    //
    // Get the PP logical disk device object ID from our local map based on the ppfdDiskDeviceObject
    //

    //NOTE:
    // I am placing the Sector locking queue in this global structure to avoid the issue of
    // a lock held by non-pnp device object for a sector being ignored by an IO to the PNP device object.
    // IO to the PNP device object and non-PNP device object will end up here.
    // So a per device object queue might not be the best idea since an IO from the paralell 
    // device will also end up in here.

    pDiskObjectData = ppfdGetDiskObjIDFromPPFDFido( ppfdDiskDeviceObject, &DiskObjID );
    if(( pDiskObjectData == NULL)||(DiskObjID == FBE_OBJECT_ID_INVALID)){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Drive not in object table! Entry 0x%p ID 0x%x\n",
                            __FUNCTION__,pDiskObjectData,DiskObjID);        
        ntStatus = EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST;
        return ntStatus;
    }

    //Allocate context memory.
    ioContext = ppfdAllocateIOContext();// Memory will be allocated from a look aside list and initilized.
                                        //Memory context to contain context and packet memory.
                                        //Packet ptr in context will point to the packet begginning and will be populated by the allocate()
    if (ioContext == NULL){
        //Error allocating memory
        ntStatus = EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
        return ntStatus;
    }

    ntStatus = ppfdPhysicalPackageInitializeIoContext(ioContext,StartLBA,Length,pSGL,pDiskObjectData,Irp );
    if (ntStatus != EMCPAL_STATUS_SUCCESS){
        ppfdReleaseIOContext(ioContext);
        return (ntStatus);
    }

    //Once the state m/c is started, the completion of IO should happen inside the s/m
    //So the return after this point should be STATUS_PENDING
    ppfdWriteStatus = ppfdWriteStartIO(ioContext);
    if ((ppfdWriteStatus != PPFD_WRITE_STATUS_PENDING) && 
        (ppfdWriteStatus != PPFD_WRITE_STATUS_SUCCESS)){
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s ppfdWriteStartIO returned error Context 0x%p!\n",__FUNCTION__, ioContext);
        }
    return( EMCPAL_STATUS_PENDING );
}


static fbe_status_t ppfdPhysicalPackageWriteCompletionCallback(fbe_packet_t * io_packet, fbe_packet_completion_context_t context)
{
    PPFD_WRITE_CONTEXT * ioContext = (PPFD_WRITE_CONTEXT *)context;
    fbe_payload_ex_t * io_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_scsi_error_code_t error_code; 
    fbe_u32_t reported_sc_data = 0;
    BOOLEAN IOSuccess = TRUE;


    PPFD_TRACE(PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s\n",__FUNCTION__ );

    io_payload_p = fbe_transport_get_payload_ex( io_packet );
    if( io_payload_p == NULL ) 
    {
        //This is a erroneos  condition ..panic here
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s get Payload Failed!\n",__FUNCTION__);
        panic(PPFD_IO_INVALID_PAYLOAD_IN_COMPLETION,(UINT_PTR)io_packet);
                
    }

    block_operation_p = fbe_payload_ex_get_block_operation( io_payload_p);
    if( block_operation_p == NULL )
    {        
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s block_operation_p [ERROR]\n",__FUNCTION__ );
        panic(PPFD_IO_INVALID_BLOCK_OP_IN_COMPLETION,(UINT_PTR)io_packet);
    }

    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &qualifier ); 

    PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s block_status=%d - %s\n",
                    __FUNCTION__,block_status, ppfdGetFbeBlockOperationStatusString( block_status ));
    if( block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS )
    {
         // Get more details on the specific error
         fbe_payload_ex_get_scsi_error_code(io_payload_p, &error_code);
         fbe_payload_ex_get_sense_data (io_payload_p, &reported_sc_data);
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s qualifier=%d - %s\n",
                        __FUNCTION__,qualifier, ppfdGetFbeBlockOperationQualifierString( qualifier ) );
         PPFD_TRACE( PPFD_TRACE_FLAG_IO, TRC_K_STD, "PPFD: %s Scsi error code=0x%x, scdata=0x%x\n",
                        __FUNCTION__,error_code, reported_sc_data );
         IOSuccess = FALSE;
    }

    ppfdWriteAdvanceState(ioContext,IOSuccess);   
    return FBE_STATUS_OK;
}


static PPFD_WRITE_CONTEXT * ppfdAllocateIOContext(void)
{    
    PPFD_WRITE_CONTEXT * ppfdWriteContext = NULL;    
    
    ppfdWriteContext = (PPFD_WRITE_CONTEXT *)EmcpalLookasideAllocate(&PPFD_GDS.IoContextLookasideListHead);
    if (ppfdWriteContext != NULL){
        fbe_zero_memory(ppfdWriteContext,sizeof(PPFD_WRITE_CONTEXT));        
    }
    else
    {
        return NULL;
    }
    ppfdWriteContext->packet = ppfdGetContiguousPacket();
    if (ppfdWriteContext->packet == NULL){
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s Error allocating contiguous packet memoty!\n",__FUNCTION__ );
        ppfdReleaseIOContext(ppfdWriteContext);
        ppfdWriteContext = NULL;
    }
    
    return (ppfdWriteContext);
}

static void ppfdReleaseIOContext(PPFD_WRITE_CONTEXT *ppfdWriteContext)
{    
    if (ppfdWriteContext->packet != NULL){
        ppfdReturnContiguousPacket(ppfdWriteContext->packet);
    }
    EmcpalLookasideFree(&PPFD_GDS.IoContextLookasideListHead,ppfdWriteContext);
}

static fbe_sg_element_t * ppfdAllocateSGLMemory(SIZE_T  NumberOfSGLElements)
{    
    fbe_sg_element_t * sgl_element_p = NULL;
    PUCHAR  pMem = NULL;
    PPFD_SGL_LIST_HEADER *pSglListHeader = NULL;

    //Dynamic allocation
    pMem = (PUCHAR)ppfdAllocateMemory(((NumberOfSGLElements * sizeof(fbe_sg_element_t)) + sizeof(PPFD_SGL_LIST_HEADER)));
    pSglListHeader = (PPFD_SGL_LIST_HEADER *)pMem;
    pSglListHeader->PoolID = SGL_LIST_POOL_ID_DYNAMIC;

    pMem = pMem + sizeof(PPFD_SGL_LIST_HEADER);
    sgl_element_p = (fbe_sg_element_t *)pMem;
    return (sgl_element_p);
}

static void ppfdFreeSGLMemory(fbe_sg_element_t *sgl_element_p)
{   

    PUCHAR  pMem = NULL;
    PPFD_SGL_LIST_HEADER *pSglListHeader = NULL;

    pMem = (PUCHAR)sgl_element_p;
    pMem = pMem - sizeof(PPFD_SGL_LIST_HEADER);
    pSglListHeader = (PPFD_SGL_LIST_HEADER *)pMem;

    ppfdFreeMemory((void *)pMem);
    return;
}

static PPFD_WRITE_STATUS ppfdAllocateBlocks(ULONG  block_size,
                                     void ** pre_read_begin_edge_block_p,
                                     void ** dummy_read_block_p,
                                     void ** pre_read_end_edge_block_p)
{

    if (pre_read_begin_edge_block_p != NULL){

        *pre_read_begin_edge_block_p = ppfdGetContiguousBlock();
        if (*pre_read_begin_edge_block_p == NULL)
        {
            return (PPFD_WRITE_STATUS_ERROR);
        }
    }

    if (dummy_read_block_p  != NULL){
        *dummy_read_block_p = ppfdGetContiguousBlock();
        if (*dummy_read_block_p == NULL)
        {
            if (pre_read_begin_edge_block_p != NULL){
                ppfdReturnContiguousBlock(*pre_read_begin_edge_block_p);
                *pre_read_begin_edge_block_p = NULL;
            }
            return (PPFD_WRITE_STATUS_ERROR);
        }
    }

    if (pre_read_end_edge_block_p != NULL){
        *pre_read_end_edge_block_p = ppfdGetContiguousBlock();
        if (*pre_read_end_edge_block_p == NULL)
        {
            if (pre_read_begin_edge_block_p != NULL){
                ppfdReturnContiguousBlock(*pre_read_begin_edge_block_p);
                *pre_read_begin_edge_block_p = NULL;
            }

            if (dummy_read_block_p != NULL){
                ppfdReturnContiguousBlock(*dummy_read_block_p);
                *dummy_read_block_p = NULL;
            }

            return (PPFD_WRITE_STATUS_ERROR);
        }
    }

    return (PPFD_WRITE_STATUS_SUCCESS);
}

static void ppfdReleaseBlocks(void * pre_read_begin_edge_block_p,
                 void * dummy_read_block_p,
                 void * pre_read_end_edge_block_p)
{
    if (pre_read_begin_edge_block_p != NULL){        
        ppfdReturnContiguousBlock(pre_read_begin_edge_block_p);
    }
    if (dummy_read_block_p != NULL){
        ppfdReturnContiguousBlock(dummy_read_block_p);        
    }
    if (pre_read_end_edge_block_p != NULL){        
        ppfdReturnContiguousBlock(pre_read_end_edge_block_p);
    }

    return;
}

/*******************************************************************************************

 Function:
    void 
     ppfdWriteIoRetryThread( void *Context ) 

 Description:
    Retry thread function for restarting IO that was pended earlier.
    
 Arguments:
     

 Return Value:
    PPFD_WRITE_STATUS - Success, Failure or Pended

 Others:   

 Revision History:

 *******************************************************************************************/

static void ppfdWriteIoRetryThread( void *Context ) 
{ 
    PPFD_WRITE_CONTEXT *ioContext = NULL;
    PPFD_WRITE_STATUS  ppfdWriteStatus;

    (void) EmcPalSetThreadToDataPathPriority();

    while(TRUE){
        fbe_rendezvous_event_wait(&PPFD_GDS.IoRetryEvent, 0);
        fbe_rendezvous_event_clear(&PPFD_GDS.IoRetryEvent);
        do{
            ppfdWriteDeQueueIoForRetry(&ioContext);            
            if (ioContext != NULL){
                ppfdWriteStatus = ppfdWriteAdvanceState(ioContext,TRUE);
            }
        }while((ioContext != NULL));

        if (PPFD_GDS.AbortIoRetryThread){
            PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: %s Thread terminating.\n",__FUNCTION__);
            break;
        }
    }    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

EMCPAL_STATUS ppfdWriteStartIoRestartThread(void)
{
    EMCPAL_STATUS ntStatus;

    ntStatus = fbe_thread_init(&PPFD_GDS.IoRetryThreadHandle,  "ppfd_ioretry",
                               (EMCPAL_THREAD_START_FUNCTION) ppfdWriteIoRetryThread,
                                (void*) NULL);
    if (EMCPAL_IS_SUCCESS(ntStatus)){
       PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: %s created thread successfully\n",__FUNCTION__);
    }else{
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s: Create thread failed! ntStatus=0x%x\n",__FUNCTION__, ntStatus );
        panic(PPFD_FAILED_CREATE_THREAD,(UINT_PTR)NULL);
    }
    return( ntStatus );
}

static fbe_status_t ppfdInitContiguousPacketQueue(void)
{
	fbe_u32_t 						count = 0;
	ppfd_packet_q_element_t *	packet_q_element = NULL;
	fbe_status_t					status;

	fbe_queue_init(&ppfd_contiguous_packet_q_head);
	fbe_spinlock_init(&ppfd_contiguous_packet_q_lock);

	for (count = 0; count < PPFD_PRE_ALLOCATED_CONTIG_PACKETS; count ++) {
		packet_q_element = fbe_api_allocate_contiguous_memory(sizeof(ppfd_packet_q_element_t));
		if (packet_q_element == NULL) {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s: can't allocated contig. packets\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;
		}

		fbe_zero_memory(packet_q_element, sizeof(ppfd_packet_q_element_t));
		fbe_queue_element_init(&packet_q_element->q_element);
		status = fbe_transport_initialize_packet(&packet_q_element->packet);
		if (status != FBE_STATUS_OK) {
			PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s: can't init packet\n", __FUNCTION__); 
            fbe_api_free_contiguous_memory(packet_q_element);
			return FBE_STATUS_GENERIC_FAILURE;/*potenital leak but this is very bad anyway*/
		}

		fbe_queue_push(&ppfd_contiguous_packet_q_head, &packet_q_element->q_element);
	}

	return FBE_STATUS_OK;
}

static fbe_status_t ppfdDestroyContiguosPacketQueue(void)
{
	fbe_u32_t 	count = 0;
	ppfd_packet_q_element_t *	packet_q_element = NULL;

	fbe_spinlock_destroy(&ppfd_contiguous_packet_q_lock);

	for (count = 0; count < PPFD_PRE_ALLOCATED_CONTIG_PACKETS; count ++) {
		if (fbe_queue_is_empty(&ppfd_contiguous_packet_q_head)) {
			PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: there are some outstanding packets\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;
		}

        packet_q_element = (ppfd_packet_q_element_t *)fbe_queue_pop(&ppfd_contiguous_packet_q_head);
		fbe_transport_destroy_packet(&packet_q_element->packet);
		fbe_api_free_contiguous_memory(packet_q_element);
	}
	
	

	return FBE_STATUS_OK;

}


static fbe_packet_t * ppfdGetContiguousPacket()
{
	fbe_packet_t *packet = NULL;
	ppfd_packet_q_element_t *packet_q_element = NULL;

    fbe_spinlock_lock(&ppfd_contiguous_packet_q_lock);
	if (!fbe_queue_is_empty(&ppfd_contiguous_packet_q_head)) {		
		packet_q_element = (ppfd_packet_q_element_t *)fbe_queue_pop(&ppfd_contiguous_packet_q_head);
		packet = &packet_q_element->packet;
		fbe_spinlock_unlock(&ppfd_contiguous_packet_q_lock);
	}else{
        fbe_spinlock_unlock(&ppfd_contiguous_packet_q_lock);
		PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: pre allocated queue is empty !!!\n", __FUNCTION__);
	}

	return packet;

}
#define PPFD_CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))

static void ppfdReturnContiguousPacket (fbe_packet_t *packet)
{
	/*for now it's easy, the queue elment starts before the packet*/
	/*fbe_queue_element_t *q_element  = (fbe_queue_element_t *)((fbe_u8_t *)((fbe_u8_t *)packet - sizeof(fbe_queue_element_t)));*/
    fbe_queue_element_t *q_element  = (fbe_queue_element_t *)(PPFD_CONTAINING_RECORD(packet,ppfd_packet_q_element_t,packet));

	fbe_transport_reuse_packet(packet);

    fbe_spinlock_lock(&ppfd_contiguous_packet_q_lock);
	fbe_queue_push(&ppfd_contiguous_packet_q_head, q_element);
	fbe_spinlock_unlock(&ppfd_contiguous_packet_q_lock);

}


static fbe_status_t ppfdInitContiguousBlockQueue(void)
{
	fbe_u32_t 						count = 0;
	ppfd_block_q_element_t *	    block_q_element = NULL;	

	fbe_queue_init(&ppfd_contiguous_block_q_head);
	fbe_spinlock_init(&ppfd_contiguous_block_q_lock);

    /* TODO: Allocate larger chunks and partition it instead of 600 520byte allocations.*/
	for (count = 0; count < PPFD_PRE_ALLOCATED_CONTIG_PACKETS; count ++) {
		block_q_element = fbe_api_allocate_contiguous_memory(sizeof(ppfd_block_q_element_t));
		if (block_q_element == NULL) {
			PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: %s: can't allocated contig. blocks\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;
		}

		fbe_zero_memory(block_q_element, sizeof(ppfd_block_q_element_t));
		fbe_queue_element_init(&block_q_element->q_element);
		fbe_queue_push(&ppfd_contiguous_block_q_head, &block_q_element->q_element);
	}

	return FBE_STATUS_OK;
}

static fbe_status_t ppfdDestoryContiguosBlockQueue(void)
{
	fbe_u32_t 	count = 0;
	ppfd_block_q_element_t *	    block_q_element = NULL;

	fbe_spinlock_destroy(&ppfd_contiguous_block_q_lock);

	for (count = 0; count < PPFD_PRE_ALLOCATED_CONTIG_PACKETS; count ++) {
		if (fbe_queue_is_empty(&ppfd_contiguous_block_q_head)) {
			PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: there are some outstanding blocks\n", __FUNCTION__); 
			return FBE_STATUS_GENERIC_FAILURE;
		}

        block_q_element = (ppfd_block_q_element_t *)fbe_queue_pop(&ppfd_contiguous_block_q_head);		
		fbe_api_free_contiguous_memory(block_q_element);
    }

	return FBE_STATUS_OK;

}


static fbe_u8_t * ppfdGetContiguousBlock()
{
	fbe_u8_t *block = NULL;
	ppfd_block_q_element_t *block_q_element = NULL;

    fbe_spinlock_lock(&ppfd_contiguous_block_q_lock);

	if (!fbe_queue_is_empty(&ppfd_contiguous_block_q_head)) {		
		block_q_element = (ppfd_block_q_element_t *)fbe_queue_pop(&ppfd_contiguous_block_q_head);
		block = (fbe_u8_t *)(&(block_q_element->block_buffer[0]));
		fbe_spinlock_unlock(&ppfd_contiguous_block_q_lock);
	}else{
        fbe_spinlock_unlock(&ppfd_contiguous_block_q_lock);
		PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: pre allocated queue is empty !!!\n", __FUNCTION__);
	}
	return block;
}

static void ppfdReturnContiguousBlock(fbe_u8_t *block)
{
	/*for now it's easy, the queue elment starts before the packet*/
	/*fbe_queue_element_t *q_element  = (fbe_queue_element_t *)((fbe_u8_t *)((fbe_u8_t *)block - sizeof(fbe_queue_element_t)));*/
    fbe_queue_element_t *q_element  = (fbe_queue_element_t *)(PPFD_CONTAINING_RECORD(block,ppfd_block_q_element_t,block_buffer));

    fbe_spinlock_lock(&ppfd_contiguous_block_q_lock);
	fbe_queue_push(&ppfd_contiguous_block_q_head, q_element);
	fbe_spinlock_unlock(&ppfd_contiguous_block_q_lock);

}
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL; /* FBE30 we may need to define PPFD package */
    return FBE_STATUS_OK;
}



/*************** FBE OBJECT MAP implementation ******/


/*********************************************************************
 *            ppfd_object_map_dispatch_queue ()
 *********************************************************************
 *
 *  Description: deque objects and call the function to change their state *
 *
 *  Return Value: none
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static void ppfd_object_map_dispatch_queue(void)
{
    update_object_msg_t         * update_request = NULL;
    fbe_status_t	            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   drive_slot;
	fbe_lifecycle_state_t 		state;
    
    while (1) {
         fbe_spinlock_lock(&ppfd_object_map_update_queue_lock);
        /*wo we have something on the queue ?*/
        if (!fbe_queue_is_empty(&ppfd_object_map_update_queue_head)) {
            /*get the next item from the queue*/
            update_request = (update_object_msg_t *) fbe_queue_pop(&ppfd_object_map_update_queue_head);
            fbe_spinlock_unlock(&ppfd_object_map_update_queue_lock); 


            if (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {

                state = FBE_LIFECYCLE_STATE_INVALID;
			    status = fbe_notification_convert_notification_type_to_state(update_request->notification_info.notification_type, &state);
			    if (status != FBE_STATUS_OK) {
				     PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD,
					       "PPFD: %s:notification,obj:0x%X can't map state:0x%llX\n",
						    __FUNCTION__, update_request->object_id, update_request->notification_info.notification_type); 

			    }

                switch(state){
                    case FBE_LIFECYCLE_STATE_READY:
                        /* Query location information since this is a new entry for us.*/
                        status = ppfd_get_object_location(update_request);
                        if (status == FBE_STATUS_OK){
                             if( ppfdIsStateChangeMsgForBootDrive(update_request) ){
                                PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD,
                                    "PPFD: READY:notification,slot: %d obj:0x%X is in state:%s\n",
                                                    update_request->drive,
                                                    update_request->object_id, 
							                        fbe_api_state_to_string(state)); 

                                ppfdSetPPDiskObjIdInMap( update_request->drive, update_request->object_id );
                                ppfdHandleAsyncEvent( update_request);
                             }else if (ppfdIsStateChangeMsgForBootEnclosure(update_request)){
                                 ppfdSetEnclObjIdInMap(update_request->object_id );
                             }
                        }
                        break;

                    case FBE_LIFECYCLE_STATE_DESTROY:
                    case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
                    case FBE_LIFECYCLE_STATE_PENDING_FAIL:
                        /* Lookup entry in our table.*/
                        /* If entry is not found, we can ignore this notification.*/
                        status = ppfd_locate_object_location_in_table(update_request,&drive_slot);
                        if (status == FBE_STATUS_OK){
                            /* If found, update table entry and schedule LOG OUT and DEVICE FAILED notifications.*/
                            PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: %s: Using cached info: ObjID 0x%x Slot : %d \n", __FUNCTION__, 
                                                                update_request->object_id,drive_slot);
                            /* Object ID matched an etry in the table and the object is currently in a state
                                where we cannot query it. So try and fill in the blanks using the information
                                we cached earlier.*/
                            update_request->port  = PPFD_BOOT_PORT_NUMBER;
                            update_request->encl  = PPFD_BOOT_ENCLOSURE_NUMBER;
                            update_request->drive = drive_slot;

                            if( ppfdIsStateChangeMsgForBootDrive( update_request ) ){   
                                PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD,
                                    "PPFD: notification,slot: %d obj:0x%X is in state:%s\n",
                                                    update_request->drive,
                                                    update_request->object_id, 
							                        fbe_api_state_to_string(state)); 
                                ppfdSetPPDiskObjIdInMap( update_request->drive, FBE_OBJECT_ID_INVALID );
                                ppfdHandleAsyncEvent( update_request);
                            }else if (ppfdIsStateChangeMsgForBootEnclosure(update_request)){                                         
                                ppfdSetEnclObjIdInMap(FBE_OBJECT_ID_INVALID );
                            }                            
                        }
                        break;

                    case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
                        /* Lookup entry in out table.*/
                        /* If entry is not found, we can ignore this notification.*/
                        status = ppfd_locate_object_location_in_table(update_request,&drive_slot);
                        if (status ==FBE_STATUS_OK){
                            /* If found, schedule LOG OUT notification.*/
                            status = ppfd_get_object_location(update_request);
                            if (status == FBE_STATUS_OK){
                                 if( ppfdIsStateChangeMsgForBootDrive( update_request ) ){   
                                    /* Not marking OBJ ID to invalid. This opens a short window but we 
                                    are sending a LOG OUT to NtMirror/ASIDC in the next step.
                                    It is the responsibility of the above mentioned drivers to hold off IO
                                    once they receive LOG OUT till the subsequent LOG IN notification is received.
                                    */
                                    ppfdHandleAsyncEvent( update_request);
                                 }
                            }
                        }

                        break;
                }
            }

			/*and delete it, we don't need it anymore*/
			ppfdFreeObjectNotificationStruct(update_request);

        } else {
            fbe_spinlock_unlock(&ppfd_object_map_update_queue_lock); 
            break;
        }
    }       
}

/*********************************************************************
 *            ppfd_object_map_thread_func ()
 *********************************************************************
 *
 *  Description: the thread that will dispatch the state changes from the queue
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static void ppfd_object_map_thread_func(void * context)
{
    EMCPAL_STATUS           nt_status;

    FBE_UNREFERENCED_PARAMETER(context);
    
    PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: %s:\n", __FUNCTION__); 

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&ppfd_update_semaphore, NULL);
        if(object_map_thread_flag == OBJECT_MAP_THREAD_RUN) {
            /*check if there are any notifictions waiting on the queue*/
            ppfd_object_map_dispatch_queue();
        } else {
            break;
        }
    }

    PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: %s:\n", __FUNCTION__); 

    object_map_thread_flag = OBJECT_MAP_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*********************************************************************
 *            ppfd_ppfd_object_map_init ()
 *********************************************************************
 *
 *  Description:
 *    This function is used for initializing the map and filling it
 *
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/27/07: sharel   created
 *    
 *********************************************************************/

fbe_status_t ppfd_ppfd_object_map_init (void)
{
    EMCPAL_STATUS        nt_status;
    fbe_status_t        fbe_status;

    if (ppfd_map_interface_initialized == FBE_FALSE) {
        /*initialize a queue of update requests and the lock to it*/
        fbe_queue_init(&ppfd_object_map_update_queue_head);
        fbe_spinlock_init(&ppfd_object_map_update_queue_lock);    
    
        /*initialize the semaphore that will control the thread*/
        fbe_semaphore_init(&ppfd_update_semaphore, 0, 0x7FFFFFFF);   
        
        EmcpalLookasideListCreate(EmcpalDriverGetCurrentClientObject(),
                                  &PPFD_GDS.ObjectMessageNotification,
                                  NULL, NULL, sizeof(update_object_msg_t),
                                  0 /* depth */, 'afpp');

        object_map_thread_flag = OBJECT_MAP_THREAD_RUN;
    
        /*start the thread that will execute updates from the queue*/
        nt_status = fbe_thread_init(&ppfd_object_map_thread_handle, "ppfd_objmap", ppfd_object_map_thread_func, NULL);
    
        if (nt_status != EMCPAL_STATUS_SUCCESS) {
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
            return FBE_STATUS_GENERIC_FAILURE;
        }    
    
        /*register for notification from physical package*/
        fbe_status = ppfd_object_map_register_notification_element();
        if (fbe_status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_status  = ppfd_object_map_update_all();/*fill the map with objects*/ /*fbe_api_get_logical_drive_object_id_by_location()*/
        if (fbe_status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD,  "PPFD: %s completed - Map is initialized !\n", __FUNCTION__); 
        ppfd_map_interface_initialized = TRUE;
        return FBE_STATUS_OK;
    }else {
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: %s Map is already initialized, can't do it twice w/o destroy\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

}
/*********************************************************************
 *            fbe_api_ppfd_object_map_destroy ()
 *********************************************************************
 *
 *  Description:
 *    frees used resources
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/27/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t ppfd_ppfd_object_map_destroy (void)
{
    

    if (ppfd_map_interface_initialized == FBE_TRUE){

        /*flag the theread we should stop*/
        object_map_thread_flag = OBJECT_MAP_THREAD_STOP;
        
        /*change the state of the sempahore so the thread will not block*/
        fbe_semaphore_release(&ppfd_update_semaphore, 0, 1, FBE_FALSE);
        
        /*wait for it to end*/
        fbe_thread_wait(&ppfd_object_map_thread_handle);
        fbe_thread_destroy(&ppfd_object_map_thread_handle);
        
        /*destroy all other resources*/
        fbe_semaphore_destroy(&ppfd_update_semaphore);
        
        fbe_spinlock_destroy(&ppfd_object_map_update_queue_lock);
        fbe_queue_destroy (&ppfd_object_map_update_queue_head);

        ppfd_object_map_unregister_notification_element();
        

        ppfd_map_interface_initialized = FALSE;
        return FBE_STATUS_OK;
    
    }else{
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: map not initialized, can't destroy\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
}


/*********************************************************************
 *            ppfd_object_map_register_notification_element ()
 *********************************************************************
 *
 *  Description: register for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t ppfd_object_map_register_notification_element(void)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_api_notification_interface_register_notification((FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL
                                                                    | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY  ),
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE | FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE,
																 (fbe_api_notification_callback_function_t)ppfd_object_map_notification_function,
																  NULL,
																  &ppfd_reg_id);



    if (status != FBE_STATUS_OK) {
		PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to register for events: status %d\n", __FUNCTION__, status); 
	}

    return status;
}

/*********************************************************************
 *            ppfd_object_map_unregister_notification_element ()
 *********************************************************************
 *
 *  Description: remove ourselvs from events notification    
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t ppfd_object_map_unregister_notification_element(void)
{
	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_api_notification_interface_unregister_notification((fbe_api_notification_callback_function_t)ppfd_object_map_notification_function, ppfd_reg_id);
	if (status != FBE_STATUS_OK){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to unregister events, status:%d\n", __FUNCTION__, status); 
	}

    return status;
}

/*********************************************************************
 *            ppfd_object_map_update_all ()
 *********************************************************************
 *
 *  Description: query the physical package for all the objects in the list
                 and fill the object map based on their location in the tree
 *
 *  Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    6/27/07: sharel   created
 *   
 *********************************************************************/
static fbe_status_t ppfd_object_map_update_all(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t slot = 0;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD,  "PPFD: %s: Starting to build object map...\n", __FUNCTION__); 

    status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_id);
    if(status == FBE_STATUS_OK){
        ppfdSetEnclObjIdInMap(object_id);
    }else{
        ppfdSetEnclObjIdInMap(FBE_OBJECT_ID_INVALID);
    }

    for (slot = 0; slot < TOTALDRIVES; slot++){        
        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_id);
        if(status == FBE_STATUS_OK){
            ppfdSetPPDiskObjIdInMap(slot, object_id);
        }else{
            ppfdSetPPDiskObjIdInMap(slot, FBE_OBJECT_ID_INVALID);
        }
    }
    return FBE_STATUS_OK;

}



/*********************************************************************
 *            ppfd_object_map_notification_function ()
 *********************************************************************
 *
 *  Description: function that is called every time there is an event in physical package   
 *
 *  Inputs: object_id - the object that had the event
 *          fbe_lifecycle_state_t what is the new state of the object
 *          context - any context we saved when we registered
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    9/10/07:sharel changes to support object state instead of notification type
 *    
 *********************************************************************/
fbe_status_t ppfd_object_map_notification_function(update_object_msg_t * update_object_msg, void * context)
{
    update_object_msg_t *   update_msg = NULL;

   /*we allocate here and free after the message is used from the queue*/
	update_msg = ppfdAllocateObjectNotificationStruct();
    if (update_msg == NULL) {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to allocate memory for map update\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*copy theinformation we have now*/
    update_msg->object_id = update_object_msg->object_id;
    EmcpalCopyMemory(&update_msg->notification_info, &update_object_msg->notification_info, sizeof (fbe_notification_info_t));

    fbe_spinlock_lock (&ppfd_object_map_update_queue_lock);
    fbe_queue_push(&ppfd_object_map_update_queue_head, &update_msg->queue_element);
    fbe_spinlock_unlock (&ppfd_object_map_update_queue_lock);
    fbe_semaphore_release(&ppfd_update_semaphore, 0, 1, FBE_FALSE);
        
    return FBE_STATUS_OK;
}


static update_object_msg_t * ppfdAllocateObjectNotificationStruct(void)
{    
    update_object_msg_t * ppfdObjMsg = NULL;    
    
    ppfdObjMsg = (update_object_msg_t *)EmcpalLookasideAllocate(&PPFD_GDS.ObjectMessageNotification);
    if (ppfdObjMsg != NULL){
        fbe_zero_memory(ppfdObjMsg,sizeof(update_object_msg_t));
    }
    return (ppfdObjMsg);
}

static void ppfdFreeObjectNotificationStruct(update_object_msg_t *ppfdObjMsg)
{    
    EmcpalLookasideFree(&PPFD_GDS.ObjectMessageNotification,ppfdObjMsg);
}



/*********************************************************************
 *            ppfd_get_object_location ()
 *********************************************************************
 *
 *  Description: return the physical location of the object in the map
 *               
 *
 *  Inputs: update_msg -  the data structure to fill
 *
 *  Return Value: none
 *
 *  History: 
 *    
 *********************************************************************/
static fbe_status_t ppfd_get_object_location(update_object_msg_t * update_object_msg)
{
	fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_get_object_port_number (update_object_msg->object_id, &update_object_msg->port);
    if (status != FBE_STATUS_OK){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to get object port number. Object ID 0x%x . Status %d\n",
                        __FUNCTION__,update_object_msg->object_id,status); 
        return status;
    }

    status = fbe_api_get_object_enclosure_number (update_object_msg->object_id, &update_object_msg->encl);
    if (status != FBE_STATUS_OK){
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to get object encl number. Object ID 0x%x . Status %d\n",
                        __FUNCTION__,update_object_msg->object_id,status); 
        return status;
    }

    if (update_object_msg->notification_info.object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE){
        status = fbe_api_get_object_drive_number (update_object_msg->object_id, &update_object_msg->drive);
        if (status != FBE_STATUS_OK){
            PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: %s: unable to get object slot number. Object ID 0x%x . Status %d\n",
                            __FUNCTION__,update_object_msg->object_id,status); 
            return status;
        }
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            ppfd_locate_object_location_in_table ()
 *********************************************************************
 *
 *  Description: return the physical location of the object in the map
 *               
 *
 *  Inputs: update_msg -  the data structure to fill
 *
 *  Return Value: none
 *
 *  History: 
 *    
 *********************************************************************/
static fbe_status_t ppfd_locate_object_location_in_table(update_object_msg_t * update_msg,fbe_u32_t  *drive_slot)
{
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       slot = 0;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    *drive_slot = PPFD_INVALID_SLOT_NUMBER;

    if ((update_msg == NULL) || (update_msg->object_id == FBE_OBJECT_ID_INVALID)){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(update_msg->notification_info.object_type){
        case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
            for (slot = 0; slot < TOTALDRIVES; slot++){        
                ppfdGetPPDiskObjIdInMap(slot,&object_id);
                if (object_id == update_msg->object_id){
                    status = FBE_STATUS_OK;
                    *drive_slot = slot;
                }
            }
            break;
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            ppfdGetEnclObjIdInMap(&object_id);
            if (object_id == update_msg->object_id){
                status = FBE_STATUS_OK;
            }
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return (status);
}
/*********************************************************************
 *            ppfd_carsh_dump_info_update_thread ()
 *********************************************************************
 *
 *  Description: Send expander and phy info to miniport driver for
 *                crash dump instance use.
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    09/26/12: aj   created
 *    
 *********************************************************************/
static void ppfd_carsh_dump_info_update_thread(void * context)
{
    EMCPAL_STATUS           nt_status;

    FBE_UNREFERENCED_PARAMETER(context);
    
    PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: %s:\n", __FUNCTION__); 

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&crash_dump_info_update_semaphore, NULL);
        if(crash_dump_info_thread_flag == OBJECT_MAP_THREAD_RUN) {
            /* Send required notification to miniport driver. */
            ppfdPhysicalPackageInitCrashDump();
        } else {
            break;
        }
    }

    PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: %s:\n", __FUNCTION__); 

    crash_dump_info_thread_flag = OBJECT_MAP_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
