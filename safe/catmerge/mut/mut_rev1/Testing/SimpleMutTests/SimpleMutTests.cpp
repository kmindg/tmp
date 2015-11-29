/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimpleMutTests.cpp
 ***************************************************************************
 *
 * DESCRIPTION:  Unit test suite containing simple to debug MUT tests
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/01/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
extern "C" {
# include "mut.h"
};
# include "mut_cpp.h"
# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"
# include "EmcUTIL_CsxShell_Interface.h"
#include "EmcPAL_UserBootstrap.h"
#include "EmcPAL_Misc.h"
#include "ktrace.h"

int j = 0;
static int DivideByZero() {
    int i=1;
    i = i/j;
    return i;
}

static void PerformHardAssert() {
    int i=1, j=2;
	MUT_ASSERT_INT_EQUAL(i,j);
    KvPrint("Hard Assert Hit");
}

static void setup(void) {
    KvPrint("Starting up the suite. Inside suiteStartup()");
}

static void teardown(void) {
    KvPrint("Tearing down the suite. Inside suiteTeardown()");
    //DivideByZero();
    //CSX_PANIC();
}

class SimpleMutBaseClass: public CAbstractTest
{
public:
    void StartUp()
    {
        mut_printf(MUT_LOG_HIGH, "In SimpleMutBaseClass::StartUp()");
    }
    void TearDown()
    {
        mut_printf(MUT_LOG_HIGH, "In SimpleMutBaseClass::TearDown()");
    }
};

class SimpleNoopMutTest: public SimpleMutBaseClass
{
public:
    void Test()
    {
        mut_printf(MUT_LOG_HIGH, "In SimpleNoopMutTest::Test()");
    }
};

/*
 * This test only works when run from within the debugger.
 *
 */
class TestMutExitMonitor: public CAbstractTest
{
public:
    CSX_VOLATILE BOOL exceptionEncountered;

    void Test()
    {
        exceptionEncountered = FALSE;
       CSX_TRY { 
            set_monitor_exit(NULL);
            exit(-1);
       }
       CSX_CATCH(TestMutExitMonitor_exceptionHandler, CSX_NULL) {
            exceptionEncountered = TRUE;
            mut_printf(MUT_LOG_LOW, "In __except block");
       } CSX_END_TRY_CATCH;
       mut_printf(MUT_LOG_LOW, "After __try block");

       MUT_ASSERT_TRUE_MSG(exceptionEncountered, "Int3 Exception was not encountered");
    }

    static csx_status_e TestMutExitMonitor_exceptionHandler(csx_rt_proc_exception_info_t *exception, csx_rt_proc_catch_filter_context_t context)
    {
        CSX_UNREFERENCED_PARAMETER(exception);
        CSX_UNREFERENCED_PARAMETER(context);
        mut_printf(MUT_LOG_LOW, "In TestMutExitMonitor_exceptionHandler");
        return CSX_STATUS_EXCEPTION_EXECUTE_HANDLER;
    }
};

class TestTimeout_Within: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess(void *context)
    {
		// Throw an exception
		KvPrint("A new thread was created. It will now wait for 5 sec. Test: will not timeout");
		csx_p_thr_sleep_secs(5);
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In TestTimeout_Within::Test()");
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "TestTimeout_Within_thread_timeout",
                            ThreadedProcess,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
    }
};

class TestTimeout_Equal: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess(void *context)
    {
		// Throw an exception
		KvPrint("A new thread was created. It will now wait for 10 sec (waiting time = timeout). Test: will not timeout");
		csx_p_thr_sleep_secs(10);
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In TestTimeout_Equal::Test()");
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "TestTimeout_Equal_thread_timeout",
                            ThreadedProcess,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
    }
};

class Test_IntAssert_Fail_InTest: public SimpleMutBaseClass
{
public:
    void Test()
    { 
		KvPrint("1. Inside Test_IntAssert_Fail_InTest:Test()");
		int a = 3, b = 4;
		MUT_ASSERT_INT_EQUAL(a,b);
		
		KvPrint("CONTROL SHOULD NOT REACH HERE");
		
	    KvPrint("2. Inside Test_IntAssert_Fail_InTest:Test()");
		a = 7, b = 8;
		MUT_ASSERT_INT_EQUAL(a,b);
    }
};


