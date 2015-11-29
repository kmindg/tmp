#ifndef _DLS_TYPES_H_
#define _DLS_TYPES_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-2000
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DlsTypes.h
//
// Contents:
//      external DLS lock declarations
//
// Constants:
//
//      DlsCurrentVersion
//      DLS_CURRENT_VERSION
//      DlsLockMonickerMaxNameLen
//      DLS_lOCK_MONICKER_MAX_NAME_LEN
//      DlsLockDomainMaxNameLen
//      DLS_LOCK_DOMAIN_MAX_NAME_LEN
//      DlsCabalMaxMembers
//      DLS_CABAL_MAX_MEMBERS
//      DLS_INVALID_FAIL_FAST_DELAY
//      DLS_DEFAULT_FAIL_FAST_DELAY
//
// Types:
//
//      DLS_LOCK_HANDLE
//      DLS_LOCK_NAME
//      DLS_LOCK_MAILBOX
//      DLS_LOCK_MODE
//      DLS_EVENT
//      DLS_OPENLOCK_EVENT_DATA
//      DLS_CLOSELOCK_EVENT_DATA
//      DLS_CONVERTLOCK_EVENT_DATA
//      DLS_EVENT_DATA
//      DLS_QUERYLOCKMODE_EVENT_DATA
//      DLS_FORCEMAILBOX_EVENT_DATA
//      DLS_REPLACEMAILBOX_EVENT_DATA
//      DLS_UPROOTMAILBOX_EVENT_DATA
//
// Revision History:
//      01-Nov-98   MWagner    Created.
//
//--

