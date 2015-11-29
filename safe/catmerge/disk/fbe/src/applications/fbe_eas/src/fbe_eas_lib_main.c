#include "fbe/fbe_cli.h"
#include "fbe_eas_private.h"
#include "fbe_eas_lib_eas_cmds.h"

int  eas_operating_mode;

static fbe_cli_cmd_t eas_cmds[] =
{
    {   FBE_CLI_CMD_TYPE_INIT_REQUIRED,
        "EAS",
        "eas",
        "",
        "Erase and Sanitize",
        fbe_cli_eas,
        FBE_CLI_USER_ACCESS    
    },
};

static int fbe_cli_cmd_count = sizeof(eas_cmds)/sizeof(fbe_cli_cmd_t);

void fbe_eas_cmd_lookup(char *argv0, fbe_cli_cmd_t **cmd)
{
    int ii;
    fbe_bool_t  cmdFound = FALSE;

    *cmd = NULL;
    for (ii = 0; ii < fbe_cli_cmd_count; ii++) {
        *cmd = &eas_cmds[ii];
        if ((strcmp(argv0, eas_cmds[ii].name) == 0) ||(strcmp(argv0, eas_cmds[ii].name_a_p) == 0)) {
            cmdFound = TRUE;
            break;
        }
    }

    if (!cmdFound)
    {
        *cmd = NULL;
    }

}

void fbe_eas_print_usage(void)
{
    fbe_cli_cmd_t * cmd;
    int ii;
    char buf[64] = {0};

    printf("\n");
    printf("usage: fbe_eas.exe <command>\n\n");
    printf("commands:\n");
    printf("Notes: command full Name/Abbreviation - Summary\n\n");

    for (ii = 0; ii < fbe_cli_cmd_count; ii++) 
    {
        cmd = &eas_cmds[ii];

        // If user specifies ENG mode, then print out all commands.  If not, only print out 
        // User Access commands. 
        if (((eas_operating_mode == USER_MODE) && (cmd->access == FBE_CLI_USER_ACCESS)) ||
            (eas_operating_mode == ENG_MODE))
        {
            fbe_sprintf(buf, sizeof(buf), "%s/%s", cmd->name, cmd->name_a_p);
            printf(" %-18s -%s\n", buf, cmd->description);
        }
    }
    printf("\n");
    fflush(stdout);
}

void fbe_eas_cmd_print_usage(int argc , char ** argv)
{
    fbe_eas_print_usage();
}

#if 0
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}
#endif
