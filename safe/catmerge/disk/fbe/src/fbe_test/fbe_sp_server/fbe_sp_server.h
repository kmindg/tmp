#ifndef FBE_SP_SEVER_H
#define FBE_SP_SEVER_H

#include "fbe/fbe_types.h"

fbe_status_t fbe_sp_server_config_init(fbe_u32_t server_mode, fbe_u32_t sp_mode);
void fbe_sp_server_config_destroy(fbe_u32_t server_mode);


#endif /* FBE_SP_SEVER_H */

