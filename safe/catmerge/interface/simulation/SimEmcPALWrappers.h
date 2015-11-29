#ifndef _SIM_EMC_PAL_WRAPPERS_H
#define _SIM_EMC_PAL_WRAPPERS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimEmcPalWrappers.h
 ***************************************************************************
 *
 * DESCRIPTION:  Convenience wrappers to Emcpal library functions
 *               Usage of these methods do not need to include EmcPAL.h
 *
 * FUNCTIONS: SimThreadSleep
 *
 * NOTES:
 *
 * HISTORY:
 *    10/27/2009  Martin Buckley Initial Version
 *
 **************************************************************************/

# include "generic_types.h"
# include "AbstractProgramControl.h"

extern "C" void SimThreadSleep(UINT_64E millisecondWaitTime);
 

class SimEmcPALProgramControl: public AbstractProgramControl
{
public:
    SimEmcPALProgramControl(const char *command,
                            UINT_32 startflags = 0,
                            UINT_64E restartdelaymilliseconds = 15*1000);
    ~SimEmcPALProgramControl();

    const char *getCommand();

    UINT_32 get_startflags();
    void set_startflags(UINT_32 startflags);

    virtual bool get_flag(UINT_32 flag);
    virtual void set_flag(UINT_32 flag, bool value);

    UINT_32 get_flags();
    void set_flags(UINT_32 flags);


    void run();

    void waitForProcessExit();

private:
    void runProgram();
    static void programLaunch(PVOID context);

    const char          *mCommand;
    PVOID               mThreadID;
    UINT_32             mStartFlags;
    volatile UINT_32    mFlags;
    volatile UINT_32    mInvocation;
    UINT_64E            mStartTime;
    UINT_64E            mEndTime;
    UINT_64E            mRestartDelayMilliseconds;
};

#endif /* _SIM_EMC_PAL_WRAPPERS_H */
