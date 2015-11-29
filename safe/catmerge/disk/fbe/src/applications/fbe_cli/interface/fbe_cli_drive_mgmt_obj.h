#ifndef FBE_CLI_DRIVE_MGMT_OBJ_H
#define FBE_CLI_DRIVE_MGMT_OBJ_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_drive_mgmt(int argc , char ** argv);      /* user access cmds */
void fbe_cli_cmd_drive_mgmt_srvc(int argc , char ** argv); /* service access cmds */

char * fbe_cli_convert_drive_type_enum_to_string(fbe_drive_type_t drive_type); 

#define DRIVE_MGMT_USAGE  "\
dmo - shows drive management object information\n\
usage:  dmo -help                    - For this help information.\n\
        dmo -b bus -e encl -s slot   - Target a single drive.\n\
               -drive_info           - Display drive information.\n\
               -drivegetlog          - Issue drivegetlog command to a drive. Only supports drive with paddle card\n\
               -dieh_get_stats       - Display DIEH health info \n\
               -dieh_clear_stats     - Clear DIEH stats.  Returns drive to healthy state \n\
        dmo -dieh_display            - Display DIEH configuration. \n\
        dmo -dieh_load [<xml_file>]  - Load a new DIEH configuration.  If optional filename is \n\
                                       not provided then default xml file will be loaded.\n\
        dmo -get_crc_actions         - Get the logical crc error actions. Optional b-e-s can be provided.\n\
        dmo -set_crc_actions [value] - Set the logical crc error actions. [value] is optional. If \n\
                                       not provided then current registry setting will be loaded.\n\
                                       Optional b-e-s can be provided.\n\
        dmo -getfuelgauge            - Collect SMART data(log page 0x30) and Fuel Gauge data(log page 0x31) from SSD drive. \n\
        dmo -getfuelgaugeinterval/-getfgi            - Get the mini. poll interval from the registry. \n\
        dmo -setfuelgaugeinterval/-setfgi <interval> - Set the mini. poll interval in the global variable. \n\
        dmo -fuelgauge <0|1>         - Enable <1> or disable <0> the collect SMART data and Fuel Gauge data. \n\
        dmo -set_policy_state system_qdepth <on|off> - Enable <on> or disable <off> vault drive qdepth change. \n\
        dmo -drive_debug_ctrl <remove|remove_defer|insert|insert_defer> - Debug command on drives, support defer remove/insert. \n\
\n"

#define DRIVE_MGMT_SRVC_USAGE  "\
dmo_srvc - shows drive management object information\n\
usage:  dmo_srvc -help                  - For this help information.\n\
        dmo_srvc -dieh_add_record       - Adds DIEH config record. Interactive Mode. \n\
        dmo_srvc -dieh_remove_record    - Removes a DIEH config record.\n\
        dmo_srvc -mode_select_all       - sends mode select to all drives. \n\
\n"
#endif /* FBE_CLI_DRIVE_MGMT_OBJ_H */
