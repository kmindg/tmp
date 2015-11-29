#ifndef __FBE_CLI_SECTOR_TRACE_INFO_H__
#define __FBE_CLI_SECTOR_TRACE_INFO_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_sector_trace_info.h
 ***************************************************************************
 *
 * @brief
 *  This file contains sector_trace_info command cli definitions.
 *
 * @version
 *  08/10/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


void fbe_cli_sector_trace_info(int argc , char ** argv);

#define SECTOR_TRACE_INFO_CMD_USAGE "\
sector_trace_info | stinfo –h | -help \n\
sector_trace_info | stinfo -v | -verbose \n\
                           -set_trace_level | -stl <value> [-persist]\n\
                           -set_trace_mask  | -stm <value> [-persist]\n\
                           -reset_counters \n\
                           -display_counters \n\
                           -set_stop_on_error_flags | -ssoef <value> [-event <value>] [-persist]\n\
Switches:\n\
    -help|-h	           - Help \n\
    -verbose|-v            - Verbose mode \n\
    -set_trace_level|-stl  - Set the level of error to trace [trace level value ] \n\
                             Set to default if value is not given\n\
    -set_trace_mask|-stm   - Set trace mask that determines which error types are traces[trace mask value ]\n\
                             Set to default if value is not given\n\
    -reset_counters        - Reset the trace counters\n\
    -display_counters      - Display the trace counters\n\
    -set_stop_on_error_flags|-ssoef - Set the types of errors that will PANIC the SP [panic mask value ]\n\
                             Set to 0 if no value given\n\
    -event <value>         - Optional option with ssoef to set unsol event that will PANIC the SP eg: 0xE168C003 for SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR\n\
    -persist               - Optional option to persist in registry\n"


void fbe_cli_error_trace_counters(int argc , char ** argv);

#define ERROR_TRACE_COUNTERS_CMD_USAGE "\
error_trace_counters  \n\
Switches:\n\
    -get				   - Display error counters \n\
    -clear				   - Reset error counters\n"


/*************************
 * end file fbe_cli_sector_trace_info.h
 *************************/

#endif /* end __FBE_CLI_SECTOR_TRACE_INFO_H__ */
