#ifndef __FBE_CLI_MMINFO_H__
#define __FBE_CLI_MMINFO_H__

/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_mminfo.h
 ***************************************************************************
 *
 * @brief
 *  This file contains metadata memory info command cli definitions.
 *
 * @version
 *  10/29/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cli_mminfo(int argc , char ** argv);
#define MMINFO_CMD_USAGE "\n\
mminfo -help|h\n\
mminfo [-p] -object_id x\n\
mminfo -object_id x [switches]\n\
\n\
if -p is specified, display peer metadata memory (switches parameters will be ignored).\n\
if no switches, display metadata memory data of the object\n"


/*************************
 * end file fbe_cli_mminfo.h
 *************************/

#endif /* end __FBE_CLI_MMINFO_H__ */
