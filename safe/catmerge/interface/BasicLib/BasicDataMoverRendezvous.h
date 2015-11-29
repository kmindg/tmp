#ifndef __BasicDataMoverRendezvous_h__
#define __BasicDataMoverRendezvous_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicDataMoverRendezvous.h
//
// Contents:
//      Defines the BasicDataMoverRendezvous class. 
//
// Revision History:   
//--

#include "flare_ioctls.h"
#include "EmcPAL_List.h"
#include "BasicIoRequest.h"
#include "K10SpinLock.h"
#include "BasicLockedObject.h"
#include "ktrace.h"
#include "ktrace_structs.h"

#define DMR_HASH_SIZE 128
#define GET_KEY(_Irp) *((ULONG *)(((PCHAR)_Irp->GetDmIoctlContext()) + sizeof(EMCPAL_LIST_ENTRY)))

// Provides a mechanism to match the pair of BasicIoRequests which share the
// same Data Mover key.
class BasicDataMoverRendezvous {

public:

    // constructor
    BasicDataMoverRendezvous(DM_LAYER_ID layerID) : mLayerID(layerID) { }

    // Finds waiting irp of pair, or causes caller to wait.
    //   @param irp - one-half of a DM SOURCE/DESTINATION pair.
    //
    // Returns:
    //     NULL - No match, the irp is queued.
    //     otherwise - the matching IRP, and irp is not queued.
    //
    // Note: Linkage of request pair that is compatible with BasicLib
    // completion calls and templated queued lists is available by
    // using Irp->CreatePairing(siblingIrp) and Irp->BreakPairing()
    // which save pointers between the two requests using
    // Irp->Tail.Overlay.DriverContext[2].
    // This is not done by default to prevent problems with legacy
    // drivers that may have other uses for this field.
    // See definition of BasicIoRequest for more information.
    PBasicIoRequest Rendezvous(PBasicIoRequest Irp)  {
        ULONG key = GET_KEY(Irp);
        ULONG n = hash(key);
        return mHashTable[n].Rendezvous(Irp, key, mLayerID);
    }
  
private:

    class IrpHash : public BasicLockedObject {
    public:

        // Finds waiting irp of pair, or causes caller to wait.
        //   @param irp - one-half of a DM SOURCE/DESTINATION pair.
        //
        // Returns:
        //     NULL - No match, the irp is queued or completed/canceled.
        //     otherwise - the matching IRP, and irp is not queued.
        PBasicIoRequest Rendezvous (PBasicIoRequest Irp, ULONG key, DM_LAYER_ID layerID) 
        {
            PBasicIoRequest siblingDmIrp, canceledIrp = NULL;
            ListOfBasicIoRequest cancelList;
            
            // Save a pointer to this hash entry in the IRP.
            Irp->SavePointerWhileRequestOwner(this, NULL, NULL);
            
            // Perform callback.
            ((PDMHSCB)Irp->GetCurrentPDCATable())((PEMCPAL_IRP)Irp, Irp->GetCurrentReadWriteLength(), layerID);
            
            // Check to see if the IRP was canceled.
            if(Irp->IsCancelPending())
            {
                Irp->CompleteRequest(EMCPAL_STATUS_CANCELLED);
                return NULL;
            }
            
            // Grab the lock.
            AcquireSpinLock();
            
            // Search hash set for a matching DM IRP.
            if((siblingDmIrp = MatchDmIrpLockHeld(key, &cancelList)) == NULL)
            {
#if DBG
#ifndef C4_INTEGRATED
                KvTracex(TRC_K_STD, "BDMR%d: No match for IRP 0x%p, key %d\n", layerID, Irp, key);
#endif  /* C4_INTEGRATED too verbose logging */
#endif
                // No match was found. Try to insert the IRP in the hash set.
                if(mWaitingIrps.AddTail(Irp, WaitingIrpCancelRoutine) == false)
                {
                    // Unable to insert IRP in hash set since it has already
                    // been canceled. Queue it to the cancel list for now.
                    cancelList.AddTail(Irp);
                }
                
                // Drop the lock.
                ReleaseSpinLock();
            }
            else
            {
                // Drop the lock.
                ReleaseSpinLock();
#if DBG
#ifndef C4_INTEGRATED
                KvTracex(TRC_K_STD, "BDMR%d: Match found for IRP 0x%p, key %d, sibling 0x%p\n", layerID, Irp, key, siblingDmIrp);
#endif  /* C4_INTEGRATED too verbose logging */
#endif
                // Verify we have a source and destination.
                if(Irp->GetCurrentRequestSubtype() == FLARE_DM_SOURCE)
                {
                    if(siblingDmIrp->GetCurrentRequestSubtype() == FLARE_DM_SOURCE)
                    {
                        KvTracex(TRC_K_STD, "BDMR%d: Invalid match for SRC IRP 0x%p - 0x%p\n", layerID, Irp, siblingDmIrp);
                        FF_ASSERT(false);
                    }
                }
                else
                {
                    if(siblingDmIrp->GetCurrentRequestSubtype() == FLARE_DM_DESTINATION)
                    {
                        KvTracex(TRC_K_STD, "BDMR%d: Invalid match for DST IRP 0x%p - 0x%p\n", layerID, Irp, siblingDmIrp);
                        FF_ASSERT(false);
                    }
                }
            }
            
            // Clean up any IRPs that ended up on the cancel list.
            while((canceledIrp = cancelList.RemoveHead()) != NULL)
            {
                canceledIrp->CompleteRequestCompletionStatusAlreadySet();
            }
            
            return siblingDmIrp;
        }


