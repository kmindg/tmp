/* $CCRev: ./array_sw/catmerge/FabricArray/interface/BasicTrace.h@@/main/daytona_mr1/1 */
/************************************************************************
 *
 *  Copyright (c) 2006-2013 EMC Corporation.
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
 *
 ************************************************************************/

#ifndef __ListOfCancelableIrps__
#define __ListOfCancelableIrps__

# include "EmcPAL_Irp.h"
# include "ListsOfIRPs.h"

// A ListOfCancelableIrps is a ListOfIrps that allows a cancel handler to be specified
// which will allow the IRP to be canceled while it is on this list.
//
// The user is required to single thread all calls to an instance of this class, typically
// by marking all calls with a lock held.
//
// The cancel handler will only be called if the IRP is on this list, and it must do the
// following:
//
// - Release the cancel spin lock
// - Translate the IRP/DeviceObject to the particular instance of this class that the  IRP
//   is queued to
// - Acquire the lock that protects the paricular instance of the queue
// - Call ListOfCancelableIrps::CancelHandler on with the IRP.    
// - Release the lock
// - Complete the IRP with STATUS_CANCELLED
//
// ListOfCancelableIrps::CancelHandler MUST be called on the correct queue for the IRP, or
// the  IRP will be lost. ListOfCancelableIrps::CancelHandler can also be called on the
// wrong queue, so one cancel handler is try multiple queues, or there can be a separate
// cancel handler  for multiple types of queues.
class ListOfCancelableIrps {
public:
	// Add an IRP to the head of the list
	// @param Irp - the IRP to add
	// @param cancelRoutine - the routine which will be used in the IoSetCancelRoutine()
	//                        call, see requirements above. Must not be NULL.
	//
	// Returns true if the IRP was added to the list, false if the IRP was not added because
	// the  Irp->Cancel was already set (IoStatus is set to STATUS_CANCELLED).
	inline bool AddHead(PEMCPAL_IRP Irp, PEMCPAL_IRP_CANCEL_FUNCTION cancelRoutine);

	// Add an IRP to the tail of the list
	// @param Irp - the IRP to add
	// @param cancelRoutine - the routine which will be used in the IoSetCancelRoutine()
	//                        call, see requirements above. Must not be NULL.
	//
	// Returns true if the IRP was added to the list, false if the IRP was not added because
	// the  Irp->Cancel was already set (IoStatus is set to STATUS_CANCELLED).
	inline bool AddTail(PEMCPAL_IRP Irp, PEMCPAL_IRP_CANCEL_FUNCTION cancelRoutine);

	// Remove the first IRP from this list that does not have a cancel routine in progress
	// for it. Returns the IRP or NULL if there is no IRP on the list, or the only IRP on the
	// list has an immediate cancel pending
	inline PEMCPAL_IRP RemoveHead();

    // Remove a specific IRP from this list, if it does not have a cancel routine in progress
    // for it. 
    // Returns: true only of the Irp was removed from the list.
    inline bool Remove(PEMCPAL_IRP Irp);

    // Returns true if RemoveHead() would return NULL, false otherwise.
    inline bool IsEmptyLockHeld() const;
    
    // Returns true if there is nothing on the list.
    // Cannot distinguish cancelled requests that are pending removal from other requests.
    inline bool IsEmptyNoLockHeld() const;

    // The cancelRoutine specified must call this if it is called, after it has released the
    // cancel spin lock and acquired any locks it uses to protect this list.
    //
    // @param Irp - the Irp being cancelled.
    //
    // Returns true if the Irp was found and removed from this list.  Returns false if the
    // IRP was not on this list and the caller must therefore try some other list.
    // If true is returned, Irp->IoStatus.Status is set to STATUS_CANCELLED, and Information to 0.
    inline bool CancelIrp(PEMCPAL_IRP Irp);


private:
	// The actual list.
	ListOfIRPs  mIrpList;
};

inline bool ListOfCancelableIrps::AddTail(PEMCPAL_IRP Irp, PEMCPAL_IRP_CANCEL_FUNCTION cancelRoutine)
{
	DBG_ASSERT(cancelRoutine != NULL);

	/* Warning 8127: The function being used as a CancelRoutine routine does not exactly 
	* match the type expected. Have 'void (__cdecl *)(struct _DEVICE_OBJECT *,struct _IRP *)', 
	* expecting 'void (__stdcall *)(struct _DEVICE_OBJECT *,struct _IRP *)':  It is likely 
	* that the difference is that the actual function returns a value, and the expected function type is void.
	*/
	/* Suppress the prefast this warning.
	*/
	#pragma warning(disable: 4068)
	#pragma prefast(disable: 8127)
	// Set the cancel routine that will remove this IRP from the queue if aborted.    
	EmcpalIrpCancelRoutineSet(Irp, cancelRoutine);
	#pragma prefast(default: 8127)
	#pragma warning(default: 4068)

	// If was canceled before we set the cancel routine, we want to return FALSE.
	if (EmcpalIrpIsCancelInProgress(Irp)) {

		// Try to unset the cancel routine
		if (EmcpalIrpCancelRoutineSet(Irp, NULL) != NULL) {

            EmcpalIrpSetBadStatus(Irp, EMCPAL_STATUS_CANCELLED);
			// The cancel flag was set previously, and we have undone the cancel callback.
			return false;
		}
		else {
			// The cancel occurred in the small window between the setting of the cancel routine
			// and the testing of the Cancel flag.  The cancel routine will be waiting on the lock
			// that we hold
		}
	}	

	ListOfIRPsEnqueue(&mIrpList, Irp);
	return true;
}

