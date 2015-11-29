#ifndef DATA_CHANGE_NOTIFY_INTERFACE_H
#define DATA_CHANGE_NOTIFY_INTERFACE_H

// File: DataChangeNotifyInterface.h      

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2009 - 2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:
//
//      This is the header file for the Data Change Notify driver.
//      This file contains data structure definitions, IOCTL definitions and 
//      exported function prototypes.
//
//      The basic flow for using data change notify service is as below.
//
//  Clones
//  MirrorView
//  SAN Copy                                                                   RPS
//  MLU                                   DataChangeNotify                    SnapBack
//   |                                     |                                    |
//   |                                     |                                    |
//   |                                     | <-- DataChangeNotifyRegister() --  |
//   |                                     |                                    |
// 
//           :
//           :
//           :
//   |                                   |                                      | 
//   | ----- DataChangeNotifyOpStart() --> |                                    |
//   |                                   | -- PDATACHANGE_NOTIFY_CALLBACK()-->  |
//   |                                   | <-------- STATUS_SUCCESS ---------   |
//   | <-------------------------------  |                                      |
//   |                                   |                                      |
//           :
//           :
//           :
// 
//   | ----- DataChangeNotifyOpStart()-> |                                      |
//   |                                   | -- PDATACHANGENOTIFY_CALLBACK()-->   |
//   |                                   | <-------- STATUS_PENDING ---------   |
//   |                                   |                 :                    |
//   |                                   |                 :                    |
//   |                                   |<-DataChangeNotifyNotificationComplete|
//   | <-------------------------------  |                                      |
//      
//
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"
#include "K10Debug.h"
#include "k10defs.h"
#include "ktrace.h"

#ifdef __cplusplus
};
#endif


//
//  The following defines the length of the client name registering with data change notify
//


#define             DATA_CHANGE_NOTIFY_CLIENT_NAME_LENGTH          64

//
//  The following defines the string used to store client of the notification.
//

typedef char        DATA_CHANGE_NOTIFY_CLIENT_NAME[ DATA_CHANGE_NOTIFY_CLIENT_NAME_LENGTH + 1];

//
//  The following defines the upper bound on the "LuCount" field of DATA_CHANGE_NOTIFICATION_INPUT_BUFFER and 
//  DATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER.
//
#define             DATA_CHANGE_NOTIFICATION_MAX_LUS            1024



typedef enum
{
    DataChangeNotifyOperationInvalid = 0,
    DataChangeNotifyOperationStart,
    DataChangeNotifyOperationComplete

}DATA_CHANGE_NOTIFY_OPERATION, *PDATA_CHANGE_NOTIFY_OPERATION;


//
//  The following defines various layered driver operations that 
//  can cause notifications to be sent to the data change driver.
//

typedef enum DATA_CHANGE_NOTIFY_CLIENT_OPERATION
{
    DataChangeNotifyClientOperationInvalid = -1,
    DataChangeNotifyClientOperationSnapViewRollbackStart,
    DataChangeNotifyClientOperationSnapViewRollbackStartComplete,
    DataChangeNotifyClientOperationSnapViewRollbackComplete,
    DataChangeNotifyClientOperationClonesReverseSyncStart,
    DataChangeNotifyClientOperationClonesReverseSyncStartComplete,
    DataChangeNotifyClientOperationClonesReverseSyncComplete,
    DataChangeNotifyClientOperationCPMCopyStart,
    DataChangeNotifyClientOperationCPMCopyComplete,
    DataChangeNotifyClientOperationMluSnapshotRollbackStart,
    DataChangeNotifyClientOperationMluSnapshotRollbackComplete,
    DataChangeNotifyClientOperationMluSnapshotAttachStart,
    DataChangeNotifyClientOperationMluSnapshotAttachComplete,
    DataChangeNotifyClientOperationSnapViewActivateStart,
    DataChangeNotifyClientOperationSnapViewActivateComplete,
    DataChangeNotifyClientOperationClonesSyncStart,
    DataChangeNotifyClientOperationClonesSyncComplete,
    DataChangeNotifyClientOperationMax

}DATA_CHANGE_NOTIFY_CLIENT_OPERATION, *PDATA_CHANGE_NOTIFY_CLIENT_OPERATION;

