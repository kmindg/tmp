#ifndef FBE_CLI_LIB_BOARD_MGMT_CMD_H
#define FBE_CLI_LIB_BOARD_MGMT_CMD_H


#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe_cli_private.h"
#include "generic_utils_lib.h"
#include "generic_utils.h"
#include "fbe_board_mgmt_utils.h"

/* Function prototypes*/
void fbe_cli_cmd_boardmgmt(fbe_s32_t argc,char ** argv);
static void fbe_cli_print_board_info(void);
static void fbe_cli_print_board_cache_sataus(void);
static void fbe_cli_print_suitcase_info(fbe_u8_t which_sp);
static void fbe_cli_print_board_bmc_info(void);
static void fbe_cli_print_peer_boot_info(void);
static void fbe_cli_print_cache_card_status(void);
static void fbe_cli_print_dimm_status(void);
extern void fbe_cli_print_flt_reg_status(SPECL_FAULT_REGISTER_STATUS * fltRegStatusPtr);
extern char * fbe_cli_print_HwCacheStatus(fbe_common_cache_status_t hw_cache_status);

#define BOARDMGMT_USAGE                               "\
boardmgmt             - Displays ESP Board Management Information.\n\
 USAGE:  \n\
--------------------------------------------------------------------\n\
 boardmgmt -help       - Displays help information.\n\
 boardmgmt             - Displays board information (Default).\n\
 boardmgmt -c <boardCompType>\n\
    boardCompType : misc, suitcase, mcu, bmc, fltreg, cachecard, dimm \n\
"

#endif /*FBE_CLI_LIB_BOARD_MGMT_CMD_H*/

