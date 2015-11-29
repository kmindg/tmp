/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractArrayDirector.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions that define Testable Systems
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/19/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
# include "generic_types.h"
# include "BasicLib/ConfigDatabase.h"
# include "AbstractLoadableDriver.h"

/*
 * An object that can configure drivers into a bootable array
 */
class AbstractArrayConfigurator {
public:
    virtual ~AbstractArrayConfigurator() {}

    /*
     * Used to register drivers to be tested as a system
     * Most frequently used during Test Setup()
     */
    virtual void registerDriver(AbstractLoadableDriver *driver) = 0;
};
