#ifndef FBE_CLI_EVENT_LOG_CMDS_H
#define FBE_CLI_EVENT_LOG_CMDS_H


/*************************
 * INCLUDE FILES
 *************************/
#include "fbe/fbe_cli.h"
#include "fbe/fbe_types.h"


/*************************
 * PROTOTYPES
 *************************/
void fbe_cli_cmd_getlogs(fbe_s32_t argc,char ** argv);
void fbe_cli_cmd_clearlogs(fbe_s32_t argc,char ** argv);


#define GETLOGS_USAGE "\
getlogs/getlogs  Show all event log messages \n\
Usage: getlogs -h \n\
       getlogs <package name>\n\
       <package name> command issued for this package<valid packages are pp,sep,esp>.\n"

#define CLEARLOGS_USAGE "\
clearlogs/clearlogs  Wipes out all event log messages for the given package \n\
Usage: clearlogs -h \n\
       clearlogs <package name>\n\
       <package name> command issued for this package<valid packages are pp,sep,esp>.\n"


#endif /* FBE_CLI_EVENT_LOG_CMDS_H */
