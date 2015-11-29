/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractTimer.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions for a Timer
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    05/20/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _ABSTRACTTIMER_
#define _ABSTRACTTIMER_

class AbstractTimer {
public:
    /*
     * Destructor for Timer
     */
    virtual ~AbstractTimer() {};

    /*
     * Method start() resets timer.
     * After a specified timer, timer performs an action
     */
    virtual void start() = 0;

    /*
     * Method stop() disables timer countdown
     */
    virtual void stop() = 0;
};
#endif // _ABSTRACTTIMER_
