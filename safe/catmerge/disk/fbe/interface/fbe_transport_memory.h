#ifndef FBE_TRANSPORT_MEMORY_H
#define FBE_TRANSPORT_MEMORY_H

#include "fbe/fbe_transport.h"

fbe_packet_t * fbe_transport_allocate_packet(void);
void fbe_transport_release_packet(fbe_packet_t * packet);

fbe_status_t fbe_transport_allocate_packet_async(	fbe_packet_t * packet,
													fbe_memory_completion_function_t completion_function,
													fbe_memory_completion_context_t completion_context);

void * fbe_transport_allocate_buffer(void);
void fbe_transport_release_buffer(void * buffer);

#endif /* FBE_TRANSPORT_MEMORY_H */

