//***************************************************************************
// Copyright (C) Data General Corporation 1989-2003
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

#ifndef _TMOS_H_
#define _TMOS_H_
//++
// File Name:
//      TMOS.h
//
// Contents:
//      Interface to TMOS - Transaction Multi-Step Operation Support Library
//
//-----------------------------------------------------------------------------
//   USING TMOS
//-----------------------------------------------------------------------------
//
// Normal Sequence of Operations
//
// 1) Client encodes all information needed to process a TMOS Operation on a
//    local or remote SP in an opaque (to TMOS) Client Data Struct.
//
// 2) Client figures out a list of "Complicit SPs" (for TMOS v 1.0., probably
//    just local and peer SPs in the same array
//
// 3) Client figures out size of per SP status 
//
// 4) Client calls TMOSStoreDocketEntry(handle, &cookie, ...) and saves cookie
//
// 5) Client initializes a per SP status template
//
// 6) Client calls TMOSStart(handle, cookie, ...)
//
// 7) TMOS Calls ClientTransition( cookie, TMOS_FSM_STATE_START, TMOS_FSM_TRANSITION_PREPARE, ...) 
//    on all Complicit SPs
//
// 8) Client calls TMOSGetClientData(handle, cookie, ...) on each SP to get argument,
//    processes Prepare() on each SP, and sets data in  POperationReport
//
// 9) TMOS calls ClientCalculateSatus() with a list of per SP Operation Reports
//
// 10) TMOS Calls ClientTransition( cookie, TMOS_FSM_STATE_PREPARED, TMOS_FSM_TRANSITION_OPERATE, ...) 
//    on all Complicit SPs
//
// 11) TMOS calls ClientCalculateSatus() with a list of per SP Operation Reports
//
// 12) TMOS calls ReturnStatus() with list of Operation Reports
//
// 13) Client calls TMOSPurgeDocketEntry(handle, cookie)
//
//-----------------------------------------------------------------------------
//   ABUSING TMOS
//-----------------------------------------------------------------------------
//
// 1) Clients can specify a non-standard start state (for example TMOS_FSM_STATE_PREPARED
//    for operations that don't need to Prepare(), or TMOS_FSM_STATE_COMPLETED to purge
//    a stale or failed operation)
//
// 2) In ClientCalculateSatus() or ClientTranstion(), Clients can set the 
//    Operation Report's Compulsory Next State to a non-standard state and TMOS
//    will ignore the FSM, and use Compulsory Next State as the next state.
//  
//
// Revision History:
//      04/18/2003    Mike P. Wagner   Created
//
//--

//++
// Include Files
//--

#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"

#ifdef __cplusplus
}
#endif

#include "CmiUpperInterface.h"

//++
// End Includes
//--

//++
// Macro:
//
//      TMOS_CLIENT_NAME_LENGTH_MAX          
//
// Description:
//      The maximum length of the Driver Name passed to TMOSInitializeHandle()
//      The Driver Name will be used to concoct a PSM Data area, which can be
//      32 wide characters
//
//
// Revision History:
//      06/12/2003    Mike P. Wagner   Created
//
//--
//

#ifdef __cplusplus

const ULONG  TMOS_CLIENT_NAME_LENGTH_MAX   =   16;

#else

    #define   TMOS_CLIENT_NAME_LENGTH_MAX          16

#endif

// .End TMOS_CLIENT_NAME_LENGTH_MAX          

//++
// Macro:
//
//      TMOS_MAX_DOCKET_ENTRIES          
//
// Description:
//      The maximum number of Docket Entries, which is the maximum number of outstading
//      TMOS operations (per driver).
//
//
// Revision History:
//      06/12/2003    Mike P. Wagner   Created
//
//--
//

#ifdef __cplusplus

const ULONG  TMOS_MAX_DOCKET_ENTRIES   =   4096;

#else

    #define   TMOS_MAX_DOCKET_ENTRIES          4096 

#endif

// .End TMOS_MAX_DOCKET_ENTRIES          

