#ifndef FBE_CLI_LIB_MODULE_MGMT_CMD_H
#define FBE_CLI_LIB_MODULE_MGMT_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_module_info.h"
#include "fbe_cli_private.h"

/* Function prototypes*/
void fbe_cli_cmd_ioports(fbe_s32_t argc,char ** argv);
static void fbe_cli_print_all_iom_physical_info(void);
static void fbe_cli_print_all_iom_logical_info(void);
static void fbe_cli_print_all_iomenv_info(void);
static void fbe_cli_print_all_port_limit_info(void);
static void fbe_cli_print_all_port_affinity_settings(void);
static void fbe_cli_print_all_iom_and_ports_info(void);
static void fbe_cli_print_all_iomodules_info(fbe_u32_t which_sp,fbe_u32_t device_type);
static void fbe_cli_print_all_ports_info(fbe_esp_module_limits_info_t *limits_info,
                                         fbe_esp_module_io_module_info_t *module_info,
                                         fbe_u64_t device_type,
                                         fbe_u32_t which_sp);
static void fbe_cli_print_physical_info(fbe_u32_t which_sp,fbe_u32_t device_type);
static void fbe_cli_print_logical_info(fbe_esp_module_io_port_info_t *io_port_info, fbe_u64_t device_type);
static void fbe_cli_print_iomenv_info(fbe_u32_t which_sp,fbe_u64_t device_type);
static void fbe_cli_print_mezzanine_info(fbe_esp_module_io_module_info_t *mezzanine_info,
                                         fbe_esp_module_mgmt_module_status_t module_status_info,
                                         fbe_u32_t num_mezzanine_slots);
static void fbe_cli_print_iomodule_info(fbe_esp_module_io_module_info_t *iomodule_info,
                                        fbe_esp_module_mgmt_module_status_t module_status_info,
                                        fbe_u32_t slot_num);
static void fbe_cli_print_port_info(fbe_esp_module_io_port_info_t *io_port_info, fbe_u32_t port_num);
static void fbe_cli_print_port_info_by_role(fbe_port_role_t port_role,fbe_u32_t which_sp,fbe_u32_t device_type);
static void fbe_cli_print_port_affinity_settings(fbe_u32_t which_sp, fbe_u32_t device_type);

static fbe_char_t * fbe_module_mgmt_get_env_status(fbe_esp_module_io_module_info_t  module_info);
static fbe_u32_t fbe_cli_module_mgmt_get_platform_port_limit(fbe_environment_limits_platform_port_limits_t *platform_port_limits,
                                                             IO_CONTROLLER_PROTOCOL port_protocol, 
                                                             fbe_port_role_t port_role);
void fbe_cli_module_mgmt_config_mgmt_port(fbe_s32_t argc,char ** argv);
void  fbe_cli_cmd_set_port_cmd(fbe_s32_t argc,char ** argv);
static void fbe_cli_print_all_port_link_status(void);
static void fbe_cli_print_port_link_status(fbe_u32_t which_sp, fbe_u32_t device_type);



#define IOPORTS_USAGE                               \
" ioports            - Display IOM and Port Information\n\n"  \
" usage:  \n"  \
" ioports -help         - For this help information\n"  \
" ioports               - Displays All IOM and Port Information (Default)\n"  \
" ioports -physical     - Displays IOM Information (Physical)\n"  \
" ioports -logical      - Displays Port Information by Role\n"  \
" ioports -iomenv       - Displays IOM Environment status\n"  \
" ioports -limits       - Displays current port limit\n"  \
" ioports -affinity     - Displays Port Interrupt Affinity Settings\n"

#define FBE_CONFIGMGMTPORT_USAGE                      \
"Configure the management port speed setting.\n"    \
" cmp | configmgmtport <options>: \n"             \
"    -status: display management port status \n"  \
"    -auto on/off: auto negotiate mode \n"        \
" Note:  off requires both -speed and -duplex specified\n" \
"    -speed 10m/100m/1000m: port speed \n"           \
"    -duplex half/full: duplex mode  \n"          \
"    -norevert: revert disabled (default: revert enabled)\n"

