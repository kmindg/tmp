#ifndef FBE_STRIPE_LOCK_H
#define FBE_STRIPE_LOCK_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"

fbe_status_t fbe_stripe_lock_operation_entry(struct fbe_packet_s *  packet);

fbe_status_t fbe_metadata_stripe_hash_init(fbe_u8_t * ptr, fbe_u32_t memory_size, fbe_u64_t default_divisor);
fbe_status_t fbe_metadata_stripe_hash_destroy(void * ptr);

#endif /* FBE_STRIPE_LOCK_H */

