/***********************************************************************************
 
 Copyright (C)  EMC Corporation 2009
 All rights reserved.
 Licensed material - property of EMC Corporation.

 File Name:
    ppfdAsynchEvent.c
    
 Contents:
    PPFD Event Queue related functions.  Physical Package (PP) "State Change Message" events are
    inserted in a queue when the PPFD callback registered with PP is called (see ppfdStateChangeCallback).
    The code in ths file handles inserting these events in the queue, pulling them off the queue,
    translating them into CPD event info messages (for NtMirror and ASIDC), and sending the
    CPD EVent info to the clients that registered for callbacks via CPD_IOCTL_REGISTER_CALLBACKS.

 ***********************************************************************************/
 
#include "ppfdMisc.h"
#include "ppfdKtraceProto.h"
#include "ppfd_panic.h"
#include "fbe/fbe_notification_lib.h"

extern PPFD_CALLBACK_REGISTRATION_DATA ppfdCallbackRegistrationInfo;
extern PPFD_GLOBAL_DATA PPFD_GDS;
extern fbe_semaphore_t      crash_dump_info_update_semaphore;
// Function Prototypes
EMCPAL_LOCAL BOOL
ppfdTranslatePPStateMsgToCPDEventInfo( update_object_msg_t *pStateChangeMsg,
                                       CPD_EVENT_INFO* pCpdEventInfo,
                                       CPD_EVENT_TYPE* pEvent2 );
EMCPAL_LOCAL BOOL
ppfdSendEventToClients( CPD_EVENT_INFO *pCpdEventInfo,
                        CPD_EVENT_TYPE CpdEventType2 );

// Imported Functions
ULONG ppfdPhysicalPackageGetLoopIDFromMap( ULONG Drive, BOOL IsPnPDisk );
BOOL ppfdPhysicalPackageInitCrashDump( void );
BOOL ppfdIsStateChangeMsgForCurrentSPBootDrive(  update_object_msg_t *pStateChangeMsg );

/***************************************************************************
 
 Function:
    void ppfdHandleAsyncEvent( PPFD_EVENT_INFO *pEvent)

 Description:

 Arguments: 
  
    pEvent   - Pointer to the PPFD_EVENT_INFO structure for this event
 
  Return Value:
 
   None
 
 ***************************************************************************/
