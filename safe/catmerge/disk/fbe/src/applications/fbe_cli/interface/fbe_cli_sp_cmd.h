#ifndef __FBE_CLI_SP_H__
#define __FBE_CLI_SP_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_sp.h
 ***************************************************************************
 *
 * @brief
 *  This file contains sp command cli definitions.
 *
 * @version
 *  12/01/2010 - Created. Dongz
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_limits.h"

void fbe_cli_cmd_sp(int argc , char ** argv);
void fbe_cli_cmd_espcs(int argc , char ** argv);

#define SP_CMD_USAGE "\
  Show summary of various statistics/revisions. \n"
#define ESPCS_CMD_USAGE "\
  Show detailed ESP CacheStatus (-all for full details). \n"


#define FBE_CLI_SP_MAX_PS_COUNT 8
#define FBE_CLI_SP_MAX_FAN_COUNT 16
#define FBE_CLI_SP_CMD_STRING_LEN 256

typedef struct drive_print_format{
    fbe_esp_encl_type_t encl_type;
    fbe_u32_t bank_width;
}drive_print_format_t;

extern char * fbe_cli_print_EnclosureType(fbe_esp_encl_type_t encl_type);
extern char * fbe_cli_print_SpsCablingStatus(fbe_sps_cabling_status_t sps_cabling_status);
extern char * fbe_cli_print_SpsFaults(fbe_sps_fault_info_t *spsFaultInfo);
extern char * fbe_cli_print_HwCacheStatus(fbe_common_cache_status_t hw_cache_status);

/*************************
 * end file fbe_cli_sp.h
 *************************/

#endif /* end __FBE_CLI_SP_H__ */
