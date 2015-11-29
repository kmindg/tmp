#ifndef FBE_CLI_LIB_PERSIST_CMD_H
#define FBE_CLI_LIB_PERSIST_CMD_H

#include "fbe/fbe_cli.h"
#include "fbe_cli_private.h"
#include "fbe_cli_cmi_service.h"

/* Function prototypes*/
void fbe_cli_lib_persist_cmd(int argc , char ** argv);
void fbe_cli_persist_hook(int argc , char ** argv);


#define PERSIST_INFO_USAGE  "\
persist  -shows information about persistence service\n\
\n\
    usage:  persist -h   : for help\n\
            persist -info: to show persistence related info\n\
"

#define PERSIST_HOOK_USAGE  "\
persist_debug_hook or ps_hook <operation> <hook_type> <target package>\n\
\n\
    usage:  persist_debug_hook -add_hook hook_type <sep|esp>:\n\
                    add debug hook in persist service for SEP or ESP\n\
            e.g.\n\
                ps_hook -add_hook panic_writing_journal sep\n\
                        panic SP when writing journal region\n\
                ps_hook -add_hook panic_writing_live_region sep\n\
                        panic SP when writing live data region\n\
            \n\
            persist_debug_hook -remove_hook hook_type <sep|esp>\n\
                    remove debug hook in persist service for SEP or ESP\n\
            e.g.\n\
                ps_hook -remove_hook panic_writing_journal esp\n\
                        remove panic SP when writing journal region on ESP running\n\
                ps_hook -remove_hook panic_writing_live_region sep\n\
                        remove panic SP when writing live data region when sep running\n\
"


#endif /* FBE_CLI_LIB_PERSIST_CMD_H */
