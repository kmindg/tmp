/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_abstract_test_runner.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT test runner interface
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/
#ifndef __MUTABSTRACTTESTRUNNER__
#define  __MUTABSTRACTTESTRUNNER__

# include "mut_test_entry.h"

class Mut_Test_entry;

class Mut_Runnable {
public:
    virtual ~Mut_Runnable() {}
    virtual const char *getName() = 0;
    virtual void run() = 0;
};

class Mut_abstract_test_runner: public Mut_Runnable {
public:
    virtual ~Mut_abstract_test_runner() {}
    virtual void setTestStatus(enum Mut_test_status_e status) = 0;
    virtual Mut_test_entry *getTest() = 0; // So that the current_runner can query for current test
};

#endif
