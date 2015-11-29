/***********************************************************************
*.++
*  COMPANY CONFIDENTIAL:
*     Copyright (C) EMC Corporation, 2002
*     All rights reserved.
*     Licensed material - Property of EMC Corporation.
*.--                                                                    
***********************************************************************/

/***********************************************************************
*.++
* FILE NAME:
*      UceExport.h
*
* DESCRIPTION:
*      Definitions and structures used throughout the Universal Copy
*		Engine (UCE).
*
* REVISION HISTORY:
*      15-May-2002   pmisra       Added pClientCopyCmd
*      23-May-2002   majmera      LargeLu Support
*      08-Jan-2002   pmisra       Created
*                               
***********************************************************************/

#ifndef _UCEEXPORT_H_
#define _UCEEXPORT_H_

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
//#include <ntdef.h>
//#include <ntstatus.h>
#include "k10defs.h"
#include "DeltaMapUtils.h"


/***********************************************************************
*.++
* CONSTANT:
*		UCE_MAX_NUM_STREAMS
*
* DESCRIPTION:
*      This is the maximum number of concurrent copy commands that the 
*      UCE will support.  
*
* REVISION HISTORY:
*      24-Feb-01  pmisra		Created.
*.--
***********************************************************************/

#define UCE_MAX_NUM_STREAMS     32

/***********************************************************************
*.++
* CONSTANT:
*		UCE_BLOCK_SIZE
*
* DESCRIPTION:
*      This is the size of a block. It is equivalent to 512 bytes  
*
* REVISION HISTORY:
*      24-Feb-01  pmisra		Created.
*.--
***********************************************************************/

#define UCE_BLOCK_SIZE          512

#define UCE_MIN_THROTTLE_VALUE  1
#define UCE_MAX_THROTTLE_VALUE  10

/********************************************************************************
 * NAME: 
 *   UCE_STATUS_*
 *
 * DESCRIPTION:
 *     These #defines provide the values for the UCE's error and informational 
 *     status types.
 *   
 *  HISTORY
 *     21st-Feb-2002     Created.   -Pawan Misra
 *
 *******************************************************************************/

#define UCE_STATUS_BASE						0x71350000

#define	UCE_STATUS_SUCCESS					UCE_STATUS_BASE + 0
#define UCE_STATUS_SRC_DEVICE_FAILED        UCE_STATUS_BASE + 1
#define UCE_STATUS_NOT_ENOUGH_RESOURCES     UCE_STATUS_BASE + 2
#define UCE_STATUS_NO_VALID_DEST_DEVICE     UCE_STATUS_BASE + 3
#define UCE_STATUS_COMMAND_PAUSED           UCE_STATUS_BASE + 4
#define UCE_STATUS_INVALID_COMMAND_ID       UCE_STATUS_BASE + 5


/***********************************************************************
*.++
* TYPE:
*      void_type
*
* DESCRIPTION:
*      Type returned by a function which returns no values.
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    -Ported from CpmDef.h
*.--
***********************************************************************/

typedef void void_type;


/***********************************************************************
*.++
* TYPE:
*      PEMCPAL_DEVICE_OBJECT
*
* DESCRIPTION:
*      Pointer to a Device Object pointer
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    -Ported from CpmDef.h
*.--
***********************************************************************/

typedef PEMCPAL_DEVICE_OBJECT *PPEMCPAL_DEVICE_OBJECT;


/***********************************************************************
*.++
* CONSTANT:
*		UCE_THROTTLE_DELAY
*
* DESCRIPTION:
*      Value used by the Engine to increase throttling by adding 
*		a delay between the reads.  This is equal to 1 msec (10 * 100nsec)
*		expressed in terms of system time units (100 nanosecond).  
*
* REVISION HISTORY:
*      24-Feb-01  pmisra		Created.
*.--
***********************************************************************/

#define UCE_THROTTLE_DELAY		10

