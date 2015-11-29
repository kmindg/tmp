#ifndef __FBE_CLI_CMD_DPS_STATS_H__
#define __FBE_CLI_CMD_DPS_STATS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_cmd_perf_stats.h
 ***************************************************************************
 *
 * @brief
 *  This file contains DPS memory statistics command cli definitions.
 *
 * @version
 *  03/30/2011 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_memory.h"

void fbe_cli_cmd_dps_add_mem(int argc , char ** argv);
#define DPS_ADD_MEM_CMD_USAGE "\
dps_add_mem \n\
Flags: <none>\n\
     increase DPS memory for NEIT.\n"

void fbe_cli_cmd_dps_stats(int argc , char ** argv);

#define DPS_STATS_CMD_USAGE "\
dps -pool|-priority | -both (-neit)\n\
Flags:\n\
     default	- If no option provided, display all stats by pool, priority and by both.\n\
    -pool		- Get and display dps memory statistics by pool\n\
    -priority	- Get and display dps memory statistics by priority.\n\
    -both		- Get and display dps memory statistiscs by priority.\n\
	-fast_pool [-diff] - Get and display dps memory fast pools statistiscs -diff will display the difference from last time.\n\
    -neit  - Get DPS statistics from the neit package.\n"

void fbe_cli_cmd_display_env_limits(int argc , char ** argv);
#define DISPLAY_ENV_LIMITS_CMD_USAGE "\
display_environment_limits \n\
Options: <none>\n\
         Display environment limits such as fru_count, max_rg, max_user_lun.\n\n"

/*************************
 * end file fbe_cli_cmd_dps_stats.h
 *************************/

#endif /* end __FBE_CLI_CMD_DPS_STATS_H__ */
