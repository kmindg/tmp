#ifndef FBE_CLI_BUS_CMD_H
#define FBE_CLI_BUS_CMD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_bus_cmd.h
 ***************************************************************************
 *
 * @brief
 *  This file contains bus command definitions.
 *
 * @version
 *  21/Feb/2012 - Created. Eric Zhou
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_cli.h"

/*************************
 * FUNCTION PROTOTYPES
 *************************/
void fbe_cli_cmd_bus_info(fbe_s32_t argc,char ** argv);

#define BUSINFO_USAGE "\
businfo/bi  Display bus information.\n\
Usage: businfo -h for help.\n\
       businfo show all backend buses information.\n\
"

#endif /* FBE_CLI_BUS_CMD_H */