#ifdef __cplusplus
extern "C" {
#endif

//++
// Variable:
//      DlsCurrentVersion
//
// Description:
//      The version of the DLS
//
// Revision History:
//      06-Jan-99    MWagner  Created.
//
//--
#define DLS_VERSION_1       1
#define DLS_VERSION_2       2

#define DLS_CURRENT_VERSION DLS_VERSION_2 
#define DlsCurrentVersion   DLS_CURRENT_VERSION
//.End                                                                        


//++
// Variable:
//      DlsLockMonickerMaxNameLen
//
// Description:
//      The length in bytes of a DLS Lock Monicker
//
// Revision History:
//      06-Jan-99    MWagner  Created.
//
//--
#define DlsLockMonickerMaxNameLen 256 
#define DLS_LOCK_MONICKER_MAX_NAME_LEN 256
//.End                                                                        

//++
// Variable:
//      DlsLockDomainMaxNameLen
//
// Description:
//      The length in bytes of a DLS Lock Domain
//
// Revision History:
//      06-Jan-99    MWagner  Created.
//
//--
#define DlsLockDomainMaxNameLen 256 
#define DLS_LOCK_DOMAIN_MAX_NAME_LEN 256
//.End                                                                        


//++
// Variable:
//      DlsCabalMaxMembers
//
// Description:
//      The maximum number of SPs in a Cabal
//
// Revision History:
//      06-Jan-99    MWagner  Created.
//
//--
#define DlsCabalMaxMembers 2
#define DLS_CABAL_MAX_MEMBERS 2
//.End                                                                        

//++
//Variable:
//      DLS_INVALID_FAIL_FAST_DELAY
//
// Description:
//      Use this value as the Fail Fast Delay parameter
//      for DistributedLockServiceOpenLock() to "turn
//      off" Fail Fast behavior.
//      
//
// Revision History:
//      5/1/00   MWagner    Created.
//
//--
#define  DLS_INVALID_FAIL_FAST_DELAY      (0xffffffff)
//.End DLS_INVALID_FAIL_FAST_DELAY                                                                       


//++
//Variable:
//      DLS_DEFAULT_FAIL_FAST_DELAY
//
// Description:
//      Use this value as the Fail Fast Delay parameter
//      for DistributedLockServiceOpenLock() to specify
//      the default timeout for Dls Fail Fast behavior.
//      
//
// Revision History:
//      5/1/00   MWagner    Created.
//
//--
#define  DLS_DEFAULT_FAIL_FAST_DELAY      (0x0)
//.End DLS_DEFAULT_FAIL_FAST_DELAY                                                                       

//++
// Type:
//      DLS_LOCK_HANDLE
//
// Description:
//      A DLS lock handle
//
// Revision History:
//      02-Dec-98   MWagner    Created.
//
//--
typedef PVOID DLS_LOCK_HANDLE, *PDLS_LOCK_HANDLE;

//++
// Type:
//      UDLS_LOCK_HANDLE
//
// Description:
//      A uDLS lock handle
//
// Revision History:
//      02-Dec-98   MWagner    Created.
//
//--
typedef ULONGLONG UDLS_LOCK_HANDLE, *PUDLS_LOCK_HANDLE;


//.End                                                                        
//++
// Type:
//      DLS_LOCK_NAME
//
// Description:
//      A DLS Lock Name
//
// Members:
//      ULONG       dlnVersion      :  version number
//      CHAR        dlnDomain[DlsLockDomainMaxNameLen]    
//                                   :  a "domain" name 
//      CHAR        dlnMonicker[DlsLockMonickerMaxNameLen]
//                                   : a name within domain
//
// Revision History:
//      06-Jan-99   MWagner  Created.
//
//--
typedef struct _DLS_LOCK_NAME
{
      ULONG       dlnVersion;
      CHAR        dlnDomain[DlsLockDomainMaxNameLen]; 
      CHAR        dlnMonicker[DlsLockMonickerMaxNameLen]; 
} DLS_LOCK_NAME, *PDLS_LOCK_NAME;
//.End                                                                        

//++
// Type:
//      DLS_LOCK_MAILBOX
//
// Description:
//      A DLS MailBox
//
//      n.b., the prefix is DLMB to avoid conflicts with the
//            DLS Lock Map DLM prefix.
//
// Members:
//      ULONG       DHM_Version      :  version number
//      ULONG       dlmbSize         :  the size (in bytes) of the
//                                      data 
//      ULONG       dlmbBytesUsed    :  the number of bytes acutally
//                                      used in the data, the number
//                                      of bytes the DLS should actually
//                                      send to anohter SP to synchronize
//                                      the mailbox
//      CHAR        dlmbData[1]      :  the data
//
// Access:
//      Allocated from non-paged pool.
//      Available when IRQL <= DISPATCH_LEVEL
//
// Revision History:
//      02-Dec-98   MWagner    Created.
//
//--
typedef struct _DLS_LOCK_MAILBOX
{
    ULONG       dlmbVersion      ;  
    ULONG       dlmbSize         ;  
    ULONG       dlmbBytesUsed    ;  
    CHAR        dlmbData[1]      ;
     
} DLS_LOCK_MAILBOX, *PDLS_LOCK_MAILBOX;
//.End                                                                        

//++
// Enum:
//      DLS_LOCK_MODE
//
// Description:
//      Access modes of a DLS Lock
//
//
// Members:
//      DlsLockModeInvalid = -1,
//      DlsLockModeNull           Unlocked
//      DlsLockModeRead           Shared Lock
//      DlsLockModeWrite          Exclusive Lock
//      DlsLockModeMax
//
// Revision History:
//      18-Dec-98   MWagner    Created.
//
//--
typedef enum _DLS_LOCK_MODE
{
    DlsLockModeInvalid = -1,
    DlsLockModeNull,
    DlsLockModeRead,  
    DlsLockModeWrite,
    DlsLockModeMax
} DLS_LOCK_MODE, *PDLS_LOCK_MODE;
//.End                                                                        

//++
// Enum:
//      DLS_EVENT_TYPE
//
// Description:
//      Lock Op Event types
//
// Members:
//  DlsEventInvalid = -1,
//  DlsEventOpenLock,
//  DlsEventCloseLock,
//  DlsEventConvertLock,
//  DlsEventConvertConflict,
//  DlsEventQueryLockMode,
//  DlsEventForceMailBox,
//  DlsEventReplaceMailBox,
//  DlsEventUprootMailBox,
//  DlsEventConvertComplete
//
// Revision History:
//      18-Dec-98   MWagner    Created.
//
//--
typedef enum _DLS_EVENT
{
    DlsEventInvalid = -1,
    DlsEventOpenLock,
    DlsEventCloseLock,
    DlsEventConvertLock,
    DlsEventConvertConflict,
    DlsEventQueryLockMode,
    DlsEventForceMailBox,
    DlsEventReplaceMailBox,
    DlsEventUprootMailBox,
    DlsEventConvertComplete
} DLS_EVENT, *PDLS_EVENT;
//.End                                                                        

//++++++++++++
// EVENT DATA
//
// n.b,, the various event data structures violate the
//       the DLS structure member prefix convenstions-
//       they all start with DED
//------------

//++
// Type:
//      DLS_EVENT_DATA
//
// Description:
//      The event returned the the DLS client
//
// Members:
//      ULONG       DE_Version  :  version number
//
// Revision History:
//      07-Jan-99   MWagner  Created.
//
//--
typedef struct _DLS_EVENT_DATA
{
        DLS_LOCK_HANDLE     dedLock;
        PVOID               dedContext;
        EMCPAL_STATUS       dedStatus;
        DLS_LOCK_MODE       dedMode;
} DLS_EVENT_DATA, *PDLS_EVENT_DATA;
//.End                                                                        


//++
// Type:
//      DLS_LOCK_CALLBACKFUNC
//
// Description:
//      The DLS Lock Callback function type
//
//      This callback may be called in an arbitary thread context
//      (in a CMI callback), so it may run at DISPATCH level.
//
// Revision History:
//      06-Jan-99   MWagner  Created.
//
//--
typedef EMCPAL_STATUS (*DLS_LOCK_CALLBACKFUNC)(DLS_EVENT event, PVOID data);
typedef DLS_LOCK_CALLBACKFUNC* PDLS_LOCK_CALLBACKFUNC;

#ifdef __cplusplus
};
#endif

//.End                                                                        

#endif // __DLS_TYPES__
