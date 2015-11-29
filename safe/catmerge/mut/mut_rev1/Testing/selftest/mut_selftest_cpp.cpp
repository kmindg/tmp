/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_selftest_cpp.cpp
 ***************************************************************************
 *
 * DESCRIPTION:  MUT Self Test C++ part
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2007   Igor Bobrovnikov (bobroi) Initial version
 *
 **************************************************************************/

#define I_AM_NATIVE_CODE
//#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
//#include <conio.h>
#include <time.h>
#include <string.h>

#include "mut_cpp.h"
#include "mut_test_control.h"
extern MUT_API Mut_test_control *mut_control;

extern "C" {
    #include "mut_selftest_tools.h"
}

class CCppTest: public CAbstractTest {

   
    virtual void Test()
    {
        log_test_begin("CCppTest");
        LOG_TEST_ASSERT_MSG(
            "Assert %s", "MUT_FAIL_MSG",
            3/*A*/)
        log_test_end("CCppTest", LOG_STATUS_FAILED);

        MUT_FAIL_MSG(EXPECTED_FMT) /*A*/
        return;
    }
};

extern "C" void _stdcall mut_selftest_cpp_add_tests(mut_testsuite_t * root)
{
    MUT_ADD_TEST_CLASS(root, CCppTest)
}
