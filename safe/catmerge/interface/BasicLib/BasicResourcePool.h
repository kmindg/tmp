/************************************************************************
*
*  Copyright (c) 2010 EMC Corporation.
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

#ifndef __BasicResourcePool_h__
#define __BasicResourcePool_h__
//++
// File Name:
//      BasicBinarySemaphore.h
//
// Contents:
//  Defines the BasicResourceWaiter  and BasicPreallocatedResourcePool class.
//
//
// Revision History:
//--


# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"

// Defines a simple pool of resources with a FIFO meachansm which allows a single
// requestor to allocate one resource, and to wait for that resource if none are currently
// available.   Resources must be populated by the caller.
template < class ResourceType, class ResourceListType, class WaiterType=BasicWaiter, class WaiterListType=ListOfBasicWaiter>
class BasicResourcePool : public BasicLockedObject
{
public:
    virtual ~BasicResourcePool() {
        FF_ASSERT(mResources.IsEmpty());
        FF_ASSERT(mWaiters.IsEmpty());
    }

    // Allocate a resource. The caller is required to associate the waiter and the
    // resource.
    // @param waiter - The item to queue if the resource is not available.  May be NULL.
    // Returns: 
    //    NULL - no resource was available, waiter is queued (if not NULL)
    //    other - the resource allocated.  waiter is unchanged by this call.
    ResourceType * Allocate(WaiterType * waiter) 
    {
        AcquireSpinLock();
        ResourceType * resource = mResources.RemoveHead();
        if (!resource && waiter) {
            mWaiters.AddTail(waiter);
        }
        ReleaseSpinLock();

        return resource;
    }

    // Release a resource. 
    // @param resource - The resource being released.  Only added to queue if NULL is
    //                   returned, otherwise unchanged.
    // Returns: 
    //    waiter - the oldest waiter.  The caller is required to associate the waiter and
    //             the resource.
    //    NULL - if no waiters.
    WaiterType  * Release(ResourceType * resource) 
    {
        AcquireSpinLock();

        WaiterType * waiter = mWaiters.RemoveHead();

        if(waiter == NULL && resource) mResources.AddHead(resource);

        ReleaseSpinLock();

        return waiter;
    }

private:
    ResourceListType mResources;

    WaiterListType   mWaiters;


};

// Defines a simple pool of resources with a FIFO meachansm which allows a single
// requestor to allocate one resource, and to wait for that resource if none are currently
// available.   Resources are populated automatically embedding memory for them in the
// object.
// @param DedicatedResources - The number of dedicated resources for this pool.  These are
//                             preallocated within the object so that there is no memory
//                             allocation.
template < class ResourceType, class ResourceListType, int DedicatedResources, class WaiterType=BasicWaiter, class WaiterListType=ListOfBasicWaiter >
class BasicPreallocatedResourcePool : public BasicResourcePool<ResourceType, ResourceListType, WaiterType, WaiterListType>
{
public:

    BasicPreallocatedResourcePool() {
        for (ULONG i=0; i < DedicatedResources; i++) {
            Release(&mPreallocated[i]);
        }
    }

    virtual ~BasicPreallocatedResourcePool() {
        for (ULONG i=0; i < DedicatedResources; i++) {
            ResourceType * t = this->Allocate(NULL);
            FF_ASSERT(t != NULL);
        }
    }

    ResourceType     mPreallocated[DedicatedResources];
};



#endif  // __BasicDeviceObject__

