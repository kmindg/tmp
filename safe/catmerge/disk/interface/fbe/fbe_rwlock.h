#ifndef FBE_RWLOCK_H
#define FBE_RWLOCK_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_atomic.h"

typedef struct fbe_rwlock_s{
	fbe_atomic_t	read_count;
	fbe_semaphore_t access_lock;
}fbe_rwlock_t;


__forceinline static void fbe_rwlock_init(fbe_rwlock_t * rwlock)
{
	fbe_atomic_exchange(&rwlock->read_count, 0); /* Initialize counter */
	fbe_semaphore_init(&rwlock->access_lock, 1, 1); /* The semaphore initialy signalled */
	rwlock->read_count = 0;
}

__forceinline static void fbe_rwlock_write_lock(fbe_rwlock_t * rwlock)
{
    fbe_semaphore_wait(&rwlock->access_lock, NULL);
}

__forceinline static void fbe_rwlock_write_unlock(fbe_rwlock_t * rwlock)
{
    fbe_semaphore_release(&rwlock->access_lock, 0, 1, FALSE);
}

__forceinline static void fbe_rwlock_read_lock(fbe_rwlock_t * rwlock)
{
	fbe_atomic_t count;
	count = fbe_atomic_increment(&rwlock->read_count);
	if(count == 1){ /* The read_count was 0 i.e.  first read access */
		fbe_semaphore_wait(&rwlock->access_lock, NULL);
	}
}

__forceinline static void fbe_rwlock_read_unlock(fbe_rwlock_t * rwlock)
{
	fbe_atomic_t count;
	count = fbe_atomic_decrement(&rwlock->read_count);
	if(count == 0){ /* The read_count is 0 now i.e. last read access */
		fbe_semaphore_release(&rwlock->access_lock, 0, 1, FALSE);
	}
}

__forceinline static void fbe_rwlock_destroy(fbe_rwlock_t * rwlock)
{
    fbe_semaphore_destroy(&rwlock->access_lock);
}

#endif /* FBE_RWLOCK_H */