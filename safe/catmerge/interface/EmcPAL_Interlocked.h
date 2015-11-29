#ifndef _EMCPAL_INTERLOCKED_H_
#define _EMCPAL_INTERLOCKED_H_
/*!
 * @file EmcPAL_Interlocked.h
 * @addtogroup emcpal_interlocked
 * @{
 * @brief
 *      The Interlocked EmcPAL functions follow the Windows naming convention for interlocked operations (with a couple of exceptions) and also follow the Windows parameter ordering standard.
 *      Note that these functions use the CSX interlocked APIs under the covers.
 */

/***************************************************************************
* Copyright (C) EMC Corporation 2009
* All rights reserved.
* Licensed material -- property of EMC Corporation
**************************************************************************/


#include "EmcPAL.h"

/*! @brief EmcpalInterlockedIncrement - Atomically increment a value
 *  @param addend Pointer to value to increment
 *  @return Incremented value
 */
EMCPAL_LOCAL CSX_FINLINE
LONG
EmcpalInterlockedIncrement(volatile LONG *addend)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_inc_return((csx_atomic_t*)addend);
#else
    return InterlockedIncrement(addend);
#endif
}  // end EmcpalInterlockedIncrement

/*! @brief EmcpalInterlockedDecrement - Atomically decrement a value
 *  @param addend Pointer to value to decrement
 *  @return Decremented value
 */
EMCPAL_LOCAL CSX_FINLINE
LONG
EmcpalInterlockedDecrement(volatile LONG *addend)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_dec_return((csx_atomic_t*)addend);
#else
    return InterlockedDecrement(addend);
#endif
}  // end EmcpalInterlockedDecrement

/*! @brief EmcpalInterlockedExchange - Atomically set a value
 *  @param target Pointer to target
 *  @param value Value to be set to target
 *  @return Original/initial value of the target
 */
EMCPAL_LOCAL CSX_FINLINE
LONG
EmcpalInterlockedExchange(volatile LONG *target, LONG value)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_swap_nsint(target, value);
#else
    return InterlockedExchange(target, value);
#endif
}  // end EmcpalInterlockedExchange

/*! @brief EmcpalInterlockedCompareExchange32 - Atomic compare and exchange operation on two 32-bit values.
 *         If target value equals to the compare value the exchange value is set to the target.
 *  @param xptr Pointer to target
 *  @param vnew Exchange value
 *  @param vold Compare value
 *  @return original/initial value of the target
 */
EMCPAL_LOCAL CSX_FINLINE
INT_32
EmcpalInterlockedCompareExchange32(CSX_VOLATILE INT_32* xptr, INT_32 vnew, INT_32 vold)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_cas_32((csx_s32_t*) xptr, vold, vnew);
#else
    return InterlockedCompareExchange((LONG*)xptr, vnew, vold);
#endif
}  // end EmcpalInterlockedCompareExchange32

/*! @brief EmcpalInterlockedCompareExchange64 - Atomic compare and exchange operation on two 64-bit values.
 *         If target value equals to the compare value the exchange value is set to the target.
 *  @param xptr Pointer to target
 *  @param vnew Exchange value
 *  @param vold Compare value
 *  @return original/initial value of the target
 */
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedCompareExchange64(CSX_VOLATILE INT_64* xptr, INT_64 vnew, INT_64 vold)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_cas_64((csx_s64_t*) xptr, vold, vnew);
#else
    return InterlockedCompareExchange64(xptr, vnew, vold);
#endif
}  // end EmcpalInterlockedCompareExchange64

/*! @brief EmcpalInterlockedComparePointer - Atomic compare and exchange operation on two pointers.
 *         If target pointer equals to the compare value the exchange value is set to the target pointer.
 *  @param xptr Traget pointer
 *  @param vnew Exchange value
 *  @param vold Compare pointer
 *  @return original/initial value of the target
 */
EMCPAL_LOCAL CSX_FINLINE
void*
EmcpalInterlockedCompareExchangePointer(CSX_VOLATILE csx_pvoid_t* xptr, csx_pvoid_t vnew, csx_pvoid_t vold)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_cas_ptr(xptr, vold, vnew);
#else
    return InterlockedCompareExchangePointer(xptr, vnew, vold);
#endif
}  // end EmcpalInterlockedCompareExchangePointer

/*! @brief EmcpalInterlockedExchange64 - Atomically swap 64-bit values
 *  @param target Pointer to 64-bit target
 *  @param value value
 *  @return Original target value
 */
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedExchange64(volatile INT_64* target, INT_64 value)
{
    return csx_p_atomic_swap_64((csx_s64_t*) target, value);
} // end EmcpalInterlockedExchange64

/*! @brief EmcpalInterlockedExchangePointer - Atomically swaps ptr1 and ptr2, returns original value of ptr1
 *  @param ptr1 Pointer 1
 *  @param ptr2 Pointer 2
 *  @return original value of pointer-1
 */
EMCPAL_LOCAL CSX_FINLINE
void*
EmcpalInterlockedExchangePointer(CSX_VOLATILE csx_pvoid_t* ptr1, csx_pvoid_t ptr2)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_swap_ptr(ptr1, ptr2);
#else
    return InterlockedExchangePointer(ptr1, ptr2);
#endif
}  // end EmcpalInterlockedExchangePointer

