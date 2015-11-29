#ifndef FBE_ATOMIC_H
#define FBE_ATOMIC_H

#include "fbe_types.h"
#include "fbe_winddk.h"
#include "EmcPAL_Interlocked.h"

__forceinline static LONG bgs_atomic_exchange(LONG * target, LONG value)
{
	return  EmcpalInterlockedExchange(target, value );

}
#ifdef _AMD64_ /* 64 bit compilation */
typedef LONGLONG volatile fbe_atomic_t;
typedef LONG volatile fbe_atomic_32_t;
typedef CHAR volatile fbe_atomic_8_t;

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_t fbe_atomic_exchange(fbe_atomic_t * target, fbe_atomic_t value)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedExchange64(target, value );
#else
    return EmcpalInterlockedExchange64((INT_64 *)target, value);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_t fbe_atomic_increment(fbe_atomic_t * target)
{
#if FBE_ENABLE_WIN_API
    return  EmcpalInterlockedIncrement64( target);
#else
    return EmcpalInterlockedIncrement64(target);
#endif
}

/* returns the decremented value. */
__forceinline static fbe_atomic_t fbe_atomic_decrement(fbe_atomic_t * target)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedDecrement64( target);
#else
    return EmcpalInterlockedDecrement64(target);
#endif
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_t fbe_atomic_compare_exchange(fbe_atomic_t * target, fbe_atomic_t value, fbe_atomic_t comparand)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedCompareExchange64 ( target, value, comparand);
#else
    return EmcpalInterlockedCompareExchange64((INT_64*)target, value, comparand);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_t fbe_atomic_add(fbe_atomic_t * target, fbe_atomic_t val)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedExchangeAdd64 (target, val);
#else
    return EmcpalInterlockedExchangeAdd64(target, val);