class Test_IntAssert_Fail_InThread: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess(void *context)
    {
		KvPrint("1. Inside Test_IntAssert_Fail_InThread:Test()");
		int a = 3, b = 4;
		MUT_ASSERT_INT_EQUAL(a,b);
		
		KvPrint("CONTROL SHOULD NOT REACH HERE");
		
	    KvPrint("2. Inside Test_IntAssert_Fail_InThread:Test()");
		a = 7, b = 8;
		MUT_ASSERT_INT_EQUAL(a,b);
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_IntAssert_Fail_InThread::Test()");
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_IntAssert_Fail_InThread",
                            ThreadedProcess,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
    }
};


class TestTimeout_Teardown: public SimpleMutBaseClass
{
public:
    void Test()
    { 

        KvPrint("In TestTimeout::Test()");
		
    }
    void TearDown()
    {
        KvPrint("In TestTimeout_Teardown::TearDown()");
		csx_p_thr_sleep_secs(20000);
    }

};


class Test_Invoking_Timeout: public SimpleMutBaseClass
{
public:
    void Test()
    { 
        KvPrint("In Test_Invoking_Timeout:Test()");
		csx_p_thr_sleep_secs(500000);
    }

};

class Test_Thread_Invoking_Timeout: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess(void *context)
    {
		// Throw an exception
		KvPrint("Test_Thread_Invoking_Timeout::Test()");
		csx_p_thr_sleep_secs(500000);
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_Thread_Invoking_Timeout::Test()");
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_Thread_Invoking_Timeout",
                            ThreadedProcess,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
    }
};


class Test_Raising_Exception: public SimpleMutBaseClass
{
public:
    void Test()
    { 
        KvPrint("In Test_Raising_Exception:Test()");
		EmcpalDebugBreakPoint();
    }

};

class Test_Thread_Raising_Exception: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess(void *context) {
		// Throw an exception
		KvPrint("Test_Thread_Raising_Exception::ThreadedProcess()");
		EmcpalDebugBreakPoint();
	}

    void Test() { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_Thread_Raising_Exception::Test()");
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_Thread_Raising_Exception",
                            ThreadedProcess,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
    }
};

class Test_Threads_Attempting_Abort: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess1(void *context) {
		int i=0, j=1;
		KvPrint("Test_Threads_Attempting_Abort::ThreadedProcess_1()");
		MUT_ASSERT_INT_EQUAL(i,j); // This will fail
	}
	
	// Thread proc
    static void ThreadedProcess2(void *context) {
		int i = 0, j = 2;
		KvPrint("Test_Threads_Attempting_Abort::ThreadedProcess_2()");
		MUT_ASSERT_INT_EQUAL(i,j); // This will fail
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);
		PEMCPAL_THREAD thread2 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_Threads_Attempting_Abort::Test()");
		
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread2,
                            "Test_Threads_Attempting_Abort",
                            ThreadedProcess2,
                            NULL);  
		
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_Threads_Attempting_Abort",
                            ThreadedProcess1,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
		EmcpalThreadWait(thread2);
    }
};

class Test_Threads_Attempting_AbortOnTimeout: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess1(void *context) {
		// Throw an exception
		KvPrint("Test_Threads_Attempting_Abort::ThreadedProcess_1()::Sleeping Forever");
		csx_p_thr_sleep_secs(500000);
	}
	
	// Thread proc
    static void ThreadedProcess2(void *context) {
		// Throw an exception
		KvPrint("Test_Threads_Attempting_Abort::ThreadedProcess_2()::Sleeping Forever");
		csx_p_thr_sleep_secs(500000);
	}

    void Test()
    { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);
		PEMCPAL_THREAD thread2 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_Threads_Attempting_Abort::Test()");
		
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread2,
                            "Test_Threads_Attempting_Abort",
                            ThreadedProcess2,
                            NULL);  
		
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_Threads_Attempting_Abort",
                            ThreadedProcess1,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
		EmcpalThreadWait(thread2);
    }
};

class Test_Threads_Attempting_ToRaiseEception: public SimpleMutBaseClass
{
public:
	EMCPAL_THREAD mThread_Control;

		// Thread proc
    static void ThreadedProcess1(void *context) {
		EmcpalDebugBreakPoint();
	}
	
