/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractArrayManager.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions that defines an interface to 
 *               start/stop an Array.
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

/*
 * An ArrayManager.
 */
class AbstractArrayManager {
public:
    virtual ~AbstractArrayManager() {}

    /*
     * Used to Configure & Boot the system that will be tested
     * Most frequently used during Suite or Test Setup()
     */
    virtual void BootSystem() = 0;

    
    /*
     * Used to shut down system after testing completes
     * Most frequently used during Suite or Test teardown()
     */
    virtual void ShutdownSystem() = 0;

    /*
     * Uset to halt system
     * Most frequently uused during Suite or Test teardown()
     */
    virtual void HaltSystem() = 0;
};
