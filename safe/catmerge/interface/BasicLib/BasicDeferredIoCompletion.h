#ifndef __BasicDeferredIoCompletion_h__
#define __BasicDeferredIoCompletion_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2011
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicDeferredIoCompletion.h
//
// Contents:
//      Defines the BasicDeferredIoCompletion class.
//      This class provides a paradigm for deferring I/O completions.
//
// Revision History:
//      Created. S. Morley
//
// Notes:
//      This is an easy way to break recursion between drivers that
//      immediately retry failed Irps on the completion thread.




class BasicDeferredIoCompletion : private BasicDeferredProcedureCall, private BasicLockedObject
{

public:

    // Queue an Irp completion to be handled on a separate thread
    void DeferredCompleteBasicIoRequest(BasicIoRequest *Irp, EMCPAL_STATUS Status, ULONG_PTR info = 0)
    {
        Irp->SetCompletionStatus(Status, info);
        DeferredCompleteBasicIoRequest(Irp);
    }

    // Queue an Irp completion to be handled on a separate thread by scheduling a DPC
    void DeferredCompleteBasicIoRequest(BasicIoRequest *Irp)
    {
        AcquireSpinLock();
        
        bool wasEmpty = mToBeCompletedList.IsEmpty();
        
        mToBeCompletedList.AddTail(Irp);
        if(wasEmpty)
        {
            DeferredProcedureCallSchedule();
        }
        
        ReleaseSpinLock();
    }

    // Called on a DPC to complete a queued Irp completion
    void DeferredProcedureCallHandler()
    {
        ListOfBasicIoRequest tmpq;
        BasicIoRequest *req;
        
        // Copy the queue to a temp
        AcquireSpinLock();
        for(;;)
        {
            if((req = mToBeCompletedList.RemoveHead()) == NULL)
            {
                break;
            }
            tmpq.AddTail(req);
        }
        ReleaseSpinLock();
        
        // process the temp queue and complete the req
        for(;;)
        {
            if((req = tmpq.RemoveHead()) == NULL)
            {
                break;
            }
            req->CompleteRequestCompletionStatusAlreadySet();
        }
    }
    
private:
    
    ListOfBasicIoRequest  mToBeCompletedList;

};

#endif