	// Thread proc
    static void ThreadedProcess2(void *context) {
		EmcpalDebugBreakPoint();
	}

    void Test() { 
		PEMCPAL_THREAD thread1 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);
		PEMCPAL_THREAD thread2 = (PEMCPAL_THREAD) (new char[sizeof(EMCPAL_THREAD)]);

        KvPrint("In Test_Threads_Attempting_ToRaiseEception::Test()");
		
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread2,
                            "Test_Threads_Attempting_ToRaiseEception",
                            ThreadedProcess2,
                            NULL);  
		
		// Create a thread. Wait more than timeout
		EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            thread1,
                            "Test_Threads_Attempting_ToRaiseEception",
                            ThreadedProcess1,
                            NULL);   // no context
		
		// Wait for it to complete
		EmcpalThreadWait(thread1);
		EmcpalThreadWait(thread2);
    }
};


class TestExceptionInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestExceptionInStartUpClass::StartUp()"); csx_p_thr_sleep_secs(2);
        csx_rt_proc_raise_exception(CSX_TRUE);
    }
};

class TestExceptionInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestExceptionInTestClass::Test()"); csx_p_thr_sleep_secs(2); 
        csx_rt_proc_raise_exception(CSX_TRUE);
    }
};

class TestExceptionInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestExceptionInTearDownClass::TearDown()"); csx_p_thr_sleep_secs(2); 
        csx_rt_proc_raise_exception(CSX_TRUE);
    }
};

class TestExceptionInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); csx_p_thr_sleep_secs(2); 
        csx_rt_proc_raise_exception(CSX_TRUE);
	}

    void Test() {
		KvPrint("In TestExceptionInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

class TestBreakPointInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestBreakPointInStartUpClass::StartUp()"); csx_p_thr_sleep_secs(2);
        EmcpalDebugBreakPoint();	
    }
};

class TestBreakPointInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestBreakPointInTestClass::Test()"); csx_p_thr_sleep_secs(2); 
        EmcpalDebugBreakPoint();	
    }
};

class TestBreakPointInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestExceptionInTearDownClass::TearDown()"); csx_p_thr_sleep_secs(2); 
        EmcpalDebugBreakPoint();	
    }
};

class TestBreakPointInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); csx_p_thr_sleep_secs(2); 
        EmcpalDebugBreakPoint();	
	}

    void Test() {
		KvPrint("In TestExceptionInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

class TestDivideByZeroInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestDivideByZeroInStartUpClass::StartUp()"); csx_p_thr_sleep_secs(2);
        DivideByZero();
        KvPrint("This should not execute");
    }
};

class TestDivideByZeroInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestDivideByZeronInTestClass::Test()"); csx_p_thr_sleep_secs(2); 
        DivideByZero();
        KvPrint("This should not execute");
    }
};

class TestDivideByZeroInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestDivideByZeroInTearDownClass::TearDown()"); csx_p_thr_sleep_secs(2); 
        DivideByZero();
        KvPrint("This should not execute");
    }
};

class TestDivideByZeroInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); csx_p_thr_sleep_secs(2); 
        DivideByZero();
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestDivideByZeroInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

class TestMutHardAssertInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestMutHardAssertInStartUpClass::StartUp()"); csx_p_thr_sleep_secs(2);
        PerformHardAssert();
        KvPrint("This should not execute");
    }
};

class TestMutHardAssertInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestMutHardAssertInTestClass::Test()"); csx_p_thr_sleep_secs(2); 
        PerformHardAssert();
        KvPrint("This should not execute");
    }
};

class TestMutHardAssertInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestMutHardAssertInTearDownClass::TearDown()"); csx_p_thr_sleep_secs(2); 
        PerformHardAssert();
        KvPrint("This should not execute");
    }
};

class TestMutHardAssertInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); csx_p_thr_sleep_secs(2); 
        PerformHardAssert();
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestMutHardAssertInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

// Timeout tests
class TestTimeoutInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestTimeoutInStartUpClass::StartUp()"); 
        csx_p_thr_sleep_secs(15);
        KvPrint("This should not execute");
    }
};

class TestTimeoutInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestTimeoutInTestClass::Test()"); 
        csx_p_thr_sleep_secs(15);
        KvPrint("This should not execute");
    }
};

class TestTimeoutInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestTimeoutInTearDownClass::TearDown()"); 
        csx_p_thr_sleep_secs(15);
        KvPrint("This should not execute");
    }
};

class TestTimeoutInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); 
        csx_p_thr_sleep_secs(15);
        KvPrint("This should not execute");
        KvPrint("and This should not execute");
        KvPrint("This should not execute");
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestTimeoutInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

// This test will do hard assert in test and in a thread created by the test
class TestMutHardAssertInTestAndThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); 
        PerformHardAssert(); 
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestMutHardAssertInTestAndThreadClass::Test()");

        // Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
        PerformHardAssert(); 
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

// This test will do hard assert in test and in a thread created by the test
class TestMutHardAssertInTestWithFiveThreadsClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control1, mThread_Control2, mThread_Control3, mThread_Control4, mThread_Control5;	

	// Thread proc
    static void ThreadProc1(void *context) {
		KvPrint("Inside ThreadProc1"); csx_p_thr_sleep_secs(2); 
	}
    static void ThreadProc2(void *context) {
		KvPrint("Inside ThreadProc2"); csx_p_thr_sleep_secs(2); 
	}
    static void ThreadProc3(void *context) {
		KvPrint("Inside ThreadProc3"); csx_p_thr_sleep_secs(2); 
	}
    static void ThreadProc4(void *context) {
		KvPrint("Inside ThreadProc4"); csx_p_thr_sleep_secs(2); 
	}
    static void ThreadProc5(void *context) {
		KvPrint("Inside ThreadProc5"); csx_p_thr_sleep_secs(2); 
	}

    void Test() {
		KvPrint("In TestMutHardAssertInTestAndThreadClass::Test()");

        // Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control1, "Exception_thread1", ThreadProc1, NULL);   // no context
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control2, "Exception_thread2", ThreadProc2, NULL);   // no context
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control3, "Exception_thread3", ThreadProc3, NULL);   // no context
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control4, "Exception_thread4", ThreadProc4, NULL);   // no context
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control5, "Exception_thread5", ThreadProc5, NULL);   // no context
        csx_p_thr_sleep_secs(2); 
        PerformHardAssert(); 
		// EmcpalThreadWait(&mThread_Control); //DO NOT WAIT, see what happens
    }
};


// Timeout tests
class TestCsxPanicInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestCsxPanicInStartUpClass::StartUp()"); 
        CSX_PANIC(); 
        KvPrint("This should not execute");
    }
};

class TestCsxPanicInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestCsxPanicInTestClass::Test()"); 
        CSX_PANIC(); 
        KvPrint("This should not execute");
    }
};

class TestCsxPanicInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestCsxPanicInTearDownClass::TearDown()"); 
        CSX_PANIC(); 
        KvPrint("This should not execute");
    }
};

class TestCsxPanicInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); 
        CSX_PANIC(); 
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestCsxPanicInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

// Timeout tests
class TestCsxHardAssertInStartUpClass: public SimpleNoopMutTest
{
public:
    void StartUp() {
		KvPrint("In TestCsxHardAssertInStartUpClass::StartUp()"); 
        CSX_ASSERT_H_RDC(CSX_FALSE);
        KvPrint("This should not execute");
    }
};

class TestCsxHardAssertInTestClass: public SimpleNoopMutTest
{
public:
    void Test() {
		KvPrint("In TestCsxHardAssertInTestClass::Test()"); 
        CSX_ASSERT_H_RDC(CSX_FALSE);
        KvPrint("This should not execute");
    }
};

class TestCsxHardAssertInTearDownClass: public SimpleNoopMutTest
{
public:
    void TearDown() {
		KvPrint("In TestCsxHardAssertInTearDownClass::TearDown()"); 
        CSX_ASSERT_H_RDC(CSX_FALSE);
        KvPrint("This should not execute");
    }
};

class TestCsxHardAssertInThreadClass: public SimpleNoopMutTest
{
public:
	EMCPAL_THREAD mThread_Control;	

	// Thread proc
    static void ThreadProc(void *context) {
		KvPrint("Throwing exception in separate thread"); 
        CSX_ASSERT_H_RDC(CSX_FALSE);
        KvPrint("This should not execute");
	}

