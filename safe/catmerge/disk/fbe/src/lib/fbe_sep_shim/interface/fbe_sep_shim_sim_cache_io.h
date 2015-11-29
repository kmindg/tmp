#ifndef FBE_SEP_SHIM_CACHE_IO_H
#define FBE_SEP_SHIM_CACHE_IO_H

#include "simulation/cache_to_mcr_transport.h"
#include "simulation/cache_to_mcr_transport_server_interface.h"

void fbe_sep_shim_sim_mj_read_write(cache_to_mcr_operation_code_t opcode,
							   cache_to_mcr_serialized_io_t * serialized_io,
							   server_command_completion_function_t completion_function,
							   void * completion_context);


void fbe_sep_shim_sim_sgl_read_write(cache_to_mcr_serialized_io_t * serialized_io,
									 server_command_completion_function_t completion_function,
									 void * completion_context);


#endif /*FBE_SEP_SHIM_CACHE_IO_H*/
