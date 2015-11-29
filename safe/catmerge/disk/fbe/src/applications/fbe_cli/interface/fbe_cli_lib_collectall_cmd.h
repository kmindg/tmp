#ifndef FBE_CLI_LIB_COLLECTALL_CMD_H
#define FBE_CLI_LIB_COLLECTALL_CMD_H

#include "fbe/fbe_cli.h"
#include "fbe_cli_private.h"
#include "fbe_cli_cmi_service.h"
#include "fbe_cli_lib_provision_drive_cmds.h"
#include "fbe_cli_rginfo.h"
#include "fbe_cli_database.h"
#include "fbe_cli_bvd.h"
#include "fbe_cli_bus_cmd.h"
#include "fbe_cli_luninfo.h"
#include "fbe_cli_event_log_cmds.h"


// Function prototypes
void fbe_cli_lib_display_cmds(int default_argc, char** default_argv);
void fbe_cli_lib_collectall_cmd(int argc , char ** argv);

#define COLLECT_ALL_USAGE  "\
collectall  -shows output for differnt fbe_cli commands\n\
                usage:  collectall -h   for help\n\
                        collectall\n\
"

#endif /* FBE_CLI_LIB_COLLECTALL_CMD_H */
