#ifndef BGSL_ATOMIC_H
#define BGSL_ATOMIC_H

#include "bgsl_types.h"
#include "bgsl_winddk.h"

#include "EmcPAL_Interlocked.h"

typedef LONG bgsl_atomic_t;

/* returns the value of the variable at target when the call occurred. */
__forceinline static bgsl_atomic_t bgsl_atomic_exchange(bgsl_atomic_t * target, bgsl_atomic_t value)
{
	return  EmcpalInterlockedExchange(target, value );
}

/* returns the incremented value. */
__forceinline static bgsl_atomic_t bgsl_atomic_increment(bgsl_atomic_t * target)
{
	return  EmcpalInterlockedIncrement( target);
}

/* returns the decremented value. */
__forceinline static bgsl_atomic_t bgsl_atomic_decrement(bgsl_atomic_t * target)
{
	return  EmcpalInterlockedDecrement( target);
}
#endif /* BGSL_ATOMIC_H */
