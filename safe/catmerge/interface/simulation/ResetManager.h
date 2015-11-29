/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ResetManager.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of a manager of a list of Resetable instances
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/31/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __RESETMANAGER__
#define __RESETMANAGER__

# include "generic_types.h"
# include "simulation/AbstractResetable.h"

class ResetManager: AbstractResetable {
public:
    ResetManager();
    ResetManager(char *name);
    ~ResetManager();

    void setName(char *name);
    const char *getName();
    virtual void reset();

    void addReset(AbstractResetable *reset);
    void removeReset(AbstractResetable *reset);

    AbstractResetable *findReset(char *name);
    void listResets();

private:
    static void processReset(ResetManager *manager, AbstractResetable *reset);
    char *mName;
    AbstractResetable mListOfResets;
};

#endif