//
//  Type:           DATA_CHANGE_NOTIFY_OP_START_STATUS
//
//  Description:    This structure contains the data that is returned to indicate
//                  status of start notification for an LU.
//
//  Members:
//
//      LuId
//          LU ID of the LU for which the start notification was sent.
//
//      LuStatus
//          Status returned by the Splitter/Snapback driver for the LU.
//          * STATUS_SUCCESS - If operation was successful on this LUN.
//          * STATUS_OPERATION_NOT_ALLOWED_ON_SPLITTER_SECONDARY_LU - The
//            operation cannot continue on this LUN.
//

typedef struct  _DATA_CHANGE_NOTIFY_OP_START_STATUS
{
    K10_LU_ID               LuId;

    EMCPAL_STATUS           LuStatus;

}DATA_CHANGE_NOTIFY_OP_START_STATUS, *PDATA_CHANGE_NOTIFY_OP_START_STATUS;

//
//  DATA_CHANGE_NOTIFICATION_INPUT_BUFFER
//
//  This is the input buffer that notificaton clients receive as part
//  of the operation notification.
//
//  Members:
//      OperationType - Operation this notification is sent for.
//
//      LuCount - Number of LUs in the notification.
//
//      LuIdLit[] - List of LUIDs in the notification.
//                  The caller allocates storage for the list of LUs to be passed as part if inbuf.
//
typedef struct  _DATA_CHANGE_NOTIFICATION_INPUT_BUFFER
{
    DATA_CHANGE_NOTIFY_CLIENT_OPERATION         OperationType;
    
    ULONG                                       LuCount;
    
    K10_LU_ID                                   LuIdList[DATA_CHANGE_NOTIFICATION_MAX_LUS];

} DATA_CHANGE_NOTIFICATION_INPUT_BUFFER, *PDATA_CHANGE_NOTIFICATION_INPUT_BUFFER;


//
//  DATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER
//
//  This structure contains the operation notification response from the
//  notification client.
//                  
//  Members:
//      Status -  Overall status of the operation.
//
//      LuCount - Number of LUs in the notification.
//
//      LuStatus - Individual status of each member LU.
//
typedef struct  _DATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER
{    
    EMCPAL_STATUS                          Status;

    ULONG                                  LuCount;
    
    DATA_CHANGE_NOTIFY_OP_START_STATUS     LuStatus[DATA_CHANGE_NOTIFICATION_MAX_LUS];

} DATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER, *PDATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER;

//
//  PDATA_CHANGE_NOTIFY_CALLBACK
// 
//  This function typedef describes notification callback routine for the 
//  notification events.  This function must be capable of executing at
//  DISPATCH_LEVEL.
// 
//  DataChangeNotifyOp [in] - opcode indicating either a start or a complete operation.
//
//  PDCNContext        [in] - context associated with the notification request, this needs 
//                            to be returned back as part of DataChangeNotifyNotificationComplete()
//
//  pInputBuffer [in] - Pointer to data change notify service allocated
//                      memory that describes the notification request.
// 
//  pOutputBuffer [out] - Pointer to data change notify service allocated
//                        memory.  The callback must store the response in this
//                        buffer.
// 
//  Returns:
//
//      STATUS_SUCCESS - If the entire operation was successful.  The client
//          will not execute the DataChangeNotifyNotificationComplete() callback
//          routine.
//
//      STATUS_OPERATION_NOT_ALLOWED_ON_SPLITTER_SECONDARY_LU - If one of the
//          LUNs reported a faliure.  The splitter notification library
//          should consult the individual members of pOutputBuffer->LuIdList
//          to determine which LUN caused the failure.  The client
//          will not execute the DataChangeNotifyNotificationComplete() callback
//          routine.
//
//      STATUS_PENDING - If the entire operation was successful.  The client
//          will execute the DataChangeNotifyNotifyNotificationComplete() callback
//          routine.  The contents of pOutputBuffer are not valid.
//
typedef EMCPAL_STATUS
(*PDATA_CHANGE_NOTIFY_CALLBACK) (
    IN  DATA_CHANGE_NOTIFY_OPERATION                   DataChangeNotifyOp,
    IN  PVOID                                          PDCNContext,
    IN  PDATA_CHANGE_NOTIFICATION_INPUT_BUFFER         pInputBuffer,
    IN  OUT PDATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER    pOutputBuffer
);