void ppfdHandleAsyncEvent( update_object_msg_t *PPStateChangeMsg)
{
    update_object_msg_t StateChangeMsg;
    CPD_EVENT_TYPE CpdEventType2;
    CPD_EVENT_INFO CpdEventInfo;

    if (PPStateChangeMsg == NULL)
    {
        /* this should NEVER happen */
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdHandleAsyncEvent failed NULL Input pointer!\n" );       
        panic(PPFD_ASYNC_EVENT_INFO_INVALID,(ULONG_PTR)PPStateChangeMsg);
        return;
    }

    // Initialize the crash dump by getting the phys/slot mapping for the enclosure from Physicalpackage
    // and senidng the info to the minipport.  We can't do this before the enclosure object has been discovered by PP, 
    // so we'll do it here one time. The intent is to only initialize the crash dump support once during startup when we 
    // know the PhysicalPackage and miniport are ready.  By doing it here in the asynch event handler, we know that a disk 
    // event has been received so PP is ready.
    //
    //if(!(PPFD_GDS.bCrashDumpPrimaryInitialized && PPFD_GDS.bCrashDumpSecondaryInitialized))
    {
        //Send the notification every time a boot drive is inserted.
        //This update is to cover a scenario where enclosure 0 expanders are swapped.
        //ppfdPhysicalPackageInitCrashDump(); 
        if (ppfdIsStateChangeMsgForCurrentSPBootDrive(PPStateChangeMsg)){
            PPFD_TRACE(PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: Initiating crash dump drive mapping notification.\n");
            fbe_semaphore_release(&crash_dump_info_update_semaphore, 0, 1, FBE_FALSE);
        }
    }

    // Translate the event to a CPD Event Info and send it to clients that have registered with
    // PPFD (Ntmirror and possibly ASIDC)
    PPFD_TRACE(PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: ppfdHandleAsyncEvent PP State=0x%llX\n",
        (unsigned long long)PPStateChangeMsg->notification_info.notification_type );       

    fbe_zero_memory(&CpdEventInfo,sizeof(CpdEventInfo));
    fbe_copy_memory (&StateChangeMsg, PPStateChangeMsg, sizeof(update_object_msg_t));

    // Translate the Physical Package State change Msg to a "CPD Event Info" message and
    // call the registered clients if successful in the translation.
    if(  ppfdTranslatePPStateMsgToCPDEventInfo( &StateChangeMsg, &CpdEventInfo, &CpdEventType2 ))
    {
        // Call the registered clients
        // Note that if CpdEventType2 is filled in with an event, then the function belwo will call teh client
        // 2 times with 2 events.  This is because some of the PP state change messages translate to 2 different
        // CPD event info messages that need to be sent consecutively.
        //
        PPFD_TRACE( PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: ppfdHandleAsyncEvent calling registered clients\n");        
        ppfdSendEventToClients( &CpdEventInfo, CpdEventType2 );
        PPFD_TRACE(PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: ppfdHandleAsyncEvent returned from calling clients\n");        
    }
    else
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdHandleAsyncEvent [ERROR] Could not translate the state change msg to CPD event\n");        
    }
}

/*******************************************************************************************
 
 Function:
    BOOL ppfdTranslatePPStateMsgToCPDEventInfo( update_object_msg_t *pStateChangeMsg,
                                CPD_EVENT_INFO* pCpdEventInfo, CPD_EVENT_TYPE* pEvent2  )

 Description:
    Translate the "lifecycle state" (defined by the PhysicalPackage) into a CPD_EVENT_XXX using "a priori" knowledge of the desired
    mapping of lifecycle states to CPD events.  A single lifecycle state may require 2 CPD events, so the parameters to this function
    include 2 CPD event pointers to fill in the data.

    It is assumed that the message passed to this function for translation is for one of the logical drives 0-4
    on enclosure 0, port 0. No checking is done to "filter" out translating messages for other
    device state changes.

 Arguments:
    life_cycle_state - The PhysicalPackage defined lifecycle state to be translated (example FBE_LIFECYCLE_STATE_READY)
    pCpdEventInfo - Pointer to CPD_EVENT_INFO structure to be filled in after translating the life)
    pEvent2 -

 Return Value:
   TRUE 
   FALSE

 Others:   

 Revision History:

*******************************************************************************************/
EMCPAL_LOCAL BOOL
ppfdTranslatePPStateMsgToCPDEventInfo( update_object_msg_t *pStateChangeMsg,
                                       CPD_EVENT_INFO* pCpdEventInfo,
                                       CPD_EVENT_TYPE* pEvent2 )
{
    CPD_EVENT_TYPE CPDEvent1=0;
    CPD_EVENT_TYPE CPDEvent2=0;
    BOOL bStatus=TRUE;
    fbe_lifecycle_state_t LifeCycleState;
	fbe_s32_t Drive;
    fbe_notification_type_t	NotificationType;
    
    if ((pStateChangeMsg==NULL)||(pCpdEventInfo==NULL) ||(pEvent2 ==NULL ))
    {
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdTranslatePPStateMsgToCPDEventInfo Invalid Parameter 0x%p 0x%p 0x%p!\n",pStateChangeMsg, pCpdEventInfo,pEvent2 );            
        return( FALSE );
    }

    fbe_notification_convert_notification_type_to_state(pStateChangeMsg->notification_info.notification_type, &LifeCycleState);
    Drive = pStateChangeMsg->drive;
    NotificationType = pStateChangeMsg->notification_info.notification_type;
    //
    // First translate the life cycle state to a CPD_EVENT_XXXX
    //
    switch( LifeCycleState )
    {
        case FBE_LIFECYCLE_STATE_READY:
            CPDEvent1 = CPD_EVENT_LOGIN;
            break;

        // For all of these, send a logout followed by device failed
        //

        // If the drive is removed, we're getting the state FBE_LIFECYCLE_STATE_PENDING_ACTIVATE followed by
        // FBE_LIFECYCLE_STATE_PENDING_DESTROY
        //
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
        //case FBE_LIFECYCLE_STATE_DESTROY:
            CPDEvent1 = CPD_EVENT_LOGOUT;
            CPDEvent2 = CPD_EVENT_DEVICE_FAILED;
            PPFD_TRACE(PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: ppfdTranslatePPStateToCPDEvent LO and DF State 0x%x\n", LifeCycleState );        
            break;

        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            PPFD_TRACE(PPFD_TRACE_FLAG_ASYNC, TRC_K_STD, "PPFD: ppfdTranslatePPStateToCPDEvent LO State FBE_LIFECYCLE_STATE_PENDING_ACTIVATE\n");        
            CPDEvent1 = CPD_EVENT_LOGOUT;
            //CPDEvent2 = CPD_EVENT_DEVICE_FAILED;
            break;

        case FBE_LIFECYCLE_STATE_ACTIVATE:
        case FBE_LIFECYCLE_STATE_OFFLINE:
            // Ignore these states, return failure indicates to caller that there is no event mapping needed for this event
            bStatus = FALSE;
            break;

        default:
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_STD, "PPFD: ppfdTranslatePPStateToCPDEvent Unknown FBE Lifecycle State 0x%x\n", LifeCycleState );        
            bStatus=FALSE;
    }

    if( bStatus )
    {
        pCpdEventInfo->type=CPDEvent1;
        *pEvent2=CPDEvent2;     // this can be 0 if the lifecycle state has a one to one mapping with a CPD event
    }


    // We really can't translate the "LoopID" (u.port_login.miniport_login_context)here.
    // That could be either 128,129,130,131 (NtMirror) or 0,1,2,3 (ASIDC non aliased).  All we know
    // from the PP is the drive object ID which can map to either of these loop ID's
    // So, that translation will be done before doing the callback based on the which callback is
    // being executed..the callback that was registered by NtMirror or the callback registered by ASIDC

    // Just set it for 0,1,2,3 or 4 here and let the final translation occur when sending the event
    // Drive should be 0-4 here
    pCpdEventInfo->u.port_login.miniport_login_context = (void *)(UINT_PTR) Drive;

    return( bStatus );
}

/*******************************************************************************************

 Function:
    BOOL ppfdSendEventToClients( CPD_EVENT_INFO *pCpdEventInfo, CPD_EVENT_TYPE CpdEventType2 )

 
 Description:
 
 Arguments:
    none

 Return Value:

 Others:   

 Revision History:

*******************************************************************************************/
EMCPAL_LOCAL BOOL
ppfdSendEventToClients( CPD_EVENT_INFO *pCpdEventInfo,
                        CPD_EVENT_TYPE CpdEventType2 )
{
    PPFD_CALLBACK_DATA *p_ppfdCallback;
    UCHAR CallbackEntry;
    CPD_EVENT_INFO localCpdEventInfo;
    BOOL bStatus = TRUE;
    ULONG LoopID;
    ULONG Drive;

    // Copy CPD event info to our local variable. This allows us to make any last second changes (before sending the event) 
    // if we want to without changing the original data passed to us..
    fbe_copy_memory (&localCpdEventInfo, pCpdEventInfo, sizeof(CPD_EVENT_INFO));

    // Get the "loopid" for this drive.  The loopid is really only applicable to Fibre channel, but
    // we can generate this same ID in PPFD for SAS/SATA so NtMirror and ASIDC can remain unchanged
    // The loop ID could be 128,129,130,131 (for NtMirror aliases), or 0,1,2,3 for ASIDC
    // 

    // The first callback registered will be for NtMirror, so we can translate the LoopID for the pnp drive first
    // miniport_login_context should be 0-4 at this point based on the tranlsation done in 
    // ppfdTranslatePPStateMsgToCPDEventInfo
    //
    Drive = (ULONG)CPD_GET_LOGIN_CONTEXT_FROM_LOOPMAP_INDEX_ULONG(localCpdEventInfo.u.port_login.miniport_login_context);

    LoopID = ppfdPhysicalPackageGetLoopIDFromMap( Drive, TRUE ); //TRUE=PnP Drive xlation
    localCpdEventInfo.u.port_login.miniport_login_context=(void *)(UINT_PTR)LoopID;

    for( CallbackEntry=0; CallbackEntry < PPFD_MAX_CLIENTS;  CallbackEntry++ )
    {
        // For the second callback, update the LoopId
        if( CallbackEntry != 0 )
        {
            LoopID = ppfdPhysicalPackageGetLoopIDFromMap( Drive, FALSE );
            localCpdEventInfo.u.port_login.miniport_login_context=(void *)(UINT_PTR)LoopID;
        }

        p_ppfdCallback = &ppfdCallbackRegistrationInfo.RegisteredCallbacks[CallbackEntry];
        if( p_ppfdCallback->async_callback )
        {
            p_ppfdCallback->async_callback ( p_ppfdCallback->context, &localCpdEventInfo );
            // Send the second event if there is one..
            if( CpdEventType2 )
            {
                localCpdEventInfo.type = CpdEventType2;
                p_ppfdCallback->async_callback ( p_ppfdCallback->context, &localCpdEventInfo );
            }
        }
    }

    return( bStatus );
}
