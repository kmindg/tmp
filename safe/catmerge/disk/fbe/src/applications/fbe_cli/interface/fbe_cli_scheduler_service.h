#ifndef FBE_CLI_SCHEDULER_H
#define FBE_CLI_SCHEDULER_H

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_scheduler(int argc , char ** argv);

#define FBE_SCHEDULER_USAGE  "\
sched - control and show scheduler information\n\
usage:  sched -help               - For this help information.\n\
        sched -set_scale          - Sets credits on a scale of 0-100 \n\
        sched -get_scale          - Shows current scale\n\
                             \n\
        sched -add_hook -object_id (id) -state (state) -substate (substate) -val1 (val1) -val2 (val2) \n\
                        -action (log | continue | pause | continue | clear)\n\
                        -check_type (check_states | vals_eq | vals_lt | vals_gt | )\n\
        sched -get_hook -object_id (id) -state (state) -substate (substate) -val1 (val1) -val2 (val2) \n\
                        -action (log | continue | pause | continue | clear)\n\
                        -check_type (check_states | vals_eq | vals_lt | vals_gt | )\n\
        sched -del_hook -object_id (id) -state (state) -substate (substate) -val1 (val1) -val2 (val2) \n\
                        -action (log | continue | pause | continue | clear)\n\
                        -check_type (check_states | vals_eq | vals_lt | vals_gt | )\n\
\n\
Examples:  sched -add_hook -object_id 0x115 -action pause -state 25 -substate 27 -check_type check_states\n\
           sched -get_hook -object_id 0x115 -action pause -state 25 -substate 27 -check_type check_states\n\
           sched -del_hook -object_id 0x115 -action pause -state 25 -substate 27 -check_type check_states\n\
        \n"

   
#endif /* FBE_CLI_SCHEDULER_H */