//++
// Macro:
//
//      TMOS_VERSION_ONE
//
// Description:
//      First TMOS Version
//
//
// Revision History:
//      04/18/2003    Mike P. Wagner   Created
//
//--
//
#define TMOS_VERSION_ONE                                0x1 

// .End TMOS_VERSION_ONE

//++
// Macro:
//
//      TMOS_VERSION
//
// Description:
//      Current TMOS Version
//
//
// Revision History:
//      04/18/2003    Mike P. Wagner   Created
//
//--
//
#define TMOS_VERSION                               (TMOS_VERSION_ONE)

// .End TMOS_VERSION


//++
// Type:
//      TMOS_COOKIE
//
// Description:
//      An array specific and TMOS operation identidfier
//
//      To avoid spaghetti include files, space for a CMI SPID, rather than the SPID itself
//      is used in the definition.
//
// Members:
//      Version          - used to detect "old" cookies
//      SPID             - the SPID of the originator of the operation
//      Sequence         - the nth operation for the driver on this array
//      Slot             - the Journal Entry slot used for this operation
//
// Storage Class:
//      Persisent 
//      
// Revision History:
//      04/18/04   Mike P. Wagner    Created.
//
//--
typedef struct _TMOS_COOKIE
{
    ULONG           Version;
    SPID            Spid;
    ULONG           Sequence;
    ULONG           Slot;

} TMOS_COOKIE, *PTMOS_COOKIE;

// .End TMOS_COOKIE

//++
// Macro:
//
//      TMOS_INVALID_COOKIE_VERSION
//
// Description:
//      Value to initialize an invalid cookie's version to. 
//
//
// Revision History:
//      03/8/2004    Mike Burriss   Created
//
//--
//
#define TMOS_INVALID_COOKIE_VERSION            ( 0xDEAFBABE )

// .End TMOS_INVALID_COOKIE_VERSION


//++
// Enumeration:
//
//      TMOS_FSM_STATE
//
// Description:
//      The states in the TMOS FSM      
//
// Members:
//      
//    TMOS_FSM_STATE_START       - the start state
//    TMOS_FSM_STATE_PREPARED    - Prepare has succeeded
//    TMOS_FSM_STATE_COMPLETED   - the Operate, Reject, or Purge operation
//                                 has completed
//    TMOS_FSM_STATE_MISBEGOTTEN - the Prepare tranistion failed
//    TMOS_FSM_STATE_FAILED      - the Operate transistion failed
//    TMOS_FSM_STATE_END         - the end state, the Log Entry may be removed
//
// Storage Class:
//      Ephemeral 
//      
// Revision History:
//      04/18/2003   Mike P. Wagner    Created.
//
//--
typedef enum _TMOS_FSM_STATE
{
    TMOS_FSM_STATE_INVALID = CSX_U32_MINUS_ONE,
    TMOS_FSM_STATE_START = 0,
    TMOS_FSM_STATE_PREPARED,
    TMOS_FSM_STATE_COMPLETED,
    TMOS_FSM_STATE_MISBEGOTTEN,
    TMOS_FSM_STATE_FAILED,
    TMOS_FSM_STATE_END,

    //  Add new values here...

    TMOS_FSM_STATE_MAX

}   TMOS_FSM_STATE, *PTMOS_FSM_STATE;
// .End TMOS_FSM_STATE

