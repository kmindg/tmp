/*
 * mut_abstract_test_runner.h
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */

#ifndef MUT_ABSTRACT_TEST_RUNNER_H_
#define MUT_ABSTRACT_TEST_RUNNER_H_

class Mut_Runnable{
public:
	virtual ~Mut_Runnable() {}
	virtual const char *getName() = 0;
	virtual int run() = 0;
};

class Mut_abstract_test_runner: public Mut_Runnable {
public:
    virtual ~Mut_abstract_test_runner() {}
    virtual void setTestStatus(enum Mut_test_status_e status) = 0;
};
#endif /* MUT_ABSTRACT_TEST_RUNNER_H_ */