/***********************************************************************
*.++
* TYPE:
*      UCE_STREAM_LOCK, *UCE_PSTREAM_LOCK
*
* DESCRIPTION:
*      The UCE_STREAM_LOCK structure combines the ERESOURCE and the associated
*      ERESOURCE_THREAD into a single structure. There are routines defined to 
*      Initialize, Acquire, and Release this resource.
*
* MEMBERS:
*      UceResource         The Actual ERESOURCE structure
*      UceResourceThread   Current thread holding the UceResource
*
* REVISION HISTORY:
*      08-Jan-2002  pmisra   created
*.--
***********************************************************************/

typedef struct UCE_STREAM_LOCK
{
    ERESOURCE           UceResource;
    ERESOURCE_THREAD    UceResourceThread;
} UCE_STREAM_LOCK, *PUCE_STREAM_LOCK;


/***********************************************************************
*.++
* TYPE:
*      UCE_OPERATION_TYPE
*
* DESCRIPTION:
*      Enumeration for the different types of I/O operations the
*      Universal Copy Engine can initiate
*
* MEMBERS: 
*      READ:  Read I/O operation
*      WRITE: Write I/O operation
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    created
*.--
***********************************************************************/

typedef enum _UCE_OPERATION_TYPE
{
    UCE_READ    = 1,
    UCE_WRITE   = 2
} UCE_OPERATION_TYPE;

 
/***********************************************************************
*.++
* TYPE:
*      COMMAND_ID
*
* DESCRIPTION:
*		
*      COMMAND_ID is a unique value used to track Copy Commands.
*		The unique command ID will be the current time.
*
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    Ported from CpmDef.h
*.--
***********************************************************************/

typedef EMCPAL_LARGE_INTEGER COMMAND_ID, *PCOMMAND_ID;


/***********************************************************************
*.++
* TYPE:
*      UCE_COPY_STATE, *PUCE_COPY_STATE
*
* DESCRIPTION:
*      The FLAGS is a structure that contains the current state
*      of the Copy Command
*
* MEMBERS:
*      Pause:               If TRUE, the Copy Command is Paused
*      Abort:               If TRUE, the Copy Command has been Aborted
*      Quiesce              If TRUE, the Copy Command needs to be quiesced
*      CommandDead:         If TRUE, the Copy Command has Failed 
*      CommandDone:         If TRUE, the Copy Command is Done 
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    created
*.--
***********************************************************************/

typedef struct _UCE_COPY_STATE
{
    BOOLEAN                  Pause;
    BOOLEAN                  Abort;
    BOOLEAN                  Quiesce;
    BOOLEAN                  CommandDead;
    BOOLEAN                  CommandDone;

} UCE_COPY_STATE, *PUCE_COPY_STATE;

/**********************************************************************
*.++
* TYPE:
*      UCE_DEVICE_LOCATION
*
* DESCRIPTION:
*      Enumeration for the location of the device
*
* MEMBERS: 
*      UCE_UNKNOWN:   Unknown device location
*      UCE_BACK_END:  Back-end device
*      UCE_FRONT_END: Front-end device
*
* REVISION HISTORY:
*      02-Feb-22   pmisra   Created.
*.--
***********************************************************************/

typedef enum _UCE_DEVICE_LOCATION
{
    UCE_UNKNOWN     = 1,
    UCE_BACK_END    = 2,
    UCE_FRONT_END   = 3
} UCE_DEVICE_LOCATION;

/***********************************************************************
*.++
* TYPE:
*      UCE_DEVICE_INFO, *PUCE_DEVICE_INFO
*
* DESCRIPTION:
*      The DEVICE_INFO is a structure that contains information
*      associated with each device
*
* MEMBERS:
*      DeviceID:          World Wide ID for the device
*      DevStartAddress:   Starting block address for the device
*      pDeviceObject:     Pointer to the device object	   
*      Location:          BE or FE device
*      pQuiesce:          Pointer to boolean indicating device is quiesced
*      pPeerOwnsLU:       Pointer to boolean indicating device is trespassed 
*      DeviceFailed:      Boolean indicating device failure
*      pClientDevice:     Pointer to client specific device structure
*
* REVISION HISTORY:
*      01-Jan-2002  pmisra    created
*.--
***********************************************************************/