//++
// Enumeration:
//
//      TMOS_FSM_STRANSITION
//
// Description:
//      The states in the TMOS FSM      
//
// Members:
//      
//    TMOS_FSM_TRANSITION_INVALID        - TMOS Internal
//    TMOS_FSM_TRANSITION_NOOP           - TMOS Internal
//    TMOS_FSM_TRANSITION_COMPLETE       - TMOS Internal
//    TMOS_FSM_TRANSITION_PREPARE        - validate operation
//    TMOS_FSM_TRANSITION_OPERATE        - perform operation
//    TMOS_FSM_TRANSITION_RETURN_STATUS  - return status to client
//    TMOS_FSM_TRANSITION_REJECT         - prepare failed, clean up
//    TMOS_FSM_TRANSITION_PURGE          - operate failed, clean up
//    TMOS_FSM_TRANSITION_COMPLETE       - TMOS Internal
//
// Storage Class:
//      Ephemeral 
//      
// Revision History:
//      06/10/2003   Mike P. Wagner    Created.
//
//--
typedef enum _TMOS_FSM_TRANSITION
{
    TMOS_FSM_TRANSITION_INVALID = CSX_U32_MINUS_ONE,
    TMOS_FSM_TRANSITION_NOOP = 0,
    TMOS_FSM_TRANSITION_PREPARE,
    TMOS_FSM_TRANSITION_OPERATE,
    TMOS_FSM_TRANSITION_RETURN_STATUS,
    TMOS_FSM_TRANSITION_REJECT,
    TMOS_FSM_TRANSITION_PURGE,
    TMOS_FSM_TRANSITION_COMPLETE,

    //  Add new values here...

    TMOS_FSM_TRANSITION_MAX

}   TMOS_FSM_TRANSITION, *PTMOS_FSM_TRANSITION;
// .End TMOS_FSM_TRANSITION

//++
// Type:
//      TMOS_OPERATION_SP_REPORT
//
// Description:
//      This structure is used to convey TMOS and Client Status Codes.  It is a variable size structure,
//      since the Client Data size is set by the Client
//
// Members:
//      Version                  - used to detect "old" error packets
//      Cookie                   - TMOS Operation Identifier
//      TMOS_Status              - TMOS Status
//      ClientShortStatus        - lets the Client store status in case the
//                                 PClientStatus is corrupted
//      SP                       - SP whose report this is
//      State                    - current State of TMOS_Operation
//      CompulsoryNextSate       - used to override TMOS State Machine
//      TransitionSequence       - used to detect a response to a previous Transition
//      Sequence                 - used to detect a duplicate response to the current Transition
//      ClientStatusSize         - size of the Client Status Data
//      PClienStatus             - the "in place" Client Status Data
//
// Storage Class:
//      Ephemeral 
//      
// Revision History:
//      04/18/2003   Mike P. Wagner    Created.
//
//--
typedef struct _TMOS_OPERATION_SP_REPORT
{
    ULONG               Version;
    TMOS_COOKIE         Cookie;
    SPID                SP;
    TMOS_FSM_STATE      State;
    TMOS_FSM_STATE      CompulsoryNextState;
    EMCPAL_STATUS       TMOS_Status;
    EMCPAL_STATUS       ClientShortStatus;
    ULONG               TransitionSequence;
    ULONG               Sequence;
    ULONG               ClientStatusSize;
    UCHAR               PClientStatus[1];

} TMOS_OPERATION_SP_REPORT, *PTMOS_OPERATION_SP_REPORT;
// .End TMOS_OPERATION_SP_REPORT


//++
// Type:
//      PTMOS_TRANSITION_FUNCTION
//
// Description:
//      The type of function that TMOS will call when it needs to make a transition
//
//      The Client can use the Current state, the transition, and the information
//      in the Operation Report to take action.
//
//      The Client can also set PBClientWillProcessAsynchronously to specify Async
//      processing, and then call TMOSCompleteTransition() with the TMOS Context.
//
// Revision History:
//      06/09/2003   Mike P. Wagner    Created.
//
// Arguments
//      Cookie                            - returned from call to TMOSStoreDocketEntry()
//      CurrentState                      - current state of TMOS Operation
//      POperationReport                  - current status of TMOS Operation on this SP
//      Transition                        - FSM Transition
//      TMOSContext                       - used if Client chooses aysnc processing
//      PBClientWillProcessAsynchronously - set to TRUE to specify async processing
//      
//
//--
typedef VOID (*PTMOS_TRANSITION_FUNCTION) (
                                          TMOS_COOKIE                    Cookie,
                                          TMOS_FSM_STATE                 CurrentState,
                                          PTMOS_OPERATION_SP_REPORT      POperationReport,
                                          TMOS_FSM_TRANSITION            Transition,
                                          PVOID                          TMOSContext,
                                          PBOOLEAN                       PBClientWillProcessAsynchronously );

