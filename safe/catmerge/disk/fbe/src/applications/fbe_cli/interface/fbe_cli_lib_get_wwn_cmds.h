#ifndef FBE_CLI_LIB_GET_WWN_CMDS_H
#define FBE_CLI_LIB_GET_WWN_CMDS_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_module_info.h"

/* Function prototypes*/
void fbe_cli_cmd_get_wwn(fbe_s32_t argc,char ** argv);
static void fbe_cli_get_wwn_seed(void);

/* Usage*/
#define GET_WWN_USAGE                               \
" getwwn/gw   - get current World Wide Name Seed\n" \
" Usage:  \n"\
"       getwwn -h\n"\
"       getwwn\n\n"

#endif /*FBE_CLI_LIB_GET_WWN_CMDS_H*/

