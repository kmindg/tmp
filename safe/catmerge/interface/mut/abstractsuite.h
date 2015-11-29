/*
 * abstractsuite.h
 *
 *  Created on: Jan 12, 2012
 *      Author: londhn
 */
 
#ifndef ABSTRACTSUITE_H
#define ABSTRACTSUITE_H
  
#include "mut/executor.h"
 
class AbstractSuite:public Executor{
	virtual int execute()=0;
	virtual void populate()=0;
	virtual void addTest(DefaultTest *test, unsigned long timeout = 0)=0;
	virtual void startUp()=0;
	virtual void tearDown()=0;
};

#endif