#endif
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_exchange(fbe_atomic_32_t * target, fbe_atomic_32_t value)
{
#if FBE_ENABLE_WIN_API
    return  EmcpalInterlockedExchange(target, value );
#else
    return EmcpalInterlockedExchange(target, value);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_increment(fbe_atomic_32_t * target)
{
#if FBE_ENABLE_WIN_API
    return  EmcpalInterlockedIncrement( target);
#else
    return EmcpalInterlockedIncrement(target);
#endif
}

/* returns the decremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_decrement(fbe_atomic_32_t * target)
{
#if FBE_ENABLE_WIN_API
    return  EmcpalInterlockedDecrement( target);
#else
    return EmcpalInterlockedDecrement(target);
#endif
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_compare_exchange(fbe_atomic_32_t * target, fbe_atomic_32_t value, fbe_atomic_32_t comparand)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedCompareExchange ( target, value, comparand);
#else
    return EmcpalInterlockedCompareExchange32((INT_32*)target, value, comparand);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_add(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedExchangeAdd (target, val);
#else
    return EmcpalInterlockedExchangeAdd32((INT_32*)target, (INT_32)val);
#endif
}
/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_32_t fbe_atomic_32_and(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
	return  InterlockedAnd(target, val);
#else
        return csx_p_atomic_and_32((csx_s32_t *)target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_32_t fbe_atomic_32_or(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
	return  InterlockedOr (target, val);
#else
        return csx_p_atomic_or_32((csx_s32_t *)target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_t fbe_atomic_or(fbe_atomic_t * target, fbe_atomic_t val)
{
#if FBE_ENABLE_WIN_API
	return  InterlockedOr64 (target, val);
#else
        return csx_p_atomic_or_64(target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_t fbe_atomic_and(fbe_atomic_t * target, fbe_atomic_t val)
{
#if FBE_ENABLE_WIN_API
	return  InterlockedAnd64(target, val);
#else
        return csx_p_atomic_and_64(target, val);
#endif
}

#else /* 32 bit compilation */
typedef LONGLONG volatile fbe_atomic_t;
typedef LONG volatile fbe_atomic_32_t;
typedef CHAR volatile fbe_atomic_8_t;

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_t fbe_atomic_exchange(fbe_atomic_t * target, fbe_atomic_t value)
{
    return  EmcpalInterlockedExchange64(target, value );
/*    return  EmcpalInterlockedExchange((LONG*)target, ( LONG)value ); */
}

/* returns the incremented value. */
__forceinline static fbe_atomic_t fbe_atomic_increment(fbe_atomic_t * target)
{
	return  EmcpalInterlockedIncrement64( target);
/*	return  EmcpalInterlockedIncrement( (fbe_atomic_32_t*)target); */
}

/* returns the decremented value. */
__forceinline static fbe_atomic_t fbe_atomic_decrement(fbe_atomic_t * target)
{
	return  EmcpalInterlockedDecrement64( target);
/*	return  EmcpalInterlockedDecrement( (fbe_atomic_32_t*)target); */
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_t fbe_atomic_compare_exchange(fbe_atomic_t * target, fbe_atomic_t value, fbe_atomic_t comparand)
{
    /*return  InterlockedCompareExchange64( target, value, comparand);*/
#if FBE_ENABLE_WIN_API
    return InterlockedCompareExchange( (LONG*)target, (LONG)value, (LONG)comparand);
#else
    return EmcpalInterlockedCompareExchange64(target, value, comparand);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_t fbe_atomic_add(fbe_atomic_t * target, fbe_atomic_t val)
{
	/*return  InterlockedExchangeAdd64(target, val);*/
#if FBE_ENABLE_WIN_API
    return InterlockedExchangeAdd((LONG*)target, (LONG)val);
#else
    return EmcpalInterlockedExchangeAdd64(target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_t fbe_atomic_or(fbe_atomic_t * target, fbe_atomic_t val)
{
#if FBE_ENABLE_WIN_API
	return  InterlockedOr64(target, (LONG)val);
#else
        return csx_p_atomic_or_64(target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_t fbe_atomic_and(fbe_atomic_t * target, fbe_atomic_t val)
{
#if FBE_ENABLE_WIN_API
	return InterlockedAnd64(target, (LONG)val);
#else
        return csx_p_atomic_and_64(target, val);
#endif
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_exchange(fbe_atomic_32_t * target, fbe_atomic_32_t value)
{
	return  EmcpalInterlockedExchange(target, value );
}

/* returns the incremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_increment(fbe_atomic_32_t * target)
{
	return  EmcpalInterlockedIncrement( target);
}

/* returns the decremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_decrement(fbe_atomic_32_t * target)
{
    return EmcpalInterlockedDecrement( target);
}

/* returns the value of the variable at target when the call occurred. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_compare_exchange(fbe_atomic_32_t * target, fbe_atomic_32_t value, fbe_atomic_32_t comparand)
{
#if FBE_ENABLE_WIN_API
    return InterlockedCompareExchange ( target, value, comparand);
#else
    return EmcpalInterlockedCompareExchange32((INT_32*)target, value, comparand);
#endif
}

/* returns the incremented value. */
__forceinline static fbe_atomic_32_t fbe_atomic_32_add(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
    return InterlockedExchangeAdd (target, val);
#else
    return EmcpalInterlockedExchangeAdd32((volatile INT_32 *)target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_32_t fbe_atomic_32_and(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedAnd(target, val);
#else
    return csx_p_atomic_and_32((csx_s32_t *)target, val);
#endif
}

/* returns the original value of the Destination parameter */
__forceinline static fbe_atomic_32_t fbe_atomic_32_or(fbe_atomic_32_t * target, fbe_atomic_32_t val)
{
#if FBE_ENABLE_WIN_API
    return  InterlockedOr (target, val);
#else
    return csx_p_atomic_or_32((csx_s32_t *)target, val);
#endif
}

#endif
 
#endif /* FBE_ATOMIC_H */
