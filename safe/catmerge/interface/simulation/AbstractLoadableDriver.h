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
# include  "BasicLib/ConfigDatabase.h"

/*
 * A Service.  A service is a component of a Driver.
 * When creating simple unit tests, wrap the code to be tested into
 * an AbstractService.
 */
class AbstractService {
public:
    virtual ~AbstractService() {}
    /*
     * Will be used to start code 
     */
    virtual BOOL ServiceStart();

    /*
     * Will be used to stop code
     */
    virtual void ServiceStop();
};

/*
 * A Loadable Driver. This interface provides a means to control
 * a driver within any available test environemt, a statically linked executable, 
 * a CSX mdoule, shared library running in one or more processes running on
 * one or more workstations.
 * 
 */
class AbstractLoadableDriver {
public:

    /*
     * Will be used to start Driver
     */
    virtual BOOL    DriverLoad(DatabaseKey * rootKey, SP_ID spid, char* driverName);

    /*
     * Will be used to stop Driver
     */
    virtual VOID    DriverUnload();
};
