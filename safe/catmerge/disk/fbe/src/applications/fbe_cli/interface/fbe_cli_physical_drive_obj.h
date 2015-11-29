#ifndef FBE_CLI_PHYSICAL_DRIVE_OBJ_H
#define FBE_CLI_PHYSICAL_DRIVE_OBJ_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_physical_drive(int argc , char ** argv);

#define FBE_CLI_PDO_ARG_MAX 32

#define PHYSICAL_DRIVE_USAGE  "\
pdo - shows physical drive information\n\
usage:  pdo -help                   - For this help information.\n\
        pdo [-p port] [-e encl] [-s slot] - Target drive sub-set filtered by any combination of port,\n\
                                            enclosure and slot with the command. List sorted by port-encl-slot.\n\
            -order order <p/o>      - Send commands in Physical(p) or Object(o) order \n\
            -o object_id            - Target a single physical drive with the command.\n\
            -display                - Display detailed info of a physical drive.\n\
            -abr                    - Display physical drive information in abbreviated mode. \n\
            -trace_level level      - Change the trace level to a new value.\n\
            -set_state state        - Set the lifecycle state of the physical drive.\n\
            -p port -e encl -s slot -set_writeunc lba(HEX) - Send a Write Uncorrectable to a single SATA drive. \n\
            -power_cycle duration   - Power cycle the drive. [duration] in 500ms increments.\n\
            -force_recovery         - Force PDO to recover from FAIL state.\n\
            -get_DC_log             - Get the error logs and write to a disk file (might take a while to complete). \n\
            -get_io_count <B_E_S>   - Get outstanding I/O counts. \n\
            -dc_rcv_diag_ena <0/1>  - Enable(1)/Disable(0) PDO to log the receive diagnostic page to disk once a day. \n\
            -force_health_check     - Force PDO to enable a quick self test (QST) on the drive. \n\
            -reset_drive            - Sends reset drive command. \n\
            -reset_slot             - Sends reset slot command. \n\
            -set_qdepth depth       - Sends command to set qdepth, default qdepth is 10 (0xa). \n\
            -get_qdepth             - Retrieves qdepth. \n\
            -receive_diag <command> - Sends command to retrieve diag. \n\
            -get_drivelog           - Command Not yet implemented. \n\
            -link_fault <0/1>       - Set(1)/Disable(0) link fault flag for this PDO. \n\
            -set_drive_fault        - Sets drive fault flag for this PDO. \n\
            -set_io_throttle <MB>	- Sets I/O throttle value in MB \n\
            -mode_select            - Issues mode select to drive using override table\n\
            -p port -e encl -s slot -create_read_unc lba(HEX)\n\
                                     Use write long to create an uncorrectable read.\n\
            -p port -e encl -s slot -create_read_unc_range start_lba(HEX) end_lba(HEX) inc_lba(HEX)\n\
                                     Create several uncorrectable reads.\n\
            -get_mode_pages [-pc <current|changeable|default|saved>] [-raw] \n\
                                     - Displays mode pages.  \n\
                                        -pc : page control value.  defaults to 'saved'\n\
                                        -raw : displays in raw format.\n\
            -get_rla_abort_required - Determine whether or not Read Look Ahead (RLA) abort is \n\
                                      required based upon this drive's firmware revision. \n\
                                      RLAs only occur on some Seagate drives when they complete \n\
                                      a VERIFY and don't have any outstanding I/Os, but they \n\
                                      can be aborted by issuing a Test Unit Ready (TUR) to the \n\
                                      drive. \n\
"

#define SET_PDO_SRVC_USAGE "\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -link_fault <clear or set>  : Sets and clears link_fault flag for this PDO \n\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -drive_fault <set>          : Sets drive_fault flag for this PDO \n\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -list_sanitize_patterns     : List sanitize pattern codes for following cmd\n\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -sanitize <pattern_code>    : Issue special format cmd to securely erase drive\n\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -sanitize_status            : Get sanitize status\n\
pdo_srvc {-bes <b_e_s> | -object_id <id>} -set_media_threshold <cmd> [option]: Changes DIEH Media threshold\n\
                                                    <cmd>:  'default', 'increase', or 'disable'\n\
                                                    [option]:  Only applies to 'increase'. A percentage multipler\n\
Example:\n\
pdo_srvc -object_id 0x10 -link_fault clear\n\
pdo_srvc -object_id 0x10 -drive_fault set\n\
"

#define FBE_CLI_PDO_MAX_MODE_SENSE_10_SIZE ((fbe_u16_t)320)

fbe_status_t fbe_cli_physical_drive_display_stats(fbe_u32_t object_id);
void fbe_cli_cmd_set_pdo_drive_states(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_cmd_set_ica_stamp(int argc , char ** argv);
fbe_status_t fbe_cli_pdo_parse_bes_opt(char *bes_str, fbe_object_id_t * pdo_object_id_p);

#endif /* FBE_CLI_PHYSICAL_DRIVE_OBJ_H */
