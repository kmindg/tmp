/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               InterlockExchangeSpinLock.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaratione of a SpinLock
 *                  that uses InterlockEchange operations
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/03/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
# include "generic_types.h"

class InterlockExchangeSpinLock {
public:
    InterlockExchangeSpinLock();

    void lock();
    void unlock();
private:
     volatile LONG mLock;
};
