#ifndef FBE_SEP_SHIM_SIMULATION_INTERFACE_H
#define FBE_SEP_SHIM_SIMULATION_INTERFACE_H

#include "fbe/fbe_types.h"
#include "simulation/cache_to_mcr_transport.h"
#include "simulation/cache_to_mcr_transport_server_interface.h"

typedef fbe_status_t (* fbe_sep_shim_sim_map_device_name_to_block_edge_func_t)(const fbe_u8_t *dev_name, void **block_edge, fbe_object_id_t *bvd_object_id);
typedef fbe_status_t (* fbe_sep_shim_sim_cache_io_entry_t)(cache_to_mcr_operation_code_t opcode,
														   cache_to_mcr_serialized_io_t * serialized_io,
														   server_command_completion_function_t completion_function,
														   void * completion_context);

extern fbe_sep_shim_sim_map_device_name_to_block_edge_func_t  lun_map_entry;
extern fbe_sep_shim_sim_cache_io_entry_t sep_shim_cache_io_entry;

fbe_status_t fbe_sep_shim_sim_map_device_name_to_block_edge(const fbe_u8_t *dev_name, void **block_edge, fbe_object_id_t *bvd_object_id);

fbe_status_t fbe_sep_shim_sim_cache_io_entry(cache_to_mcr_operation_code_t opcode,
											 cache_to_mcr_serialized_io_t * serialized_io,
											 server_command_completion_function_t completion_function,
											 void * completion_context);

#endif /*FBE_SEP_SHIM_SIMULATION_INTERFACE_H*/