//
//  DATA_CHANGE_NOTIFY_REGISTRATION_CLIENTS
// 
//  This enumeration lists the clients who wish to be notified when a
//  destructive operation starts or stops.
//
typedef enum DATA_CHANGE_NOTIFY_REGISTRATION_CLIENT
{
    DataChangeNotifyRegistrationClientInvalid = 0,
    DataChangeNotifyRegistrationClientSplitter,
    DataChangeNotifyRegistrationClientSnapBack,
    DataChangeNotifyRegistrationClientMax

} DATA_CHANGE_NOTIFY_REGISTRATION_CLIENT, *PDATA_CHANGE_NOTIFY_REGISTRATION_CLIENT;


#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS 
CSX_MOD_EXPORT
DataChangeNotifyOpStart(
    IN     DATA_CHANGE_NOTIFY_CLIENT_OPERATION   ClientOpType,
    IN      PK10_LU_ID                           pLuIdList,
    IN      ULONG                                LuCount,
    IN OUT  DATA_CHANGE_NOTIFY_OP_START_STATUS   LuStatus[]
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS 
CSX_MOD_EXPORT 
DataChangeNotifyOpComplete(
    IN  DATA_CHANGE_NOTIFY_CLIENT_OPERATION      ClientOpType,
    IN  PK10_LU_ID                               pLuIdList,
    IN  ULONG                                    LuCount    
    );

//
//  DataChangeNotifyRegister()
//
//  This routine registers a client for notification of a destructive
//  start/stop event.
//
//  Parameters:
//      RegistrationClientId [in] - The client that is registering.
// 
//      RegistrationClientNotifyCallback [in] - A pointer to a notification callback routine that the
//          data change notify service will execute upon calls to 
//          DataChangeNotifyOpStart() and DataChangeNotifyOpComplete().
//
//     RegistrationClientName [in] - Client name .
//
//
//  Returns:
//      STATUS_SUCCESS - if the operation is successful.
//      Error - if the splitter notification library could not register the
//          client.
//
#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS 
CSX_MOD_EXPORT
DataChangeNotifyRegister(
    IN  DATA_CHANGE_NOTIFY_REGISTRATION_CLIENT       RegistrationClientId, 
    IN  PDATA_CHANGE_NOTIFY_CALLBACK                 RegistrationClientNotifyCallback,
    IN  DATA_CHANGE_NOTIFY_CLIENT_NAME               RegistrationClientName
    );


//
//  DataChangeNotifyDeRegister()
//
//  This routine de-registers a client's notification callback routine
//
//  Parameters:
//      client [in] - The client that is de-registering.
//
//  Returns:
//      STATUS_SUCCESS - if the operation is successful.
//      Error - if the library could not deregister the client.
//
#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS  
CSX_MOD_EXPORT
DataChangeNotifyDeRegister(
    IN DATA_CHANGE_NOTIFY_REGISTRATION_CLIENT      RegistrationClientId  
    );

//
//  DataChangeNotifyNotificationComplete()
//
//  Clients execute this routine when they've completed processing a
//  notification event.
//
//  Parameters:
//
//  pNotifyContext [in] - Pointer to DataChangeNotify driver context
//                        which is passed in as part of start or complete
//                        callback .
//  pOutputBuffer [in] - Pointer to DataChangeNotify driver allocated
//                       memory.  The callback must store the response in this
//                       buffer.
//
//  Returns:
//      STATUS_SUCCESS - if the operation is successful.
//
#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS 
CSX_MOD_EXPORT
DataChangeNotifyNotificationComplete(
    IN PVOID                                       pNotifyContext,
    IN PDATA_CHANGE_NOTIFICATION_OUTPUT_BUFFER     pOutputBuffer
    );


#endif  // DATA_CHANGE_NOTIFY_INTERFACE_H
