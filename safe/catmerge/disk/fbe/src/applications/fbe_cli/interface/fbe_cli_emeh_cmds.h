#ifndef __FBE_CLI_EMEH_CMDS_H__
#define __FBE_CLI_EMEH_CMDS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_emeh.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definition for Extended Media Error Handling (EMEH)
 *  CLI command.
 *
 * @date
 *  03/11/2015  Ron Proulx  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_common.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void fbe_cli_emeh(int argc, char** argv);
#define FBE_CLI_EMEH_USAGE "\
emeh - Display or change Extended Media Error Handling (EMEH) on target SP\n\
\n\
Usage:\n\
emeh -get -class              Get the EMEH values for the raid group class (i.e. the SP)\n\
     -get -rg_obj <obj id>    Get the EMEH values for the raid group specified by <obj id>\n\
\n\
     -set -class  <emeh mode> [-inc_prct <percent increase>] [-persist] \n\
                              Set the EMEH mode (enable or disable) and optionally\n\
                              the increase percent for the raid group class (i.e. the SP)\n\
                              If -persist is specified the EMEH registry values are changed.\n\
          <emeh mode>         The EMEH mode to set for the SP:\n\
                              A emeh mode of 1 enables EMEH for the SP \n\
                              A emeh mode of 5 disables EMEH for the SP \n\
          -inc_prct           Change the increase percent by the value supplied. \n\
          <percent increase>  The percent to increase the EMEH threshold before a drive \n\
                              is failed due to too many media errors.  The default increase is \n\
                              100 percent of the current value. \n\
          -persist            Persist (thru the registry) the modified values. \n\
\n\
     -set -rg_obj <obj id> <emeh control> [-inc_prct <percent increase>] \n\
                              Set the EMEH condition (based on the emeh_control) for the\n\
                              raid group specified by the raid group object id.\n\
          <obj id>            The object id of the raid group to set emeh condition for. \n\
          <emeh_control>      A emeh control of 3 restores the default EMEH settings for the raid group \n\
                              A emeh control of 4 increases the media error thresholds for the raid group \n\
                              A emeh control of 5 disable EMEH for the specified raid group\n\
                              A emeh control of 6 enables EMEH for the specified raid group\n\
          -inc_prct           Change the increase percent by the value supplied. \n\
          <percent increase>  The percent to increase the EMEH threshold before a drive \n\
                              is failed due to too many media errors.  The default increase is \n\
                              100 percent of the current value. \n\
\n\
Note: Only changes the values on the target SP. \n\
E.g\n\
  emeh -get -class\n\
  emeh -set -class 5 -persist\n\
  emeh -get -rg_obj 0x118\n\
  emeh -set -rg_obj 0x118 4 -inc_prct 200\n\
  emeh -set -rg_obj 0x118 3 \n\
\n"

/*************************
 * end file fbe_cli_emeh_emds.h
 *************************/

#endif /* end __FBE_CLI_EMEH_CMDS_H__ */
