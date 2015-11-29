#ifndef FBE_SIM_H
#define FBE_SIM_H

#include "fbe/fbe_types.h"

fbe_status_t fbe_sp_sim_config_init(fbe_u32_t sp_mode, fbe_u16_t sp_port_base, fbe_u16_t cmi_port_base, fbe_u16_t disk_sever_port_base);
void fbe_sp_sim_config_destroy(void);


#endif /* FBE_SIM_H */