typedef struct _UCE_DEVICE_INFO
{
    K10_WWID                 DeviceID;
    EMCPAL_LARGE_INTEGER            DevStartAddress;
    PEMCPAL_DEVICE_OBJECT           pDeviceObject;
    UCE_DEVICE_LOCATION      Location;
    PBOOLEAN                 pQuiesce;
    PBOOLEAN                 pPeerOwnsLU;
    BOOLEAN                  DeviceFailed;
	PVOID                    pClientDevice;

} UCE_DEVICE_INFO, *PUCE_DEVICE_INFO;


/***********************************************************************
*.++
* TYPE:
*      UCE_BUF_DESCRIPTOR, *PUCE_BUF_DESCRIPTOR
*
* DESCRIPTION:
*      This structure describes a buffer.  It contains all the information  
*      required to uniquely identify each buffer. 
*
* MEMBERS:  
*      pIrp:				Pointer to the IRP
*	   pClientBufStruct:   Client specific buffer structure
*      WorkItem:   Work Item that is required for the setup and execution 
*                  of a worker thread
*      StreamID:   Stream identifier
*      BufferID:   Number that uniquely identifies each buffer
*      ReadAddr:   Address of the last read operation
*	   WriteAddr:  Address of last write operation
*      Operation:  Could be either a Read or a Write
*      Status:     Status of the I/O (from the IRP)
*      NextDest:   This is the next destination that needs to be serviced
*      DelaySet:   Flag set during the Throttle Delay mechanism
*      IOSize:     Valid data length in the buffer (may not be using the
*                  whole buffer if this is the last piece of a multi-buffer
*                  copy command).
*		StartChunk  First chunk a buffers services
*		ChunkRange  Number of chunks a buffer is servicing
*                  
*
* REVISION HISTORY:
*      08-Jan-2002   pmisra      created
*.--
***********************************************************************/

typedef struct _UCE_BUF_DESCRIPTOR
{
    PEMCPAL_IRP                     pIrp;
	PVOID					 pClientBufStruct;
    WORK_QUEUE_ITEM          WorkItem;
    ULONG                    StreamID;
    ULONG                    BufferID;
    EMCPAL_LARGE_INTEGER            ReadAddr;
	EMCPAL_LARGE_INTEGER            WriteAddr;
    UCE_OPERATION_TYPE       Operation;
    EMCPAL_STATUS            Status;
    LONG                     NextDest;
    BOOLEAN                  DelaySet;
    ULONG                    IOSize;
	ULONG                    StartChunk;
	ULONG                    ChunkRange;
} UCE_BUF_DESCRIPTOR, *PUCE_BUF_DESCRIPTOR, **PPUCE_BUF_DESCRIPTOR;


/*****************************************************************************
*++
*
*      Prototypes of client callbacks registered with the UCE
*--
*****************************************************************************/



typedef void_type
( *PGSKT_READ_CALLBACK ) ( IN PUCE_BUF_DESCRIPTOR    pBuffer, 
                           IN PEMCPAL_DEVICE_OBJECT         pSrcDeviceObject,
                           IN ULONG                  BufSize );

typedef void_type
( *PGSKT_WRITE_CALLBACK ) ( IN PUCE_BUF_DESCRIPTOR   pBuffer,
                            IN PEMCPAL_DEVICE_OBJECT        pDestDeviceObject,
                            IN ULONG                 BufSize,
							IN PVOID                 pClientDevice);


typedef BOOLEAN
( *PCOLLISION_CHECK_CALLBACK ) ( IN PDMH_DELTA_MAP   pDeltaMap,
								 IN EMCPAL_LARGE_INTEGER    DataStartAddr,
				                 IN ULONG            DataLength );


typedef void_type
( *PCMD_COMPLETE_CALLBACK ) ( IN PULONG              pStreamID );


