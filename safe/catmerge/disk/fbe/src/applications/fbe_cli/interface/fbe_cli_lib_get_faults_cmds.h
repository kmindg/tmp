#ifndef FBE_CLI_LIB_GET_FAULTS_CMD_H
#define FBE_CLI_LIB_GET_FAULTS_CMD_H

#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe_cli_private.h"
#include "fbe_cli_sp_cmd.h"

/* Function prototypes*/
void fbe_cli_cmd_get_faults(fbe_s32_t argc, char ** argv);
static fbe_bool_t fbe_cli_print_esp_sp_faults(void);
static fbe_bool_t fbe_cli_print_esp_sp_fltReg_faults(void);
static fbe_bool_t fbe_cli_print_esp_suitcase_faults(void);
static fbe_bool_t fbe_cli_print_esp_bmc_faults(void);

static fbe_bool_t fbe_cli_print_esp_module_faults(fbe_u64_t deviceType);

static fbe_bool_t fbe_cli_print_esp_mgmt_module_faults(void);

static fbe_bool_t fbe_cli_print_esp_bbu_faults(void);
static fbe_bool_t fbe_cli_print_esp_sps_faults(void);

static fbe_bool_t fbe_cli_print_esp_encl_faults(void);
static fbe_bool_t fbe_cli_print_esp_specific_encl_faults(fbe_device_physical_location_t *pLocation);

static fbe_bool_t fbe_cli_print_esp_fan_faults(fbe_device_physical_location_t *pLocation);
static fbe_bool_t fbe_cli_print_esp_ps_faults(fbe_device_physical_location_t *pLocation);

static fbe_bool_t fbe_cli_print_esp_drive_faults(fbe_device_physical_location_t *pLocation);

static fbe_bool_t fbe_cli_any_resume_prom_fault(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation, char *fault_string);

static void fbe_cli_cat_fault_string(char *fault_string, char *fault_reason, fbe_u32_t string_length);

#define FAULT_STRING_LENGTH 500
#define GET_FAULTS_USAGE                               \
" getfaults             - Show System Array faults.\n\n"  \
" USAGE:  \n"  \
"--------------------------------------------------------------------\n" \
" getfaults -help          - Show help information.\n\n"  \
" getfaults                - Show esp faults(Default).\n"  \
" getfaults esp|pp|sep     - Show system array faults by package.\n"

#endif /*FBE_CLI_LIB_GET_FAULTS_CMD_H*/

