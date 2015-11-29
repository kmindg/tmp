#ifndef FBE_CLI_BECHECK_H
#define FBE_CLI_BECHECK_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_becheck.h
 ***************************************************************************
 *
 * @brief
 *  This file contains becheck command cli definitions.
 *
 * @version
 *  06/03/2015 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cli_becheck_cmd(int argc, char **argv);

#define BECHECK_CMD_USAGE "\n\
becheck -help|h\n\
becheck -install/all -id -debug -v\n\
\n\
By default, becheck will run in normal mode\n\
You can use switches to change mode\n\
\n\
    -install       run becheck in install mode\n\
    -all           run becheck in full mode\n\
    -id            print drives id\n\
    -v             print more information\n\
    -debug         print debug log\n\
\n"

/*************************
 * end file fbe_cli_becheck.h
 *************************/


#endif