//.End PTMOS_TRANSITION_FUNCTION                                                      

//++
// Type:
//      PTMOS_CALCULATE_STATUS_FUNCTION
//
// Description:
//      The type of function that TMOS will call when it needs determine whether a
//      transition succedded or failed
//
// Arguments
//      Cookie           - operation identifier returned from TMOSDocketStoreEntry()
//      NumberOfReports  - number of (pointers to) Reports in the following array
//      OperationReports - an array of (pointers to) Reports
//
// Revision History:
//      06/09/2003   Mike P. Wagner    Created.
//
//--
typedef EMCPAL_STATUS  (*PTMOS_CALCULATE_STATUS_FUNCTION) (
                                                     TMOS_COOKIE                   Cookie,
                                                     ULONG                         NumberOfOperationReports,
                                                     PTMOS_OPERATION_SP_REPORT     OperationReports[]);

//.End PTMOS_CALCULATE_STATUS_FUNCTION

//++
// Type:
//      PTMOS_RETURN_STATUS_FUNCTION
//
// Description:
//      The type of function that TMOS will call when it needs determine whether a
//      transition succedded or failed
//
// Arguments
//      Cookie           - operation identifier returned from TMOSDocketStoreEntry()
//      NumberOfReports  - number of (pointers to) Reports in the following array
//      OperationReports - an array of (pointers to) Reports
//
// Revision History:
//      06/09/2003   Mike P. Wagner    Created.
//
//--
typedef VOID  (*PTMOS_RETURN_STATUS_FUNCTION) (
                                              TMOS_COOKIE                   Cookie,
                                              ULONG                         NumberOfOperationReports,
                                              PTMOS_OPERATION_SP_REPORT     OperationReports[]);

//.End PTMOS_RETURN_STATUS_FUNCTION

//++
// Type:
//      TMOS_Handle
//
// Description:
//      The handle returned from the TMOS_Initialize() call, used for all
//      subsequent operations by the Client
//
// Revision History:
//      04/18/2003   Mike P. Wagner    Created.
//
//--

#ifdef __cplusplus

namespace TMOS_FSM_Names{

//++
// Const
//    TransitionNames
//
// Description
//    For debugging
//--
    const PCHAR TransitionNames[] =
    {
        "TMOS_FSM_TRANSITION_NOOP",
        "TMOS_FSM_TRANSITION_PREPARE",
        "TMOS_FSM_TRANSITION_OPERATE",
        "TMOS_FSM_TRANSITION_RETURN_STATUS",
        "TMOS_FSM_TRANSITION_REJECT",
        "TMOS_FSM_TRANSITION_PURGE",
        "TMOS_FSM_TRANSITION_COMPLETE"
    };
//++
// End Const TransitionNames
//--

//++
// Const
//    StateNames
//
// Description
//    For debugging
//--
    const PCHAR StateNames[] =
    {
        "TMOS_FSM_STATE_START",
        "TMOS_FSM_STATE_PREPARED",
        "TMOS_FSM_STATE_COMPLETED",
        "TMOS_FSM_STATE_MISBEGOTTEN",
        "TMOS_FSM_STATE_FAILED",
        "TMOS_FSM_STATE_END",
    };
//++
// End Const StateNames
//--
};

//++
// Type:
//      TMOS_Edifice
//
// Description:
//      TMOS Edifice is an abstract base class to force all TMOS classes to 
//      define Build() and Raze() functions. This wart on TMOS's butt is a
//      workaround since C++ exceptions cannot be thrown from contructors in
//      the kernel. The idea is that all TMOS constructors *must* succeed, 
//      and any subsequent initialization will be performed in Build(). 
//
//      Raze() is provided for the sake of symmetry.
//      
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
class TMOS_Edifice
{

public:
//++
// Function:
//      TMOS_Edifice::Build
//
// Description:
//      Build() is supplied to perform any initialization that might fail.
//
//      The final action of a successful Build member should call Certify()
//
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--

