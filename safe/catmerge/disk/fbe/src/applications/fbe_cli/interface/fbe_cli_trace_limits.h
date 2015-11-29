#ifndef __FBE_CLI_TRACE_LIMITS_H__
#define __FBE_CLI_TRACE_LIMITS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_trace_limits.h
 ***************************************************************************
 *
 * @brief
 *  This file contains trace limits command cli definitions.
 *
 * @version
 *  1/11/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"


void fbe_cli_trace_limits(int argc , char ** argv);
#define TRACE_LIMITS_CMD_USAGE "\
trace_limits -h |-help\n\
trace_limits -package (sep/pp/esp) -d -level <level> | -stop | -trace | -error_count count\n\
                               \n\
Switches:\n\
    -package package(sep/pp/esp) - Package to get/set trace limits.\n\
    -d    - Display mode (else set mode)\n\
    -stop - Set error limit action to stop.\n\
    -level level  - trace level to set for\n\
    -trace  - Set error limit action to trace.\n\
    -error_count - Set number of errors to take action after.\n\
\n\
Examples:\n\
    trace_limits -stop -package pp - Enables trace limits.\n\
    trace_limits -disable -package pp - Disables trace limits.\n\
    trace_level -stop -error_count 5 -level 1 -package sep\n\
    trace_level -disable -level 2 -package sep \n\
    trace_limits -stop -package pp -level 1 -error_count 5 \n\
                           \n"

/*************************
 * end file fbe_cli_trace_limits.h
 *************************/

#endif /* end __FBE_CLI_TRACE_LIMITS_H__ */
