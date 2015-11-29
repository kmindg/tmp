/***************************************************************************
 * Copyright (C) EMC Corporation 2012-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DefaultSPControl.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the DefaultSPControl class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    05/12/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __DEFAULTSPCONTROL__
#define __DEFAULTSPCONTROL__

# include "simulation/AbstractSPControl.h"
# include "simulation/ArrayDirector.h"

class DefaultSPControl: public AbstractSPControl {
public:
    DefaultSPControl();
    ~DefaultSPControl();

    void Configure_SP(SPIdentity_t sp);
    void Boot_SP(SPIdentity_t sp);
    void Dump_SP(SPIdentity_t sp, char* dumpApplication, char* cmdLine);
    void Restore_SP(SPIdentity_t sp, char* restoreApplication, char* cmdLine);
    void Shutdown_SP(SPIdentity_t sp);
    void Halt_SP(SPIdentity_t sp);
    void ValidateSPNotRunning(SPIdentity_t sp);

    virtual const char  *construct_command(SPIdentity_t sp = SPA);
    virtual const char  *construct_alternate_command(SPIdentity_t sp = SPA, char* app = "", char* cmdLine = "");
    virtual const char  *construct_loadLibaryPath(SPIdentity_t sp = SPA);
    virtual Options     *getCommandLineOptions(SPIdentity_t sp = SPA);
    virtual void        addCommandLineOptions(Options *options, SPIdentity_t sp = SPA);
    virtual const char  *getSPProgram(SPIdentity_t sp);
    virtual void setSPProgram(const char* szMySPprogram);
    virtual void setProgramPath(SPIdentity_t sp, const char* szMyProgramPath);
    virtual const char  *getProgramPath(SPIdentity_t sp);
    virtual void addNewCommandLineOption(SPIdentity_t sp, Program_Option *option);
    virtual void resetCommandLine(SPIdentity_t sp);

private:
    const char                  *mCommandLine[2];
    Options                     *mCommandLineOptions[2];
    const char                  *mAltCommandLine[2];
    Options                     *mAltCommandLineOptions[2];
    ArrayDirector               *mSimDirector;

    //cache test share the same program with sp, but mcr test not 
    char mszSPprogram[256];
    //path of program to run if needed, sp path or debugger path
    char mszProgramPath[2][512];
    //mszProgramPath+program name
    char mszTotalPathName[2][1024];

    // Set to true when ArrayDirector flags initially set to 0
    bool mInitializedFlags;
};

#endif
