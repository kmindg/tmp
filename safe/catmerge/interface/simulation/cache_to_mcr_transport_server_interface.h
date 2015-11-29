
#ifndef CACHE_TO_MCR_TRANSPORT_SERVER_INTERFACE_H
#define CACHE_TO_MCR_TRANSPORT_SERVER_INTERFACE_H

#include "fbe/fbe_types.h"
#include "simulation/cache_to_mcr_transport.h"

typedef void (*server_command_completion_function_t)(fbe_u8_t * returned_packet, void *context);

fbe_status_t cache_to_mcr_transport_server_init(cache_to_mcr_target_t sp_mode);
fbe_status_t cache_to_mcr_transport_server_destroy(cache_to_mcr_target_t sp_mode);

#endif /*CACHE_TO_MCR_TRANSPORT_SERVER_INTERFACE_H*/

