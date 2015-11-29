
#ifndef _EMCPAL_INLINE_H_
#define _EMCPAL_INLINE_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2013
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file EmcPAL_Inline.h
 * 
 *  Contents:
 *       The header file for the inlined portion of the platform abstraction 
 * 		(EmcPAL) layer.
 * 
 *  @version
 *      02-09 AMD - Created.
 * 
 */

#include "EmcPAL.h"

#ifndef EMCPAL_USE_CSX_SPL
#include "EmcKeInline.h"
#endif

//**********************************************************************
/*! @addtogroup emcpal_spinlocktrylock
 *  @{
 */

/*!
 *  @brief
 *       Tries to acquire spinlock.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       BOOL - TRUE : if lock is acquired
 *                   FALSE: if lock is not acquired.
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
BOOL EmcpalSpinlockTryLock (PEMCPAL_SPINLOCK   pSpinlock)    /*!< Ptr to the spinlock to lock */
{
#ifdef EMCPAL_USE_CSX_SPL 
    return (CSX_STATUS_SUCCESS == csx_p_spl_trylock(pSpinlock));
#else
     return EmcKeTryAcquireSpinLock (&(pSpinlock->lockObject), &(pSpinlock->irql));
#endif

}  // end EmcpalSpinTryLock


//**********************************************************************
/*! @addtogroup emcpal_spinlock
 *  @{
 */

/*!
 *  @brief
 *       Waits on, and acquires a spinlock.  Note that the irql is also
 * 		raised to dispatch level.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockLock (PEMCPAL_SPINLOCK   pSpinlock)	/*!< Ptr to the spinlock to lock */
{
#ifdef EMCPAL_USE_CSX_SPL 
    csx_p_spl_lock_nid (pSpinlock);

#else
    EmcKeAcquireSpinLock (&(pSpinlock->lockObject),
						  &(pSpinlock->irql));
#endif

}  // end EmcpalSpinlockLock

//**********************************************************************
/*!
 *  @brief
 *       Waits on, and acquires a spinlock when the caller is
 * 		already at dispatch level.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockLockSpecial (PEMCPAL_SPINLOCK   pSpinlock)	/*!< Ptr to the spinlock to lock */
{
#ifdef EMCPAL_USE_CSX_SPL 
    csx_p_spl_lock_nid (pSpinlock);

#else
    EmcKeAcquireSpinLockAtDpcLevel (&(pSpinlock->lockObject));
    pSpinlock->irql = EMCPAL_LEVEL_DISPATCH;

#endif

}  // end EmcpalSpinlockLockSpecial


//**********************************************************************
/*!
 *  @brief
 *       Releases a spinlock.  Note that the irql lowered to its
 * 		prior level (i.e., the level it had before acquiring the 
 * 		spinlock.)
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockUnlock (PEMCPAL_SPINLOCK  pSpinlock)	/*!< Ptr to the spinlock to unlock */
{
#ifdef EMCPAL_USE_CSX_SPL 
    csx_p_spl_unlock_nid (pSpinlock);

#else 
    EmcKeReleaseSpinLock (&(pSpinlock->lockObject),
						  pSpinlock->irql);

#endif

}  // end EmcpalSpinlockUnlock

//**********************************************************************
/*!
 *  @brief
 *       Releases a spinlock but remains at dispatch level.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockUnlockSpecial (PEMCPAL_SPINLOCK  pSpinlock)	/*!< Ptr to the spinlock to unlock */
{
#ifdef EMCPAL_USE_CSX_SPL 
    csx_p_spl_unlock_nid (pSpinlock);  

#else
    EmcKeReleaseSpinLockFromDpcLevel (&(pSpinlock->lockObject));

#endif

}  // end EmcpalSpinlockUnlockSpecial

//**********************************************************************
/*!
 *  @brief
 *       Waits on, and acquires a queued spinlock.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return 
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockQueuedLock (PEMCPAL_SPINLOCK_QUEUED		pSpinlock,	/*!< Ptr to the spinlock to lock */
						  PEMCPAL_SPINLOCK_QUEUE_NODE	pNode)		/*!< Missing detail */
{
#ifdef EMCPAL_USE_CSX_SPL
	CSX_UNREFERENCED_PARAMETER (pNode);

    csx_p_spl_lock_nid (pSpinlock);

#else
    KeAcquireInStackQueuedSpinLock (pSpinlock,
									pNode);
#endif

}  // end EmcpalSpinlockQueuedLock

//**********************************************************************
/*!
 *  @brief
 *       Release a queued spinlock.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockQueuedUnlock (PEMCPAL_SPINLOCK_QUEUED		pSpinlock,	/*!< Ptr to the spinlock to lock */
							PEMCPAL_SPINLOCK_QUEUE_NODE	pNode)		/*!< Missing detail */
{
#ifdef EMCPAL_USE_CSX_SPL
	CSX_UNREFERENCED_PARAMETER (pNode);

    csx_p_spl_unlock (pSpinlock);

#else
	CSX_UNREFERENCED_PARAMETER (pSpinlock);

    KeReleaseInStackQueuedSpinLock (pNode);

#endif

}  // end EmcpalSpinlockQueuedUnlock

//++
//.End file EmcPAL_inline.h
//--


//**********************************************************************
/*!
 *  @brief
 *       Releases a spinlock in a different order than it was taken
 *       (in relation to another lock).  
 *
 *		 NOTE: the locks will swap IRQLs to
 *       guarantee we don't drop the IRQL when we aren't supposed to.
 *
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *       VOID
 * 
 */
EMCPAL_LOCAL CSX_FINLINE
VOID
EmcpalSpinlockUnlockOutOfOrder (PEMCPAL_SPINLOCK  pSpinlock,		/*!< Ptr to the spinlock to unlock */
                                PEMCPAL_SPINLOCK  pSpinlockOther)	/*!< Ptr to the other spinlock (IRQL will be used in 
																		release) */
{
#ifdef EMCPAL_USE_CSX_SPL 
    csx_p_spl_unlock_out_of_order_nid (pSpinlock, pSpinlockOther);

#else
    KIRQL tmp;
    tmp = pSpinlock->irql;
    pSpinlock->irql = pSpinlockOther->irql;
    pSpinlockOther->irql = tmp;
    EmcKeReleaseSpinLock (&(pSpinlock->lockObject),
                          pSpinlock->irql);

#endif

}  // end EmcpalSpinlockUnlockOutOfOrder
/*! @} */
#endif /* _EMCPAL_INLINE_H_ */
