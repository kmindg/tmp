#ifndef FBE_CLI_LIB_SIGNATURE_CMD_H
#define FBE_CLI_LIB_SIGNATURE_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"


/* Function prototypes*/

void fbe_cli_cmd_signature(fbe_s32_t argc, char **argv);


/* Usage*/
#define FBE_CLI_SIGNATURE_USAGE                               \
" operate_fru_signature/opfrusig   - get/set disk's signature\n \
 Usage:  \n\
      @This CLI can only be used on active side !@\n\
      General Switches:\n\
              -get       Read signature from target disk.\n\
              -set       Write signature to the target disk.\n\
              -clear     Clear the signature of target disk.\n\
              -b         Bus number.\n\
              -e         Enclosure number.\n\
              -s         Slot number.\n\
              -h/-help   Get help\n\
       E.g.\n\
       opfrusig -get -b 0 -e 0 -s 1    -Read the disk 0_0_1 signature and print it\n\
       opfrusig -set -b 0 -e 1 -s 2    -Write signature to the disk 0_1_2\n\
       opfrusig -clear -b 1 -e 1 -s 2  -Clear signature of disk 1_1_2\n\
"

#endif /*FBE_CLI_LIB_SIGNATURE_CMD_H*/

