#include "fbe/fbe_winddk.h"
#include "base_object_private.h"
#include "fbe_base_config_private.h"

fbe_status_t 
fbe_base_config_send_functional_packet(fbe_base_config_t * base_config, fbe_packet_t * packet_p, fbe_edge_index_t edge_index)
{
    fbe_status_t status;
    fbe_block_edge_t * edge_p = NULL;

    fbe_base_config_get_block_edge(base_config, &edge_p, edge_index);
    status = fbe_block_transport_send_functional_packet(edge_p, packet_p);
    return status;
}