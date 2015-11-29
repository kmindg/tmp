#ifndef FBE_API_PACKET_QUEUE_H
#define FBE_API_PACKET_QUEUE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"


typedef struct fbe_api_packet_q_element_s{
	fbe_queue_element_t q_element;/*MUST be first*/
	fbe_packet_t packet;
	fbe_bool_t dynamically_allocated;
}fbe_api_packet_q_element_t;

fbe_status_t fbe_api_common_init_contiguous_packet_queue(void);
fbe_status_t fbe_api_common_destroy_contiguous_packet_queue(void);

#endif /*FBE_API_PACKET_QUEUE_H*/