/*! @brief EmcpalInterlockedIncrement64 - Atomically increment 64-value
 *  @param addend Target value to be incremented
 *  @return Incremented target value
 */
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedIncrement64(volatile INT_64* addend)
{
    return csx_p_atomic64_inc_return((csx_atomic64_t*) addend);
} // end EmcpalInterlockedIncrement64

/*! @brief EmcpalInterlockedIncrement64 - Atomically decrement 64-value
 *  @param addend Target value to be decremented
 *  @return Decremented target value
 */
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedDecrement64(volatile INT_64* addend)
{
    return csx_p_atomic64_dec_return((csx_atomic64_t*)addend);
} // end EmcpalInterlockedDecrement64

/*! @brief EmcpalInterlockedExchangeAdd32 - Atomically add value to 32-bit target
 *  @param addend Pointer to target
 *  @param value value to be added
 *  @return Original value of the target (before addition)
 */
EMCPAL_LOCAL CSX_FINLINE
INT_32
EmcpalInterlockedExchangeAdd32(volatile INT_32* addend, INT_32 value)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_add_return((csx_atomic_t*) addend,value);
#else
    return InterlockedExchangeAdd((LONG*)addend, value);
#endif
} // end EmcpalInterlockedExchangeAdd32

/*! @brief EmcpalInterlockedExchangeAdd32 - Atomically add value to 64-bit target
 *  @param addend Pointer to target
 *  @param value value to be added
 *  @return Original value of the target (before addition)
 */
// Returns the ORIGINAL value of addend, not the result of the addition 
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedExchangeAdd64(volatile INT_64* addend, INT_64 value)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic64_add_return((csx_atomic64_t*) addend,value);
#else
    return InterlockedExchangeAdd64(addend, value);
#endif
} // end EmcpalInterlockedExchangeAdd64

/*! @brief EmcpalInterlockedOr64 - Atomically OR value to 64-bit target
 *  @param destination Pointer to target
 *  @param value value to be OR'd
 *  @return Original value of the target (before OR operation)
 */
EMCPAL_LOCAL CSX_FINLINE
INT_64
EmcpalInterlockedOr64(volatile INT_64* Destination, INT_64 value)
{
#ifdef EMCPAL_USE_CSX_INTERLOCKED
    return csx_p_atomic_or_64(Destination,value);
#else
    return InterlockedOr64(Destination, value);
#endif
} // end EmcpalInterlockedExchangeAdd64

/*!
 * @} end group emcpal_interlocked
 * @} end file EmcPAL_Interlocked.h
 */
#if defined(EMCPAL_USE_CSX_LISTS) && defined(EMCPAL_USE_CSX_SPL)

EMCPAL_LOCAL CSX_FINLINE
csx_dlist_entry_t *
EmcpalInterlockedInsertHeadList(csx_dlist_entry_t * head, csx_dlist_entry_t * entry, PEMCPAL_SPINLOCK lock)
{
    csx_dlist_entry_t * old_head = NULL;
    csx_p_spl_lock(lock);
    if (!csx_dlist_is_empty(head)) {
        old_head = entry->csx_dlist_field_next;
    }
    csx_dlist_add_entry_head(head, entry);
    csx_p_spl_unlock(lock);
    return old_head;
} // end EmcpalInterlockedInsertHeadList

EMCPAL_LOCAL CSX_FINLINE
csx_dlist_entry_t *
EmcpalInterlockedInsertTailList(csx_dlist_entry_t * head, csx_dlist_entry_t * entry, PEMCPAL_SPINLOCK lock)
{
    csx_dlist_entry_t * old_head = NULL;
    csx_p_spl_lock(lock);
    if (!csx_dlist_is_empty(head)) {
        old_head = entry->csx_dlist_field_prev;
    }
    csx_dlist_add_entry_tail(head, entry);
    csx_p_spl_unlock(lock);
    return old_head;
} // end EmcpalInterlockedInsertTailList

EMCPAL_LOCAL CSX_FINLINE
csx_dlist_entry_t *
EmcpalInterlockedRemoveHeadList(csx_dlist_entry_t * head, PEMCPAL_SPINLOCK lock)
{
    csx_dlist_entry_t * old_head = NULL;
    csx_p_spl_lock(lock);
    if (!csx_dlist_is_empty(head)) {
        old_head = csx_dlist_remove_entry_at_head(head);
    }
    csx_p_spl_unlock(lock);
    return old_head;
} // end EmcpalInterlockedRemoveHeadList

#else

EMCPAL_LOCAL CSX_FINLINE
PLIST_ENTRY
EmcpalInterlockedInsertHeadList(PLIST_ENTRY head, PLIST_ENTRY entry, PEMCPAL_SPINLOCK lock)
{
    return ExInterlockedInsertHeadList(head, entry, &lock->lockObject);
} // end EmcpalInterlockedInsertHeadList

EMCPAL_LOCAL CSX_FINLINE
PLIST_ENTRY
EmcpalInterlockedInsertTailList(PLIST_ENTRY head, PLIST_ENTRY entry, PEMCPAL_SPINLOCK lock)
{
    return ExInterlockedInsertTailList(head, entry, &lock->lockObject);
} // end EmcpalInterlockedInsertTailList

EMCPAL_LOCAL CSX_FINLINE
PLIST_ENTRY
EmcpalInterlockedRemoveHeadList(PLIST_ENTRY head, PEMCPAL_SPINLOCK lock)
{
    return ExInterlockedRemoveHeadList(head, &lock->lockObject);
} // end EmcpalInterlockedRemoveHeadList

#endif

#endif /* _EMCPAL_INLINE_H_ */
