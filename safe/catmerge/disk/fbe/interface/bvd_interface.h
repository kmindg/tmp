
#ifndef BVD_INTERFACE_H
#define BVD_INTERFACE_H


#include "fbe/fbe_types.h"
#include "fbe_block_transport.h"

fbe_status_t fbe_bvd_interface_map_os_device_name_to_block_edge(const fbe_u8_t *name, void **block_edge_ptr, fbe_object_id_t *bvd_object_id);

#endif /*BVD_INTERFACE_H*/