    virtual EMCPAL_STATUS Build() = 0;

//++
// End TMOS_Edifice::Build
//--

//++
// Function:
//      TMOS_Edifice::Raze
//
// Description:
//      Undoes any actions taken by Build
//
//      The final action of a Raze member function must set the Certified member
//      to FALSE;
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--

    virtual VOID Raze() = 0;

//++
// End TMOS_Edifice::Raze
//--

//++
// Function:
//      TMOS_Edifice::Certified
//
// Description:
//      Has the Edifice been Certified?
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
    virtual BOOLEAN Certified() const
    {
        return Certification;
    }
//++
// End function  TMOS_Edifice::Certified
//--


//++
// Function:
//      TMOS_Edifice:Certify
//
// Description:
//      Certify the Edifice
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
    virtual void Certify()
    {
        Certification = TRUE;
    }
//++
// End function  TMOS_Edifice::Certify
//--

//++
// Function:
//      TMOS_Edifice:Certify
//
// Description:
//      Certify the Edifice
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
    virtual void DeCertify()
    {
        Certification = FALSE;
    }
//++
// End function  TMOS_Edifice::Certify
//--

protected:

//++
// Function:
//      TMOS_Edifice::TMOS_Edifice
//
// Description:
//      ctor
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
    TMOS_Edifice():Certification(FALSE)
    {
    }
//++
// End TMOS_Edifice::TMOS_Edifice
//--

//++
// Function:
//      TMOS_Edifice::~TMOS_Edifice
//
// Description:
//      dtor
//      Scott Meyer says "Make destructors virtual in base classes."
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
    virtual ~TMOS_Edifice()
    {
    }
//++
// End TMOS_Edifice::~TMOS_Edifice
//--

//++
// End TMOS_Edifice::Raze
//--
private:

    BOOLEAN                         Certification;

};

//++
// End class TMOS_Edifice
//--

//++
// Class:
//      TMOS_Handle
//
// Description:
//      TMOS Handle is an "empty" interface for Clients. The interface will 
//      actually be implemented fully in TMOS_Body
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//        
//--
class TMOS_Body;

//++
// End Class TMOS_Handle
//--
//++
// Type:
//      TMOS_Handle
//
// Description:
//      An empty struct for the C compiler
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
typedef struct _TMOS_Handle 
{
    ULONG               Initialized;
    class TMOS_Body*    PBody;

} TMOS_Handle;

//++
// End struct TMOS_Handle
//--

#else

//++
// Type:
//      TMOS_Handle
//
// Description:
//      An empty struct for the C compiler
//
// Revision History:
//      06/11/2003   Mike P. Wagner    Created.
//
//--
typedef struct _TMOS_Handle 
{
    ULONG               Initialized;
    struct _TMOS_Body * PBody;

} TMOS_Handle;
//++
// End struct TMOS_Handle
//--

#endif


typedef TMOS_Handle*  PTMOS_Handle;


