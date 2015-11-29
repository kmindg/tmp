#ifndef FBE_CLI_LIB_BOARD_CMD_H
#define FBE_CLI_LIB_BOARD_CMD_H

#define REBOOTSP_USAGE                               \
"       rebootsp -sp <a/b> [-hold 0/1/2/4]\n"          \
"       -sp <a/b>        a: reboot SPA\n"              \
"                        b: reboot SPB\n"              \
"       -hold <0/1/2/4>  0: Normal reboot (default)\n" \
"                        1: Hold-In-POST after reboot\n" \
"                        2: Hold-In-Reset after reboot\n" \
"                        4: Hold-In-POWEROFF\n"


#define REBOOT_SP_OPTIONS_NONE              0x0
#define REBOOT_SP_OPTIONS_HOLD_IN_POST      0x1
#define REBOOT_SP_OPTIONS_HOLD_IN_RESET     0x2
#define REBOOT_SP_OPTIONS_HOLD_IN_POWER_OFF 0x4

#define BOARDPRVDATA_USAGE                     \
"       boardprvdata -set_async_io          \n"\
"       -set_async_io						\n"\
"       -set_sync_io						\n"\
"       -disable_dmrb_zeroing				\n"\
"       -enable_dmrb_zeroing				\n" 



#endif /*FBE_CLI_LIB_BOARD_CMD_H*/
