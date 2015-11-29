#ifndef FBE_CLI_LIB_ENCLOSURE_MGMT_CMD_H
#define FBE_CLI_LIB_ENCLOSURE_MGMT_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe_cli_lifecycle_state.h"
#include "fbe_encl_mgmt_utils.h"

/* Function prototypes*/
void fbe_cli_cmd_enclmgmt(fbe_s32_t argc,char ** argv);

void fbe_cli_cmd_wwn_seed(int argc , char ** argv);

#define WWN_SEED_USAGE \
"wwnseed   \t- Set, get array midplane wwn seed info and user modified wwn seed flag.\n\
USAGE: \n\
wwnseed \t\t- Display array midplane wwn seed, user modified wwn seed flag.\n\
wwnseed -cflag \t\t- Clear user modified wwn seed flag.\n\
wwnseed -umodify # \t- User modified array wwn seed. \n\
                   \t  The flag userModifiedWwnSeedFlag will be set. Enter the hexdecimal. \n\
wwnseed -set # \t- set array wwn seed only. \n\
               \t  The flag userModifiedWwnSeedFlag won't be set. Enter the hexdecimal.\n"

void fbe_cli_cmd_system_id(fbe_s32_t argc,char ** argv);

#define SYSTEMID_USAGE \
"systemid/sysid \t- Get and set system id info.\n\
Usage:  \n\
systemid \t\t Display the usage. \n\
systemid -help \t\t Display the usage. \n\
system_id -r \t\t- Read the system id info.\n\
systemid -umodify -sn <serialNo> -pn <partNo> \t Modify the serial number and/or the part number.\n"


#define ENCLMGMT_USAGE \
"enclmgmt	\t- Displays Enclosure information\n\n"  \
"Usage:  \n"  \
"enclmgmt \t\t- Displays information of all Enclosures (Default)\n"  \
"enclmgmt -b <bus_no>    - Displays information of all enclosures of specified bus\n"  \
"enclmgmt -b <bus_no> -e <encl_no> - Displays information about enclosure of specified bus and enclosure number\n"  \
"enclgmgmt -help  \t- Display help information\n"  

#endif /*FBE_CLI_LIB_ENCLOSURE_MGMT_CMD_H*/
