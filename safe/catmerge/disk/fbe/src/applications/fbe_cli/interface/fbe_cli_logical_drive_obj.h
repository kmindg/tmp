#ifndef FBE_CLI_LOGICAL_DRIVE_OBJ_H
#define FBE_CLI_LOGICAL_DRIVE_OBJ_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_logical_drive(int argc , char ** argv);

#define LOGICAL_DRIVE_USAGE  "\
ldo - shows logical drive information\n\
usage:  ldo -help                   - For this help information.\n\
        ldo -p port -e encl -s slot - Target a single logical drive with the command\n\
            -all                    - Issue command to all LUNs.\n\
            -trace_level level      - Change the trace level to a new value.\n\
            -display_terse          - Display table output of logical drive.\n\
            -display_verbose        - Display detailed info of logical drive.\n\
            -set_state state        - Set the lifecycle state of the logical drive.\n"

#define PROACTIVE_SPARE_DRIVE_USAGE  "\
ps - start proactive copy operation for disk\n\
usage:  ps -help                   - For this help information.\n\
        ps -p port -e encl -s slot - Target a single disk with the command\n\
           -p - bus number.\n\
           -e - enclosure number.\n\
           -s - slot number.\n\
 "           


void fbe_cli_cmd_set_ica_stamp(int argc , char ** argv);

#endif /* FBE_CLI_LOGICAL_DRIVE_OBJ_H */