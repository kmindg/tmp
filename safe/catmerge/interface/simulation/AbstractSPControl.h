/***************************************************************************
 * Copyright (C) EMC Corporation 2021
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractSPControl.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Interface AbstractSPControl
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    05/12/2012  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTSPCONTROL__
#define __ABSTRACTSPCONTROL__

# include "simulation/Program_Option.h"


typedef enum SPIdentity_e {
    SPA = 0,
    SPB = 1
}SPIdentity_t;

class AbstractSPControl {
public:

    virtual ~AbstractSPControl() {}

    virtual void Configure_SP(SPIdentity_t sp) = 0;
    virtual void Boot_SP(SPIdentity_t sp) = 0;
    virtual void Dump_SP(SPIdentity_t sp, char* dumpApplication, char* cmdLine) = 0;
    virtual void Restore_SP(SPIdentity_t sp, char* restoreApplication, char* cmdLine) = 0;
    virtual void Shutdown_SP(SPIdentity_t sp) = 0;
    virtual void Halt_SP(SPIdentity_t sp) = 0;

    virtual const char  *construct_command(SPIdentity_t sp = SPA) = 0;
    virtual const char  *construct_alternate_command(SPIdentity_t sp = SPA, char* app = "", char* cmdLine = "") = 0;
    virtual Options     *getCommandLineOptions(SPIdentity_t sp = SPA) = 0;
    virtual void        addCommandLineOptions(Options *options, SPIdentity_t sp = SPA) = 0;
    virtual const char  *getSPProgram(SPIdentity_t sp = SPA) = 0;
};

#endif
