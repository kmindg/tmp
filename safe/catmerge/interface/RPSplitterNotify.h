/*******************************************************************************
 * Copyright (C) EMC Corporation, 2007
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

//
//  File:
//
//      RPSplitterNotify.h
//
//  Description:
//
//      This header file contains all the interfaces exported by the 
//      Splitter notification library.
//      
//  History:
//
//      28-Aug-2007    NS;       Initial creation.
//

#ifndef __RP_SPLITTER_NOTIFY_H__
#define __RP_SPLITTER_NOTIFY_H__

#include "K10RPSplitterMessages.h"

//
//  The following defines the length of the Splitter notification library's
//  client name.
//


#define             SPLITTER_NOTIFY_CLIENT_NAME_LENGTH          64

//
//  The following defines the string used to store client of the notification 
//  library.
//

typedef char        SPLITTER_NOTIFY_CLIENT_NAME[ SPLITTER_NOTIFY_CLIENT_NAME_LENGTH + 1];


//
//  The following defines various layered driver operations that 
//  can cause notifications to be sent to the Splitter driver.
//

typedef enum SPLITTER_NOTIFY_CLIENT_OPERATION
{
    SplitterNotifyClientOperationInvalid = -1,
    SplitterNotifyClientOperationRollbackStart,
    SplitterNotifyClientOperationRollbackStartComplete,
    SplitterNotifyClientOperationRollbackComplete,
    SplitterNotifyClientOperationClonesReverseSyncStart,
    SplitterNotifyClientOperationClonesReverseSyncStartComplete,
    SplitterNotifyClientOperationClonesReverseSyncComplete,
    SplitterNotifyClientOperationCPMCopyStart,
    SplitterNotifyClientOperationCPMCopyComplete,
    SplitterNotifyClientOperationMluSnapShotRollbackStart,
    SplitterNotifyClientOperationMluSnapShotRollbackComplete,
    SplitterNotifyClientOperationMluSnapShotAttachStart,
    SplitterNotifyClientOperationMluSnapShotAttachComplete,
    SplitterNotifyClientOperationMax

}SPLITTER_NOTIFY_CLIENT_OPERATION, *PSPLITTER_NOTIFY_CLIENT_OPERATION;

//
//  Type:           SPLITTER_NOTIFY_OP_START_STATUS
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
//          Status returned by the Splitter driver for the LU.
//

typedef struct  _SPLITTER_NOTIFY_OP_START_STATUS
{
    K10_LU_ID               LuId;
    EMCPAL_STATUS           LuStatus;

}SPLITTER_NOTIFY_OP_START_STATUS, *PSPLITTER_NOTIFY_OP_START_STATUS;


EMCPAL_STATUS
SplitterNotifyInitialize(
    IN SPLITTER_NOTIFY_CLIENT_NAME          ClientName
    );


EMCPAL_STATUS
SplitterNotifyShutdown( void );


EMCPAL_STATUS
SplitterNotifyOpStart(
    IN      SPLITTER_NOTIFY_CLIENT_OPERATION	ClientOpType,
    IN      PK10_LU_ID          				pLuIdList,
    IN      ULONG               				LuCount,
    IN OUT  SPLITTER_NOTIFY_OP_START_STATUS	    LuStatus[]
    );

EMCPAL_STATUS
SplitterNotifyOpComplete(
    IN  SPLITTER_NOTIFY_CLIENT_OPERATION    	ClientOpType,
    IN  PK10_LU_ID          				    pLuIdList,
    IN  ULONG               					LuCount    
    );

#endif // __RP_SPLITTER_NOTIFY_H__






