#ifndef FBE_CLI_LIFECYCLE_STATE_H
#define FBE_CLI_LIFECYCLE_STATE_H
 
#include "fbe/fbe_cli.h"
#include "fbe/fbe_package.h"

fbe_status_t fbe_cli_set_lifecycle_state(fbe_u32_t object_id, fbe_u32_t state, fbe_package_id_t package_id);
char * fbe_cli_print_LifeCycleState(fbe_lifecycle_state_t state);

#endif /* FBE_CLI_LIFECYCLE_STATE_H */
