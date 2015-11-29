/************************************************************************
*
*  Copyright (c) 2002, 2005-2013 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/

#ifndef __LockedListOfCancelableBasicIoRequest__
#define __LockedListOfCancelableBasicIoRequest__
//++
// File Name:
//      BasicIoRequest.h
//
// Contents:
//  Defines the LockedListOfCancelableBasicIoRequest class.
//
//
// Revision History:
//--



#include "BasicIoRequest.h"
#include "BasicLockedObject.h"


// A list of BasicIoRequests that maintains is own locking (but allowing the user access
// to those locks), and hiding all aspects of cancel handling from the the user. 
// Cancelled requests disappear from the list and are completed with STATUS_CANCELLED. 
// Adding a cancelled request to the list results in the request being completed with
// STATUS_CANCELLED (except for adds with locks held).
// FIX: Not tested...
class LockedListOfCancelableBasicIoRequest : protected ListOfCancelableBasicIoRequests, public BasicLockedObject {
public:
    // Determine if the list is empty where empty ignores BasicIoRequests that are being
    // canceled.
    // Returns: True if there are no BasicIoRequests on the list except ones which are being canceled.
    bool IsEmptyLockHeld() const { return ListOfCancelableBasicIoRequests::IsEmptyLockHeld(); }
    
    // Determine if the list is empty where empty includes BasicIoRequests that are being canceled.
    // Returns: True if there are no BasicIoRequests on the list.
    bool IsEmptyNoLockHeld() const { return ListOfCancelableBasicIoRequests::IsEmptyNoLockHeld(); }

    // Remove an item from the head of the list.  Internally provides all locking.
    // @param param1 - optional output parameter returning optional input parameter set when queued.
    // Returns: the item removed from the list.  NULL if the list was empty (since the
    // lock is not held, it is was, not is).
    PBasicIoRequest RemoveHead(PVOID * param1 = NULL) {
        AcquireSpinLock();
        BasicIoRequest * irp = ListOfCancelableBasicIoRequests::RemoveHead();
        if (irp && param1) 
        {
            PVOID junk;
            
            irp->RetrievePointerWhileRequestOwner(&junk,param1);
        }
        ReleaseSpinLock();
        return irp;
    }

    // Remove an item from the head of the list.  This caller must have previously called
    // AcuireSpinLock.
        // @param param1 - optional output parameter returning optional input parameter set when queued.
    // Returns: the item removed from the list.  NULL if the list is empty.
    PBasicIoRequest RemoveHeadLockHeld(PVOID * param1 = NULL) {
        BasicIoRequest * irp = ListOfCancelableBasicIoRequests::RemoveHead();
        if (irp && param1) 
        {
            PVOID junk;
            
            irp->RetrievePointerWhileRequestOwner(&junk,param1);
        }
        return irp;
    }

    
    // Remove the specific request from  list.  Internally provides all locking.
    // Returns: true - the request was removed from the list, false the item was not on
    // the list.
    bool Remove(PBasicIoRequest Irp) {
        AcquireSpinLock();
        bool result = ListOfCancelableBasicIoRequests::Remove(Irp);
        ReleaseSpinLock();
        return result;
    }

    // Remove the specific request from  list.  
    // Returns: true - the request was removed from the list, false the item was not on
    // the list.
    bool RemoveLockHeld(PBasicIoRequest Irp) {
        return ListOfCancelableBasicIoRequests::Remove(Irp);
    }

    // Add a requesst to the head of the list.  If the request is already cancelled, it
    // will be completed prior to returning.
    // @param Irp - the IRP to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    // Note: utilizes BasicIoRequest::SavePointerWhileRequestOwner(), so the caller
    // cannot.
    void AddHead(PBasicIoRequest Irp, PVOID param1 = NULL){

        AcquireSpinLock();
        Irp->SavePointerWhileRequestOwner(this, param1);
        if (!ListOfCancelableBasicIoRequests::AddHead(Irp, &IrpCancelRoutine) ) {
            ReleaseSpinLock();
            Irp->CompleteRequestCompletionStatusAlreadySet();
        }
        else {
            ReleaseSpinLock();
        }
    }

    // Add an IRP to the head of the list
    // @param Irp - the IRP to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    //
    // Returns:
    //     - true if the IRP was added to the list
    //     - false if the IRP was not added because the Irp->Cancel was already set
    //       (IoStatus is set to STATUS_CANCELLED), the IRP was not added or completed
    //       (the caller must complete).
    // Note: utilizes BasicIoRequest::SavePointerWhileRequestOwner(), so the caller
    // cannot.
    bool AddHeadLockHeld(PBasicIoRequest Irp, PVOID param1 = NULL){

        Irp->SavePointerWhileRequestOwner(this, param1);
        return  ListOfCancelableBasicIoRequests::AddHead(Irp, &IrpCancelRoutine) ;
    }

    // Add an IRP to the tail of the list, supporting cancellation.  If the IRP is
    // cancelled, it will simply disappear from the list and be completed.
    // @param Irp - the IRP to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    void AddTail(PBasicIoRequest Irp, PVOID param1 = NULL) 
    {

        AcquireSpinLock();
        Irp->SavePointerWhileRequestOwner(this, param1);
        if (!ListOfCancelableBasicIoRequests::AddTail(Irp, &IrpCancelRoutine) ) {
            ReleaseSpinLock();
            Irp->CompleteRequestCompletionStatusAlreadySet();
        }
        else {
            ReleaseSpinLock();
        }
    }
    // Add a request to the tail of the list.
    // @param Irp - the request to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    // Returns:
    //     - true if the IRP was added to the list
    //     - false if the IRP was not added because the Irp->Cancel was already set
    //       (IoStatus is set to STATUS_CANCELLED), the IRP was not added or completed.
    // Note: utilizes BasicIoRequest::SavePointerWhileRequestOwner(), so the caller
    // cannot.
    bool AddTailLockHeld(PBasicIoRequest Irp, PVOID param1 = NULL){

        Irp->SavePointerWhileRequestOwner(this, param1);
        return  ListOfCancelableBasicIoRequests::AddTail(Irp, &IrpCancelRoutine) ;
    }

    // Add a request to the tail of the list, but don't call completion handler if cancelled.  Use this
    // if the caller holds its own lock, so the IRP should not be completed here.
    //
    // @param Irp - the request to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    // Returns:
    //     - true if the IRP was added to the list
    //     - false if the IRP was not added because the Irp->Cancel was already set
    //       (IoStatus is set to STATUS_CANCELLED), the IRP was not added or completed.
    // Note: utilizes BasicIoRequest::SavePointerWhileRequestOwner(), so the caller
    // cannot.
    bool AddTailNoCompletion(PBasicIoRequest Irp, PVOID param1 = NULL){
        AcquireSpinLock();
        Irp->SavePointerWhileRequestOwner(this, param1);
        bool result =  ListOfCancelableBasicIoRequests::AddTail(Irp, &IrpCancelRoutine) ;
        ReleaseSpinLock();
        return result;
    }

    // Add a request to the head of the list, but don't call completion handler if cancelled.  Use this
    // if the caller holds its own lock, so the IRP should not be completed here.
    //
    // @param Irp - the request to add
    // @param param1 - an optional addition parameter to save with the I/O request which can be return on remove.
    // Returns:
    //     - true if the IRP was added to the list
    //     - false if the IRP was not added because the Irp->Cancel was already set
    //       (IoStatus is set to STATUS_CANCELLED), the IRP was not added or completed.
    // Note: utilizes BasicIoRequest::SavePointerWhileRequestOwner(), so the caller
    // cannot.
    bool AddHeadNoCompletion(PBasicIoRequest Irp, PVOID param1 = NULL){
        AcquireSpinLock();
        Irp->SavePointerWhileRequestOwner(this, param1);
        bool result =  ListOfCancelableBasicIoRequests::AddHead(Irp, &IrpCancelRoutine) ;
        ReleaseSpinLock();
        return result;
    }

private:

    // Handles the callback when the request is cancelled.
    static void IrpCancelRoutine(IN PVOID Unused, IN PBasicIoRequest  Irp)
    {
        Irp->CancelRoutinePreamble();

        // Retrieve list pointer from Irp
        LockedListOfCancelableBasicIoRequest *This = (LockedListOfCancelableBasicIoRequest*) Irp->RetrievePointerWhileRequestOwner();
        This->AcquireSpinLock();

        // Sets status to STATUS_CANCELLED if cancelled.
        bool CSX_MAYBE_UNUSED found = This->ListOfCancelableBasicIoRequests::CancelIrp(Irp);
        This->ReleaseSpinLock();

        // The protocol is such that the IRP must be found.
#if defined(USE_CPP_FF_ASSERT)
        FF_ASSERT_NO_THIS(found);
#else // USE_CPP_FF_ASSERT
        FF_ASSERT(found);
#endif // USE_CPP_FF_ASSERT

        Irp->CompleteRequestCompletionStatusAlreadySet();
    }
};


#endif  // 