inline bool ListOfCancelableIrps::AddHead(PEMCPAL_IRP Irp, PEMCPAL_IRP_CANCEL_FUNCTION cancelRoutine)
{
	DBG_ASSERT(cancelRoutine != NULL);

	/* Warning 8127: The function being used as a CancelRoutine routine does not exactly 
	* match the type expected. Have 'void (__cdecl *)(struct _DEVICE_OBJECT *,struct _IRP *)', 
	* expecting 'void (__stdcall *)(struct _DEVICE_OBJECT *,struct _IRP *)':  It is likely 
	* that the difference is that the actual function returns a value, and the expected function type is void.
	*/
	/* Suppress the prefast this warning.
	*/
	#pragma warning(disable: 4068)
	#pragma prefast(disable: 8127)
	// Set the cancel routine that will remove this IRP from the queue if aborted.    
	EmcpalIrpCancelRoutineSet(Irp, cancelRoutine);
	#pragma prefast(default: 8127)
	#pragma warning(default: 4068)

	// If was canceled before we set the cancel routine, we want to return FALSE.
	if (EmcpalIrpIsCancelInProgress(Irp)) {

		// Try to unset the cancel routine
		if (EmcpalIrpCancelRoutineSet(Irp, NULL) != NULL) {

            EmcpalIrpSetBadStatus(Irp, EMCPAL_STATUS_CANCELLED);

			// The cancel flag was set previously, and we have undone the cancel callback.
			return false;
		}
		else {
			// The cancel occurred in the small window between the setting of the cancel routine
			// and the testing of the Cancel flag.  The cancel routine will be waiting on the lock
			// that we hold
		}
	}	

	ListOfIRPsRequeue(&mIrpList, Irp);
	return true;
}



inline bool ListOfCancelableIrps::CancelIrp(IN PEMCPAL_IRP  Irp)
{
	bool found = ListOfIRPsRemove(&mIrpList, Irp) != FALSE;

	if (found) {
        EmcpalIrpSetBadStatus(Irp, EMCPAL_STATUS_CANCELLED);
	}

	return found;
}

inline PEMCPAL_IRP ListOfCancelableIrps::RemoveHead()
{
	PEMCPAL_IRP			First;

	First  = ListOfIRPsDequeue(&mIrpList);

	if (!First) {
		return NULL;
	}

	PEMCPAL_IRP Irp = First;

	// Clear the Cancel routine, normally returns non-NULL when we have cleared the cancel
	// routine.
	while(EmcpalIrpCancelRoutineSet(Irp, NULL) == NULL) {

		// The cancel routine has been called, and is blocked by the lock we hold. Put the IRP
		// at the tail of the queue, and find the next one.
		ListOfIRPsEnqueue(&mIrpList, Irp);

		// Now get the next one in order.
		Irp = ListOfIRPsDequeue(&mIrpList);

		// If this is the first one we dequeued, all have a cancel pending.
		if (Irp == First) {
			ListOfIRPsEnqueue(&mIrpList, Irp);
			return NULL;
		}
	}

	// There is no cancel pending, so we can return this IRP.
	return Irp;
}

inline bool ListOfCancelableIrps::Remove(PEMCPAL_IRP Irp)
{
    if (! ListOfIRPsRemove(&mIrpList, Irp)) { return false; }

	// Clear the Cancel routine, normally returns non-NULL when we have cleared the cancel
	// routine.
	if (EmcpalIrpCancelRoutineSet(Irp, NULL) == NULL) {

		// The cancel routine has been called, and is blocked by the lock we hold. Put the IRP
		// back on the queue, and claim it is not there.
		ListOfIRPsEnqueue(&mIrpList, Irp);

        return false;
    }

	return true;
}

inline bool ListOfCancelableIrps::IsEmptyLockHeld() const
{
	PEMCPAL_IRP_LIST_ENTRY Entry;

	for_each_SLListElement(mIrpList.list, Entry, EMCPAL_IRP_LIST_ENTRY_LINK) {
		PEMCPAL_IRP Irp = CSX_CONTAINING_STRUCTURE(Entry, EMCPAL_IRP, EMCPAL_IRP_LIST_ENTRY_FIELD);

		// We don't add IRPs to the list if their cancel flag is set, so if it  were set now,
		// the cancel routine would be in the process of being called.
		if (!EmcpalIrpIsCancelInProgress(Irp)) return FALSE;
	}

	// No IRP on the list then are not pending cancel.
	return TRUE;
}

inline bool ListOfCancelableIrps::IsEmptyNoLockHeld() const 
{
    return SLListIsEmpty(mIrpList.list);
}

# endif

