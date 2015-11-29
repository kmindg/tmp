/*
 * abstracttest.h
 *
 *  Created on: Jan 12, 2012
 *      Author: londhn
 */

#ifndef ABSTRACTTEST_H
#define ABSTRACTTEST_H

#include "mut/executor.h"
class AbstractTest: public Executor{
public:
    //! The main test function.
    /*!
      It specifies the test algoritm
      This function shall be overriden for each new test.
    */
    virtual void test() = 0;

    /*!
      This function may be overriden if necessary.
      It specifies the startup logic of the test algorithm.
      Main test function will not be called if startup failed.
    */
    virtual void startUp()=0;

    /*!
      This function may be overriden if necessary.
      It specifies the teardown logic of the test algorithm.
      This function will be called even if main test function failed
    */
    virtual void tearDown() = 0;

    //! A test destructor
    /*!
      This function may be overriden if necessary
    */
    virtual ~AbstractTest()
    {
    };

    virtual int execute()=0;
};

#endif /* ABSTRACT_TEST_H */