    void Test() {
		KvPrint("In TestCsxHardAssertInThreadClass::Test()");
		
		// Start up another thread
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mThread_Control, "Exception_thread", ThreadProc, NULL);   // no context
		EmcpalThreadWait(&mThread_Control); // Wait for it to complete
    }
};

int __cdecl main(int argc , char ** argv)
{
    mut_testsuite_t * CSX_MAYBE_UNUSED test;
    mut_testsuite_t * exception_handling_tests;

    Emcutil_Shell_MainApp theApp;
    mut_init(argc, argv);

    test = mut_create_testsuite("SimpleMutTests");
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, TestTimeout_Within, 10);
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, TestTimeout_Equal, 10);
    //MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Invoking_Timeout, 10);
    //MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Thread_Invoking_Timeout, 10);
    //MUT_ADD_TEST_CLASS_TIMEOUT(test, TestTimeout_Teardown, 10);
	
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_IntAssert_Fail_InTest, 600);
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_IntAssert_Fail_InThread, 600);
	
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Raising_Exception, 600);
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Thread_Raising_Exception, 600);
    
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Threads_Attempting_Abort, 600);
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Threads_Attempting_AbortOnTimeout, 10);
	//MUT_ADD_TEST_CLASS_TIMEOUT(test, Test_Threads_Attempting_ToRaiseEception, 600);

	
    // ---------------------------------------------------- 
    //MUT_ADD_TEST_CLASS(test, SimpleNoopMutTest);
    
    // These tests will be used to verify the exception handling behavior
    //exception_handling_tests = mut_create_testsuite("ExceptionHandling_Suite");
    exception_handling_tests = mut_create_testsuite_advanced("ExceptionHandling_Suite", setup, teardown);

    // Tests that will raise and exception and behavior of each test will have to be seen with and without -autotestmode

    /* These tests are manual version of the autoTestMode tests in mut_selftest.
	 They are intended to be run with and without the -autoTestMode command line option.
	 If run without they will trap into the debugger, if with it, then the suite will exit
	 right after the exception, cleaning up on the way out (closing down the log, marking test(s)
	 passed, failed, and not tested). */
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestExceptionInStartUpClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestExceptionInTestClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestExceptionInTearDownClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestExceptionInThreadClass, 60);
	

    // Test invoking EmcpalDebugBreakPoint()	
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestBreakPointInStartUpClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestBreakPointInTestClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestBreakPointInTearDownClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestBreakPointInThreadClass, 60);

    // Tests invoking Divide by 0
   	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestDivideByZeroInStartUpClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestDivideByZeroInTestClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestDivideByZeroInTearDownClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestDivideByZeroInThreadClass, 60);

    // Tests invoking MUT_ASSERT
   	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInStartUpClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInTestClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInTearDownClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInThreadClass, 60);
	
	// Tests invoking Timeouts 
   	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestTimeoutInStartUpClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestTimeoutInTestClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestTimeoutInTearDownClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestTimeoutInThreadClass, 10);
	

    // Tests invoking a hard assert in test as well as thread.
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInTestAndThreadClass, 60);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestMutHardAssertInTestWithFiveThreadsClass, 60);
     

  	// Tests invoking CSX_PANIC 
   	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxPanicInStartUpClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxPanicInTestClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxPanicInTearDownClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxPanicInThreadClass, 10);
 
  	// Tests invoking CSX hard assert
   	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxHardAssertInStartUpClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxHardAssertInTestClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxHardAssertInTearDownClass, 10);
	MUT_ADD_TEST_CLASS_TIMEOUT(exception_handling_tests, TestCsxHardAssertInThreadClass, 10);

    // Now, series of tests that cause Exception in AbortHandler/UnhandledExceptionHandler code
    
    /*
     * Test launches debugger, can not be part of automation
     * as it currently is hanging in CSX, after the main routine exits
     */
    //MUT_ADD_TEST_CLASS(test, TestMutExitMonitor);

    //MUT_RUN_TESTSUITE(test);
    MUT_RUN_TESTSUITE(exception_handling_tests);
    //EmcpalDebugBreakPoint();

    exit(0);
}

