/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimUtilitiesTest.cpp
 ***************************************************************************
 *
 * DESCRIPTION:  Unit Tests for the simUtilities library
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/15/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#define I_AM_NATIVE_CODE
# include <stdio.h>
# include <stdlib.h>
# include <windows.h>
# include <vector>

# include "generic_types.h"
# include "mut.h"
# include "mut_cpp.h"
# include "simulation/shmem_class.h"
# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"
# include "simulation/BvdSimMutProgramBaseClass.h"

#define CALL_FIRST 1
#define CALL_LAST 0

extern void shmem_class_add_tests(mut_testsuite_t * suite);
extern void simDirector_add_tests(mut_testsuite_t * suite);

extern void simDirector_launch(int argc , char ** argv);

class ExceptionProcessingBaseClass;  // forward reference
ExceptionProcessingBaseClass *instance;

class ExceptionProcessingBaseClass: public CAbstractTest
{
public:
    BOOL encounteredException_in_VectoredExceptionHandler;
    BOOL encounteredException_in_ThreadExceptionHandler;
    BOOL encounteredException_in_ThreadExceptionFilter;
    BOOL encounteredException_in_UnhandledExceptionHandler;

    PVOID VectoredExceptionHandlerPtr;
    EMCPAL_THREAD mThread_Control;

    void StartUp()
    {
        VectoredExceptionHandlerPtr = NULL;
        encounteredException_in_VectoredExceptionHandler = FALSE;
        encounteredException_in_ThreadExceptionHandler = FALSE;
        encounteredException_in_UnhandledExceptionHandler = FALSE;
        encounteredException_in_ThreadExceptionFilter = FALSE;

        instance = this;
    }

    void TearDown()
    {
        if(VectoredExceptionHandlerPtr != NULL)
        {
            csx_rt_proc_unregister_early_exception_callback(VectoredExceptionHandlerPtr);
        }
        VectoredExceptionHandlerPtr = NULL;
    }

    static csx_status_e VectoredExceptionHandler_routine(csx_rt_proc_exception_info_t *exceptionInfo)
    {
        mut_printf(MUT_LOG_LOW, "VectoredExceptionHandler_routine encountered exception\n");
        instance->encounteredException_in_VectoredExceptionHandler = TRUE;

        /*
         * returning continue search should pass control to thread exception handlers
         */
        return CSX_STATUS_EXCEPTION_CONTINUE_SEARCH;
    }

    static csx_status_e UnhandledExceptionHandler_routine(csx_rt_proc_exception_info_t *exceptionInfo)
    {
        mut_printf(MUT_LOG_LOW, "UnhandledExceptionHandler_routine encountered exception\n");
        instance->encounteredException_in_UnhandledExceptionHandler = TRUE;

        /*
         * returning continue search should launch the debugger
         */
        return CSX_STATUS_EXCEPTION_CONTINUE_SEARCH;
    }

    void setupVectoredExceptionHandler(int FirstHandler)
    {
        VectoredExceptionHandlerPtr = csx_rt_proc_register_early_exception_callback(VectoredExceptionHandler_routine, FirstHandler);
    }

    void setupUnhandledExceptionHandler()
    {
        csx_rt_proc_set_unhandled_exception_callback(UnhandledExceptionHandler_routine);
    }

    static csx_status_e exception_filter(csx_rt_proc_exception_info_t *exception, csx_rt_proc_catch_filter_context_t context)
    {
        CSX_UNREFERENCED_PARAMETER(context);
        mut_printf(MUT_LOG_LOW, "The thread exception_filter processed exception\n");
        instance->encounteredException_in_ThreadExceptionFilter = TRUE;
        return CSX_STATUS_EXCEPTION_EXECUTE_HANDLER;  // pass control to the __exception block
    }

    static void ThreadControl(void *context)
    {
        CSX_TRY 
        {
            int x = 5;
            int y = 0;
            int CSX_MAYBE_UNUSED k = x/y;
        }
        CSX_CATCH(exception_filter, CSX_NULL)
        {
            mut_printf(MUT_LOG_LOW, "The thread exception block was executed\n");
            instance->encounteredException_in_ThreadExceptionHandler = TRUE;
        } CSX_END_TRY_CATCH;

    }

    void launchThreadThatThrowsException()
    {
        EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(),
                            &mThread_Control,
                            "Exception_threaed",
                            ThreadControl,
                            NULL);   // no context
        EmcpalThreadWait(&mThread_Control);
        EmcpalThreadDestroy(&mThread_Control);
    }
};

class VectoredException_FirstHandler_Test: public ExceptionProcessingBaseClass
{
public:

    void Test()
    {
        setupVectoredExceptionHandler(CALL_FIRST);
        launchThreadThatThrowsException();
    }
};

class VectoredException_LastHandler_Test: public ExceptionProcessingBaseClass
{
public:

    void Test()
    {
        setupVectoredExceptionHandler(CALL_LAST);
        launchThreadThatThrowsException();
    }
};

class ThreadBasedExceptionHandler_Test: public ExceptionProcessingBaseClass
{
public:

    void Test()
    {
        launchThreadThatThrowsException();
    }
};


class UnhandledException_Test: public ExceptionProcessingBaseClass
{
public:

    void Test()
    {
        setupUnhandledExceptionHandler();
        launchThreadThatThrowsException();
    }
};


int __cdecl main(int argc , char ** argv)
{
    Emcutil_Shell_MainApp csxProgramControl(MY_CONTAINER_ARGS, GenerateMY_RT_ARGS(argc, argv));
    mut_testsuite_t * suite;

    /*
     * Perform standard MUT suite invokation
     */
    mut_init(argc, argv); 
    
    suite = mut_create_testsuite("MicrosoftExceptionHandlerTestSuite");

    MUT_ADD_TEST_CLASS(suite, ThreadBasedExceptionHandler_Test);
    MUT_ADD_TEST_CLASS(suite, VectoredException_LastHandler_Test);
    MUT_ADD_TEST_CLASS(suite, VectoredException_FirstHandler_Test);
    MUT_ADD_TEST_CLASS(suite, UnhandledException_Test);

    MUT_RUN_TESTSUITE(suite);
    CSX_EXIT(0);
}
