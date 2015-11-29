#ifndef PPFD_MISC_H
#define PPFD_MISC_H

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "flare_sgl.h" /* for SGL */
#include "cpd_interface.h"    /* for CPD_EXT_SRB */
#include "fbe_api_common.h"   // for the global tracking structure FBE related 
#include "spid_kernel_interface.h"      // to get the SP ID (SPA or SPB)
#include "spid_types.h"
#include "fbe_api_object_map_interface.h" // for update_object_msg_t 
#include "fbe_winddk.h" // for fbe_rendezvous_event_t

/***************************************************************************
 *  Copyright (C)  EMC Corporation 2008-2009
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

// When searching for the backend 0 Scsiport, we need some maximum Scsiport number.
// Setting it to 16 for now
#define MAX_SCSI_PORTS 16

#define PPFD_MAX_FULL_PORT_SCAN_ON_INIT     4 //Number of maximum full scans for BE port during init.
/****************************
 * Registry String Constants
 ****************************/
#define PPFD_REGISTRY_NAME_BREAK_ENTRY         "BreakOnEntry"
#define PPFD_REGISTRY_NAME_KTRACE_LEVEL        "KtraceDebugLevel"
#define PPFD_REGISTRY_NAME_WINDBG_ENABLE       "WindbgDebugEnable"


/***********************
 * Ktrace Level Defines
 ***********************/
#define MAX_ARGS        6

extern ULONG ppfdGlobalTraceFlags;         // Controls ktarce message levels


#define PPFD_TRACE_FLAG_WARN   0x1     // Warning message flag
#define PPFD_TRACE_FLAG_ERR    0x2     // Error message flag
#define PPFD_TRACE_FLAG_INFO   0x4     // Informational message flag
#define PPFD_TRACE_FLAG_OPT    0x8     // Optional message flag (verbose output for IRP processing, etc.)
#define PPFD_TRACE_FLAG_DBG    0x10    // Debug message flag
#define PPFD_TRACE_FLAG_IO     0x20    // I/O messages flag - enable messages for I/O through PhysicalPackage Interface 
#define PPFD_TRACE_FLAG_ASYNC  0x40    // Async Event messages enabled (disk reomval/insertions, etc)

#define PPFD_GLOBAL_TRACE_FLAGS_DEFAULT  (PPFD_TRACE_FLAG_ERR | PPFD_TRACE_FLAG_WARN | PPFD_TRACE_FLAG_INFO | PPFD_TRACE_FLAG_ASYNC)

#define PPFD_TRACE ppfdKvTrace

