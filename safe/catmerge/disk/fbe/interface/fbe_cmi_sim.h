#ifndef FBE_CMI_SIM_H
#define FBE_CMI_SIM_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_terminator_api.h"

fbe_status_t fbe_cmi_sim_set_get_sp_id_func(fbe_terminator_api_get_sp_id_function_t function);
fbe_status_t fbe_cmi_sim_set_get_cmi_port_base_func(fbe_terminator_api_get_cmi_port_base_function_t function);

#endif /*FBE_CMI_SIM_H*/
