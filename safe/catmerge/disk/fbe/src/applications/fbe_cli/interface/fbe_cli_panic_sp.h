#ifndef FBE_CLI_PANIC_SP_H
#define FBE_CLI_PANIC_SP_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_panic_sp(int argc , char ** argv);

#define FBE_CLI_PANIC_USAGE  "\
panic - panic the sp\n\
usage:  panic         - to panic local sp.\n\
"

   
#endif /* FBE_CLI_PANIC_SP_H */

