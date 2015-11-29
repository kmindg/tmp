#ifndef FBE_MULTICORE_QUEUE_H
#define FBE_MULTICORE_QUEUE_H

#include "fbe_types.h"
#include "fbe_queue.h"
#include "fbe_winddk.h"

typedef struct fbe_multicore_queue_entry_s {
	fbe_queue_head_t head; /* One head per core */
	fbe_spinlock_t   lock;	/* One lock per core */
}fbe_multicore_queue_entry_t;

typedef struct fbe_multicore_queue_s {
	fbe_multicore_queue_entry_t * entry; /* One entry per core */
}fbe_multicore_queue_t;

/* This function will initialize and allocate all resources */
fbe_status_t fbe_multicore_queue_init(fbe_multicore_queue_t * multicore_queue);

/* This function will destroy and release all resources */
fbe_status_t fbe_multicore_queue_destroy(fbe_multicore_queue_t * multicore_queue);


fbe_status_t fbe_multicore_queue_push(fbe_multicore_queue_t * multicore_queue, 
									  fbe_queue_element_t * element, 
									  fbe_cpu_id_t cpu_id);


fbe_status_t fbe_multicore_queue_push_front(fbe_multicore_queue_t * multicore_queue, 
											fbe_queue_element_t * element,
											fbe_cpu_id_t cpu_id);

fbe_queue_element_t * fbe_multicore_queue_pop(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);

fbe_bool_t fbe_multicore_queue_is_empty(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);

fbe_queue_element_t * fbe_multicore_queue_front(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);


/* return next element or NULL for error */
fbe_queue_element_t * fbe_multicore_queue_next(fbe_multicore_queue_t * multicore_queue, 
											   fbe_queue_element_t * element, 
											   fbe_cpu_id_t cpu_id);

/* return next element or NULL for error */
fbe_queue_element_t * fbe_multicore_queue_prev(fbe_multicore_queue_t * multicore_queue, 
											   fbe_queue_element_t * element, 
											   fbe_cpu_id_t cpu_id);


fbe_u32_t fbe_multicore_queue_length(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);

fbe_status_t fbe_multicore_queue_lock(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);
fbe_status_t fbe_multicore_queue_unlock(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);

fbe_queue_head_t * fbe_multicore_queue_head(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id);

#endif /*FBE_MULTICORE_QUEUE_H */