#ifndef FBE_CLI_H
#define FBE_CLI_H
 
#include "csx_ext.h" 
#include "fbe/fbe_winddk.h"
#include "fbe_trace.h"

typedef enum fbe_cli_cmd_type_e {
    FBE_CLI_CMD_TYPE_STANDALONE,
    FBE_CLI_CMD_TYPE_INIT_REQUIRED,
    FBE_CLI_CMD_TYPE_QUIT,
} fbe_cli_cmd_type_t;

#define FBE_CLI_USER_ACCESS 0
#define FBE_CLI_SRVC_ACCESS 1

typedef void (*fbe_cli_cmd_func)(int argc , char ** argv);

typedef struct fbe_cli_cmd_s {
    fbe_cli_cmd_type_t type;
    const char * name;
    const char * name_a_p;
    const char * arg_options;
    const char * description;
    fbe_cli_cmd_func func;
    fbe_u32_t access;             /* access level required to execute this command */
} fbe_cli_cmd_t;

typedef struct fbe_cli_cmd_old_s {
    fbe_cli_cmd_type_t type;
	const char * name;
	const char * arg_options;
	const char * description;
	fbe_cli_cmd_func func;
} fbe_cli_cmd_old_t;

#define FBE_CLI_CONSOLE_MAX_FILENAME 128
#define MAX_BUFFER_SIZEOF 8192

#define FBE_CLI_CONSOLE_OUTPUT_NONE   0x00
#define FBE_CLI_CONSOLE_OUTPUT_STDOUT 0x01
#define FBE_CLI_CONSOLE_OUTPUT_FILE   0x02
#define FBE_CLI_CONSOLE_OUTPUT_FILENAME "flarecons.out"

/* Common utilities */
void fbe_cli_printf(const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void fbe_cli_raw_printf(const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void fbe_cli_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));
void fbe_cli_error(const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,1,2)));

void fbe_cli_print_usage(void);
void fbe_cli_prompt_mode(void);
void fbe_cli_split(char *line, int *argc, char ** argv);
void fbe_cli_cmd_lookup(char *argv0, fbe_cli_cmd_t **cmd);

void fbe_cli_cmd_old_lookup(char *argv0, fbe_cli_cmd_old_t **cmd);
void fbe_cli_run_direct_command(char *direct_command);

fbe_bool_t fbe_cli_is_simulation(void);

#endif /* FBE_CLI_H */
