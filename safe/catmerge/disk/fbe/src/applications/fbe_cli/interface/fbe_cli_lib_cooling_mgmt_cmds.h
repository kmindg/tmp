 #ifndef FBE_CLI_COOLING_MGMT_CMD_H 
 #define FBE_CLI_COOLING_MGMT_CMD_H 
     
    
#include "fbe/fbe_cli.h" 
#include "fbe/fbe_types.h" 
#include "fbe/fbe_devices.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h" 
#include "fbe/fbe_shim_flare_interface.h"
#include "fbe_base_environment_debug.h"


/* Function prototypes*/ 
void fbe_cli_cmd_coolingmgmt(fbe_s32_t argc,char ** argv); 

#define COOLINGMGMT_USAGE  "\n\
coolingmgmt -help          - Display help information\n\
coolingmgmt                - Displays information of all fans (Default)\n\
coolingmgmt [-b <bus_num>][-e <encl_num>][-s <slot_num>] - Displays information about one or more fan(s) using optional arguments as selectors. \n\
"

#endif /*FBE_CLI_COOLING_MGMT_CMD_H*/   