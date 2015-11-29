
/***************************************************************************
 *                               AbstractProgramControl.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of an Abstract interface for managing 
 *               program executions
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/17/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTPROGRAMCONTROL
#define __ABSTRACTPROGRAMCONTROL

# include "csx_ext.h"
# include "generic_types.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

typedef enum program_control_flags_e {
    MONITOR = 1,
    RESTART_ON_EXIT = 2,
    PROCESS_RUNNING  = 3,
    ABORT_ON_UNEXPECTED_EXIT = 4,
} program_control_flags_t;

class AbstractProgramControl
{
public:
    virtual ~AbstractProgramControl() {}

    virtual const char *getCommand() = 0;


    virtual bool get_flag(UINT_32 flag) = 0;
    virtual void set_flag(UINT_32 flag, bool value) = 0;

    virtual UINT_32 get_flags() = 0;
    virtual void set_flags(UINT_32 flags) = 0;


    virtual void run() = 0;

    virtual void waitForProcessExit() = 0;

    SHMEMDLLPUBLIC static AbstractProgramControl *factory(const char *command,
                                                            UINT_32 startFlags = (1 << ABORT_ON_UNEXPECTED_EXIT),
                                                            UINT_64E restartdelaymilliseconds = 15*1000);
};
#endif //__ABSTRACTPROGRAMCONTROL
