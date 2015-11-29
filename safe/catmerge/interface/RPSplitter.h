/*******************************************************************************
 * Copyright (C) EMC Corporation, 2007
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

//
//  File:
//
//      RPSplitter.h
//
//  Description:
//
//      This is the primary header file for the RecoverPoint Splitter driver.
//      This file contains data structure definitions, IOCTL definitions and 
//      exported function prototypes.
//      
//  History:
//
//      28-Aug-2007    NS;       Initial creation.
//


#ifndef __RP_SPLITTER_H__
#define __RP_SPLITTER_H__

#include "K10Debug.h"
#include "k10defs.h"
#include "k10ntddk.h"
#include "ktrace.h"
#include "psm.h"
#include "RPSplitterNotify.h"

//
//  The following defines various IOCTLs that the Splitter driver will respond to.
//

typedef enum
{
    SPLITTER_OPERATION_INDEX_MIN = 1,
    SPLITTER_NOTIFIY_OPERATION_START,
    SPLITTER_NOTIFIY_OPERATION_COMPLETE

}SPLITTER_OPERATION_INDEX, *PSPLITTER_OPERATION_INDEX;


//
//  Define the device type value.  Note that values used by Microsoft
//  Corporation are in the range 0-32767, and 32768-65535 are reserved for use
//  by customers.
//

#define     FILE_DEVICE_SPLITTER                        49995


//
//  Define IOCTL index values.  Note that function codes 0-2047 are reserved
//  for Microsoft Corporation, and 2048-4095 are reserved for customers.
//

#define     IOCTL_INDEX_SPLITTER_BASE                   2900


#define     SPLITTER_BUILD_IOCTL( _x_ )                 EMCPAL_IOCTL_CTL_CODE( (ULONG) FILE_DEVICE_SPLITTER,  \
                                                                (IOCTL_INDEX_SPLITTER_BASE + _x_),  \
                                                                EMCPAL_IOCTL_METHOD_BUFFERED,  \
                                                                EMCPAL_IOCTL_FILE_ANY_ACCESS )
                                                    

#define IOCTL_NOTIFY_SPLITTER_OPERATION_START           SPLITTER_BUILD_IOCTL( SPLITTER_NOTIFIY_OPERATION_START )
#define IOCTL_NOTIFY_SPLITTER_OPERATION_COMPLETE        SPLITTER_BUILD_IOCTL( SPLITTER_NOTIFIY_OPERATION_COMPLETE )

//
// The following defines the device name used to send a commands
// to the Splitter.
//

#define     SPLITTER_DEVICE_NAME                        "\\Device\\Kshsplt"


//
//  Type:           SPLITTER_NOTIFICATION_INPUT_BUFFER
//
//  Description:    This is the input buffer that Splitter driver receives 
//                  as part of the start operation notification.
//                  
//  Members:
//
//      OperationType
//          Operation this notification is sent for.
//
//      LuCount
//          Number of LUs in the notification.
//
//      LuIdLit[]
//          List of LUIDs in the notification.
//

typedef struct  _SPLITTER_NOTIFICATION_INPUT_BUFFER
{
    SPLITTER_NOTIFY_CLIENT_OPERATION    OperationType;
    ULONG                               LuCount;
    K10_LU_ID                           LuIdList[1];

} SPLITTER_NOTIFICATION_INPUT_BUFFER, *PSPLITTER_NOTIFICATION_INPUT_BUFFER;

//
//  Type:           SPLITTER_NOTIFICATION_OUTPUT_BUFFER
//
//  Description:    This structure contains the start operation notification 
//                  response from the Splitter driver.
//                  
//  Members:
//
//      Status
//          Overall status of the operation.
//
//      LuCount
//          Number of LUs in the notification.
//
//      LuStatus
//           Individual status of each member LU.
//

typedef struct  _SPLITTER_NOTIFICATION_OUTPUT_BUFFER
{
    
    EMCPAL_STATUS                       Status;
    ULONG                               LuCount;                        
    SPLITTER_NOTIFY_OP_START_STATUS     LuStatus[1];           

} SPLITTER_NOTIFICATION_OUTPUT_BUFFER, *PSPLITTER_NOTIFICATION_OUTPUT_BUFFER;


#endif // __RP_SPLITTER_H__

