#ifndef FBE_MULTICORE_SPINLOCK_H
#define FBE_MULTICORE_SPINLOCK_H

#include "fbe_types.h"
#include "fbe_queue.h"
#include "fbe_winddk.h"

typedef struct fbe_multicore_spinlock_entry_s {
	fbe_spinlock_t   lock;	/* One lock per core */
}fbe_multicore_spinlock_entry_t;

typedef struct fbe_multicore_spinlock_s {
	fbe_multicore_spinlock_entry_t * entry; /* One entry per core */
}fbe_multicore_spinlock_t;

/* This function will initialize and allocate all resources */
fbe_status_t fbe_multicore_spinlock_init(fbe_multicore_spinlock_t * multicore_spinlock);

/* This function will destroy and release all resources */
fbe_status_t fbe_multicore_spinlock_destroy(fbe_multicore_spinlock_t * multicore_spinlock);

fbe_status_t fbe_multicore_spinlock_lock(fbe_multicore_spinlock_t * multicore_spinlock, fbe_cpu_id_t cpu_id);
fbe_status_t fbe_multicore_spinlock_unlock(fbe_multicore_spinlock_t * multicore_spinlock, fbe_cpu_id_t cpu_id);


#endif /*FBE_MULTICORE_SPINLOCK_H */