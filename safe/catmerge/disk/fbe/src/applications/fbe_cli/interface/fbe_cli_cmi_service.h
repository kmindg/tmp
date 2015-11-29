#ifndef FBE_CLI_CMI_H
#define FBE_CLI_CMI_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmi_commands(int argc , char ** argv);

#define FBE_CLI_CMI_USAGE  "\
cmi - Show CMI information\n\
usage:  cmi -help          - For this help information.\n\
        cmi -info          - Show CMI infiormation\n\
        cmi -get_io_stat   - Show CMI IO statistics\n\
        cmi -clear_io_stat - Clear CMI IO statistics\n\
"

   
#endif /* FBE_CLI_CMI_H */