#ifdef __cplusplus
extern "C" {
#endif

    EMCPAL_STATUS
    TMOSInitializeHandle( PTMOS_Handle                    PHandle,
                          csx_pchar_t                          ClientName, 
                          ULONG                           NumberOfDocketEntries,
                          ULONG                           SizeOfDocketEntry,
                          ULONG                           DefaultNumberOfComplicitSPs,
                          ULONG                           DefaultClientStatusSize,
                          PEMCPAL_ALLOC_FUNCTION              PAlloc,
                          PEMCPAL_FREE_FUNCTION                  PFree,
                          PTMOS_TRANSITION_FUNCTION       PTransition,
                          PTMOS_CALCULATE_STATUS_FUNCTION PCalculateStatus,
                          PTMOS_RETURN_STATUS_FUNCTION    PReturnStatus,
                          ULONG                           MPSDomainName
                        );

    EMCPAL_STATUS
    TMOSUninitializeHandle(
                    PTMOS_Handle                    PHandle,
                    csx_pchar_t                          ClientName,
                    PEMCPAL_FREE_FUNCTION                  PFree
                    );


    EMCPAL_STATUS
    TMOSInitializeHandleWithConduit( PTMOS_Handle                    PHandle,
                                     csx_pchar_t                     ClientName, 
                                     ULONG                           NumberOfDocketEntries,
                                     ULONG                           SizeOfDocketEntry,
                                     ULONG                           DefaultNumberOfComplicitSPs,
                                     ULONG                           DefaultClientStatusSize,
                                     PEMCPAL_ALLOC_FUNCTION              PAlloc,
                                     PEMCPAL_FREE_FUNCTION                  PFree,
                                     PTMOS_TRANSITION_FUNCTION       PTransition,
                                     PTMOS_CALCULATE_STATUS_FUNCTION PCalculateStatus,
                                     PTMOS_RETURN_STATUS_FUNCTION    PReturnStatus,
                                     ULONG                           MPSDomainName,
                                     CMI_CONDUIT_ID                  MPSConduit
                                   );
    EMCPAL_STATUS
    TMOSRebuildDocket( csx_pchar_t                          ClientName, 
                       ULONG                           Counter,
                       ULONG                           NumberOfDocketEntries,
                       ULONG                           SizeOfDocketEntry,
                       PEMCPAL_ALLOC_FUNCTION              PAlloc,
                       PEMCPAL_FREE_FUNCTION                  PFree);

    EMCPAL_STATUS 
    TMOSStoreDocketEntry( TMOS_Handle    Handle,
                          PTMOS_COOKIE   Cookie,
                          ULONG          NumberOfComplicitSPs,
                          PSPID          PSComplicitSPs,
                          ULONG          ClientStatusSize,
                          ULONG          ClientDataSize,
                          PVOID          PClientData);

    EMCPAL_STATUS    TMOSPurgeDocketEntry( TMOS_Handle    Handle,
                                      TMOS_COOKIE    Cookie);
    EMCPAL_STATUS 
    TMOSGetDocketEntry( TMOS_Handle    Handle,
                        TMOS_COOKIE    Cookie,
                        PULONG         PNumberOfComplicitSPs,
                        PSPID          PComplicitSPs,
                        PULONG         PClientStatusSize,
                        PULONG         PClientDataSize,
                        PVOID          PClientData);
    EMCPAL_STATUS 
    TMOSGetClientData( TMOS_Handle    Handle,
                       TMOS_COOKIE    Cookie,
                       PULONG         PClientDataSize,
                       PVOID          PClientData);
    EMCPAL_STATUS 
    TMOSStoreClientData( TMOS_Handle    Handle,
                         TMOS_COOKIE    Cookie,
                         ULONG          ClientDataSize,
                         PVOID          PClientData);
    EMCPAL_STATUS 
    TMOSStart( TMOS_Handle     Handle,
               TMOS_COOKIE     Cookie,
               ULONG           StatusSize,
               PVOID           InitialClientStatus,
               TMOS_FSM_STATE  Start);

    EMCPAL_STATUS
    TMOSAsyncCompleteTransition( TMOS_Handle                    Handle,
                                 TMOS_COOKIE                    Cookie,
                                 PVOID                          TMOSContext,
                                 PTMOS_OPERATION_SP_REPORT      POperationReport);

    EMCPAL_STATUS 
    TMOSGetFirstCookie( TMOS_Handle     Handle, 
                        PTMOS_COOKIE     PCookie);

    EMCPAL_STATUS 
    TMOSGetNextCookie( TMOS_Handle     Handle, 
                       TMOS_COOKIE     CurrentCookie,
                       PTMOS_COOKIE    PNextCookie);

#ifdef __cplusplus

}

#endif

//.End TMOS_Handle                                                               

#endif  // _TMOS_H_

// End TMOS.h
