#ifndef FBE_CLI_BVD_H
#define FBE_CLI_BVD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_bvd.h
 ***************************************************************************
 *
 * @brief
 *  This file contains BVD commands related definitions
 *
 * @version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_common.h"

void fbe_cli_bvd_commands(int argc, char** argv);

#define FBE_CLI_BVD_USAGE "\
bvd <options>\n\
Commands:\n\
-get_stat           -   Show IO related statistics in BVD\n\
-enable_stat        -   Enable statistics gathering\n\
-disable_stat       -   Disable statistics gathering\n\
-clear_stat         -   Clear current counters\n\
-set_async_io       -   Break context for I/O from $ \n\
-set_sync_io        -   Do not break context for I/O from $\n\
-set_async_compl       -   Break context for I/O completion to $ \n\
-set_sync_compl        -   Do not break context for I/O completion to $\n\
-enable_gp          -   Enable run queue group priority queuing\n\
-disable_gp         -   Disable run queue group priority queuing\n\
-enable_pp_gp          -   Enable run queue group priority queuing\n\
-disable_pp_gp         -   Disable run queue group priority queuing\n\
-set_rq_method {0 | 1 | 2} -   0 - same core, 1 - next core, 2 - round robin\n\
-set_alert_time seconds -  sets the alert time in seconds\n\
-shutdown               -  Sends a shutdown command to the BVD to simulate a reboot.\n\
\n"

#endif /*FBE_CLI_BVD_H*/
