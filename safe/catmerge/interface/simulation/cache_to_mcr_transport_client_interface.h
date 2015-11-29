
#ifndef CACHE_TO_MCR_TRANSPORT_CLINET_INTERFACE_H
#define CACHE_TO_MCR_TRANSPORT_CLINET_INTERFACE_H

#include "fbe/fbe_types.h"
#include "simulation/cache_to_mcr_transport.h"

typedef void (*cache_to_mcr_completion_function_t)(cache_to_mcr_serialized_io_t *returned_buffer, void *context);

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  
 */
extern "C" {
#endif

fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_init_client(cache_to_mcr_target_t target_sp);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_destroy_client(cache_to_mcr_target_t target_sp);

fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_send_buffer(fbe_u8_t * memory,
																	fbe_u32_t length,
																	cache_to_mcr_completion_function_t completion_function,
																	void *  completion_context);

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  
 */
};
#endif

#endif /*CACHE_TO_MCR_TRANSPORT_CLINET_INTERFACE_H*/