typedef void_type
( *PDEVICE_ERROR_CALLBACK ) ( IN ULONG               StreamID,
							  IN LONG                DeviceIndex,
							  IN EMCPAL_STATUS       IOStatus,
							  IN EMCPAL_LARGE_INTEGER       TransferCount );				                                        

/***********************************************************************
*.++
* TYPE:
*      UCE_PRIMITIVE_COPY_CMD, *PUCE_PRIMITIVE_COPY_CMD
*
* DESCRIPTION:
*      The PRIMITIVE_COPY_CMD is a structure that contains the
*      copy command parameters required by the Copy Engine to service
*      a copy command.
*
* MEMBERS:
*      CommandID:          Unique ID for the copy command 
*      pDeltaMap:          Pointer to the Delta Map.
*      SrcDeviceInfo:      Structure of source device info
*      pDestDeviceInfo:    Pointer to array of dest device info structures
*      DestDeviceCount:    The number of destination devices
*      CopyLength:         Number of blocks to copy
*      Throttle:           Current throttle value
*      TransferCount:      Number of blocks already copied
*      CommmandStatus:     Completion status for the command
*      IOStatus:           Status for the IO
*      ErrorTargetIndex:   Device index of the failed device
*      State:              Current state of the command
*	   pCmdComplete:       Command completion callback
*	   pGasketRead:        Callback for Gasket Read function
*	   pGasketWrite:       Callback for Gasket Write function
*	   pDeviceError:       Callback for notification of device error
*	   pCollisionCheck:    Callback for collision check
*      pGetResources       
*      pClientCopyCmd      Pointer to client specific copy command
*
* REVISION HISTORY:
*      25-May-02     pmisra   Added pointer Client specific copy cmd.
*      08-Jan-02     pmisra   Created - ported from CpmDef.h.
*.--
***********************************************************************/

typedef struct _UCE_PRIMITIVE_COPY_CMD
{
    COMMAND_ID                   CommandID;
	PDMH_DELTA_MAP			     pDeltaMap;
    UCE_DEVICE_INFO		         SrcDeviceInfo;
	PUCE_DEVICE_INFO             pDestDeviceInfo;
	ULONG                        DestDeviceCount;
	EMCPAL_LARGE_INTEGER                CopyLength;
	ULONG                        Throttle;    
	EMCPAL_LARGE_INTEGER                TransferCount;
    EMCPAL_STATUS                CommandStatus;   
    EMCPAL_STATUS                IOStatus;   
    LONG                         ErrorTargetIndex;
	UCE_COPY_STATE               State;
	PCMD_COMPLETE_CALLBACK	     pCmdComplete;
	PGSKT_READ_CALLBACK	         pGsktRead;
	PGSKT_WRITE_CALLBACK	     pGsktWrite;
	PDEVICE_ERROR_CALLBACK	     pDeviceError;
	PCOLLISION_CHECK_CALLBACK    pCollisionCheck;
//   PVOID					     pGetResources;
	PVOID                        pClientCopyCmd;
 
} UCE_PRIMITIVE_COPY_CMD, *PUCE_PRIMITIVE_COPY_CMD;

/***********************************************************************
*.++
* TYPE:
*      UCE_RESOURCES, *PUCE_RESOURCES
*
*  Description:
*      
*
*  Members:
*		PrimitiveCmd:
*		pBufferList:
*		Quiesce:
*		PeerOwnsLU:
*
*
* REVISION HISTORY:
*      08-Jan-2002   pmisra    Created.
*.--
***********************************************************************/

typedef struct _UCE_RESOURCES
{
    PUCE_PRIMITIVE_COPY_CMD  PrimitiveCmd;
	PUCE_BUF_DESCRIPTOR      pBufferList;
    BOOLEAN					 Quiesce;
	BOOLEAN					 PeerOwnsLU;

} UCE_RESOURCES, *PUCE_RESOURCES;

#endif //_UCEEXPORT_H_
