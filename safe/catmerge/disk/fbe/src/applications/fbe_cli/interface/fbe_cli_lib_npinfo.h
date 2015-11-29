#ifndef __FBE_CLI_NPINFO_H__
#define __FBE_CLI_NPINFO_H__

/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_npinfo.h
 ***************************************************************************
 *
 * @brief
 *  This file contains nonpaged metadata info command cli definitions.
 *
 * @version
 *  04/16/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cli_npinfo(int argc , char ** argv);
#define NPINFO_CMD_USAGE "\n\
npinfo -help|h\n\
npinfo -object_id x\n\
npinfo -object_id x [switches]\n\
\n\
if no switches, display nonpaged data of the object\n"


/*************************
 * end file fbe_cli_npinfo.h
 *************************/

#endif /* end __FBE_CLI_NPINFO_H__ */