#define ppfdKvTrace(TraceLevel, Buffer, pMsg, ...) {		\
	if (TraceLevel & ppfdGlobalTraceFlags) {		\
		if (ppfdGlobalWindbgTrace) {			\
			KvPrintx(Buffer, pMsg, ##__VA_ARGS__);	\
		} else {					\
			KvTracex(Buffer, pMsg, ##__VA_ARGS__);	\
		}						\
	}							\
}

/**********************
 * Enable Windbg Output
 **********************/
#define PPFD_ALLOW_DEBUG_PRINT TRUE
extern ULONG ppfdGlobalWindbgTrace;

#define MAX_LOG_CHARACTER_BUFFER    120
#define MAX_NUM_OF_BYTES_IN_STRING  256

#if DBG
#define DebugBuild  TRUE
#else
#define DebugBuild  FALSE
#endif
/*
 Block realted constants.
*/
#define PPFD_CLIENT_BLOCK_SIZE                  520
#define PPFD_CLIENT_OPTIMIUM_BLOCK_SIZE           1
#define PPFD_CLIENT_OPTIMIUM_BLOCK_ALIGNMENT      0
/*
DetectBackendType() return values
*/
typedef enum _BACKEND_TYPE
{
    UNSUPPORTED_BACKEND=0,
    FC_BACKEND,
    SAS_BACKEND,
    SATA_BACKEND,
}BACKEND_TYPE;

#define TOTALDRIVES 12
#define ALIAS_MIRROR_PATH_ID 4

#define PPFD_CPD_CONFIG_PASSTHRU_LEN (32)
#define PPFD_SRB_TIMEOUT (20)               // Used in SRB, where units are in seconds 
#define PPFD_CPDSRB_TIMEOUT (10)    // Used in CLARiiON Miniport CpdSrb, where units are in seconds.

#define TicksPerSecond  (10000000L)  /* 1 tick = 100 ns */
#define arraysize(p) (sizeof(p)/sizeof((p)[0]))



//
// Get the LBA data from the SRB Command Descriptor Block (10 byte format CDB)
// The order of the bytes is swapped for Big Endian/Little Endian conversion
//
// For 10 byte CDB, bytes 2-5 are the start LBA and bytes 7-8 are the transfer length
//
#define GET_LBA_FROM_CDB10( pSRB ) \
        ( (ULONG) ((cpd_os_io_get_cdb(pSRB)[2] << 24 ) | (cpd_os_io_get_cdb(pSRB)[3] << 16 ) | (cpd_os_io_get_cdb(pSRB)[4] << 8 )| (cpd_os_io_get_cdb(pSRB)[5])) )

#define GET_LENGTH_FROM_CDB10( pSRB ) \
        ( (ULONG) ((cpd_os_io_get_cdb(pSRB)[7] << 8 ) | (cpd_os_io_get_cdb(pSRB)[8])) )


//Copied from ASIDC header 
#define INQUIRY_DATA_BUFFER_SIZE  40
#define MAX_BUSES_PER_HBA         4
#define MAX_LUS                 128     // Max Logical Units per
#define INQUIRY_DATA_SIZE                           \
    (((sizeof (SCSI_ADAPTER_BUS_INFO) + MAX_BUSES_PER_HBA) *    \
         sizeof (SCSI_BUS_DATA)) +              \
     (MAX_LUS *                         \
         (sizeof (SCSI_INQUIRY_DATA) + INQUIRY_DATA_BUFFER_SIZE)))


// This data structure is used to represent each physical disk that PPFD provides access to.
// The SCSI_ADDRESS members include the target ID, path ID, port, etc. for each disk. Note that we have separate
// entries for "PnP disk" and "non PnP disk".  
typedef struct _PPFDDISKOBJ {
    //
    // Using a SCSI_ADDRESS structure for our PPFD disk device object because NtMirror sends an IOCTL_SCSI_GET_ADDRESS
    // to get the SCSI_ADDRESS data for a pnp Disk..so we can just return the information from this structure
    //
    SCSI_ADDRESS ScsiAddrPPFDDisk;      // Non PnP disk  (used by ASIDC)
    SCSI_ADDRESS ScsiAddrPPFDpnpDisk;   // Aliased PnP Disk
    fbe_object_id_t DiskObjID;          // Physical Package Disk Object ID for this disk
    fbe_lifecycle_state_t lifecycle_state; //Disk lifecycle state
    PEMCPAL_DEVICE_OBJECT ppfdFiDO;            // PPFD Disk Device Object for this disk (created during DriverEntry for ASIDC to use)
    PEMCPAL_DEVICE_OBJECT ppfdPnPFiDO;         // PPFD Pnp Disk Device Object for this disk (created in ppfdAddDevice() for NtMirror)

    fbe_block_transport_negotiate_t negotiated_block_size;
    BOOLEAN                         negotiated_block_size_valid;
    fbe_spinlock_t                 ObjectInfoSpinLock;
    fbe_spinlock_t                 IORegionSpinLock;
    EMCPAL_LIST_ENTRY            WriteLockListHead;
}PPFDDISKOBJ, *p_PPFDDISKOBJ;

typedef struct _PPFDENCLOBJ{
    fbe_object_id_t        encl_object_id;  // Physical Package Enclosure Object ID
    fbe_lifecycle_state_t  lifecycle_state; //Encl lifecycle state
}PPFDENCLOBJ, *p_PPFDENCLOBJ;

// PPFD Driver Global Data Structure - This structure makes
// debug easier by making key data structures visible to Windbg.
// 
// 
// PPFD Disk device Objects map. The entries are "in order" so that PPFDDiskObjectsMap[0] is for disk 0, PPFDDiskObjectsMap[1] 
// is disk 1, etc.  This map is used for a variety of purposes.  Most importantly it contains the PPFD Disk Device Objects 
// (ppfd FiDO's)that represent each boot disk (separate PPFD disk device objects are maintained for pnp disks used by NtMirror and 
// non pnp disk used by ASIDC). 
//


typedef struct _PPFD_GLOBAL_DATA {
    PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject;
    PEMCPAL_DEVICE_OBJECT Be0ScsiPortObject;
    BACKEND_TYPE beType;
    SP_ID SpId;
    UCHAR pnpDiskNumber;

    EMCPAL_LOOKASIDE IoContextLookasideListHead;

    EMCPAL_LOOKASIDE SmallSGLListLookasideListHead; // 36 elements
    EMCPAL_LOOKASIDE MediumSGLListLookasideListHead; // 132 elements

    EMCPAL_LOOKASIDE ObjectMessageNotification;

    fbe_spinlock_t        IoRetryQueueSpinLock;
    EMCPAL_LIST_ENTRY            IoRetryListHead;

    BOOLEAN               AbortIoRetryThread;
    fbe_thread_t          IoRetryThreadHandle;
    fbe_rendezvous_event_t  IoRetryEvent;

    PPFDENCLOBJ           PPFDEnclObject; //Represents boot enclosure (Bus 0 Encl 0)
    PPFDDISKOBJ           PPFDDiskObjectsMap[ TOTALDRIVES ];
    fbe_spinlock_t        ObjectMapInfoLock;

}PPFD_GLOBAL_DATA, *p_PPFD_GLOBAL_DATA;

#define PPFD_MEDIUM_SGL_LIST_MAX_SGL_COUNT  132
#define PPFD_SMALL_SGL_LIST_MAX_SGL_COUNT   36

typedef enum _PPFD_SGL_LIST_POOL_ID{
    SGL_LIST_POOL_ID_INVALID,
    SGL_LIST_POOL_ID_SMALL,
    SGL_LIST_POOL_ID_MEDIUM,
    SGL_LIST_POOL_ID_DYNAMIC
}PPFD_SGL_LIST_POOL_ID;

#define PPFD_SGL_MEORY_SIGNATURE    0x5EEDABCD

typedef struct _PPFD_SGL_LIST_HEADER{
    ULONG                  Signature;    
    PPFD_SGL_LIST_POOL_ID  PoolID;
}PPFD_SGL_LIST_HEADER;

typedef struct _PPFD_CALLBACK_DATA {
    void *      context;
    BOOLEAN     (* async_callback) (void *, struct CPD_EVENT_INFO_TAG *);
    void *      callback_handle;
}PPFD_CALLBACK_DATA,*p_PPFD_CALLBACK_DATA;

#define PPFD_MAX_CLIENTS 8

typedef struct _PPFD_CALLBACK_REGISTRATION_DATA {
    PPFD_CALLBACK_DATA RegisteredCallbacks[PPFD_MAX_CLIENTS];
    UCHAR NumRegisteredCallbacks;
}PPFD_CALLBACK_REGISTRATION_DATA,*p_PPFD_CALLBACK_REGISTRATION_DATA;

// Defines for wait tineout length for disk ready when NtMirror is "logging" in to the drive at boot time
// (via CPD_IOCTL_ADD_DEVICE)
// In reality, we should find the boot disks within 1-2 seconds of waiting unless the drive is removed.  
// This will wait up to 30 seconds.
#define RETRY_GET_PP_DISK_OBJ_ID 60     // x500ms for total wait time for boot disks to be ready
#define WAIT_FOR_PP_MS 500     // 1/2 second

#define PPFD_REGISTER_CALLBACK_HANDLE_INDEX_MASK    0x000000FF
#define PPFD_REGISTER_CALLBACK_HANDLE_PREFIX        0x55F00000

/* WARNING:  If you change the following enum make sure you also update the PpfdWriteStateTable[] 
 * which is defined in ppfdPhysicalPackageInterface.c
 */
typedef enum _PPFD_WRITE_STATE{
    PPFD_WRITE_INITIAL_STATE = 0,
    PPFD_CHECK_WRITE_EDGE_PRESENT_STATE,
    PPFD_ALLOCATE_PRE_READ_SGL_STATE,
    PPFD_ALLOCATE_PRE_READ_BLOCKS_STATE,
    PPFD_INITIALIZE_PRE_READ_SGL_STATE,
    PPFD_LOCK_PRE_READ_BLOCKS_STATE,
    PPFD_PRE_READ_IO_STATE,
    PPFD_PRE_READ_CLEAN_UP_STATE, //Map read IO status, destroy packet
    /* PPFD_LOCK_WRITE_BLOCKS_STATE, ** Not needed since region is locked during pre-read */
    PPFD_WRITE_IO_STATE,    
    PPFD_WRITE_COMPLETE_STATE, //Release locks, pre-read blocks,pre-read SGL,map read write status, destroy packet, complete IRP.
    PPFD_WRITE_INVALID_STATE 
}PPFD_WRITE_STATE;

typedef struct _PPFD_WRITE_STATE_ENTRY{    
    PPFD_WRITE_STATE    NextStateOnSuccess;
    PPFD_WRITE_STATE    NextStateOnFailure;
}PPFD_WRITE_STATE_ENTRY;

typedef struct _PPFD_WRITE_CONTEXT{
    ULONG   Signature;
    EMCPAL_LIST_ENTRY       ListEntry;
    EMCPAL_LIST_ENTRY       IoQueueListEntry;
    p_PPFDDISKOBJ    pDiskObjectData;
    PPFD_WRITE_STATE CurrentState;    
    ULONG    Status;
    fbe_sg_element_t   *pre_read_sgl;
    void *      dummy_read_block;
    void *      pre_read_begin_edge_block;
    void *      pre_read_end_edge_block;
    PEMCPAL_IRP        OriginalIrp;
    ULONG       OriginalStartLBA;
    ULONG       OriginalLength;
    PSGL        OriginalSGL;
    //Additional info for locks
    BOOLEAN         RegionLocked;
    fbe_block_transport_negotiate_t negotiate_block_size;
    fbe_lba_t start_lba;
    fbe_block_count_t block_count;
    fbe_u32_t       disk_object_id;
    fbe_payload_pre_read_descriptor_t pre_read_descriptor;
    fbe_bool_t      start_aligned;
    fbe_bool_t      end_aligned;
    fbe_packet_t   *packet;
}PPFD_WRITE_CONTEXT;

typedef enum _PPFD_WRITE_STATUS{
    PPFD_WRITE_STATUS_UNKNOWN = 0,
    PPFD_WRITE_STATUS_ERROR,
    PPFD_WRITE_STATUS_PENDING,
    PPFD_WRITE_STATUS_SUCCESS
}PPFD_WRITE_STATUS;

EMCPAL_STATUS ppfdWriteStartIoRestartThread(void);
void ppfdHandleAsyncEvent( update_object_msg_t *PPStateChangeMsg);
#endif //PPFD_MISC_H
