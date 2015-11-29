/*
 * mut_thread_test_runner.h
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */

#ifndef MUT_THREAD_TEST_RUNNER_H_
#define MUT_THREAD_TEST_RUNNER_H_

#include "mut_abstract_test_runner.h"

#include "EmcPAL.h"

class Mut_test_control;
class DefaultSuite;
class DefaultTest;

class Mut_thread_test_runner: public Mut_abstract_test_runner{
public:
    Mut_thread_test_runner(Mut_test_control *mut_control,
                            DefaultSuite *current_suite,
							DefaultTest *current_test);

    const char *getName();

    int run();

    virtual void setTestStatus(enum Mut_test_status_e status);

    Mut_test_control *getControl();
    DefaultSuite    *getSuite();
    DefaultTest   *getTest();

private:
    Mut_test_control    *mControl;
    DefaultSuite       *mSuite;
    DefaultTest      *mTest;		
	EMCPAL_MONOSTABLE_TIMER mTimer;	
	static void mutCleanUpAndExit(EMCPAL_TIMER_CONTEXT context);
};


#endif /* MUT_THREAD_TEST_RUNNER_H_ */