#define FBE_SETPORT_USAGE        \
"        setport <switches>\n\n\
Switches:\n\
 no switch: get input for persist port from user. \n\
 [-remove] Optional switch, set ALL ports (except BE0) to\n\
                an uninitialized state\n\
 [-remove_list] Optional switch, remove prots listed.\n\
 \t Format: setport -remove_list -c #1 -s #1 -p #1 -c #2 -s #2 -p #2...\n\
 [-persist_list] Optional switch, persist prots listed.\n\
 \t Format: setport -persist_list -c #1 -s #1 -p #1 -l #1 -g #1 -pr #1 -psr #1 -c #2 -s #2 -p #2 -l #2 -g #2 -pr #2 -psr #2...\n\
 [-persist] Optional switch, persist ALL ports from hardware.\n\
 -c <#>         Class of IO module\n\
                  0 for unknown\n\
                  1 for slic\n\
                  2 for mezzanine\n\
                  3 for BM\n\
 -s <#>         Slot # of IO Module\n\
 -p <#>         Port # as it appears on the label for IOM Module\n\
 -l <#>         Logical Number (FE or BE bus number, 0x0 to 0xFE)\n\
 -g <#>         IOM Group\n\
                1 - Group A - FC 4G - Tomahawk\n\
                2 - Group B - iSCSI 1G - Harpoon\n\
                3 - Group C - SAS BE only - Coromandel/Hypernova/MoonLite\n\
                4 - Group D - FC 8G - Glacier\n\
                5 - Group E - iSCSI 10G - Poseidon\n\
                6 - Group F - FCoE - Heatwave\n\
                7 - Group G - iSCSI 1G 4 Port - Supercell\n\
                8 - Group H - SAS FE only - Coldfront\n\
                9 - Group I - iSCSI 10G Copper - Eruption\n\
               10 - Group J - iSCSI 10G Optical - El Nino\n\
               11 - Group K - SAS 6G BE - Moonlite\n\
               12 - Group L - iSCSI 10G 4 Port Optical - Landslide\n\
               13 - Group M - SAS 12G BE - Snowdevil/Dustdevil\n\
               14 - Group N - iSCSI 1G 4 Port SLIC 2.0 - Thunderhead\n\
               15 - Group 0 - iSCSI 10G 4 Port SLIC 2.0 - Rockslide\n\
               16 - Group P - FC 16G - Vortex\n\
               17 - Group Q - iSCSI 10G - Maelstrom\n\
 -pr <#>        Port Role\n\
                  0 for UNINITIALIZED\n\
                  1 for Front End\n\
                  2 for Back End\n\
 -psr <#>       Port Subrole\n\
                  0 for UNINTIALIZED\n\
                  1 for SPECIAL\n\
                  2 for NORMAL\n\
 [-upgrade]     Optional switch for SLIC Upgrade\n\
 [-replace]     Optional switch to reboot and hold in reset to replace SLIC\n\
 [-remove_list]         Optional switch for port removal\n\
                remove_list -c <class> -s <slot> [-p <port>]\n\
                -p optional for -rem. If not specified,\n\
                all ports for that slic would be removed [except BE0]\n"

/* minimum valid # of set_port cmd args */
#define VALID_SETPORT_CMD_ARGS_SIZE 14
#define VALID_SETPORT_REM_PORT_CMD_ARGS_SIZE 8
#define VALID_SETPORT_REM_ALL_PORT_CMD_ARGS_SIZE 6
/* start position of set_port cmd in arg array*/
#define SET_PORT_CMD_FIRST_ARG_POS 1
#define SET_PORT_REM_PORT_CMD_FIRST_ARG_POS 1

#define SET_PORT_PERSIST_ARG_NUM 14
#define SET_PORT_REM_ARG_MIN 4
/* FCLI IOM and Port Table Headings for Flexport Support*/
#define IOM_ENV_HEADING "IOM#   ENV STATE    EXT STATUS    UNIQUE ID    USES SFP      SUPP SPEEDS\n"
#define MEZZ_ENV_HEADING "CTRL#   ENV STATE    UNIQUE ID    USES SFP      SUPP SPEEDS\n"
#define IOM_HEADING "IOM# INSERTED #_PORTS  PWR_STAT    TYPE      STATE  SUBSTATE   IN_CARR\n"
#define PORT_HEADING "IOM#  PLABEL# LOGICAL#    ROLE   STATE     SUBSTATE  PROTOCOL   SUBROLE\n"
#define MEZZANINE_HEADING "INSERTED  #_CTRLS    STATE    SUBSTATE       FFID\n"
#define CTRLS_HEADING "PLABEL# CTRL#  PNUM#  LOGICAL#   ROLE    STATE    SUBSTATE   PROTOCOL   SUBROLE\n"
#define DASHED_LINE "-------------------------------------------------------------------------------\n"

#define IOM_GROUP_EXPECTED_VALUES \
"IO Module Group Expected values are:\n\
1 - Group A - FC 4G - Tomahawk\n\
2 - Group B - iSCSI 1G - Harpoon\n\
3 - Group C - SAS BE only - Coromandel/Hypernova/MoonLite\n\
4 - Group D - FC 8G - Glacier\n\
5 - Group E - iSCSI 10G - Poseidon\n\
6 - Group F - FCoE - Heatwave\n\
7 - Group G - iSCSI 1G 4 Port - Supercell\n\
8 - Group H - SAS FE only - Coldfront\n\
9 - Group I - iSCSI 10G Copper - Eruption\n\
10 - Group J - iSCSI 10G Optical - El Nino\n\
11 - Group K - SAS 6G BE - Moonlite\n\
12 - Group L - iSCSI 10G 4 Port Optical - Landslide\n\
13 - Group M - SAS 12G BE - Snowdevil/Dustdevil\n\
14 - Group N - iSCSI 1G 4 Port SLIC 2.0 - Thunderhead\n\
15 - Group 0 - iSCSI 10G 4 Port SLIC 2.0 - Rockslide\n\
16 - Group P - FC 16G - Vortex\n\
17 - Group Q - iSCSI 10G - Maelstrom\n\n"

#endif /* FBE_CLI_LIB_MODULE_MGMT_CMD_H */
