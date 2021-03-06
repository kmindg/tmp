#include "fbe_cli_private.h"
#include "fbe/fbe_api_common.h"

static void prompt_split(char *line, int *argc, char ** argv, fbe_u32_t max_args)
{
    static fbe_u8_t * prompt_delimit = " ,\t";

    *argc = 0;
    *argv = strtok(line, prompt_delimit);
    while (*argv != NULL && (*argc < (max_args - 1))) {
        *argc = *argc + 1;
        *argv++;
        *argv = strtok(NULL, prompt_delimit);
    }
}

/*!*******************************************************************
 * @var fbe_cli_command_to_ktrace_buff
 *********************************************************************
 * @brief Function to send fbe_cli commands into ktrace start buffer.
 *
 *  @param    cmd - Command
 *  @param    length - size of the command.
 *
 * @return - none.  
 *
 * @author
 *  2/28/2013 - Created. Preethi Poddatur
 *********************************************************************/
void fbe_cli_command_to_ktrace_buff(fbe_u8_t *cmd, fbe_u32_t length) 
{
   fbe_status_t status = FBE_STATUS_OK;

   status = fbe_api_command_to_ktrace_buff(cmd, length);
   if (status != FBE_STATUS_OK) {
      fbe_cli_printf("%s: FBE_CLI command was not entered into ktrace start buffer - %d\n", __FUNCTION__, status);
   }
   return;
}


/******************************************
 * end fbe_cli_command_to_ktrace_buff()
 ******************************************/

void fbe_cli_prompt_mode()
{
    fbe_u8_t cmd_line[CMD_MAX_SIZE+1] = {0};
    char *argv[CMD_MAX_ARGC] = {0};
    fbe_u32_t argc = 0;
    int i, ch;
    fbe_cli_cmd_t *cmd = NULL;
    fbe_u8_t cmd_line_ktrace[MAX_KTRACE_BUFF+1] = {0};
    fbe_u32_t ktrace_buf_len;

    while(FBE_TRUE) {
        fbe_cli_printf("FBE_CLI>");
        /* reset the counter and buffer*/
        argc = 0;
		cmd_not_executed = TRUE;
        fbe_zero_memory(cmd_line, sizeof(cmd_line));
        fbe_zero_memory(cmd_line_ktrace, sizeof(cmd_line_ktrace));
        fbe_zero_memory(argv, sizeof(argv));

        ktrace_buf_len = 0;
        /* Read in single line from "stdin" */
	for( i = 0; (i < CMD_MAX_SIZE) && ((ch = getchar()) != EOF) && (ch != '\n'); i++ )
        {
            cmd_line[i] = (fbe_u8_t)ch;
            if (i < MAX_KTRACE_BUFF) {
                cmd_line_ktrace[i] = (fbe_u8_t)ch;
                cmd_line_ktrace[i+1] = '\0';
                ktrace_buf_len ++;
            }
        }
        /* for EOF character, breaks out from the loop*/
        if(ch == EOF){
            break;
        }  
		
        /* Terminate string with null character: */
        cmd_line[i] = '\0';
        /* for debug only: fbe_cli_printf( "Input was: %s\n", cmd_line); */
        prompt_split(cmd_line, &argc, argv, CMD_MAX_ARGC);
        /* we have some args, look up the cmd and launch the cmd and give the */
        if (argc == 0) {
            continue;
        }
        fbe_cli_cmd_lookup(argv[0], &cmd);
        if (cmd == NULL) {
            fbe_cli_print_usage();
            continue;
        }
        if (cmd->type == FBE_CLI_CMD_TYPE_QUIT) {
            break;
        }
        argc--;
		if (operating_mode == USER_MODE &&
                        cmd->access != FBE_CLI_USER_ACCESS)
        {
                        /* Skip over this command,
                         * we will ignore these commands in user mode.
                         */
        }
		else
		{
            fbe_cli_command_to_ktrace_buff(cmd_line_ktrace, ktrace_buf_len);
            (*cmd->func)(argc, &argv[1]);
		   cmd_not_executed = FALSE;
		}
		 /* Output error message */
        if (cmd_not_executed)
		{
		   printf("Command \"%s\" NOT found\n", argv[0]);
        }
    }
    printf("\n");
    fflush(stdout);
    
}

void fbe_cli_run_direct_command(char *direct_command)
{
    char *argv[CMD_MAX_ARGC] = {0};
    fbe_u32_t argc = 0;
    fbe_cli_cmd_t *cmd = NULL;

    
	fbe_cli_printf("FBE_CLI>");
	fbe_cli_printf(direct_command);
	fbe_cli_printf("\n");
	/* reset the counter and buffer*/
	argc = 0;
	cmd_not_executed = TRUE;
    fbe_zero_memory(argv, sizeof(argv));

    /* for debug only: fbe_cli_printf( "Input was: %s\n", cmd_line); */
	prompt_split(direct_command, &argc, argv, CMD_MAX_ARGC);
	/* we have some args, look up the cmd and launch the cmd and give the */
	if (argc == 0) {
		return;
	}
	fbe_cli_cmd_lookup(argv[0], &cmd);
	if (cmd == NULL) {
		fbe_cli_print_usage();
		return;
	}
	if (cmd->type == FBE_CLI_CMD_TYPE_QUIT) {
		return;
	}
	argc--;
	if (operating_mode == USER_MODE &&
					cmd->access != FBE_CLI_USER_ACCESS)
	{
					/* Skip over this command,
					 * we will ignore these commands in user mode.
					 */
	}
	else
	{
	   (*cmd->func)(argc, &argv[1]);
	   cmd_not_executed = FALSE;
	}
	 /* Output error message */
	if (cmd_not_executed)
	{
	   printf("Command \"%s\" NOT found\n", argv[0]);
	}

    printf("\n");
    fflush(stdout);

}



