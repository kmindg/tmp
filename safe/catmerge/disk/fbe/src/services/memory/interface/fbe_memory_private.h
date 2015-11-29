#ifndef FBE_MEMORY_PRIVATE_H
#define FBE_MEMORY_PRIVATE_H

#include "fbe/fbe_memory.h"

extern fbe_memory_allocation_function_t memory_allocation_function;
extern fbe_memory_release_function_t memory_release_function;

fbe_status_t fbe_memory_dps_init(fbe_bool_t b_retry_forever);
fbe_status_t fbe_memory_dps_destroy(void);

fbe_status_t fbe_memory_dps_add_memory(fbe_packet_t * packet);

fbe_status_t fbe_memory_fill_dps_statistics(fbe_memory_dps_statistics_t *dps_statisitcs_p);


fbe_status_t fbe_memory_persistence_init(void);
fbe_status_t fbe_memory_persistence_destroy(void);
fbe_status_t fbe_memory_persistence_unpin(fbe_packet_t * packet);
fbe_status_t fbe_memory_persistence_read_and_pin(fbe_packet_t * packet);
fbe_status_t fbe_memory_persistence_check_vault_busy(fbe_packet_t * packet_p);

fbe_status_t fbe_memory_persistence_setup(void);
fbe_status_t fbe_memory_persistence_cleanup(void);
fbe_status_t fbe_persistent_memory_control_entry(fbe_packet_t * packet);
void fbe_persistent_memory_get_params(fbe_persistent_memory_set_params_t **set_params_p);

fbe_status_t fbe_memory_dps_reduce_size(fbe_packet_t * packet);

#endif /* FBE_MEMORY_PRIVATE_H */
