#ifndef FBE_CLI_LIB_GREEN_CMDS_H
#define FBE_CLI_LIB_GREEN_CMDS_H

void fbe_cli_green(int argc, char** argv);

#define ZERO 0

#define GREEN_USAGE         \
"\ngreen <operations> [switches]\n"\
"       green -state enable|disable|state\n"\
"       green -wakeup_time #period\n"\
"       green -rg #raid_group_id -state enable|disable|state -idle_time #period\n"\
"       green -disk [-b #bus -e #enc -s #slot] -state enable|disable -idle_time #period\n"\
"       green -list [-b #bus -e #enc -s #slot]\n"\
"Operations:\n"\
"     -help           (display the usage of the green functionality.)\n"\
"     -state          (enables or disables or show the state of the system green functionality.)\n"\
"     -wakeup_time    (set the wakeup time in minute(s) for system green functionality.)\n"\
"     -list           (lists stand by state of all drives in the system.)\n"\
"     -rg  no option  (display the power saving info for the specified raid group.)\n"\
"          -state enable|disable|state (enable or disable or show the state of power saving for the specified raid group.)\n"\
"          -idle_time (set idle_time in seconds for the specified raid group.)\n"\
"     -disk           (enable|disable power save for the specified disk.)\n"\
"Switches:\n"\
"     -b  - bus number. e.g. -b 1\n"\
"     -e  - enclosure number. e.g. -e 1\n"\
"     -s  - slot number. e.g. -s 1\n\n"\
"Example(s):\n"\
"   gr -state enable                         : Enables the system green functionality.\n"\
"   gr -state state                          : Displays the system green state.\n"\
"   gr -disk -b 0 -e 2 -s 1 -state enable    : Enables the power save of disk of bus\n"\
"                                              0 enc. 2 slot 1.\n"\
"   gr -rg 2 -idle_time 1800                 : Set the power saving idle time as 1800 seconds for the raid group 2.\n"\
"   gr -rg 1                                 : Display the power saving info for the raid group 1.\n"\
"   gr -list -b 0 -e 0 -s 5                  : Lists the power save state of disk of bus 0 enc 0 slot 5\n\n"

#endif //FBE_CLI_LIB_GREEN_CMDS_H
