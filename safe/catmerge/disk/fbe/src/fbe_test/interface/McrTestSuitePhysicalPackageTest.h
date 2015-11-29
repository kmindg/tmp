//this is for MUT v1.0

#ifndef MCR_TEST_SUITE_PHYSICAL_PACKAGE_TEST_H
#define MCR_TEST_SUITE_PHYSICAL_PACKAGE_TEST_H
#include "mut_cpp.h"



class McrTestSuitePhysicalPackage
{
	public:

    McrTestSuitePhysicalPackage(void (*startFun)() = NULL , void (*tearFun)() = NULL );

    virtual ~McrTestSuitePhysicalPackage();

    void execute(); 

private:

    mut_testsuite_t *mSuite;
	const char* mSuiteName;
	void(* mStartup)();
	void(* mTeardown)();

    void populate();
};
	
#endif//end FBE_TEST_SUITE_PHYSICAL_PACKAGE_TEST_H