    private:

        PBasicIoRequest MatchDmIrpLockHeld(ULONG key, ListOfBasicIoRequest * canceledList)
        {
            PBasicIoRequest Irp, matchedIrp = NULL;
            ListOfBasicIoRequest templist;

            // Grab each IRP off the list and compare key with the tag_id stored in the Irp.
            // Note that RemoveHead() will never give us an IRP that has already been
            // canceled.
            while((Irp = mWaitingIrps.RemoveHead()) != NULL)
            {
                if(GET_KEY(Irp) == key && !Irp->IsCancelPending())
                {
                    // Found a matching IRP.
                    matchedIrp = Irp;
                    break;
                }
                else
                {
                    // Put Irp on templist.
                    templist.AddTail(Irp);
                }
            }

            // Move the IRPs we put on the temp list back to mWaitingIrps.
            while((Irp = templist.RemoveHead()) != NULL)
            {
                if(mWaitingIrps.AddTail(Irp, WaitingIrpCancelRoutine) == false)
                {
                    // Tried to put the IRP back on the list but it
                    // was canceled while we were holding onto it.
                    // Put it in the cancel list so we can complete it
                    // after we drop the lock.
                    canceledList->AddTail(Irp);
                }
            }

            return matchedIrp;
        }

        // Standard form cancel routine.
        static void WaitingIrpCancelRoutine(IN PVOID Unused, IN PBasicIoRequest Irp)
        {
            Irp->CancelRoutinePreamble();

            // Retrieve CachePartition pointer from Irp
            IrpHash *This = (IrpHash*) Irp->RetrievePointerWhileRequestOwner();
            This->AcquireSpinLock();
            bool found = This->mWaitingIrps.CancelIrp(Irp);
            This->ReleaseSpinLock();

            // If we found it then no other request was holding onto it.
            // Go ahead and complete the IRP.
            if(found)
            {
#if DBG
#ifndef C4_INTEGRATED
                KvTracex(TRC_K_STD, "BDMR: Cancelling IRP 0x%p for DM 0x%p\n", Irp, Irp->GetDmIoctlContext());
#endif  /* C4_INTEGRATED too verbose logging */
#endif
                Irp->CompleteRequestCompletionStatusAlreadySet();
            }
        }

        ListOfCancelableBasicIoRequests   mWaitingIrps;
    };

    IrpHash             mHashTable[DMR_HASH_SIZE];
    DM_LAYER_ID         mLayerID;
    inline ULONG hash(ULONG key) { return (key % DMR_HASH_SIZE); }
};

#endif  // __BasicDataMoverRendezvous_h__



