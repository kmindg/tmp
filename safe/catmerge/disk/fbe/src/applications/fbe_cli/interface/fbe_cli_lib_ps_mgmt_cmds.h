#ifndef FBE_CLI_LIB_PS_MGMT_CMD_H
#define FBE_CLI_LIB_PS_MGMT_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe_ps_mgmt_util.h"
#include "fbe_cli_private.h"

/* Function prototypes*/
void fbe_cli_cmd_psmgmt(fbe_s32_t argc,char ** argv);
static void fbe_cli_print_specific_PS_status(fbe_u8_t bus_num, fbe_u8_t encl_num ,fbe_u8_t slot_num);
static void fbe_cli_print_all_PS_slots_status_on_encl(fbe_u8_t bus_num, fbe_u8_t encl_num);
static void fbe_cli_print_all_PS_status_on_bus(fbe_u8_t bus_num);
static void fbe_cli_print_all_PS_status(void);

static BOOL fbe_cli_verify_encl_num_on_bus(fbe_u8_t bus_num, fbe_u8_t encl_num);
static BOOL fbe_cli_verify_ps_slot_num_on_encl(fbe_u8_t bus_num, fbe_u8_t encl_num, fbe_u8_t slot_num);

#define PSMGMT_USAGE                               \
" psmgmt                - Displays ESP power supply Information.\n\n"  \
" USAGE:  \n"  \
"--------------------------------------------------------------------\n" \
" psmgmt -help          - Displays help information.\n\n"  \
" psmgmt                - Displays status for all power supplies on the system(Default).\n"  \
" psmgmt -b #           - Displays status for all power supplies on the specified bus.\n"  \
" psmgmt -b # -e #      - Displays status for all power supplies on the specified enclosure(Requires bus).\n"  \
" psmgmt -b # -e # -s # - Displays status for the specified power supply(requires bus and encl pos).\n"  \
" psmgmt -set_expected_ps_type - Sets the expected power supply type for Processor Enclosure Octane/None/Other.\n"

#endif /*FBE_CLI_LIB_PS_MGMT_CMD_H*/

